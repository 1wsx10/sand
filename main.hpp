#pragma once

#include "libdraw.hpp"
#include <cstddef>
#include <type_traits>

const unsigned WORLD_HEIGHT = 50;
const unsigned WORLD_WIDTH = 80;

template<typename ENUM_T>
constexpr auto to_underlying(ENUM_T my_enum) -> std::underlying_type_t<ENUM_T> {
	static_assert(std::is_enum<ENUM_T>::value, "to_underlying must be called on enum\n");
	return static_cast<std::underlying_type_t<ENUM_T>>(my_enum);
}


struct grain {
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
	static_assert(sizeof(colours) / sizeof(*colours) == to_underlying(type::COUNT), "missing a grain colour");

	grain::type type;
};



