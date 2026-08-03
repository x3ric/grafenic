#include "shader/init.c"
#include "color.c"
#include "cache.c"

static inline Color DrawColor(Color color) {
    if (color.a == 0) color.a = 255;
    return color;
}

static inline bool SameColor(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static void SetQuadVertices(GLfloat* vertices, float x, float y, float width, float height) {
    vertices[0] = x;
    vertices[1] = y + height;
    vertices[2] = 0.0f;
    vertices[3] = 0.0f;
    vertices[4] = 1.0f;
    vertices[5] = x + width;
    vertices[6] = y + height;
    vertices[7] = 0.0f;
    vertices[8] = 1.0f;
    vertices[9] = 1.0f;
    vertices[10] = x;
    vertices[11] = y;
    vertices[12] = 0.0f;
    vertices[13] = 0.0f;
    vertices[14] = 0.0f;
    vertices[15] = x + width;
    vertices[16] = y;
    vertices[17] = 0.0f;
    vertices[18] = 1.0f;
    vertices[19] = 0.0f;
}

void DrawRect(int x, int y, int width, int height, Color color) {
    if (width == 0 || height == 0) return;
    color = DrawColor(color);
    BindTexture(GetCachedTexture(color, true, false, NULL, 0, 0));
    SetBlend(true);
    Rect((RectObject){
        {x, y + height, 0.0f},         // Bottom Left
        {x + width, y + height, 0.0f}, // Bottom Right
        {x, y, 0.0f},                  // Top Left
        {x + width, y, 0.0f},          // Top Right
        shaderdefault,                 // Shader
        camera,                        // Camera
    });
}

#define MAX_BATCH_RECTS 4096
static GLfloat rectBatchVertices[MAX_BATCH_RECTS * 20];
static Color rectBatchColors[MAX_BATCH_RECTS];
static GLuint rectIndices[MAX_BATCH_RECTS * 6];
static int rectBatchCount = 0;
static bool rectIndicesInitialized = false;

static void InitializeRectIndices(void) {
    if (rectIndicesInitialized) return;
    for (int i = 0; i < MAX_BATCH_RECTS; i++) {
        int index = i * 6;
        int vertex = i * 4;
        rectIndices[index + 0] = vertex + 0; // Bottom Left
        rectIndices[index + 1] = vertex + 1; // Bottom Right
        rectIndices[index + 2] = vertex + 2; // Top Left
        rectIndices[index + 3] = vertex + 1; // Bottom Right
        rectIndices[index + 4] = vertex + 3; // Top Right
        rectIndices[index + 5] = vertex + 2; // Top Left
    }
    rectIndicesInitialized = true;
}

void DrawRectBatch(int x, int y, int width, int height, Color color) {
    if (width == 0 || height == 0) return;
    if (rectBatchCount >= MAX_BATCH_RECTS) FlushRectBatch();
    InitializeRectIndices();
    color = DrawColor(color);
    SetQuadVertices(rectBatchVertices + rectBatchCount * 20, (float)x, (float)y, (float)width, (float)height);
    rectBatchColors[rectBatchCount++] = color;
}

void FlushRectBatch() {
    if (rectBatchCount == 0) return;
    SetBlend(true);
    int startRect = 0;
    Color currentColor = rectBatchColors[0];
    for (int i = 1; i <= rectBatchCount; i++) {
        if (i < rectBatchCount && SameColor(rectBatchColors[i], currentColor)) continue;
        int count = i - startRect;
        BindTexture(GetCachedTexture(currentColor, true, false, NULL, 0, 0));
        RenderShader((ShaderObject){
            camera,
            shaderdefault,
            rectBatchVertices + startRect * 20,
            rectIndices,
            (size_t)count * 20 * sizeof(GLfloat),
            (size_t)count * 6 * sizeof(GLuint),
            camera.transform,
            false
        });
        if (i < rectBatchCount) {
            startRect = i;
            currentColor = rectBatchColors[i];
        }
    }
    rectBatchCount = 0;
}

void DrawRectBorder(int x, int y, int width, int height, int thickness, Color color) {
    if (width <= 0 || height <= 0 || thickness <= 0) return;
    thickness = MinInt(thickness, MinInt(width, height));
    DrawRect(x, y, width, thickness, color);
    DrawRect(x, y + height - thickness, width, thickness, color);
    int middle = height - thickness * 2;
    if (middle > 0) {
        DrawRect(x, y + thickness, thickness, middle, color);
        DrawRect(x + width - thickness, y + thickness, thickness, middle, color);
    }
}

void DrawLine(float x0, float y0, float x1, float y1, int thickness, Color color) {
    if (thickness <= 0) return;
    color = DrawColor(color);
    float dx = x1 - x0;
    float dy = y1 - y0;
    float lengthSquared = dx * dx + dy * dy;
    if (lengthSquared <= 0.000001f) {
        DrawRect((int)(x0 - thickness * 0.5f), (int)(y0 - thickness * 0.5f), thickness, thickness, color);
        return;
    }
    float scale = thickness * 0.5f / sqrtf(lengthSquared);
    float offsetX = -dy * scale;
    float offsetY = dx * scale;
    BindTexture(GetCachedTexture(color, true, false, NULL, 0, 0));
    SetBlend(true);
    Rect((RectObject){
        {x0 - offsetX, y0 - offsetY, 0.0f}, // Bottom Left
        {x0 + offsetX, y0 + offsetY, 0.0f}, // Bottom Right
        {x1 - offsetX, y1 - offsetY, 0.0f}, // Top Left
        {x1 + offsetX, y1 + offsetY, 0.0f}, // Top Right
        shaderdefault,                      // Shader
        camera,                             // Camera
    });
}

typedef struct {
    GLuint texture;
    Color color;
    int radius;
    int thickness;
    unsigned long lastUsed;
    bool used;
} CircleTexture;

#define CIRCLE_CACHE_SIZE 128
static CircleTexture circleCache[CIRCLE_CACHE_SIZE] = {0};
static unsigned long circleAccessCounter = 1;

static unsigned int CircleHash(int radius, int thickness, Color color) {
    unsigned int hash = 2166136261u;
    hash = (hash ^ (unsigned int)radius) * 16777619u;
    hash = (hash ^ (unsigned int)thickness) * 16777619u;
    hash = (hash ^ color.r) * 16777619u;
    hash = (hash ^ color.g) * 16777619u;
    hash = (hash ^ color.b) * 16777619u;
    hash = (hash ^ color.a) * 16777619u;
    return hash;
}

static GLuint CreateCircleTexture(int radius, int thickness, Color color) {
    int outerRadius = radius + thickness;
    int diameter = outerRadius * 2;
    size_t size = (size_t)diameter * (size_t)diameter * 4;
    unsigned char* pixels = calloc(size, 1);
    if (!pixels) return 0;
    int innerSquared = radius * radius;
    int outerSquared = outerRadius * outerRadius;
    for (int y = 0; y < diameter; y++) {
        int dy = y - outerRadius;
        for (int x = 0; x < diameter; x++) {
            int dx = x - outerRadius;
            int distanceSquared = dx * dx + dy * dy;
            bool visible = thickness == 0 ? distanceSquared < innerSquared : distanceSquared < outerSquared && distanceSquared >= innerSquared;
            if (!visible) continue;
            size_t index = ((size_t)y * diameter + x) * 4;
            pixels[index + 0] = color.r;
            pixels[index + 1] = color.g;
            pixels[index + 2] = color.b;
            pixels[index + 3] = color.a;
        }
    }
    GLuint texture = CreateTextureFromBitmap(pixels, diameter, diameter, true);
    free(pixels);
    return texture;
}

static GLuint GetCircleTexture(int radius, int thickness, Color color) {
    unsigned int start = CircleHash(radius, thickness, color) % CIRCLE_CACHE_SIZE;
    int empty = -1;
    for (int i = 0; i < CIRCLE_CACHE_SIZE; i++) {
        int index = (start + i) % CIRCLE_CACHE_SIZE;
        CircleTexture* entry = &circleCache[index];
        if (!entry->used) {
            empty = index;
            break;
        }
        if (entry->radius == radius && entry->thickness == thickness && SameColor(entry->color, color)) {
            entry->lastUsed = circleAccessCounter++;
            return entry->texture;
        }
    }
    int index = empty;
    if (index < 0) {
        unsigned long oldest = ULONG_MAX;
        for (int i = 0; i < CIRCLE_CACHE_SIZE; i++) {
            if (circleCache[i].lastUsed < oldest) {
                oldest = circleCache[i].lastUsed;
                index = i;
            }
        }
        if (renderTexture == circleCache[index].texture) UnbindTexture();
        glDeleteTextures(1, &circleCache[index].texture);
    }
    GLuint texture = CreateCircleTexture(radius, thickness, color);
    if (!texture) return 0;
    circleCache[index] = (CircleTexture){texture, color, radius, thickness, circleAccessCounter++, true};
    return texture;
}

void CleanUpCircleCache(void) {
    for (int i = 0; i < CIRCLE_CACHE_SIZE; i++) {
        if (!circleCache[i].used || !circleCache[i].texture) continue;
        if (renderTexture == circleCache[i].texture) UnbindTexture();
        glDeleteTextures(1, &circleCache[i].texture);
    }
    memset(circleCache, 0, sizeof(circleCache));
    circleAccessCounter = 1;
}

void DrawCircle(int x, int y, int r, Color color) {
    if (r <= 0) return;
    color = DrawColor(color);
    GLuint textureID = GetCircleTexture(r, 0, color);
    if (!textureID) return;
    BindTexture(textureID);
    SetBlend(true);
    Rect((RectObject){
        {x - r, y - r, 0.0f}, // Bottom Left
        {x + r, y - r, 0.0f}, // Bottom Right
        {x - r, y + r, 0.0f}, // Top Left
        {x + r, y + r, 0.0f}, // Top Right
        shaderdefault,         // Shader
        camera,                // Camera
    });
}

void DrawCircleBorder(int x, int y, int r, int thickness, Color color) {
    if (r <= 0 || thickness <= 0) return;
    color = DrawColor(color);
    GLuint textureID = GetCircleTexture(r, thickness, color);
    if (!textureID) return;
    int outer = r + thickness;
    BindTexture(textureID);
    SetBlend(true);
    Rect((RectObject){
        {x - outer, y - outer, 0.0f}, // Bottom Left
        {x + outer, y - outer, 0.0f}, // Bottom Right
        {x - outer, y + outer, 0.0f}, // Top Left
        {x + outer, y + outer, 0.0f}, // Top Right
        shaderdefault,                // Shader
        camera,                       // Camera
    });
}

void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    color = DrawColor(color);
    BindTexture(GetCachedTexture(color, true, false, NULL, 0, 0));
    SetBlend(true);
    Triangle((TriangleObject){
        {x1, y1, 0.0f}, // Vert0: x, y, z
        {x2, y2, 0.0f}, // Vert1: x, y, z
        {x3, y3, 0.0f}, // Vert2: x, y, z
        shaderdefault,  // Shader
        camera,         // Camera
    });
}

void DrawTriangleBorder(int x1, int y1, int x2, int y2, int x3, int y3, int thickness, Color color) {
    DrawLine(x1, y1, x2, y2, thickness, color);
    DrawLine(x3, y3, x1, y1, thickness, color);
    DrawLine(x2, y2, x3, y3, thickness, color);
}

void DrawCube(GLfloat size, GLfloat x, GLfloat y, GLfloat z, GLfloat rotx, GLfloat roty, GLfloat rotz, Color color) {
    if (size <= 0.0f) return;
    color = DrawColor(color);
    BindTexture(GetCachedTexture(color, true, false, NULL, 0, 0));
    SetBlend(true);
    Cube((CubeObject){
        .transform = {
            .position = {x, y, z},              // Position: x, y, z
            .localposition = {0.0f, 0.0f, 0.0f}, // LocalPosition: x, y, z
            .rotation = {rotx, roty, rotz}       // Rotation: x, y, z
        },
        .size = size,             // Size
        .shader = shaderdefault,  // Shader
        .cam = camera             // Camera
    });
}

#include "image.c"
#include "font.c"
