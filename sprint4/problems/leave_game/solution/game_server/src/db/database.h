#pragma once

#include <pqxx/pqxx>

#include <string>
#include <vector>
#include <optional>

#include "connection_pool.h"

struct Record {
    std::string name;
    int score;
    double time_s;
};

class Database {
public:
    explicit Database(const std::string& db_url);

    void AddRecords(const std::vector<Record>& records);
    
    void GetRecords(const std::optional<int>& start,
                    const std::optional<int>& max_items,
                    std::function<void(std::vector<Record>)> callback);

private:
    template <typename Callback = std::function<void(const pqxx::result&)>, typename... Args>
    void AsyncExecute(const pqxx::zview& query, Callback&& callback = [](const pqxx::result&){}, Args&&... args) {
        // Нужно распаковать аргументы в кортеж для передачи в асинхронный метод
        auto args_tuple = std::make_tuple(std::forward<Args>(args)...);

        connection_pool_.AsyncGetConnection(
            [this, query, callback = std::forward<Callback>(callback),
             args_tuple = std::move(args_tuple)] (ConnectionPool::ConnectionWrapper connection) mutable {
                pqxx::work work{*connection};

                // Теперь распакуем аргументы из кортежа в exec_query. Больно смотреть :(
                pqxx::result res;
                if constexpr (sizeof...(Args) > 0) {
                    // Если есть аргументы, передаем их в exec_params
                    res = std::apply([&work, &query](auto&&... unpacked_args) {
                        return work.exec_params(query, std::forward<decltype(unpacked_args)>(unpacked_args)...);
                    }, args_tuple);
                } else {
                    // Если нет аргументов, то просто выполняем запрос
                    res = work.exec(query);
                }

                work.commit();
                callback(res);
            }
        );
    }

    ConnectionPool connection_pool_;
};

