// -------------------------------------------------------------------------------------------------------------
// tcpvideosink.cpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com 
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------


/**
 * @file tcpvideosink.cpp
 * @ingroup tcp_video_sink
 * @brief Gstreamer TCP video sink module.
 * 
 * The file contains the entry point of the video sink. The application defines its configuration mechanism
 * with the use of the <em>program options</em>, of the <em>boost</em> library. The video sink receives video
 * over TCP with the aid of the Gstreamer Data Protocol (GDP).
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

#include "tcpvideosink.hpp"

/**
 * @ingroup tcp_video_sink
 * @brief Gstreamer TCP video sink entry point.
 */
int
main( int argc, char* argv[] )
{
	std::string host_ip;
	boost::mutex mutex;

	int host_port, com_pkgs = 0;
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
		using namespace tcp_video_streamer;

		// configure, setup and parse program options

		std::stringstream sstr;

		sstr << std::endl
			 << "Program Description" << std::endl
			 << "-------------------" << std::endl << std::endl
			 << "A Gstreamer h264 video receiver (video sink). This application " << std::endl
			 << "receives through network a TCP/IP video stream to a selected port " << std::endl
			 << "and propagates it to a virtual pre-configured video device.";

		app_opts::options_description desc ( sstr.str() );
		app_opts::options_description conf_desc( "" );
		app_opts::variables_map opts_map;

		desc.add_options()
			( "help", "produce this message" )
			( "config-path", app_opts::value<std::string>()->default_value( "/etc/tcpvideostream.conf/vsink.conf" )
				, "Sets the configuration file path." )
		;
		
		app_opts::store( app_opts::parse_command_line( argc, argv, desc ), opts_map );
		app_opts::notify( opts_map );

		conf_desc.add_options()
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
			( "capturedevice.name", app_opts::value<std::string>(), "" )
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

		if( !opts_map["help"].empty() )
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		if( opts_map["connection.localhost"].empty()     || opts_map["connection.port"].empty() 
		 || opts_map["localvideosource.pattern"].empty() || opts_map["capturedevice.name"].empty() 
		 || opts_map["timeoverlay.halignment"].empty()   || opts_map["clockoverlay.halignment"].empty()
		 || opts_map["clockoverlay.valignment"].empty()  || opts_map["clockoverlay.shaded-background"].empty() 
		 || opts_map["clockoverlay.time-format"].empty() || opts_map["localvideosource.video-header"].empty()
		 || opts_map["localvideosource.width"].empty() 	 || opts_map["localvideosource.height"].empty() 
		 || opts_map["localvideosource.framerate-num"].empty()  || opts_map["localvideosource.framerate-den"].empty() 
		 || opts_map["localvideosource.fourcc-format0"].empty() || opts_map["localvideosource.fourcc-format1"].empty() 
		 || opts_map["localvideosource.fourcc-format2"].empty() || opts_map["localvideosource.fourcc-format3"].empty() 
		 || opts_map["localvideosource.interlaced"].empty() )
		{
			std::cout << "You must properly set the connection options." <<  std::endl;
			return EXIT_FAILURE;
		}

		host_ip = opts_map["connection.localhost"].as<std::string>();
		host_port = opts_map["connection.port"].as<int>();

		// initialize Gstreamer

		gst_init( &argc, &argv );

		// instantiate pipeline

		gstbin_pt pipeline( GST_BIN( gst_pipeline_new( "sink-flow" ) ), cust_deleter<GstBin>() );

		// construct pipeline

		pipeline << gst_element_factory_make( "tcpserversrc", "tcpsource" )
			 << gst_element_factory_make( "gdpdepay", "gdpdepay")
			 << gst_element_factory_make( "ffdec_h264", "h264dec" )
			 << gst_element_factory_make( "identity", "identity" ) 
			 << gst_element_factory_make( "input-selector", "iselector" )
			 << gst_element_factory_make( "ffmpegcolorspace", "ffmpegcs" )
			 << gst_element_factory_make( "timeoverlay", "timeoverlay" )
			 << gst_element_factory_make( "clockoverlay", "clockoverlay" )
			 << gst_element_factory_make( "tee", "tee" );

		{
			GstPad *src0, *src1;
			
			//src0 = gst_element_get_request_pad( gst_bin_get_by_name( pipeline.get(), "tee" ), "src%d" );
			src1 = gst_element_get_request_pad( gst_bin_get_by_name( pipeline.get(), "tee" ), "src%d" );

			//gst_bin_add( pipeline.get(), gst_element_factory_make( "queue", "queue0" ) );
			gst_bin_add( pipeline.get(), gst_element_factory_make( "queue", "queue1" ) );

			GstPad *queue0_sink, *queue1_sink;
			
			//queue0_sink = gst_element_get_static_pad( gst_bin_get_by_name( pipeline.get(), "queue0" ), "sink" );
			queue1_sink = gst_element_get_static_pad( gst_bin_get_by_name( pipeline.get(), "queue1" ), "sink" );
			
			//gst_pad_link( src0, queue0_sink );
			gst_pad_link( src1, queue1_sink );

			////gst_bin_add( pipeline.get(), gst_element_factory_make( "x264enc", "x264enc" ) );
			//gst_bin_add( pipeline.get(), gst_element_factory_make( "ffenc_mpeg4", "ffenc_mpeg4" ) );
			//gst_bin_add( pipeline.get(), gst_element_factory_make( "mpegtsmux", "mpegtsmux" ) );
			//gst_bin_add( pipeline.get(), gst_element_factory_make( "filesink", "filesink" ) );			
			
			//GstPad *mux_sink = gst_element_get_request_pad( gst_bin_get_by_name( pipeline.get(), "mpegtsmux" ), "sink_%d" );
			////GstPad *enc_src = gst_element_get_static_pad( gst_bin_get_by_name( pipeline.get(), "x264enc" ), "src" );
			//GstPad *enc_src = gst_element_get_static_pad( gst_bin_get_by_name( pipeline.get(), "ffenc_mpeg4" ), "src" );
			
			//gst_pad_link( enc_src, mux_sink );

			//gst_element_link( gst_bin_get_by_name( pipeline.get(), "queue0" )
			//	//, gst_bin_get_by_name( pipeline.get(), "x264enc" ) );
			//	, gst_bin_get_by_name( pipeline.get(), "ffenc_mpeg4" ) );
			//gst_element_link( gst_bin_get_by_name( pipeline.get(), "mpegtsmux" )
			//	, gst_bin_get_by_name( pipeline.get(), "filesink" ) );

			gst_bin_add( pipeline.get(), gst_element_factory_make( "v4l2sink", "v4l2sink" ) );

			gst_element_link( gst_bin_get_by_name( pipeline.get(), "queue1" )
				, gst_bin_get_by_name( pipeline.get(), "v4l2sink" ) );
		}

		// register pipelines' bus handler

		bmsg bmsg_p = boost::make_tuple( loop, gst_bin_get_by_name( pipeline.get(), "iselector" )
			, &loop_flag, &mutex );

		{
			gstbus_pt pipe_bus( gst_pipeline_get_bus( GST_PIPELINE( pipeline.get() ) )
				, cust_deleter<GstBus>() );

			gst_bus_add_watch( pipe_bus.get(), pipeline_bus_handler
				, static_cast<gpointer>( &bmsg_p ) );
		}

		gst_bin_add( pipeline.get(), gst_element_factory_make( "videotestsrc", "videosource" ) );

		insert_filter( gst_bin_get_by_name( pipeline.get(), "videosource" )
			, gst_bin_get_by_name( pipeline.get(), "iselector" )
			, opts_map );

		// set Gstreamer element properties


		//g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "ximagesink" ) )
		//	, "sync", false
		//	, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "tcpsource" ) )
			, "host", opts_map["connection.localhost"].as<std::string>().c_str()
			, "port", opts_map["connection.port"].as<int>()
			, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "videosource" ) )
			, "pattern", opts_map["localvideosource.pattern"].as<int>()
			, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "v4l2sink" ) )
			, "device", opts_map["capturedevice.name"].as<std::string>().c_str() //"/dev/video0"
			, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "timeoverlay" ) )
			, "halignment", opts_map["timeoverlay.halignment"].as<int>() //2
			, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "clockoverlay" ) )
			, "halignment", opts_map["clockoverlay.halignment"].as<int>() //  2
			, "valignment", opts_map["clockoverlay.valignment"].as<int>() // 1
			, "shaded-background", opts_map["clockoverlay.shaded-background"].as<bool>() //true
			, "time-format", opts_map["clockoverlay.time-format"].as<std::string>().c_str() 
			, NULL ); 
		
		//g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "filesink" ) )
		//	, "location", "/home/kostas/workspace/streaming_output/worker0-%06d.ts"
		//	, NULL );		

		{
			auto isel = gst_bin_get_by_name( pipeline.get(), "iselector" );
			g_object_set( G_OBJECT( isel )
				, "active-pad", gst_element_get_static_pad( isel, "sink1" )
				, NULL );
		}

		// register identity's receive callback

		dbmsg identity_param = boost::make_tuple( gst_bin_get_by_name( pipeline.get(), "iselector" )
			, &mutex, &com_pkgs, &loop_flag );

		g_signal_connect( gst_bin_get_by_name( pipeline.get(), "identity" ), "handoff"
			, G_CALLBACK( identity_handler ), static_cast<gpointer>( &identity_param ) );

		// start pipeline

		gst_element_set_state( GST_ELEMENT( pipeline.get() ), GST_STATE_PLAYING );

		// issue a watchdog thread

		auto watch_cfg = boost::bind( watch_loop, gst_bin_get_by_name( pipeline.get(), "iselector" )
			, &loop_flag, &com_pkgs, &mutex );

		boost::thread watch_thread( watch_cfg );

		// start program loop

		g_main_loop_run( loop );

		gst_element_set_state( GST_ELEMENT( pipeline.get() ), GST_STATE_NULL );
	}
	catch( const boost::exception& e )
	{
		std::cerr << boost::diagnostic_information( e );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
