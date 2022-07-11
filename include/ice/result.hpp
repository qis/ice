#pragma once
#include <ice/error.hpp>

namespace ice {

template <class T>
struct is_result : std::false_type {};

template <class T>
constexpr bool is_result_v = is_result<T>::value;

template <class T, class V = std::remove_cvref_t<T>>
concept ResultType = requires()
{
  // clang-format off
  requires(
    std::is_void_v<V> || (
      !std::is_const_v<V> &&
      !std::is_reference_v<V> &&
      !std::is_same_v<V, ice::error> &&
      !ice::is_result_v<V>
    )
  );
  // clang-format on
};

template <ResultType T = void>
class result final {
public:
  constexpr result() noexcept {}

  constexpr result(ice::error error) noexcept :
    error_(error ? error : ice::errc::result)
  {}

  template <class Arg, class... Args>
  constexpr result(Arg&& arg, Args&&... args)
    noexcept(std::is_nothrow_constructible_v<T, Arg, Args...>)
    requires(std::is_constructible_v<T, Arg, Args...> &&
             !std::is_constructible_v<ice::error, Arg, Args...>) :
    error_(), value_(std::forward<Arg>(arg), std::forward<Args>(args)...)
  {}

  // clang-format off

  //template <class Arg, class... Args>
  //constexpr result(Arg&& arg, Args&&... args)
  //  noexcept(std::is_nothrow_constructible_v<T, Arg, Args...>)
  //  requires(std::is_constructible_v<T, Arg, Args...> &&
  //           !std::is_constructible_v<ice::error, Arg, Args...>) :
  //  error_(), value_(std::forward<Arg>(arg), std::forward<Args>(args)...)
  //{}

  constexpr result(result&& other)
    noexcept(std::is_nothrow_move_constructible_v<T>)
    requires(std::is_move_constructible_v<T>) :
    error_(other.error_)
  {
    if (!error_) {
      new (&value_) T{ std::move(other.value_) };
    }
  }
  
  constexpr result(result&& other)
    requires(!std::is_move_constructible_v<T>) = delete;

  constexpr result(const result& other)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires(std::is_copy_constructible_v<T>) :
    error_(other.error_)
  {
    if (!error_) {
      new (&value_) T{ other.value_ };
    }
  }
  
  constexpr result(const result& other)
    requires(!std::is_copy_constructible_v<T>) = delete;

  constexpr result& operator=(result&& other)
    noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T>)
    requires(std::is_move_assignable_v<T>)
  {
    if (!error_) {
      value_.~T();
    }
    error_ = other.error_;
    if (!error_) {
      new (&value_) T{ std::move(other.value_) };
    }
    return *this;
  }
  
  constexpr result& operator=(result&& other)
    requires(!std::is_move_assignable_v<T>) = delete;

  constexpr result& operator=(const result& other)
    noexcept(std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_destructible_v<T>)
    requires(std::is_copy_assignable_v<T>)
  {
    if (!error_) {
      value_.~T();
    }
    error_ = other.error_;
    if (!error_) {
      new (&value_) T{ other.value_ };
    }
    return *this;
  }
  
  constexpr result& operator=(const result& other)
    requires(!std::is_copy_assignable_v<T>) = delete;

  constexpr result& operator=(ice::error error)
    noexcept(std::is_nothrow_destructible_v<T>)
  {
    if (!error_) {
      value_.~T();
    }
    error_ = error ? error : ice::errc::result;
    return *this;
  }

  // clang-format on

  constexpr ~result() noexcept(std::is_nothrow_destructible_v<T>)
  {
    if (!error_) {
      value_.~T();
    }
  }

  explicit constexpr operator bool() const noexcept
  {
    return !error_;
  }

  constexpr ice::error error() const noexcept
  {
    return error_;
  }

  constexpr const T& value() const& noexcept
  {
    ICE_ASSERT(!error_);
    return value_;
  }

  constexpr T& value() & noexcept
  {
    ICE_ASSERT(!error_);
    return value_;
  }

  constexpr T&& value() && noexcept
  {
    ICE_ASSERT(!error_);
    return std::move(value_);
  }

private:
  ice::error error_{ ice::errc::result };
  union {
    T value_;
  };
};

template <>
class result<void> {
public:
  constexpr result() noexcept {}

  constexpr result(ice::error error) noexcept :
    error_(error ? error : ice::errc::result)
  {}

  constexpr result(result&& other) noexcept = default;
  constexpr result(const result& other) noexcept = default;
  constexpr result& operator=(result&& other) noexcept = default;
  constexpr result& operator=(const result& other) noexcept = default;

  constexpr result& operator=(ice::error error) noexcept
  {
    error_ = error ? error : ice::errc::result;
    return *this;
  }

  constexpr ~result() = default;

  explicit constexpr operator bool() const noexcept
  {
    return !error_;
  }

  constexpr ice::error error() const noexcept
  {
    return error_;
  }

  constexpr void value() const noexcept
  {
    ICE_ASSERT(!error_);
  }

private:
  ice::error error_{};
};

template <class T>
struct is_result<ice::result<T>> : std::true_type {};

}  // namespace ice

template <class T>
struct std::formatter<ice::result<T>> : std::formatter<T> {
  template <class FormatContext>
  auto format(const ice::result<T>& result, FormatContext& context)
  {
    if (result) {
      return std::formatter<T>::format(result.value(), context);
    }
    return std::format_to(context.out(), "{}", result.error());
  }
};

template <>
struct std::formatter<ice::result<void>> : std::formatter<std::string_view> {
  template <class FormatContext>
  auto format(const ice::result<void>& result, FormatContext& context)
  {
    if (result) {
      return std::formatter<std::string_view>::format("void", context);
    }
    return std::format_to(context.out(), "{}", result.error());
  }
};
