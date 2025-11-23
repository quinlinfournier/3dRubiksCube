//
// Created by quinf on 11/22/2025.
//

#include "Solver.h"

Solver::Solver(RubiksCube *cube) {
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                facelet[face][row][col] = '?';
            }
        }
    }
}


void Solver::updateFaceRepresentation() {
    // Init States
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                facelet[face][row][col] = '?';
            }
        }
    }
}

char Solver::colorToChar(const color &c) {
    return '?';
}

void Solver::mapPieceToFace(glm::ivec3 pos, Cubelet* piece) {

}


void Solver::testCubeAccess(RubiksCube* cube) {
    if (!cube) {
        std::cout << "ERROR: Cube is null!" << std::endl;
        return;
    }

    std::cout << "=== CUBE SOLVER ACCESS TEST ===" << std::endl;

    // Use the correct method names and std::array reference
    const auto& currentPositions = cube->getCurrentPosition();
    const auto& solvedPositions = cube->getSolvedPosition();

    // Print first 3 pieces
    for (int i = 0; i < 3; i++) {
        std::cout << "Piece " << i << ": ("
                  << currentPositions[i].x << "," << currentPositions[i].y << "," << currentPositions[i].z
                  << ")" << std::endl;
    }

    std::cout << "=== ACCESS TEST COMPLETE ===" << std::endl;
}

void Solver::solve(RubiksCube* cube) {
    if (!cube) return;

    std::cout << "Starting auto-solve..." << std::endl;
    const auto& current = cube->getCurrentPosition();
    const auto& solved = cube->getSolvedPosition();

    // Check id already solved
    bool isSolved = true;
    for (int i = 0; i < 26; i++) {
        if (current[i] != solved[i]) {
            isSolved = false;
            break;
        }
    }

    std::vector<std::string> moves = generateSolution(current, solved);
    //test access
    // std::cout << "First piece - current: (" << current[0].x << "," << current[0].y << "," << current[0].z << ")" << std::endl;
    // std::cout << "First piece - solved: (" << solved[0].x << "," << solved[0].y << "," << solved[0].z << ")" << std::endl;

    for (const auto& move : moves) {
        cube-> executeMove(move);
    }
}

std::vector<std::string> Solver::generateSolution(const std::array<glm::ivec3, 26>& current,
                                                 const std::array<glm::ivec3, 26>& solved) {
    std::vector<std::string> moves;

    // Count misplaced pieces
    int misplacedCount = 0;
    for (int i = 0; i < 26; i++) {
        if (current[i] != solved[i]) {
            misplacedCount++;
        }
    }

    std::cout << "Found " << misplacedCount << " misplaced pieces" << std::endl;

    // BETTER APPROACH: Use standard solving method
    // Let's implement a simple "reverse scramble" approach first

    // For testing, let's do a specific sequence that we know works
    // This sequence: R U R' U' should return cube to original state if applied twice
    moves = {"R", "U", "R'", "U'", "R", "U", "R'", "U'"};

    std::cout << "Using test sequence to return to solved state" << std::endl;

    return moves;
}