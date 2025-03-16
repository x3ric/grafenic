
GLuint VAO;
GLuint VBO;
GLuint EBO;

Shader shaderdefault;
Shader shaderfont;

#include "utils.c"
#include "math.c"
#include "camera.c"

void InitializeShader() {
    // Load Shaders
    shaderdefault = LoadShader("./res/shaders/default.vert", "./res/shaders/default.frag");
    shaderfont = LoadShader("./res/shaders/default.vert", "./res/shaders/font.frag");
    // Generate VAO, VBO, and EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // Bind VAO
    glBindVertexArray(VAO);
    // Bind VBO & allocate proper memory
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * FLOAT_PER_VERTEX * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    // Bind EBO & allocate proper memory
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(GLuint), NULL, GL_STREAM_DRAW);
    // Vertex Position Attribute (aPos)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, FLOAT_PER_VERTEX * sizeof(GLfloat), (void*)0);
    // Texture Coordinates Attribute (aTexCoords)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, FLOAT_PER_VERTEX * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    // Unbind VAO first, then buffers
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TerminateShader(void){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderdefault.Program);
    glDeleteProgram(shaderfont.Program);
}