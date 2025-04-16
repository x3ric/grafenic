
char* LspEscapeJson(const char* text) {
    if (!text) return NULL;
    size_t len = strlen(text);
    char* escaped = malloc(len * 2 + 1);
    if (!escaped) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\\' || text[i] == '"' || text[i] == '\n' || text[i] == '\r') {
            escaped[j++] = '\\';
            if (text[i] == '\n') { escaped[j++] = 'n'; continue; } 
            else if (text[i] == '\r') { escaped[j++] = 'r'; continue; }
        }
        escaped[j++] = text[i];
    }
    escaped[j] = '\0';
    return escaped;
}

char* LspFindJsonValue(const char* json, const char* key) {
    if (!json || !key) return NULL;
    char searchKey[LSP_JSON_TOKEN_MAX];
    snprintf(searchKey, sizeof(searchKey), "\"%s\"", key);
    const char* keyPos = strstr(json, searchKey);
    if (!keyPos) return NULL;
    keyPos += strlen(searchKey);
    const char* colon = strchr(keyPos, ':');
    if (!colon) return NULL;
    const char* valueStart = colon + 1;
    while (*valueStart && isspace(*valueStart)) valueStart++;
    if (!*valueStart) return NULL;
    return (char*)valueStart;
}

char* LspFindJsonArray(const char* json, const char* key) {
    char* value = LspFindJsonValue(json, key);
    if (!value || *value != '[') return NULL;
    return value;
}

int LspJsonArrayLength(const char* array) {
    if (!array || *array != '[') return 0;
    int count = 0, depth = 0;
    bool inString = false, itemStarted = false;
    for (const char* p = array + 1; *p; p++) {
        if (inString && p[-1] == '\\') continue;
        switch (*p) {
            case '"': inString = !inString; break;
            case '[': case '{': if (!inString) depth++; break;
            case ']': case '}': if (!inString) { depth--; if (depth < 0) return count; } break;
            case ',': if (!inString && depth == 0) { count++; itemStarted = false; } break;
            default: if (!inString && depth == 0 && !isspace(*p) && !itemStarted) itemStarted = true; break;
        }
    }
    return count + (itemStarted ? 1 : 0);
}

char* LspJsonArrayItem(const char* array, int index) {
    if (!array || *array != '[' || index < 0) return NULL;
    const char* p = array + 1;
    while (isspace(*p)) p++;
    int currentIndex = 0, depth = 0;
    bool inString = false;
    while (*p) {
        if (currentIndex == index) return (char*)p;
        if (inString && p[-1] == '\\') { p++; continue; }
        switch (*p) {
            case '"': inString = !inString; break;
            case '[': case '{': if (!inString) depth++; break;
            case ']': case '}': if (!inString) depth--; break;
            case ',': if (!inString && depth == 0) { currentIndex++; p++; while (isspace(*p)) p++; continue; } break;
        }
        if (*p == ']' && !inString && depth < 0) break;
        p++;
    }
    return NULL;
}

char* LspJsonExtractString(const char* json, char* buffer, size_t bufferSize) {
    if (!json || !buffer || bufferSize == 0) return NULL;
    buffer[0] = '\0';
    const char* start = strchr(json, '"');
    if (!start) return NULL;
    start++;
    size_t i = 0;
    while (*start && *start != '"' && i < bufferSize - 1) {
        if (*start == '\\' && *(start + 1)) {
            start++;
            if (i >= bufferSize - 1) break;
            switch (*start) {
                case 'n': buffer[i++] = '\n'; break;
                case 'r': buffer[i++] = '\r'; break;
                case 't': buffer[i++] = '\t'; break;
                case '\\': buffer[i++] = '\\'; break;
                case '"': buffer[i++] = '"'; break;
                default: buffer[i++] = *start; break;
            }
        } else {
            buffer[i++] = *start;
        }
        start++;
    }
    buffer[i] = '\0';
    return buffer;
}

int LspJsonExtractInt(const char* json, int defaultValue) {
    if (!json) return defaultValue;
    while (*json && isspace(*json)) json++;
    if (!*json || !isdigit(*json)) return defaultValue;
    return atoi(json);
}
