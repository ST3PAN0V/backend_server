#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>

namespace http_handler
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace json = boost::json;
	using namespace std::literals;

	// Запрос, тело которого представлено в виде строки
	using StringRequest = http::request<http::string_body>;
	// Ответ, тело которого представлено в виде строки
	using StringResponse = http::response<http::string_body>;

	struct ContentType
	{
		ContentType() = delete;
		constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
		// При необходимости внутрь ContentType можно добавить и другие типы контента
	};

	// Создаёт StringResponse с заданными параметрами
	StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type = ContentType::APPLICATION_JSON);

	StringResponse HandleRequest(auto&& req, model::Game& game)
	{

		const auto text_response = [&req](http::status status, std::string_view text)
			{
				return MakeStringResponse(status, text, req.version(), req.keep_alive());
			};

		std::string_view req_type = req.method_string();

		if (req_type != "GET"sv)
		{
			return text_response(http::status::method_not_allowed, { "Invalid method" });
		}

		std::string_view target = req.target();

		size_t size = target.size();

		if (size >= 4)
		{
			json::array response;

			if (std::string_view(target.begin(), target.begin() + 5) == "/api/"sv)
			{
				if (target == "/api/v1/maps"sv || target == "/api/v1/maps/"sv)
				{
					//Sends Off JSON Dictionary With Maps
					
					for (auto& map : game.GetMaps())
					{
						json::object elem;

						elem.emplace("id", *map.GetId());
						elem.emplace("name", map.GetName());

						response.push_back(elem);
					}

					return text_response(http::status::ok, { json::serialize(response) });
				}

				if (size >= 13)
				{
					if (std::string_view(target.begin(), target.begin() + 13) == "/api/v1/maps/"sv)
					{
						using Id = util::Tagged<std::string, model::Map>;
						Id id{ std::string(target.begin() + 13, target.end()) };

						const model::Map* maptr = game.FindMap(id);

						if (maptr != nullptr)
						{
							//JSON object with map data
							json::object elem;

							//Initializing object using map id and name
							elem.emplace("id", *maptr->GetId());
							elem.emplace("name", maptr->GetName());

							//Road container
							json::array roads;

							for (const model::Road& road_data : maptr->GetRoads())
							{
								//Temporary road object
								json::object road;

								model::Point start = road_data.GetStart();
								model::Point end = road_data.GetEnd();

								road.emplace("x0", start.x);
								road.emplace("y0", start.y);

								if (road_data.IsHorizontal())
								{
									road.emplace("x1", end.x);
								}
								else
								{
									road.emplace("y1", end.y);
								}

								//Pushing road to the road container
								roads.push_back(road);
							}

							//Inserting road data to the map object
							elem.emplace("roads", roads);

							//Building container
							json::array buildings;

							for (const model::Building& building_data : maptr->GetBuildings())
							{
								//Temporary building object
								json::object building;

								model::Rectangle dimensions = building_data.GetBounds();

								building.emplace("x", dimensions.position.x);
								building.emplace("y", dimensions.position.y);
								building.emplace("w", dimensions.size.width);
								building.emplace("h", dimensions.size.height);

								//Pushing building to the building container
								buildings.push_back(building);
							}

							//Inserting building data to the map object
							elem.emplace("buildings", buildings);

							//Office container
							json::array offices;

							for (const model::Office& office_data : maptr->GetOffices())
							{
								//Temporary office object
								json::object office;

								office.emplace("id", *office_data.GetId());

								model::Point pos = office_data.GetPosition();
								office.emplace("x", pos.x);
								office.emplace("y", pos.y);

								model::Offset offset = office_data.GetOffset();
								office.emplace("offsetX", offset.dx);
								office.emplace("offsetY", offset.dy);

								//Pushing office to the building container
								offices.push_back(office);
							}

							//Inserting office data to the map object
							elem.emplace("offices", offices);

							//Printing elem instead of response :o
							return text_response(http::status::ok, { json::serialize(elem) });
						}
						else
						{
							//Throwing error if requested map isn't found

							json::object elem;

							elem.emplace("code", "mapNotFound");
							elem.emplace("message", "Map not found");

							return text_response(http::status::not_found, { json::serialize(elem) });
						}
					}
				}

			}				
			//Throwing error when URL starts with /api/ but doesn't correlate to any of the commands

			json::object elem;

			elem.emplace("code", "badRequest");
			elem.emplace("message", "Bad request");

			return text_response(http::status::bad_request, { json::serialize(elem) });
		}

		std::string response_str = "Why Are We Here?";

		return text_response(http::status::ok, { response_str });

	}

	//RequestHandler
	class RequestHandler
	{
	public:
		explicit RequestHandler(model::Game& game)
			: game_{ game }
		{}

		RequestHandler(const RequestHandler&) = delete;
		RequestHandler& operator=(const RequestHandler&) = delete;

		template <typename Body, typename Allocator, typename Send>
		void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send)
		{
			// Обработать запрос request и отправить ответ, используя send
			send(HandleRequest(req, game_));
		}

	private:
		model::Game& game_;
	};

}  // namespace http_handler
