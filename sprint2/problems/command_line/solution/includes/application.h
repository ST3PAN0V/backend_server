#pragma once
#include <sdk.h>
#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <random>
#include <model.h>
#include <sstream>
#include <optional>
#include <iomanip>
#include <gameplay.h>

namespace application {

namespace net = boost::asio;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

enum APPLICATION_ERROR {
    APPLICATION_NO_ERROR,
    BAD_JSON,
    MAP_NOT_FOUND,
    INVALID_NAME,
    INVALID_TOKEN,
    UNKNOWN_TOKEN,
    INVALID_ARGUMENT
};

std::optional<std::string> check_token(const std::string& autorization_text);
std::string SerializeMessageCode(const std::string& code, const std::string& message);

class Application
{
public:
    explicit Application(model::Game& game) : game_{ game } {}

    std::string GetMapJson(const std::string& request_target, http::status& response_status);
    std::string Join(const std::string& jsonBody, APPLICATION_ERROR& join_error);
    std::string GetPlayers(const std::string& auth_message, APPLICATION_ERROR& auth_error);
    std::string GetState(const std::string& auth_message, APPLICATION_ERROR& app_error);
    std::string ActionPlayer(const std::string& auth_message, APPLICATION_ERROR& app_error, const std::string& jsonBody);
    std::string Tick(const std::string& jsonBody, APPLICATION_ERROR& app_error);
    gameplay::Player* GetPlayerFromToken(const std::string& auth_message, APPLICATION_ERROR& app_error, std::string& app_error_msg);
private:
    model::Game& game_;
    gameplay::Players players_;
    gameplay::PlayerTokens player_tokens_;

    gameplay::Player* GetPlayer(const std::string& name, const std::string& mapId);
};
}