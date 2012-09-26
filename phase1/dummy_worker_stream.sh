#!/bin/bash
WIDTH=320
HEIGHT=240
FRAMERATE=30
HOST=10.8.1.6
HOST=localhost
STREAMNAME=worker0
PORT=5000

[ -n "${1}" ] && STREAMNAME="${1}"
[ -n "${2}" ] && PORT="${2}"

gst-launch-0.10 -v videotestsrc ! video/x-raw-yuv,width=${WIDTH},height=${HEIGHT},framerate=${FRAMERATE}/1 ! textoverlay text=${STREAMNAME} font-desc="Sans 26"  valign=top halign=center !  timeoverlay  font-desc="Sans 18" halign=left valign=bottom text="Stream time:" shaded-background=true ! x264enc ! rtph264pay name=pay0 pt=96 ! udpsink port=${PORT} host=${HOST} 


