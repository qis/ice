// #include <ice/error.hpp>
// #include <format>
// #include <cstdio>
//
// namespace one {
//
// enum class errc {
//   failure = 0,
//   unknown,
// };
//
// struct error_info final : std::error_category {
//   const char* name() const noexcept override
//   {
//     return "core";
//   }
//   std::string message(int code) const override
//   {
//     switch (static_cast<errc>(code)) {
//     case errc::failure:
//       return "failure";
//     case errc::unknown:
//       return "unknown";
//     }
//     return ice::format_error_code(code);
//   }
// };
//
// inline const error_info& error_category() noexcept
// {
//   static const error_info info;
//   return info;
// }
//
// inline std::error_code make_error_code(errc errc) noexcept
// {
//   return { static_cast<int>(errc), error_category() };
// }
//
// }  // namespace one
//
// int main()
// {
//   const auto e = ice::make_system_error(errno);
//
//   const std::string pack = ice::format_error(e);              // FFFFFFFE: 00000000 (0)
//   const std::string type = ice::format_error_type(e.type());  // FFFFFFFE
//   const std::string code = ice::format_error_code(e.code());  // 00000000 (0)
//
//   std::puts(ice::format_error(" " + pack + "\r").data());     // FFFFFFFE: 00000000 (0)
//   std::puts(ice::format_error(type, code).data());            // FFFFFFFE: 00000000 (0)
//
//   ice::load<std::errc>();
//
//   std::puts(ice::format_error("\t" + pack + "\n").data());    // system: Success
//   std::puts(ice::format_error(type, code).data());            // system: Success
//
//   ice::load<ice::errc>();
//   ice::load<one::errc>();
//
//   std::puts(ice::format_error(std::errc::no_link).data());    // generic: Link has been severed
//   std::puts(ice::format_error(one::errc::failure).data());    // core: failure
//   std::puts(ice::format_error(ice::errc::result).data());     // ice: result not initialized
// }

#pragma once
#include <ice/config.hpp>
#include <compare>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <cstdint>

namespace ice {

// clang-format off

template <class T, class V = std::remove_cvref_t<T>>
concept ErrorEnumType = requires(T) {
  requires(std::is_integral_v<V> && !std::is_same_v<V, bool>);
  requires(sizeof(V) <= sizeof(std::int32_t));
};

template <class T, class E = std::remove_cvref_t<T>>
concept ErrorEnum = requires(T) {
  requires(std::is_enum_v<E> && !std::is_convertible_v<E, std::underlying_type_t<E>>);
  requires(ErrorEnumType<std::underlying_type_t<E>>);
};

// clang-format on

enum class error_type : std::uint32_t { success = 0, system = 0xFFFFFFFE };

namespace detail {

constexpr error_type make_error_type(const char* name) noexcept
{
  std::uint32_t hash = 0x811C9DC5;
  while (*name) {
    hash = (static_cast<std::uint32_t>(*name++) ^ hash) * 0x01000193;
  }
  return static_cast<error_type>(hash);
}

template <ErrorEnum T>
constexpr error_type make_error_type() noexcept
{
  return make_error_type(ICE_FUNCTION);
}

}  // namespace detail

template <ErrorEnum E>
constexpr error_type make_error_type() noexcept
{
  constexpr auto type = detail::make_error_type<std::remove_cvref_t<E>>();
  static_assert(type != error_type::success, "reserved error type");
  static_assert(type != error_type::system, "reserved error type");
  return type;
}

class alignas(std::uint64_t) error {
public:
  constexpr error() noexcept = default;

  template <ErrorEnum E>
  constexpr error(E errc) noexcept :
    type_(make_error_type<E>()), code_(static_cast<std::uint32_t>(errc))
  {}

  constexpr error(error&& other) noexcept = default;
  constexpr error(const error& other) noexcept = default;
  constexpr error& operator=(error&& other) noexcept = default;
  constexpr error& operator=(const error& other) noexcept = default;

  constexpr ~error() = default;

  explicit constexpr operator bool() const noexcept
  {
    return type_ != error_type::success;
  }

  constexpr error_type type() const noexcept
  {
    return type_;
  }

  constexpr int code() const noexcept
  {
    return code_;
  }

  ICE_API std::string name() const;
  ICE_API std::string message() const;

  template <error_type T, ErrorEnumType V>
  friend constexpr error make_error(V code) noexcept;

  friend std::string format_error_type(error_type type);

  ICE_API friend std::string format_error(std::string_view pack);
  ICE_API friend std::string format_error(std::string_view type, std::string_view code);

  friend constexpr auto operator<=>(error, error) noexcept = default;

private:
  template <ErrorEnumType V>
  constexpr error(error_type type, V code) noexcept :
    type_(type), code_(static_cast<std::int32_t>(code))
  {}

  error_type type_{ error_type::success };
  std::int32_t code_{ 0 };
};

template <ErrorEnum E>
constexpr error make_error(E errc) noexcept
{
  return { errc };
}

template <ErrorEnum E, ErrorEnumType V>
constexpr error make_error(V code) noexcept
{
  return { static_cast<E>(code) };
}

template <error_type T, ErrorEnumType V>
constexpr error make_error(V code) noexcept
{
  return { T, static_cast<int>(code) };
}

constexpr error make_system_error(int code) noexcept
{
  return make_error<error_type::system>(code);
}

ICE_API std::string format_error_code(int code);

inline std::string format_error_type(error_type type)
{
  return error{ type, 0 }.name();
}

inline std::string format_error(ice::error e)
{
  return std::string{ e.name() } + ": " + e.message();
}

ICE_API std::string format_error(std::string_view pack);
ICE_API std::string format_error(std::string_view type, std::string_view code);

ICE_API bool load(error_type type, const std::error_category& category);

template <ErrorEnum E>
inline bool load(const std::error_category& category)
{
  return load(make_error_type<E>(), category);
}

template <ErrorEnum E>
inline bool load()
{
  return load(make_error_type<E>(), make_error_code(E{}).category());
}

enum class errc {
  result = 1,
};

struct error_info : std::error_category {
  ICE_API const char* name() const noexcept override;
  ICE_API std::string message(int code) const override;
};

ICE_API const std::error_category& error_category() noexcept;

inline std::error_code make_error_code(errc errc) noexcept
{
  return { static_cast<int>(errc), error_category() };
}

}  // namespace ice

template <class Char>
struct std::formatter<ice::error_type, Char> : std::formatter<std::string_view, Char> {
  template <class Context>
  auto format(ice::error_type t, Context& context)
  {
    return std::formatter<std::string_view, Char>::format(ice::format_error_type(t), context);
  }
};

template <class Char>
struct std::formatter<ice::error, Char> : std::formatter<std::string_view, Char> {
  template <class Context>
  auto format(ice::error e, Context& context)
  {
    return std::formatter<std::string_view, Char>::format(ice::format_error(e), context);
  }
};