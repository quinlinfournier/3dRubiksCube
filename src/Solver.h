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
    SOLVED,
    FAILED
};
enum OLLState { DOT, L_SHAPE, LINE_SHAPE, CROSS_SHAPE };


class Solver {
private:
    // ------- F2L -------

    struct CubeFace {
        glm::ivec3 normal; // vector pointing out from the face
        std::string name; // U, d, ...
        glm::vec3 color; // center
    };

    Face orientationMap[6] = {UP, DOWN, LEFT, RIGHT, FRONT, BACK};

    std::vector<CubeFace> faces;

    glm::mat3 orientation = glm::mat3(1.0f); // identity = standard orientation

    int currentF2LSlot = 0;  // 0-3 for the four F2L slots
    struct F2LPair {
        int cornerID;
        int edgeID;
        char color1;
        char color2;
        glm::ivec3 targetCornerPos;
        glm::ivec3 targetEdgePos;
    };

    bool cubeRotationDone = false;
    bool cornersAnalyzed = false;
    bool verificationDone = false;
    int f2lUMoveCounter = 0;
    bool orientationResetDone = false;
    int solveCheck = 0;


    // Get the 4 F2L pairs (after white cross is solved)
    std::vector<F2LPair> getF2LPairs() const;

    // Find a specific F2L corner piece
    int findF2LCorner(char color1, char color2) const;

    // Find a specific F2L edge piece
    int findF2LEdge(char color1, char color2) const;

    // Check if a corner is oriented correctly (white facing down)
    bool isCornerOrientedCorrectly(Cubelet* corner) const;

    // Check if an edge is oriented correctly

    // Check if an F2L pair is solved
    bool isF2LPairSolved(const F2LPair& pair) const;
    // Get corner colors in correct order

    // Get which face a specific color is on for a corner
    Face getColorFace(Cubelet* piece, char targetColor) const;

    // Solve a single F2L pair
    std::vector<std::string> solveF2LPair(const F2LPair& pair,glm::ivec3 cornerPos,glm::ivec3 edgePos,Cubelet* corner,Cubelet* edge) const;
    // Extract corner from slot if already inserted but wrong
    std::vector<std::string> extractCornerFromSlot(glm::ivec3 pos) const;

    // Extract edge from middle layer if stuck
    std::vector<std::string> extractEdgeFromMiddle(glm::ivec3 pos) const;

    std::vector<std::string> insertF2LPair(const F2LPair& pair, glm::ivec3 cornerPos, glm::ivec3 edgePos, Cubelet* corner, Cubelet* edge) const;

    std::string faceToString(Face face) const;
    std::string getSlotName(glm::ivec3 targetPos) const;

    void debugAllFaceColors() const;
    void debugSpecificCubelet(glm::ivec3 pos) const;
    void debugAllF2LPositions() const;

    // Solve Last Layer
    bool isLCorrectOrientation();
    bool isLineHorizontal();

    bool isYellowCrossSolved() const;

    void rotateVirtualX();


    OLLState detectOLLState();

    std::vector<std::string> solveOLLCross();

    int countAlignedEdges();
    bool edgeMatchesCenter(int face);

    bool cornerMatches(int idx);

    std::vector<std::string> solveCornerPermutation();

    std::vector<std::string> orientCornerOnce();

    void resetOrientationAfterF2L();

    std::vector<std::string> solveCornerOrientation();

    // Small helper — compare two colors with tolerance
    bool sameColor(const color& c1, const color& c2) const {
        const float epsilon = 0.01f;
        return std::abs(c1.red - c2.red) < epsilon &&
               std::abs(c1.green - c2.green) < epsilon &&
               std::abs(c1.blue - c2.blue) < epsilon;
    }

    Face virtualUpFace(const Cubelet* c, RubiksCube* cube) {
        for (int f = 0; f < 6; f++) {
            if (c->getFaceColor((Face)f).blue == 0 &&
                c->getFaceColor((Face)f).red == 1 &&
                c->getFaceColor((Face)f).green == 1)
            {
                // That’s yellow (1,1,0)
                return (Face)f;
            }
        }
        return UP; // fallback
    }

    glm::ivec3 centerPos(Face f);

    bool cornerIsCorrect(int idx);

    int findCorrectCornerIndex();


    std::vector<std::string> solveLastLayerEdges();
    bool oppositeCorrect();
    bool adjacentCorrect();
    int countPermutedEdges();

    // Solve the last 4 corners
    bool cornerInCorrectLocation(glm::ivec3 pos);

    int countCorrectCorners();

    glm::ivec3 getFirstCorrectCorner();

    std::vector<std::string> solveLastLayerCorners();

    color centerColor(Face f);

    bool areCornersSolved();


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

    std::vector<std::string>  findCorrectEdgePair();

    bool cornerMatchesCenters(const glm::ivec3 &pos);

    std::vector<std::string> finalAUF();



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

    // Coordinate-based queries
    char getColorAtPosition(const glm::ivec3& pos, const glm::ivec3& faceDir) const;
    glm::ivec3 findFaceWithColor(const glm::ivec3& pos, char targetColor) const;

    // F2L solving
    std::vector<std::string> solveNextSlot();
    bool isSlotSolved(int slotIndex) const;

    // Cube rotation handling
    void applyCubeRotation(const std::string& move);
    glm::ivec3 rotatePosition(const glm::ivec3& pos) const;

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

    char getFaceColor(const Cubelet* piece, Face face) const;
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
    void debugCornerAnalysis() const;
    void printColorRGB(const color& c) const;
    void debugActualColors() const;



};

#endif //FINAL_PROJECT_QJFOURNI_SOLVER_H