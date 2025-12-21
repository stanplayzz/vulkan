#include "command_block.hpp"
#include <chrono>
#include <print>

using namespace std::chrono_literals;

namespace sve {
	CommandBlock::CommandBlock(vk::Device const device, vk::Queue const queue, vk::CommandPool const command_pool)
	: m_device(device), m_queue(queue) {
		auto allocate_info = vk::CommandBufferAllocateInfo{};
		allocate_info.setCommandPool(command_pool)
			.setCommandBufferCount(1)
			.setLevel(vk::CommandBufferLevel::ePrimary);
		auto command_buffers = m_device.allocateCommandBuffersUnique(allocate_info);
		m_command_buffer = std::move(command_buffers.front());

		auto begin_info = vk::CommandBufferBeginInfo{};
		begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		m_command_buffer->begin(begin_info);
	}

	void CommandBlock::submit_and_wait() {
		if (!m_command_buffer) return;

		m_command_buffer->end();
		auto submit_info = vk::SubmitInfo2KHR{};
		auto const command_buffer_info = vk::CommandBufferSubmitInfo{ *m_command_buffer };
		submit_info.setCommandBufferInfos(command_buffer_info);
		auto fence = m_device.createFenceUnique({});
		m_queue.submit2(submit_info, *fence);

		static constexpr auto timeout_v = static_cast<std::uint64_t>(std::chrono::nanoseconds(30s).count());
		auto const result = m_device.waitForFences(*fence, vk::True, timeout_v);
		if (result != vk::Result::eSuccess) {
			std::println(stderr, "Failed to submit Command Buffer");
		}
		m_command_buffer.reset();
	}
}