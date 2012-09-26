#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
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
CAPSSTRING1='\"Z0KAHukCg+QgAAB9AAAdTACAAA\\=\\=\\,aM48gAA\\=\"' # 320x240
CAPSSTRING1='\"Z0KAHukBQHpCAAAH0AAB1MAIAA\\=\\=\\,aM48gAA\\=\"' # 640 480
#CAPSSTRING1='\"Z01ADOygoP2AiAAAAwALuaygAHihTLA\\=\\,aOvssg\\=\\=\"' # 320x240 testpattern
CAPSSTRING2='\"Z0KAHukDQYvLCAAAH0AAB1MAIAA\\=\\,aM48gAA\\=\"' # 376x416
CAPSSTRING2='\"Z01ADOygoP2AiAAAAwALuaygAHihTLA\\=\\,aOvssg\\=\\=\"' # 320x240 testpattern
CAPSSTRING2='\"Z01AFeygoP2AiAAAAwALuaygAHixbLA\\=\\,aOvssg\\=\\=\"' # 320 x240 testpattern

./workerserver.sh worker1 5000 8090 8091 8092 8093 /dev/video0 n H264 "${CAPSSTRING1}" &
./workerserver.sh worker2 5001 8190 8191 8192 8193 /dev/video1 n H264 "${CAPSSTRING2}" &
./dummy_worker_stream.sh worker2 5001 &
trap control_c SIGINT
wait



