// -------------------------------------------------------------------------------------------------------------
// gst-test_sink.cpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com 
// Last revision : June 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @defgroup nc_tcp_sink Gstreamer TCP video sink
 * @ingroup nc_tcp_video_streamer_prot
 * @brief A Gstreamer TCP video sink prototype.
 */

/**
 * @file gst-test_sink.cpp
 * @ingroup nc_tcp_sink
 * @brief A Gstreamer TCP video sink prototype.
 * 
 * The file contains the entry point of the video sink. The application defines its configuration mechanism
 * with the use of the <em>program options</em>, of the <em>boost</em> library. The video sink receives video
 * over TCP with the aid of the Gstreamer Data Protocol (GDP).
 *
 * @sa <em>Gstreamer API documentation</em>, boost::program_options
 */

#include <iostream>
#include <string>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "gst-test_sink.hpp"

/**
 * @ingroup nc_tcp_sink
 * @brief Gstreamer TCP video sink entry point.
 */
int
main( int argc, char* argv[] )
{
	std::string host_ip;
	boost::mutex mutex;

	int host_port = 5000, com_pkgs = 0;
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
		using namespace nc_tcp_video_streamer_prot;

		// configure, setup and parse program options

		std::stringstream sstr;

		sstr << std::endl
			 << "Program Description" << std::endl
			 << "-------------------" << std::endl << std::endl
			 << "A Gstreamer h264 video receiver (video sink). This application " << std::endl
			 << "receives through network a TCP/IP video stream to a selected port. " << std::endl
			 << "Use for testing purposes";

		app_opts::options_description desc ( sstr.str() );

		desc.add_options()
			( "help", "produce this message" )
			( "localhost", app_opts::value<std::string>(), "local ip address [default:\"\"]" )
			( "port", app_opts::value<int>(), "listening port [default:5000]" )
		;

		app_opts::variables_map opts_map;
		app_opts::store( app_opts::parse_command_line( argc, argv, desc ), opts_map );
		app_opts::notify( opts_map );

		if( opts_map.count( "help" ) )
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		if( opts_map["localhost"].empty() )
		{
			std::cout << "You must supply with a valid ip address for target." <<  std::endl;
			return EXIT_FAILURE;
		}
		else
		{
			host_ip = opts_map["localhost"].as<std::string>();
		}

		if( !opts_map["port"].empty() )
			host_port = opts_map["port"].as<int>();

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
				 << gst_element_factory_make( "ximagesink", "ximagesink" );

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
			, gst_bin_get_by_name( pipeline.get(), "iselector" ) );

		// set Gstreamer element properties


		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "ximagesink" ) )
			, "sync", false
			, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "tcpsource" ) )
			, "host", host_ip.c_str()
			, "port", host_port
			, NULL );

		g_object_set( G_OBJECT( gst_bin_get_by_name( pipeline.get(), "videosource" ) )
			, "pattern", 1
			, NULL );

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
