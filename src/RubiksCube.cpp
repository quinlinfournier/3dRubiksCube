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
                glm::vec3 cubiePos = glm::vec3(x * 1.0f, y * 1.0f, z * 1.0f);

                // 2. Get the specific colors for this piece
                std::vector<color> initialColors = getInitialColors(x, y, z);

                // 3. Create the Cubie instance
                cubelet.push_back(std::make_unique<Cubelet>(
                    cubeletShader,
                    cubiePos,
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

// --- Update Function (Placeholder for animation) ---
void RubiksCube::update(float deltaTime) {
    // This is where layer rotation animation logic will go.
    // Example: if (currentRotation.angle < 90.0f) { applyRotation(deltaTime); }
}

// --- Rotation Placeholders ---
void RubiksCube::startRotation(char axis, float layerValue, float angle) {
    // This is where you'll select the 9 pieces and start the animation state.
    std::cout << "Starting rotation: " << axis << " layer=" << layerValue << " angle=" << angle << std::endl;
}

bool RubiksCube::isRotating() const {
    // Must return true if an animation is active. For now:
    return false;
}