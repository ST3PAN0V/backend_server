#pragma once

#include "app.h"
#include "model.h"
#include "ticker.h"
#include "loot_data.h"

#include <boost/beast/http.hpp>

#include <stdexcept>

namespace http = boost::beast::http;
namespace net = boost::asio;
using Callback = std::function<void(const std::string&)>;

struct ApiException : public std::runtime_error {
    explicit ApiException(const std::string& message, const std::string& code, http::status status)
        : std::runtime_error(message), code(code), status(status) {}

    std::string code;
    http::status status;
};

class ApiHandler {
public:
    explicit ApiHandler(net::io_context& ioc, model::Game& game, loot::Generator& loot_generator, loot::Data& loot_data, int tick_period = 0, bool randomize_spawn = false) 
    : app_{randomize_spawn}
    , game_(game)
    , loot_generator_(loot_generator)
    , loot_data_(loot_data)
    , tick_period_(tick_period)
    , api_strand_(net::make_strand(ioc)) {
        auto ticker = std::make_shared<Ticker>(api_strand_, std::chrono::milliseconds(tick_period_),
            [&](std::chrono::milliseconds delta) { TickAction(delta.count()); }
        );
        ticker->Start();
    }

    void GetMaps(const Callback& callback) const;
    void GetMapById(const std::string& id, const Callback& callback) const;
    void JoinGame(const std::string& body, const Callback& callback);
    void GetPlayers(const std::string& token_str, const Callback& callback) const;
    void GetGameState(const std::string& token_str, const Callback& callback) const;
    void PlayerAction(const std::string& token_str, const std::string& body, const Callback& callback) const;
    void GameTick(const std::string& body, const Callback& callback);

private:
    void TickAction(int64_t time_ms);
    App app_;
    model::Game& game_;
    loot::Generator& loot_generator_;
    loot::Data& loot_data_;
    int tick_period_;
    net::strand<net::io_context::executor_type> api_strand_;
};
