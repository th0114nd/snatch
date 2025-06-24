#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include <vector>

#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <folly/MPMCQueue.h>
#include <folly/String.h>
#include <folly/dynamic.h>
#include <folly/fbstring.h>
#include <folly/json.h>

#include "id3/tag.h"
#include "id3/misc_support.h"

#include <Python/Python.h>

using std::cout;
using std::endl;
using folly::fbstring;

// Data with the id in the URL for the track on youtube.com
// and song metadata
struct track_loc {
    fbstring id;
    fbstring artist;
    fbstring title;
};

bool is_cmus_running() {
    return not std::system("cmus-remote -Q >/dev/null 2>/dev/null");
}

// Make a temporary directory to store downloaded tracks
fbstring create_dir() {
    auto now = std::chrono::system_clock::now();
    auto now_unix = std::chrono::system_clock::to_time_t(now);
    auto now_stct = std::localtime(&now_unix);
    auto count = 64;
    char date[count];
    std::strftime(date, count, "/tmp/snatch-%F:XXXXXX", now_stct);
    return fbstring(mkdtemp(date));
}

// Parses a line of input: "\w*<artist>\w*:\w*<title>\w*"
void find_artist_title(fbstring &line, fbstring &artist, fbstring &title) {
    if (folly::split(':', line, artist, title)) {
        boost::trim(artist);
        boost::trim(title);
    } else {
        artist = "Darude";
        title = "Sandstorm";
    }
}

// Queries the search API and chooses the first result (I'm feeling lucky) as a
// likely music video/lyric posting with the song as the audio track.
fbstring acquire_id(fbstring line) {
    const auto api_key = "AIzaSyB7n4YGgRDqlg_cfbZCLzRw6akq9pIRC7M";
    auto urlFormat = "https://www.googleapis.com/youtube/v3/search?q=%s&key=%s" \
        "&maxResults=1&part=snippet";
    fbstring esc_line;
    folly::uriEscape(line, esc_line, folly::UriEscapeMode::QUERY);
    auto url = folly::stringPrintf(urlFormat, esc_line.c_str(), api_key);
    folly::dynamic resp = folly::dynamic::object;
    try {
        curlpp::Cleanup myCleanup;
        std::ostringstream os;
        os << curlpp::options::Url(url);
        auto json_resp = os.str();
        resp = folly::parseJson(json_resp);
    } catch (std::exception e) {
        std::cerr << "Connection to youtube search api failed" << std::endl;
    }
    auto id = resp["items"][0]["id"]["videoId"];;
    return id.asString();
}

// Assembles the relevant strings to the future file, i.e. where it
// is and what to call it when it gets here.
track_loc find(fbstring input) {
    cout << "Finding..." << endl;
    fbstring id, artist, title;
    id = acquire_id(input);
    find_artist_title(input, artist, title);
    cout << "X:" <<artist << ' ' << title << endl;
    track_loc tl = {id, artist, title};
    return tl;
}

// Calls into the python interpreter to ask youtube_dl politely
// to work its magic.
int py_download(fbstring id, fbstring fn_template) {
    bool threadsafe = true;
    if (threadsafe) {
        auto format = "youtube-dl --quiet --audio-format=\"mp3\" --extract-audio " \
                      "-o \"%s\" -- %s";
        auto cmd = folly::stringPrintf(format, fn_template.c_str(), id.c_str());
        cout << "tmlpl: " << fn_template << endl;
        cout << "id   : " << id << endl;
        cout << "Dlcmd: " << cmd << endl;
        return std::system(cmd.c_str());
    } else {
        auto pModule = PyImport_ImportModule("download");
        if (!pModule) {
            std::cerr << "Couldn't import download.py." << std::endl;
            return 1;
        }
        auto pFunc = PyObject_GetAttrString(pModule, "take_video");
        if (not (pFunc && PyCallable_Check(pFunc))) {
            std::cerr << "Couldn't import take_video from download.py." << std::endl;
            return 1;
        }
        auto pArgs = PyTuple_New(2);
        auto pVidid = PyString_FromString(id.c_str());
        auto pTemplate = PyString_FromString(fn_template.c_str());
        PyTuple_SetItem(pArgs, 0, pVidid);
        PyTuple_SetItem(pArgs, 1, pTemplate);
        PyObject_CallObject(pFunc, pArgs);
        return 0;
    }
}

// Updates the id3 tags so that the music player can display metadata
void label_song(const char *filename, const char *title, const char *artist) {
    ID3_Tag tag(filename);
    ID3_AddTitle(&tag, title, true);
    ID3_AddArtist(&tag, artist, true);
    tag.Update();
}

int queue_song(const char *filename) {
    auto queue_cmd = folly::stringPrintf("cmus-remote -q \"%s\"", filename);
    return std::system(queue_cmd.c_str());
}

int download(track_loc tl, fbstring dir) {
    // Acquires the file, labels it upon receipt and sends it to the music
    // player.
    cout << "Downloading..." << endl;
    auto stem = dir + "/" + tl.title;
    auto fn_template = stem + ".%(ext)s";
    auto filename = (stem + ".mp3").c_str();
    auto id = tl.id.c_str();
    auto artist = tl.artist.c_str();
    auto title = tl.title.c_str();
    auto cdir = dir.c_str();

    cout << fbstring(id) << endl;
    cout << fbstring(artist) << endl;
    cout << fbstring(title) << endl;
    cout << fbstring(cdir) << endl;

    if (py_download(id, fn_template)) {
        std::cerr << "Could not download file";
        return 1;
    }
    label_song(filename, title, artist);
    return queue_song(filename);
}


int main(int argc, char *argv[]) {
    // Start up the interpreter, the SetArgv allows relative imports to work.
    Py_Initialize();
    PySys_SetArgv(argc, argv);

    if (not is_cmus_running()) {
        std::cerr << "Please run cmus first" << std::endl;
        return 1;
    }

    // One queue for "Artist: Title" user input and
    // one for the info about where to download such a track.
    folly::MPMCQueue<fbstring> artitles(10);
    folly::MPMCQueue<track_loc> track_locs(10);

    // Responsible for transferring user input to the queue.
    std::thread reader([&artitles] {
        char line[80];
        while (std::cin.getline(line, 80)) {
            artitles.blockingWrite(line);
        }
    });

    auto workers = {0, 1, 2};

    // Finders take an artitle and locate a likely YouTube id for
    // the corresponding song and feed it to a downloader
    std::vector<std::thread> finders;
    for (int i : workers) {
        finders.push_back(std::thread([&artitles, &track_locs] {
            for (;;) {
                fbstring str;
                artitles.blockingRead(str);
                auto tl = find(str);
                track_locs.blockingWrite(std::move(tl));
            }
        }));
    }

    auto dir = create_dir();

    // Downloaders take a video id and acquire the audio file
    // in that video and tag it with the appropriate metadata.
    std::vector<std::thread> downloaders;
    for (int i : workers) {
        downloaders.push_back(std::thread([&track_locs, &dir] {
            for (;;) {
                track_loc tl;
                track_locs.blockingRead(tl);
                if (download(tl, dir)) {
                    std::cerr << "Could not download " << tl.id << endl;
                }
            }
        }));
    }

    // Not reached below here.
    reader.join();
    for (auto i: workers) {
        finders[i].join();
    }
    for (auto i : workers) {
        downloaders[i].join();
    }
    // Clean up the interpreter
    Py_Finalize();
}
