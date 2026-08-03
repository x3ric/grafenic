
Img LoadImage(ImgInfo info) {
    Img img = {0};
    if (!info.filename) return img;
    stbi_set_flip_vertically_on_load(true);
    img.data = stbi_load(info.filename, &img.width, &img.height, &img.channels, STBI_rgb_alpha);
    if (!img.data) return img;
    glGenTextures(1, &img.raw);
    BindTexture(img.raw);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
    glTexOpt(info.nearest ? GL_NEAREST : GL_LINEAR, GL_CLAMP_TO_EDGE);
    UnbindTexture();
    stbi_image_free(img.data);
    img.data = NULL;
    img.channels = 4;
    return img;
}

void BindImg(Img image) {
    if (!image.raw) return;
    SetBlend(true);
    BindTexture(image.raw);
}

static void DrawImageInternal(Img image, float x, float y, float width, float height, GLfloat angle, Shader shader) {
    if (!image.raw || width == 0.0f || height == 0.0f || !shader.Program) return;
    float cx = x + width * 0.5f;
    float cy = y + height * 0.5f;
    float x0 = x, y0 = y + height;
    float x1 = x + width, y1 = y + height;
    float x2 = x, y2 = y;
    float x3 = x + width, y3 = y;
    if (angle != 0.0f) {
        float c = cosf(angle);
        float s = sinf(angle);
        float px[4] = {x0, x1, x2, x3};
        float py[4] = {y0, y1, y2, y3};
        for (int i = 0; i < 4; i++) {
            float dx = px[i] - cx;
            float dy = py[i] - cy;
            px[i] = cx + dx * c - dy * s;
            py[i] = cy + dx * s + dy * c;
        }
        x0 = px[0]; y0 = py[0];
        x1 = px[1]; y1 = py[1];
        x2 = px[2]; y2 = py[2];
        x3 = px[3]; y3 = py[3];
    }
    GLfloat vertices[] = {
        x0, y0, 0.0f, 0.0f, 0.0f, // Bottom Left
        x1, y1, 0.0f, 1.0f, 0.0f, // Bottom Right
        x2, y2, 0.0f, 0.0f, 1.0f, // Top Left
        x3, y3, 0.0f, 1.0f, 1.0f  // Top Right
    };
    static GLuint indices[] = {0, 1, 2, 1, 3, 2};
    BindImg(image);
    RenderShader((ShaderObject){camera, shader, vertices, indices, sizeof(vertices), sizeof(indices), camera.transform, false});
}

void DrawImage(Img image, float x, float y, float width, float height, GLfloat angle) {
    DrawImageInternal(image, x, y, width, height, angle, shaderdefault);
}

void DrawImageShader(Img image, float x, float y, float width, float height, GLfloat angle, Shader shader) {
    DrawImageInternal(image, x, y, width, height, angle, shader);
}

void SaveScreenshot(const char* filename, int x, int y, int width, int height) {
    if (!filename || width <= 0 || height <= 0) return;
    printf("Saving screenshot to -> %s\n", filename);
    size_t rowSize = (size_t)width * 4;
    size_t imageSize = rowSize * (size_t)height;
    unsigned char* pixels = malloc(imageSize);
    unsigned char* row = malloc(rowSize);
    if (!pixels || !row) {
        free(pixels);
        free(row);
        return;
    }
    int adjustedY = window.screen_height - y - height;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, adjustedY, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    for (int top = 0, bottom = height - 1; top < bottom; top++, bottom--) {
        unsigned char* topRow = pixels + (size_t)top * rowSize;
        unsigned char* bottomRow = pixels + (size_t)bottom * rowSize;
        memcpy(row, topRow, rowSize);
        memcpy(topRow, bottomRow, rowSize);
        memcpy(bottomRow, row, rowSize);
    }
    stbi_write_jpg(filename, width, height, 4, pixels, 90);
    free(row);
    free(pixels);
}
