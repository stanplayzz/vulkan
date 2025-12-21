#pragma once
#include "vma.hpp"
#include "resource_buffering.hpp"
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>


namespace sve {
	class DescriptorBuffer
	{
	public:
		explicit DescriptorBuffer(VmaAllocator allocator, std::uint32_t queue_family, vk::BufferUsageFlags usage);

		void write_at(std::size_t frame_index, std::span<std::byte const> bytes);

		[[nodiscard]] vk::DescriptorBufferInfo descripter_info_at(std::size_t frame_index) const;

	private:
		struct Buffer {
			vma::Buffer buffer{};
			vk::DeviceSize size{};
		};

		void write_to(Buffer& out, std::span<std::byte const> bytes) const;

		VmaAllocator m_allocator{};
		std::uint32_t m_queue_family{};
		vk::BufferUsageFlags m_usage{};
		Buffered<Buffer> m_buffers{};
	};
}