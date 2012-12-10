#!/bin/bash

# $1 Ptu MAC address
# $2 Serving Slot ID 
# $3 Session ID
# $4 Log file

umask 0000

SINKBIN=videosink
CFGPATH=/etc/videostream.conf
MINFILESIZE=512
MJPEGVBRATE=6000000
H264VBRATE=512

stop_execution()
{
	echo >&2 $@
	exit 1
}

if [ ! $# -eq 4 ]; then
        stop_execution "Please provide all the necessary parameters."
fi

if [ ! -e ${CFGPATH}/${1} ]; then
        stop_execution "There is no PTU registered with this MAC address."
fi

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
	fi
done < ${CFGPATH}/${1}

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

echo `date +"%H:%M:%S.%N"` " - Raising Sink" >> ${4}
${SINKBIN} --config-path ${CFGPATH}/${1} --session-id ${3} & 
VSPID=$!
sleep 2 
#/usr/local/bin/cvlc --play-and-exit -q v4l2://${VIDEODEVICE} &
/usr/local/bin/cvlc --play-and-exit -q v4l2://${VIDEODEVICE} --sout="#duplicate{dst=\"transcode{width=${VIDEOHEIGHT},height=${VIDEOWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${SLOTID}.m3u8,index-url=${LIVESERVERIP}/streaming/${SLOTID}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${SLOTID}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${SLOTID}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${SLOTID}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${SLOTID}.mjpg\"}}" 2> /dev/null &
VLCPID=$!

control_c()
{
	while [ `ps --no-headers -p ${VSPID} -o "%p" | awk '{print $1}'` ]
	do
		kill ${VSPID} 2> /dev/null 
	done
	
	while [ `ps --no-headers -p ${VLCPID} -o "%p" | awk '{print $1}'` ]
	do
		kill -SIGINT ${VLCPID} 2> /dev/null
	done
	
	for file in ${OUTPUTPATH}/${3}/*; do
		
		FILESIZE=`du $file | awk '{print $1}'`
		
		if [ ${FILESIZE} -lt ${MINFILESIZE} ]; then
			rm -f $file
		fi
	done

	exit 0
}

trap control_c SIGINT

while true 
do
	EXEC_FLAG=0

	if [ ! `ps --no-headers -p ${VSPID} -o "%p" | awk '{print $1}'` ]; then

		while [ `ps --no-headers -p ${VLCPID} -o "%p" | awk '{print $1}'` ]
		do
			kill -SIGINT ${VLCPID} 2> /dev/null
		done

		EXEC_FLAG=1
	fi
	
        if [ ! `ps --no-headers -p ${VLCPID} -o "%p" | awk '{print $1}'` ]; then

                while [ `ps --no-headers -p ${VSPID} -o "%p" | awk '{print $1}'` ]
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

		echo `date +"%H:%M:%S.%N"` " - Raising Sink" >> ${4}
		${SINKBIN} --config-path ${CFGPATH}/${1} --session-id ${3} & 
		VSPID=$!
		sleep 2 
		#/usr/local/bin/cvlc --play-and-exit -q v4l2://${VIDEODEVICE} &
		/usr/local/bin/cvlc --play-and-exit -q v4l2://${VIDEODEVICE} --sout="#duplicate{dst=\"transcode{width=${VIDEOHEIGHT},height=${VIDEOWIDTH},fps=15,vcodec=h264,vb=${H264VBRATE},scale=1,acodec=none,venc=x264{aud,profile=baseline,level=30,keyint=15,min-keyint=15,ref=1,nocabac}}:duplicate{dst=std{access=livehttp{seglen=2,delsegs=true,numsegs=15,index=/var/www/streaming/${SLOTID}.m3u8,index-url=${LIVESERVERIP}/streaming/${SLOTID}-########.ts},mux=ts{use-key-frames},dst=/var/www/streaming/${SLOTID}-########.ts},dst=std{access=http,mux=ts,dst=:${PORTMP4}/${SLOTID}.mp4},dst=rtp{dst=localhost,port=${PORTRTP},sdp=rtsp://:${PORTRTSP}/${SLOTID}.sdp}}\",dst=\"transcode{fps=15,vcodec=mjpg,vb=${MJPEGVBRATE}}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:${PORTMJPG}/${SLOTID}.mjpg\"}}" 2> /dev/null &
		VLCPID=$!
	fi

	sleep 2 
done
