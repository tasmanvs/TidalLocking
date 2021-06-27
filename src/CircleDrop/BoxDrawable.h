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

#include "InstanceData.h"
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

namespace Magnum {
namespace Examples {

typedef SceneGraph::Object<
    SceneGraph::TranslationRotationScalingTransformation2D>
    Object2D;
typedef SceneGraph::Scene<
    SceneGraph::TranslationRotationScalingTransformation2D>
    Scene2D;

class BoxDrawable : public SceneGraph::Drawable2D {

public:
  explicit BoxDrawable(Object2D &object,
                       Containers::Array<InstanceData> &instanceData,
                       const Color3 &color,
                       SceneGraph::DrawableGroup2D &drawables);

private:
  void draw(const Matrix3 &transformation, SceneGraph::Camera2D &) override;

  Containers::Array<InstanceData> &_instanceData;
  Color3 _color;
};

} // namespace Examples
} // namespace Magnum