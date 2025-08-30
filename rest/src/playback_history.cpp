#include "webvplayer/playback_history.hpp"
#include "crow/json.h"
#include "crow/logging.h"
#include "webvplayer/resource.hpp"
#include <chrono>
#include <cstdint>
#include <fstream>

using webvplayer::ResourceTree;
using webvplayer::PlaybackHistoryManager;

void PlaybackHistoryManager::load(fs::path const &dataFile) {
	dataFile_ = dataFile;
	std::ifstream dataFStream(dataFile_);

	if(dataFStream.fail())
		return;

	dataFStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

	std::stringstream dataRaw;
	dataFStream >> dataRaw.rdbuf();
	auto data = crow::json::load(dataRaw.str());

	for(auto &dir : data) {
		for(auto &file : dir) {
			auto &history = history_[fs::path(dir.key())].files[file.key()];
			history.progress = std::chrono::seconds(file["progress"].u());
			history.bWatched = file["watched"].b();
			history.ts = Entry::Ts(Entry::Ts::duration(file["ts"].i()));
		}
	}
}

void PlaybackHistoryManager::dump() const {
	if(dataFile_.empty())
		return;

	CROW_LOG_INFO << "Writing data file " << dataFile_;

	std::ofstream dataFStream(dataFile_, std::ios::trunc);
	dataFStream.exceptions(std::ifstream::badbit | std::ifstream::failbit | std::ifstream::failbit);

	crow::json::wvalue data;

	for(auto const &[dirPath, dir] : history_) {
		auto &dirJson = data[dirPath.string()];
		CROW_LOG_DEBUG << "\tDirectory: " << dirPath;
		for(auto const &[filename, entry] : dir.files) {
			dirJson[filename] = {
				{"progress", static_cast<uint64_t>(entry.progress.count())},
				{"watched", entry.bWatched},
				{"ts", entry.ts.time_since_epoch().count() }
			};

			CROW_LOG_DEBUG << "\t\tFile: " << filename;
		}
	}

	dataFStream << data.dump();
}

bool PlaybackHistoryManager::hasBeenWatched(fs::path const &dir, std::string filename) const {
	auto itDir = history_.find(dir);
	if(itDir == history_.end()) 
		return false;

	auto &directory = itDir->second;
	auto it = directory.files.find(filename);
	return it != directory.files.end()? it->second.bWatched : false; 
}

int  PlaybackHistoryManager::progress(fs::path const &dir, std::string filename) const {
	auto itDir = history_.find(dir);
	if(itDir == history_.end()) 
		return 0;

	auto &directory = itDir->second;
	auto it = directory.files.find(filename);
	return it != directory.files.end()? static_cast<int>(it->second.progress.count()) : 0;
}

void PlaybackHistoryManager::setWatched(fs::path const &dir, std::string filename, bool bWatched) {
	auto &file = history_[dir].files[filename];
	file.bWatched = bWatched;
	file.ts = Entry::Ts::clock::now();
}

void PlaybackHistoryManager::setProgress(fs::path const &dir, std::string filename, std::chrono::seconds progress) {
	CROW_LOG_DEBUG << "Setting progress for [" << (dir/filename).string() << "] to " << progress.count() << "s.";
	auto &file = history_[dir].files[filename];
	file.progress = progress;
}

