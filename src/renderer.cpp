#include "renderer.hpp"
#include "window.hpp"
#include "utils/vertex.hpp"
#include <ranges>
#include <chrono>
#include <bit>
#include <glm/ext/matrix_clip_space.hpp>

constexpr auto MAX_OBJECTS = 16;;
constexpr auto MAX_TEXTURES = 16;;

using namespace std::chrono_literals;

namespace sve {
	namespace {
		constexpr auto layout_binding(std::uint32_t binding, vk::DescriptorType const type) {
			return vk::DescriptorSetLayoutBinding{ binding, type, 1, vk::ShaderStageFlagBits::eAllGraphics };
		}
	}

	std::vector<vk::DescriptorSet> Renderer::allocate_sets() const {
		auto allocate_info = vk::DescriptorSetAllocateInfo{};
		allocate_info.setDescriptorPool(*m_descriptor_pool)
			.setSetLayouts(m_set_layout_views);
		return m_device.allocateDescriptorSets(allocate_info);
	}


	Renderer::Renderer(CreateInfo& ci) 
	: m_gpu(ci.gpu), m_device(ci.device), m_window(ci.window), m_instance(ci.instance), m_queue(ci.queue),
	  m_format(ci.format), m_swapchain(ci.swapchain), m_allocator(*ci.allocator) {


		create_render_sync();
		create_imgui();
		create_descriptor_pool();
		create_cmd_block_pool();
		create_pipeline_layout();
		create_descriptor_sets();

		m_view_ubo.emplace(m_allocator, m_gpu.queue_family, vk::BufferUsageFlagBits::eUniformBuffer);
		m_instance_ssbo.emplace(m_allocator, m_gpu.queue_family, vk::BufferUsageFlagBits::eStorageBuffer);
	}

	void Renderer::create_render_sync() {
		auto command_pool_ci = vk::CommandPoolCreateInfo{};
		command_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
			.setQueueFamilyIndex(m_gpu.queue_family);
		m_render_cmd_pool = m_device.createCommandPoolUnique(command_pool_ci);

		auto command_buffer_ai = vk::CommandBufferAllocateInfo{};
		command_buffer_ai.setCommandPool(*m_render_cmd_pool)
			.setCommandBufferCount(static_cast<std::uint32_t>(resource_buffering_v))
			.setLevel(vk::CommandBufferLevel::ePrimary);
		auto const command_buffers = m_device.allocateCommandBuffers(command_buffer_ai);
		assert(command_buffers.size() == m_render_sync.size());

		static constexpr auto fence_create_info_v = vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled };
		for (auto [sync, command_buffer] : std::views::zip(m_render_sync, command_buffers)) {
			sync.command_buffer = command_buffer;
			sync.draw = m_device.createSemaphoreUnique({});
			sync.drawn = m_device.createFenceUnique(fence_create_info_v);
		}
	}

	void Renderer::create_imgui() {
		auto const imgui_ci = DearImGui::CreateInfo{
			.window = m_window,
			.api_version = vk_version_v,
			.instance = m_instance,
			.physical_device = m_gpu.device,
			.queue_family = m_gpu.queue_family,
			.device = m_device,
			.queue = m_queue,
			.color_format = m_format,
			.samples = vk::SampleCountFlagBits::e1
		};
		m_imgui.emplace(imgui_ci);
	}

	void Renderer::create_descriptor_pool() {
		static constexpr auto pool_sizes_v = std::array{
			vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 8},
			vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler,8},
		};
		auto pool_ci = vk::DescriptorPoolCreateInfo{};
		pool_ci.setPoolSizes(pool_sizes_v).setMaxSets(16);
		m_descriptor_pool = m_device.createDescriptorPoolUnique(pool_ci);
	}

	void Renderer::create_pipeline_layout() {
		static constexpr auto set_0_bindings_v = std::array{
			layout_binding(0, vk::DescriptorType::eUniformBuffer)
		};
		static constexpr auto set_1_bindings_v = std::array{
			vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, MAX_TEXTURES, vk::ShaderStageFlagBits::eFragment},
		};
		static constexpr auto set_2_bindings_v = std::array{
			layout_binding(0, vk::DescriptorType::eStorageBuffer),
		};

		auto set_layout_cis = std::array<vk::DescriptorSetLayoutCreateInfo, 3>{};
		set_layout_cis[0].setBindings(set_0_bindings_v);
		set_layout_cis[1].setBindings(set_1_bindings_v);
		set_layout_cis[2].setBindings(set_2_bindings_v);

		for (auto const& set_layout_ci : set_layout_cis) {
			m_set_layouts.push_back(m_device.createDescriptorSetLayoutUnique(set_layout_ci));
			m_set_layout_views.push_back(*m_set_layouts.back());
		}

		vk::PushConstantRange pc{};
		pc.stageFlags = vk::ShaderStageFlagBits::eFragment;
		pc.offset = 0;
		pc.size = sizeof(uint32_t);

		auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
		pipeline_layout_ci.setSetLayouts(m_set_layout_views);
		pipeline_layout_ci.setPushConstantRanges(pc);
		m_pipeline_layout = m_device.createPipelineLayoutUnique(pipeline_layout_ci);
	}

	void Renderer::create_cmd_block_pool() {
		auto command_pool_ci = vk::CommandPoolCreateInfo{};
		command_pool_ci.setQueueFamilyIndex(m_gpu.queue_family)
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		m_cmd_block_pool = m_device.createCommandPoolUnique(command_pool_ci);
	}

	void Renderer::create_descriptor_sets() {
		for (auto& descriptor_sets : m_descriptor_sets) {
			descriptor_sets = allocate_sets();
		}
	}

	bool Renderer::acquire_render_target() {
		m_framebuffer_size = glfw::framebuffer_size(m_window);
		// skip if minimized
		if (m_framebuffer_size.x <= 0 || m_framebuffer_size.y <= 0) return false;

		auto& render_sync = m_render_sync.at(m_frame_index);

		static constexpr auto fence_timeout_v = static_cast<std::uint64_t>(std::chrono::nanoseconds{ 3s }.count());
		auto result = m_device.waitForFences(*render_sync.drawn, vk::True, fence_timeout_v);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error{ "Failed to wait for Render Fence" };
		}

		m_render_target = m_swapchain.aquire_next_image(*render_sync.draw);
		if (!m_render_target)
		{
			m_swapchain.recreate(m_framebuffer_size);
			return false;
		}

		m_device.resetFences(*render_sync.drawn);
		m_imgui->new_frame();

		return true;
	}

	void Renderer::update_instance_ssbo() {
		std::vector<glm::mat4> models;
		models.reserve(m_objects_to_draw.size());

		for (auto& object : m_objects_to_draw) {
			models.push_back(object->transform.model_matrix());
		}

		m_instance_ssbo->write_at(m_frame_index, std::as_bytes(std::span{ models }));
	}

	void Renderer::update_view() {
		auto const half_size = 0.5f * glm::vec2{ m_framebuffer_size };
		auto const mat_projection = glm::ortho(-half_size.x, half_size.x, -half_size.y, half_size.y);
		auto const mat_view = m_view_transform.view_matrix();
		auto const mat_vp = mat_projection * mat_view;
		auto const bytes = std::bit_cast<std::array<std::byte, sizeof(mat_vp)>>(mat_vp);
		m_view_ubo->write_at(m_frame_index, bytes);
	}

	void Renderer::bind_descriptor_sets(vk::CommandBuffer const command_buffer) const {
		auto writes = std::array<vk::WriteDescriptorSet, 2>{};
		auto const& descriptor_sets = m_descriptor_sets.at(m_frame_index);

		auto write = vk::WriteDescriptorSet{};
		auto const set0 = descriptor_sets[0];
		auto const view_ubo_info = m_view_ubo->descripter_info_at(m_frame_index);

		write.setBufferInfo(view_ubo_info)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setDstSet(set0)
			.setDstBinding(0);
		writes[0] = write;

		auto const set2 = descriptor_sets[2];
		auto ssbo_info = m_instance_ssbo->descripter_info_at(m_frame_index);
		write.setBufferInfo(ssbo_info)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setDstSet(set2)
			.setDstBinding(0);
		writes[1] = write;

		m_device.updateDescriptorSets(writes, {});

		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, descriptor_sets, {});
	}

	void Renderer::update_textures_array(std::span<Texture*> textures) {
		std::vector<vk::DescriptorImageInfo> infos;
		infos.reserve(textures.size());

		for (auto* tex : textures) {
			infos.push_back(tex->descriptor_info());
		}

		vk::WriteDescriptorSet write{};
		write.setDstSet(m_descriptor_sets[m_frame_index][1])
			.setDstBinding(0)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setImageInfo(infos)
			.setDescriptorCount(static_cast<uint32_t>(infos.size()));

		m_device.updateDescriptorSets(write, {});
	}

	void Renderer::inspect() {
		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowSize({ 200.f, 100.f }, ImGuiCond_Once);
		if (ImGui::Begin("Inspect")) {
			/*if (ImGui::Checkbox("Wireframe", &m_wireframe)) {
				m_shader->polygon_mode = m_wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
			}
			if (m_wireframe) {
				auto const& line_width_range = m_gpu.properties.limits.lineWidthRange;
				ImGui::SetNextItemWidth(100.f);
				ImGui::DragFloat("Line Width", &m_shader->line_width, 0.25f, line_width_range[0], line_width_range[1]);
			}*/

			static auto const inspect_transform = [](Transform& out) {
				ImGui::DragFloat2("Position", &out.position.x);
				ImGui::DragFloat("Rotation", &out.rotation);
				ImGui::DragFloat2("Scale", &out.scale.x, 0.1f);
				};

			ImGui::Separator();
			if (ImGui::TreeNode("View")) {
				inspect_transform(m_view_transform);
				ImGui::TreePop();
			}

			ImGui::Separator();
			if (ImGui::TreeNode("Instances")) {
				for (size_t i = 0; i < m_objects_to_draw.size(); i++) {
					auto const label = std::to_string(i);
					if (ImGui::TreeNode(label.c_str())) {
						inspect_transform(m_objects_to_draw.at(i)->transform);
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}

	vk::CommandBuffer Renderer::begin_frame() {
		auto const& render_sync = m_render_sync.at(m_frame_index);

		auto command_buffer_bi = vk::CommandBufferBeginInfo{};
		command_buffer_bi.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		render_sync.command_buffer.begin(command_buffer_bi);
		return render_sync.command_buffer;
	}

	void Renderer::transition_for_render(vk::CommandBuffer const command_buffer) const {
		auto dependency_info = vk::DependencyInfo{};
		auto barrier = m_swapchain.base_barrier();

		barrier.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eAttachmentOptimal)
			.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
			.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstAccessMask(barrier.srcAccessMask)
			.setDstStageMask(barrier.srcStageMask);
		dependency_info.setImageMemoryBarriers(barrier);
		command_buffer.pipelineBarrier2(dependency_info);
	}

	void Renderer::transition_for_present(vk::CommandBuffer const command_buffer) const {
		auto dependency_info = vk::DependencyInfo{};
		auto barrier = m_swapchain.base_barrier();

		barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
			.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
			.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
			.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDstAccessMask(barrier.srcAccessMask)
			.setDstStageMask(barrier.srcStageMask);

		dependency_info.setImageMemoryBarriers(barrier);
		command_buffer.pipelineBarrier2(dependency_info);
	}

	void Renderer::submit_and_present() {
		auto const& render_sync = m_render_sync.at(m_frame_index);
		render_sync.command_buffer.end();

		auto submit_info = vk::SubmitInfo2{};
		auto const command_buffer_info = vk::CommandBufferSubmitInfo{ render_sync.command_buffer };
		auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
		wait_semaphore_info.setSemaphore(*render_sync.draw)
			.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
		auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
		signal_semaphore_info.setSemaphore(m_swapchain.get_present_semaphore())
			.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
		submit_info.setCommandBufferInfos(command_buffer_info)
			.setWaitSemaphoreInfos(wait_semaphore_info)
			.setSignalSemaphoreInfos(signal_semaphore_info);
		m_queue.submit2(submit_info, *render_sync.drawn);

		m_frame_index = (m_frame_index + 1) % m_render_sync.size();

		m_render_target.reset();

		auto const fb_size_changed = m_framebuffer_size != m_swapchain.get_size();
		auto const out_of_date = !m_swapchain.present(m_queue);
		if (fb_size_changed || out_of_date)
		{
			m_swapchain.recreate(m_framebuffer_size);
		}
	}

	void Renderer::draw_objects(vk::CommandBuffer const command_buffer) {
		uint32_t ssbo_index = 0;
		for (uint32_t i = 0; i < m_objects_to_draw.size(); i++)
		{
			auto object = m_objects_to_draw[i];
			command_buffer.pushConstants(
				*m_pipeline_layout,
				vk::ShaderStageFlagBits::eFragment,
				0,
				sizeof(uint32_t),
				&object->texture_index
			);
			object->material.shader->bind(command_buffer, m_framebuffer_size);
			command_buffer.bindVertexBuffers(0, object->mesh.vertex_buffer.get().buffer, vk::DeviceSize{});
			command_buffer.bindIndexBuffer(object->mesh.vertex_buffer.get().buffer, 4 * sizeof(Vertex), vk::IndexType::eUint32);
			command_buffer.drawIndexed(object->mesh.index_count, object->instance_count, 0, 0, ssbo_index);
			ssbo_index += object->instance_count;
		}
	}

	void Renderer::prepare_frame_resources() {
		std::vector<Texture*> unique_textures;

		unique_textures.reserve(m_objects_to_draw.size());

		for (auto& obj : m_objects_to_draw) {
			if (std::ranges::find(unique_textures, obj->material.texture) == unique_textures.end())
				unique_textures.push_back(obj->material.texture);

			for (auto& obj : m_objects_to_draw) {
				obj->texture_index = static_cast<uint32_t>(std::ranges::find(unique_textures, obj->material.texture) - unique_textures.begin());
			}

			update_textures_array(unique_textures);
		}
	}

	void Renderer::submit(Object& object) {
		m_objects_to_draw.push_back(&object);
	}

	void Renderer::draw(Color clear_color) {
		if (!acquire_render_target()) return;
		prepare_frame_resources();

		auto const command_buffer = begin_frame();
		transition_for_render(command_buffer);

		auto color_attachment = vk::RenderingAttachmentInfo{};
		color_attachment.setImageView(m_render_target->image_view)
			.setImageLayout(vk::ImageLayout::eAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setClearValue(clear_color.to_vk_clear_srgb());
		auto rendering_info = vk::RenderingInfo{};
		auto const render_area = vk::Rect2D{ vk::Offset2D{}, m_render_target->extent };
		rendering_info.setRenderArea(render_area)
			.setColorAttachments(color_attachment)
			.setLayerCount(1);

		bind_descriptor_sets(command_buffer);

		
		inspect();
		update_instance_ssbo();
		update_view();


		command_buffer.beginRendering(rendering_info);

		draw_objects(command_buffer);
		

		command_buffer.endRendering();

		m_imgui->end_frame();

		color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
		rendering_info.setColorAttachments(color_attachment)
			.setPDepthAttachment(nullptr);
		command_buffer.beginRendering(rendering_info);
		m_imgui->render(command_buffer);
		command_buffer.endRendering();

		transition_for_present(command_buffer);
		submit_and_present();

		m_objects_to_draw.clear();
	}
}