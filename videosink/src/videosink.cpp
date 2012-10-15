// -------------------------------------------------------------------------------------------------------------
// videosink.cpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com 
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------


/**
 * @file videosink.cpp
 * @ingroup video_sink
 * @brief Gstreamer video sink module.
 * 
 * The file contains the entry point of the video sink. The application defines its configuration mechanism
 * with the use of the <em>program options</em>, of the <em>boost</em> library. The video sink receives video
 * over the network with the aid of the Gstreamer Data Protocol (GDP).
 *
 * @sa <em>Gstreamer API documentation</em>, boost::program_options
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "videosink.hpp"

/**
 * @ingroup video_sink
 * @brief Gstreamer video sink entry point.
 */
int
main( int argc, char* argv[] )
{
	boost::mutex mutex;

	int com_pkgs = 0;
	bool loop_flag = false;

	// initialize program loop

	GMainLoop* loop = g_main_loop_new( NULL, false );

	if( !loop )
	{
		std::cerr << "Cannot initiate program loop." << std::cerr;
		return EXIT_FAILURE;
	}

	try
	{
		namespace app_opts = boost::program_options;
		using namespace video_streamer;

		// configure, setup and parse program options

		std::stringstream sstr;

		sstr << std::endl
			 << "Program Description" << std::endl
			 << "-------------------" << std::endl << std::endl
			 << "A Gstreamer h264 video receiver (video sink). This application" << std::endl
			 << "receives through network a video stream to a selected port and" << std::endl
			 << "propagates it to a virtual pre-configured video device";

		app_opts::options_description desc ( sstr.str() );
		app_opts::options_description conf_desc( "" );
		app_opts::variables_map opts_map;

		desc.add_options()
			( "help", "produce this message" )
			( "config-path", app_opts::value<std::string>()->default_value( "/etc/videostream.conf/vsink.conf" )
				, "Sets the configuration file path." )
		;
		
		app_opts::store( app_opts::parse_command_line( argc, argv, desc ), opts_map );
		app_opts::notify( opts_map );

		if( !opts_map["help"].empty() )
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		conf_desc.add_options()
			( "connection.transfer-protocol", app_opts::value<std::string>(), "" )
			( "connection.localhost", app_opts::value<std::string>(), "" )
			( "connection.port", app_opts::value<int>(), "" )
			( "localvideosource.pattern", app_opts::value<int>(), "" )
			( "localvideosource.video-header", app_opts::value<std::string>(), "" )
			( "localvideosource.width", app_opts::value<int>(), "" )
			( "localvideosource.height", app_opts::value<int>(), "" )
			( "localvideosource.framerate-num", app_opts::value<int>(), "" )
			( "localvideosource.framerate-den", app_opts::value<int>(), "" )
			( "localvideosource.fourcc-format0", app_opts::value<char>(), "" )
			( "localvideosource.fourcc-format1", app_opts::value<char>(), "" )
			( "localvideosource.fourcc-format2", app_opts::value<char>(), "" )
			( "localvideosource.fourcc-format3", app_opts::value<char>(), "" )
			( "localvideosource.interlaced", app_opts::value<bool>(), "" )
			( "videobackup.location", app_opts::value<std::string>(), "" )
			( "videobackup.pattern", app_opts::value<std::string>(), "" )
			( "capturedevice.name", app_opts::value<std::string>(), "" )
			( "imageoverlay.location", app_opts::value<std::string>(), "" )
			( "timeoverlay.halignment", app_opts::value<int>(), "" )
			( "clockoverlay.halignment", app_opts::value<int>(), "" )
			( "clockoverlay.valignment", app_opts::value<int>(), "" )
			( "clockoverlay.shaded-background", app_opts::value<bool>(), "" )
			( "clockoverlay.time-format", app_opts::value<std::string>(), "" )
		;

		std::ifstream config_file( opts_map["config-path"].as<std::string>().c_str() );

		if( !config_file )
		{
			std::cout << "Configuration file not found. Exiting..." << std::endl;
			return EXIT_FAILURE;
		}

		app_opts::store( app_opts::parse_config_file( config_file, conf_desc ), opts_map );
		app_opts::notify( opts_map );

		config_file.close();

		if( opts_map["connection.transfer-protocol"].empty()
		 || opts_map["connection.localhost"].empty()     
		 || opts_map["connection.port"].empty() 
		 || opts_map["localvideosource.pattern"].empty() 
	   	 || opts_map["localvideosource.video-header"].empty()
		 || opts_map["localvideosource.width"].empty() 	 
		 || opts_map["localvideosource.height"].empty() 
		 || opts_map["localvideosource.framerate-num"].empty()  
		 || opts_map["localvideosource.framerate-den"].empty() 
		 || opts_map["localvideosource.fourcc-format0"].empty() 
		 || opts_map["localvideosource.fourcc-format1"].empty() 
		 || opts_map["localvideosource.fourcc-format2"].empty() 
		 || opts_map["localvideosource.fourcc-format3"].empty() 
		 || opts_map["localvideosource.interlaced"].empty() 
		 || opts_map["videobackup.location"].empty()
		 || opts_map["videobackup.pattern"].empty() 
		 || opts_map["capturedevice.name"].empty() 
		 || opts_map["imageoverlay.location"].empty() 
		 || opts_map["timeoverlay.halignment"].empty()   
		 || opts_map["clockoverlay.halignment"].empty()
		 || opts_map["clockoverlay.valignment"].empty()  
		 || opts_map["clockoverlay.shaded-background"].empty() 
		 || opts_map["clockoverlay.time-format"].empty() ) 
		{
			std::cout << "You must properly set the configuration options." <<  std::endl;
			return EXIT_FAILURE;
		}

		// initialize Gstreamer

		gst_init( &argc, &argv );

		// instantiate pipeline

		gstbin_pt pipeline( GST_BIN( gst_pipeline_new( "tcpvideosink" ) ), cust_deleter<GstBin>() );
		
		element_set elements( opts_map );
		
		gst_bin_add_many( pipeline.get()
			, elements.h264dec,       elements.identity,      elements.iselector
			, elements.ffmpegcs0,     elements.timeoverlay,   elements.clockoverlay
			, elements.networksource, elements.gdpdepay,      elements.tee
			, elements.videosource,   elements.v4l2sink,      elements.avimux
			, elements.filesink,      elements.coglogoinsert, elements.queue0
			, elements.queue1
			, NULL );	

		gst_element_link_many( elements.networksource, elements.gdpdepay, elements.tee, NULL );

		gst_element_link_many( elements.queue1, elements.h264dec, elements.identity
			, elements.ffmpegcs0, elements.timeoverlay, elements.clockoverlay
			, elements.coglogoinsert, elements.iselector, elements.v4l2sink, NULL );
		
		gst_element_link( elements.avimux, elements.filesink );
		insert_filter( elements.videosource, elements.iselector, opts_map );

		gst_element_link( elements.avimux, elements.filesink );	
	
		elements.sel_sink0 = gst_element_get_static_pad( elements.iselector, "sink0" );
		elements.sel_sink1 = gst_element_get_static_pad( elements.iselector, "sink1" );	
		
		GstPad *tee_src0, *tee_src1;
		GstPad *queue0_src, *queue0_sink, *queue1_sink;
		GstPad *avimux_sink;

		tee_src0 = gst_element_get_request_pad( elements.tee, "src%d" );
		tee_src1 = gst_element_get_request_pad( elements.tee, "src%d" );
		queue0_sink = gst_element_get_static_pad( elements.queue0, "sink" );
		queue0_src = gst_element_get_static_pad( elements.queue0, "src" );
		queue1_sink = gst_element_get_static_pad( elements.queue1, "sink" );
		avimux_sink = gst_element_get_request_pad( elements.avimux, "video_%d" );

		gst_pad_link( tee_src0, queue0_sink );
		gst_pad_link( tee_src1, queue1_sink ); 
		gst_pad_link( queue0_src, avimux_sink );

		gst_object_unref( tee_src0 );
		gst_object_unref( tee_src1 );
		gst_object_unref( queue0_sink );
		gst_object_unref( queue0_src );
		gst_object_unref( queue1_sink );
		gst_object_unref( avimux_sink );
	
		GST_DEBUG_BIN_TO_DOT_FILE( GST_BIN( pipeline.get() ), GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS, "init" );

		// register pipelines' bus handler

		bmsg bmsg_p = boost::make_tuple( loop, &elements, &loop_flag, &mutex );

		{
			gstbus_pt pipe_bus( gst_pipeline_get_bus( GST_PIPELINE( pipeline.get() ) )
				, cust_deleter<GstBus>() );

			gst_bus_add_watch( pipe_bus.get(), pipeline_bus_handler
				, static_cast<gpointer>( &bmsg_p ) );
		}

		// register identity's receive callback

		dbmsg identity_param = boost::make_tuple( &mutex, &com_pkgs, &loop_flag );

		g_signal_connect( elements.identity, "handoff", G_CALLBACK( identity_handler )
			, static_cast<gpointer>( &identity_param ) );

		// start pipeline

		gst_element_set_state( GST_ELEMENT( pipeline.get() ), GST_STATE_PLAYING );

		// issue a watchdog thread

		auto watch_cfg = boost::bind( watch_loop, &elements, &loop_flag, &com_pkgs, &mutex );

		boost::thread watch_thread( watch_cfg );

		// start program loop

		g_main_loop_run( loop );

		gst_object_unref( elements.sel_sink0 );
		gst_object_unref( elements.sel_sink1 );
	
		gst_element_set_state( GST_ELEMENT( pipeline.get() ), GST_STATE_NULL );
	}
	catch( const boost::exception& e )
	{
		std::cerr << boost::diagnostic_information( e );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
