
CodepointMap codepointMap[CODEPOINT_MAP_SIZE] = {0};

static FT_Face currentPixelFace = NULL;
static float currentPixelSize = 0.0f;
static GLuint fontTextures[FONT_CACHE_SIZE * 2] = {0};
static int fontTextureCount = 0;
static FT_Face loadedFaces[FONT_CACHE_SIZE] = {0};
static FT_Library loadedLibraries[FONT_CACHE_SIZE] = {0};
static int loadedFontCount = 0;

static bool SetFontPixelSize(FT_Face face, float size) {
    if (!face) return false;
    if (currentPixelFace == face && fabsf(currentPixelSize - size) < 0.01f) return true;
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)size) != 0) return false;
    currentPixelFace = face;
    currentPixelSize = size;
    return true;
}

static void RegisterFontTexture(GLuint texture) {
    if (!texture) return;
    for (int i = 0; i < fontTextureCount; i++) {
        if (fontTextures[i] == texture) return;
    }
    if (fontTextureCount < (int)(sizeof(fontTextures) / sizeof(fontTextures[0]))) {
        fontTextures[fontTextureCount++] = texture;
    }
}

static void UnregisterFontTexture(GLuint texture) {
    for (int i = 0; i < fontTextureCount; i++) {
        if (fontTextures[i] != texture) continue;
        fontTextures[i] = fontTextures[--fontTextureCount];
        return;
    }
}

static void RegisterLoadedFont(FT_Library library, FT_Face face) {
    for (int i = 0; i < loadedFontCount; i++) {
        if (loadedFaces[i] == face) return;
    }
    if (loadedFontCount < FONT_CACHE_SIZE) {
        loadedLibraries[loadedFontCount] = library;
        loadedFaces[loadedFontCount] = face;
        loadedFontCount++;
    }
}

int GetGlyphIndex(uint32_t codepoint) {
    if (codepoint >= 32 && codepoint < 32 + MAX_GLYPHS) return (int)codepoint - 32;
    unsigned int hash = codepoint % CODEPOINT_MAP_SIZE;
    for (int i = 0; i < CODEPOINT_MAP_SIZE; i++) {
        unsigned int index = (hash + i) % CODEPOINT_MAP_SIZE;
        if (codepointMap[index].used && codepointMap[index].codepoint == codepoint) {
            return codepointMap[index].glyphIndex;
        }
        if (!codepointMap[index].used) break;
    }
    return 0;
}

int CalculateAtlasSize(int numGlyphs, float fontSize, int oversampling) {
    if (oversampling < 1) oversampling = 1;
    float estimatedAreaPerGlyph = fontSize * fontSize * 0.55f * oversampling * oversampling;
    int atlasDimension = (int)ceilf(sqrtf(estimatedAreaPerGlyph * numGlyphs * 1.25f));
    int powerOfTwo = 256;
    while (powerOfTwo < atlasDimension) powerOfTwo *= 2;
    return powerOfTwo;
}

Font GenAtlas(Font font) {
    if (font.fontSize <= 1.0f) font.fontSize = ATLAS_FONT_SIZE;
    if (font.oversampling < 1) font.oversampling = 1;
    if (!font.face || !SetFontPixelSize(font.face, font.fontSize)) return font;
    if (font.subpixel) FT_Library_SetLcdFilter(font.library, FT_LCD_FILTER_DEFAULT);
    stbrp_rect rects[MAX_GLYPHS] = {0};
    int maxDimension = 1;
    FT_Int32 loadFlags = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT;
    if (font.subpixel) loadFlags |= FT_LOAD_TARGET_LCD;
    for (int i = 0; i < MAX_GLYPHS; i++) {
        int codepoint = 32 + i;
        rects[i].id = i;
        if (FT_Load_Char(font.face, codepoint, loadFlags) != 0) {
            rects[i].w = 1;
            rects[i].h = 1;
            continue;
        }
        FT_Bitmap* bitmap = &font.face->glyph->bitmap;
        int width = font.subpixel ? (int)bitmap->width / 3 : (int)bitmap->width;
        int height = (int)bitmap->rows;
        rects[i].w = (stbrp_coord)MaxInt(width + 2, 1);
        rects[i].h = (stbrp_coord)MaxInt(height + 2, 1);
        maxDimension = MaxInt(maxDimension, MaxInt(rects[i].w, rects[i].h));
    }
    int atlasSize = CalculateAtlasSize(MAX_GLYPHS, font.fontSize, font.oversampling);
    while (atlasSize < maxDimension) atlasSize *= 2;
    bool packed = false;
    while (!packed && atlasSize <= 8192) {
        stbrp_node* nodes = malloc((size_t)atlasSize * sizeof(stbrp_node));
        if (!nodes) return font;
        stbrp_context context;
        stbrp_init_target(&context, atlasSize, atlasSize, nodes, atlasSize);
        for (int i = 0; i < MAX_GLYPHS; i++) {
            rects[i].x = 0;
            rects[i].y = 0;
            rects[i].was_packed = 0;
        }
        packed = stbrp_pack_rects(&context, rects, MAX_GLYPHS) != 0;
        free(nodes);
        if (!packed) atlasSize *= 2;
    }
    if (!packed) return font;
    font.atlasWidth = atlasSize;
    font.atlasHeight = atlasSize;
    size_t atlasBytes = (size_t)atlasSize * (size_t)atlasSize * 4;
    font.atlasData = calloc(atlasBytes, 1);
    if (!font.atlasData) return font;
    for (int i = 0; i < MAX_GLYPHS; i++) {
        int codepoint = 32 + i;
        Glyph* glyph = &font.glyphs[i];
        int x = rects[i].x + 1;
        int y = rects[i].y + 1;
        if (FT_Load_Char(font.face, codepoint, loadFlags) != 0) {
            *glyph = (Glyph){x, y, x, y, 0, 0, font.fontSize / 3.0f,
                (float)x / atlasSize, (float)y / atlasSize,
                (float)x / atlasSize, (float)y / atlasSize};
            continue;
        }
        FT_GlyphSlot slot = font.face->glyph;
        FT_Bitmap* bitmap = &slot->bitmap;
        int glyphWidth = font.subpixel ? (int)bitmap->width / 3 : (int)bitmap->width;
        int glyphHeight = (int)bitmap->rows;
        for (int row = 0; row < glyphHeight; row++) {
            const unsigned char* src = bitmap->pitch >= 0 ? bitmap->buffer + row * bitmap->pitch : bitmap->buffer + (glyphHeight - 1 - row) * -bitmap->pitch;
            unsigned char* dst = font.atlasData + ((size_t)(y + row) * atlasSize + x) * 4;
            if (font.subpixel) {
                for (int col = 0; col < glyphWidth; col++) {
                    int srcIndex = col * 3;
                    int alpha = (src[srcIndex] + src[srcIndex + 1] + src[srcIndex + 2]) / 3;
                    dst[col * 4 + 0] = 255;
                    dst[col * 4 + 1] = 255;
                    dst[col * 4 + 2] = 255;
                    dst[col * 4 + 3] = (unsigned char)alpha;
                }
            } else {
                for (int col = 0; col < glyphWidth; col++) {
                    dst[col * 4 + 0] = 255;
                    dst[col * 4 + 1] = 255;
                    dst[col * 4 + 2] = 255;
                    dst[col * 4 + 3] = src[col];
                }
            }
        }
        glyph->x0 = x;
        glyph->y0 = y;
        glyph->x1 = x + glyphWidth;
        glyph->y1 = y + glyphHeight;
        glyph->xoff = slot->bitmap_left;
        glyph->yoff = slot->bitmap_top;
        glyph->xadvance = slot->advance.x >> 6;
        glyph->u0 = (float)glyph->x0 / atlasSize;
        glyph->v0 = (float)glyph->y0 / atlasSize;
        glyph->u1 = (float)glyph->x1 / atlasSize;
        glyph->v1 = (float)glyph->y1 / atlasSize;
    }
    glGenTextures(1, &font.textureID);
    BindTexture(font.textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasSize, atlasSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, font.atlasData);
    glTexOpt(font.nearest ? GL_NEAREST : GL_LINEAR, GL_CLAMP_TO_EDGE);
    RegisterFontTexture(font.textureID);
    free(font.atlasData);
    font.atlasData = NULL;
    font.glyphCount = MAX_GLYPHS;
    return font;
}

bool FindSpaceInAtlas(Font* font, int width, int height, int* x, int* y) {
    if (!font || !x || !y || width <= 0 || height <= 0 || font->glyphCount >= MAX_GLYPHS) return false;
    return false;
}

int AddGlyphToAtlas(Font* font, uint32_t codepoint) {
    if (!font || !font->face) return 0;
    if (codepoint >= 32 && codepoint < 32 + MAX_GLYPHS) return (int)codepoint - 32;
    return 0;
}

Font LoadFont(const char* fontPath) {
    Font font = {0};
    if (!fontPath) return font;
    if (FT_Init_FreeType(&font.library) != 0) return font;
    FT_Error error = FT_New_Face(font.library, fontPath, 0, &font.face);
    if (error) {
        FT_Done_FreeType(font.library);
        return (Font){0};
    }
    RegisterLoadedFont(font.library, font.face);
    font.fontSize = ATLAS_FONT_SIZE;
    font.oversampling = 1;
    return GenAtlas(font);
}

FontCacheEntry fontCacheTable[FONT_CACHE_SIZE] = {0};
unsigned long fontCacheAccessCounter = 1;

Font SetFontSize(Font font, float fontSize) {
    if (!font.face) return font;
    if (fontSize <= 1.0f) fontSize = ATLAS_FONT_SIZE;
    if (font.textureID && fabsf(font.fontSize - fontSize) < 0.01f) return font;
    uintptr_t faceKey = (uintptr_t)font.face;
    unsigned int hash = (unsigned int)(faceKey ^ (faceKey >> 16) ^ (unsigned int)(fontSize * 100.0f)) % FONT_CACHE_SIZE;
    int empty = -1;
    for (int i = 0; i < FONT_CACHE_SIZE; i++) {
        int index = (hash + i) % FONT_CACHE_SIZE;
        FontCacheEntry* entry = &fontCacheTable[index];
        if (!entry->used) {
            empty = index;
            break;
        }
        if (entry->face == font.face && entry->nearest == font.nearest && entry->subpixel == font.subpixel && fabsf(entry->fontSize - fontSize) < 0.01f) {
            entry->lastUsed = fontCacheAccessCounter++;
            return entry->font;
        }
    }
    int index = empty;
    if (index < 0) {
        unsigned long oldest = ULONG_MAX;
        for (int i = 0; i < FONT_CACHE_SIZE; i++) {
            if (fontCacheTable[i].lastUsed < oldest) {
                oldest = fontCacheTable[i].lastUsed;
                index = i;
            }
        }
        GLuint texture = fontCacheTable[index].font.textureID;
        if (texture) {
            if (renderTexture == texture) UnbindTexture();
            glDeleteTextures(1, &texture);
            UnregisterFontTexture(texture);
        }
    }
    font.fontSize = fontSize;
    font.textureID = 0;
    Font generated = GenAtlas(font);
    if (!generated.textureID) return font;
    fontCacheTable[index] = (FontCacheEntry){
        .fontSize = fontSize,
        .face = font.face,
        .nearest = font.nearest,
        .subpixel = font.subpixel,
        .font = generated,
        .used = true,
        .lastUsed = fontCacheAccessCounter++
    };
    return generated;
}

void PreloadFontSizes(Font font) {
    const float commonSizes[] = {12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 24.0f};
    for (size_t i = 0; i < sizeof(commonSizes) / sizeof(commonSizes[0]); i++) {
        SetFontSize(font, commonSizes[i]);
    }
}

CharSizeCache charSizeCache[CHAR_CACHE_SIZE] = {0};
StringSizeCache stringSizeCache[STRING_CACHE_SIZE] = {0};
unsigned long stringSizeAccessCounter = 1;

unsigned int HashTextString(const char* str, float fontSize) {
    unsigned int hash = 2166136261u;
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 16777619u;
    }
    hash ^= (unsigned int)(fontSize * 100.0f);
    return hash % STRING_CACHE_SIZE;
}

TextSize GetTextSize(Font font, float fontSize, const char* text) {
    if (!font.face) return (TextSize){0, 0};
    if (fontSize <= 1.0f) fontSize = 1.0f;
    if (!text || !text[0]) return (TextSize){0, (int)(fontSize * 1.2f)};
    size_t textLen = strlen(text);
    unsigned int hash = HashTextString(text, fontSize) ^ (unsigned int)(uintptr_t)font.face;
    hash %= STRING_CACHE_SIZE;
    if (textLen < MAX_CACHED_STRING_LEN) {
        for (int i = 0; i < STRING_CACHE_SIZE; i++) {
            int index = (hash + i) % STRING_CACHE_SIZE;
            StringSizeCache* entry = &stringSizeCache[index];
            if (!entry->valid) break;
            if (entry->face == font.face && fabsf(entry->fontSize - fontSize) < 0.01f && strcmp(entry->text, text) == 0) {
                entry->lastUsed = stringSizeAccessCounter++;
                return entry->size;
            }
        }
    }
    if (!SetFontPixelSize(font.face, fontSize)) return (TextSize){0, 0};
    FT_Face face = font.face;
    int lineHeight = face->size->metrics.height >> 6;
    if (lineHeight <= 0) lineHeight = (int)ceilf(fontSize * 1.2f);
    int currentLineWidth = 0;
    int maxLineWidth = 0;
    int lines = 1;
    FT_UInt previousGlyph = 0;
    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            maxLineWidth = MaxInt(maxLineWidth, currentLineWidth);
            currentLineWidth = 0;
            previousGlyph = 0;
            lines++;
            continue;
        }
        FT_UInt glyphIndex = FT_Get_Char_Index(face, c);
        if (previousGlyph && glyphIndex && FT_HAS_KERNING(face)) {
            FT_Vector delta;
            FT_Get_Kerning(face, previousGlyph, glyphIndex, FT_KERNING_DEFAULT, &delta);
            currentLineWidth += delta.x >> 6;
        }
        unsigned int charHash = ((unsigned int)c * 16777619u) ^ (unsigned int)(fontSize * 100.0f) ^ (unsigned int)(uintptr_t)face;
        CharSizeCache* charEntry = &charSizeCache[charHash % CHAR_CACHE_SIZE];
        int charWidth;
        if (charEntry->valid && charEntry->face == face && charEntry->c == c && fabsf(charEntry->fontSize - fontSize) < 0.01f) {
            charWidth = charEntry->width;
        } else {
            if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0) continue;
            charWidth = face->glyph->advance.x >> 6;
            *charEntry = (CharSizeCache){c, face, fontSize, charWidth, lineHeight, true};
        }
        currentLineWidth += charWidth;
        previousGlyph = glyphIndex;
    }
    TextSize size = {MaxInt(maxLineWidth, currentLineWidth), lineHeight * lines};
    if (textLen < MAX_CACHED_STRING_LEN) {
        int slot = -1;
        for (int i = 0; i < STRING_CACHE_SIZE; i++) {
            int index = (hash + i) % STRING_CACHE_SIZE;
            if (!stringSizeCache[index].valid) {
                slot = index;
                break;
            }
            if (stringSizeCache[index].face == font.face && fabsf(stringSizeCache[index].fontSize - fontSize) < 0.01f && strcmp(stringSizeCache[index].text, text) == 0) {
                slot = index;
                break;
            }
        }
        if (slot < 0) {
            unsigned long oldest = ULONG_MAX;
            for (int i = 0; i < STRING_CACHE_SIZE; i++) {
                if (stringSizeCache[i].lastUsed < oldest) {
                    oldest = stringSizeCache[i].lastUsed;
                    slot = i;
                }
            }
        }
        StringSizeCache* entry = &stringSizeCache[slot];
        strncpy(entry->text, text, MAX_CACHED_STRING_LEN - 1);
        entry->text[MAX_CACHED_STRING_LEN - 1] = '\0';
        entry->face = font.face;
        entry->fontSize = fontSize;
        entry->size = size;
        entry->valid = true;
        entry->lastUsed = stringSizeAccessCounter++;
    }
    return size;
}

TextSize GetTextSizeCached(Font font, float fontSize, const char* text) {
    return GetTextSize(font, fontSize, text);
}

void RenderShaderText(ShaderObject obj, Color color, float fontSize) {
    (void)fontSize;
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
    GLuint4f(obj.shader, "Color", color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glDrawElements(GL_TRIANGLES, (GLsizei)(obj.size_indices / sizeof(GLuint)), GL_UNSIGNED_INT, 0);
}

static GLfloat textVertices[MAX_BATCH_CHARS * 20];
static GLuint textIndices[MAX_BATCH_CHARS * 6];
static bool textIndicesInitialized = false;

static void InitializeTextIndices(void) {
    if (textIndicesInitialized) return;
    for (int i = 0; i < MAX_BATCH_CHARS; i++) {
        int index = i * 6;
        int vertex = i * 4;
        textIndices[index + 0] = vertex + 0;
        textIndices[index + 1] = vertex + 1;
        textIndices[index + 2] = vertex + 2;
        textIndices[index + 3] = vertex + 2;
        textIndices[index + 4] = vertex + 3;
        textIndices[index + 5] = vertex + 0;
    }
    textIndicesInitialized = true;
}

static void SetGlyphVertices(GLfloat* vertices, Glyph* glyph, float x, float y, float scale) {
    float width = (glyph->x1 - glyph->x0) * scale;
    float height = (glyph->y1 - glyph->y0) * scale;
    vertices[0] = x;
    vertices[1] = y + height;
    vertices[2] = 0.0f;
    vertices[3] = glyph->u0;
    vertices[4] = glyph->v1;
    vertices[5] = x + width;
    vertices[6] = y + height;
    vertices[7] = 0.0f;
    vertices[8] = glyph->u1;
    vertices[9] = glyph->v1;
    vertices[10] = x + width;
    vertices[11] = y;
    vertices[12] = 0.0f;
    vertices[13] = glyph->u1;
    vertices[14] = glyph->v0;
    vertices[15] = x;
    vertices[16] = y;
    vertices[17] = 0.0f;
    vertices[18] = glyph->u0;
    vertices[19] = glyph->v0;
}

void DrawText(int x, int y, Font font, float fontSize, const char* text, Color color) {
    if (!font.face || !font.textureID || !text) return;
    if (fontSize <= 1.0f) fontSize = 1.0f;
    if (color.a == 0) color.a = 255;
    InitializeTextIndices();
    if (!SetFontPixelSize(font.face, font.fontSize)) return;
    float scale = fontSize / font.fontSize;
    int lineHeight = (int)((font.face->size->metrics.height >> 6) * scale);
    float ascent = (font.face->size->metrics.ascender >> 6) * scale;
    float xpos = (float)x;
    float ypos = (float)y + ascent;
    int charCount = 0;
    FT_UInt previousGlyph = 0;
    BindTexture(font.textureID);
    SetBlend(true);
    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            xpos = (float)x;
            ypos += lineHeight;
            previousGlyph = 0;
            continue;
        }
        if (c < 32) continue;
        FT_UInt glyphIndex = FT_Get_Char_Index(font.face, c);
        if (previousGlyph && glyphIndex && FT_HAS_KERNING(font.face)) {
            FT_Vector delta;
            FT_Get_Kerning(font.face, previousGlyph, glyphIndex, FT_KERNING_DEFAULT, &delta);
            xpos += (delta.x >> 6) * scale;
        }
        Glyph* glyph = &font.glyphs[c - 32];
        float glyphX = xpos + glyph->xoff * scale;
        float glyphY = ypos - glyph->yoff * scale;
        SetGlyphVertices(textVertices + charCount * 20, glyph, glyphX, glyphY, scale);
        charCount++;
        xpos += glyph->xadvance * scale;
        previousGlyph = glyphIndex;
        if (charCount == MAX_BATCH_CHARS) {
            RenderShaderText((ShaderObject){camera, shaderfont, textVertices, textIndices,
                (size_t)charCount * 20 * sizeof(GLfloat), (size_t)charCount * 6 * sizeof(GLuint), camera.transform, false}, color, fontSize);
            charCount = 0;
        }
    }
    if (charCount > 0) {
        RenderShaderText((ShaderObject){camera, shaderfont, textVertices, textIndices,
            (size_t)charCount * 20 * sizeof(GLfloat), (size_t)charCount * 6 * sizeof(GLuint), camera.transform, false}, color, fontSize);
    }
}

static GLfloat textBatchVertices[MAX_BATCH_CHARS * 20];
static Color textBatchColors[MAX_BATCH_CHARS];
static int textBatchCount = 0;
static GLuint currentTextureID = 0;
static float currentFontSize = 0.0f;

void FlushTextBatch() {
    if (textBatchCount == 0) return;
    BindTexture(currentTextureID);
    SetBlend(true);
    int startChar = 0;
    Color currentColor = textBatchColors[0];
    for (int i = 1; i <= textBatchCount; i++) {
        if (i < textBatchCount && SameColor(textBatchColors[i], currentColor)) continue;
        int count = i - startChar;
        RenderShaderText((ShaderObject){
            camera,
            shaderfont,
            textBatchVertices + startChar * 20,
            textIndices,
            (size_t)count * 20 * sizeof(GLfloat),
            (size_t)count * 6 * sizeof(GLuint),
            camera.transform,
            false
        }, currentColor, currentFontSize);
        if (i < textBatchCount) {
            startChar = i;
            currentColor = textBatchColors[i];
        }
    }
    textBatchCount = 0;
}

void DrawTextBatch(int x, int y, Font font, float fontSize, const char* text, Color color) {
    if (!font.face || !font.textureID || !text) return;
    if (fontSize <= 1.0f) fontSize = 1.0f;
    if (color.a == 0) color.a = 255;
    InitializeTextIndices();
    if (textBatchCount > 0 && (currentTextureID != font.textureID || currentFontSize != fontSize)) FlushTextBatch();
    currentTextureID = font.textureID;
    currentFontSize = fontSize;
    if (!SetFontPixelSize(font.face, font.fontSize)) return;
    float scale = fontSize / font.fontSize;
    int lineHeight = (int)((font.face->size->metrics.height >> 6) * scale);
    float ascent = (font.face->size->metrics.ascender >> 6) * scale;
    float xpos = (float)x;
    float ypos = (float)y + ascent;
    FT_UInt previousGlyph = 0;
    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            xpos = (float)x;
            ypos += lineHeight;
            previousGlyph = 0;
            continue;
        }
        if (c < 32) continue;
        if (textBatchCount == MAX_BATCH_CHARS) FlushTextBatch();
        FT_UInt glyphIndex = FT_Get_Char_Index(font.face, c);
        if (previousGlyph && glyphIndex && FT_HAS_KERNING(font.face)) {
            FT_Vector delta;
            FT_Get_Kerning(font.face, previousGlyph, glyphIndex, FT_KERNING_DEFAULT, &delta);
            xpos += (delta.x >> 6) * scale;
        }
        Glyph* glyph = &font.glyphs[c - 32];
        float glyphX = xpos + glyph->xoff * scale;
        float glyphY = ypos - glyph->yoff * scale;
        SetGlyphVertices(textBatchVertices + textBatchCount * 20, glyph, glyphX, glyphY, scale);
        textBatchColors[textBatchCount++] = color;
        xpos += glyph->xadvance * scale;
        previousGlyph = glyphIndex;
    }
}

void CleanUpFontCache(void) {
    FlushTextBatch();
    for (int i = 0; i < fontTextureCount; i++) {
        if (!fontTextures[i]) continue;
        if (renderTexture == fontTextures[i]) UnbindTexture();
        glDeleteTextures(1, &fontTextures[i]);
    }
    fontTextureCount = 0;
    for (int i = 0; i < loadedFontCount; i++) {
        if (loadedFaces[i]) FT_Done_Face(loadedFaces[i]);
        if (loadedLibraries[i]) FT_Done_FreeType(loadedLibraries[i]);
    }
    loadedFontCount = 0;
    currentPixelFace = NULL;
    currentPixelSize = 0.0f;
    memset(fontCacheTable, 0, sizeof(fontCacheTable));
    memset(charSizeCache, 0, sizeof(charSizeCache));
    memset(stringSizeCache, 0, sizeof(stringSizeCache));
    memset(codepointMap, 0, sizeof(codepointMap));
    fontCacheAccessCounter = 1;
    stringSizeAccessCounter = 1;
}
