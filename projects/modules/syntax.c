
#include <ctype.h>
#include <dlfcn.h>
#include <dirent.h>
#include <tree_sitter/api.h>

// Compile with $(pkg-config --libs tree-sitter)

typedef struct {
    unsigned char r, g, b, a;
} SyntaxColor;

typedef void (*DrawTextFunc)(float x, float y, void* font, float fontSize, const char* text, SyntaxColor color, void* userData);

typedef struct {
    const char *name;                 // Language name
    const char *extensions;           // File extensions (comma-separated)
    void *handle;                     // Library handle
    const TSLanguage *(*func)(void);  // Function pointer to get language
} LanguageLib;

typedef struct {
    const char *nodeName;
    SyntaxColor color;
} ColorMap;

typedef struct {
    SyntaxColor *colors;         // Color array for text
    int maxColors;               // Max size of color array
    bool initialized;            // Initialization flag
    TSParser *parser;            // Tree-sitter parser
    TSTree *tree;                // Parsed syntax tree
    char *lastText;              // Last parsed text cache
    int lastTextLen;             // Length of cached text
    const char *language;        // Current language name
    LanguageLib *languages;      // Available languages
    int numLanguages;            // Number of languages
    SyntaxColor defaultColor;    // Default text color
    DrawTextFunc drawFunc;       // Function to draw text
    void* fontPtr;               // Font pointer for drawing
    float fontSize;              // Font size for drawing
    void *userData;              // User data for draw function
    ColorMap *colorMap;          // Node type to color mapping
    int numColors;               // Number of color mappings
    bool debugMode;              // Enable debug output
} SyntaxHighlighter;

static SyntaxHighlighter highlighter = {0};

#define MAX_LANGUAGES 30
static LanguageLib defaultLanguages[MAX_LANGUAGES] = {
    {"c", ".c,.h", NULL, NULL},
    {"cpp", ".cpp,.hpp,.cc,.hh,.cxx,.hxx", NULL, NULL},
    {"python", ".py,.pyw", NULL, NULL},
    {"javascript", ".js,.jsx,.mjs", NULL, NULL},
    {"typescript", ".ts,.tsx", NULL, NULL},
    {"rust", ".rs", NULL, NULL},
    {"bash", ".sh,.bash", NULL, NULL},
    {"lua", ".lua", NULL, NULL},
    {"markdown", ".md,.markdown", NULL, NULL},
    {"vim", ".vim", NULL, NULL},
    {"css", ".css", NULL, NULL},
    {"html", ".html,.htm", NULL, NULL},
    {"json", ".json", NULL, NULL},
    {"go", ".go", NULL, NULL},
    {"java", ".java", NULL, NULL},
    {"php", ".php", NULL, NULL},
    {"ruby", ".rb", NULL, NULL},
    {"scala", ".scala", NULL, NULL},
    {"swift", ".swift", NULL, NULL},
    {"yaml", ".yml,.yaml", NULL, NULL},
    {"toml", ".toml", NULL, NULL},
    {"ocaml", ".ml,.mli", NULL, NULL},
    {"haskell", ".hs", NULL, NULL},
    {"elixir", ".ex,.exs", NULL, NULL},
    {"erlang", ".erl", NULL, NULL},
    {"clojure", ".clj,.cljs", NULL, NULL},
    {"sql", ".sql", NULL, NULL},
    {"zig", ".zig", NULL, NULL},
    {"nix", ".nix", NULL, NULL},
    {"kotlin", ".kt,.kts", NULL, NULL}
};

#define MAX_COLOR_MAPPINGS 50
static ColorMap defaultColorMap[MAX_COLOR_MAPPINGS] = {
    {"comment", {100, 200, 100, 255}},             // Green for comments
    {"string", {230, 180, 80, 255}},               // Yellow for strings
    {"raw_string", {230, 180, 80, 255}},           // Yellow for raw strings
    {"string_literal", {230, 180, 80, 255}},       // Yellow for string literals
    {"string_content", {230, 180, 80, 255}},       // Yellow for string content
    {"escape_sequence", {240, 150, 120, 255}},     // Orange for escape sequences
    {"char", {230, 180, 80, 255}},                 // Yellow for chars
    {"character", {230, 180, 80, 255}},            // Yellow for characters
    {"number", {240, 150, 120, 255}},              // Orange for numbers
    {"float", {240, 150, 120, 255}},               // Orange for floats
    {"integer", {240, 150, 120, 255}},             // Orange for integers
    {"boolean", {240, 150, 120, 255}},             // Orange for booleans
    {"keyword", {130, 180, 255, 255}},             // Blue for keywords
    {"type", {100, 200, 230, 255}},                // Cyan for types
    {"type_identifier", {100, 200, 230, 255}},     // Cyan for type identifiers
    {"primitive_type", {100, 200, 230, 255}},      // Cyan for primitive types
    {"function", {220, 150, 220, 255}},            // Purple for functions
    {"function_definition", {220, 150, 220, 255}}, // Purple for function definitions
    {"function_call", {220, 150, 220, 255}},       // Purple for function calls
    {"method", {220, 150, 220, 255}},              // Purple for methods
    {"method_call", {220, 150, 220, 255}},         // Purple for method calls
    {"property", {200, 200, 160, 255}},            // Tan for properties
    {"field", {200, 200, 160, 255}},               // Tan for fields
    {"variable", {220, 220, 220, 255}},            // White for variables
    {"identifier", {220, 220, 220, 255}},          // White for identifiers
    {"operator", {230, 150, 150, 255}},            // Red for operators
    {"punctuation", {200, 200, 200, 255}},         // Light grey for punctuation
    {"delimiter", {200, 200, 200, 255}},           // Light grey for delimiters
    {"bracket", {200, 200, 200, 255}},             // Light grey for brackets
    {"tag", {150, 220, 180, 255}},                 // Green for tags (HTML/XML)
    {"attribute", {180, 200, 140, 255}},           // Olive for attributes
    {"special", {255, 128, 128, 255}},             // Bright red for special items
    {"error", {255, 100, 100, 255}},               // Red for errors
    {"preprocessor", {200, 140, 140, 255}},        // Pinkish for preprocessor
    {"include", {200, 140, 140, 255}},             // Pinkish for includes
    {"define", {200, 140, 140, 255}},              // Pinkish for defines
    {"directive", {200, 140, 140, 255}},           // Pinkish for directives
    {"macro", {200, 140, 140, 255}},               // Pinkish for macros
    {"constant", {180, 180, 240, 255}},            // Light blue for constants
    {"namespace", {180, 210, 210, 255}},           // Light cyan for namespaces
    {"class", {100, 200, 230, 255}},               // Cyan for classes
    {"decorator", {190, 180, 220, 255}},           // Light purple for decorators
    {"annotation", {190, 180, 220, 255}},          // Light purple for annotations
    {"parameter", {210, 190, 170, 255}},           // Tan for parameters
    {"argument", {210, 190, 170, 255}},            // Tan for arguments
    {"regex", {210, 160, 200, 255}},               // Pink for regex
    {"regexp", {210, 160, 200, 255}},              // Pink for regexp
    {"symbol", {200, 200, 120, 255}},              // Yellow for symbols
    {"label", {200, 200, 120, 255}},               // Yellow for labels
    {NULL, {220, 220, 220, 255}}                   // Default text color (terminator)
};

#define COPY_COLOR(dst, src) do { \
    (dst).r = (src).r; \
    (dst).g = (src).g; \
    (dst).b = (src).b; \
    (dst).a = (src).a; \
} while(0)

static bool LoadLanguage(LanguageLib *lib) {
    if (lib->handle) return true;
    printf("Attempting to load language: %s\n", lib->name);
    const char *paths[] = {
        "/usr/lib/tree-sitter/libtree-sitter-%s.so",
        "/usr/local/lib/tree-sitter/libtree-sitter-%s.so",
        "./libtree-sitter-%s.so",
        "./modules/libtree-sitter-%s.so",
        "../modules/libtree-sitter-%s.so",
        "libtree-sitter-%s.so"
    };
    for (int i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        char libPath[512];
        snprintf(libPath, sizeof(libPath), paths[i], lib->name);
        printf("  Trying: %s\n", libPath);
        lib->handle = dlopen(libPath, RTLD_LAZY);
        if (lib->handle) {
            printf("  Successfully loaded: %s\n", libPath);
            break;
        }
    }
    if (!lib->handle) {
        printf("  ERROR: Failed to load library for %s: %s\n", lib->name, dlerror());
        return false;
    }
    char funcName[128];
    snprintf(funcName, sizeof(funcName), "tree_sitter_%s", lib->name);
    printf("  Looking for function: %s\n", funcName);
    lib->func = (const TSLanguage *(*)(void))dlsym(lib->handle, funcName);
    if (!lib->func) {
        printf("  ERROR: Failed to find function %s: %s\n", funcName, dlerror());
        dlclose(lib->handle);
        lib->handle = NULL;
        return false;
    }
    printf("  SUCCESS: Language %s loaded and ready\n", lib->name);
    return true;
}

static LanguageLib *FindLanguageByExtension(const char *filename) {
    if (!filename) return NULL;
    const char *ext = strrchr(filename, '.');
    if (!ext) return NULL;
    for (int i = 0; i < highlighter.numLanguages; i++) {
        LanguageLib *lib = &highlighter.languages[i];
        if (!lib->name) continue;
        char extensions[128];
        strncpy(extensions, lib->extensions, sizeof(extensions));
        extensions[sizeof(extensions) - 1] = '\0';
        char *saveptr;
        char *token = strtok_r(extensions, ",", &saveptr);
        while (token) {
            if (strcasecmp(ext, token) == 0) {
                return lib;
            }
            token = strtok_r(NULL, ",", &saveptr);
        }
    }
    return NULL;
}

static SyntaxColor GetColorForNodeType(const char *type) {
    if (!type) return highlighter.defaultColor;
    for (int i = 0; i < highlighter.numColors; i++) {
        if (!highlighter.colorMap[i].nodeName) break;
        if (strstr(type, highlighter.colorMap[i].nodeName)) {
            return highlighter.colorMap[i].color;
        }
    }
    return highlighter.defaultColor;
}

static void ApplyNodeHighlighting(TSNode node, int startOffset) {
    if (ts_node_is_null(node)) return;
    const char *type = ts_node_type(node);
    SyntaxColor color = GetColorForNodeType(type);
    uint32_t startByte = ts_node_start_byte(node);
    uint32_t endByte = ts_node_end_byte(node);
    for (uint32_t i = startByte; i < endByte && i < highlighter.lastTextLen; i++) {
        highlighter.colors[i] = color;
    }
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        ApplyNodeHighlighting(child, startOffset);
    }
}

bool SyntaxInit(SyntaxColor defaultColor, DrawTextFunc drawFunc, void* fontPtr, float fontSize, void *userData) {
    if (highlighter.initialized) return true;
    highlighter.parser = ts_parser_new();
    if (!highlighter.parser) return false;
    highlighter.defaultColor = defaultColor;
    highlighter.drawFunc = drawFunc;
    highlighter.fontPtr = fontPtr;
    highlighter.fontSize = fontSize;
    highlighter.userData = userData;
    highlighter.debugMode = false;
    highlighter.maxColors = 65536;
    highlighter.colors = (SyntaxColor*)calloc(highlighter.maxColors, sizeof(SyntaxColor));
    if (!highlighter.colors) {
        ts_parser_delete(highlighter.parser);
        return false;
    }
    for (int i = 0; i < highlighter.maxColors; i++) {
        COPY_COLOR(highlighter.colors[i], defaultColor);
    }
    highlighter.numLanguages = MAX_LANGUAGES;
    highlighter.languages = (LanguageLib*)calloc(highlighter.numLanguages, sizeof(LanguageLib));
    if (!highlighter.languages) {
        free(highlighter.colors);
        ts_parser_delete(highlighter.parser);
        return false;
    }
    for (int i = 0; i < MAX_LANGUAGES; i++) {
        highlighter.languages[i].name = defaultLanguages[i].name;
        highlighter.languages[i].extensions = defaultLanguages[i].extensions;
        highlighter.languages[i].handle = NULL;
        highlighter.languages[i].func = NULL;
    }
    int colorCount = 0;
    while (colorCount < MAX_COLOR_MAPPINGS && defaultColorMap[colorCount].nodeName) {
        colorCount++;
    }
    colorCount++;
    highlighter.colorMap = (ColorMap*)calloc(colorCount, sizeof(ColorMap));
    if (!highlighter.colorMap) {
        free(highlighter.languages);
        free(highlighter.colors);
        ts_parser_delete(highlighter.parser);
        return false;
    }
    for (int i = 0; i < colorCount; i++) {
        highlighter.colorMap[i].nodeName = defaultColorMap[i].nodeName;
        highlighter.colorMap[i].color = defaultColorMap[i].color;
    }
    highlighter.numColors = colorCount;
    highlighter.initialized = true;
    return true;
}

void SyntaxCleanup() {
    if (!highlighter.initialized) return;
    if (highlighter.tree) {
        ts_tree_delete(highlighter.tree);
        highlighter.tree = NULL;
    }
    if (highlighter.parser) {
        ts_parser_delete(highlighter.parser);
        highlighter.parser = NULL;
    }
    if (highlighter.lastText) {
        free(highlighter.lastText);
        highlighter.lastText = NULL;
    }
    if (highlighter.colors) {
        free(highlighter.colors);
        highlighter.colors = NULL;
    }
    if (highlighter.colorMap) {
        free(highlighter.colorMap);
        highlighter.colorMap = NULL;
    }
    if (highlighter.languages) {
        for (int i = 0; i < highlighter.numLanguages; i++) {
            if (highlighter.languages[i].handle) {
                dlclose(highlighter.languages[i].handle);
                highlighter.languages[i].handle = NULL;
            }
        }
        free(highlighter.languages);
        highlighter.languages = NULL;
    }
    highlighter.initialized = false;
}

bool SyntaxSetLanguageFromFile(const char *filename) {
    if (!highlighter.initialized || !filename) return false;
    LanguageLib *lib = FindLanguageByExtension(filename);
    if (lib && LoadLanguage(lib)) {
        ts_parser_set_language(highlighter.parser, lib->func());
        highlighter.language = lib->name;
        if (highlighter.tree) {
            ts_tree_delete(highlighter.tree);
            highlighter.tree = NULL;
        }
        return true;
    }
    return false;
}

bool SyntaxSetLanguage(const char *langName) {
    if (!highlighter.initialized) return false;
    if (!langName) {
        highlighter.language = NULL;
        return true;
    }
    for (int i = 0; i < highlighter.numLanguages; i++) {
        LanguageLib *lib = &highlighter.languages[i];
        if (!lib->name) continue;
        if (strcasecmp(lib->name, langName) == 0) {
            if (LoadLanguage(lib)) {
                ts_parser_set_language(highlighter.parser, lib->func());
                highlighter.language = lib->name;
                if (highlighter.tree) {
                    ts_tree_delete(highlighter.tree);
                    highlighter.tree = NULL;
                }
                return true;
            }
            break;
        }
    }
    return false;
}

bool SyntaxCycleLanguage() {
    if (!highlighter.initialized) return false;
    int currentIdx = -1;
    for (int i = 0; i < highlighter.numLanguages; i++) {
        if (highlighter.languages[i].name && highlighter.language && 
            strcasecmp(highlighter.languages[i].name, highlighter.language) == 0) {
            currentIdx = i;
            break;
        }
    }
    for (int i = 1; i <= highlighter.numLanguages; i++) {
        int idx = (currentIdx + i) % highlighter.numLanguages;
        if (highlighter.languages[idx].name && LoadLanguage(&highlighter.languages[idx])) {
            ts_parser_set_language(highlighter.parser, highlighter.languages[idx].func());
            highlighter.language = highlighter.languages[idx].name;
            if (highlighter.tree) {
                ts_tree_delete(highlighter.tree);
                highlighter.tree = NULL;
            }
            
            return true;
        }
    }
    highlighter.language = NULL;
    return false;
}

void SyntaxUpdate(const char *text, int textLen) {
    printf("SyntaxUpdate called with textLen: %d\n", textLen);
    if (!highlighter.initialized) {
        printf("  Error: Highlighter not initialized\n");
        return;
    }
    if (!text || textLen <= 0) {
        printf("  Error: Invalid text data\n");
        return;
    }
    if (!highlighter.language) {
        printf("  No language set, skipping parse\n");
        if (highlighter.tree) {
            ts_tree_delete(highlighter.tree);
            highlighter.tree = NULL;
        }
        return;
    }
    printf("  Language: %s\n", highlighter.language);
    if (textLen > highlighter.maxColors) {
        printf("  Reallocating colors array to %d bytes\n", textLen);
        SyntaxColor *newColors = (SyntaxColor*)realloc(highlighter.colors, textLen * sizeof(SyntaxColor));
        if (!newColors) {
            printf("  Error: Failed to reallocate colors array\n");
            return;
        }
        highlighter.colors = newColors;
        highlighter.maxColors = textLen;
        for (int i = 0; i < textLen; i++) {
            COPY_COLOR(highlighter.colors[i], highlighter.defaultColor);
        }
    }
    bool needsReparse = highlighter.lastText == NULL || textLen != highlighter.lastTextLen || memcmp(text, highlighter.lastText, textLen) != 0;
    printf("  Needs reparse: %s\n", needsReparse ? "Yes" : "No");
    if (needsReparse) {
        if (highlighter.tree) {
            printf("  Deleting old syntax tree\n");
            ts_tree_delete(highlighter.tree);
            highlighter.tree = NULL;
        }
        if (highlighter.lastText) {
            free(highlighter.lastText);
            highlighter.lastText = NULL;
        }
        highlighter.lastText = (char*)malloc(textLen + 1);
        if (!highlighter.lastText) {
            printf("  Error: Failed to allocate memory for text\n");
            return;
        }
        memcpy(highlighter.lastText, text, textLen);
        highlighter.lastText[textLen] = '\0';
        highlighter.lastTextLen = textLen;
        for (int i = 0; i < textLen; i++) {
            COPY_COLOR(highlighter.colors[i], highlighter.defaultColor);
        }
        if (!highlighter.parser) {
            printf("  Error: No parser available\n");
            return;
        }
        bool languageLoaded = false;
        for (int i = 0; i < highlighter.numLanguages; i++) {
            if (highlighter.languages[i].name && 
                strcmp(highlighter.languages[i].name, highlighter.language) == 0) {
                if (LoadLanguage(&highlighter.languages[i])) {
                    languageLoaded = true;
                    printf("  Language loaded successfully\n");
                    ts_parser_set_language(highlighter.parser, highlighter.languages[i].func());
                    break;
                }
            }
        }
        if (!languageLoaded) {
            printf("  Error: Failed to load language: %s\n", highlighter.language);
            return;
        }
        printf("  Parsing text...\n");
        highlighter.tree = ts_parser_parse_string(highlighter.parser, NULL, text, textLen);
        if (!highlighter.tree) {
            printf("  Error: Failed to parse text\n");
            return;
        }
        printf("  Parse successful, applying highlighting\n");
        TSNode rootNode = ts_tree_root_node(highlighter.tree);
        if (ts_node_is_null(rootNode)) {
            printf("  Error: Root node is null\n");
            return;
        }
        ApplyNodeHighlighting(rootNode, 0);
        printf("  Highlighting applied\n");
    }
}

bool SyntaxCheckLanguageAvailability(const char *langName) {
    printf("Checking language availability: %s\n", langName);
    for (int i = 0; i < highlighter.numLanguages; i++) {
        if (highlighter.languages[i].name && 
            strcmp(highlighter.languages[i].name, langName) == 0) {
            bool success = LoadLanguage(&highlighter.languages[i]);
            printf("Language %s is %s\n", langName, success ? "available" : "not available");
            if (success && highlighter.languages[i].handle) {
                dlclose(highlighter.languages[i].handle);
                highlighter.languages[i].handle = NULL;
                highlighter.languages[i].func = NULL;
            }
            return success;
        }
    }
    printf("Language %s not found in available languages\n", langName);
    return false;
}

void SyntaxFindLanguageLibraries() {
    const char *searchPaths[] = {
        "/usr/lib/tree-sitter/",
        "/usr/local/lib/tree-sitter/",
        "./",
        "./modules/",
        "../modules/"
    };
    for (int i = 0; i < sizeof(searchPaths)/sizeof(searchPaths[0]); i++) {
        for (int j = 0; j < highlighter.numLanguages; j++) {
            const char *lang = highlighter.languages[j].name;
            if (!lang) continue;
            char libPath[512];
            snprintf(libPath, sizeof(libPath), "%slibtree-sitter-%s.so", searchPaths[i], lang);
            FILE *f = fopen(libPath, "rb");
            if (f) {
                printf("  Found: %s\n", libPath);
                fclose(f);
            }
        }
    }
}
void SyntaxDrawText(float x, float y, const char *text, int length, int startPos) {
    if (!highlighter.initialized || !highlighter.drawFunc || !text || length <= 0) return;
    if ((startPos < 0 || startPos >= highlighter.lastTextLen)||(!highlighter.language || !highlighter.tree)) {
        highlighter.drawFunc(x, y, highlighter.fontPtr, highlighter.fontSize, text, highlighter.defaultColor, highlighter.userData);
        return;
    }
    Font *font = (Font*)highlighter.fontPtr;
    float scale = highlighter.fontSize / font->fontSize;
    for (int i = 0; i < length; i++) {
        SyntaxColor charColor;
        if (startPos + i < highlighter.lastTextLen) {
            charColor = highlighter.colors[startPos + i];
        } else {
            charColor = highlighter.defaultColor;
        }
        char charStr[2] = {text[i], '\0'};
        float charWidth = 0;
        unsigned char c = (unsigned char)text[i];
        if (c >= 32 && c < 32 + MAX_GLYPHS) {
            const Glyph *g = &font->glyphs[c - 32];
            charWidth = g->xadvance * scale;
        }
        highlighter.drawFunc(x, y, highlighter.fontPtr, highlighter.fontSize, charStr, charColor, highlighter.userData);
        x += charWidth;
    }
}

const char* SyntaxGetLanguage() {
    if (!highlighter.initialized) return NULL;
    return highlighter.language;
}

bool SyntaxIsEnabled() {
    if (!highlighter.initialized) return false;
    return highlighter.language != NULL;
}

void SyntaxToggle() {
    if (!highlighter.initialized) return;
    if (highlighter.language) {
        highlighter.language = NULL;
    } else {
        for (int i = 0; i < highlighter.numLanguages; i++) {
            if (highlighter.languages[i].name && LoadLanguage(&highlighter.languages[i])) {
                ts_parser_set_language(highlighter.parser, highlighter.languages[i].func());
                highlighter.language = highlighter.languages[i].name;
                break;
            }
        }
    }
}

void SyntaxSetDebug(bool debug) {
    highlighter.debugMode = debug;
}

void SyntaxSetFontSize(float size) {
    highlighter.fontSize = size;
}

void SyntaxDrawWrapper(float x, float y, void* fontPtr, float fontSize, const char* text, SyntaxColor color, void* userData) {
    Color textColor = {color.r, color.g, color.b, color.a};
    DrawTextBatch(x, y, *(Font*)fontPtr, fontSize, text, textColor);
}

static void _PrintNode(TSNode node, int depth, char* text) {
    if (ts_node_is_null(node)) return;
    for (int i = 0; i < depth; i++) printf("  ");
    const char *type = ts_node_type(node);
    uint32_t startByte = ts_node_start_byte(node);
    uint32_t endByte = ts_node_end_byte(node);
    SyntaxColor color = GetColorForNodeType(type);
    printf("%s (%d-%d) Color:[%d,%d,%d,%d] ", 
           type, startByte, endByte, 
           color.r, color.g, color.b, color.a);
    if (text && endByte > startByte) {
        printf("Text: \"");
        int excerptLen = endByte - startByte;
        if (excerptLen > 30) excerptLen = 30;
        for (int i = 0; i < excerptLen; i++) {
            char c = text[startByte + i];
            if (c == '\n') printf("\\n");
            else if (c == '\r') printf("\\r");
            else if (c == '\t') printf("\\t");
            else if (c >= 32 && c < 127) printf("%c", c);
            else printf("\\x%02x", (unsigned char)c);
        }
        if (excerptLen < endByte - startByte) printf("...");
        printf("\"");
    }
    printf("\n");
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        _PrintNode(child, depth + 1, text);
    }
}

void SyntaxDebugPrint() {
    if (!highlighter.initialized) {
        printf("Highlighter not initialized!\n");
        return;
    }
    if (!highlighter.language) {
        printf("No language set!\n");
        return;
    }
    printf("Current language: %s\n", highlighter.language);
    if (!highlighter.tree) {
        printf("No syntax tree available!\n");
        return;
    }
    if (!highlighter.lastText) {
        printf("No text available for debugging!\n");
        return;
    }
    printf("Document length: %d bytes\n", highlighter.lastTextLen);
    printf("First 10 colors in buffer:\n");
    for (int i = 0; i < 10 && i < highlighter.lastTextLen; i++) {
        SyntaxColor color = highlighter.colors[i];
        char c = highlighter.lastText[i];
        char display = (c >= 32 && c < 127) ? c : '.';
        printf("[%d] '%c' (%d) -> RGB(%d,%d,%d,%d)\n", 
               i, display, (unsigned char)c,
               color.r, color.g, color.b, color.a);
    }
    printf("\nColor map (%d entries):\n", highlighter.numColors);
    for (int i = 0; i < highlighter.numColors; i++) {
        if (!highlighter.colorMap[i].nodeName) break;
        SyntaxColor color = highlighter.colorMap[i].color;
        printf("  \"%s\" -> RGB(%d,%d,%d,%d)\n", 
               highlighter.colorMap[i].nodeName,
               color.r, color.g, color.b, color.a);
    }
    TSNode root = ts_tree_root_node(highlighter.tree);
    if (ts_node_is_null(root)) {
        printf("Root node is null!\n");
        return;
    }
    printf("\nSyntax Tree:\n");
    _PrintNode(root, 0, highlighter.lastText);
    printf("\nDraw function: %p\n", (void*)highlighter.drawFunc);
    printf("Font pointer: %p\n", highlighter.fontPtr);
    printf("Font size: %.1f\n", highlighter.fontSize);
}

bool SyntaxInitWithLanguageDetection(SyntaxColor defaultColor, DrawTextFunc drawFunc, void* fontPtr, float fontSize, void* userData, const char *filename, char *text, int textLen, char *statusMsg, size_t statusMsgSize) {
    if(!SyntaxInit(defaultColor, drawFunc, fontPtr, fontSize, userData)) return false;
    SyntaxFindLanguageLibraries();
    bool languageSet = false;
    if(filename && strlen(filename) > 0) {
        if(SyntaxSetLanguageFromFile(filename)) {
            languageSet = true;
            if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: %s", SyntaxGetLanguage());
            printf("Language set to: %s based on file extension\n", SyntaxGetLanguage());
        } else {
            if(text && textLen > 0) {
                if(strstr(filename, ".txt") && (strstr(text, "#include") || strstr(text, "int main"))) {
                    if(SyntaxSetLanguage("c")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: C (auto-detected)"); printf("Language detected as C from content\n"); }
                } else if(strstr(text, "<!DOCTYPE html") || strstr(text, "<html")) {
                    if(SyntaxSetLanguage("html")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: HTML (auto-detected)"); printf("Language detected as HTML from content\n"); }
                } else if(strstr(text, "#!/bin/bash") || strstr(text, "#!/usr/bin/env bash")) {
                    if(SyntaxSetLanguage("bash")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Bash (auto-detected)"); printf("Language detected as Bash from content\n"); }
                } else if(strstr(text, "#!/usr/bin/python") || strstr(text, "#!/usr/bin/env python")) {
                    if(SyntaxSetLanguage("python")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Python (auto-detected)"); printf("Language detected as Python from content\n"); }
                } else if(strstr(text, "package ") && strstr(text, "import ") && (strstr(text, "func ") || strstr(text, "type "))) {
                    if(SyntaxSetLanguage("go")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Go (auto-detected)"); printf("Language detected as Go from content\n"); }
                } else if(strstr(text, "fn ") && strstr(text, "-> ") && (strstr(text, "let ") || strstr(text, "mut "))) {
                    if(SyntaxSetLanguage("rust")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Rust (auto-detected)"); printf("Language detected as Rust from content\n"); }
                }
            }
        }
    }
    if(!languageSet) {
        if(SyntaxSetLanguage("c")) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: C (default)"); printf("Defaulting to C language\n"); }
        else { if(SyntaxCycleLanguage()) { languageSet = true; if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: %s (fallback)", SyntaxGetLanguage()); printf("Using fallback language: %s\n", SyntaxGetLanguage()); } else printf("Failed to set any syntax language\n"); }
    }
    if(languageSet && text && textLen > 0) SyntaxUpdate(text, textLen);
    return true;
}
