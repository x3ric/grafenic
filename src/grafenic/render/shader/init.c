typedef struct {
    GLuint Program;
    const char* vertex;
    const char* fragment;
    time_t lastvertmodtime;
    time_t lastfragmodtime;
    bool hotreloading;
} Shader;

#define NUM_PBO 2

GLuint VAO;           // Vertex Array Object
GLuint VBO;           // Vertex Buffer Object
GLuint EBO;           // Index Buffer Object
GLuint PBO[NUM_PBO];  // Pixel Buffer Object

void CreateTextureWithPBO(unsigned char* framebuffer, GLuint framebufferTexture) {
    static int currentPboIndex = 0;
    if (framebuffer) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[currentPboIndex]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, window.screen_width * window.screen_height * 4, NULL, GL_STREAM_DRAW);
        GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if (ptr) {
            memcpy(ptr, framebuffer, window.screen_width * window.screen_height * 4);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, window.screen_width, window.screen_height, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        currentPboIndex = (currentPboIndex + 1) % NUM_PBO;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

#define FLOAT_PER_VERTEX 5

void GenArrays(){
    // Generate VAO, VBO, and EBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    // Bind VAO
        glBindVertexArray(VAO);
    // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Get Max Capacity EBO Buffer
        GLint maxElementBufferSize;
        glGetIntegerv(GL_MAX_ELEMENT_INDEX, &maxElementBufferSize);
    // Allocate memory for VBO
        glBufferData(GL_ARRAY_BUFFER, GL_MAX_VERTEX_ATTRIBS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
    // Bind EBO and allocate memory for it
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GL_MAX_ELEMENT_INDEX * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    // Vertex positions attribute (matches aPos in your shader)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, FLOAT_PER_VERTEX * sizeof(GLfloat), (void*)0);
    // Texture coordinates attribute (matches aTexCoords in your shader)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, FLOAT_PER_VERTEX * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    // Unbind VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Unbind VAO
        glBindVertexArray(0);
    // Generate and initialize PBOs for asynchronous data transfer
        glGenBuffers(2, PBO);
        for (int i = 0; i < 2; ++i) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, window.screen_width * window.screen_height * 4, NULL, GL_STREAM_DRAW);
        }
    // Unbind PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

#include "utils.c"
#include "camera.c"

Shader shaderdefault; // Shader program default

void InitializeShader() {
    // Generate Shader default
        shaderdefault = LoadShader("./res/shaders/default.vert","./res/shaders/default.frag");
        GenArrays();
    // Generate texture for frame buffer
        glGenTextures(1,&window.frame.buffertexture);
        glBindTexture(GL_TEXTURE_2D, window.frame.buffertexture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0, 0, window.screen_width, window.screen_height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexOpt(GL_NEAREST,GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
}
