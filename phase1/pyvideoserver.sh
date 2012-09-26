#!/usr/bin/python
import gobject #; gobject.threads_init()
import pygtk, gtk
import sys, os
import pygst
import time
pygst.require("0.10")
import re
import gst
import select 
import socket 
import threading
from optparse import OptionParser

def recv_timeout(the_socket,the_size,timeout=2):
#	the_socket.setblocking(0)
	the_socket.settimeout(timeout)
#	total_data=[];data='';begin=time.time()
#	while 1:
#if you got some data, then break after wait sec
#		if total_data and time.time()-begin>timeout:
#			break
#if you got no data at all, wait a little longer
#		elif time.time()-begin>timeout*2:
#			break
#		try:
#			data=the_socket.recv(the_size)
#			if data:
#				total_data.append(data)
#				begin=time.time()
#			else:
#				time.sleep(0.1)
#		except:
#			pass
#	return ''.join(total_data)
	data=the_socket.recv(the_size)
	return data



class HandleCommand:
	def __init__(self): 
		init=1
	def commdict(self,strr,optargs):
		return { 
			'stop' : lambda argg :  self.stop(),
			'overlay' : lambda argg :  self.overlay(argg),
			'input' : lambda argg :  self.inputselector(argg),
			'drop' : lambda argg :  self.drop(argg),
			'play' : lambda argg :  self.play(),
			'quit' : lambda argg :  self.quit(),
			'nop'  : lambda argg : self.nop(),
		     	'echo'  : lambda argg : self.echo(argg),
		}[strr](optargs)
	def overlay(self,optargs):
#		optpairs=re.split(' |,',optpair)
		optpairs=str.split(optargs)
		for optpair in optpairs:
			ppair=re.split('=',optpair)
			{
				'location' : lambda argg : G.setoverlay('location',argg)  ,
				'alpha' : lambda argg : G.setoverlay('alpha',argg)  ,
				'xpos' : lambda argg : G.setoverlay('xpos',argg)  ,
				'ypos' : lambda argg : G.setoverlay('ypos',argg)  ,
				'x' : lambda argg : G.setoverlay('x',argg)  ,
				'y' : lambda argg : G.setoverlay('y',argg)  ,
				'x2' : lambda argg : G.setoverlay('x2',argg)  ,
				'y2' : lambda argg : G.setoverlay('y2',argg)  ,
				'domove' : lambda argg : G.setoverlay('domove',argg)  ,
			}[ppair[0]](ppair[1])

		return 0
	def inputselector(self,inputsource):
		G.set_inputselector(inputsource)
		return 0
	def drop(self,setdrop):
		G.set_drop(setdrop)
		return 0
	def stop(self):
		G.start_stop_stream("Stop")
		return 0
	def play(self):
		G.start_stop_stream("Start")
		return 0
	def nop(self):
		return 0
	def quit(self):
		print "quit"
		return -1
	def echo(self,str):
		print str
		return 0
	def handle(self,comm): 
		print "Got" + comm
		return 1
	def stringdecode(self,strbuf): 
		commands=re.split(';|:|\n|\r',strbuf)
		for commandline in commands:
			if commandline:
				comm=commandline.split()	
#				optargs=join(comm[1:-1])
				optlength=len(comm)
				optargs=' '.join(comm[1:optlength])
#				print "command: " + comm[0] + "|" + optargs

				try:
					a=1
#					self.commdict['quit'](optargs)
					ret=self.commdict(comm[0],optargs)
					if ret == -1 :
						print "will quit"
#						cs.close()
#						os.kill(os.getpid(), signal.SIGTERM)
#						os.kill(os.getgid(), signal.SIGTERM)
#						sys.exit(1)

				except:
					print "invalid command"
		return 1

hc=HandleCommand()	

class Server(threading.Thread): 
	def __init__(self): 
		threading.Thread.__init__(self)
		self.host = '' 
		self.port = 50000 
		self.backlog = 5 
		self.size = 1024 
		self.server = None 
		self.threads = [] 
		self.running = 0

	def open_socket(self): 
		try: 
			self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
			self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
			self.server.bind((self.host,self.port)) 
			self.server.listen(5) 
		except socket.error, (value,message): 
			if self.server: 
				self.server.close() 
			print "Could not open socket: " + message 
			sys.exit(1) 

	def run(self): 
		self.open_socket() 
		input = [self.server,sys.stdin] 
		self.running = 1 
		while self.running: 
#			print "select"
			inputready,outputready,exceptready = select.select(input,[],[],1) 
#			print "selected"
			for s in inputready: 
				if s == self.server: 
# handle the server socket 
       					c = Client(self.server.accept()) 
					c.start() 
					self.threads.append(c) 

				elif s == sys.stdin: 
# handle standard input 
					junk = sys.stdin.readline() 
					res = hc.stringdecode(junk)
					if res == 0:
						self.running = 0 
		print "server close"				
#		self.server.shutdown(1) 
		self.server.close() 
		self.running=-1

# close all threads 

	def close(self):
		self.running=0
#		print "join"
		for c in self.threads: 
			print "client close"				
#			c.shutdown(1) 
			c.close() 
			c.join() 
		while self.running == 0:
			time.sleep(0.1)
#		print "joined"

class Client(threading.Thread): 
	def __init__(self,(client,address)): 
		threading.Thread.__init__(self) 
		self.client = client 
		self.address = address 
		self.size = 1024 
		self.running=0

	def close(self): 
		self.running=0
		self.client.close() 
	def run(self): 
		self.running = 1 
		while self.running: 
			try:
				data = recv_timeout(self.client,self.size) 
				res = hc.stringdecode(data)
				if res == -1:
					print "-1"
					self.running = 0 
#				self.client.send(data) 
			except socket.timeout:
			  	continue
#			except:
			except socket.error, (value,message): 
				print "socket error: " + message
				self.running = 0 
				break  
			except : 
				print "Unexpected error: ", sys.exc_info()[0]
				self.running = 0 
				break  
		self.client.close() 

class CommandServer:
	def __init__(self):
	        self.threads = [] 
	def run(self):
       		s = Server() 
		s.start() 
		self.threads.append(s) 
	def close(self):
		for s in self.threads: 
			s.close();
			s.join() 

class videoserver:
	def init_vars(self,options):
		self.videodev=options.videodev
		self.streamname=options.streamname
		self.caps=options.caps
		self.scalewidth=options.scalewidth
		self.scaleheight=options.scaleheight
		self.enc=options.enc
		self.port=options.port
		self.framerate=options.framerate
		self.dovideorate=options.dovideorate
		self.dofacedetect=options.dofacedetect
		if self.enc == "mjpeg":
			srccaps = "application/x-rtp,media=video,clock-rate=90000,encoding-name=JPEG,payload=96,framerate="+self.framerate
			self.depayloader = "rtpjpegdepay"
			self.decoder = "jpegdec"
			videoratecaps = gst.Caps("image/jpeg,framerate=" + self.framerate)
#			videoratecaps = gst.Caps("video/x-jpeg,framerate=" + self.framerate)

		else:
			srccaps = "application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96,framerate="+self.framerate+",sprop-parameter-sets="+self.caps
			self.depayloader = "rtph264depay"
			self.decoder = "ffdec_h264"
			videoratecaps = gst.Caps("video/x-raw-yuv,framerate=" + self.framerate)
		self.netsrc='udpsrc caps="' + srccaps + '" port=' + str(self.port) + ' '
		if self.dovideorate == True:
			self.videorate=' !  videorate ! video/x-raw-yuv,framerate=' + self.framerate + '  '
		else:
			self.videorate=' '
		print self.videodev

		print self.netsrc
	def parse_launch(self):
		parse_launch = self.netsrc + ' ! ' + self.depayloader + ' ! ' + self.decoder  + ' ! queue  ' + self.videorate + ' !  timeoverlay !  clockoverlay halign=right valign=bottom shaded-background=true time-format="%H:%M:%S"  !  v4l2sink device=' + self.videodev + ' sync=false' 
		parse_launch = self.netsrc + ' ! ' + self.depayloader + ' ! ' + self.decoder  + ' ! queue  ' + self.videorate + ' ! videoscale ! video/x-raw-yuv,width=' + str(self.scalewidth) + ',height=' + str(self.scaleheight) + ' !  timeoverlay !  clockoverlay halign=right valign=bottom shaded-background=true time-format="%H:%M:%S"  !  v4l2sink device=' + self.videodev + ' sync=false' 
		print parse_launch
		self.player = gst.parse_launch ( parse_launch )
#	p = gst.parse_launch (""" videotestsrc pattern=18 ! ffmpegcolorspace ! v4l2sink device=""" + vs.videodev )

class GTK_Main:
	
	def __init__(self):
		self.xpos2 = 200
		self.ypos2 = 200

	def init_window(self):
		window = gtk.Window(gtk.WINDOW_TOPLEVEL)
		window.set_title("Mpeg2-Player")
		window.set_default_size(500, 400)
		window.connect("destroy", gtk.main_quit, "WM destroy")
		vbox = gtk.VBox()
		window.add(vbox)
		hbox = gtk.HBox()
		vbox.pack_start(hbox, False)
		self.entry = gtk.Entry()
		hbox.add(self.entry)
		self.button = gtk.Button("Start")
		hbox.pack_start(self.button, False)
		self.button.connect("clicked", self.start_stop)
		self.movie_window = gtk.DrawingArea()
		vbox.add(self.movie_window)
		window.show_all()

	def init_vars(self,options):
		self.videodev=options.videodev
		self.streamname=options.streamname
		self.caps=options.caps
		self.enc=options.enc
		self.port=options.port
		self.framerate=options.framerate
		self.dovideorate=options.dovideorate
		self.dofacedetect=options.dofacedetect
		print self.videodev

	def init_pipeline(self):
		self.player = gst.Pipeline("player")
#source = gst.element_factory_make("v4l2src", "file-source")
		testsource = gst.element_factory_make("videotestsrc", "testsource")
		testsource.set_property("is-live", False)
		testsource.set_property("pattern", 1)
		self.testsource=testsource
		source = gst.element_factory_make("udpsrc", "udpsource")
		if self.enc == "mjpeg":
			srccaps="application/x-rtp,media=video,encoding-name=JPEG,payload=96,framerate="+self.framerate
			depayloader= gst.element_factory_make("rtpjpegdepay", "rtp-depayloader")
			decoder= gst.element_factory_make("jpegdec", "decoder")
			videoratecaps = gst.Caps("image/jpeg,framerate=" + self.framerate)
#			videoratecaps = gst.Caps("video/x-jpeg,framerate=" + self.framerate)

		else:
			srccaps="application/x-rtp,media=video,encoding-name=H264,payload=96,framerate="+self.framerate+",sprop-parameter-sets="+self.caps
			depayloader= gst.element_factory_make("rtph264depay", "rtp-depayloader")
			decoder= gst.element_factory_make("ffdec_h264", "decoder")
			videoratecaps = gst.Caps("video/x-raw-yuv,framerate=" + self.framerate)
#print "//////////////////" + srccaps
		source.props.caps = gst.Caps(srccaps)
		source.set_property("port",self.port)
		valve = gst.element_factory_make("valve", "valve")
		valve.set_property("drop", False)
		self.valve=valve
		videorate = gst.element_factory_make("videorate", "videorate")
#		print "----------" + videoratecaps 
#		print  videoratecaps 
		videoratecapsfilter = gst.element_factory_make("capsfilter", "videoratecapsfilter")
		videoratecapsfilter.set_property("caps", videoratecaps)
		demuxer = gst.element_factory_make("mpegdemux", "demuxer")
		demuxer.connect("pad-added", self.demuxer_callback)
#		self.video_decoder = gst.element_factory_make("mpeg2dec", "video-decoder")
#		jpg_decoder = gst.element_factory_make("jpegdec", "jpg-decoder")
#		jpg_source = gst.element_factory_make("multifilesrc", "jpg-source")
#		jpg_source.set_property("location", "/home/chpap/syslinux/syslinux-4.04/sample/syslinux_splash.jpg")
#		self.jpg_source=jpg_source
		png_decoder = gst.element_factory_make("pngdec", "png-decoder")
		png_source = gst.element_factory_make("multifilesrc", "png-source")
		png_source.props.caps = gst.Caps("image/png,framerate=1/1")
		png_source.set_property("location", "/usr/share/app-install/icons/debian-logo.png")
		mixer = gst.element_factory_make("videomixer", "mixer")
#		mixer.connect("pad-added", self.mixer_callback, )
		self.audio_decoder = gst.element_factory_make("mad", "audio-decoder")
		audioconv = gst.element_factory_make("audioconvert", "converter")
		audiosink = gst.element_factory_make("autoaudiosink", "audio-output")
		facedetect= gst.element_factory_make("facedetect", "facedetect")
#		videosink = gst.element_factory_make("autovideosink", "video-output")
#videosink = gst.element_factory_make("ximagesink", "video-output")
		input_selector= gst.element_factory_make("input-selector", "input-selector")
		self.input_selector=input_selector
		videosink = gst.element_factory_make("v4l2sink", "video-output")
		videosink.set_property("sync","false")
		videosink.set_property("device",self.videodev)
		self.queuev2 = gst.element_factory_make("queue", "queuev2")
		self.queuev3 = gst.element_factory_make("queue", "queuev3")
		self.queuea = gst.element_factory_make("queue", "queuea")
		self.queuev = gst.element_factory_make("queue", "queuev")
		ffmpeg1 = gst.element_factory_make("ffmpegcolorspace", "ffmpeg1")
		ffmpeg2 = gst.element_factory_make("ffmpegcolorspace", "ffmpeg2")
		ffmpeg3 = gst.element_factory_make("ffmpegcolorspace", "ffmpeg3")
		ffmpeg4 = gst.element_factory_make("ffmpegcolorspace", "ffmpeg4")
		ffmpeg5 = gst.element_factory_make("ffmpegcolorspace", "ffmpeg5")
		ffmpeg6 = gst.element_factory_make("ffmpegcolorspace", "ffmpeg6")
		videobox = gst.element_factory_make("videobox", "videobox")
		self.videobox=videobox
		tee1 = gst.element_factory_make("tee", "tee1")
		alphacolor = gst.element_factory_make("alpha", "alphacolor")
		alphacolor2 = gst.element_factory_make("alpha", "alphacolor2")
		alphacolor3 = gst.element_factory_make("alpha", "alphacolor3")
		
		self.player.add(source, mixer, videosink,  self.queuev, self.queuev2, ffmpeg1, ffmpeg2 ,  videobox ,alphacolor, alphacolor2, ffmpeg3, ffmpeg4, depayloader, decoder ,  videorate, videoratecapsfilter, facedetect,ffmpeg5)
		self.player.add( png_source, png_decoder, input_selector,self.queuev3)
		self.player.add( tee1 , alphacolor3, ffmpeg6)
#		self.player.add(source, mixer, videosink,  self.queuev, ffmpeg1, ffmpeg2 , jpg_source, jpg_decoder, videobox, alphacolor, ffmpeg3)
#		self.player.add(source, demuxer, self.video_decoder, png_decoder, png_source, mixer,
#			self.audio_decoder, audioconv, audiosink, videosink, self.queuea, self.queuev,
#			ffmpeg1, ffmpeg2, ffmpeg3, videobox, alphacolor)
#		gst.element_link_many(source, demuxer)
#		gst.element_link_many(self.queuev, self.video_decoder, ffmpeg1, mixer, ffmpeg2, videosink)
		gst.element_link_many(source,  depayloader ) 
		if self.dovideorate == True:
			gst.element_link_many( depayloader,  self.queuev2, videorate , videoratecapsfilter , decoder,   tee1 , self.queuev) 
		else:
			gst.element_link_many( depayloader,  decoder, tee1,  self.queuev)
		if self.dofacedetect == True:
			gst.element_link_many(self.queuev ,   ffmpeg4 , facedetect , ffmpeg5 ,   alphacolor2 ,  ffmpeg1, input_selector,  mixer, ffmpeg2, videosink)
		else:
			gst.element_link_many(self.queuev ,   ffmpeg4 ,  alphacolor2 ,  ffmpeg1, input_selector,  mixer, ffmpeg2, videosink)
#		gst.element_link_many(jpg_source, jpg_decoder, alphacolor, ffmpeg3, videobox, mixer)
#		gst.element_link_many(self.queuea, self.audio_decoder, audioconv, audiosink)
		gst.element_link_many(png_source, png_decoder, alphacolor, ffmpeg3, videobox, mixer)

		self.player.add(self.testsource, self.valve)
		gst.element_link_many(self.testsource,self.valve,self.queuev3,self.input_selector)
#		self.testsource.set_property("is-live", True)
#		gst.element_link_many(tee1 , alphacolor3 , ffmpeg6 ,  mixer)

#		self.player.add(self.testsource, valve)
#		gst.element_link_many(self.testsource,valve,self.input_selector)
		
		bus = self.player.get_bus()
		bus.add_signal_watch()
		bus.enable_sync_message_emission()
		bus.connect("message", self.on_message)
		bus.connect("sync-message::element", self.on_sync_message)
		
		videobox.set_property("border-alpha", 0)
		videobox.set_property("alpha", 0.5)
		videobox.set_property("left", -10)
		videobox.set_property("top", -10)
		self.running=True
#		self.control1 = gst.Controller( self.player.get_by_name("mixer").get_pad("sink_1") , "xpos", "ypos")
#		self.control1.set_interpolation_mode("xpos", gst.INTERPOLATE_LINEAR)
#		self.control1.set_interpolation_mode("ypos", gst.INTERPOLATE_LINEAR)
#		self.control1.set("xpos", 0 , 0 ); 
#		self.control1.set("ypos", 0 , 0 );
#		self.mixerpad1=self.player.get_by_name("mixer").get_pad("sink_1")
#		self.control1 = gst.Controller(self.mixerpad1, "xpos", "ypos")
#		self.control1.set_interpolation_mode("xpos", gst.INTERPOLATE_LINEAR)
#		self.control1.set_interpolation_mode("ypos", gst.INTERPOLATE_LINEAR)
#		self.control1.set("xpos", 0, 200); self.control1.set("xpos", 0.5 * gst.SECOND , 300);
#		self.control1.set("ypos", 0, 200); self.control1.set("ypos", 0.5 * gst.SECOND , 300);
#		self.control2 = gst.Controller(alphacolor, "alpha")
#		self.control2.set_interpolation_mode("alpha", gst.INTERPOLATE_LINEAR)
#		self.control2.set("alpha", 0, 1.0);self.control1.set("alpha", 0.5 * gst.SECOND , 1.0);
#		self.control1.set("ypos", 5 * gst.SECOND , self.ypos2);

	def setoverlay(self,prop,val):	
#		print str.atof(val)
		try:
			{
				'location' : lambda argg : self.jpg_source.set_property("location", arg) ,
				'alpha' : lambda argg : self.videobox.set_property("alpha",float(argg))  ,
				'xpos' : lambda argg : self.videobox.set_property("left",int(argg))  ,
				'ypos' : lambda argg : self.videobox.set_property("top",int(argg))  ,
				'x' : lambda argg : self.player.get_by_name("mixer").get_pad("sink_1").set_property("xpos", int(argg)) ,
				'y' : lambda argg : self.player.get_by_name("mixer").get_pad("sink_1").set_property("ypos", int(argg)) ,
			}[prop](val)
		except:
			if prop == 'x2':
				self.xpos2 = int(val) 
				return
			if prop == 'y2':
				self.xpos2 = int(val) 
				return
			if prop == 'domove':
				print "domove"
				self.control1 = gst.Controller( self.player.get_by_name("mixer").get_pad("sink_1") , "xpos", "ypos")
				self.control1.set_interpolation_mode("xpos", gst.INTERPOLATE_LINEAR)
				self.control1.set_interpolation_mode("ypos", gst.INTERPOLATE_LINEAR)
				self.control1.set("xpos", 0 , 0 ); self.control1.set("xpos", 5 * gst.SECOND , 200);
				self.control1.set("ypos", 0 , 0 ); self.control1.set("ypos", 5 * gst.SECOND , self.ypos2 );
				time.sleep(7)
#				self.control1 = gst.Controller( self.videobox , "left")
#				self.control1.set_interpolation_mode("left", gst.INTERPOLATE_LINEAR)
#				self.control1.set_interpolation_mode("ypos", gst.INTERPOLATE_LINEAR)
#				self.control1 = gst.Controller(self.mixerpad1, "xpos", "ypos")
#				print self.control1
#				self.control1.set("xpos", 0, 100);
#				self.control1.set("ypos", 0, 100);
#				self.control1.set("left", 5 * gst.SECOND , -100 );
#				self.control1.set("alpha", 5 * gst.SECOND , 0);
#				self.control1.set("ypos", 0 , 100 );
#				control1.set("ypos", 0 , 0 ); control1.set("ypos", 5 * gst.SECOND , self.ypos2 );
				print "domoved"
				return
			pass
#'y2' : lambda argg : self.player.get_by_name("mixer").get_pad("sink_1").set_property("ypos2", int(argg)) ,
#		print self.player.get_by_name("mixer").get_pad("sink_1").set_property("xpos",20)
	def set_inputselector(self,source):
		print "set input_selector"
		print source
		if int(source) == -1:
			print "link testsrc"
#			self.player.set_state(gst.STATE_NULL)
#			self.player.add(self.testsource, self.valve)
#			gst.element_link_many(self.testsource,self.valve,self.input_selector)

#	self.testsource.set_property("is-live", True)
#			self.player.set_state(gst.STATE_PLAYING)

		switch = self.player.get_by_name("input-selector")
		for padno, pad in enumerate([p for p in switch.pads() if p.get_direction() == gst.PAD_SINK]):
			print padno
			print pad.get_name()
			if padno == int(source):
				print "switch"
#				self.player.set_state(gst.STATE_NULL)
				stop_time = switch.emit('block')
#			        newpad = switch.get_static_pad(pad.get_name())
				newpad = pad
				start_time = newpad.get_property('running-time')
				switch.emit('switch', newpad, stop_time, start_time)
#				self.player.set_state(gst.STATE_PLAYING)
				print "switched"


#		self.valve.set_property("drop",True)
#			self.valve.set_property("drop","false")

	def set_drop(self,dodrop):
		print "set drop"
#	 	gstit=self.player.get_by_name("mixer").sink_pads()
#	 	print gstit.next()
	
	 	
#		self.player.get_by_name("mixer").get_pad("sink_0").set_property("xpos", int(argg))
		self.player.get_by_name("valve").set_property("drop", True)
		if int(dodrop) == 1:
			print "true"
			self.player.get_by_name("valve").set_property("drop", True)
		else:
			print "false"
			self.player.get_by_name("valve").set_property("drop", False)
#		self.valve.set_property("drop",True)
#			self.valve.set_property("drop","false")

	def start_stop_stream(self,action):
		if action == "Start":
			self.player.set_state(gst.STATE_PLAYING)
		else:
			self.player.set_state(gst.STATE_NULL)


	def start_stop(self, w):
		self.start_stop_stream(self.button.get_label())
		if action == "Start":
			filepath = self.entry.get_text()
#			if os.path.isfile(filepath):
#				self.player.get_by_name("file-source").set_property("location", filepath)
			self.button.set_label("Stop")
		else:
			self.button.set_label("Start")
						
	def on_exit(self,  *args ):
		self.player.set_state(gst.STATE_NULL)
		self.running = False
	def on_message(self, bus, message):
		t = message.type
		if t == gst.MESSAGE_EOS:
			self.player.set_state(gst.STATE_NULL)
#			self.button.set_label("Start")
		elif t == gst.MESSAGE_ERROR:
			err, debug = message.parse_error()
			print "Error: %s" % err, debug
			self.player.set_state(gst.STATE_NULL)
#			self.button.set_label("Start")
	
	def on_sync_message(self, bus, message):
		if message.structure is None:
			return
		message_name = message.structure.get_name()
		if message_name == "prepare-xwindow-id":
			imagesink = message.src
			imagesink.set_property("force-aspect-ratio", True)
			imagesink.set_xwindow_id(self.movie_window.window.xid)
	
	def mixer_callback(self, demuxer, pad, target):
	        tpad = target.get_compatible_pad(pad)
		print heheheh
		pad.link(tpad)
	def demuxer_callback(self, demuxer, pad):
		if pad.get_property("template").name_template == "video_%02d":
			queuev_pad = self.queuev.get_pad("sink")
			pad.link(queuev_pad)
		elif pad.get_property("template").name_template == "audio_%02d":
			queuea_pad = self.queuea.get_pad("sink")
			pad.link(queuea_pad)
	def decoder_callback(self, decoder, pad, target):
	        tpad = target.get_compatible_pad(pad)
	        if tpad:
	            pad.link(tpad)
		
try:		
	usage = "usage: %prog [options] arg"
	parser = OptionParser(usage)
	parser.add_option("-r", "--fps", dest="framerate", default="30/1",
			                  help="framerate", metavar="FPS")
	parser.add_option("-p", "--port", dest="port", default=5000,
			                  help="port", metavar="PORT")
	parser.add_option("-e", "--enc", dest="enc", default="mjpeg",
			                  help="encoding", metavar="mjpeg/h264")
#	parser.add_option("-c", "--caps", dest="caps", default="\"Z01AHuygUB7YCIAAAAMAu5rKAAeLFss\\=\\,aOvssg\\=\\=\"",
	parser.add_option("-c", "--caps", dest="caps", default="\"Z0KAHukFCLywgAAB9AAAdTACAA\\=\\=\\,aM48gAA\\=\"",
			                  help="sprop-parameter-sets", metavar="CAPS")
	parser.add_option("-d", "--device", dest="videodev", default="/dev/video0",
			                  help="Video4Linux2 device", metavar="V4L2DEV")
	parser.add_option("-n", "--stream-name", dest="streamname", default="worker0",
			                  help="stream name", metavar="STREAMNAME")
	parser.add_option("-f", "--file", dest="filename",
			                  help="write report to FILE", metavar="FILE")
	parser.add_option("--facedetect", action="store_true", dest="dofacedetect", default=False,
		                    help="drop frames to match videorate")
	parser.add_option("-v", "--enforcevideorate", action="store_true", dest="dovideorate", default=False,
		                    help="drop frames to match videorate")
	parser.add_option("-q", "--quiet", action="store_false", dest="verbose", default=True,
		                    help="don't print status messages to stdout")
	parser.add_option("-W", "--scalewidth",  dest="scalewidth", default="640", metavar="SCALEWIDTH" , help="scaling width after decoding")
	parser.add_option("-H", "--scaleheight",  dest="scaleheight", default="480", metavar="SCALEHEIGHT" , help="scaling height after decoding")
	parser.add_option("-t", "--tcp", action="store_true", dest="tcp", default=False,
		                    help="TCP mode (unimplemented)")


	(options, args) = parser.parse_args()
	loop=gobject.MainLoop()
	gobject.threads_init()
	context = loop.get_context()
	print "parse launch"
	vs=videoserver()
	vs.init_vars(options)
	vs.parse_launch()


#	G=GTK_Main()
##	G.init_window()
#	G.init_vars(options)
#	G.init_pipeline()
#	gtk.gdk.threads_init()

	cs=CommandServer()
	cs.run()	
#	G.start_stop_stream("Start")
	vs.player.set_state (gst.STATE_PLAYING)
	print "playing"

#	gtk.main()
	loop.run()
#	while G.running:
#		context.iteration(True)
except KeyboardInterrupt:
	print "KeyboardInterrupt kkk"
	cs.close()
#	time.sleep(2)
	sys.exit(1)
	pass
