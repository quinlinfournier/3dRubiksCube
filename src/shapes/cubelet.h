#ifndef M4OEP_QJFOURNI_CUBELET_H
#define M4OEP_QJFOURNI_CUBELET_H

// cubelet.h

// Define the alignment macro based on likely compiler (GCC/Clang/MinGW)
#if defined(__GNUC__) || defined(__clang__)
#define CUBELET_ALIGN __attribute__((aligned(16)))
#elif defined(_MSC_VER)
#define CUBELET_ALIGN __declspec(align(16))
#else
#define CUBELET_ALIGN
#endif

#include "../shader/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

struct color {
    float red, green, blue;
    color(float r = 0.0f, float g = 0.0f, float b = 0.0f) : red(r), green(g), blue(b) {}
};

enum Face { FRONT=0, BACK=1, RIGHT=2, LEFT=3, UP=4, DOWN=5 };

class CUBELET_ALIGN Cubelet {
private:
    unsigned int VAO, VBO, EBO;
    Shader& shader;

    glm::ivec3 gridPos;
    glm::vec3 worldPos;
    glm::vec3 scale;
    glm::mat4 modelMatrix;

    std::vector<color> face_colors;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    void initVector();
    void initVAO();
    void initVBO();
    void initEBO();

public:
    Cubelet(Shader& shader, glm::ivec3 gridPos, glm::vec3 scale, std::vector<color> colors);
    ~Cubelet();

    void draw(const glm::mat4& view, const glm::mat4& projection) const;
    void rotateLocal(const glm::mat4& rotationMatrix);
    void updateModelMatrix();
    void fixFloatError();


    // Getters & Setters
    glm::ivec3 getGridPosition() const { return gridPos; }
    void setGridPosition(glm::ivec3 newGridPos);
    glm::vec3 getWorldPosition() const { return worldPos; }
    void setWorldPosition(glm::vec3 newWorldPos);

    color getFaceColor(Face face) const;

    // Rotation
    void rotateAroundY(bool clockwise);
    void rotateAroundX(bool clockwise);
    void rotateAroundZ(bool clockwise);

    void debugColors() const; // Add this method
    void updateVertexColors(); // Update VBO with current face_colors

    std::string colorToName(const color& c) const;

};

#endif