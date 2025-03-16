#include <float.h>
#include <libgen.h>
typedef struct { char* text; int length; } Line;
typedef struct { float targetY, currentY, targetX, currentX, velocity; bool smoothScroll; } ScrollState;
typedef struct { int originalLine; int startCol; int length; bool isWrapped; } WrappedLine;
#define MAXTEXT 16384
#define MAX_FILENAME 256
Line lines[MAXTEXT] = {0};
int numLines = 1, cursorLine = 0, cursorCol = 0, lineHeight = 0, gutterWidth = 0;
bool isSelecting = false, showLineNumbers = true, showStatusBar = true, showScrollbar = true, wordWrap = false;
bool isFileDirty = false, isScrollbarDragging = false, isHorizontalScrollbarDragging = false;
bool insertMode = true, showMinimap = true, isMinimapDragging = false, showModeline = true;
int selStartLine = -1, selStartCol = -1, selEndLine = -1, selEndCol = -1;
int statusBarHeight = 25, scrollbarWidth = 12, minimapWidth = 100, modeLineHeight = 20;
double fontSize = 24.0f;
float minimapScale = 0.2f;
char fileEncoding[16] = "UTF-8", lineEnding[8] = "LF", filename[MAX_FILENAME] = "Untitled.txt", statusMsg[256] = "";
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
WrappedLine wrappedLines[MAXTEXT * 2] = {0};
int numWrappedLines = 0;

bool IsSelValid() { return selStartLine != -1 && selEndLine != -1 && (selStartLine != selEndLine || selStartCol != selEndCol); }
void InitLine(int idx) { if (idx < MAXTEXT && (lines[idx].text = malloc(MAXTEXT))) { lines[idx].text[0] = '\0'; lines[idx].length = 0; } else { fprintf(stderr, "Memory error\n"); exit(1); } }
void FreeLines() { for (int i = 0; i < numLines; i++) { free(lines[i].text); lines[i].text = NULL; } numLines = 0; }
void InsertChar(char c) { if (lines[cursorLine].length < MAXTEXT - 1) { memmove(&lines[cursorLine].text[cursorCol+1], &lines[cursorLine].text[cursorCol], lines[cursorLine].length - cursorCol + 1); lines[cursorLine].text[cursorCol++] = c; lines[cursorLine].length++; isFileDirty = true; } }

void DeleteChar() {
    if (cursorCol > 0) {
        memmove(&lines[cursorLine].text[cursorCol-1], &lines[cursorLine].text[cursorCol], 
                lines[cursorLine].length - cursorCol + 1);
        lines[cursorLine].length--;
        cursorCol--;
        isFileDirty = true;
    } else if (cursorLine > 0) {
        int prevLineLen = lines[cursorLine-1].length;
        int currLineLen = lines[cursorLine].length;
        if (prevLineLen + currLineLen >= MAXTEXT) {
            int spaceLeft = MAXTEXT - 1 - prevLineLen;
            if (spaceLeft > 0) {
                strncat(lines[cursorLine-1].text, lines[cursorLine].text, spaceLeft);
                lines[cursorLine-1].length += spaceLeft;
            }
        } else {
            strcat(lines[cursorLine-1].text, lines[cursorLine].text);
            lines[cursorLine-1].length += lines[cursorLine].length;
        }
        cursorCol = prevLineLen;
        free(lines[cursorLine].text);
        for (int i = cursorLine + 1; i < numLines; i++) {
            lines[i-1] = lines[i];
        }
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
    return fmax(0, ((wordWrap ? numWrappedLines : numLines) - maxVisLines) * lineHeight);
}
int MaxHorizontalScroll() {
    if (wordWrap) return 0;
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
    UpdateGutterWidth();
    int viewWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    float charWidth = GetTextSize(font, fontSize, "M").width;
    scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), roundf(scroll.targetX / charWidth) * charWidth));
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
int FindCharPos(int wrappedLine, int mouseX) {
    if (wrappedLine < 0 || wrappedLine >= (wordWrap ? numWrappedLines : numLines)) return 0;
    int xOffset = showLineNumbers ? gutterWidth : 0;
    mouseX += scroll.currentX;
    if (mouseX <= xOffset) return 0;
    const char* lineText;
    int startCol = 0;
    int lineLen;
    if (wordWrap) {
        int origLine = wrappedLines[wrappedLine].originalLine;
        startCol = wrappedLines[wrappedLine].startCol;
        lineLen = wrappedLines[wrappedLine].length;
        lineText = lines[origLine].text + startCol;
    } else {
        lineText = lines[wrappedLine].text;
        lineLen = lines[wrappedLine].length;
    }
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
void RecalculateWrappedLines() {
    if (!wordWrap) {
        numWrappedLines = numLines;
        for (int i = 0; i < numLines; i++) {
            wrappedLines[i].originalLine = i;
            wrappedLines[i].startCol = 0;
            wrappedLines[i].length = lines[i].length;
            wrappedLines[i].isWrapped = false;
        }
        return;
    }
    numWrappedLines = 0;
    int availableWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    for (int i = 0; i < numLines; i++) {
        const char* text = lines[i].text;
        int lineLength = lines[i].length;
        int startCol = 0;
        if (lineLength == 0 || GetTextSize(font, fontSize, text).width <= availableWidth) {
            wrappedLines[numWrappedLines].originalLine = i;
            wrappedLines[numWrappedLines].startCol = 0;
            wrappedLines[numWrappedLines].length = lineLength;
            wrappedLines[numWrappedLines].isWrapped = false;
            numWrappedLines++;
            continue;
        }
        while (startCol < lineLength) {
            int endCol = startCol;
            int lastBreakPoint = -1;
            float currentWidth = 0;
            while (endCol < lineLength) {
                char charStr[2] = {text[endCol], '\0'};
                float charWidth = GetTextSize(font, fontSize, charStr).width;
                if (currentWidth + charWidth > availableWidth) {
                    break;
                }
                currentWidth += charWidth;
                if (text[endCol] == ' ' || text[endCol] == '\t' || text[endCol] == '-') {
                    lastBreakPoint = endCol;
                }
                endCol++;
            }
            if (endCol == startCol) {
                endCol = startCol + 1;
            } 
            else if (lastBreakPoint != -1 && endCol < lineLength) {
                endCol = lastBreakPoint + 1;
            }
            wrappedLines[numWrappedLines].originalLine = i;
            wrappedLines[numWrappedLines].startCol = startCol;
            wrappedLines[numWrappedLines].length = endCol - startCol;
            wrappedLines[numWrappedLines].isWrapped = (startCol > 0);
            numWrappedLines++;
            startCol = endCol;
            if (numWrappedLines >= MAXTEXT * 2 - 1) {
                break;
            }
        }
    }
}
void WrappedToOriginal(int wrappedLine, int wrappedCol, int* origLine, int* origCol) {
    if (!wordWrap || wrappedLine >= numWrappedLines) {
        *origLine = wrappedLine;
        *origCol = wrappedCol;
        return;
    }
    *origLine = wrappedLines[wrappedLine].originalLine;
    *origCol = wrappedLines[wrappedLine].startCol + wrappedCol;
}
void OriginalToWrapped(int origLine, int origCol, int* wrappedLine, int* wrappedCol) {
    if (!wordWrap) {
        *wrappedLine = origLine;
        *wrappedCol = origCol;
        return;
    }
    *wrappedLine = 0;
    *wrappedCol = 0;
    for (int i = 0; i < numWrappedLines; i++) {
        if (wrappedLines[i].originalLine == origLine) {
            int start = wrappedLines[i].startCol;
            int end = start + wrappedLines[i].length;
            if (origCol >= start && origCol < end) {
                *wrappedLine = i;
                *wrappedCol = origCol - start;
                return;
            } else if (origCol == end && end == lines[origLine].length) {
                *wrappedLine = i;
                *wrappedCol = wrappedLines[i].length;
                return;
            }
        }
    }
    for (int i = numWrappedLines - 1; i >= 0; i--) {
        if (wrappedLines[i].originalLine == origLine) {
            *wrappedLine = i;
            *wrappedCol = wrappedLines[i].length;
            return;
        }
    }
}
void UpdateCursor(double x, double y) {
    UpdateGutterWidth();
    if (showScrollbar && x >= window.screen_width - scrollbarWidth) return;
    float adjustedY = y + scroll.currentY;
    int wrappedLine = (int)(adjustedY / lineHeight);
    wrappedLine = fmax(0, fmin(wordWrap ? numWrappedLines - 1 : numLines - 1, wrappedLine));
    int wrappedCol = FindCharPos(wrappedLine, x);
    if (wordWrap) {
        int origLine, origCol;
        WrappedToOriginal(wrappedLine, wrappedCol, &origLine, &origCol);
        cursorLine = origLine;
        cursorCol = origCol;
    } else {
        cursorLine = wrappedLine;
        cursorCol = wrappedCol;
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
        memmove(&lines[startLine].text[startCol], &lines[startLine].text[endCol], 
                lines[startLine].length - endCol + 1);
        lines[startLine].length -= (endCol - startCol);
    } else {
        size_t endLineRemaining = strlen(lines[endLine].text + endCol);
        if (startCol + endLineRemaining >= MAXTEXT) {
            lines[startLine].text[startCol] = '\0';
            lines[startLine].length = startCol;
        } else {
            strcpy(lines[startLine].text + startCol, lines[endLine].text + endCol);
            lines[startLine].length = startCol + endLineRemaining;
        }
        for (int i = startLine + 1; i <= endLine; i++) {
            free(lines[i].text);
        }
        int linesToRemove = endLine - startLine;
        for (int i = endLine + 1; i < numLines; i++) {
            lines[i - linesToRemove] = lines[i];
        }
        
        numLines -= linesToRemove;
    }
    cursorLine = startLine;
    cursorCol = startCol;
    selStartLine = selStartCol = selEndLine = selEndCol = -1;
    isSelecting = false;
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
    UpdateGutterWidth();
    int sbWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    float ratio = fmin(1.0f, fmax(0.0f, (x - gutterWidth) / sbWidth));
    float charWidth = GetTextSize(font, fontSize, "M").width;
    int maxHScroll = MaxHorizontalScroll();
    float gridSnappedScroll = roundf((ratio * maxHScroll) / charWidth) * charWidth;
    scroll.targetX = fmax(0, fmin(maxHScroll, gridSnappedScroll));
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
    int maxVisibleLines = wordWrap ? numWrappedLines : numLines;
    int end = fmin(maxVisibleLines, start + window.screen_height / lineHeight + 1);
    DrawRectBatch(0, 0, gutterWidth, window.screen_height, (Color){5, 5, 5, 255});
    int wrappedCursorLine = cursorLine;
    if (wordWrap) {
        int wrappedCol;
        OriginalToWrapped(cursorLine, cursorCol, &wrappedCursorLine, &wrappedCol);
    }
    for (int i = start; i < end; i++) {
        if (i < 0 || i >= maxVisibleLines) continue;
        char num[16];
        int lineNumber;
        bool isWrapped = false;
        if (wordWrap) {
            lineNumber = wrappedLines[i].originalLine + 1;
            isWrapped = wrappedLines[i].isWrapped;
            if (isWrapped) {
                snprintf(num, sizeof(num), "~");
            } else {
                snprintf(num, sizeof(num), "%d", lineNumber);
            }
        } else {
            lineNumber = i + 1;
            snprintf(num, sizeof(num), "%d", lineNumber);
        }
        TextSize size = GetTextSize(font, fontSize, num);
        int tx = gutterWidth - size.width;
        int ty = i * lineHeight - scroll.currentY;
        bool isCurrentLine = (i == wrappedCursorLine);
        if (isCurrentLine) {
            DrawRectBatch(0, ty, gutterWidth, lineHeight, (Color){20, 20, 20, 100});
        }
        Color color;
        if (isCurrentLine) {
            color = textColor;
        } else if (isWrapped) {
            color = (Color){100, 100, 100, 180};
        } else {
            color = (Color){150, 150, 150, 255};
        }
        DrawTextBatch(tx, ty, font, fontSize, num, color);
    }
    FlushRectBatch();
}
float GetDocumentHeight() { return (wordWrap ? numWrappedLines : numLines) * lineHeight; }
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
void DrawHorizontalScrollbar() {
    if (!showScrollbar || wordWrap) return;
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
void HandleMinimapDrag(double x, double y) {
    int minimapX = window.screen_width - minimapWidth - (showScrollbar ? scrollbarWidth : 0);
    int minimapHeight = window.screen_height;
    int dragBuffer = 20;
    if (x >= minimapX - dragBuffer && x < minimapX + minimapWidth + dragBuffer && 
        y >= -dragBuffer && y < minimapHeight + dragBuffer) {
        float clampedY = fmax(0, fmin(minimapHeight - 1, y));
        float clickRatio = clampedY / minimapHeight;
        float documentHeight = numLines * lineHeight;
        float targetPosition = clickRatio * documentHeight;
        int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
        float centeringFactor;
        if (clickRatio < 0.2f || clickRatio > 0.8f) {
            centeringFactor = 0.25f;
        } else {
            centeringFactor = 0.5f; 
        }
        targetPosition -= visibleHeight * centeringFactor;
        scroll.targetY = fmax(0, fmin(MaxScroll(), floor(targetPosition)));
        lastVerticalScrollTime = window.time;
    }
}
void DrawScrollbarAndMinimap() {
    if(!showScrollbar&&!showMinimap)return;
    int RIGHT_EDGE=window.screen_width;
    int FULL_HEIGHT=window.screen_height;
    float docHeight=numLines*lineHeight;
    if(docHeight<=0)return;
    if(showScrollbar&&showMinimap) {
        DrawRectBatch(RIGHT_EDGE-minimapWidth-scrollbarWidth,0,minimapWidth+scrollbarWidth,FULL_HEIGHT,(Color){5,5,8,180});
    } else if(showScrollbar) {
        //DrawRectBatch(RIGHT_EDGE-scrollbarWidth,0,scrollbarWidth,FULL_HEIGHT,(Color){5,5,8,180});
    } else if(showMinimap) {
        DrawRectBatch(RIGHT_EDGE-minimapWidth,0,minimapWidth,FULL_HEIGHT,(Color){5,5,8,180});
    }
    float visibleRatio=GetVisibleHeightRatio();
    int thumbHeight=fmax(30,FULL_HEIGHT*visibleRatio);
    int maxScroll=MaxScroll();
    float scrollRatio=(maxScroll>0)?fmin(1.0f,fmax(0.0f,scroll.currentY/(float)maxScroll)):0;
    int thumbY=floor(scrollRatio*(FULL_HEIGHT-thumbHeight));
    if(showMinimap) {
        int mmX=RIGHT_EDGE-minimapWidth-(showScrollbar?scrollbarWidth:0);
        float totalDocumentHeight = numLines * lineHeight;
        float minimapScale = FULL_HEIGHT / totalDocumentHeight;
        int maxLen = 0;
        for(int i = 0; i < numLines; i++) {
            if(lines[i].length > maxLen) maxLen = lines[i].length;
        }
        maxLen = fmax(1, maxLen);
        bool hasSel = IsSelValid();
        int selS = 0, selE = 0;
        if(hasSel) {
            selS = selStartLine < selEndLine || (selStartLine == selEndLine && selStartCol < selEndCol) ? selStartLine : selEndLine;
            selE = selStartLine > selEndLine || (selStartLine == selEndLine && selStartCol > selEndCol) ? selStartLine : selEndLine;
        }
        for(int i = 0; i < numLines; i++) {
            float lineY = floor(((float)i * lineHeight) * minimapScale);
            float lineH = fmax(1.0f, lineHeight * minimapScale);
            int len = lines[i].length;
            float contentWidth = minimapWidth - 4;
            float lineW = fmax(4.0f, contentWidth * fmin(1.0f, fmax(0.15f, (float)len / fmax(80.0f, (float)maxLen * 0.8f))));
            Color lineColor = {
                (unsigned char)(textColor.r * 0.5f),
                (unsigned char)(textColor.g * 0.5f),
                (unsigned char)(textColor.b * 0.5f),
                (unsigned char)(100 + fmin(150, len))
            };
            if(hasSel && i >= selS && i <= selE) {
                lineColor.r = 150;
                lineColor.g = 120;
                lineColor.b = 220;
                lineColor.a = 160;
            }
            DrawRectBatch(mmX + 2, floor(lineY), floor(lineW), ceil(lineH), lineColor);
            if(i == cursorLine) {
                Color cursorLineColor = {
                    (unsigned char)(textColor.r * 0.6f),
                    (unsigned char)(textColor.g * 0.6f),
                    (unsigned char)(textColor.b * 0.6f),
                    160
                };
                DrawRectBatch(mmX + 2, floor(lineY), floor(lineW), ceil(lineH), cursorLineColor);
            }
        }
        float cursorY = floor(((float)cursorLine * lineHeight) * minimapScale);
        int currentLineLen = lines[cursorLine].length;
        float contentWidth = minimapWidth - 4;
        float lineW = fmax(4.0f, contentWidth * fmin(1.0f, fmax(0.15f, (float)currentLineLen / fmax(80.0f, (float)maxLen * 0.8f))));
        float xPosRatio = currentLineLen > 0 ? (float)cursorCol / (float)currentLineLen : 0.0f;
        float dotSizeWidth = fmax(1.0f, (lineHeight / 8) * minimapScale);
        float dotSizeHeight = lineHeight * minimapScale;
        float dotX = mmX + 2 + xPosRatio * lineW;
        dotX = fmax(mmX + 2, fmin(mmX + 2 + lineW - dotSizeWidth, dotX));
        DrawRectBatch(
            dotX, 
            cursorY, 
            dotSizeWidth, 
            dotSizeHeight, 
            (Color){255, 220, 200, 220}
        ); 
        float hlY = thumbY;
        float hlH = thumbHeight;
        DrawRectBatch(mmX, hlY, minimapWidth, hlH, (Color){30, 30, 40, 80});
        DrawRectBatch(mmX, hlY, minimapWidth, 1, (Color){100, 100, 120, 120});
        DrawRectBatch(mmX, hlY + hlH - 1, minimapWidth, 1, (Color){100, 100, 120, 120});
    }
    if (showMinimap) {
        int sbX = RIGHT_EDGE - scrollbarWidth;
        double timeSince = window.time - lastVerticalScrollTime;
        int alpha = timeSince < scrollbarFadeDelay ? 150 : 100;
        if(alpha > 0 || isScrollbarDragging) {
            if(isScrollbarDragging) alpha = 200;
            DrawRectBatch(sbX, thumbY, scrollbarWidth, thumbHeight, (Color){255, 255, 255, alpha});
        }
        DrawHorizontalScrollbar();
    } else if (showScrollbar) {
        int sbX = RIGHT_EDGE - scrollbarWidth;
        double timeSince = window.time - lastVerticalScrollTime;
        int alpha = timeSince < scrollbarFadeDelay ? 150 : 0;
        if(alpha > 0 || isScrollbarDragging) {
            if(isScrollbarDragging) alpha = 200;
            DrawRectBatch(sbX, thumbY, scrollbarWidth, thumbHeight, (Color){255, 255, 255, alpha});
        }
        DrawHorizontalScrollbar();
    }
    FlushRectBatch();
}
void DrawModeline() {
    if (!showModeline) return;
    int modeLineh = 20;
    int y = window.screen_height - (showStatusBar ? statusBarHeight : 0) - modeLineh;
    Color bgColor = {18, 20, 26, 245};
    DrawRect(0, y, window.screen_width, modeLineh, bgColor);
    float fontSizeModeline = 24 * 0.75;
    TextSize lineHeightMetric = GetTextSize(font, fontSizeModeline, "Aj|");
    int textY = y + (modeLineh - lineHeightMetric.height) / 2;
    int leftX = 10, rightX = window.screen_width - 10;
    char statusIndicator[2] = {isFileDirty ? '*' : '+', '\0'};
    Color statusColor = isFileDirty ? (Color){230, 100, 100, 255} : (Color){100, 230, 100, 255};
    DrawText(leftX, textY, font, fontSizeModeline, statusIndicator, statusColor);
    leftX += GetTextSize(font, fontSizeModeline, statusIndicator).width + 8;
    char* displayName = filename;
    int maxFileWidth = (window.screen_width * 0.4) - leftX;
    char fileDisplay[128];
    TextSize nameSize = GetTextSize(font, fontSizeModeline, displayName);
    if (nameSize.width > maxFileWidth) {
        int maxChars = maxFileWidth / (fontSizeModeline * 0.6);
        snprintf(fileDisplay, sizeof(fileDisplay), "%.*s...", maxChars - 3, displayName);
    } else {
        snprintf(fileDisplay, sizeof(fileDisplay), "%s", displayName);
    }
    DrawText(leftX, textY, font, fontSizeModeline, fileDisplay, (Color){220, 220, 225, 255});
    leftX += GetTextSize(font, fontSizeModeline, fileDisplay).width + 15;
    int statusMaxWidth = rightX - leftX - 200;
    if (statusMaxWidth > 0 && statusMsg[0]) {
        char statusDisplay[128];
        strncpy(statusDisplay, statusMsg, sizeof(statusDisplay));
        statusDisplay[sizeof(statusDisplay)-1] = '\0';
        TextSize statusSize = GetTextSize(font, fontSizeModeline, statusDisplay);
        while (statusSize.width > statusMaxWidth && strlen(statusDisplay) > 4) {
            statusDisplay[strlen(statusDisplay)-1] = '\0';
            statusSize = GetTextSize(font, fontSizeModeline, statusDisplay);
        }
        if (statusSize.width > statusMaxWidth) {
            snprintf(statusDisplay, sizeof(statusDisplay), "%.*s...", (int)(statusMaxWidth / (fontSizeModeline * 0.6)) - 3, statusMsg);
        }
        DrawText(leftX, textY, font, fontSizeModeline, statusDisplay, (Color){200, 200, 200, 200});
        leftX += GetTextSize(font, fontSizeModeline, statusDisplay).width + 15;
    }
    char timeString[20];
    time_t now = time(NULL);
    strftime(timeString, sizeof(timeString), "%H:%M", localtime(&now));
    TextSize timeSize = GetTextSize(font, fontSizeModeline, timeString);
    rightX -= timeSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, timeString, (Color){180, 180, 190, 255});
    rightX -= 15;
    char modeText[12];
    snprintf(modeText, sizeof(modeText), "%s", insertMode ? "INS" : "OVR");
    TextSize modeSize = GetTextSize(font, fontSizeModeline, modeText);
    rightX -= modeSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, modeText, insertMode ? (Color){160, 200, 220, 255} : (Color){220, 180, 160, 255});
    rightX -= 15;
    char wrapText[12];
    snprintf(wrapText, sizeof(wrapText), "%s", wordWrap ? "WRAP" : "NOWRAP");
    TextSize wrapSize = GetTextSize(font, fontSizeModeline, wrapText);
    rightX -= wrapSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, wrapText, wordWrap ? (Color){160, 220, 180, 255} : (Color){220, 160, 180, 255});
    rightX -= 15;
    char posText[32];
    snprintf(posText, sizeof(posText), "%d:%d %d", cursorLine + 1, cursorCol + 1, (int)fontSize);
    TextSize posSize = GetTextSize(font, fontSizeModeline, posText);
    rightX -= posSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, posText, (Color){180, 190, 210, 255});
    rightX -= 15;
    char encText[32];
    snprintf(encText, sizeof(encText), "%s %s", fileEncoding, lineEnding);
    TextSize encSize = GetTextSize(font, fontSizeModeline, encText);
    rightX -= encSize.width;
    DrawText(rightX, textY, font, fontSizeModeline, encText, (Color){150, 150, 160, 200});
}
void InsertNewline() {
    if (numLines >= MAXTEXT) return;
    InitLine(numLines);
    for (int i = numLines; i > cursorLine + 1; i--) {
        lines[i] = lines[i-1];
    }
    lines[cursorLine+1].length = lines[cursorLine].length - cursorCol;
    memcpy(lines[cursorLine+1].text, &lines[cursorLine].text[cursorCol], lines[cursorLine+1].length + 1);
    lines[cursorLine].text[cursorCol] = '\0';
    lines[cursorLine].length = cursorCol;
    numLines++;
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
    bool hasCRLF = false;
    while (numLines < MAXTEXT) {
        end = memchr(start, '\n', strlen(start));
        size_t len = end ? (size_t)(end - start) : strlen(start);
        if (end && len > 0 && start[len-1] == '\r') {
            len--;
            hasCRLF = true;
        }
        InitLine(numLines);
        memcpy(lines[numLines].text, start, len);
        lines[numLines].text[len] = '\0';
        lines[numLines].length = len;
        numLines++;
        if (!end) break;
        start = end + 1;
    }
    free(fileContent);
    strcpy(lineEnding, hasCRLF ? "CRLF" : "LF");
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
float GetCursorScreenX() { return GetCursorXPosition() - scroll.currentX; }
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
    AdjustScrollToCursor();
}
void DrawEditor() {
    static float lastFontSize = 0;
    static float cachedScale = 0;
    static int cachedLineHeight = 0;
    static int lastWidth = 0;
    if (lastFontSize != fontSize || lastWidth != window.screen_width) {
        lastFontSize = fontSize;
        lastWidth = window.screen_width;
        cachedScale = fontSize / font.fontSize;
        cachedLineHeight = (int)GetTextSize(font, fontSize, "gj|").height;
        if (wordWrap) {
            RecalculateWrappedLines();
        }
    }
    float scale = cachedScale;
    lineHeight = cachedLineHeight;
    UpdateGutterWidth();
    int textX = showLineNumbers ? gutterWidth : 0;
    int startLine = MaxInt(0, (int)(scroll.currentY / lineHeight));
    int endLine = MinInt(wordWrap ? numWrappedLines : numLines, startLine + (window.screen_height / lineHeight) + 2);
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
        const char *lineText;
        int lineLength;
        int origLine;
        int startCol = 0;
        if (wordWrap) {
            origLine = wrappedLines[i].originalLine;
            startCol = wrappedLines[i].startCol;
            lineLength = wrappedLines[i].length;
            lineText = lines[origLine].text + startCol;
        } else {
            origLine = i;
            lineText = lines[i].text;
            lineLength = lines[i].length;
        }
        if (widthCache.text != lineText || widthCache.length != lineLength) {
            if (widthCache.widths) free(widthCache.widths);
            widthCache.widths = malloc((lineLength + 1) * sizeof(float));
            widthCache.text = (char*)lineText;
            widthCache.length = lineLength;
            float* cumWidths = widthCache.widths;
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
        }
        float* cumWidths = widthCache.widths;
        bool lineHasSelection = false;
        int selStart = 0, selEnd = 0;
        if (isSelectionValid) {
            if (wordWrap) {
                int wrapSelStartLine, wrapSelStartCol, wrapSelEndLine, wrapSelEndCol;
                OriginalToWrapped(normStartLine, normStartCol, &wrapSelStartLine, &wrapSelStartCol);
                OriginalToWrapped(normEndLine, normEndCol, &wrapSelEndLine, &wrapSelEndCol);
                if (i >= wrapSelStartLine && i <= wrapSelEndLine) {
                    lineHasSelection = true;
                    if (i == wrapSelStartLine) {
                        if (origLine == normStartLine) {
                            selStart = normStartCol - startCol;
                            if (selStart < 0) selStart = 0;
                        } else {
                            selStart = 0;
                        }
                    } else {
                        selStart = 0;
                    }
                    if (i == wrapSelEndLine) {
                        if (origLine == normEndLine) {
                            selEnd = normEndCol - startCol;
                            if (selEnd > lineLength) selEnd = lineLength;
                        } else {
                            selEnd = lineLength;
                        }
                    } else {
                        selEnd = lineLength;
                    }
                }
            } else {
                if (i >= normStartLine && i <= normEndLine) {
                    lineHasSelection = true;
                    selStart = (i == normStartLine) ? normStartCol : 0;
                    selEnd = (i == normEndLine) ? normEndCol : lineLength;
                }
            }
        }
        if (lineHasSelection) {
            selStart = Clamp(selStart, 0, lineLength);
            selEnd = Clamp(selEnd, 0, lineLength);
            if (selStart != selEnd) {
                int selStartX = (int)(textX - scroll.currentX + cumWidths[selStart]);
                int selEndX = (int)(textX - scroll.currentX + cumWidths[selEnd]);
                int selectionWidth = selEndX - selStartX;
                DrawRectBatch(selStartX, lineY, selectionWidth < 1 ? 1 : selectionWidth, lineHeight, (Color){100,100,200,100});
            }
        }
        char tempText[MAXTEXT + 1];
        strncpy(tempText, lineText, lineLength);
        tempText[lineLength] = '\0';
        const float drawX = textX - scroll.currentX;
        DrawTextBatch(drawX, lineY, font, fontSize, tempText, textColor);
        FlushTextBatch();
        int wrappedCursorLine, wrappedCursorCol;
        if (wordWrap) {
            OriginalToWrapped(cursorLine, cursorCol, &wrappedCursorLine, &wrappedCursorCol);
            if (i == wrappedCursorLine) {
                const int clampedCol = Clamp(wrappedCursorCol, 0, lineLength);
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
        } else if (i == cursorLine) {
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
    static double keyTimes[GLFW_KEY_LAST+1]={0};
    static bool keyState[GLFW_KEY_LAST+1]={0};
    static int targCol=-1;
    static double initDelay=0.02, repeatInt=0.02;
    bool ctrl=(mods&GLFW_MOD_CONTROL)!=0, shift=(mods&GLFW_MOD_SHIFT)!=0;
    int pLine=cursorLine, pCol=cursorCol, newLine, newCol;
    if(action==GLFW_PRESS){keyState[key]=1;keyTimes[key]=window.time;}
    else if(action==GLFW_REPEAT){
        double elapsed=window.time-keyTimes[key];
        if(!keyState[key]){keyState[key]=1;keyTimes[key]=window.time;}
        else if(elapsed<(keyState[key]&&elapsed>initDelay?repeatInt:initDelay))return;
        keyTimes[key]=window.time;
    }else if(action==GLFW_RELEASE){
        keyState[key]=0;
        if(key==GLFW_KEY_LEFT_SHIFT||key==GLFW_KEY_RIGHT_SHIFT){
            if(!IsSelValid()){selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            isSelecting=0;
        }
        return;
    }
    if(shift&&!isSelecting){selStartLine=cursorLine;selStartCol=cursorCol;isSelecting=1;}
    if(key==GLFW_KEY_W&&ctrl){
        wordWrap=!wordWrap;
        snprintf(statusMsg,sizeof(statusMsg),"Word Wrap: %s",wordWrap?"ON":"OFF");
        if(wordWrap){
            RecalculateWrappedLines();
            int wl,wc; OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
            float curY=wl*lineHeight;
            int visH=window.screen_height-(showStatusBar?statusBarHeight:0)-(showModeline?modeLineHeight:0);
            if(curY<scroll.currentY||curY>=scroll.currentY+visH-lineHeight)
                scroll.targetY=fmax(0,curY-visH/2);
            scroll.targetX=0;
        }
        return;
    }
    if(wordWrap){
        int wl,wc; OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
        switch(key){
            case GLFW_KEY_LEFT:
                if(ctrl){
                    if(cursorCol>0){
                        while(cursorCol>0&&isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                        while(cursorCol>0&&!isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                    }else if(cursorLine>0){
                        cursorLine--;
                        cursorCol=lines[cursorLine].length;
                    }
                }else{
                    if(cursorCol > 0){
                        cursorCol--;
                    }else if(cursorLine > 0){
                        cursorLine--;
                        cursorCol = lines[cursorLine].length;
                    }
                    OriginalToWrapped(cursorLine, cursorCol, &wl, &wc);
                }
                break;
            case GLFW_KEY_RIGHT:
                if(ctrl){
                    if(cursorCol<lines[cursorLine].length){
                        while(cursorCol<lines[cursorLine].length&&!isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                        while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    }else if(cursorLine<numLines-1){
                        cursorLine++;
                        cursorCol=0;
                    }
                }else{
                    if(wc<wrappedLines[wl].length){
                        wc++;
                        WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                    }else if(wl<numWrappedLines-1){
                        wl++;
                        wc=0;
                        WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                    }
                }
                break;
            case GLFW_KEY_UP:
                if(targCol<0)targCol=wc;
                if(wl>0){
                    wl=fmax(0, ctrl ? wl-3 : wl-1);
                    wc=fmin(targCol,wrappedLines[wl].length);
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
                break;
            case GLFW_KEY_DOWN:
                if(targCol<0)targCol=wc;
                if(wl<numWrappedLines-1){
                    wl=fmin(numWrappedLines-1, ctrl ? wl+3 : wl+1);
                    wc=fmin(targCol,wrappedLines[wl].length);
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
                break;
            case GLFW_KEY_HOME:
                if(ctrl){
                    cursorLine=0;
                    cursorCol=0;
                    scroll.targetY=0;
                }else{
                    wc=0;
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
                break;
            case GLFW_KEY_END:
                if(ctrl){
                    cursorLine=numLines-1;
                    cursorCol=lines[cursorLine].length;
                    scroll.targetY=MaxScroll();
                }else{
                    wc=wrappedLines[wl].length;
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
                break;
            case GLFW_KEY_PAGE_UP: {
                int visLines=window.screen_height/lineHeight;
                wl=fmax(0,wl-visLines);
                wc=fmin(targCol>=0?targCol:wc,wrappedLines[wl].length);
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                scroll.targetY=fmax(0,scroll.targetY-window.screen_height+lineHeight);
                break;
            }
            case GLFW_KEY_PAGE_DOWN: {
                int visLines=window.screen_height/lineHeight;
                wl=fmin(numWrappedLines-1,wl+visLines);
                wc=fmin(targCol>=0?targCol:wc,wrappedLines[wl].length);
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                scroll.targetY=fmin(MaxScroll(),scroll.targetY+window.screen_height-lineHeight);
                break;
            }
            default: goto default_handler;
        }
    }else{
default_handler:
        switch(key){
            case GLFW_KEY_LEFT:
                if(ctrl){
                    if(cursorCol>0){
                        while(cursorCol>0&&isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                        while(cursorCol>0&&!isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                    }else if(cursorLine>0){
                        cursorLine--;
                        cursorCol=lines[cursorLine].length;
                    }
                }else{
                    if(cursorCol>0)cursorCol--;
                    else if(cursorLine>0){
                        cursorLine--;
                        cursorCol=lines[cursorLine].length;
                    }
                }
                break;
            case GLFW_KEY_RIGHT:
                if(ctrl){
                    if(cursorCol<lines[cursorLine].length){
                        while(cursorCol<lines[cursorLine].length&&!isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                        while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    }else if(cursorLine<numLines-1){
                        cursorLine++;
                        cursorCol=0;
                    }
                }else{
                    if(cursorCol<lines[cursorLine].length)cursorCol++;
                    else if(cursorLine<numLines-1){
                        cursorLine++;
                        cursorCol=0;
                    }
                }
                break;
            case GLFW_KEY_UP:
                if(targCol<0)targCol=cursorCol;
                newLine=cursorLine;
                if(ctrl)newLine=fmax(0,cursorLine-3);
                else if(cursorLine>0)newLine=cursorLine-1;
                cursorLine=newLine;
                cursorCol=fmin(targCol,lines[cursorLine].length);
                break;
            case GLFW_KEY_DOWN:
                if(targCol<0)targCol=cursorCol;
                newLine=cursorLine;
                if(ctrl)newLine=fmin(numLines-1,cursorLine+3);
                else if(cursorLine<numLines-1)newLine=cursorLine+1;
                cursorLine=newLine;
                cursorCol=fmin(targCol,lines[cursorLine].length);
                break;
            case GLFW_KEY_HOME:
                if(ctrl){
                    cursorLine=0;
                    cursorCol=0;
                    scroll.targetY=0;
                    scroll.targetX=0;
                }else{
                    int origCol=cursorCol;
                    cursorCol=0;
                    if(origCol==0){
                        while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                        if(cursorCol==lines[cursorLine].length||cursorCol==origCol)cursorCol=0;
                    }
                    scroll.targetX=0;
                }
                break;
            case GLFW_KEY_END:
                if(ctrl){
                    cursorLine=numLines-1;
                    cursorCol=lines[cursorLine].length;
                    scroll.targetY=MaxScroll();
                    scroll.targetX=MaxHorizontalScroll();
                }else{
                    cursorCol=lines[cursorLine].length;
                }
                break;
            case GLFW_KEY_PAGE_UP:{
                int visLines=window.screen_height/lineHeight;
                cursorLine=fmax(0,cursorLine-visLines);
                cursorCol=fmin(cursorCol,lines[cursorLine].length);
                scroll.targetY=fmax(0,scroll.targetY-window.screen_height+lineHeight);
                break;
            }
            case GLFW_KEY_PAGE_DOWN:{
                int visLines=window.screen_height/lineHeight;
                cursorLine=fmin(numLines-1,cursorLine+visLines);
                cursorCol=fmin(cursorCol,lines[cursorLine].length);
                scroll.targetY=fmin(MaxScroll(),scroll.targetY+window.screen_height-lineHeight);
                break;
            }
            case GLFW_KEY_INSERT:insertMode=!insertMode;break;
            case GLFW_KEY_BACKSPACE:
                if(IsSelValid())DeleteSelection();
                else if(ctrl)DeleteWordBackward();
                else DeleteChar();
                break;
            case GLFW_KEY_DELETE:
                if(IsSelValid())DeleteSelection();
                else if(ctrl)DeleteWordForward();
                else DeleteCharAfter();
                break;
            case GLFW_KEY_ENTER:if(IsSelValid())DeleteSelection();InsertNewline();break;
            case GLFW_KEY_A:
                if(ctrl){
                    selStartLine=0;selStartCol=0;
                    selEndLine=numLines-1;selEndCol=lines[numLines-1].length;
                    cursorLine=selEndLine;cursorCol=selEndCol;
                    isSelecting=1;
                    snprintf(statusMsg,sizeof(statusMsg),"Selected all");
                }
                break;
            case 47:if(ctrl&&fontSize>=12.0f)Zoom(-2.0f);break;
            case 93:if(ctrl&&fontSize<=72.0f)Zoom(2.0f);break;
            case GLFW_KEY_L:if(ctrl){showLineNumbers=!showLineNumbers;snprintf(statusMsg,sizeof(statusMsg),"LineNumbers= %s",showLineNumbers?"ON":"OFF");}break;
            case GLFW_KEY_R:if(ctrl){showScrollbar=!showScrollbar;snprintf(statusMsg,sizeof(statusMsg),"Scrollbar = %s",showScrollbar?"ON":"OFF");}break;
            case GLFW_KEY_M:if(ctrl){showMinimap=!showMinimap;snprintf(statusMsg,sizeof(statusMsg),"Minimap = %s",showMinimap?"ON":"OFF");}break;
            case GLFW_KEY_N:if(ctrl){showModeline=!showModeline;scroll.targetY=fmin(scroll.targetY,MaxScroll());}break;
            case GLFW_KEY_S:
                if(ctrl){
                    if(mods&GLFW_MOD_SHIFT)snprintf(statusMsg,sizeof(statusMsg),"Save As not implemented");
                    else if(isFileDirty){
                        char* text=ConstructTextFromLines();
                        if(FileSave(filename,text)==NULL)snprintf(statusMsg,sizeof(statusMsg),"Error saving file -> %s",filename);
                        else{snprintf(statusMsg,sizeof(statusMsg),"File saved -> %s",filename);isFileDirty=0;}
                        free(text);
                    }else scroll.smoothScroll=!scroll.smoothScroll;
                }
                break;
        }
    }
    cursorLine=fmax(0,fmin(numLines-1,cursorLine));
    cursorCol=fmax(0,fmin(lines[cursorLine].length,cursorCol));
    if(isSelecting){selEndLine=cursorLine;selEndCol=cursorCol;}
    if(!shift&&!ctrl&&(pLine!=cursorLine||pCol!=cursorCol)&&!(key==GLFW_KEY_BACKSPACE||key==GLFW_KEY_DELETE)){
        isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;
    }
    int visH=window.screen_height-(showStatusBar?statusBarHeight:0)-(showModeline?modeLineHeight:0);
    int visW=window.screen_width-(showLineNumbers?gutterWidth:0)-(showScrollbar?scrollbarWidth:0);
    float curX,curY;
    if(wordWrap){
        int wl,wc; OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
        curY=wl*lineHeight;
        curX=0;
    }else{
        curX=GetCursorXPosition();
        curY=cursorLine*lineHeight;
    }
    float vTop=scroll.currentY, vBot=vTop+visH, buf=lineHeight;
    if(curY<vTop+buf)scroll.targetY=curY-buf;
    else if(curY+lineHeight>vBot-buf)scroll.targetY=curY-visH+lineHeight+buf;
    if(!wordWrap){
        float vLeft=scroll.currentX, vRight=vLeft+visW;
        float hBuf=GetTextSize(font,fontSize,"MM").width;
        if(curX<vLeft+hBuf)scroll.targetX=curX-hBuf;
        else if(curX>vRight-hBuf)scroll.targetX=curX-visW+hBuf;
        scroll.targetX=fmax(0,fmin(MaxHorizontalScroll(),scroll.targetX));
        float cw=GetTextSize(font,fontSize,"M").width;
        scroll.targetX=roundf(scroll.targetX/cw)*cw;
    } else scroll.targetX=0;
    scroll.targetY=fmax(0,fmin(MaxScroll(),scroll.targetY));
    lastVerticalScrollTime=lastHorizontalScrollTime=window.time;
    if(key==GLFW_KEY_UP||key==GLFW_KEY_DOWN||key==GLFW_KEY_PAGE_UP||key==GLFW_KEY_PAGE_DOWN){
        if(!(key==GLFW_KEY_UP||key==GLFW_KEY_DOWN)||(!shift&&!ctrl))targCol=-1;
    }else targCol=-1;
    if(wordWrap&&isFileDirty&&(key==GLFW_KEY_BACKSPACE||key==GLFW_KEY_DELETE||key==GLFW_KEY_ENTER||key==GLFW_KEY_TAB||(action==GLFW_PRESS&&isprint(key))))
        RecalculateWrappedLines();
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
                if (x >= minimapX && x <= minimapX + minimapWidth && y >= 0 && y <= window.screen_height) {
                    isMinimapDragging = true;
                    HandleMinimapDrag(x, y);
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
                while (wordStart > 0 && (isalnum(lineText[wordStart-1]) || lineText[wordStart-1] == '_')) 
                    wordStart--;
                int wordEnd = cursorCol;
                while (wordEnd < lineLen && (isalnum(lineText[wordEnd]) || lineText[wordEnd] == '_')) 
                    wordEnd++;
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
        HandleMinimapDrag(x, y);
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
        int textX = showLineNumbers ? gutterWidth : 0;
        int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
        int linesPerScreen = visibleHeight / lineHeight;
        int currentTopLine = (int)(scroll.targetY / lineHeight);
        int edge = lineHeight * 2;
        if (y < edge) {        scroll.targetY = fmax(0, (currentTopLine - 1) * lineHeight);
        }
        else if (y > window.screen_height - edge) {        int maxTopLine = fmax(0, numLines - linesPerScreen);
            scroll.targetY = fmin(maxTopLine * lineHeight, (currentTopLine + 1) * lineHeight);
        }
        int viewWidth = window.screen_width - textX - (showScrollbar ? scrollbarWidth : 0);
        float charWidth = GetTextSize(font, fontSize, "A").width;
        int currentTopChar = (int)(scroll.currentX / charWidth);
        int horizEdge = 50;
        if (x < horizEdge) {        scroll.targetX = fmax(0, (currentTopChar - 1) * charWidth);
        }
        else if (x > window.screen_width - horizEdge) {        int maxTopChar = fmax(0, (GetDocumentMaxWidth() - viewWidth) / charWidth);
            scroll.targetX = fmin(MaxHorizontalScroll(), (currentTopChar + 1) * charWidth);
        }
    }
}
void ScrollCallback(GLFWwindow* win, double x, double y) {
    int ctrlPressed = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL)==GLFW_PRESS;
    int shiftPressed = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    if(ctrlPressed && y!=0) {
        float zoomAmount = (y<0) ? -2.0f : 2.0f;
        if((zoomAmount<0 && fontSize>=10.0f) || (zoomAmount>0 && fontSize<=70.0f)) {
            Zoom(zoomAmount);
            UpdateGutterWidth();
            lastVerticalScrollTime = lastHorizontalScrollTime = window.time;
        }
        return;
    }
    if(x!=0 || shiftPressed) {
        double amount = (x!=0) ? x : y;
        float charWidth = GetTextSize(font, fontSize, "M").width;
        scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), scroll.targetX - amount*charWidth*2));
    } else {
        scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY - y*lineHeight));
    }
    if(scroll.smoothScroll) scroll.velocity = 0;
}
void CharCallback(GLFWwindow* win, unsigned int c) {
    if (isprint(c)) {
        const int MAX_LINE_EXTENSION = 3;
        if (insertMode) {
            InsertChar((char)c);
        } else {
            if (cursorCol < lines[cursorLine].length) {lines[cursorLine].text[cursorCol] = (char)c;cursorCol++;
            } else {
                if (lines[cursorLine].length < MAXTEXT - MAX_LINE_EXTENSION - 1) { lines[cursorLine].text[lines[cursorLine].length] = (char)c;lines[cursorLine].length++;cursorCol = lines[cursorLine].length; }
            }
        }
        if (lines[cursorLine].length >= MAXTEXT - 1) {
            lines[cursorLine].length = MAXTEXT - 1;
            lines[cursorLine].text[lines[cursorLine].length] = '\0';
        }
    }
    isFileDirty = true;
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
    FlushRectBatch();
    FlushTextBatch();
    DrawHorizontalScrollbar();
    DrawScrollbarAndMinimap();
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
