typedef struct {
    float x;
    float y;
    float z;
    GLfloat angle;
} Camera;

Camera camera = {0.0f, 0.0f , 0.0f, (GLfloat)0};

void RenderShaderOrtho(Shader shader, GLfloat vertices[], GLfloat vertSize, GLuint indices[], GLfloat indexSize, int count) {
    if(shader.hotreloading){ // regenerate shader on file change
        shader = ShaderHotReload(shader);
    }
    GLdouble left = 0;
    GLdouble right = SCREEN_WIDTH;
    GLdouble bottom = SCREEN_HEIGHT;
    GLdouble top = 0;
    GLdouble zNear = -1;
    GLdouble zFar = 1;
    GLfloat orthoMatrix[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -2.0f / (zFar - zNear), 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zFar + zNear) / (zFar - zNear), 1.0f
    };
    if (debug.wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    // Bind VAO
        glBindVertexArray(VAO);
    // Bind VBO and update with new vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertSize, vertices);
    // Bind EBO and update with new index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexSize, indices);
    // Use the shader program
        glUseProgram(shader.Program);
    // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, orthoMatrix);
        glUniform1f(glGetUniformLocation(shader.Program, "z"), camera.z);
        glUniform1f(glGetUniformLocation(shader.Program, "iTime"), glfwGetTime());
        glUniform2f(glGetUniformLocation(shader.Program, "iResolution"), SCREEN_WIDTH, SCREEN_HEIGHT);
        glUniform2f(glGetUniformLocation(shader.Program, "iMouse"), mouse.x, mouse.y);
    // Draw using indices
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
    // Unbind shader program
        glUseProgram(0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Triangle(Shader shader, GLfloat angle, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3) {
    // Calculate angle
    GLfloat angleInDegrees = camera.angle + angle;
    GLfloat angleInRadians = angleInDegrees * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    //Camera Movement
    x1 += camera.x;
    y1 += camera.y;
    x2 += camera.x;
    y2 += camera.y;
    x3 += camera.x;
    y3 += camera.y;
    // Calculate the center of the tinagle for rotation
    float centerX = (x1 + x2 + x3) / 3.0f;
    float centerY = (y1 + y2 + y3) / 3.0f;
    GLfloat rotatedX1 = cosAngle * (x1 - centerX) - sinAngle * (y1 - centerY) + centerX;
    GLfloat rotatedY1 = sinAngle * (x1 - centerX) + cosAngle * (y1 - centerY) + centerY;
    GLfloat rotatedX2 = cosAngle * (x2 - centerX) - sinAngle * (y2 - centerY) + centerX;
    GLfloat rotatedY2 = sinAngle * (x2 - centerX) + cosAngle * (y2 - centerY) + centerY;
    GLfloat rotatedX3 = cosAngle * (x3 - centerX) - sinAngle * (y3 - centerY) + centerX;
    GLfloat rotatedY3 = sinAngle * (x3 - centerX) + cosAngle * (y3 - centerY) + centerY;
    // Define vertices
    GLfloat vertices[] = {
        // Positions          // Texture Coords        
        rotatedX1, rotatedY1, 0.0f, 0.0f,  // Vertex 0 
        rotatedX2, rotatedY2, 1.0f, 0.0f,  // Vertex 1 
        rotatedX3, rotatedY3, 0.5f, 1.0f,  // Vertex 2 
    };
    /*
            0
            / \ 
            /   \ 
            /     \ 
        1-------2 
    */
    GLuint indices[] = {
        0,1,2
    };
    GLuint verts = sizeof(vertices);
    GLuint indexs = sizeof(indices);
    RenderShaderOrtho(shader,vertices ,verts ,indices ,indexs , 3);
}

void Quad(float x, float y, float width, float height, GLfloat angle,Shader shader) {
    // Calculate angle
    GLfloat angleInDegrees = camera.angle + angle;
    GLfloat angleInRadians = angleInDegrees * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    //Camera Movement
        x += camera.x;
        y += camera.y;
    // Calculate the center of the quad for rotation
    float centerX = x + width / 2.0f;
    float centerY = y + height / 2.0f;
    // Define the rotated vertices directly in the vertices array
    GLfloat vertices[] = {
        // First Triangle
        (x - centerX) * cosAngle - (y + height - centerY) * sinAngle + centerX, (x - centerX) * sinAngle + (y + height - centerY) * cosAngle + centerY, 0.0f, 0.0f,  // Bottom Left
        (x + width - centerX) * cosAngle - (y - centerY) * sinAngle + centerX, (x + width - centerX) * sinAngle + (y - centerY) * cosAngle + centerY, 1.0f, 1.0f,  // Top Right
        (x - centerX) * cosAngle - (y - centerY) * sinAngle + centerX, (x - centerX) * sinAngle + (y - centerY) * cosAngle + centerY, 0.0f, 1.0f,  // Top Left
        // Second Triangle
        (x - centerX) * cosAngle - (y + height - centerY) * sinAngle + centerX, (x - centerX) * sinAngle + (y + height - centerY) * cosAngle + centerY, 0.0f, 0.0f,  // Bottom Left
        (x + width - centerX) * cosAngle - (y + height - centerY) * sinAngle + centerX, (x + width - centerX) * sinAngle + (y + height - centerY) * cosAngle + centerY, 1.0f, 0.0f,  // Bottom Right
        (x + width - centerX) * cosAngle - (y - centerY) * sinAngle + centerX, (x + width - centerX) * sinAngle + (y - centerY) * cosAngle + centerY, 1.0f, 1.0f,  // Top Right
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
    GLuint verts = sizeof(vertices);
    GLuint indexs = sizeof(indices);
    RenderShaderOrtho(shader,vertices ,verts , indices, indexs, 6);
}

void Zelda(Shader shader, GLfloat angle, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat x3, GLfloat y3) {
    // Calculate angle
    GLfloat angleInDegrees = camera.angle + angle;
    GLfloat angleInRadians = angleInDegrees * (M_PI / 180.0f);
    GLfloat cosAngle = cos(angleInRadians);
    GLfloat sinAngle = sin(angleInRadians);
    //Camera Movement
    x1 += camera.x;
    y1 += camera.y;
    x2 += camera.x;
    y2 += camera.y;
    x3 += camera.x;
    y3 += camera.y;
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
        // Outer Triangle      // Texture Coords 
            rotatedX3, rotatedY3,  0.5f, 1.0f,  // 0 Bottom Left
            rotatedX2, rotatedY2,  1.0f, 0.0f,  // 1 Bottom right
            rotatedX1, rotatedY1,  0.0f, 0.0f,  // 2 Top middle
        // Inner Inverted Triangle
            midX31, midY31,        0.0f, 1.0f,  // 3 Top Left
            midX12, midY12,        0.5f, 0.0f,  // 4 Top right
            midX23, midY23,        1.0f, 1.0f,  // 5 Bottom middle
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
    GLuint verts = sizeof(vertices);
    GLuint indexs = sizeof(indices);
    RenderShaderOrtho(shader,vertices ,verts ,indices ,indexs, 9);
}

