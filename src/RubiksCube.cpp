#include "RubiksCube.h"
#include <iostream>
#include <cmath>

// --- Helper: Get Initial Colors for a Cubie ---
std::vector<color> RubiksCube::getInitialColors(int x, int y, int z) {
    // Face color mapping (0:Front, 1:Back, 2:Right, 3:Left, 4:Top, 5:Bottom)
    std::vector<color> colors(6, BLACK); // Default to BLACK for internal faces

    // Front/Back (Z-Axis)
    if (z == 1) colors[FRONT] = BLUE;    // Front face (Z=1) is BLUE
    if (z == -1) colors[BACK] = GREEN;  // Back face (Z=-1) is GREEN

    // Right/Left (X-Axis)
    if (x == 1) colors[RIGHT] = RED;     // Right face (X=-1) is RED
    if (x == -1) colors[LEFT] = ORANGE; // Left face (X=1) is ORANGE

    // Top/Bottom (Y-Axis)
    if (y == 1) colors[UP] = WHITE;   // Top face (Y=1) is WHITE
    if (y == -1) colors[DOWN] = YELLOW; // Bottom face (Y=-1) is YELLOW

    return colors;
}

// --- Constructor: Assemble the Cube ---
RubiksCube::RubiksCube(Shader& shader) : cubeletShader(shader) {
    // Iterate through all 27 potential positions (-1, 0, 1 for each axis)
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                // Skip the central piece (0, 0, 0)
                if (x == 0 && y == 0 && z == 0) {
                    continue;
                }

                glm::ivec3 gridPos = glm::ivec3(x, y, z);
                std::vector<color> initialColors = getInitialColors(x, y, z);

                cubelet.push_back(std::make_unique<Cubelet>(
                    cubeletShader,
                    gridPos,
                    glm::vec3(0.95f, 0.95f, 0.95f), // Size: slightly smaller than 1.0 to show gaps
                    initialColors
                ));
            }
        }
    }
}

// --- Drawing Function ---
void RubiksCube::draw(const glm::mat4& view, const glm::mat4& projection) {
    // Simply iterate through all 26 cubies and draw them
    for (const auto& cubelet : cubelet) {
        cubelet->draw(view, projection);
    }
}

// --- Helper Methods ---
glm::vec3 RubiksCube::getAxisVector(char axis) {
    if (axis == 'X') return glm::vec3(1.0f, 0.0f, 0.0f);
    if (axis == 'Y') return glm::vec3(0.0f, 1.0f, 0.0f);
    if (axis == 'Z') return glm::vec3(0.0f, 0.0f, 1.0f);
    return glm::vec3(0.0f);
}

glm::vec3 RubiksCube::getWorldPositionFromGrid(glm::ivec3 gridPos) {
    return glm::vec3(gridPos.x * 1.0f, gridPos.y * 1.0f, gridPos.z * 1.0f);
}

glm::ivec3 RubiksCube::calculateNewGridPosition(glm::ivec3 oldPos, char axis, bool clockwise) {
    if (axis == 'X') {
        return clockwise ? glm::ivec3(oldPos.x, -oldPos.z, oldPos.y)
                        : glm::ivec3(oldPos.x, oldPos.z, -oldPos.y);
    } else if (axis == 'Y') {
        return clockwise ? glm::ivec3(oldPos.z, oldPos.y, -oldPos.x)
                        : glm::ivec3(-oldPos.z, oldPos.y, oldPos.x);
    } else if (axis == 'Z') {
        return clockwise ? glm::ivec3(-oldPos.y, oldPos.x, oldPos.z)
                        : glm::ivec3(oldPos.y, -oldPos.x, oldPos.z);
    }
    return oldPos;
}

// --- Update Function ---
void RubiksCube::update(float deltaTime) {
    if (!isRotating()) return;

    float angleThisFrame = currentRotation.speed * deltaTime;
    float angleRemaining = std::abs(currentRotation.totalAngle) - currentRotation.currentAngle;

    // Check if rotation is complete
    if (angleThisFrame >= angleRemaining) {

        bool clockwise = (currentRotation.totalAngle > 0);

        // Update face colors for all rotating pieces
        for (Cubelet* piece : rotatingPieces) {
            if (currentRotation.axis == 'Y') {
                piece->rotateAroundY(clockwise);
            } else if (currentRotation.axis == 'X') {
                piece->rotateAroundX(clockwise);
            } else if (currentRotation.axis == 'Z') {
                piece->rotateAroundZ(clockwise);
            }
            piece->updateVertexColors(); // Update GPU buffer with new colors
        }

        for (Cubelet* piece : rotatingPieces) {
            glm::ivec3 oldPos = piece->getGridPosition();
            glm::ivec3 newPos = calculateNewGridPosition(
                oldPos,
                currentRotation.axis,
                currentRotation.totalAngle > 0
            );
            std::cout << "Finalizing: (" << oldPos.x << "," << oldPos.y << "," << oldPos.z
                      << ") -> (" << newPos.x << "," << newPos.y << "," << newPos.z << ")\n";
            piece->setGridPosition(newPos);

            piece->updateModelMatrix(); // This clears any accumulated rotation
        }

        // Reset state
        currentRotation.axis = '\0';
        rotatingPieces.clear();
        std::cout << "Rotation completed!\n";
        return;
    }

    // ANIMATE: Apply rotation around cube center
    float rotationDir = (currentRotation.totalAngle > 0) ? 1.0f : -1.0f;
    float angle = rotationDir * angleThisFrame;

    glm::vec3 axis = getAxisVector(currentRotation.axis);

    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis);

    for (Cubelet* piece : rotatingPieces) {
        piece -> rotateLocal(rotationMatrix);
    }

    currentRotation.currentAngle += std::abs(angleThisFrame);
}


// --- Rotation ---
void RubiksCube::startRotation(char axis, float layerValue, float angle) {
    if (isRotating()) {
        return;
    }

    // Init State
    currentRotation.axis = axis;
    currentRotation.layerValue = (int)std::round(layerValue); // Snap to -1, 0, or 1
    currentRotation.totalAngle = angle;
    currentRotation.currentAngle = 0.0f;
    rotatingPieces.clear();

    // Group the 9 pieces that belong to a layer
    for (const auto& cubelet : cubelet) {
        glm::ivec3 pos = cubelet->getGridPosition();

        bool isPieceInLayer = false;

        if (axis == 'X' && std::abs(pos.x - layerValue) < 0.01f) {
            isPieceInLayer = true;
        } else if (axis == 'Y' && std::abs(pos.y - layerValue) < 0.01f) {
            isPieceInLayer = true;
        } else if (axis == 'Z' && std::abs(pos.z - layerValue) < 0.01f) {
            isPieceInLayer = true;
        }

        if (isPieceInLayer) {
            rotatingPieces.push_back(cubelet.get());
        }
    }
    // After color rotation, add debug output
    std::cout << "=== AFTER COLOR ROTATION ===\n";
    for (Cubelet* piece : rotatingPieces) {
        piece->debugColors();
    }
}

bool RubiksCube::isRotating() const {
    return currentRotation.axis != '\0';
}