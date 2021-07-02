

#include <imgui.h>
#include <iostream>

#include "CircleDrop.h"
#include "CircleDrawable.h"
#include "BoxDrawable.h"

namespace Magnum {
namespace Examples {

b2Body *
CircleDrop::createBody(Object2D &object, const Vector2 &halfSize,
                       const b2BodyType type, const DualComplex &transformation,
                       const Float density,
                       const ShapeType shape_type /* = ShapeType::Box*/) {
  b2BodyDef bodyDefinition;
  bodyDefinition.position.Set(transformation.translation().x(),
                              transformation.translation().y());
  bodyDefinition.angle = Float(transformation.rotation().angle());
  bodyDefinition.type = type;
  b2Body *body = _world->CreateBody(&bodyDefinition);

  if (shape_type == ShapeType::Box) {
    b2FixtureDef fixture;
    b2PolygonShape shape;
    fixture.friction = 0.8f;
    fixture.density = density;
    fixture.shape = &shape;
    shape.SetAsBox(halfSize.x(), halfSize.y());
    body->CreateFixture(&fixture);

  } else if (shape_type == ShapeType::Circle) {
    b2FixtureDef fixture;
    b2CircleShape shape;
    fixture.friction = 0.8f;
    fixture.density = density;
    fixture.shape = &shape;
    shape.m_radius = radius_;
    body->CreateFixture(&fixture);
  }

  /* Why keep things simple if there's an awful and backwards-incompatible
     way, eh? https://github.com/erincatto/box2d/pull/658 */
  body->GetUserData().pointer = reinterpret_cast<std::uintptr_t>(&object);

  object.setScaling(halfSize);

  return body;
}

CircleDrop::CircleDrop(const Arguments &arguments)
    : Platform::Application{arguments, NoCreate} {

  /* Make it possible for the user to have some fun */
  Utility::Arguments args;
  args.addOption("transformation", "1 0 0 0")
      .setHelp("transformation", "initial pyramid transformation")
      .addSkippedPrefix("magnum", "engine-specific options")
      .parse(arguments.argc, arguments.argv);

  global_transform_ = args.value<DualComplex>("transformation").normalized();

  /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
     MSAA if we have enough DPI. */
  {
    const Vector2 dpiScaling = this->dpiScaling({});
    Configuration conf;
    conf.setTitle("Circle Drop").setSize(conf.size(), dpiScaling);

    conf.addWindowFlags(Configuration::WindowFlag::Resizable);
    GLConfiguration glConf;
    glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
    if (!tryCreate(conf, glConf))
      create(conf, glConf.setSampleCount(0));
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
  _circle_mesh = MeshTools::compile(Primitives::circle2DSolid(10U, {}));

  _boxInstanceBuffer = GL::Buffer{};
  _mesh.addVertexBufferInstanced(_boxInstanceBuffer, 1, 0,
                                 Shaders::Flat2D::TransformationMatrix{},
                                 Shaders::Flat2D::Color3{});

  _circleInstanceBuffer = GL::Buffer{};
  _circle_mesh.addVertexBufferInstanced(_circleInstanceBuffer, 1, 0,
                                        Shaders::Flat2D::TransformationMatrix{},
                                        Shaders::Flat2D::Color3{});

  /* Create the ground */
  auto ground = new Object2D{&_scene};
  createBody(*ground, {11.0f, 0.5f}, b2_staticBody,
             DualComplex::translation(Vector2::yAxis(-8.0f)));
  new BoxDrawable{*ground, _boxInstanceData, 0xa5c9ea_rgbf, _drawables};

  create_pyramid();

  setSwapInterval(1);
  setMinimalLoopPeriod(16);

  init_imgui();
}

void CircleDrop::create_pyramid() {
  /* Create a pyramid of circles */
  const auto pyramid_height = static_cast<size_t>(height_);
  for (std::size_t row = 0; row != pyramid_height; ++row) {
    for (std::size_t item = 0; item != pyramid_height - row; ++item) {
      auto circle = new Object2D{&_scene};
      const DualComplex transformation =
          global_transform_ *
          DualComplex::translation(
              {Float(row) * radius_ + 0.1f + Float(item) * 1.2f - 8.5f,
               Float(row) * 1.0f - 6.0f});
      createBody(*circle, {radius_, radius_}, b2_dynamicBody, transformation,
                 1.f, ShapeType::Circle);
      new CircleDrawable{*circle, _circleInstanceData, 0x2f83aa_rgbf,
                         _drawables};
    }
  }
}

void CircleDrop::box2d_mouse_press_event(MouseEvent &event) {
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
  createBody(*destroyer, {radius_, radius_}, b2_dynamicBody,
             DualComplex::translation(position), 1.f, ShapeType::Circle);
  new CircleDrawable{*destroyer, _circleInstanceData, 0xffff66_rgbf,
                     _drawables};

  auto offset = DualComplex::translation({0.0f, -2.0f});
  auto position_2 = DualComplex::translation(position) * offset;

  auto destroyer2 = new Object2D{&_scene};
  createBody(*destroyer2, {1.5f, 0.5f}, b2_dynamicBody, position_2, 2.0f);
  new BoxDrawable{*destroyer2, _boxInstanceData, 0xff0000_rgbf, _drawables};

  event.setAccepted();
}

void CircleDrop::imgui_mouse_press_event(MouseEvent &event) {
  if (event.isAccepted()) {
    return;
  }

  if (imgui_context_.handleMousePressEvent(event)) {
    event.setAccepted(true);
    return;
  }
}

void CircleDrop::mousePressEvent(MouseEvent &event) {
  imgui_mouse_press_event(event);
  box2d_mouse_press_event(event);
}

void CircleDrop::draw_event_box2d() {
  /* Step the world and update all object positions */
  _world->Step(1.0f / 60.0f, 6, 2);
  for (b2Body *body = _world->GetBodyList(); body; body = body->GetNext()) {

    if (body->GetPosition().y < -50) {
      // Clearing the features means that they don't get drawn. Otherwise the
      // objects stay stationary.
      (*reinterpret_cast<Object2D *>(body->GetUserData().pointer))
          .features()
          .clear();
      _world->DestroyBody(body);
      continue;
    }
    /* Why keep things simple if there's an awful backwards-incompatible
       way, eh? https://github.com/erincatto/box2d/pull/658 */
    (*reinterpret_cast<Object2D *>(body->GetUserData().pointer))

        .setTranslation({body->GetPosition().x, body->GetPosition().y})
        .setRotation(Complex::rotation(Rad(body->GetAngle())));
  }

  /* Populate instance data with transformations and colors */
  arrayResize(_boxInstanceData, 0);
  arrayResize(_circleInstanceData, 0);
  _camera->draw(_drawables);

  /* Upload instance data to the GPU and draw everything in a single call */
  _boxInstanceBuffer.setData(_boxInstanceData, GL::BufferUsage::DynamicDraw);
  _mesh.setInstanceCount(_boxInstanceData.size());
  _shader.setTransformationProjectionMatrix(_camera->projectionMatrix());
  _shader.draw(_mesh);

  _circleInstanceBuffer.setData(_circleInstanceData,
                                GL::BufferUsage::DynamicDraw);
  _circle_mesh.setInstanceCount(_circleInstanceData.size());
  _shader.draw(_circle_mesh);
}

void CircleDrop::draw_event_imgui() {
  imgui_context_.newFrame();

  /* Enable text input, if needed */
  if (ImGui::GetIO().WantTextInput && !isTextInputActive())
    startTextInput();
  else if (!ImGui::GetIO().WantTextInput && isTextInputActive())
    stopTextInput();

  if (ImGui::Button("Create Pyramid")) {
    create_pyramid();
  }

  ImGui::SliderFloat("Circle Radius", &radius_, 0.f, 10.f);
  ImGui::SliderInt("Pyramid Height", &height_, 1, 100);

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

void CircleDrop::drawEvent() {
  GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                               GL::FramebufferClear::Depth);
  draw_event_box2d();
  draw_event_imgui();

  swapBuffers();
  redraw();
}

void CircleDrop::viewportEvent(ViewportEvent &event) {

  /* Resize the main framebuffer */
  GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

  /* Recompute the camera's projection matrix */
  _camera->setViewport(event.framebufferSize());

  /* Relayout ImGui */
  imgui_context_.relayout(Vector2{event.windowSize()} / event.dpiScaling(),
                          event.windowSize(), event.framebufferSize());
}

void CircleDrop::keyPressEvent(KeyEvent &event) {
  switch (event.key()) {
  default:
    if (imgui_context_.handleKeyPressEvent(event))
      event.setAccepted(true);
  }
}

void CircleDrop::keyReleaseEvent(KeyEvent &event) {
  if (imgui_context_.handleKeyReleaseEvent(event)) {
    event.setAccepted(true);
    return;
  }
}

void CircleDrop::mouseReleaseEvent(MouseEvent &event) {
  if (imgui_context_.handleMouseReleaseEvent(event))
    event.setAccepted(true);
}

void CircleDrop::mouseMoveEvent(MouseMoveEvent &event) {
  if (imgui_context_.handleMouseMoveEvent(event)) {
    event.setAccepted(true);
    return;
  }

  if (!event.buttons())
    return;
}

void CircleDrop::mouseScrollEvent(MouseScrollEvent &event) {
  const Float delta = event.offset().y();
  if (Math::abs(delta) < 1.0e-2f)
    return;

  if (imgui_context_.handleMouseScrollEvent(event)) {
    /* Prevent scrolling the page */
    event.setAccepted();
    return;
  }
}

void CircleDrop::textInputEvent(TextInputEvent &event) {
  if (imgui_context_.handleTextInputEvent(event))
    event.setAccepted(true);
}

void CircleDrop::init_imgui() {
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

    ImGui::GetIO().IniFilename = "bin/imgui/saved_layout.ini";

    imgui_context_ = ImGuiIntegration::Context{
        *ImGui::GetCurrentContext(), Vector2{windowSize()} / dpiScaling(),
        windowSize(), framebufferSize()};

    /* Setup proper blending to be used by ImGui */
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
  }
}

} // namespace Examples
} // namespace Magnum

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CircleDrop)
