#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <span>

namespace sve::glfw {
	struct Deleter {
		void operator()(GLFWwindow* window) const noexcept;
	};

	using Window = std::unique_ptr<GLFWwindow, Deleter>;

	[[nodiscard]] Window create_window(glm::ivec2 size, char const* title);
	vk::UniqueSurfaceKHR create_surface(GLFWwindow* window, vk::Instance instance);

	std::span<char const* const> instance_extensions();

	glm::ivec2 framebuffer_size(GLFWwindow* window);
}