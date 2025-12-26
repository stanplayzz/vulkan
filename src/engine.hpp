#pragma once
#include "scoped_waiter.hpp"
#include "window.hpp"
#include "gpu.hpp"
#include "swapchain.hpp"
#include "resource_buffering.hpp"
#include "render_target.hpp"
#include "dear_imgui.hpp"
#include "shader_program.hpp"
#include "vma.hpp"
#include "utils/vertex.hpp"
#include "descriptor_buffer.hpp"
#include "texture.hpp"
#include "transform.hpp"
#include "renderer.hpp"
#include "utils/object.hpp"
#include <imgui.h>
#include <vulkan/vulkan.hpp>
#include <filesystem>


namespace sve {
	namespace fs = std::filesystem;

	class App {
	public:
		void run();
	private:
		struct RenderSync {
			vk::UniqueSemaphore draw{};
			vk::UniqueFence drawn{};
			vk::CommandBuffer command_buffer{};
		};

		glfw::Window m_window{};
		vk::UniqueInstance m_instance{};
		vk::UniqueSurfaceKHR m_surface{};
		Gpu m_gpu{};
		vk::UniqueDevice m_device{};
		vk::Queue m_queue{};

		vma::Allocator m_allocator{};

		std::optional<Swapchain> m_swapchain{};
		vk::UniqueCommandPool m_cmd_block_pool{};
		vk::UniqueCommandPool m_render_cmd_pool{};
		Buffered<RenderSync> m_render_sync{};
		std::size_t m_frame_index{};

		glm::ivec2 m_framebuffer_size{};
		std::optional<RenderTarget> m_render_target{};

		fs::path m_assets_dir{};

		std::optional<ShaderProgram> m_shader{};
		bool m_wireframe{};

		std::optional<Renderer> m_renderer{};
		vma::Buffer m_vbo{};
		std::optional<Texture> m_texture{};
		std::vector<glm::mat4> m_instance_data{};
		std::optional<DescriptorBuffer> m_instance_ssbo{};

		Transform m_view_transform{};
		std::array<Transform, 2> m_instances{};

		Object m_object;

		ScopedWaiter m_waiter{};

		[[nodiscard]] fs::path asset_path(std::string_view uri) const;
		[[nodiscard]] CommandBlock create_command_block() const;



		void create_window();
		void create_instance();
		void create_surface();
		void select_gpu();
		void create_device();
		void create_allocator();
		void create_swapchain();
		void create_shader();
		void create_shader_resources();
		void create_renderer();
		void main_loop();

		void update_instances();
	};
}