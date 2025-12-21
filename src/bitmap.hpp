#pragma once
#include <span>
#include <cstddef>
#include <glm/vec2.hpp>

namespace sve {
	struct Bitmap {
		std::span<std::byte const> bytes{};
		glm::ivec2 size{};
	};
}