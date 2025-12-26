#pragma once
#include <vulkan/vulkan.hpp>

namespace sve {
	namespace {
		constexpr float u8_to_f32(uint8_t v) {
			return float(v) * (1.0f / 255.0f);
		}

		auto srgb_to_linear = [](float c) {
			return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
			};
	}

	struct Color
	{
		uint8_t r{ 255 }, g{ 255 }, b{ 255 }, a{ 255 };

		constexpr Color() = default;

		constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

		vk::ClearColorValue to_vk_clear_srgb() const {
			auto srgb_to_linear = [](float c) {
				return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
				};
			return {
				srgb_to_linear(r / 255.f),
				srgb_to_linear(g / 255.f),
				srgb_to_linear(b / 255.f),
				a / 255.f
			};
		}

		static const Color White;
		static const Color Black;
		static const Color Red;
		static const Color Green;
		static const Color Blue;
		static const Color Orange;
		static const Color Cyan;
		static const Color Purple;
		static const Color Transparent;
	};

	inline constexpr Color Color::White{ 255, 255, 255 };
	inline constexpr Color Color::Black{ 0, 0, 0 };
	inline constexpr Color Color::Red{ 255, 0, 0 };
	inline constexpr Color Color::Green{ 0, 255, 0 };
	inline constexpr Color Color::Blue{ 0, 0, 255 };
	inline constexpr Color Color::Orange{ 255, 165, 0 };
	inline constexpr Color Color::Cyan{ 0, 255, 255 };
	inline constexpr Color Color::Purple{ 128, 0, 128 };
	inline constexpr Color Color::Transparent{ 0, 0, 0, 0 };
}