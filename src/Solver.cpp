//
// Created by quinf on 11/22/2025.
//

#include "Solver.h"
#include <iostream>
#include <cmath>
#include <map>
#include <utility>
#include <iomanip>
#include <algorithm>
/*
 * Solver
 * Automaticly Solves Rubiks Cube in 4 steps
 * 0. WHITE CROSS
 * 1. F2L - First two layers (the 4 corners
 * 2. OLL - Yellow Cross
 * 3. PLL - Last Corners
 *
 */
Solver::Solver(RubiksCube *cube) : cube(cube) {
    // Initialize all member variables
    currentState = IDLE;
    currentStep = 0;
    currentTargetColor = 'B';
    moveCounter = 0;
    currentF2LSlot = 0;
    stuckCount.clear();
    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                facelet[face][row][col] = '?';
            }
        }
    }
    debugFaceIndexOrder();
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

    stuckCount.clear();
}

std::string Solver::getNextMove() {
    if (currentState != SOLVING) {
        return "";
    }

    // Execute queued moves first
    if (!currentMoves.empty()) {
        std::string nextMove = currentMoves.front();
        currentMoves.erase(currentMoves.begin());
        if (stepThroughMode) movesSinceLastFreeze = 0;
        return nextMove;
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

    // ====== WHITE CROSS (Step 0) ======
    if (currentStep == 0) {
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
                // FINAL VERIFICATION: Make sure white cross is actually visually solved
                bool visuallySolved = true;
                for (char color : {'B', 'R', 'G', 'O'}) {
                    int pieceID = findWhiteEdge(color, solved);
                    glm::ivec3 pos = current[pieceID];
                    Cubelet* piece = cube->getCubelet(pos);
                    if (!piece || getFaceColor(piece, UP) != 'W') {
                        visuallySolved = false;
                        std::cout << "FINAL CHECK FAILED: W-" << color << " not solved! Restarting white cross..." << std::endl;
                        currentTargetColor = 'B';  // Start over from first edge
                        break;
                    }
                }

                if (visuallySolved) {
                    std::cout << "==================================" << std::endl;
                    std::cout << "WHITE CROSS COMPLETE!" << std::endl;
                    std::cout << "==================================" << std::endl;
                    currentStep = 1;  // Move to F2L
                    currentF2LSlot = 0;
                    std::cout << "Starting F2L..." << std::endl;
                    return "";  // Return empty to trigger next step on next call
                } else {
                    std::cout << "White cross verification failed, restarting from W-B..." << std::endl;
                    currentTargetColor = 'B';  // Start over
                    currentMoves.clear();  // Clear any queued moves
                }
            }
        }

        std::cout << "Moving to next edge W-" << currentTargetColor << std::endl;
        return "";
    }

    // ====== F2L (Step 1) ======
    if (currentStep == 1) {
        // F2L Solving Phase
        std::cout << "\n=== F2L STEP ===" << std::endl;

        // If we have moves queued, execute them
        if (!currentMoves.empty()) {
            std::string nextMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());
            std::cout << "Executing: " << nextMove << std::endl;
            return nextMove;
        }

        // First: Rotate cube for better view
        if (!cubeRotationDone) {
            std::cout << "Rotating cube for F2L view..." << std::endl;
            currentMoves = {"R", "X", "L'","R", "X", "L'"};
            cubeRotationDone = true;
            debugFaceIndexOrder();
            return getNextMove(); // Recursively get first move
        }

        // Check if all F2L slots are solved
        if (currentF2LSlot >= 4) {
            std::cout << "\n==================================" << std::endl;
            std::cout << " F2L COMPLETE! " << std::endl;
            std::cout << "==================================" << std::endl;
            currentStep = 2; // Move to next phase
            return "";
        }

        // Get the target F2L pair for current slot
        std::vector<F2LPair> allPairs = getF2LPairs();

        if (currentF2LSlot >= allPairs.size()) {
            std::cout << "ERROR: F2L slot out of range" << std::endl;
            currentState = FAILED;
            return "";
        }

        F2LPair targetPair = allPairs[currentF2LSlot];

        // Find the actual pieces
        targetPair.cornerID = findF2LCorner(targetPair.color1, targetPair.color2);
        targetPair.edgeID = findF2LEdge(targetPair.color1, targetPair.color2);

        if (targetPair.cornerID == 1 || targetPair.edgeID == 1) {
            std:cout  << "ERROR: Could not find F2L pieces for slot " << currentF2LSlot << std::endl;
            std::cout << "  Target pair: " << targetPair.color1 << "-" << targetPair.color2 << std::endl;
            currentState = FAILED;
            return "";
        }

        // Get current positions
        const auto& posit = cube->getCurrentPosition();
        glm::ivec3 cornerPos = posit[targetPair.cornerID];
        glm::ivec3 edgePos = posit[targetPair.edgeID];

        Cubelet* corner = cube->getCubelet(cornerPos);
        Cubelet* edge = cube->getCubelet(edgePos);

        if (!corner || !edge) {
            std::cout << "ERROR: Could not get cubelet objects" << std::endl;
            currentState = FAILED;
            return "";
        }

        // Debug info
        std::cout << "\n--- F2L Slot " << currentF2LSlot + 1 << " ---" << std::endl;
        std::cout << "Target: " << targetPair.color1 << "-" << targetPair.color2 << std::endl;
        std::cout << "Corner at: (" << cornerPos.x << "," << cornerPos.y << "," << cornerPos.z << ")" << std::endl;
        std::cout << "Edge at: (" << edgePos.x << "," << edgePos.y << "," << edgePos.z << ")" << std::endl;

        if (isF2LPairSolved(targetPair)) {
            std::cout << "Slot Solved Moving to Next Slot..." << std::endl;
            currentF2LSlot++;
            return "U";
        }

        // Generate solution based on piece positions/orientations
        currentMoves = solveF2LPair(targetPair, cornerPos, edgePos, corner, edge);


        if (!currentMoves.empty()) {

            // DOUBLE CHECK #2
            if (isF2LPairSolved(targetPair)) {
                std::cout << "[DOUBLE CHECK] Pair solved after move generation. Clearing moves." << std::endl;
                currentMoves.clear();
                currentF2LSlot++;
                return "U";
            }

            std::string firstMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());

            std::cout << "Solving with: " << firstMove;
            for (size_t i = 0; i < currentMoves.size(); i++) {
                std::cout << " " << currentMoves[i];
            }
            std::cout << std::endl;

            return firstMove;
        }


        // If no moves generated but not solved, rotate and try again
        std::cout << "No solution found, rotating U..." << std::endl;
        return "";
    }

    // ====== OLL (Step 2) ======
    if (currentStep == 2) {
        std::cout << "OLL Step" << std::endl;
        currentMoves = solveOLLCross();
        if (!currentMoves.empty()) {
            std::string nextMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());
            std::cout << "OLL Cross move: " << nextMove << std::endl;
            return nextMove;
        }

        // OLL cross is complete, check if it's really solved
        if (detectOLLState() == CROSS_SHAPE) {
            std::cout << "==================================\n";
            std::cout << " OLL CROSS COMPLETE!\n";
            std::cout << "==================================\n";
            currentStep = 3;  // Move to PLL (edge permutation)
            return "";
        }
        return "";
    }

    // ====== PLL - Edge Permutation (Step 3) ======
    if (currentStep == 3) {
        if (!orientationResetDone) {
            resetOrientationAfterF2L();
            orientationResetDone = true;
        }

        std::cout << "=== PLL EDGE PERMUTATION ===" << std::endl;

        currentMoves = solveLastLayerEdges_Fixed();

        if (countAlignedEdges_Fixed() == 4) {
            std::cout << "PLL EDGES COMPLETE!" << std::endl;
            currentStep = 4;
            return "";
        }
    }

    // STEP 4: PLL CORNERS
    if (currentStep == 4) {
        std::cout << "=== PLL CORNER PERMUTATION ===" << std::endl;
        if (fullySolved) {
            currentMoves = {"R'", "X'", "L","R'", "X'", "L"};

            std::cout << "CUBE SOLVED!" << std::endl;
            currentState = SOLVED;
            return "";
        }

        currentMoves = solveLastLayerCorners_Fixed();
    }

    return "";
}

char Solver::getFaceColor(const Cubelet* piece, Face face) const {
    if (!piece) {
        return '?';
    }
    const color& c = piece->getFaceColor(face);
    return colorToChar(c);
}

bool Solver::isWhiteCrossSolved() const {
    const auto& current = cube->getCurrentPosition();
    const auto& solved = cube->getSolvedPosition();

    for (char color : {'B', 'R', 'G', 'O'}) {
        int pieceID = findWhiteEdge(color, solved);
        if (pieceID == -1) return false;

        glm::ivec3 pos = current[pieceID];
        Cubelet* piece = cube->getCubelet(pos);
        if (!piece) return false;

        // Check position AND orientation
        if (pos != solved[pieceID]) return false;
        if (getFaceColor(piece, UP) != 'W') return false;
    }
    return true;
}
bool Solver::isYellowCrossSolved() const {
    const auto& current = cube->getCurrentPosition();
    const auto& solved = cube->getSolvedPosition();

    for (char color : {'B', 'R', 'G', 'O'}) {
        int pieceID = findWhiteEdge(color, solved);
        if (pieceID == -1) return false;

        glm::ivec3 pos = current[pieceID];
        Cubelet* piece = cube->getCubelet(pos);
        if (!piece) return false;

        // Check position AND orientation
        if (pos != solved[pieceID]) return false;
        if (getFaceColor(piece, UP) != 'Y') return false;
    }
    return true;
}

char Solver::colorToChar(const color &c) const {
    // FIRST check if it's black (internal face)
    if (c.red == 0.0f && c.green == 0.0f && c.blue == 0.0f) {
        return '?';  // Internal face, no sticker
    }

    // Then check actual colors with tolerance
    struct Named { color col; char ch; };
    const std::vector<Named> palette = {
        {{1.0f, 1.0f, 1.0f}, 'W'},
        {{1.0f, 1.0f, 0.0f}, 'Y'},
        {{0.0f, 0.0f, 1.0f}, 'B'},
        {{0.0f, 0.5f, 0.0f}, 'G'},
        {{1.0f, 0.0f, 0.0f}, 'R'},
        {{1.0f, 0.5f, 0.0f}, 'O'}
    };

    float best = 1e9;
    char bestCh = '?';

    for (const auto &n : palette) {
        float d = (c.red - n.col.red) * (c.red - n.col.red)
                + (c.green - n.col.green) * (c.green - n.col.green)
                + (c.blue - n.col.blue) * (c.blue - n.col.blue);
        if (d < best) {
            best = d;
            bestCh = n.ch;
        }
    }
    return bestCh;
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

// Check if an edge is solved
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

    int pid = findWhiteEdge(currentTargetColor, solved);
    std::string key = "";
    if (pid != -1) {
        glm::ivec3 curp = current[pid];
        Cubelet* p = cube->getCubelet(curp);
        char wf = getWhiteFace(p);
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
        if (targetColor == 'B') return {"F"};
        else if (targetColor == 'R') return {"R"};
        else if (targetColor == 'G') return {"B"};
        else if (targetColor == 'O') return {"L"};
    }

    // CASE 2: Edge is in the WRONG top layer position
    if (currentPos.y == 1 && currentPos != targetPos) {
        std::cout << "W-" << targetColor << " is in WRONG top layer position." << std::endl;
        if (currentPos.z == 1) return {"F"};
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
            std::cout << "  Current position: (" << currentPos.x << "," << currentPos.y << "," << currentPos.z << ")" << std::endl;
            std::cout << "  White on face: " << whiteFace << std::endl;

            // CASE A: White facing DOWN - Perfect orientation, just insert
            if (whiteFace == 'D') {
                std::cout << "White facing down - perfect orientation, inserting!" << std::endl;
                if (targetColor == 'B') return {"F","F"};  // Edge at (0,-1,1)
                if (targetColor == 'R') return {"R", "R"};  // Edge at (1,-1,0)
                if (targetColor == 'G') return {"B", "B"};  // Edge at (0,-1,-1)
                if (targetColor == 'O') return {"L", "L"};  // Edge at (-1,-1,0)
            }

 // CASE B: White facing RIGHT
        else if (whiteFace == 'R') {
            std::cout << "White facing RIGHT" << std::endl;

            // Edge at (0,-1,1) - bottom front, white facing right
            if (currentPos.x == 0 && currentPos.z == 1) {
                std::cout << "  Position: Bottom-Front, solving W-B edge" << std::endl;
                return {"R"};
            }
            // Edge at (1,-1,0) - bottom right, white facing right
            else if (currentPos.x == 1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Right, solving W-R edge" << std::endl;
                return {"R"};
            }
            // Edge at (0,-1,-1) - bottom back, white facing right
            else if (currentPos.x == 0 && currentPos.z == -1) {
                std::cout << "  Position: Bottom-Back, solving W-G edge" << std::endl;
                return {"R"};
            }
            // Edge at (-1,-1,0) - bottom left, white facing right
            else if (currentPos.x == -1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Left, solving W-O edge" << std::endl;
                return {"R"};
            }
        }

            // CASE C: White facing BACK - Need to check WHERE edge is
        else if (whiteFace == 'B') {
            std::cout << "White facing BACK" << std::endl;

            // Edge at (0,-1,1) - bottom front, white facing back
            if (currentPos.x == 0 && currentPos.z == 1) {
                std::cout << "  Position: Bottom-Front, solving W-B edge" << std::endl;
                return {"D", "R", "D'", "R", "R"};
            }
            // Edge at (1,-1,0) - bottom right, white facing back
            else if (currentPos.x == 1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Right, solving W-R edge" << std::endl;
                return {"B", "D'", "B'", "D", "R", "R"};
            }
            // Edge at (0,-1,-1) - bottom back, white facing back
            else if (currentPos.x == 0 && currentPos.z == -1) {
                std::cout << "  Position: Bottom-Back, solving W-G edge" << std::endl;
                return {"B", "D'", "B", "D"};
            }
            // Edge at (-1,-1,0) - bottom left, white facing back
            else if (currentPos.x == -1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Left, solving W-O edge" << std::endl;
                return {"B", "D", "B'", "D'", "L", "L"};
            }
        }

        // CASE D: White facing FRONT
        else if (whiteFace == 'F') {
            std::cout << "White facing FRONT" << std::endl;

            // Edge at (0,-1,1) - bottom front, white facing front (toward front)
            if (currentPos.x == 0 && currentPos.z == 1) {
                std::cout << "  Position: Bottom-Front, solving W-B edge" << std::endl;
                // Instead of just F, we need to extract it properly to top layer
                return {"F", "F"};
            }
            // Edge at (1,-1,0) - bottom right, white facing front
            else if (currentPos.x == 1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Right, solving W-R edge" << std::endl;
                return {"F", "D'", "F'", "D"};
            }
            // Edge at (0,-1,-1) - bottom back, white facing front (away from back)
            else if (currentPos.x == 0 && currentPos.z == -1) {
                std::cout << "  Position: Bottom-Back, solving W-G edge" << std::endl;
                return {"F", "D'", "F'", "D"};
            }
            // Edge at (-1,-1,0) - bottom left, white facing front
            else if (currentPos.x == -1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Left, solving W-O edge" << std::endl;
                return {"F", "D'", "F'", "D"};
            }
        }

        // CASE E: White facing LEFT
        else if (whiteFace == 'L') {
            std::cout << "White facing LEFT" << std::endl;

            // Edge at (0,-1,1) - bottom front, white facing left
            if (currentPos.x == 0 && currentPos.z == 1) {
                std::cout << "  Position: Bottom-Front, solving W-B edge" << std::endl;
                return {"L"};
            }
            // Edge at (1,-1,0) - bottom right, white facing left (toward center)
            else if (currentPos.x == 1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Right, solving W-R edge" << std::endl;
                return {"L"};
            }
            // Edge at (0,-1,-1) - bottom back, white facing left
            else if (currentPos.x == 0 && currentPos.z == -1) {
                std::cout << "  Position: Bottom-Back, solving W-G edge" << std::endl;
                return {"L"};
            }
            // Edge at (-1,-1,0) - bottom left, white facing left (away from center)
            else if (currentPos.x == -1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Left, solving W-O edge" << std::endl;
                return {"L"};
            }
        }

        // CASE F: White facing UP
        else if (whiteFace == 'U') {
            std::cout << "ERROR: White facing UP in bottom layer - moving to reorient" << std::endl;
            return {"D"};
        }

        // FALLBACK: Unknown orientation
        else {
            std::cout << "White facing unknown (" << whiteFace << "), rotating bottom..." << std::endl;
            return {"D"};
        }
    }
    // Edge not aligned
    else {
        std::cout << "W-" << targetColor << " in bottom layer but not aligned. Rotating..." << std::endl;
        return {"D"};
    }


    }
// CASE 4: Middle Layer
if (currentPos.y == 0) {
    std::cout << "=== MIDDLE LAYER EXTRACTION ===" << std::endl;
    std::cout << "W-" << targetColor << " is in middle layer." << std::endl;
    std::cout << "  Position: (" << currentPos.x << "," << currentPos.y << "," << currentPos.z << ")" << std::endl;

    char whiteFace = getWhiteFace(piece);
    std::cout << "  White facing: " << whiteFace << std::endl;

    // Front-Right edge (1,0,1)
    if (currentPos.x == 1 && currentPos.z == 1) {
        std::cout << "  Location: Front-Right edge (1,0,1)" << std::endl;
        if (whiteFace == 'F') {
            std::cout << "  → Using R to extract (perpendicular to F)" << std::endl;
            return {"R"};  // Use perpendicular move
        }
        else {
            std::cout << "  → Using F to extract (perpendicular to R)" << std::endl;
            return {"F"};  // Use perpendicular move
        }
    }
    // Front-Left edge (-1,0,1)
    else if (currentPos.x == -1 && currentPos.z == 1) {
        std::cout << "  Location: Front-Left edge (-1,0,1)" << std::endl;
        if (whiteFace == 'F') {
            std::cout << "  → Using L to extract (perpendicular to F)" << std::endl;
            return {"L"};  // Use perpendicular move
        }
        else {
            std::cout << "  → Using F to extract (perpendicular to L)" << std::endl;
            return {"F"};  // Use perpendicular move
        }
    }
    // Back-Right edge (1,0,-1)
    else if (currentPos.x == 1 && currentPos.z == -1) {
        std::cout << "  Location: Back-Right edge (1,0,-1)" << std::endl;
        if (whiteFace == 'B') {
            std::cout << "  → Using R to extract (perpendicular to B)" << std::endl;
            return {"R"};  // Use perpendicular move
        }
        else {
            std::cout << "  → Using B to extract (perpendicular to R)" << std::endl;
            return {"B"};  // Use perpendicular move
        }
    }
    // Back-Left edge (-1,0,-1)
    else if (currentPos.x == -1 && currentPos.z == -1) {
        std::cout << "  Location: Back-Left edge (-1,0,-1)" << std::endl;
        if (whiteFace == 'B') {
            std::cout << "  → Using L to extract (perpendicular to B)" << std::endl;
            return {"L"};  // Use perpendicular move
        }
        else {
            std::cout << "  → Using B to extract (perpendicular to L)" << std::endl;
            return {"B"};  // Use perpendicular move
        }
    }

    std::cout << "ERROR: Middle layer piece at unexpected position!" << std::endl;
    std::cout << "  This shouldn't happen - edge should be at a corner of middle layer" << std::endl;
    std::cout << "  Falling back to D rotation" << std::endl;
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
    // F2L
std::vector<Solver::F2LPair> Solver::getF2LPairs() const {
    std::vector<F2LPair> pairs;

    int cRG = findF2LCorner('R', 'G');
    int eRG = findF2LEdge('R', 'G');
    // White corners with their adjacent colors
    // Remember: White is on UP in solved state
    pairs.push_back({findF2LCorner('O', 'G'), findF2LEdge('G', 'O'), 'G', 'O',
    glm::ivec3(-1, -1, 1),    // corner position
    glm::ivec3(-1, 0, 1) }); // edge position

    pairs.push_back({findF2LCorner('O', 'B'), findF2LEdge('O', 'B'), 'O', 'B',
        glm::ivec3(-1, -1, -1),   // corner position
        glm::ivec3(-1, 0, -1) }); // edge position

    pairs.push_back({findF2LCorner('B', 'R'), findF2LEdge('B', 'R'), 'B', 'R',
        glm::ivec3(1, -1, -1),    // corner position
        glm::ivec3(1, 0, -1) }); // edge position

    pairs.push_back({cRG, eRG, 'R', 'G',
        glm::ivec3(1, -1, 1),     // corner position
        glm::ivec3(1, 0, 1) });  // edge position



    return pairs;
}


int Solver::findF2LCorner(char color1, char color2) const {
    const auto& current = cube->getCurrentPosition();

    for (int i = 0; i < 26; i++) {
        glm::ivec3 pos = current[i];

        // Corners are at positions with all coordinates = ±1
        if (std::abs(pos.x) == 1 && std::abs(pos.y) == 1 && std::abs(pos.z) == 1) {
            // This is a corner piece
            Cubelet* piece = cube->getCubelet(pos);
            if (!piece) continue;

            // Check if this corner has white and the two target colors
            bool hasWhite = false;
            bool hasColor1 = false;
            bool hasColor2 = false;

            std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
            for (Face face : faces) {
                char color = getFaceColor(piece, face);
                if (color == 'W') hasWhite = true;
                if (color == color1) hasColor1 = true;
                if (color == color2) hasColor2 = true;
            }

            // This is our white corner if it has white and both target colors
            if (hasWhite && hasColor1 && hasColor2) {
                return i;
            }
        }
    }
    return -1;
}

int Solver::findF2LEdge(char color1, char color2) const {
    const auto& current = cube->getCurrentPosition();

    for (int i = 0; i < 26; i++) {
        glm::ivec3 pos = current[i];

        // F2L edges must be at valid edge positions
        bool isValidEdgePosition = false;

        if (pos.y == 0 && std::abs(pos.x) == 1 && std::abs(pos.z) == 1) {
            isValidEdgePosition = true;
        }
        else if (std::abs(pos.y) == 1) {
            int zeroCount = (pos.x == 0) + (pos.z == 0);
            if (zeroCount == 1) {
                isValidEdgePosition = true;
            }
        }

        if (!isValidEdgePosition) continue;

        Cubelet* piece = cube->getCubelet(pos);
        if (!piece) continue;

        bool hasColor1 = false;
        bool hasColor2 = false;
        bool hasWhite = false;
        bool hasYellow = false;

        std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
        for (Face face : faces) {
            char color = getFaceColor(piece, face);
            if (color == color1) hasColor1 = true;
            if (color == color2) hasColor2 = true;
            if (color == 'W') hasWhite = true;
            if (color == 'Y') hasYellow = true;
        }

        // F2L edges should NOT have white OR yellow
        if (!hasWhite && !hasYellow && hasColor1 && hasColor2) {
            return i;
        }
    }
    return -1;
}

std::vector<std::string> Solver::solveF2LPair(const F2LPair& pair,
                                              glm::ivec3 cornerPos,
                                              glm::ivec3 edgePos,
                                              Cubelet* corner,
                                              Cubelet* edge) const {

    std::cout << "=== EDGE PIECE ANALYSIS ===" << std::endl;
    std::cout << "Looking for: " << pair.color1 << "-" << pair.color2 << " edge" << std::endl;
    std::cout << "Found edge at: (" << edgePos.x << "," << edgePos.y << "," << edgePos.z << ")" << std::endl;
    std::cout << "Edge colors:" << std::endl;
    std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
    for (Face face : faces) {
        char color = getFaceColor(edge, face);
        if (color != '?') {
            std::cout << "  Face " << face << ": " << color << std::endl;
        }
    }

    // STEP 1: Extract corner if it's in a bottom slot (y == -1) (&& cornerPos != pair.targetCornerPos)
    if (cornerPos.y == -1 ) {
        std::cout << "Corner in bottom slot - extracting to top..." << std::endl;
        return extractCornerFromSlot(cornerPos);
    }

    // STEP 2: Extract edge if it's in middle layer (y == 0) (&& edgePos != pair.targetEdgePos)
    if (edgePos.y == 0 ) {
        std::cout << "Edge in middle layer - extracting to top..." << std::endl;
        return extractEdgeFromMiddle(edgePos);
    }

    // STEP 3: Both pieces should now be in top layer (y == 1)
    // Align corner above the target slot
    if (cornerPos.y == 1 && edgePos.y == 1) {
        int targetX = 1;  // e.g., 1, -1, 1
        int targetZ = 1;  // e.g., 1, -1, -1, 1

        std::cout << "Target slot X,Z: (" << targetX << ", " << targetZ << ")" << std::endl;
        std::cout << "Corner currently at: (" << cornerPos.x << ", " << cornerPos.y << ", " << cornerPos.z << ")" << std::endl;

        // Check if corner is above the target slot
        bool cornerAboveSlot = (cornerPos.x == targetX && cornerPos.z == targetZ);

        if (!cornerAboveSlot) {
            std::cout << "Corner NOT above target slot - rotating U..." << std::endl;
            return {"U"};
        }

        // Corner is now above target slot
        std::cout << "Corner positioned above target slot at ("
                  << cornerPos.x << "," << cornerPos.y << "," << cornerPos.z << ")" << std::endl;

        // insert the pair
        return insertF2LPair(pair, cornerPos, edgePos, corner, edge);
    }

    // Fallback
    return {"U"};
}

std::vector<std::string> Solver::extractCornerFromSlot(glm::ivec3 pos) const {
    std::cout << "Extracting corner from bottom slot at ("
              << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;

    if (pos.y == -1) {
        // Simple extraction: Move up to top layer
        if (pos.x == 1 && pos.z == 1)   return {"F", "U", "F'", "U"};
        if (pos.x == 1 && pos.z == -1)  return {"R", "U", "R'", "U"};
        if (pos.x == -1 && pos.z == -1) return {"B", "U'", "B'", "U"};
        if (pos.x == -1 && pos.z == 1)  return {"L", "U", "L'", "U"};
    }

    return {"U"};
}

std::vector<std::string> Solver::extractEdgeFromMiddle(glm::ivec3 pos) const {
    std::cout << "Extracting edge from middle at ("
              << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;

    if (pos.y == 0) {
        // Simple extraction: Move up to top layer
        if (pos.x == 1 && pos.z == 1)   return {"F", "U", "F'", "U"};
        if (pos.x == 1 && pos.z == -1)  return {"R", "U", "R'", "U"};
        if (pos.x == -1 && pos.z == -1) return {"B", "U'", "B'", "U"};
        if (pos.x == -1 && pos.z == 1)  return {"L", "U", "L'", "U"};
    }

    return {"U"};
}

std::vector<std::string> Solver::insertF2LPair(const F2LPair& pair, glm::ivec3 cornerPos, glm::ivec3 edgePos, Cubelet* corner, Cubelet* edge) const {
    std::cout << "MADE IT TO THE INSERT FUNCTION" << std::endl;
    Face whiteOnCorner = getColorFace(corner,'W');
    Face c1 = getColorFace(edge,pair.color1);
    Face c2 = getColorFace(edge,pair.color2);

    // Helper lambdas
    auto isEdge = [&](Face a, Face b){
        return (c1 == a && c2 == b) || (c1 == b && c2 == a);
    };
    cout << "\n=== F2L INSERT DEBUG ===\n";
    cout << "White on corner: " << faceToString(whiteOnCorner) << "\n";
    cout << "Edge faces: "
         << faceToString(c1) << " (color1=" << pair.color1 << "), "
         << faceToString(c2) << " (color2=" << pair.color2 << ")\n";
    cout << "CornerPos: (" << cornerPos.x << ", " << cornerPos.y << ", " << cornerPos.z << ")\n";
    cout << "EdgePos:   (" << edgePos.x   << ", " << edgePos.y   << ", " << edgePos.z   << ")\n";

    if (whiteOnCorner == RIGHT)
    {
        // color-specific variants when edge = UP+FRONT
        if (isEdge(UP, FRONT)) {
            if (pair.color1=='R' && pair.color2=='G') {
                std::cout << "RLF2L 2 - RG" << std::endl;
                return {"U","F","U'","F'"};
            }
            if (pair.color1=='B' && pair.color2=='R') {
                std::cout << "RLF2L 2 - BR" << std::endl;
                return {"U'","R","U","U","R'"};    
            }
            if (pair.color1=='G' && pair.color2=='O') {
                std::cout << "RLF2L 2 - GO" << std::endl;
                return {"L","U'","L'","U"};       
            }
            if (pair.color1=='O' && pair.color2=='B') {
                std::cout << "RLF2L 2 - OB" << std::endl;
                return {"B","U'", "U'","B'"};          
            }
        }

        if (c1 == DOWN && c2 == UP) {
            std::cout << "RLF2L 12 - ALL" << std::endl;
            return {"R'","R'","U","U","R"};
        }

        if (c1 == UP && c2 == RIGHT) {
            std::cout << "F2L: white RIGHT + edge UR found" << std::endl;
            return {"U'","R","U","R'", "U"};
        }

        if (c1 == UP && c2 == BACK) {
            std::cout << "RLF2L 8" << std::endl;
            return {"U'","F","U","U","F'"};
        }

        if (c1 == UP && c2 == LEFT) {
            std::cout << "RLF2L 6" << std::endl;
            return {"U'","F","U","F'"};   // kept first instance as requested
        }

        if (c1 == BACK && c2 == UP) {
            std::cout << "RLF2L 4" << std::endl;
            return {"R'","U","R", "U"};
        }

        if (c1 == LEFT && c2 == UP) {
            std::cout << "RLF2L 10" << std::endl;
            return {"U'","F","U'","F'"};
        }

        if (c1 == RIGHT && c2 == UP) {
            std::cout << "RLF2L 14" << std::endl;
            return {"U","R'","U","U","R"};
        }
    }

    if (whiteOnCorner == FRONT)
    {
        // color-specific variants when edge = RIGHT+UP
        if (c1 == RIGHT && c2 == UP)
        {
            if (pair.color1=='R' && pair.color2=='G') {
                std::cout << "RLF2L 1 - RG" << std::endl;
                return {"U'","R'","U","R"};
            }
            if (pair.color1=='B' && pair.color2=='R') {
                std::cout << "RLF2L 1 - BR" << std::endl;
                return {"B'","U","B"};
            }
            if (pair.color1=='G' && pair.color2=='O') {
                std::cout << "RLF2L 1 - OG" << std::endl;
                return {"U","F'","U","U","F"};
            }
            if (pair.color1=='O' && pair.color2=='B') {
                std::cout << "RLF2L 1 - OB" << std::endl;
                return {"L'","U'","U'","L"};
            }
        }

        if (c1 == FRONT && c2 == UP) {
            std::cout << "RLF2L 3" << std::endl;
            return {"F","U","U","F'", "U'"};
        }

        // UP + LEFT (first occurrence) kept
        if (c1 == UP && c2 == LEFT) {
            std::cout << "RLF2L 3" << std::endl;
            return {"F","U","U","F'","U'"};
        }

        if (c1 == UP && c2 == FRONT) {
            std::cout << "F2L: white FRONT + edge UF" << std::endl;
            return {"F","U'","F'"};
        }

        if (c1 == BACK && c2 == UP) {
            std::cout << "RLF2L 9" << std::endl;
            return {"U", "R'", "U", "R" ,"U'"};   
        }

        if (c1 == UP && c2 == BACK) {
            std::cout << "RLF2L 9 (reversed)" << std::endl;
            return {"U", "R'", "U", "R" ,"U'"};
        }

        if (c1 == UP && c2 == RIGHT) {
            std::cout << "RLF2L 11" << std::endl;
            return {"U","R'","U","U","R"};
        }

        if (c1 == LEFT && c2 == UP) {
            std::cout << "RLF2L 7, FIXED INSERTION" << std::endl;
            return {"U", "R'", "U", "U", "R"};
        }
    }

    if (whiteOnCorner == UP)
    {
        if (c1 == LEFT && c2 == UP) {
            std::cout << "RLF2L 17" << std::endl;
            return {"F","U","U","F'","U'"};    
        }
        if (c1 == UP && c2 == FRONT) {
            std::cout << "RLF2L 18" << std::endl;
            return {"R'", "U'", "R", "U", "R'", "U'", "R"};   
        }
        if (c1 == UP && c2 == BACK) {
            std::cout << "RLF2L 19" << std::endl;
            return {"F","U'","F'", "U"};      
        }
        if (c1 == UP && c2 == LEFT) {
            std::cout << "RLF2L 20" << std::endl;
            return {"R'","U","R", "U"};             
        }
        if (c1 == BACK && c2 == UP) {
            std::cout << "RLF2L 19 (reversed)" << std::endl;
            return {"F","U'","F'"};            
        }
        if (c1 == UP && c2 == RIGHT) {
            std::cout << "RLF2L 24" << std::endl;
            return {"F","U","U", "F'", "U"};             

        }
        if (c1 == RIGHT && c2 == UP) {
            std::cout << "RLF2L 17 (reversed)" << std::endl;
            return {"F","U","U","F'"};         
        }
        if (c1 == FRONT && c2 == UP) {
            std::cout << "RLF2L 23 (reversed)" << std::endl;
            return {"R'","U","U","R"};         
        }
    }

    // NO MATCH → FEED FORWARD WITH U
    return {"U"};
}


std::string Solver::faceToString(Face face) const {
    switch(face) {
        case UP: return "UP";
        case DOWN: return "DOWN";
        case FRONT: return "FRONT";
        case BACK: return "BACK";
        case RIGHT: return "RIGHT";
        case LEFT: return "LEFT";
        default: return "UNKNOWN";
    }
}

bool Solver::isF2LPairSolved(const F2LPair& pair) const {
    const auto& cur = cube->getCurrentPosition();

    // 1. Check if corner is in correct slot
    if (cur.at(pair.cornerID) != pair.targetCornerPos)
        return false;

    // 2. Check if edge is in correct slot
    if (cur.at(pair.edgeID) != pair.targetEdgePos)
        return false;

    // 3. Get cubelets
    Cubelet* corner = cube->getCubelet(pair.targetCornerPos);
    Cubelet* edge   = cube->getCubelet(pair.targetEdgePos);

    if (!corner || !edge) return false;

    // 4. Check the colors on faces in the solved spot

    // Corner must have:
    //  - white on DOWN
    //  - color1 on RIGHT/LEFT/FRONT/BACK depending on slot
    //  - color2 on RIGHT/LEFT/FRONT/BACK depending on slot
    // Use getFaceColor
    if (getFaceColor(corner, DOWN) != 'W')
        return false;

    // Corner must match both pair colors
    bool cornerHasC1 =
        getFaceColor(corner, RIGHT) == pair.color1 ||
        getFaceColor(corner, LEFT)  == pair.color1 ||
        getFaceColor(corner, FRONT) == pair.color1 ||
        getFaceColor(corner, BACK)  == pair.color1;

    bool cornerHasC2 =
        getFaceColor(corner, RIGHT) == pair.color2 ||
        getFaceColor(corner, LEFT)  == pair.color2 ||
        getFaceColor(corner, FRONT) == pair.color2 ||
        getFaceColor(corner, BACK)  == pair.color2;

    if (!cornerHasC1 || !cornerHasC2)
        return false;

    // 5. Edge must match both colors and be in correct spot
    bool edgeHasC1 =
        getFaceColor(edge, RIGHT) == pair.color1 ||
        getFaceColor(edge, LEFT)  == pair.color1 ||
        getFaceColor(edge, FRONT) == pair.color1 ||
        getFaceColor(edge, BACK)  == pair.color1;

    bool edgeHasC2 =
        getFaceColor(edge, RIGHT) == pair.color2 ||
        getFaceColor(edge, LEFT)  == pair.color2 ||
        getFaceColor(edge, FRONT) == pair.color2 ||
        getFaceColor(edge, BACK)  == pair.color2;

    if (!edgeHasC1 || !edgeHasC2)
        return false;

    return true;
}

Face Solver::getColorFace(Cubelet* piece, char targetColor) const {
    std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
    for (Face face : faces) {
        if (getFaceColor(piece, face) == targetColor) {
            return face;
        }
    }
    return UP; // Default
}

bool Solver::isCornerOrientedCorrectly(Cubelet* piece) const {
    if (!piece) return false;
    return getFaceColor(piece, DOWN) == 'W';
}

// Solve Last Layer
void Solver::resetOrientationAfterF2L() {
    orientationMap[UP]    = DOWN;
    orientationMap[DOWN]  = UP;
    orientationMap[FRONT]  = BACK;
    orientationMap[BACK] = FRONT;
    orientationMap[LEFT] = LEFT;
    orientationMap[RIGHT]  = RIGHT;
}

bool Solver::isLCorrectOrientation() {
    // L-shape should have yellow on two ADJACENT edges
    // The algorithm works when Back and Left edges have yellow

    glm::ivec3 UB(0, 1, -1);   // Back edge
    glm::ivec3 UL(-1, 1, 0);   // Left edge

    const Cubelet* cuUB = cube->getCubelet(UB);
    const Cubelet* cuUL = cube->getCubelet(UL);

    if (!cuUB || !cuUL) {
        return false;
    }

    bool back_yellow = (getFaceColor(cuUB, UP) == 'Y');
    bool left_yellow = (getFaceColor(cuUL, UP) == 'Y');

    std::cout << "[L-Check] Back=" << back_yellow << ", Left=" << left_yellow << "\n";

    return back_yellow && left_yellow;
}

bool Solver::isLineHorizontal() {
    // Line should be horizontal: yellow on LEFT and RIGHT edges
    // Left edge: (-1,1,0), Right edge: (1,1,0)
    glm::ivec3 UL(-1, 1, 0);
    glm::ivec3 UR(1, 1, 0);

    const Cubelet* cuUL = cube->getCubelet(UL);
    const Cubelet* cuUR = cube->getCubelet(UR);

    if (!cuUL || !cuUR) {
        std::cout << "[isLineHorizontal] Missing cubelet\n";
        return false;
    }

    bool ul_yellow = (getFaceColor(cuUL, UP) == 'Y');
    bool ur_yellow = (getFaceColor(cuUR, UP) == 'Y');

    std::cout << "[isLineHorizontal] Left=" << ul_yellow << ", Right=" << ur_yellow << "\n";

    return ul_yellow && ur_yellow;
}

OLLState Solver::detectOLLState() {
    const glm::ivec3 UF(0, 1, 1);
    const glm::ivec3 UR(1, 1, 0);
    const glm::ivec3 UB(0, 1, -1);
    const glm::ivec3 UL(-1, 1, 0);

    const Cubelet* cuUF = cube->getCubelet(UF);
    const Cubelet* cuUR = cube->getCubelet(UR);
    const Cubelet* cuUB = cube->getCubelet(UB);
    const Cubelet* cuUL = cube->getCubelet(UL);

    if (!cuUF || !cuUR || !cuUB || !cuUL) {
        std::cout << "[detectOLLState] missing cubelet(s) — defaulting to DOT\n";
        return DOT;
    }

    bool uf_up = (getFaceColor(cuUF, UP) == 'Y');
    bool ur_up = (getFaceColor(cuUR, UP) == 'Y');
    bool ub_up = (getFaceColor(cuUB, UP) == 'Y');
    bool ul_up = (getFaceColor(cuUL, UP) == 'Y');

    int count = (int)uf_up + (int)ur_up + (int)ub_up + (int)ul_up;

    // Debug: show what colors are actually on UP face
    std::cout << "[detectOLLState] UP face colors:\n";
    std::cout << "  Front edge (0,1,1): " << getFaceColor(cuUF, UP) << " (yellow=" << uf_up << ")\n";
    std::cout << "  Right edge (1,1,0): " << getFaceColor(cuUR, UP) << " (yellow=" << ur_up << ")\n";
    std::cout << "  Back edge (0,1,-1): " << getFaceColor(cuUB, UP) << " (yellow=" << ub_up << ")\n";
    std::cout << "  Left edge (-1,1,0): " << getFaceColor(cuUL, UP) << " (yellow=" << ul_up << ")\n";


    std::cout << "[detectOLLState] Yellow edges facing up: " << count << "\n";

    if (count == 4) {
        return CROSS_SHAPE;
    }
    if (count == 2) {
        // Line: opposite edges (UF+UB or UR+UL)
        if ((uf_up && ub_up) || (ur_up && ul_up)) {
            std::cout << "[detectOLLState] -> LINE (opposite edges)\n";
            return LINE_SHAPE;
        }
        // L-shape: adjacent edges
        std::cout << "[detectOLLState] -> L-SHAPE (adjacent edges)\n";
        return L_SHAPE;
    }
    std::cout << "[detectOLLState] -> DOT\n";
    return DOT;
}


std::vector<std::string> Solver::solveOLLCross() {
    OLLState state = detectOLLState();

    switch (state) {
        case CROSS_SHAPE:
            std::cout << "OLL already solved (cross)" << std::endl;
            return {};

        case LINE_SHAPE: {
            for (int i = 0; i < 2; i++) {
                if (isLineHorizontal()) {
                    std::cout << "OLL Line pattern -> applying algorithm" << std::endl;
                    return { "F'", "R'", "U'", "R", "U", "F" };
                }
                return {"U"};
            }
            std::cout << "WARNING: Line not detected as horizontal after 4 rotations, applying algorithm anyway" << std::endl;
            return {"F'", "R'", "U", "R", "U'", "F"};

        }

        case L_SHAPE: {
            if (isLCorrectOrientation()) {
                std::cout << "OLL L pattern -> applying algorithm" << std::endl;
                return { "F'", "R'", "U'", "R", "U", "F" };
                break;
            }
            // Not oriented correctly, rotate U and check again
            std::cout << "L not oriented, rotating U'" << std::endl;
            return {"U'"};
        }

        case DOT:{
            std::cout << "OLL Dot -> apply L then Line" << std::endl;
            return {"R'", "U", "R", "U'", "F", "U'", "U'", "F'", "U'", "F", "U'", "U'", "F'"};
            }
        }

    return {};
}

// PLL
int Solver::countAlignedEdges_Fixed() {
    int count = 0;
    std::cout << "\n=== CHECKING EDGE ALIGNMENT ===" << std::endl;
    for (int i = 0; i < 4; i++) {
        if (isEdgeAligned(i)) {
            count++;
        }
    }

    std::cout << "Total aligned: " << count << "/4" << std::endl;

    return count;
}

bool Solver::edgeMatchesCenter_Fixed(int edgeIndex) {
    static const glm::ivec3 edgePositions[4] = {
        {0, 1, 1},   // FRONT edge
        {1, 1, 0},   // RIGHT edge
        {0, 1, -1},  // BACK edge
        {-1, 1, 0}   // LEFT edge
    };

    Cubelet* edge = cube->getCubelet(edgePositions[edgeIndex]);
    if (!edge) return false;

    Face faceToCheck;
    glm::ivec3 centerPos;
    Face centerFaceToCheck;

    if (edgeIndex == 0) {
        faceToCheck = BACK;
        centerPos = {0, 0, 1};
        centerFaceToCheck = BACK;
    }
    else if (edgeIndex == 1) {
        faceToCheck = RIGHT;
        centerPos = {1, 0, 0};
        centerFaceToCheck = RIGHT;
    }
    else if (edgeIndex == 2) {
        faceToCheck = FRONT;
        centerPos = {0, 0, -1};
        centerFaceToCheck = FRONT;
    }
    else {
        faceToCheck = LEFT;
        centerPos = {-1, 0, 0};
        centerFaceToCheck = LEFT;
    }

    color edgeColor = edge->getFaceColor(faceToCheck);

    Cubelet* center = cube->getCubelet(centerPos);
    if (!center) return false;

    color centerColor = center->getFaceColor(centerFaceToCheck);

    bool match = sameColor(edgeColor, centerColor);

    std::cout << "  Edge " << edgeIndex
              << ": " << colorToChar(edgeColor)
              << " vs center " << colorToChar(centerColor)
              << " = " << (match ? "MATCH" : "NO MATCH") << std::endl;

    return match;
}

// PLL

std::vector<std::string> Solver::solveLastLayerEdges_Fixed() {
    std::cout << "\n=== PLL EDGE ANALYSIS ===" << std::endl;
    //Edge 0: FRONT edge (0,1,1) = Green-Yellow edge
    //Edge 1: RIGHT edge (1,1,0) = Blue-Yellow edge
    //Edge 2: BACK edge (0,1,-1) = Red-Yellow edge
    //Edge 3: LEFT edge (-1,1,0) = Orange-Yellow edge
    // Define edge positions and their names
    if (!baseEdge) {
        bool match = isEdgeAligned(3);
        if (match) {
            baseEdge = true;
        } else {
            return{"U"};
        }
    } else {
        if (isEdgeAligned(0)) {
            std::cout << "green lined up" << endl;
            baseEdge = false;
            return {"B'", "U'", "U'", "B", "U", "B'", "U", "B", "U'"};
        } else if (isEdgeAligned(1)){
            std::cout << "red lined up" << endl;
        baseEdge = false;
        return {"U'", "R'", "U'", "R", "U'", "R'", "U'", "U'", "R", "U'", "L'", "U'", "L", "U'", "L'", "U'", "U'", "L", "U'"}; //"B'", "U", "U", "B", "U", "B'", "U", "B", "U"};
        } else {
            std::cout << "blue lined up" << endl;
            baseEdge = false;
            return {"R'", "U'", "U'", "R", "U", "R'", "U", "R", "U'"};
        }
    }

    // Don't add any moves yet - just return empty
    return {};
}

bool Solver::isEdgeAligned(int edgeIndex) {
    // Define edge positions and their properties
    struct EdgeInfo {
        glm::ivec3 position;
        std::string name;
        Face edgeFaceToCheck;
        glm::ivec3 centerPosition;
        Face centerFaceToCheck;
    };

    static const EdgeInfo edges[4] = {
        {{0, 1, 1}, "FRONT", FRONT, {0, 0, 1}, FRONT},
        {{1, 1, 0}, "RIGHT", RIGHT, {1, 0, 0}, RIGHT},
        {{0, 1, -1}, "BACK", BACK, {0, 0, -1}, BACK},
        {{-1, 1, 0}, "LEFT", LEFT, {-1, 0, 0}, LEFT}
    };

    if (edgeIndex < 0 || edgeIndex >= 4) {
        std::cout << "ERROR: Invalid edge index " << edgeIndex << std::endl;
        return false;
    }

    const EdgeInfo& edgeInfo = edges[edgeIndex];

    // Get the center piece
    Cubelet* centerPiece = cube->getCubelet(edgeInfo.centerPosition);
    if (!centerPiece) {
        std::cout << "ERROR: Could not get center piece for edge " << edgeIndex
                  << " at position (" << edgeInfo.centerPosition.x << ","
                  << edgeInfo.centerPosition.y << "," << edgeInfo.centerPosition.z << ")" << std::endl;
        return false;
    }

    // Get the edge piece
    Cubelet* edgePiece = cube->getCubelet(edgeInfo.position);
    if (!edgePiece) {
        std::cout << "ERROR: Could not get edge piece for edge " << edgeIndex
                  << " at position (" << edgeInfo.position.x << ","
                  << edgeInfo.position.y << "," << edgeInfo.position.z << ")" << std::endl;
        return false;
    }

    // Get the colors
    color centerColorRGB = centerPiece->getFaceColor(edgeInfo.centerFaceToCheck);
    color edgeColorRGB = edgePiece->getFaceColor(edgeInfo.edgeFaceToCheck);

    // Check if they match
    bool match = sameColor(edgeColorRGB, centerColorRGB);

    // Optional: Print debug info
    char centerChar = colorToChar(centerColorRGB);
    char edgeChar = colorToChar(edgeColorRGB);

    std::cout << "Edge " << edgeIndex << " (" << edgeInfo.name << "): "
              << "Edge color = " << edgeChar << ", Center color = " << centerChar
              << " -> " << (match ? "ALIGNED" : "MISALIGNED") << std::endl;

    return match;
}


std::vector<std::string> Solver::findCorrectEdgePair() {
    // 0 = FRONT, 1 = RIGHT, 2 = BACK, 3 = LEFT
    if (!edgeMatchesCenter_Fixed(FRONT) && !edgeMatchesCenter_Fixed(RIGHT) && !edgeMatchesCenter_Fixed(BACK) && !edgeMatchesCenter_Fixed(LEFT)) {
        return {"U"};
    }
    // Pair at FRONT–RIGHT → rotate U2 to move it to BACK
    if (edgeMatchesCenter_Fixed(FRONT) && edgeMatchesCenter_Fixed(RIGHT))
        return {"U'", "U'"};   // rotate twice

    // Pair at RIGHT–BACK → rotate U once
    if (edgeMatchesCenter_Fixed(RIGHT) && edgeMatchesCenter_Fixed(BACK))
        return {"U'"};          // rotate one time

    // Pair already at BACK–LEFT → no moves
    if (edgeMatchesCenter_Fixed(BACK) && edgeMatchesCenter_Fixed(LEFT))
        return {};              // already in correct spot

    // Pair at LEFT–FRONT → rotate U once forward (U)
    if (edgeMatchesCenter_Fixed(LEFT) && edgeMatchesCenter_Fixed(FRONT))
        return {"U"};           // rotate the other way

    // NO PAIR FOUND
    return {
        "R'", "U", "U", "R", "U", "R'", "U", "R",
        "R'", "U", "U", "R", "U", "R'", "U", "R", "U", "U"
    };
}

// PLL Solving the Corners
bool Solver::cornerInCorrectLocation_Fixed(glm::ivec3 pos) {
    Cubelet* corner = cube->getCubelet(pos);
    if (!corner) {
        std::cout << "  Corner at (" << pos.x << "," << pos.y << "," << pos.z
                  << "): NULL" << std::endl;
        return false;
    }

    std::cout << "  Checking corner at (" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;

    // Get the ACTUAL visible colors on this corner (after XX rotation)
    char cornerRight = getFaceColor(corner, RIGHT);
    char cornerLeft = getFaceColor(corner, LEFT);
    char cornerFront = getFaceColor(corner, FRONT);
    char cornerBack = getFaceColor(corner, BACK);
    char cornerUp = getFaceColor(corner, UP);
    char cornerDown = getFaceColor(corner, DOWN);

    std::cout << "  Corner colors: R=" << cornerRight << " L=" << cornerLeft
              << " F=" << cornerFront << " B=" << cornerBack
              << " U=" << cornerUp << " D=" << cornerDown << std::endl;

    // For debugging, let's just check if the corner has yellow on UP
    // This is a simpler check that might work
    if (cornerUp == 'Y') {
        std::cout << "  Corner has yellow on UP face" << std::endl;

        // Also check if it matches one adjacent center
        bool matchesOneCenter = false;

        if (pos.x == 1 && cornerRight == 'R') {
            std::cout << "  Matches RIGHT center" << std::endl;
            matchesOneCenter = true;
        }
        if (pos.x == -1 && cornerLeft == 'O') {
            std::cout << "  Matches LEFT center" << std::endl;
            matchesOneCenter = true;
        }
        if (pos.z == 1 && cornerBack == 'G') {
            std::cout << "  Matches FRONT center (via BACK face)" << std::endl;
            matchesOneCenter = true;
        }
        if (pos.z == -1 && cornerFront == 'B') {
            std::cout << "  Matches BACK center (via FRONT face)" << std::endl;
            matchesOneCenter = true;
        }

        return matchesOneCenter;
    }

    std::cout << "  Corner does NOT have yellow on UP face" << std::endl;
    return false;
}

int Solver::countCorrectCorners_Fixed() {
    std::cout << "\n=== CHECKING CORNER LOCATIONS ===" << std::endl;
    return countCorrectCornerOrientations();
}

int Solver::countCorrectCornerOrientations() {
    std::cout << "\n=== CHECKING CORNER ORIENTATIONS ===" << std::endl;

    static const glm::ivec3 corners[4] = {
        {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, 1, 1}
    };

    int count = 0;
    for (int i = 0; i < 4; i++) {
        Cubelet* corner = cube->getCubelet(corners[i]);
        if (corner) {
            char upColor = getFaceColor(corner, UP);
            std::cout << "Corner " << i << " at (" << corners[i].x << ","
                      << corners[i].y << "," << corners[i].z
                      << "): UP=" << upColor;

            if (upColor == 'Y') {
                count++;
                std::cout << " ✓ YELLOW" << std::endl;
            } else {
                std::cout << " ✗ NOT YELLOW" << std::endl;
            }
        }
    }

    std::cout << "Correct orientations: " << count << "/4" << std::endl;
    return count;
}

std::vector<std::string> Solver::solveLastLayerCorners_Fixed() {
    int correctLocation = countCorrectCornersLocations();
    std::cout << "correctLocation: " << correctLocation << std::endl;
    if (correctLocation == 0) {
        std::cout << "Case A.5" << std::endl;
        return {"R'", "U", "L", "U'", "R", "U", "L'", "U'"};
    } else if (correctLocation == 1) {
        if (isCornerInCorrectLocation(0)) {
            std::cout << "Case A" << std::endl;
            return {"L", "U'", "R'", "U", "L'", "U'", "R", "U"};
        } else if (isCornerInCorrectLocation(1)) {
            std::cout << "Case B" << std::endl;
            return {"F", "U'", "B'", "U", "F'", "U'", "B", "U"};
        } else if (isCornerInCorrectLocation(2)) {
            std::cout << "Case C" << std::endl;
            return {"R", "U'", "L'", "U", "R'", "U'", "L", "U"};
        } else if (isCornerInCorrectLocation(3)) {
            std::cout << "Case D" << std::endl;
            return {"R'", "U", "L", "U'", "R", "U", "L'", "U'"};
        }
    } else if (correctLocation == 4){
        std::cout << "Case E" << std::endl;
        return orientCorners();
    }
    return {};
}

std::vector<std::string> Solver::orientCorners() {
    int solvedCorners = countCorrectCorners_Fixed();
    std::cout << "orientCorners" << std::endl;
    if (solvedCorners == 0) {
        return {
        "R'", "U", "U", "R", "U", "R'", "U", "R",
            "L", "U", "U", "L'", "U'", "L", "U'", "L'"};
    }else if (solvedCorners == 1) {
        if (isCornerInCorrectLocation(1) || isCornerInCorrectLocation(2)) {
            std::cout << "Case 1/2" << std::endl;
            return {"L", "U", "U", "L'", "U'", "L", "U'", "L'",
                "R'", "U", "U", "R", "U", "R'", "U", "R"
                    };
        } else if (isCornerInCorrectLocation(0) || isCornerInCorrectLocation(3)) {
            std::cout << "Case 0/3" << std::endl;
            return {
                "L", "U", "U", "L'", "U'", "L", "U'", "L'",
                "R'", "U", "U", "R", "U", "R'", "U", "R"
                    };
        }
    }
     else if (solvedCorners == 2) {
        if (isCornerInCorrectLocation(0) && isCornerInCorrectLocation(1)) {
            std::cout << "Case 0 - 1" << std::endl;
            return {"B'", "U'", "U'", "B", "U", "B'", "U", "B",
                       "F", "U'", "U'", "F'", "U'", "F", "U'", "F'"

                // "L", "U", "U", "L'", "U'", "L", "U'", "L'",
                // "R'", "U", "U", "R", "U", "R'", "U", "R"
                    };
        } else if (isCornerInCorrectLocation(1) && isCornerInCorrectLocation(2)) {
            std::cout << "Case 1 - 2" << std::endl;
            return {};
                    // "U'",
                    // "R'", "U", "U", "R", "U", "R'", "U", "R",
                    // "L", "U", "U", "L'", "U'", "L", "U'", "L'"};
        } else if (isCornerInCorrectLocation(2) && isCornerInCorrectLocation(3)) {
            std::cout << "Case 2 - 3" << std::endl;
            return { };
                    // "R'", "U", "U", "R", "U", "R'", "U", "R",
                    // "L", "U", "U", "L'", "U'", "L", "U'", "L'"};
        } else if (isCornerInCorrectLocation(3) && isCornerInCorrectLocation(1)) {
            std::cout << "Case 3 - 1" << std::endl;
            return {};
                   // "U'",
                   // "R'", "U", "U", "R", "U", "R'", "U", "R",
                   //  "L", "U", "U", "L'", "U'", "L", "U'", "L'"};
        } else if (isCornerInCorrectLocation(1) && isCornerInCorrectLocation(3) || isCornerInCorrectLocation(0) && isCornerInCorrectLocation(2)) {
            std::cout << "Case 1 - 3 or 0 - 2" << std::endl;
            return {
                    "R'", "U", "U", "R", "U", "R'", "U", "R",
                    "L", "U", "U", "L'", "U'", "L", "U'", "L'"};
        }
    } else if (solvedCorners == 4) {
        fullySolved = true;
        return {};
    }
    return {};
}

bool Solver::isCornerInCorrectLocation(int cornerIndex) {
    static const glm::ivec3 positions[4] = {
        {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, 1, 1}
    };

    static const std::vector<std::vector<char>> expectedColors = {
        {'Y', 'G', 'R'}, {'Y', 'R', 'B'}, {'Y', 'B', 'O'}, {'Y', 'O', 'G'}
    };

    if (cornerIndex < 0 || cornerIndex >= 4) return false;

    Cubelet* corner = cube->getCubelet(positions[cornerIndex]);
    if (!corner) return false;

    // Get all visible colors
    std::vector<char> actualColors;
    std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};

    for (Face face : faces) {
        char color = getFaceColor(corner, face);
        if (color != '?') {
            actualColors.push_back(color);
        }
    }

    // Sort and compare
    std::vector<char> sortedActual = actualColors;
    std::sort(sortedActual.begin(), sortedActual.end());

    std::vector<char> sortedExpected = expectedColors[cornerIndex];
    std::sort(sortedExpected.begin(), sortedExpected.end());

    return (sortedActual == sortedExpected);
}

bool Solver::isCornerCorrectlyOriented(int cornerIndex) {
    static const glm::ivec3 positions[4] = {
        {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, 1, 1}
    };

    if (cornerIndex < 0 || cornerIndex >= 4) return false;

    Cubelet* corner = cube->getCubelet(positions[cornerIndex]);
    if (!corner) return false;

    return (getFaceColor(corner, UP) == 'Y');
}

int Solver::countCorrectCornersLocations() {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (isCornerInCorrectLocation(i)) {
            count++;
        }
    }
    return count;
}

color Solver::centerColor(Face f) {
    glm::ivec3 pos;

    switch (f) {
        case FRONT: pos = {0,0,1}; break;
        case BACK:  pos = {0,0,-1}; break;
        case RIGHT: pos = {1,0,0}; break;
        case LEFT:  pos = {-1,0,0}; break;
        case UP:    pos = {0,1,0}; break;
        case DOWN:  pos = {0,-1,0}; break;
    }

    return cube->getCubelet(pos)->getFaceColor(f);
}

bool Solver::areCornersSolved_Fixed() {

    static const glm::ivec3 corners[4] = {
        {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, 1, 1}
    };

    // First check all corners have yellow on UP
    for (const auto& pos : corners) {
        Cubelet* corner = cube->getCubelet(pos);
        if (!corner || getFaceColor(corner, UP) != 'Y') {
            std::cout << "  Corner at (" << pos.x << "," << pos.y << "," << pos.z
                      << ") missing or doesn't have yellow on UP" << std::endl;
            return false;
        }
    }

    std::cout << "  All corners have yellow on UP ✓" << std::endl;

    // NOW check if they're in correct positions
    // After XX rotation, what we see at each position:
    // - At FRONT position (Z=+1): we see BACK face of corner
    // - At BACK position (Z=-1): we see FRONT face of corner
    // - At RIGHT position (X=+1): we see RIGHT face of corner
    // - At LEFT position (X=-1): we see LEFT face of corner

    // Check corner 0: (1,1,1) - UFR
    Cubelet* corner0 = cube->getCubelet({1, 1, 1});
    if (corner0) {
        char right0 = getFaceColor(corner0, RIGHT);
        char back0 = getFaceColor(corner0, BACK);

        // Should be R on RIGHT and G on BACK
        if (right0 != 'R' || back0 != 'G') {
            std::cout << "  Corner 0 (UFR) wrong: R=" << right0 << " B=" << back0
                      << " (should be R and G)" << std::endl;
            return false;
        }
    }

    // Check corner 1: (1,1,-1) - URB
    Cubelet* corner1 = cube->getCubelet({1, 1, -1});
    if (corner1) {
        char right1 = getFaceColor(corner1, RIGHT);
        char front1 = getFaceColor(corner1, FRONT);

        // Should be R on RIGHT and B on FRONT
        if (right1 != 'R' || front1 != 'B') {
            std::cout << "  Corner 1 (URB) wrong: R=" << right1 << " F=" << front1
                      << " (should be R and B)" << std::endl;
            return false;
        }
    }

    // Check corner 2: (-1,1,-1) - UBL
    Cubelet* corner2 = cube->getCubelet({-1, 1, -1});
    if (corner2) {
        char left2 = getFaceColor(corner2, LEFT);
        char front2 = getFaceColor(corner2, FRONT);  // Shows at BACK position

        // Should be O on LEFT and B on FRONT
        if (left2 != 'O' || front2 != 'B') {
            std::cout << "  Corner 2 (UBL) wrong: L=" << left2 << " F=" << front2
                      << " (should be O and B)" << std::endl;
            return false;
        }
    }

    // Check corner 3: (-1,1,1) - ULF
    Cubelet* corner3 = cube->getCubelet({-1, 1, 1});
    if (corner3) {
        char left3 = getFaceColor(corner3, LEFT);
        char back3 = getFaceColor(corner3, BACK);

        // Should be O on LEFT and G on BACK
        if (left3 != 'O' || back3 != 'G') {
            std::cout << "  Corner 3 (ULF) wrong: L=" << left3 << " B=" << back3
                      << " (should be O and G)" << std::endl;
            return false;
        }
    }

    std::cout << "  All corners in correct positions!" << std::endl;
    return true;
}

static const glm::ivec3 cornerPos[4] = {
    { 1, 1,  1}, // 0 = UFR
    { 1, 1, -1}, // 1 = URB
    {-1, 1, -1}, // 2 = UBL
    {-1, 1,  1}  // 3 = ULF
};

static const Face cornerFaces[4][3] = {
    { UP, RIGHT, FRONT },   // UFR
    { UP, BACK, RIGHT },    // URB
    { UP, LEFT, BACK },     // UBL
    { UP, FRONT, LEFT }     // ULF
};

static const glm::ivec3 centerPosCorner[4][3] = {
    { {0,1,0}, {1,0,0}, {0,0,1} },   // UFR → Up, Right, Front
    { {0,1,0}, {0,0,-1}, {1,0,0} },  // URB → Up, Back, Right
    { {0,1,0}, {-1,0,0}, {0,0,-1} }, // UBL → Up, Left, Back
    { {0,1,0}, {0,0,1}, {-1,0,0} }   // ULF → Up, Front, Left
};

bool Solver::cornerIsCorrect(int idx) {
    const Cubelet* c = cube->getCubelet(cornerPos[idx]);
    if (!c) return false;

    for (int f = 0; f < 3; f++) {
        Face face = cornerFaces[idx][f];
        glm::ivec3 cp = centerPosCorner[idx][f];

        const Cubelet* center = cube->getCubelet(cp);
        if (!center) return false;

        color c1 = c->getFaceColor(face);
        color c2 = center->getFaceColor(face);

        if (!sameColor(c1, c2))
            return false;
    }
    return true;
}

bool Solver::cornerMatches(int idx) {
    static const glm::ivec3 cornerPos[4] = {
        { 1, 1,  1}, // UFR
        { 1, 1, -1}, // URB
        {-1, 1, -1}, // UBL
        {-1, 1,  1}  // ULF
    };

    const glm::ivec3 pos = cornerPos[idx];
    const Cubelet* c =  cube->getCubelet(pos);
    if (!c) return false;

    int matchCount = 0;

    if (pos.x == 1) {
        if (sameColor(c->getFaceColor(RIGHT), centerColor(RIGHT))) matchCount++;
    } else {
        if (sameColor(c->getFaceColor(LEFT), centerColor(LEFT))) matchCount++;
    }

    if (pos.z == 1) {
        if (sameColor(c->getFaceColor(FRONT), centerColor(FRONT))) matchCount++;
    } else {
        if (sameColor(c->getFaceColor(BACK), centerColor(BACK))) matchCount++;
    }

    return (matchCount == 2);
}

void Solver::debugCenterColors() {
    std::cout << "\n=== CENTER COLORS DETAILED (AFTER XX ROTATION) ===" << std::endl;

    glm::ivec3 centerPositions[6] = {
        {0, 1, 0},   // UP
        {0, -1, 0},  // DOWN
        {0, 0, 1},   // FRONT
        {0, 0, -1},  // BACK
        {1, 0, 0},   // RIGHT
        {-1, 0, 0}   // LEFT
    };

    std::string names[6] = {"UP", "DOWN", "FRONT", "BACK", "RIGHT", "LEFT"};

    for (int i = 0; i < 6; i++) {
        Cubelet* center = cube->getCubelet(centerPositions[i]);
        if (!center) {
            std::cout << names[i] << " center: NULL" << std::endl;
            continue;
        }

        std::cout << names[i] << " center at (" << centerPositions[i].x << ","
                  << centerPositions[i].y << "," << centerPositions[i].z << "):" << std::endl;

        // Check ALL faces to see what colors are actually there
        for (Face face : {UP, DOWN, FRONT, BACK, RIGHT, LEFT}) {
            color rgb = center->getFaceColor(face);
            char colorChar = colorToChar(rgb);

            std::cout << "  Face " << faceToString(face) << ": RGB("
                      << rgb.red << "," << rgb.green << "," << rgb.blue
                      << ") -> '" << colorChar << "'" << std::endl;
        }
    }
}

void Solver::debugFaceIndexOrder() {
    static const glm::ivec3 centerPos[6] = {
        {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}, {1,0,0}, {-1,0,0}
    };
    static const char* names[6] = {
        "UP","DOWN","FRONT","BACK","RIGHT","LEFT"
    };

    std::cout << "=== ACTUAL FACE ORDER INSIDE CUBELET ===\n";

    for (int f=0; f<6; f++) {
        Cubelet* c = cube->getCubelet(centerPos[f]);
        std::cout << names[f] << ": ";
        for (int i=0; i<6; i++) {
            color col = c->getFaceColor((Face)i);
            std::cout << colorToChar(col) << " ";
        }
        std::cout << "\n";
    }
}

bool Solver::isCornerCorrectlyOriented(glm::ivec3 pos) const {
    Cubelet* corner = cube->getCubelet(pos);
    if (!corner) return false;

    // After XX rotation, yellow should be on DOWN face
    return getFaceColor(corner, UP) == 'Y';
}


