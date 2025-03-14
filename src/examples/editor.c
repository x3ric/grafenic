#include "../window.h"
#include "modules/ui.c"
#include <float.h>

typedef struct { char* text; int length; } Line;
typedef struct { float targetY, currentY, velocity; bool smoothScroll; } ScrollState;

Font font;
Shader shaderfontcursor;
Line lines[1024] = {0};
int numLines = 1, cursorLine = 0, cursorCol = 0, lineHeight, gutterWidth = 50;
ScrollState scroll = {0, 0, 0, true};
static double lastKeyTime[GLFW_KEY_LAST + 1] = {0};
static bool keyStates[GLFW_KEY_LAST + 1] = {false};
double fontSize = 40.0f;
bool showLineNumbers = true, showStatusBar = true, showScrollbar = true, isFileDirty = false;
bool isSelecting = false, isScrollbarDragging = false;
int selStartLine = -1, selStartCol = -1, selEndLine = -1, selEndCol = -1;
int statusBarHeight = 25, scrollbarWidth = 12;
char filename[256] = "Untitled.txt", statusMsg[256] = "";
Color bgColor = {30,30,30,255}, textColor = {220,220,220,255}, gutterColor = {40,40,40,255}, lineNumColor = {150,150,150,255}, statusBarColor = {50,50,70,255}, statusTextColor = {200,200,200,255}, scrollbarColor = {60,60,60,255}, scrollThumbColor = {120,120,140,200}, curLineHighlight = {50,50,50,100};

void InitLine(int idx) {
    if (idx >= 1024) return;
    lines[idx].text = malloc(1024);
    if (!lines[idx].text) { fprintf(stderr, "Memory error\n"); exit(1); }
    lines[idx].text[0] = '\0';
    lines[idx].length = 0;
}

void FreeLines() { for (int i = 0; i < numLines; i++) free(lines[i].text); }

void InsertChar(char c) {
    if (lines[cursorLine].length < 1023) {
        memmove(&lines[cursorLine].text[cursorCol+1], &lines[cursorLine].text[cursorCol], lines[cursorLine].length - cursorCol + 1);
        lines[cursorLine].text[cursorCol++] = c;
        lines[cursorLine].length++;
        isFileDirty = true;
    }
}

void DeleteChar() {
    if (cursorCol > 0) {
        memmove(&lines[cursorLine].text[cursorCol-1], &lines[cursorLine].text[cursorCol], lines[cursorLine].length - cursorCol + 1);
        lines[cursorLine].length--;
        cursorCol--;
        isFileDirty = true;
    } else if (cursorLine > 0) {
        cursorCol = lines[cursorLine-1].length;
        strcat(lines[cursorLine-1].text, lines[cursorLine].text);
        lines[cursorLine-1].length += lines[cursorLine].length;
        free(lines[cursorLine].text);
        memmove(&lines[cursorLine], &lines[cursorLine+1], (numLines-cursorLine-1) * sizeof(Line));
        numLines--;
        cursorLine--;
        isFileDirty = true;
    }
}

void DeleteCharAfter() {
    if (cursorCol < lines[cursorLine].length) {
        memmove(&lines[cursorLine].text[cursorCol], &lines[cursorLine].text[cursorCol+1], lines[cursorLine].length - cursorCol);
        lines[cursorLine].length--;
        isFileDirty = true;
    } else if (cursorLine < numLines - 1) {
        strcat(lines[cursorLine].text, lines[cursorLine+1].text);
        lines[cursorLine].length += lines[cursorLine+1].length;
        free(lines[cursorLine+1].text);
        memmove(&lines[cursorLine+1], &lines[cursorLine+2], (numLines-cursorLine-2) * sizeof(Line));
        numLines--;
        isFileDirty = true;
    }
}

void InsertNewline() {
    if (numLines >= 1024) return;
    memmove(&lines[cursorLine+1], &lines[cursorLine], (numLines-cursorLine) * sizeof(Line));
    numLines++;
    InitLine(cursorLine+1);
    strcpy(lines[cursorLine+1].text, &lines[cursorLine].text[cursorCol]);
    lines[cursorLine+1].length = lines[cursorLine].length - cursorCol;
    lines[cursorLine].text[cursorCol] = '\0';
    lines[cursorLine].length = cursorCol;
    cursorLine++;
    cursorCol = 0;
    isFileDirty = true;
}

int MaxScroll() {
    int maxVisLines = (window.screen_height) / lineHeight;
    return fmax(0, (numLines - maxVisLines) * lineHeight);
}

void UpdateScroll(double dt) {
    if (scroll.smoothScroll) {
        float diff = scroll.targetY - scroll.currentY;
        if (fabs(diff) > 0.5f) {
            scroll.velocity = scroll.velocity * 0.7f + diff * 9.0f * dt;
            scroll.currentY += scroll.velocity * dt;
        } else {
            scroll.currentY = scroll.targetY;
            scroll.velocity = 0;
        }
    } else scroll.currentY = scroll.targetY;
}

void AdjustScrollToCursor() {
    int cursorY = cursorLine * lineHeight;
    int viewStart = scroll.targetY;
    int viewEnd = scroll.targetY + window.screen_height - lineHeight;
    if (cursorY < viewStart)
        scroll.targetY = fmax(0, cursorY - lineHeight);
    else if (cursorY > viewEnd)
        scroll.targetY = fmin(MaxScroll(), cursorY - window.screen_height + lineHeight + (showStatusBar ? statusBarHeight : 0));
}

int FindCharPos(int line, int mouseX) {
    if (line < 0 || line >= numLines) return 0;
    int xOffset = showLineNumbers ? gutterWidth : 0;
    if (mouseX <= xOffset) return 0;
    const char* lineText = lines[line].text;
    int lineLen = strlen(lineText);
    int bestPos = 0;
    float bestDist = FLT_MAX, currentX = xOffset;
    for (int i = 0; i <= lineLen; i++) {
        float charWidth;
        if (i < lineLen) {
            char charStr[2] = {lineText[i], '\0'};
            TextSize size = GetTextSize(font, fontSize, charStr);
            charWidth = size.width;
        } else {
            charWidth = GetTextSize(font, fontSize, "m").width / 2;
        }
        float midX = currentX + charWidth / 2;
        float dist = fabs(mouseX - midX);
        if (dist < bestDist) {
            bestDist = dist;
            bestPos = i;
        }
        currentX += charWidth;
    }
    return bestPos;
}

void UpdateCursor(double x, double y) {
    if (showScrollbar && x >= window.screen_width - scrollbarWidth) return;
    int adjustedY = y + scroll.currentY;
    int line = adjustedY / lineHeight;
    if (line >= 0 && line < numLines) {
        cursorLine = line;
        cursorCol = FindCharPos(cursorLine, x);
    }
}

int GetGlobalPos(int line, int col) {
    int pos = 0;
    for (int i = 0; i < line; ++i) pos += strlen(lines[i].text) + 1;
    return pos + col;
}

void DeleteSelection() {
    if (selStartLine == -1 || selEndLine == -1) return;
    int startLine = selStartLine, startCol = selStartCol;
    int endLine = selEndLine, endCol = selEndCol;
    if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
        int tl = startLine, tc = startCol;
        startLine = endLine; startCol = endCol;
        endLine = tl; endCol = tc;
    }
    if (startLine == endLine) {
        memmove(&lines[startLine].text[startCol], &lines[startLine].text[endCol], lines[startLine].length - endCol + 1);
        lines[startLine].length -= (endCol - startCol);
    } else {
        strncat(lines[startLine].text + startCol, lines[endLine].text + endCol, 
                1024 - startCol - 1);
        lines[startLine].length = strlen(lines[startLine].text);
        int linesToRemove = endLine - startLine;
        for (int i = endLine + 1; i < numLines; i++)
            lines[i - linesToRemove] = lines[i];
        for (int i = numLines - linesToRemove; i < numLines; i++) {
            free(lines[i].text);
            lines[i].text = NULL;
            lines[i].length = 0;
        }
        numLines -= linesToRemove;
    }
    cursorLine = startLine;
    cursorCol = startCol;
    selStartLine = selStartCol = selEndLine = selEndCol = -1;
    isSelecting = false;
    isFileDirty = true;
}

void SaveFile(const char* name) {
    FILE* file = fopen(name, "w");
    if (!file) {
        snprintf(statusMsg, sizeof(statusMsg), "Error: Could not save file");
        return;
    }
    for (int i = 0; i < numLines; i++) fprintf(file, "%s\n", lines[i].text);
    fclose(file);
    strcpy(filename, name);
    isFileDirty = false;
    snprintf(statusMsg, sizeof(statusMsg), "File saved: %s", name);
}

void LoadFile(const char* name) {
    FILE* file = fopen(name, "r");
    if (!file) {
        snprintf(statusMsg, sizeof(statusMsg), "Error: Could not open file");
        return;
    }
    for (int i = 0; i < numLines; i++) free(lines[i].text);
    numLines = 0;
    char buffer[1024];
    while (fgets(buffer, 1024, file) && numLines < 1024) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
            len--;
        }   
        InitLine(numLines);
        strcpy(lines[numLines].text, buffer);
        lines[numLines].length = len;
        numLines++;
    }
    if (numLines == 0) { InitLine(0); numLines = 1; }
    fclose(file);
    strcpy(filename, name);
    isFileDirty = false;
    cursorLine = cursorCol = 0;
    scroll.targetY = scroll.currentY = 0;
    snprintf(statusMsg, sizeof(statusMsg), "File loaded: %s", name);
}

void ScrollCallback(GLFWwindow* win, double x, double y) {
    scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY - y * lineHeight * 2.5f));
    if (scroll.smoothScroll) scroll.velocity = 0;
}

void CharCallback(GLFWwindow* win, unsigned int c) {
    if (isprint(c)) InsertChar((char)c);
    snprintf(statusMsg, sizeof(statusMsg), "Line: %d Col: %d Lines: %d | %s%s", cursorLine+1, cursorCol+1, numLines, filename, isFileDirty ? " *" : "");
}

void HandleKey(int key, double time) {
    double interval = 0.03;
    if (!keyStates[key] || (time - lastKeyTime[key] > interval)) {
        keyStates[key] = true;
        lastKeyTime[key] = time;
    }
}

bool IsInScrollbar(double x, double y) {
    if (!showScrollbar) return false;
    int sbX = window.screen_width - scrollbarWidth;
    int sbH = window.screen_height;
    return x >= sbX && x <= window.screen_width && y >= 0 && y <= sbH;
}

void HandleScrollDrag(double y) {
    int sbHeight = window.screen_height;
    int maxScroll = MaxScroll();
    float ratio = y / sbHeight;
    ratio = fmin(1.0f, fmax(0.0f, ratio));
    scroll.targetY = ratio * maxScroll;
}

void KeyCallback(GLFWwindow* win, int key, int scan, int action, int mods) {
    double time = glfwGetTime();
    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (ctrl) {
            if (!isSelecting) {
                selStartLine = cursorLine;
                selStartCol = cursorCol;
                isSelecting = true;
            }
        } else {
            if (!isSelecting) {
                selStartLine = selEndLine = cursorLine;
                selStartCol = selEndCol = cursorCol;
            }
        }
        HandleKey(key, time);
        switch (key) {
            case GLFW_KEY_BACKSPACE:
                if (ctrl && isSelecting) DeleteSelection();
                else DeleteChar();
                break;
            case GLFW_KEY_DELETE:
                if (ctrl && isSelecting) DeleteSelection();
                else DeleteCharAfter();
                break;
            case GLFW_KEY_ENTER: InsertNewline(); break;
            case GLFW_KEY_LEFT:
                if (cursorCol > 0) cursorCol--;
                else if (cursorLine > 0) {
                    cursorLine--;
                    cursorCol = lines[cursorLine].length;
                }
                break;
            case GLFW_KEY_RIGHT:
                if (cursorCol < lines[cursorLine].length) cursorCol++;
                else if (cursorLine < numLines-1) {
                    cursorLine++;
                    cursorCol = 0;
                }
                break;
            case GLFW_KEY_UP:
                if (cursorLine > 0) {
                    cursorLine--;
                    cursorCol = fmin(cursorCol, lines[cursorLine].length);
                }
                break;
            case GLFW_KEY_DOWN:
                if (cursorLine < numLines-1) {
                    cursorLine++;
                    cursorCol = fmin(cursorCol, lines[cursorLine].length);
                }
                break;
            case 47: if (ctrl && fontSize >= 50.0f) fontSize -= 8.5f; break;
            case 93: if (ctrl && fontSize <= Scaling(175.0)) fontSize += 8.5f; break;
            case GLFW_KEY_HOME:
                if (ctrl) {
                    cursorLine = cursorCol = 0;
                    scroll.targetY = 0;
                } else cursorCol = 0;
                break;
            case GLFW_KEY_END:
                if (ctrl) {
                    cursorLine = numLines-1;
                    cursorCol = lines[cursorLine].length;
                    scroll.targetY = MaxScroll();
                } else cursorCol = lines[cursorLine].length;
                break;
            case GLFW_KEY_PAGE_UP:
                cursorLine = fmax(0, cursorLine - window.screen_height/lineHeight);
                cursorCol = fmin(cursorCol, lines[cursorLine].length);
                scroll.targetY = fmax(0, scroll.targetY - window.screen_height + lineHeight);
                break;
            case GLFW_KEY_PAGE_DOWN:
                cursorLine = fmin(numLines-1, cursorLine + window.screen_height/lineHeight);
                cursorCol = fmin(cursorCol, lines[cursorLine].length);
                scroll.targetY = fmin(MaxScroll(), scroll.targetY + window.screen_height - lineHeight);
                break;
            case GLFW_KEY_L:
                if (ctrl) {
                    showLineNumbers = !showLineNumbers;
                    snprintf(statusMsg, sizeof(statusMsg), "Line numbers: %s", 
                             showLineNumbers ? "ON" : "OFF");
                }
                break;
            case GLFW_KEY_R:
                if (ctrl) {
                    showScrollbar = !showScrollbar;
                    snprintf(statusMsg, sizeof(statusMsg), "Scrollbar: %s", 
                             showScrollbar ? "ON" : "OFF");
                }
                break;
            case GLFW_KEY_S:
                if (ctrl) {
                    if (mods & GLFW_MOD_SHIFT)
                        snprintf(statusMsg, sizeof(statusMsg), "Save As not implemented");
                    else if (isFileDirty) SaveFile(filename);
                    else scroll.smoothScroll = !scroll.smoothScroll;
                }
                break;
        }
        if (isSelecting) { selEndLine = cursorLine; selEndCol = cursorCol; }
        AdjustScrollToCursor();
        snprintf(statusMsg, sizeof(statusMsg), "Line: %d Col: %d Lines: %d | %s%s", 
                 cursorLine+1, cursorCol+1, numLines, filename, isFileDirty ? " *" : "");
    } else if (action == GLFW_RELEASE) {
        keyStates[key] = false;
        if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
            isSelecting = false;
    }
}

void MouseCallback(GLFWwindow* win, int button, int action, int mods) {
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (IsInScrollbar(x, y)) {
                isScrollbarDragging = true;
                HandleScrollDrag(y);
            } else {
                UpdateCursor(x, y);
                selStartLine = selEndLine = cursorLine;
                selStartCol = selEndCol = cursorCol;
                isSelecting = true;
            }
        } else if (action == GLFW_RELEASE) {
            isScrollbarDragging = false;
            if (!IsInScrollbar(x, y) && 
                selStartLine == selEndLine && selStartCol == selEndCol) {
                selStartLine = selEndLine = -1;
                selStartCol = selEndCol = -1;
            }
            isSelecting = false;
        }
    }
}

void CursorPosCallback(GLFWwindow* win, double x, double y) {
    if (isScrollbarDragging) {
        HandleScrollDrag(y);
        return;
    }
    if (isSelecting && glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (IsInScrollbar(x, y)) return;
        UpdateCursor(x, y);
        selEndLine = cursorLine;
        selEndCol = cursorCol;
        int edge = lineHeight * 2;
        if (y < edge)
            scroll.targetY = fmax(0, scroll.targetY - lineHeight);
    }
}

bool IsSelValid() {
    return selStartLine != -1 && selEndLine != -1 && (selStartLine != selEndLine || selStartCol != selEndCol);
}

void DrawLineNumbers() {
    if (!showLineNumbers) return;
    int start = scroll.currentY / lineHeight;
    int end = fmin(numLines, start + window.screen_height / lineHeight + 1);
    for (int i = start; i < end; i++) {
        char num[16];
        snprintf(num, sizeof(num), "%d", i + 1);
        TextSize size = GetTextSize(font, fontSize, num);
        int tX = gutterWidth - size.width ;
        int tY = i * lineHeight - scroll.currentY;
        Color color = (i == cursorLine) ? textColor : lineNumColor;
        DrawText(tX, tY, font, fontSize, num, color);
    }
}

void DrawScrollbar() {
    if (!showScrollbar) return;
    int sbHeight = window.screen_height;
    int sbX = window.screen_width - scrollbarWidth;
    int totalHeight = numLines * lineHeight;
    float ratio = (float)window.screen_height / (float)totalHeight;
    ratio = fmin(1.0f, ratio);
    int thumbHeight = fmax(30, sbHeight * ratio);
    float scrollRatio = scroll.currentY / (float)fmax(1, MaxScroll());
    scrollRatio = fmin(1.0f, fmax(0.0f, scrollRatio));
    int thumbY = scrollRatio * (sbHeight - thumbHeight);
    DrawRect(sbX, thumbY, scrollbarWidth, thumbHeight, scrollThumbColor);
}

void DrawEditor() {
    lineHeight = GetTextSize(font, fontSize, "gj|").height;
    DrawLineNumbers();
    int textX = showLineNumbers ? gutterWidth : 0;
    int startLine = scroll.currentY / lineHeight;
    int endLine = fmin(numLines, startLine + window.screen_height / lineHeight + 1);
    for (int i = startLine; i < endLine; i++) {
        int lineY = (i * lineHeight) - scroll.currentY;
        if (i == cursorLine) {
            DrawRect(0, lineY, window.screen_width, lineHeight, curLineHighlight);
        }
        int selStartCol = -1, selEndCol = -1;
        if (IsSelValid() && 
            ((i >= selStartLine && i <= selEndLine) || 
             (i >= selEndLine && i <= selStartLine))) {
            selStartCol = (i == selStartLine) ? selStartCol : 0;
            selEndCol = (i == selEndLine) ? selEndCol : lines[i].length;
            if (selStartLine > selEndLine || 
                (selStartLine == selEndLine && selStartCol > selEndCol)) {
                int tempLine = selStartLine, tempCol = selStartCol;
                selStartLine = selEndLine;
                selStartCol = selEndCol;
                selEndLine = tempLine;
                selEndCol = tempCol;
            }
        }
        char* lineText = lines[i].text;
        int lineLen = lines[i].length;
        float currentX = textX;
        for (int j = 0; j < lineLen; j++) {
            char charStr[2] = {lineText[j], '\0'};
            TextSize charSize = GetTextSize(font, fontSize, charStr);
            if (IsSelValid() && 
                i >= selStartLine && i <= selEndLine && 
                j >= selStartCol && j < selEndCol) {
                DrawRect(currentX, lineY, charSize.width, lineHeight, (Color){100, 100, 200, 100});
            }
            DrawText(currentX, lineY, font, fontSize, charStr, textColor);
            currentX += charSize.width;
        }
        if (i == cursorLine) {
            float cursorX = textX;
            for (int j = 0; j < cursorCol; j++) {
                char charStr[2] = {lines[i].text[j], '\0'};
                TextSize charSize = GetTextSize(font, fontSize, charStr);
                cursorX += charSize.width;
            }
            static double lastBlinkTime = 0;
            double currentTime = glfwGetTime();
            if (fmod(currentTime, 1.0) < 0.5) {
                DrawRect(cursorX, lineY, (textX * fontSize) / 80, lineHeight, textColor); // cursor as a block
                //DrawRect(cursorX, lineY, 2, lineHeight, textColor); // useful in not monospaced fonts or in  ins mode
            }
        }
    }
    DrawScrollbar();
}

int main() {
    WindowInit(1920, 1080, "Text Editor");
    font = LoadFont("./res/fonts/Monocraft.ttf");//font.nearest = false;
    //shaderdefault.hotreloading = true;shaderfont.hotreloading = true;
    InitLine(0);
    glfwSetCharCallback(window.w, CharCallback);
    glfwSetKeyCallback(window.w, KeyCallback);
    glfwSetScrollCallback(window.w, ScrollCallback);
    glfwSetMouseButtonCallback(window.w, MouseCallback);
    glfwSetCursorPosCallback(window.w, CursorPosCallback);
    while (!WindowState()) {
        WindowClear();
        UpdateScroll(window.deltatime * 10.0f);
        DrawEditor();
        ExitPromt(font);
        WindowProcess();
    }
    FreeLines();
    WindowClose();
    return 0;
}