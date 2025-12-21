#pragma once
#include "scoped.hpp"
#include <vulkan/vulkan.hpp>

namespace sve {
	struct ScopedWaterDeleter {
		void operator()(vk::Device const device) const noexcept {
			device.waitIdle();
		}
	};

	using ScopedWaiter = Scoped<vk::Device, ScopedWaterDeleter>;
}