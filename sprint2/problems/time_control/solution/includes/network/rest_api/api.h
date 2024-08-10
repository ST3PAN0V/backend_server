#pragma once
#include <network/rest_api/response_base.h>
#include <application.h>

namespace http_handler {

enum API_TYPE {
    MAPS_API = 0,
    GAME_JOIN,
    GAME_PLAYERS,
    GAME_STATE,
    GAME_PLAYER_ACTION,
    GAME_TICK,
    UNKNOWN
};

class Api : public ResponseBase {
    application::Application app_;
    bool use_tick_api_;

    virtual Response MakeGetHeadResponse(const StringRequest& req) override;
	virtual Response MakePostResponse(const StringRequest& req) override;
    virtual Response MakeUnknownMethodResponse(const StringRequest& req) override;

    API_TYPE RestApiHandlerType(const std::string_view& request_target);

    StringResponse GetPlayers(const StringRequest& request);
    StringResponse GetState(const StringRequest& request);
    StringResponse SetActionPlayer(const StringRequest& request);

    template<typename Fn>
    StringResponse ExecuteAuthorized(const StringRequest& request, Fn&& app_action);

public:
	Api(model::Game& game, bool use_tick_api) : 
        app_(game), 
        use_tick_api_(use_tick_api) {}
};

};