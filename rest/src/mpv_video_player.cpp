#include "webvplayer/mpv_video_player.hpp"
#include "crow/logging.h"
#include "webvplayer/video_player.hpp"
#include <mpv/client.h>
#include <optional>
#include <tuple>

using webvplayer::MPVVideoPlayer;
using webvplayer::VideoPlayer;
using PlayerStatus = webvplayer::VideoPlayer::Status;
namespace fs = std::filesystem;
using std::vector;
using std::string;

MPVVideoPlayer::MPVVideoPlayer() : mpv(nullptr), mpv_wnd(0) {
	mpv = mpv_create();
	if (!mpv)
		throw std::runtime_error("MPV: Cannot create.");

	mpv_initialize(mpv);
}

MPVVideoPlayer::~MPVVideoPlayer() {
	mpv_terminate_destroy(mpv);
}

void MPVVideoPlayer::play(fs::path const &file) {
	const char *cmd[] = {"loadfile", file.c_str(), nullptr};
	mpv_command_async(mpv, 0, cmd);
	mpv_command_string(mpv, "set fullscreen yes");
}

void MPVVideoPlayer::stop() {
	mpv_terminate_destroy(mpv);
	mpv = mpv_create();
	mpv_initialize(mpv);
}

void MPVVideoPlayer::pause() {
	mpv_command_string(mpv, "set pause yes");
}

void MPVVideoPlayer::resume() {
	mpv_command_string(mpv, "set pause no");
}

void MPVVideoPlayer::go(int value, TimePosType type) {
	std::string flags = type == TimePosType::ABSOLUTE? "absolute" :
	                    type == TimePosType::ABS_NORM? "absolute-percent" : 
		                                               "relative";
	std::string command = std::format("seek {} {}", value, flags);

	mpv_command_string(mpv, command.c_str());
}

void MPVVideoPlayer::selectAudioTrack(std::size_t const &track) {
	mpv_command_string(mpv, std::format("set aid {}", track).c_str());
}

void MPVVideoPlayer::selectSubtitlesTrack(std::size_t const &track) {
	mpv_command_string(mpv, std::format("set sid {}", track).c_str());
}

PlayerStatus MPVVideoPlayer::status() const {
	int bIdle;
	int bPaused;
	mpv_get_property(mpv, "idle-active", MPV_FORMAT_FLAG, &bIdle);
	mpv_get_property(mpv, "pause", MPV_FORMAT_FLAG, &bPaused);

	if(bIdle)
		return PlayerStatus::IDLE;
	if(bPaused)
		return PlayerStatus::PAUSED;

	return PlayerStatus::PLAYING;
}

fs::path MPVVideoPlayer::file() const {
	char const *path = mpv_get_property_string(mpv, "path");
	return fs::path(path==nullptr? "" : path);
}

std::chrono::seconds MPVVideoPlayer::currentTime() const {
	double time_pos;
	mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &time_pos);
	return std::chrono::seconds(static_cast<long long>(time_pos));
}

std::chrono::seconds MPVVideoPlayer::duration() const {
	double duration;
	mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &duration);
	return std::chrono::seconds(static_cast<long long>(duration));
}

namespace {
	template<VideoPlayer::TrackType T>
	VideoPlayer::TrackList<T> getTracks(mpv_handle *mpv, string targetType) {
		int64_t nTracks;
		mpv_get_property(mpv, "track-list/count", MPV_FORMAT_INT64, &nTracks);

		webvplayer::VideoPlayer::TrackList<T> list;

		for(int64_t i=0; i < nTracks; ++i) {
			char const *type = mpv_get_property_string(mpv, std::format("track-list/{}/type", i).c_str());
			if(string(type) == targetType) {
				char const *trackTitle = mpv_get_property_string(mpv, std::format("track-list/{}/title", i).c_str());
				int64_t id;
				mpv_get_property(mpv, std::format("track-list/{}/id", i).c_str(), MPV_FORMAT_INT64, &id);
				list.emplace_back(trackTitle==nullptr? "default" : trackTitle, id, false);

				int bSelected;
				mpv_get_property(mpv, std::format("track-list/{}/selected", i).c_str(), MPV_FORMAT_FLAG, &bSelected);
				if(bSelected)
				    list.back().bSelected = true;
			}
		}

		return list;
	}
}

VideoPlayer::TrackList<VideoPlayer::TrackType::AUDIO> MPVVideoPlayer::getAudioTracks() const {
    return getTracks<VideoPlayer::TrackType::AUDIO>(mpv, "audio");
}

VideoPlayer::TrackList<VideoPlayer::TrackType::SUBTITLES> MPVVideoPlayer::getSubtitlesTracks() const {
    return getTracks<VideoPlayer::TrackType::SUBTITLES>(mpv, "sub");
}

