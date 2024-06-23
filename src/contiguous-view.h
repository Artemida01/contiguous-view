#pragma once

#include "runtime-assert.h"
#include "type_traits"

#include <algorithm>
#include <cstddef>

inline constexpr size_t dynamic_extent = -1;

template <size_t Ext>
struct size_spec {
  size_spec() = default;

  size_spec(size_t a) {}

  size_t size() const {
    return Ext;
  }
};

template <>
struct size_spec<dynamic_extent> {
  size_t size_;

  size_spec(size_t sss)
      : size_(sss) {}

  size_t size() const {
    return size_;
  }
};

template <typename T, size_t Extent = dynamic_extent>
class contiguous_view {
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
      : begin_(nullptr)
      , size_spec_(0) {}

  template <typename It>
  explicit contiguous_view(It first, size_t count)
      : begin_(std::to_address(first))
      , size_spec_(count) {
    runtime_assert(count <= size(), "too much elements");
  }

  template <typename It>
  explicit contiguous_view(It first, It last)
      : begin_(std::to_address(first))
      , size_spec_(last - first) {
    runtime_assert(first <= last, "Incorrect iterators");
    runtime_assert(last - first <= size(), "Too much elements");
  }

  contiguous_view(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N>
    requires (N == dynamic_extent || Extent == dynamic_extent || N == Extent)
  explicit(!(std::is_same_v<U, std::remove_const_t<T>> && (Extent == dynamic_extent || Extent == N)))
      contiguous_view(const contiguous_view<U, N>& other) noexcept
      : begin_(other.begin())
      , size_spec_(other.size()) {
    runtime_assert(other.size() <= size(), "Too much size");
  }

  contiguous_view& operator=(const contiguous_view& other) noexcept = default;

  pointer data() const noexcept {
    return begin_;
  }

  size_t size() const noexcept {
    return size_spec_.size();
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
    return begin() + size();
  }

  const_iterator cend() const noexcept {
    return begin() + size();
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
    return *(end() - 1);
  }

  auto subview(size_t offset, size_t count = dynamic_extent) const {
    if (count == dynamic_extent) {
      runtime_assert(offset <= size(), "Offset must be lower than size.");
      return contiguous_view<T, dynamic_extent>(begin() + offset, end());
    } else {
      runtime_assert(offset + count <= size(), "Size + Offset must be lower or equals than size.");
      return contiguous_view<T, dynamic_extent>(begin() + offset, begin() + offset + count);
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////

  template <size_t Offset, size_t Count = dynamic_extent>
  contiguous_view<T, Count> subview() const {
    if (Count == dynamic_extent) {
      runtime_assert(Offset <= size(), "Offset must be lower than size.");
      return contiguous_view<T, Count>(begin() + Offset, begin() + size()); /// переписать
    } else {
      runtime_assert(Offset + Count <= size(), "Offset must be lower than size.");
      return contiguous_view<T, Count>(begin() + Offset, begin() + Offset + Count);
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////

  template <size_t Count>
  contiguous_view<T, Count> first() const {
    runtime_assert(Count <= Extent, "Count out of bounds.");
    runtime_assert(Count != -1 && Count < size(), "Count out of bounds.");
    return contiguous_view<T, Count>(begin(), Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    runtime_assert(count <= Extent, "Count out of bounds.");
    runtime_assert(count != -1 && count < size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin(), count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    runtime_assert(Count <= Extent, "Count out of bounds.");
    runtime_assert(Count != -1 && Count < size(), "Count out of bounds.");
    return contiguous_view<T, Count>(begin() + size() - Count, Count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    runtime_assert(count <= Extent, "Count out of bounds.");
    runtime_assert(count != -1 && count < size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin() + size() - count, count);
  }

  auto as_bytes() const {
    return contiguous_view<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>, dynamic_extent>(
        reinterpret_cast<std::conditional_t<std::is_const_v<T>, const std::byte*, std::byte*>>(begin()),
        size_bytes()
    );
  }

  void swap(contiguous_view other) {
    using std::swap;
    swap(other.begin_, begin_);
    swap(other.size_spec_, size_spec_);
  }

  explicit operator std::string_view() const {
    runtime_assert(
        std::is_same_v<T, const char>,
        "Only contiguous_view<const char> can be converted to std::string_view."
    );
    return std::string_view(reinterpret_cast<const char*>(begin()), static_cast<std::size_t>(size()));
  }

private:
  iterator begin_;
  [[no_unique_address]] size_spec<Extent> size_spec_;
};
