typedef struct {
    GLuint Program;
    const char* vertex;
    const char* fragment;
    time_t lastvertmodtime;
    time_t lastfragmodtime;
    bool hotreloading;
} Shader;

Shader fontshaderdefault; // Shader program default for fonts
Shader pixelshaderdefault; // Shader program default for pixels

GLuint framebufferTexture;
GLuint textTexture;
GLuint VAO;           // Vertex Array Object
GLuint VBO;           // Vertex Buffer Object
GLuint EBO;           // Index Buffer Object
GLuint PBO[2];        // Pixel Buffer Object
int currentPboIndex = 0;

bool pixels = false; // track when is runned "DrawPixel" to clear only when needed "more performance when not using it"

GLuint CompileShader(const char* shaderSource, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);
    GLint success;
    glValidateProgram(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

const char* LoadShaderText(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Failed to open %s\n", filepath);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(file);
        printf("Failed to allocate memory for %s\n", filepath);
        return NULL;
    }
    size_t bytesRead = fread(buffer, 1, length, file);
    fclose(file);
    if (bytesRead != length) {
        printf("Failed to read %s\n", filepath);
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';
    return buffer;
}

GLuint LinkShaders(const char* vertex, const char* fragment) {
    GLuint vertexShader = CompileShader(vertex, GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader(fragment, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        glDeleteProgram(program);
        return 0;
    }
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

Shader LoadShader(const char* vertex, const char* fragment) {
    const char* fragmentsrc = fragment;
    const char* vertexsrc = vertex;
    if (FileExists(vertex)) {
        vertexsrc = LoadShaderText(vertex);
    }
    if (FileExists(fragment)) {
        fragmentsrc = LoadShaderText(fragment);
    }
    GLuint shaderProgram = LinkShaders(vertexsrc, fragmentsrc);
    if (vertex != vertexsrc) {
        free((void*)vertexsrc);
    }
    if (fragment != fragmentsrc) {
        free((void*)fragmentsrc);
    }
    return (Shader){shaderProgram, vertex, fragment};
}

Shader ShaderHotReload(Shader shader){
    time_t currentVertexModTime = GetFileModTime(shader.vertex);
    time_t currentFragmentModTime = GetFileModTime(shader.fragment);
    if (currentVertexModTime != shader.lastvertmodtime || currentFragmentModTime != shader.lastfragmodtime) {
        glDeleteProgram(shader.Program);
        shader = LoadShader(shader.vertex,shader.fragment);
        shader.lastvertmodtime = currentVertexModTime;
        shader.lastfragmodtime = currentFragmentModTime;
    }
    return shader;
}

void GenArrays(){
    // Generate VAO, VBO, and EBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    // Bind VAO
        glBindVertexArray(VAO);
    // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Allocate memory for VBO (6 vertices * 4 attributes per vertex - x, y, u, v)
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 24, NULL, GL_DYNAMIC_DRAW);
    // Bind EBO and allocate memory for it
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 9, NULL, GL_DYNAMIC_DRAW);
    // Vertex positions attribute (matches aPos in your shader)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    // Texture coordinates attribute (matches aTexCoords in your shader)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    // Unbind VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Unbind VAO
        glBindVertexArray(0);
    // Generate and initialize PBOs for asynchronous data transfer
        glGenBuffers(2, PBO);
        for (int i = 0; i < 2; ++i) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, SCREEN_WIDTH * SCREEN_HEIGHT * 4, NULL, GL_STREAM_DRAW);
        }
    // Unbind PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void CreateTextureWithPBO(unsigned char* framebuffer, GLuint framebufferTexture) {
    if(pixels){
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBO[currentPboIndex]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, SCREEN_WIDTH * SCREEN_HEIGHT * 4, NULL, GL_STREAM_DRAW);
    GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (ptr) {
        memcpy(ptr, framebuffer, SCREEN_WIDTH * SCREEN_HEIGHT * 4);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    currentPboIndex = (currentPboIndex + 1) % 2;
    }
}

void InitializeOpenGL() {
    // Generate Shader default
        pixelshaderdefault = LoadShader("./res/shaders/default.vert","./res/shaders/pixel.frag");
        fontshaderdefault = LoadShader("./res/shaders/default.vert","./res/shaders/font.frag");
        GenArrays();
    // Generate Texture for the TextRect 
        glGenTextures(1,&textTexture);
        glBindTexture(GL_TEXTURE_2D, textTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexOpt(GL_NEAREST,GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    // Generate texture for frame buffer
        glGenTextures(1,&framebufferTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexOpt(GL_NEAREST,GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
}

void DeleteShader(Shader shader) {
    glDeleteProgram(shader.Program);
}

GLint GLuint1f(Shader shader,float in,const char* var){
    glUniform1f(glGetUniformLocation(shader.Program, var), in);
}

GLint GLuint2f(Shader shader,float in1,float in2,const char* var){
    glUniform2f(glGetUniformLocation(shader.Program, var), in1, in2);
}

GLint GLumatrix4fv(Shader shader,GLfloat* in,const char* var){
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, var), 1, GL_FALSE, in);
}
