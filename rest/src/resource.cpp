#include "webvplayer/resource.hpp"
#include "webvplayer/name_format.hpp"
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_set>
#include <crow/logging.h>

using webvplayer::ResourceTree;
using webvplayer::Resource;
using std::string;
using std::vector;
using std::ranges::copy;
using std::ranges::sort;
namespace fs = std::filesystem;

string webvplayer::ext2mime(fs::path const &file) {
	if(fs::is_directory(file))
		return DirectoryMime;

	static std::unordered_set<std::string_view> const VideoExtensions = {
		"3gp", "amv", "asf", "avi", "divx", "dv", "f4v", "flv", "m1v", "m2ts", "m2v", "m4v", 
		"mkv", "mov", "mp4", "mpe", "mpeg", "mpg", "mpv", "mxf", "mts", "ogv", "qt", "rm", 
		"rmvb", "ts", "vob", "webm", "wmv", "xvid", "rar"
	};

	string extWithDot = file.extension().string();

	if(extWithDot == "")
		return "";
	
	string extension(extWithDot.substr(1));

	if(VideoExtensions.contains(extension))
			return string("video/") + extension;

	return "";
}

fs::path ResourceTree::Node::innerPath() const {
	if(resource_.path().is_absolute())
		return resource_.path();

	if(parent_ == nullptr)
		throw std::filesystem::filesystem_error("Resource has no parent", std::make_error_code(std::errc::no_such_file_or_directory));

	return (parent_->innerPath() / resource_.path()).lexically_normal();
}


ResourceTree::Node const &ResourceTree::Node::addChild_(Resource &&resource) const {
	auto next = std::ranges::lower_bound(children_, resource.name(), {}, &ResourceTree::Node::name);
	if(next != children_.end() && next->name() == resource.name())
		return *next;
	return *children_.emplace(next, std::move(resource), *this);
}

ResourceTree::Node const &ResourceTree::Node::getChild_(fs::path const &uri) const {
	if(uri == "" || uri == ".")
		return *this;

	std::string childName(*uri.begin()); 
	auto child = std::ranges::lower_bound(children_, childName, [](auto const &a, auto const &b){
		CROW_LOG_DEBUG << "Comparing: [" << a << "] with [" << b << "]";
		return a < b;
	}, &ResourceTree::Node::name);
	if(child == children_.end()) {
		CROW_LOG_DEBUG << "Child not found: " << childName;
	}
	else {
		CROW_LOG_DEBUG << "Found child: " << child->name();
	}

	if(child == children_.end() || child->name() != childName) 
		throw std::filesystem::filesystem_error("File not found", std::make_error_code(std::errc::no_such_file_or_directory));

	return child->getChild_(fs::relative(uri, childName));
}

void ResourceTree::Node::expand_(name_format::Formatter const &formatter) const {
	for(auto const &dirEntry : fs::recursive_directory_iterator(innerPath())) {
		if(fs::is_directory(dirEntry)) 
			continue;
		
		string mime = ext2mime(dirEntry.path());
		if(!mime.empty()) {
			std::string name = formatter(dirEntry.path().filename().string());
			addChild_(Resource(name, dirEntry.path()));
		}
	}
}

vector<ResourceTree::Node> const  &ResourceTree::list(fs::path const &uri) const {
	static name_format::Formatter fallbackFormatter;
	Node const &node = root_.getChild(uri);

	if(node.children_.empty()) {
		auto it = formatters_.find(uri);
		node.expand_(it == formatters_.end()? fallbackFormatter : it->second);	
	}
	
	return node.children_;
}
