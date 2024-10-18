#include "database.h"
#include "tagged_uuid.h"

using pqxx::operator"" _zv;

Database::Database(const std::string& db_url)
    : connection_pool_([db_url]() {return std::make_shared<pqxx::connection>(db_url);}) {
    AsyncExecute(
        R"( CREATE TABLE IF NOT EXISTS retired_players (
            uuid UUID NOT NULL,
            name VARCHAR(100) NOT NULL,
            score INT NOT NULL,
            playTime DOUBLE PRECISION NOT NULL,
            PRIMARY KEY (uuid)
        );)");

    AsyncExecute(
        R"( CREATE INDEX IF NOT EXISTS idx_retired_players_score_playtime_name
            ON retired_players (score DESC, playTime ASC, name ASC
        );)");
}

void Database::AddRecords(const std::vector<Record>& records) {
    for (const auto& record : records) {
        const auto uuid = util::TaggedUUID<struct PlayerTag>::New();
        
        AsyncExecute(
        R"( INSERT INTO retired_players (uuid, name, score, playTime)
            VALUES ($1, $2, $3, $4)
        )"_zv
        , [](const pqxx::result&){}
        , uuid.ToString(), record.name, record.score, record.time_s);
    }
}

void Database::GetRecords(const std::optional<int>& start, const std::optional<int>& max_items, std::function<void(std::vector<Record>)> callback) {
    int offset = start.value_or(0);
    int limit = max_items.value_or(100);

    auto records = std::make_shared<std::vector<Record>>();

    auto result_handler = [callback = std::move(callback), records](const pqxx::result& res) mutable {
        for (const auto& row : res) {
            records->emplace_back(Record{
                row["name"].c_str(),
                row["score"].as<int>(),
                row["playTime"].as<double>()/1000
            });
        }
        callback(std::move(*records));
    };

    AsyncExecute(
        R"( SELECT uuid, name, score, playTime
            FROM retired_players
            ORDER BY score DESC, playTime ASC, name ASC
            LIMIT $1 OFFSET $2
    )"_zv, 
    std::move(result_handler), 
    limit, offset);
}
