// -------------------------------------------------------------------------------------------------------------
// videosink_pipeline.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @file videosink_pipeline.hpp
 * @ingroup video_sink
 * @brief The video streamer sink-side pipeline.
 */

#ifndef VIDEOSINK_PIPELINE_HPP
#define VIDEOSINK_PIPELINE_HPP

#include <gst/gst.h>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

#include "log.hpp"
#include "videostreamer_common.hpp"

#define OUTPUT_VIDEO_NAMING "%y%m%d-%H.%M.%S-"

namespace video_sink
{
	using namespace video_streamer;
	using namespace auxiliary_libraries;

	/**
	 * @ingroup video_source
	 * @brief Intercepts two consecutive video elements with a caps filter
	 * Given two Gstreamer elements, this function inserts a video filter element in between and performs the
	 * linking. Caution, it links only elements that are video-related.
	 *
	 * @param elem1 assuming a left-to-right flow this is the left-most element
	 * @param elem2 assuming a left-to-right flow this is the right-most element
	 * @param opts_map set of filter properties
	 *
	 * @throws video_streamer::api_error in case the element linking failed
	 */
	inline void
	insert_video_filter( GstElement* elem1, GstElement* elem2, const boost::program_options::variables_map& opts_map )
	{
		gboolean link_flag = false;
		GstCaps* caps;

		caps = gst_caps_new_simple( opts_map["localvideosource.video-header"].as<std::string>().c_str()
		  , "width", G_TYPE_INT, opts_map["localvideosource.width"].as<int>()
		  , "height", G_TYPE_INT, opts_map["localvideosource.height"].as<int>()
		  , "framerate", GST_TYPE_FRACTION, opts_map["localvideosource.framerate-num"].as<int>()
		      , opts_map["localvideosource.framerate-den"].as<int>()
		  , "format", GST_TYPE_FOURCC, GST_MAKE_FOURCC( opts_map["localvideosource.fourcc-format0"].as<char>()
			  , opts_map["localvideosource.fourcc-format1"].as<char>()
			  , opts_map["localvideosource.fourcc-format2"].as<char>()
			  , opts_map["localvideosource.fourcc-format3"].as<char>() )
	      , "interlaced", G_TYPE_BOOLEAN, opts_map["localvideosource.interlaced"].as<bool>()
		  , NULL );

		link_flag = gst_element_link_filtered( elem1, elem2, caps );

		gst_caps_unref( caps );

		if( !link_flag )
		{
			LOG_CLOG( log_error ) << "Filter linking error.";
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Couldn't connect to filter." ) );
		}
	}

	/**
	 * @ingroup video_source
	 * @brief on-pad-added gstreamer element linking event handler.
	 * This callback once registered with a gstreamer element that supports dynamic pads, it handles the events
	 * thrown by the element during a new pad construction.
	 *
	 * @param element the gstreamer element that has raised the event
	 * @param pad the newly created pad
	 */
	static void on_pad_added( GstElement *element, GstPad *pad )
	{
		GstCaps *caps;
		GstStructure *caps_struct;

		if( ! ( caps = gst_pad_get_caps( pad ) ) )
		{
			LOG_CLOG( log_error ) << "Failed to get caps.";
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Caps Error." ) );
		}

		if( ! ( caps_struct = gst_caps_get_structure( caps, 0 ) ) )
		{
			LOG_CLOG( log_error ) << "Failed to retrieve caps structure.";
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Caps Error." ) );
		}

		LOG_CLOG( log_debug_1 ) << "Demuxer pad creation: " << gst_caps_to_string( caps );

		const gchar *c = gst_structure_get_name( caps_struct );

		if( g_strrstr( c, "video" ) || g_strrstr( c, "image" ) )
		{
			GstBin* pipeline = GST_BIN( gst_element_get_parent( element ) );

			if( !pipeline )
			{
				LOG_CLOG( log_error ) << "Couldn't obtain parent bin.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Parent retrieval error." ) );
			}

			GstElement* h264dec = gst_bin_get_by_name( pipeline, "h264dec" );

			if( !h264dec )
			{
				LOG_CLOG( log_error ) << "Couldn't obtain decoder element from pipeline.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Pipeline component retrieval error." ) );
			}

			GstPad *targetsink = gst_element_get_pad( h264dec, "sink" );

            if( !targetsink )
            {
              	LOG_CLOG( log_error ) << "Failed to retrieve decoder pad.";
              	BOOST_THROW_EXCEPTION( api_error() << api_info( "Pad retrieving error." ) );
            }

            gst_pad_link( pad, targetsink );
            gst_object_unref( targetsink );
            gst_object_unref( h264dec );
            gst_object_unref( pipeline );
		}
		else if( g_strrstr( c, "audio" ) )
		{
			// Do stuff for audio
		}
		else
		{
			LOG_CLOG( log_error ) << "Failed to obtain source pad.";
			BOOST_THROW_EXCEPTION( api_error() << api_info( "Pad retrieving error." ) );
		}

		gst_caps_unref( caps );
	}

	/**
	 * @ingroup video_sink
	 * @brief Video streamer sink (server-side) pipeline.
	 *
	 * The pipeline streams video and audio from a network source to a virtual video device and a storage target.
	 * The topology of the pipeline is shown here: <add reference to schema>. The user of this object has
	 * unrestricted access to every pipeline element.
	 */
	struct videosink_pipeline
	{
		/**
		 * @brief Video sink pipeline constructor.
		 *
		 * @param opts_map the <em>boost::program_options::variables_map</em> contains the entire configuration
		 * parameters set, these parameters determine upon construction the properties of the individual elements
		 *
		 * @throws video_streamer::api_error in creation or linking errors.
		 */
		videosink_pipeline( const boost::program_options::variables_map& opts_map )
			: root_bin( GST_BIN( gst_pipeline_new( "videosink" ) ), cust_deleter<GstBin>() )
		{

			create_add_element( root_bin, elements, "tcpserversrc", "networksource" );

			g_object_set( G_OBJECT( elements["networksource"] )
				, "host", opts_map["connection.localhost"].as<std::string>().c_str()
				, "port", opts_map["connection.port"].as<int>()
				, "do-timestamp", true
				, NULL );

			create_add_element( root_bin, elements, "queue2", "queue0" );
			create_add_element( root_bin, elements, "queue2", "queue1" );
			create_add_element( root_bin, elements, "multifilesink", "filesink" );
			create_add_element( root_bin, elements, "ffdec_h264", "h264dec" );
			create_add_element( root_bin, elements, "identity", "identity" );
			create_add_element( root_bin, elements, "input-selector", "iselector" );
			create_add_element( root_bin, elements, "ffmpegcolorspace", "ffmpegcs0" );
			create_add_element( root_bin, elements, "timeoverlay", "timeoverlay" );
			create_add_element( root_bin, elements, "tee", "tee" );
			create_add_element( root_bin, elements, "videotestsrc", "videosource" );
			create_add_element( root_bin, elements, "v4l2sink", "v4l2sink" );
			create_add_element( root_bin, elements, "coglogoinsert", "coglogoinsert" );
			create_add_element( root_bin, elements, "mpegtsdemux", "demux" );

			if( !gst_element_link_many( elements["networksource"], elements["tee"], NULL )
			||  !gst_element_link( elements["queue1"], elements["demux"] ) )
			{
				LOG_CLOG( log_error ) << "Failed to link elements.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Linking failure." ) );
			}

			if( opts_map["clockoverlay.enable-clock"].as<bool>() )
			{
				create_add_element( root_bin, elements, "clockoverlay", "clockoverlay" );

				g_object_set( G_OBJECT( elements["clockoverlay"] )
					, "halignment", opts_map["clockoverlay.halignment"].as<int>()
					, "valignment", opts_map["clockoverlay.valignment"].as<int>()
					, "shaded-background", opts_map["clockoverlay.shaded-background"].as<bool>()
					, "time-format", opts_map["clockoverlay.time-format"].as<std::string>().c_str()
					, "font-desc", opts_map["clockoverlay.font"].as<std::string>().c_str()
					, NULL );

				if( !gst_element_link_many( elements["h264dec"], elements["identity"], elements["ffmpegcs0"]
						, elements["timeoverlay"], elements["clockoverlay"], elements["coglogoinsert"]
						, elements["iselector"], elements["v4l2sink"], NULL ) )
				{
					LOG_CLOG( log_error ) << "Failed to link elements.";
					BOOST_THROW_EXCEPTION( api_error() << api_info( "Linking failure." ) );
				}
			}
			else
			{
				if( !gst_element_link_many( elements["h264dec"], elements["identity"], elements["ffmpegcs0"]
						, elements["timeoverlay"], elements["coglogoinsert"], elements["iselector"]
						, elements["v4l2sink"], NULL ) )
				{
					LOG_CLOG( log_error ) << "Failed to link elements.";
					BOOST_THROW_EXCEPTION( api_error() << api_info( "Linking failure." ) );
				}
			}

			insert_video_filter( elements["videosource"], elements["iselector"], opts_map );

			std::stringstream ss;

			auto dt_facet = new boost::posix_time::time_facet();
			dt_facet->format( OUTPUT_VIDEO_NAMING );
			ss.imbue( std::locale( std::cout.getloc(), dt_facet ) );

			ss << opts_map["videobackup.output-path"].as<std::string>() << "/"
			   << opts_map["session-id"].as<std::string>() << "/"
			   << boost::posix_time::second_clock::local_time() << "%05d.ts";

			boost::filesystem3::path full_path( ss.str() );

			if( !boost::filesystem3::exists( full_path.parent_path() ) )
			{
				boost::filesystem3::create_directories( full_path.parent_path() );
			}

			g_object_set( G_OBJECT( elements["filesink"] )
				, "location", ss.str().c_str()
				, "sync", true
				, "async", false
				, "next-file", 1
			    , "enable-last-buffer", false
				, NULL );

			g_object_set( G_OBJECT( elements["videosource"] )
				, "pattern", opts_map["localvideosource.pattern"].as<int>()
				, "is-live", true
				, NULL );

			g_object_set( G_OBJECT( elements["timeoverlay"] )
				, "halignment", opts_map["timeoverlay.halignment"].as<int>()
				, "font-desc", opts_map["timeoverlay.font"].as<std::string>().c_str()
				, NULL );

			g_object_set( G_OBJECT( elements["coglogoinsert"] )
				, "location", opts_map["imageoverlay.img-location"].as<std::string>().c_str()
				, NULL );

			g_object_set( G_OBJECT( elements["v4l2sink"] )
				, "device", opts_map["capturedevice.name"].as<std::string>().c_str()
				, "sync", false
				, NULL );

			pads["sel_sink0"] = gst_element_get_static_pad( elements["iselector"], "sink0" );
			pads["sel_sink1"] = gst_element_get_static_pad( elements["iselector"], "sink1" );

			if( !pads["sel_sink0"] || !pads["sel_sink1"])
			{
				LOG_CLOG( log_error ) << "Failed to retrieve input-selector pads.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Pad retrieving error." ) );
			}

			GstPad *tee_src0, *tee_src1, *queue0_src, *queue0_sink, *queue1_sink, *filesink_sink;

			tee_src0 = gst_element_get_request_pad( elements["tee"], "src%d" );
			tee_src1 = gst_element_get_request_pad( elements["tee"], "src%d" );
			queue0_sink = gst_element_get_static_pad( elements["queue0"], "sink" );
			queue0_src = gst_element_get_static_pad( elements["queue0"], "src" );
			queue1_sink = gst_element_get_static_pad( elements["queue1"], "sink" );
			filesink_sink = gst_element_get_static_pad( elements["filesink"], "sink" );

			if( !tee_src0 || !tee_src1 || !queue0_sink || !queue0_src || !queue1_sink || !filesink_sink )
			{
				LOG_CLOG( log_error ) << "Failed to retrieve element pads.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Pad retrieving error." ) );
			}

			if( gst_pad_link( tee_src0, queue0_sink ) != GST_PAD_LINK_OK
			 || gst_pad_link( tee_src1, queue1_sink ) != GST_PAD_LINK_OK
			 || gst_pad_link( queue0_src, filesink_sink ) != GST_PAD_LINK_OK )
			{
				LOG_CLOG( log_error ) << "Failed to link pads.";
				BOOST_THROW_EXCEPTION( api_error() << api_info( "Pad linking error." ) );
			}

			gst_object_unref( tee_src0 );
			gst_object_unref( tee_src1 );
			gst_object_unref( queue0_sink );
			gst_object_unref( queue0_src );
			gst_object_unref( queue1_sink );
			gst_object_unref( filesink_sink );

			g_signal_connect( elements["demux"], "pad-added", G_CALLBACK( on_pad_added ), NULL);
		}

		/**
		 * @brief Video sink pipeline destructor
		 *
		 * Releases Gstreamer resources whose ownership is taken by the data members.
		 */
		~videosink_pipeline()
		{
			gst_object_unref( pads["sel_sink0"] );
			gst_object_unref( pads["sel_sink1"] );
		}

		/**
		 * @brief Managed reference of the pipeline.
		 */
		gstbin_pt root_bin;

		/**
		 * @brief Pipeline element container.
		 */
		elem_map_type elements;

		/**
		 * @brief Pad container.
		 */
		pad_map_type pads;
	};
}

#endif /* VIDEOSINK_PIPELINE_HPP */
