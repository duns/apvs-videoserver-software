#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
WORKERSERVERBIN=./workerserver_record.sh
WORKERSERVERBINNOREC=./workerserver.sh
CDIR=`dirname $0`
PID=$$
control_c()
# run if user hits control-c
{
  echo -en "\nExiting\n"
	PGID=`ps -p ${PID} -o "%r"`
	echo kill -SIGINT -$PGID
	kill -SIGINT -$PGID
	sleep 5
	kill -SIGKILL -$PGID
   exit 0
}

# trap keyboard interrupt (control-c)

cd $CDIR

#trap control_c SIGINT
#./dummy_worker_stream.sh worker0 5000 &
#./dummy_worker_stream.sh worker1 5001 &
CAPSSTRING='\"Z01AFuygUB7YCIAAAAMAu5rKAAeLFss\\=\\,aOvssg\\=\\=\"' # 640x480
#CAPSSTRING='\"Z0KAHukDwXvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 480x360
CAPSSTRING='\"Z0KAHukCg+QgAAB9AAAdTACAAA\\=\\=\\,aM48gAA\\=\"' # 320x240
#CAPSSTRING='\"Z0KAHukDQYvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 376x416
#CAPSSTRING='\"Z0KAHukDwXvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' #480x360 
#CAPSSTRING='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' #640x480
#CAPSSTRING='\"Z0KAHukC4XsssIAAAfQAAHUwAgA\\=\\,aM48gAA\\=\"' #360X360
#CAPSSTRING='\"Z01AFeygoP2AiAAAAwALuaygAHixbLA\\=\\,aOvssg\\=\\=\"' # testpattern

#CAPSSTRING='\"Z0KAHukDwXvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 480x360
CAPSSTRING0='\"Z0KAHukCg+QgAAB9AAAdTACAAA\\=\\=\\,aM48gAA\\=\"' # 320x240
CAPSSTRING0='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' # 640 480
#CAPSSTRING1='\"Z01ADOygoP2AiAAAAwALuaygAHihTLA\\=\\,aOvssg\\=\\=\"' # 320x240 testpattern
CAPSSTRING1='\"Z0KAHukDQYvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 376x416
CAPSSTRING1='\"Z01ADOygoP2AiAAAAwALuaygAHihTLA\\=\\,aOvssg\\=\\=\"' # 320x240 testpattern
CAPSSTRING1='\"Z01AFeygoP2AiAAAAwALuaygAHixbLA\\=\\,aOvssg\\=\\=\"' # 320 x240 testpattern
CAPSSTRING1='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' # 640 480
CAPSSTRING2='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' # 640 480
CAPSSTRING3='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' # 640 480
CAPSSTRINGTEST='\"Z01AFeygoP2AiAAAAwALuaygAHixbLA\\=\\,aOvssg\\=\\=\"' # 320 x240 testpattern

${WORKERSERVERBIN} worker0 5000 8090 8091 8092 8093 /dev/video0 n H264 640 480 "${CAPSSTRING0}" &
${WORKERSERVERBIN} worker1 5001 8190 8191 8192 8193 /dev/video1 n H264 640 480 "${CAPSSTRING1}" &
${WORKERSERVERBIN} worker2 5002 8290 8291 8292 8293 /dev/video2 n H264 640 480 "${CAPSSTRING2}" &
#${WORKERSERVERBIN} worker2 5002 8290 8291 8292 8293 /dev/video2 n  MJPEG 640 480
${WORKERSERVERBIN} worker3 5003 8390 8391 8392 8393 /dev/video3 n H264 640 480 "${CAPSSTRING3}" &
#${WORKERSERVERBIN} worker4 5004 8490 8491 8492 8493 /dev/video4 n MJPEG 640 480 &
#${WORKERSERVERBIN} worker4 5004 8490 8491 8492 8493 /dev/video4 n MJPEG 1280 960 &
${WORKERSERVERBINNOREC} test 5009 8990 8991 8992 8993 /dev/video5 n H264 640 480 "${CAPSSTRINGTEST}"  &
./dummy_worker_stream.sh teststream 5009 &
trap control_c SIGINT
wait



