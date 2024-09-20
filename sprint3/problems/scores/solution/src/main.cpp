#include "./../headers/sdk.h"
#include "./../headers/parser.h"
#include "./../headers/json_loader.h"
#include "./../headers/server_core/request_handler.h"
#include "./../headers/server_core/api_handler.h"
#include "./../headers/server_core/application.h"
#include "./../headers/logger/logger.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    parser::Args args;
    try {
        args = *parser::ParseCommandLine(argc, argv);
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    try {
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                Logger::LogServerStop(EXIT_FAILURE, ec.message());
            }
            else {
                Logger::LogServerStop(EXIT_SUCCESS);
            }
        });

        auto json_object = json_loader::GetRootJsonObject(args.config_file);
        auto game = json_loader::LoadGame(json_object);
        auto loot_generator = json_loader::LoadLootGenerator(json_object);
        auto loot_data = json_loader::LoadLootData(json_object);
        ApiHandler api_handler{ioc, game, loot_generator, loot_data, args.tick_time_ms, args.randomize_spawn_point};

        http_handler::RequestHandler handler{args.static_folder, api_handler};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        Logger::LogServerStart(address.to_string(), port);

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
