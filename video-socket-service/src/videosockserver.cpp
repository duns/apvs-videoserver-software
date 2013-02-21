// -------------------------------------------------------------------------------------------------------------
// videosockserver.cpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision :
// -------------------------------------------------------------------------------------------------------------

#include <sstream>
#include <string>

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/range.hpp>

#include <stdlib.h>

#include "log.hpp"

#define MAX_MSG_BUFFER_LENGTH 4096

using boost::asio::ip::tcp;
using namespace auxiliary_libraries;

auxiliary_libraries::log_level global_log_level = log_debug_2;

int main( int argc, char* argv[] )
{
	try
	{
		namespace app_opts = boost::program_options;

		std::stringstream sstr;

		sstr << std::endl
			<< "Program Description" << std::endl
			<< "-------------------" << std::endl << std::endl
			<< "A TCP socket listener that starts and stops the video service.";

		app_opts::options_description desc ( sstr.str() ), conf_desc( "" );
		app_opts::variables_map opts_map;

		desc.add_options()
			( "help", "produce this message" )
			( "listening-port", app_opts::value<int>()->default_value( 2001 ), "The server's listening port." )
			( "server-script"
			  , app_opts::value<std::string>()->default_value( std::string( "/etc/init.d/videoserver_single" ) )
			  , "" )
			( "verbosity", app_opts::value<int>()->default_value( 0 ), "Program messages detail." )
			;

		app_opts::store( app_opts::parse_command_line( argc, argv, desc ), opts_map );
		app_opts::notify( opts_map );

		if( !opts_map["help"].empty() )
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		global_log_level = static_cast<auxiliary_libraries::log_level>( opts_map["verbosity"].as<int>() );

		boost::asio::io_service io_service;
		tcp::endpoint endpoint(tcp::v4(), opts_map["listening-port"].as<int>() );
		tcp::acceptor acceptor(io_service, endpoint);
		boost::array<char, MAX_MSG_BUFFER_LENGTH> msg_buf;

		while(true)
		{
			tcp::socket socket( io_service );
			boost::system::error_code err_code;
			acceptor.accept( socket, err_code );
			int bytes_received;

			try
			{
			if ( !err_code )
			{
				std::stringstream bufstream;


				bytes_received=socket.receive( boost::asio::buffer( msg_buf ) );
				int astart=0,aend=bytes_received;
				for(;astart<bytes_received;astart++)
					if(msg_buf[astart]=='{')
						break;
				for(;aend>=astart;aend--)
					if(msg_buf[aend]=='}')
						break;
				boost::iterator_range<char*> subarray(&msg_buf[astart], &msg_buf[aend+1]);
				bufstream << "{\"root\":" << subarray << "\n}\0";

				LOG_CLOG( log_debug_0 ) << "Message send from client:\n" << bufstream.str();

				boost::property_tree::ptree pt;
				boost::property_tree::read_json( bufstream, pt );
				const boost::property_tree::ptree::value_type& mt = *pt.get_child("root.Messages").begin();

				std::string sender = pt.get<std::string>("root.Sender")
    				, command = mt.second.get<std::string>("Type").compare( "StartRecording" )?"stop":"start"
    				, mac_address = mt.second.get<std::string>("MacAddress")
    				, file_name = mt.second.get<std::string>("FileName")
    				, hostname = mt.second.get<std::string>("HostName");

    			LOG_CLOG( log_debug_0 ) << "Parameters identified:\n"
    									<< "Sender : " << sender
    									<< "\nCommand Type : " << command
    									<< "\nMAC Address : " << mac_address
    									<< "\nHost name : " <<  hostname
    									<< "\nFile name : " << file_name;

    			msg_buf.fill(0);

    			std::stringstream shellstream;
    			shellstream << opts_map["server-script"].as<std::string>()
    						<< " " <<  command << " ";
			if(mac_address.length()!=0)
				shellstream << mac_address;
			else
				if(hostname.length()!=0)
					shellstream << hostname;
				else
				{
					LOG_CLOG( log_error ) << "Invalid message. Ignoring...";
					continue;
				}
						


			if(command.length()!=0&&file_name.length()!=0)
			{
				shellstream << " " << file_name;
    				LOG_CLOG( log_debug_0 ) << shellstream.str();
    				system( shellstream.str().c_str() );
			}
			else
				LOG_CLOG( log_error ) << "Invalid message. Ignoring...";
			}

			}
			catch( const boost::exception& e )
			{
				LOG_CLOG( log_debug_1 ) << boost::diagnostic_information( e );
				LOG_CLOG( log_error ) << "Invalid message. Ignoring...";
			}
		}

	}
	catch( const boost::exception& e )
	{
		LOG_CLOG( log_debug_1 ) << boost::diagnostic_information( e );
		LOG_CLOG( log_error ) << "Fatal error occurred. Exiting...";

		return EXIT_FAILURE;
	}

	return 0;
}
