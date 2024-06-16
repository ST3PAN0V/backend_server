#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <memory>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    enum class MoveInfo {
        MISS = 0,
        HIT  = 1,
        KILL = 2
    };

    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while (!IsGameEnded()) {
            PrintFields();
            if (my_initiative) {
                
                std::cout << "Your turn: ";

                auto move = SendMove(socket);
                int result = ReadResult(socket);
                switch (result) {
                    case 0:
                        other_field_.MarkMiss(move.second, move.first);
                        break;
                    case 1:
                        other_field_.MarkHit(move.second, move.first);
                        continue;
                    case 2:
                        other_field_.MarkKill(move.second, move.first);
                        continue;
                }
                my_initiative = false;

            } else {
                
                std::cout << "Wait your turn..." << std::endl;
                auto result = ReadMove(socket);
                SendResult(socket, result);
                if (result == SeabattleField::ShotResult::HIT || result == SeabattleField::ShotResult::KILL) continue;
                my_initiative = true;
            }
        }

    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    SeabattleField::ShotResult ReadMove(tcp::socket& socket) {
        auto opponent_move = ReadExact<2>(socket);

        std::cout << "Shot to " << (*opponent_move) << std::endl;

        auto coordinates = *ParseMove(*opponent_move);

        return(my_field_.Shoot(coordinates.second, coordinates.first));
    }
    

    int ReadResult(tcp::socket& socket) {
        auto answer = *ReadExact<1>(socket);
        std::string_view response;
        switch (std::stoi(answer)) {
            case 1: 
                response = "Hit!";
                break;
            case 2: 
                response = "Kill!";
                break;
            case 0:
                response = "Miss!";
                break;
        }
        std::cout<< response << std::endl;
        return std::stoi(answer);
    }

    void SendResult(tcp::socket& socket, SeabattleField::ShotResult shot_result) {
        std::string_view ans = std::to_string(static_cast<int>(shot_result));
        WriteExact(socket, ans);
    }

    std::pair<int, int> SendMove(tcp::socket& socket) {
        std::string move;
        std::cin >> move;
        WriteExact(socket, move);
        return *ParseMove(move);
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return;
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        return;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
