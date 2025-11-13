// RubiksCube.cpp

#include "RubiksCube.h"
#include <iostream>

// --- Helper: Get Initial Colors for a Cubie ---
// This function determines which of the 6 faces should be colored
// based on the cubie's starting position (x, y, z).
std::vector<color> RubiksCube::getInitialColors(int x, int y, int z) {
    // Face color mapping (0:Front, 1:Back, 2:Right, 3:Left, 4:Top, 5:Bottom)
    std::vector<color> colors(6, BLACK); // Default to BLACK for internal faces

    // Front/Back (Z-Axis)
    if (z == 1) colors[0] = BLUE;    // Front face (Z=1) is BLUE
    if (z == -1) colors[1] = GREEN;  // Back face (Z=-1) is GREEN

    // Right/Left (X-Axis)
    if (x == 1) colors[2] = RED;     // Right face (X=1) is RED
    if (x == -1) colors[3] = ORANGE; // Left face (X=-1) is ORANGE

    // Top/Bottom (Y-Axis)
    if (y == 1) colors[4] = WHITE;   // Top face (Y=1) is WHITE
    if (y == -1) colors[5] = YELLOW; // Bottom face (Y=-1) is YELLOW

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

                // 1. Calculate the starting center position
                // We use 1.0f separation, minus a small margin (0.05f) for the cubie size
                glm::vec3 cubeletPos = glm::vec3(x * 1.0f, y * 1.0f, z * 1.0f);

                // 2. Get the specific colors for this piece
                std::vector<color> initialColors = getInitialColors(x, y, z);

                // 3. Create the Cubie instance
                cubelet.push_back(std::make_unique<Cubelet>(
                    cubeletShader,
                    cubeletPos,
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

void RubiksCube::finalizeRotation() {

    // 1. Update Position and Color for each piece
    for (Cubelet* piece : rotatingPieces) {
        glm::vec3 oldPos = piece->getPosition();
        glm::vec3 newPos = oldPos;

        // Apply Vector Rotation Rules based on the axis and direction (TotalAngle > 0 is Clockwise)

        if (currentRotation.axis == 'X') {
            if (currentRotation.totalAngle > 0) { // R (Clockwise): (x, y, z) -> (x, z, -y)
                newPos = glm::vec3(oldPos.x, oldPos.z, -oldPos.y);
            } else { // R' (Counter-Clockwise): (x, y, z) -> (x, -z, y)
                newPos = glm::vec3(oldPos.x, -oldPos.z, oldPos.y);
            }
        } else if (currentRotation.axis == 'Y') {
            if (currentRotation.totalAngle > 0) { // U (Clockwise): (x, y, z) -> (-z, y, x)
                newPos = glm::vec3(-oldPos.z, oldPos.y, oldPos.x);
            } else { // U' (Counter-Clockwise): (x, y, z) -> (z, y, -x)
                newPos = glm::vec3(oldPos.z, oldPos.y, -oldPos.x);
            }
        } else if (currentRotation.axis == 'Z') {
            if (currentRotation.totalAngle > 0) { // F (Clockwise): (x, y, z) -> (y, -x, z)
                newPos = glm::vec3(oldPos.y, -oldPos.x, oldPos.z);
            } else { // F' (Counter-Clockwise): (x, y, z) -> (-y, x, z)
                newPos = glm::vec3(-oldPos.y, oldPos.x, oldPos.z);
            }
        }

        // CRITICAL STEP: Snap new position to the grid (-1, 0, or 1)
        // to eliminate accumulated floating-point errors from the animation.
        newPos.x = std::round(newPos.x);
        newPos.y = std::round(newPos.y);
        newPos.z = std::round(newPos.z);

        // Permanently update the piece's grid position and reset the translation component
        // of its model matrix to the new, exact grid coordinates.
        piece->setPosition(newPos);

        // Update the piece's internal color arrangement (the actual puzzle state change).
        swapPieceColors(piece, currentRotation.axis, currentRotation.totalAngle > 0);
    }

    // 2. Reset the rotation state
    // This allows the user to input the next turn.
    currentRotation.axis = '\0';
    rotatingPieces.clear();
}

// --- Update Function (Placeholder for animation) ---
void RubiksCube::update(float deltaTime) {
    if (!isRotating()) {
        return;
    }

    // Determine the amount of angle we want to rotate this frame
    float angleRemaining = std::abs(currentRotation.totalAngle) - currentRotation.currentAngle;
    float angleThisFrame = currentRotation.speed * deltaTime;

    // Ensure we don't overshoot the target angle
    if (angleThisFrame >= angleRemaining) {
        angleThisFrame = angleRemaining;
        finalizeRotation(); // Finished! Snap to grid and update state.
        return; // Exit update after finalizing
    }

    // Apply the sign of the totalAngle to the rotation
    float rotationDir = (currentRotation.totalAngle > 0) ? 1.0f : -1.0f;
    float finalAngle = rotationDir * angleThisFrame;

    // 1. Create the rotation matrix for this frame
    glm::mat4 rotationMatrix(1.0f);
    if (currentRotation.axis == 'X') {
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(finalAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    } else if (currentRotation.axis == 'Y') {
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(finalAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    } else if (currentRotation.axis == 'Z') {
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(finalAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    // 2. Apply the matrix to all rotating pieces
    for (Cubelet* piece : rotatingPieces) {
        piece->rotateLocal(rotationMatrix);
    }

    // 3. Update the total rotated angle (using absolute value)
    currentRotation.currentAngle += angleThisFrame;
}

// --- Rotation Placeholders ---
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

    // group the 9 pieces that belong to a layer
    for (const auto& cubelet : cubelet) {
        // int i = 1;
        glm::vec3 pos = cubelet->getPosition();

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
            // cubelet->setPosition(90 * i);
            // i++;
        }
    }

}

bool RubiksCube::isRotating() const {
    // returns true if it is currently rotating
    return currentRotation.axis != '\0';
}

// In RubiksCube.cpp

void RubiksCube::swapPieceColors(Cubelet* piece, char axis, bool clockwise) {
    // Face Index Mapping: 0:Front(+Z), 1:Back(-Z), 2:Right(+X), 3:Left(-X), 4:Top(+Y), 5:Bottom(-Y)
    std::vector<color>& colors = piece->getFaceColors();
    std::vector<color> tempColors = colors;

    if (axis == 'X') { // R/R' affects: 0, 1, 4, 5 (Front, Back, Top, Bottom)
        if (clockwise) { // R
            colors[4] = tempColors[0];
            colors[5] = tempColors[4];
            colors[1] = tempColors[5];
            colors[0] = tempColors[1];
        } else { // L
            colors[1] = tempColors[0];
            colors[5] = tempColors[4];
            colors[1] = tempColors[5];
            colors[0] = tempColors[4];
        }
    } else if (axis == 'Y') { // U/U' affects: 0, 1, 2, 3 (Front, Back, Right, Left)
        if (clockwise) { // U
            colors[3] = tempColors[0];
            colors[1] = tempColors[3];
            colors[2] = tempColors[1];
            colors[0] = tempColors[2];
        } else { // D
            colors[2] = tempColors[0];
            colors[1] = tempColors[2];
            colors[3] = tempColors[1];
            colors[0] = tempColors[3];
        }
    } else if (axis == 'Z') { // F/F' affects: 2, 3, 4, 5 (Right, Left, Top, Bottom)
        if (clockwise) { // F
            colors[4] = tempColors[2];
            colors[5] = tempColors[4];
            colors[3] = tempColors[5];
            colors[2] = tempColors[3];
        } else { // B
            colors[3] = tempColors[2];
            colors[5] = tempColors[3];
            colors[4] = tempColors[5];
            colors[2] = tempColors[4];
        }
    }
}
