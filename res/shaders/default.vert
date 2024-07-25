#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform vec2 iMouse;
uniform vec2 iResolution;
uniform float iTime;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;

void main() {
    texCoord = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}