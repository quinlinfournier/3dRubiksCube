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

enum OLLState { 
    DOT, 
    L_SHAPE, 
    LINE_SHAPE, 
    CROSS_SHAPE 
};

class Solver {
public:
    Solver() = default;
    Solver(RubiksCube* cube);

    // Main interface
    void solve(RubiksCube* cube);
    std::string getNextMove();

    // State queries
    bool isSolving() const { return currentState == SOLVING; }
    bool isComplete() const { return currentState == WCCOMPLETE; }
    bool isFailed() const { return currentState == FAILED; }
    bool isIdle() const { return currentState == IDLE; }
    bool hasNextMove() const { return !currentMoves.empty() || currentState == SOLVING; }
    
    SolverState getCurrentState() const { return currentState; }
    char getCurrentTargetColor() const { return currentTargetColor; }
    int getMoveCounter() const { return moveCounter; }
    int getCurrentStep() const { return currentStep; }

    void setState(SolverState state) { currentState = state; }
    void setCube(RubiksCube* newCube) { cube = newCube; }

    bool fullySolved = false;

    // Debug controls
    void toggleDebugFreeze() { 
        debugFreeze = !debugFreeze; 
        std::cout << "Debug freeze " << (debugFreeze ? "ENABLED" : "DISABLED") << std::endl;
    }
    void toggleStepThroughMode() { 
        stepThroughMode = !stepThroughMode; 
        std::cout << "Step-through mode " << (stepThroughMode ? "ENABLED" : "DISABLED") << std::endl;
    }
    void resumeSolving() { 
        debugFreeze = false; 
        std::cout << "Resuming solving..." << std::endl;
    }
    void advanceOneStep() {
        if (stepThroughMode || debugFreeze) {
            movesSinceLastFreeze = 1;
            std::cout << "Advancing one step..." << std::endl;
        }
    }
    bool isDebugFrozen() const { return debugFreeze; }
    bool isStepThrough() const { return stepThroughMode; }

private:
    // Core data
    RubiksCube* cube;
    SolverState currentState = IDLE;
    
    int currentStep = 0;
    int moveCounter = 0;
    const int MAX_MOVES = 100;
    
    std::vector<std::string> currentMoves;
    std::vector<std::string> moveQueue;
    
    // White Cross state
    char currentTargetColor = 'B';
    mutable std::map<std::string, int> stuckCount;
    mutable std::map<char, int> edgeStuckCounter;
    mutable std::map<char, std::vector<glm::ivec3>> previousStates;
    
    // F2L state
    int currentF2LSlot = 0;
    bool cubeRotationDone = false;
    bool cornersAnalyzed = false;
    bool verificationDone = false;
    int f2lUMoveCounter = 0;
    
    struct F2LPair {
        int cornerID;
        int edgeID;
        char color1;
        char color2;
        glm::ivec3 targetCornerPos;
        glm::ivec3 targetEdgePos;
    };
    
    struct CubeFace {
        glm::ivec3 normal;
        std::string name;
        glm::vec3 color;
    };
    
    std::vector<CubeFace> faces;
    glm::mat3 orientation = glm::mat3(1.0f);
    
    Face orientationMap[6] = {UP, DOWN, LEFT, RIGHT, FRONT, BACK};
    Face cubeFaceMap[6] = {(Face)4, (Face)5, (Face)0, (Face)1, (Face)2, (Face)3};
    
    char facelet[6][3][3];
    
    // OLL/PLL state
    bool orientationResetDone = false;
    int solveCheck = 0;
    bool baseEdge = false;

    std::map<std::string, int> edgeStuckCount;  // For edge tracking
    std::map<std::string, int> cornerStuckCount; // For corner tracking
    
    // Debug state
    bool debugFreeze = false;
    bool stepThroughMode = false;
    int movesSinceLastFreeze = 0;

    // Utility functions
    char colorToChar(const color& c) const;
    char getFaceColor(const Cubelet* piece, Face face) const;
    char getWhiteFace(Cubelet* piece) const;
    Face getColorFace(Cubelet* piece, char targetColor) const;
    std::string faceToString(Face face) const;
    
    bool sameColor(const color& c1, const color& c2) const {
        const float epsilon = 0.02f;
        return std::abs(c1.red - c2.red) < epsilon &&
               std::abs(c1.green - c2.green) < epsilon &&
               std::abs(c1.blue - c2.blue) < epsilon;
    }

    // White Cross
    int findWhiteEdge(char targetColor, const std::array<glm::ivec3, 26>& solved) const;
    bool isEdgeSolved(int pieceID, const std::array<glm::ivec3, 26>& current,
                      const std::array<glm::ivec3, 26>& solved, RubiksCube* cubePtr) const;
    std::vector<std::string> solveSingleWhiteEdge(char targetColor,
                                                 const std::array<glm::ivec3, 26>& current,
                                                 const std::array<glm::ivec3, 26>& solved,
                                                 RubiksCube* cubePtr) const;
    bool isWhiteCrossSolved() const;
    bool isYellowCrossSolved() const;

    // F2L
    std::vector<F2LPair> getF2LPairs() const;
    int findF2LCorner(char color1, char color2) const;
    int findF2LEdge(char color1, char color2) const;
    bool isF2LPairSolved(const F2LPair& pair) const;
    bool isCornerOrientedCorrectly(Cubelet* corner) const;
    
    std::vector<std::string> solveF2LPair(const F2LPair& pair, glm::ivec3 cornerPos,
                                          glm::ivec3 edgePos, Cubelet* corner, Cubelet* edge) const;
    std::vector<std::string> extractCornerFromSlot(glm::ivec3 pos) const;
    std::vector<std::string> extractEdgeFromMiddle(glm::ivec3 pos) const;
    std::vector<std::string> insertF2LPair(const F2LPair& pair, glm::ivec3 cornerPos, 
                                           glm::ivec3 edgePos, Cubelet* corner, Cubelet* edge) const;

    // OLL
    OLLState detectOLLState();
    bool isLCorrectOrientation();
    bool isLineHorizontal();
    std::vector<std::string> solveOLLCross();
    void resetOrientationAfterF2L();

    // PLL
    std::vector<std::string> solveLastLayerEdges();
    std::vector<std::string> findCorrectEdgePair();
    bool isEdgeAligned(int edgeIndex);
    bool isCornerCorrectlyOriented(int cornerIndex);
    bool isCornerInCorrectLocation(int cornerIndex);
    int countCorrectCornersLocations();
    std::vector<std::string> orientCorners();


    bool cornerIsCorrect(int idx);
    bool cornerMatches(int idx);
    bool isCornerCorrectlyOriented(glm::ivec3 pos) const;

    color centerColor(Face f);

    // PLL
    bool edgeMatchesCenter_Fixed(int edgeIndex);
    int countAlignedEdges_Fixed();
    std::vector<std::string> solveLastLayerEdges_Fixed();
    
    bool cornerInCorrectLocation_Fixed(glm::ivec3 pos);
    int countCorrectCornerOrientations();
    int countCorrectCorners_Fixed();
    std::vector<std::string> solveLastLayerCorners_Fixed();
    bool areCornersSolved_Fixed();


    // Debug
    void debugFaceIndexOrder();

    void debugCenterColors();

};

#endif // FINAL_PROJECT_QJFOURNI_SOLVER_H
