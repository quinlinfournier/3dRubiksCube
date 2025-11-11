#include "engine.h"

const color red(1, 0, 0);
const color green(0, 1, 0);
const color blue(0, 0, 1);
const color yellow(1, 1, 0);
const color magenta(1, 0, 1);
const color cyan(0, 1, 1);
const color white(1, 1, 1);
const color black(0, 0, 0);

Engine::Engine() : keys(), cameraZ(-3.0f) {
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
  cubeLeft =
    make_unique<Cube>(cubeShader, vec3(-0.5f, 0.0f, 0.0f),
                      vec3(0.5f, 0.5f, 0.5f), vector<color>({red, green, blue, yellow, magenta, cyan, white, black}));
  // initialize cubeRight
  cubeRight =
    make_unique<Cube>(cubeShader, vec3(0.5f, 0.0f, 0.0f),
                      vec3(0.5f, 0.5f, 0.5f), vector<color>({red, green, blue, yellow, magenta, cyan, white, black}));
}

void Engine::initMatrices() {
  // The view matrix is the camera's position and orientation in the world
  // We start at (0, 0, 3) and look at (0, 0, 0) with the up vector being (0, 1,
  // 0)
  view = lookAt(vec3(0.0f, 0.0f, 3.0f), vec3(0.0f, 0.0f, 0.0f),
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
  if (keys[GLFW_KEY_UP]) {
    // To make the cube appear to tilt up, it needs to rotate around its center point in relation to the x-axis
    cubeLeft->rotateX(-0.01f);
    cubeRight->rotateX(-.01f);

  }
  if (keys[GLFW_KEY_DOWN]) {
    cubeLeft->rotateX(.01f);
    cubeRight->rotateX(.01f);

  }
  if (keys[GLFW_KEY_RIGHT]) {
    cubeLeft->rotateY(.01f);
    cubeRight->rotateY(-.01f);

  }
  if (keys[GLFW_KEY_LEFT]) {
    cubeLeft->rotateY(-.01f);
    cubeRight->rotateY(.01f);

  }

  // Rotate the second cube to mirror the first
}

void Engine::update() {
  // Calculate delta time
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

}

void Engine::render() {
  // Clear the screen before rendering the frame
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color
  glClear(GL_COLOR_BUFFER_BIT |
          GL_DEPTH_BUFFER_BIT); // Also need to clear the depth buffer bit

  // Resetting model and view matrices every frame prevents the cubes from
  // spinning
  modelLeft = glm::mat4(1.0f);
  modelRight = glm::mat4(1.0f);
  view = glm::mat4(1.0f);

  // Move the camera back 3 units to view the cube
  view = glm::translate(view, glm::vec3(0.0f, 0.0f, cameraZ));

  cubeShader.use();
  // Draw cube
  cubeLeft->setUniforms(modelLeft, view, projection);
  cubeLeft->draw(modelLeft, view, projection);
  // Draw the second cube
  cubeRight->setUniforms(modelLeft, view, projection);
  cubeRight->draw(modelLeft, view, projection);

  glfwSwapBuffers(window);
}

bool Engine::shouldClose() { return glfwWindowShouldClose(window); }
