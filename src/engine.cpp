#include "engine.hpp"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#include <glm/gtc/matrix_transform.hpp>
#include <bit>
#include <cassert>
#include <chrono>
#include <fstream>
#include <print>
#include <ranges>


namespace {
	[[maybe_unused]] constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);
}

using namespace std::chrono_literals;

namespace sve {
	namespace {
		template <typename T>
		[[nodiscard]] constexpr auto to_byte_array(T const& t) {
			return std::bit_cast<std::array<std::byte, sizeof(T)>>(t);
		}

		constexpr auto layout_binding(std::uint32_t binding, vk::DescriptorType const type) {
			return vk::DescriptorSetLayoutBinding{ binding, type, 1, vk::ShaderStageFlagBits::eAllGraphics };
		}

		[[nodiscard]] fs::path locate_assets_dir() {
			static constexpr std::string_view dir_name_v{ "assets" };
			for (auto path = fs::current_path();
				!path.empty() && path.has_parent_path(); path = path.parent_path()) {
				auto ret = path / dir_name_v;
				if (fs::is_directory(ret)) return ret;
			}
			std::println("[sve] Warning: Could not locate '{}' directory", dir_name_v);
			return fs::current_path();
		}

		[[nodiscard]] std::vector<char const*> get_layers(std::span<char const* const> desired) {
			auto ret = std::vector<char const*>{};
			ret.reserve(desired.size());
			auto const available = vk::enumerateInstanceLayerProperties();
			for (char const* layer : desired) {
				auto const pred = [layer = std::string_view{ layer }](
					vk::LayerProperties const& properties) {
						return properties.layerName == layer;
					};
				if (std::ranges::find_if(available, pred) == available.end()) {
					std::println("[sve] [WARNING] Vulkan Layer '{}' not found", layer);
					continue;
				}
				ret.push_back(layer);
			}
			return ret;
		}

		[[nodiscard]] std::vector<std::uint32_t> to_spir_v(fs::path const& path) {
			auto file = std::ifstream{ path, std::ios::binary | std::ios::ate };
			if (!file.is_open()) {
				throw std::runtime_error{ std::format("Failed to open file: '{}'", path.generic_string()) };
			}

			auto const size = file.tellg();
			auto const usize = static_cast<std::uint64_t>(size);

			if (usize % sizeof(std::uint32_t) != 0) {
				throw std::runtime_error{ std::format("Invalid SPIR_V size: {}", usize) };
			}

			file.seekg({}, std::ios::beg);
			auto ret = std::vector<std::uint32_t>{};
			ret.resize(usize / sizeof(std::uint32_t));
			void* data = ret.data();
			file.read(static_cast<char*>(data), size);
			return ret;
		}
	}

	void App::run() {
		m_assets_dir = locate_assets_dir();

		create_window();
		create_instance();
		create_surface();
		select_gpu();
		create_device();
		create_allocator();
		create_swapchain();

		create_renderer();
		create_shader();

		create_shader_resources();


		main_loop();
	}

	void App::create_window() {
		m_window = glfw::create_window({ 1280, 720 }, "Stan's Vulkan Engine");
	}

	void App::create_instance() {
		VULKAN_HPP_DEFAULT_DISPATCHER.init();
		auto const loader_version = vk::enumerateInstanceVersion();
		if (loader_version < vk_version_v) {
			throw std::runtime_error{ "Loader does not support Vulkan 1.3" };
		}

		auto app_info = vk::ApplicationInfo{};
		app_info.setPApplicationName("Stan's Vulkan Engine").setApiVersion(vk_version_v);

		auto instance_ci = vk::InstanceCreateInfo{};
		auto const extensions = glfw::instance_extensions();
		instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(extensions);

		static constexpr auto layers_v = std::array{
			"VK_LAYER_KHRONOS_shader_object"
		};
		auto const layers = get_layers(layers_v);
		instance_ci.setPEnabledLayerNames(layers);

		m_instance = vk::createInstanceUnique(instance_ci);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);
	}

	void App::create_surface() {
		m_surface = glfw::create_surface(m_window.get(), *m_instance);
	}

	void App::select_gpu() {
		m_gpu = get_suitable_gpu(*m_instance, *m_surface);
		std::println("Using GPU: {}", std::string_view{ m_gpu.properties.deviceName });
	}

	void App::create_device() {
		auto queue_ci = vk::DeviceQueueCreateInfo{};

		static constexpr auto queue_priorities_v = std::array{ 1.0f };
		queue_ci.setQueueFamilyIndex(m_gpu.queue_family).setQueueCount(1).setQueuePriorities(queue_priorities_v);

		auto enabled_features = vk::PhysicalDeviceFeatures{};
		enabled_features.fillModeNonSolid = m_gpu.features.fillModeNonSolid;
		enabled_features.wideLines = m_gpu.features.wideLines;
		enabled_features.samplerAnisotropy = m_gpu.features.samplerAnisotropy;
		enabled_features.sampleRateShading = m_gpu.features.sampleRateShading;

		auto sync_feature = vk::PhysicalDeviceSynchronization2Features{ vk::True };
		auto dynamic_rendering_feature = vk::PhysicalDeviceDynamicRenderingFeatures{ vk::True };
		sync_feature.setPNext(&dynamic_rendering_feature);
		auto shader_object_feature = vk::PhysicalDeviceShaderObjectFeaturesEXT{ vk::True };
		dynamic_rendering_feature.setPNext(&shader_object_feature);

		auto device_ci = vk::DeviceCreateInfo{};
		static constexpr auto extensions_v = std::array{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_EXT_shader_object" };
		device_ci.setPEnabledExtensionNames(extensions_v).setQueueCreateInfos(queue_ci).setPEnabledFeatures(&enabled_features).setPNext(&sync_feature);

		m_device = m_gpu.device.createDeviceUnique(device_ci);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);

		static constexpr std::uint32_t queue_index_v{ 0 };
		m_queue = m_device->getQueue(m_gpu.queue_family, queue_index_v);

		m_waiter = *m_device;
	}

	void App::create_allocator() {
		m_allocator = vma::create_allocator(*m_instance, m_gpu.device, *m_device);
	}

	void App::create_swapchain() {
		auto const size = glfw::framebuffer_size(m_window.get());
		m_swapchain.emplace(*m_device, m_gpu, *m_surface, size);
	}

	void App::create_shader() {
		auto const vertex_spirv = to_spir_v(asset_path("shader.vert"));
		auto const fragment_spirv = to_spir_v(asset_path("shader.frag"));

		static constexpr auto vertex_input_v = ShaderVertexInput{
			.attributes = vertex_attributes_v,
			.bindings = vertex_binding_v
		};

		auto const shader_ci = ShaderProgram::CreateInfo{
			.device = *m_device,
			.vertex_spirv = vertex_spirv,
			.fragment_spirv = fragment_spirv,
			.vertex_input = vertex_input_v,
			.set_layouts = m_renderer->m_set_layout_views
		};
		m_shader.emplace(shader_ci);
	}

	void App::create_shader_resources() {

		static constexpr auto vertices_v = std::array{
			Vertex{.position = { -200.f, -200.f }, .uv = {0.0f, 1.0f}},
			Vertex{.position = {  200.f, -200.f }, .uv = {1.0f, 1.0f}},
			Vertex{.position = {  200.f,  200.f }, .uv = {1.0f, 0.0f}},
			Vertex{.position = { -200.f,  200.f }, .uv = {0.0f, 0.0f}},
		};

		static constexpr auto indices_v = std::array{ 0u, 1u, 2u, 2u, 3u, 0u };

		static auto constexpr vertices_bytes_v = to_byte_array(vertices_v);
		static auto constexpr indices_bytes_v = to_byte_array(indices_v);

		static auto constexpr total_bytes_v = std::array<std::span<std::byte const>, 2>{ vertices_bytes_v, indices_bytes_v };

		auto buffer_ci = vma::BufferCreateInfo{
			.allocator = m_allocator.get(),
			.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
			.queue_family = m_gpu.queue_family
		};
		

		m_vbo = vma::create_device_buffer(buffer_ci, create_command_block(), total_bytes_v);
		
		//m_view_ubo.emplace(m_allocator.get(), m_gpu.queue_family, vk::BufferUsageFlagBits::eUniformBuffer);
		m_instance_ssbo.emplace(m_allocator.get(), m_gpu.queue_family, vk::BufferUsageFlagBits::eStorageBuffer);

		using Pixel = std::array<std::byte, 4>;
		static constexpr auto rgby_pixels_v = std::array{
			Pixel{ std::byte{0xff}, {}, {}, std::byte{0xff} },
			Pixel{ std::byte{}, std::byte{0xff}, {}, std::byte{0xff}},
			Pixel{ std::byte{}, std::byte{}, std::byte{0xff}, std::byte{0xff}},
			Pixel{ std::byte{0xff}, std::byte{0xff}, std::byte{}, std::byte{0xff} },
		};

		static constexpr auto rgby_bytes_v = std::bit_cast<std::array<std::byte, sizeof(rgby_pixels_v)>>(rgby_pixels_v);
		static constexpr auto rgby_bitmap_v = Bitmap{
			.bytes = rgby_bytes_v,
			.size = {2, 2}
		};

		auto texture_ci = Texture::CreateInfo{
			.device = *m_device,
			.allocator = m_allocator.get(),
			.queue_family = m_gpu.queue_family,
			.command_block = create_command_block(),
			.bitmap = rgby_bitmap_v
		};

		texture_ci.sampler.setMagFilter(vk::Filter::eNearest);
		m_texture.emplace(std::move(texture_ci));

		m_object.mesh.vertex_buffer = vma::create_device_buffer(buffer_ci, create_command_block(), total_bytes_v);
		m_object.mesh.index_count = 6;
		m_object.material.texture = &m_texture.value();
		m_object.material.shader = &m_shader.value();
	}

	void App::create_renderer() {
		auto renderer_ci = RendererCreateInfo{ .swapchain = *m_swapchain };
		renderer_ci.device = *m_device;
		renderer_ci.format = m_swapchain->get_format();
		renderer_ci.gpu = m_gpu;
		renderer_ci.instance = *m_instance;
		renderer_ci.queue = m_queue;
		renderer_ci.window = &*m_window;
		renderer_ci.allocator = &m_allocator.get();

		m_renderer.emplace(renderer_ci);
	}

	fs::path App::asset_path(std::string_view const uri) const {
		return m_assets_dir / uri;
	}

	CommandBlock App::create_command_block() const {
		return CommandBlock{ *m_device, m_queue, *m_renderer->m_cmd_block_pool };
	}

	void App::main_loop() {
		

		while (glfwWindowShouldClose(m_window.get()) == GLFW_FALSE) {
			glfwPollEvents();
			

			update_instances();

			m_renderer->submit(m_object);

			m_renderer->draw(Color(10, 10, 10));
			 
		}
	}

	void App::update_instances() {
		m_instance_data.clear();
		m_instance_data.reserve(m_instances.size());
		for (auto const& transform : m_instances) {
			m_instance_data.push_back(transform.model_matrix());
		}
		
		auto const span = std::span{ m_instance_data };
		void* data = span.data();
		auto const bytes = std::span{ static_cast<std::byte const*>(data), span.size_bytes() };
		m_instance_ssbo->write_at(m_frame_index, bytes);
	}
}