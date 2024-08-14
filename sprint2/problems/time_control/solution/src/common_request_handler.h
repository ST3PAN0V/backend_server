#pragma once

#include <boost/beast/http.hpp>

namespace http_handler 
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    
    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;
    using FileResponse = http::response<http::file_body>;   
    using namespace std::literals;
    
    std::string ErrorToJson(const std::string& code, 
                            const std::string& message);
    
    struct ContentType 
    {
        ContentType() = delete;
        
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view TEXT_PLAIN = "text/plain"sv;
        
        constexpr static std::string_view APP_JSON = "application/json"sv;
    };
    
    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, 
                                      std::string_view body, 
                                      unsigned http_version,
                                      bool keep_alive,
                                      std::string_view content_type = ContentType::APP_JSON);
    StringResponse MakeStringNoCashResponse(http::status status, 
                                            std::string_view body, 
                                            unsigned http_version,
                                            bool keep_alive,
                                            std::string_view content_type = ContentType::APP_JSON);
    StringResponse MakeStringCashResponse(http::status status, 
                                          std::string_view body, 
                                          unsigned http_version,
                                          bool keep_alive,
                                          std::string_view content_type = ContentType::APP_JSON);
}
