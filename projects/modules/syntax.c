#include <ctype.h>
#include <dlfcn.h>
#include <dirent.h>
#include <tree_sitter/api.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Compile with $(pkg-config --libs tree-sitter)

typedef void (*DrawTextFunc)(float x, float y, void* font, float fontSize, const char* text, Color color, void* userData);

typedef struct {
    const char *name;                 // Language name
    const char *extensions;           // File extensions (comma-separated)
    void *handle;                     // Library handle
    const TSLanguage *(*func)(void);  // Function pointer to get language
} LanguageLib;

#define FNV_PRIME_32 16777619
#define FNV_OFFSET_32 2166136261U

typedef struct {
    const char *key;
    Color value;
    uint32_t hash;
    bool used;
} NodeColorEntry;

typedef struct {
    NodeColorEntry *entries;
    size_t capacity;
    size_t size;
    float load_factor;
    size_t threshold;
} NodeColorMap;

typedef struct {
    int open;
    int close;
} BracketPair;

#define MAX_BRACKET_PAIRS 256

typedef struct {
    Color *colors;          // Color array for text
    int maxColors;          // Max size of color array
    bool initialized;       // Initialization flag
    TSParser *parser;       // Tree-sitter parser
    TSTree *tree;           // Parsed syntax tree
    char *lastText;         // Last parsed text cache
    int lastTextLen;        // Length of cached text
    const char *language;   // Current language name
    LanguageLib *languages; // Available languages
    int numLanguages;       // Number of languages
    Color defaultColor;     // Default text color
    DrawTextFunc drawFunc;  // Function to draw text
    void* fontPtr;          // Font pointer for drawing
    float fontSize;         // Font size for drawing
    void *userData;         // User data for draw function
    NodeColorMap colorMap;  // Node type to color mapping
    bool debugMode;         // Enable debug output
    
    // Bracket tracking
    BracketPair bracketPairs[256];  // Store matched bracket pairs
    int bracketPairCount;           // Number of tracked pairs
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

#define Color00 (Color){  57,  53,  80, 255 } // #393550
#define Color01 (Color){ 197, 163, 224, 255 } // #C5A3E0
#define Color02 (Color){ 144, 184, 209, 255 } // #90B8D1
#define Color03 (Color){ 154, 196, 226, 255 } // #9AC4E2
#define Color04 (Color){ 168, 241, 245, 255 } // #A8F1F5
#define Color05 (Color){ 194, 192, 235, 255 } // #C2C0EB
#define Color06 (Color){ 226, 186, 248, 255 } // #E2BAF8
#define Color07 (Color){ 218, 233, 245, 255 } // #DAE9F5
#define Color08 (Color){ 179, 195, 201, 255 } // #B3C3C9
#define Color09 (Color){ 198, 165, 225, 255 } // #C6A5E1
#define Color10 (Color){ 145, 185, 210, 255 } // #91B9D2
#define Color11 (Color){ 154, 191, 224, 255 } // #9ABFE0
#define Color12 (Color){ 169, 241, 246, 255 } // #A9F1F6
#define Color13 (Color){ 195, 194, 236, 255 } // #C3C2ED
#define Color14 (Color){ 227, 188, 249, 255 } // #E3BCF9
#define Color15 (Color){ 219, 235, 246, 255 } // #DBEBF6

#define Color16 (Color){ 80,  144, 62,  255 } // #50903E
#define Color17 (Color){ 205, 92,  92,  255 } // #CD5C5C
#define Color18 (Color){ 255, 204, 51,  255 } // #FFCC33
#define Color19 (Color){ 255, 99,  71,  255 } // #FF6347
#define Color20 (Color){ 50,  205, 50,  255 } // #32CD32
#define Color21 (Color){ 255, 69,  0,   255 } // #FF4500
#define Color22 (Color){ 70,  130, 180, 255 } // #4682B4
#define Color23 (Color){ 255, 105, 180, 255 } // #FF69B4
#define Color24 (Color){ 0,   128, 128, 255 } // #008080
#define Color25 (Color){ 138, 43,  226, 255 } // #8A2BE2
#define Color26 (Color){ 34,  139, 34,  255 } // #228B22
#define Color27 (Color){ 186, 85,  211, 255 } // #BA55D3
#define Color28 (Color){ 255, 215, 0,   255 } // #FFD700
#define Color29 (Color){ 255, 20,  147, 255 } // #FF1493
#define Color30 (Color){ 20,  18,  30,  255 } // #14121E

typedef struct {
    const char *nodeName;
    Color color;
} ColorMapping;

static const ColorMapping initialColorMappings[] = {
    {"comment", Color00},
    {"string", Color01},
    {"number", Color02},
    {"keyword", Color03},
    {"type", Color04},
    {"function", Color05},
    {"property", Color06},
    {"variable", Color07},
    {"operator", Color08},
    {"punctuation", Color09},
    {"struct", Color10},
    {"param", Color11},
    {"preproc", Color12},
    {"constant", Color13},
    {"namespace", Color14},
    {"highlight", Color15},
    {NULL, {0, 0, 0, 0}}
};

static inline uint32_t hash_string(const char *str) {
    uint32_t hash = FNV_OFFSET_32;
    for (const unsigned char *s = (const unsigned char *)str; *s; s++) {
        hash ^= *s;
        hash *= FNV_PRIME_32;
    }
    return hash;
}

static bool InitNodeColorMap(NodeColorMap *map, size_t capacity) {
    size_t actual_capacity = 16;
    while (actual_capacity < capacity) actual_capacity <<= 1;
    map->entries = (NodeColorEntry*)calloc(actual_capacity, sizeof(NodeColorEntry));
    if (!map->entries) return false;
    map->capacity = actual_capacity;
    map->size = 0;
    map->load_factor = 0.75f;
    map->threshold = (size_t)(actual_capacity * map->load_factor);
    return true;
}

static inline size_t FindSlot(NodeColorMap *map, const char *key, uint32_t hash) {
    size_t mask = map->capacity - 1;
    size_t index = hash & mask;
    while (true) {
        NodeColorEntry *entry = &map->entries[index];
        if (!entry->used) {
            return index;
        } else if (entry->hash == hash && strcmp(entry->key, key) == 0) {
            return index;
        }
        index = (index + 1) & mask;
    }
}

static bool ResizeColorMap(NodeColorMap *map, size_t new_capacity) {
    NodeColorEntry *old_entries = map->entries;
    size_t old_capacity = map->capacity;
    if (!InitNodeColorMap(map, new_capacity)) return false;
    for (size_t i = 0; i < old_capacity; i++) {
        NodeColorEntry *entry = &old_entries[i];
        if (entry->used) {
            size_t index = FindSlot(map, entry->key, entry->hash);
            map->entries[index] = *entry;
            map->size++;
        }
    }
    free(old_entries);
    return true;
}

static bool AddNodeColor(NodeColorMap *map, const char *nodeName, Color color) {
    if (!map || !nodeName) return false;
    if (map->size >= map->threshold) {
        if (!ResizeColorMap(map, map->capacity * 2)) return false;
    }
    uint32_t hash = hash_string(nodeName);
    size_t index = FindSlot(map, nodeName, hash);
    NodeColorEntry *entry = &map->entries[index];
    if (!entry->used) {
        map->size++;
    }
    entry->key = nodeName;
    entry->value = color;
    entry->hash = hash;
    entry->used = true;
    return true;
}

static Color GetNodeColor(NodeColorMap *map, const char *nodeName, Color defaultColor) {
    if (!map || !nodeName) return defaultColor;
    uint32_t hash = hash_string(nodeName);
    size_t mask = map->capacity - 1;
    size_t index = hash & mask;
    size_t startIndex = index;
    do {
        NodeColorEntry *entry = &map->entries[index];
        if (!entry->used) {
            return defaultColor;
        } else if (entry->hash == hash && strcmp(entry->key, nodeName) == 0) {
            return entry->value;
        }
        index = (index + 1) & mask;
    } while (index != startIndex);
    return defaultColor;
}

static void InitColorMap(NodeColorMap *map) {
    for (int i = 0; initialColorMappings[i].nodeName; i++) 
        AddNodeColor(map, initialColorMappings[i].nodeName, initialColorMappings[i].color);
    struct { const char *types[32]; Color color; } typeMappings[] = {
        { {"line_comment", "block_comment", "doc_comment", "documentation_comment", "javadoc", "comment_block", "comment_line", NULL}, Color00 },
        { {"string_literal", "string_content", "raw_string", "char", "character_literal", "heredoc", "template_string", "f_string", "escape_sequence", "escaped_character", NULL}, Color01 },
        { {"integer", "integer_literal", "float", "float_literal", "numeric_literal", "decimal", "hex", "octal", "binary", "boolean", "true", "false", NULL}, Color02 },
        { {"type_identifier", "primitive_type", "builtin_type", "type_definition", "template", "type_annotation", "class_definition", "trait", "protocol", NULL}, Color03 },
        { {"function_definition", "function_declaration", "function_call", "call_expression", "method_call", "lambda_expression", NULL}, Color04 },
        { {"struct", "enum", "union", "struct_definition", "enum_definition", NULL}, Color05 },
        { {"property_identifier", "field_identifier", "member_access", "member_expression", NULL}, Color06 },
        { {"variable_declaration", "var_declaration", "identifier", NULL}, Color07 },
        { {"assignment", "assignment_operator", "arithmetic_operator", "comparison_operator", "logical_operator", NULL}, Color08 },
        { {"delimiter", "bracket", "semicolon", "comma", NULL}, Color09 },
        { {"namespace_definition", "module", "package", "import", "export", "using", NULL}, Color10 },
        { {"include", "preproc_ifdef", "preproc_ifndef", "preproc_def", "preproc_if", "preproc_else", "preproc_endif", NULL}, Color11 },
        { {"public", "private", "protected", "static", "final", "abstract", "virtual", "const", "mutable", NULL}, Color12 },
        { {"function", "method", "constructor", "destructor", "lambda", "arrow_function", "call_expression", NULL}, Color05 },
        { {"property", "field", "member", "property_identifier", "field_identifier", NULL}, Color06 },
        { {"variable", "identifier", "var_declaration", "variable_definition", NULL}, Color07 },
        { {"operator", "assignment", "arithmetic_operator", "comparison_operator", "logical_operator", "bitwise_operator", NULL}, Color08 },
        { {"punctuation", "delimiter", "bracket", "semicolon", "comma", NULL}, Color09 },
        { {"argument", "parameter", "parameters", "formal_parameter", "parameter_list", "self", "this", NULL}, Color10 },
        { {"namespace", "module", "package", "import", "export", "using", NULL}, Color11 },
        { {"preprocessor", "include", "define", "ifdef", "ifndef", "endif", "preproc_directive", NULL}, Color12 },
        { {"constant", "const", "constexpr", "enum_item", "static_const", NULL}, Color13 },
        { {"macro", "directive", "annotation", "decorator", NULL}, Color14 },
        { {"highlight", "focus", "selection", NULL}, Color15 }
    };
    for (size_t i = 0; i < sizeof(typeMappings) / sizeof(typeMappings[0]); i++) 
        for (int j = 0; typeMappings[i].types[j]; j++) 
            AddNodeColor(map, typeMappings[i].types[j], typeMappings[i].color);
}

#define COPY_Color(dst, src) do { \
    (dst).r = (src).r; \
    (dst).g = (src).g; \
    (dst).b = (src).b; \
    (dst).a = (src).a; \
} while(0)

static bool LoadLanguage(LanguageLib *lib) {
    if (!lib || !lib->name) return false;
    if (lib->handle) return true;
    
    if (highlighter.debugMode) {
        printf("Loading language: %s\n", lib->name);
    }
    const char *paths[] = {
        "/usr/lib/tree-sitter/libtree-sitter-%s.so",
        "/usr/local/lib/tree-sitter/libtree-sitter-%s.so",
        "./libtree-sitter-%s.so",
        "./modules/libtree-sitter-%s.so",
        "../modules/libtree-sitter-%s.so",
        "libtree-sitter-%s.so"
    };
    bool found = false;
    for (int i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        char libPath[512];
        int result = snprintf(libPath, sizeof(libPath), paths[i], lib->name);
        if (result < 0 || result >= sizeof(libPath)) {
            continue;
        }
        lib->handle = dlopen(libPath, RTLD_LAZY);
        if (lib->handle) {
            if (highlighter.debugMode) {
                printf("  Successfully loaded: %s\n", libPath);
            }
            found = true;
            break;
        }
    }
    if (!found) {
        if (highlighter.debugMode) {
            printf("  Failed to load library for %s: %s\n", lib->name, dlerror());
        }
        return false;
    }
    char funcName[128];
    int result = snprintf(funcName, sizeof(funcName), "tree_sitter_%s", lib->name);
    if (result < 0 || result >= sizeof(funcName)) {
        if (highlighter.debugMode) {
            printf("  Function name too long for %s\n", lib->name);
        }
        dlclose(lib->handle);
        lib->handle = NULL;
        return false;
    }
    lib->func = (const TSLanguage *(*)(void))dlsym(lib->handle, funcName);
    if (!lib->func) {
        if (highlighter.debugMode) {
            printf("  Failed to find function %s: %s\n", funcName, dlerror());
        }
        dlclose(lib->handle);
        lib->handle = NULL;
        return false;
    }
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
        strncpy(extensions, lib->extensions, sizeof(extensions) - 1);
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

static Color GetColorForNodeType(const char *type) {
    if (!type) return highlighter.defaultColor;
    Color color = GetNodeColor(&highlighter.colorMap, type, highlighter.defaultColor);
    if (memcmp(&color, &highlighter.defaultColor, sizeof(Color)) != 0) {
        return color;
    }
    size_t mask = highlighter.colorMap.capacity - 1;
    for (size_t i = 0; i < highlighter.colorMap.capacity; i++) {
        NodeColorEntry *entry = &highlighter.colorMap.entries[i];
        if (entry->used && entry->key && strstr(type, entry->key)) {
            return entry->value;
        }
    }
    return highlighter.defaultColor;
}

static void ApplyNodeHighlighting(TSNode node) {
    if (ts_node_is_null(node)) return;
    const char *type = ts_node_type(node);
    Color color = GetColorForNodeType(type);
    uint32_t startByte = ts_node_start_byte(node);
    uint32_t endByte = ts_node_end_byte(node);
    for (uint32_t i = startByte; i < endByte && i < highlighter.lastTextLen; i++) {
        highlighter.colors[i] = color;
    }
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        ApplyNodeHighlighting(child);
    }
}

bool SyntaxInit(Color defaultColor, DrawTextFunc drawFunc, void* fontPtr, float fontSize, void *userData) {
    if (highlighter.initialized) return true;
    highlighter.parser = ts_parser_new();
    if (!highlighter.parser) {
        fprintf(stderr, "Error: Failed to create tree-sitter parser\n");
        return false;
    }
    highlighter.defaultColor = defaultColor;
    highlighter.drawFunc = drawFunc;
    highlighter.fontPtr = fontPtr;
    highlighter.fontSize = fontSize;
    highlighter.userData = userData;
    highlighter.debugMode = false;
    highlighter.maxColors = 65536;
    highlighter.colors = (Color*)calloc(highlighter.maxColors, sizeof(Color));
    if (!highlighter.colors) {
        fprintf(stderr, "Error: Failed to allocate color buffer\n");
        ts_parser_delete(highlighter.parser);
        return false;
    }
    for (int i = 0; i < highlighter.maxColors; i++) {
        COPY_Color(highlighter.colors[i], defaultColor);
    }
    highlighter.numLanguages = MAX_LANGUAGES;
    highlighter.languages = (LanguageLib*)calloc(highlighter.numLanguages, sizeof(LanguageLib));
    if (!highlighter.languages) {
        fprintf(stderr, "Error: Failed to allocate language list\n");
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
    if (!InitNodeColorMap(&highlighter.colorMap, 512)) {
        fprintf(stderr, "Error: Failed to initialize color map\n");
        free(highlighter.languages);
        free(highlighter.colors);
        ts_parser_delete(highlighter.parser);
        return false;
    }
    InitColorMap(&highlighter.colorMap);
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
    if (highlighter.colorMap.entries) {
        free(highlighter.colorMap.entries);
        highlighter.colorMap.entries = NULL;
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

static void ProcessBrackets(const char *text, int textLen) {
    if (!text || textLen <= 0) return;
    Color parenColors[5] = {Color01, Color06, Color11, Color01, Color06};
    Color bracketColors[5] = {Color02, Color07, Color12, Color02, Color07};
    Color braceColors[5] = {Color03, Color08, Color13, Color03, Color08};
    Color angleColors[5] = {Color04, Color09, Color14, Color04, Color09};
    typedef struct {
        int pos;
        char type;
        int depth;
    } BracketInfo;
    BracketInfo stack[1024];
    int stackSize = 0;
    highlighter.bracketPairCount = 0;
    for (int i = 0; i < textLen; i++) {
        char c = text[i];
        if (i < highlighter.lastTextLen) {
            Color nodeColor = highlighter.colors[i];
            Color stringColor = GetNodeColor(&highlighter.colorMap, "string", highlighter.defaultColor);
            Color commentColor = GetNodeColor(&highlighter.colorMap, "comment", highlighter.defaultColor);
            if ((memcmp(&nodeColor, &stringColor, sizeof(Color)) == 0) ||
                (memcmp(&nodeColor, &commentColor, sizeof(Color)) == 0)) {
                continue;
            }
        }
        if (c == '(' || c == '[' || c == '{' || 
            (c == '<' && i > 0 && (isalpha(text[i-1]) || text[i-1] == ' ' || text[i-1] == ','))) {
            if (stackSize < 1024) {
                stack[stackSize].pos = i;
                stack[stackSize].type = c;
                stackSize++;
            }
        }
        else if ((c == ')' || c == ']' || c == '}' || 
                (c == '>' && i > 0 && text[i-1] != '-' && text[i-1] != '=')) && stackSize > 0) {
            char expected = 0;
            switch (c) {
                case ')': expected = '('; break;
                case ']': expected = '['; break;
                case '}': expected = '{'; break;
                case '>': expected = '<'; break;
            }  
            if (expected) {
                int j = stackSize - 1;
                while (j >= 0 && stack[j].type != expected) j--;
                if (j >= 0) {
                    int openPos = stack[j].pos;
                    int depth = j;
                    if (expected == '(') {
                        highlighter.colors[openPos] = parenColors[depth % 5];
                        highlighter.colors[i] = parenColors[depth % 5];
                    } else if (expected == '[') {
                        highlighter.colors[openPos] = bracketColors[depth % 5];
                        highlighter.colors[i] = bracketColors[depth % 5];
                    } else if (expected == '{') {
                        highlighter.colors[openPos] = braceColors[depth % 5];
                        highlighter.colors[i] = braceColors[depth % 5];
                    } else if (expected == '<') {
                        highlighter.colors[openPos] = angleColors[depth % 5];
                        highlighter.colors[i] = angleColors[depth % 5];
                    }
                    if (highlighter.bracketPairCount < MAX_BRACKET_PAIRS) {
                        highlighter.bracketPairs[highlighter.bracketPairCount].open = openPos;
                        highlighter.bracketPairs[highlighter.bracketPairCount].close = i;
                        highlighter.bracketPairCount++;
                    }
                    for (int k = j; k < stackSize - 1; k++) {
                        stack[k] = stack[k+1];
                    }
                    stackSize--;
                }
            }
        }
    }
}

int SyntaxFindMatchingBracket(int pos) {
    if (!highlighter.initialized || pos < 0 || pos >= highlighter.lastTextLen)
        return -1;
    for (int i = 0; i < highlighter.bracketPairCount; i++) {
        if (highlighter.bracketPairs[i].open == pos)
            return highlighter.bracketPairs[i].close;
        if (highlighter.bracketPairs[i].close == pos)
            return highlighter.bracketPairs[i].open;
    }
    return -1;
}

bool SyntaxIsOpeningBracket(char c) {
    return c == '(' || c == '[' || c == '{' || c == '<' || c == '"' || c == '\'' || c == '`';
}

char SyntaxGetClosingBracket(char c) {
    switch (c) {
        case '(': return ')';
        case '[': return ']';
        case '{': return '}';
        case '<': return '>';
        case '"': return '"';
        case '\'': return '\'';
        case '`': return '`';
        default: return 0;
    }
}

void SyntaxUpdate(const char *text, int textLen) {
    if (!highlighter.initialized || !text || textLen <= 0 || !highlighter.language) {
        return;
    }
    if (textLen > highlighter.maxColors) {
        Color *newColors = (Color*)realloc(highlighter.colors, textLen * sizeof(Color));
        if (!newColors) {
            fprintf(stderr, "Error: Failed to resize color buffer\n");
            return;
        }
        for (int i = highlighter.maxColors; i < textLen; i++) {
            COPY_Color(newColors[i], highlighter.defaultColor);
        }
        highlighter.colors = newColors;
        highlighter.maxColors = textLen;
    }
    bool needsReparse = 
        highlighter.lastText == NULL || 
        textLen != highlighter.lastTextLen || 
        (highlighter.lastText && memcmp(text, highlighter.lastText, textLen) != 0);
    if (!needsReparse) {
        return; 
    }
    if (highlighter.tree) {
        ts_tree_delete(highlighter.tree);
        highlighter.tree = NULL;
    }
    if (highlighter.lastText) {
        free(highlighter.lastText);
    }
    highlighter.lastText = (char*)malloc(textLen + 1);
    if (!highlighter.lastText) {
        fprintf(stderr, "Error: Failed to allocate text buffer\n");
        return;
    }
    memcpy(highlighter.lastText, text, textLen);
    highlighter.lastText[textLen] = '\0';
    highlighter.lastTextLen = textLen;
    for (int i = 0; i < textLen; i++) {
        COPY_Color(highlighter.colors[i], highlighter.defaultColor);
    }
    bool languageLoaded = false;
    for (int i = 0; i < highlighter.numLanguages; i++) {
        if (highlighter.languages[i].name && 
            strcmp(highlighter.languages[i].name, highlighter.language) == 0) {
            if (LoadLanguage(&highlighter.languages[i])) {
                languageLoaded = true;
                ts_parser_set_language(highlighter.parser, highlighter.languages[i].func());
                break;
            }
        }
    }
    if (!languageLoaded) {
        if (highlighter.debugMode) {
            printf("Failed to load language: %s\n", highlighter.language);
        }
        return;
    }
    highlighter.tree = ts_parser_parse_string(highlighter.parser, NULL, text, textLen);
    if (!highlighter.tree) {
        if (highlighter.debugMode) {
            printf("Failed to parse text\n");
        }
        return;
    }
    TSNode rootNode = ts_tree_root_node(highlighter.tree);
    if (!ts_node_is_null(rootNode)) {
        ApplyNodeHighlighting(rootNode);
        ProcessBrackets(text, textLen);
    }
}

bool SyntaxCheckLanguageAvailability(const char *langName) {
    if (!langName) return false;
    for (int i = 0; i < highlighter.numLanguages; i++) {
        if (highlighter.languages[i].name && 
            strcmp(highlighter.languages[i].name, langName) == 0) {
            bool success = LoadLanguage(&highlighter.languages[i]);
            if (success && highlighter.languages[i].handle) {
                dlclose(highlighter.languages[i].handle);
                highlighter.languages[i].handle = NULL;
                highlighter.languages[i].func = NULL;
            }
            
            return success;
        }
    }
    return false;
}

void SyntaxFindLanguageLibraries() {
    if (highlighter.debugMode) {
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
}

void SyntaxDrawText(float x, float y, const char *text, int length, int startPos) {
    if (!highlighter.initialized || !highlighter.drawFunc || !text || length <= 0) {
        return;
    }
    if (startPos < 0 || startPos >= highlighter.lastTextLen || 
        !highlighter.language || !highlighter.tree) {
        highlighter.drawFunc(x, y, highlighter.fontPtr, highlighter.fontSize, text, highlighter.defaultColor, highlighter.userData);
        return;
    }
    Font *font = (Font*)highlighter.fontPtr;
    float scale = highlighter.fontSize / font->fontSize;
    int batchStart = 0;
    Color batchColor = highlighter.colors[startPos];
    for (int i = 1; i <= length; i++) {
        Color currentColor;
        if (i < length && startPos + i < highlighter.lastTextLen) {
            currentColor = highlighter.colors[startPos + i];
        } else {
            currentColor = highlighter.defaultColor;
        }
        if (i == length || 
            currentColor.r != batchColor.r || 
            currentColor.g != batchColor.g || 
            currentColor.b != batchColor.b || 
            currentColor.a != batchColor.a) {
            int batchLen = i - batchStart;
            if (batchLen <= 0) continue;
            if (batchLen < 256) {
                char batchText[256];
                memcpy(batchText, text + batchStart, batchLen);
                batchText[batchLen] = '\0';
                highlighter.drawFunc(x, y, highlighter.fontPtr, highlighter.fontSize, batchText, batchColor, highlighter.userData);
                float batchWidth = 0;
                for (int j = batchStart; j < i; j++) {
                    unsigned char c = (unsigned char)text[j];
                    if (c >= 32 && c < 32 + MAX_GLYPHS) {
                        const Glyph *g = &font->glyphs[c - 32];
                        batchWidth += g->xadvance * scale;
                    }
                }
                x += batchWidth;
            } else {
                for (int j = batchStart; j < i; j++) {
                    char charStr[2] = {text[j], '\0'};
                    highlighter.drawFunc(x, y, highlighter.fontPtr, highlighter.fontSize, 
                                      charStr, batchColor, highlighter.userData);
                    unsigned char c = (unsigned char)text[j];
                    if (c >= 32 && c < 32 + MAX_GLYPHS) {
                        const Glyph *g = &font->glyphs[c - 32];
                        x += g->xadvance * scale;
                    }
                }
            }
            batchStart = i;
            if (i < length) {
                batchColor = currentColor;
            }
        }
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

void SyntaxDrawWrapper(float x, float y, void* fontPtr, float fontSize, const char* text, Color color, void* userData) {
    Color textColor = {color.r, color.g, color.b, color.a};
    DrawTextBatch(x, y, *(Font*)fontPtr, fontSize, text, textColor);
}

static void PrintNode(TSNode node, int depth, char* text) {
    if (!highlighter.debugMode || ts_node_is_null(node)) return;
    for (int i = 0; i < depth; i++) printf("  ");
    const char *type = ts_node_type(node);
    uint32_t startByte = ts_node_start_byte(node);
    uint32_t endByte = ts_node_end_byte(node);
    Color color = GetColorForNodeType(type);
    printf("%s (%d-%d) Color:[%d,%d,%d,%d] ", type, startByte, endByte, color.r, color.g, color.b, color.a);
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
        PrintNode(child, depth + 1, text);
    }
}

void SyntaxDebugPrint() {
    if (!highlighter.debugMode || !highlighter.initialized) {
        return;
    }
    printf("Current language: %s\n", highlighter.language ? highlighter.language : "None");
    if (!highlighter.tree || !highlighter.lastText) {
        printf("No syntax tree or text available for debugging\n");
        return;
    }
    TSNode root = ts_tree_root_node(highlighter.tree);
    if (!ts_node_is_null(root)) {
        printf("\nSyntax Tree:\n");
        PrintNode(root, 0, highlighter.lastText);
    }
}

bool SyntaxInitWithLanguageDetection(Color defaultColor, DrawTextFunc drawFunc, void* fontPtr, float fontSize, void* userData, const char *filename, char *text, int textLen, char *statusMsg, size_t statusMsgSize) {
    if(!SyntaxInit(defaultColor, drawFunc, fontPtr, fontSize, userData)) {
        return false;
    }
    if (highlighter.debugMode) {
        SyntaxFindLanguageLibraries();
    }
    bool languageSet = false;
    if(filename && strlen(filename) > 0) {
        if(SyntaxSetLanguageFromFile(filename)) {
            languageSet = true;
            if(statusMsg) {
                snprintf(statusMsg, statusMsgSize, "Syntax: %s", SyntaxGetLanguage());
            }
        } 
        else if(text && textLen > 0) {
            if(strstr(filename, ".txt")) {
                if(strstr(text, "#include") || strstr(text, "int main")) {
                    if(SyntaxSetLanguage("c")) { 
                        languageSet = true; 
                        if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: C (auto-detected)");
                    }
                } 
                else if(strstr(text, "<!DOCTYPE html") || strstr(text, "<html")) {
                    if(SyntaxSetLanguage("html")) { 
                        languageSet = true; 
                        if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: HTML (auto-detected)");
                    }
                } 
                else if(strstr(text, "#!/bin/bash") || strstr(text, "#!/usr/bin/env bash")) {
                    if(SyntaxSetLanguage("bash")) { 
                        languageSet = true; 
                        if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Bash (auto-detected)");
                    }
                } 
                else if(strstr(text, "#!/usr/bin/python") || strstr(text, "#!/usr/bin/env python")) {
                    if(SyntaxSetLanguage("python")) { 
                        languageSet = true; 
                        if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Python (auto-detected)");
                    }
                } 
                else if(strstr(text, "package ") && strstr(text, "import ") && 
                       (strstr(text, "func ") || strstr(text, "type "))) {
                    if(SyntaxSetLanguage("go")) { 
                        languageSet = true; 
                        if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Go (auto-detected)");
                    }
                } 
                else if(strstr(text, "fn ") && strstr(text, "-> ") && 
                       (strstr(text, "let ") || strstr(text, "mut "))) {
                    if(SyntaxSetLanguage("rust")) { 
                        languageSet = true; 
                        if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: Rust (auto-detected)");
                    }
                }
            }
        }
    }
    if(!languageSet) {
        if(SyntaxSetLanguage("c")) { 
            languageSet = true; 
            if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: C (default)");
        }
        else if(SyntaxCycleLanguage()) { 
            languageSet = true; 
            if(statusMsg) snprintf(statusMsg, statusMsgSize, "Syntax: %s (fallback)", SyntaxGetLanguage());
        }
    }
    if(languageSet && text && textLen > 0) {
        SyntaxUpdate(text, textLen);
    }  
    return true;
}