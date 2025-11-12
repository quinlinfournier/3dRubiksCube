//
// Created by quinf on 11/11/2025.
//

#ifndef M4OEP_QJFOURNI_CUBELET_H
#define M4OEP_QJFOURNI_CUBELET_H

#include "../shader/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cassert>

struct color {
    float red, green, blue;
    // Default constructor
    color(float r = 0.0f, float g = 0.0f, float b = 0.0f) : red(r), green(g), blue(b) {}
};

class Cubelet {
    private:
        unsigned int VAO, VBO, EBO;
        Shader& shader;

        // Position cublet in the 3x3x3 grid
        glm::vec3 pos;
        glm::vec3 scale;

        // 6 Colors
        std::vector<color> face_colors;

        // Store the current transformation matrix
        glm::mat4 modelMatrix;


        std::vector<float> vertices;
        std::vector<unsigned int> indices;



        void initVector();
        // Vortex Array Object
        void initVAO();
        // Vortex Buffer Object
        void initVBO();
        // Element Buffer Object
        void initEBO();

    public:
        // Constructor requires 6 colors (one for each face)
        Cubelet(Shader& shader, glm::vec3 pos, glm::vec3 scale, std::vector<color> colors);
        ~Cubelet();

        void setUniforms(const glm::mat4 &view, const glm::mat4 &projection) const;
        void draw(const glm::mat4& view, const glm::mat4& projection) const;

    // --- Methods for Rubik's Cube Logic ---

        // Applies an ongoing rotation to the Cubie's model matrix during animation.
        void rotateLocal(const glm::mat4& rotationMatrix);

        // Updates the piece's grid position after a rotation is complete.
        void setPosition(glm::vec3 newPos);

        // Getters
        glm::vec3 getPosition() const { return pos; }
};


#endif //M4OEP_QJFOURNI_CUBELET_H