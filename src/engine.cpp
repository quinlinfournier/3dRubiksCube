#include "engine.h"
#include "RubiksCube.h"

const color red(1, 0, 0);
const color green(0, 1, 0);
const color blue(0, 0, 1);
const color yellow(1, 1, 0);
const color magenta(1, 0, 1);
const color cyan(0, 1, 1);
const color white(1, 1, 1);
const color black(0, 0, 0);

Engine::Engine() : keys(), cameraZ(-8.0f) {
  this->initWindow();
  this->initShaders();
  this->initShapes();
  this->initMatrices();
}

Engine::~Engine() {}

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

  window = glfwCreateWindow(width, height, "engine", nullptr, nullptr);
  glfwMakeContextCurrent(window);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    cout << "Failed to initialize GLAD" << endl;
    return -1;
  }

  // OpenGL configuration
  glViewport(0, 0, width, height);
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glfwSwapInterval(1);

  return 0;
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
  // The view matrix is the camera's position and orientation in the world
  // We start at (0, 0, 3) and look at (0, 0, 0) with the up vector being (0, 1,
  // 0)
  view = lookAt(vec3(0.0f, 0.0f, 3.0f),
              vec3(0.0f, 0.0f, 0.0f),
                 vec3(0.0f, 1.0f, 0.0f));
  // The projection matrix for 3D is distorted by 45 degrees to give a
  // perspective view
  projection = glm::perspective(
      glm::radians(45.0f),
      static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
  modelLeft = glm::mat4(1.0f);
  modelRight = glm::mat4(1.0f);
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
  if (keys[GLFW_KEY_ESCAPE])
    glfwSetWindowShouldClose(window, true);


  // Rotate the cube with keys
  // Hint: keys are GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_COMMA, and GLFW_KEY_PERIOD
  // if (keys[GLFW_KEY_UP]) {
  //   // To make the cube appear to tilt up, it needs to rotate around its center point in relation to the x-axis
  //   cubeLeft->rotateX(-0.01f);
  //   cubeRight->rotateX(-.01f);
  //
  // }
  // if (keys[GLFW_KEY_DOWN]) {
  //   cubeLeft->rotateX(.01f);
  //   cubeRight->rotateX(.01f);
  //
  // }
  // if (keys[GLFW_KEY_RIGHT]) {
  //   cubeLeft->rotateY(.01f);
  //   cubeRight->rotateY(-.01f);
  //
  // }
  // if (keys[GLFW_KEY_LEFT]) {
  //   cubeLeft->rotateY(-.01f);
  //   cubeRight->rotateY(.01f);
  //
  // }
  if (!rubiksCube->isRotating()) { // Only allow input if cube is not animating
    if (keys[GLFW_KEY_R]) {
      rubiksCube->startRotation('X', 1.0f, 90.0f); // Right face: X=1 layer, 90 deg
    }
    if (keys[GLFW_KEY_L]) {
      rubiksCube->startRotation('X', -1.0f, -90.0f); // Left face: X=-1 layer, -90 deg
    }
    // ... add U, D, F, B rotations ...
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
  // Clear the screen before rendering the frame
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color
  glClear(GL_COLOR_BUFFER_BIT |
          GL_DEPTH_BUFFER_BIT); // Also need to clear the depth buffer bit

  // Resetting model and view matrices for camera setup
  glm::mat4 model = glm::mat4(1.0f);
  view = glm::mat4(1.0f);
  view = glm::translate(view, glm::vec3(0.0f, 0.0f, cameraZ)); // cameraZ is typically -3.0f

  // Draw the entire RubiksCube model
  rubiksCube->draw(view, projection);

  glfwSwapBuffers(window);
}
bool Engine::shouldClose() {
  return glfwWindowShouldClose(window);
}
