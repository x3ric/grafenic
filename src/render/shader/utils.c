// Shader Utils

typedef struct {
    GLuint program;
    unsigned int hash;
    GLint location;
    bool used;
} UniformCacheEntry;

#define UNIFORM_CACHE_SIZE 128

static UniformCacheEntry uniformCache[UNIFORM_CACHE_SIZE] = {0};
static GLuint renderTexture = UINT_MAX;
static GLuint renderProgram = UINT_MAX;
static GLuint renderVAO = UINT_MAX;
static bool renderBlendKnown = false;
static bool renderBlend = false;

static unsigned int HashUniform(const char* name) {
    unsigned int hash = 2166136261u;
    while (*name) {
        hash ^= (unsigned char)*name++;
        hash *= 16777619u;
    }
    return hash;
}

static GLint GetUniformLocationCached(GLuint program, const char* name) {
    unsigned int hash = HashUniform(name);
    unsigned int start = (hash ^ program) % UNIFORM_CACHE_SIZE;
    for (unsigned int i = 0; i < UNIFORM_CACHE_SIZE; i++) {
        unsigned int index = (start + i) % UNIFORM_CACHE_SIZE;
        UniformCacheEntry* entry = &uniformCache[index];
        if (!entry->used) {
            entry->program = program;
            entry->hash = hash;
            entry->location = glGetUniformLocation(program, name);
            entry->used = true;
            return entry->location;
        }
        if (entry->program == program && entry->hash == hash) {
            return entry->location;
        }
    }
    return glGetUniformLocation(program, name);
}

static void ClearUniformCache(GLuint program) {
    for (int i = 0; i < UNIFORM_CACHE_SIZE; i++) {
        if (uniformCache[i].used && uniformCache[i].program == program) {
            uniformCache[i].used = false;
        }
    }
}

GLuint CompileShader(const char* shaderSource, GLenum type) {
    if (!shaderSource) return 0;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
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
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    char* buffer = malloc((size_t)length + 1);
    if (!buffer) {
        fclose(file);
        printf("Failed to allocate memory for %s\n", filepath);
        return NULL;
    }
    size_t bytesRead = fread(buffer, 1, (size_t)length, file);
    fclose(file);
    if (bytesRead != (size_t)length) {
        printf("Failed to read %s\n", filepath);
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';
    return buffer;
}

GLuint LinkShaders(const char* vertex, const char* fragment) {
    GLuint vertexShader = CompileShader(vertex, GL_VERTEX_SHADER);
    if (!vertexShader) return 0;
    GLuint fragmentShader = CompileShader(fragment, GL_FRAGMENT_SHADER);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

Shader LoadShader(const char* vertex, const char* fragment) {
    const char* vertexsrc = vertex;
    const char* fragmentsrc = fragment;
    bool freeVertex = false;
    bool freeFragment = false;
    if (FileExists(vertex)) {
        vertexsrc = LoadShaderText(vertex);
        freeVertex = true;
    }
    if (FileExists(fragment)) {
        fragmentsrc = LoadShaderText(fragment);
        freeFragment = true;
    }
    GLuint shaderProgram = LinkShaders(vertexsrc, fragmentsrc);
    if (freeVertex) free((void*)vertexsrc);
    if (freeFragment) free((void*)fragmentsrc);
    Shader shader = {0};
    shader.Program = shaderProgram;
    shader.vertex = vertex;
    shader.fragment = fragment;
    shader.lastvertmodtime = FileExists(vertex) ? GetFileModTime(vertex) : 0;
    shader.lastfragmodtime = FileExists(fragment) ? GetFileModTime(fragment) : 0;
    return shader;
}

Shader ShaderHotReload(Shader shader) {
    if (!shader.hotreloading) return shader;
    time_t currentVertexModTime = FileExists(shader.vertex) ? GetFileModTime(shader.vertex) : shader.lastvertmodtime;
    time_t currentFragmentModTime = FileExists(shader.fragment) ? GetFileModTime(shader.fragment) : shader.lastfragmodtime;
    if (currentVertexModTime == shader.lastvertmodtime && currentFragmentModTime == shader.lastfragmodtime) {
        return shader;
    }
    Shader updated = LoadShader(shader.vertex, shader.fragment);
    if (!updated.Program) return shader;
    updated.hotreloading = shader.hotreloading;
    if (renderProgram == shader.Program) UseShader(0);
    ClearUniformCache(shader.Program);
    glDeleteProgram(shader.Program);
    return updated;
}

void DeleteShader(Shader shader) {
    if (!shader.Program) return;
    if (renderProgram == shader.Program) UseShader(0);
    ClearUniformCache(shader.Program);
    glDeleteProgram(shader.Program);
}

// OpenGl Utils

void BindTexture(GLuint texture) {
    if (renderTexture == texture) return;
    glBindTexture(GL_TEXTURE_2D, texture);
    renderTexture = texture;
}

void SetBlend(bool enabled) {
    if (renderBlendKnown && renderBlend == enabled) return;
    if (enabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    renderBlend = enabled;
    renderBlendKnown = true;
}

void UseShader(GLuint program) {
    if (renderProgram == program) return;
    glUseProgram(program);
    renderProgram = program;
}

void ResetRenderState(void) {
    renderTexture = UINT_MAX;
    renderProgram = UINT_MAX;
    renderVAO = UINT_MAX;
    renderBlendKnown = false;
}

void UnbindTexture() {
    BindTexture(0);
}

void glTexOpt(GLint filter, GLint warp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, warp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, warp);
}

GLint GLuint1i(Shader shader, const char* var, float in) {
    GLint location = GetUniformLocationCached(shader.Program, var);
    glUniform1i(location, (GLint)in);
    return location;
}

GLint GLuint1f(Shader shader, const char* var, float in) {
    GLint location = GetUniformLocationCached(shader.Program, var);
    glUniform1f(location, in);
    return location;
}

GLint GLuint2f(Shader shader, const char* var, float in1, float in2) {
    GLint location = GetUniformLocationCached(shader.Program, var);
    glUniform2f(location, in1, in2);
    return location;
}

GLint GLuint3f(Shader shader, const char* var, float in1, float in2, float in3) {
    GLint location = GetUniformLocationCached(shader.Program, var);
    glUniform3f(location, in1, in2, in3);
    return location;
}

GLint GLuint4f(Shader shader, const char* var, float in1, float in2, float in3, float in4) {
    GLint location = GetUniformLocationCached(shader.Program, var);
    glUniform4f(location, in1, in2, in3, in4);
    return location;
}

GLint GLumatrix4fv(Shader shader, const char* var, GLfloat* in) {
    GLint location = GetUniformLocationCached(shader.Program, var);
    glUniformMatrix4fv(location, 1, GL_FALSE, in);
    return location;
}
