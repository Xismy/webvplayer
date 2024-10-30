#include "webvplayer/webvplayer.hpp"
#include "crow/app.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/websocket.h"
#include "webvplayer/video_player.hpp"
#include "webvplayer/mpv_video_player.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <unordered_set>

using webvplayer::Server;
using webvplayer::VideoPlayer;
using std::vector;
using std::string;
using std::ifstream;
namespace fs = std::filesystem;

namespace {
	bool isValidVideoExtension(fs::directory_entry const &file) {
		static std::unordered_set<std::string> const VALID_EXTENSIONS = {
			".3gp", ".amv", ".asf", ".avi", ".divx", ".dv", ".f4v", ".flv", ".m1v", ".m2ts", ".m2v", ".m4v", 
			".mkv", ".mov", ".mp4", ".mpe", ".mpeg", ".mpg", ".mpv", ".mxf", ".mts", ".ogv", ".qt", ".rm", 
			".rmvb", ".ts", ".vob", ".webm", ".wmv", ".xvid", ".rar"
		};

		return VALID_EXTENSIONS.contains(file.path().extension());
	}

	string pathReverseLookup(std::unordered_map<string, fs::path> const &map, fs::path const &path) {
		auto found = std::ranges::find_if(map, [&path] (auto &pair){ return pair.second == path; });

		if(found == map.end())
			throw fs::filesystem_error("File not found.", std::make_error_code(std::errc::no_such_file_or_directory));

		return found->first;
	}

	auto jsonGet(crow::json::rvalue const &json, string const &field) {
		try {
			return json[field];
		}
		catch (std::runtime_error const &e) {
			throw webvplayer::MissingRequiredConfigParamException(field);
		}
	}

	void trackListToJson(crow::json::wvalue &dst, VideoPlayer::TrackList const &src) {
		auto [selected, tracks] = src;
		if(selected.has_value())
			dst["selected"] = selected.value();
		else
			dst["selected"] = nullptr;

		dst["tracks"] = tracks;
	}
}

Server::Server() noexcept {
	app_.loglevel(crow::LogLevel::DEBUG);
	CROW_ROUTE(app_, "/")([this]() { return this->listResources(); });
	CROW_ROUTE(app_, "/serie/<string>")([this](string const &dir) { return this->listDir(dir); });
	CROW_ROUTE(app_, "/player").methods(crow::HTTPMethod::GET)([this]() { return this->getPlayerStatus(); });
	CROW_ROUTE(app_, "/player").methods(crow::HTTPMethod::POST)([this](crow::request const &req) { return this->dispatchPlayerAction(req); });
	CROW_WEBSOCKET_ROUTE(app_, "/player_update").onopen(
		[this](crow::websocket::connection & conn) { this->addConnection(conn); }
	).onclose(
		[this](crow::websocket::connection & conn, std::string const &) { this->removeConnection(conn); }
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
	for(auto const &res : resourcesList) {
		resources_.emplace(res["name"].s(), fs::canonical(fs::path(res["path"].s())));
		CROW_LOG_INFO << "Resource dir at [" << res["path"].s() << "].";
	}
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

crow::response Server::listResources() const {
	vector<crow::json::wvalue> list;
	list.reserve(resources_.size());
	std::ranges::transform(resources_, std::back_inserter(list), [](auto const &res) {
		return res.first;	
	});
	return crow::json::wvalue(list);
}

crow::response Server::listDir(string const &dir) const {
	try {
		vector<string> dirList;
		auto path = resources_.at(dir);
		auto filter = std::ranges::views::filter(isValidVideoExtension);

		auto filteredDirs = fs::recursive_directory_iterator(path) | filter;
		std::ranges::transform(filteredDirs, std::back_inserter(dirList), [](auto const &dir) { return dir.path().filename(); });
		std::ranges::sort(dirList);

		vector<crow::json::wvalue> response;
		std::ranges::copy(dirList, std::back_inserter(response));
		return crow::json::wvalue(response);
	}
	catch(std::out_of_range const &our) {
		return crow::response(crow::NOT_FOUND);
	}
}

crow::response Server::getPlayerStatus() const {
	crow::json::wvalue status{crow::json::wvalue::object()};
	status["status"] = static_cast<int>(player_->status());
	status["time-pos"] = player_->currentTime().count();
	status["length"] = player_->duration().count();
	status["serie"] = nullptr;
	status["file"] = nullptr;

	fs::path file = player_->file();

	if(file.empty())
		return status;

	try{
		std::string name = file.filename();
		fs::path dir = file.parent_path();
		status["serie"] = pathReverseLookup(resources_, dir);
		status["file"] = name;
	}
	catch(fs::filesystem_error const &err) {
		CROW_LOG_ERROR << err.what();
	}

	trackListToJson(status["audio-tracks"], player_->getAudioTracks());
	trackListToJson(status["subs-tracks"], player_->getSubtitlesTracks());

	return status;
}

crow::response Server::dispatchPlayerAction(crow::request const &req) const {
	auto body = crow::json::load(req.body);
	
	if(!body.has("action")) {
		CROW_LOG_ERROR << "Missing parameter: \"action\".";
		return crow::response(crow::BAD_REQUEST);
	}
	
	string actionStr = body["action"].s();
	switch(getActionFromString(actionStr)) {
		case VideoPlayerAction::PLAY:
			return play(body);
		case VideoPlayerAction::RESUME:
			return resume();
		case VideoPlayerAction::PAUSE:
			return pause();
		case VideoPlayerAction::STOP:
			return stop();
		case VideoPlayerAction::GOTO:
			return setTime(body);
		case VideoPlayerAction::UNKNOWN:
		default:
			CROW_LOG_ERROR << "Unknown action: \"" << actionStr << "\".";
			return crow::response(crow::BAD_REQUEST);
	}
}

crow::response Server::play(crow::json::rvalue const &body) const {
	if(!body.has("serie") || !body.has("file")) {
		CROW_LOG_ERROR << "Missing parameter: \"serie\" or \"file\".";
		return crow::response(crow::BAD_REQUEST);
	}

	string resStr = body["serie"].s();
	fs::path res;

	try {
		res = resources_.at(resStr);
	}
	catch(std::out_of_range const &our) {
		CROW_LOG_ERROR << "Resource not found: " << resStr;
		return crow::response(crow::NOT_FOUND);
	}

	string fileStr = body["file"].s();
	auto dirRange = fs::recursive_directory_iterator(res);
	CROW_LOG_DEBUG << "Play request for file: " << fileStr; 
	auto itFile = std::ranges::find_if(dirRange, [fileStr](auto const &dirEntry) {
		CROW_LOG_DEBUG << dirEntry.path(); 
		return dirEntry.path().filename() == fileStr;
	});

	if(itFile == fs::end(dirRange))
		return crow::response(crow::NOT_FOUND);

	CROW_LOG_INFO << "Playing [" << itFile->path().string() << "].";
	player_->play(itFile->path());

	sendEvent_(body);

	return crow::response(crow::OK);
}

crow::response Server::resume() const {
	player_->resume();

	crow::json::wvalue event;
	event["action"] = string(getActionString_(VideoPlayerAction::RESUME));
	event["time-pos"] = player_->currentTime().count();
	sendEvent_(event);

	return crow::response(crow::OK);
}

crow::response Server::pause() const {
	player_->pause();

	crow::json::wvalue event;
	event["action"] = string(getActionString_(VideoPlayerAction::PAUSE));
	event["time-pos"] = player_->currentTime().count();
	sendEvent_(event);
	return crow::response(crow::OK);
}

crow::response Server::stop() const {
	player_->stop();
	
	crow::json::wvalue event;
	event["action"] = string(getActionString_(VideoPlayerAction::STOP));
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

	crow::json::wvalue event;
	event["action"] = string(getActionString_(VideoPlayerAction::GOTO));
	event["time-pos"] = player_->currentTime().count();
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
