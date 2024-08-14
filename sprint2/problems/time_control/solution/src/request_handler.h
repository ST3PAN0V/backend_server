#pragma once

#include <iostream>

#include <utility>
#include <filesystem>
#include <variant>

#include "http_server.h"
#include "model.h"

#include "api_handler.h"
#include "common_request_handler.h"

namespace http_handler 
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    
    using namespace std::literals;
    
        
    
    class RequestHandler:
            public std::enable_shared_from_this<RequestHandler> 
    {
        public:
            using Strand = net::strand<net::io_context::executor_type>;
            using FileRequestResult = std::variant<StringResponse, FileResponse>;
            
            explicit RequestHandler(model::Game& game, 
                                    const std::string& stat_cont_path,
                                    Strand api_strand)
                : root_folder_{std::filesystem::weakly_canonical(
                                   std::filesystem::current_path() / stat_cont_path)}
                , m_apiHandler{game}
                , api_strand_{api_strand}
            {
            }
        
            RequestHandler(const RequestHandler&) = delete;
            RequestHandler& operator=(const RequestHandler&) = delete;
        
            template <typename Body, typename Allocator, typename Send>
            void operator()(const net::ip::tcp::socket& socket, 
                            http::request<Body, http::basic_fields<Allocator>>&& req, 
                            Send&& send) 
            {                                                           
                const std::string target{req.target()};
                
                if(m_apiHandler.hasApiRequest(target))
                {     
                    auto handle = [req = std::forward<decltype(req)>(req), this, &send]()
                    {
                        //Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(this->api_strand_.running_in_this_thread());
                        auto responce = this->m_apiHandler.ApiProcessRequest(req);
                                                                        
                        send(responce);  
                    };
                    
                    net::dispatch(api_strand_, handle);

                    return;
                }
                
                //запрос статики

                
                std::visit(
                    [&send] (auto&& result) 
                    {
                        send(std::forward<decltype(result)>(result));
                    },
                    StaticProcessRequest(req));   
            }
        
        private:            
            std::filesystem::path root_folder_;            
            ApiHandler m_apiHandler;
            Strand api_strand_;
            
        private:   
            FileRequestResult StaticProcessRequest(const StringRequest& req) const; 
            
            std::string decodeURL(const std::string& message) const;            
            bool hasFile(std::filesystem::path file) const;
            bool hasRootRequest(const std::string& target) const;
            std::filesystem::path decodePath(const std::string& target) const;
            
            static bool IsSubPath(std::filesystem::path path, std::filesystem::path base);
            static std::string_view mime_type(std::string_view path);
    };

}  // namespace http_handler
