#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <thread>
#include <cstdlib>

#include "sdk.h"
#include "parser.h"
#include "json_loader.h"
#include "request_handler.h"
#include "api_handler.h"
#include "logger.h"
#include "state_storage.h"
#include "db/database.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

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

    const char* db_url = std::getenv(DB_URL_ENV_NAME);
    if (!db_url) {
        std::cout << DB_URL_ENV_NAME + " environment variable not found"s << std::endl;
        return EXIT_FAILURE;
    }    
    try {
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        auto json_object = json_loader::GetRootJsonObject(args.config_file);
        auto game = json_loader::LoadGame(json_object);
        auto loot_generator = json_loader::LoadLootGenerator(json_object);
        auto loot_data = json_loader::LoadLootData(json_object);

        App app{args.randomize_spawn_point};
        StateStorage storage(args.state_file, args.save_state_period_ms, game, app);
        storage.Read();

        Database db_{db_url};

        ApiHandler api_handler{ioc, game, app, storage, loot_generator, loot_data, db_, args.tick_time_ms};
        http_handler::RequestHandler handler{args.static_folder, api_handler};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &storage](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                Logger::LogServerStop(EXIT_FAILURE, ec.message());
            }
            else {
                Logger::LogServerStop(EXIT_SUCCESS);
            }
            storage.Write();
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
