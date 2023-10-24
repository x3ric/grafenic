#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform vec2 iMouse;
uniform vec2 iResolution;
uniform float iTime;
uniform mat4 viewMatrix;
uniform float z;

out vec2 texCoord;

void main() {
    texCoord = aTexCoords;
    float effectiveZoom = z == 0.0 ? 1.0 : z;
    vec4 pos = viewMatrix * vec4(aPos, 0, 1.0);
    gl_Position = vec4(pos.x * effectiveZoom, pos.y * effectiveZoom, pos.z * effectiveZoom, pos.w);
}
