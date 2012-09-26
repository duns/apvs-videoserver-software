// -------------------------------------------------------------------------------------------------------------
// gst-test_common.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : September 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @defgroup nc_tcp_video_streamer_prot TCP-based Gstreamer-powered video streaming prototypes
 * @ingroup prototypes
 * @brief Video streaming over TCP.
 */ 

/**
 * @file gst-test_common.hpp
 * @ingroup nc_tcp_video_streamer_prot
 * @brief Common type declarations and global function definitions for Gstreamer TCP video streaming prototypes.
 */

/**
 * @namespace nc_tcp_video_streamer_prot
 * @ingroup nc_tcp_video_streamer_prot
 * @brief Video streaming prototypes using Gstreamer.
 */

#ifndef GST_TEST_COMMON_HPP
#define GST_TEST_COMMON_HPP

#include <gst/gst.h>
#include <boost/exception/all.hpp>
 
namespace nc_tcp_video_streamer_prot
{
	// managed Gstreamer resources

	/**
         * @ingroup nc_tcp_video_streamer_prot
         * @brief Managed dynamic Gstreamer bin resource.
         */
	typedef boost::shared_ptr<GstBin> gstbin_pt;

	/**
         * @ingroup nc_tcp_video_streamer_prot
         * @brief Managed dynamic Gstreamer bus resource.
         */
	typedef boost::shared_ptr<GstBus> gstbus_pt;

	// boost::exception error info types

	/**
         * @ingroup nc_tcp_video_streamer_prot
         * @brief Message type for legacy API errors.
         * This type is utilized for information propagation along with legacy-api-triggered C++ exceptions. 
         */
	typedef boost::error_info<struct tag_my_info, std::string> api_info;

	/**
         * @ingroup nc_tcp_video_streamer_prot
         * @brief Message type for Gstreamer Bus errors.
         * This type is utilized for information propagation along with C++ exceptions originating
         * from Gstreamer bus message handler.
         */
	typedef boost::error_info<struct tag_my_info, std::string> bus_info;

	// custom destructor for managed Gstreamer resources

	/**
         * @ingroup nc_tcp_video_streamer_prot
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
	 * @ingroup nc_tcp_video_streamer_prot
	 * @brief Exception type for Gstreamer API errors.
	 */
	struct api_error: virtual boost::exception, virtual std::exception { };

	/**
	 * @ingroup nc_tcp_video_streamer_prot
	 * @brief Exception type for Gstreamer bus errors.
	 */
	struct bus_error: virtual boost::exception, virtual std::exception { };

	// << overload for easy pipeline construction

	/**
	 * @ingroup nc_tcp_video_streamer_prot
	 * @brief Global streaming operator overload.
	 * Eases the sequential Gstreamer flow construction from simple Gstreamer elements.
	 *
	 * @param container input Gstreamer element container (bin or pipeline)
	 * @param elem Gstreamer element to be added in the container
	 * @returns reference to the Gstreamer container (in order to enable chaining)
	 *
	 * @throws nc_tcp_video_streamer_prot::api_error in case the element linking failed
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
	 * @ingroup nc_tcp_video_streamer_prot
	 * @brief Inserts filter between two consecutive elements
	 * Given two Gstreamer elements that are indented to be linked against each other, this function inserts
	 * a hard-coded filter element in between and performs the linking.
	 *
	 * @param elem_1 assuming a left-to-right flow this is the left-most element
	 * @param elem_2 assuming a left-to-right flow this is the right-most element
	 * 
	 * @throws nc_tcp_video_streamer_prot::api_error in case the element linking failed
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
}

#endif // GST_TEST_COMMON_HPP


