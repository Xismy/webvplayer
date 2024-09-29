#ifndef __WVP_MPV_VIDEO_PLAYER_HPP__
#define __WVP_MPV_VIDEO_PLAYER_HPP__

#include "video_player.hpp"

extern "C" {
#include <mpv/client.h>
}

namespace webvplayer {
	namespace fs = std::filesystem;

	class MPVVideoPlayer : public VideoPlayer {
	private:
		mpv_handle *mpv;
		int mpv_wnd;

		void handleMpvEvent(mpv_event *event);

	public:
		MPVVideoPlayer();
		~MPVVideoPlayer();

		void play(fs::path const &file) override;
		void stop() override;
		void pause() override;
		void resume() override;
		void go(int value, TimePosType type = TimePosType::RELATIVE) override;

		Status status() const override;
		fs::path file() const override;
		std::chrono::seconds currentTime() const override;
		std::chrono::seconds duration() const override;
		TrackList getAudioTracks() const override;
		TrackList getSubtitlesTracks() const override;

	};

} // namespace webvplayer

#endif
