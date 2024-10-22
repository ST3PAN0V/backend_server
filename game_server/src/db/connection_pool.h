#pragma once

#include <boost/asio.hpp>
#include <pqxx/pqxx>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>
#include <cassert>
#include <functional>
#include <thread>

namespace net = boost::asio;

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}, pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const noexcept {
            return *conn_;
        }

        pqxx::connection* operator->() const noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    template <typename ConnectionFactory>
    ConnectionPool(ConnectionFactory&& connection_factory)
        : strand_(net::make_strand(io_context_)),
        work_guard_(net::make_work_guard(io_context_)) { // Добавляем work_guard_
        auto capacity = std::thread::hardware_concurrency();
        pool_.reserve(capacity);
        
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }

        for (size_t i = 0; i < capacity; ++i) {
            thread_pool_.emplace_back([this]() {
                io_context_.run();
            });
        }
    }


    template <typename CompletionToken>
    void AsyncGetConnection(CompletionToken&& token) {
        net::post(strand_, [this, token = std::forward<CompletionToken>(token)]() mutable {

            std::unique_lock lock{mutex_};
            cond_var_.wait(lock, [this] {
                return used_connections_ < pool_.size();
            });

            auto conn = std::move(pool_[used_connections_++]);

            lock.unlock();
            token(ConnectionWrapper{std::move(conn), *this});
        });
    }

    void ReturnConnection(ConnectionPtr&& conn) {
        {
            std::lock_guard lock{mutex_};
            pool_[--used_connections_] = std::move(conn);
        }
        cond_var_.notify_one();
    }

    ~ConnectionPool() {
        work_guard_.reset(); // Сбрасываем work_guard_, чтобы позволить io_context завершиться
        io_context_.stop();
        for (auto& thread : thread_pool_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;

    net::io_context io_context_;
    net::strand<net::io_context::executor_type> strand_;
    std::vector<std::thread> thread_pool_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
};
