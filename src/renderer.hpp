#pragma once
#include "shader_program.hpp"
#include "vma.hpp"
#include "texture.hpp"
#include "utils/transform.hpp"
#include "gpu.hpp"
#include "descriptor_buffer.hpp"
#include "render_target.hpp"
#include "swapchain.hpp"
#include "dear_imgui.hpp"
#include "utils/color.hpp"
#include "utils/object.hpp"
#include <imgui.h>
#include <vulkan/vulkan.hpp>


namespace sve {
	struct RendererCreateInfo {
		vk::Device device{};
		Gpu gpu{};
		GLFWwindow* window{};
		vk::Queue queue{};
		vk::Instance instance{};
		vk::Format format{};
		Swapchain& swapchain;
		VmaAllocator* allocator{};
	};

	class Renderer {
	public:
		using CreateInfo = RendererCreateInfo;
		explicit Renderer(CreateInfo& create_info);

		void submit(Object& object);
		void draw(Color clear_color = Color::Black);

		std::vector<vk::DescriptorSetLayout> m_set_layout_views{};
		vk::UniqueCommandPool m_cmd_block_pool{};
	private:
		Gpu m_gpu{};
		vk::Device m_device{};
		GLFWwindow* m_window{};
		vk::Instance m_instance{};
		vk::Queue m_queue{};
		vk::Format m_format{};
		Swapchain& m_swapchain;
		VmaAllocator m_allocator{};

		struct RenderSync {
			vk::UniqueSemaphore draw{};
			vk::UniqueFence drawn{};
			vk::CommandBuffer command_buffer{};
		};

		glm::ivec2 m_framebuffer_size{};
		vk::UniqueCommandPool m_render_cmd_pool{};
		Buffered<RenderSync> m_render_sync{};
		std::size_t m_frame_index{};

		std::optional<RenderTarget> m_render_target{};
		std::optional<DearImGui> m_imgui{};

		vk::UniqueDescriptorPool m_descriptor_pool{};
		std::vector<vk::UniqueDescriptorSetLayout> m_set_layouts{};
		vk::UniquePipelineLayout m_pipeline_layout{};
		Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets{};

		std::optional<DescriptorBuffer> m_view_ubo{};
		std::optional<DescriptorBuffer> m_instance_ssbo;
		Transform m_view_transform{};

		std::vector<Object*> m_objects_to_draw{};

		bool m_wireframe{};

		void create_render_sync();
		void create_imgui();
		void create_descriptor_pool();
		void create_pipeline_layout();
		void create_cmd_block_pool();
		void create_descriptor_sets();

		void inspect();
		void update_view();
		void update_instance_ssbo();
		void bind_descriptor_sets(vk::CommandBuffer const command_buffer) const;
		void update_textures_array(std::span<Texture*> textures);

		vk::CommandBuffer begin_frame();
		void transition_for_render(vk::CommandBuffer command_buffer) const;
		void transition_for_present(vk::CommandBuffer command_buffer) const;
		void submit_and_present();


		void draw_objects(vk::CommandBuffer const command_buffer);
		void prepare_frame_resources();

		[[nodiscard]] std::vector<vk::DescriptorSet> allocate_sets() const;
		[[nodiscard]] bool acquire_render_target();
	};
}