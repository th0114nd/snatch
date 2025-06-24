import yt_dlp as youtube_dl
import urllib
import requests
import os.path

def find_video(name: str) -> str:
    escaped_name = urllib.parse.quote(name)
    print(escaped_name)
    # api_key = "AIzaSyB7n4YGgRDqlg_cfbZCLzRw6akq9pIRC7M"
    api_key = "AIzaSyAH0uWu8gf2iy3c5ponbU2Gfsr2UGaVW9M"
    url = f"https://www.googleapis.com/youtube/v3/search?q={escaped_name}&key={api_key}" \
        "&maxResults=1&part=snippet";
    print(url)
    resp = requests.get(url)
    data = resp.json()
    return data['items'][0]['id']['videoId']

def take_video(dir: str, vidid: str, template, i: int):
    full_template = f'{dir}/{i:02d} {template}'
    if os.path.exists(f'{full_template}.mp3'):
        print(f"Skipping {full_template}.mp3: already exists")
        return

    ydl_opts = {
        'quiet': True,
        'outtmpl': full_template,
        'forcefilename': True,
        'postprocessors': [
            {'key': 'FFmpegExtractAudio',
             'preferredcodec': 'mp3'}
            ]
    }
    url = f"http://www.youtube.com/watch?v={vidid}"
    with youtube_dl.YoutubeDL(ydl_opts) as ydl:
        result = ydl.download([url])
        print(result)

SONGS_SEATING = """
Oh Wonder - Technicolor Beat
Vera Hold
St South - Get Good (Infinitefreefall Remix)
Resonance - Home
Rose's Thorn - Tokimonsta
We Belong - RAC Odesza Remix
When u need me - muramasa
"""


SONGS_CEREMONY = """
Dancing Queen - ABBA
Take on Me - A-ha
Girls Just Want to have fun - Cyndi Lauper
Gimme! Gimme! Gimme! - ABBA
Gangnam Style - PSY
Barbie Girl - Aqua
Crazy in Love - Beyonce, JAY-Z
Shalala Lala - Vengaboys
Y.M.C.A. - Village People
Don't Stop Believing - Journey
One Way Ticket - Eruption
I Wanna Dance with Somebody - Whitney Houston
A Thousand Miles - Vanessa Carlton
One More Time - Daft Punk
Funkytown - Lipps Inc.
I Love Rock 'N Roll - Joan Jett & the Blackhearts
September - Earth, Wind & Fire
Wannabe - Spice Girls
Heart of Glass - Blondie
Don't Stop Me Now - Queen
Hey Ya! - Outkast
We Will Rock You - Queen
Shut Up and Dance - Walk The Moon
Mr. Brightside - The Killers
"""

SONGS_DINING = """
Always Forever - Cults
The Brae - Yumi Zouma
The Look - Metronomy
Burning - The Whitest Boy Alive
Down the Line - Beach Fossils
Amoeba - Clairo
Ghostride - Crumb
They Said - Wild Ones
Agitations Tropicales - L'Impératrice
Saltwater - Geowulf
4EVER - Clairo
Seattle Party - Chastity Belt
Darling - Real Estate
Yam Yam - No Vacation
Way to be Loved - TOPS
Weekend Friend - Goth Babe
"""

SONGS_PLAYING_THEM_OUT = """
Sweet Love - Anita Baker
Linger - The Cranberries
I Feel the Earth Move - Carole King
Stand by Me - Otis Redding
Days Like This - Van Morrison
Harvest Moon - Neil Young
Walkin' Back to Georgia - Jim Croce
Listen to the Music - The Doobie Brothers
Summer Breeze - Seals and Crofts
Baker Street - Gerry Rafferty
Just the Way You Are - Billy Joel
American Pie - Don Mclean
"""

COCKTAIL_HOUR = """
Summertime - Charlie Parker
Yardbird Suite - Charlie Parker
My Old Flame - Charlie Parker
Relaxing With Lee - Take 2/Incomplete - Dizzy Gillespie, Charlie Parker, Thelonious Monk
A Night In Tunisia - Live - Charlie Parker
All the Things You Are - Charlie Parker
C Jam Blues - Oscar Peterson Trio
Ruby, My Dear - Thelonious Monk, John Coltrane
Moose The Mooche - Charlie Parker
Billie's Bounce - Charlie Parker
Just Friends - Charlie Parker
Confirmation - Charlie Parker
Out Of Nowhere - Charlie Parker
Boplicity - Miles Davis
Straight, No Chaser (feat. John Coltrane) - Miles Davis, John Coltrane, Cannonball Adderley
An Oscar For Treadwell - Incomplete - Dizzy Gillespipe, Charlie Parker, Thelonious Monk
Scrapple From The Apple - Charlie Parker
April in Paris - Charlie Parker
Groovin' High - Dizzy Gillespie
The Feeling Of Jazz - Duke Ellington, John Coltrane
"""




if __name__ == '__main__':
    for i, song in enumerate(COCKTAIL_HOUR.split('\n')):
        if not song:
            continue
        vidid = find_video(song)
        print(f"Downloading http://www.youtube.com/watch?v={vidid}")
        take_video('cocktail_tracks', vidid, song, i)

