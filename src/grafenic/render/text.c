#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

typedef struct {
    float x0, y0, x1, y1;  // coordinates of the glyph in the atlas
    float xoff, yoff;      // left/top offsets
    float xadvance;        // advance widt
} Glyph;

#define MAX_GLYPHS 256

typedef struct {
    stbtt_fontinfo fontInfo;   // Font information
    unsigned char* fontBuffer; // Font data buffer
    unsigned char* atlasData;  // Atlas texture data
    int atlasWidth;            // Dimensions of the atlas Width
    int atlasHeight;           // Dimensions of the atlas Height
    Glyph glyphs[MAX_GLYPHS];  // Glyph data for ASCII characters 32-127
    float fontSize;            // Font size for which glyphs were generated
    bool nearest;              // Nearest filter
} Font;

Font GenAtlas(Font font) {
    if (font.fontSize <= 1) font.fontSize = 24.0;
    if (!font.fontBuffer) return font;
    if (!font.nearest) font.nearest = false;
    font.atlasWidth = 512;
    font.atlasHeight = 512;
    font.atlasData = (unsigned char*)calloc(font.atlasWidth * font.atlasHeight, sizeof(unsigned char));
    if (!font.atlasData) {
        free(font.fontBuffer);
        return font;
    }
    stbtt_pack_context packContext;
    if (!stbtt_PackBegin(&packContext, font.atlasData, font.atlasWidth, font.atlasHeight, 0, 5, NULL)) {
        free(font.atlasData);
        free(font.fontBuffer);
        return font;
    }
    stbtt_PackSetOversampling(&packContext, 2, 2);
    stbtt_packedchar packedChars[MAX_GLYPHS];
    if (!stbtt_PackFontRange(&packContext, font.fontBuffer, 0, font.fontSize, 32, MAX_GLYPHS, packedChars)) {
        stbtt_PackEnd(&packContext);
        free(font.atlasData);
        free(font.fontBuffer);
        return font;
    }
    for (int i = 0; i < MAX_GLYPHS; ++i) {
        font.glyphs[i].x0 = packedChars[i].x0;
        font.glyphs[i].y0 = packedChars[i].y0;
        font.glyphs[i].x1 = packedChars[i].x1;
        font.glyphs[i].y1 = packedChars[i].y1;
        font.glyphs[i].xoff = packedChars[i].xoff;
        font.glyphs[i].yoff = packedChars[i].yoff;
        font.glyphs[i].xadvance = packedChars[i].xadvance;
        font.glyphs[i].x0 /= font.atlasWidth;
        font.glyphs[i].y0 /= font.atlasHeight;
        font.glyphs[i].x1 /= font.atlasWidth;
        font.glyphs[i].y1 /= font.atlasHeight;
    }
    stbtt_PackEnd(&packContext);
    stbi_write_png("/tmp/fontatlas.png", font.atlasWidth, font.atlasHeight, 1, font.atlasData, font.atlasWidth);
    return font;
}

Font LoadFont(const char* fontPath) {
    Font font = {0};
    long size;
    FILE* fp = fopen(fontPath, "rb");
    if (!fp) return font;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    font.fontBuffer = (unsigned char*)malloc(size);
    if (font.fontBuffer) {
        fread(font.fontBuffer, size, 1, fp);
        if (!stbtt_InitFont(&font.fontInfo, font.fontBuffer, stbtt_GetFontOffsetForIndex(font.fontBuffer, 0))) {
            free(font.fontBuffer);
            fclose(fp);
            return font;
        }
    }
    fclose(fp);
    font = GenAtlas(font);
    return font;
}

typedef struct {
    int width;
    int height;
} TextSize;

TextSize GetTextSize(Font font, float fontSize, const char* text) {
    if (!font.fontBuffer) return (TextSize){0, 0};
    TextSize size = {0, 0};
    float scale = stbtt_ScaleForPixelHeight(&font.fontInfo, fontSize);
    int ascent, descent;
    stbtt_GetFontVMetrics(&font.fontInfo, &ascent, &descent, 0); 
    int max_height = (int)((ascent) * scale);
    for (size_t i = 0; text[i] != '\0'; ++i) {
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&font.fontInfo, text[i], &advanceWidth, &leftSideBearing);
        size.width += (int)(advanceWidth * scale);
    }
    size.height = max_height;
    return size;
}

void DrawTextPixel(int x, int y, Font font, float fontSize, const char* text, Color color) {
    if (fontSize <= 1) fontSize = 1;
    if (color.a == 0) color.a = 255;
    if (!font.fontBuffer) return;
    float scale = stbtt_ScaleForPixelHeight(&font.fontInfo, fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font.fontInfo, &ascent, &descent, &lineGap);
    y -= (int)((ascent + descent) * scale);
    int ch_x = x;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        int width, height, xoffset, yoffset;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font.fontInfo, 0, scale, text[i], &width, &height, &xoffset, &yoffset);
        for (int j = 0; j < height; ++j) {
            for (int k = 0; k < width; ++k) {
                int pixel = j * width + k;
                unsigned char alpha = bitmap[pixel];
                if (alpha > 0) {
                    DrawPixel(ch_x + k + xoffset, y - j - yoffset, (Color){color.r, color.g, color.b, color.a / (alpha*3)});
                }
            }
        }
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&font.fontInfo, text[i], &advanceWidth, &leftSideBearing);
        ch_x += (int)((advanceWidth + (text[i+1] ? stbtt_GetCodepointKernAdvance(&font.fontInfo, text[i], text[i+1]) : 0)) * scale);
        stbtt_FreeBitmap(bitmap, NULL);
    }
}

void DrawText(int x, int y, Font font, float fontSize, const char* text, Color color) {
    if (fontSize <= 1.0f)
        fontSize = 1.0f;
    if (color.a == 0)
        color.a = 255;
    if (!font.fontBuffer || !font.atlasData)
        return;
    float baseFontSize = font.fontSize;
    float scale = stbtt_ScaleForPixelHeight(&font.fontInfo, baseFontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font.fontInfo, &ascent, &descent, &lineGap);
    int baseTextHeight = (int)((ascent - descent) * scale);
    int baseTextWidth = 0;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n') {
            baseTextWidth = 0;
            continue;
        }
        int glyphIndex = text[i] - 32;
        float xAdvance = font.glyphs[glyphIndex].xadvance;
        baseTextWidth += (int)xAdvance;
    }
    static GLuint textTexture = 0;
    if (textTexture == 0) {
        glGenTextures(1, &textTexture);
    }
    glBindTexture(GL_TEXTURE_2D, textTexture);
    unsigned char* bitmap = (unsigned char*)calloc(baseTextWidth * baseTextHeight * 4, sizeof(unsigned char));
    if (!bitmap) {
        return;
    }
    int offsetX = 0;
    int offsetY = 0;
    int offset = 0;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n') {
            offsetY += baseTextHeight;
            offsetX = 0;
            continue;
        }
        int glyphIndex = text[i] - 32;
        float xAdvance = font.glyphs[glyphIndex].xadvance;
        int charWidth = (int)(font.glyphs[glyphIndex].x1 - font.glyphs[glyphIndex].x0);
        int charHeight = (int)(font.glyphs[glyphIndex].y1 - font.glyphs[glyphIndex].y0);
        int xoffset = (int)font.glyphs[glyphIndex].xoff;
        int yoffset = (int)font.glyphs[glyphIndex].yoff;
        unsigned char* char_bitmap = stbtt_GetCodepointBitmap(&font.fontInfo, 0, scale, text[i], &charWidth, &charHeight, &xoffset, &yoffset);
        if (i == 0) {offset = 1+(charHeight/2) ;}
        for (int j = 0; j < charHeight; ++j) {
            for (int k = 0; k < charWidth; ++k) {
                int bitmap_x = offsetX + k + xoffset;
                int bitmap_y = offset + offsetY - j - yoffset;
                if (bitmap_x < 0 || bitmap_x >= baseTextWidth || bitmap_y < 0 || bitmap_y >= baseTextHeight) continue;
                int pixel = (bitmap_y * baseTextWidth + bitmap_x) * 4;
                unsigned char alpha = char_bitmap[j * charWidth + k];
                bitmap[pixel] = color.r;
                bitmap[pixel + 1] = color.g;
                bitmap[pixel + 2] = color.b;
                bitmap[pixel + 3] = (color.a * alpha) / 255;
            }
        }
        offsetX += (int)xAdvance;
        stbtt_FreeBitmap(char_bitmap, NULL);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, baseTextWidth, baseTextHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
    free(bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (font.nearest) {
        glTexOpt(GL_NEAREST,GL_CLAMP_TO_EDGE);
    } else {
        glTexOpt(GL_LINEAR,GL_CLAMP_TO_EDGE);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    float quadScaleFactor = fontSize / baseFontSize;
    int scaledTextHeight = (int)(baseTextHeight * quadScaleFactor);
    int scaledTextWidth = (int)(baseTextWidth * quadScaleFactor);
    Rect((RectObject){
        { x, y + scaledTextHeight, 0.0f },                   // Bottom Left
        { x + scaledTextWidth, y + scaledTextHeight, 0.0f }, // Bottom Right
        { x, y, 0.0f },                                      // Top Left
        { x + scaledTextWidth, y, 0.0f },                    // Top Right
        shaderdefault,                                       // Shader
        camera,                                              // Camera
    });
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
}
