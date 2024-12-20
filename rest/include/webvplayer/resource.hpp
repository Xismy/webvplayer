#ifndef __WEBVPLAYER_RESOURCE_HPP__
#define __WEBVPLAYER_RESOURCE_HPP__

#include <string>
#include <filesystem>
#include <regex>
#include <vector>

namespace webvplayer {
	class ResourceCollection {
		std::string name_;
		std::filesystem::path path_;
		mutable std::vector<std::pair<std::string, std::string> > resources_; 
		std::regex translationRegex_;
		std::string translationPattern_;

		std::string translate(std::string const &name) const;

	public:
		ResourceCollection(std::string const &name, std::string const &path="", std::string const &translationRegex="", std::string const &translationPattern="");	
		auto getResources() const -> decltype(resources_);
		std::string name() const { return name_; }
		std::filesystem::path path() const { return path_; }
		bool operator==(ResourceCollection const &other) const { return name_ == other.name_; }
		std::filesystem::path getFilePath(std::string const &name) const;
	};

}

template<>
struct std::hash<webvplayer::ResourceCollection> {
	auto operator()(webvplayer::ResourceCollection const &res) const { 
		std::hash<std::string> hash;
		return hash(res.name());
	}
};

#endif
