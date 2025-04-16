void EditorLspKeyHandler(int key, int action, int mods) {
    if (action != GLFW_PRESS) return;
    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (key == GLFW_KEY_ESCAPE) {
        if (lsp.completions.active || lsp.hoverInfo.active) {
            lsp.completions.active = false;
            lsp.completions.count = 0;
            lsp.completions.selectedIndex = 0;
            lsp.hoverInfo.active = false;
            preventUiReopen = true;
            preventionStartTime = window.time;
            return;
        }
    }
    if (key == GLFW_KEY_SPACE && ctrl && !shift) {
        preventUiReopen = false;
        LspRequestCompletion(cursorLine, cursorCol);
        return;
    }
    if (key == GLFW_KEY_I && ctrl) {
        preventUiReopen = false;
        LspRequestHover(cursorLine, cursorCol);
        return;
    }
    if (lsp.completions.active) {
        switch (key) {
            case GLFW_KEY_DOWN:
                lsp.completions.selectedIndex = 
                    (lsp.completions.selectedIndex + 1) % lsp.completions.count;
                return;
            case GLFW_KEY_UP:
                lsp.completions.selectedIndex = 
                    (lsp.completions.selectedIndex - 1 + lsp.completions.count) % 
                    lsp.completions.count;
                return;
            case GLFW_KEY_TAB:
            case GLFW_KEY_ENTER:
                if (LspApplySelectedCompletion()) {
                    if (wordWrap) RecalculateWrappedLines();
                    return;
                }
                break;
        }
    }
    if (ctrl && shift) {
        switch (key) {
            case GLFW_KEY_SPACE:
                ToggleAutoCompletion();
                return;
        }
    }
    switch (key) {
        case GLFW_KEY_F9:
            ToggleLsp();
            return;
        case GLFW_KEY_F10:
            preventUiReopen = false;
            lsp.hoverInfo.active = false;
            snprintf(statusMsg, sizeof(statusMsg), "Go To Definition");
            GoToDefinition();
            return;
        case GLFW_KEY_F11:
            preventUiReopen = !preventUiReopen;
            if (preventUiReopen) {
                preventionStartTime = window.time;
                lsp.completions.active = false;
                lsp.hoverInfo.active = false;
                snprintf(statusMsg, sizeof(statusMsg), "LSP popups disabled");
            } else {
                snprintf(statusMsg, sizeof(statusMsg), "LSP popups enabled");
            }
            return;
        case GLFW_KEY_F12:
            if (lsp.active) LspRestart();
            return;
    }
}

void EditorLspCharHandler(unsigned int c) {
    if (preventUiReopen) {
        return;
    }
    lastTypingTime = window.time;
    if (c == '.' || c == '>' || c == ':' || c == '(' || c == '[') {
        LspRequestCompletion(cursorLine, cursorCol);
    }
}