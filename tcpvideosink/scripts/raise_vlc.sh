#!/bin/bash

cvlc --play-and-exit -q  v4l2:///dev/video0 
#cvlc --play-and-exit -q  v4l2:///dev/video0  --sout="#duplicate{dst=\"transcode{width=416,height=376,fps=15,vcodec=mjpg,vb=8000}:standard{access=http{mime=multipart/x-mixed-replace;boundary=--7b3cc56e5f51db803f790dad720ed50a},mux=mpjpeg,dst=:8090/worker0.mjpg\"}}"
