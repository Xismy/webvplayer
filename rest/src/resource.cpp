#include "webvplayer/resource.hpp"
#include <algorithm>
#include <filesystem>
#include <regex>
#include <unordered_set>
#include <ranges>
#include <crow/logging.h>

using webvplayer::ResourceCollection;
using std::string;
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
}

ResourceCollection::ResourceCollection(string const &name, string const &path, string const &translationRegex, string const &translationPattern) :
	name_(name),
	path_(path),
	translationRegex_(translationRegex),
	translationPattern_(translationPattern)
{}

auto ResourceCollection::getResources() const -> decltype(resources_) {
	if(!resources_.empty())
		return resources_;

	auto filter = std::ranges::views::filter(isValidVideoExtension);
	auto filteredDirs = fs::recursive_directory_iterator(path_) | filter;
	std::ranges::transform(filteredDirs, std::back_inserter(resources_),
		[this](auto const &dir) {
			string filename = dir.path().filename();
			string translated = translate(filename);
			return std::make_pair(filename, translated);
		}
	);

	std::ranges::sort(resources_, {}, &decltype(resources_)::value_type::first);

	return resources_;
}

string ResourceCollection::translate(string const &name) const {
	string output;
	std::string_view outputFormat = translationPattern_;
	
	std::smatch args;
	std::regex_search(name, args, translationRegex_);

	while(!outputFormat.empty()) {
		std::match_results<std::string_view::iterator> token;
		if(!std::regex_search(outputFormat.begin(), outputFormat.end(), token, std::regex("([^{]+)|\\{([0-9]+)\\}|\\{(\\{)")))
			return name;
		
		if(token[1].matched)
			output += token.str(1);
		else if(token[2].matched)
			output += args[std::stol(token.str(2))];
		else if(token[3].matched)
			output += token.str(3);

		outputFormat.remove_prefix(token.length(0));
	}

	return output;
}

fs::path ResourceCollection::getFilePath(std::string const &name) const {
	auto resources = getResources();
	auto it = std::ranges::find_if(resources, [name](auto const &res) {
		return res.second == name;
	});

	if(it == resources_.end())
		throw std::out_of_range("Resource not found: " + name);

	return path() / it->first;
}
