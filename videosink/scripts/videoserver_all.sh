#!/bin/bash

# $1 Ptu MAC address
# $2 Serving Slot ID 
# $3 Session ID
# $4 Log directory

umask 0000

export GST_DEBUG=2,cog:0

LOGFILENAME=`date +"%y%m%d-%H%M%S"`.log

echo "-------------------- SESSION START (`date`) --------------------" >> ${4}/${LOGFILENAME}

/usr/local/videoserver/videoserver.sh ${1} ${2} ${3} ${4}/${LOGFILENAME} >> ${4}/${LOGFILENAME} 2>&1

echo "******************** SESSION END (`date`) ********************" >> ${4}/${LOGFILENAME}

unset GST_DEBUG
