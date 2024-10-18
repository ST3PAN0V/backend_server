#pragma once

#include <boost/program_options.hpp>

#include <optional>
#include <string>
#include <iostream>
#include <stdexcept>

namespace parser {
    
struct Args {
    std::string config_file;
    std::string static_folder;
    int tick_time_ms;
    bool randomize_spawn_point;
    std::string state_file;
    int save_state_period_ms;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    using namespace std::literals;
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_time_ms)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_folder)->value_name("dir"s), "set static folder")
        ("state-file", po::value(&args.state_file)->value_name("file"s), "set state file path")
        ("save-state-period", po::value(&args.save_state_period_ms)->value_name("milliseconds"s), "game time")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_point), "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file has not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files folder path is not specified"s);
    }
    
    return args;
}

} // namespace parser