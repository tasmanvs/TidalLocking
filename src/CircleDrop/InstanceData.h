#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/ConfigurationValue.h> // Needed so that magnum doesn't complain about incomplete Matrix3 Color3 type

namespace Magnum {
namespace Examples {

struct InstanceData {
  Matrix3 transformation;
  Color3 color;
};

} // namespace Examples
} // namespace Magnum
