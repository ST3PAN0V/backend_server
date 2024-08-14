#include <sstream>
#include <regex>

#include "request_handler.h"

//#include <boost/json.hpp>


//namespace json = boost::json;
namespace fs = std::filesystem;
namespace sys = boost::system;

namespace http_handler 
{
    std::string_view RequestHandler::mime_type(std::string_view path)
    {
        using beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if(pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
        if(iequals(ext, ".htm"))  return "text/html";
        if(iequals(ext, ".html")) return "text/html";
        if(iequals(ext, ".php"))  return "text/html";
        if(iequals(ext, ".css"))  return "text/css";
        if(iequals(ext, ".txt"))  return "text/plain";
        if(iequals(ext, ".js"))   return "text/javascript";
        if(iequals(ext, ".json")) return "application/json";
        if(iequals(ext, ".xml"))  return "application/xml";                
        if(iequals(ext, ".png"))  return "image/png";
        if(iequals(ext, ".jpe"))  return "image/jpeg";
        if(iequals(ext, ".jpeg")) return "image/jpeg";
        if(iequals(ext, ".jpg"))  return "image/jpeg";
        if(iequals(ext, ".gif"))  return "image/gif";
        if(iequals(ext, ".bmp"))  return "image/bmp";
        if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if(iequals(ext, ".tiff")) return "image/tiff";
        if(iequals(ext, ".tif"))  return "image/tiff";
        if(iequals(ext, ".svg"))  return "image/svg+xml";
        if(iequals(ext, ".svgz")) return "image/svg+xml";
        if(iequals(ext, ".mp3"))  return "audio/mpeg";
        return "application/octet-stream";
    }
    
    
    
    bool RequestHandler::IsSubPath(fs::path path, fs::path base) 
    {
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(path);
        base = fs::weakly_canonical(base);
    
        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) 
        {
            if (p == path.end() || *p != *b)
            {
                return false;
            }
        }
        
        return true;
    }
       
        
    bool RequestHandler::hasRootRequest(const std::string& target) const
    {
        if(target.size() != 1)
        {            
            return false;
        }
        
        const char c = target.front();
        
        return c == '/';        
    }
    
    std::filesystem::path RequestHandler::decodePath(const std::string& target) const
    {
        std::filesystem::path tmp_path{root_folder_};
        tmp_path += decodeURL(target);
        
        std::filesystem::path abs_path = std::filesystem::weakly_canonical(tmp_path);
        
        return abs_path;
    }
    
    RequestHandler::FileRequestResult RequestHandler::StaticProcessRequest(
            const StringRequest& req) const
    {
        if((req.method() != http::verb::get) &&
           (req.method() != http::verb::head))
        {
            return MakeStringResponse(http::status::method_not_allowed, 
                                      "Invalid method"sv, 
                                      req.version(), 
                                      req.keep_alive());
        }
        
        std::string target{req.target()};
        
        std::filesystem::path currentPath{decodePath(target)};
        
        if(!IsSubPath(currentPath, root_folder_))
        {
            //неправильны путь              
            return MakeStringResponse(http::status::bad_request,
                                      "Bad request", 
                                      req.version(), 
                                      req.keep_alive(),
                                      ContentType::TEXT_PLAIN);            
        }
        
        
        std::string path{currentPath.generic_string()};
        if(hasRootRequest(target))
        {
            path.append("/index.html");
        }              
        
        beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), beast::file_mode::scan, ec);
        
        if(ec)
        {            
            return MakeStringResponse(http::status::not_found,
                                      target, 
                                      req.version(), 
                                      req.keep_alive(),
                                      ContentType::TEXT_PLAIN);
        }
        
        const auto size{body.size()};
        
        FileResponse out;
        out.version(11);  // HTTP/1.1
        out.result(http::status::ok);
        
        out.insert(http::field::content_type, mime_type(path));
        out.body() = std::move(body);
        out.content_length(size);
        out.keep_alive(req.keep_alive());
        return out;
    }
    
    std::string RequestHandler::decodeURL(const std::string& message) const
    {
        size_t posOfQMark = message.find_first_of('%');
        
        if(std::string::npos == posOfQMark)
        {
            return message;
        }
        
        std::string out;
        size_t lastPos{0};       
        while(std::string::npos != posOfQMark )
        {
            out.append(message.substr(lastPos, posOfQMark - lastPos));
            
            //вычисляем ascii код
            if((posOfQMark + 2) >= message.size())
            {
                //нетхватет данных для декодировки
                return out;
            }
            
            std::istringstream iss(message.substr(posOfQMark + 1, 2));
            int code;
            iss >> std::hex >> code;
            
            //сохраняем считанный код
            out += static_cast<char>(code);
            
            
            lastPos = posOfQMark + 3;
            
            //ищем дальше
            posOfQMark = message.find("%", posOfQMark + 3);
        }
        
        if(lastPos)
        {
            out.append(message.substr(lastPos, message.size() - lastPos));
        }
        
        return out;
    }
    
    
}  // namespace http_handler
