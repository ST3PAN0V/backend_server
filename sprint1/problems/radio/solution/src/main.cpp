#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using net::ip::udp;
using namespace std::literals;

int main(int argc, char** argv) {
    static const int port = 3333;

    if (std::string(argv[1]) == "client") {
        std::cout << "Client start recording..." << std::endl;

        Recorder recorder(ma_format_u8, 1);
        Player player(ma_format_u8, 1);

        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(argv[2], ec), port);

        while(true) {
            std::string str;
            std::getline(std::cin, str);
            auto rec_result = recorder.Record(65000, 1.5s);

            size_t frame_size = recorder.GetFrameSize();
            size_t num_frames = rec_result.data.size() / frame_size; // количество фреймов = общее количество байт / размер одного фрейма
            size_t total_bytes = num_frames * frame_size;

            socket.send_to(net::buffer(rec_result.data.data(), total_bytes), endpoint);
        }

    } else { // server
        std::cout << "Server start listening..." << std::endl;

        Player player(ma_format_u8, 1);

        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        while(true) {
            std::array<char, 65000> recv_buf;
            udp::endpoint remote_endpoint;
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            size_t frame_size = player.GetFrameSize();
            size_t num_frames = size / frame_size; // количество фреймов = общее количество байт / размер одного фрейма

            player.PlayBuffer(recv_buf.data(), size, 1.5s);

        }
    }
}
