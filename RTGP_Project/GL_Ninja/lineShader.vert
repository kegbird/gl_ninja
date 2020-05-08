/*
12_lambert.vert: Vertex shader for the Lambert illumination model

N. B.) the shader treats a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 330 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;

void main()
{
        gl_Position = vec4(position.x, position.y, position.z, 1.0);
}
