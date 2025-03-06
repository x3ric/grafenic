#include "../src/window.h"
#include "modules/ui.c"
#include <float.h>

typedef struct { char* text; int length; } Line;
typedef struct { 
    float targetY, currentY, targetX, currentX, velocity;
    bool smoothScroll; 
} ScrollState;

Font font;
Shader shaderfontcursor;
Line lines[1024] = {0};
static double lastVerticalScrollTime = 0;
static double lastHorizontalScrollTime = 0;
static const double scrollbarFadeDelay = 1.5;
int numLines = 1, cursorLine = 0, cursorCol = 0, lineHeight, gutterWidth = 0;
ScrollState scroll = {0, 0, 0, 0, 0, true};
static double lastKeyTime[GLFW_KEY_LAST + 1] = {0};
static bool keyStates[GLFW_KEY_LAST + 1] = {false};
double fontSize = 24.0f;
bool showLineNumbers = true, showStatusBar = true, showScrollbar = true, isFileDirty = false, isSelecting = false, isScrollbarDragging = false, isHorizontalScrollbarDragging = false, insertMode = true;;
int selStartLine = -1, selStartCol = -1, selEndLine = -1, selEndCol = -1, statusBarHeight = 25, scrollbarWidth = 12;
char filename[256] = "Untitled.txt", statusMsg[256] = "";
Color textColor = {220,220,220,255};

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

int MaxHorizontalScroll() {
    int maxWidth = 0;
    for (int i = 0; i < numLines; i++) {
        if (i < 0 || i >= numLines) continue;
        TextSize size = GetTextSize(font, fontSize, lines[i].text);
        int textWidth = size.width;
        if (textWidth > maxWidth) {
            maxWidth = textWidth;
        }
    }
    int viewWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    return fmax(0, maxWidth - viewWidth + fontSize);
}

void UpdateScroll(double dt) {
    if (scroll.smoothScroll) {
        float smoothingFactor = 10.0f;
        float t = 1 - exp(-smoothingFactor * dt);
        scroll.currentY = scroll.currentY + (scroll.targetY - scroll.currentY) * t;
        scroll.currentX = scroll.currentX + (scroll.targetX - scroll.currentX) * t;
    } else {
        scroll.currentY = scroll.targetY;
        scroll.currentX = scroll.targetX;
    }
}

void AdjustScrollToCursor() {
    int cursorY = cursorLine * lineHeight;
    int viewStart = scroll.targetY;
    int viewEnd = scroll.targetY + window.screen_height - lineHeight;
    if (cursorY < viewStart)
        scroll.targetY = fmax(0, cursorY - lineHeight);
    else if (cursorY > viewEnd)
        scroll.targetY = fmin(MaxScroll(), cursorY - window.screen_height + lineHeight + (showStatusBar ? statusBarHeight : 0));
    if (cursorLine < 0 || cursorLine >= numLines) return;
    int textX = showLineNumbers ? gutterWidth : 0;
    int cursorX = textX;
    for (int j = 0; j < cursorCol; j++) {
        char charStr[2] = {lines[cursorLine].text[j], '\0'};
        TextSize charSize = GetTextSize(font, fontSize, charStr);
        cursorX += charSize.width;
    }
    int viewWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    int leftEdge = textX + scroll.targetX;
    int rightEdge = textX + viewWidth - fontSize - scroll.targetX;
    if (cursorX < leftEdge + fontSize) {
        scroll.targetX = fmax(0, cursorX - textX - fontSize);
    } else if (cursorX > rightEdge) {
        scroll.targetX = cursorX - viewWidth + fontSize * 2;
    }
    scroll.targetX = fmin(scroll.targetX, MaxHorizontalScroll(cursorLine));
}

int FindCharPos(int line, int mouseX) {
    if (line < 0 || line >= numLines) return 0;
    int xOffset = showLineNumbers ? gutterWidth : 0;
    mouseX += scroll.currentX;
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

void ScrollCallback(GLFWwindow* win, double x, double y) {
    int mods = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if (x != 0 || mods) {
        double scrollAmount = (x != 0) ? x : y;
        scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), scroll.targetX - scrollAmount * fontSize * 2.5f));
    } else {
        scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY - y * lineHeight * 2.5f));
    }
    if (scroll.smoothScroll) scroll.velocity = 0;
}

void CharCallback(GLFWwindow* win, unsigned int c) {
    if (isprint(c)) {
        if (insertMode) {
            InsertChar((char)c);
        } else {
            if (cursorCol < lines[cursorLine].length) {
                lines[cursorLine].text[cursorCol] = (char)c;
                cursorCol++;
                isFileDirty = true;
            } else {
                InsertChar((char)c);
            }
        }
    }
    snprintf(statusMsg, sizeof(statusMsg), "Line: %d Col: %d Lines: %d | %s%s | Mode: %s", cursorLine+1, cursorCol+1, numLines, filename, isFileDirty ? " *" : "", insertMode ? "Insert" : "Overwrite");
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

bool IsInHorizontalScrollbar(double x, double y) {
    if (!showScrollbar) return false;
    int sbHeight = 10;
    int sbY = window.screen_height - sbHeight;
    int sbWidth = window.screen_width - (showScrollbar ? scrollbarWidth : 0);
    return x >= 0 && x <= sbWidth && y >= sbY && y <= window.screen_height;
}

void HandleScrollDrag(double y) {
    int sbHeight = window.screen_height;
    int maxScroll = MaxScroll();
    float ratio = y / sbHeight;
    ratio = fmin(1.0f, fmax(0.0f, ratio));
    scroll.targetY = ratio * maxScroll;
}

void HandleHorizontalScrollDrag(double x) {
    int sbWidth = window.screen_width - (showScrollbar ? scrollbarWidth : 0);
    int maxHScroll = MaxHorizontalScroll();
    float ratio = x / sbWidth;
    ratio = fmin(1.0f, fmax(0.0f, ratio));
    scroll.targetX = ratio * maxHScroll;
}

char* ConstructTextFromLines() {
    int totalLength = 0;
    for (int i = 0; i < numLines; i++) {
        totalLength += lines[i].length + 1;
    }
    char* text = (char*)malloc(totalLength + 1);
    if (text == NULL) {
        return NULL;
    }
    int pos = 0;
    for (int i = 0; i < numLines; i++) {
        memcpy(text + pos, lines[i].text, lines[i].length);
        pos += lines[i].length;
        text[pos++] = '\n';
    }
    text[pos] = '\0';
    return text;
}

void KeyCallback(GLFWwindow* win, int key, int scan, int action, int mods) {
    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (ctrl && !isSelecting) {
            selStartLine = cursorLine;
            selStartCol = cursorCol;
            isSelecting = true;
        } else if (!isSelecting) {
            selStartLine = selEndLine = cursorLine;
            selStartCol = selEndCol = cursorCol;
        }
        int prevLine = cursorLine;
        int prevCol = cursorCol;
        HandleKey(key, window.deltatime);
        switch (key) {
            case GLFW_KEY_LEFT:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                if (cursorCol > 0) cursorCol--;
                else if (cursorLine > 0) {
                    cursorLine--;
                    cursorCol = lines[cursorLine].length;
                }
                break;
            case GLFW_KEY_RIGHT:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                if (cursorCol < lines[cursorLine].length) cursorCol++;
                else if (cursorLine < numLines-1) {
                    cursorLine++;
                    cursorCol = 0;
                }
                break;  
            case GLFW_KEY_UP:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                if (cursorLine > 0) {
                    cursorLine--;
                    cursorCol = fmin(cursorCol, lines[cursorLine].length);
                }
                break;
            case GLFW_KEY_DOWN:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                if (cursorLine < numLines-1) {
                    cursorLine++;
                    cursorCol = fmin(cursorCol, lines[cursorLine].length);
                }
                break; 
            case GLFW_KEY_HOME:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                if (ctrl) {
                    cursorLine = 0;
                    cursorCol = 0;
                    scroll.targetY = 0;
                    scroll.targetX = 0;
                } else {
                    cursorCol = 0;
                    scroll.targetX = 0;
                }
                break;
            case GLFW_KEY_END:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                if (ctrl) {
                    cursorLine = numLines-1;
                    cursorCol = lines[cursorLine].length;
                    scroll.targetY = MaxScroll();
                    scroll.targetX = MaxHorizontalScroll(cursorLine);
                } else {
                    cursorCol = lines[cursorLine].length;
                    scroll.targetX = MaxHorizontalScroll(cursorLine);
                }
                break;
            case GLFW_KEY_PAGE_UP:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                cursorLine = fmax(0, cursorLine - window.screen_height/lineHeight);
                cursorCol = fmin(cursorCol, lines[cursorLine].length);
                scroll.targetY = fmax(0, scroll.targetY - window.screen_height + lineHeight);
                break;
                
            case GLFW_KEY_PAGE_DOWN:
                if (shift && !isSelecting) {
                    isSelecting = true;
                    selStartLine = prevLine;
                    selStartCol = prevCol;
                }
                cursorLine = fmin(numLines-1, cursorLine + window.screen_height/lineHeight);
                cursorCol = fmin(cursorCol, lines[cursorLine].length);
                scroll.targetY = fmin(MaxScroll(), scroll.targetY + window.screen_height - lineHeight);
                break;
            case GLFW_KEY_INSERT:
                insertMode = !insertMode;
                snprintf(statusMsg, sizeof(statusMsg), "Mode: %s", 
                         insertMode ? "Insert" : "Overwrite");
                break;
            case GLFW_KEY_A:
                if (ctrl) {
                    selStartLine = 0;
                    selStartCol = 0;
                    selEndLine = numLines - 1;
                    selEndCol = lines[numLines - 1].length;
                    cursorLine = selEndLine;
                    cursorCol = selEndCol;
                    isSelecting = true;
                    snprintf(statusMsg, sizeof(statusMsg), "Selected all text");
                }
                break;
            case GLFW_KEY_BACKSPACE:
                if (ctrl && isSelecting) DeleteSelection();
                else DeleteChar();
                break;
            case GLFW_KEY_DELETE:
                if (ctrl && isSelecting) DeleteSelection();
                else DeleteCharAfter();
                break;
            case GLFW_KEY_ENTER: InsertNewline(); break;
            case 47: if (ctrl && fontSize >= 24.0f) fontSize -= (lineHeight * 0.25f); break;
            case 93: if (ctrl && fontSize <= 150.0) fontSize += (lineHeight * 0.25f); break;
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
                    if (mods & GLFW_MOD_SHIFT) {
                        snprintf(statusMsg, sizeof(statusMsg), "Save As not implemented");
                    } else {
                        if (isFileDirty) {
                            char* textToSave = ConstructTextFromLines();
                            if (FileSave(filename, textToSave) == NULL) {
                                snprintf(statusMsg, sizeof(statusMsg), "Error saving file: %s", filename);
                            } else {
                                snprintf(statusMsg, sizeof(statusMsg), "File saved: %s", filename);
                            }
                            free(textToSave);
                        } else {
                            scroll.smoothScroll = !scroll.smoothScroll;
                        }
                    }
                }
                break;
        }
        if (isSelecting) {
            selEndLine = cursorLine;
            selEndCol = cursorCol;
        }
        if (!shift && !ctrl && (prevLine != cursorLine || prevCol != cursorCol)) {
            isSelecting = false;
            selStartLine = selEndLine = -1;
            selStartCol = selEndCol = -1;
        }
        AdjustScrollToCursor();
        if (key != GLFW_KEY_INSERT) {
            snprintf(statusMsg, sizeof(statusMsg), "Line: %d Col: %d Lines: %d | %s%s | Mode: %s", 
                    cursorLine+1, cursorCol+1, numLines, filename, isFileDirty ? " *" : "",
                    insertMode ? "Insert" : "Overwrite");
        }
    } else if (action == GLFW_RELEASE) {
        keyStates[key] = false;
        if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
            if (!(mods & GLFW_MOD_SHIFT)) {
                isSelecting = false;
            }
        }
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
            if (!(mods & GLFW_MOD_CONTROL)) {
                isSelecting = false;
            }
        }
    }
}

void MouseCallback(GLFWwindow* win, int button, int action, int mods) {
    static double lastClickTime = 0;
    static int clickCount = 0;
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (window.deltatime - lastClickTime < 0.4) {
                clickCount++;
            } else {
                clickCount = 1;
            }
            lastClickTime = window.deltatime;
            if (IsInScrollbar(x, y)) {
                isScrollbarDragging = true;
                HandleScrollDrag(y);
                return;
            } else if (IsInHorizontalScrollbar(x, y)) {
                isHorizontalScrollbarDragging = true;
                HandleHorizontalScrollDrag(x);
                return;
            }
            UpdateCursor(x, y);
            if (clickCount == 1) {
                selStartLine = cursorLine;
                selStartCol = cursorCol;
                selEndLine = cursorLine;
                selEndCol = cursorCol;
                isSelecting = true;
            } else if (clickCount == 2) {
                int lineLen = lines[cursorLine].length;
                const char* lineText = lines[cursorLine].text;
                int wordStart = cursorCol;
                while (wordStart > 0 && 
                       (isalnum(lineText[wordStart-1]) || lineText[wordStart-1] == '_')) {
                    wordStart--;
                }
                int wordEnd = cursorCol;
                while (wordEnd < lineLen && 
                       (isalnum(lineText[wordEnd]) || lineText[wordEnd] == '_')) {
                    wordEnd++;
                }
                selStartLine = cursorLine;
                selStartCol = wordStart;
                selEndLine = cursorLine;
                selEndCol = wordEnd;
                cursorCol = wordEnd;
                isSelecting = true;
            } else if (clickCount >= 3) {
                selStartLine = cursorLine;
                selStartCol = 0;
                selEndLine = cursorLine;
                selEndCol = lines[cursorLine].length;
                cursorCol = selEndCol;
                isSelecting = true;
                clickCount = 0;
            }
        } else if (action == GLFW_RELEASE) {
            isScrollbarDragging = false;
            isHorizontalScrollbarDragging = false;
            if (selStartLine == selEndLine && selStartCol == selEndCol) {
                selStartLine = selEndLine = -1;
                selStartCol = selEndCol = -1;
                isSelecting = false;
            }
        }
    }
}

void CursorPosCallback(GLFWwindow* win, double x, double y) {
    if (isScrollbarDragging) {
        HandleScrollDrag(y);
        return;
    } else if (isHorizontalScrollbarDragging) {
        HandleHorizontalScrollDrag(x);
        return;
    }
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && isSelecting) {
        if (IsInScrollbar(x, y) || IsInHorizontalScrollbar(x, y)) return;
        int oldCursorLine = cursorLine;
        int oldCursorCol = cursorCol;
        UpdateCursor(x, y);
        selEndLine = cursorLine;
        selEndCol = cursorCol;
        int edge = lineHeight * 2;
        if (y < edge) {
            scroll.targetY = fmax(0, scroll.targetY - lineHeight);
        } else if (y > window.screen_height - edge) {
            scroll.targetY = fmin(MaxScroll(), scroll.targetY + lineHeight);
        }
        int horizEdge = 50;
        if (x < horizEdge) {
            scroll.targetX = fmax(0, scroll.targetX - fontSize * 2);
        } else if (x > window.screen_width - horizEdge) {
            scroll.targetX = fmin(MaxHorizontalScroll(cursorLine), scroll.targetX + fontSize * 2);
        }
    }
}

bool IsSelValid() {
    return selStartLine != -1 && selEndLine != -1 && (selStartLine != selEndLine || selStartCol != selEndCol);
}

void DrawLineNumbers() {
    if (!showLineNumbers) return;
    int start = scroll.currentY / lineHeight;
    int end = fmin(numLines, start + window.screen_height / lineHeight + 1);
    DrawRect(0, 0, gutterWidth, window.screen_height, (Color){5, 5, 5, 255});
    for (int i = start; i < end; i++) {
        char num[16];
        snprintf(num, sizeof(num), "%d", i + 1);
        TextSize size = GetTextSize(font, fontSize, num);
        int tX = gutterWidth - size.width;
        int tY = i * lineHeight - scroll.currentY;
        if (i == cursorLine) {
            DrawRect(0, tY, gutterWidth, lineHeight, (Color){20, 20, 20, 100});
        }
        Color color = (i == cursorLine) ? textColor : (Color){150, 150, 150, 255};
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
    double timeSinceLastScroll = window.time - lastVerticalScrollTime;
    int alpha = timeSinceLastScroll < scrollbarFadeDelay ? 150 : 0;
    if (alpha > 0 || isScrollbarDragging) {
        if (isScrollbarDragging) alpha = 200;
        DrawRect(sbX, thumbY, scrollbarWidth, thumbHeight, (Color){255, 255, 255, alpha});
    }
}

void DrawHorizontalScrollbar() {
    if (!showScrollbar) return;
    int sbHeight = 10;
    int sbY = window.screen_height - sbHeight;
    int sbWidth = window.screen_width - (showScrollbar ? scrollbarWidth : 0);
    int maxHScroll = MaxHorizontalScroll();
    if (maxHScroll <= 0) return;
    float ratio = (float)sbWidth / (float)(maxHScroll + sbWidth);
    ratio = fmin(1.0f, ratio);
    int thumbWidth = fmax(30, sbWidth * ratio);
    float scrollRatio = scroll.currentX / (float)fmax(1, maxHScroll);
    scrollRatio = fmin(1.0f, fmax(0.0f, scrollRatio));
    int thumbX = scrollRatio * (sbWidth - thumbWidth);
    double timeSinceLastScroll = window.time - lastHorizontalScrollTime;
    int alpha = timeSinceLastScroll < scrollbarFadeDelay ? 150 : 0;
    if (alpha > 0 || isHorizontalScrollbarDragging) {
        if (isHorizontalScrollbarDragging) alpha = 200;
        DrawRect(thumbX, sbY, thumbWidth, sbHeight, (Color){255, 255, 255, alpha});
    }
}

void UpdateLastScrollTimes() {
    static float lastScrollY = 0;
    static float lastScrollX = 0;
    if (scroll.currentY != lastScrollY) {
        lastVerticalScrollTime = window.time;
        lastScrollY = scroll.currentY;
    }
    if (scroll.currentX != lastScrollX) {
        lastHorizontalScrollTime = window.time;
        lastScrollX = scroll.currentX;
    }
}

void DrawEditor() {
    lineHeight = GetTextSize(font, fontSize, "gj|").height;
    int textX = showLineNumbers ? gutterWidth : 0;
    gutterWidth = 1.35f * fontSize;
    int startLine = scroll.currentY / lineHeight;
    int endLine = fmin(numLines, startLine + window.screen_height / lineHeight + 1);
    static double lastBlinkTime = 0;
    static bool blinkState = true;
    const double blinkInterval = 0.5;
    if (window.time - lastBlinkTime > blinkInterval) {
        blinkState = !blinkState;
        lastBlinkTime = window.time;
    }
    bool isCursorVisible = blinkState;
    int normStartLine = selStartLine, normStartCol = selStartCol;
    int normEndLine = selEndLine, normEndCol = selEndCol;
    if (IsSelValid() && (normStartLine > normEndLine ||
        (normStartLine == normEndLine && normStartCol > normEndCol))) {
        int tempLine = normStartLine, tempCol = normStartCol;
        normStartLine = normEndLine;
        normStartCol = normEndCol;
        normEndLine = tempLine;
        normEndCol = tempCol;
    }
    for (int i = startLine; i < endLine; i++) {
        int lineY = (i * lineHeight) - scroll.currentY;
        if (i == cursorLine) {
            DrawRect(0, lineY, window.screen_width, lineHeight, (Color){7,7,7,100});
        }
    }
    for (int i = startLine; i < endLine; i++) {
        int lineY = (i * lineHeight) - scroll.currentY;
        float currentX = textX - scroll.currentX;
        const char *lineText = lines[i].text;
        int lineLength = lines[i].length;
        bool lineHasSelection = IsSelValid() && i >= normStartLine && i <= normEndLine;
        if (lineHasSelection) {
            float selStartX = textX - scroll.currentX;
            float selectionWidth = 0;
            if (i == normStartLine) {
                for (int j = 0; j < normStartCol; j++) {
                    if (j < lineLength) {
                        char charStr[2] = {lineText[j], '\0'};
                        TextSize charSize = GetTextSize(font, fontSize, charStr);
                        selStartX += charSize.width;
                    }
                }
            }
            int startCol = (i == normStartLine) ? normStartCol : 0;
            int endCol = (i == normEndLine) ? normEndCol : lineLength;
            for (int j = startCol; j < endCol; j++) {
                if (j < lineLength) {
                    char charStr[2] = {lineText[j], '\0'};
                    TextSize charSize = GetTextSize(font, fontSize, charStr);
                    selectionWidth += charSize.width;
                }
            }
            if (selectionWidth > 0) {
                if (isCursorVisible) {
                    DrawRectBorder(selStartX, lineY, selectionWidth, lineHeight, 3, (Color){100, 100, 200, 100});
                } else {
                    DrawRect(selStartX, lineY, selectionWidth, lineHeight, (Color){100, 100, 200, 100});
                }
            }
        }
        currentX = textX - scroll.currentX;
        for (int j = 0; j < lineLength; j++) {
            char charStr[2] = {lineText[j], '\0'};
            Color drawColor = textColor;
            TextSize charSize = GetTextSize(font, fontSize, charStr);
            bool isInSelection = IsSelValid() && 
                            i >= normStartLine && i <= normEndLine && 
                            j >= (i == normStartLine ? normStartCol : 0) && 
                            j < (i == normEndLine ? normEndCol : lineLength);
            if (i == cursorLine && j == cursorCol) {
                if (isInSelection || isSelecting) {
                    DrawRect(currentX, lineY, charSize.width/8, lineHeight, (Color){255,255,255,255});
                } else if (insertMode) {
                    if (isCursorVisible) {
                        DrawRect(currentX, lineY, charSize.width, lineHeight, (Color){255,255,255,255});
                        drawColor = (Color){0,0,0,255};
                    } else {
                        DrawRectBorder(currentX, lineY, charSize.width, lineHeight, 2, (Color){255,255,255,255});
                    }
                } else {
                    DrawRect(currentX, lineY, charSize.width/8, lineHeight, (Color){255,255,255,255});
                }
            }
            DrawText(currentX, lineY, font, fontSize, charStr, drawColor);
            currentX += charSize.width;
        }
        if (i == cursorLine && cursorCol == lineLength) {
            int charwidth = GetTextSize(font, fontSize, "m").width;
            bool eolInSelection = IsSelValid() && 
                               i >= normStartLine && i <= normEndLine && 
                               (i != normEndLine || cursorCol <= normEndCol);
            if (eolInSelection || isSelecting) {
                DrawRect(currentX, lineY, charwidth/8, lineHeight, (Color){255,255,255,255});
            } else if (insertMode) {
                if (isCursorVisible) {
                    DrawRect(currentX, lineY, charwidth/2, lineHeight, (Color){255,255,255,255});
                } else {
                    DrawRectBorder(currentX, lineY, charwidth/2, lineHeight, 2, (Color){255,255,255,255});
                }
            } else {
                DrawRect(currentX, lineY, charwidth/8, lineHeight, (Color){255,255,255,255});
            }
        }
    }
    DrawLineNumbers();
    DrawScrollbar();
    DrawHorizontalScrollbar();
    UpdateLastScrollTimes();
}

bool LoadFile(const char* filename) {
    char* fileContent = FileLoad(filename);
    if (!fileContent) {
        snprintf(statusMsg, sizeof(statusMsg), "Error: Could not load file");
        return false;
    }
    char* line = strtok(fileContent, "\n");
    numLines = 0;
    while (line != NULL && numLines < 1024) {
        lines[numLines].text = strdup(line);
        lines[numLines].length = strlen(line);
        numLines++;
        line = strtok(NULL, "\n");
    }
    free(fileContent);
    snprintf(statusMsg, sizeof(statusMsg), "File loaded: %s", filename);
    return true;
}

int main(int argc, char *argv[]) {
    window.opt.transparent = true;
    WindowInit(1920, 1080, "Text Editor");
    font = LoadFont("./res/fonts/Monocraft.ttf");//font.nearest = false;
    //shaderfont.hotreloading = true;//shaderdefault.hotreloading = true;
    if (argc > 1) {
        if (argv[1][0] == '-') {
            if (argv[1][1] == 'h') {
                printf("Usage: %s [filename]\n", argv[0]);
                return 0;
            }
        }
        strcpy(filename, argv[1]);
        if (FileExists(filename)) {
            if (!LoadFile(filename)) {
                InitLine(0);
            }
        } else {
            InitLine(0);
        }
    } else {
        InitLine(0);
    }
    glfwSetCharCallback(window.w, CharCallback);
    glfwSetKeyCallback(window.w, KeyCallback);
    glfwSetScrollCallback(window.w, ScrollCallback);
    glfwSetMouseButtonCallback(window.w, MouseCallback);
    glfwSetCursorPosCallback(window.w, CursorPosCallback);
    //glClearColor(0.0f, 0.0f, 0.0f, 0.75f);
    while (!WindowState()) {
        WindowClear();
        UpdateScroll(window.deltatime);
        DrawEditor();
        // Fps
            static char fpsText[16];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", window.fps);
            DrawText(0, -3, font, 25, fpsText, (Color){245, 245, 245, 145});
        WindowProcess();
    }
    FreeLines();
    WindowClose();
    return 0;
}


// Features

// selection using lines instead of rectangles and optimize the selection
// line wrapping
// command line with all options to eval commads inside of the editor
// modline with ln col encoding liene endings clock file status git dir 
// file manager buffer
// terminal buffer
// pipe commands to buffer
// undo,redo,save as
// shortcut open,recent file,ident,comment
// timeline of files old 7 days
// buffer window managment and tabs
// sudo save using like pkexec
// markdown rendering
// code folding
// code snippets
// jupyter notebooks like cells with whatever interpreter you want

// Modules

// minimap
// diff folder,clipboard,git,timeline
// search & replace & regex 
// autopair brackets,quotes 
// color picker
// syntax language mode treesitter highlight
// lsp for autocompletion
// git info
// copilot like ai
// snyk anlysis
