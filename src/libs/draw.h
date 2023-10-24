#ifndef DRAW_H
#define DRAW_H

// COLORS
#define LIGHTGRAY  (Color){ 200, 200, 200}   // Light Gray
#define GRAY       (Color){ 130, 130, 130}   // Gray
#define DARKGRAY   (Color){ 80, 80, 80}      // Dark Gray
#define YELLOW     (Color){ 253, 249, 0}     // Yellow
#define GOLD       (Color){ 255, 203, 0}     // Gold
#define ORANGE     (Color){ 255, 161, 0}     // Orange
#define PINK       (Color){ 255, 109, 194}   // Pink
#define RED        (Color){ 230, 41, 55}     // Red
#define MAROON     (Color){ 190, 33, 55}     // Maroon
#define GREEN      (Color){ 0, 228, 48}      // Green
#define LIME       (Color){ 0, 158, 47}      // Lime
#define DARKGREEN  (Color){ 0, 117, 44}      // Dark Green
#define SKYBLUE    (Color){ 102, 191, 255}   // Sky Blue
#define BLUE       (Color){ 0, 121, 241}     // Blue
#define DARKBLUE   (Color){ 0, 82, 172}      // Dark Blue
#define PURPLE     (Color){ 200, 122, 255}   // Purple
#define VIOLET     (Color){ 135, 60, 190}    // Violet
#define DARKPURPLE (Color){ 112, 31, 126}    // Dark Purple
#define BEIGE      (Color){ 211, 176, 131}   // Beige
#define BROWN      (Color){ 127, 106, 79}    // Brown
#define DARKBROWN  (Color){ 76, 63, 47}      // DarkBrown
#define WHITE      (Color){ 255, 255, 255}   // White
#define BLACK      (Color){ 0, 0, 0}         // Black
#define MAGENTA    (Color){ 255, 0, 255}     // Magenta
#define BLANK      (Color){ 0, 0, 0, 0 }     // Blank (Transparent)
                                                     //
typedef struct {
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
} Color;

Color HexToColor(const char* hex) {
    Color color;

    if (strlen(hex) == 7) { // Format: #RRGGBB
        sscanf(hex, "#%2hhx%2hhx%2hhx", &color.r, &color.g, &color.b);
        color.a = 255; // default alpha value
    } else if (strlen(hex) == 9) { // Format: #RRGGBBAA
        sscanf(hex, "#%2hhx%2hhx%2hhx%2hhx", &color.r, &color.g, &color.b, &color.a);
    } else {
        // Invalid format
        color = (Color){0, 0, 0, 255}; // default to black if format is not recognized
    }

    return color;
}

    // Primitives

        void DrawPixel(int x, int y, Color color) {
            if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
                return;
            }
            if (color.a == 0) { color.a = 255; }
            int index = (y * SCREEN_WIDTH + x) * 4;
            float alpha = color.a / 255.0f;
            float invAlpha = 1.0f - alpha;
            framebuffer[index]     = (GLubyte)(color.r * alpha + framebuffer[index]     * invAlpha);
            framebuffer[index + 1] = (GLubyte)(color.g * alpha + framebuffer[index + 1] * invAlpha);
            framebuffer[index + 2] = (GLubyte)(color.b * alpha + framebuffer[index + 2] * invAlpha);
            framebuffer[index + 3] = color.a;
        }

        void DrawRect(int x, int y, int width, int height, Color color) {
            if (color.a == 0) { color.a = 255; }
            for (int i = x; i < x + width; i++) {
                for (int j = y; j < y + height; j++) {
                    DrawPixel(i, j, (Color){color.r, color.g, color.b, color.a});
                }
            }
        }

        void DrawLine(int x0, int y0, int x1, int y1, Color color, int thickness) {
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

        // GL Drawing "gpu"

            void ClearColor(Color color) {
                if (color.a == 0) { color.a = 255; }
                glClearColor((GLclampf)color.r/255.0f, (GLclampf)color.g/255.0f, (GLclampf)color.b/255.0f, (GLclampf)color.a/255.0f);
            }

            void DrawRectGL(float x, float y, float width, float height, Color color) {
                if (color.a == 0) { color.a = 255; }
                glColor4f(color.r/255.0f, color.g/255.0f, color.b/255.0f, color.a/255.0f);
                glBegin(GL_QUADS);
                    glVertex2f(x, y);
                    glVertex2f(x + width, y);
                    glVertex2f(x + width, y + height);
                    glVertex2f(x, y + height);
                glEnd();
            }
    
    // TEXT

        #include <ft2build.h>
        #include FT_FREETYPE_H

        typedef struct {
            FT_Face face;
            FT_Library library;
        } Font;

        Font LoadFont(const char* fontPath) {
            Font font;
            if (FT_Init_FreeType(&font.library)) {
                font.face = NULL;
                return font;
            }
            if (FT_New_Face(font.library, fontPath, 0, &font.face)) {
                FT_Done_FreeType(font.library);
                font.face = NULL;
                return font;
            }
            return font;
        }

        void DrawText(int x, int y, Font font, int fontSize, const char* text, Color color) {
            if (color.a == 0) { color.a = 255; }
            if (!font.face) {
                return;
            }
            FT_Set_Pixel_Sizes(font.face, 0, fontSize);
            int pen_x = x;
            int pen_y = y;
            for (int i = 0; i < strlen(text); i++) {
                if (FT_Load_Char(font.face, text[i], FT_LOAD_RENDER)) {
                    continue;
                }
                FT_Bitmap* bitmap = &font.face->glyph->bitmap;
                FT_GlyphSlot slot = font.face->glyph;
                for (int row = 0; row < bitmap->rows; ++row) {
                    for (int col = 0; col < bitmap->width; ++col) {
                        int pixel = row * bitmap->pitch + col;
                        GLubyte alpha = bitmap->buffer[pixel];
                        if (alpha > 0) {
                            int adjusted_x = pen_x + col + slot->bitmap_left;
                            int adjusted_y = pen_y - row - (fontSize - slot->bitmap_top);
                            DrawPixel(adjusted_x, adjusted_y, (Color){color.r, color.g, color.b, color.a});
                        }
                    }
                }
                pen_x += slot->advance.x >> 6;
            }
        }
        
        typedef struct {
            int width;
            int height;
        } TextSize;

        TextSize GetTextSize(Font font, int fontSize, const char* text) {
            if (!font.face) {
                return (TextSize){0, 0};
            }
            FT_Set_Pixel_Sizes(font.face, 0, fontSize);
            int width = 0;
            int max_height = 0;
            for (int i = 0; i < strlen(text); i++) {
                if (FT_Load_Char(font.face, text[i], FT_LOAD_RENDER)) {
                    continue;
                }
                FT_GlyphSlot slot = font.face->glyph;
                width += slot->advance.x >> 6;
                int glyphHeight = slot->bitmap_top;
                if(glyphHeight > max_height) {
                    max_height = glyphHeight;
                }
            }
            return (TextSize){width, max_height};
        }

    // IMAGE
        #define STB_IMAGE_IMPLEMENTATION
        #include "stb_image.h"

        typedef struct {
            unsigned char* data;
            int width, height;
            int channels;
        } Image;

        Image img;

        Image LoadImage(const char* filepath) {
            img.data = stbi_load(filepath, &img.width, &img.height, &img.channels, STBI_rgb_alpha); // 4 for RGBA
            if (!img.data) {
                img.width = 0;
                img.height = 0;
                img.channels = 0;
            }
            return img;
        }

        void DrawImage(int x, int y, Image img) {
            if (!img.data) return;
            for (int j = 0; j < img.height; j++) {
                for (int i = 0; i < img.width; i++) {
                    int pixelIndex = (j * img.width + i) * img.channels;
                    GLubyte r = img.data[pixelIndex];
                    GLubyte g = img.data[pixelIndex + 1];
                    GLubyte b = img.data[pixelIndex + 2];
                    GLubyte a = (img.channels == 4) ? img.data[pixelIndex + 3] : 255;
                    if (a > 0) {
                        DrawPixel(x + i, y + (img.height - 1 - j), (Color){r, g, b, a});
                    }
                }
            }
        }

        void DrawImageTilled(Image img) {
            if (!img.data) return;
            for (int screenX = 0; screenX < SCREEN_WIDTH; screenX += img.width) {
                for (int screenY = 0; screenY < SCREEN_HEIGHT; screenY += img.height) {
                    DrawImage(screenX, screenY, img);
                }
            }
        }

        // TEXTURE

            typedef struct {
                GLuint raw;           // OpenGL texture ID
                unsigned char* data;  // Image data
                int width, height;    // Texture dimensions
                int channels;         // Number of channels (e.g., RGB, RGBA)
            } Texture;

            Texture LoadTexture(const char *filename) {
                Texture texture;
                texture.data = stbi_load(filename, &texture.width, &texture.height, &texture.channels, STBI_rgb_alpha);
                if (texture.data == NULL) {
                    texture.raw = 0;
                    return texture;
                }
                glGenTextures(1, &texture.raw);
                glBindTexture(GL_TEXTURE_2D, texture.raw);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                stbi_image_free(texture.data);
                texture.data = NULL;
                return texture;
            }

            Texture LoadTextureFromImage(Image img) {
                Texture texture;
                if (img.data == NULL) {
                    texture.raw = 0;
                    return texture;
                }
                texture.width = img.width;
                texture.height = img.height;
                texture.channels = img.channels;
                glGenTextures(1, &texture.raw);
                glBindTexture(GL_TEXTURE_2D, texture.raw);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                return texture;
            }

            void DrawTexture(Texture texture, float x, float y, float width, float height, GLfloat angleInDegrees) {
                glBindTexture(GL_TEXTURE_2D, texture.raw);

                glPushMatrix();
                float centerX = x + width * 0.5f;
                float centerY = y + height * 0.5f;
                glTranslatef(centerX, centerY, 0);
                glRotatef(angleInDegrees, 0, 0, 1);
                glTranslatef(-centerX, -centerY, 0);

                glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
                    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y);
                    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height);
                    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height);
                glEnd();

                glPopMatrix(); // Restore the matrix state

                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY); 
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

        
        // Draw Cube 3D

            bool EnableColor = false;
            bool EnableTexture = true;
            void DrawCube(Texture texture) {
                glBindTexture(GL_TEXTURE_2D, texture.raw);
                float kh=1.0;
                GLfloat vertices[] =
                {
                //координаты X, Y, Z каждой из 4 точек каждой из 6 граней куба
                    -1, -1, -1,        -1,  -1,  1,        -kh,  1,  kh,  		-kh,  1, -kh, //1 грань куба
                    1, -1, -1,  	    1,  -1,  1,		    kh,  1,  kh,  		 kh,  1, -kh,
                    -1, -1, -1,  	   -1,  -1,  1,		    1,  -1,  1,		     1, -1, -1,
                    -kh,  1, -kh, 	   -kh,  1,  kh, 		kh,  1,  kh,         kh,  1, -kh,
                    -1, -1, -1, 	   -kh,  1, -kh, 		kh,  1, -kh,         1, -1, -1,
                    -1, -1,  1, 	   -kh,  1,  kh,  		kh,  1,  kh,         1, -1,  1
                };
                GLfloat colors[] =
                //R, G ,B цвет
                {
                    0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1, //1 грань  - синяя
                    1, 1, 0,   1, 1, 0,   1, 1, 0,   1, 1, 0, //2 грань  - желтая
                    0, 1, 1,   0, 1, 1,   0, 1, 1,   0, 1, 1, //3 грань (дно) - голубая
                    1, 0, 1,   1, 0, 1,   1, 0, 1,   1, 0, 1, //4 грань (вершина - пурпурная
                    0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0, //5 грань  - зеленая
                    1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0, //6 грань  - красная
                };
                //двумерные текстурные координаты
                GLfloat textures[]=
                {
                0.0, 0.0,	1.0, 0.0,	1.0, 1.0,	0.0, 1.0,	//текстурные координаты 1 грани 
                0.0, 0.0,	1.0, 0.0,	1.0, 1.0,	0.0, 1.0,	//текстурные координаты 2 грани 
                0.0, 0.0,	1.0, 0.0,	1.0, 1.0,	0.0, 1.0,	//текстурные координаты 3 грани 
                0.0, 0.0,	1.0, 0.0,	1.0, 1.0,	0.0, 1.0,	//текстурные координаты 4 грани 
                0.0, 0.0,	1.0, 0.0,	1.0, 1.0,	0.0, 1.0,	//текстурные координаты 5 грани 
                0.0, 0.0,	1.0, 0.0,	1.0, 1.0,	0.0, 1.0,	//текстурные координаты 6 грани 
                };
                /*  indices of front, top, left, bottom, right, back faces  */
                GLubyte indices[] = 
                {
                0, 1, 2, 3,
                4, 5, 6, 7,
                8, 9, 10, 11,
                12, 13, 14, 15,
                16, 17, 18, 19,
                20, 21, 22, 23,
                };
                static float alpha = 0;
                //Move cube
                glTranslatef(0,0,-10);
                //attempt to rotate cube
                glScalef(sin(frametime), sin(frametime), sin(frametime));
                glRotatef(alpha , 1, 1.3, -0.5);
                /* We have a vertex, color and a texture array */
                glEnableClientState(GL_VERTEX_ARRAY);
                if (EnableColor) glEnableClientState(GL_COLOR_ARRAY);	// enable or disable color
                if (EnableTexture) {
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY); 
                    glEnable(GL_TEXTURE_2D);
                } else {
                    glDisable(GL_TEXTURE_2D);
                }
                glVertexPointer(3, GL_FLOAT, 0, vertices);
                glColorPointer(3, GL_FLOAT, 0, colors);
                glTexCoordPointer(2, GL_FLOAT, 0, textures);
                /* Draw */
                // Send data : 24 vertices 
                //glDrawArrays(GL_QUADS, 0, 24);
                glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, indices);
                /* Cleanup states */
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_COLOR_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);
                alpha += 1;
                glBindTexture(GL_TEXTURE_2D, 0);
            }

    // Camera

        #include <math.h>
        void PerspectiveCam(GLfloat fovY, GLfloat aspect, GLfloat zNear, GLfloat zFar) {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            GLfloat f = 1.0f / tan(fovY / 2.0f);
            GLfloat m[16];
            m[0]  = f / aspect; m[4]  = 0.0f;
            m[1]  = 0.0f;       m[5]  = f;
            m[2]  = 0.0f;       m[6]  = 0.0f;
            m[3]  = 0.0f;       m[7]  = 0.0f;

            m[8]  = 0.0f;
            m[9]  = 0.0f;
            m[10] = (zFar + zNear) / (zNear - zFar);
            m[11] = -1.0f;

            m[12] = 0.0f;
            m[13] = 0.0f;
            m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
            m[15] = 0.0f;
            glLoadMatrixf(m);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
        }

        void OrthoCam(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(left, right, bottom, top, zNear, zFar);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
        }

#endif // DRAW_H
