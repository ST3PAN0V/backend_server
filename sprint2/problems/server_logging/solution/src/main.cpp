#include "sdk.h"
#include "json_loader.h"
#include "request_handler.h"

#include <boost/asio/signal_set.hpp>
#include <thread>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace beast = boost::beast;
using tcp = net::ip::tcp;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-folder>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // Инициализируем и настраиваем boost логгер 
        init_logging();

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                json::object errorData{{"code"s, ec.value()}};
                BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, errorData) << "server exited"sv;
                ioc.stop();
            } else {
                json::object errorData{{"code"s, ec.value()}, {"exception", ec.what()}};
                BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, errorData) << "server exited"sv;
            }
            
        });
        
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler basicHandler{game};
        http_handler::LoggingRequestHandler loggingHandler{basicHandler};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        net::ip::address address = net::ip::make_address("0.0.0.0");
        const unsigned int port = 8080;

        tcp::endpoint endpoint{address, port};

        http_server::ServeHttp(ioc, endpoint, [&loggingHandler, &game, &argv, &endpoint](auto&& req, auto&& send) {
            loggingHandler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), argv[2], endpoint);
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        //std::cout << "Server has started..."sv << std::endl;

        json::object loggingData{{"port"s, port}, {"address"s, address.to_string()}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, loggingData) << "server started"sv;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

    } catch (const std::exception& ex) {
        json::object errorData{{"code"s, ex.what()}, {"exception", ex.what()}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, errorData) << "main"sv;
        //std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
