/*
12_lambert.frag: Fragment shader for the Lambert illumination model

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 330 core

// output shader variable
out vec4 colorFrag;

// diffusive component (passed from the application)
uniform vec3 diffuseColor;
// weight of the diffusive component
uniform float Kd;

// vettore di incidenza della luce (calcolato nel vertes shader)
// light incidence direction (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDir;
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;

void main(){

  // normalization of the per-fragment normal
  vec3 N = normalize(vNormal);
  // normalization of the per-fragment light incidence direction
  vec3 L = normalize(lightDir.xyz);

  // Lambert coefficient
  float lambertian = max(dot(L,N), 0.0);

  // Lambert illumination model
  vec3 color = vec3(Kd * lambertian * diffuseColor);

  colorFrag  = vec4(color,1.0);

}
