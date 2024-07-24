
void DrawRect(int x, int y, int width, int height, Color color) {
    if (color.a == 0) color.a = 255;
    static GLuint textureID;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char pixels[] = { color.r, color.g, color.b, color.a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, textureID);
    Rect(x, y, width, height, 0.0f, shaderdefault);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &textureID);
}

void DrawRectBorder(int x, int y, int width, int height, int thickness, Color color) {
    if (color.a == 0) color.a = 255;
    DrawRect(x, y, width, thickness, color);
    DrawRect(x, y + height - thickness, width, thickness, color);
    DrawRect(x, y, thickness, height, color);
    DrawRect(x + width - thickness, y, thickness, height, color);
}

void DrawLine(float x0, float y0, float x1, float y1, int thickness, Color color) {
    if (color.a == 0) color.a = 255;
    static GLuint textureID;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char pixels[] = { color.r, color.g, color.b, color.a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    Line(x0, y0, x1, y1, thickness, shaderdefault);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &textureID);
}

void DrawCircle(int x, int y, int r, Color color) {
    if (color.a == 0) color.a = 255;
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char pixels[r * 2][r * 2][4];
    for (int i = 0; i < r * 2; i++) {
        for (int j = 0; j < r * 2; j++) {
            float dx = i - r;
            float dy = j - r;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < r) {
                pixels[i][j][0] = color.r;
                pixels[i][j][1] = color.g;
                pixels[i][j][2] = color.b;
                pixels[i][j][3] = color.a;
            } else {
                pixels[i][j][0] = 0;
                pixels[i][j][1] = 0;
                pixels[i][j][2] = 0;
                pixels[i][j][3] = 0;
            }
        }
    }
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, r * 2, r * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    Rect(x - r, y - r, r * 2, r * 2, 0.0f, shaderdefault);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &textureID);
}

void DrawCircleBorder(int x, int y, int r, int thickness, Color color) {
    if (color.a == 0) color.a = 255;
    static GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    int diameter = r * 2 + thickness * 2;
    unsigned char pixels[diameter][diameter][4];
    for (int i = 0; i < diameter; i++) {
        for (int j = 0; j < diameter; j++) {
            float dx = i - r - thickness;
            float dy = j - r - thickness;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < r + thickness && distance >= r) {
                pixels[i][j][0] = color.r;
                pixels[i][j][1] = color.g;
                pixels[i][j][2] = color.b;
                pixels[i][j][3] = color.a;
            } else {
                pixels[i][j][0] = 0;
                pixels[i][j][1] = 0;
                pixels[i][j][2] = 0;
                pixels[i][j][3] = 0;
            }
        }
    }
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, diameter, diameter, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    Rect(x - r - thickness, y - r - thickness, diameter, diameter, 0.0f, shaderdefault);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &textureID);
}

void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    if (color.a == 0) color.a = 255;
    static GLuint textureID;
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char pixels[] = { color.r, color.g, color.b, color.a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, textureID);
    Triangle(x1,y1,x2,y2,x3,y3,0.0f,shaderdefault);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &textureID);
}

void DrawTriangleBorder(int x1, int y1, int x2, int y2, int x3, int y3, int thickness, Color color) {
    DrawLine(x1, y1, x2, y2, thickness, color);
    DrawLine(x3, y3, x1, y1, thickness, color);
    DrawLine(x2, y2, x3, y3, thickness, color);
}
