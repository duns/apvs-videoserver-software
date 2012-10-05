// -------------------------------------------------------------------------------------------------------------
// gst-common.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @defgroup tcp_video_streamer TCP-based Gstreamer-powered video streamer
 * @ingroup server-modules
 * @brief Video streaming over TCP.
 */ 

/**
 * @file gst-common.hpp
 * @ingroup tcp_video_streamer
 * @brief Common type declarations and global function definitions for Gstreamer TCP video streamer.
 */

/**
 * @namespace tcp_video_streamer
 * @ingroup tcp_video_streamer
 * @brief Video streaming using Gstreamer.
 */

#ifndef GST_COMMON_HPP
#define GST_COMMON_HPP

#include <gst/gst.h>
#include <boost/exception/all.hpp>
#include <boost/program_options.hpp>
 
namespace tcp_video_streamer
{
	// managed Gstreamer resources

	/**
         * @ingroup tcp_video_streamer
         * @brief Managed dynamic Gstreamer bin resource.
         */
	typedef boost::shared_ptr<GstBin> gstbin_pt;

	/**
         * @ingroup tcp_video_streamer
         * @brief Managed dynamic Gstreamer bus resource.
         */
	typedef boost::shared_ptr<GstBus> gstbus_pt;

	// boost::exception error info types

	/**
         * @ingroup tcp_video_streamer
         * @brief Message type for legacy API errors.
         * This type is utilized for information propagation along with legacy-api-triggered C++ exceptions. 
         */
	typedef boost::error_info<struct tag_my_info, std::string> api_info;

	/**
         * @ingroup tcp_video_streamer
         * @brief Message type for Gstreamer Bus errors.
         * This type is utilized for information propagation along with C++ exceptions originating
         * from Gstreamer bus message handler.
         */
	typedef boost::error_info<struct tag_my_info, std::string> bus_info;

	// custom destructor for managed Gstreamer resources

	/**
         * @ingroup tcp_video_streamer
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

	// custom exception types

	/**
	 * @ingroup tcp_video_streamer
	 * @brief Exception type for Gstreamer API errors.
	 */
	struct api_error: virtual boost::exception, virtual std::exception { };

	/**
	 * @ingroup tcp_video_streamer
	 * @brief Exception type for Gstreamer bus errors.
	 */
	struct bus_error: virtual boost::exception, virtual std::exception { };

	/**
	 * @ingroup tcp_video_streamer
	 * @brief Global streaming operator overload.
	 * Eases the sequential Gstreamer flow construction from simple Gstreamer elements.
	 *
	 * @param container input Gstreamer element container (bin or pipeline)
	 * @param elem Gstreamer element to be added in the container
	 * @returns reference to the Gstreamer container (in order to enable chaining)
	 *
	 * @throws tcp_video_streamer::api_error in case the element linking failed
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
	 * @ingroup tcp_video_streamer
	 * @brief Inserts filter between two consecutive elements
	 * Given two Gstreamer elements that are indented to be linked against each other, this function inserts
	 * a hard-coded filter element in between and performs the linking.
	 *
	 * @param elem_1 assuming a left-to-right flow this is the left-most element
	 * @param elem_2 assuming a left-to-right flow this is the right-most element
	 * 
	 * @throws tcp_video_streamer::api_error in case the element linking failed
 	 */
	inline void
	insert_filter( GstElement* elem_1, GstElement* elem_2 )
	{
		gboolean link_flag = false;
		GstCaps* caps;

		caps = gst_caps_new_simple( "video/x-raw-yuv"
			, "width", G_TYPE_INT, 640
			, "height", G_TYPE_INT, 480
			, "framerate", GST_TYPE_FRACTION, 30, 1
			, "format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0')
			, "interlaced", G_TYPE_BOOLEAN, false
			, NULL );

		link_flag = gst_element_link_filtered( elem_1, elem_2, caps );

		gst_caps_unref( caps );

		if( !link_flag )
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Couldn't connect to filter." ) );
	}

	/**
	 * @ingroup tcp_video_streamer
	 * @brief Inserts filter between two consecutive elements
	 * Given two Gstreamer elements that are indented to be linked against each other, this function inserts
	 * a hard-coded filter element in between and performs the linking.
	 *
	 * @param elem_1 assuming a left-to-right flow this is the left-most element
	 * @param elem_2 assuming a left-to-right flow this is the right-most element
	 * @param opts_map set of options for setting the filter properties
	 *
	 * @throws tcp_video_streamer::api_error in case the element linking failed
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


