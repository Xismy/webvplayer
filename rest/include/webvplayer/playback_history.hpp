#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace webvplayer {
	namespace fs = std::filesystem;
	class PlaybackHistoryManager {
	struct Entry {
		using Ts = std::chrono::time_point<std::chrono::system_clock>;
		Ts ts;
		std::chrono::seconds progress;
		bool bWatched;
	};

	struct Directory {
		std::unordered_map<std::string, Entry> files;
	};

	private:
		fs::path dataFile_;
		std::unordered_map<fs::path, Directory> history_;


	public:
		void load(fs::path const &dataFile);
		void dump() const;

		bool hasBeenWatched(fs::path const &dir, std::string filename) const;
		int  progress(fs::path const &dir, std::string filename) const;

		void setWatched(fs::path const &dir, std::string filename, bool bWatched);
		void setProgress(fs::path const &dir, std::string filename, std::chrono::seconds progress);
	};
}
