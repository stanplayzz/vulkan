#pragma once
#include <vulkan/vulkan.hpp>

namespace sve {
	constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);

	struct Gpu {
		vk::PhysicalDevice device{};
		vk::PhysicalDeviceProperties properties{};
		vk::PhysicalDeviceFeatures features{};
		std::uint32_t queue_family{};
	};

	[[nodiscard]] Gpu get_suitable_gpu(vk::Instance instance, vk::SurfaceKHR surface);
}
