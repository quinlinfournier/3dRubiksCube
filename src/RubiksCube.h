#ifndef RUBIKSCUBE_H
#define RUBIKSCUBE_H

#include "shapes/Cubelet.h" // Includes the Cubelet class and necessary GLM headers
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class RubiksCube {
private:
    struct RotationState {
        char axis = '\0';         // 'X', 'Y', or 'Z'
        int layerValue = 0;       // -1, 0, or 1
        float totalAngle = 0.0f;  // Target total angle (e.g., 90.0f or -90.0f)
        float currentAngle = 0.0f;// Angle animated so far (always positive)
        float speed = 270.0f;     // Rotation speed in degrees per second (increased for snappier feel)
    };

    RotationState currentRotation;

    // Vector to hold the 9 pointers to the Cubelets currently being rotated
    std::vector<Cubelet*> rotatingPieces;

    // Container for the 26 individual pieces
    std::vector<std::unique_ptr<Cubelet>> cubelet;
    Shader& cubeletShader;

    // Define the standard colors for the faces
    const color WHITE = color(1.0f, 1.0f, 1.0f);
    const color YELLOW = color(1.0f, 1.0f, 0.0f);
    const color BLUE = color(0.0f, 0.0f, 1.0f);
    const color GREEN = color(0.0f, 0.5f, 0.0f);
    const color RED = color(1.0f, 0.0f, 0.0f);
    const color ORANGE = color(1.0f, 0.5f, 0.0f);
    const color BLACK = color(0.0f, 0.0f, 0.0f);

    // Helper function to get the initial colors for a cubelet at (x, y, z)
    std::vector<color> getInitialColors(int x, int y, int z);

public:
    RubiksCube(Shader& shader);
    ~RubiksCube() = default;
    void finalizeRotation();
    void update(float deltaTime); // Used for animation
    void draw(const glm::mat4& view, const glm::mat4& projection);

    // Rotation Functions (To be implemented later)
    void startRotation(char axis, float layerValue, float angle);
    bool isRotating() const; // Check if an animation is in progress
    void swapPieceColors(Cubelet* piece, char axis, bool clockwise);};

#endif // RUBIKSCUBE_H