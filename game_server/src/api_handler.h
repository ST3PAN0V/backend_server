#pragma once

#include "app/app.h"
#include "app/loot_data.h"
#include "model/model.h"

#include "ticker.h"
#include "state_storage.h"
#include "db/database.h"

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
    explicit ApiHandler(net::io_context& ioc, model::Game& game, App& app, StateStorage& storage, 
                        loot::Generator& loot_generator, loot::Data& loot_data, Database& db, int tick_period = 0) 
    : game_(game)
    , app_(app)
    , storage_(storage)
    , loot_generator_(loot_generator)
    , loot_data_(loot_data)
    , db_(db)
    , tick_period_(tick_period)
    , api_strand_(net::make_strand(ioc)) {
        if (tick_period_) {
            auto ticker = std::make_shared<Ticker>(api_strand_, std::chrono::milliseconds(tick_period_),
                [&](std::chrono::milliseconds delta) { TickAction(delta.count()); }
            );
            ticker->Start();
        }
    }

    void GetMaps(const Callback& callback) const;
    void GetMapById(const std::string& id, const Callback& callback) const;
    void JoinGame(const std::string& body, const Callback& callback);
    void GetPlayers(const std::string& token_str, const Callback& callback) const;
    void GetGameState(const std::string& token_str, const Callback& callback) const;
    void PlayerAction(const std::string& token_str, const std::string& body, const Callback& callback) const;
    void GameTick(const std::string& body, const Callback& callback);
    void GetRecords(const std::optional<int>& start, const std::optional<int>& max_items, const Callback& callback) const;

private:
    void TickAction(int64_t time_ms);

    model::Game& game_;
    App& app_;
    StateStorage& storage_;
    loot::Generator& loot_generator_;
    loot::Data& loot_data_;
    Database& db_;
    int tick_period_;
    net::strand<net::io_context::executor_type> api_strand_;
};
