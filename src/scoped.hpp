#pragma once
#include <concepts>
#include <utility>


namespace sve {
	template <typename Type>
	concept Scopeable = std::equality_comparable<Type>;

	template<Scopeable Type, typename Deleter>
	class Scoped {
	public:
		Scoped(Scoped const&) = delete;
		auto operator=(Scoped const&) = delete;

		Scoped() = default;

		constexpr Scoped(Scoped&& rhs) noexcept : m_t(std::exchange(rhs.m_t, Type{})) {}
		
		constexpr Scoped& operator=(Scoped&& rhs) noexcept {
			if (&rhs != this) {
				std::swap(m_t, rhs.m_t);
			}
			return *this;
		}

		explicit(false) constexpr Scoped(Type t) : m_t(std::move(t)) {}

		constexpr ~Scoped() {
			if (m_t == Type{}) {
				return;
			}
			Deleter{}(m_t);
		}

		[[nodiscard]] constexpr Type const& get() const { return m_t; };
		[[nodiscard]] constexpr Type& get() { return m_t; };

	private:
		Type m_t{};
	};
}