#pragma once
#include "gpu.hpp"
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <render_target.hpp>

namespace sve {
	class Swapchain {
	public:
		explicit Swapchain(vk::Device device, Gpu const& gpu, vk::SurfaceKHR surface, glm::ivec2 size);

		bool recreate(glm::ivec2 size);

		[[nodiscard]] glm::ivec2 get_size() const {
			return { m_ci.imageExtent.width, m_ci.imageExtent.height };
		}
		[[nodiscard]] vk::Format get_format() const {
			return m_ci.imageFormat;
		}

		[[nodiscard]] std::optional<RenderTarget> aquire_next_image(vk::Semaphore to_signal);
		[[nodiscard]] vk::ImageMemoryBarrier2 base_barrier() const;

		[[nodiscard]] vk::Semaphore get_present_semaphore() const;
		[[nodiscard]] bool present(vk::Queue queue);
	private:
		void populate_images();
		void create_image_views();
		void create_present_semaphores();

		vk::Device m_device{};
		Gpu m_gpu{};

		vk::SwapchainCreateInfoKHR m_ci{};
		vk::UniqueSwapchainKHR m_swapchain{};
		std::vector<vk::Image> m_images{};
		std::vector<vk::UniqueImageView> m_image_views{};
		std::vector<vk::UniqueSemaphore> m_present_semaphores{};
		std::optional<std::size_t> m_image_index{};
	};
}