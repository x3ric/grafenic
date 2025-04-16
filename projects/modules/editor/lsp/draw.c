
void LspDrawDiagnostics() {
    if (!lsp.active || lsp.diagnosticCount == 0) return;
    int startLine = (int)(scroll.currentY / lineHeight);
    int endLine = startLine + (window.screen_height / lineHeight) + 1;
    int textX = showLineNumbers ? gutterWidth : 0;
    for (int i = 0; i < lsp.diagnosticCount; i++) {
        LspDiagnostic* diag = &lsp.diagnostics[i];
        if (diag->line < startLine || diag->line >= endLine || diag->line >= numLines) 
            continue;
        const char* lineText = lines[diag->line].text;
        int lineLen = lines[diag->line].length;
        int diagLineY = (diag->line * lineHeight) - (int)scroll.currentY;
        int startCol = fmax(0, fmin(diag->startCol, lineLen));
        int endCol = fmax(startCol, fmin(diag->endCol, lineLen));
        int startX = textX;
        for (int c = 0; c < startCol; c++) {
            char charStr[2] = {lineText[c], '\0'};
            startX += GetTextSizeCached(font, fontSize, charStr).width;
        }
        int endX = startX;
        for (int c = startCol; c < endCol; c++) {
            char charStr[2] = {lineText[c], '\0'};
            endX += GetTextSizeCached(font, fontSize, charStr).width;
        }
        if (startCol == endCol) {
            endX = startX + GetTextSizeCached(font, fontSize, "m").width;
        }
        startX -= (int)scroll.currentX;
        endX -= (int)scroll.currentX;
        Color underlineColor;
        switch (diag->severity) {
            case 1: underlineColor = Color19; break; 
            case 2: underlineColor = Color18; break; 
            case 3: underlineColor = Color20; break; 
            case 4: underlineColor = Color03; break; 
            default: underlineColor = Color08; break;
        }
        float lineThickness = fmax(1.0f, (fontSize / 20.0f));
        float yPos = diagLineY + lineHeight + lineThickness - 1;
        float dashLength = fmax(3.0f, fontSize / 6.0f);
        float gapLength = fmax(2.0f, fontSize / 10.0f);
        float currentX = startX;
        while (currentX < endX) {
            float segmentEnd = fmin(currentX + dashLength, endX);
            DrawLine(currentX, yPos, segmentEnd, yPos, lineThickness, underlineColor);
            currentX = segmentEnd + gapLength;
        }
    }
    FlushRectBatch();
    FlushTextBatch();
    for (int i = 0; i < lsp.diagnosticCount; i++) {
        LspDiagnostic* diag = &lsp.diagnostics[i];
        if (diag->line < startLine || diag->line >= endLine || diag->line >= numLines) continue;
        const char* lineText = lines[diag->line].text;
        int lineLen = lines[diag->line].length;
        int diagLineY = (diag->line * lineHeight) - (int)scroll.currentY;
        int startCol = fmax(0, fmin(diag->startCol, lineLen));
        int endCol = fmax(startCol, fmin(diag->endCol, lineLen));
        int startX = textX;
        for (int c = 0; c < startCol; c++) {
            char charStr[2] = {lineText[c], '\0'};
            startX += GetTextSizeCached(font, fontSize, charStr).width;
        }
        int endX = startX;
        if (startCol == endCol) {
            endX = startX + GetTextSizeCached(font, fontSize, "m").width;
        } else {
            for (int c = startCol; c < endCol; c++) {
                char charStr[2] = {lineText[c], '\0'};
                endX += GetTextSizeCached(font, fontSize, charStr).width;
            }
        }
        startX -= (int)scroll.currentX;
        endX -= (int)scroll.currentX;
        bool isHovered = (
            (mouse.x >= startX && mouse.x <= endX && 
             mouse.y >= diagLineY && mouse.y <= diagLineY + lineHeight) ||
            (mouse.x <= textX && mouse.x >= textX - 20 && 
             mouse.y >= diagLineY && mouse.y <= diagLineY + lineHeight)
        );
        if (isHovered) {
            Color diagColor;
            const char* sevLabel;
            switch (diag->severity) {
                case 1: diagColor = Color19; sevLabel = "Error"; break;
                case 2: diagColor = Color18; sevLabel = "Warning"; break;
                case 3: diagColor = Color20; sevLabel = "Info"; break;
                default: diagColor = Color08; sevLabel = "Note"; break;
            }
            float tooltipFontSize = fontSize * 0.78f;
            int padding = (int)(fontSize * 0.3f);
            char headerText[256];
            snprintf(headerText, sizeof(headerText), "%s at Line %d", 
                     sevLabel, diag->line + 1);
            const char* msg = diag->message ? diag->message : "No description available";
            int msgLen = strlen(msg);
            TextSize headerSize = GetTextSizeCached(font, tooltipFontSize, headerText);
            int maxLineWidth = headerSize.width;
            int tempPos = 0;
            int charWidth = (int)(tooltipFontSize * 0.5f);
            int maxCharsPerLine = (int)(fontSize * 25.0f) / charWidth;
            while (tempPos < msgLen) {
                int lineEnd = tempPos;
                int lineLen = 0;
                while (lineEnd < msgLen && lineLen < maxCharsPerLine && msg[lineEnd] != '\n') {
                    lineEnd++;
                    lineLen++;
                }
                char tempBuf[512] = {0};
                strncpy(tempBuf, msg + tempPos, lineLen);
                tempBuf[lineLen] = '\0';
                TextSize lineSize = GetTextSizeCached(font, tooltipFontSize, tempBuf);
                if (lineSize.width > maxLineWidth) {
                    maxLineWidth = lineSize.width;
                }
                tempPos = lineEnd;
                if (tempPos < msgLen && msg[tempPos] == '\n') {
                    tempPos++;
                }
            }
            int tooltipWidth = maxLineWidth + padding * 2 + (int)(fontSize * 2.0f);
            tooltipWidth = fmax(tooltipWidth, (int)(fontSize * 18.0f));
            tooltipWidth = fmin(tooltipWidth, (int)(fontSize * 35.0f));
            tooltipWidth = fmin(tooltipWidth, window.screen_width - 20);
            int contentWidth = tooltipWidth - padding * 2;
            int charsPerLine = contentWidth / charWidth;
            int lineCount = 0;
            tempPos = 0;
            while (tempPos < msgLen) {
                int lineLen = fmin(msgLen - tempPos, charsPerLine);
                for (int i = 0; i < lineLen; i++) {
                    if (msg[tempPos + i] == '\n') {
                        lineLen = i + 1;
                        break;
                    }
                }
                if (tempPos + lineLen < msgLen && lineLen == charsPerLine) {
                    int j = lineLen;
                    while (j > 0 && msg[tempPos + j] != ' ' && msg[tempPos + j] != '\n') {
                        j--;
                    }
                    if (j > 0) lineLen = j + 1;
                }
                lineCount++;
                tempPos += lineLen;
                while (tempPos < msgLen && (msg[tempPos] == ' ' || msg[tempPos] == '\n')) {
                    tempPos++;
                }
            }
            int headerHeight = (int)(tooltipFontSize * 1.3f);
            int lineHeight = (int)(tooltipFontSize * 1.05f);
            int tooltipHeight = headerHeight + (lineHeight * lineCount) + padding * 2;
            int maxHeightAllowed = window.screen_height - 20;
            if (tooltipHeight > maxHeightAllowed) {
                tooltipHeight = maxHeightAllowed;
                lineCount = (tooltipHeight - headerHeight - padding * 2) / lineHeight;
            }
            int tooltipX = mouse.x + (int)(fontSize * 0.3f);
            int tooltipY;
            if (mouse.y < window.screen_height / 2) {
                tooltipY = diagLineY + lineHeight + 2;
            } else {
                tooltipY = diagLineY - tooltipHeight - 2;
            }
            if (tooltipX + tooltipWidth > window.screen_width)
                tooltipX = window.screen_width - tooltipWidth - 5;
            if (tooltipX < 5)
                tooltipX = 5;
            if (tooltipY + tooltipHeight > window.screen_height)
                tooltipY = window.screen_height - tooltipHeight - 5;
            if (tooltipY < 5)
                tooltipY = 5;
            DrawRectBatch(tooltipX, tooltipY, tooltipWidth, tooltipHeight, Color30);
            DrawRectBorder(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 1, diagColor);
            DrawRectBatch(tooltipX, tooltipY, tooltipWidth, headerHeight, diagColor);
            DrawTextBatch(
                tooltipX + padding,
                tooltipY + (headerHeight - tooltipFontSize) / 2,
                font, 
                tooltipFontSize, 
                headerText, 
                Color15
            );
            int contentX = tooltipX + padding;
            int contentY = tooltipY + headerHeight + padding/2;
            int linesDrawn = 0;
            int pos = 0;
            while (pos < msgLen && linesDrawn < lineCount) {
                while (pos < msgLen && (msg[pos] == ' ' || msg[pos] == '\n')) {
                    pos++;
                }
                if (pos >= msgLen) break;
                int lineLen = fmin(msgLen - pos, charsPerLine);
                for (int i = 0; i < lineLen; i++) {
                    if (msg[pos + i] == '\n') {
                        lineLen = i;
                        break;
                    }
                }
                if (pos + lineLen < msgLen && lineLen == charsPerLine) {
                    int j = lineLen;
                    while (j > 0 && msg[pos + j] != ' ') {
                        j--;
                    }
                    if (j > 0) {
                        lineLen = j;
                    }
                }
                if (lineLen > 0) {
                    char lineBuf[512] = {0};
                    strncpy(lineBuf, msg + pos, lineLen);
                    lineBuf[lineLen] = '\0';
                    int end = lineLen - 1;
                    while (end >= 0 && (lineBuf[end] == ' ' || lineBuf[end] == '\t')) {
                        lineBuf[end--] = '\0';
                    }
                    DrawTextBatch(
                        contentX,
                        contentY + (linesDrawn * lineHeight),
                        font,
                        tooltipFontSize,
                        lineBuf,
                        Color15
                    );
                    linesDrawn++;
                }
                pos += lineLen;
                if (pos < msgLen && msg[pos] == '\n') {
                    pos++;
                }
                while (pos < msgLen && msg[pos] == ' ') {
                    pos++;
                }
                if (lineLen <= 0) break;
            }
        }
    }
    FlushRectBatch();
    FlushTextBatch();
}

void LspDrawCompletions() {
    if (!lsp.completions.active || lsp.completions.count == 0) return;
    static double completionStartTime = 0;
    static bool isFirstShow = true;
    if (isFirstShow) {
        completionStartTime = window.time;
        isFirstShow = false;
        return;
    }
    if (window.time - completionStartTime < 0.1) return;
    float completionFontSize = fontSize * 0.95f;
    float detailFontSize = fontSize * 0.85f;
    int itemHeight = (int)(fontSize * 1.3f);
    int padding = (int)(fontSize * 0.5f);
    int iconWidth = (int)(fontSize * 1.5f);
    int maxWidth = (int)(window.screen_width * 0.4f);
    int cursorX, cursorY;
    if (wordWrap) {
        int wl, wc;
        OriginalToWrapped(lsp.completions.startLine, lsp.completions.startCol, &wl, &wc);
        cursorY = wl * lineHeight - scroll.currentY;
    } else {
        cursorY = lsp.completions.startLine * lineHeight - scroll.currentY;
    }
    int textX = showLineNumbers ? gutterWidth : 0;
    cursorX = textX;
    if (lsp.completions.startLine < numLines) {
        for (int i = 0; i < lsp.completions.startCol && i < lines[lsp.completions.startLine].length; i++) {
            char charStr[2] = {lines[lsp.completions.startLine].text[i], '\0'};
            TextSize size = GetTextSizeCached(font, fontSize, charStr);
            cursorX += size.width;
        }
    }
    cursorX -= scroll.currentX;
    int popupWidth = 0;
    for (int i = 0; i < lsp.completions.count; i++) {
        TextSize labelSize = GetTextSizeCached(font, completionFontSize, lsp.completions.items[i].label);
        int itemWidth = labelSize.width + iconWidth + padding * 3;
        if (lsp.completions.items[i].detail[0] != '\0') {
            TextSize detailSize = GetTextSizeCached(font, detailFontSize, lsp.completions.items[i].detail);
            itemWidth += detailSize.width + padding;
        }
        if (itemWidth > popupWidth) popupWidth = itemWidth;
    }
    popupWidth = fmax(200, fmin(maxWidth, popupWidth));
    int maxVisibleItems = 10;
    int visibleItems = fmin(maxVisibleItems, lsp.completions.count);
    int popupHeight = visibleItems * itemHeight + padding * 2;
    int hintHeight = (int)(fontSize * 1.2f);
    popupHeight += hintHeight;
    int popupX = cursorX;
    int popupY = cursorY + lineHeight + 2;
    if (popupX + popupWidth > window.screen_width) 
        popupX = window.screen_width - popupWidth - 5;
    if (popupX < 0) 
        popupX = 0;
    if (popupY + popupHeight > window.screen_height) 
        popupY = cursorY - popupHeight - 2;
    if (popupY < 0) 
        popupY = 0;
    DrawRectBatch(popupX, popupY, popupWidth, popupHeight, Color30);
    DrawRectBorder(popupX, popupY, popupWidth, popupHeight, 1, Color13);
    DrawRectBatch(popupX, popupY, popupWidth, (int)(fontSize * 0.8f) + padding/2, Color04);
    DrawTextBatch(
        popupX + padding, 
        popupY + padding/4, 
        font, 
        fontSize * 0.7f, 
        "Completions", 
        Color15
    );
    int titleBarHeight = (int)(fontSize * 0.8f) + padding/2;
    int visibleStart = 0;
    if (lsp.completions.count > visibleItems) {
        if (lsp.completions.selectedIndex >= visibleItems) 
            visibleStart = lsp.completions.selectedIndex - visibleItems/2;
        if (lsp.completions.selectedIndex < visibleStart)
            visibleStart = lsp.completions.selectedIndex;
        visibleStart = fmax(0, fmin(lsp.completions.count - visibleItems, visibleStart));
    }
    for (int i = 0; i < visibleItems && i + visibleStart < lsp.completions.count; i++) {
        int itemIndex = i + visibleStart;
        CompletionItem* item = &lsp.completions.items[itemIndex];
        int itemY = popupY + titleBarHeight + i * itemHeight;
        if (itemIndex == lsp.completions.selectedIndex) {
            DrawRectBatch(popupX + 1, itemY, popupWidth - 2, itemHeight, Color00);
            DrawRectBorder(popupX + 1, itemY, popupWidth - 2, itemHeight, 1, Color13);
        }
        Color iconColor;
        char iconChar = ' ';
        switch (item->kind) {
            case 1:  iconChar = 'T'; iconColor = Color23; break;
            case 2:  iconChar = 'M'; iconColor = Color03; break;
            case 3:  iconChar = 'F'; iconColor = Color03; break;
            case 4:  iconChar = 'C'; iconColor = Color02; break;
            case 5:  iconChar = 'F'; iconColor = Color16; break;
            case 6:  iconChar = 'V'; iconColor = Color16; break;
            case 7:  iconChar = 'C'; iconColor = Color01; break;
            case 8:  iconChar = 'I'; iconColor = Color01; break;
            case 9:  iconChar = 'M'; iconColor = Color22; break;
            case 10: iconChar = 'P'; iconColor = Color16; break;
            case 11: iconChar = 'U'; iconColor = Color18; break;
            case 12: iconChar = 'V'; iconColor = Color16; break;
            case 13: iconChar = 'E'; iconColor = Color01; break;
            case 14: iconChar = 'K'; iconColor = Color19; break;
            case 15: iconChar = 'S'; iconColor = Color28; break;
            case 16: iconChar = 'C'; iconColor = Color08; break;
            case 17: iconChar = 'F'; iconColor = Color08; break;
            case 18: iconChar = 'R'; iconColor = Color08; break;
            case 19: iconChar = 'F'; iconColor = Color08; break;
            case 20: iconChar = 'E'; iconColor = Color01; break;
            case 21: iconChar = 'C'; iconColor = Color08; break;
            case 22: iconChar = 'S'; iconColor = Color01; break;
            case 23: iconChar = 'E'; iconColor = Color08; break;
            case 24: iconChar = 'O'; iconColor = Color08; break;
            case 25: iconChar = 'T'; iconColor = Color06; break;
            default: iconChar = '?'; iconColor = Color07; break;
        }
        char iconStr[2] = {iconChar, '\0'};
        int iconX = popupX + padding;
        int iconY = itemY + (itemHeight - fontSize) / 2;
        if (itemIndex == lsp.completions.selectedIndex) {
            DrawRectBatch(
                iconX - padding/2, 
                iconY - padding/4, 
                fontSize + padding, 
                fontSize + padding/2, 
                Color05
            );
        }
        DrawTextBatch(iconX, iconY, font, fontSize, iconStr, iconColor);
        Color labelColor = (itemIndex == lsp.completions.selectedIndex) ? Color15 : Color07;
        DrawTextBatch(
            iconX + iconWidth, 
            iconY, 
            font, 
            completionFontSize, 
            item->label, 
            labelColor
        );
        if (item->detail[0] != '\0') {
            TextSize labelSize = GetTextSizeCached(font, completionFontSize, item->label);
            Color detailColor = (itemIndex == lsp.completions.selectedIndex) ? Color02 : Color08;
            char detailStr[256];
            strncpy(detailStr, item->detail, sizeof(detailStr) - 1);
            detailStr[sizeof(detailStr) - 1] = '\0';
            int maxDetailWidth = popupWidth - iconWidth - labelSize.width - padding * 4;
            TextSize detailSize = GetTextSizeCached(font, detailFontSize, detailStr);
            if (detailSize.width > maxDetailWidth) {
                int len = strlen(detailStr);
                while (len > 3 && detailSize.width > maxDetailWidth) {
                    detailStr[len-3] = '.';
                    detailStr[len-2] = '.';
                    detailStr[len-1] = '.';
                    detailStr[len] = '\0';
                    len--;
                    detailSize = GetTextSizeCached(font, detailFontSize, detailStr);
                }
            }
            DrawTextBatch(
                iconX + iconWidth + labelSize.width + padding, 
                iconY + (fontSize - detailFontSize) / 2, 
                font, 
                detailFontSize, 
                detailStr, 
                detailColor
            );
        }
    }
    if (lsp.completions.count > visibleItems) {
        int scrollbarWidth = 4;
        int scrollbarHeight = (visibleItems * (popupHeight - titleBarHeight - hintHeight)) / lsp.completions.count;
        scrollbarHeight = fmax(20, scrollbarHeight);
        float scrollRatio = (float)visibleStart / (lsp.completions.count - visibleItems);
        int scrollbarY = popupY + titleBarHeight + scrollRatio * (popupHeight - titleBarHeight - hintHeight - scrollbarHeight);
        DrawRectBatch(
            popupX + popupWidth - scrollbarWidth - 2, 
            popupY + titleBarHeight, 
            scrollbarWidth, 
            popupHeight - titleBarHeight - hintHeight, 
            Color29
        );
        DrawRectBatch(
            popupX + popupWidth - scrollbarWidth - 2, 
            scrollbarY, 
            scrollbarWidth, 
            scrollbarHeight, 
            Color13
        );
    }
    int hintY = popupY + popupHeight - hintHeight;
    DrawRectBatch(popupX, hintY, popupWidth, hintHeight, Color30);
    DrawRectBorder(popupX, hintY, popupWidth, 1, 1, Color05);
    char hintText[] = "↑/↓: Navigate  Tab: Select  Esc: Cancel";
    TextSize hintSize = GetTextSizeCached(font, fontSize * 0.75, hintText);
    int hintX = popupX + (popupWidth - hintSize.width) / 2;
    DrawTextBatch(hintX, hintY + (hintHeight - hintSize.height) / 2, font, fontSize * 0.75, hintText, Color02);
    if (!lsp.completions.active) isFirstShow = true;
}

void LspDrawHoverInfo() {
    if (!lsp.hoverInfo.active || !lsp.hoverInfo.contents[0]) return;
    float baseHoverFontSize = fontSize * 0.9f;
    int horizontalPadding = (int)(fontSize * 0.6f);
    int verticalPadding = (int)(fontSize * 0.3f);
    float lineSpacingFactor = 1.1f;
    int cursorX, cursorY;
    if (wordWrap) {
        int wl, wc;
        OriginalToWrapped(lsp.hoverInfo.line, lsp.hoverInfo.col, &wl, &wc);
        cursorY = wl * lineHeight - scroll.currentY;
    } else {
        cursorY = lsp.hoverInfo.line * lineHeight - scroll.currentY;
    }
    int textX = showLineNumbers ? gutterWidth : 0;
    cursorX = textX;
    if (lsp.hoverInfo.line < numLines) {
        for (int i = 0; i < lsp.hoverInfo.col && i < lines[lsp.hoverInfo.line].length; i++) {
            char charStr[2] = {lines[lsp.hoverInfo.line].text[i], '\0'};
            TextSize size = GetTextSizeCached(font, fontSize, charStr);
            cursorX += size.width;
        }
    }
    cursorX -= scroll.currentX;
    typedef struct {
        const char* pattern;
        Color color;
        bool isPrefix;
    } SyntaxPattern;
    SyntaxPattern syntaxPatterns[] = {
        {"class", Color01, false}, {"struct", Color01, false}, {"enum", Color01, false},
        {"typedef", Color01, false}, {"static", Color09, false}, {"const", Color09, false},
        {"int", Color06, false}, {"char", Color06, false}, {"bool", Color06, false},
        {"void", Color06, false}, {"size_t", Color06, false}, {"//", Color08, true},
        {"/*", Color08, true}, {"#", Color08, true}, {"\"", Color28, false},
        {"'", Color28, false}, {"@param", Color03, true}, {"@return", Color03, true},
        {"Parameters:", Color22, true}, {"Returns:", Color22, true}, {"**", Color14, false},
        {"`", Color10, false},
    };
    char* tempCopy = strdup(lsp.hoverInfo.contents);
    if (!tempCopy) return;
    char* tmpContext = NULL;
    char* tmpLine = strtok_r(tempCopy, "\n", &tmpContext);
    int maxRawLineWidth = 0;
    while (tmpLine) {
        TextSize lineSize = GetTextSizeCached(font, baseHoverFontSize, tmpLine);
        maxRawLineWidth = fmax(maxRawLineWidth, lineSize.width);
        tmpLine = strtok_r(NULL, "\n", &tmpContext);
    }
    free(tempCopy);
    bool isDefinition = (strstr(lsp.hoverInfo.contents, "DEFINITION DETAILS") != NULL);
    int tooltipWidth = maxRawLineWidth + (horizontalPadding * 2);
    tooltipWidth += (int)(fontSize * 2.0f);
    if (isDefinition) {
        tooltipWidth = fmax(tooltipWidth, (int)(fontSize * 35.0f));
    } else {
        tooltipWidth = fmax(tooltipWidth, (int)(fontSize * 25.0f));
    }
    tooltipWidth = fmin(tooltipWidth, window.screen_width - 10);
    int usableWidth = tooltipWidth - (horizontalPadding * 2);
    char* lineCopy = strdup(lsp.hoverInfo.contents);
    if (!lineCopy) return;
    char* context = NULL;
    char* line = strtok_r(lineCopy, "\n", &context);
    char** wrappedLines = malloc(1000 * sizeof(char*));
    if (!wrappedLines) {
        free(lineCopy);
        return;
    }
    int numWrappedLines = 0;
    int totalHeight = 0;
    while (line) {
        bool isSpecialLine = (strstr(line, "```") == line || 
                             strstr(line, "@param") == line || 
                             strstr(line, "//") == line);
        if (isSpecialLine || strlen(line) == 0) {
            wrappedLines[numWrappedLines] = strdup(line);
            TextSize lineSize = GetTextSizeCached(font, baseHoverFontSize, line);
            totalHeight += (int)(lineSize.height * lineSpacingFactor);
            numWrappedLines++;
        } else {
            int len = strlen(line);
            int startPos = 0;
            while (startPos < len) {
                int charsInLine = 0;
                int lineWidth = 0;
                int lastSpace = -1;
                for (int i = startPos; i <= len; i++) {
                    if (i == len || line[i] == ' ') {
                        int wordEnd = i;
                        int wordStart = startPos + charsInLine;
                        char wordBuf[1024] = {0};
                        int wordLen = wordEnd - wordStart;
                        if (wordLen > 0 && wordLen < 1024) {
                            strncpy(wordBuf, line + wordStart, wordLen);
                            wordBuf[wordLen] = '\0';
                            TextSize wordSize = GetTextSizeCached(font, baseHoverFontSize, wordBuf);
                            if (lineWidth + wordSize.width > usableWidth && lineWidth > 0) {
                                break;
                            }
                            lineWidth += wordSize.width;
                            charsInLine = i - startPos;
                            if (i < len && line[i] == ' ') {
                                lastSpace = i;
                                charsInLine++;
                            }
                        }
                    }
                }
                if (lastSpace == -1 && charsInLine == 0 && len - startPos > 0) {
                    charsInLine = 1;
                    while (startPos + charsInLine < len) {
                        char testBuf[1024] = {0};
                        strncpy(testBuf, line + startPos, charsInLine);
                        testBuf[charsInLine] = '\0';
                        TextSize testSize = GetTextSizeCached(font, baseHoverFontSize, testBuf);
                        if (testSize.width > usableWidth) break;
                        charsInLine++;
                    }
                    charsInLine = fmax(1, charsInLine - 1);
                }
                if (charsInLine > 0) {
                    char* wrappedLine = malloc(charsInLine + 1);
                    if (wrappedLine) {
                        strncpy(wrappedLine, line + startPos, charsInLine);
                        wrappedLine[charsInLine] = '\0';
                        wrappedLines[numWrappedLines++] = wrappedLine;
                        TextSize lineSize = GetTextSizeCached(font, baseHoverFontSize, wrappedLine);
                        totalHeight += (int)(lineSize.height * lineSpacingFactor);
                    }
                }
                startPos += charsInLine;
                while (startPos < len && line[startPos] == ' ') startPos++;
                if (startPos >= len) break;
            }
        }
        line = strtok_r(NULL, "\n", &context);
    }
    free(lineCopy);
    totalHeight += (int)(baseHoverFontSize);
    int tooltipHeight = totalHeight + (verticalPadding * 2);
    tooltipHeight = fmin(tooltipHeight, window.screen_height - 10);
    int tooltipX = cursorX + (int)(fontSize * 0.5f);
    int tooltipY = cursorY + lineHeight + 2;
    if (tooltipX + tooltipWidth > window.screen_width - 5) {
        tooltipX = window.screen_width - tooltipWidth - 5;
    }
    if (tooltipY + tooltipHeight > window.screen_height - 5) {
        tooltipY = cursorY - tooltipHeight - 2;
    }
    DrawRectBatch(tooltipX, tooltipY, tooltipWidth, tooltipHeight, Color30);
    DrawRectBorder(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 1, Color05);
    DrawRectBatch(tooltipX, tooltipY, tooltipWidth, 2, Color04);
    FlushRectBatch();
    int lineY = tooltipY + verticalPadding;
    bool inCodeBlock = false;
    int visibleLines = numWrappedLines;
    for (int i = 0; i < visibleLines; i++) {
        if (!wrappedLines[i] || !wrappedLines[i][0]) {
            lineY += (int)(baseHoverFontSize * 0.6f);
            continue;
        }
        if (strncmp(wrappedLines[i], "```", 3) == 0) {
            inCodeBlock = !inCodeBlock;
            DrawTextBatch(tooltipX + horizontalPadding, lineY, font, baseHoverFontSize, wrappedLines[i], Color10);
            lineY += (int)(baseHoverFontSize * lineSpacingFactor);
            continue;
        }
        Color textColor = inCodeBlock ? Color10 : Color07;
        if (!inCodeBlock) {
            for (size_t p = 0; p < sizeof(syntaxPatterns)/sizeof(syntaxPatterns[0]); p++) {
                const char* pattern = syntaxPatterns[p].pattern;
                bool isPrefix = syntaxPatterns[p].isPrefix;
                bool matched = (isPrefix && strncmp(wrappedLines[i], pattern, strlen(pattern)) == 0) || 
                              (!isPrefix && strstr(wrappedLines[i], pattern));
                if (matched) {
                    textColor = syntaxPatterns[p].color;
                    break;
                }
            }
        }
        DrawTextBatch(tooltipX + horizontalPadding, lineY, font, baseHoverFontSize, wrappedLines[i], textColor);
        lineY += (int)(baseHoverFontSize * lineSpacingFactor);
    }
    char* closeHint = "Esc to close";
    float hintFontSize = baseHoverFontSize * 0.65f;
    TextSize hintSize = GetTextSizeCached(font, hintFontSize, closeHint);
    int hintX = tooltipX + tooltipWidth - hintSize.width - horizontalPadding;
    int hintY = tooltipY + tooltipHeight - (int)(baseHoverFontSize * 0.9f);
    DrawTextBatch(hintX, hintY, font, hintFontSize, closeHint, Color02);
    for (int i = 0; i < numWrappedLines; i++) {
        free(wrappedLines[i]);
    }
    free(wrappedLines);
    FlushTextBatch();
}