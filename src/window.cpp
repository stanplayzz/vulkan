#include "window.hpp"

#include <print>

namespace sve::glfw {
	void Deleter::operator()(GLFWwindow* window) const noexcept {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	Window create_window(glm::ivec2 const size, char const* title) {
		static auto const on_error = [](int const code, char const* description) {
			std::println(stderr, "[GLFW] Error {}: {}", code, description);
			};
		glfwSetErrorCallback(on_error);
		if (glfwInit() != GLFW_TRUE) 
			throw std::runtime_error{ "Failed to initialize GLFW" };
		if (glfwVulkanSupported() != GLFW_TRUE) 
			throw std::runtime_error{ "Vulkan not supported" };
		auto ret = Window{};
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		ret.reset(glfwCreateWindow(size.x, size.y, title, nullptr, nullptr));
		if (!ret) 
			throw std::runtime_error{ "Failed to create GLFW window" };
		return ret;
	}

	vk::UniqueSurfaceKHR create_surface(GLFWwindow* window, vk::Instance const instance) {
		VkSurfaceKHR ret{};
		auto const result = glfwCreateWindowSurface(instance, window, nullptr, &ret);
		if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) {
			throw std::runtime_error{ "Failed to create Vulkan surface" };
		}
		return vk::UniqueSurfaceKHR{ ret, instance };
	}

	std::span<char const* const> instance_extensions() {
		auto count = std::uint32_t{};
		auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
		return { extensions, static_cast<size_t>(count) };
	}

	glm::ivec2 framebuffer_size(GLFWwindow* window) {
		auto ret = glm::ivec2{};
		glfwGetFramebufferSize(window, &ret.x, &ret.y);
		return ret;
	}
}
