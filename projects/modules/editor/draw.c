
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
        TextSize size = GetTextSizeCached(font, fontSize, num);
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

void DrawScrollbarAndMinimap() {
    if (!showMinimap && !showScrollbar) return;
    int RIGHT_EDGE = window.screen_width;
    int FULL_HEIGHT = window.screen_height;
    float docHeight = numLines * lineHeight;
    if (docHeight <= 0) return;
    if (showMinimap) {
        DrawRectBatch(RIGHT_EDGE - minimapWidth - scrollbarWidth, 0, minimapWidth + scrollbarWidth, FULL_HEIGHT, (Color){5,5,8,180});
    }
    float visibleRatio = GetVisibleHeightRatio();
    int thumbHeight = fmax(30, FULL_HEIGHT * visibleRatio);
    int maxScroll = MaxScroll();
    float scrollRatio = (maxScroll > 0) ? fmin(1.0f, fmax(0.0f, scroll.currentY / (float)maxScroll)) : 0;
    int thumbY = floor(scrollRatio * (FULL_HEIGHT - thumbHeight));
    if (showMinimap) {
        static int cachedMaxLen = 0;
        static int lastNumLines = 0;
        static int lastModified = 0;
        static float lastMinimapScale = 0;
        int mmX = RIGHT_EDGE - minimapWidth - (showScrollbar ? scrollbarWidth : 0);
        float totalDocumentHeight = numLines * lineHeight;
        float minimapScale = FULL_HEIGHT / totalDocumentHeight;
        bool documentChanged = (numLines != lastNumLines || isFileDirty != lastModified || fabs(minimapScale - lastMinimapScale) > 0.001f);
        if (documentChanged) {
            int sampleInterval = (numLines > 1000) ? numLines / 100 : 1;
            int maxLen = 0;
            for (int i = 0; i < numLines; i += sampleInterval) {
                if (lines[i].length > maxLen) maxLen = lines[i].length;
            }
            cachedMaxLen = fmax(1, maxLen);
            lastNumLines = numLines;
            lastModified = isFileDirty;
            lastMinimapScale = minimapScale;
        }
        bool hasSel = IsSelValid();
        int normStartLine, normStartCol, normEndLine, normEndCol;
        if (hasSel) {
            if (selStartLine < selEndLine || (selStartLine == selEndLine && selStartCol < selEndCol)) {
                normStartLine = selStartLine;
                normStartCol = selStartCol;
                normEndLine = selEndLine;
                normEndCol = selEndCol;
            } else {
                normStartLine = selEndLine;
                normStartCol = selEndCol;
                normEndLine = selStartLine;
                normEndCol = selStartCol;
            }
        }
        int sampleInterval = 1;
        if (numLines * minimapScale > FULL_HEIGHT) {
            sampleInterval = ceil(numLines / (FULL_HEIGHT / (lineHeight * minimapScale)));
        }
        float contentWidth = minimapWidth - 4;
        bool syntaxEnabled = SyntaxIsEnabled();
        for (int i = 0; i < numLines; i += sampleInterval) {
            float lineY = floor(((float)i * lineHeight) * minimapScale);
            if (lineY < -lineHeight || lineY > FULL_HEIGHT) continue;
            float originalLineH = fmax(1.0f, lineHeight * minimapScale);
            float lineH = originalLineH * 0.5f;
            float centeredLineY = lineY + (originalLineH - lineH) / 2;
            int len = lines[i].length;
            const char* lineText = lines[i].text;
            int leadingWhitespace = 0;
            for (int j = 0; j < len; j++) {
                if (lineText[j] == ' ') leadingWhitespace++;
                else if (lineText[j] == '\t') leadingWhitespace += 4;
                else break;
            }
            float effectiveLen = len > 0 ? len - leadingWhitespace : 0;
            float whitespaceRatio = len > 0 ? (float)leadingWhitespace / (float)cachedMaxLen : 0;
            float contentRatio = len > 0 ? fmin(1.0f, effectiveLen / (float)cachedMaxLen) : 0;
            float indentOffset = whitespaceRatio * contentWidth;
            float effectiveWidth = contentRatio * contentWidth;
            if (effectiveWidth < 4.0f && len > 0) effectiveWidth = 4.0f;
            int globalPos = 0;
            for (int j = 0; j < i; j++) {
                globalPos += lines[j].length + 1;
            }
            if (syntaxEnabled && len > 0) {
                const int MAX_CHUNKS = 8;
                int numChunks = fmin(MAX_CHUNKS, fmax(1, len / 5));
                float chunkWidth = effectiveWidth / numChunks;
                for (int chunk = 0; chunk < numChunks; chunk++) {
                    int chunkStart = (chunk * len) / numChunks;
                    int chunkEnd = ((chunk + 1) * len) / numChunks;
                    if (chunk == numChunks - 1) chunkEnd = len;
                    Color dominantColor = highlighter.defaultColor;
                    int sampleIdx = (chunkStart + chunkEnd) / 2;
                    if (sampleIdx < len && globalPos + sampleIdx < highlighter.lastTextLen) {
                        dominantColor = highlighter.colors[globalPos + sampleIdx];
                    }
                    Color chunkColor = {
                        (unsigned char)(dominantColor.r * 0.85f),
                        (unsigned char)(dominantColor.g * 0.85f),
                        (unsigned char)(dominantColor.b * 0.85f),
                        (unsigned char)(120 + fmin(135, len))
                    };
                    float chunkX = mmX + 2 + indentOffset + (chunk * chunkWidth);
                    DrawRectBatch(chunkX, floor(centeredLineY), ceil(chunkWidth), ceil(lineH), chunkColor);
                }
            } else {
                Color lineColor = {
                    (unsigned char)(textColor.r * 0.5f),
                    (unsigned char)(textColor.g * 0.5f),
                    (unsigned char)(textColor.b * 0.5f),
                    (unsigned char)(100 + fmin(150, len))
                };
                DrawRectBatch(mmX + 2 + indentOffset, floor(centeredLineY), floor(effectiveWidth), ceil(lineH), lineColor);
            }
            if (hasSel && i >= normStartLine && i <= normEndLine) {
                float selStartX = mmX + 2 + indentOffset;
                float selWidth = effectiveWidth;
                if (i == normStartLine && normStartCol > 0) {
                    float startRatio = (float)(normStartCol - leadingWhitespace) / fmax(1, effectiveLen);
                    if (startRatio < 0) startRatio = 0;
                    selStartX = mmX + 2 + indentOffset + startRatio * effectiveWidth;
                    selWidth = effectiveWidth - (startRatio * effectiveWidth);
                }
                if (i == normEndLine && normEndCol < lines[i].length) {
                    float endRatio = (float)(normEndCol - leadingWhitespace) / fmax(1, effectiveLen);
                    if (endRatio < 0) endRatio = 0;
                    selWidth = (endRatio * effectiveWidth) - (selStartX - (mmX + 2 + indentOffset));
                }
                Color selectionColor = {120, 120, 220, 150};
                DrawRectBatch(selStartX, floor(centeredLineY), fmax(1, selWidth), ceil(lineH), selectionColor);
            }
        }
        if (cursorLine >= 0 && cursorLine < numLines) {
            float cursorY = floor(((float)cursorLine * lineHeight) * minimapScale);
            int currentLineLen = lines[cursorLine].length;
            const char* curLineText = lines[cursorLine].text;
            int leadingWhitespace = 0;
            for (int j = 0; j < currentLineLen; j++) {
                if (curLineText[j] == ' ') leadingWhitespace++;
                else if (curLineText[j] == '\t') leadingWhitespace += 4;
                else break;
            }
            float effectiveLen = currentLineLen > 0 ? currentLineLen - leadingWhitespace : 0;
            float whitespaceRatio = currentLineLen > 0 ? (float)leadingWhitespace / (float)cachedMaxLen : 0;
            float contentRatio = currentLineLen > 0 ? fmin(1.0f, effectiveLen / (float)cachedMaxLen) : 0;
            float indentOffset = whitespaceRatio * contentWidth;
            float effectiveWidth = contentRatio * contentWidth;
            if (effectiveWidth < 4.0f && currentLineLen > 0) effectiveWidth = 4.0f;
            float originalLineH = fmax(1.0f, lineHeight * minimapScale);
            float cursorLineH = originalLineH * 0.5f;
            float centeredCursorY = cursorY + (originalLineH - cursorLineH) / 2;
            Color cursorLineBgColor = {60, 60, 80, 80};
            DrawRectBatch(mmX + 2 + indentOffset, floor(centeredCursorY), floor(effectiveWidth), ceil(cursorLineH), cursorLineBgColor);
            if (currentLineLen > 0 && cursorCol >= leadingWhitespace) {
                float xPosRatio = fmin(1.0f, (float)(cursorCol - leadingWhitespace) / fmax(1, effectiveLen));
                float cursorX = mmX + 2 + indentOffset + (xPosRatio * effectiveWidth);
                float cursorWidth = fmax(1.0f, minimapScale);
                Color cursorColor = {255, 255, 255, 255};
                DrawRectBatch(
                    cursorX,
                    floor(centeredCursorY),
                    cursorWidth,
                    ceil(cursorLineH),
                    cursorColor
                );
            } else if (currentLineLen > 0) {
                float cursorX = mmX + 2 + (indentOffset * cursorCol / leadingWhitespace);
                float cursorWidth = fmax(1.0f, minimapScale);
                Color cursorColor = {255, 255, 255, 255};
                DrawRectBatch(
                    cursorX,
                    floor(centeredCursorY),
                    cursorWidth,
                    ceil(cursorLineH),
                    cursorColor
                );
            }
        }
        DrawRectBatch(mmX, thumbY, minimapWidth, thumbHeight, (Color){30, 30, 40, 80});
        DrawRectBatch(mmX, thumbY, minimapWidth, 1, (Color){100, 100, 120, 120});
        DrawRectBatch(mmX, thumbY + thumbHeight - 1, minimapWidth, 1, (Color){100, 100, 120, 120});
        FlushRectBatch();
    }
    if (showMinimap) {
        int sbX = RIGHT_EDGE - scrollbarWidth;
        double timeSince = window.time - lastVerticalScrollTime;
        int alpha = timeSince < scrollbarFadeDelay ? 150 : 100;
        if (alpha > 0 || isScrollbarDragging) {
            if (isScrollbarDragging) alpha = 200;
            DrawRectBatch(sbX, thumbY, scrollbarWidth, thumbHeight, (Color){255, 255, 255, alpha});
        }
        DrawHorizontalScrollbar();
    } else if (showScrollbar) {
        int sbX = RIGHT_EDGE - scrollbarWidth;
        double timeSince = window.time - lastVerticalScrollTime;
        int alpha = timeSince < scrollbarFadeDelay ? 150 : 0;
        if (alpha > 0 || isScrollbarDragging) {
            if (isScrollbarDragging) alpha = 200;
            DrawRectBatch(sbX, thumbY, scrollbarWidth, thumbHeight, (Color){255, 255, 255, alpha});
        }
        DrawHorizontalScrollbar();
    }
}

void DrawModeline() {
    if (!showModeline) return;
    int modeLineh = 25;
    int y = window.screen_height - modeLineh;
    DrawRectBatch(0, y, window.screen_width, modeLineh, Color30);
    float fontSizeModeline = 20;
    TextSize lineHeightMetric = GetTextSizeCached(font, fontSizeModeline, "Aj|");
    int textY = y + (modeLineh - lineHeightMetric.height) / 2;
    int leftX = 10, rightX = window.screen_width - 10;
    char statusIndicator[2] = {isFileDirty ? '*' : '+', '\0'};
    Color statusColor = isFileDirty ? Color17 : Color16;
    DrawTextBatch(leftX, textY, font, fontSizeModeline, statusIndicator, statusColor);
    leftX += GetTextSizeCached(font, fontSizeModeline, statusIndicator).width + 8;
    char* displayName = filename;
    int maxFileWidth = (window.screen_width * 0.4) - leftX;
    char fileDisplay[128];
    TextSize nameSize = GetTextSizeCached(font, fontSizeModeline, displayName);
    if (nameSize.width > maxFileWidth) {
        int maxChars = maxFileWidth / (fontSizeModeline * 0.6);
        snprintf(fileDisplay, sizeof(fileDisplay), "%.*s...", maxChars - 3, displayName);
    } else {
        snprintf(fileDisplay, sizeof(fileDisplay), "%s", displayName);
    }
    DrawTextBatch(leftX, textY, font, fontSizeModeline, fileDisplay, (Color){220, 220, 225, 255});
    leftX += GetTextSizeCached(font, fontSizeModeline, fileDisplay).width + 15;
    int statusMaxWidth = rightX - leftX - 200;
    if (statusMaxWidth > 0 && statusMsg[0]) {
        char statusDisplay[128];
        strncpy(statusDisplay, statusMsg, sizeof(statusDisplay));
        statusDisplay[sizeof(statusDisplay)-1] = '\0';
        TextSize statusSize = GetTextSizeCached(font, fontSizeModeline, statusDisplay);
        while (statusSize.width > statusMaxWidth && strlen(statusDisplay) > 4) {
            statusDisplay[strlen(statusDisplay)-1] = '\0';
            statusSize = GetTextSizeCached(font, fontSizeModeline, statusDisplay);
        }
        if (statusSize.width > statusMaxWidth) {
            snprintf(statusDisplay, sizeof(statusDisplay), "%.*s...", (int)(statusMaxWidth / (fontSizeModeline * 0.6)) - 3, statusMsg);
        }
        DrawTextBatch(leftX, textY, font, fontSizeModeline, statusDisplay, (Color){200, 200, 200, 200});
        leftX += GetTextSizeCached(font, fontSizeModeline, statusDisplay).width + 15;
    }
    char timeString[20];
    time_t now = time(NULL);
    strftime(timeString, sizeof(timeString), "%H:%M", localtime(&now));
    TextSize timeSize = GetTextSizeCached(font, fontSizeModeline, timeString);
    rightX -= timeSize.width;
    DrawTextBatch(rightX, textY, font, fontSizeModeline, timeString, (Color){180, 180, 190, 255});
    rightX -= 15;
    char lspStatus[32] = {0};
    if (lsp.active) {
        if (lsp.diagnosticCount > 0) {
            int errors = 0, warnings = 0;
            for (int i = 0; i < lsp.diagnosticCount; i++) {
                if (lsp.diagnostics[i].severity == 1) errors++;
                else if (lsp.diagnostics[i].severity == 2) warnings++;
            }
            snprintf(lspStatus, sizeof(lspStatus), " %d Warn %d Err", warnings, errors);
        } else {
            snprintf(lspStatus, sizeof(lspStatus), "x");
        }
        TextSize lspSize = GetTextSizeCached(font, fontSizeModeline, lspStatus);
        rightX -= lspSize.width + 15;
        DrawTextBatch(rightX, textY, font, fontSizeModeline, lspStatus, (Color){120, 180, 120, 255});
    }
    char modeText[12];
    snprintf(modeText, sizeof(modeText), "%s", insertMode ? "INS" : "OVR");
    TextSize modeSize = GetTextSizeCached(font, fontSizeModeline, modeText);
    rightX -= modeSize.width;
    DrawTextBatch(rightX, textY, font, fontSizeModeline, modeText, insertMode ? (Color){160, 200, 220, 255} : (Color){220, 180, 160, 255});
    rightX -= 15;
    char wrapText[12];
    snprintf(wrapText, sizeof(wrapText), "%s", wordWrap ? "W" : "");
    TextSize wrapSize = GetTextSizeCached(font, fontSizeModeline, wrapText);
    rightX -= wrapSize.width;
    DrawTextBatch(rightX, textY, font, fontSizeModeline, wrapText, wordWrap ? (Color){160, 220, 180, 255} : (Color){220, 160, 180, 255});
    rightX -= 15;
    char posText[32];
    snprintf(posText, sizeof(posText), "%d:%d %d", cursorLine + 1, cursorCol + 1, (int)fontSize);
    TextSize posSize = GetTextSizeCached(font, fontSizeModeline, posText);
    rightX -= posSize.width;
    DrawTextBatch(rightX, textY, font, fontSizeModeline, posText, (Color){180, 190, 210, 255});
    rightX -= 15;
    char encText[32];
    snprintf(encText, sizeof(encText), "%s %s", fileEncoding, lineEnding);
    TextSize encSize = GetTextSizeCached(font, fontSizeModeline, encText);
    rightX -= encSize.width;
    DrawTextBatch(rightX, textY, font, fontSizeModeline, encText, (Color){150, 150, 160, 200});
}


void DrawCursor(int curCol, int lineLen, float* widths, float x, float scrollX, float scale, int lineH, bool hasSel, bool visible, bool ins, int y, const char* text) {
    int col = Clamp(curCol, 0, lineLen);
    float cx = x - scrollX + widths[col];
    float cw = (col < lineLen) ? widths[col+1] - widths[col] : font.glyphs[0].xadvance * scale;
    if (hasSel || !ins) {
        if (visible) {
            DrawRectBatch(cx, y, cw/3, lineH, (Color){255,255,255,255});
            FlushRectBatch();
        }
    } else if (visible) {
        int w = (int)cw;
        DrawRectBatch(cx, y, w, lineH, (Color){255,255,255,255});
        FlushRectBatch();
        if (col < lineLen) DrawTextBatch(cx, y, font, fontSize, (char[]){text[col],0}, (Color){0,0,0,255});
    } else DrawRectBorder(cx, y, (int)cw, lineH, 3, (Color){255,255,255,255});
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
        cachedLineHeight = (int)GetTextSizeCached(font, fontSize, "gj|").height;
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
        if (SyntaxIsEnabled()) {
            int globalPos = 0;
            for (int i = 0; i < origLine; i++) {
                globalPos += lines[i].length + 1;
            }
            globalPos += startCol;
            SyntaxDrawText(drawX, lineY, tempText, lineLength, globalPos);
        } else {
            DrawTextBatch(drawX, lineY, font, fontSize, tempText, textColor);
        }
        int wrappedCursorLine, wrappedCursorCol;
        if (wordWrap) {
            OriginalToWrapped(cursorLine, cursorCol, &wrappedCursorLine, &wrappedCursorCol);
            if (i == wrappedCursorLine) {
                DrawCursor(wrappedCursorCol, lineLength, cumWidths, textX, scroll.currentX, scale, lineHeight, lineHasSelection, isCursorVisible, insertMode, lineY, lineText);
            }
        } else if (i == cursorLine) {
            DrawCursor(cursorCol, lineLength, cumWidths, textX, scroll.currentX, scale, lineHeight, lineHasSelection, isCursorVisible, insertMode, lineY, lineText);
        }
    }
    if (widthCache.widths) {
        free(widthCache.widths);
        widthCache.widths = NULL;
    }
}

void DrawFps() {
    static char fpsText[16];
    snprintf(fpsText, sizeof(fpsText), "%.0f", window.fps);
    TextSize fps = GetTextSizeCached(font, 22, fpsText);
    DrawText(window.screen_width - fps.width - 10, (window.screen_height/2)-(fps.height/2), font, 22, fpsText, Color00);
}
