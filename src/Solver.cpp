//
// Created by quinf on 11/22/2025.
//

#include "Solver.h"
#include <iostream>
#include <cmath>
#include <map>

Solver::Solver(RubiksCube *cube) : cube(cube) {
    // Initialize all member variables
    currentState = IDLE;
    currentStep = 0;
    currentTargetColor = 'B';
    moveCounter = 0;

    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                facelet[face][row][col] = '?';
            }
        }
    }
}



char Solver::getFaceColor(Cubelet* piece, Face face) const {
    if (!piece) {
        return '?';
    }
    const color& c = piece->getFaceColor(face);
    return colorToChar(c);
}


std::string Solver::getNextMove() {
    if (currentState != SOLVING) {
        return "";
    }

    // Safety check to prevent infinite loops
    if (moveCounter++ > MAX_MOVES) {
        std::cout << "ERROR: Solver stuck in infinite loop! Aborting." << std::endl;
        currentState = FAILED;
        return "";
    }

    // If we have moves queued from previous step, return the next one
    if (!currentMoves.empty()) {
        std::string nextMove = currentMoves.front();
        currentMoves.erase(currentMoves.begin());
        std::cout << "Executing queued move: " << nextMove << std::endl;
        return nextMove;
    }

    // Generate next set of moves based on current cube state
    if (!cube) {
        std::cout << "ERROR: Cube is null!" << std::endl;
        currentState = FAILED;
        return "";
    }

    const auto& current = cube->getCurrentPosition();
    const auto& solved = cube->getSolvedPosition();

    // Try to solve the current target edge
    currentMoves = solveSingleWhiteEdge(currentTargetColor, current, solved, cube);

    if (!currentMoves.empty()) {
        std::string nextMove = currentMoves.front();
        currentMoves.erase(currentMoves.begin());
        std::cout << "Solving W-" << currentTargetColor << " edge, next move: " << nextMove << std::endl;
        return nextMove;
    }

    // Current edge is solved, check if we need to verify all edges
    std::cout << "W-" << currentTargetColor << " edge solved, moving to next." << std::endl;

    if (currentTargetColor == 'B') {
        currentTargetColor = 'R';
    }
    else if (currentTargetColor == 'R') {
        currentTargetColor = 'G';
    }
    else if (currentTargetColor == 'G') {
        currentTargetColor = 'O';
    }
    else if (currentTargetColor == 'O') {
        // We've processed all edges, now verify they're ALL solved
        bool allSolved = true;
        for (char color : {'B', 'R', 'G', 'O'}) {
            int pieceID = findWhiteEdge(color, solved);
            if (pieceID == -1 || !isEdgeSolved(pieceID, current, solved, cube)) {
                allSolved = false;
                std::cout << "W-" << color << " edge was disturbed, re-solving..." << std::endl;
                currentTargetColor = color;
                break;
            }
        }

        if (allSolved) {
            currentState = COMPLETE;
            std::cout << "White Cross Complete!" << std::endl;
            return "";
        }
    }

    std::cout << "Moving to next edge W-" << currentTargetColor << std::endl;
    return "";
}

char Solver::colorToChar(const color &c) const {
    struct Named { color col; char ch; };
    const std::vector<Named> palette = {
        {{1,1,1}, 'W'}, {{1,1,0}, 'Y'}, {{0,0,1}, 'B'},
        {{0,0.5f,0}, 'G'}, {{1,0,0}, 'R'}, {{1,0.5f,0}, 'O'}
    };
    float best = 1e9; char bestCh = '?';
    for (auto &n : palette) {
        float d = (c.red - n.col.red)*(c.red - n.col.red)
                + (c.green - n.col.green)*(c.green - n.col.green)
                + (c.blue - n.col.blue)*(c.blue - n.col.blue);
        if (d < best) { best = d; bestCh = n.ch; }
    }
    return bestCh;
}

void Solver::testCubeAccess(RubiksCube* cube) {
    if (!cube) {
        std::cout << "ERROR: Cube is null!" << std::endl;
        return;
    }

    std::cout << "=== CUBE SOLVER ACCESS TEST ===" << std::endl;

    const auto& currentPositions = cube->getCurrentPosition();
    const auto& solvedPositions = cube->getSolvedPosition();

    // 1. Find the ID of the piece that belongs at (1, 1, 1) - The White-Red-Blue Corner
    int targetPieceID = -1;
    glm::ivec3 solvedPos(1, 1, 1);
    for (int i = 0; i < 26; i++) {
        if (solvedPositions[i] == solvedPos) {
            targetPieceID = i;
            break;
        }
    }

    if (targetPieceID != -1) {
        std::cout << "Target Piece ID (W-R-B Corner) is: " << targetPieceID << std::endl;

        // 2. Get the current position and the actual Cubelet object
        glm::ivec3 currentPos = currentPositions[targetPieceID];
        Cubelet* piece = cube->getCubelet(currentPos); // Use the new function

        std::cout << "Piece currently at grid position: ("
                  << currentPos.x << "," << currentPos.y << "," << currentPos.z
                  << ")" << std::endl;

        // 3. Inspect the piece's current color/orientation
        if (piece) {
            std::cout << "Current Orientation (Faces of the piece itself):" << std::endl;

            // Use the Cubelet::Face enum to read the local colors
            char frontColor = getFaceColor(piece, FRONT); // Face facing the +Z direction
            char backColor = getFaceColor(piece, BACK);   // Face facing the -Z direction
            char rightColor = getFaceColor(piece, RIGHT); // Face facing the +X direction
            char leftColor = getFaceColor(piece, LEFT);   // Face facing the -X direction
            char upColor = getFaceColor(piece, UP);       // Face facing the +Y direction
            char downColor = getFaceColor(piece, DOWN);   // Face facing the -Y direction

            std::cout << "  - UP color:    " << upColor << " (Should be W)" << std::endl;
            std::cout << "  - FRONT color: " << frontColor << " (Should be B)" << std::endl;
            std::cout << "  - RIGHT color: " << rightColor << " (Should be R)" << std::endl;
            std::cout << "  - LEFT color:  " << leftColor << std::endl;
            std::cout << "  - DOWN color:  " << downColor << std::endl;
            std::cout << "  - BACK color:  " << backColor << std::endl;
        } else {
            std::cout << "ERROR: Could not find cubelet at current position ("
                      << currentPos.x << "," << currentPos.y << "," << currentPos.z << ")" << std::endl;
        }

    } else {
        std::cout << "ERROR: Could not find piece with solved position (1, 1, 1)." << std::endl;
    }

    std::cout << "=== ACCESS TEST COMPLETE ===" << std::endl;
}

void Solver::solve(RubiksCube* cube) {
    if (!cube) return;

    this->cube = cube;

    if (currentState == SOLVING) {
        std::cout << "Solver is already active" << std::endl;
        return;
    }

    std::cout << "Starting auto-solve..." << std::endl;
    currentState = SOLVING;
    currentStep = 0;
    currentTargetColor = 'B';
    currentMoves.clear();
    moveQueue.clear();
    moveCounter = 0;
}


int Solver::findWhiteEdge(char targetColor, const std::array<glm::ivec3, 26>& solved) const {
    // White edges are at positions with exactly one coordinate = 0 and Y = 1
    // Solved positions: (0,1,1)=W-B, (1,1,0)=W-R, (0,1,-1)=W-G, (-1,1,0)=W-O
    for (int i = 0; i < 26; i++) {
        glm::ivec3 pos = solved[i];
        // Check if it's an edge (one coord is 0, and another is ±1)
        int zeroCount = (pos.x == 0) + (pos.y == 0) + (pos.z == 0);
        if (zeroCount == 1 && pos.y == 1) {
            // This is a top edge in solved state
            // We need to identify which edge by checking the colors
            // For now, use position to determine color
            if (pos.z == 1 && targetColor == 'B') return i;      // W-B edge
            if (pos.x == 1 && targetColor == 'R') return i;      // W-R edge
            if (pos.z == -1 && targetColor == 'G') return i;     // W-G edge
            if (pos.x == -1 && targetColor == 'O') return i;     // W-O edge
        }
    }
    return -1;
}

// Check if an edge is solved (correct position AND orientation)
bool Solver::isEdgeSolved(int pieceID, const std::array<glm::ivec3, 26>& current,
                          const std::array<glm::ivec3, 26>& solved, RubiksCube* cubePtr) const {
    if (current[pieceID] != solved[pieceID]) return false;

    Cubelet* piece = cubePtr->getCubelet(current[pieceID]);
    if (!piece) return false;

    // Check if white is on top face
    char topColor = getFaceColor(piece, UP);
    return topColor == 'W';
}


std::vector<std::string> Solver::solveSingleWhiteEdge(char targetColor,
                                                     const std::array<glm::ivec3, 26>& current,
                                                     const std::array<glm::ivec3, 26>& solved,
                                                     RubiksCube* cubePtr) const {

    int pieceID = findWhiteEdge(targetColor, solved);
    if (pieceID == -1) {
        std::cout << "ERROR: Could not find W-" << targetColor << " edge!" << std::endl;
        return {};
    }

    glm::ivec3 currentPos = current[pieceID];
    Cubelet* piece = cubePtr->getCubelet(currentPos);
    if (!piece) {
        std::cout << "ERROR: Could not get cubelet at position!" << std::endl;
        return {};
    }

    // Check if edge is already solved
    if (isEdgeSolved(pieceID, current, solved, cubePtr)) {
        std::cout << "W-" << targetColor << " edge already solved." << std::endl;
        return {};
    }

    // *** STUCK COUNT MUST BE HERE TO CATCH ALL LOOPS ***
    int pid = findWhiteEdge(currentTargetColor, solved);
    std::string key = "";
    if (pid != -1) {
        glm::ivec3 curp = current[pid];
        Cubelet* p = cube->getCubelet(curp);
        char wf = getWhiteFace(p); // Use existing getWhiteFace helper
        key = std::string(1, currentTargetColor) + ":" +
              std::to_string(curp.x) + "," + std::to_string(curp.y) + "," + std::to_string(curp.z) + ":" +
              std::string(1, wf);
    }

    if (!key.empty()) {
        int &c = stuckCount[key];
        // Lowered threshold from 6 to 3 to catch 2-move loops faster
        if (c++ > 3) {
            std::cout << "DEBUG: detected stuck on " << key << ", forcing fallback rotation D'" << std::endl;
            stuckCount.clear();
            return {"D'"}; // Use D' to break the "D-move" loop
        }
    }
    // *** END STUCK COUNT BLOCK ***

    std::cout << "Processing W-" << targetColor << " edge at ("
              << currentPos.x << "," << currentPos.y << "," << currentPos.z
              << "), white on " << getWhiteFace(piece) << " face" << std::endl;

    if (getWhiteFace(piece) == '?') {
        std::cout << "ERROR: Cannot find white sticker!" << std::endl;
        return {};
    }

    glm::ivec3 targetPos;
    if (targetColor == 'B') targetPos = glm::ivec3(0, 1, 1);
    else if (targetColor == 'R') targetPos = glm::ivec3(1, 1, 0);
    else if (targetColor == 'G') targetPos = glm::ivec3(0, 1, -1);
    else if (targetColor == 'O') targetPos = glm::ivec3(-1, 1, 0);

    // CASE 1: Edge is in correct position but FLIPPED (wrong orientation)
    if (currentPos == targetPos) {
        std::cout << "W-" << targetColor << " is in correct position but white on " << getWhiteFace(piece) << " (should be on U)." << std::endl;
        if (targetColor == 'B') return {"F", "F"};
        else if (targetColor == 'R') return {"R", "R"};
        else if (targetColor == 'G') return {"B", "B"};
        else if (targetColor == 'O') return {"L", "L"};
    }

    // CASE 2: Edge is in the WRONG top layer position
    if (currentPos.y == 1 && currentPos != targetPos) {
        std::cout << "W-" << targetColor << " is in WRONG top layer position." << std::endl;
        if (currentPos.z == 1) return {"F", "F"};
        if (currentPos.z == -1) return {"B", "B"};
        if (currentPos.x == 1) return {"R", "R"};
        if (currentPos.x == -1) return {"L", "L"};
    }

    // CASE 3: Edge is in bottom layer (Y = -1)
    if (currentPos.y == -1) {
        glm::ivec3 targetBottom = glm::ivec3(targetPos.x, -1, targetPos.z);
        char whiteFace = getWhiteFace(piece);

        if (currentPos == targetBottom) {
            std::cout << "W-" << targetColor << " is aligned under target position." << std::endl;
            std::cout << "  White on face: " << whiteFace << std::endl;

            // Aligned and White is facing DOWN (correct for insertion)
            if (whiteFace == 'D') {
                std::cout << "White facing down - perfect orientation, inserting!" << std::endl;
                if (targetColor == 'B') return {"F", "F"};
                if (targetColor == 'R') return {"R", "R"};
                if (targetColor == 'G') return {"B", "B"};
                if (targetColor == 'O') return {"L", "L"};
            }
            // Aligned but White is on the SIDE (flipped)
            else {
                std::cout << "White facing side (" << whiteFace << "), moving piece to force re-alignment..." << std::endl;
                if (targetColor == 'B') return {"F", "R", "D'"};
                if (targetColor == 'R') return {"R", "F", "D'"};
                if (targetColor == 'G') return {"F", "R", "D'"};
                if (targetColor == 'O') return {"L", "B", "D'"};            }
        }
        else {
            // Piece is in bottom layer but not aligned
            std::cout << "W-" << targetColor << " in bottom layer but not aligned. Rotating..." << std::endl;
            return {"D"};
        }
    }

    // CASE 4: Edge is in middle layer (Y = 0)
    if (currentPos.y == 0) {
        std::cout << "W-" << targetColor << " is in middle layer. Extracting..." << std::endl;

        if (std::abs(currentPos.z) == 1) {
            if (currentPos.z == 1) return {"F"};
            else return {"B"};
        }
        if (std::abs(currentPos.x) == 1) {
            if (currentPos.x == 1) return {"R"};
            else return {"L"};
        }

        std::cout << "ERROR: Middle layer piece at unhandled position." << std::endl;
        return {"D"}; // Fallback
    }

    // Ultimate fallback
    std::cout << "Using fallback rotation for W-" << targetColor << std::endl;
    return {"D'"};
}

char Solver::getWhiteFace(Cubelet* piece) const {
    if (!piece) return '?';

    if (getFaceColor(piece, UP) == 'W')    return 'U';
    if (getFaceColor(piece, DOWN) == 'W')  return 'D';
    if (getFaceColor(piece, FRONT) == 'W') return 'F';
    if (getFaceColor(piece, RIGHT) == 'W') return 'R';
    if (getFaceColor(piece, BACK) == 'W')  return 'B';
    if (getFaceColor(piece, LEFT) == 'W')  return 'L';

    return '?';
}

std::vector<std::string> Solver::solveSingleEdge(char targetColor,const std::array<glm::ivec3, 26>& current,const std::array<glm::ivec3, 26>& solved,RubiksCube* cubePtr) const {
    return {};
}

