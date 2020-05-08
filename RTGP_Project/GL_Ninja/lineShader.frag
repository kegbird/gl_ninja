/*
12_lambert.frag: Fragment shader for the Lambert illumination model

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 330 core

out vec4 colorFrag;

uniform vec3 color;

void main(){
        colorFrag  = vec4(1.0, 1.0, 1.0, 1.0);
}
