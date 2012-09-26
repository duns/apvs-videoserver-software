#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-0.10
TRANSCODE=ALL
STREAMNAME=worker0
STREAMPORT=5000
PORTMJPG=8090
PORTRTP=8091
PORTRTSP=8092
PORTMP4=8093
VIDEODEV="/dev/video0"
MJPEGFPS=15
INPUTFPS="15/1"
FRAMERATE=15
MJPEGFPS=30
INPUTFPS="30/1"
FRAMERATE=30
DROPFRAMES="-v"
ROTATE="n"
PYVIDEOSERVER="./pyvideoserver.sh"
STREAMFILEDIR="/var/filestreams"
#TEETOFILE=""

SERVERIP=10.8.1.6
SERVERIP=pcatlaswpss02
MJPEGVBRATE=600
MJPEGVBRATE=$(( MJPEGVBRATE * MJPEGFPS ))
MJPEGVBRATE=8000
H264VBRATE=512
SCALEWIDTH=416
SCALEHEIGHT=376

SCALEWIDTH=640
SCALEHEIGHT=480

CAPSSTRING='\"Z01AFuygUB7YCIAAAAMAu5rKAAeLFss\\=\\,aOvssg\\=\\=\"' # 640x480
#CAPSSTRING='\"Z0KAHukDwXvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 480x360
CAPSSTRING='\"Z0KAHukCg+QgAAB9AAAdTACAAA\\=\\=\\,aM48gAA\\=\"' # 320x240
#CAPSSTRING='\"Z0KAHukDQYvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 376x416
#CAPSSTRING='\"Z0KAHukDwXvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' #480x360 
#CAPSSTRING='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' #640x480
#CAPSSTRING='\"Z0KAHukC4XsssIAAAfQAAHUwAgA\\=\\,aM48gAA\\=\"' #360X360
#CAPSSTRING='\"Z01AFeygoP2AiAAAAwALuaygAHixbLA\\=\\,aOvssg\\=\\=\"' # testpattern

	ENCODING=MJPEG
	ENCODING=H264

[ -n "${1}" ] && STREAMNAME="${1}"
[ -n "${2}" ] && STREAMPORT="${2}"
[ -n "${3}" ] && PORTMJPG="${3}"
[ -n "${4}" ] && PORTRTP="${4}"
[ -n "${5}" ] && PORTRTSP="${5}"
[ -n "${6}" ] && PORTMP4="${6}"
[ -n "${7}" ] && VIDEODEV="${7}"
[ -n "${8}" ] && ROTATE="${8}"
[ -n "${9}" ] && ENCODING="${9}"
[ -n "${10}" ] && SCALEWIDTH="${10}"
[ -n "${11}" ] && SCALEHEIGHT="${11}"
[ -n "${12}" ] && CAPSSTRING="${12}"

FILEOUTNAME=${STREAMNAME}

DATE=`date +"%F-%T"`
DATE=`date +"%F-%H.%M.%S"`
#SCALEWIDTH=480
#SCALEHEIGHT=360


#set -x 
TEETOFILE=" tee1. ! queue !  mpegtsmux ! filesink location=${STREAMFILEDIR}/${FILEOUTNAME}-${DATE}.mp4 "
TEETOFILE=" tee1. ! queue !  mpegtsmux ! multifilesink location=${STREAMFILEDIR}/${FILEOUTNAME}-${DATE}-%06d.ts next-file=1 "

EXTRAEFFECTS="  "
#EXTRAEFFECTS=" ! ffmpegcolorspace ! facedetect ! ffmpegcolorspace  "



launch_videodev_py()
{
	case $ENCODING in
		H264)
		ENCODINGNAME="h264";
		;;
		MJPEG)
		ENCODINGNAME="mjpeg";
		;;
	esac
		${PYVIDEOSERVER}  -d ${VIDEODEV} -e ${ENCODINGNAME} -c ${CAPSSTRING} ${DROPFRAMES} --fps ${INPUTFPS} -p ${STREAMPORT} -W ${SCALEWIDTH} -H ${SCALEHEIGHT}
	echo videodev exited
}
launch_videodev()
{
	case $ENCODING in
		H264)
		ENCODINGNAME="H264";
	DEPAY="rtph264depay";
	DECODE="ffdec_h264"
	VSRC="udpsrc caps=\"application/x-rtp,media=(string)video,framerate=${FRAMERATE}/1,clock-rate=(int)90000,encoding-name=(string)${ENCODINGNAME},payload=(int)96,sprop-parameter-sets=(string)${CAPSSTRING}\" port=${STREAMPORT} "
		;;
		MJPEG)
		ENCODINGNAME="JPEG";
	DEPAY="rtpjpegdepay";
	DECODE="jpegdec"
	VSRC="udpsrc caps=\"application/x-rtp,media=(string)video,framerate=${FRAMERATE}/1,clock-rate=(int)90000,encoding-name=(string)${ENCODINGNAME},payload=(int)96\" port=${STREAMPORT} "
		;;
	esac

	VIDEOSOURCEDECODED="${VSRC} !  ${DEPAY} !  tee name=tee1 ! ${DECODE} "
	OVERLAYS=" ffmpegcolorspace ! timeoverlay halign=right  !  clockoverlay halign=right valign=bottom shaded-background=true time-format="%H:%M:%S"  !"
	OVERLAYFILE=/usr/local/videoserver/overlays/arrow1.png
OVERLAYFILE=/usr/share/app-install/icons/debian-logo.png
	OVERLAYPIPE="videotestsrc pattern=18 ! video/x-raw-yuv,width=200,height=100 ! alpha ! ffmpegcolorspace ! videobox alpha=0 !  queue  ! mix.sink_1"
	#OVERLAYPIPE="videotestsrc pattern=18 ! queue ! ffmpegcolorspace ! alpha ! mix."
#	OVERLAYPIPE="multifilesrc caps=\"image/png,framerate=1/1\" location=${OVERLAYFILE} ! pngdec ! alpha ! ffmpegcolorspace ! videobox !  queue  ! mix.sink_1"
	#OVERLAYPIPE=""
#	EXTRAEFFECTS="ffmpegcolorspace ! queue !  "
#	VIDEOSOURCEDECODED="videotestsrc ! video/x-raw-yuv,framerate=${MJPEGFPS}/1 "
	while true;
		do
		if [ ${ROTATE} == "y" ] ; then

	#gst-launch-0.10  udpsrc caps="application/x-rtp,media=(string)video,framerate=${FRAMERATE}/1,clock-rate=(int)90000,encoding-name=(string)${ENCODINGNAME},payload=(int)96,sprop-parameter-sets=(string)${CAPSSTRING}" port=${STREAMPORT} !  ${DEPAY} ! ${DECODE} ${EXTRAEFFECTS} !  videoscale ! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT} !  videoflip method=clockwise !  ${OVERLAYS}  v4l2sink device=${VIDEODEV}  sync=false 
	gst-launch-0.10  ${VIDEOSOURCEDECODED} ! queue ! ffmpegcolorspace ${EXTRAEFFECTS} !  videoscale !  videoflip method=clockwise! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT}  ! videomixer name=mix !  ${OVERLAYS}  v4l2sink device=${VIDEODEV}  sync=false 
		else
			
#	gst-launch-0.10  udpsrc caps="application/x-rtp,media=(string)video,framerate=${FRAMERATE}/1,clock-rate=(int)90000,encoding-name=(string)${ENCODINGNAME},payload=(int)96,sprop-parameter-sets=(string)${CAPSSTRING}" port=${STREAMPORT} !  ${DEPAY} ! ${DECODE} ${EXTRAEFFECTS} ! videoscale ! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT} !   timeoverlay !  clockoverlay halign=right valign=bottom shaded-background=true time-format="%H:%M:%S"  !  v4l2sink device=${VIDEODEV}  sync=false 
# gst-launch-0.10 -e videotestsrc pattern="snow" ! video/x-raw-yuv, framerate=10/1, width=200, height=150 !  videobox border-alpha=0 alpha=0.6 top=-20 left=-25 ! videomixer name=mix ! ffmpegcolorspace ! v4l2sink device=${VIDEODEV}  sync=false  videotestsrc ! video/x-raw-yuv, framerate=10/1, width=640, height=360 ! mix.
#	VIDEOSOURCEDECODED="videotestsrc ! video/x-raw-yuv,framerate=${MJPEGFPS}/1 "

#	gst-launch-0.10  ${VIDEOSOURCEDECODED} ! queue  ${EXTRAEFFECTS} !  videoscale ! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT} ! alpha ! videomixer name=mix ! ${OVERLAYS}   v4l2sink device=${VIDEODEV}  sync=false ${OVERLAYPIPE}
	gst-launch-0.10   videomixer name=mix sink_1::xpos=0 sink_1::ypos=0 sink_1::alpha=0.5 sink_1::zorder=3 sink_1::xpos=0 sink_0::ypos=0 sink_0::alpha=1 sink_0::zorder=2 ! ${OVERLAYS}   v4l2sink device=${VIDEODEV}  sync=false   ${VIDEOSOURCEDECODED} ! queue  ! ffmpegcolorspace  ${EXTRAEFFECTS} !  videoscale ! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT} !   mix.sink_0 ${OVERLAYPIPE} ${TEETOFILE}

		fi
		done
}

launch_streamserver()
{
	while true
		do
		[ ${ROTATE} != "y" ]  && SCALEHEIGHTT=${SCALEHEIGHT} && SCALEHEIGHT=${SCALEWIDTH} && SCALEWIDTH=${SCALEHEIGHTT}
case ${TRANSCODE} in
	MJPEGONLY)
#LIVESERVERIP=192.168.3.204
#cvlc --play-and-exit -q  v4l2://${VIDEODEV}  --sout="#transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=${LIVESERVERIP}:${PORTMJPG}/${STREAMNAME}.mjpg\"}" #2>/dev/null
#cvlc --play-and-exit -q  v4l2://${VIDEODEV}  :v4l2-fps=0.2 --sout="#duplicate{dst=\"transcode{fps=${MJPEGFPS},vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${STREAMNAME}.mjpg\"}}" #2>/dev/null
cvlc --play-and-exit -q  v4l2://${VIDEODEV}  --sout="#duplicate{dst=\"transcode{width=${SCALEWIDTH},height=${SCALEHEIGHT},fps=${MJPEGFPS},vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${STREAMNAME}.mjpg\"}}" #2>/dev/null
;;
	ALL)
cvlc --play-and-exit -q  v4l2://${VIDEODEV}  --sout="#duplicate{dst=\"transcode{width=${SCALEHEIGHT},height=${SCALEWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${STREAMNAME}.m3u8,index-url=${LIVESERVERIP}/streaming/${STREAMNAME}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${STREAMNAME}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${STREAMNAME}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${STREAMNAME}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${STREAMNAME}.mjpg\"}}" #2>/dev/null
#cvlc --play-and-exit -q  v4l2://${VIDEODEV}   --sout="#duplicate{dst=\"transcode{width=${SCALEHEIGHT},height=${SCALEWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${STREAMNAME}.m3u8,index-url=${LIVESERVERIP}/streaming/${STREAMNAME}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${STREAMNAME}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${STREAMNAME}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${STREAMNAME}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${STREAMNAME}.mjpg\"}}" #2>/dev/null
;;
	esac
		sleep 2
	done
}

control_c()
# run if user hits control-c
{
  echo -en "\nExiting\n"
set -x
	  for (( CHILDNUM=1; CHILDNUM<=CHILDS; CHILDNUM ++ ))
		  do
			  PID=${CPID[${CHILDNUM}]}
#PGID=`ps -p ${PID} -o "%r"|awk '{print $NF}'`
PGID=`ps --no-headers -p ${PID} -o "%r"|awk '{print $1}'`
#	echo Child pid=${PID} group=${PGID}
#	echo kill -SIGTERM -$PID
	kill -SIGINT $PID
	kill -SIGINT $PID
#	sleep 1
#	kill -SIGTERM -$PGID
#kill -SIGTERM $PID
#	sleep 3
#kill -SIGKILL $PID 2>/dev/null
	PGIDS="$PGIDS -$PGID"
	kill -SIGKILL $PID
		done
	kill -SIGKILL $PGIDS

   exit 0
}
 
# trap keyboard interrupt (control-c)
trap control_c SIGINT
umask 000
set -x
LOGFILE=/tmp/vid-${STREAMNAME}-${STREAMPORT}.log
date > ${LOGFILE}
echo launching videodev
set -x
echo launching stream server
#launch_videodev_py & 
launch_videodev & 
#launch_videodev  >> ${LOGFILE} 2>&1 & 
CPID[1]=$!
CHILDS=1
#wait
#exit
echo launching stream server
#launch_streamserver  >> ${LOGFILE} 2>&1 &
launch_streamserver &
CPID[2]=$!
CHILDS=2
wait
