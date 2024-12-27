#ifndef __WEBVPLAYER_HPP__
#define __WEBVPLAYER_HPP__

#include "crow/app.h"
#include "crow/json.h"
#include "crow/middlewares/cors.h"
#include "crow/websocket.h"
#include "enum_class_reflection.hpp"
#include <unordered_set>
#include <string>
#include "webvplayer/resource.hpp"
#include <webvplayer/video_player.hpp>

namespace webvplayer {

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
			GOTO,
			SELETC_AUDIO_TRACK,
			SELECT_SUBTITLES_TRACK
		};

		using VideoPlayerActionR = EnumClassReflection<VideoPlayerAction, 
			 "unknown",
			 "play",
			 "stop",
			 "pause",
			 "resume",
			 "goto",
			 "select-audio-track",
			 "select-subs-track"
		>;

	private:
		crow::App<crow::CORSHandler> app_;
		std::unordered_set<crow::websocket::connection*> conns_;
		std::unordered_set<ResourceCollection> resources_;
		VideoPlayer *player_ = nullptr;

		void sendEvent_(crow::json::wvalue const &json) const;

	public:
		Server() noexcept;
		Server(Server const &) = delete;
		Server &operator=(Server const &) = delete;
		Server(Server &&other) = delete;
		Server &operator=(Server &&other) = delete;
		void run(std::vector<std::string> const &args);
		crow::response listCollections() const;
		crow::response listResources(std::string const &collection) const;
		crow::response getPlayerStatus() const;
		crow::response dispatchPlayerAction(crow::request const &req) const;
		crow::response play(crow::json::rvalue const &req) const;
		crow::response resume() const;
		crow::response pause() const;
		crow::response stop() const;
		crow::response setTime(crow::json::rvalue const &body) const;
		crow::response selectAudioTrack(crow::json::rvalue const &body) const;
		crow::response selectSubtitlesTrack(crow::json::rvalue const &body) const;
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
