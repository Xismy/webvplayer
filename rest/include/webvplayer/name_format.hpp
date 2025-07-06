#ifndef __WEBVPLAYER_NAME_FORMAT_HPP__
#define __WEBVPLAYER_NAME_FORMAT_HPP__
#include <algorithm>
#include <assert.h>
#include <regex>
#include <string>
#include <vector>

namespace webvplayer::name_format {
	namespace detail {
		struct Rule {
			std::regex pattern;
			std::string format;

			std::string operator()(std::string const &uglyName) const {
				return std::regex_replace(uglyName, pattern, format);
			}
		};
	} // namespace detail

	class Formatter {
		std::vector<detail::Rule> rules_;

	public:
		Formatter () {
			addRule(std::regex("\\[.*?\\]"), ""); 
			addRule(std::regex("_"), " "); 
			addRule(std::regex(" *\\.[^.]+$"), "");
			addRule(std::regex("^ +"), "");
		}

		void addRule(std::regex pattern, std::string format) { rules_.push_back({pattern, format}); }

		std::string operator()(std::string name) const {
			std::ranges::for_each(rules_, [&name](detail::Rule const &rule) { name = rule(name); });
			return name;
		}
	};
} // namespace webvplayer::name_format
#endif
