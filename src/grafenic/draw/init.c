#include "color.c"
#include "shader.c"
#include "camera.c"

void DrawPixels(float x, float y, float width, float height) { 
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//alpha blending
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    CreateTextureWithPBO(framebuffer,framebufferTexture);
    Quad(x, y, width, height,0,pixelshaderdefault);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

void DrawPixel(int x, int y, Color color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    if (color.a == 0) { color.a = 255; }
    int index = (y * SCREEN_WIDTH + x) * 4;
    float alpha = color.a / 255.0f;
    float invAlpha = 1.0f - alpha;
    pixels = true;
    if(framebuffer){
        framebuffer[index]     = (GLubyte)(color.r * alpha + framebuffer[index]     * invAlpha);
        framebuffer[index + 1] = (GLubyte)(color.g * alpha + framebuffer[index + 1] * invAlpha);
        framebuffer[index + 2] = (GLubyte)(color.b * alpha + framebuffer[index + 2] * invAlpha);
        framebuffer[index + 3] = color.a;
    }
}

Color GetPixel(int x, int y) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return (Color){0, 0, 0, 255};
    }
    GLubyte pixelData[4];
    glReadPixels(x, SCREEN_HEIGHT - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    Color color = {pixelData[0], pixelData[1], pixelData[2], pixelData[3]};
    return color;
}

bool IsInside(int x, int y,int rectX, int rectY, int rectWidth, int rectHeight) {
    rectY = SCREEN_HEIGHT - rectHeight;
    return x >= rectX && x < rectX + rectWidth && y >= rectY && y < rectY + rectHeight;
}

void DrawRect(int x, int y, int width, int height, Color color) {
    if (color.a == 0) { color.a = 255; }
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + height; j++) {
            DrawPixel(i, j, (Color){color.r, color.g, color.b, color.a});
        }
    }
}

void DrawRectBorder(int x, int y, int width, int height, int thickness, Color color) {
    if (color.a == 0) color.a = 255;
    for (int i = x; i < x + width; i++) {
        for (int t = 0; t < thickness; t++) {
            DrawPixel(i, y + t, color);
            DrawPixel(i, y + height - 1 - t, color);
        }
    }
    for (int j = y + thickness; j < y + height - thickness; j++) {
        for (int t = 0; t < thickness; t++) {
            DrawPixel(x + t, j, color);
            DrawPixel(x + width - 1 - t, j, color);
        }
    }
}

void DrawLine(int x0, int y0, int x1, int y1, int thickness, Color color) {
    if (color.a == 0) color.a = 255;
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    for (int offset = 0; offset < thickness; ++offset) {
        int x = x0, y = y0, currErr = err;
        while (x != x1 || y != y1) {
            DrawPixel(x, y, color);
            int e2 = 2 * currErr;
            if (e2 > -dy) { currErr -= dy; x += sx; }
            if (e2 < dx) { currErr += dx; y += sy; }
        }
        DrawPixel(x, y, color);
        if (dx > dy) {
            y0 += sx;
            y1 += sx;
        } else {
            x0 += sy;
            x1 += sy;
        }
    }
}

void DrawCircle(int x, int y, int r, Color color) {
    if (color.a == 0) { color.a = 255; }
    int h = x;
    int k = SCREEN_HEIGHT - y;
    for (int i = h - r; i <= h + r; i++) {
        for (int j = k - r; j <= k + r; j++) {
            if ((i - h) * (i - h) + (j - k) * (j - k) <= r * r) {
                DrawPixel(i, j, (Color){color.r, color.g, color.b, color.a});
            }
        }
    }
}

void DrawCircleBorder(int x, int y, int r, int thickness, Color color) {
    if (color.a == 0) { color.a = 255; }
    int h = x;
    int k = SCREEN_HEIGHT - y;
    for (int i = h - r - thickness; i <= h + r + thickness; i++) {
        for (int j = k - r - thickness; j <= k + r + thickness; j++) {
            int distanceSquared = (i - h) * (i - h) + (j - k) * (j - k);
            if (distanceSquared <= (r + thickness) * (r + thickness) && distanceSquared >= (r - thickness) * (r - thickness)) {
                DrawPixel(i, j, (Color){color.r, color.g, color.b, color.a});
            }
        }
    }
}

void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    if (color.a == 0) { color.a = 255; }
    int minX = (x1 < x2) ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
    int minY = (y1 < y2) ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
    int maxX = (x1 > x2) ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
    int maxY = (y1 > y2) ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);
    float triangleArea = (x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2)) / 2.0f;
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float b1 = (x*(y2-y3) + x2*(y3-y) + x3*(y-y2)) / (2.0f*triangleArea);
            float b2 = (x*(y3-y1) + x1*(y-y3) + x3*(y1-y)) / (2.0f*triangleArea);
            float b3 = (x*(y1-y2) + x1*(y2-y) + x2*(y-y1)) / (2.0f*triangleArea);
            if (b1 >= 0 && b2 >= 0 && b3 >= 0) {
                DrawPixel(x, y, (Color){color.r, color.g, color.b, color.a});
            }
        }
    }
}

void DrawTriangleBorder(int x1, int y1, int x2, int y2, int x3, int y3, int thickness, Color color) {
    DrawLine(x1, y1, x2, y2, thickness, color);
    DrawLine(x3, y3, x1, y1, thickness, color);
    DrawLine(x2, y2, x3, y3, thickness, color);
}

#include "image.c"
#include "text.c"   
