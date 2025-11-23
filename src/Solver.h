//
// Created by quinf on 11/22/2025.
//
#include "RubiksCube.h"
#include <vector>
#include <string>
#include <memory>

#ifndef FINAL_PROJECT_QJFOURNI_SOLVER_H
#define FINAL_PROJECT_QJFOURNI_SOLVER_H


class Solver {
private:
    RubiksCube* cube;
    // Faces
    enum Face {
        UP = 0, // White
        FRONT = 1, // Blue
        RIGHT = 2, // Red
        BACK = 3,  // Green
        LEFT = 4,  // Orange
        DOWN = 5   // Yellow
    };

    // Face Representation [face][row][col]
    char facelet[6][3][3];

    // Face mapping
    void updateFaceRepresentation();
    char colorToChar(const color& c);
    void mapPieceToFace(glm::ivec3 pos, Cubelet* piece);
    std::vector<std::string> generateSolution(const glm::ivec3* current, const glm::ivec3 solved);

public:
    Solver() = default;
    Solver(RubiksCube* cube);


    // Debugging
    void testCubeAccess(RubiksCube* cube);
    void solve(RubiksCube* cube);
    std::vector<std::string> generateSolution(const std::array<glm::ivec3, 26>& current,const std::array<glm::ivec3, 26>& solved);
};


#endif //FINAL_PROJECT_QJFOURNI_SOLVER_H