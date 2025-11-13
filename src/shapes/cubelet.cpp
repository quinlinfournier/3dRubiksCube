//
// Created by quinf on 11/11/2025.
//

#include "cubelet.h"

// --- Constructors & Destructors ---
Cubelet::Cubelet(Shader& shader, glm::vec3 pos, glm::vec3 scale, std::vector<color> colors)
    : shader(shader), pos(pos), scale(scale), face_colors(colors) {

    assert(face_colors.size() == 6 );

    // Initialize the model matrix: Identity -> Translate to Grid Pos -> Scale to Cubelet Size
    this->modelMatrix = glm::mat4(1.0f);
    this->modelMatrix = glm::translate(this->modelMatrix, pos);
    this->modelMatrix = glm::scale(this->modelMatrix, scale);

    this->initVector();
    this->initVAO();
    this->initVBO();
    this->initEBO();
}
Cubelet::~Cubelet() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

// --- Init Geometry ---
void Cubelet::initVector() {
    // Define 24 vertices (4 per face) so each face can have a solid, uniform color.
    // Face Colors: 0:Front, 1:Back, 2:Right, 3:Left, 4:Top, 5:Bottom

    // Vertices: (X, Y, Z, R, G, B) - 6 floats per vertex

    // --- FRONT FACE (Z=0.5) --- Color: face_colors[0] (Indices 0-3)
    this->vertices.insert(this->vertices.end(), {
        0.5f,  0.5f,  0.5f, face_colors[0].red, face_colors[0].green, face_colors[0].blue,
       -0.5f,  0.5f,  0.5f, face_colors[0].red, face_colors[0].green, face_colors[0].blue,
        0.5f, -0.5f,  0.5f, face_colors[0].red, face_colors[0].green, face_colors[0].blue,
       -0.5f, -0.5f,  0.5f, face_colors[0].red, face_colors[0].green, face_colors[0].blue,
    });

    // --- BACK FACE (Z=-0.5) --- Color: face_colors[1] (Indices 4-7)
    this->vertices.insert(this->vertices.end(), {
        0.5f,  0.5f, -0.5f, face_colors[1].red, face_colors[1].green, face_colors[1].blue,
       -0.5f,  0.5f, -0.5f, face_colors[1].red, face_colors[1].green, face_colors[1].blue,
        0.5f, -0.5f, -0.5f, face_colors[1].red, face_colors[1].green, face_colors[1].blue,
       -0.5f, -0.5f, -0.5f, face_colors[1].red, face_colors[1].green, face_colors[1].blue,
    });

    // --- RIGHT FACE (X=0.5) --- Color: face_colors[2] (Indices 8-11)
    this->vertices.insert(this->vertices.end(), {
        0.5f,  0.5f,  0.5f, face_colors[2].red, face_colors[2].green, face_colors[2].blue,
        0.5f,  0.5f, -0.5f, face_colors[2].red, face_colors[2].green, face_colors[2].blue,
        0.5f, -0.5f,  0.5f, face_colors[2].red, face_colors[2].green, face_colors[2].blue,
        0.5f, -0.5f, -0.5f, face_colors[2].red, face_colors[2].green, face_colors[2].blue,
    });

    // --- LEFT FACE (X=-0.5) --- Color: face_colors[3] (Indices 12-15)
    this->vertices.insert(this->vertices.end(), {
       -0.5f,  0.5f,  0.5f, face_colors[3].red, face_colors[3].green, face_colors[3].blue,
       -0.5f,  0.5f, -0.5f, face_colors[3].red, face_colors[3].green, face_colors[3].blue,
       -0.5f, -0.5f,  0.5f, face_colors[3].red, face_colors[3].green, face_colors[3].blue,
       -0.5f, -0.5f, -0.5f, face_colors[3].red, face_colors[3].green, face_colors[3].blue,
    });

    // --- TOP FACE (Y=0.5) --- Color: face_colors[4] (Indices 16-19)
    this->vertices.insert(this->vertices.end(), {
        0.5f,  0.5f,  0.5f, face_colors[4].red, face_colors[4].green, face_colors[4].blue,
       -0.5f,  0.5f,  0.5f, face_colors[4].red, face_colors[4].green, face_colors[4].blue,
        0.5f,  0.5f, -0.5f, face_colors[4].red, face_colors[4].green, face_colors[4].blue,
       -0.5f,  0.5f, -0.5f, face_colors[4].red, face_colors[4].green, face_colors[4].blue,
    });

    // --- BOTTOM FACE (Y=-0.5) --- Color: face_colors[5] (Indices 20-23)
    this->vertices.insert(this->vertices.end(), {
        0.5f, -0.5f,  0.5f, face_colors[5].red, face_colors[5].green, face_colors[5].blue,
       -0.5f, -0.5f,  0.5f, face_colors[5].red, face_colors[5].green, face_colors[5].blue,
        0.5f, -0.5f, -0.5f, face_colors[5].red, face_colors[5].green, face_colors[5].blue,
       -0.5f, -0.5f, -0.5f, face_colors[5].red, face_colors[5].green, face_colors[5].blue,
    });

    // Indices for 6 faces (6 indices per face = 36 total)
    this->indices.insert(this->indices.end(), {
        // Front (0-3)
        0, 1, 2,    1, 3, 2,
        // Back (4-7)
        4, 6, 5,    5, 6, 7,
        // Right (8-11)
        8, 9, 10,   9, 11, 10,
        // Left (12-15)
        12, 14, 13, 13, 14, 15,
        // Top (16-19)
        16, 18, 17, 18, 19, 17,
        // Bottom (20-23)
        20, 21, 22, 21, 23, 22
    });
    }
    void Cubelet::initVAO() {
        glGenVertexArrays(1, &this->VAO);
        glBindVertexArray(this->VAO);
    }

    void Cubelet::initVBO() {
        glGenBuffers(1, &this->VBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Position attribute (layout location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute (layout location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void Cubelet::initEBO() {
        glGenBuffers(1, &this->EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    // --- Drawing and Transformation ---

    void Cubelet::setUniforms(const glm::mat4 &view, const glm::mat4 &projection) const {
        // Pass the Cubie's current transformation matrix to the shader
        this->shader.setMatrix4("model", this->modelMatrix);
        this->shader.setMatrix4("view", view);
        this->shader.setMatrix4("projection", projection);
    }

    void Cubelet::draw(const glm::mat4& view, const glm::mat4& projection) const {
        this->shader.use();
        this->setUniforms(view, projection); // Set the unique model matrix for this piece

        glBindVertexArray(this->VAO);
        glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // --- Rubik's Cube Logic Helpers ---

    void Cubelet::rotateLocal(const glm::mat4& rotationMatrix) {
        // Simple: just apply the rotation to the model matrix for orientation
        modelMatrix = rotationMatrix * modelMatrix;
    }


    void Cubelet::setPosition(glm::vec3 newPos) {
        // 1. Update the internal position tracker
        this->pos = newPos;

        glm::mat4 rotationScale = modelMatrix; // Save the rotation and scale matrix for later
        rotationScale[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Remove translation

        modelMatrix = glm::translate(glm::mat4(1.0f), newPos); // Re-apply translation

    }

    glm::vec3 Cubelet::getPosition() const {
        return pos;
    }
