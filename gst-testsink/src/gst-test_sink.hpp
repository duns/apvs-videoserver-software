// -------------------------------------------------------------------------------------------------------------
// gst-test_sink.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : June 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @file gst-test_sink.hpp
 * @ingroup nc_tcp_sink
 * @brief Type declarations and global function definitions for TCP video streaming sink prototype.
 */

#ifndef GST_TEST_SINK_HPP
#define GST_TEST_SINK_HPP

#include <gstreamermm.h>
#include <boost/tuple/tuple.hpp>
#include <boost/thread.hpp>

#include "gst-test_common.hpp"

/**
 * @ingroup nc_tcp_sink
 * @brief Fraction of a second (i.e. 1/QOS_TIME_SLICE in sec)
 */
#define QOS_TIME_SLICE 500

namespace nc_tcp_video_streamer_prot
{
	// tuple types for multiple parameter passing through legacy pointer args

	/**
 	 * @ingroup nc_tcp_sink
	 * @brief Type of data associated to pipeline's bus messages. 
	 */
	typedef boost::tuple<GMainLoop*, GstElement*, bool*, boost::mutex*> bmsg;

	/**
 	 * @ingroup nc_tcp_sink
	 * @brief Type of data associated to messages received from Identity element.
	 */
	typedef boost::tuple<GstElement*, boost::mutex*, int*, bool*> dbmsg;

	// triggers at every data buffer passing through the pipeline

	/**
 	 * @ingroup nc_tcp_sink
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

		boost::lock_guard<boost::mutex> lock( *m_data->get<1>() );

		if( !*m_data->get<3>() )
		{
			GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( m_data->get<0>() ) );
			GstBus* bus = gst_pipeline_get_bus( GST_PIPELINE( pipeline ) );

			GstStructure* state_pause  = gst_structure_new( "pause", "p", G_TYPE_STRING, "p", NULL );
			GstStructure* state_switch = gst_structure_new( "switch", "p", G_TYPE_STRING, "s", NULL );
			GstStructure* state_play = gst_structure_new( "play", "p", G_TYPE_STRING, "P", NULL );

			GstMessage* msg_pause = gst_message_new_custom( GST_MESSAGE_ELEMENT, GST_OBJECT( identity ), state_pause );
			GstMessage* msg_switch = gst_message_new_custom( GST_MESSAGE_ELEMENT, GST_OBJECT( identity ), state_switch );
			GstMessage* msg_play = gst_message_new_custom( GST_MESSAGE_ELEMENT, GST_OBJECT( identity ), state_play );

			gst_bus_post( bus, msg_pause );
			gst_bus_post( bus, msg_switch );
			gst_bus_post( bus, msg_play );

			gst_object_unref( GST_OBJECT( pipeline ) );
			gst_object_unref( GST_OBJECT( bus ) );

			*m_data->get<3>() = true;
		}
		*m_data->get<2>()+= 1;
	}

	// pipeline bus event handler

	/**
 	 * @ingroup nc_tcp_sink
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
				GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( m_data->get<1>() ) );

				gst_element_set_state( pipeline, GST_STATE_PLAYING );
				gst_object_unref( GST_OBJECT( pipeline ) );

				*m_data->get<2>() = true;
			}
		}
			break;

		case GST_MESSAGE_EOS:
		{
			GstElement* isel = m_data->get<1>();
			GstPad* sink1 = gst_element_get_static_pad( isel, "sink1" );

			g_object_set( G_OBJECT( isel ), "active-pad", sink1, NULL );
			gst_object_unref( GST_OBJECT( sink1 ) );			

			GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( isel ) ); // get pipeline object

			gst_element_set_state( pipeline, GST_STATE_READY );
			gst_element_set_state( pipeline, GST_STATE_PLAYING );

			gst_object_unref( GST_OBJECT( pipeline ) );

			*m_data->get<2>() = false; // switch loop flag
		}
			break;

		case GST_MESSAGE_ELEMENT:
		{
			if( !strcmp( msg->src->name, "identity" ) )
			{
				const GstStructure* state_struct = gst_message_get_structure( msg );
				GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( m_data->get<1>() ) );

				if( !strcmp( gst_structure_get_name( state_struct ), "pause" ) )
				{
					std::cout << "Pausing pipeline..." << std::endl;

					gst_element_set_state( GST_ELEMENT( msg->src ), GST_STATE_NULL ); 
					gst_element_set_state( GST_ELEMENT( gst_bin_get_by_name( GST_BIN( pipeline ), "iselector" ) )
						, GST_STATE_NULL ); 
					gst_element_set_state( GST_ELEMENT( gst_bin_get_by_name( GST_BIN( pipeline ), "ffmpegcs" ) )
						, GST_STATE_NULL ); 
					gst_element_set_state( pipeline, GST_STATE_PAUSED );
				}
				else if( !strcmp( gst_structure_get_name( state_struct ), "switch" ) )
				{
					std::cout << "Switching pads..." << std::endl;

					GstElement* isel = m_data->get<1>();					
					GstPad* sink0 = gst_element_get_static_pad( isel, "sink0" );

					g_object_set( G_OBJECT( isel ), "active-pad", sink0, NULL );
					gst_element_sync_state_with_parent( isel );

					gst_object_unref( GST_OBJECT( sink0 ) ); 
				}
				else if( !strcmp( gst_structure_get_name( state_struct ), "play" ) )
				{
					std::cout << "Restarting pipeline..." << std::endl;

					gst_element_set_state( pipeline, GST_STATE_PLAYING );
				}
			
				g_object_unref( G_OBJECT( pipeline ) );
			}
		}
		break;
		default:
			std::cerr << "Unhandled message send by: " << msg->src->name << std::endl;
			break;
		}

		return true;
	}

	// watchdog threads' process

	/**
 	 * @ingroup nc_tcp_sink
	 * @brief Watch-dog thread loop 
	 *
	 * The watch-dog thread periodically tests the loop flag along with the number of data packets 
	 * received in the time space between current and last check, in order to determine whether the 
	 * connection is lost  or its quality has been dramatically deteriorated. If one of the previous 
	 * mentioned conditions is true it switches from the tcp stream to a dummy one and resets the
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
	watch_loop( GstElement* iselector, bool* loop_flag, int* com_pkgs, boost::mutex* mutex )
	{
		while( true )
		{
			boost::this_thread::sleep( boost::posix_time::milliseconds( QOS_TIME_SLICE ) );
			boost::lock_guard<boost::mutex> lock( *mutex );

			if( !*com_pkgs && *loop_flag )
			{
				*loop_flag = false;

				GstPad* sink1 = gst_element_get_static_pad( iselector, "sink1" );

				g_object_set( G_OBJECT( iselector ), "active-pad", sink1, NULL );

				gst_object_unref( GST_OBJECT( sink1 ) );

				// get a pointer to the pipeline

				GstElement* pipeline = GST_ELEMENT( gst_element_get_parent( iselector ) ); 

				gst_element_set_state( pipeline, GST_STATE_READY );
				gst_element_set_state( pipeline, GST_STATE_PLAYING );

				gst_object_unref( GST_OBJECT( pipeline ) );
			}

			*com_pkgs = 0;
		}
	}
}

#endif /* GST_TEST_SINK_HPP */
