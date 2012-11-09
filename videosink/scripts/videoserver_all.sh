#!/bin/bash

# $1 Ptu MAC address
# $2 Serving Slot ID 
# $3 Session ID

/usr/local/videoserver/videoserver.sh $1 $2 $3  > /tmp/videoserver.log 2>&1
