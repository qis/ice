#include "ice/version.hpp"

namespace ice {

version runtime_version() noexcept
{
  return { ICE_VERSION_MAJOR, ICE_VERSION_MINOR, ICE_VERSION_PATCH };
}

}  // namespace ice