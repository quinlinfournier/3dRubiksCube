#include "engine.h"
#include "RubiksCube.h"
#include "Solver.h"

Engine::Engine() : cameraZ(-8.0f) {
  if (!initWindow()) {
    std::cout << "Failed to initialize window" << std::endl;
    return;
  }
  initShaders();
  initShapes(); // Create Cube
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
  }this->rubiksCube = make_unique<RubiksCube>(cubeShader);

    // // Debug: print cubelets and centers once at startup
    // rubiksCube->printAllCubelets();   // prints every cubelet face mapping
    // rubiksCube->printFaceCenters();   // prints U/D/L/R/F/B center colors
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

    // Track key states
    for (int key = 0; key < 1024; ++key) {
        int state = glfwGetKey(window, key);
        if (state == GLFW_PRESS && !keyLatch[key]) {
            keyLatch[key] = true;

            // --- ONE-SHOT ACTIONS BELOW ---

            // Close window
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(window, true);

            // Cube moves (only if cube not rotating)
            if (!rubiksCube->isRotating()) {

                bool shiftPressed = isShiftPressed();

                if (key == GLFW_KEY_Z) rotateRight(!shiftPressed);
                if (key == GLFW_KEY_C) rotateLeft(!shiftPressed);
                if (key == GLFW_KEY_W) rotateUp(!shiftPressed);
                if (key == GLFW_KEY_X) rotateDown(!shiftPressed);
                if (key == GLFW_KEY_Q) rotateFront(!shiftPressed);
                if (key == GLFW_KEY_E) rotateBack(!shiftPressed);

                if (key == GLFW_KEY_A) rotateMiddle('X', !shiftPressed);
                if (key == GLFW_KEY_S) rotateMiddle('Y', !shiftPressed);
                if (key == GLFW_KEY_D) rotateMiddle('Z', !shiftPressed);
            }

            // Scramble
            if (key == GLFW_KEY_P && !isScrambling && !rubiksCube->isRotating()) {
                for (int f = 0; f < 20; f++)
                    scrambleMoves.push(rand() % 12);
                isScrambling = true;
                std::cout << "Scrambling..." << std::endl;
            }

            // Start solver tests
            if (key == GLFW_KEY_T) {
                if (!cubeSolver) initSolver();
                testSolverAccess();
            }

            // Auto solve
            if (key == GLFW_KEY_SPACE) {
                if (!cubeSolver) initSolver();
                startAutoSolve();
            }
        }
        else if (state == GLFW_RELEASE) {
            keyLatch[key] = false;
        }
    }

    // Camera rotation (continuous movement allowed)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cameraY -= 60.f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraY += 60.f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    cameraX -= 60.f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  cameraX += 60.f * deltaTime;

    updateCamera();
}


void Engine::update() {
  // Calculate delta time
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  rubiksCube->update(deltaTime);

  if (isScrambling && !rubiksCube->isRotating() && !scrambleMoves.empty()) {
      int move = scrambleMoves.front();
      scrambleMoves.pop();

      switch(move) {
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
       if (scrambleMoves.empty()) {
           isScrambling = false;
       }
  }

    if (cubeSolver && cubeSolver->isSolving() && !rubiksCube->isRotating()) {
        std::string move = cubeSolver->getNextMove();

        if (!move.empty()) {
            std::cout << "Solver executing next move: " << move << std::endl;
            rubiksCube->executeMove(move);
        }
        else if (cubeSolver->getCurrentState() == SOLVING) {
            // Check if we're actually solved
            if (rubiksCube->isSolved()) {
                std::cout << "Cube solved!" << std::endl;
                cubeSolver->setState(COMPLETE);
            }
        }
    }
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

    // Random number
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

void Engine::initSolver() {
    if (rubiksCube && !cubeSolver) {
        cubeSolver = std::make_unique<Solver>(rubiksCube.get());
        std::cout << "Solver Init successful" << std::endl;
        cubeSolver->testCubeAccess(rubiksCube.get());
    }
}

void Engine::testSolverAccess() {
    if (cubeSolver && rubiksCube) {
        cubeSolver -> testCubeAccess(rubiksCube.get());
    }
}

void Engine::startAutoSolve() {
    if (cubeSolver && rubiksCube) {
        cubeSolver->solve(rubiksCube.get());
    } else {
        if (!cubeSolver) std::cout << "Solver not initialized" << std::endl;
        if (!rubiksCube) std::cout << "Cube not initialized" << std::endl;
    }
}

void RubiksCube::printAllCubelets() const {
    // std::cout << "=== ALL CUBELETS DEBUG ===" << std::endl;
    // for (const auto& piece : cubelet) {
    //     glm::ivec3 pos = piece->getGridPosition();
    //     std::cout << "Cubelet at (" << pos.x << "," << pos.y << "," << pos.z << "): ";
    //     piece->debugColors();
    // }
}
