#!/bin/bash

# $1 Ptu MAC address
# $2 Serving Slot ID 
# $3 Session ID
# $4 Log file

umask 0000

#LIVESERVERIP=10.42.1.1
SINKBIN=/usr/src/server-software/videosink/bin/videosink
CFGPATH=/etc/videostream.conf.d
[ -n "$5" ] && CFGPATH="$5"
[ -n "$6" ] && SINKBIN="$6"
MINFILESIZE=512
MJPEGVBRATE=4000000
H264VBRATE=2048
FRAMERATE=15
FRAMERATEIN=25

stop_execution()
{
	echo >&2 $@
	exit 1
}

if [ $# -lt 3 ]; then
        stop_execution "Please provide all the necessary parameters."
fi

if [ ! -e ${CFGPATH}/${1} ]; then
        stop_execution "There is no PTU registered with this MAC address."
fi

[ -n "$4" ] && exec >> "${4}"  2>&1

echo "-------------------- SESSION START (`date`) --------------------"
while read line 
do
	if [[ $line == name=* ]]; then
		VIDEODEVICE=${line:5}
	elif [[ $line == width=* ]]; then
		VIDEOWIDTH=${line:6}
	elif [[ $line == height=* ]]; then
		VIDEOHEIGHT=${line:7}
	elif [[ $line == output-path=* ]]; then
		OUTPUTPATH=${line:12}
	elif [[ $line == port=* ]]; then
		STREAMPORT=${line:5}
	fi
done < ${CFGPATH}/${1}
AUDIODEVICEIN="plughw:Loopback,1,"`echo ${VIDEODEVICE}|sed s/.*video//`
AUDIODEVICEOUT="plughw:Loopback,0,"`echo ${VIDEODEVICE}|sed s/.*video//`

SLOTCFG=`printf "s-%0*d\n" 3 ${2}`

if [ ! -e ${CFGPATH}/${SLOTCFG} ]; then
        stop_execution "There is no such slot."
fi

while read line
do
	if [[ $line == id=* ]]; then
		SLOTID=${line:3}
	elif [[ $line == mjpg=* ]]; then
		PORTMJPG=${line:5}
	elif [[ $line == rtp=* ]]; then
		PORTRTP=${line:4}
	elif [[ $line == rtsp=* ]]; then
		PORTRTSP=${line:5}
	elif [[ $line == mp4=* ]]; then
		PORTMP4=${line:4}
	fi	
done < ${CFGPATH}/${SLOTCFG}

echo `date +"%H:%M:%S.%N"` " - Raising Sink" 
GST_DEBUG='*:0' ${SINKBIN} --config-path ${CFGPATH}/${1} --session-id ${3} & 
#gst-launch -v videotestsrc pattern=18 ! video/x-raw-yuv,width=640,height=480,framerate=${FRAMERATEIN}/1 ! v4l2sink device=${VIDEODEVICE} &

VSPID=$!
sleep 2 
exec_audio()
{
gst-launch -v  udpsrc port=$STREAMPORT caps="application/x-rtp" ! queue ! rtppcmudepay ! mulawdec ! audioconvert ! alsasink device=${AUDIODEVICEIN}

}
exec_vlc(){

#vlc -I dummy --mms-caching 0 http://www.nasa.gov/55644main_NASATV_Windows.asx vlc://quit --sout='#transcode{width=320,height=240,fps=25,vcodec=h264,vb=256,venc=x264{aud,profile=baseline,level=30,keyint=30,ref=1},acodec=mp3,ab=96}:std{access=livehttp{seglen=10,delsegs=true,numsegs=5,index=/var/www/streaming/mystream.m3u8,index-url=http://mydomain.com/streaming/mystream-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/mystream-########.ts}'
#/usr/local/bin/cvlc --play-and-exit -q v4l2://${VIDEODEVICE} &
#/usr/bin/cvlc  --play-and-exit -q v4l2://${VIDEODEVICE} --sout="#transcode{width640,height=480,fps=25,vcodec=h264,vb=256,venc=x264{aud,profile=baseline,level=30,keyint=30,ref=1},acodec=mp4a,samplerate=44100,ab=96}:std{access=livehttp{seglen=10,delsegs=true,numsegs=5,index=/var/www/streaming/${SLOTID}.m3u8,index-url=${LIVESERVERIP}/streaming/${SLOTID}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${SLOTID}-########.ts" &
#/usr/bin/cvlc --play-and-exit -q v4l2://${VIDEODEVICE} --sout="#duplicate{dst=\"transcode{width=${VIDEOHEIGHT},height=${VIDEOWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${SLOTID}.m3u8,index-url=${LIVESERVERIP}/streaming/${SLOTID}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${SLOTID}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${SLOTID}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${SLOTID}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${SLOTID}.mjpg\"}}" 2> /dev/null &
#-sout="#transcode{width=640,height=480,fps=30,vcodec=h264,vb=1024,venc=x264{keyint=15,level=30,profile=baseline,cabac=1},threads=auto,audio-sync,deinterlace,ab=96,samplerate=44100,channels=2,acodec=mp4a}:standard{access=livehttp{seglen=5, delsegs=true,numsegs=50,index=/var/www/streaming/mystream.m3u8,index-url=http://10.42.1.1/streaming/mystream-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/mystream-########.ts}"
/usr/bin/cvlc  -L -q v4l2://${VIDEODEVICE} --input-slave="alsa://${AUDIODEVICEOUT}" --sout="#duplicate{dst=\"transcode{width=${VIDEOHEIGHT},height=${VIDEOWIDTH},fps=${FRAMERATE},vcodec=h264,vb=${H264VBRATE},scale=1,venc=x264{profile=baseline,level=30,keyint=15,bframes=0,ref=1,cabac=1},threads=auto,ab=96,samplerate=44100,channels=1,acodec=mp4a}:duplicate{dst=std{access=livehttp{seglen=10,delsegs=true,numsegs=10,index=/var/www/streaming/${SLOTID}.m3u8,index-url=${LIVESERVERIP}/streaming/${SLOTID}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${SLOTID}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${SLOTID}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${SLOTID}.sdp}}\",dst=\"transcode{fps=${FRAMERATE},vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${SLOTID}.mjpg\"}}" & # 2> /dev/null &
#/usr/bin/cvlc  -L -q v4l2://${VIDEODEVICE} --input-slave="alsa://asymed" --sout="#duplicate{dst=\"transcode{width=${VIDEOHEIGHT},height=${VIDEOWIDTH},fps=25,vcodec=h264,vb=${H264VBRATE},scale=1,venc=x264{profile=baseline,level=30,keyint=15,bframes=0,ref=1,cabac=1},threads=auto,ab=96,audio-sync,samplerate=44100,channels=1,acodec=mp4a}:duplicate{dst=std{access=livehttp{seglen=3,delsegs=true,numsegs=10,index=/var/www/streaming/${SLOTID}.m3u8,index-url=${LIVESERVERIP}/streaming/${SLOTID}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${SLOTID}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${SLOTID}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${SLOTID}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${SLOTID}.mjpg\"}}" & # 2> /dev/null &
VLCPID=$!
}
exec_audio &
AUDIOPID=$!
sleep 1
exec_vlc 


control_c()
{
#set -x
	echo $$ > /tmp/ss
	PGID=`ps --no-heading -p $$ -o "%r"| awk '{print $1}'`
	kill -INT -${PGID}
	sleep 2
	kill -KILL -${PGID}
	exit 0
	while [ -n "`ps --no-headers -p ${VSPID} -o '%p' | awk '{print $1}'`" ]
	do
		echo VSPID
		kill ${VSPID} 2> /dev/null 
		sleep 0.5
	done
	
	while [ -n "`ps --no-headers -p ${VLCPID} -o '%p' | awk '{print $1}'`" ]
	do
		echo VLCPID
		kill -SIGINT ${VLCPID} 2> /dev/null
		sleep 0.5
	done
	echo AUDIOPID
	kill ${AUDIOPID} 2> /dev/null 
	
#	for file in ${OUTPUTPATH}/${3}/*; do
#		
#		FILESIZE=`du $file | awk '{print $1}'`
#		
#		if [ ${FILESIZE} -lt ${MINFILESIZE} ]; then
#		echo ${FILESIZE} -lt ${MINFILESIZE} 
#		echo	rm -f $file
#		fi
#	done

	exit 0
}

trap control_c SIGINT SIGTERM 

#set -x
while true 
do
	EXEC_FLAG=0

	if [ -z "`ps --no-headers -p ${VSPID} -o '%p' | awk '{print $1}'`" ]; then

		while [ -n "`ps --no-headers -p ${VLCPID} -o '%p' | awk '{print $1}'`" ]
		do
			kill -SIGINT ${VLCPID} 2> /dev/null
		done

		EXEC_FLAG=1
	fi
	
        if [ -z "`ps --no-headers -p ${VLCPID} -o '%p' | awk '{print $1}'`" ]; then

                while [ -n "`ps --no-headers -p ${VSPID} -o '%p' | awk '{print $1}'`" ]
                do
                        kill ${VSPID} 2> /dev/null
                done

                EXEC_FLAG=1
        fi

        if [ ${EXEC_FLAG} -eq 1 ]; then
	
		for file in ${OUTPUTPATH}/${3}/*; do
			
			FILESIZE=`du $file | awk '{print $1}'`
			
			if [ ${FILESIZE} -lt ${MINFILESIZE} ]; then
				rm -f $file
			fi
		done

		echo `date +"%H:%M:%S.%N"` " - Raising Sink" 
		GST_DEBUG='*:0' ${SINKBIN} --config-path ${CFGPATH}/${1} --session-id ${3} & 
		VSPID=$!
		sleep 2 
		exec_vlc  
	fi

	sleep 2 
done

