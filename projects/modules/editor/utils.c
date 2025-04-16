bool IsSelValid() { return selStartLine != -1 && selEndLine != -1 && (selStartLine != selEndLine || selStartCol != selEndCol); }
void InitLine(int idx) { if (idx < MAXTEXT && (lines[idx].text = malloc(MAXTEXT))) { lines[idx].text[0] = '\0'; lines[idx].length = 0; } else { fprintf(stderr, "Memory error\n"); exit(1); } }
void FreeLines() { for (int i = 0; i < numLines; i++) { free(lines[i].text); lines[i].text = NULL; } numLines = 0; }
int GetVisibleHeight() { return window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0); }
void ClampCursor() { cursorLine = fmax(0, fmin(numLines - 1, cursorLine)); cursorCol = fmax(0, fmin(lines[cursorLine].length, cursorCol)); }
void ClearAutoPairs() { numAutoPairs = 0; }


void InsertChar(char c) {
    SaveUndoCharInsert();
    if (cursorLine < 0 || cursorLine >= numLines) {
        fprintf(stderr, "Error: Invalid cursor line %d\n", cursorLine);
        return;
    }
    if (lines[cursorLine].length >= MAXTEXT - 1) {
        snprintf(statusMsg, sizeof(statusMsg), "Line too long (max %d characters)", MAXTEXT - 1);
        return;
    }
    if (cursorCol < 0 || cursorCol > lines[cursorLine].length) {
        fprintf(stderr, "Error: Invalid cursor column %d (line length: %d)\n", 
                cursorCol, lines[cursorLine].length);
        return;
    }
    if (cursorCol < lines[cursorLine].length) {
        memmove(&lines[cursorLine].text[cursorCol+1], 
                &lines[cursorLine].text[cursorCol], 
                lines[cursorLine].length - cursorCol + 1);
    }
    lines[cursorLine].text[cursorCol] = c;
    cursorCol++;
    lines[cursorLine].length++;
    lines[cursorLine].text[lines[cursorLine].length] = '\0';
    isFileDirty = true;
}

void DeleteCharAfter() {
    SaveUndoCharDelete();
    if (cursorLine < 0 || cursorLine >= numLines || 
        cursorCol < 0 || cursorCol > lines[cursorLine].length) {
        return;
    }
    for (int i = 0; i < numAutoPairs; i++) {
        if (autoPairs[i].line == cursorLine && 
            autoPairs[i].col == cursorCol && 
            autoPairs[i].wasAutoInserted) {
            if (cursorCol < lines[cursorLine].length && 
                lines[cursorLine].text[cursorCol+1] == autoPairs[i].close) {
                memmove(&lines[cursorLine].text[cursorCol+1], 
                        &lines[cursorLine].text[cursorCol+2], 
                        lines[cursorLine].length - cursorCol - 1);
                lines[cursorLine].length--;
                lines[cursorLine].text[lines[cursorLine].length] = '\0';
                for (int j = i; j < numAutoPairs - 1; j++) {
                    autoPairs[j] = autoPairs[j+1];
                }
                numAutoPairs--;
            }
            break;
        }
    }
    if (cursorCol >= lines[cursorLine].length) {
        if (cursorLine >= numLines - 1) return;
        int currLineLen = lines[cursorLine].length;
        int nextLineLen = lines[cursorLine+1].length;
        if (currLineLen + nextLineLen >= MAXTEXT) {
            int spaceLeft = MAXTEXT - 1 - currLineLen;
            if (spaceLeft > 0) {
                strncat(lines[cursorLine].text, lines[cursorLine+1].text, spaceLeft);
                lines[cursorLine].length += spaceLeft;
                lines[cursorLine].text[lines[cursorLine].length] = '\0';
                snprintf(statusMsg, sizeof(statusMsg), "Line truncated (max %d characters)", MAXTEXT - 1);
            }
        } else {
            strcat(lines[cursorLine].text, lines[cursorLine+1].text);
            lines[cursorLine].length += lines[cursorLine+1].length;
        }
        free(lines[cursorLine+1].text);
        for (int i = cursorLine + 2; i < numLines; i++) {
            lines[i-1] = lines[i];
        }
        numLines--;
        isFileDirty = true;
        return;
    }
    memmove(&lines[cursorLine].text[cursorCol], 
            &lines[cursorLine].text[cursorCol+1], 
            lines[cursorLine].length - cursorCol);
    lines[cursorLine].length--;
    lines[cursorLine].text[lines[cursorLine].length] = '\0';
    isFileDirty = true;
}

void DeleteChar() {
    SaveUndoCharDelete(); 
    if (cursorCol > 0) {
        cursorCol--;
        DeleteCharAfter();
    } else if (cursorLine > 0) {
        SaveUndoLineBreak();
        int prevLineLen = lines[cursorLine-1].length;
        cursorLine--;
        cursorCol = prevLineLen;
        DeleteCharAfter();
    }
}

int MaxScroll() { return fmax(0, ((wordWrap ? numWrappedLines : numLines) - ((window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0)) / lineHeight)) * lineHeight); }

int MaxHorizontalScroll() {
    if (wordWrap) return 0;
    static int cachedMaxWidth = 0, cachedFontSize = 0, cachedNumLines = 0, cachedScreenWidth = 0;
    if (cachedFontSize == fontSize && cachedNumLines == numLines && cachedScreenWidth == window.screen_width) return cachedMaxWidth;
    int maxWidth = 0;
    for (int i = 0; i < numLines; i++) {
        TextSize size = GetTextSizeCached(font, fontSize, lines[i].text);
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

void WrappedToOriginal(int wrappedLine, int wrappedCol, int* origLine, int* origCol) {
    if (!wordWrap || wrappedLine >= numWrappedLines) {
        *origLine = wrappedLine;
        *origCol = wrappedCol;
        return;
    }
    *origLine = wrappedLines[wrappedLine].originalLine;
    int startCol = wrappedLines[wrappedLine].startCol;
    int length = wrappedLines[wrappedLine].length;
    wrappedCol = fmin(wrappedCol, length);
    *origCol = startCol + wrappedCol;
    *origCol = fmin(*origCol, lines[*origLine].length);
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

void AdjustScrollToCursor() {
    UpdateGutterWidth();
    int viewWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    float charWidth = GetTextSizeCached(font, fontSize, "M").width;
    int wl = 0, wc = 0;
    if (wordWrap) {
        OriginalToWrapped(cursorLine, cursorCol, &wl, &wc);
        scroll.targetX = 0;
        float cursorY = wl * lineHeight;
        int visibleHeight = GetVisibleHeight();
        if (cursorY < scroll.currentY || 
            cursorY > scroll.currentY + visibleHeight - lineHeight) {
            scroll.targetY = fmax(0, cursorY - visibleHeight / 2);
        }
    } else {
        scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), roundf(scroll.targetX / charWidth) * charWidth));
    }
}

void UpdateScroll(double dt) {
    dt = fmin(dt, 0.05f);
    if (scroll.smoothScroll) {
        float f = 10.0f, t = 1.0f - exp(-f * dt);
        float dy = scroll.targetY - scroll.currentY;
        float dx = scroll.targetX - scroll.currentX;
        const float ε = 0.1f;
        if (fabs(dy) > ε) {
            scroll.currentY += dy * t;
            if (fabs(dy) < ε * 2) scroll.currentY = scroll.targetY;
        }
        if (fabs(dx) > ε) {
            scroll.currentX += dx * t;
            if (fabs(dx) < ε * 2) scroll.currentX = scroll.targetX;
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
    const char* text;
    int start = 0, len;
    if (wordWrap) {
        int origLine = wrappedLines[wrappedLine].originalLine;
        start = wrappedLines[wrappedLine].startCol;
        len = wrappedLines[wrappedLine].length;
        text = lines[origLine].text + start;
    } else {
        text = lines[wrappedLine].text;
        len = lines[wrappedLine].length;
    }
    float x = xOffset;
    for (int i = 0; i <= len; i++) {
        float w = (i < len) 
        ? GetTextSizeCached(font, fontSize, (char[]){text[i],0}).width 
        : GetTextSizeCached(font, fontSize, "m").width / 2;  
        float next = x + w;
        if (mouseX < (x + next) / 2) return i;
        x = next;
    }
    return len;
}

void RecalculateWrappedLines() {
    numWrappedLines = 0;
    if (!wordWrap) {
        for (int i = 0; i < numLines && i < MAXTEXT * 2; i++) {
            wrappedLines[i] = (WrappedLine){i, 0, lines[i].length, false};
            numWrappedLines++;
        }
        return;
    }
    int availWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
    availWidth = fmax(100, availWidth);
    for (int i = 0; i < numLines; i++) {
        if (numWrappedLines >= MAXTEXT * 2 - 1) break;
        const char* text = lines[i].text;
        int lineLen = lines[i].length;
        if (lineLen == 0 || GetTextSizeCached(font, fontSize, text).width <= availWidth) {
            wrappedLines[numWrappedLines++] = (WrappedLine){i, 0, lineLen, false};
            continue;
        }
        int startCol = 0;
        while (startCol < lineLen && numWrappedLines < MAXTEXT * 2 - 1) {
            int endCol = startCol, lastBreak = -1;
            float curWidth = 0;
            while (endCol < lineLen) {
                char c = text[endCol];
                float charW = GetTextSizeCached(font, fontSize, (char[]){c,0}).width;
                if (curWidth + charW > availWidth) break;
                curWidth += charW;
                if (c == ' ' || c == '\t' || c == '-') lastBreak = endCol;
                endCol++;
            }
            if (endCol == startCol) endCol = startCol + 1;
            else if (lastBreak != -1 && endCol < lineLen) endCol = lastBreak + 1;
            wrappedLines[numWrappedLines++] = (WrappedLine){i, startCol, endCol - startCol, (startCol > 0)};
            startCol = endCol;
        }
    }
}

void UpdateCursor(double x, double y) {
    UpdateGutterWidth();
    if (showScrollbar && x >= window.screen_width - scrollbarWidth) return;
    float adjustedY = y + scroll.currentY;
    int wrappedLine = (int)(adjustedY / lineHeight);
    int maxLine = wordWrap ? numWrappedLines - 1 : numLines - 1;
    wrappedLine = fmax(0, fmin(maxLine, wrappedLine));
    int wrappedCol = FindCharPos(wrappedLine, x);
    if (wordWrap && numWrappedLines > 0) {
        int origLine, origCol;
        WrappedToOriginal(wrappedLine, wrappedCol, &origLine, &origCol);
        if (origLine >= 0 && origLine < numLines) {
            cursorLine = origLine;
            cursorCol = fmin(origCol, lines[origLine].length);
        }
    } else if (wrappedLine >= 0 && wrappedLine < numLines) {
        cursorLine = wrappedLine;
        cursorCol = fmin(wrappedCol, lines[cursorLine].length);
    }
    ClampCursor();
}

int GetGlobalPos(int line, int col) {
    int pos = 0;
    for (int i = 0; i < line; ++i) pos += strlen(lines[i].text) + 1;
    return pos + col;
}

void DeleteSelection() {
    if (selStartLine == -1 || selEndLine == -1 || 
        selStartLine < 0 || selStartLine >= numLines ||
        selEndLine < 0 || selEndLine >= numLines) {
            selStartLine = selStartCol = selEndLine = selEndCol = -1;
            return;
        }
        int startLine = selStartLine, startCol = selStartCol;
        int endLine = selEndLine, endCol = selEndCol;
        if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
            int tl = startLine, tc = startCol;
            startLine = endLine; startCol = endCol;
            endLine = tl; endCol = tc;
        }
        startCol = fmax(0, fmin(startCol, lines[startLine].length));
        endCol = fmax(0, fmin(endCol, lines[endLine].length));
        if (startLine == endLine) {
            if (startCol >= endCol) {
                selStartLine = selStartCol = selEndLine = selEndCol = -1;
                isSelecting = false;
                return;
            }
            int charsToMove = lines[startLine].length - endCol;
            if (charsToMove > 0) {
                memmove(&lines[startLine].text[startCol], 
                    &lines[startLine].text[endCol], 
                    charsToMove + 1);
                } else {
                    lines[startLine].text[startCol] = '\0';
                }
                lines[startLine].length -= (endCol - startCol);
            } else {
                size_t endLineRemaining = lines[endLine].length - endCol;
                if (startCol + endLineRemaining >= MAXTEXT) {
                    lines[startLine].text[startCol] = '\0';
                    lines[startLine].length = startCol;
                    snprintf(statusMsg, sizeof(statusMsg), "Selection deleted (text truncated)");
                } else {
                    if (endLineRemaining > 0) {
                        memcpy(lines[startLine].text + startCol, lines[endLine].text + endCol, endLineRemaining + 1);
                        lines[startLine].length = startCol + endLineRemaining;
                    } else {
                lines[startLine].text[startCol] = '\0';
                lines[startLine].length = startCol;
            }
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
    if (wordWrap) {
        RecalculateWrappedLines();
    }
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
    float charWidth = GetTextSizeCached(font, fontSize, "M").width;
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

float GetDocumentHeight() { return (wordWrap ? numWrappedLines : numLines) * lineHeight; }

float GetVisibleHeightRatio() {
    float docHeight = GetDocumentHeight();
    if (docHeight <= 0) return 1.0f;
    int visibleHeight = GetVisibleHeight();
    return fmin(1.0f, visibleHeight / docHeight);
}

int GetDocumentMaxWidth() {
    int maxWidth = 0;
    for (int i = 0; i < numLines; i++) {
        TextSize size = GetTextSizeCached(font, fontSize, lines[i].text);
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
        int visibleHeight = GetVisibleHeight();
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

extern bool isInPasteOperation;

void InsertNewline() {
    if(!isInPasteOperation) {
        SaveUndoLineBreak();
    }
    if (numLines >= MAXTEXT) {
        snprintf(statusMsg, sizeof(statusMsg), "Maximum number of lines reached (%d)", MAXTEXT);
        return;
    }
    if (cursorLine < 0 || cursorLine >= numLines || 
        cursorCol < 0 || cursorCol > lines[cursorLine].length) {
        return;
    }
    for (int i = numLines; i > cursorLine + 1; i--) {
        lines[i] = lines[i-1];
    }
    InitLine(cursorLine + 1);
    if (!lines[cursorLine + 1].text) {
        snprintf(statusMsg, sizeof(statusMsg), "Failed to allocate memory for new line");
        return;
    }
    int newLineLength = lines[cursorLine].length - cursorCol;
    if (newLineLength > 0) {
        memcpy(lines[cursorLine+1].text, &lines[cursorLine].text[cursorCol], newLineLength);
        lines[cursorLine+1].length = newLineLength;
        lines[cursorLine+1].text[newLineLength] = '\0';
        lines[cursorLine].text[cursorCol] = '\0';
        lines[cursorLine].length = cursorCol;
    }
    numLines++;
    cursorLine++;
    cursorCol = 0;
    scroll.targetX = 0;
    int visibleHeight = GetVisibleHeight();
    int newLineY = cursorLine * lineHeight;
    int cursorViewPosition = newLineY - scroll.currentY;
    if (cursorViewPosition < 0 || cursorViewPosition >= visibleHeight) {
        scroll.targetY = newLineY - (visibleHeight / 2);
        scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
    }
    UpdateGutterWidth();
    isFileDirty = true;
    if (wordWrap) {
        RecalculateWrappedLines();
    }
}

bool LoadFile(const char* filename) {
    if (!filename || !*filename) {
        snprintf(statusMsg, sizeof(statusMsg), "Error: Invalid filename");
        return false;
    }
    char* fileContent = FileLoad(filename);
    if (!fileContent) {
        snprintf(statusMsg, sizeof(statusMsg), "Error: Could not load file '%s'", filename);
        return false;
    }
    FreeLines();
    numLines = 0;
    char* start = fileContent;
    char* end;
    bool hasCRLF = false;
    while (numLines < MAXTEXT) {
        end = strchr(start, '\n');
        size_t len = end ? (size_t)(end - start) : strlen(start);
        if (end && len > 0 && start[len-1] == '\r') {
            len--;
            hasCRLF = true;
        }
        InitLine(numLines);
        if (!lines[numLines].text) {
            free(fileContent);
            snprintf(statusMsg, sizeof(statusMsg), "Error: Memory allocation failed");
            return false;
        }
        if (len >= MAXTEXT) {
            len = MAXTEXT - 1;
            snprintf(statusMsg, sizeof(statusMsg), "Warning: Line %d truncated (too long)", numLines + 1);
        }
        memcpy(lines[numLines].text, start, len);
        lines[numLines].text[len] = '\0';
        lines[numLines].length = len;
        numLines++;
        if (!end) break;
        start = end + 1;
    }
    free(fileContent);
    if (strlen(start) > 0 && numLines >= MAXTEXT) {
        snprintf(statusMsg, sizeof(statusMsg), "Warning: File truncated (too many lines)");
    }
    strcpy(lineEnding, hasCRLF ? "CRLF" : "LF");
    if (numLines == 0) {
        InitLine(0);
        if (!lines[0].text) {
            snprintf(statusMsg, sizeof(statusMsg), "Error: Memory allocation failed");
            return false;
        }
        numLines = 1;
    }
    cursorLine = cursorCol = 0;
    scroll.targetX = scroll.currentX = 0;
    scroll.targetY = scroll.currentY = 0;
    isFileDirty = false;
    return true;
}

float GetCursorXPosition() {
    int textX = showLineNumbers ? gutterWidth : 0;
    float cursorX = textX;
    for (int i = 0; i < cursorCol; i++) {
        char charStr[2] = {lines[cursorLine].text[i], '\0'};
        TextSize size = GetTextSizeCached(font, fontSize, charStr);
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
    SaveDeleteWordForward();
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
    lineHeight = (int)GetTextSizeCached(font, fontSize, "gj|").height;
    float ratio = fontSize / oldFontSize;
    float newCursorY = cursorLine * lineHeight;
    float newCursorX = cursorScreenX * ratio;
    scroll.targetY = newCursorY - cursorScreenY;
    scroll.targetX = GetCursorXPosition() - newCursorX;
    scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
    scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), scroll.targetX));
    UpdateGutterWidth();
    AdjustScrollToCursor();
    SyntaxSetFontSize(fontSize);
}
