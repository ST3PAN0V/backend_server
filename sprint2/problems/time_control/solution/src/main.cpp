#include "sdk.h"

#include <iostream>
#include <thread>

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>

#include <boost/json.hpp>

#include <boost/beast/http.hpp>


#include "json_loader.h"
#include "request_handler.h"


//для запуска из build/bin
//./game_server ../../data/config.json ../../static
//для запуска из build
//bin/game_server ../data/config.json ../static

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;
namespace http = boost::beast::http;

namespace 
{
    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn)
    {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--n) 
        {
            workers.emplace_back(fn);
        }
        fn();
    }
}  // namespace

static const std::string ip_address{"0.0.0.0"};
const auto address = net::ip::make_address(ip_address);
constexpr net::ip::port_type port = 8080;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void MyFormatter(const logging::record_view& rec, 
                 logging::formatting_ostream& strm) 
{
    auto ts = *rec[timestamp];

    std::string time = to_iso_extended_string(ts);    
    json::object obj{
        {"timestamp", time},
        {"message", *rec[logging::expressions::smessage]},        
        {"data", *rec[additional_data] }
    };
    strm<<serialize(obj);
} 

void BoostLogConfigure()
{
    boost::log::add_common_attributes();
    
    logging::add_console_log( 
        std::cout,        
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
    ); 
}

template<class SomeRequestHandler>
class LoggingRequestHandler
{
        static void LogRequest(const net::ip::tcp::socket& socket,
                               const http::request<http::string_body>& r)
        {
            using namespace std::literals;
            
            const std::string ip{socket.lowest_layer().remote_endpoint().address().to_string()} ;
            const std::string method{http::to_string(r.method())};
            const std::string URI{r.target()};
            json::value custom_data{
                {"ip"s, ip},
                {"URI"s, URI},
                {"method"s, method}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "request received"sv;
        }
        
        static void LogResponse(int64_t response_time, 
                                int code, 
                                std::string_view content_type)
        {
            using namespace std::literals;
            
            json::value custom_data{
                {"response_time"s, response_time},
                {"code"s, code},
                {"content_type"s, content_type}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                        << "response sent"sv;
        }
    
    public:
        template <typename Body, typename Allocator, typename Send>
        void operator () (const net::ip::tcp::socket& socket, 
                          http::request<Body, http::basic_fields<Allocator>>&& req, 
                          Send&& send) 
        {
            LogRequest(socket, req);
            
            using namespace std::chrono;
            system_clock::time_point start_ts{system_clock::now()};
        
            decorated_(socket, std::forward<decltype(req)>(req),  
                       [&send, &start_ts](auto&& response)
            {   
                const auto diff = system_clock::now() - start_ts;
                const int64_t dd = duration_cast<milliseconds>(diff).count();                
                                             
                LogResponse(dd, 
                            response.result_int(), 
                            response.at(http::field::content_type));        
                send(std::move(response));
            });
        }
        
        explicit LoggingRequestHandler(SomeRequestHandler& decorated):
        decorated_{decorated}
        {}
    
    private:
        SomeRequestHandler& decorated_;
};

namespace http_server 
{
    void ReportError(beast::error_code ec, std::string_view what) 
    {
        using namespace std::literals;
        
        json::value custom_data{
            {"code"s, ec.value()},
            {"text"s, ec.message()},
            {"where"s, what}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "error"sv;
    }
}

int main(int argc, const char* argv[]) 
{
    if (argc != 3)
    {
        std::cerr << "Usage: game_server <game-config-json> <stati-content-folder>"sv << std::endl;
        return EXIT_FAILURE;
    }
    
    BoostLogConfigure();
    
    
    try 
    { 
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
    
        if(game.GetMaps().empty())
        {
            //std::string path{argv[1]};
            //std::cerr << "File game-config is empty: "sv<< path << std::endl;
            json::value custom_data{
                {"code"s, EXIT_FAILURE}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                        << "server exited"sv;
            return EXIT_FAILURE;
        }
                
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        auto api_strand = net::make_strand(ioc);
        
        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number)
        {
            if (!ec) 
            {
                ioc.stop();
            }
        });
        
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game, std::string{argv[2]}, api_strand};

        LoggingRequestHandler<http_handler::RequestHandler> logging_handler{handler};
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов        
        http_server::ServeHttp(ioc, {address, port}, 
                               [&logging_handler](
                               const net::ip::tcp::socket& socket, 
                               auto&& req, 
                               auto&& send) 
        {
            logging_handler(socket, 
                            std::forward<decltype(req)>(req), 
                            std::forward<decltype(send)>(send));
        });
        

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        //std::cout << "Server has started..."sv << std::endl;
        json::value custom_data{
            {"port"s, port}, 
            {"address"s, ip_address}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "server started"sv;
            
        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] 
        {
            ioc.run();
        });
    } 
    catch (const std::exception& ex) 
    {
        std::string what_str{ex.what()}; 
        json::value custom_data{
            {"code"s, EXIT_FAILURE},
            {"exception"s, what_str}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                    << "server exited"sv;
        return EXIT_FAILURE;
    }
    
    json::value custom_data{
        {"code"s, 0}
    };
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data)
                                << "server exited"sv;
    return 0;
}
