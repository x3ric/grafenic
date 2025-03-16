#include <float.h>

typedef struct { char* text; int length; } Line;
typedef struct { float targetY, currentY, targetX, currentX, velocity; bool smoothScroll; } ScrollState;

#define MAXTEXT 16384
#define MAX_FILENAME 256

Line lines[MAXTEXT] = {0};
int numLines = 1, cursorLine = 0, cursorCol = 0, lineHeight = 0, gutterWidth = 0;
bool isSelecting = false, showLineNumbers = true, showStatusBar = true, showScrollbar = true;
bool isFileDirty = false, isScrollbarDragging = false, isHorizontalScrollbarDragging = false;
bool insertMode = true, showMinimap = true, isMinimapDragging = false, showModeline = true;
int selStartLine = -1, selStartCol = -1, selEndLine = -1, selEndCol = -1;
int statusBarHeight = 25, scrollbarWidth = 12, minimapWidth = 100, modeLineHeight = 20;
double fontSize = 24.0f;
float minimapScale = 0.2f;
char filename[MAX_FILENAME] = "Untitled.txt", statusMsg[256] = "";
ScrollState scroll = {0, 0, 0, 0, 0, true};
static double lastVerticalScrollTime = 0, lastHorizontalScrollTime = 0, scrollbarFadeDelay = 1.5;
static double lastKeyTime[GLFW_KEY_LAST + 1] = {0};
static bool keyStates[GLFW_KEY_LAST + 1] = {false};

Color textColor = {220, 220, 220, 255};
Color minimapBgColor = {10, 10, 10, 180};
Color minimapVisibleAreaColor = {40, 40, 40, 180};
Color minimapCursorColor = {215, 215, 215, 245};
Color minimapTextColor = {150, 150, 150, 180};
Color modeLineBgColor = {30, 30, 30, 125};
Color modeLineTextColor = {200, 200, 200, 255};

bool IsSelValid() { return selStartLine != -1 && selEndLine != -1 && (selStartLine != selEndLine || selStartCol != selEndCol); }

void InitLine(int idx) {
    if (idx >= MAXTEXT) return;
    lines[idx].text = malloc(MAXTEXT);
    if (!lines[idx].text) { fprintf(stderr, "Memory error\n"); exit(1); }
    lines[idx].text[0] = '\0';
    lines[idx].length = 0;
}

void FreeLines() {
    for (int i = 0; i < numLines; i++) {
        free(lines[i].text);
        lines[i].text = NULL;
    }
    numLines = 0;
}

void InsertChar(char c) {
    if (lines[cursorLine].length < MAXTEXT - 1) {
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

int MaxScroll() {
    int maxVisLines = (window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0)) / lineHeight;
    return fmax(0, (numLines - maxVisLines) * lineHeight);
}

int MaxHorizontalScroll() {
    static int cachedMaxWidth = 0, cachedFontSize = 0, cachedNumLines = 0, cachedScreenWidth = 0;
    if (cachedFontSize == fontSize && cachedNumLines == numLines && cachedScreenWidth == window.screen_width) return cachedMaxWidth;
    int maxWidth = 0;
    for (int i = 0; i < numLines; i++) {
        TextSize size = GetTextSize(font, fontSize, lines[i].text);
        if (size.width > maxWidth) maxWidth = size.width;
    }
    int viewWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    cachedMaxWidth = fmax(0, maxWidth - viewWidth + fontSize);
    cachedFontSize = fontSize;
    cachedNumLines = numLines;
    cachedScreenWidth = window.screen_width;
    return cachedMaxWidth;
}

void UpdateGutterWidth() {
    float baseDigitWidth = (1.35f * fontSize) / 2;
    int maxVisLineNum = fmin(numLines, scroll.currentY / lineHeight + window.screen_height / lineHeight + 1);
    int digits = maxVisLineNum > 0 ? (int)floor(log10(maxVisLineNum)) + 1 : 1;
    gutterWidth = fmax(digits * baseDigitWidth, baseDigitWidth);
}

void AdjustScrollToCursor() {
    int cursorY = cursorLine * lineHeight;
    int viewStart = scroll.targetY;
    int viewEnd = scroll.targetY + window.screen_height - lineHeight - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
    if (cursorY < viewStart) scroll.targetY = fmax(0, cursorY - lineHeight);
    else if (cursorY > viewEnd) scroll.targetY = fmin(MaxScroll(), cursorY - window.screen_height + lineHeight + (showStatusBar ? statusBarHeight : 0) + (showModeline ? modeLineHeight : 0));
    if (cursorLine < 0 || cursorLine >= numLines) return;
    int textX = showLineNumbers ? gutterWidth : 0;
    int viewWidth = window.screen_width - textX - (showScrollbar ? scrollbarWidth : 0);
    int documentCursorX = textX;
    for (int j = 0; j < cursorCol; j++) {
        char charStr[2] = {lines[cursorLine].text[j], '\0'};
        TextSize charSize = GetTextSize(font, fontSize, charStr);
        documentCursorX += charSize.width;
    }
    int visibleLeft = scroll.targetX;
    int visibleRight = scroll.targetX + viewWidth;
    if (documentCursorX < visibleLeft + fontSize) scroll.targetX = fmax(0, documentCursorX - fontSize);
    else if (documentCursorX > visibleRight - fontSize) scroll.targetX = documentCursorX - viewWidth + fontSize * 2;
    scroll.targetX = fmin(scroll.targetX, MaxHorizontalScroll());
}

void UpdateScroll(double dt) {
    dt = fmin(dt, 0.05f);
    if (scroll.smoothScroll) {
        float factor = 10.0f;
        float t = 1.0f - exp(-factor * dt);
        float deltaY = scroll.targetY - scroll.currentY;
        float deltaX = scroll.targetX - scroll.currentX;
        const float EPSILON = 0.1f;
        if (fabs(deltaY) > EPSILON) {
            scroll.currentY += deltaY * t;
            if (fabs(deltaY) < EPSILON * 2) scroll.currentY = scroll.targetY;
        }
        if (fabs(deltaX) > EPSILON) {
            scroll.currentX += deltaX * t;
            if (fabs(deltaX) < EPSILON * 2) scroll.currentX = scroll.targetX;
        }
    } else {
        scroll.currentY = scroll.targetY;
        scroll.currentX = scroll.targetX;
    }
}

int FindCharPos(int line, int mouseX) {
    if (line < 0 || line >= numLines) return 0;
    int xOffset = showLineNumbers ? gutterWidth : 0;
    mouseX += scroll.currentX;
    if (mouseX <= xOffset) return 0;
    const char* lineText = lines[line].text;
    int lineLen = lines[line].length;
    float currentX = xOffset;
    for (int i = 0; i <= lineLen; i++) {
        float charWidth;
        if (i < lineLen) {
            char charStr[2] = {lineText[i], '\0'};
            charWidth = GetTextSize(font, fontSize, charStr).width;
        } else {
            charWidth = GetTextSize(font, fontSize, "m").width / 2;
        }
        float nextX = currentX + charWidth;
        if (mouseX < (currentX + nextX) / 2) return i;
        currentX = nextX;
    } 
    return lineLen;
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
        strncat(lines[startLine].text + startCol, lines[endLine].text + endCol, MAXTEXT - startCol - 1);
        lines[startLine].length = startCol + strlen(lines[endLine].text + endCol);
        int linesToRemove = endLine - startLine;
        for (int i = endLine + 1; i < numLines; i++)
            lines[i - linesToRemove] = lines[i];  
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
        double amount = (x != 0) ? x : y;
        scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), scroll.targetX - amount * fontSize * 2.5f));
    } else {
        scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY - y * lineHeight * 2.5f));
    }
    if (scroll.smoothScroll) scroll.velocity = 0;
}

void CharCallback(GLFWwindow* win, unsigned int c) {
    if (isprint(c)) {
        const int MAX_LINE_EXTENSION = 3;
        if (insertMode) {
            InsertChar((char)c);
        } else {
            if (cursorCol < lines[cursorLine].length) {
                lines[cursorLine].text[cursorCol] = (char)c;
                cursorCol++;
            } else {
                if (lines[cursorLine].length < MAXTEXT - MAX_LINE_EXTENSION - 1) {
                    lines[cursorLine].text[lines[cursorLine].length] = (char)c;
                    lines[cursorLine].length++;
                    cursorCol = lines[cursorLine].length;
                }
            }
        }
        if (lines[cursorLine].length >= MAXTEXT - 1) {
            lines[cursorLine].length = MAXTEXT - 1;
            lines[cursorLine].text[lines[cursorLine].length] = '\0';
        }
    }
    snprintf(statusMsg, sizeof(statusMsg), "Line: %d Col: %d Lines: %d | %s%s | Mode: %s", 
             cursorLine + 1, cursorCol + 1, numLines, filename, isFileDirty ? " *" : "", 
             insertMode ? "Insert" : "Overwrite");
    
    isFileDirty = true;
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
    return x >= window.screen_width - scrollbarWidth && y >= 0 && y <= window.screen_height;
}

bool IsInHorizontalScrollbar(double x, double y) {
    if (!showScrollbar) return false;
    int sbHeight = 10;
    return x >= 0 && x <= window.screen_width - (showScrollbar ? scrollbarWidth : 0) && y >= window.screen_height - sbHeight;
}

void HandleScrollDrag(double y) {
    float ratio = fmin(1.0f, fmax(0.0f, y / window.screen_height));
    scroll.targetY = ratio * MaxScroll();
}

void HandleHorizontalScrollDrag(double x) {
    int sbWidth = window.screen_width - (showScrollbar ? scrollbarWidth : 0);
    float ratio = fmin(1.0f, fmax(0.0f, x / sbWidth));
    scroll.targetX = ratio * MaxHorizontalScroll();
}

char* ConstructTextFromLines() {
    int totalLength = 0;
    for (int i = 0; i < numLines; i++) totalLength += lines[i].length + 1;
    char* text = malloc(totalLength + 1);
    if (!text) return NULL;
    int pos = 0;
    for (int i = 0; i < numLines; i++) {
        memcpy(text + pos, lines[i].text, lines[i].length);
        pos += lines[i].length;
        if (i < numLines - 1 || (i == numLines - 1 && lines[i].length > 0)) text[pos++] = '\n';
    }
    text[pos] = '\0';
    return text;
}

void DrawLineNumbers() {
    if (!showLineNumbers) return;
    int start = scroll.currentY / lineHeight;
    int end = fmin(numLines, start + window.screen_height / lineHeight + 1);
    DrawRectBatch(0, 0, gutterWidth, window.screen_height, (Color){5, 5, 5, 255});
    for (int i = start; i < end; i++) {
        char num[16];
        snprintf(num, sizeof(num), "%d", i + 1);
        TextSize size = GetTextSize(font, fontSize, num);
        int tx = gutterWidth - size.width;
        int ty = i * lineHeight - scroll.currentY;
        if (i == cursorLine) DrawRectBatch(0, ty, gutterWidth, lineHeight, (Color){20, 20, 20, 100});
        Color color = (i == cursorLine) ? textColor : (Color){150, 150, 150, 255};
        DrawTextBatch(tx, ty, font, fontSize, num, color);
    }
    FlushRectBatch();
    FlushTextBatch();
}

float GetDocumentHeight() {
    return numLines * lineHeight;
}

float GetVisibleHeightRatio() {
    float docHeight = GetDocumentHeight();
    if (docHeight <= 0) return 1.0f;
    int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
    return fmin(1.0f, visibleHeight / docHeight);
}

int GetDocumentMaxWidth() {
    int maxWidth = 0;
    for (int i = 0; i < numLines; i++) {
        TextSize size = GetTextSize(font, fontSize, lines[i].text);
        if (size.width > maxWidth) maxWidth = size.width;
    }
    return maxWidth;
}

float GetVisibleWidthRatio() {
    int maxWidth = GetDocumentMaxWidth();
    if (maxWidth <= 0) return 1.0f;
    int visibleWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    return fmin(1.0f, visibleWidth / (float)maxWidth);
}

void DrawScrollbar() {
    if (!showScrollbar) return;
    int sbX = window.screen_width - scrollbarWidth;
    float visibleRatio = GetVisibleHeightRatio();
    int thumbHeight = fmax(30, window.screen_height * visibleRatio);
    int maxScroll = MaxScroll();
    float scrollRatio = (maxScroll > 0) ? fmin(1.0f, fmax(0.0f, scroll.currentY / (float)maxScroll)) : 0;
    int thumbY = scrollRatio * (window.screen_height - thumbHeight);
    double timeSince = window.time - lastVerticalScrollTime;
    int alpha = timeSince < scrollbarFadeDelay ? 150 : 0;
    if (alpha > 0 || isScrollbarDragging) {
        if (isScrollbarDragging) alpha = 200;
        DrawRectBatch(sbX, thumbY, scrollbarWidth, thumbHeight, (Color){255, 255, 255, alpha});
    }
    FlushRectBatch();
}

void DrawHorizontalScrollbar() {
    if (!showScrollbar) return;
    int sbHeight = 10;
    int sbY = window.screen_height - sbHeight;
    int sbWidth = window.screen_width - (showScrollbar ? scrollbarWidth : 0);
    int maxHScroll = MaxHorizontalScroll();
    if (maxHScroll <= 0) return;
    float visibleRatio = GetVisibleWidthRatio();
    int thumbWidth = fmax(30, sbWidth * visibleRatio);
    float scrollRatio = fmin(1.0f, fmax(0.0f, scroll.currentX / (float)maxHScroll));
    int thumbX = scrollRatio * (sbWidth - thumbWidth);
    double timeSince = window.time - lastHorizontalScrollTime;
    int alpha = timeSince < scrollbarFadeDelay ? 150 : 0;
    if (alpha > 0 || isHorizontalScrollbarDragging) {
        if (isHorizontalScrollbarDragging) alpha = 200;
        DrawRectBatch(thumbX, sbY, thumbWidth, sbHeight, (Color){255, 255, 255, alpha});
    }
    FlushRectBatch();
}

void DrawMinimap() {
    if (!showMinimap) return;
    int minimapX = window.screen_width - minimapWidth - (showScrollbar ? scrollbarWidth : 0);
    DrawRectBatch(minimapX, 0, minimapWidth, window.screen_height, minimapBgColor);
    int visibleStartLine = (int)(scroll.currentY / lineHeight);
    int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
    int visibleEndLine = visibleStartLine + (visibleHeight / lineHeight);
    visibleStartLine = MaxInt(0, visibleStartLine);
    visibleEndLine = MinInt(numLines - 1, visibleEndLine);
    float minimapLineHeight = fmax(1.0f, lineHeight * minimapScale);
    float contentScale = fmin(1.0f, (float)window.screen_height / (minimapLineHeight * numLines));
    for (int i = 0; i < numLines; i++) {
        float y = i * minimapLineHeight * contentScale;
        if (y >= window.screen_height) break;
        float scaledLineHeight = fmax(1.0f, minimapLineHeight * contentScale - 1);
        int length = lines[i].length;
        if (length > 0) {
            float lineWidthFactor = fmin(0.9f, length / 120.0f);
            float lineWidth = fmin(minimapWidth - 4, minimapWidth * lineWidthFactor);
            Color lineColor = minimapTextColor;
            DrawRectBatch(minimapX + 2, y, lineWidth, scaledLineHeight, lineColor);
        }
    }
    float highlightY = visibleStartLine * minimapLineHeight * contentScale;
    float highlightHeight = (visibleEndLine - visibleStartLine + 1) * minimapLineHeight * contentScale;
    highlightHeight = fmax(highlightHeight, 8);
    DrawRectBatch(minimapX, highlightY, minimapWidth, highlightHeight, minimapVisibleAreaColor);
    float cursorY = cursorLine * minimapLineHeight * contentScale;
    DrawRectBatch(minimapX, cursorY, minimapWidth, 2, minimapCursorColor);
    FlushRectBatch();
}

void UpdateLastScrollTimes() {
    static float lastScrollY = 0, lastScrollX = 0;
    if (scroll.currentY != lastScrollY) {
        lastVerticalScrollTime = window.time;
        lastScrollY = scroll.currentY;
    }
    if (scroll.currentX != lastScrollX) {
        lastHorizontalScrollTime = window.time;
        lastScrollX = scroll.currentX;
    }
}
void DrawModeline() {
    if (!showModeline) return;
    int y = window.screen_height - (showStatusBar ? statusBarHeight : 0) - modeLineHeight;
    Color bgColor = {18, 20, 26, 245};
    DrawRect(0, y, window.screen_width, modeLineHeight, bgColor);
    float fontSizeModeline = fontSize * 0.75;
    time_t rawtime;
    struct tm* timeinfo;
    char timeString[20];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, sizeof(timeString), "%H:%M", timeinfo);
    TextSize lineHeight = GetTextSize(font, fontSizeModeline, "Aj|");
    int textY = y + (modeLineHeight - lineHeight.height) / 2;
    int leftX = 10;
    char statusIndicator[2] = {0};
    Color statusColor;
    if (isFileDirty) {
        statusIndicator[0] = '*'; // Asterisk for modified (instead of bullet point)
        statusColor = (Color){230, 100, 100, 255}; // Red
    } else {
        statusIndicator[0] = '+'; // Plus for saved (instead of check mark)
        statusColor = (Color){100, 230, 100, 255}; // Green
    }
    DrawText(leftX, textY, font, fontSizeModeline, statusIndicator, statusColor);
    leftX += GetTextSize(font, fontSizeModeline, statusIndicator).width + 6;
    char fileDisplay[64] = {0};
    char* displayName = filename;
    char* lastSlash = strrchr(filename, '/');
    int fileMaxWidth = window.screen_width * 0.4;
    if (GetTextSize(font, fontSizeModeline, filename).width > fileMaxWidth && lastSlash) {
        displayName = lastSlash + 1;
    }
    if (GetTextSize(font, fontSizeModeline, displayName).width > fileMaxWidth) {
        int maxChars = fileMaxWidth / (fontSizeModeline * 0.6);
        int nameLen = strlen(displayName);
        
        if (nameLen > maxChars) {
            snprintf(fileDisplay, sizeof(fileDisplay), "%.*s...", maxChars - 3, displayName);
        } else {
            strcpy(fileDisplay, displayName);
        }
    } else {
        strcpy(fileDisplay, displayName);
    }
    Color fileColor = {220, 220, 225, 255};
    DrawText(leftX, textY, font, fontSizeModeline, fileDisplay, fileColor);
    leftX += GetTextSize(font, fontSizeModeline, fileDisplay).width + 10;
    int rightX = window.screen_width - 10;
    Color clockColor = {180, 180, 190, 255};
    TextSize clockSize = GetTextSize(font, fontSizeModeline, timeString);
    rightX -= clockSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, timeString, clockColor);
    rightX -= 15;
    DrawRect(rightX, y + 4, 1, modeLineHeight - 8, (Color){60, 65, 75, 200});
    rightX -= 10;
    char modeText[12];
    snprintf(modeText, sizeof(modeText), "%s", insertMode ? "INS" : "OVR");
    Color modeColor = insertMode ? 
                     (Color){160, 200, 220, 255} : // Blue for insert
                     (Color){220, 180, 160, 255};  // Orange for overwrite
    TextSize modeSize = GetTextSize(font, fontSizeModeline, modeText);
    rightX -= modeSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, modeText, modeColor);
    rightX -= 15;
    DrawRect(rightX, y + 4, 1, modeLineHeight - 8, (Color){60, 65, 75, 200});
    rightX -= 10;
    char posText[32];
    snprintf(posText, sizeof(posText), "Ln %d Col %d", cursorLine + 1, cursorCol + 1);
    Color posColor = {180, 190, 210, 255};
    TextSize posSize = GetTextSize(font, fontSizeModeline, posText);
    rightX -= posSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, posText, posColor);
    rightX -= 15;
    DrawRect(rightX, y + 4, 1, modeLineHeight - 8, (Color){60, 65, 75, 200});
    rightX -= 10;
    if (rightX - leftX > 100) {
        char encText[16] = "UTF-8 LF";
        Color encColor = {150, 150, 160, 200};
        TextSize encSize = GetTextSize(font, fontSizeModeline, encText);
        rightX -= encSize.width;
        DrawText(rightX, textY, font, fontSizeModeline, encText, encColor);
    }
}

void InsertNewline() {
    if (numLines >= MAXTEXT) return;
    memmove(&lines[cursorLine+1], &lines[cursorLine], (numLines - cursorLine) * sizeof(Line));
    numLines++;
    InitLine(cursorLine + 1);
    strcpy(lines[cursorLine+1].text, &lines[cursorLine].text[cursorCol]);
    lines[cursorLine+1].length = lines[cursorLine].length - cursorCol;
    lines[cursorLine].text[cursorCol] = '\0';
    lines[cursorLine].length = cursorCol;
    cursorLine++;
    cursorCol = 0;
    scroll.targetX = 0;
    int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0);
    int newLineY = cursorLine * lineHeight;
    int cursorViewPosition = newLineY - scroll.currentY;
    if (cursorViewPosition < 0 || cursorViewPosition >= visibleHeight)
        scroll.targetY = newLineY - (visibleHeight / 2);
    scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
    UpdateGutterWidth();
    isFileDirty = true;
}

bool LoadFile(const char* filename) {
    char* fileContent = FileLoad(filename);
    if (!fileContent) {
        snprintf(statusMsg, sizeof(statusMsg), "Error: Could not load file");
        return false;
    }
    numLines = 0;
    char* start = fileContent;
    char* end;
    while (numLines < MAXTEXT) {
        end = memchr(start, '\n', strlen(start));
        size_t len = end ? (size_t)(end - start) : strlen(start);
        InitLine(numLines);
        memcpy(lines[numLines].text, start, len);
        lines[numLines].text[len] = '\0';
        lines[numLines].length = len;
        numLines++;
        if (!end) break;
        start = end + 1;
    }
    free(fileContent);
    snprintf(statusMsg, sizeof(statusMsg), "File loaded: %s", filename);
    return true;
}

float GetCursorXPosition() {
    int textX = showLineNumbers ? gutterWidth : 0;
    float cursorX = textX;
    for (int i = 0; i < cursorCol; i++) {
        char charStr[2] = {lines[cursorLine].text[i], '\0'};
        TextSize size = GetTextSize(font, fontSize, charStr);
        cursorX += size.width;
    }
    return cursorX;
}

float GetCursorScreenX() {
    return GetCursorXPosition() - scroll.currentX;
}

void DeleteWordBackward() {
    if (cursorCol == 0) {
        DeleteChar();
        return;
    }
    int origCol = cursorCol;
    int newCol = cursorCol - 1;
    while (newCol > 0 && isspace((unsigned char)lines[cursorLine].text[newCol]))
        newCol--; 
    while (newCol > 0 && !isspace((unsigned char)lines[cursorLine].text[newCol-1]))
        newCol--;
    selStartLine = cursorLine;
    selStartCol = newCol;
    selEndLine = cursorLine;
    selEndCol = origCol;
    DeleteSelection();
}

void DeleteWordForward() {
    if (cursorCol >= lines[cursorLine].length) {
        DeleteCharAfter();
        return;
    }
    int origCol = cursorCol;
    int newCol = cursorCol;
    while (newCol < lines[cursorLine].length && isspace((unsigned char)lines[cursorLine].text[newCol]))
        newCol++;
    while (newCol < lines[cursorLine].length && !isspace((unsigned char)lines[cursorLine].text[newCol]))
        newCol++; 
    selStartLine = cursorLine;
    selStartCol = origCol;
    selEndLine = cursorLine;
    selEndCol = newCol;
    DeleteSelection();
}

void Zoom(float amount) {
    float cursorScreenX = GetCursorScreenX();
    float cursorScreenY = cursorLine * lineHeight - scroll.currentY;
    float oldFontSize = fontSize;
    fontSize += amount;
    fontSize = fmax(8.0f, fmin(72.0f, fontSize));
    modeLineHeight = (int)(fontSize * 0.8f);
    lineHeight = (int)GetTextSize(font, fontSize, "gj|").height;
    float ratio = fontSize / oldFontSize;
    float newCursorY = cursorLine * lineHeight;
    float newCursorX = cursorScreenX * ratio;
    scroll.targetY = newCursorY - cursorScreenY;
    scroll.targetX = GetCursorXPosition() - newCursorX;
    scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
    scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), scroll.targetX));
    UpdateGutterWidth();
}

void DrawEditor() {
    static float lastFontSize = 0;
    static float cachedScale = 0;
    static int cachedLineHeight = 0;
    if (lastFontSize != fontSize) {
        lastFontSize = fontSize;
        cachedScale = fontSize / font.fontSize;
        cachedLineHeight = (int)GetTextSize(font, fontSize, "gj|").height;
    }
    float scale = cachedScale;
    lineHeight = cachedLineHeight;
    UpdateGutterWidth();
    int textX = showLineNumbers ? gutterWidth : 0;
    int startLine = MaxInt(0, (int)(scroll.currentY / lineHeight));
    int endLine = MinInt(numLines, startLine + (window.screen_height / lineHeight) + 2);
    static double lastBlinkTime = 0;
    static bool blinkState = true;
    bool isCursorVisible = (window.time - lastBlinkTime > 0.5) ? (lastBlinkTime = window.time, blinkState = !blinkState, blinkState) : blinkState;
    int normStartLine = selStartLine, normStartCol = selStartCol;
    int normEndLine = selEndLine, normEndCol = selEndCol;
    bool isSelectionValid = IsSelValid();
    if (isSelectionValid && (normStartLine > normEndLine || (normStartLine == normEndLine && normStartCol > normEndCol))) {
        int tempLine = normStartLine, tempCol = normStartCol;
        normStartLine = normEndLine; normStartCol = normEndCol;
        normEndLine = tempLine; normEndCol = tempCol;
    }
    struct { char* text; float* widths; int length; } widthCache = {NULL, NULL, 0};
    for (int i = startLine; i < endLine; i++) {
        const int lineY = (int)((i * lineHeight) - (int)scroll.currentY);
        const char *lineText = lines[i].text;
        const int lineLength = lines[i].length;
        float* cumWidths;
        if (widthCache.text != lineText || widthCache.length != lineLength) {
            if (widthCache.widths) free(widthCache.widths);
            widthCache.widths = malloc((lineLength + 1) * sizeof(float));
            widthCache.text = (char*)lineText;
            widthCache.length = lineLength;
            cumWidths = widthCache.widths;
            cumWidths[0] = 0;
            for (int j = 0; j < lineLength; j++) {
                const unsigned char c = (unsigned char)lineText[j];
                float charWidth = 0;
                if (c >= 32 && c < 32 + MAX_GLYPHS) {
                    const Glyph *g = &font.glyphs[c - 32];
                    charWidth = g->xadvance * scale;
                }
                cumWidths[j + 1] = cumWidths[j] + charWidth;
            }
        } else {
            cumWidths = widthCache.widths;
        }
        const bool lineHasSelection = isSelectionValid && i >= normStartLine && i <= normEndLine;
        if (lineHasSelection) {
            int selStart = Clamp((i == normStartLine) ? normStartCol : 0, 0, lineLength);
            int selEnd = Clamp((i == normEndLine) ? normEndCol : lineLength, 0, lineLength);
            if (selStart != selEnd) {
                int selStartX = (int)(textX - scroll.currentX + cumWidths[selStart]);
                int selEndX = (int)(textX - scroll.currentX + cumWidths[selEnd]);
                int selectionWidth = selEndX - selStartX;
                DrawRectBatch(selStartX, lineY, selectionWidth < 1 ? 1 : selectionWidth, lineHeight, (Color){100,100,200,100});
            }
        }
        const float drawX = textX - scroll.currentX;
        DrawTextBatch(drawX, lineY, font, fontSize, lineText, textColor);
        FlushTextBatch();
        if (i == cursorLine) {
            const int clampedCol = Clamp(cursorCol, 0, lineLength);
            const float cursorX = textX - scroll.currentX + cumWidths[clampedCol];
            float charWidth = (clampedCol < lineLength) ? cumWidths[clampedCol + 1] - cumWidths[clampedCol] : font.glyphs[0].xadvance * scale;
            if (lineHasSelection || !insertMode) {
                if (isCursorVisible) {
                    DrawRectBatch(cursorX, lineY, charWidth/3, lineHeight, (Color){255,255,255,255});
                    FlushRectBatch();
                }
            } else if (isCursorVisible) {
                int intCharWidth = (int)charWidth;
                DrawRectBatch(cursorX, lineY, intCharWidth, lineHeight, (Color){255,255,255,255});
                FlushRectBatch();
                if (clampedCol < lineLength) {
                    char currentChar[2] = {lineText[clampedCol], '\0'};
                    DrawTextBatch(cursorX, lineY, font, fontSize, currentChar, (Color){0,0,0,255});
                    FlushTextBatch();
                }
            } else {
                int intCharWidth = (int)charWidth;
                DrawRectBorder(cursorX, lineY, intCharWidth, lineHeight, 3, (Color){255,255,255,255});
            }
        }
    }
    if (widthCache.widths) {
        free(widthCache.widths);
        widthCache.widths = NULL;
    }
}

void KeyCallback(GLFWwindow* win, int key, int scan, int action, int mods) {
    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        int prevLine = cursorLine, prevCol = cursorCol;
        if (shift && !isSelecting) {
            selStartLine = cursorLine;
            selStartCol = cursorCol;
            isSelecting = true;
        }
        HandleKey(key, window.deltatime);
        switch (key) {
            case GLFW_KEY_LEFT:
                if (ctrl) {
                    if (cursorCol > 0) {
                        while (cursorCol > 0 && isspace((unsigned char)lines[cursorLine].text[cursorCol-1])) cursorCol--;
                        while (cursorCol > 0 && !isspace((unsigned char)lines[cursorLine].text[cursorCol-1])) cursorCol--;
                    } else if (cursorLine > 0) {
                        cursorLine--; cursorCol = lines[cursorLine].length;
                    }
                } else {
                    if (cursorCol > 0) cursorCol--;
                    else if (cursorLine > 0) { cursorLine--; cursorCol = lines[cursorLine].length; }
                }
                break;
            case GLFW_KEY_RIGHT:
                if (ctrl) {
                    if (cursorCol < lines[cursorLine].length) {
                        while (cursorCol < lines[cursorLine].length && !isspace((unsigned char)lines[cursorLine].text[cursorCol])) cursorCol++;
                        while (cursorCol < lines[cursorLine].length && isspace((unsigned char)lines[cursorLine].text[cursorCol])) cursorCol++;
                    } else if (cursorLine < numLines - 1) { cursorLine++; cursorCol = 0; }
                } else {
                    if (cursorCol < lines[cursorLine].length) cursorCol++;
                    else if (cursorLine < numLines - 1) { cursorLine++; cursorCol = 0; }
                }
                break;
            case GLFW_KEY_UP:
                if (ctrl) {
                    static int upDownTargetCol = -1;
                    if (upDownTargetCol < 0) upDownTargetCol = cursorCol;
                    cursorLine = fmax(0, cursorLine - 3);
                    cursorCol = fmin(upDownTargetCol, lines[cursorLine].length);
                } else {
                    static int upDownTargetCol = -1;
                    upDownTargetCol = -1;
                    if (cursorLine > 0) { cursorLine--; cursorCol = fmin(cursorCol, lines[cursorLine].length); }
                }
                break;
            case GLFW_KEY_DOWN:
                if (ctrl) {
                    static int upDownTargetCol = -1;
                    if (upDownTargetCol < 0) upDownTargetCol = cursorCol;
                    cursorLine = fmin(numLines-1, cursorLine + 3);
                    cursorCol = fmin(upDownTargetCol, lines[cursorLine].length);
                } else {
                    static int upDownTargetCol = -1;
                    upDownTargetCol = -1;
                    if (cursorLine < numLines-1) { cursorLine++; cursorCol = fmin(cursorCol, lines[cursorLine].length); }
                }
                break;
            case GLFW_KEY_HOME:
                if (ctrl) { cursorLine = 0; cursorCol = 0; scroll.targetY = scroll.targetX = 0; }
                else { cursorCol = 0; scroll.targetX = 0; }
                break;
            case GLFW_KEY_END:
                if (ctrl) {
                    cursorLine = numLines-1;
                    cursorCol = lines[cursorLine].length;
                    scroll.targetY = MaxScroll();
                    scroll.targetX = MaxHorizontalScroll();
                } else { cursorCol = lines[cursorLine].length; scroll.targetX = MaxHorizontalScroll(); }
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
            case GLFW_KEY_INSERT:
                insertMode = !insertMode;
                snprintf(statusMsg, sizeof(statusMsg), "Mode: %s", insertMode ? "Insert" : "Overwrite");
                break;
            case GLFW_KEY_A:
                if (ctrl) {
                    selStartLine = 0; selStartCol = 0;
                    selEndLine = numLines - 1; selEndCol = lines[numLines - 1].length;
                    cursorLine = selEndLine; cursorCol = selEndCol;
                    isSelecting = true;
                    snprintf(statusMsg, sizeof(statusMsg), "Selected all text");
                }
                break;
            case GLFW_KEY_BACKSPACE:
                if (IsSelValid()) DeleteSelection();
                else if (ctrl) DeleteWordBackward();
                else DeleteChar();
                break;   
            case GLFW_KEY_DELETE:
                if (IsSelValid()) DeleteSelection();
                else if (ctrl) DeleteWordForward();
                else DeleteCharAfter();
                break;
            case GLFW_KEY_ENTER:
                if (IsSelValid()) DeleteSelection();
                InsertNewline();
                break;
            case 47: // Zoom out (Ctrl + '-')
                if (ctrl && fontSize >= 12.0f) Zoom(-2.0f);
                break;
            case 93: // Zoom in (Ctrl + '+')
                if (ctrl && fontSize <= 72.0f) Zoom(2.0f);
                break;
            case GLFW_KEY_L:
                if (ctrl) {
                    showLineNumbers = !showLineNumbers;
                    snprintf(statusMsg, sizeof(statusMsg), "Line numbers: %s", showLineNumbers ? "ON" : "OFF");
                }
                break;
            case GLFW_KEY_R:
                if (ctrl) {
                    showScrollbar = !showScrollbar;
                    snprintf(statusMsg, sizeof(statusMsg), "Scrollbar: %s", showScrollbar ? "ON" : "OFF");
                }
                break;
            case GLFW_KEY_M:
                if (ctrl) {
                    showMinimap = !showMinimap;
                    snprintf(statusMsg, sizeof(statusMsg), "Minimap: %s", showMinimap ? "ON" : "OFF");
                }
                break;
            case GLFW_KEY_N:
                if (ctrl) {
                    showModeline = !showModeline;
                    snprintf(statusMsg, sizeof(statusMsg), "Modeline: %s", showModeline ? "ON" : "OFF");
                    scroll.targetY = fmin(scroll.targetY, MaxScroll());
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
                                isFileDirty = false;
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
        if (!shift && !ctrl && (prevLine != cursorLine || prevCol != cursorCol) && 
            !(key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE)) {
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
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
            if (IsSelValid()) {
                isSelecting = false;
            } else {
                selStartLine = selEndLine = -1;
                selStartCol = selEndCol = -1;
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
            if (showMinimap) {
                int minimapX = window.screen_width - minimapWidth - (showScrollbar ? scrollbarWidth : 0);
                if (x >= minimapX && x <= minimapX + minimapWidth) {
                    isMinimapDragging = true;
                    float clickRatio = fmin(1.0f, fmax(0.0f, y / (float)window.screen_height));
                    scroll.targetY = clickRatio * fmax(0, numLines * lineHeight - window.screen_height);
                    return;
                }
            }
            if (window.deltatime - lastClickTime < 0.4) clickCount++;
            else clickCount = 1;
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
                selStartLine = selEndLine = cursorLine;
                selStartCol = selEndCol = cursorCol;
                isSelecting = true;
            } else if (clickCount == 2) {
                int lineLen = lines[cursorLine].length;
                const char* lineText = lines[cursorLine].text;
                int wordStart = cursorCol;
                while (wordStart > 0 && (isalnum(lineText[wordStart-1]) || lineText[wordStart-1] == '_')) wordStart--;
                int wordEnd = cursorCol;
                while (wordEnd < lineLen && (isalnum(lineText[wordEnd]) || lineText[wordEnd] == '_')) wordEnd++;
                selStartLine = selEndLine = cursorLine;
                selStartCol = wordStart;
                selEndCol = wordEnd;
                cursorCol = wordEnd;
                isSelecting = true;
            } else if (clickCount >= 3) {
                selStartLine = selEndLine = cursorLine;
                selStartCol = 0;
                selEndCol = lines[cursorLine].length;
                cursorCol = selEndCol;
                isSelecting = true;
                clickCount = 0;
            }
        } else if (action == GLFW_RELEASE) {
            isMinimapDragging = isScrollbarDragging = isHorizontalScrollbarDragging = false;
            if (selStartLine == selEndLine && selStartCol == selEndCol) {
                selStartLine = selEndLine = -1;
                selStartCol = selEndCol = -1;
                isSelecting = false;
            }
        }
    }
}

void CursorPosCallback(GLFWwindow* win, double x, double y) {
    if (isMinimapDragging) {
        y = fmax(0, fmin(y, window.screen_height));
        float dragRatio = y / (float)window.screen_height;
        scroll.targetY = dragRatio * fmax(0, numLines * lineHeight - window.screen_height);
        return;
    }
    if (isScrollbarDragging) {
        HandleScrollDrag(y);
        return;
    } else if (isHorizontalScrollbarDragging) {
        HandleHorizontalScrollDrag(x);
        return;
    }
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && isSelecting) {
        if (IsInScrollbar(x, y) || IsInHorizontalScrollbar(x, y)) return;
        UpdateCursor(x, y);
        selEndLine = cursorLine;
        selEndCol = cursorCol;
        int edge = lineHeight * 2;
        if (y < edge) scroll.targetY = fmax(0, scroll.targetY - lineHeight);
        else if (y > window.screen_height - edge) scroll.targetY = fmin(MaxScroll(), scroll.targetY + lineHeight);
        int horizEdge = 50;
        if (x < horizEdge) scroll.targetX = fmax(0, scroll.targetX - fontSize * 2);
        else if (x > window.screen_width - horizEdge) scroll.targetX = fmin(MaxHorizontalScroll(), scroll.targetX + fontSize * 2);
    }
}

void DrawFps() {
    static char fpsText[16];
    snprintf(fpsText, sizeof(fpsText), "%.0f", window.fps);
    TextSize fps = GetTextSize(font, 22, fpsText);
    DrawText(window.screen_width - fps.width - 10, (window.screen_height/2)-(fps.height/2), font, 22, fpsText, (Color){245, 245, 245, 145});
}

void Close() {
    FreeLines();
}

void Draw() {
    UpdateScroll(window.deltatime);
    DrawEditor();
    DrawLineNumbers();
    DrawScrollbar();
    DrawHorizontalScrollbar();
    UpdateLastScrollTimes();
    DrawMinimap();
    DrawModeline();
    DrawFps();
}

int Init(int argc, char *argv[]) {
    if (argc > 1) {
        if (argv[1][0] == '-' && argv[1][1] == 'h') {
            printf("Usage: %s [filename]\n", argv[0]);
            WindowStateSet(true);
        }
        strncpy(filename, argv[1], MAX_FILENAME - 1);
        filename[MAX_FILENAME - 1] = '\0';
        if (FileExists(filename)) {
            if (!LoadFile(filename)) InitLine(0);
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
    PreloadFontSizes(font);
}
