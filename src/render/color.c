
Color HexToColor(const char* hex) {
    Color color;
    if (strlen(hex) == 7) {
        sscanf(hex, "#%2hhx%2hhx%2hhx", &color.r, &color.g, &color.b);
        color.a = 255;
    } else if (strlen(hex) == 9) {
        sscanf(hex, "#%2hhx%2hhx%2hhx%2hhx", &color.r, &color.g, &color.b, &color.a);
    } else {
        color = (Color){0, 0, 0, 255};
    }
    return color;
}

void glColor(Color color){
    if (color.a == 0) { color.a = 255; }
    glColor4f((GLclampf)color.r/255.0f, (GLclampf)color.g/255.0f, (GLclampf)color.b/255.0f, (GLclampf)color.a/255.0f);
}

void ClearColor(Color color) {
    if (color.a == 0) { color.a = 255; }
    glClearColor((GLclampf)color.r/255.0f, (GLclampf)color.g/255.0f, (GLclampf)color.b/255.0f, (GLclampf)color.a/255.0f);
}
