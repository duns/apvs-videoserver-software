// -------------------------------------------------------------------------------------------------------------
// videosink.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @defgroup video_sink Gstreamer video sink
 * @ingroup server-modules
 * @brief Gstreamer video sink module.
 */

/**
 * @file videosink.hpp
 * @ingroup video_sink
 * @brief Type declarations and global function definitions for video streaming sink module.
 */

#ifndef VIDEOSINK_HPP
#define VIDEOSINK_HPP

#include <gstreamermm.h>
#include <boost/tuple/tuple.hpp>
#include <boost/thread.hpp>

#include "log.hpp"
#include "videostreamer_common.hpp"
#include "videosink_pipeline.hpp"

namespace video_sink
{
	using namespace auxiliary_libraries;
	using namespace video_streamer;

	/**
 	 * @ingroup video_sink
	 * @brief Type of pipeline's callback parameters.
	 */
	typedef boost::tuple<boost::mutex*, videosink_pipeline*, int*, bool*, GMainLoop*> vs_params;

	/**
 	 * @ingroup video_sink
	 * @brief Identity 'receive-buffer' handler
     * This callback is being invoked every time a data buffer passes through the Gstreamer Identity
	 * element. The signal besides a reference of the data buffer can also carry arbitrary user
	 * specified data.
	 * 
	 * @sa <a href="gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer-plugins/html/gstreamer
-plugins-identity.html">Identity element documentation. </a>
	 *
	 * @param identity reference to the element who's receive event is being handled
	 * @param buffer data buffer currently being passed through the element
	 * @param user_data reference to the user specified associated data 
	 */
	static void
	identity_handler( GstIdentity* identity, GstBuffer* buffer, gpointer user_data )
	{
		vs_params* m_data = static_cast<vs_params*>( user_data );

		boost::lock_guard<boost::mutex> lock( *m_data->get<0>() );

		if( !*m_data->get<3>() )
		{
			LOG_CLOG( log_info ) << "Network activity observed...";
			GstBus* bus = gst_pipeline_get_bus( GST_PIPELINE( m_data->get<1>()->root_bin.get() ) );
			GstMessage* msg_switch = gst_message_new_custom(
				GST_MESSAGE_ELEMENT, GST_OBJECT( identity )
			  , gst_structure_new( "switch_stream", "p", G_TYPE_STRING, "p", NULL ) );
			gst_bus_post( bus, msg_switch );
			gst_object_unref( GST_OBJECT( bus ) );
		}

		*m_data->get<2>()+= 1;
	}

	/**
 	 * @ingroup video_sink
	 * @brief Gstreamer bus message callback
	 * This function is registered during pipeline initialization as the pipeline's bus message handler.
	 * The handler is being invoked every time the pipeline publishes a message on the pipeline bus.
	 * Messages can be initialization, validation or even error messages. All of these are declared in
	 * Gstreamer API. During callback registration the user is able to associate any arbitrary data with
	 * the messages.
	 *
	 * @sa <em>Gstreamer API Documentation</em>
	 *
	 * @param bus the message bus of a Gstreamer pipeline
	 * @param msg the published message
	 * @param data associated data 	 
	 * @returns a flag that denotes whether the message have been handled or not (in the latter case
	 * the handler is being reinvoked by the pipeline bus)
	 * @throws video_streamer::bus_error in case of unrecoverable error during streaming.
	 */
	static gboolean
	pipeline_bus_handler( GstBus* bus, GstMessage* msg, gpointer data )
	{
		vs_params* m_data = static_cast<vs_params*>( data );
		
		boost::lock_guard<boost::mutex> lock( *m_data->get<0>() );

		switch( GST_MESSAGE_TYPE( msg ) )
		{

		case GST_MESSAGE_ERROR:
		{
			GError*err;
			gchar *err_str;
			std::string err_msg;

			gst_message_parse_error( msg, &err, &err_str );
			err_msg = std::string( err_str ) + std::string( " *-* " ) + std::string( err->message );

			g_error_free( err );
			g_free( err_str );

			LOG_CLOG( log_error ) << "'" << msg->src->name << "' threw a: " << GST_MESSAGE_TYPE_NAME( msg );
			gst_element_set_state( GST_ELEMENT( m_data->get<1>()->root_bin.get() ), GST_STATE_NULL );
			BOOST_THROW_EXCEPTION( bus_error() << bus_info( err_msg ) );
		}
		break;
		case GST_MESSAGE_ELEMENT:
		{
			if( !strcmp( msg->src->name, "identity" ) )
			{
				if( !strcmp( gst_structure_get_name( gst_message_get_structure( msg ) ), "switch_stream" ) )
				{
					LOG_CLOG( log_info ) << "Setting selected elements to NULL...";

					gst_element_set_state( GST_ELEMENT( msg->src ), GST_STATE_NULL );
					gst_element_set_state( m_data->get<1>()->elements["iselector"], GST_STATE_NULL );
					gst_element_set_state( m_data->get<1>()->elements["ffmpegcs0"], GST_STATE_NULL );

					LOG_CLOG( log_info ) << "Switching pads...";

					g_object_set( G_OBJECT( m_data->get<1>()->elements["iselector"] )
						, "active-pad", m_data->get<1>()->pads["sel_sink0"]
						, NULL );

					LOG_CLOG( log_info ) << "Restarting pipeline...";

					if( global_log_level > log_debug_0 )
						GST_DEBUG_BIN_TO_DOT_FILE( m_data->get<1>()->root_bin.get()
							, GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS, "net-flow" );

					gst_element_set_state( GST_ELEMENT( m_data->get<1>()->root_bin.get() ), GST_STATE_PLAYING );
				
					*m_data->get<3>() = true;
				}
				else if( !strcmp( gst_structure_get_name( gst_message_get_structure( msg ) ), "hard_reset" ) )
				{
					LOG_CLOG( log_info ) << "Hard reset issued.";
					g_main_loop_quit( m_data->get<4>() );
				}
			}
		}
		break;
		default:
			LOG_CLOG( log_debug_0 ) << "'" << msg->src->name << "' threw a: " << GST_MESSAGE_TYPE_NAME( msg );
		break;
		}

		return true;
	}

	/**
 	 * @ingroup video_sink
	 * @brief Watch-dog thread loop 
	 *
	 * The watch-dog thread periodically tests the loop flag along with the number of data packets 
	 * received in the time space between current and last check, in order to determine whether the 
	 * connection is lost  or its quality has been dramatically deteriorated. If one of the previous 
	 * mentioned conditions is true it switches from the network stream to a dummy one and resets the
	 * pipeline to defaults.
	 *
	 * @sa <a href="gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer-plugins/html/gstreamer
-plugins-input-selector.html">Gstreamer input-selector element</a>
	 *
	 * @param pipeline a pointer to the video streaming pipeline
	 * @param loop_flag a pointer to a loop flag provided by the executing pipeline
	 * @param com_pkgs a pointer to the number of packets received between current and last check
	 * @param mutex a pointer to a mutex provided by the executing pipeline
	 * @param qos_milli_time sleeping time of each iteration in milliseconds
	 */
	static void
	watch_loop( videosink_pipeline* pipeline, bool* loop_flag, int* com_pkgs, boost::mutex* mutex
			, int qos_milli_time, int* hr_time, int hr_limit )
	{
		while( true )
		{
			boost::this_thread::sleep( boost::posix_time::milliseconds( qos_milli_time ) );
			boost::lock_guard<boost::mutex> lock( *mutex );

			LOG_CLOG( log_debug_2 ) << "Watchdog loop: packets received in " << qos_milli_time
				<< " milliseconds = " << *com_pkgs;

			*hr_time += qos_milli_time;

			if( !*com_pkgs && *loop_flag )
			{
				LOG_CLOG( log_info ) << "No incoming packets, switching pads...";

				*loop_flag = false;

				g_object_set( G_OBJECT( pipeline->elements["iselector"] )
					, "active-pad", pipeline->pads["sel_sink1"]
					, NULL );

				if( global_log_level > log_debug_0 )
					GST_DEBUG_BIN_TO_DOT_FILE( pipeline->root_bin.get()
							, GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS, "local-flow" );
					
				gst_element_set_state( GST_ELEMENT( pipeline->root_bin.get() ), GST_STATE_READY );
				gst_element_set_state( GST_ELEMENT( pipeline->root_bin.get() ), GST_STATE_PLAYING );
			}

			if( *com_pkgs )
			{
				*hr_time = 0;
			}

			*com_pkgs = 0;

			if( *hr_time > hr_limit )
			{
				LOG_CLOG( log_info ) << "Hard reset time limit reached, signaling destruction...";

				GstBus* bus = gst_pipeline_get_bus( GST_PIPELINE( pipeline->root_bin.get() ) );
				GstMessage* msg_switch = gst_message_new_custom(
					GST_MESSAGE_ELEMENT, GST_OBJECT( pipeline->elements["identity"] )
				  , gst_structure_new( "hard_reset", "p", G_TYPE_STRING, "p", NULL ) );
				gst_bus_post( bus, msg_switch );
				gst_object_unref( GST_OBJECT( bus ) );
			}
		}
	}
}

#endif /* VIDEOSINK_HPP */
