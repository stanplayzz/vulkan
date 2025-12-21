#pragma once
#include <vma.hpp>

namespace sve {
	[[nodiscard]] constexpr auto create_sampler_ci(vk::SamplerAddressMode const wrap, vk::Filter const filter) {
		auto ret = vk::SamplerCreateInfo{};
		ret.setAddressModeU(wrap)
			.setAddressModeV(wrap)
			.setAddressModeW(wrap)
			.setMinFilter(filter)
			.setMagFilter(filter)
			.setMaxLod(VK_LOD_CLAMP_NONE)
			.setBorderColor(vk::BorderColor::eFloatTransparentBlack)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest);
		return ret;
	}

	constexpr auto sampler_ci_v = create_sampler_ci(vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear);

	struct TextureCreateInfo {
		vk::Device device;
		VmaAllocator allocator;
		std::uint32_t queue_family;
		CommandBlock command_block;
		Bitmap bitmap;
		vk::SamplerCreateInfo sampler{ sampler_ci_v };
	};

	class Texture {
	public:
		using CreateInfo = TextureCreateInfo;
		
		explicit Texture(CreateInfo create_info);

		[[nodiscard]] vk::DescriptorImageInfo descriptor_info() const;
	private:
		vma::Image m_image{};
		vk::UniqueImageView m_view{};
		vk::UniqueSampler m_sampler{};
	};
}
