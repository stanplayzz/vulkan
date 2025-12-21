#pragma once
#include "scoped.hpp"
#include "command_block.hpp"
#include "bitmap.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace sve::vma {
	struct Deleter
	{
		void operator()(VmaAllocator allocator) const noexcept;
	};

	using Allocator = Scoped<VmaAllocator, Deleter>;

	[[nodiscard]] Allocator create_allocator(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device);

	struct RawBuffer {
		[[nodiscard]] std::span<std::byte> mapped_span() {
			return std::span{ static_cast<std::byte*>(mapped), size };
		}

		bool operator==(RawBuffer const& rhs) const = default;

		VmaAllocator allocator{};
		VmaAllocation allocation{};
		vk::Buffer buffer{};
		vk::DeviceSize size{};
		void* mapped{};
	};

	struct BufferDeleter {
		void operator()(RawBuffer const& raw_buffer) const noexcept;
	};
	
	using Buffer = Scoped<RawBuffer, BufferDeleter>;

	struct BufferCreateInfo {
		VmaAllocator allocator;
		vk::BufferUsageFlags usage;
		std::uint32_t queue_family;
	};

	enum class BufferMemoryType : std::int8_t { Host, Device };

	[[nodiscard]] Buffer create_buffer(BufferCreateInfo const& create_info, BufferMemoryType memory_type, vk::DeviceSize size);

	using ByteSpans = std::span<std::span<std::byte const> const>;

	[[nodiscard]] Buffer create_device_buffer(BufferCreateInfo const& create_info, CommandBlock command_block, ByteSpans const& byte_spans);

	struct RawImage {
		bool operator==(RawImage const& rhs) const = default;

		VmaAllocator allocator{};
		VmaAllocation allocation{};
		vk::Image image{};
		vk::Extent2D extent{};
		vk::Format format{};
		std::uint32_t levels{};
	};

	struct ImageDeleter {
		void operator()(RawImage const& raw_image) const noexcept;
	};
	
	using Image = Scoped<RawImage, ImageDeleter>;

	struct ImageCreateInfo {
		VmaAllocator allocator{};
		std::uint32_t queue_family{};
	};

	[[nodiscard]] Image create_image(ImageCreateInfo const& create_info, vk::ImageUsageFlags usage, std::uint32_t levels, vk::Format format, vk::Extent2D extent);

	[[nodiscard]] Image create_sampled_image(ImageCreateInfo const& create_info, CommandBlock command_block, Bitmap const& bitmap);
}