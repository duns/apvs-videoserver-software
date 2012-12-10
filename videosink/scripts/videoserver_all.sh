#!/bin/bash
[ -n "$1" ] && exec >> "$1"  2>&1
umask 0000
control_c()
{
/etc/init.d/videoserver_single stop 0
/etc/init.d/videoserver_single stop 1
/etc/init.d/videoserver_single stop 2
/etc/init.d/videoserver_single stop 3
exit 0
#echo PARENT $$
#PGID=`ps --no-heading -p $$ -o "%r"| awk '{print $1}'`
#kill -INT -$PGID
}
idle()
{
while true;
do
sleep 10
done
}

export GST_DEBUG=2,cog:0
trap control_c SIGINT

/etc/init.d/videoserver_single start 0
/etc/init.d/videoserver_single start 1
/etc/init.d/videoserver_single start 2
/etc/init.d/videoserver_single start 3
#/usr/local/videoserver/videoserver_top.sh 0 /var/log/videoserver &
#/usr/local/videoserver/videoserver_top_testing.sh 0 /var/log/videoserver &
#/usr/local/videoserver/videoserver_top_testing.sh 1 /var/log/videoserver &
#/usr/local/videoserver/videoserver_top_testing.sh 2 /var/log/videoserver &
#/usr/local/videoserver/videoserver_top_testing.sh 3 /var/log/videoserver 
idle 
unset GST_DEBUG
