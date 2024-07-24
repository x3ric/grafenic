#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform int perspective;
uniform vec2 iMouse;
uniform vec2 iResolution;
uniform float iTime;
uniform vec3 rotation;
uniform vec3 position;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;

void main() {
    texCoord = aTexCoords;
    if(perspective == 1) {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    } else {
        // Convert from screen coordinates to normalized device coordinates
            vec2 pivot = vec2((iResolution.x / 2.0) + position.x, (iResolution.y / 2.0) + position.y);
            vec2 relativePos = aPos.xy - pivot;
        // rotationMatrix
            mat2 rotationMatrix = mat2(
                cos(rotation.z), -sin(rotation.z),
                sin(rotation.z), cos(rotation.z)
            );
        // Apply rotation
            vec2 finalPos = rotationMatrix * relativePos + pivot;
        // Apply the view matrix
            gl_Position = projection * vec4(finalPos, 0.0, 1.0);
    }
}