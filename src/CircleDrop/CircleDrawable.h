
#pragma once

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/Shaders/Flat.h>
#include <box2d/b2_body.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_collision.h>
#include <box2d/b2_shape.h>
#include <box2d/box2d.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Tags.h>
#include <Corrade/Utility/Arguments.h>

#include <SDL_video.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/MeshView.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Magnum.h>
#include <Magnum/Math/ConfigurationValue.h>
#include <Magnum/Math/DualComplex.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation2D.h>
#include <Magnum/Trade/MeshData.h>

#include "InstanceData.h"


namespace Magnum {
namespace Examples {

class CircleDrawable : public SceneGraph::Drawable2D {
public:
  explicit CircleDrawable(Object2D &object,
                          Containers::Array<InstanceData> &instanceData,
                          const Color3 &color,
                          SceneGraph::DrawableGroup2D &drawables)
      : SceneGraph::Drawable2D{object, &drawables},
        _instanceData(instanceData), _color{color} {}

private:
  void draw(const Matrix3 &transformation, SceneGraph::Camera2D &) override {
    arrayAppend(_instanceData, Containers::InPlaceInit, transformation, _color);
  }

  Containers::Array<InstanceData> &_instanceData;
  Color3 _color;
};

} // namespace Examples
} // namespace Magnum