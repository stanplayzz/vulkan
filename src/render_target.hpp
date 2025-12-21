#pragma once
#include <vulkan/vulkan.hpp>

namespace sve {
	struct RenderTarget {
		vk::Image image{};
		vk::ImageView image_view{};
		vk::Extent2D extent{};
	};
}