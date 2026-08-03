
static CachedTexture textureCache[TEXTURE_CACHE_SIZE] = {0};
static unsigned long textureAccessCounter = 1;

static uint64_t HashBytes(const unsigned char* data, size_t size) {
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static uint64_t TextureKey(Color color, bool linear, bool isBitmap, const unsigned char* bitmapData, int width, int height) {
    uint64_t key = ((uint64_t)color.r << 56) | ((uint64_t)color.g << 48) | ((uint64_t)color.b << 40) | ((uint64_t)color.a << 32);
    key ^= ((uint64_t)(uint32_t)width << 16) ^ (uint32_t)height;
    key ^= linear ? 0x9E3779B97F4A7C15ULL : 0;
    key ^= isBitmap ? 0xD1B54A32D192ED03ULL : 0;
    if (isBitmap && bitmapData && width > 0 && height > 0) {
        key ^= HashBytes(bitmapData, (size_t)width * (size_t)height * 4);
    }
    return key;
}

GLuint CreateTextureFromBitmap(const unsigned char* bitmapData, int width, int height, bool linear) {
    if (!bitmapData || width <= 0 || height <= 0) return 0;
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    BindTexture(textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData);
    glTexOpt(linear ? GL_LINEAR : GL_NEAREST, GL_CLAMP_TO_EDGE);
    return textureID;
}

GLuint CreateTextureFromColor(Color color, bool linear) {
    GLuint textureID = 0;
    const unsigned char pixels[] = {color.r, color.g, color.b, color.a};
    glGenTextures(1, &textureID);
    BindTexture(textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexOpt(linear ? GL_LINEAR : GL_NEAREST, GL_CLAMP_TO_EDGE);
    return textureID;
}

GLuint GetCachedTexture(Color color, bool linear, bool isBitmap, const unsigned char* bitmapData, int width, int height) {
    uint64_t key = TextureKey(color, linear, isBitmap, bitmapData, width, height);
    unsigned int start = (unsigned int)(key ^ (key >> 32)) % TEXTURE_CACHE_SIZE;
    int emptyIndex = -1;
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        int index = (start + i) % TEXTURE_CACHE_SIZE;
        CachedTexture* entry = &textureCache[index];
        if (!entry->used) {
            emptyIndex = index;
            break;
        }
        if (entry->key == key && entry->isBitmap == isBitmap && entry->width == width && entry->height == height && entry->linear == linear) {
            entry->lastUsed = textureAccessCounter++;
            return entry->texture;
        }
    }
    int index = emptyIndex;
    if (index < 0) {
        unsigned long oldestAccess = ULONG_MAX;
        for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
            if (textureCache[i].lastUsed < oldestAccess) {
                oldestAccess = textureCache[i].lastUsed;
                index = i;
            }
        }
        if (textureCache[index].texture) {
            if (renderTexture == textureCache[index].texture) UnbindTexture();
            glDeleteTextures(1, &textureCache[index].texture);
        }
    }
    GLuint textureID = isBitmap ? CreateTextureFromBitmap(bitmapData, width, height, linear) : CreateTextureFromColor(color, linear);
    if (!textureID) return 0;
    textureCache[index] = (CachedTexture){
        .texture = textureID,
        .color = color,
        .width = width,
        .height = height,
        .linear = linear,
        .isBitmap = isBitmap,
        .used = true,
        .lastUsed = textureAccessCounter++,
        .key = key
    };
    return textureID;
}

void CleanUpTextureCache() {
    for (int i = 0; i < TEXTURE_CACHE_SIZE; i++) {
        if (textureCache[i].used && textureCache[i].texture) {
            if (renderTexture == textureCache[i].texture) UnbindTexture();
            glDeleteTextures(1, &textureCache[i].texture);
        }
    }
    memset(textureCache, 0, sizeof(textureCache));
    textureAccessCounter = 1;
}
