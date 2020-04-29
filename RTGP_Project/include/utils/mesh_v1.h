/*
Mesh class - v1
- allocation and initialization of VBO, VAO, and EBO buffers, and sets as OpenGL must consider the data in the buffers

VBO : Vertex Buffer Object - memory allocated on GPU memory to store the mesh data (vertices and their attributes, like e.g. normals, etc)
EBO : Element Buffer Object - a buffer maintaining the indices of vertices composing the mesh faces
VAO : Vertex Array Object - a buffer that helps to "manage" VBO and its inner structure. It stores pointers to the different vertex attributes stored in the VBO. When we need to render an object, we can just bind the corresponding VAO, and all the needed calls to set up the binding between vertex attributes and memory positions in the VBO are automatically configured.
See https://learnopengl.com/#!Getting-started/Hello-Triangle for details.

N.B. 1) no texturing in this version of the class

N.B. 2) adaptation of https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/mesh.h

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

#pragma once

using namespace std;

// Std. Includes
#include <vector>

// GL Includes
#include <glad/glad.h> // Contains all the necessery OpenGL includes
// we use GLM data structures to write data in the VBO, VAO and EBO buffers
#include <glm/glm.hpp>

// data structure for vertices
struct Vertex {
    // vertex coordinates
    glm::vec3 Position;
    // Normal
    glm::vec3 Normal;
    // Texture coordinates
    glm::vec2 TexCoords;
    // Tangent
    glm::vec3 Tangent;
    // Bitangent
    glm::vec3 Bitangent;
};

/////////////////// MESH class ///////////////////////
class Mesh {
public:
    // data structures for vertices, and indices of vertices (for faces)
    vector<Vertex> vertices;
    vector<GLuint> indices;
    // VAO
    GLuint VAO;

    //////////////////////////////////////////
    // Constructor
    Mesh(vector<Vertex> vertices, vector<GLuint> indices)
    {
        this->vertices = vertices;
        this->indices = indices;

        // initialization of OpenGL buffers
        this->setupMesh();
    }

    //////////////////////////////////////////

    // rendering of mesh
    void Draw()
    {
        // VAO is made "active"
        glBindVertexArray(this->VAO);
        // rendering of data in the VAO
        glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
        // VAO is "detached"
        glBindVertexArray(0);
    }

    //////////////////////////////////////////
    // buffers are deallocated when application ends
    void Delete()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
    // VBO and EBO
    GLuint VBO, EBO;

    //////////////////////////////////////////
    // buffer objects\arrays are initialized
    // a brief description of their role and how they are binded can be found at:
    // https://learnopengl.com/#!Getting-started/Hello-Triangle
    // (in different parts of the page), or here:
    // http://www.informit.com/articles/article.aspx?p=1377833&seqNum=8
    void setupMesh()
    {
        // we create the buffers
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);
        glGenBuffers(1, &this->EBO);

        // VAO is made "active"
        glBindVertexArray(this->VAO);
        // we copy data in the VBO - we must set the data dimension, and the pointer to the structure cointaining the data
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);
        // we copy data in the EBO - we must set the data dimension, and the pointer to the structure cointaining the data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

        // we set in the VAO the pointers to the different vertex attributes (with the relative offsets inside the data structure)
        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        // Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
        // Texture Coordinates
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));
        // Tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Tangent));
        // Bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
    }
};
