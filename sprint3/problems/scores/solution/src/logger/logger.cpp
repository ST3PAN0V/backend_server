#include "./../../headers/logger/logger.h"

#include <boost/json.hpp>

#include <iostream>

using namespace std::literals;


void Logger::Init() {
    logging::add_common_attributes();
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );

    logging::add_console_log(
        std::cout,
        keywords::format = &Logger::JsonFormatter,
        keywords::auto_flush = true
    );
}

void Logger::JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    json::object obj;

    auto timestamp = boost::log::extract<boost::posix_time::ptime>("TimeStamp", rec);
    if (timestamp) {
        obj.emplace("timestamp", boost::posix_time::to_iso_extended_string(*timestamp));
    }

    auto message = boost::log::extract<std::string>("Message", rec);
    if (message) {
        obj.emplace("message", *message);
    }

    auto additional_data = boost::log::extract<json::value>("AdditionalData", rec);
    if (additional_data) {
        obj.emplace("data", *additional_data);
    }

    strm << json::serialize(obj);
}

void Logger::LogServerStart(const std::string& address, int port) {
    json::object data{
        {"port", port},
        {"address", address}
    };
    Logger::GetInstance().Log(logging::trivial::info, "server started", data);
}

void Logger::LogServerStop(int code, const std::string& exception) {
    json::object data{
        {"code", code}
    };
    if (!exception.empty()) {
        data["exception"] = exception;
    }
    Logger::GetInstance().Log(logging::trivial::info, "server exited", data);
}

void Logger::LogRequestReceived(const std::string& ip, std::string_view uri, std::string_view method) {
    json::object data{
        {"ip", ip},
        {"URI", uri},
        {"method", method}
    };
    Logger::GetInstance().Log(logging::trivial::info, "request received", data);
}

void Logger::LogResponseSent(const std::string& ip, int response_time, int code, std::string_view content_type) {
    json::object data{
        {"ip", ip},
        {"response_time", response_time},
        {"code", code},
        {"content_type", content_type.empty() ? json::value(nullptr) : json::value(std::string{content_type})}
    };
    Logger::GetInstance().Log(logging::trivial::info, "response sent", data);
}

void Logger::LogError(int code, const std::string& text, std::string_view where) {
    json::object data{
        {"code", code},
        {"text", text},
        {"where", where}
    };
    Logger::GetInstance().Log(logging::trivial::error, "error", data);
}
