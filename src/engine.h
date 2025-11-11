#ifndef GRAPHICS_ENGINE_H
#define GRAPHICS_ENGINE_H

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

#include "shapes/cube.h"
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

  // Transformation matrix
  glm::mat4 modelLeft;      // Model matrix for a 3D object
  glm::mat4 modelRight;
  glm::mat4 view;       // The camera's position and orientation in the world
  glm::mat4 projection; // Orthographic projection matrix maps a 3D scene to a
                        // 2D viewport


  // Shapes
  unique_ptr<Cube> cubeLeft;
  unique_ptr<Cube> cubeRight;

  // Keep track of the camera's distance from the origin
  // Moving the camera closer and farther will have the
  // visual effect of making the cube larger and smaller
  float cameraZ;

  // Shaders
  Shader cubeShader;

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

  /* deltaTime variables */
  float deltaTime = 0.0f; // Time between current frame and last frame
  float lastFrame = 0.0f; // Time of last frame (used to calculate deltaTime)

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
