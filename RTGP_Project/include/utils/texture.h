#pragma once
#include <assimp/scene.h>

struct Texture {
    GLuint id;
    string type;
    aiString path;
};
