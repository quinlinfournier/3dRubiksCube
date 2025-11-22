#ifndef RUBIKSCUBE_H
#define RUBIKSCUBE_H

#include "shapes/Cubelet.h" // Includes the Cubelet class and necessary GLM headers
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <unordered_map>

class RubiksCube {
private:
    // Give each a permanent id number
    std::array<int,26> cubeletID;
    std::array<int,26> currentOrientation;
    std::array<int,26> solvedOrientation;

    std::unordered_map<long long, Cubelet*> cubeletMap;

    inline long long pack(int x, int y, int z) {
        return (long long)((x+2) * 25 + (y+2) * 5 + (z + 2));
    }


    // Track where the ID is located
    std::array<glm::ivec3,26> cubeletPos;

    // Track where the position should be
    std::array<glm::ivec3,26> solvedPosition;




    void initNumbering();

    bool isSolved() const;



    struct RotationState {
        char axis = '\0';         // 'X', 'Y', or 'Z'
        int layerValue = 0;       // -1, 0, or 1
        float totalAngle = 0.0f;  // Target total angle (e.g., 90.0f or -90.0f)
        float currentAngle = 0.0f;// Angle animated (always positive)
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
    glm::vec3 getAxisVector(char axis);
    glm::vec3 getWorldPositionFromGrid(glm::ivec3 gridPos);
    glm::ivec3 calculateNewGridPosition(glm::ivec3 oldPos, char axis, bool clockwise);

public:
    RubiksCube(Shader& shader);
    ~RubiksCube() = default;
    void update(float deltaTime); // Used for animation
    void draw(const glm::mat4& view, const glm::mat4& projection);

    // Rotation Functions (To be implemented later)
    void startRotation(char axis, float layerValue, float angle);
    bool isRotating() const; // Check if an animation is in progress

    // Debuging
    void printPosition();
    void debugPositionTracking(const std::vector<glm::ivec3>& oldPos,
                               const std::vector<glm::ivec3>& newPos);
    void rebuildMap();
    void rebuildPositions();

};

#endif // RUBIKSCUBE_H