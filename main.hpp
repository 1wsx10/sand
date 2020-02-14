#pragma once

#include "libdraw.hpp"
#include <cstddef>
#include <type_traits>

extern const unsigned WORLD_HEIGHT = 50;
extern const unsigned WORLD_WIDTH = 80;

template<typename ENUM_T>
constexpr auto to_underlying(ENUM_T my_enum) -> std::underlying_type_t<ENUM_T> {
	static_assert(std::is_enum<ENUM_T>::value, "to_underlying must be called on enum\n");
	return static_cast<std::underlying_type_t<ENUM_T>>(my_enum);
}


struct matter {
	enum class type : unsigned {
		none,
		sand,
		water,

		//
		COUNT
	};

	static constexpr RGBT colours[] = {
		{0,0,0,0},
		{255,255,0,0},
		{0,0,255,0},
	};
	static_assert(sizeof(colours) / sizeof(*colours) == to_underlying(type::COUNT), "missing a matter colour");

	matter::type type;
	bool is_active = true;
};


template<typename NUM>
struct vec2 {
	NUM x;
	NUM y;

	void snprint(char* buf, size_t bufsize) const {
		snprintf(buf, bufsize, "(%d, %d)", x, y);
	}
};

using vec2u = vec2<unsigned>;
using vec2i = vec2<int>;
