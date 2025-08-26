#include "webvplayer/webvplayer.hpp"
#include "crow/app.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/websocket.h"
#include "webvplayer/name_format.hpp"
#include "webvplayer/resource.hpp"
#include "webvplayer/video_player.hpp"
#include "webvplayer/mpv_video_player.hpp"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_set>
#include <ranges>

#include "webvplayer/webplayer_exceptions.hpp"

using webvplayer::Server;
using webvplayer::VideoPlayer;
using webvplayer::Resource;
using webvplayer::ResourceTree;
using NameFormatter = webvplayer::name_format::Formatter;
using std::vector;
using std::string;
using std::ifstream;
namespace fs = std::filesystem;
namespace json = crow::json;

namespace {
	auto jsonGet(json::rvalue const &json, string const &field) {
		try {
			return json[field];
		}
		catch (std::runtime_error const &e) {
			throw webvplayer::MissingRequiredConfigParamException(field);
		}
	}

	template<VideoPlayer::TrackType T>
	void trackListToJson(json::wvalue &dst, VideoPlayer::TrackList<T> const &src) {

		vector<json::wvalue> list;
		for(auto const &track : src) {
			list.push_back({
				{"title", track.title},
				{"id", track.id},
				{"selected", track.bSelected}
			});
		}
		dst = static_cast<json::wvalue>(list);
	}

	void loadResources(ResourceTree &resources, ResourceTree::Node const &parent, json::rvalue const &json) {
		for(auto const &res : json) {
			string name = res["name"].s();
	
			if(!res.has("path")) {
				if(!res.has("resources"))
					throw webvplayer::MissingRequiredConfigParamException("path (whithin resources block)");
				
				auto const & child = resources.add(parent, Resource(name));
				loadResources(resources, child, res["resources"]);
				continue;
			}

			fs::path path = fs::canonical(fs::path(res["path"].s()));
			resources.add(parent, Resource(name, path));
	
			CROW_LOG_INFO << "Resource dir at [" << path.string() << "].";
		}
	}

	template<VideoPlayer::TrackType T>
	crow::response selectTrack(VideoPlayer *player, crow::json::rvalue const &body) {
		if(!body.has("track")) {
			CROW_LOG_ERROR << "Missing parameter: \"track\".";
			return crow::response(crow::BAD_REQUEST);
		}

		if constexpr(T == VideoPlayer::TrackType::AUDIO)
			player->selectAudioTrack(body["track"].i());
		else if constexpr(T == VideoPlayer::TrackType::SUBTITLES)
			player->selectSubtitlesTrack(body["track"].i());

		return crow::response(crow::OK);
	}

	string toResourceName(string const &uri) {
		if(uri.size() == 0)
			return uri;

		auto splitted = uri | std::views::split('%');
		string resourceName;
		resourceName.reserve(uri.size());
		bool escaped = uri[0] == '%';
		for(auto token : splitted) {
			if(escaped && token.size() >= 2) {
				char byteStr[3] = {*(token.begin()), *(token.begin() + 1), 0};
				char byte = std::strtol(byteStr, nullptr, 16);
				resourceName.push_back(byte);
				resourceName.append(token.begin() + 2, token.end());
			}
			else {
				resourceName.append(token.begin(), token.end());
				escaped = true;
			}
		}

		return resourceName;
	}
}

Server::Server() noexcept {
	app_.loglevel(crow::LogLevel::DEBUG);
	CROW_ROUTE(app_, "/")([this]() { return this->listResources(""); });
	CROW_ROUTE(app_, "/list/")([this]() { return this->listResources(""); });
	CROW_ROUTE(app_, "/list/<path>")([this](fs::path const &resource) { return this->listResources(resource); });
	CROW_ROUTE(app_, "/player").methods(crow::HTTPMethod::GET)([this]() { return this->getPlayerStatus(); });
	CROW_ROUTE(app_, "/player").methods(crow::HTTPMethod::POST)([this](crow::request const &req) { return this->dispatchPlayerAction(req); });
	CROW_ROUTE(app_, "/poweroff").methods(crow::HTTPMethod::POST)([]() {system("poweroff"); return crow::response(crow::OK); });
	CROW_WEBSOCKET_ROUTE(app_, "/player_update").onopen(
		[this](crow::websocket::connection & conn) { this->addConnection(conn); }
	).onclose(
		[this](crow::websocket::connection & conn, std::string const &, unsigned short) { this->removeConnection(conn); }
	);
}

void Server::run(vector<string> const &args) {
	ifstream configFile;
	configFile.exceptions(std::ios::failbit | std::ios::badbit);

	auto it = args.begin(); 
	while(it != args.end()) {
		auto arg = *(it++);
		if(arg == "-c" || arg == "--config") {
			CROW_LOG_INFO << "Reading config file: [" << *it << "].";
			configFile.open(*(it++));
		}
		else 
			throw BadArgumentException(arg.c_str());
	}

	if(!configFile.is_open())
		throw MissingRequiredArgumentException("-c|--config");


	std::stringstream configBuf;
	configBuf << configFile.rdbuf();
	auto config = crow::json::load(configBuf.str());

	auto &cors = app_.get_middleware<crow::CORSHandler>();
	std::string frontHost = jsonGet(config, "front host").s();
	std::string frontPort = jsonGet(config, "front port").s();
	cors.global().origin(std::format("http://{}:{}", frontHost, frontPort));

	auto resourcesList = jsonGet(config, "resources");
	loadResources(resources_, resources_.root(), resourcesList);
	player_ = getPlayerByName(jsonGet(config,"video player").s());
	app_.port(jsonGet(config, "api port").i()).run();
}

VideoPlayer *Server::getPlayerByName(string const &player) {
	CROW_LOG_INFO << "Video player selected [" << player << "].";
	switch(mapVideoPlayer(player)) {
		case VideoPlayerId::MPV:
			return new MPVVideoPlayer();
		case VideoPlayerId::UNDEFINED:
		default:
			CROW_LOG_ERROR << "Player not supported. Using mpv.";
			return new MPVVideoPlayer();
	}
}

crow::response Server::listResources(fs::path resource) const {
	try {
		CROW_LOG_DEBUG << "Listing resources at [" << resource.string() << "].";
		crow::json::wvalue response(std::vector<crow::json::wvalue>{});

		resource = fs::path(toResourceName(resource.string()));
		
		for(ResourceTree::Node const &child : resources_.list(resource)) {
			response[response.size()] = crow::json::wvalue {
				{"name", child.name()},
				{"mime", child.mime()}
			};
		}

		return response;
	}
	catch(fs::filesystem_error const &err) {
		CROW_LOG_ERROR << err.what();
		return crow::response(crow::NOT_FOUND);
	}
}

crow::response Server::getPlayerStatus() const {
	crow::json::wvalue status {
		{"status", static_cast<int>(player_->status())},
		{"time-pos", player_->currentTime().count()},
		{"length", player_->duration().count()},
		{"uri", nullptr},
		{"volume", player_->getVolume()}
	};

	if(loadedResource_.has_value()) {
		status["uri"] = loadedResource_->string();
		trackListToJson(status["audio-tracks"], player_->getAudioTracks());
		trackListToJson(status["subs-tracks"], player_->getSubtitlesTracks());
	}

	return status;
}

crow::response Server::dispatchPlayerAction(crow::request const &req) {
	auto body = crow::json::load(req.body);
	
	if(!body.has("action")) {
		CROW_LOG_ERROR << "Missing parameter: \"action\".";
		return crow::response(crow::BAD_REQUEST);
	}
	
	string actionStr = body["action"].s();
	switch(VideoPlayerActionR(actionStr)) {
		case VideoPlayerAction::LOAD:
			return load(body);
		case VideoPlayerAction::PLAY:
			return load(body, true);
		case VideoPlayerAction::RESUME:
			return resume();
		case VideoPlayerAction::PAUSE:
			return pause();
		case VideoPlayerAction::STOP:
			return stop();
		case VideoPlayerAction::GOTO:
			return setTime(body);
		case VideoPlayerAction::SELETC_AUDIO_TRACK:
			return selectAudioTrack(body);
		case VideoPlayerAction::SELECT_SUBTITLES_TRACK:
			return selectSubtitlesTrack(body);
		case VideoPlayerAction::SET_VOLUME:
			return setVolume(body);
		case VideoPlayerAction::UNKNOWN:
		default:
			CROW_LOG_ERROR << "Unknown action: \"" << actionStr << "\".";
			return crow::response(crow::BAD_REQUEST);
	}
}

crow::response Server::load(crow::json::rvalue const &body, bool bPlay) {
	if(!body.has("uri")) {
		CROW_LOG_ERROR << "Missing parameter: \"uri\".";
		return crow::response(crow::BAD_REQUEST);
	}

	fs::path uri(body["uri"].s());
	CROW_LOG_DEBUG << "Play request for : " << uri.string(); 

	try {
		auto const &resource = resources_.find(uri);
		loadedResource_ = uri;
		player_->load(resource, bPlay);
		crow::json::wvalue event(body);
		trackListToJson(event["audio-tracks"], player_->getAudioTracks());
		trackListToJson(event["subs-tracks"], player_->getSubtitlesTracks());
		sendEvent_(event);

		CROW_LOG_INFO << "Playing [" << resource.string() << "].";

		return crow::response(crow::OK);
	}
	catch(fs::filesystem_error const &err) {
		CROW_LOG_ERROR << "Resource not found: " << uri.string();
		return crow::response(crow::NOT_FOUND);
	}
}

crow::response Server::resume() const {
	player_->resume();

	crow::json::wvalue event {
		{"action", VideoPlayerActionR(VideoPlayerAction::RESUME).str()},
		{"time-pos", player_->currentTime().count()}
	};
	sendEvent_(event);

	return crow::response(crow::OK);
}

crow::response Server::pause() const {
	player_->pause();

	crow::json::wvalue event {
		{"action", VideoPlayerActionR(VideoPlayerAction::PAUSE).str()},
		{"time-pos", player_->currentTime().count()}
	};
	sendEvent_(event);
	return crow::response(crow::OK);
}

crow::response Server::stop() {
	player_->stop();
	loadedResource_.reset();
	
	crow::json::wvalue event {
		{"action", VideoPlayerActionR(VideoPlayerAction::STOP).str()}
	};
	sendEvent_(event);

	return crow::response(crow::OK);
}

crow::response Server::setTime(crow::json::rvalue const &body) const {
	if(!body.has("value")) {
		CROW_LOG_ERROR << "Missing parameter \"value\".";
		return crow::response(crow::BAD_REQUEST);
	}

	if(body.has("type")) {
		string type = body["type"].s();
		player_->go(body["value"].i(), VideoPlayer::timePosType(type));
	}
	else
		player_->go(body["value"].i());

	crow::json::wvalue event {
		{"action", VideoPlayerActionR(VideoPlayerAction::GOTO).str()},
		{"time-pos", player_->currentTime().count()}
	};
	sendEvent_(event);

	return crow::response(crow::OK);
}

crow::response Server::selectAudioTrack(crow::json::rvalue const &body) const {
	return selectTrack<VideoPlayer::TrackType::AUDIO>(player_, body);
}

crow::response Server::selectSubtitlesTrack(crow::json::rvalue const &body) const {
	return selectTrack<VideoPlayer::TrackType::SUBTITLES>(player_, body);
}

crow::response Server::setVolume(crow::json::rvalue const &body) const {
	if(!body.has("volume")) {
		CROW_LOG_ERROR << "Missing parameter \"volume\".";
		return crow::response(crow::BAD_REQUEST);
	}

	int vol = body["volume"].i();
	if(vol < 0) vol = 0;
	if(vol> 130) vol= 130;
	player_->setVolume(vol);

	crow::json::wvalue event {
		{"action", VideoPlayerActionR(VideoPlayerAction::SET_VOLUME).str()},
		{"volume", vol}
	};
	sendEvent_(event);

	return crow::response(crow::OK);
}

void Server::addConnection(crow::websocket::connection &conn) {
    CROW_LOG_INFO << "Client connected: " << conn.get_remote_ip();
    conns_.insert(&conn);
}

void Server::removeConnection(crow::websocket::connection &conn) {
    CROW_LOG_INFO << "Client disconnected: " << conn.get_remote_ip();
    conns_.erase(&conn);
}


void Server::sendEvent_(crow::json::wvalue const &json) const {
    for(auto *conn : conns_) {
		conn->send_text(json.dump());
    }
}

Server::~Server() {
	delete player_;	
}

int main(int nargs, char const **args) {
	Server server;
	try { 
		server.run(std::vector<std::string>(args + 1, args + nargs));
	}
	catch(std::exception const &e) {
		CROW_LOG_ERROR << e.what();
	}
}
