#include "./../../headers/server_core/http_server.h"
#include "./../../headers/logger/logger.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

using namespace std::literals;

namespace http_server {

void ReportError(beast::error_code ec, std::string_view what) {
    Logger::LogError(ec.value(), ec.message(), what);
}

void ReportRequest(const std::string& ip, std::string_view uri, std::string_view method) {
    Logger::LogRequestReceived(ip, uri, method);
}

void ReportResponse(const std::string& ip, std::chrono::milliseconds response_time, int code, std::string_view content_type) {
    Logger::LogResponseSent(ip, response_time.count(), code, content_type);
}


void SessionBase::Run() {
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    using namespace std::literals;
        
    request_ = {};
    stream_.expires_after(30s);

    http::async_read(stream_, buffer_, request_,
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }

    request_start_time_ = std::chrono::steady_clock::now();
    ReportRequest(stream_.socket().remote_endpoint().address().to_string(),
                  request_.target(),
                  request_.method_string());
                  
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        return ReportError(ec, "close"sv);
    }
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        return Close();
    }

    auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - request_start_time_);
    ReportResponse(stream_.socket().remote_endpoint().address().to_string(),
                   response_time,
                   response_.result_int(),
                   response_.has_content_length() ? response_["Content-Type"] : "null"sv);

    Read();
}

}  // namespace http_server
