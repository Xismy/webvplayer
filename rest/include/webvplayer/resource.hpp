#ifndef __WEBVPLAYER_RESOURCE_HPP__
#define __WEBVPLAYER_RESOURCE_HPP__

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "name_format.hpp"

namespace webvplayer {
	namespace fs = std::filesystem;
	
	constexpr char const *DirectoryMime = "application/directory";
	std::string ext2mime(fs::path const &file);

	class Resource {
	private:
		std::string name_;
		std::string mime_;
		std::optional<fs::path> path_;

	public:
		Resource (std::string const &name) :
			name_(name),
			mime_(DirectoryMime) {}

		Resource(std::string const &name, fs::path const &path) : 
			name_(name),
			mime_(ext2mime(path)),
			path_(path) {}

		Resource(Resource &) = delete;
		Resource &operator=(Resource &) = default;
		Resource(Resource &&) = default;
		Resource &operator=(Resource &&) = default;

		std::string name() const { return name_; }
		fs::path path() const { return path_.value_or("."); }
		std::string mime() const { return mime_; }
		bool isDirectory() const { return mime_ == DirectoryMime; }
		bool isVideo() const { return mime_.starts_with("video/"); }
	};

	class ResourceTree {
	public:
		class Node {
			friend class ResourceTree;
			Resource resource_;
			Node const *parent_;
			/*When Node represents a filesystem directory, children will be read on demand.
			 *Loading them does not imply modifying the represented node, so it is a mutable attribute.*/
			mutable std::vector<Node> children_;

		public:
			Node() :
				resource_(""),
				parent_(nullptr) {}

			Node(Resource &&resource, Node const &parent) :
				resource_(std::move(resource)),
				parent_(&parent) {}

			Node(Node const &) = delete;
			Node &operator=(Node const &) = delete;
			Node(Node &&) = default;
			Node &operator=(Node &&) = default;
		
			std::string name() const { return resource_.name(); }
			std::string mime() const { return resource_.mime(); }
			fs::path innerPath() const;
			Node const &getChild(fs::path const &uri) const { return getChild_(uri.lexically_normal());}

		private:
			Node const &addChild_(Resource &&resource) const;
			Node const &getChild_(fs::path const &uri) const;
			void expand_(name_format::Formatter const &formatter) const;
		};

	private:
		Node root_;
		std::unordered_map<fs::path, name_format::Formatter> formatters_;

	public:
		ResourceTree() = default;
		Node const &root() { return root_; }
		Node const &add(Node const &parent, Resource &&resource) { return parent.addChild_(std::move(resource)); }
		fs::path find(fs::path const &uri) const { return root_.getChild(uri).innerPath(); }
		std::vector<Node> const &list(fs::path const &uri) const;
	};
} // namespace webvplayer

#endif
