// -------------------------------------------------------------------------------------------------------------
// videosink.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @defgroup video_sink Gstreamer video sink
 * @ingroup video_streamer
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

#include "gst-common.hpp"

/**
 * @ingroup video_sink
 * @brief Fraction of a second (i.e. 1/QOS_TIME_SLICE in sec)
 */
#define QOS_TIME_SLICE 500

namespace video_streamer
{
	// tuple types for multiple parameter passing through legacy pointer args

	/**
 	 * @ingroup video_sink
	 * @brief Type of data associated to pipeline's bus messages. 
	 */
	typedef boost::tuple<GMainLoop*, element_set*, bool*, boost::mutex*> bmsg;

	/**
 	 * @ingroup video_sink
	 * @brief Type of data associated to messages received from Identity element.
	 */
	typedef boost::tuple<boost::mutex*, int*, bool*> dbmsg;

	// triggers at every data buffer passing through the pipeline

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
		dbmsg* m_data = static_cast<dbmsg*>( user_data );

		boost::lock_guard<boost::mutex> lock( *m_data->get<0>() );

		if( !*m_data->get<2>() )
		{
			GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( identity ) );
			GstBus* bus = gst_pipeline_get_bus( GST_PIPELINE( pipeline ) );

			GstMessage* msg_switch = gst_message_new_custom( GST_MESSAGE_ELEMENT, GST_OBJECT( identity )
				, gst_structure_new( "switch_stream", "p", G_TYPE_STRING, "p", NULL ) );

			gst_bus_post( bus, msg_switch );

			gst_object_unref( GST_OBJECT( pipeline ) );
			gst_object_unref( GST_OBJECT( bus ) );
		}
		*m_data->get<1>()+= 1;
	}

	// pipeline bus event handler

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
	 */
	static gboolean
	pipeline_bus_handler( GstBus* bus, GstMessage* msg, gpointer data )
	{
		bmsg* m_data = static_cast<bmsg*>( data );

		GMainLoop* loop = m_data->get<0>();
		
		boost::lock_guard<boost::mutex> lock( *m_data->get<3>() );

		switch( GST_MESSAGE_TYPE( msg ) )
		{
		case GST_MESSAGE_ERROR:
		{
			GError *err;
			gchar* err_str;
			std::string err_msg;

			gst_message_parse_error( msg, &err, &err_str );
			err_msg = std::string( err_str ) + std::string( " " ) + std::string( err->message );

			gint err_code = err->code;

			g_error_free( err );
			g_free( err_str );

			if( err_code != 5 ) // (5) -> network connection lost
			{
				g_main_loop_quit( loop );
				BOOST_THROW_EXCEPTION( bus_error() << bus_info( err_msg ) );
			}
			else
			{
				GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( m_data->get<1>()->networksource ) );

				gst_element_set_state( pipeline, GST_STATE_PLAYING );
				gst_object_unref( GST_OBJECT( pipeline ) );

				*m_data->get<2>() = true;
			}
		}
			break;

		case GST_MESSAGE_EOS:
		{
		}
		break;
		case GST_MESSAGE_ELEMENT:
		{
			if( !strcmp( msg->src->name, "identity" ) )
			{
				const GstStructure* state_struct = gst_message_get_structure( msg );
				GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( m_data->get<1>()->networksource ) );

				if( !strcmp( gst_structure_get_name( state_struct ), "switch_stream" ) )
				{
					std::cout << "Pausing pipeline..." << std::endl;
				
					gst_element_set_state( GST_ELEMENT( msg->src ), GST_STATE_NULL ); 
					gst_element_set_state( m_data->get<1>()->iselector, GST_STATE_NULL ); 
					gst_element_set_state( m_data->get<1>()->ffmpegcs0, GST_STATE_NULL ); 

					std::cout << "Switching pads..." << std::endl;

					g_object_set( G_OBJECT( m_data->get<1>()->iselector )
						, "active-pad", m_data->get<1>()->sel_sink0
						, NULL );
					
					std::cout << "Restarting pipeline..." << std::endl;

					GST_DEBUG_BIN_TO_DOT_FILE( GST_BIN( pipeline ), GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS, "switch" );					
					gst_element_set_state( pipeline, GST_STATE_PLAYING );
				}
				
				g_object_unref( G_OBJECT( pipeline ) );
				
				*m_data->get<2>() = true;
			}
		}
		break;
		default:
			//std::cerr << "Unhandled message send by: " << msg->src->name << std::endl;
			break;
		}

		return true;
	}

	// watchdog threads' process

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
	 * @param iselector Gstreamer switching element <em>input-selector</em>
	 * @param loop_flag a pointer to a loop flag provided by the executing pipeline
	 * @param com_pkgs a pointer to the number of packets received between current and last check
	 * @param mutex a pointer to a mutex provided by the executing pipeline
	 */
	static void
	watch_loop( element_set* elements, bool* loop_flag, int* com_pkgs, boost::mutex* mutex )
	{
		while( true )
		{
			boost::this_thread::sleep( boost::posix_time::milliseconds( QOS_TIME_SLICE ) );
			boost::lock_guard<boost::mutex> lock( *mutex );

			if( !*com_pkgs && *loop_flag )
			{
				std::cout << "Watchdog switching pads..." << std::endl;

				*loop_flag = false;
				
				GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( elements->iselector ) ); 
					
				g_object_set( G_OBJECT( elements->iselector )
					, "active-pad", elements->sel_sink1 
					, NULL );
					
				GST_DEBUG_BIN_TO_DOT_FILE( GST_BIN( pipeline ), GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS, "switch_local" );
					
				gst_element_set_state( pipeline, GST_STATE_READY );
				gst_element_set_state( pipeline, GST_STATE_PLAYING );

				gst_object_unref( GST_OBJECT( pipeline ) );
			}

			*com_pkgs = 0;
		}
	}
}

#endif /* VIDEOSINK_HPP */
