#ifndef __WEBVPLAYER_HPP__
#define __WEBVPLAYER_HPP__

#include "crow/app.h"
#include "crow/json.h"
#include "crow/middlewares/cors.h"
#include "crow/websocket.h"
#include <exception>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <webvplayer/video_player.hpp>

namespace webvplayer {
	namespace fs = std::filesystem;

	class BadArgumentException : public std::exception {
		std::string arg_;
		std::string what_;
	public:
		BadArgumentException(std::string arg) : 
			arg_(arg),
			what_("Bad argument exception: " + arg + "."){}
		char const *what() const noexcept override { return what_.c_str(); }
		char const *arg() const noexcept { return arg_.c_str(); }
	};

	class MissingRequiredArgumentException : public std::exception {
		std::string arg_;
		std::string what_;
	public:
		MissingRequiredArgumentException(std::string arg) :
			arg_(arg),
			what_("Missing required argument exception: " + arg + "."){}
		char const *what() const noexcept override { return what_.c_str(); }
		char const *arg() const noexcept { return arg_.c_str(); }
	};
	
	class MissingRequiredConfigParamException : public std::exception {
		std::string field_;
		std::string what_;
	public:
		MissingRequiredConfigParamException(std::string field) :
			field_(field),
			what_("Missing required config param exception: " + field + "."){}
		char const *what() const noexcept override { return what_.c_str(); }
		char const *field() const noexcept { return field_.c_str(); }
	};

	class Server {
	public:
		enum class VideoPlayerId {
			MPV,
			UNDEFINED
		};

		enum class VideoPlayerAction {
			UNKNOWN,
			PLAY,
			STOP,
			PAUSE,
			RESUME,
			GOTO
		};

	private:
		crow::App<crow::CORSHandler> app_;
		std::unordered_set<crow::websocket::connection const*> conns_;
		std::unordered_map<std::string, fs::path> resources_;
		VideoPlayer *player_ = nullptr;

	public:
		Server() noexcept;
		Server(Server const &) = delete;
		Server &operator=(Server const &) = delete;
		Server(Server &&other) = delete;
		Server &operator=(Server &&other) = delete;
		void run(std::vector<std::string> const &args);
		crow::response listResources() const;
		crow::response listDir(std::string const &dir) const;
		crow::response getPlayerStatus() const;
		crow::response dispatchPlayerAction(crow::request const &req) const;
		crow::response play(crow::json::rvalue const &req) const;
		crow::response resume() const;
		crow::response pause() const;
		crow::response stop() const;
		crow::response setTime(crow::json::rvalue const &body) const;
		void addConnection(crow::websocket::connection &conn);
		void removeConnection(crow::websocket::connection &conn);
		
		static VideoPlayerId mapVideoPlayer(std::string const &name) {
			if(name == "mpv")
				return VideoPlayerId::MPV;

			return Server::VideoPlayerId::UNDEFINED;
		}

		static VideoPlayer *getPlayerByName(std::string const &backend);
		~Server();
	};
}

#endif
