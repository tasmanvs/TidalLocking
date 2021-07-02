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

enum class ShapeType { Circle, Box };

typedef SceneGraph::Object<
    SceneGraph::TranslationRotationScalingTransformation2D>
    Object2D;
typedef SceneGraph::Scene<
    SceneGraph::TranslationRotationScalingTransformation2D>
    Scene2D;

using namespace Math::Literals;

class CircleDrop : public Platform::Application {
public:
  explicit CircleDrop(const Arguments &arguments);
  void create_pyramid();

private:
  void drawEvent() override;
  void draw_event_box2d();
  void draw_event_imgui();

  void mousePressEvent(MouseEvent &event) override;
  void viewportEvent(ViewportEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;
  void mouseScrollEvent(MouseScrollEvent &event) override;
  void textInputEvent(TextInputEvent &event) override;

  void imgui_mouse_press_event(MouseEvent &event);
  void box2d_mouse_press_event(MouseEvent &event);

  void init_imgui();

  b2Body *createBody(Object2D &object, const Vector2 &size, b2BodyType type,
                     const DualComplex &transformation, Float density = 1.0f,
                     const ShapeType shape_type = ShapeType::Box);

  GL::Mesh _mesh{NoCreate};
  GL::Mesh _circle_mesh{NoCreate};
  GL::Buffer _boxInstanceBuffer{NoCreate};
  GL::Buffer _circleInstanceBuffer{NoCreate};

  Shaders::Flat2D _shader{NoCreate};
  Containers::Array<InstanceData> _boxInstanceData;
  Containers::Array<InstanceData> _circleInstanceData;

  Scene2D _scene;
  Object2D *_cameraObject;
  SceneGraph::Camera2D *_camera;
  SceneGraph::DrawableGroup2D _drawables;
  Containers::Optional<b2World> _world;
  ImGuiIntegration::Context imgui_context_{NoCreate};

  DualComplex global_transform_;
  float radius_{1.f};
  int height_{30};
};




} // namespace Examples
} // namespace Magnum