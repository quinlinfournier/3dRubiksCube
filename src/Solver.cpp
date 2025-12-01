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
    currentF2LSlot = 0;

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
        std::cout << "F2L Step - Slot " << currentF2LSlot << std::endl;

        // TODO: Implement F2L logic here
        // For now, just mark as complete
        std::cout << "F2L not yet implemented. Marking as complete." << std::endl;
        currentState = WCCOMPLETE;
        return "";
    }

    // ====== OLL (Step 2) ======
    if (currentStep == 2) {
        std::cout << "OLL Step" << std::endl;
        // TODO: Implement OLL logic
        currentState = WCCOMPLETE;
        return "";
    }

    // ====== PLL (Step 3) ======
    if (currentStep == 3) {
        std::cout << "PLL Step" << std::endl;
        // TODO: Implement PLL logic
        currentState = WCCOMPLETE;
        return "";
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

std::vector<Solver::F2LPair> Solver::getF2LPairs() const {
    std::vector<F2LPair> pairs;

    // The 4 slots with colors
    // Slot 1:W-B-R
    pairs.push_back({-1,-1, 'B', 'R',
        glm::ivec3(1,-1,1), glm::ivec3(1,0,1) });
    // Slot 2:W-R-G
    pairs.push_back({-1,-1, 'R', 'G',
        glm::ivec3(1,-1,-1), glm::ivec3(1,0,-1) });
    // Slot 3:W-G-O
    pairs.push_back({-1,-1, 'G', 'O',
        glm::ivec3(-1,-1,-1), glm::ivec3(1,0,-1) });
    // Slot 4:W-O-B
    pairs.push_back({-1,-1, 'O', 'B',
        glm::ivec3(-1,-1,-1), glm::ivec3(-1,0,1) });

    return pairs;
}

int Solver::findF2LCorner(char color1, char color2) const {
    const auto& solved = cube->getSolvedPosition();

    for (int i = 0; i < 26; i++) {
        glm::ivec3 pos = solved[i];

        // Corners have all three coordinated as 1
        if (std::abs(pos.x) == 1 && std::abs(pos.y) == 1 && std::abs(pos.z) == 1) {
            if (pos.y == -1) {
                // Check if this corner has the right colors
                if ((pos.x == 1 && pos.z == 1 && color1 == 'B' && color2 == 'R') ||
                    (pos.x == 1 && pos.z == -1 && color1 == 'R' && color2 == 'G') ||
                    (pos.x == -1 && pos.z == -1 && color1 == 'G' && color2 == 'O') ||
                    (pos.x == -1 && pos.z == 1 && color1 == 'O' && color2 == 'B')) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int Solver::findF2LEdge(char color1, char color2) const {
    const auto& solved = cube->getSolvedPosition();

    for (int i = 0; i < 26; i++) {
        glm::ivec3 pos = solved[i];

        // Middle layer edges have y=0 and one other coordinate = 0
        int zeroCount = (pos.x == 0 ? 1 : 0) + (pos.y == 0 ? 1 : 0) + (pos.z == 0 ? 1 : 0);
        if (zeroCount == 1 && pos.y == 0) {
            // Check if this edge has the right colors
            if ((pos.x == 1 && pos.z == 1 && color1 == 'B' && color2 == 'R') ||
                (pos.x == 1 && pos.z == -1 && color1 == 'R' && color2 == 'G') ||
                (pos.x == -1 && pos.z == -1 && color1 == 'G' && color2 == 'O') ||
                (pos.x == -1 && pos.z == 1 && color1 == 'O' && color2 == 'B')) {
                return i;
                }
        }
    }
    return -1;
}

bool Solver::isCornerOrientedCorrectly(Cubelet* piece) const {
    if (!piece) return false;
    return getFaceColor(piece, DOWN) == 'W';
}

bool Solver::isF2LPairSolved(const F2LPair& pair) const {
    const auto& current = cube->getCurrentPosition();

    if (pair.cornerID == -1 || pair.edgeID == -1) return false;

    // Check if both pieces are in correct positions
    if (current[pair.cornerID] != pair.targetCornerPos) return false;
    if (current[pair.edgeID] != pair.targetEdgePos) return false;

    // Check orientation
    Cubelet* corner = cube->getCubelet(current[pair.cornerID]);
    Cubelet* edge = cube->getCubelet(current[pair.edgeID]);

    if (!corner || !edge) return false;

    // Corner should have white facing down
    if (getFaceColor(corner, DOWN) != 'W') return false;

    // Edge and corner colors should match on their adjacent faces
    return true;
}

Face Solver::getColorFace(Cubelet* piece, char targetColor) const {
    if (!piece) return FRONT;

    if (getFaceColor(piece, FRONT) == targetColor) return FRONT;
    if (getFaceColor(piece, BACK) == targetColor) return BACK;
    if (getFaceColor(piece, RIGHT) == targetColor) return RIGHT;
    if (getFaceColor(piece, LEFT) == targetColor) return LEFT;
    if (getFaceColor(piece, UP) == targetColor) return UP;
    if (getFaceColor(piece, DOWN) == targetColor) return DOWN;

    return FRONT;
}

std::vector<std::string> Solver::extractCornerFromSlot(glm::ivec3 pos) const {
    // Extract corner based on which slot it's in
    if (pos.x == 1 && pos.z == 1) {
        return {"R", "U", "R'"};  // Front-right slot
    } else if (pos.x == 1 && pos.z == -1) {
        return {"B", "U", "B'"};  // Back-right slot
    } else if (pos.x == -1 && pos.z == -1) {
        return {"L", "U", "L'"};  // Back-left slot
    } else if (pos.x == -1 && pos.z == 1) {
        return {"F", "U", "F'"};  // Front-left slot
    }
    return {"U"};
}

std::vector<std::string> Solver::extractEdgeFromMiddle(glm::ivec3 pos) const {
    // Extract edge from middle layer to top
    if (pos.x == 1 && pos.z == 1) {
        return {"R", "U", "R'", "U'", "F'", "U'", "F"};
    } else if (pos.x == 1 && pos.z == -1) {
        return {"B", "U", "B'", "U'", "R'", "U'", "R"};
    } else if (pos.x == -1 && pos.z == -1) {
        return {"L", "U", "L'", "U'", "B'", "U'", "B"};
    } else if (pos.x == -1 && pos.z == 1) {
        return {"F", "U", "F'", "U'", "L'", "U'", "L"};
    }
    return {"U"};
}

std::vector<std::string> Solver::solveF2LPair(const F2LPair& pair) const {
    const auto& current = cube->getCurrentPosition();

    if (pair.cornerID == -1 || pair.edgeID == -1) {
        std::cout << "ERROR: Invalid F2L pair IDs" << std::endl;
        return {};
    }

    glm::ivec3 cornerPos = current[pair.cornerID];
    glm::ivec3 edgePos = current[pair.edgeID];

    Cubelet* corner = cube->getCubelet(cornerPos);
    Cubelet* edge = cube->getCubelet(edgePos);

    if (!corner || !edge) {
        std::cout << "ERROR: Could not get F2L pieces" << std::endl;
        return {};
    }

    // CASE 1: Corner in slot but wrong - extract it
    if (cornerPos.y == -1 && cornerPos != pair.targetCornerPos) {
        std::cout << "Extracting corner from wrong slot" << std::endl;
        return extractCornerFromSlot(cornerPos);
    }

    // CASE 2: Edge in middle layer but wrong - extract it
    if (edgePos.y == 0 && edgePos != pair.targetEdgePos) {
        std::cout << "Extracting edge from middle layer" << std::endl;
        return extractEdgeFromMiddle(edgePos);
    }

    // CASE 3: Both in top layer - pair and insert
    if (cornerPos.y == 1 && edgePos.y == 1) {
        std::cout << "Both pieces in top layer, pairing..." << std::endl;
        return pairUpPieces(pair);
    }

    // CASE 4: Need to move pieces to top layer
    if (cornerPos.y != 1) {
        return extractCornerFromSlot(cornerPos);
    }
    if (edgePos.y != 1) {
        return extractEdgeFromMiddle(edgePos);
    }

    return {"U"};  // Default rotation
}

std::vector<std::string> Solver::pairUpPieces(const F2LPair& pair) const {
    // This is a simplified F2L pairing algorithm
    // In practice, there are 41 different F2L cases
    // Here's a basic algorithm that handles common cases

    const auto& current = cube->getCurrentPosition();
    glm::ivec3 cornerPos = current[pair.cornerID];
    glm::ivec3 edgePos = current[pair.edgeID];

    Cubelet* corner = cube->getCubelet(cornerPos);
    Cubelet* edge = cube->getCubelet(edgePos);

    // Basic F2L insertion (simplified - handles basic case)
    // When corner and edge are separated in top layer

    // Standard F2L algorithm for front-right slot (R U R')
    // You'll need to adapt this for each slot and case
    if (pair.color1 == 'B' && pair.color2 == 'R') {
        return {"U", "R", "U'", "R'"};
    } else if (pair.color1 == 'R' && pair.color2 == 'G') {
        return {"U", "B", "U'", "B'"};
    } else if (pair.color1 == 'G' && pair.color2 == 'O') {
        return {"U", "L", "U'", "L'"};
    } else if (pair.color1 == 'O' && pair.color2 == 'B') {
        return {"U", "F", "U'", "F'"};
    }

    return {"U"};
}

