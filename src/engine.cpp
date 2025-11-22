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
  if (!cubeShader.ID) {
    std::cout << "ERROR: Failed to load cube shader!" << std::endl;
  } else {
    std::cout << "Shader loaded successfully, ID: " << cubeShader.ID << std::endl;
  }
}

void Engine::initShapes() {
    this->rubiksCube = make_unique<RubiksCube>(cubeShader);
}

void Engine::initMatrices() {
    updateCamera();

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

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !keys[GLFW_KEY_P]) {
        rubiksCube->printPosition();
        keys[GLFW_KEY_M] = true;  // Prevent repeated printing
    } else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        keys[GLFW_KEY_M] = false;
    }

  // Close window if escape key is pressed
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

    // Cube rotation controls (only when not already rotating)
    if (!rubiksCube->isRotating()) {
        // We need to handle each key separately to properly detect shift combinations

        // R - Right face
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateRight(!shiftPressed); // R clockwise, R' counterclockwise
        }

        // L - Left face
        else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateLeft(!shiftPressed); // L clockwise, L' counterclockwise
        }

        // U - Up face
        else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateUp(!shiftPressed); // U clockwise, U' counterclockwise
        }

        // D - Down face
        else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateDown(!shiftPressed); // D clockwise, D' counterclockwise
        }

        // F - Front face
        else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateFront(!shiftPressed); // F clockwise, F' counterclockwise
        }

        // B - Back face
        else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateBack(!shiftPressed); // B clockwise, B' counterclockwise
        }
        // Middle layer controls - A, S, D
        // M slice (between L and R) - A key
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateMiddle('X',!shiftPressed);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateMiddle('Y',!shiftPressed);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            bool shiftPressed = isShiftPressed();
            rotateMiddle('Z',!shiftPressed);
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            executeRandomMove();
        }
    }

    // Camera controls
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        // Rotate camera around cube
        cameraY -= 60.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraY += 60.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        cameraX -= 60.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraX += 60.0f * deltaTime;
    }

    // Update view matrix with new camera angles
    updateCamera();


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

    // Camera is now updated in processInput() via updateCamera()
    rubiksCube->draw(view, projection);
    glfwSwapBuffers(window);
}

bool Engine::shouldClose() {
  return glfwWindowShouldClose(window);
}

void Engine::updateCamera() {
    // Calculate camera position using spherical coordinates
    glm::vec3 cameraPos;
    cameraPos.x = cameraDistance * cos(glm::radians(cameraY)) * cos(glm::radians(cameraX));
    cameraPos.y = cameraDistance * sin(glm::radians(cameraX));
    cameraPos.z = cameraDistance * sin(glm::radians(cameraY)) * cos(glm::radians(cameraX));

    view = glm::lookAt(cameraPos,
                      glm::vec3(0.0f, 0.0f, 0.0f), // Look at center of cube
                      glm::vec3(0.0f, 1.0f, 0.0f)); // Up vector
}

// Helper to check shift key
bool Engine::isShiftPressed() {
    return glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

// Rotation functions
void Engine::rotateFront(bool clockwise) {
    rubiksCube->startRotation('Z', 1.0f, clockwise ? 90.0f : -90.0f);
}

void Engine::rotateBack(bool clockwise) {
    rubiksCube->startRotation('Z', -1.0f, clockwise ? -90.0f : 90.0f);
}

void Engine::rotateRight(bool clockwise) {
    rubiksCube->startRotation('X', 1.0f, clockwise ? 90.0f : -90.0f);
}

void Engine::rotateLeft(bool clockwise) {
    rubiksCube->startRotation('X', -1.0f, clockwise ? -90.0f : 90.0f);
}

void Engine::rotateUp(bool clockwise) {
    rubiksCube->startRotation('Y', 1.0f, clockwise ? 90.0f : -90.0f);
}

void Engine::rotateDown(bool clockwise) {
    rubiksCube->startRotation('Y', -1.0f, clockwise ? -90.0f : 90.0f);
}
void Engine::rotateMiddle(char axis, bool clockwise) {
    rubiksCube->startRotation(axis, 0.0f, clockwise ? -90.0f : 90.0f);
}



void Engine::executeRandomMove() {
    if (rubiksCube->isRotating()) return;

    // Random numbert
    int randomMove = rand() % 12;

    switch(randomMove) {
        case 0:  rotateRight(true);   break;  // R
        case 1:  rotateRight(false);  break;  // R'
        case 2:  rotateLeft(true);    break;  // L
        case 3:  rotateLeft(false);   break;  // L'
        case 4:  rotateUp(true);      break;  // U
        case 5:  rotateUp(false);     break;  // U'
        case 6:  rotateDown(true);    break;  // D
        case 7:  rotateDown(false);   break;  // D'
        case 8:  rotateFront(true);   break;  // F
        case 9:  rotateFront(false);  break;  // F'
        case 10: rotateBack(true);    break;  // B
        case 11: rotateBack(false);   break;  // B'
    }

    std::cout << "Executed random move: " << randomMove << std::endl;
}

