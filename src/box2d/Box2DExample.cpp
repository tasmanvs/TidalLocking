/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
             — Vladimír Vondruš <mosra@centrum.cz>
        2018 — Michal Mikula <miso.mikula@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/ConfigurationValue.h>
#include <Magnum/Math/DualComplex.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/TranslationRotationScalingTransformation2D.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/MeshData.h>

/* Box2D 2.3 (from 2014) uses mixed case, 2.4 (from 2020) uses lowercase */
#ifdef __has_include
#if __has_include(<box2d/box2d.h>)
#include <box2d/box2d.h>
#else
#include <Box2D/Box2D.h>
#define IT_IS_THE_OLD_BOX2D
#endif
/* If the compiler doesn't have __has_include, assume it's extremely old, and
   thus an extremely old Box2D is more likely as well */
#else
#include <Box2D/Box2D.h>
#define IT_IS_THE_OLD_BOX2D
#endif

namespace Magnum {
namespace Examples {

typedef SceneGraph::Object<
    SceneGraph::TranslationRotationScalingTransformation2D>
    Object2D;
typedef SceneGraph::Scene<
    SceneGraph::TranslationRotationScalingTransformation2D>
    Scene2D;

using namespace Math::Literals;

struct InstanceData {
  Matrix3 transformation;
  Color3 color;
};

class Box2DExample : public Platform::Application {
public:
  explicit Box2DExample(const Arguments &arguments);

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

  b2Body *createBody(Object2D &object, const Vector2 &size, b2BodyType type,
                     const DualComplex &transformation, Float density = 1.0f);

  GL::Mesh _mesh{NoCreate};
  GL::Buffer _instanceBuffer{NoCreate};
  Shaders::Flat2D _shader{NoCreate};
  Containers::Array<InstanceData> _instanceData;

  Scene2D _scene;
  Object2D *_cameraObject;
  SceneGraph::Camera2D *_camera;
  SceneGraph::DrawableGroup2D _drawables;
  Containers::Optional<b2World> _world;
  ImGuiIntegration::Context imgui_context_{NoCreate};
};

class BoxDrawable : public SceneGraph::Drawable2D {
public:
  explicit BoxDrawable(Object2D &object,
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

b2Body *Box2DExample::createBody(Object2D &object, const Vector2 &halfSize,
                                 const b2BodyType type,
                                 const DualComplex &transformation,
                                 const Float density) {
  b2BodyDef bodyDefinition;
  bodyDefinition.position.Set(transformation.translation().x(),
                              transformation.translation().y());
  bodyDefinition.angle = Float(transformation.rotation().angle());
  bodyDefinition.type = type;
  b2Body *body = _world->CreateBody(&bodyDefinition);

  b2PolygonShape shape;
  shape.SetAsBox(halfSize.x(), halfSize.y());

  b2FixtureDef fixture;
  fixture.friction = 0.8f;
  fixture.density = density;
  fixture.shape = &shape;
  body->CreateFixture(&fixture);

  /* Why keep things simple if there's an awful and backwards-incompatible
     way, eh? https://github.com/erincatto/box2d/pull/658 */
  body->GetUserData().pointer = reinterpret_cast<std::uintptr_t>(&object);

  object.setScaling(halfSize);

  return body;
}

Box2DExample::Box2DExample(const Arguments &arguments)
    : Platform::Application{arguments, NoCreate} {

  /* Make it possible for the user to have some fun */
  Utility::Arguments args;
  args.addOption("transformation", "1 0 0 0")
      .setHelp("transformation", "initial pyramid transformation")
      .addSkippedPrefix("magnum", "engine-specific options")
      .parse(arguments.argc, arguments.argv);

  const DualComplex globalTransformation =
      args.value<DualComplex>("transformation").normalized();

  /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
     MSAA if we have enough DPI. */
  {
    const Vector2 dpiScaling = this->dpiScaling({});
    Configuration conf;
    conf.setTitle("Magnum Box2D Example").setSize(conf.size(), dpiScaling);
    GLConfiguration glConf;
    glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
    if (!tryCreate(conf, glConf))
      create(conf, glConf.setSampleCount(0));
  }
  /* Setup ImGui, load a better font */
  {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    const Vector2 size = Vector2{windowSize()} / dpiScaling();
    Utility::Resource rs{"data"};
    Containers::ArrayView<const char> font =
        rs.getRaw("SourceSansPro-Regular.ttf");
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<char *>(font.data()), Int(font.size()),
        16.0f * framebufferSize().x() / size.x(), &fontConfig);

    imgui_context_ = ImGuiIntegration::Context{
        *ImGui::GetCurrentContext(), Vector2{windowSize()} / dpiScaling(),
        windowSize(), framebufferSize()};

    /* Setup proper blending to be used by ImGui */
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  }

  /* Configure camera */
  _cameraObject = new Object2D{&_scene};
  _camera = new SceneGraph::Camera2D{*_cameraObject};
  _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
      .setProjectionMatrix(Matrix3::projection({20.0f, 20.0f}))
      .setViewport(GL::defaultFramebuffer.viewport().size());

  /* Create the Box2D world with the usual gravity vector */
  _world.emplace(b2Vec2{0.0f, -9.81f});

  /* Create an instanced shader */
  _shader = Shaders::Flat2D{Shaders::Flat2D::Flag::VertexColor |
                            Shaders::Flat2D::Flag::InstancedTransformation};

  /* Box mesh with an (initially empty) instance buffer */
  _mesh = MeshTools::compile(Primitives::squareSolid());
  _instanceBuffer = GL::Buffer{};
  _mesh.addVertexBufferInstanced(_instanceBuffer, 1, 0,
                                 Shaders::Flat2D::TransformationMatrix{},
                                 Shaders::Flat2D::Color3{});

  /* Create the ground */
  auto ground = new Object2D{&_scene};
  createBody(*ground, {11.0f, 0.5f}, b2_staticBody,
             DualComplex::translation(Vector2::yAxis(-8.0f)));
  new BoxDrawable{*ground, _instanceData, 0xa5c9ea_rgbf, _drawables};

  /* Create a pyramid of boxes */
  const auto pyramid_height = 30;
  for (std::size_t row = 0; row != pyramid_height; ++row) {
    for (std::size_t item = 0; item != pyramid_height - row; ++item) {
      auto box = new Object2D{&_scene};
      const DualComplex transformation =
          globalTransformation *
          DualComplex::translation(
              {Float(row) * 0.6f + Float(item) * 1.2f - 8.5f,
               Float(row) * 1.0f - 6.0f});
      createBody(*box, {0.5f, 0.5f}, b2_dynamicBody, transformation);
      new BoxDrawable{*box, _instanceData, 0x2f83cc_rgbf, _drawables};
    }
  }

  setSwapInterval(1);
#if !defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(CORRADE_TARGET_ANDROID)
  setMinimalLoopPeriod(16);
#endif
}

// Working on getting imgui events in. Need to do some find and replace, We
// currently have 2x mousePressEvents.
void Box2DExample::box2d_mouse_press_event(MouseEvent &event) {
  if (event.isAccepted()) {
    return;
  }

  if (event.button() != MouseEvent::Button::Left)
    return;

  /* Calculate mouse position in the Box2D world. Make it relative to window,
     with origin at center and then scale to world size with Y inverted. */
  const auto position =
      _camera->projectionSize() * Vector2::yScale(-1.0f) *
      (Vector2{event.position()} / Vector2{windowSize()} - Vector2{0.5f});

  auto destroyer = new Object2D{&_scene};
  createBody(*destroyer, {0.5f, 0.5f}, b2_dynamicBody,
             DualComplex::translation(position), 2.0f);
  new BoxDrawable{*destroyer, _instanceData, 0xffff66_rgbf, _drawables};

  auto offset = DualComplex::translation({0.0f, -2.0f});
  auto position_2 = DualComplex::translation(position) * offset;

  auto destroyer2 = new Object2D{&_scene};
  createBody(*destroyer2, {1.5f, 0.5f}, b2_dynamicBody, position_2, 2.0f);
  new BoxDrawable{*destroyer2, _instanceData, 0xff0000_rgbf, _drawables};

  event.setAccepted();
}

void Box2DExample::imgui_mouse_press_event(MouseEvent &event) {
  if (event.isAccepted()) {
    return;
  }

  if (imgui_context_.handleMousePressEvent(event)) {
    event.setAccepted(true);
    return;
  }
}

void Box2DExample::mousePressEvent(MouseEvent &event) {
  imgui_mouse_press_event(event);
  box2d_mouse_press_event(event);
}

void Box2DExample::draw_event_box2d() {
  /* Step the world and update all object positions */
  _world->Step(1.0f / 60.0f, 6, 2);
  for (b2Body *body = _world->GetBodyList(); body; body = body->GetNext()) {
#ifndef IT_IS_THE_OLD_BOX2D
    /* Why keep things simple if there's an awful backwards-incompatible
       way, eh? https://github.com/erincatto/box2d/pull/658 */
    (*reinterpret_cast<Object2D *>(body->GetUserData().pointer))
#else
    (*static_cast<Object2D *>(body->GetUserData()))
#endif
        .setTranslation({body->GetPosition().x, body->GetPosition().y})
        .setRotation(Complex::rotation(Rad(body->GetAngle())));
  }

  /* Populate instance data with transformations and colors */
  arrayResize(_instanceData, 0);
  _camera->draw(_drawables);

  /* Upload instance data to the GPU and draw everything in a single call */
  _instanceBuffer.setData(_instanceData, GL::BufferUsage::DynamicDraw);
  _mesh.setInstanceCount(_instanceData.size());
  _shader.setTransformationProjectionMatrix(_camera->projectionMatrix())
      .draw(_mesh);
}

void Box2DExample::draw_event_imgui() {
  imgui_context_.newFrame();

  /* Enable text input, if needed */
  if (ImGui::GetIO().WantTextInput && !isTextInputActive())
    startTextInput();
  else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
    stopTextInput();

  // ImGui::Text("Hello, world!");
  ImGui::ShowDemoWindow();

  GL::Renderer::enable(GL::Renderer::Feature::Blending);
  GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
  GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
  GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);

  imgui_context_.drawFrame();

  GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
  GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
  GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
  GL::Renderer::disable(GL::Renderer::Feature::Blending);
}

void Box2DExample::drawEvent() {
  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                               GL::FramebufferClear::Depth);
  draw_event_box2d();
  draw_event_imgui();

  swapBuffers();
  redraw();
}

void Box2DExample::viewportEvent(ViewportEvent &event) {
  /* Resize the main framebuffer */
  GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

  /* Relayout ImGui */
  imgui_context_.relayout(Vector2{event.windowSize()} / event.dpiScaling(),
                          event.windowSize(), event.framebufferSize());

  /* Recompute the camera's projection matrix */
  _camera->setViewport(event.framebufferSize());
}

void Box2DExample::keyPressEvent(KeyEvent &event) {
  switch (event.key()) {
  default:
    if (imgui_context_.handleKeyPressEvent(event))
      event.setAccepted(true);
  }
}

void Box2DExample::keyReleaseEvent(KeyEvent &event) {
  if (imgui_context_.handleKeyReleaseEvent(event)) {
    event.setAccepted(true);
    return;
  }
}

void Box2DExample::mouseReleaseEvent(MouseEvent &event) {
  if (imgui_context_.handleMouseReleaseEvent(event))
    event.setAccepted(true);
}

void Box2DExample::mouseMoveEvent(MouseMoveEvent &event) {
  if (imgui_context_.handleMouseMoveEvent(event)) {
    event.setAccepted(true);
    return;
  }

  if (!event.buttons())
    return;
}

void Box2DExample::mouseScrollEvent(MouseScrollEvent &event) {
  const Float delta = event.offset().y();
  if (Math::abs(delta) < 1.0e-2f)
    return;

  if (imgui_context_.handleMouseScrollEvent(event)) {
    /* Prevent scrolling the page */
    event.setAccepted();
    return;
  }
}

void Box2DExample::textInputEvent(TextInputEvent &event) {
  if (imgui_context_.handleTextInputEvent(event))
    event.setAccepted(true);
}

} // namespace Examples
} // namespace Magnum

MAGNUM_APPLICATION_MAIN(Magnum::Examples::Box2DExample)
