#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>


namespace sve {
	struct Vertex {
		glm::vec2 position{};
		glm::vec3 color{ 1.0f };
		glm::vec2 uv{};
	};

	constexpr auto vertex_attributes_v = std::array{
		vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)},
		vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)},
		vk::VertexInputAttributeDescription2EXT{2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},
	};

	constexpr auto vertex_binding_v = std::array{
		vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex), vk::VertexInputRate::eVertex, 1}
	};

}