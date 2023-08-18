/**
 * @file
 */

#pragma once

#include <stddef.h> // size_t

namespace core {

/**
 * @brief Fixed size array - does not allocate
 * @ingroup Collections
 */
template<class TYPE, size_t SIZE>
struct Array {
	TYPE _items[SIZE];

	[[nodiscard]] constexpr size_t size() const {
		return SIZE;
	}

	[[nodiscard]] const TYPE* data() const {
		return _items;
	}

	[[nodiscard]] TYPE* data() {
		return _items;
	}

	[[nodiscard]] constexpr bool empty() const {
		return false;
	}

	[[nodiscard]] inline constexpr TYPE& operator[](size_t index) {
		return _items[index];
	}

	[[nodiscard]] inline constexpr const TYPE& operator[](size_t index) const {
		return _items[index];
	}

	constexpr void fill(const TYPE& value) {
		for (size_t i = 0u; i < SIZE; ++i) {
			_items[i] = value;
		}
	}

	[[nodiscard]] constexpr TYPE* begin() {
		return _items;
	}

	[[nodiscard]] constexpr const TYPE* begin() const {
		return _items;
	}

	[[nodiscard]] constexpr TYPE* end() {
		return &_items[SIZE];
	}

	[[nodiscard]] constexpr const TYPE* end() const {
		return &_items[SIZE];
	}
};

}
