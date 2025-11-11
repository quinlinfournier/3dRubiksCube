#ifndef GRAPHICS_CUBE_H
#define GRAPHICS_CUBE_H

#include "../shader/shader.h"
#include "../util/color.h"

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using std::vector, glm::vec3, glm::mat4;


class Cube {
public:
    Cube(Shader& shader, vec3 pos, vec3 size, vector<color> colors);
    ~Cube();
    void draw(const mat4& model, const mat4& view, const mat4& projection) const;
    void setUniforms(const mat4& model, const mat4& view, const mat4& projection) const;

    void rotateX(float delta);
    void rotateY(float delta);
    void rotateZ(float delta);

    void changeSize(float delta);

private:
    Shader shader;
    vec3 pos;
    vec3 size;
    vector<color> colors;
    float degreeX;
    float degreeY;
    float degreeZ;

    // Vectors
    vector<float> vertices;
    vector<unsigned int> indices;

    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    void initVectors();
    void initVAO();
    void initVBO();
    void initEBO();
};


#endif //GRAPHICS_CUBE_H
