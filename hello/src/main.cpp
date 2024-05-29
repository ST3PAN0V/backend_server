#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
namespace chrono = std::chrono;
using namespace std::literals;
using boost::system::error_code;


class Robot : public std::enable_shared_from_this<Robot> {
    ...
    template <typename Callback>
    void Walk(int distance, Callback&& cb) {
        const auto t = 1s * distance / SPEED;
        std::cout << id_ << "> Walk for "sv << t.count() << "sec\n"sv;

        timer_.expires_after(chrono::duration_cast<Duration>(t));

        // self ссылается на текущий объект и продлевает ему время жизни
        timer_.async_wait([distance, cb = std::forward<Callback>(cb),
                           self = shared_from_this()](error_code ec) {
            if (ec) throw std::runtime_error(ec.what());
            self->walk_distance_ += distance;
            std::cout << self->id_ << "> Walked distance: "sv << self->walk_distance_ << "m\n"sv;
            cb();
        });
    }

    template <typename Callback>
    void Rotate(int angle, Callback&& cb) {
        const auto t = 1s * std::abs(angle) / ROTATION_SPEED;
        std::cout << id_ << "> Rotate for "sv << t.count() << "sec\n"sv;

        timer_.expires_after(chrono::duration_cast<Duration>(t));

        timer_.async_wait(
            [angle, cb = std::forward<Callback>(cb), self = shared_from_this()](error_code ec) {
                if (ec) throw std::runtime_error(ec.what());
                self->angle_ = (self->angle_ + angle) % 360;
                std::cout << self->id_ << "> Rotation angle: "sv << self->angle_ << "deg.\n"sv;
                cb();
            });
    }

private:
    net::steady_timer timer_;
    int id_;
    int angle_ = 0;
    int walk_distance_ = 0;
}; 

void RunRobots(net::io_context& io) {
    auto r1 = std::make_shared<Robot>(io, 1);
    auto r2 = std::make_shared<Robot>(io, 2);

    r1->Rotate(60, [r1] {
        r1->Walk(4, [r1] {});
    });
    r2->Walk(2, [r2] {
        r2->Walk(3, [r2] {});
    });
} 

int main() {
    net::io_context io;
    RunRobots(io);
    for (;;) {
        try {
            io.run();
            break;
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    }
    std::cout << "Done\n"sv;
} 