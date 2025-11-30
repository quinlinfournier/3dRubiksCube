#include "cubelet.h"

Cubelet::Cubelet(Shader& shader, glm::ivec3 gridPos, glm::vec3 scale, std::vector<color> colors)
    : shader(shader), gridPos(gridPos), scale(scale), face_colors(colors) {

    worldPos = glm::vec3(gridPos);
    updateModelMatrix();
    this->initVector();
    this->initVAO();
    // this->initVBO();
    // this->initEBO();
}

Cubelet::~Cubelet() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Cubelet::initVector() {
    vertices.clear();
    indices.clear();

    // Helper lambda to push a vertex (x,y,z,r,g,b)
    auto pushVertex = [&](float x, float y, float z, const color& c) {
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(c.red);
        vertices.push_back(c.green);
        vertices.push_back(c.blue);
    };

    // Define the 6 faces: FRONT, BACK, RIGHT, LEFT, UP, DOWN
    const glm::vec3 v[8] = {
        { 0.5f,  0.5f,  0.5f}, // 0: top-right-front
        {-0.5f,  0.5f,  0.5f}, // 1: top-left-front
        { 0.5f, -0.5f,  0.5f}, // 2: bottom-right-front
        {-0.5f, -0.5f,  0.5f}, // 3: bottom-left-front
        { 0.5f,  0.5f, -0.5f}, // 4: top-right-back
        {-0.5f,  0.5f, -0.5f}, // 5: top-left-back
        { 0.5f, -0.5f, -0.5f}, // 6: bottom-right-back
        {-0.5f, -0.5f, -0.5f}  // 7: bottom-left-back
    };

    // Each face: 4 vertices, CCW winding from outside
    struct Face { int a,b,c,d; int colorIndex; };
    Face faces[6] = {
        {0,1,2,3, FRONT},  // Front
        {4,6,5,7, BACK},   // Back
        {0,2,4,6, RIGHT},  // Right
        {1,5,3,7, LEFT},   // Left
        {0,4,1,5, UP},     // Top
        {2,3,6,7, DOWN}    // Bottom
    };

    // Build vertices and indices
    for (int f = 0; f < 6; ++f) {
        int base = vertices.size() / 6; // 6 floats per vertex
        pushVertex(v[faces[f].a].x, v[faces[f].a].y, v[faces[f].a].z, face_colors[faces[f].colorIndex]);
        pushVertex(v[faces[f].b].x, v[faces[f].b].y, v[faces[f].b].z, face_colors[faces[f].colorIndex]);
        pushVertex(v[faces[f].c].x, v[faces[f].c].y, v[faces[f].c].z, face_colors[faces[f].colorIndex]);
        pushVertex(v[faces[f].d].x, v[faces[f].d].y, v[faces[f].d].z, face_colors[faces[f].colorIndex]);

        // Two triangles per face (CCW)
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 1);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }
}

void Cubelet::initVAO() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Cubelet::initVBO() {
    glBindVertexArray(this->VAO);

    glGenBuffers(1, &this->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Position attribute (layout location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (layout location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Cubelet::initEBO() {
    glGenBuffers(1, &this->EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);
}

void Cubelet::draw(const glm::mat4& view, const glm::mat4& projection) const {
    this->shader.use();

    // Set uniforms
    this->shader.setMatrix4("model", this->modelMatrix);
    this->shader.setMatrix4("view", view);
    this->shader.setMatrix4("projection", projection);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); // Use elements instead of arrays
    glBindVertexArray(0);
}



void Cubelet::rotateLocal(const glm::mat4& rotationMatrix) {
    modelMatrix = rotationMatrix * modelMatrix;
}

void Cubelet::fixFloatError() {
    glm::mat4 rotationScale = modelMatrix;
    rotationScale[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    modelMatrix = glm::translate(glm::mat4(1.0f), worldPos) * rotationScale;
}

void Cubelet::updateModelMatrix() {
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, worldPos);
    modelMatrix = glm::scale(modelMatrix, scale);
}

void Cubelet::setGridPosition(glm::ivec3 newGridPos) {
    gridPos = newGridPos;
    worldPos = glm::vec3(gridPos);
    updateModelMatrix();
}


void Cubelet::setWorldPosition(glm::vec3 newWorldPos) {
    worldPos = newWorldPos;

    glm::mat3 rotationScale = glm::mat3(modelMatrix);
    modelMatrix = glm::mat4(rotationScale);
    modelMatrix[3] = glm::vec4(worldPos, 1.0f);
}

void Cubelet::rotateAroundY(bool clockwise) {
    // rotation around cube Y axis (U / D moves affect pieces' local colors)
    // Cycle: FRONT -> RIGHT -> BACK -> LEFT -> FRONT   (for clockwise Y)
    color old[6];
    for (int i=0;i<6;++i) old[i] = face_colors[i];

    if (clockwise) {
        face_colors[FRONT] = old[LEFT];
        face_colors[RIGHT] = old[FRONT];
        face_colors[BACK]  = old[RIGHT];
        face_colors[LEFT]  = old[BACK];
        face_colors[UP]    = old[UP];
        face_colors[DOWN]  = old[DOWN];
    } else {
        face_colors[FRONT] = old[RIGHT];
        face_colors[LEFT]  = old[FRONT];
        face_colors[BACK]  = old[LEFT];
        face_colors[RIGHT] = old[BACK];
        face_colors[UP]    = old[UP];
        face_colors[DOWN]  = old[DOWN];
    }
}

void Cubelet::rotateAroundX(bool clockwise) {
    // rotation around X axis (R / L)
    // Cycle: UP -> BACK -> DOWN -> FRONT -> UP   (clockwise looking +X -> -X)
    color old[6];
    for (int i=0;i<6;++i) old[i] = face_colors[i];

    if (clockwise) {
        // new[FRONT] = old[UP], new[DOWN] = old[FRONT], new[BACK] = old[DOWN], new[UP] = old[BACK]
        face_colors[FRONT] = old[UP];
        face_colors[DOWN]  = old[FRONT];
        face_colors[BACK]  = old[DOWN];
        face_colors[UP]    = old[BACK];
        // LEFT and RIGHT unchanged
        face_colors[LEFT]  = old[LEFT];
        face_colors[RIGHT] = old[RIGHT];
    } else {
        // inverse cycle: FRONT -> UP -> BACK -> DOWN -> FRONT
        face_colors[FRONT] = old[DOWN];
        face_colors[UP]    = old[FRONT];
        face_colors[BACK]  = old[UP];
        face_colors[DOWN]  = old[BACK];
        face_colors[LEFT]  = old[LEFT];
        face_colors[RIGHT] = old[RIGHT];
    }
}

void Cubelet::rotateAroundZ(bool clockwise) {
    // rotation around Z axis (F / B)
    // Cycle: UP -> RIGHT -> DOWN -> LEFT -> UP   (clockwise looking +Z -> -Z)
    color old[6];
    for (int i=0;i<6;++i) old[i] = face_colors[i];

    if (clockwise) {
        face_colors[RIGHT] = old[DOWN];
        face_colors[UP]  = old[RIGHT];
        face_colors[LEFT]  = old[UP];
        face_colors[DOWN]    = old[LEFT];
        face_colors[FRONT] = old[FRONT];
        face_colors[BACK]  = old[BACK];
    } else {
        face_colors[RIGHT] = old[UP];
        face_colors[DOWN]    = old[RIGHT];
        face_colors[LEFT]  = old[DOWN];
        face_colors[UP]  = old[LEFT];
        face_colors[FRONT] = old[FRONT];
        face_colors[BACK]  = old[BACK];
    }
}


void Cubelet::debugColors() const {
    std::cout << "Cubelet at grid ("
              << gridPos.x << ", "
              << gridPos.y << ", "
              << gridPos.z << ")\n";

    static const char* faceNames[6] = {
        "FRONT", "BACK", "RIGHT", "LEFT", "UP", "DOWN"
    };

    for (int i = 0; i < face_colors.size(); i++) {
        std::cout << "  " << faceNames[i]
                  << ": " << colorToName(face_colors[i])
                  << std::endl;
    }
}


void Cubelet::updateVertexColors()
{
    // Rebuild interleaved vertex data (x,y,z,r,g,b) for 24 vertices (4 per face).
    std::vector<float> updatedVertices;
    updatedVertices.reserve(24 * 6); // 24 vertices * 6 floats each

    // Helper to push 4 corners for a face in the same order as initVector()
    auto pushFace4 = [&](int faceIndex,
                         const glm::vec3& v0, const glm::vec3& v1,
                         const glm::vec3& v2, const glm::vec3& v3)
    {
        const color &c = face_colors[faceIndex];
        auto pushVertex = [&](const glm::vec3 &p) {
            updatedVertices.push_back(p.x);
            updatedVertices.push_back(p.y);
            updatedVertices.push_back(p.z);
            updatedVertices.push_back(c.red);
            updatedVertices.push_back(c.green);
            updatedVertices.push_back(c.blue);
        };

        // same order used in initVector()
        pushVertex(v0);
        pushVertex(v1);
        pushVertex(v2);
        pushVertex(v3);
    };

    // --- FRONT FACE (Z=+0.5) --- face_colors[0]
    pushFace4(FRONT,
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f)
    );

    // --- BACK FACE (Z=-0.5) --- face_colors[1]
    pushFace4(BACK,
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f)
    );

    // --- RIGHT FACE (X=+0.5) --- face_colors[2] in initVector() you used face_colors[2] for RIGHT
    pushFace4(RIGHT,
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f)
    );

    // --- LEFT FACE (X=-0.5) --- face_colors[3]
    pushFace4(LEFT,
        glm::vec3(-0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f)
    );

    // --- TOP FACE (Y=+0.5) --- face_colors[4]
    pushFace4(UP,
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f)
    );

    // --- BOTTOM FACE (Y=-0.5) --- face_colors[5]
    pushFace4(DOWN,
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f)
    );

    // Replace CPU-side interleaved vertex buffer
    this->vertices.swap(updatedVertices);

    // Rebuild indices to match 4-vertex-per-face layout (two triangles per face).
    this->indices.clear();
    this->indices.reserve(36);
    for (unsigned int f = 0; f < 6; ++f) {
        unsigned int base = f * 4u;
        // Note: these follow the same pattern you used in initVector()
        this->indices.push_back(base + 0);
        this->indices.push_back(base + 1);
        this->indices.push_back(base + 2);

        this->indices.push_back(base + 1);
        this->indices.push_back(base + 3);
        this->indices.push_back(base + 2);
    }

    // Upload updated VBO and EBO. Bind VAO so the element array buffer binding is stored in it.
    glBindVertexArray(this->VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(float),
                 this->vertices.data(), GL_DYNAMIC_DRAW);

    // EBO (element array buffer)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int),
                 this->indices.data(), GL_STATIC_DRAW);

    // Unbind VAO for safety
    glBindVertexArray(0);
}

color Cubelet::getFaceColor(Face face) const {
    if  (face >= 0 && face < face_colors.size()) {
        return face_colors[face];
    }
    else
        return color(0.0f, 0.0f, 0.0f);
}


std::string Cubelet::colorToName(const color& c) const {
    // Exact matches used in your RubiksCube.h
    if (c.red == 1.0f && c.green == 1.0f && c.blue == 1.0f) return "WHITE";
    if (c.red == 1.0f && c.green == 1.0f && c.blue == 0.0f) return "YELLOW";
    if (c.red == 0.0f && c.green == 0.0f && c.blue == 1.0f) return "BLUE";
    if (c.red == 0.0f && c.green == 0.5f && c.blue == 0.0f) return "GREEN";
    if (c.red == 1.0f && c.green == 0.0f && c.blue == 0.0f) return "RED";
    if (c.red == 1.0f && c.green == 0.5f && c.blue == 0.0f) return "ORANGE";
    if (c.red == 0.0f && c.green == 0.0f && c.blue == 0.0f) return "BLACK";

    // otherwise print RGB
    char buf[64];
    snprintf(buf, sizeof(buf), "(%.2f, %.2f, %.2f)", c.red, c.green, c.blue);
    return std::string(buf);
}



