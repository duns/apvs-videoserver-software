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

using namespace video_streamer;
using namespace video_sink;

auxiliary_libraries::log_level global_log_level = log_debug_2;

/**
 * @ingroup video_sink
 * @brief Gstreamer video sink entry point.
 */
int
main( int argc, char* argv[] )
{
	GMainLoop* loop = g_main_loop_new( NULL, false );

	boost::mutex mutex;
	int com_pkgs = 0;
	bool loop_flag = false;

	if( !loop )
	{
		LOG_CLOG( log_error ) << "Cannot initiate program loop.";
		return EXIT_FAILURE;
	}

	try
	{
		namespace app_opts = boost::program_options;

		std::stringstream sstr;

		sstr << std::endl
			 << "Program Description" << std::endl
			 << "-------------------" << std::endl << std::endl
			 << "A Gstreamer h264 video receiver (video sink). This application" << std::endl
			 << "receives through network a video stream to a selected port and" << std::endl
			 << "propagates it to a virtual pre-configured video device";

		app_opts::options_description desc( sstr.str() ), conf_desc( "" );
		app_opts::variables_map opts_map;

		desc.add_options()
			( "help", "produce this message" )
			( "config-path", app_opts::value<std::string>(), "Sets the configuration file path." )
			( "session-id" , app_opts::value<std::string>(), "Session unique identifier."        )
		;
		
		app_opts::store( app_opts::parse_command_line( argc, argv, desc ), opts_map );
		app_opts::notify( opts_map );

		if( !opts_map["help"].empty() )
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		if( opts_map["config-path"].empty() )
		{
			LOG_CLOG( log_error ) << "You must supply a path for the configuration file.";
			return EXIT_FAILURE;
		}

		if( opts_map["session-id"].empty() )
		{
			LOG_CLOG( log_error ) << "You must supply a valid session identifier.";
			return EXIT_FAILURE;
		}

		conf_desc.add_options()
			( "connection.localhost"           , app_opts::value<std::string>(), "" )
			( "connection.port"                , app_opts::value<int>()        , "" )
			( "localvideosource.pattern"       , app_opts::value<int>()        , "" )
			( "localvideosource.video-header"  , app_opts::value<std::string>(), "" )
			( "localvideosource.width"         , app_opts::value<int>()        , "" )
			( "localvideosource.height"        , app_opts::value<int>()        , "" )
			( "localvideosource.framerate-num" , app_opts::value<int>()        , "" )
			( "localvideosource.framerate-den" , app_opts::value<int>()        , "" )
			( "localvideosource.fourcc-format0", app_opts::value<char>()       , "" )
			( "localvideosource.fourcc-format1", app_opts::value<char>()       , "" )
			( "localvideosource.fourcc-format2", app_opts::value<char>()       , "" )
			( "localvideosource.fourcc-format3", app_opts::value<char>()       , "" )
			( "localvideosource.interlaced"    , app_opts::value<bool>()       , "" )
			( "videobackup.output-path"        , app_opts::value<std::string>(), "" )
			( "capturedevice.name"             , app_opts::value<std::string>(), "" )
			( "imageoverlay.img-location"      , app_opts::value<std::string>(), "" )
			( "timeoverlay.halignment"         , app_opts::value<int>()        , "" )
			( "timeoverlay.font"               , app_opts::value<std::string>(), "" )
			( "execution.messages-detail"      , app_opts::value<int>()        , "" )
			( "execution.watchdog-awareness"   , app_opts::value<int>()        , "" )
		;

		std::ifstream config_file( opts_map["config-path"].as<std::string>().c_str() );

		if( !config_file )
		{
			LOG_CLOG( log_error ) << "Configuration file not found. Exiting...";
			return EXIT_FAILURE;
		}

		app_opts::store( app_opts::parse_config_file( config_file, conf_desc ), opts_map );
		app_opts::notify( opts_map );

		config_file.close();

		bool param_flag = true;
		auto opts_vector = conf_desc.options();
		typedef boost::shared_ptr<app_opts::option_description> podesc_type;

		std::for_each( opts_vector.begin(), opts_vector.end()
				, [&global_log_level, &opts_map, &param_flag] ( podesc_type& arg ) -> void
				{
					auto cur_opt = arg.get()->format_name().substr(2, arg.get()->format_name().length() - 2);
					if( opts_map[cur_opt].empty() )
					{
						LOG_CLOG( log_error ) << "Parameter '" << cur_opt << "' is not properly set.";
						param_flag = false;
					}
				} );

		if( !param_flag )
		{
			LOG_CLOG( log_error ) << "Invalid configuration. Exiting...";
			return EXIT_FAILURE;
		}

		global_log_level = static_cast<auxiliary_libraries::log_level>(
			opts_map["execution.messages-detail"].as<int>() );

		LOG_CLOG( log_info ) << "Initializing Gstreamer...";

		gst_init( &argc, &argv );

		LOG_CLOG( log_info ) << "Constructing pipeline...";

		videosink_pipeline videosink( opts_map );

		if( global_log_level > log_warning )
			GST_DEBUG_BIN_TO_DOT_FILE( GST_BIN( videosink.root_bin.get() )
				, GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS, "pipeline-schema" );

		LOG_CLOG( log_info ) << "Registering callbacks...";

		vs_params c_params = boost::make_tuple( &mutex, &videosink, &com_pkgs, &loop_flag, loop );

		{
			gstbus_pt pipe_bus(	gst_pipeline_get_bus( GST_PIPELINE( videosink.root_bin.get() ) )
				  , cust_deleter<GstBus>() );

			gst_bus_add_watch( pipe_bus.get(), pipeline_bus_handler, static_cast<gpointer>( &c_params ) );
		}

		g_signal_connect( videosink.elements["identity"], "handoff", G_CALLBACK( identity_handler )
			, static_cast<gpointer>( &c_params ) );

		LOG_CLOG( log_info ) << "Initializing watchdog...";

		auto watch_cfg = boost::bind( watch_loop, &videosink, &loop_flag, &com_pkgs, &mutex
			, opts_map["execution.watchdog-awareness"].as<int>() );

		boost::thread watch_thread( watch_cfg );

		LOG_CLOG( log_info ) << "Starting pipeline...";

		gst_element_set_state( GST_ELEMENT( videosink.root_bin.get() ), GST_STATE_PLAYING );

		g_main_loop_run( loop );

		LOG_CLOG( log_info ) << "Destroying pipeline...";

		gst_element_set_state( GST_ELEMENT( videosink.root_bin.get() ), GST_STATE_NULL );
	}
	catch( const boost::exception& e )
	{
		LOG_CLOG( log_debug_1 ) << boost::diagnostic_information( e );
		LOG_CLOG( log_error ) << "Fatal error occurred. Exiting...";

		g_main_loop_quit( loop );

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
