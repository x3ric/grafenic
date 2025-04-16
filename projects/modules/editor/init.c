#include "modules/editor/syntax.c"
#include "modules/editor/lsp/def.h"
#include "modules/editor/def.h"
#include "modules/editor/undo.c"
#include "modules/editor/utils.c"
#include "modules/editor/draw.c"
#include "modules/editor/clipboard.c"
#include "modules/editor/callbacks.c"

void Draw() {
    UpdateScroll(window.deltatime);
    static bool lastDirtyState = false;
    if (SyntaxIsEnabled() && (isFileDirty || lastDirtyState != isFileDirty)) {
        char* text = ConstructTextFromLines();
        if (text) {
            SyntaxUpdate(text, strlen(text));
            free(text);
            lastDirtyState = isFileDirty;
        }
    }
    DrawEditor();
    FlushTextBatch();
    DrawLineNumbers();
    DrawHorizontalScrollbar();
    DrawScrollbarAndMinimap();
    DrawModeline();
    UpdateLsp();
    FlushRectBatch();
    FlushTextBatch();
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
    Color defaultTextColor = {textColor.r, textColor.g, textColor.b, textColor.a};
    char* initialText = ConstructTextFromLines();
    int textLen = initialText ? strlen(initialText) : 0;
    if (!SyntaxInitWithLanguageDetection(defaultTextColor, SyntaxDrawWrapper, &font, fontSize, NULL, filename, initialText, textLen, statusMsg, sizeof(statusMsg))) {
        printf("Failed to initialize syntax highlighting\n");
    }
    if (initialText) {
        free(initialText);
    }
    LspInit();
    UndoSystemInit();
    glfwSetCharCallback(window.w, CharCallback);
    glfwSetKeyCallback(window.w, KeyCallback);
    glfwSetScrollCallback(window.w, ScrollCallback);
    glfwSetMouseButtonCallback(window.w, MouseCallback);
    glfwSetCursorPosCallback(window.w, CursorPosCallback);
    PreloadFontSizes(font);
}

void Close() {
    FreeLines();
    LspCleanup();
    SyntaxCleanup();
    ClearUndoHistory();
    FreeClipboardBuffer();
}

#include "modules/editor/lsp/init.c"
