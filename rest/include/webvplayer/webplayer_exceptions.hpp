#ifndef __WEBVPLAYER_EXCEPTIONS_HPP__ 
#define __WEBVPLAYER_EXCEPTIONS_HPP__ 

#include <stdexcept>
#include <string>

namespace webvplayer {
	class BadArgumentException : public std::runtime_error {
		std::string arg_;
	public:
		BadArgumentException(std::string arg) : 
			std::runtime_error("Bad argument exception: " + arg + "."),
			arg_(arg)
		{}
		char const *arg() const noexcept { return arg_.c_str(); }
	};

	class MissingRequiredArgumentException : public std::runtime_error {
		std::string arg_;
	public:
		MissingRequiredArgumentException(std::string arg) :
			std::runtime_error("Missing required argument exception: " + arg + "."),
			arg_(arg)
		{}
		char const *arg() const noexcept { return arg_.c_str(); }
	};
	
	class MissingRequiredConfigParamException : public std::runtime_error {
		std::string field_;
	public:
		MissingRequiredConfigParamException(std::string field) :
			std::runtime_error("Missing required config param exception: " + field + "."),
			field_(field)
		{}
		char const *field() const noexcept { return field_.c_str(); }
	};
}

#endif
