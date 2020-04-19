#ifndef BTWM_UTILS_HPP
#define BTWM_UTILS_HPP

#include <cstdint>
#include <array>
#include <type_traits>


namespace btwm {
	inline namespace utils {
		struct rect {
			int x,y,w,h;
		};

		template <typename T>
		class array_view
		{
			T* m_beg;
			T* m_end;

			template <std::size_t SIZE, bool is_const>
			struct array_init_v;

			template <std::size_t SIZE>
			struct array_init_v<SIZE, true>{
				using array = const std::array<T,SIZE>;
			};
			template <std::size_t SIZE>
			struct array_init_v<SIZE, false>{
				using array = std::array<T,SIZE>;
			};

			template <std::size_t SIZE>
			using array_init = typename array_init_v<SIZE, std::is_const_v<T>>::array;


		public:
			constexpr array_view(T* a, std::size_t size): m_beg(a), m_end(a+size){}
			template <std::size_t SIZE>
			constexpr array_view(T data[SIZE]) : m_beg(data), m_end(data + SIZE) {}


			template <std::size_t SIZE>
			constexpr array_view(std::array<T,SIZE> & data): m_beg(data.data()), m_end(data.data() + data.size()) {}

			[[nodiscard]] constexpr auto operator [](const std::size_t index) const noexcept -> const T& { return m_beg[index]; }
			[[nodiscard]] constexpr auto operator [](const std::size_t index)       noexcept ->       T& { return m_beg[index]; }
			[[nodiscard]] constexpr auto size() const -> std::size_t { return m_end - m_beg; }
			[[nodiscard]] constexpr auto data()       noexcept ->       T* { return m_beg; }
			[[nodiscard]] constexpr auto data() const noexcept -> const T* { return m_beg; }
			[[nodiscard]] constexpr auto begin()       noexcept ->       T* { return m_beg; }
			[[nodiscard]] constexpr auto begin() const noexcept -> const T* { return m_beg; }
			[[nodiscard]] constexpr auto end()       noexcept ->       T* { return m_end; }
			[[nodiscard]] constexpr auto end() const noexcept -> const T* { return m_end; }
		};
	}
}

#endif
