#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
STREAMNAME=worker0
STREAMPORT=5000
PORTMJPG=8090
PORTRTP=8091
PORTRTSP=8092
PORTMP4=8093
VIDEODEV="/dev/video0"
ROTATE="n"

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
[ -n "${10}" ] && CAPSSTRING="${10}"



SERVERIP=192.168.3.204
SERVERIP=10.8.1.6
SERVERIP=pcatlaswpss02
MJPEGVBRATE=2000
H264VBRATE=512
FRAMERATE=15
SCALEWIDTH=416
SCALEHEIGHT=376

SCALEWIDTH=640
SCALEHEIGHT=480

#SCALEWIDTH=480
#SCALEHEIGHT=360


#set -x 




launch_videodev()
{
	case $ENCODING in
		H264)
		ENCODINGNAME="H264";
	DEPAY="rtph264depay";
	DECODE="ffdec_h264"
		;;
		MJPEG)
		ENCODINGNAME="JPEG";
	DEPAY="rtpjpegdepay";
	DECODE="jpegdec"
		;;
	esac

		if [ ${ROTATE} == "y" ] ; then

	gst-launch-0.10  udpsrc caps="application/x-rtp,media=(string)video,framerate=${FRAMERATE}/1,clock-rate=(int)90000,encoding-name=(string)${ENCODINGNAME},payload=(int)96,sprop-parameter-sets=(string)${CAPSSTRING}" port=${STREAMPORT} !  ${DEPAY} ! ${DECODE} !  videoscale ! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT} !  videoflip method=clockwise !# timeoverlay !  clockoverlay halign=right valign=bottom shaded-background=true time-format="%H:%M:%S"  !  v4l2sink device=${VIDEODEV}  sync=false 
		else
			
	gst-launch-0.10  udpsrc caps="application/x-rtp,media=(string)video,framerate=${FRAMERATE}/1,clock-rate=(int)90000,encoding-name=(string)${ENCODINGNAME},payload=(int)96,sprop-parameter-sets=(string)${CAPSSTRING}" port=${STREAMPORT} !  ${DEPAY} ! ${DECODE} !  videoscale ! video/x-raw-yuv,width=${SCALEWIDTH},height=${SCALEHEIGHT} !   timeoverlay !  clockoverlay halign=right valign=bottom shaded-background=true time-format="%H:%M:%S"  !  v4l2sink device=${VIDEODEV}  sync=false 

		fi
}

launch_streamserver()
{
	while true
		do
		[ ${ROTATE} != "y" ]  && SCALEHEIGHTT=${SCALEHEIGHT} && SCALEHEIGHT=${SCALEWIDTH} && SCALEWIDTH=${SCALEHEIGHTT}

cvlc --play-and-exit -q  v4l2://${VIDEODEV}  --sout="#duplicate{dst=\"transcode{width=${SCALEHEIGHT},height=${SCALEWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${STREAMNAME}.m3u8,index-url=${LIVESERVERIP}/streaming/${STREAMNAME}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${STREAMNAME}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${STREAMNAME}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${STREAMNAME}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${STREAMNAME}.mjpg\"}}" #2>/dev/null
#cvlc --play-and-exit -q  v4l2://${VIDEODEV}   --sout="#duplicate{dst=\"transcode{width=${SCALEHEIGHT},height=${SCALEWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${STREAMNAME}.m3u8,index-url=${LIVESERVERIP}/streaming/${STREAMNAME}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${STREAMNAME}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${STREAMNAME}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${STREAMNAME}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${STREAMNAME}.mjpg\"}}" #2>/dev/null
		sleep 2
	done
}

control_c()
# run if user hits control-c
{
  echo -en "\nExiting\n"
   exit 0
}
 
# trap keyboard interrupt (control-c)
umask 000
LOGFILE=/tmp/vid-${STREAMNAME}-${STREAMPORT}.log
date > ${LOGFILE}
echo launching videodev
launch_videodev  >> ${LOGFILE} 2>&1 & 
trap control_c SIGINT
echo launching stream server
launch_streamserver  >> ${LOGFILE} 2>&1
