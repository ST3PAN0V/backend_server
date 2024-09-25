#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>

namespace logging = boost::log;
namespace json = boost::json;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

class Logger {

public:
    static Logger& GetInstance()  {
        static Logger instance;
        return instance;
    }

    static void LogServerStart(const std::string& address, int port);
    static void LogServerStop(int code, const std::string& exception = "");
    static void LogRequestReceived(const std::string& ip, std::string_view uri, std::string_view method);
    static void LogResponseSent(const std::string& ip, int response_time, int code, std::string_view content_type);
    static void LogError(int code, const std::string& text, std::string_view where);

private:
    Logger() {
        Init();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void Init();

    template<typename T>
    void Log(logging::trivial::severity_level level, const std::string& message, const T& data) {
        BOOST_LOG_SEV(lg, level) << logging::add_value(additional_data, data) << message;
    }
    
    static void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

    logging::sources::severity_logger<logging::trivial::severity_level> lg;
};
