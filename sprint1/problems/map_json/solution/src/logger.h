#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/settings.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp>
#include <string>
#include <iostream>

using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int) 
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::object)

inline void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object formattedData;
    auto ts = rec[timestamp];
    formattedData["timestamp"] = to_iso_extended_string(*ts);
    formattedData["data"] = *rec[additional_data];
    formattedData["message"] = *rec[expr::smessage];
    strm << formattedData << std::endl;
}

inline void init_logging() {
    logging::add_common_attributes();
    
    logging::add_console_log( 
        std::cout,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    );
}

