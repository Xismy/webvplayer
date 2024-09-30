#ifndef __WVP_VIDEO_PLAYER_HPP__
#define __WVP_VIDEO_PLAYER_HPP__
#include <chrono>
#include <filesystem>
#include <optional>

namespace webvplayer {
	namespace fs = std::filesystem;

	class VideoPlayer {
	public:
		enum class TimePosType {
			UNKNOWN,
			RELATIVE,
			ABSOLUTE,
			ABS_NORM,
		};

		enum class Status {
			IDLE,
			PLAYING,
			PAUSED,
		};

		static constexpr TimePosType timePosType(std::string_view const tpt) noexcept {
			if(tpt == "relative")
				return TimePosType::RELATIVE;
			if(tpt == "absolute")
				return TimePosType::ABSOLUTE;
			if(tpt == "abs_norm")
				return TimePosType::ABS_NORM;

			return TimePosType::UNKNOWN;
		}

		virtual void play(fs::path const &file) = 0;
		virtual void stop() = 0;
		virtual void pause() = 0;
		virtual void resume() = 0;
		virtual void go(int value, TimePosType type = TimePosType::RELATIVE) = 0;

		virtual Status status() const = 0;
		virtual fs::path file() const = 0;
		virtual std::chrono::seconds currentTime() const = 0;
		virtual std::chrono::seconds duration() const = 0;
		using TrackList = std::tuple<std::optional<std::size_t>, std::vector<std::string> >;
		virtual TrackList getAudioTracks() const = 0;
		virtual TrackList getSubtitlesTracks() const = 0;
		virtual ~VideoPlayer() {}
	};
}

#endif
