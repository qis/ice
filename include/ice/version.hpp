#pragma once
#include <ice/config.hpp>
#include <compare>
#include <limits>
#include <string_view>
#include <cstdint>

namespace ice {

class version {
public:
  constexpr version() noexcept = default;
  constexpr version(std::string_view sv) noexcept : value_(parse(sv).value()) {}

  // clang-format off

  constexpr version(std::uint16_t major, std::uint16_t minor = 0, std::uint32_t patch = 0) noexcept :
    value_((std::uint64_t{ major } << 48 & 0xFFFF000000000000) |
           (std::uint64_t{ minor } << 32 & 0x0000FFFF00000000) |
           (std::uint64_t{ patch }       & 0x00000000FFFFFFFF))
  {}

  // clang-format on

  constexpr std::uint16_t major() const noexcept
  {
    return static_cast<std::uint16_t>(value_ >> 48 & 0xFFFF);
  }

  constexpr std::uint16_t minor() const noexcept
  {
    return static_cast<std::uint16_t>(value_ >> 32 & 0xFFFF);
  }

  constexpr std::uint32_t patch() const noexcept
  {
    return static_cast<std::uint32_t>(value_ & 0xFFFFFFFF);
  }

  constexpr std::uint64_t value() const noexcept
  {
    return value_;
  }

  friend constexpr auto operator<=>(version, version) noexcept = default;

private:
  static constexpr version parse(std::string_view sv) noexcept
  {
    constexpr auto mm = std::numeric_limits<std::uint16_t>::max() / 10;
    constexpr auto pm = std::numeric_limits<std::uint32_t>::max() / 10;

    auto it = sv.cbegin();
    const auto end = sv.cend();

    std::uint16_t major = 0;
    while (it != end && *it >= '0' && *it <= '9') {
      if (major > mm) {
        return { 0xFFFF };
      }
      major *= 10;
      major += *it - '0';
      ++it;
    }
    if (*it == '.') {
      ++it;
    }

    std::uint16_t minor = 0;
    while (it != end && *it >= '0' && *it <= '9') {
      if (minor > mm) {
        return { major, 0xFFFF };
      }
      minor *= 10;
      minor += *it - '0';
      ++it;
    }
    if (*it == '.') {
      ++it;
    }

    std::uint32_t patch = 0;
    while (it != end && *it >= '0' && *it <= '9') {
      if (patch > pm) {
        return { major, minor, 0xFFFFFFFF };
      }
      patch *= 10;
      patch += *it - '0';
      ++it;
    }
    if (*it == '.') {
      ++it;
    }

    return { major, minor, patch };
  }

  std::uint64_t value_;
};

namespace detail {

ICE_API std::string_view format_version(version v, char* data, std::size_t size);

}  // namespace detail

ICE_API version runtime_version() noexcept;

}  // namespace ice

template <class Char>
struct std::formatter<ice::version, Char> : std::formatter<std::string_view, Char> {
  template <class Context>
  auto format(ice::version v, Context& context)
  {
    // clang-format off
    constexpr auto size =
      std::numeric_limits<decltype(v.major())>::digits10 + 1 +
      std::numeric_limits<decltype(v.minor())>::digits10 + 1 +
      std::numeric_limits<decltype(v.patch())>::digits10;
    // clang-format on
    Char data[size + 1]{};
    const auto sv = ice::detail::format_version(v, data, size);
    return std::formatter<std::string_view, Char>::format(sv, context);
  }
};