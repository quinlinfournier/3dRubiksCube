//
// Created by quinf on 11/22/2025.
//

#include "Solver.h"
#include <iostream>
#include <cmath>
#include <map>
#include <utility>
#include <iomanip>
#include <list>
#include <algorithm>




Solver::Solver(RubiksCube *cube) : cube(cube) {
    // Initialize all member variables
    currentState = IDLE;
    currentStep = 0;
    currentTargetColor = 'B';
    moveCounter = 0;
    currentF2LSlot = 0;

    for (int face = 0; face < 6; face++) {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                facelet[face][row][col] = '?';
            }
        }
    }
    debugFaceIndexOrder();
}

char Solver::getFaceColor(const Cubelet* piece, Face face) const {
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

    if (debugFreeze) {
        if (stepThroughMode && movesSinceLastFreeze > 0) {
            movesSinceLastFreeze --;
        } else {
            displayDebugInfo();
            return "";
        }
    }

    // Execute queued moves first
    if (!currentMoves.empty()) {
        std::string nextMove = currentMoves.front();
        currentMoves.erase(currentMoves.begin());
        if (stepThroughMode) movesSinceLastFreeze = 0;
        return nextMove;
    }

    // ==========================
    // Safety check to prevent infinite loops
    // if (moveCounter++ > MAX_MOVES) {
    //     std::cout << "ERROR: Solver stuck in infinite loop! Aborting." << std::endl;
    //     currentState = FAILED;
    //     return "";
    // }
    // ===========================

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

            // DOUBLE CHECK #2 — pair solved AFTER generating moves?
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

        if (!currentMoves.empty()) {
            std::string nextMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());
            return nextMove;
        }

        if (countAlignedEdges_Fixed() == 4) {
            std::cout << "PLL EDGES COMPLETE!" << std::endl;
            currentStep = 4;
            return "";
        }
    }

    // STEP 4: PLL CORNERS - USE FIXED VERSION
    if (currentStep == 4) {
        std::cout << "=== PLL CORNER PERMUTATION ===" << std::endl;

        currentMoves = solveLastLayerCorners_Fixed();

        if (!currentMoves.empty()) {
            std::string nextMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());
            return nextMove;
        }

        if (areCornersSolved_Fixed()) {
            std::cout << "CUBE SOLVED!" << std::endl;
            currentState = WCCOMPLETE;
            return "";
        }
    }

    return "";
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

 // CASE B: White facing RIGHT - Need to check WHERE edge is
        else if (whiteFace == 'R') {
            std::cout << "White facing RIGHT" << std::endl;

            // Edge at (0,-1,1) - bottom front, white facing right
            if (currentPos.x == 0 && currentPos.z == 1) {
                std::cout << "  Position: Bottom-Front, solving W-B edge" << std::endl;
                return {"R"};
            }
            // Edge at (1,-1,0) - bottom right, white facing right (away from center)
            else if (currentPos.x == 1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Right, solving W-R edge" << std::endl;
                return {"R"};
            }
            // Edge at (0,-1,-1) - bottom back, white facing right
            else if (currentPos.x == 0 && currentPos.z == -1) {
                std::cout << "  Position: Bottom-Back, solving W-G edge" << std::endl;
                return {"R"};
            }
            // Edge at (-1,-1,0) - bottom left, white facing right (toward center)
            else if (currentPos.x == -1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Left, solving W-O edge" << std::endl;
                return {"R"};
            }
        }

            // CASE C: White facing BACK - Need to check WHERE edge is
        else if (whiteFace == 'B') {
            std::cout << "White facing BACK" << std::endl;

            // Edge at (0,-1,1) - bottom front, white facing back (away from front)
            if (currentPos.x == 0 && currentPos.z == 1) {
                std::cout << "  Position: Bottom-Front, solving W-B edge" << std::endl;
                return {"D", "R", "D'", "R", "R"};
            }
            // Edge at (1,-1,0) - bottom right, white facing back
            else if (currentPos.x == 1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Right, solving W-R edge" << std::endl;
                return {"B", "D'", "B'", "D", "R", "R"};
            }
            // Edge at (0,-1,-1) - bottom back, white facing back (toward back)
            else if (currentPos.x == 0 && currentPos.z == -1) {
                std::cout << "  Position: Bottom-Back, solving W-G edge" << std::endl;
                return {"B", "D'", "B", "D"};  // ← THIS IS THE PROBLEMATIC LINE
            }
            // Edge at (-1,-1,0) - bottom left, white facing back
            else if (currentPos.x == -1 && currentPos.z == 0) {
                std::cout << "  Position: Bottom-Left, solving W-O edge" << std::endl;
                return {"B", "D", "B'", "D'", "L", "L"};
            }
        }

        // CASE D: White facing FRONT - Need to check WHERE edge is
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

        // CASE E: White facing LEFT - Need to check WHERE edge is
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

        // CASE F: White facing UP - This shouldn't happen in bottom layer, but handle it
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
    // Edge not aligned - rotate bottom to align it
    else {
        std::cout << "W-" << targetColor << " in bottom layer but not aligned. Rotating..." << std::endl;
        return {"D"};
    }


    }
// CASE 4: Middle Layer - FIXED VERSION
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
            return {"L"};  // Use perpendicular move - L instead of F
        }
        else {
            std::cout << "  → Using F to extract (perpendicular to L)" << std::endl;
            return {"F"};  // Use perpendicular move - F instead of L
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
        bool hasYellow = false;  // ADD THIS

        std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
        for (Face face : faces) {
            char color = getFaceColor(piece, face);
            if (color == color1) hasColor1 = true;
            if (color == color2) hasColor2 = true;
            if (color == 'W') hasWhite = true;
            if (color == 'Y') hasYellow = true;  // ADD THIS
        }

        // F2L edges should NOT have white OR yellow
        if (!hasWhite && !hasYellow && hasColor1 && hasColor2) {  // CHANGE THIS LINE
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
    std::cout << "===========================" << std::endl;

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
    cout << "=========================\n";
    // =====================================================
    //                 RLF2L 2  (white RIGHT)  -- cleaned
    // =====================================================
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
                return {"U'","R","U","U","R'"};   // kept as you wrote
            }
            if (pair.color1=='G' && pair.color2=='O') {
                std::cout << "RLF2L 2 - GO" << std::endl;
                return {"L","U'","L'","U"};      // kept as you wrote
            }
            if (pair.color1=='O' && pair.color2=='B') {
                std::cout << "RLF2L 2 - OB" << std::endl;
                return {"B","U'", "U'","B'"};         // kept as you wrote
            }
        }

        // flipped edge (kept as separate, do not merge reversed)
        if (c1 == DOWN && c2 == UP) {
            std::cout << "RLF2L 12 - ALL" << std::endl;
            return {"R'","R'","U","U","R"};
        }

        // other RIGHT-oriented cases (first occurrence kept)
        if (c1 == UP && c2 == RIGHT) {
            std::cout << "F2L: white RIGHT + edge UR found" << std::endl;
            return {"U'","R","U","R'", "U"}; //"U","R","U","R'", "U"
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

        // (the later duplicate `if (c1 == UP && c2 == LEFT)` was removed per rule 1:A)

        if (c1 == LEFT && c2 == UP) {
            std::cout << "RLF2L 10" << std::endl;
            return {"U'","F","U'","F'"};
        }

        if (c1 == RIGHT && c2 == UP) {
            std::cout << "RLF2L 14" << std::endl;
            return {"U","R'","U","U","R"};
        }
    }

    // =====================================================
    //                 RLF2L 1  (white FRONT)  -- cleaned
    // =====================================================
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
            return {"F","U","U","F'","U'"};   // kept as you wrote (first occurrence)
        }

        if (c1 == UP && c2 == FRONT) {
            std::cout << "F2L: white FRONT + edge UF" << std::endl;
            return {"F","U'","F'"};
        }

        // removed duplicate UP+LEFT branch (kept first one per 1:A)

        if (c1 == BACK && c2 == UP) {
            std::cout << "RLF2L 9" << std::endl;
            return {"U", "R'", "U", "R" ,"U'"};  // kept as you wrote
        }

        if (c1 == UP && c2 == BACK) {
            std::cout << "RLF2L 9 (reversed)" << std::endl;
            return {"U", "R'", "U", "R" ,"U'"};  // kept as you wrote (reversed case)
        }

        if (c1 == UP && c2 == RIGHT) {
            std::cout << "RLF2L 11" << std::endl;
            return {"U","R'","U","U","R"};
        }

        if (c1 == LEFT && c2 == UP) {
            std::cout << "RLF2L 7, FIXED INSERTION" << std::endl;
            return {"U", "R'", "U", "U", "R"}; //"U", "F", "U'", "F'"
        }

    }

    // =====================================================
    //                 RLF2L 17/18 (white UP)  -- cleaned
    // =====================================================
    if (whiteOnCorner == UP)
    {
        if (c1 == LEFT && c2 == UP) {
            std::cout << "RLF2L 17" << std::endl;
            return {"F","U","U","F'","U'"};   // kept as you wrote
        }
        if (c1 == UP && c2 == FRONT) {
            std::cout << "RLF2L 18" << std::endl;
            return {"R'", "U'", "R", "U", "R'", "U'", "R"};  // kept as you wrote
        }
        if (c1 == UP && c2 == BACK) {
            std::cout << "RLF2L 19" << std::endl;
            return {"F","U'","F'", "U"};     // kept as you wrote
        }
        if (c1 == UP && c2 == LEFT) {
            std::cout << "RLF2L 20" << std::endl;
            return {"R'","U","R", "U"};            // kept as you wrote
        }
        if (c1 == BACK && c2 == UP) {
            std::cout << "RLF2L 19 (reversed)" << std::endl;
            return {"F","U'","F'"};           // kept as you wrote
        }
        if (c1 == UP && c2 == RIGHT) {
            std::cout << "RLF2L 24" << std::endl;
            return {"F","U","U", "F'", "U"};            // kept as you wrote

        }
        if (c1 == RIGHT && c2 == UP) {
            std::cout << "RLF2L 17 (reversed)" << std::endl;
            return {"F","U","U","F'"};        // kept as you wrote
        }
        if (c1 == FRONT && c2 == UP) {
            std::cout << "RLF2L 23 (reversed)" << std::endl;
            return {"R'","U","U","R"};        // kept as you wrote
        }
    }

    // =====================================================
    // NO MATCH → FEED FORWARD WITH U
    // =====================================================
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

void Solver::debugAllFaceColors() const {
    std::cout << "           COMPLETE CUBE FACE COLOR DEBUG                    " << std::endl;

    const auto& current = cube->getCurrentPosition();

    // Iterate through all 26 pieces
    for (int i = 0; i < 26; i++) {
        glm::ivec3 pos = current[i];
        Cubelet* piece = cube->getCubelet(pos);

        if (!piece) continue;

        std::cout << "Piece ID: " << i << " at Position ("
                  << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

        // Check each face and show both coordinate and color
        std::vector<std::pair<Face, std::string>> faces = {
            {UP, "UP    (Y=+1)"},
            {DOWN, "DOWN  (Y=-1)"},
            {FRONT, "FRONT (Z=+1)"},
            {BACK, "BACK  (Z=-1)"},
            {RIGHT, "RIGHT (X=+1)"},
            {LEFT, "LEFT  (X=-1)"}
        };

        for (const auto& [face, faceName] : faces) {
            char colorChar = getFaceColor(piece, face);
            color colorRGB = piece->getFaceColor(face);

            // Only show non-black faces (actual visible faces)
            if (colorChar != '?') {
                std::cout << "│   " << faceName << ": "
                          << colorChar << " RGB("
                          << colorRGB.red << ", "
                          << colorRGB.green << ", "
                          << colorRGB.blue << ")" << std::endl;
            }
        }
    }
}

void Solver::debugSpecificCubelet(glm::ivec3 pos) const {
    std::cout << "          DETAILED CUBELET DEBUG at ("
              << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

    Cubelet* piece = cube->getCubelet(pos);
    if (!piece) {
        std::cout << "ERROR: No cubelet found at this position!" << std::endl;
        return;
    }

    // Determine piece type
    int nonZeroCoords = (pos.x != 0) + (pos.y != 0) + (pos.z != 0);
    std::string pieceType;
    if (nonZeroCoords == 3) pieceType = "CORNER";
    else if (nonZeroCoords == 2) pieceType = "EDGE";
    else pieceType = "CENTER";

    std::cout << "\nPiece Type: " << pieceType << std::endl;
    std::cout << "                    FACE ANALYSIS                        " << std::endl;

    std::vector<std::tuple<Face, std::string, std::string>> faceDetails = {
        {UP,    "UP   ", "  Coordinate: Y = +1"},
        {DOWN,  "DOWN ", "  Coordinate: Y = -1"},
        {FRONT, "FRONT", "  Coordinate: Z = +1"},
        {BACK,  "BACK ", "  Coordinate: Z = -1"},
        {RIGHT, "RIGHT", "  Coordinate: X = +1"},
        {LEFT,  "LEFT ", "  Coordinate: X = -1"}
    };

    for (const auto& [face, faceName, coordinate] : faceDetails) {
        char colorChar = getFaceColor(piece, face);
        color colorRGB = piece->getFaceColor(face);

        std::cout << "│ " << faceName << ": ";

        if (colorRGB.red == 0.0f && colorRGB.green == 0.0f && colorRGB.blue == 0.0f) {
            std::cout << "[INTERNAL - No sticker]" << std::endl;
        } else {
            std::cout << colorChar << " - RGB("
                      << std::fixed << std::setprecision(2)
                      << colorRGB.red << ", "
                      << colorRGB.green << ", "
                      << colorRGB.blue << ")" << std::endl;
            std::cout << "│        " << coordinate << std::endl;
        }
    }
}

void Solver::debugAllF2LPositions() const {
    std::cout << "              F2L PAIRS POSITION DEBUG                         " << std::endl;

    std::vector<F2LPair> pairs = getF2LPairs();
    const auto& current = cube->getCurrentPosition();

    for (size_t i = 0; i < pairs.size(); i++) {
        const auto& pair = pairs[i];

        std::cout << " F2L SLOT " << (i + 1) << ": " << pair.color1 << "-" << pair.color2 << std::endl;

        // Corner analysis
        if (pair.cornerID != -1) {
            glm::ivec3 cornerPos = current[pair.cornerID];
            Cubelet* corner = cube->getCubelet(cornerPos);

            std::cout << " CORNER (ID " << pair.cornerID << "):" << std::endl;
            std::cout << "   Current Position: (" << cornerPos.x << ", " << cornerPos.y << ", " << cornerPos.z << ")" << std::endl;
            std::cout << "   Target Position:  (" << pair.targetCornerPos.x << ", " << pair.targetCornerPos.y << ", " << pair.targetCornerPos.z << ")" << std::endl;

            if (corner) {
                std::cout << "   Colors on piece:" << std::endl;
                for (Face face : {UP, DOWN, FRONT, BACK, RIGHT, LEFT}) {
                    char c = getFaceColor(corner, face);
                    if (c != '?') {
                        std::cout << "     " << faceToString(face) << ": " << c << std::endl;
                    }
                }
            }
        } else {
            std::cout << " CORNER: NOT FOUND!" << std::endl;
        }

        std::cout << std::endl;

        // Edge analysis
        if (pair.edgeID != -1) {
            glm::ivec3 edgePos = current[pair.edgeID];
            Cubelet* edge = cube->getCubelet(edgePos);

            std::cout << " EDGE (ID " << pair.edgeID << "):" << std::endl;
            std::cout << "   Current Position: (" << edgePos.x << ", " << edgePos.y << ", " << edgePos.z << ")" << std::endl;
            std::cout << "   Target Position:  (" << pair.targetEdgePos.x << ", " << pair.targetEdgePos.y << ", " << pair.targetEdgePos.z << ")" << std::endl;

            if (edge) {
                std::cout << "   Colors on piece:" << std::endl;
                for (Face face : {UP, DOWN, FRONT, BACK, RIGHT, LEFT}) {
                    char c = getFaceColor(edge, face);
                    if (c != '?') {
                        std::cout << "     " << faceToString(face) << ": " << c << std::endl;
                    }
                }
            }
        } else {
            std::cout << " EDGE: NOT FOUND!" << std::endl;
        }

        std::cout << std::endl;
        std::cout << " Status: " << (isF2LPairSolved(pair) ? "✓ SOLVED" : "✗ UNSOLVED") << std::endl;
    }
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
        if (edgeMatchesCenter_Fixed(i)) {
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
void Solver::dumpCentersAndUEdges() const {
    std::cout << "=== CENTER COLORS ===\n";
    std::vector<std::pair<Face, glm::ivec3>> centers = {
        {FRONT, {0,0,1}}, {RIGHT, {1,0,0}}, {BACK, {0,0,-1}}, {LEFT, {-1,0,0}},
        {UP, {0,1,0}}, {DOWN, {0,-1,0}}
    };
    for (auto &p : centers) {
        const Cubelet* c = cube->getCubelet(p.second);
        if (!c) { std::cout << faceToString(p.first) << ": NULL\n"; continue; }
        color col = c->getFaceColor(p.first);
        std::cout << faceToString(p.first) << ": " << colorToChar(col) << " (" << col.red << "," << col.green << "," << col.blue << ")\n";
    }

    std::cout << "=== U-LAYER EDGES (face & outward sticker) ===\n";
    static const glm::ivec3 edgePos[4] = {{0,1,1},{1,1,0},{0,1,-1},{-1,1,0}};
    static const Face faceMap[4] = {FRONT, RIGHT, BACK, LEFT};
    for (int i=0;i<4;i++) {
        const Cubelet* e = cube->getCubelet(edgePos[i]);
        if (!e) { std::cout << "Edge " << i << ": NULL\n"; continue; }
        color c = e->getFaceColor(faceMap[i]);
        std::cout << "Edge " << i << " pos("<<edgePos[i].x<<","<<edgePos[i].y<<","<<edgePos[i].z<<") "
                  << "face="<<faceToString(faceMap[i])<<" -> "<<colorToChar(c)<<"\n";
    }
}


int Solver::countPermutedEdges() {
    int n = 0;
    if (edgeMatchesCenter_Fixed(FRONT)) n++;
    if (edgeMatchesCenter_Fixed(RIGHT)) n++;
    if (edgeMatchesCenter_Fixed(BACK))  n++;
    if (edgeMatchesCenter_Fixed(LEFT))  n++;
    return n;
}


bool Solver::adjacentCorrect() {
    // Check if two adjacent edges are correct
    return (edgeMatchesCenter_Fixed(0) && edgeMatchesCenter_Fixed(1)) ||  // Front-Right
           (edgeMatchesCenter_Fixed(1) && edgeMatchesCenter_Fixed(2)) ||  // Right-Back
           (edgeMatchesCenter_Fixed(2) && edgeMatchesCenter_Fixed(3)) ||  // Back-Left
           (edgeMatchesCenter_Fixed(3) && edgeMatchesCenter_Fixed(0));    // Left-Front
}


bool Solver::oppositeCorrect() {
    // Check if two opposite edges are correct
    return (edgeMatchesCenter_Fixed(0) && edgeMatchesCenter_Fixed(2)) ||  // Front-Back
           (edgeMatchesCenter_Fixed(1) && edgeMatchesCenter_Fixed(3));    // Right-Left
}

std::vector<std::string> Solver::solveLastLayerEdges_Fixed() {
    int aligned = countAlignedEdges_Fixed();

    if (aligned == 4) {
        std::cout << "All edges aligned!" << std::endl;
        return {};
    }

    if (aligned == 0) {
        std::cout << "No edges aligned, rotating U" << std::endl;
        return {"U"};
    }

    if (aligned == 2) {
        bool match0 = edgeMatchesCenter_Fixed(0);
        bool match1 = edgeMatchesCenter_Fixed(1);
        bool match2 = edgeMatchesCenter_Fixed(2);
        bool match3 = edgeMatchesCenter_Fixed(3);

        bool adjacent = (match0 && match1) || (match1 && match2) ||
                       (match2 && match3) || (match3 && match0);

        if (adjacent) {
            std::cout << "Two adjacent edges aligned" << std::endl;

            if (!(match0 && match1)) {
                return {"U"};
            }

            Cubelet* rightEdge = cube->getCubelet({1, 1, 0});
            Cubelet* frontCenter = cube->getCubelet({0, 0, 1});

            color rightEdgeColor = rightEdge->getFaceColor(RIGHT);
            color frontCenterColor = frontCenter->getFaceColor(BACK);

            bool rightGoesToFront = sameColor(rightEdgeColor, frontCenterColor);

            if (rightGoesToFront) {
                std::cout << "Using Ua (counterclockwise)" << std::endl;
                return {"R'", "U", "R'", "U'", "R'", "U'", "R'", "U", "R", "U", "R'", "R'"};
            } else {
                std::cout << "Using Ub (clockwise)" << std::endl;
                return {"R'", "R'", "U'", "R'", "U'", "R", "U", "R", "U", "R", "U'", "R"};
            }
        }

        std::cout << "Two opposite edges aligned" << std::endl;

        if (!match0) {
            return {"U"};
        }

        Cubelet* rightEdge = cube->getCubelet({1, 1, 0});
        Cubelet* frontCenter = cube->getCubelet({0, 0, 1});

        color rightEdgeColor = rightEdge->getFaceColor(RIGHT);
        color frontCenterColor = frontCenter->getFaceColor(BACK);

        bool rightGoesToFront = sameColor(rightEdgeColor, frontCenterColor);

        if (rightGoesToFront) {
            std::cout << "Using Ua" << std::endl;
            return {"R'", "U", "R'", "U'", "R'", "U'", "R'", "U", "R", "U", "R'", "R'"};
        } else {
            std::cout << "Using Ub" << std::endl;
            return {"R'", "R'", "U'", "R'", "U'", "R", "U", "R", "U", "R", "U'", "R"};
        }
    }

    std::cout << "Unexpected alignment count: " << aligned << ", rotating U" << std::endl;
    return {"U"};
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

    // NO PAIR FOUND → return U-perm trigger
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

    // int count = 0;
    //
    // if (cornerInCorrectLocation_Fixed({1, 1, 1})) count++;
    // if (cornerInCorrectLocation_Fixed({1, 1, -1})) count++;
    // if (cornerInCorrectLocation_Fixed({-1, 1, -1})) count++;
    // if (cornerInCorrectLocation_Fixed({-1, 1, 1})) count++;
    //
    // std::cout << "Total correct: " << count << "/4" << std::endl;

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

glm::ivec3 Solver::getFirstCorrectCorner_Fixed() {
    if (cornerInCorrectLocation_Fixed({1, 1, 1})) return {1, 1, 1};
    if (cornerInCorrectLocation_Fixed({1, 1, -1})) return {1, 1, -1};
    if (cornerInCorrectLocation_Fixed({-1, 1, -1})) return {-1, 1, -1};
    if (cornerInCorrectLocation_Fixed({-1, 1, 1})) return {-1, 1, 1};
    return {0, 0, 0};
}

std::vector<std::string> Solver::solveLastLayerCorners_Fixed() {
    std::cout << "\n=== FINAL PLL CORNER PHASE ===" << std::endl;

    // First check if cube is actually solved
    if (isCubeActuallySolved()) {
        std::cout << "CUBE SOLVED!" << std::endl;
        currentState = WCCOMPLETE;
        return {};
    }

    // Check if all corners have yellow on UP
    int oriented = countCorrectCornerOrientations();
    if (oriented < 4) {
        std::cout << oriented << "/4 corners oriented, applying Sune" << std::endl;
        return {"R'", "U'", "R", "U'", "R'", "U'", "U'", "R"};
    }

    std::cout << "All corners have yellow on UP ✓" << std::endl;

    // Now we have oriented corners but wrong positions
    // Check for A-perm case (adjacent corner swap)
    std::cout << "Checking for adjacent corner swap..." << std::endl;

    // Try up to 4 U moves to find the right alignment for A-perm
    for (int i = 0; i < 4; i++) {
        // Check if we have the A-perm pattern
        if (hasAdjacentCornerSwapPattern()) {
            std::cout << "Adjacent corner swap detected, applying clockwise A-perm" << std::endl;
            return {"R'", "U'", "R", "U", "R", "F'", "R'", "R'", "U", "R", "U", "R'", "U'", "R", "F"};
        }

        // Rotate U and check again
        if (i < 3) {
            std::cout << "Rotating U to find correct A-perm alignment" << std::endl;
            return {"U"};
        }
    }

    // Default: try A-perm anyway
    std::cout << "Applying A-perm (default)" << std::endl;
    return {"R'", "U'", "R", "U", "R", "F'", "R'", "R'", "U", "R", "U", "R'", "U'", "R", "F"};
}

bool Solver::isCubeActuallySolved() {
    // Check edges
    if (!edgeMatchesCenter_Fixed(0) || !edgeMatchesCenter_Fixed(1) ||
        !edgeMatchesCenter_Fixed(2) || !edgeMatchesCenter_Fixed(3)) {
        return false;
        }

    // Check corners using the new areCornersSolved_Fixed()
    return areCornersSolved_Fixed();
}

bool Solver::hasAdjacentCornerSwapPattern() {
    // Check corner at UFR (1,1,1)
    Cubelet* urfCorner = cube->getCubelet({1, 1, 1});
    if (!urfCorner) return false;

    char urfRight = getFaceColor(urfCorner, RIGHT);
    char urfBack = getFaceColor(urfCorner, BACK);  // Shows at FRONT position

    std::cout << "UFR corner: R=" << urfRight << " B=" << urfBack << std::endl;

    // In A-perm case (adjacent swap), the UFR corner will have:
    // - Either the RIGHT color correct but FRONT wrong, OR
    // - The FRONT color correct but RIGHT wrong
    bool rightCorrect = (urfRight == 'R');
    bool frontCorrect = (urfBack == 'G');  // G should be on BACK face (shows at FRONT)

    std::cout << "  Right correct: " << (rightCorrect ? "YES" : "NO")
              << ", Front correct: " << (frontCorrect ? "YES" : "NO") << std::endl;

    // Adjacent swap: corner has exactly ONE correct color
    return (rightCorrect && !frontCorrect) || (!rightCorrect && frontCorrect);
}

std::vector<std::string> Solver::determineAPermDirection() {
    // Check if corner at UFR wants to go to URB or ULF
    Cubelet* corner = cube->getCubelet({1, 1, 1});
    if (!corner) return {"U"};

    // Get the color that should be on the RIGHT face
    char rightColor = getFaceColor(corner, RIGHT);

    // Check if this matches the RIGHT center
    Cubelet* rightCenter = cube->getCubelet({1, 0, 0});
    char centerRightColor = getFaceColor(rightCenter, RIGHT);

    std::cout << "Corner RIGHT color: " << rightColor
              << ", Center RIGHT color: " << centerRightColor << std::endl;

    if (rightColor == centerRightColor) {
        // Corner is already matched on RIGHT face, use clockwise A-perm
        std::cout << "Using clockwise A-perm" << std::endl;
        return {
            "R", "F'", "R", "B'", "B'", "R'", "F", "R",
            "B'", "B'", "R'", "R'"
        };
    } else {
        // Use counter-clockwise A-perm
        std::cout << "Using counter-clockwise A-perm" << std::endl;
        return {
            "R'", "R'", "B'", "B'", "R'", "F'", "R",
            "B'", "B'", "R'", "F", "R"
        };
    }
}

// Add this function to orient corners (Sune/antisune algorithms)
std::vector<std::string> Solver::orientLastLayerCorners() {
    std::cout << "=== CORNER ORIENTATION ===" << std::endl;

    // Check for various OLL cases
    // For simplicity, use Sune algorithm
    std::cout << "Applying Sune algorithm for corner orientation" << std::endl;
    return {
        "R'", "U'", "R", "U'", "R'", "U'", "U'", "R"
    };
}

    bool Solver::cornerMatchesCenters(const glm::ivec3 &pos){
    // Corner positions on top layer (Y=1)
    static const glm::ivec3 cornerPos[4] = {
        { 1, 1,  1}, // UFR (Front-Right)
        { 1, 1, -1}, // URB (Right-Back)
        {-1, 1, -1}, // UBL (Back-Left)
        {-1, 1,  1}  // ULF (Left-Front)
    };


    // After X X rotation (180° around X-axis), faces are flipped:
    // - FRONT face is now BACK face
    // - BACK face is now FRONT face
    // - UP face is now DOWN face
    // - DOWN face is now UP face
    // - LEFT and RIGHT stay the same

    const Cubelet* corner = cube->getCubelet(pos);
    if (!corner) {
        std::cout << "  Corner at (" << pos.x << "," << pos.y << "," << pos.z << "): ERROR - missing\n";
        return false;
    }

    // Get the corner's 3 visible colors
    std::vector<char> cornerColors;

    // Check all 6 faces, collect non-internal colors
    std::vector<Face> facess = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
    for (Face f : facess) {
        color c = corner->getFaceColor(f);
        char ch = colorToChar(c);
        if (ch != '?') {  // Skip internal/black faces
            cornerColors.push_back(ch);
        }
    }

    if (cornerColors.size() != 3) {
        std::cout << "  Corner has " << cornerColors.size() << " colors (should be 3)\n";
        return false;
    }

    // Get the 3 center colors adjacent to this corner
    std::vector<char> centerColors;

    // Determine which centers are adjacent based on corner position
    // X-axis: RIGHT or LEFT
    if (pos.x == 1) {
        const Cubelet* rightCenter = cube->getCubelet({1, 0, 0});
        if (rightCenter) {
            char c = colorToChar(rightCenter->getFaceColor(RIGHT));
            if (c != '?') centerColors.push_back(c);
        }
    } else if (pos.x == -1) {
        const Cubelet* leftCenter = cube->getCubelet({-1, 0, 0});
        if (leftCenter) {
            char c = colorToChar(leftCenter->getFaceColor(LEFT));
            if (c != '?') centerColors.push_back(c);
        }
    }

    // Z-axis: FRONT or BACK (with XX flip correction)
    if (pos.z == 1) {
        const Cubelet* frontCenter = cube->getCubelet({0, 0, 1});
        if (frontCenter) {
            // After XX rotation, check BACK face (was FRONT before flip)
            char c = colorToChar(frontCenter->getFaceColor(BACK));
            if (c != '?') centerColors.push_back(c);
        }
    } else if (pos.z == -1) {
        const Cubelet* backCenter = cube->getCubelet({0, 0, -1});
        if (backCenter) {
            // After XX rotation, check FRONT face (was BACK before flip)
            char c = colorToChar(backCenter->getFaceColor(FRONT));
            if (c != '?') centerColors.push_back(c);
        }
    }

    // Y-axis: UP (at Y=1 after XX flip, the "up" center's DOWN face is visible)
    if (pos.y == 1) {
        const Cubelet* topCenter = cube->getCubelet({0, 1, 0});
        if (topCenter) {
            // After XX flip, UP/DOWN are swapped, check DOWN face
            char c = colorToChar(topCenter->getFaceColor(DOWN));
            if (c != '?') centerColors.push_back(c);
        }
    }

    if (centerColors.size() != 3) {
        std::cout << "  Found " << centerColors.size() << " center colors (should be 3)\n";
        return false;
    }

    // Sort both to compare (order doesn't matter for location correctness)
    std::sort(cornerColors.begin(), cornerColors.end());
    std::sort(centerColors.begin(), centerColors.end());

    bool matches = (cornerColors == centerColors);

    std::cout << "  Corner at (" << pos.x << "," << pos.y << "," << pos.z << "): ";
    std::cout << "Corner={" << cornerColors[0] << cornerColors[1] << cornerColors[2] << "}, ";
    std::cout << "Centers={" << centerColors[0] << centerColors[1] << centerColors[2] << "} = ";
    std::cout << (matches ? "CORRECT" : "WRONG") << "\n";

    return matches;
}

int Solver::findCorrectCornerIndex() {
    for (int i = 0; i < 4; i++)
        if (cornerIsCorrect(i))
            return i;
    return -1;
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
    debugCurrentCornerState();

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
        char back0 = getFaceColor(corner0, BACK);  // Shows at FRONT position

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
        char front1 = getFaceColor(corner1, FRONT);  // Shows at BACK position

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
        char back3 = getFaceColor(corner3, BACK);  // Shows at FRONT position

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

void Solver::debugCurrentCornerState() {
    std::cout << "\n=== CURRENT CORNER STATE ===" << std::endl;

    static const glm::ivec3 corners[4] = {
        {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}, {-1, 1, 1}
    };

    for (int i = 0; i < 4; i++) {
        glm::ivec3 pos = corners[i];
        Cubelet* corner = cube->getCubelet(pos);

        std::cout << "Corner " << i << " at (" << pos.x << "," << pos.y << "," << pos.z << "): ";

        if (!corner) {
            std::cout << "NULL" << std::endl;
            continue;
        }

        // Show all visible colors
        std::vector<Face> faces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};
        for (Face face : faces) {
            char color = getFaceColor(corner, face);
            if (color != '?') {
                std::cout << faceToString(face) << "=" << color << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "=============================" << std::endl;
}

std::vector<std::string> Solver::finalAUF() {

    // Try up to 4 orientations
    for (int i = 0; i < 4; i++) {

        // Check edges
        bool edgesGood =
            edgeMatchesCenter_Fixed(0) &&
            edgeMatchesCenter_Fixed(1) &&
            edgeMatchesCenter_Fixed(2) &&
            edgeMatchesCenter_Fixed(3);

        // Check corners
        bool cornersGood =
            cornerMatches(0) &&
            cornerMatches(1) &&
            cornerMatches(2) &&
            cornerMatches(3);

        if (edgesGood && cornersGood) {
            std::cout << "AUF complete — cube solved!\n";
            return {};  // no moves needed
        }

        // If not solved, rotate U and check again
        std::cout << "AUF rotating U\n";
        return {"U"};
    }

    // Should never reach here
    return {};
}


glm::ivec3 Solver::centerPos(Face f) {
    switch (f) {
        case UP:    return glm::ivec3(0, 1, 0);
        case DOWN:  return glm::ivec3(0, -1, 0);
        case FRONT: return glm::ivec3(0, 0, 1);
        case BACK:  return glm::ivec3(0, 0, -1);
        case LEFT:  return glm::ivec3(-1, 0, 0);
        case RIGHT: return glm::ivec3(1, 0, 0);
        default:    return glm::ivec3(0, 0, 0);
    }
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
    std::cout << "==================================================" << std::endl;
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

// Debug display functions
void Solver::displayDebugInfo() const {
    std::cout << "\n========================================" << std::endl;
    std::cout << "         DEBUG FREEZE ACTIVE" << std::endl;
    std::cout << "========================================" << std::endl;

    displayCurrentState();
    displayPlannedMoves();

    std::cout << "\nControls:" << std::endl;
    std::cout << "  F - Toggle freeze (continue/pause)" << std::endl;
    std::cout << "  G - Step through one move" << std::endl;
    std::cout << "  H - Toggle step-through mode" << std::endl;
    std::cout << "  J - Show current piece positions" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void Solver::displayCurrentState() const {
    std::cout << "\n--- CURRENT STATE ---" << std::endl;
    std::cout << "Solving step: " << currentStep << " ";

    switch(currentStep) {
        case 0: std::cout << "(White Cross)"; break;
        case 1: std::cout << "(F2L)"; break;
        case 2: std::cout << "(OLL)"; break;
        case 3: std::cout << "(PLL Edges)"; break;
        case 4: std::cout << "(PLL Corners)"; break;
        default: std::cout << "(Unknown)"; break;
    }
    std::cout << std::endl;

    if (currentStep == 0) {
        std::cout << "Target edge: W-" << currentTargetColor << std::endl;
    } else if (currentStep == 1) {
        std::cout << "F2L slot: " << (currentF2LSlot + 1) << "/4" << std::endl;
    }

    std::cout << "Moves executed: " << moveCounter << std::endl;
}

void Solver::displayPlannedMoves() const {
    std::cout << "\n--- PLANNED MOVES ---" << std::endl;
    if (currentMoves.empty()) {
        std::cout << "No moves queued (will generate on next call)" << std::endl;
    } else {
        std::cout << "Next " << currentMoves.size() << " moves: ";
        for (size_t i = 0; i < std::min(size_t(10), currentMoves.size()); i++) {
            std::cout << currentMoves[i];
            if (i < currentMoves.size() - 1) std::cout << " ";
        }
        if (currentMoves.size() > 10) {
            std::cout << " ... (+" << (currentMoves.size() - 10) << " more)";
        }
        std::cout << std::endl;
    }
}

void Solver::debugExpectedCornerColors() {
    std::cout << "\n=== EXPECTED CORNER COLORS IN SOLVED STATE ===" << std::endl;
    std::cout << "After XX rotation (cube flipped 180° around X):" << std::endl;
    std::cout << "- UP face shows DOWN face colors" << std::endl;
    std::cout << "- FRONT face shows BACK face colors" << std::endl;
    std::cout << "- BACK face shows FRONT face colors" << std::endl;
    std::cout << "- RIGHT/LEFT faces unchanged" << std::endl;

    std::cout << "\nIn solved state, corners should have:" << std::endl;
    std::cout << "1. Corner at (1,1,1) - UFR: UP=Y, FRONT=G, RIGHT=R" << std::endl;
    std::cout << "   After XX: visible faces are UP=Y, BACK=G, RIGHT=R" << std::endl;

    std::cout << "2. Corner at (1,1,-1) - URB: UP=Y, BACK=B, RIGHT=R" << std::endl;
    std::cout << "   After XX: visible faces are UP=Y, FRONT=B, RIGHT=R" << std::endl;

    std::cout << "3. Corner at (-1,1,-1) - UBL: UP=Y, BACK=B, LEFT=O" << std::endl;
    std::cout << "   After XX: visible faces are UP=Y, FRONT=B, LEFT=O" << std::endl;

    std::cout << "4. Corner at (-1,1,1) - ULF: UP=Y, FRONT=G, LEFT=O" << std::endl;
    std::cout << "   After XX: visible faces are UP=Y, BACK=G, LEFT=O" << std::endl;

    std::cout << "=================================================" << std::endl;
}