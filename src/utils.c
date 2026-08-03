
// Debug

void ErrorCallback(int error, const char* description) {
    fprintf(stderr, "Error %d: %s\n", error, description ? description : "Unknown");
}

void ClearOutput() {
    fputs("\033[H\033[J", stdout);
    fflush(stdout);
}

void print(const char* format, ...) {
    if (!format) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// Text

const char* text(const char* format, ...) {
    static char buffer[1024];
    if (!format) return "";
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return buffer;
}

int textint(char* str) {
    return str ? (int)strtol(str, NULL, 10) : 0;
}

float textfloat(char* str) {
    return str ? strtof(str, NULL) : 0.0f;
}

unsigned int textlength(const char* value) {
    return value ? (unsigned int)strlen(value) : 0;
}

const char* textsubtext(const char* value, int position, int length) {
    static char buffer[1024];
    if (!value || length <= 0) {
        buffer[0] = '\0';
        return buffer;
    }
    size_t size = strlen(value);
    if (position < 0) position = 0;
    if ((size_t)position > size) position = (int)size;
    size_t count = MinInt(length, (int)(size - position));
    if (count >= sizeof(buffer)) count = sizeof(buffer) - 1;
    memcpy(buffer, value + position, count);
    buffer[count] = '\0';
    return buffer;
}

char* textreplace(const char* value, const char* replace, const char* by) {
    if (!value || !replace || !by) return NULL;
    size_t valueLength = strlen(value);
    size_t replaceLength = strlen(replace);
    size_t byLength = strlen(by);
    if (replaceLength == 0) return strdup(value);
    size_t occurrences = 0;
    const char* scan = value;
    while ((scan = strstr(scan, replace))) {
        occurrences++;
        scan += replaceLength;
    }
    size_t resultLength = valueLength + occurrences * byLength - occurrences * replaceLength;
    char* result = malloc(resultLength + 1);
    if (!result) return NULL;
    const char* source = value;
    char* destination = result;
    while ((scan = strstr(source, replace))) {
        size_t prefix = (size_t)(scan - source);
        memcpy(destination, source, prefix);
        destination += prefix;
        memcpy(destination, by, byLength);
        destination += byLength;
        source = scan + replaceLength;
    }
    strcpy(destination, source);
    return result;
}

char* textinsert(const char* value, const char* insert, int position) {
    if (!value || !insert) return NULL;
    size_t length = strlen(value);
    size_t insertLength = strlen(insert);
    if (position < 0) position = 0;
    if ((size_t)position > length) position = (int)length;
    char* result = malloc(length + insertLength + 1);
    if (!result) return NULL;
    memcpy(result, value, position);
    memcpy(result + position, insert, insertLength);
    memcpy(result + position + insertLength, value + position, length - position + 1);
    return result;
}

const char* textjoin(const char** textList, const char* delimiter, int count) {
    static char buffer[4096];
    buffer[0] = '\0';
    if (!textList || !delimiter || count <= 0) return buffer;
    size_t used = 0;
    for (int i = 0; i < count && used < sizeof(buffer) - 1; i++) {
        const char* part = textList[i] ? textList[i] : "";
        size_t partLength = strlen(part);
        size_t available = sizeof(buffer) - 1 - used;
        size_t copyLength = partLength < available ? partLength : available;
        memcpy(buffer + used, part, copyLength);
        used += copyLength;
        if (i < count - 1 && used < sizeof(buffer) - 1) {
            size_t delimiterLength = strlen(delimiter);
            available = sizeof(buffer) - 1 - used;
            copyLength = delimiterLength < available ? delimiterLength : available;
            memcpy(buffer + used, delimiter, copyLength);
            used += copyLength;
        }
    }
    buffer[used] = '\0';
    return buffer;
}

const char** textsplit(const char* value, char delimiter, int* count) {
    if (!count) return NULL;
    *count = 0;
    if (!value) return NULL;
    int parts = 1;
    for (const char* p = value; *p; p++) if (*p == delimiter) parts++;
    const char** splits = calloc((size_t)parts, sizeof(char*));
    if (!splits) return NULL;
    const char* start = value;
    int index = 0;
    for (const char* p = value;; p++) {
        if (*p != delimiter && *p != '\0') continue;
        size_t length = (size_t)(p - start);
        char* part = malloc(length + 1);
        if (!part) {
            for (int i = 0; i < index; i++) free((void*)splits[i]);
            free(splits);
            return NULL;
        }
        memcpy(part, start, length);
        part[length] = '\0';
        splits[index++] = part;
        if (*p == '\0') break;
        start = p + 1;
    }
    *count = index;
    return splits;
}

void textappend(char* value, const char* append, int* position) {
    if (!value || !append || !position) return;
    size_t length = strlen(append);
    memcpy(value + *position, append, length + 1);
    *position += (int)length;
}

int textfindindex(const char* value, const char* find) {
    if (!value || !find) return -1;
    const char* found = strstr(value, find);
    return found ? (int)(found - value) : -1;
}

const char* textupper(const char* value) {
    static char buffer[1024];
    if (!value) return "";
    size_t i = 0;
    for (; value[i] && i < sizeof(buffer) - 1; i++) buffer[i] = (char)toupper((unsigned char)value[i]);
    buffer[i] = '\0';
    return buffer;
}

const char* textlower(const char* value) {
    static char buffer[1024];
    if (!value) return "";
    size_t i = 0;
    for (; value[i] && i < sizeof(buffer) - 1; i++) buffer[i] = (char)tolower((unsigned char)value[i]);
    buffer[i] = '\0';
    return buffer;
}

// Utils

void RandomSeed(unsigned int seed) {
    srand(seed);
}

int RandomValue(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    uint64_t range = (uint64_t)((int64_t)max - (int64_t)min) + 1;
    uint64_t value = ((uint64_t)(unsigned int)rand() << 32) ^ (unsigned int)rand();
    return (int)((int64_t)min + (int64_t)(value % range));
}

void OpenURL(const char* url) {
    if (!url || !url[0]) return;
    if (fork() == 0) {
        execlp("xdg-open", "xdg-open", url, (char*)NULL);
        _exit(127);
    }
}

void SetClipboardText(const char* value) {
    if (window.w && value) glfwSetClipboardString(window.w, value);
}

char* GetClipboardText(void) {
    if (!window.w) return NULL;
    const char* value = glfwGetClipboardString(window.w);
    return value ? strdup(value) : NULL;
}

int MaxInt(int a, int b) {
    return a > b ? a : b;
}

int MinInt(int a, int b) {
    return a < b ? a : b;
}

int Clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int Scaling(int fontsize) {
    if (fontsize <= 0 || window.width <= 0 || window.height <= 0) return 0;
    float widthScale = (float)window.screen_width / (float)window.width;
    float heightScale = (float)window.screen_height / (float)window.height;
    int scaledFontSize = (int)(fontsize * fminf(widthScale, heightScale));
    return MaxInt(scaledFontSize, 1);
}

// Smothing

float Easing(float t, const char* name) {
    if (!name || strcmp(name, "Linear") == 0) return t;
    if (strcmp(name, "SineIn") == 0) return 1.0f - cosf(t * PI * 0.5f);
    if (strcmp(name, "SineOut") == 0) return sinf(t * PI * 0.5f);
    if (strcmp(name, "SineInOut") == 0) return -(cosf(PI * t) - 1.0f) * 0.5f;
    if (strcmp(name, "CubicIn") == 0) return t * t * t;
    if (strcmp(name, "CubicOut") == 0) {
        float p = t - 1.0f;
        return p * p * p + 1.0f;
    }
    if (strcmp(name, "CubicInOut") == 0) {
        if (t < 0.5f) return 4.0f * t * t * t;
        float f = 2.0f * t - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }
    return t;
}

float Motion(float speed, float intensity) {
    return (sinf((float)glfwGetTime() * speed) + 1.0f) * 0.5f * intensity;
}

float Lerp(float start, float end, float t) {
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    return start + (end - start) * t;
}

// Time

double GetTime() {
    return glfwGetTime();
}

void SetTime(double time) {
    glfwSetTime(time);
}

bool Wait(double delaySeconds) {
    static double startTime = 0.0;
    static bool waiting = false;
    double now = glfwGetTime();
    if (!waiting) {
        startTime = now;
        waiting = true;
        return false;
    }
    if (now - startTime < delaySeconds) return false;
    waiting = false;
    return true;
}

// Collision

bool IsInside(float x, float y, float rectX, float rectY, float rectWidth, float rectHeight) {
    return x >= rectX && x <= rectX + rectWidth && y >= rectY && y <= rectY + rectHeight;
}

// File checks

bool DirExists(const char* path) {
    struct stat info;
    return path && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

bool FileExists(const char* filename) {
    struct stat info;
    return filename && stat(filename, &info) == 0 && S_ISREG(info.st_mode);
}

time_t GetFileModTime(const char* filePath) {
    struct stat info;
    return filePath && stat(filePath, &info) == 0 ? info.st_mtime : (time_t)-1;
}

int AddWatch(int inotifyFd, const char* filePath) {
    if (!filePath) return -1;
    int watch = inotify_add_watch(inotifyFd, filePath, IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);
    if (watch == -1) fprintf(stderr, "Error adding inotify watch for %s\n", filePath);
    return watch;
}

// File Saving

char* FileLoad(const char* path) {
    if (!path) return NULL;
    FILE* file = fopen(path, "rb");
    if (!file) {
        file = fopen(path, "wb");
        if (!file) return NULL;
        fclose(file);
        return strdup("");
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long fileSize = ftell(file);
    if (fileSize < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    char* content = malloc((size_t)fileSize + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    size_t readSize = fread(content, 1, (size_t)fileSize, file);
    fclose(file);
    if (readSize != (size_t)fileSize) {
        free(content);
        return NULL;
    }
    content[fileSize] = '\0';
    return content;
}

char* FileSave(const char* path, const char* value) {
    if (!path || !value) return NULL;
    FILE* file = fopen(path, "wb");
    if (!file) return NULL;
    size_t length = strlen(value);
    bool success = fwrite(value, 1, length, file) == length;
    fclose(file);
    return success ? strdup(value) : NULL;
}

void FileClear(const char* path) {
    if (path && remove(path) != 0) perror("Error deleting file");
}
