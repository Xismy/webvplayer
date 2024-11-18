#ifndef __WVP_ENUM_CLASS_REFLECTION_HPP__
#define __WVP_ENUM_CLASS_REFLECTION_HPP__
#include <algorithm>
#include <array>
#include <stdexcept>

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
	class EnumClassReflection {
		T val_;

	public:
		constexpr EnumClassReflection(T val) : val_(val) {
			if(static_cast<std::underlying_type_t<T> >(val) >= sizeof...(Names))
				throw std::out_of_range("");
		}

		EnumClassReflection (std::string_view const & str) {
			constexpr std::array<char const*, sizeof...(Names)> list {Names...};
			for(std::underlying_type_t<T> i = 0; i < sizeof ...(Names); ++i) {
				if(std::string_view(list[i]) == str) {
					val_ = static_cast<T>(i);
					return;
				}
			}

			throw std::out_of_range("");
		}

		constexpr char const *str() const { 
			auto idx = static_cast< std::underlying_type_t<T> >(val_);

			if(idx >= sizeof ...(Names))
				throw std::out_of_range("");

			constexpr std::array<char const*, sizeof...(Names)> list {Names...};
			return list[idx]; 
		}

		consteval operator char const *() const { return str(); }

		constexpr operator T() const { return val_; }
	};

	template <typename T, detail_::StringLiteral ...Names>
	::std::ostream &operator<<(std::ostream &out, EnumClassReflection<T, Names...> const &element) {
		return out << element.str();
	} 
}
#endif
