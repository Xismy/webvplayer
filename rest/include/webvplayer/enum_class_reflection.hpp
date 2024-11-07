#ifndef __WVP_ENUM_CLASS_REFLECTION_HPP__
#define __WVP_ENUM_CLASS_REFLECTION_HPP__
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace detail_ {
	template<std::size_t N>
    struct StringLiteral {
        char value[N];
        
		constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
		constexpr operator char const *() const { return value; }
    };
}

namespace webvplayer {
	template <typename T, detail_::StringLiteral ...Names>
	struct EnumClassReflection {
		static constexpr char const *toString(T const &val) { 
			auto idx = static_cast< std::underlying_type_t<T> >(val);

			if(idx >= sizeof ...(Names))
				throw std::out_of_range("");

			constexpr std::array<char const*, sizeof...(Names)> list {Names...};
			return list[idx]; 
		}
		
		template <T val>
		static consteval char const *toString() { return toString(val); }

		static constexpr T fromString(std::string_view const & str) {
			constexpr std::array<char const*, sizeof...(Names)> list {Names...};
			for(std::underlying_type_t<T> i = 0; i < sizeof ...(Names); ++i)
				if(std::string_view(list[i]) == str)
					return static_cast<T>(i);

			throw std::out_of_range("");
		}

		template <detail_::StringLiteral Name>
		static consteval T fromString() { return fromString(std::string_view(Name)); }
	};
}
#endif
