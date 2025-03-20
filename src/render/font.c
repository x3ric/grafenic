
int CalculateAtlasSize(int numGlyphs, float fontSize, int oversampling) {
    float estimatedAreaPerGlyph = (fontSize * fontSize) * oversampling * oversampling;
    float totalArea = estimatedAreaPerGlyph * numGlyphs;
    int atlasDimension = (int)sqrt(totalArea);
    int powerOfTwo = 1;
    while (powerOfTwo < atlasDimension) {
        powerOfTwo *= 2;
    }
    return powerOfTwo;
}

Font GenAtlas(Font font) {
    if (font.fontSize <= 1) font.fontSize = ATLAS_FONT_SIZE;
    if (font.oversampling <= 1) font.oversampling = 1;
    if (!font.face) return font;
    FT_Error error = FT_Set_Pixel_Sizes(font.face, 0, font.fontSize);
    if (error) {
        return font;
    }
    if (font.subpixel) {
        FT_Library_SetLcdFilter(font.library, FT_LCD_FILTER_DEFAULT);
    }
    int maxGlyphWidth = (int)(font.fontSize * 1.5f);
    int maxGlyphHeight = (int)(font.fontSize * 1.5f);
    int glyphsPerRow = 16;
    int atlasWidth = glyphsPerRow * maxGlyphWidth;
    int atlasHeight = glyphsPerRow * maxGlyphHeight;
    atlasWidth = 1;
    while (atlasWidth < glyphsPerRow * maxGlyphWidth) {
        atlasWidth *= 2;
    }
    atlasHeight = 1;
    while (atlasHeight < glyphsPerRow * maxGlyphHeight) {
        atlasHeight *= 2;
    }
    font.atlasWidth = atlasWidth;
    font.atlasHeight = atlasHeight;
    font.atlasData = (unsigned char*)calloc(font.atlasWidth * font.atlasHeight * 4, 1);
    if (!font.atlasData) {
        return font;
    }
    int cellWidth = atlasWidth / glyphsPerRow;
    int cellHeight = atlasHeight / glyphsPerRow;
    for (int i = 0; i < MAX_GLYPHS; ++i) {
        int codepoint = 32 + i;
        int row = i / glyphsPerRow;
        int col = i % glyphsPerRow;
        int x = col * cellWidth;
        int y = row * cellHeight;
        if (font.subpixel) {
            error = FT_Load_Char(font.face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LCD);
        } else {
            error = FT_Load_Char(font.face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
        }
        if (error) {
            Glyph* glyph = &font.glyphs[i];
            glyph->x0 = x;
            glyph->y0 = y;
            glyph->x1 = x;
            glyph->y1 = y;
            glyph->xoff = 0;
            glyph->yoff = 0;
            glyph->xadvance = font.fontSize / 3;
            glyph->u0 = (float)x / (float)font.atlasWidth;
            glyph->v0 = (float)y / (float)font.atlasHeight;
            glyph->u1 = glyph->u0;
            glyph->v1 = glyph->v0;
            continue;
        }
        FT_GlyphSlot slot = font.face->glyph;
        FT_Bitmap* bitmap = &slot->bitmap;
        if (bitmap->width > 0 && bitmap->rows > 0) {
            int glyphWidth = font.subpixel ? bitmap->width / 3 : bitmap->width;
            int glyphHeight = bitmap->rows;
            int offsetX = (cellWidth - glyphWidth) / 2;
            int offsetY = (cellHeight - glyphHeight) / 2;
            x += offsetX;
            y += offsetY;
            if (font.subpixel) {
                for (int row = 0; row < bitmap->rows; ++row) {
                    for (int col = 0; col < bitmap->width; col += 3) {
                        int srcIndex = row * bitmap->pitch + col;
                        int dstIndex = ((y + row) * font.atlasWidth + (x + col / 3)) * 4;
                        font.atlasData[dstIndex + 0] = 255;
                        font.atlasData[dstIndex + 1] = 255;
                        font.atlasData[dstIndex + 2] = 255;
                        int r = col < bitmap->width ? bitmap->buffer[srcIndex] : 0;
                        int g = col+1 < bitmap->width ? bitmap->buffer[srcIndex+1] : 0;
                        int b = col+2 < bitmap->width ? bitmap->buffer[srcIndex+2] : 0;
                        font.atlasData[dstIndex + 3] = (r + g + b) / 3;
                    }
                }
            } else {
                for (int row = 0; row < bitmap->rows; ++row) {
                    unsigned char* dst = &font.atlasData[((y + row) * font.atlasWidth + x) * 4];
                    unsigned char* src = &bitmap->buffer[row * bitmap->pitch];
                    for (int col = 0; col < bitmap->width; ++col) {
                        *dst++ = 255;  // R
                        *dst++ = 255;  // G
                        *dst++ = 255;  // B
                        *dst++ = *src++;  // A
                    }
                }
            }
            Glyph* glyph = &font.glyphs[i];
            glyph->x0 = x;
            glyph->y0 = y;
            glyph->x1 = x + (font.subpixel ? bitmap->width / 3 : bitmap->width);
            glyph->y1 = y + bitmap->rows;
            glyph->xoff = slot->bitmap_left;
            glyph->yoff = slot->bitmap_top;
            glyph->xadvance = slot->advance.x >> 6;
            glyph->u0 = (float)glyph->x0 / (float)font.atlasWidth;
            glyph->v0 = (float)glyph->y0 / (float)font.atlasHeight;
            glyph->u1 = (float)glyph->x1 / (float)font.atlasWidth;
            glyph->v1 = (float)glyph->y1 / (float)font.atlasHeight;
        } else {
            Glyph* glyph = &font.glyphs[i];
            glyph->x0 = x;
            glyph->y0 = y;
            glyph->x1 = x;
            glyph->y1 = y;
            glyph->xoff = slot->bitmap_left;
            glyph->yoff = slot->bitmap_top;
            glyph->xadvance = slot->advance.x >> 6;
            glyph->u0 = (float)x / (float)font.atlasWidth;
            glyph->v0 = (float)y / (float)font.atlasHeight;
            glyph->u1 = glyph->u0;
            glyph->v1 = glyph->v0;
        }
    }
    glGenTextures(1, &font.textureID);
    glBindTexture(GL_TEXTURE_2D, font.textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font.atlasWidth, font.atlasHeight, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, font.atlasData);
    glTexOpt(font.nearest ? GL_NEAREST : GL_LINEAR, GL_CLAMP_TO_EDGE);
    free(font.atlasData);
    font.atlasData = NULL;
    return font;
}

Font LoadFont(const char* fontPath) {
    Font font = {0};
    FT_Error error = FT_Init_FreeType(&font.library);
    if (error) {
        return font;
    }
    error = FT_New_Face(font.library, fontPath, 0, &font.face);
    if (error == FT_Err_Unknown_File_Format) {
        FT_Done_FreeType(font.library);
        return font;
    } else if (error) {
        FT_Done_FreeType(font.library);
        return font;
    }
    if (font.fontSize <= 1) font.fontSize = ATLAS_FONT_SIZE;
    if (font.oversampling <= 1) font.oversampling = 1;
    font = GenAtlas(font);
    return font;
}

typedef struct {
    float fontSize;
    Font font;
    bool used;
    unsigned long lastUsed;
} FontCacheEntry;

FontCacheEntry fontCacheTable[FONT_CACHE_SIZE] = {0};
unsigned long fontCacheAccessCounter = 0;

Font SetFontSize(Font font, float fontSize) {
    if (fontSize <= 1) fontSize = ATLAS_FONT_SIZE;
    unsigned int hash = (unsigned int)(fontSize * 100) % FONT_CACHE_SIZE;
    for (int i = 0; i < FONT_CACHE_SIZE; i++) {
        int index = (hash + i) % FONT_CACHE_SIZE;
        if (fontCacheTable[index].used && 
            fabsf(fontCacheTable[index].fontSize - fontSize) < 0.01f) {
            fontCacheTable[index].lastUsed = fontCacheAccessCounter++;
            return fontCacheTable[index].font;
        }
        if (!fontCacheTable[index].used) {
            font.fontSize = fontSize;
            font = GenAtlas(font);
            fontCacheTable[index].fontSize = fontSize;
            fontCacheTable[index].font = font;
            fontCacheTable[index].used = true;
            fontCacheTable[index].lastUsed = fontCacheAccessCounter++;
            return font;
        }
    }
    int lruIndex = 0;
    unsigned long oldestAccess = ULONG_MAX;
    for (int i = 0; i < FONT_CACHE_SIZE; i++) {
        if (fontCacheTable[i].lastUsed < oldestAccess) {
            oldestAccess = fontCacheTable[i].lastUsed;
            lruIndex = i;
        }
    }
    if (fontCacheTable[lruIndex].font.textureID) {
        glDeleteTextures(1, &fontCacheTable[lruIndex].font.textureID);
    }
    font.fontSize = fontSize;
    font = GenAtlas(font);
    fontCacheTable[lruIndex].fontSize = fontSize;
    fontCacheTable[lruIndex].font = font;
    fontCacheTable[lruIndex].lastUsed = fontCacheAccessCounter++;
    return font;
}

void PreloadFontSizes(Font font) {
    float commonSizes[] = {12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 24.0f};
    for (int i = 0; i < 6; i++) {
        SetFontSize(font, commonSizes[i]);
    }
}

typedef struct {
    unsigned char c;
    float fontSize;
    int width;
    int height;
    bool valid;
} CharSizeCache;

#define CHAR_CACHE_SIZE 256
CharSizeCache charSizeCache[CHAR_CACHE_SIZE] = {0};
#define STRING_CACHE_SIZE 64
#define MAX_CACHED_STRING_LEN 32

typedef struct {
    char text[MAX_CACHED_STRING_LEN];
    float fontSize;
    TextSize size;
    bool valid;
    unsigned long lastUsed;
} StringSizeCache;

StringSizeCache stringSizeCache[STRING_CACHE_SIZE] = {0};
unsigned long stringSizeAccessCounter = 0;

#define MAX_BATCH_CHARS 8192
#define MAX_BATCH_CALLS 16

typedef struct {
    Font font;
    float fontSize;
    Color color;
    GLfloat* vertices;
    GLuint* indices;
    int vertexCount;
    int indexCount;
    int charCount;
} TextBatch;

typedef struct {
    TextBatch batches[MAX_BATCH_CALLS];
    int activetextBatchCount;
    bool batchingEnabled;
} TextRenderState;

static TextRenderState textRenderState = {0};

unsigned int HashTextString(const char* str, float fontSize) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    hash = hash ^ ((unsigned int)(fontSize * 100));
    return hash % STRING_CACHE_SIZE;
}

TextSize GetTextSize(Font font, float fontSize, const char* text) {
    if (!font.face) return (TextSize){0, 0};
    if (!text || !text[0]) {
        return (TextSize){0, (int)(fontSize * 1.2f)};
    }
    size_t textLen = strlen(text);
    if (textLen < MAX_CACHED_STRING_LEN) {
        unsigned int hash = HashTextString(text, fontSize);
        for (int i = 0; i < STRING_CACHE_SIZE; i++) {
            int index = (hash + i) % STRING_CACHE_SIZE;
            if (stringSizeCache[index].valid && 
                stringSizeCache[index].fontSize == fontSize &&
                strcmp(stringSizeCache[index].text, text) == 0) {
                stringSizeCache[index].lastUsed = stringSizeAccessCounter++;
                return stringSizeCache[index].size;
            }   
            if (!stringSizeCache[index].valid) {
                break;
            }
        }
    }
    TextSize size = {0, 0};
    FT_Error error = FT_Set_Pixel_Sizes(font.face, 0, fontSize);
    if (error) {
        return size;
    }
    FT_Face face = font.face;
    int ascent = face->size->metrics.ascender >> 6;
    int descent = face->size->metrics.descender >> 6;
    int lineGap = (face->size->metrics.height - (face->size->metrics.ascender - face->size->metrics.descender)) >> 6;
    int lineHeight = ascent - descent + lineGap;
    int currentLineWidth = 0;
    int maxLineWidth = 0;
    int lines = 1;
    unsigned char prevC = 0;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            if (currentLineWidth > maxLineWidth) {
                maxLineWidth = currentLineWidth;
            }
            currentLineWidth = 0;
            lines++;
            prevC = 0;
            continue;
        }
        unsigned int charHash = (c << 16) | ((unsigned int)(fontSize * 100) & 0xFFFF);
        unsigned int charIndex = charHash % CHAR_CACHE_SIZE;
        int charWidth;
        if (charSizeCache[charIndex].valid && 
            charSizeCache[charIndex].c == c && 
            charSizeCache[charIndex].fontSize == fontSize) {
            charWidth = charSizeCache[charIndex].width;
        } else {
            error = FT_Load_Char(face, c, FT_LOAD_DEFAULT);
            if (error) {
                continue;
            }
            charWidth = face->glyph->advance.x >> 6;
            charSizeCache[charIndex].c = c;
            charSizeCache[charIndex].fontSize = fontSize;
            charSizeCache[charIndex].width = charWidth;
            charSizeCache[charIndex].height = lineHeight;
            charSizeCache[charIndex].valid = true;
        }
        currentLineWidth += charWidth;
        if (prevC && text[i+1]) {
            FT_Vector delta;
            FT_Get_Kerning(face, prevC, c, FT_KERNING_DEFAULT, &delta);
            currentLineWidth += delta.x >> 6;
        }
        prevC = c;
    }
    if (currentLineWidth > maxLineWidth) {
        maxLineWidth = currentLineWidth;
    }
    size.width = maxLineWidth;
    size.height = lineHeight * lines;
    if (textLen < MAX_CACHED_STRING_LEN) {
        unsigned int hash = HashTextString(text, fontSize);
        int slotIndex = -1;
        for (int i = 0; i < STRING_CACHE_SIZE; i++) {
            int index = (hash + i) % STRING_CACHE_SIZE;
            if (!stringSizeCache[index].valid) {
                slotIndex = index;
                break;
            }
            if (stringSizeCache[index].fontSize == fontSize &&
                strcmp(stringSizeCache[index].text, text) == 0) {
                slotIndex = index;
                break;
            }
        }
        if (slotIndex == -1) {
            unsigned long oldestAccess = ULONG_MAX;
            for (int i = 0; i < STRING_CACHE_SIZE; i++) {
                if (stringSizeCache[i].lastUsed < oldestAccess) {
                    oldestAccess = stringSizeCache[i].lastUsed;
                    slotIndex = i;
                }
            }
        }
        if (slotIndex >= 0) {
            strncpy(stringSizeCache[slotIndex].text, text, MAX_CACHED_STRING_LEN-1);
            stringSizeCache[slotIndex].text[MAX_CACHED_STRING_LEN-1] = '\0';
            stringSizeCache[slotIndex].fontSize = fontSize;
            stringSizeCache[slotIndex].size = size;
            stringSizeCache[slotIndex].valid = true;
            stringSizeCache[slotIndex].lastUsed = stringSizeAccessCounter++;
        }
    }
    return size;
}

TextSize GetTextSizeCached(Font font, float fontSize, const char* text) {
    static TextSize cachedSize;
    static char lastText[64] = "";
    static float lastSize = 0;
    if (fontSize == lastSize && strcmp(text, lastText) == 0 && strlen(text) < 63)
        return cachedSize;
    cachedSize = GetTextSize(font, fontSize, text);
    lastSize = fontSize;
    strncpy(lastText, text, 63);
    return cachedSize;
}

void RenderShaderText(ShaderObject obj, Color color, float fontSize) {
    if (obj.shader.hotreloading) {
        obj.shader = ShaderHotReload(obj.shader);
    }
    // Projection Matrix
        GLfloat Projection[16], Model[16], View[16];
        CalculateProjections(obj,Model,Projection,View);
    // Depth
        if(obj.is3d) {
            if(obj.cam.fov > 0.0f){
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LEQUAL);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
            } else {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                glFrontFace(GL_CCW);
            }
        }
    // Debug
        if (window.debug.wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else if (window.debug.point) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            if(window.debug.pointsize > 0) glPointSize(window.debug.pointsize);
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
        GLumatrix4fv(obj.shader, "projection", Projection);
        GLumatrix4fv(obj.shader, "model", Model);
        GLumatrix4fv(obj.shader, "view", View);
        //GLuint1f(obj.shader, "Size", fontSize);
        GLuint4f(obj.shader, "Color", color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        //GLuint1f(obj.shader, "iTime", glfwGetTime());
        //GLuint2f(obj.shader, "iResolution", window.screen_width, window.screen_height);
        //GLuint2f(obj.shader, "iMouse", mouse.x, mouse.y);
    // Draw using indices
        glDrawElements(GL_TRIANGLES, obj.size_indices / sizeof(GLuint), GL_UNSIGNED_INT, 0);
    // Unbind shader program
        glUseProgram(0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // Disable Effects
        if(obj.is3d) {
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
        }
}

void DrawText(int x, int y, Font font, float fontSize, const char* text, Color color) {
    if (fontSize <= 1.0f) fontSize = 1.0f;
    if (color.a == 0) color.a = 255;
    if (!font.face || !font.textureID) return;
    float scale = fontSize / font.fontSize;
    font = SetFontSize(font, font.fontSize);
    #define MAX_BATCH_CHARS 4096
    GLfloat vertices[MAX_BATCH_CHARS * 20];
    GLuint indices[MAX_BATCH_CHARS * 6];
    int charCount = 0;
    int indexCount = 0;
    int vertexCount = 0;
    int lineHeight = (font.face->size->metrics.height >> 6) * scale;
    glBindTexture(GL_TEXTURE_2D, font.textureID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float xpos = (float)x;
    float ypos = (float)y + (120.0f * scale);
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n' || charCount >= MAX_BATCH_CHARS) {
            if (charCount > 0) {
                RenderShaderText((ShaderObject){
                    camera, shaderfont, vertices, indices, 
                    vertexCount * sizeof(GLfloat), indexCount * sizeof(GLuint), 
                    camera.transform}, color, fontSize);
                charCount = 0;
                vertexCount = 0;
                indexCount = 0;
            }
            if (text[i] == '\n') {
                ypos += lineHeight;
                xpos = (float)x;
                continue;
            }
        }
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c >= 32 + MAX_GLYPHS) continue;
        Glyph* glyph = &font.glyphs[c - 32];
        float x_start = xpos + glyph->xoff * scale;
        float y_start = ypos - glyph->yoff * scale;
        float w = (glyph->x1 - glyph->x0) * scale;
        float h = (glyph->y1 - glyph->y0) * scale;
        int vbase = vertexCount;
        vertices[vbase + 0] = x_start;     
        vertices[vbase + 1] = y_start + h; 
        vertices[vbase + 2] = 0.0f;        
        vertices[vbase + 3] = glyph->u0;   
        vertices[vbase + 4] = glyph->v1;
        vertices[vbase + 5] = x_start + w; 
        vertices[vbase + 6] = y_start + h; 
        vertices[vbase + 7] = 0.0f;        
        vertices[vbase + 8] = glyph->u1;   
        vertices[vbase + 9] = glyph->v1;
        vertices[vbase + 10] = x_start + w;
        vertices[vbase + 11] = y_start;    
        vertices[vbase + 12] = 0.0f;       
        vertices[vbase + 13] = glyph->u1;  
        vertices[vbase + 14] = glyph->v0;
        vertices[vbase + 15] = x_start;    
        vertices[vbase + 16] = y_start;    
        vertices[vbase + 17] = 0.0f;       
        vertices[vbase + 18] = glyph->u0;  
        vertices[vbase + 19] = glyph->v0;
        int ibase = indexCount;
        int vstart = charCount * 4;
        indices[ibase + 0] = vstart + 0;
        indices[ibase + 1] = vstart + 1;
        indices[ibase + 2] = vstart + 2;
        indices[ibase + 3] = vstart + 2;
        indices[ibase + 4] = vstart + 3;
        indices[ibase + 5] = vstart + 0;
        vertexCount += 20;
        indexCount += 6;
        charCount++;
        xpos += glyph->xadvance * scale;
    }
    if (charCount > 0) {
        RenderShaderText((ShaderObject){
            camera, shaderfont, vertices, indices, 
            vertexCount * sizeof(GLfloat), indexCount * sizeof(GLuint), 
            camera.transform}, color, fontSize);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    //glDisable(GL_BLEND);
}

typedef struct {
    GLfloat vertices[20];     // 5 attributes (x,y,z,u,v) for 4 vertices per char
    Color color;
} BatchChar;

static BatchChar charBatch[MAX_BATCH_CHARS];
static GLuint batchIndices[MAX_BATCH_CHARS * 6];
static int textBatchCount = 0;
static GLuint currentTextureID = 0;
static float currentFontSize = 0;

void FlushTextBatch() {
    if (textBatchCount == 0) return;
    glBindTexture(GL_TEXTURE_2D, currentTextureID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int startChar = 0;
    Color currentColor = charBatch[0].color;
    for (int i = 1; i <= textBatchCount; i++) {
        bool colorChanged = i == textBatchCount || 
            memcmp(&charBatch[i].color, &currentColor, sizeof(Color)) != 0;
        if (colorChanged) {
            int charCount = i - startChar;
            GLfloat tempVertices[MAX_BATCH_CHARS * 20];
            for (int j = 0; j < charCount; j++) {
                memcpy(&tempVertices[j * 20], charBatch[startChar + j].vertices, 20 * sizeof(GLfloat));
            }
            RenderShaderText((ShaderObject){
                camera, shaderfont, tempVertices, batchIndices, 
                charCount * 20 * sizeof(GLfloat), 
                charCount * 6 * sizeof(GLuint),
                camera.transform}, currentColor, currentFontSize);
            if (i < textBatchCount) {
                startChar = i;
                currentColor = charBatch[i].color;
            }
        }
    }
    textBatchCount = 0;
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

void DrawTextBatch(int x, int y, Font font, float fontSize, const char* text, Color color) {
    if (!font.face || !font.textureID) return;
    if (fontSize <= 1.0f) fontSize = 1.0f;
    if (color.a == 0) color.a = 255;
    if (textBatchCount >= MAX_BATCH_CHARS - 1 || 
        currentTextureID != font.textureID || 
        currentFontSize != fontSize) {
        FlushTextBatch();
    }
    currentTextureID = font.textureID;
    currentFontSize = fontSize;
    float scale = fontSize / font.fontSize;
    font = SetFontSize(font, font.fontSize);
    int lineHeight = (font.face->size->metrics.height >> 6) * scale;
    float xpos = (float)x;
    float ypos = (float)y + (120.0f * scale);
    static bool indicesInitialized = false;
    if (!indicesInitialized) {
        for (int i = 0; i < MAX_BATCH_CHARS; i++) {
            int idx = i * 6;
            int vstart = i * 4;
            batchIndices[idx + 0] = vstart + 0;
            batchIndices[idx + 1] = vstart + 1;
            batchIndices[idx + 2] = vstart + 2;
            batchIndices[idx + 3] = vstart + 2;
            batchIndices[idx + 4] = vstart + 3;
            batchIndices[idx + 5] = vstart + 0;
        }
        indicesInitialized = true;
    }
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n' || textBatchCount >= MAX_BATCH_CHARS) {
            if (textBatchCount > 0) {
                FlushTextBatch();
            }
            if (text[i] == '\n') {
                ypos += lineHeight;
                xpos = (float)x;
                continue;
            }
        }
        unsigned char c = (unsigned char)text[i];
        if (c < 32 || c >= 32 + MAX_GLYPHS) continue;
        Glyph* glyph = &font.glyphs[c - 32];
        float x_start = xpos + glyph->xoff * scale;
        float y_start = ypos - glyph->yoff * scale;
        float w = (glyph->x1 - glyph->x0) * scale;
        float h = (glyph->y1 - glyph->y0) * scale;
        BatchChar* ch = &charBatch[textBatchCount];
        ch->color = color;
        // Bottom left
        ch->vertices[0] = x_start;
        ch->vertices[1] = y_start + h;
        ch->vertices[2] = 0.0f;
        ch->vertices[3] = glyph->u0;
        ch->vertices[4] = glyph->v1;
        // Bottom right
        ch->vertices[5] = x_start + w;
        ch->vertices[6] = y_start + h;
        ch->vertices[7] = 0.0f;
        ch->vertices[8] = glyph->u1;
        ch->vertices[9] = glyph->v1;
        // Top right
        ch->vertices[10] = x_start + w;
        ch->vertices[11] = y_start;
        ch->vertices[12] = 0.0f;
        ch->vertices[13] = glyph->u1;
        ch->vertices[14] = glyph->v0;
        // Top left
        ch->vertices[15] = x_start;
        ch->vertices[16] = y_start;
        ch->vertices[17] = 0.0f;
        ch->vertices[18] = glyph->u0;
        ch->vertices[19] = glyph->v0;
        // End
        textBatchCount++;
        xpos += glyph->xadvance * scale;
    }
}
