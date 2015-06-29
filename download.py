from __future__ import unicode_literals
import youtube_dl
import os

def take_video(vidid, template):
    ydl_opts = {
        'quiet': True,
        'outtmpl': unicode(template),
        'forcefilename': True,
        'postprocessors': [
            {'key': 'FFmpegExtractAudio',
             'preferredcodec': 'mp3'}
            ]
    }
    url = "http://www.youtube.com/watch?v={0}".format(vidid)
    with youtube_dl.YoutubeDL(ydl_opts) as ydl:
        result = ydl.download([url])
