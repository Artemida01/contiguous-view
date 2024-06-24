#pragma once

#include "runtime-assert.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>

inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

template <size_t Ext>
struct size_impl {
  size_impl() = default;

  size_impl(size_t) {}

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
  /// contiguous_view() noexcept
  ///     : size_impl<Extent>(Extent == dynamic_extent ? 0 : Extent)
  ///     , begin_(nullptr) {
  ///   runtime_assert(Extent == 0 || Extent == dynamic_extent, "Incorrect transformation");
  /// }
  explicit(Extent == 0 || Extent == dynamic_extent) contiguous_view() noexcept
      : size_impl<Extent>(Extent == dynamic_extent ? 0 : Extent)
      , begin_(nullptr) {}

  /// было просто
  /// explicit contiguous_view(It first, size_t count)
  template <typename It>
  explicit(Extent != dynamic_extent) contiguous_view(It first, size_t count)
      : size_impl<Extent>(count)
      , begin_(std::to_address(first)) {
    runtime_assert(count == this->size(), "Too much elements"); /// <=
  }

  template <typename It>
  explicit contiguous_view(It first, It last)
      : contiguous_view(first, std::distance(first, last)) {
    runtime_assert(first <= last, "Incorrect iterators");
  }

  contiguous_view(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N> /// explicit ---> requires
    requires (!std::is_same_v<U, std::remove_const_t<T>> || dynamic_extent == Extent || dynamic_extent == N ||
              Extent == N)
  explicit(Extent != dynamic_extent && Extent != N) contiguous_view(const contiguous_view<U, N>& other) noexcept
      : size_impl<Extent>(other.size())
      , begin_(other.begin()) {
    runtime_assert(other.size() == this->size(), "Too much size"); /// <=
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
    return contiguous_view<T, dynamic_extent>(begin() + offset, count); /// begin() + offset + count --> count
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  auto subview() const {
    if constexpr (Offset > Extent) {
      static_assert(Offset <= Extent, "Incorrect Sizes");
    }
    runtime_assert(Offset <= this->size(), "Incorrect Sizes");

    if constexpr (Count != dynamic_extent) {
      if constexpr (Offset + Count > Extent) {
        static_assert(Offset + Count <= Extent, "Incorrect Sizes");
      }
    }

    if constexpr (Count == dynamic_extent) {
      if constexpr (Extent == dynamic_extent) {
        return contiguous_view<T, dynamic_extent>(begin() + Offset, end()); /// begin() + this->size() == end()
      } else {
        return contiguous_view<T, Extent - Offset>(begin() + Offset, end()); /// begin() + this->size() == end()
      }
    } else {
      runtime_assert(Offset + Count <= this->size(), "Offset + Count must be lower than or equal to size.");
      return contiguous_view<T, Count>(begin() + Offset, Count); /// begin() + Offset + Count --> Count
    }
  }

  template <size_t Count>
  contiguous_view<T, Count> first() const {
    if constexpr (Extent != dynamic_extent) {
      static_assert(Count <= Extent, "Count out of bounds.");
    }
    runtime_assert(Count != dynamic_extent && Count <= this->size(), "Count out of bounds.");
    return contiguous_view<T, Count>(begin(), Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    runtime_assert(count <= this->size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin(), count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    if constexpr (Extent != dynamic_extent) {
      static_assert(Count <= Extent, "Count out of bounds.");
    }
    runtime_assert(Count != dynamic_extent && Count <= this->size(), "Count out of bounds.");
    return contiguous_view<T, Count>(begin() + this->size() - Count, Count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    runtime_assert(count <= this->size(), "Count out of bounds.");
    return contiguous_view<T, dynamic_extent>(begin() + this->size() - count, count);
  }

  auto as_bytes() const {
    using my_byte = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;
    using byte_ptr = std::conditional_t<std::is_const_v<T>, const std::byte*, std::byte*>;
    byte_ptr byte_begin = reinterpret_cast<byte_ptr>(begin());
    size_t byte_size = size_bytes();

    if constexpr (Extent == dynamic_extent) {
      return contiguous_view<my_byte, dynamic_extent>(byte_begin, byte_size);
    } else {
      return contiguous_view<my_byte, Extent * sizeof(T)>(byte_begin, byte_size);
    }
  }

  void swap(contiguous_view other) {
    using std::swap;
    swap(other.begin_, begin_);
    swap(static_cast<size_impl<Extent>&>(*this), static_cast<size_impl<Extent>&>(other));
  }

//  explicit operator std::string_view() const {
//    if constexpr (std::is_same_v<T, const char>) {
//      static_assert(
//          std::is_same_v<T, const char>,
//          "Only contiguous_view<const char> can be converted to std::string_view."
//      );
//    }
//    return std::string_view(reinterpret_cast<const char*>(begin()), static_cast<std::size_t>(this->size()));
//  }
  template <typename U = T, typename = std::enable_if_t<std::is_same_v<U, const char>>>
  explicit operator std::string_view() const {
    return std::string_view(reinterpret_cast<const char*>(begin()), this->size());
  }

private:
  iterator begin_;
};
