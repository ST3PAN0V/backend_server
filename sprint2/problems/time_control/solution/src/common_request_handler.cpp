#include <boost/json.hpp>

#include "common_request_handler.h"




namespace json = boost::json;



namespace http_handler 
{
    std::string ErrorToJson(const std::string& code,
                            const std::string& message)
    {
        json::object obj; 
        
        obj["code"] = code;
        obj["message"] = message;
        
        return serialize( obj );
    }
    
    
    StringResponse MakeStringResponse(http::status status, 
                                      std::string_view body,
                                      unsigned http_version, 
                                      bool keep_alive, 
                                      std::string_view content_type) 
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);//string_view const at(field name) const;
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }
    
    StringResponse MakeStringNoCashResponse(http::status status,
                                            std::string_view body,
                                            unsigned http_version,
                                            bool keep_alive, 
                                            std::string_view content_type)
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);//string_view const at(field name) const;
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        response.set(http::field::cache_control, "no-cache");
        return response;
    }
    
    StringResponse MakeStringCashResponse(http::status status, 
                                          std::string_view body, 
                                          unsigned http_version, 
                                          bool keep_alive, 
                                          std::string_view content_type)
    {
        StringResponse response(MakeStringResponse(status, 
                                                   body,
                                                   http_version, 
                                                   keep_alive, 
                                                   content_type));

        response.set(http::field::cache_control, "public");
        response.prepare_payload();
        return response;
    }

}
