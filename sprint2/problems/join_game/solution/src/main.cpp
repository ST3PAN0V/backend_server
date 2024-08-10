#include <sdk.h>
//
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options.hpp>
#include <memory>
#include <iostream>
#include <thread>

#include <json_loader.h>
#include <network/request_handler.h>
#include <logger/logger.h>
#include <ticker.h>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace po = boost::program_options;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned workers_count, const Fn& fn) {
    auto _workers_count = std::max(1u, workers_count);
    std::vector<std::jthread> workers;
    workers.reserve(_workers_count - 1);
    // Запускаем workers_count-1 рабочих потоков, выполняющих функцию fn
    while (--_workers_count) {
        workers.emplace_back(fn);
    }
    fn();
}

[[nodiscard]] std::optional<http_handler::Args> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{"All options"s};
    // Выводим описание параметров программы
    http_handler::Args args;
    uint64_t tick_time = 0;
    desc.add_options()
        // Параметр --help (-h) должен выводить информацию о параметрах командной строки.
        ("help,h", "help message")
        // Параметр --tick-period (-t) задаёт период автоматического обновления игрового состояния в миллисекундах. 
        // Если этот параметр указан, каждые N миллисекунд сервер должен обновлять координаты объектов. 
        // Если этот параметр не указан, время в игре должно управляться с помощью запроса /api/v1/game/tick к REST API
        ("tick-period,t", po::value(&args.tick_time)->value_name("milliseconds"s), "auto tick time in milliseconds")
        // Параметр --config-file (-c) задаёт путь к конфигурационному JSON-файлу игры.
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "game config file path")
        // Параметр --www-root (-w) задаёт путь к каталогу со статическими файлами игры.
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "resource root directory path")
        // Параметр --randomize-spawn-points включает режим, при котором пёс игрока появляется в случайной точке случайно выбранной дороги карты.
        ("randomize-spawn-points", "random dog spawn position");
    
    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.contains("help")) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << desc;
        return std::nullopt;
    }

    if (vm.contains("tick-period")) {
        args.use_tick_api = false;
    }
    else {
        args.use_tick_api = true;
    }
    // Проверяем наличие опций src и dst
    if (!vm.contains("config-file")) {
        throw std::runtime_error("Config files have not been specified");
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static file path is not specified");
    }
    if (vm.contains("randomize-spawn-points")) {
        args.randomize_spawn_points = true;
    }
    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

}  // namespace

int main(int argc, const char* argv[]) {
    http_handler::Args args;
    try {
        if (auto args_opt = ParseCommandLine(argc, argv)) {
            args = std::move(args_opt.value());
        }
        else {
            return EXIT_SUCCESS;
        }
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    try {
        // 0. Init boost log filter
        logger::InitBoostLogFilter();

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args.config_file);
        game.SetRandomizeSpawnPoints(args.randomize_spawn_points);        


        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                LOG_MSG().server_stop(ec);
                ioc.stop();
            }
        });

        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game, args, api_strand};
        http_handler::LoggingRequestHandler logging_handler{handler};

        // 5. Настраиваем вызов метода Game::Tick каждые tick_time миллисекунд внутри api_strand
        auto ticker = std::make_shared<ticker::Ticker>( api_strand,
                                                        args.tick_time,
                                                        [&game](uint64_t delta) { 
                                                            game.Tick(delta); 
                                                        }
        );

        // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& send, auto&& socket) {
            logging_handler(
                std::forward<decltype(req)>(req), 
                std::forward<decltype(send)>(send), 
                socket);
        });
        // 7. Запустить ticker
        ticker->Start();

        // 8. Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        LOG_MSG().server_start(address.to_string(), port);

        // 9. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
