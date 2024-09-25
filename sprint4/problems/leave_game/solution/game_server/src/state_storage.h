#pragma once

#include "model/model.h"
#include "app/app.h"
#include "serialization/manager.h"
#include "serialization/representations.h"

#include "logger.h"

#include <ranges>
#include <algorithm>


class StateStorage {
public:
    explicit StateStorage(const std::string& state_file, int save_state_period_ms, model::Game& game, App& app)
    : filename_(state_file)
    , save_state_period_ms_(save_state_period_ms)
    , game_(game)
    , app_(app)
    {}

    void Write(int64_t time_ms = 0) {
        if (time_ms && !IsTimeToSave(time_ms)) {
            return;
        }

        last_time_ms_ = 0;

        serialization::Manager manager{filename_, serialization::Manager::Mode::Write};

        std::vector<serialization::LootRepresentation> loot_reps;
        std::ranges::transform(game_.GetMaps(), std::back_inserter(loot_reps), [](const auto& map) {
            return serialization::LootRepresentation(map);
        });

        std::vector<serialization::PlayerRepresentation> player_reps;
        std::ranges::transform(app_.GetPlayers(), std::back_inserter(player_reps), [](const auto& pair) {
            const auto& [token, player_ptr] = pair;
            return serialization::PlayerRepresentation(token, *player_ptr);
        });

        try{
            manager.Write(loot_reps);
            manager.Write(player_reps);
        }
        catch(const std::exception& e) {
            Logger::LogError(0, e.what(), "state storage: write");
        }
    }

    void Read() {
        serialization::Manager manager{filename_, serialization::Manager::Mode::Read};

        std::vector<serialization::LootRepresentation> loot_reps;
        std::vector<serialization::PlayerRepresentation> player_reps;

        try{
            manager.Read(loot_reps);
            manager.Read(player_reps);
        }
        catch(const std::exception& e) {
            Logger::LogError(0, e.what(), "state storage: read");
        }
        
        std::ranges::for_each(loot_reps, [this](const auto& loot_rep) {
            auto* map = const_cast<model::Map*>(game_.FindMap(loot_rep.GetMapId()));
            if (map != nullptr) {
                map->AddLostObjects(loot_rep.Restore());
            }
        });

        
        std::ranges::for_each(player_reps, [this](const auto& player_rep) {
            auto [token, player] = player_rep.Restore();
            const auto* map = game_.FindMap(player_rep.GetMapId());
            player->SetMap(map);
            if (map != nullptr) {
                app_.AddPlayer(std::move(token), std::move(player));
            }
        });
    }

private:
    bool IsTimeToSave(int64_t time_ms) {
        last_time_ms_ += time_ms;
        return last_time_ms_ >= save_state_period_ms_;
    }

    std::string filename_;
    int64_t save_state_period_ms_;
    int64_t last_time_ms_ = 0;
    model::Game& game_;
    App& app_;
};
