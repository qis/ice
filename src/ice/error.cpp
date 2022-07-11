#include "ice/error.hpp"
#include <atomic>
#include <charconv>
#include <map>
#include <memory>
#include <mutex>

static_assert(sizeof(std::int32_t) <= sizeof(int), "unsupported int size");

namespace ice {
namespace {

using index = std::map<error_type, const std::error_category*>;

#ifdef __cpp_lib_atomic_shared_ptr
std::atomic<std::shared_ptr<index>> categories;
#else
std::shared_ptr<index> categories;
#endif

std::mutex mutex;

const std::error_category* get(error_type type) noexcept
{
#ifdef __cpp_lib_atomic_shared_ptr
  if (const auto ptr = categories.load(std::memory_order_relaxed))
#else
  if (const auto ptr = std::atomic_load(&categories))
#endif
  {
    if (const auto it = ptr->find(type); it != ptr->end()) {
      return it->second;
    }
  }
  return nullptr;
}

bool parse_value(std::string_view s, std::uint32_t& value) noexcept
{
  if (s.size() < 8) {
    return false;
  }
  s = s.substr(0, 8);
  if (s.find_first_not_of("0123456789ABCDEF") != std::string::npos) {
    return false;
  }
  std::from_chars(s.data(), s.data() + 8, value, 16);
  return true;
}

struct success_info final : std::error_category {
  const char* name() const noexcept override
  {
    return "success";
  }
  std::string message(int code) const override
  {
    return format_error_code(code);
  }
};

const std::error_category& success_category() noexcept
{
  static const success_info info;
  return info;
}

}  // namespace

const char* error_info::name() const noexcept
{
  return "ice";
}

std::string error_info::message(int code) const
{
  switch (const auto e = static_cast<errc>(code)) {
  case errc::result:
    return "result not initialized";
  }
  return format_error_code(code);
}

const std::error_category& error_category() noexcept
{
  static const error_info info;
  return info;
}

std::string error::name() const
{
  if (const auto category = get(type_)) {
    return category->name();
  }
  return std::format("{:08X}", static_cast<std::uint32_t>(type_));
}

std::string error::message() const
{
  const auto code = static_cast<int>(code_);
  if (const auto category = get(type_)) {
    return category->message(code);
  }
  return format_error_code(code);
}

std::string format_error(std::string_view pack)
{
  if (const auto pos = pack.find_first_not_of(" \f\n\r\t\v"); pos != 0) {
    pack = pack.substr(pos);
  }
  if (pack.size() < 18) {
    return std::string{ pack };
  }
  if (const auto pos = pack.find_first_of("\f\n\r\t\v"); pos != std::string::npos) {
    pack = pack.substr(0, pos);
  }
  if (const auto pos = pack.find_last_not_of(' '); pos != std::string::npos) {
    pack = pack.substr(0, pos + 1);
  }
  if (pack.size() < 18) {
    return std::string{ pack };
  }
  const auto pack_sv = std::string_view{ pack };

  std::uint32_t type_value = 0;
  const auto type_sv = pack_sv.substr(0, 8);
  if (!parse_value(type_sv, type_value)) {
    return std::string{ pack };
  }
  const auto type = static_cast<error_type>(type_value);

  if (pack_sv.substr(8, 2) != ": ") {
    return std::string{ pack };
  }

  std::uint32_t code_value = 0;
  const auto code_sv = pack_sv.substr(10, 8);
  if (!parse_value(code_sv, code_value)) {
    return format_error_type(type) + ": " + std::string{ pack_sv.substr(10) };
  }
  const auto code = static_cast<int>(code_value);

  return format_error(error{ type, code });
}

std::string format_error(std::string_view type, std::string_view code)
{
  if (std::uint32_t type_value = 0; parse_value(type, type_value)) {
    if (std::uint32_t code_value = 0; parse_value(code, code_value)) {
      return format_error(error{ static_cast<error_type>(type_value), static_cast<int>(code_value) });
    }
    return format_error_type(static_cast<error_type>(type_value)) + ": " + std::string{ code };
  }
  return std::string{ type } + ": " + std::string{ code };
}

bool load(error_type type, const std::error_category& category)
{
  std::lock_guard lock{ mutex };
#ifdef __cpp_lib_atomic_shared_ptr
  auto ptr = categories.load(std::memory_order_acquire);
#else
  auto ptr = std::atomic_load(&categories);
#endif
  if (!ptr) {
    ptr = std::make_shared<index>();
    ptr->emplace(error_type::success, &success_category());
    ptr->emplace(error_type::system, &std::system_category());
  }
  if (!ptr->emplace(type, &category).second) {
    return false;
  }
#ifdef __cpp_lib_atomic_shared_ptr
  categories.store(std::move(ptr), std::memory_order_release);
#else
  std::atomic_store(&categories, std::move(ptr));
#endif
  return true;
}

}  // namespace ice
