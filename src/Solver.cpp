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

    // Safety check to prevent infinite loops
    // if (moveCounter++ > MAX_MOVES) {
    //     std::cout << "ERROR: Solver stuck in infinite loop! Aborting." << std::endl;
    //     currentState = FAILED;
    //     return "";
    // }

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
        if (!orientationResetDone)
        {
            resetOrientationAfterF2L();
            orientationResetDone = true;
        }


        std::cout << "=== PLL EDGE PERMUTATION ===" << std::endl;

        currentMoves = solveLastLayerEdges();

        if (!currentMoves.empty()) {
            std::string nextMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());
            std::cout << "PLL Edge move: " << nextMove << std::endl;
            return nextMove;
        }

        // Check if edges are aligned
        if (countAlignedEdges() == 4) {
            std::cout << "==================================\n";
            std::cout << " PLL EDGES COMPLETE!\n";
            std::cout << "==================================\n";
            currentStep = 4;  // Move to corner permutation
            return "";
        }
    }

    // ====== PLL - Corner Permutation (Step 4) ======
    if (currentStep == 4) {
        std::cout << "=== PLL CORNER PERMUTATION ===" << std::endl;

        // If no moves buffered yet, compute them
        if (currentMoves.empty()) {
            currentMoves = solveLastLayerCorners();
        }

        // If we still have moves to execute, pop next move
        if (!currentMoves.empty()) {
            std::string nextMove = currentMoves.front();
            currentMoves.erase(currentMoves.begin());
            std::cout << "PLL Corner move: " << nextMove << std::endl;
            return nextMove;
        }


        // After executing moves, check if corners are fully solved
        if (areCornersSolved()) {
            currentMoves = {"R'", "X'", "L","R'", "X'", "L"};
            std::cout << "==================================\n";
            std::cout << " PLL CORNERS COMPLETE!\n";
            std::cout << " 🎉 CUBE SOLVED! 🎉\n";
            std::cout << "==================================\n";

            currentState = WCCOMPLETE;
            return "";
        }

        // If algorithm is done but corners still wrong → rotate U and retry
        return {};
    }
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

int Solver::countAlignedEdges() {
    int count = 0;

    if (edgeMatchesCenter(0)) count++;
    if (edgeMatchesCenter(1)) count++;
    if (edgeMatchesCenter(2))  count++;
    if (edgeMatchesCenter(3))  count++;

    std::cout << "[countAlignedEdges] Aligned edges: " << count << "\n";
    return count;
}


bool Solver::edgeMatchesCenter(int faceIndex) {
    // Edge positions on U layer
    static const glm::ivec3 edgePos[4] = {
        { 0, 1,  1}, // FRONT
        { 1, 1,  0}, // RIGHT
        { 0, 1, -1}, // BACK
        {-1, 1,  0}  // LEFT
    };

    // Center positions of side faces
    static const glm::ivec3 centerPos[4] = {
        { 0, 0,  1}, // FRONT
        { 1, 0,  0}, // RIGHT
        { 0, 0, -1}, // BACK
        {-1, 0,  0}  // LEFT
    };

    const Cubelet* edge   = cube->getCubelet(edgePos[faceIndex]);
    const Cubelet* center = cube->getCubelet(centerPos[faceIndex]);
    if (!edge || !center) return false;

    Face checkFace = FRONT;
    if (faceIndex == 1) checkFace = RIGHT;
    if (faceIndex == 2) checkFace = BACK;
    if (faceIndex == 3) checkFace = LEFT;

    // FIX: read edge sticker, not center
    color edgeColor   = colorToChar(checkFace);
    color centerColor = colorToChar(checkFace);

    bool matches = sameColor(edgeColor, centerColor);

    std::cout << "  Edge " << faceIndex << ": "
              << colorToChar(edgeColor) << " vs "
              << colorToChar(centerColor) << " = "
              << (matches ? "YES" : "NO") << "\n";

    return matches;
}



// PLL

int Solver::countPermutedEdges() {
    int n = 0;
    if (edgeMatchesCenter(FRONT)) n++;
    if (edgeMatchesCenter(RIGHT)) n++;
    if (edgeMatchesCenter(BACK))  n++;
    if (edgeMatchesCenter(LEFT))  n++;
    return n;
}


bool Solver::adjacentCorrect() {
    // Check if two adjacent edges are correct
    return (edgeMatchesCenter(0) && edgeMatchesCenter(1)) ||  // Front-Right
           (edgeMatchesCenter(1) && edgeMatchesCenter(2)) ||  // Right-Back
           (edgeMatchesCenter(2) && edgeMatchesCenter(3)) ||  // Back-Left
           (edgeMatchesCenter(3) && edgeMatchesCenter(0));    // Left-Front
}


bool Solver::oppositeCorrect() {
    // Check if two opposite edges are correct
    return (edgeMatchesCenter(0) && edgeMatchesCenter(2)) ||  // Front-Back
           (edgeMatchesCenter(1) && edgeMatchesCenter(3));    // Right-Left
}

std::vector<std::string> Solver::solveLastLayerEdges() {
    // if (solveCheck <= 4) {
    //     solveCheck++;
        // 1. If edges are already solved
        if (countAlignedEdges() == 4)
            return {};

        // 2. get rotation or algorithm from helper
        std::vector<std::string> moves = findCorrectEdgePair();

        // If moves is empty → pair is already in BACK → now run Ua/Ub
        if (moves.empty()) {

            bool isUa = edgeMatchesCenter(FRONT) && edgeMatchesCenter(RIGHT);
            bool isUb = edgeMatchesCenter(LEFT) && edgeMatchesCenter(FRONT);

            if (isUa) {
                std::cout << "Executing Ua\n";
                return {"R'", "U'", "U'", "R", "U'", "R'", "U'", "R", "U"};
            } else {
                std::cout << "Executing Ub\n";
                return {"L", "U", "U", "L'", "U", "L", "U", "L'", "U"};
            }
        }


        // Otherwise return the rotation or trigger algorithm

    return findCorrectEdgePair();
;
}

std::vector<std::string> Solver::findCorrectEdgePair() {
    // 0 = FRONT, 1 = RIGHT, 2 = BACK, 3 = LEFT
    if (!edgeMatchesCenter(FRONT) && !edgeMatchesCenter(RIGHT) && !edgeMatchesCenter(BACK) && !edgeMatchesCenter(LEFT)) {
        return {"U"};
    }
    // Pair at FRONT–RIGHT → rotate U2 to move it to BACK
    if (edgeMatchesCenter(FRONT) && edgeMatchesCenter(RIGHT))
        return {"U'", "U'"};   // rotate twice

    // Pair at RIGHT–BACK → rotate U once
    if (edgeMatchesCenter(RIGHT) && edgeMatchesCenter(BACK))
        return {"U'"};          // rotate one time

    // Pair already at BACK–LEFT → no moves
    if (edgeMatchesCenter(BACK) && edgeMatchesCenter(LEFT))
        return {};              // already in correct spot

    // Pair at LEFT–FRONT → rotate U once forward (U)
    if (edgeMatchesCenter(LEFT) && edgeMatchesCenter(FRONT))
        return {"U"};           // rotate the other way

    // NO PAIR FOUND → return U-perm trigger
    return {
        "R'", "U", "U", "R", "U", "R'", "U", "R",
        "R'", "U", "U", "R", "U", "R'", "U", "R", "U", "U"
    };
}



// PLL Solving the Corners
bool Solver::cornerInCorrectLocation(glm::ivec3 pos) {
    const Cubelet* corner = cube->getCubelet(pos);
    if (!corner) return false;

    // A corner is in correct location if its 3 sticker colors match
    // the 3 adjacent center colors (in any order/orientation)

    // Get the 3 center colors adjacent to this corner position
    std::vector<char> centerColors;

    // Determine which centers are adjacent based on position
    // Corner positions have all coords = ±1
    if (pos.x == 1) {
        // Right center
        const Cubelet* rightCenter = cube->getCubelet({1, 0, 0});
        if (rightCenter) {
            centerColors.push_back(colorToChar(rightCenter->getFaceColor(RIGHT)));
        }
    } else if (pos.x == -1) {
        // Left center
        const Cubelet* leftCenter = cube->getCubelet({-1, 0, 0});
        if (leftCenter) {
            centerColors.push_back(colorToChar(leftCenter->getFaceColor(LEFT)));
        }
    }

    if (pos.z == 1) {
        // Front center (but after XX rotation, check BACK face)
        const Cubelet* frontCenter = cube->getCubelet({0, 0, 1});
        if (frontCenter) {
            centerColors.push_back(colorToChar(frontCenter->getFaceColor(BACK)));
        }
    } else if (pos.z == -1) {
        // Back center (but after XX rotation, check FRONT face)
        const Cubelet* backCenter = cube->getCubelet({0, 0, -1});
        if (backCenter) {
            centerColors.push_back(colorToChar(backCenter->getFaceColor(FRONT)));
        }
    }

    // Top/Bottom (Y axis) - after XX rotation, UP and DOWN are swapped
    if (pos.y == 1) {
        // Currently at Y=1, which is still "up" visually but was "down" before
        // The UP face points down, so we check DOWN face
        const Cubelet* topCenter = cube->getCubelet({0, 1, 0});
        if (topCenter) {
            // After XX flip, the DOWN face of this center is what's "up"
            centerColors.push_back(colorToChar(topCenter->getFaceColor(DOWN)));
        }
    }

    // Get the corner's 3 visible colors (not yellow/white from top/bottom)
    std::vector<char> cornerColors;
    std::vector<Face> allFaces = {UP, DOWN, FRONT, BACK, RIGHT, LEFT};

    for (Face f : allFaces) {
        char c = getFaceColor(corner, f);
        if (c != '?') {  // Skip internal faces
            cornerColors.push_back(c);
        }
    }

    // Must have exactly 3 colors
    if (cornerColors.size() != 3 || centerColors.size() != 3) {
        return false;
    }

    // Check if the corner's 3 colors match the 3 center colors (unordered)
    std::sort(centerColors.begin(), centerColors.end());
    std::sort(cornerColors.begin(), cornerColors.end());

    bool matches = (centerColors == cornerColors);

    std::cout << "  Corner at (" << pos.x << "," << pos.y << "," << pos.z << "): ";
    std::cout << "Centers={" << centerColors[0] << centerColors[1] << centerColors[2] << "}, ";
    std::cout << "Corner={" << cornerColors[0] << cornerColors[1] << cornerColors[2] << "} = ";
    std::cout << (matches ? "CORRECT" : "WRONG") << "\n";

    return matches;
}

int Solver::countCorrectCorners() {
    int n = 0;
    if (cornerInCorrectLocation({ 1, 1,  1})) n++; // UFR
    if (cornerInCorrectLocation({ 1, 1, -1})) n++; // URB
    if (cornerInCorrectLocation({-1, 1, -1})) n++; // UBL
    if (cornerInCorrectLocation({-1, 1,  1})) n++; // ULF

    std::cout << "[PLL Corners] " << n << " corners in correct location\n";
    return n;
}

glm::ivec3 Solver::getFirstCorrectCorner() {
    if (cornerInCorrectLocation({ 1, 1,  1})) return { 1, 1,  1}; // UFR
    if (cornerInCorrectLocation({ 1, 1, -1})) return { 1, 1, -1}; // URB
    if (cornerInCorrectLocation({-1, 1, -1})) return {-1, 1, -1}; // UBL
    if (cornerInCorrectLocation({-1, 1,  1})) return {-1, 1,  1}; // ULF
    return {0, 0, 0}; // None found
}

std::vector<std::string> Solver::solveLastLayerCorners() {

    int correct = countCorrectCorners();

    // All corners correct
    if (correct == 4) return {};

    // Zero correct → just run A-perm CW once
    if (correct == 0) {
        return {
            "R","F'","R","B'","B'","R'","F","R","B'","B'","R'","R'"
        };
    }

    // One correct → rotate U until correct corner is UFR
    glm::ivec3 good = getFirstCorrectCorner();
    if (good != glm::ivec3(1,1,1)) {
        return {"U"};  // re-evaluate next cycle
    }

    // Good corner now at UFR → detect direction
    Cubelet* c = cube->getCubelet({1,1,1});
    bool clockwise = !sameColor(c->getFaceColor(RIGHT), centerColor(RIGHT));

    if (clockwise) {
        // A-perm CW
        return {
            "R","F'","R","B'","B'","R'","F","R","B'","B'","R'","R'"
        };
    } else {
        // A-perm CCW
        return {
            "R'","R'","B'","B'","R'","F'","R","B'","B'","R'","F"
        };
    }
}

    bool Solver::cornerMatchesCenters(const glm::ivec3 &pos){
    // Corner positions on top layer (Y=1)
    static const glm::ivec3 cornerPos[4] = {
        { 1, 1,  1}, // UFR (Front-Right)
        { 1, 1, -1}, // URB (Right-Back)
        {-1, 1, -1}, // UBL (Back-Left)
        {-1, 1,  1}  // ULF (Left-Front)
    };

    // Center positions
    static const glm::ivec3 centerPos[4] = {
        { 0, 0,  1}, // FRONT center
        { 1, 0,  0}, // RIGHT center
        { 0, 0, -1}, // BACK center
        {-1, 0,  0}  // LEFT center
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

bool Solver::areCornersSolved() {
    // FRU corner
    const Cubelet* fru = cube->getCubelet({1,1,1});
    const Cubelet* flu = cube->getCubelet({-1,1,1});
    const Cubelet* blu = cube->getCubelet({-1,1,-1});
    const Cubelet* bru = cube->getCubelet({1,1,-1});

    if (!fru || !flu || !blu || !bru) return false;

    // Only need to check UP color — PLL assumes OLL is complete
    bool a = sameColor(fru->getFaceColor(UP), cube->YELLOW);
    bool b = sameColor(flu->getFaceColor(UP), cube->YELLOW);
    bool c = sameColor(blu->getFaceColor(UP), cube->YELLOW);
    bool d = sameColor(bru->getFaceColor(UP), cube->YELLOW);

    return a && b && c && d;
}



std::vector<std::string> Solver::finalAUF() {

    // Try up to 4 orientations
    for (int i = 0; i < 4; i++) {

        // Check edges
        bool edgesGood =
            edgeMatchesCenter(0) &&
            edgeMatchesCenter(1) &&
            edgeMatchesCenter(2) &&
            edgeMatchesCenter(3);

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

    glm::ivec3 pos = cornerPos[idx];
    const Cubelet* c = cube->getCubelet(pos);
    if (!c) return false;

    // Determine which two side faces this corner has
    std::vector<Face> faces;

    if (pos.x == 1) faces.push_back(RIGHT);
    else            faces.push_back(LEFT);

    if (pos.z == 1) faces.push_back(FRONT);
    else            faces.push_back(BACK);

    int matches = 0;

    for (Face f : faces) {
        color cornerSticker = c->getFaceColor(f);
        color centerSticker = centerColor(f);

        if (sameColor(cornerSticker, centerSticker))
            matches++;
    }

    return (matches == 2);
}