#ifndef RUBIKSCUBE_H
#define RUBIKSCUBE_H

#include "shapes/Cubelet.h" // Includes the Cubelet class and necessary GLM headers
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <unordered_map>

class Solver;

class RubiksCube {

    friend class Solver;
private:
    // Give each a permanent id number
    std::array<int,26> cubeletID;
    std::array<int,26> currentOrientation;
    std::array<int,26> solvedOrientation;

    std::unordered_map<long long, Cubelet*> cubeletMap;

    inline long long pack(int x, int y, int z) const {
        return (long long)((x+2) * 25 + (y+2) * 5 + (z + 2));
    }


    // Track where the ID is located
    std::array<glm::ivec3,26> cubeletPos;

    // Track where the position should be
    std::array<glm::ivec3,26> solvedPosition;


    void initNumbering();




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

    Cubelet* getCubelet(glm::ivec3 gridPos);
    const Cubelet* getCubelet(glm::ivec3 gridPos) const;

public:

    bool isSolved() const;

    const std::array<glm::ivec3, 26>& getCurrentPosition() const {return cubeletPos;}
    const std::array<glm::ivec3, 26>& getSolvedPosition() const {return solvedPosition;}
    int getCubeCount() const {return 26;}

    int getCubeletCount() const { return cubelet.size(); }
    glm::ivec3 getCubeletPosition(int index) const {
        if (index >= 0 && index < cubelet.size()) {
            return cubelet[index]->getGridPosition();
        }
        return glm::ivec3(0,0,0);
    }
    glm::ivec3 getSolvedPosition(int index) const {
        if (index >= 0 && index < 26) {
            return solvedPosition[index];
        }
        return glm::ivec3(0,0,0);
    }

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
    // void printCenterColors(RubiksCube* cube);

    void executeMove(const std::string& move) {
        if (move == "R")  startRotation('X', 1.0f, 90.0f);
        else if (move == "R'") startRotation('X', 1.0f, -90.0f);
        else if (move == "L")  startRotation('X', -1.0f, -90.0f);
        else if (move == "L'") startRotation('X', -1.0f, 90.0f);
        else if (move == "U")  startRotation('Y', 1.0f, 90.0f);
        else if (move == "U'") startRotation('Y', 1.0f, -90.0f);
        else if (move == "D")  startRotation('Y', -1.0f, -90.0f);
        else if (move == "D'") startRotation('Y', -1.0f, 90.0f);
        else if (move == "F")  startRotation('Z', 1.0f, 90.0f);
        else if (move == "F'") startRotation('Z', 1.0f, -90.0f);
        else if (move == "B")  startRotation('Z', -1.0f, -90.0f);
        else if (move == "B'") startRotation('Z', -1.0f, 90.0f);
        // Add more moves as needed
    }
    // Debugging helpers (public)
    void printAllCubelets() const;
    void printFaceCenters() const;



};

#endif // RUBIKSCUBE_H