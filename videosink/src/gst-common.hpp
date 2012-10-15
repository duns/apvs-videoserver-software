// -------------------------------------------------------------------------------------------------------------
// gst-common.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @defgroup video_streamer Gstreamer-powered video streamer
 * @ingroup server-modules
 * @brief Video streaming over the network.
 */ 

/**
 * @file gst-common.hpp
 * @ingroup video_streamer
 * @brief Common type declarations and global function definitions for Gstreamer video streamer.
 */

/**
 * @namespace video_streamer
 * @ingroup video_streamer
 * @brief Video streaming using Gstreamer.
 */

#ifndef GST_COMMON_HPP
#define GST_COMMON_HPP

#include <gst/gst.h>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
 
namespace video_streamer
{
	// managed Gstreamer resources

	/**
         * @ingroup video_streamer
         * @brief Managed dynamic Gstreamer bin resource.
         */
	typedef boost::shared_ptr<GstBin> gstbin_pt;

	/**
         * @ingroup video_streamer
         * @brief Managed dynamic Gstreamer bus resource.
         */
	typedef boost::shared_ptr<GstBus> gstbus_pt;

	// boost::exception error info types

	/**
         * @ingroup video_streamer
         * @brief Message type for legacy API errors.
         * This type is utilized for information propagation along with legacy-api-triggered C++ exceptions. 
         */
	typedef boost::error_info<struct tag_my_info, std::string> api_info;

	/**
         * @ingroup video_streamer
         * @brief Message type for Gstreamer Bus errors.
         * This type is utilized for information propagation along with C++ exceptions originating
         * from Gstreamer bus message handler.
         */
	typedef boost::error_info<struct tag_my_info, std::string> bus_info;

	// custom destructor for managed Gstreamer resources

	/**
         * @ingroup video_streamer
         * @brief Custom deleter for smart pointers referencing Gstreamer resources.
	 * A generic custom deleter functor for Gstreamer resources.
         *
         * @tparam T type of Gstreamer resource.
	 */
	template<typename T>
	struct cust_deleter
	{
		/**
                 * @brief Calling operator overload
                 * Invokes Gstreamer resource unref function. 
                 * @param bin_ref Gstreamer resource pointer
		 */	
		void operator()( T* bin_ref ) const
		{
			gst_object_unref( GST_OBJECT( bin_ref ) );
		}
	};

	struct element_set
	{
		GstElement *queue0
			 , *queue1
			 , *queue2
			 , *avimux
			 , *filesink
			 , *h264dec
			 , *identity
			 , *iselector
			 , *ffmpegcs0
			 , *timeoverlay
			 , *clockoverlay
			 , *networksource
			 , *gdpdepay
			 , *tee
			 , *videosource
			 , *coglogoinsert
			 , *v4l2sink;

		GstCaps *ratecaps;

		GstPad *sel_sink0
		     , *sel_sink1;

		element_set( const boost::program_options::variables_map& opts_map )
		{
			if( !opts_map["connection.transfer-protocol"].as<std::string>().compare( std::string( "TCP" ) ) )
			{
				networksource = gst_element_factory_make( "tcpserversrc", "networksource" );
			
				g_object_set( G_OBJECT( networksource )
					, "host", opts_map["connection.localhost"].as<std::string>().c_str()
					, "port", opts_map["connection.port"].as<int>()
					, NULL );		
			}
			else if( !opts_map["connection.transfer-protocol"].as<std::string>().compare( std::string( "UDP" ) ) )
			{
				networksource = gst_element_factory_make( "udpsrc", "networksource" );
			
				g_object_set( G_OBJECT( networksource )
					, "uri", ( std::string( "udp://" )
					         + opts_map["connection.localhost"].as<std::string>()
					         + std::string( ":" )
					         + boost::lexical_cast<std::string>( opts_map["connection.port"].as<int>() ) ).c_str()
					, NULL );
			}
			
			queue0        = gst_element_factory_make( "queue2"          , "queue0" );
			queue1        = gst_element_factory_make( "queue2"          , "queue1" );
			queue2        = gst_element_factory_make( "queue2"          , "queue2" );
			avimux        = gst_element_factory_make( "avimux"          , "avimux" );
			filesink      = gst_element_factory_make( "multifilesink"   , "filesink" );
			h264dec       = gst_element_factory_make( "ffdec_h264"      , "h264dec" );
			identity      = gst_element_factory_make( "identity"        , "identity" );
			iselector     = gst_element_factory_make( "input-selector"  , "iselector" );
			ffmpegcs0     = gst_element_factory_make( "ffmpegcolorspace", "ffmpegcs0" );
			timeoverlay   = gst_element_factory_make( "timeoverlay"     , "timeoverlay" );
			clockoverlay  = gst_element_factory_make( "clockoverlay"    , "clockoverlay" );
			gdpdepay      = gst_element_factory_make( "gdpdepay"        , "gdpdepay" );
			tee           = gst_element_factory_make( "tee"             , "tee" );
			videosource   = gst_element_factory_make( "videotestsrc"    , "videosource" );
			v4l2sink      = gst_element_factory_make( "v4l2sink"        , "v4l2sink" );
			coglogoinsert = gst_element_factory_make( "coglogoinsert"   , "coglogoinsert" );

			g_object_set( G_OBJECT( filesink )
				, "location", ( opts_map["videobackup.location"].as<std::string>() + std::string( "/" ) 
					      + opts_map["videobackup.pattern"].as<std::string>() ).c_str()
				, "next-file", 1
				, "sync", false
				, "async", false
				, NULL );
	
			g_object_set( G_OBJECT( videosource )
				, "pattern", opts_map["localvideosource.pattern"].as<int>()
				, "is-live", true
				, NULL );
			
			g_object_set( G_OBJECT( timeoverlay )
				, "halignment", opts_map["timeoverlay.halignment"].as<int>()
				, NULL );
		
			g_object_set( G_OBJECT( coglogoinsert )
				, "location", opts_map["imageoverlay.location"].as<std::string>().c_str()
				, NULL );
		
			g_object_set( G_OBJECT( clockoverlay )
				, "halignment", opts_map["clockoverlay.halignment"].as<int>() 
				, "valignment", opts_map["clockoverlay.valignment"].as<int>() 
				, "shaded-background", opts_map["clockoverlay.shaded-background"].as<bool>() 
				, "time-format", opts_map["clockoverlay.time-format"].as<std::string>().c_str() 
				, NULL ); 		
			
			g_object_set( G_OBJECT( v4l2sink )
				, "device", opts_map["capturedevice.name"].as<std::string>().c_str() 
				, "sync", false 
				, NULL );
		}
	};

	// custom exception types

	/**
	 * @ingroup video_streamer
	 * @brief Exception type for Gstreamer API errors.
	 */
	struct api_error: virtual boost::exception, virtual std::exception { };

	/**
	 * @ingroup video_streamer
	 * @brief Exception type for Gstreamer bus errors.
	 */
	struct bus_error: virtual boost::exception, virtual std::exception { };

	/**
	 * @ingroup video_streamer
	 * @brief Global streaming operator overload.
	 * Eases the sequential Gstreamer flow construction from simple Gstreamer elements.
	 *
	 * @param container input Gstreamer element container (bin or pipeline)
	 * @param elem Gstreamer element to be added in the container
	 * @returns reference to the Gstreamer container (in order to enable chaining)
	 *
	 * @throws video_streamer::api_error in case the element linking failed
	 */
	inline gstbin_pt&
	operator<<( gstbin_pt& container, GstElement* elem )
	{
		gpointer cur_elem = NULL;
		gboolean proc_flag = true;

		gst_iterator_next( gst_bin_iterate_sorted( container.get() ), &cur_elem );

		proc_flag &= gst_bin_add( container.get(), elem );

		if( cur_elem )
		{
			proc_flag &= gst_element_link( GST_ELEMENT( cur_elem ), elem );
			gst_object_unref( GST_OBJECT( cur_elem ) );
		}

		if( !proc_flag )
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Cannot add and/or link element." ) );

		return container;
	}

	// filter interception during element linking

	/**
	 * @ingroup video_streamer
	 * @brief Inserts filter between two consecutive elements
	 * Given two Gstreamer elements that are indented to be linked against each other, this function inserts
	 * a hard-coded filter element in between and performs the linking.
	 *
	 * @param elem_1 assuming a left-to-right flow this is the left-most element
	 * @param elem_2 assuming a left-to-right flow this is the right-most element
	 * @param opts_map set of options for setting the filter properties
	 *
	 * @throws video_streamer::api_error in case the element linking failed
 	 */
	inline void
	insert_filter( GstElement* elem_1, GstElement* elem_2
		, const boost::program_options::variables_map& opts_map )
	{
		gboolean link_flag = false;
		GstCaps* caps;

		caps = gst_caps_new_simple( 
			opts_map["localvideosource.video-header"].as<std::string>().c_str() 
			, "width"     , G_TYPE_INT
			, opts_map["localvideosource.width"].as<int>() 
			, "height"    , G_TYPE_INT
			, opts_map["localvideosource.height"].as<int>() 
			, "framerate" , GST_TYPE_FRACTION
			, opts_map["localvideosource.framerate-num"].as<int>()
		        , opts_map["localvideosource.framerate-den"].as<int>()
			, "format"    , GST_TYPE_FOURCC
			, GST_MAKE_FOURCC( opts_map["localvideosource.fourcc-format0"].as<char>()
					 , opts_map["localvideosource.fourcc-format1"].as<char>()
					 , opts_map["localvideosource.fourcc-format2"].as<char>()
					 , opts_map["localvideosource.fourcc-format3"].as<char>() )
			, "interlaced", G_TYPE_BOOLEAN
			, opts_map["localvideosource.interlaced"].as<bool>()
			, NULL );

		link_flag = gst_element_link_filtered( elem_1, elem_2, caps );

		gst_caps_unref( caps );

		if( !link_flag )
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Couldn't connect to filter." ) );
	}
}

#endif // GST_COMMON_HPP


