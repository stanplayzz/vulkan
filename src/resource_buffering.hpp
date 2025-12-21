#pragma once
#include <array>

namespace sve {
	inline constexpr std::size_t resource_buffering_v{ 2 };

	template <typename Type>
	using Buffered = std::array<Type, resource_buffering_v>;
}