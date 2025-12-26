#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace sve {
	struct Transform {
		glm::vec2 position{};
		float rotation{};
		glm::vec2 scale{1.f};

		[[nodiscard]] glm::mat4 model_matrix() const;
		[[nodiscard]] glm::mat4 view_matrix() const;
	};
}