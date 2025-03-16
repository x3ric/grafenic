
static CachedTexture textureCache[TEXTURE_CACHE_SIZE] = {0};
static unsigned long textureAccessCounter = 0;
static inline int min_val(int a, int b) {
    return (a < b) ? a : b;
}

GLuint CreateTextureFromBitmap(const unsigned char* bitmapData, int width, int height, bool linear) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData);
    glTexOpt(linear ? GL_LINEAR : GL_NEAREST, GL_CLAMP_TO_EDGE);
    return textureID;
}

GLuint CreateTextureFromColor(Color color, bool linear) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char pixels[] = { color.r, color.g, color.b, color.a };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexOpt(linear ? GL_LINEAR : GL_NEAREST, GL_CLAMP_TO_EDGE);
    return textureID;
}

GLuint GetCachedTexture(Color color, bool linear, bool isBitmap, const unsigned char* bitmapData, int width, int height) {
    unsigned int hash = 5381;
    hash = ((hash << 5) + hash) + color.r;
    hash = ((hash << 5) + hash) + color.g;
    hash = ((hash << 5) + hash) + color.b;
    hash = ((hash << 5) + hash) + color.a;
    hash = ((hash << 5) + hash) + (linear ? 1 : 0);
    hash = ((hash << 5) + hash) + (isBitmap ? 1 : 0);
    hash = ((hash << 5) + hash) + width;
    hash = ((hash << 5) + hash) + height;
    if (isBitmap && bitmapData) {
        int sampleSize = min_val(32, width * height);
        for (int i = 0; i < sampleSize; i++) {
            hash = ((hash << 5) + hash) + bitmapData[i];
        }
    }
    hash = hash % TEXTURE_CACHE_SIZE;
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        int index = (hash + i) % TEXTURE_CACHE_SIZE;
        if (!textureCache[index].used) {
            GLuint textureID;
            if (isBitmap) {
                textureID = CreateTextureFromBitmap(bitmapData, width, height, linear);
            } else {
                textureID = CreateTextureFromColor(color, linear);
            }
            textureCache[index].texture = textureID;
            textureCache[index].color = color;
            textureCache[index].width = width;
            textureCache[index].height = height;
            textureCache[index].linear = linear;
            textureCache[index].isBitmap = isBitmap;
            textureCache[index].used = true;
            textureCache[index].lastUsed = textureAccessCounter++;
            return textureID;
        }
        if (textureCache[index].used &&
            textureCache[index].isBitmap == isBitmap &&
            textureCache[index].width == width &&
            textureCache[index].height == height &&
            textureCache[index].linear == linear &&
            (!isBitmap || isBitmap) &&
            (!isBitmap && textureCache[index].color.r == color.r &&
             textureCache[index].color.g == color.g &&
             textureCache[index].color.b == color.b &&
             textureCache[index].color.a == color.a)) {
            textureCache[index].lastUsed = textureAccessCounter++;
            return textureCache[index].texture;
        }
    }
    int lruIndex = 0;
    unsigned long oldestAccess = ULONG_MAX;
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (textureCache[i].used && textureCache[i].lastUsed < oldestAccess) {
            oldestAccess = textureCache[i].lastUsed;
            lruIndex = i;
        }
    }
    if (textureCache[lruIndex].used) {
        glDeleteTextures(1, &textureCache[lruIndex].texture);
    }
    GLuint textureID;
    if (isBitmap) {
        textureID = CreateTextureFromBitmap(bitmapData, width, height, linear);
    } else {
        textureID = CreateTextureFromColor(color, linear);
    }
    textureCache[lruIndex].texture = textureID;
    textureCache[lruIndex].color = color;
    textureCache[lruIndex].width = width;
    textureCache[lruIndex].height = height;
    textureCache[lruIndex].linear = linear;
    textureCache[lruIndex].isBitmap = isBitmap;
    textureCache[lruIndex].used = true;
    textureCache[lruIndex].lastUsed = textureAccessCounter++;
    return textureID;
}

void CleanUpTextureCache() {
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (textureCache[i].used) {
            glDeleteTextures(1, &textureCache[i].texture);
            textureCache[i].used = false;
        }
    }
    memset(textureCache, 0, sizeof(textureCache));
    textureAccessCounter = 0;
}
