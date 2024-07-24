#include "array.c"
#include "utils.c"
#include "var.c"
#include "camera.c"

void InitializeShader() {
    // Generate Shader default
        shaderdefault = LoadShader("./res/shaders/default.vert","./res/shaders/pixel.frag");
        GenArrays();
    // Generate texture for frame buffer
        glGenTextures(1,&framebufferTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexOpt(GL_NEAREST,GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
}
