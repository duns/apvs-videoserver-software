// -------------------------------------------------------------------------------------------------------------
// videostreamer_common.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @file videostreamer_common.hpp
 * @ingroup video_streamer
 * @brief Common type declarations and global function definitions for Gstreamer video streamer.
 */

/**
 * @namespace video_streamer
 * @ingroup video_streamer
 * @brief Video streaming using Gstreamer.
 */

#ifndef VIDEOSTREAMER_COMMON_HPP
#define VIDEOSTREAMER_COMMON_HPP

#include <gst/gst.h>
#include <boost/exception/all.hpp>

#include "log.hpp"

namespace video_streamer
{
	using namespace auxiliary_libraries;

	/**
     * @ingroup video_streamer
     * @brief Managed dynamic Gstreamer bus resource.
     */
	typedef boost::shared_ptr<GstBus> gstbus_pt;

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
 	 * @brief Managed dynamic Gstreamer bin resource.
 	 */
	typedef boost::shared_ptr<GstBin> gstbin_pt;

	/**
	 * @ingroup video_streamer
	 * @brief Gstreamer element container.
	 */
	typedef std::map<std::string, GstElement*> elem_map_type;

	/**
	 * @ingroup video_streamer
	 * @brief Gstreamer pad container.
	 */
	typedef std::map<std::string, GstPad*> pad_map_type;

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
			while( GST_OBJECT( bin_ref )->refcount > 0 )
			{
				gst_object_unref( GST_OBJECT( bin_ref ) );
			}
		}
	};

	/**
	 * @ingroup video_streamer
	 * @brief Constructs an element and adds it to the specified bin.
	 *
	 * @param bin managed reference to bin
	 * @param elements elements container
	 * @param element prototype name
	 * @param name the name of the element to be created
	 *
	 * @throws video_streamer::api_error in case the element construction fails or the bin reject the element
	 */
	inline void
	create_add_element( gstbin_pt& bin, elem_map_type& elements, const char* element, const char* name )
	{
		elements[name] = gst_element_factory_make( element, name );

		if( !elements[name] )
		{
			LOG_CLOG( log_error ) << "Cannot create element '" << name << "' of type {" << element << "}.";
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Failed to create element." ) );
		}
		else
		{
			if( !gst_bin_add( bin.get(), elements[name] ) )
			{
				LOG_CLOG( log_error ) << "Cannot add element '" << name << "' of type {"
					<< element << "} to pipeline.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Failed to add element to pipeline." ) );
			}
		}
	}
}

#endif /* VIDEOSTREAMER_COMMON_HPP */
