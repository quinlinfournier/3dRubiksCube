#include "RubiksCube.h"
#include <iostream>
#include <cmath>

bool RubiksCube::isSolved() const {
    for (int i = 0; i < 26; i++) {
        if (cubeletPos[i] != solvedPosition[i]) {
            // finds a piece that isnt correct
            return false;
        }
    }
    // All pieces are correct
    return true;
}

void RubiksCube::initNumbering() {
    int id = 0;

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                // Skip Center
                if (x ==0 && y == 0 && z == 0) continue;

                glm::ivec3 pos(x, y, z);

                // Piece 1, 2, 3, ...
                cubeletID[id] = id;
                // Where cubelet is now
                cubeletPos[id] = pos;
                // Where cubelet belongs
                solvedPosition[id] = pos;

                id++;
            }
        }
    }
}

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
                cubeletMap[pack(x, y, z)] = cubelet.back().get();

            }
        }
    }
    initNumbering();
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

glm::ivec3 RubiksCube::calculateNewGridPosition(glm::ivec3 p, char axis, bool clockwise) {
    switch(axis)
    {
        case 'X':
            if (clockwise)  return { p.x, -p.z,  p.y };   // (y,z) -> (-z,y)
            else            return { p.x,  p.z, -p.y };   // (y,z) -> (z,-y)

        case 'Y':
            if (clockwise)  return {  p.z, p.y, -p.x };   // (x,z) -> (z,-x)
            else            return { -p.z, p.y,  p.x };   // (x,z) -> (-z,x)

        case 'Z':
            if (clockwise)  return { -p.y, p.x, p.z };    // (x,y) -> (-y,x)
            else            return {  p.y, -p.x, p.z };   // (x,y) -> (y,-x)
    }
    return p;
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

        // Tempory Position Vectors
        std::vector<glm::ivec3> oldPosition;
        std::vector<glm::ivec3> newPosition;

        for (Cubelet* piece : rotatingPieces) {
            glm::ivec3 oldPos = piece->getGridPosition();
            glm::ivec3 newPos = calculateNewGridPosition(
                oldPos,
                currentRotation.axis,
                currentRotation.totalAngle > 0
            );
            oldPosition.push_back(oldPos);
            newPosition.push_back(newPos);
        }
        // Update Position Tracking
        for (size_t i = 0; i < oldPosition.size(); i++) {
            glm::ivec3 oldPos = oldPosition[i];
            glm::ivec3 newPos = newPosition[i];

            for (int j = 0; j < 26; j++) {
                if (cubeletPos[j] == oldPos) {
                    cubeletPos[j] = newPos;
                    break;
                }
            }
        }

        for (size_t i = 0; i < rotatingPieces.size(); i++) {
            rotatingPieces[i] -> setGridPosition(newPosition[i]);
            rotatingPieces[i] -> updateModelMatrix();
        }

        rebuildPositions();
        rebuildMap();




        // Reset state
        currentRotation.axis = '\0';
        rotatingPieces.clear();
        std::cout << "Rotation completed!\n";
        // In the rotation completion section, after position updates:
        // CHECK IF SOLVED

        if (isSolved()) {
            std::cout << "==================================" << std::endl;
            std::cout << "🎉🎉🎉 CUBE SOLVED! 🎉🎉🎉" << std::endl;
            std::cout << "==================================" << std::endl;
        }
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
    // std::cout << "=== AFTER COLOR ROTATION ===\n";
    // for (Cubelet* piece : rotatingPieces) {
    //     piece->debugColors();
    // }
}

bool RubiksCube::isRotating() const {
    return currentRotation.axis != '\0';
}

void RubiksCube::debugPositionTracking(const std::vector<glm::ivec3>& oldPosition,
                                       const std::vector<glm::ivec3>& newPosition) {
    std::cout << "=== DETAILED POSITION DEBUG ===" << std::endl;

    // for (size_t i = 0; i < oldPosition.size(); ++i) {
    //     std::cout << "Piece: (" << oldPosition[i].x << "," << oldPosition[i].y << "," << oldPosition[i].z
    //               << ") -> (" << newPosition[i].x << "," << newPosition[i].y << "," << newPosition[i].z << ")" << std::endl;
    // }

    // Check if any positions are duplicated
    // std::cout << "=== CHECKING FOR DUPLICATE POSITIONS ===" << std::endl;
    // for (int i = 0; i < 26; i++) {
    //     for (int j = i + 1; j < 26; j++) {
    //         if (cubeletPos[i] == cubeletPos[j]) {
    //             std::cout << "DUPLICATE! Piece " << i << " and " << j
    //                       << " both at (" << cubeletPos[i].x << ","
    //                       << cubeletPos[i].y << "," << cubeletPos[i].z << ")" << std::endl;
    //         }
    //     }
    // }
}


void RubiksCube::rebuildMap() {
    cubeletMap.clear();
    for (const auto& cptr : cubelet) {
        glm::ivec3 p = cptr-> getGridPosition();
        cubeletMap[pack(p.x, p.y, p.z)] = cptr.get();
    }
}
void RubiksCube::rebuildPositions() {
    // Assumes cubelet.size() == 26 and that cubelet order matches initNumbering() order
    for (size_t i = 0; i < cubelet.size(); ++i) {
        cubeletPos[i] = cubelet[i]->getGridPosition();
    }
}

Cubelet* RubiksCube::getCubelet(glm::ivec3 gridPos) {
    long long key = pack(gridPos.x, gridPos.y, gridPos.z);
    auto it = cubeletMap.find(key);
    if (it != cubeletMap.end()) {
        return it->second;
    }
    return nullptr;
}
const Cubelet* RubiksCube::getCubelet(glm::ivec3 gridPos) const {
    long long key = pack(gridPos.x, gridPos.y, gridPos.z);
    auto it = cubeletMap.find(key);
    if (it != cubeletMap.end()) {
        return it->second;
    }
    return nullptr;
}


// Print every cubelet and its face colors (calls Cubelet::debugColors())
// void RubiksCube::printFaceCenters() const {
//     const std::array<Face, 6> faces = { UP, DOWN, RIGHT, LEFT, FRONT, BACK };
//
//     for (Face f : faces) {
//         // Find the cubelet at center of this face
//         glm::ivec3 pos;
//         switch(f) {
//             case UP:    pos = glm::ivec3(0, 1, 0); break;
//             case DOWN:  pos = glm::ivec3(0, -1, 0); break;
//             case FRONT: pos = glm::ivec3(0, 0, 1); break;
//             case BACK:  pos = glm::ivec3(0, 0, -1); break;
//             case RIGHT: pos = glm::ivec3(1, 0, 0); break;
//             case LEFT:  pos = glm::ivec3(-1, 0, 0); break;
//         }
//
//         const Cubelet* found = getCubelet(pos);
//         if (found) {
//             color c = found->getFaceColor(f); // single color
//             char faceChar = '?';
//
//             // Compare to your cube’s color constants
//             if (c.red   == WHITE.red && c.green == WHITE.green && c.blue == WHITE.blue) faceChar = 'W';
//             else if (c.red == YELLOW.red && c.green == YELLOW.green && c.blue == YELLOW.blue) faceChar = 'Y';
//             else if (c.red == RED.red && c.green == RED.green && c.blue == RED.blue) faceChar = 'R';
//             else if (c.red == ORANGE.red && c.green == ORANGE.green && c.blue == ORANGE.blue) faceChar = 'O';
//             else if (c.red == BLUE.red && c.green == BLUE.green && c.blue == BLUE.blue) faceChar = 'B';
//             else if (c.red == GREEN.red && c.green == GREEN.green && c.blue == GREEN.blue) faceChar = 'G';
//
//             std::cout << faceChar << " ";
//         }
//     }
//     std::cout << std::endl;
// }
//
