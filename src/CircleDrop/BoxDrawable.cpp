#include "BoxDrawable.h"

namespace Magnum {
namespace Examples {

BoxDrawable::BoxDrawable(Object2D &object,
                         Containers::Array<InstanceData> &instanceData,
                         const Color3 &color,
                         SceneGraph::DrawableGroup2D &drawables)
    : SceneGraph::Drawable2D{object, &drawables},
      _instanceData(instanceData), _color{color} {}

void BoxDrawable::draw(const Matrix3 &transformation, SceneGraph::Camera2D &) {
  arrayAppend(_instanceData, Containers::InPlaceInit, transformation, _color);
}

} // namespace Examples
} // namespace Magnum