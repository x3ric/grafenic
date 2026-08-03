
static int HexDigit(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

static bool HexByte(const char* value, GLubyte* output) {
    int high = HexDigit(value[0]);
    int low = HexDigit(value[1]);
    if (high < 0 || low < 0) return false;
    *output = (GLubyte)((high << 4) | low);
    return true;
}

Color HexToColor(const char* hex) {
    Color color = {0, 0, 0, 255};
    if (!hex || hex[0] != '#') return color;
    size_t length = strlen(hex);
    if (length != 7 && length != 9) return color;
    if (!HexByte(hex + 1, &color.r) || !HexByte(hex + 3, &color.g) || !HexByte(hex + 5, &color.b)) return (Color){0, 0, 0, 255};
    if (length == 9 && !HexByte(hex + 7, &color.a)) return (Color){0, 0, 0, 255};
    return color;
}

void glColor(Color color) {
    if (color.a == 0) color.a = 255;
    glColor4ub(color.r, color.g, color.b, color.a);
}

void ClearColor(Color color) {
    if (color.a == 0) color.a = 255;
    glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
}
