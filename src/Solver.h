// Created by quinf on 11/22/2025.
//
#ifndef FINAL_PROJECT_QJFOURNI_SOLVER_H
#define FINAL_PROJECT_QJFOURNI_SOLVER_H

#include "RubiksCube.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <array>
#include <glm/glm.hpp>

enum SolverState {
    IDLE,
    SOLVING,
    WCCOMPLETE,
    F2LCOMPLETE,
    FAILED
};

class Solver {
private:
    // ------- F2L -------

    int currentF2LSlot = 0;  // 0-3 for the four F2L slots
    struct F2LPair {
        int cornerID;
        int edgeID;
        char color1;
        char color2;
        glm::ivec3 targetCornerPos;
        glm::ivec3 targetEdgePos;
    };

    // Get the 4 F2L pairs (after white cross is solved)
    std::vector<F2LPair> getF2LPairs() const;

    // Find a specific F2L corner piece
    int findF2LCorner(char color1, char color2) const;

    // Find a specific F2L edge piece
    int findF2LEdge(char color1, char color2) const;

    // Check if a corner is oriented correctly (white facing down)
    bool isCornerOrientedCorrectly(Cubelet* corner) const;

    // Check if an edge is oriented correctly
    bool isEdgeOrientedCorrectly(Cubelet* edge, char color1, char color2) const;

    // Check if an F2L pair is solved
    bool isF2LPairSolved(const F2LPair& pair) const;

    // Get corner colors in correct order
    void getCornerColors(Cubelet* corner, char& white, char& c1, char& c2) const;

    // Get which face a specific color is on for a corner
    Face getColorFace(Cubelet* piece, char targetColor) const;

    // Solve a single F2L pair
    std::vector<std::string> solveF2LPair(const F2LPair& pair) const;

    // Extract corner from slot if already inserted but wrong
    std::vector<std::string> extractCornerFromSlot(glm::ivec3 pos) const;

    // Extract edge from middle layer if stuck
    std::vector<std::string> extractEdgeFromMiddle(glm::ivec3 pos) const;

    // Pair up corner and edge in top layer
    std::vector<std::string> pairUpPieces(const F2LPair& pair) const;

    // Insert paired corner-edge into slot
    std::vector<std::string> insertF2LPair(const F2LPair& pair, glm::ivec3 cornerPos, glm::ivec3 edgePos) const;


    // Old
    RubiksCube* cube;

    mutable std::map<std::string,int> stuckCount;

    // Face Representation [face][row][col]
    char facelet[6][3][3];

    std::vector<std::string> moveQueue;
    std::vector<std::string> currentMoves;
    int currentStep = 0;
    int moveCounter = 0;
    const int MAX_MOVES = 100;
    char currentTargetColor = 'B';

    SolverState currentState = IDLE;

    mutable std::map<char, int> edgeStuckCounter;
    mutable std::map<char, std::vector<glm::ivec3>> previousStates;

    // Private helper methods
    char colorToChar(const color& c) const;
    int findWhiteEdge(char targetColor, const std::array<glm::ivec3, 26>& solved) const;

    bool isEdgeSolved(int pieceID, const std::array<glm::ivec3, 26>& current,
                      const std::array<glm::ivec3, 26>& solved, RubiksCube* cubePtr) const;

    std::vector<std::string> solveSingleWhiteEdge(char targetColor,
                                                 const std::array<glm::ivec3, 26>& current,
                                                 const std::array<glm::ivec3, 26>& solved,
                                                 RubiksCube* cubePtr) const;

    std::vector<std::string> solveFirstTwoLayers(char targetColor,
                                                 const std::array<glm::ivec3, 26>& current,
                                                 const std::array<glm::ivec3, 26>& solved,
                                                 RubiksCube* cubePtr) const;

    // New helper methods for simplified approach
    char getWhiteFace(Cubelet* piece) const;
    glm::ivec3 getTargetPosition(char targetColor) const;
    std::vector<std::string> extractAndRealign(char targetColor, char whiteFace) const;
    std::vector<std::string> handleBottomLayerCase(char targetColor, glm::ivec3 currentPos,
                                                  char whiteFace, glm::ivec3 targetPos) const;
    std::vector<std::string> alignBottomEdge(glm::ivec3 currentPos, glm::ivec3 targetPos) const;
    std::vector<std::string> extractToBottomLayer(glm::ivec3 currentPos, char whiteFace) const;

    std::vector<std::string> generateCycleBreakMoves(char targetColor, glm::ivec3 currentPos, char whiteFace) const {
        // Less disruptive cycle-breaking sequences
        // These move the piece to a known good state without destroying progress

        if (currentPos.y == 1) {
            // In top layer - use standard extraction but with different D moves
            if (targetColor == 'B' || targetColor == 'G') {
                return {"F'", "D'", "F", "D"}; // Extract with counter-rotation
            } else {
                return {"R'", "D", "R", "D'"}; // Extract with opposite rotation
            }
        } else if (currentPos.y == 0) {
            // In middle layer - extract to different bottom position
            if (currentPos.z == 1 || currentPos.z == -1) {
                return {"F'", "D'", "F", "D", "D"}; // Send to opposite side
            } else {
                return {"R'", "D", "R", "D", "D"}; // Send to different position
            }
        } else {
            // In bottom layer - rotate to break symmetry
            return {"D", "D"}; // 180° rotation often breaks cycles
        }
    }

public:
    Solver() = default;
    Solver(RubiksCube* cube);

    // State management
    bool isSolving() const { return currentState == SOLVING; }
    bool isComplete() const { return currentState == WCCOMPLETE; }
    bool isFailed() const { return currentState == FAILED; }
    bool isIdle() const { return currentState == IDLE; }

    bool hasNextMove() const {
        return !currentMoves.empty() || currentState == SOLVING;
    }

    // Getters
    SolverState getCurrentState() const { return currentState; }
    char getCurrentTargetColor() const { return currentTargetColor; }
    int getMoveCounter() const { return moveCounter; }
    int getCurrentStep() const { return currentStep; }

    char getFaceColor(Cubelet* piece, Face face) const;
    std::string getNextMove();

    // Setters
    void setState(SolverState state) {
        currentState = state;
    }
    void setCube(RubiksCube* newCube) {
        cube = newCube;
    }
    void reset() {
        currentState = IDLE;
        currentStep = 0;
        currentTargetColor = 'B';
        moveCounter = 0;
        currentMoves.clear();
        moveQueue.clear();
        edgeStuckCounter.clear();
        previousStates.clear();
    }

    // Debugging and testing
    void testCubeAccess(RubiksCube* cube);
    void solve(RubiksCube* cube);

    bool isWhiteCrossSolved() const;

    // Utility functions
    void printState() const {
        std::cout << "Solver State: ";
        switch(currentState) {
            case IDLE: std::cout << "IDLE"; break;
            case SOLVING: std::cout << "SOLVING"; break;
            case WCCOMPLETE: std::cout << "COMPLETE"; break;
            case FAILED: std::cout << "FAILED"; break;
        }
        std::cout << " | Target: W-" << currentTargetColor
                  << " | Moves: " << moveCounter
                  << " | Step: " << currentStep
                  << " | Queued: " << currentMoves.size()
                  << std::endl;
    }
};

#endif //FINAL_PROJECT_QJFOURNI_SOLVER_H