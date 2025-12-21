#include "transform.hpp"
#include <glm/gtc/matrix_transform.hpp>
namespace sve {
	namespace {
		struct Matrices {
			glm::mat4 translation{};
			glm::mat4 orientation{};
			glm::mat4 scale{};
		};
	
		[[nodiscard]] Matrices to_matrices(glm::vec2 const position, float rotation, glm::vec2 const scale) {
			static constexpr auto mat_v = glm::identity<glm::mat4>();
			static constexpr auto axis_v = glm::vec3{ 0.f, 0.f, 1.f };
			return Matrices{
				.translation = glm::translate(mat_v, glm::vec3{position, 0.0f}),
				.orientation = glm::rotate(mat_v, glm::radians(rotation), axis_v),
				.scale = glm::scale(mat_v, glm::vec3{scale, 1.0f})
			};
		}
	}

	glm::mat4 Transform::model_matrix() const {
		auto const [t, r, s] = to_matrices(position, rotation, scale);
		return t * r * s;
	}

	glm::mat4 Transform::view_matrix() const {
		auto const [t, r, s] = to_matrices(-position, -rotation, scale);
		return r * t * s;
	}
}