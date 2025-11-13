#include "engine.h"
#include "RubiksCube.h"

Engine::Engine() : cameraZ(-8.0f) {
  if (!initWindow()) {
    std::cout << "Failed to initialize window" << std::endl;
    return;
  }
  initShaders();
  initShapes();
  initMatrices();
}

Engine::~Engine() {
  glfwTerminate();
}

unsigned int Engine::initWindow(bool debug) {
  // glfw: initialize and configure
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
  glfwWindowHint(GLFW_RESIZABLE, false);

  window = glfwCreateWindow(width, height, "Rubik's Cube", nullptr, nullptr);
  if (!window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(window);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    cout << "Failed to initialize GLAD" << endl;
    return -1;
  }

  // OpenGL configuration
  glViewport(0, 0, width, height);
  // glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glfwSwapInterval(1);

  return true;
}

void Engine::initShaders() {
  // load shader manager
  shaderManager = ShaderManager();

  // Load shader into shader manager and retrieve it
  cubeShader = this->shaderManager.loadShader("../res/shaders/shape3D.vert",
                                                "../res/shaders/shape3D.frag",
                                                nullptr, "shape");
}

void Engine::initShapes() {
    this->rubiksCube = make_unique<RubiksCube>(cubeShader);
}

void Engine::initMatrices() {
  view = glm::lookAt(glm::vec3(5.0f, 4.0f, 8.0f),
                        vec3(0.0f, 0.0f, 0.0f),
                           vec3(1.0f, 1.0f, 0.0f));

  projection = glm::perspective(glm::radians(45.0f),
                              static_cast<float>(width) / static_cast<float>(height),
                              0.1f, 100.0f);
}

void Engine::processInput() {
  glfwPollEvents();

  // Set keys to true if pressed, false if released
  for (int key = 0; key < 1024; ++key) {
    if (glfwGetKey(window, key) == GLFW_PRESS)
      keys[key] = true;
    else if (glfwGetKey(window, key) == GLFW_RELEASE)
      keys[key] = false;
  }

  // Close window if escape key is pressed
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (!rubiksCube->isRotating()) {
    // Simple controls - you can expand this later
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
      rubiksCube->startRotation('X', 1.0f, 90.0f); // Right face clockwise
    }
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
      rubiksCube->startRotation('Y', 1.0f, 90.0f); // Upper face clockwise
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
      rubiksCube->startRotation('Z', 1.0f, 90.0f); // Front face clockwise
    }
  }

  // Rotate the second cube to mirror the first
}

void Engine::update() {
  // Calculate delta time
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  rubiksCube->update(deltaTime);

}

void Engine::render() {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Update view matrix for camera movement
  view = glm::lookAt(glm::vec3(5.0f, 4.0f, cameraZ),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));

  rubiksCube->draw(view, projection);
  glfwSwapBuffers(window);
}

bool Engine::shouldClose() {
  return glfwWindowShouldClose(window);
}
