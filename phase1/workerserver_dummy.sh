#!/bin/bash
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
CDIR=`dirname $0`
echo $$
control_c()
# run if user hits control-c
{
  echo -en "\nExiting\n"
	echo kill -SIGINT -$$
#	kill -SIGTERM -$$
#	sleep 5
	kill -SIGKILL -$$
   exit 0
}

# trap keyboard interrupt (control-c)

cd $CDIR

trap control_c SIGINT
./dummy_worker_stream.sh worker0 5000 &
./dummy_worker_stream.sh worker1 5001 &

./workerserver.sh worker0 5000 8090 8091 8092 8093 /dev/video0 &
./workerserver.sh worker1 5001 8190 8191 8192 8193 /dev/video1 &
trap control_c SIGINT
wait



