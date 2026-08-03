
Camera camera = {
    .transform = {
        .position = {0.0f, 0.0f, 0.0f},      // Position: x, y, z
        .localposition = {0.0f, 0.0f, 0.0f}, // LocalPosition: x, y, z
        .rotation = {0.0f, 0.0f, 0.0f}       // Rotation: x, y, z
    },
    .fov = 0.0f,  // Fov
    .far = 0.0f,  // Far Distance
    .near = 0.0f  // Near Distance
};

static GLuint renderArrayBuffer = UINT_MAX;
static GLuint renderElementBuffer = UINT_MAX;
static size_t vertexBufferCapacity = MAX_VERTICES * FLOAT_PER_VERTEX * sizeof(GLfloat);
static size_t indexBufferCapacity = MAX_INDICES * sizeof(GLuint);
static int renderPolygonMode = -1;
static bool renderDepth = false;
static bool renderCull = false;
static GLenum renderDepthFunc = 0;
static GLenum renderCullFace = 0;

static Shader ResolveRenderShader(Shader shader) {
    if (!shader.hotreloading) return shader;
    if (shader.Program == shaderdefault.Program) {
        shaderdefault = ShaderHotReload(shaderdefault);
        return shaderdefault;
    }
    if (shader.Program == shaderfont.Program) {
        shaderfont = ShaderHotReload(shaderfont);
        return shaderfont;
    }
    return shader;
}

static void SetPolygonMode(void) {
    int mode = GL_FILL;
    if (window.debug.wireframe) mode = GL_LINE;
    else if (window.debug.point) mode = GL_POINT;
    if (renderPolygonMode != mode) {
        glPolygonMode(GL_FRONT_AND_BACK, mode);
        renderPolygonMode = mode;
    }
    if (mode == GL_POINT && window.debug.pointsize > 0.0f) {
        glPointSize(window.debug.pointsize);
    }
}

static void SetDepthMode(ShaderObject obj) {
    bool depth = obj.is3d;
    bool cull = obj.is3d;
    GLenum depthFunc = obj.cam.fov > 0.0f ? GL_LEQUAL : GL_LESS;
    GLenum cullFace = obj.cam.fov > 0.0f ? GL_BACK : GL_FRONT;
    if (renderDepth != depth) {
        if (depth) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        renderDepth = depth;
    }
    if (renderCull != cull) {
        if (cull) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        renderCull = cull;
    }
    if (depth && renderDepthFunc != depthFunc) {
        glDepthFunc(depthFunc);
        renderDepthFunc = depthFunc;
    }
    if (cull && renderCullFace != cullFace) {
        glCullFace(cullFace);
        renderCullFace = cullFace;
    }
}

static void BindRenderBuffers(void) {
    if (renderVAO != VAO) {
        glBindVertexArray(VAO);
        renderVAO = VAO;
        renderElementBuffer = EBO;
    }
    if (renderArrayBuffer != VBO) {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        renderArrayBuffer = VBO;
    }
    if (renderElementBuffer != EBO) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        renderElementBuffer = EBO;
    }
}

static void UploadRenderBuffers(ShaderObject obj) {
    if (obj.size_vertices > vertexBufferCapacity) {
        while (vertexBufferCapacity < obj.size_vertices) vertexBufferCapacity *= 2;
        glBufferData(GL_ARRAY_BUFFER, vertexBufferCapacity, NULL, GL_STREAM_DRAW);
    }
    if (obj.size_indices > indexBufferCapacity) {
        while (indexBufferCapacity < obj.size_indices) indexBufferCapacity *= 2;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferCapacity, NULL, GL_STREAM_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, obj.size_vertices, obj.vertices);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, obj.size_indices, obj.indices);
}

void CalculateProjections(ShaderObject obj, GLfloat* Model, GLfloat* Projection, GLfloat* View) {
    Vec3 lpos = obj.transform.localposition;
    Vec3 pos = obj.transform.position;
    Vec3 rot = obj.transform.rotation;
    Vec3 cpos = obj.cam.transform.position;
    float centerX = window.screen_width * 0.5f;
    float centerY = window.screen_height * 0.5f;
    float distance = 1.0f;
    float nearDistance = obj.cam.near;
    float farDistance = obj.cam.far > 0.0f ? obj.cam.far : 1000.0f;
    GLfloat translateToCenter[16], rotate[16], translateBack[16], translateFinal[16];
    if (obj.cam.fov > 0.0f) { // Perspective projection
        MatrixPerspective(obj.cam.fov, (GLfloat)window.screen_width / (GLfloat)window.screen_height, nearDistance, farDistance, obj.is3d, Projection);
        MatrixTranslate(-pos.x, -pos.y, -pos.z, translateToCenter);
        MatrixRotate(rot.x, rot.y, rot.z, rotate);
        MatrixMultiply(translateToCenter, rotate, Model);
        MatrixTranslate(pos.x, pos.y, pos.z, translateBack);
        MatrixMultiply(Model, translateBack, Model);
        distance = 3.0f;
    } else { // Orthographic projection
        if (obj.is3d) { // if the model vertices are also in z axys
            MatrixOrthographicZoom(0.0f, window.screen_width, window.screen_height, 0.0f, nearDistance, farDistance, pos.z + cpos.z, true, Projection);
            MatrixTranslate(-pos.x, -pos.y, -pos.z, translateToCenter);
            MatrixRotate(rot.x, rot.y, rot.z, rotate);
            MatrixMultiply(translateToCenter, rotate, Model);
            MatrixTranslate(pos.x, pos.y, pos.z, translateBack);
            MatrixMultiply(Model, translateBack, Model);
        } else {
            MatrixOrthographicZoom(0.0f, window.screen_width, window.screen_height, 0.0f, nearDistance, farDistance, pos.z, false, Projection);
            MatrixRotate(rot.x, rot.y, rot.z, rotate);
            MatrixTranslate(centerX, centerY, 0.0f, translateBack);
            MatrixTranslate(-centerX, -centerY, 0.0f, translateToCenter);
            MatrixTranslate(pos.x, pos.y, 0.0f, translateFinal);
            MatrixMultiply(translateToCenter, rotate, Model);
            MatrixMultiply(Model, translateBack, Model);
            MatrixMultiply(Model, translateFinal, Model);
        }
    }
    MatrixLookAt(
        lpos.x, lpos.y, lpos.z + distance, // Eye position
        0.0f, 0.0f, 0.0f,                // Look at position
        0.0f, 1.0f, 0.0f,                // Up vector
        View
    );
}

void RenderShader(ShaderObject obj) {
    if (!obj.shader.Program || !obj.vertices || !obj.indices || !obj.size_vertices || !obj.size_indices) return;
    obj.shader = ResolveRenderShader(obj.shader);
    GLfloat Projection[16], Model[16], View[16];
    CalculateProjections(obj, Model, Projection, View);
    SetDepthMode(obj);
    SetPolygonMode();
    BindRenderBuffers();
    UploadRenderBuffers(obj);
    UseShader(obj.shader.Program);
    GLumatrix4fv(obj.shader, "projection", Projection);
    GLumatrix4fv(obj.shader, "model", Model);
    GLumatrix4fv(obj.shader, "view", View);
    GLuint1f(obj.shader, "iTime", (float)window.time);
    GLuint2f(obj.shader, "iResolution", (float)window.screen_width, (float)window.screen_height);
    GLuint2f(obj.shader, "iMouse", (float)mouse.x, (float)mouse.y);
    glDrawElements(GL_TRIANGLES, (GLsizei)(obj.size_indices / sizeof(GLuint)), GL_UNSIGNED_INT, 0);
}

void Triangle(TriangleObject triangle) {
    GLfloat x0 = triangle.vert0.x, y0 = triangle.vert0.y;
    GLfloat x1 = triangle.vert1.x, y1 = triangle.vert1.y;
    GLfloat x2 = triangle.vert2.x, y2 = triangle.vert2.y;
    if (triangle.cam.fov > 0.0f) {
        GLfloat sx = 2.0f / (GLfloat)window.screen_width;
        GLfloat sy = 2.0f / (GLfloat)window.screen_height;
        x0 = 1.0f - x0 * sx; y0 = 1.0f - y0 * sy;
        x1 = 1.0f - x1 * sx; y1 = 1.0f - y1 * sy;
        x2 = 1.0f - x2 * sx; y2 = 1.0f - y2 * sy;
    }
    GLfloat vertices[] = {
        x0, y0, triangle.vert0.z, 0.0f, 0.0f, // Vertex 0
        x1, y1, triangle.vert1.z, 1.0f, 0.0f, // Vertex 1
        x2, y2, triangle.vert2.z, 0.5f, 1.0f  // Vertex 2
    };
    /*
        0
        | \
        |   \
        |     \
        1-------2
    */
    static GLuint indices[] = {0, 1, 2};
    RenderShader((ShaderObject){triangle.cam, triangle.shader, vertices, indices, sizeof(vertices), sizeof(indices), triangle.cam.transform, false});
}

void Zelda(TriangleObject triangle) {
    GLfloat x0 = triangle.vert0.x, y0 = triangle.vert0.y;
    GLfloat x1 = triangle.vert1.x, y1 = triangle.vert1.y;
    GLfloat x2 = triangle.vert2.x, y2 = triangle.vert2.y;
    GLfloat mx01 = (x0 + x1) * 0.5f, my01 = (y0 + y1) * 0.5f;
    GLfloat mx12 = (x1 + x2) * 0.5f, my12 = (y1 + y2) * 0.5f;
    GLfloat mx20 = (x2 + x0) * 0.5f, my20 = (y2 + y0) * 0.5f;
    if (triangle.cam.fov > 0.0f) {
        GLfloat sx = 2.0f / (GLfloat)window.screen_width;
        GLfloat sy = 2.0f / (GLfloat)window.screen_height;
        x0 = 1.0f - x0 * sx; y0 = 1.0f - y0 * sy;
        x1 = 1.0f - x1 * sx; y1 = 1.0f - y1 * sy;
        x2 = 1.0f - x2 * sx; y2 = 1.0f - y2 * sy;
        mx01 = 1.0f - mx01 * sx; my01 = 1.0f - my01 * sy;
        mx12 = 1.0f - mx12 * sx; my12 = 1.0f - my12 * sy;
        mx20 = 1.0f - mx20 * sx; my20 = 1.0f - my20 * sy;
    }
    GLfloat vertices[] = {
        // Triangle vertices
        x0, y0, triangle.vert0.z, 0.0f, 0.0f, // Vertex 0
        x1, y1, triangle.vert1.z, 1.0f, 0.0f, // Vertex 1
        x2, y2, triangle.vert2.z, 0.5f, 1.0f, // Vertex 2
        // Midpoints
        mx01, my01, (triangle.vert0.z + triangle.vert1.z) * 0.5f, 0.0f, 0.5f, // Midpoint 01
        mx12, my12, (triangle.vert1.z + triangle.vert2.z) * 0.5f, 1.0f, 0.5f, // Midpoint 12
        mx20, my20, (triangle.vert2.z + triangle.vert0.z) * 0.5f, 0.5f, 1.0f  // Midpoint 20
    };
    /*
             2
            / \
           /   \
          5-----4
         / \   / \
        /   \ /   \
       0-----3-----1
    */
    static GLuint indices[] = {
        0, 3, 5, // Inner Triangle 1 bottom left
        3, 4, 1, // Inner Triangle 2 bottom right
        5, 4, 2  // Inner Triangle 3 top middle
    };
    RenderShader((ShaderObject){triangle.cam, triangle.shader, vertices, indices, sizeof(vertices), sizeof(indices), triangle.cam.transform, false});
}

void Rect(RectObject rect) {
    GLfloat x0 = rect.vert0.x, y0 = rect.vert0.y;
    GLfloat x1 = rect.vert1.x, y1 = rect.vert1.y;
    GLfloat x2 = rect.vert2.x, y2 = rect.vert2.y;
    GLfloat x3 = rect.vert3.x, y3 = rect.vert3.y;
    if (rect.cam.fov > 0.0f) { // Perspective projection
        GLfloat sx = 2.0f / (GLfloat)window.screen_width;
        GLfloat sy = 2.0f / (GLfloat)window.screen_height;
        x0 = 1.0f - x0 * sx; y0 = 1.0f - y0 * sy;
        x1 = 1.0f - x1 * sx; y1 = 1.0f - y1 * sy;
        x2 = 1.0f - x2 * sx; y2 = 1.0f - y2 * sy;
        x3 = 1.0f - x3 * sx; y3 = 1.0f - y3 * sy;
    }
    GLfloat vertices[] = {
        x0, y0, rect.vert0.z, 0.0f, 0.0f, // Bottom Left
        x1, y1, rect.vert1.z, 1.0f, 0.0f, // Bottom Right
        x2, y2, rect.vert2.z, 0.0f, 1.0f, // Top Left
        x3, y3, rect.vert3.z, 1.0f, 1.0f  // Top Right
    };
    /*
        0-------1
        |     / |
        |   /   |
        | /     |
        2-------3
    */
    static GLuint indices[] = {0, 1, 2, 1, 3, 2};
    RenderShader((ShaderObject){rect.cam, rect.shader, vertices, indices, sizeof(vertices), sizeof(indices), rect.cam.transform, false});
}

void Cube(CubeObject cube) {
    GLfloat hs = cube.size * 0.5f;
    Vec3 pos = cube.transform.position;
    GLfloat x1 = pos.x - hs, x2 = pos.x + hs;
    GLfloat y1 = pos.y - hs, y2 = pos.y + hs;
    GLfloat z1 = pos.z - hs, z2 = pos.z + hs;
    GLfloat vertices[] = {
        // Front face
        x1, y1, z2, 0.0f, 0.0f,
        x2, y1, z2, 1.0f, 0.0f,
        x2, y2, z2, 1.0f, 1.0f,
        x1, y2, z2, 0.0f, 1.0f,
        // Back face
        x1, y1, z1, 0.0f, 0.0f,
        x2, y1, z1, 1.0f, 0.0f,
        x2, y2, z1, 1.0f, 1.0f,
        x1, y2, z1, 0.0f, 1.0f,
        // Top face
        x1, y2, z1, 0.0f, 0.0f,
        x2, y2, z1, 1.0f, 0.0f,
        x2, y2, z2, 1.0f, 1.0f,
        x1, y2, z2, 0.0f, 1.0f,
        // Bottom face
        x1, y1, z1, 0.0f, 1.0f,
        x2, y1, z1, 1.0f, 1.0f,
        x2, y1, z2, 1.0f, 0.0f,
        x1, y1, z2, 0.0f, 0.0f,
        // Right face
        x2, y1, z1, 0.0f, 0.0f,
        x2, y2, z1, 0.0f, 1.0f,
        x2, y2, z2, 1.0f, 1.0f,
        x2, y1, z2, 1.0f, 0.0f,
        // Left face
        x1, y1, z1, 0.0f, 0.0f,
        x1, y2, z1, 1.0f, 0.0f,
        x1, y2, z2, 1.0f, 1.0f,
        x1, y1, z2, 0.0f, 1.0f
    };
    static GLuint indices[] = {
        0, 1, 2, 2, 3, 0,       // Front face
        4, 7, 6, 6, 5, 4,       // Back face
        8, 11, 10, 10, 9, 8,    // Top face
        12, 13, 14, 14, 15, 12, // Bottom face
        16, 17, 18, 18, 19, 16, // Right face
        20, 23, 22, 22, 21, 20  // Left face
    };
    RenderShader((ShaderObject){cube.cam, cube.shader, vertices, indices, sizeof(vertices), sizeof(indices), cube.transform, true});
}
