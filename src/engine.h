#ifndef GRAPHICS_ENGINE_H
#define GRAPHICS_ENGINE_H

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

#include "RubiksCube.h"
#include "shapes/cubelet.h"
#include "shader/shaderManager.h"

using std::vector, std::unique_ptr, std::make_unique, glm::ortho, glm::cross,
    glm::mat4, glm::vec3, glm::vec4;

/**
 * @brief The Engine class.
 * @details The Engine class is responsible for initializing the GLFW window,
 * loading shaders, and rendering the game state.
 */
class Engine {
private:
  /// @brief The actual GLFW window.
  GLFWwindow *window{};

  /// @brief The width and height of the window.
  const unsigned int width = 800, height = 600; // Window dimensions

  /// @brief Keyboard state (True if pressed, false if not pressed).
  /// @details Index this array with GLFW_KEY_{key} to get the state of a key.
  bool keys[1024];

  /// @brief Responsible for loading and storing all the shaders used in the
  /// project.
  /// @details Initialized in initShaders()
  ShaderManager shaderManager;

  // Shapes
  unique_ptr<RubiksCube> rubiksCube;
  // Camera
  glm::mat4 view;
  glm::mat4 projection;
  float cameraZ = -8.0f;

  // Shader
  Shader cubeShader;

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  // Keep track of the camera's distance from the origin
  // Moving the camera closer and farther will have the
  // visual effect of making the cube larger and smaller

  /// @note Call glCheckError() after every OpenGL call to check for errors.
  GLenum glCheckError_(const char *file, int line);
/// @brief Macro for glCheckError_ function. Used for debugging.
#define glCheckError() glCheckError_(__FILE__, __LINE__)

public:
  /// @brief Constructor for the Engine class.
  /// @details Initializes window and shaders.
  Engine();

  /// @brief Destructor for the Engine class.
  ~Engine();

  /// @brief Initializes the GLFW window.
  /// @return 0 if successful, -1 otherwise.
  unsigned int initWindow(bool debug = false);

  /// @brief Loads shaders from files and stores them in the shaderManager.
  /// @details Renderers are initialized here.
  void initShaders();

  /// @brief Initializes the shapes to be rendered.
  void initShapes();

  /// @brief Processes input from the user.
  /// @details (e.g. keyboard input, mouse input, etc.)
  void processInput();

  /// @brief Initializes the model, view, and projection matrices.
  void initMatrices();

  /// @brief Updates the game state.
  /// @details (e.g. collision detection, delta time, etc.)
  void update();

  /// @brief Renders the game state.
  /// @details Displays/renders objects on the screen.
  void render();


  // -----------------------------------
  // Getters
  // -----------------------------------

  /// @brief Returns true if the window should close.
  /// @details (Wrapper for glfwWindowShouldClose()).
  /// @return true if the window should close
  /// @return false if the window should not close
  bool shouldClose();
};

#endif // GRAPHICS_ENGINE_H
