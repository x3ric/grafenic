typedef struct {
    Vec3    position;
    GLfloat angle;
} Camera2D;

Camera2D camera2d = {
    0.0f, 0.0f, 0.0f,   // Position: x, y, z
    0.0f                // Angle
};

typedef struct {
    Vec3  position;
    Vec3  rotation;
    float fov;
} Camera;

Camera camera = {
    0.0f, 0.0f, 0.0f,   // Position: x, y, z
    0.0f, 0.0f, 0.0f,   // Rotation: x, y, z
    60.0f               // Fov
};

#include "../math.c"

typedef struct {
    Vec3 position;
    Vec3 globalposition;
    Vec3 rotation;
} Obj;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
typedef struct {
    bool     perspective;
    Shader   shader;
    GLfloat *vertices;
    GLuint  *indices;
    GLfloat  size_vertices;
    GLfloat  size_indices;
    Obj      transform;
} ShaderObject;

void RenderShader(ShaderObject obj) {
    if (obj.shader.hotreloading) {
        obj.shader = ShaderHotReload(obj.shader);
    }
    // Projection Matrix
        GLfloat Projection[16];
        GLfloat Model[16];
        GLfloat View[16];
        if(obj.perspective){
            Vec3 pos = obj.transform.position;
            Vec3 gpos = obj.transform.globalposition;
            Vec3 rot = obj.transform.rotation;
            MatrixPerspective(camera.fov, SCREEN_WIDTH/SCREEN_HEIGHT, 0.001f, 1000.0f, Projection);
            MatrixRotate(rot.x, rot.y, rot.z, Model);
            MatrixTranslate(gpos.x, gpos.y, gpos.z, Model);
            MatrixLookAt(pos.x, pos.y, pos.z + 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, View);
        } else {
            if(camera2d.position.z <= 0.0f)
                camera2d.position.z += 1.0f;
            MatrixOrthographic(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f, Projection);
        }
    // Debug
        if (debug.wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    // Bind VAO
        glBindVertexArray(VAO);
    // Bind VBO and update with new vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, obj.size_vertices, obj.vertices);
    // Bind EBO and update with new index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, obj.size_indices, obj.indices);
    // Use the shader program
        glUseProgram(obj.shader.Program);
    // Set uniforms
        glUniform1i(glGetUniformLocation(obj.shader.Program, "perspective"), obj.perspective ? 1 : 0);
        glUniformMatrix4fv(glGetUniformLocation(obj.shader.Program, "projection"), 1, GL_FALSE, Projection);
        if(obj.perspective){
            glUniformMatrix4fv(glGetUniformLocation(obj.shader.Program, "view"), 1, GL_FALSE, View);
            glUniformMatrix4fv(glGetUniformLocation(obj.shader.Program, "model"), 1, GL_FALSE, Model);
        } else {
            glUniform3f(glGetUniformLocation(obj.shader.Program, "position"), camera2d.position.x, camera2d.position.y, camera2d.position.z);
            glUniform1f(glGetUniformLocation(obj.shader.Program, "angle"), camera2d.angle);
        }
        glUniform1f(glGetUniformLocation(obj.shader.Program, "iTime"), glfwGetTime());
        glUniform2f(glGetUniformLocation(obj.shader.Program, "iResolution"), SCREEN_WIDTH, SCREEN_HEIGHT);
        glUniform2f(glGetUniformLocation(obj.shader.Program, "iMouse"), mouse.x, mouse.y);
    // Draw using indices
        glDrawElements(GL_TRIANGLES, obj.size_indices / sizeof(GLuint), GL_UNSIGNED_INT, 0);
    // Unbind shader program
        glUseProgram(0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Triangle(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3, GLfloat angle, Shader shader) {
    // Calculate angle
    GLfloat angleInDegrees = angle;
    GLfloat angleInRadians = angle * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    //Camera Movement
    x1 += camera2d.position.x;
    y1 += camera2d.position.y;
    x2 += camera2d.position.x;
    y2 += camera2d.position.y;
    x3 += camera2d.position.x;
    y3 += camera2d.position.y;
    // Calculate the center of the tinagle for rotation
    float centerX = (x1 + x2 + x3) / 3.0f;
    float centerY = (y1 + y2 + y3) / 3.0f;
    // Calculate rotated positions for the rectangle
    GLfloat rotatedX1 = cosAngle * (x1 - centerX) - sinAngle * (y1 - centerY) + centerX;
    GLfloat rotatedY1 = sinAngle * (x1 - centerX) + cosAngle * (y1 - centerY) + centerY;
    GLfloat rotatedX2 = cosAngle * (x2 - centerX) - sinAngle * (y2 - centerY) + centerX;
    GLfloat rotatedY2 = sinAngle * (x2 - centerX) + cosAngle * (y2 - centerY) + centerY;
    GLfloat rotatedX3 = cosAngle * (x3 - centerX) - sinAngle * (y3 - centerY) + centerX;
    GLfloat rotatedY3 = sinAngle * (x3 - centerX) + cosAngle * (y3 - centerY) + centerY;
    // Define vertices
    GLfloat vertices[] = {
        // Positions                 // Texture Coords        
        rotatedX1, rotatedY1, 0.0f,  0.0f, 0.0f,  // Vertex 0 
        rotatedX2, rotatedY2, 0.0f,  1.0f, 0.0f,  // Vertex 1 
        rotatedX3, rotatedY3, 0.0f,  0.5f, 1.0f,  // Vertex 2 
    };
    /*
        0
        | \ 
        |   \ 
        |     \ 
        1-------2 
    */
    GLuint indices[] = {
        0,1,2
    };
    RenderShader((ShaderObject){false,shader,vertices,indices,sizeof(vertices),sizeof(indices)});
}

void Rect(float x, float y, float width, float height, GLfloat angle,Shader shader) {
    // Calculate angle
    GLfloat angleInDegrees = angle;
    GLfloat angleInRadians = angleInDegrees * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    //Camera Movement
    x += camera2d.position.x;
    y += camera2d.position.y;
    // Calculate the center of the rect for rotation
    float centerX = x + width / 2.0f;
    float centerY = y + height / 2.0f;
    // Calculate rotated positions for the rectangle
    float rotatedX1 = cosAngle * (x - centerX) - sinAngle * (y - centerY) + centerX;
    float rotatedY1 = sinAngle * (x - centerX) + cosAngle * (y - centerY) + centerY;
    float rotatedX2 = cosAngle * (x + width - centerX) - sinAngle * (y - centerY) + centerX;
    float rotatedY2 = sinAngle * (x + width - centerX) + cosAngle * (y - centerY) + centerY;
    float rotatedX3 = cosAngle * (x + width - centerX) - sinAngle * (y + height - centerY) + centerX;
    float rotatedY3 = sinAngle * (x + width - centerX) + cosAngle * (y + height - centerY) + centerY;
    float rotatedX4 = cosAngle * (x - centerX) - sinAngle * (y + height - centerY) + centerX;
    float rotatedY4 = sinAngle * (x - centerX) + cosAngle * (y + height - centerY) + centerY;
    // Define vertices (Positions and Texture Coords)
    GLfloat vertices[] = {
        // First Triangle
        (x - centerX) * cosAngle - (y + height - centerY) * sinAngle + centerX, (x - centerX) * sinAngle + (y + height - centerY) * cosAngle + centerY, 0.0f,  0.0f, 0.0f,  // Bottom Left
        (x + width - centerX) * cosAngle - (y - centerY) * sinAngle + centerX, (x + width - centerX) * sinAngle + (y - centerY) * cosAngle + centerY, 0.0f,  1.0f, 1.0f,  // Top Right
        (x - centerX) * cosAngle - (y - centerY) * sinAngle + centerX, (x - centerX) * sinAngle + (y - centerY) * cosAngle + centerY, 0.0f,  0.0f, 1.0f,  // Top Left
        // Second Triangle
        (x - centerX) * cosAngle - (y + height - centerY) * sinAngle + centerX, (x - centerX) * sinAngle + (y + height - centerY) * cosAngle + centerY, 0.0f,  0.0f, 0.0f,  // Bottom Left
        (x + width - centerX) * cosAngle - (y + height - centerY) * sinAngle + centerX, (x + width - centerX) * sinAngle + (y + height - centerY) * cosAngle + centerY, 0.0f,  1.0f, 0.0f,  // Bottom Right
        (x + width - centerX) * cosAngle - (y - centerY) * sinAngle + centerX, (x + width - centerX) * sinAngle + (y - centerY) * cosAngle + centerY, 0.0f,  1.0f, 1.0f,  // Top Right
    };
    /*
        0-------1/4
        |     / |
        |   /   |
        | /     |
      2/5-------3 
    */                 
    GLuint indices[] = {   
        0,1,2,             
        3,4,5              
    };                                                                                        
    RenderShader((ShaderObject){false,shader,vertices,indices,sizeof(vertices),sizeof(indices)});
}

void Line(float x0, float y0, float x1, float y1, int thickness, Shader shader) {
    // Calculate angle
    GLfloat angleInDegrees = atan2(y1 - y0, x1 - x0);
    GLfloat angleInRadians = angleInDegrees * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    // Calculate the half thickness
    float halfThickness = thickness / 2.0f;
    // Calculate the length of the line
    float length = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    // Calculate the vertices of the rectangle for the line
    float x2 = x0 - halfThickness * sinAngle;
    float y2 = y0 + halfThickness * cosAngle;
    float x3 = x1 - halfThickness * sinAngle;
    float y3 = y1 + halfThickness * cosAngle;
    float x4 = x1 + halfThickness * sinAngle;
    float y4 = y1 - halfThickness * cosAngle;
    float x5 = x0 + halfThickness * sinAngle;
    float y5 = y0 - halfThickness * cosAngle;
    // Define the vertices for drawing the rectangle
    GLfloat vertices[] = {
        // Positions   // Texture Coords        
        x2, y2, 0.0f,  0.0f, 0.0f,  // Bottom left
        x3, y3, 0.0f,  1.0f, 0.0f,  // Top left
        x4, y4, 0.0f,  1.0f, 1.0f,  // Top right
        x5, y5, 0.0f,  0.0f, 1.0f   // Bottom right
    };
    /*
        0---1/4
        |  /|
        | / |
        |/  |
      2/5---3 
    */       
    // Indices for the rectangle (two triangles)
    GLuint indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    RenderShader((ShaderObject){false,shader,vertices,indices,sizeof(vertices),sizeof(indices)});
}

void Zelda(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3, GLfloat angle, Shader shader) {
    // Calculate angle
    GLfloat angleInDegrees = angle;
    GLfloat angleInRadians = angleInDegrees * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    //Camera Movement
    x1 += camera2d.position.x;
    y1 += camera2d.position.y;
    x2 += camera2d.position.x;
    y2 += camera2d.position.y;
    x3 += camera2d.position.x;
    y3 += camera2d.position.y;
    // Calculate the center of the triangle for rotation
    float centerX = (x1 + x2 + x3) / 3.0f;
    float centerY = (y1 + y2 + y3) / 3.0f;
    // Apply rotation to each vertex of the outer triangle
    GLfloat rotatedX1 = cosAngle * (x1 - centerX) - sinAngle * (y1 - centerY) + centerX;
    GLfloat rotatedY1 = sinAngle * (x1 - centerX) + cosAngle * (y1 - centerY) + centerY;
    GLfloat rotatedX2 = cosAngle * (x2 - centerX) - sinAngle * (y2 - centerY) + centerX;
    GLfloat rotatedY2 = sinAngle * (x2 - centerX) + cosAngle * (y2 - centerY) + centerY;
    GLfloat rotatedX3 = cosAngle * (x3 - centerX) - sinAngle * (y3 - centerY) + centerX;
    GLfloat rotatedY3 = sinAngle * (x3 - centerX) + cosAngle * (y3 - centerY) + centerY;
    // Calculate midpoints for the inner triangle
    GLfloat midX12 = (rotatedX1 + rotatedX2) / 2.0f;
    GLfloat midY12 = (rotatedY1 + rotatedY2) / 2.0f;
    GLfloat midX23 = (rotatedX2 + rotatedX3) / 2.0f;
    GLfloat midY23 = (rotatedY2 + rotatedY3) / 2.0f;
    GLfloat midX31 = (rotatedX3 + rotatedX1) / 2.0f;
    GLfloat midY31 = (rotatedY3 + rotatedY1) / 2.0f;
    // Define vertices for both triangles (outer and inner)
    GLfloat vertices[] = {
        // Outer Triangle           // Texture Coords 
        rotatedX3, rotatedY3, 0.0f, 0.5f, 1.0f,  // 0 Bottom Left
        rotatedX2, rotatedY2, 0.0f, 1.0f, 0.0f,  // 1 Bottom right
        rotatedX1, rotatedY1, 0.0f, 0.0f, 0.0f,  // 2 Top middle
        // Inner Inverted Triangle
        midX31, midY31, 0.0f,       0.0f, 1.0f,  // 3 Top Left
        midX12, midY12, 0.0f,       0.5f, 0.0f,  // 4 Top right
        midX23, midY23, 0.0f,       1.0f, 1.0f,  // 5 Bottom middle
    };
    /*
             2 
            / \ 
           /   \  
          4-----3 
         / \   / \  
        /   \ /   \ 
       1-----5-----0
    */
    // Test to use index buffers
    // Indices for both triangles         
    GLuint indices[] = {        
        0, 3, 5, // Inner Triangle 1 bottom left
        3, 2, 4, // Inner Triangle 2 bottom right     
        5, 4, 1  // Inner Triangle 3 top middle 
    };                          
    RenderShader((ShaderObject){false,shader,vertices,indices,sizeof(vertices),sizeof(indices)});
}

typedef struct {
    Obj    obj;
    int    size;
    Shader shader;
} CubeObject;

void Cube(CubeObject cube) {
    GLfloat halfSize = cube.size / 2.0f;
    Vec3 pos = cube.obj.position;
    Vec3 rot = cube.obj.rotation;
    GLfloat vertices[] = {
        // Front face
        pos.x - halfSize, pos.y - halfSize, pos.z + halfSize, 0.0f, 0.0f, // Vertex 0
        pos.x + halfSize, pos.y - halfSize, pos.z + halfSize, 1.0f, 0.0f, // Vertex 1
        pos.x + halfSize, pos.y + halfSize, pos.z + halfSize, 1.0f, 1.0f, // Vertex 2
        pos.x - halfSize, pos.y + halfSize, pos.z + halfSize, 0.0f, 1.0f, // Vertex 3
        // Back face
        pos.x - halfSize, pos.y - halfSize, pos.z - halfSize, 1.0f, 0.0f, // Vertex 4
        pos.x + halfSize, pos.y - halfSize, pos.z - halfSize, 0.0f, 0.0f, // Vertex 5
        pos.x + halfSize, pos.y + halfSize, pos.z - halfSize, 0.0f, 1.0f, // Vertex 6
        pos.x - halfSize, pos.y + halfSize, pos.z - halfSize, 1.0f, 1.0f, // Vertex 7
        // Top face
        pos.x - halfSize, pos.y + halfSize, pos.z - halfSize, 0.0f, 1.0f, // Vertex 8
        pos.x + halfSize, pos.y + halfSize, pos.z - halfSize, 1.0f, 1.0f, // Vertex 9
        pos.x + halfSize, pos.y + halfSize, pos.z + halfSize, 1.0f, 0.0f, // Vertex 10
        pos.x - halfSize, pos.y + halfSize, pos.z + halfSize, 0.0f, 0.0f, // Vertex 11
        // Bottom face
        pos.x - halfSize, pos.y - halfSize, pos.z - halfSize, 0.0f, 0.0f, // Vertex 12
        pos.x + halfSize, pos.y - halfSize, pos.z - halfSize, 1.0f, 0.0f, // Vertex 13
        pos.x + halfSize, pos.y - halfSize, pos.z + halfSize, 1.0f, 1.0f, // Vertex 14
        pos.x - halfSize, pos.y - halfSize, pos.z + halfSize, 0.0f, 1.0f, // Vertex 15
        // Left face
        pos.x - halfSize, pos.y - halfSize, pos.z - halfSize, 0.0f, 0.0f, // Vertex 16
        pos.x - halfSize, pos.y + halfSize, pos.z - halfSize, 1.0f, 0.0f, // Vertex 17
        pos.x - halfSize, pos.y + halfSize, pos.z + halfSize, 1.0f, 1.0f, // Vertex 18
        pos.x - halfSize, pos.y - halfSize, pos.z + halfSize, 0.0f, 1.0f, // Vertex 19
        // Right face
        pos.x + halfSize, pos.y - halfSize, pos.z + halfSize, 0.0f, 0.0f, // Vertex 20
        pos.x + halfSize, pos.y + halfSize, pos.z + halfSize, 1.0f, 0.0f, // Vertex 21
        pos.x + halfSize, pos.y + halfSize, pos.z - halfSize, 1.0f, 1.0f, // Vertex 22
        pos.x + halfSize, pos.y - halfSize, pos.z - halfSize, 0.0f, 1.0f  // Vertex 23
    };
    // Apply rotation to all vertices
    for (int i = 0; i < 24; ++i) {
        RotateVertex(&vertices[i * 5], &vertices[i * 5 + 1], &vertices[i * 5 + 2], rot.x, rot.y, rot.z);
    }
    GLuint indices[] = {
        0, 1, 2, 2, 3, 0,    // Front face
        4, 5, 6, 6, 7, 4,    // Back face
        12, 13, 14, 14, 15, 12, // Bottom face
        8, 9, 10, 10, 11, 8,    // Top face
        16, 17, 18, 18, 19, 16, // Left face
        20, 21, 22, 22, 23, 20  // Right face
    };
    RenderShader((ShaderObject){true, cube.shader, vertices, indices, sizeof(vertices), sizeof(indices), cube.obj});
}

void Framebuffer(float x, float y, float width, float height) {
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    CreateTextureWithPBO(framebuffer,framebufferTexture);
    Rect(x, y, width, height,0,shaderdefault);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}
