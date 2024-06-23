#pragma once

#include "runtime-assert.h"
#include "type_traits"

#include <algorithm>
#include <cstddef>

inline constexpr size_t dynamic_extent = -1;

template <typename T, size_t Extent = dynamic_extent>
class contiguous_view {
public:
  using value_type = /*std::remove_const*/ T;

  using pointer = T*;
  using const_pointer = const T*;

  using reference = T&;
  using const_reference = const T&;

  using iterator = pointer;
  using const_iterator = const_pointer;

public:
  contiguous_view() noexcept
      : begin_(nullptr)
      , end_(nullptr) {
    runtime_assert(Extent == dynamic_extent, "Default constructor can only be used with dynamic extent.");
  }

  template <typename It>
  explicit contiguous_view(It first, size_t count)
      : begin_(std::addressof(*first))
      , end_(std::addressof(*(first + count))) {
    // runtime_assert(first, "Pointer cannot be null.");
    if constexpr (Extent != dynamic_extent) {
      if (count != Extent) {
        throw std::runtime_error("Size must match Extent.");
      }
    }
  }

  operator contiguous_view<const T, Extent>() const {
    return contiguous_view<const T, Extent>(begin_, end_);
  }

  // explicit operator contiguous_view<T>() const {
  //   return contiguous_view<T>(begin(), end());
  // }

  //////////////////////////////////////////////////////////////////////////////////////////

  template <typename It>
  explicit contiguous_view(It first, It last)
      : begin_(std::addressof(*first))
      , end_(std::addressof(*last)) {
    // runtime_assert(first == decltype(first)(nullptr) && last == decltype(last)(nullptr), "Pointers cannot be null.");
    // runtime_assert(Extent == dynamic_extent || size() == Extent, "Size must match Extent.");
  }

  //////////////////////////////////////////////////////////////////////////////////////////

  contiguous_view(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N>
  contiguous_view(const contiguous_view<U, N>& other) noexcept
      : begin_(other.begin())
      , end_(other.end()) {
    static_assert(std::is_convertible<U*, T*>::value, "Incompatible types.");
    runtime_assert(
        N == dynamic_extent || Extent == dynamic_extent || N == Extent,
        "Cannot convert between different static extents."
    );
  }

  contiguous_view& operator=(const contiguous_view& other) noexcept {
    // if (&other != *this) {
    swap(contiguous_view(other));
    // }
    return *this;
  }

  pointer data() const noexcept {
    return begin_;
  }

  size_t size() const noexcept {
    return std::distance(begin(), end());
  }

  size_t size_bytes() const noexcept {
    return sizeof(T) * size();
  }

  bool empty() const noexcept {
    return (size() == 0);
  }

  iterator begin() const noexcept {
    return begin_;
  }

  const_iterator cbegin() const noexcept {
    return begin_;
  }

  iterator end() const noexcept {
    return end_;
  }

  const_iterator cend() const noexcept {
    return end_;
  }

  reference operator[](size_t idx) const {
    runtime_assert(idx < size(), "Index out of bound.");
    return *(begin() + idx);
  }

  reference front() const {
    runtime_assert(!empty(), "No such element (front).");
    return *begin();
  }

  reference back() const {
    runtime_assert(!empty(), "No such element (back).");
    return *(begin() - 1);
  }

  contiguous_view<T, dynamic_extent> subview(size_t offset, size_t count = dynamic_extent) const {
    runtime_assert(offset <= size(), "Offset out of bounds.");
    if (count == dynamic_extent) {
      count = size() - offset;
    }
    runtime_assert(offset + count <= size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin() + offset, count);
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  contiguous_view<T, 0> subview() const {
    static_assert(Offset <= Extent || Extent == dynamic_extent, "Offset out of bounds.");
    constexpr size_t new_extent =
        (Count == dynamic_extent) ? (Extent == dynamic_extent ? dynamic_extent : Extent - Offset) : Count;
    static_assert(new_extent == dynamic_extent || (Offset + new_extent) <= Extent, "Count out of bounds.");

    return contiguous_view<T, 0>(begin() + Offset, new_extent);
    // return contiguous_view<T, new_extent>(begin() + Offset, new_extent);
  }

  template <size_t Count>
  auto first() const {
    static_assert(Count <= Extent, "Count out of bounds.");
    return contiguous_view<T, Count>(begin(), Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    runtime_assert(count <= size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin(), count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    runtime_assert(Count <= Extent, "Count out of bounds.");
    return contiguous_view<T, Count>(begin() + size() - Count, Count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    runtime_assert(count <= Extent, "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin() + size() - count, count);
  }

  // contiguous_view</*const?*/ std::byte, 0> as_bytes() const {
  //   return reinterpret_cast<std::byte>);
  // }
  auto as_bytes() const {
    return contiguous_view<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>, dynamic_extent>(
        reinterpret_cast<std::conditional_t<std::is_const_v<T>, const std::byte*, std::byte*>>(begin()),
        size_bytes()
    );
  }

  void swap(contiguous_view other) {
    using std::swap;
    swap(other.begin_, begin_);
    swap(other.end_, end_);
  }

  explicit operator std::string_view() const {
    runtime_assert(
        std::is_same_v<T, const char>,
        "Only contiguous_view<const char> can be converted to std::string_view."
    );
    return std::string_view(reinterpret_cast<const char*>(begin()), static_cast<std::size_t>(end() - begin()));
  }

private:
  iterator begin_;
  iterator end_;
};
