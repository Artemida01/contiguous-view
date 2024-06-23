#pragma once

#include "runtime-assert.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>

inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

template <size_t Ext>
struct size_impl {
  size_impl() = default;

  size_impl(size_t a) {}

  size_t size() const {
    return Ext;
  }
};

template <>
struct size_impl<dynamic_extent> {
  size_t size_;

  size_impl(size_t sss)
      : size_(sss) {}

  size_t size() const {
    return size_;
  }
};

template <typename T, size_t Extent = dynamic_extent>
class contiguous_view : private size_impl<Extent> {
public:
  using value_type = std::remove_const_t<T>;

  using pointer = T*;
  using const_pointer = const T*;

  using reference = T&;
  using const_reference = const T&;

  using iterator = pointer;
  using const_iterator = const_pointer;

public:
  contiguous_view() noexcept
      : size_impl<Extent>(Extent == dynamic_extent ? 0 : Extent)
      , begin_(nullptr) {}

  template <typename It>
  explicit contiguous_view(It first, size_t count)
      : size_impl<Extent>(count)
      , begin_(std::to_address(first)) {
    runtime_assert(count <= this->size(), "too much elements");
  }

  template <typename It>
  explicit contiguous_view(It first, It last)
      : size_impl<Extent>(std::distance(first, last))
      , begin_(std::to_address(first)) {
    runtime_assert(first <= last, "Incorrect iterators");
    runtime_assert(last - first <= this->size(), "Too much elements");
  }

  contiguous_view(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N>
    requires (dynamic_extent == Extent || dynamic_extent == N || Extent == N)
  explicit(!std::is_same_v<U, std::remove_const_t<T>> || (Extent != dynamic_extent && Extent != N))
      contiguous_view(const contiguous_view<U, N>& other) noexcept
      : size_impl<Extent>(other.size())
      , begin_(other.begin()) {
    runtime_assert(other.size() <= this->size(), "Too much size");
  }

  contiguous_view& operator=(const contiguous_view& other) noexcept = default;

  pointer data() const noexcept {
    return begin_;
  }

  size_t size() const noexcept {
    return size_impl<Extent>::size();
  }

  size_t size_bytes() const noexcept {
    return sizeof(T) * this->size();
  }

  bool empty() const noexcept {
    return (this->size() == 0);
  }

  iterator begin() const noexcept {
    return begin_;
  }

  const_iterator cbegin() const noexcept {
    return begin_;
  }

  iterator end() const noexcept {
    return begin() + this->size();
  }

  const_iterator cend() const noexcept {
    return begin() + this->size();
  }

  reference operator[](size_t idx) const {
    runtime_assert(!empty(), "Index out of bound.");
    runtime_assert(this->size() > idx, "Index out of bound.");
    return *(begin() + idx);
  }

  reference front() const {
    runtime_assert(!empty(), "No such element (front).");
    return *begin();
  }

  reference back() const {
    runtime_assert(!empty(), "No such element (back).");
    return *(end() - 1);
  }

  contiguous_view<T, dynamic_extent> subview(size_t offset, size_t count = dynamic_extent) const {
    runtime_assert(offset <= this->size(), "Offset must be lower than size.");
    count = (count == dynamic_extent) ? (this->size() - offset) : count;
    runtime_assert(offset + count <= this->size(), "Size + Offset must be lower or equals than size.");
    return contiguous_view<T, dynamic_extent>(begin() + offset, begin() + offset + count);
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  auto subview() const {
    runtime_assert(Offset <= this->size(), "Offset must be lower than size.");
    if constexpr (Count == dynamic_extent) {
      if constexpr (Extent == dynamic_extent) {
        return contiguous_view<T, dynamic_extent>(begin() + Offset, begin() + this->size());
      } else {
        return contiguous_view<T, Extent - Offset>(begin() + Offset, begin() + this->size());
      }
    } else {
      runtime_assert(Offset + Count <= this->size(), "Offset must be lower than size.");
      return contiguous_view<T, Count>(begin() + Offset, begin() + Offset + Count);
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////

  template <size_t Count>
  contiguous_view<T, Count> first() const {
    runtime_assert(Count <= Extent, "Count out of bounds.");
    runtime_assert(Count != -1 && Count < this->size(), "Count out of bounds.");
    return contiguous_view<T, Count>(begin(), Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    runtime_assert(count <= Extent, "Count out of bounds.");
    runtime_assert(count != -1 && count < this->size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin(), count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    runtime_assert(Count <= Extent, "Count out of bounds.");
    runtime_assert(Count != -1 && Count < this->size(), "Count out of bounds.");
    return contiguous_view<T, Count>(begin() + this->size() - Count, Count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    runtime_assert(count <= Extent, "Count out of bounds.");
    runtime_assert(count != -1 && count < this->size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin() + this->size() - count, count);
  }

  auto as_bytes() const {
    if constexpr (Extent == dynamic_extent) {
      return contiguous_view<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>, dynamic_extent>(
          reinterpret_cast<std::conditional_t<std::is_const_v<T>, const std::byte*, std::byte*>>(begin()),
          size_bytes()
      );
    } else {
      return contiguous_view<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>, Extent * sizeof(T)>(
          reinterpret_cast<std::conditional_t<std::is_const_v<T>, const std::byte*, std::byte*>>(begin()),
          size_bytes()
      );
    }
  }

  void swap(contiguous_view other) {
    using std::swap;
    swap(other.begin_, begin_);
    swap(static_cast<size_impl<Extent>&>(*this), static_cast<size_impl<Extent>&>(other));
  }

  explicit operator std::string_view() const {
    runtime_assert(
        std::is_same_v<T, const char>,
        "Only contiguous_view<const char> can be converted to std::string_view."
    );
    return std::string_view(reinterpret_cast<const char*>(begin()), static_cast<std::size_t>(this->size()));
  }

private:
  iterator begin_;
};
