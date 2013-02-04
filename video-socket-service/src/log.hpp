// -------------------------------------------------------------------------------------------------------------
// log.hpp
// -------------------------------------------------------------------------------------------------------------
// Author(s)     : Kostas Tzevanidis
// Contact       : ktzevanid@gmail.com
// Last revision : October 2012
// -------------------------------------------------------------------------------------------------------------

/**
 * @file log.hpp
 * @ingroup aux-libs
 * @brief A C++ logging header library
 */

#ifndef LOG_HPP
#define LOG_HPP

#include <boost/thread.hpp>

#include <iostream>
#include <sstream>

/**
 * @ingroup aux-libs
 * @brief Output log to <em>std::cout</em>.
 * @param level detail level
 *
 * @sa std::cout
 */
#define LOG_COUT( level ) \
		if( level > ::global_log_level ); \
		else auxiliary_libraries::log<log_2_cout>( level )

/**
 * @ingroup aux-libs
 * @brief Output log to buffered <em>std::clog</em>.
 * @param level detail level
 *
 * @sa std::clog
 */
#define LOG_CLOG( level ) \
		if( level > ::global_log_level ); \
		else auxiliary_libraries::log<log_2_clog>( level )

/**
 * @ingroup aux-libs
 * @brief Output log to buffered <em>std::cerr</em>.
 * @param level detail level
 *
 * @sa std::cerr
 */
#define LOG_CERR( level ) \
		if( level > ::global_log_level ); \
		else auxiliary_libraries::log<log_2_cerr>( level )

/**
 * @ingroup aux-libs
 * @brief Output log to output file stream.
 * @param level detail level
 * @param fout file stream
 */
#define LOG_FILE( level, fout ) \
		if( level > ::global_log_level ); \
		else auxiliary_libraries::log<log_2_file>( fout, level )

namespace auxiliary_libraries
{
	/**
	 * @ingroup aux-libs
	 * @brief Logging detail level.
	 */
	enum log_level
	{
		log_error = 0
	  , log_info
	  , log_warning
	  , log_debug_0
	  , log_debug_1
	  , log_debug_2
	};

	/**
	 * @ingroup aux-libs
	 * @brief Type for policy distinction
	 */
	struct log_2_clog;

	/**
	 * @ingroup aux-libs
	 * @brief Type for policy distinction
	 */
	struct log_2_cerr;

	/**
	 * @ingroup aux-libs
	 * @brief Type for policy distinction
	 */
	struct log_2_cout;

	/**
	 * @ingroup aux-libs
	 * @brief Type for policy distinction
	 */
	struct log_2_file;

	/**
	 * @ingroup aux-libs
	 * @brief Base logging class.
	 *
	 * Base of concrete logging policies.
	 */
	class log_base
	{
	public:

		/**
		 * @brief Base constructor
		 *
		 * @param a_loglevel logging detail level
		 */
		explicit log_base( log_level a_loglevel = log_error )
		{
			this->m_buffer << "Thread ID: " << boost::this_thread::get_id() << " - ";

			switch( a_loglevel )
			{
			case log_error:
				this->m_buffer << "ERROR: ";
				break;
			case log_info:
				this->m_buffer << "INFO: ";
				break;
			case log_warning:
				this->m_buffer << "WARNING: ";
				break;
			case log_debug_0:
				this->m_buffer << "DEBUG 0: ";
				break;
			case log_debug_1:
				this->m_buffer << "DEBUG 1: ";
				break;
			case log_debug_2:
				this->m_buffer << "DEBUG 2: ";
				break;
			}
		}

		/**
		 * @brief Overload of insertion operator for messages streaming
		 *
		 * @tparam T message type
		 */
		template<typename T>
		log_base& operator << ( T const& value )
		{
			m_buffer << value;
			return *this;
		}

		/**
		 * @brief Trivial destructor
		 */
		virtual ~log_base() {}

	protected:

		/**
		 * @brief message buffer
		 */
		std::ostringstream m_buffer;
	};

	/**
	 * @ingroup aux-libs
	 * @brief Logging policies prototype
	 */
	template<class base_policy>
	class log : public log_base
	{ };

	/**
	 * @ingroup aux-libs
	 * @brief Logging to buffered output stream.
	 */
	template<>
	class log<log_2_clog> : public log_base
	{
	public:

		/**
		 * @brief Trivial constructor
		 *
		 * @param a_level logging detail indicator
		 */
		log( log_level a_level = log_error ) : log_base( a_level ) { }


		/**
		 * @brief Trivial destructor
		 *
		 * Flushes the buffer to the output stream.
		 */
		~log()
		{
			m_buffer << std::endl;
			std::clog << m_buffer.str();
		}
	};

	/**
	 * @ingroup aux-libs
	 * @brief Logging to standard error output.
	 */
	template<>
	class log<log_2_cerr> : public log_base
	{
	public:

		/**
		 * @brief Trivial constructor
		 *
		 * @param a_level logging detail indicator
		 */
		log( log_level a_level = log_error ) : log_base( a_level ) { }

		/**
		 * @brief Trivial destructor
		 *
		 * Flushes the log buffer to the output stream.
		 */
		~log()
		{
			m_buffer << std::endl;
			std::cerr << m_buffer.str();
		}
	};

	/**
	 * @ingroup aux-libs
	 * @brief Logging to standard output.
	 */
	template<>
	class log<log_2_cout> : public log_base
	{
	public:

		/**
		 * @brief Trivial constructor
		 *
		 * @param a_level logging detail indicator
		 */
		log( log_level a_level = log_error ) : log_base( a_level ) { }

		/**
		 * @brief Trivial destructor
		 *
		 * Flushes the log buffer to the output stream.
		 */
		~log()
		{
			m_buffer << std::endl;
			std::cout << m_buffer.str();
		}
	};

	/**
	 * @ingroup aux-libs
	 * @brief Logging to file.
	 */
	template<>
	class log<log_2_file> : public log_base
	{
	public:

		/**
		 * @brief Trivial constructor
		 *
		 * @param a_filestream output file stream
		 * @param a_level logging detail indicator
		 */
		log( std::ofstream& a_filestream, log_level a_level = log_error )
		  : log_base( a_level )
		  , m_filestream( a_filestream ){ }

		/**
		 * @brief Trivial destructor
		 *
		 * Flushes the log buffer to the output stream.
		 */
		~log()
		{
			m_buffer << std::endl;
			m_filestream << m_buffer.str();
		}

	private:

		/**
		 * @brief Output file stream.
		 */
		std::ofstream& m_filestream;
	};
}

/**
 * @ingroup aux-libs
 * @brief Global logging detail level indicator.
 */
extern auxiliary_libraries::log_level global_log_level;

#endif /* LOG_HPP */
