void RecalculateWrappedLines();
void AdjustScrollToCursor();

typedef struct DocumentState {
    int numLines;               // Total number of lines in document
    Line* lines;                // Array of line copies
    int cursorLine;             // Cursor position - line
    int cursorCol;              // Cursor position - column
    int selStartLine;           // Selection start - line
    int selStartCol;            // Selection start - column
    int selEndLine;             // Selection end - line
    int selEndCol;              // Selection end - column
    bool isSelecting;           // Selection state
    bool isFileDirty;           // Document modified state
} DocumentState;

typedef struct UndoNode {
    DocumentState state;        // Document state
    double timestamp;           // Timestamp for grouping
    struct UndoNode* next;      // Next node in stack
} UndoNode;

static UndoNode* undoStack = NULL;
static UndoNode* redoStack = NULL;
static int undoLevels = 0;
static int maxUndoLevels = 100;
static bool isUndoRedoing = false;
static double undoGroupThreshold = 0.5;

static Line CopyLine(Line* src) {
    Line dst = {NULL, 0};
    if (!src || !src->text) return dst;
    dst.length = src->length;
    dst.text = malloc(src->length + 1);
    if (!dst.text) return dst;
    memcpy(dst.text, src->text, src->length);
    dst.text[dst.length] = '\0';
    return dst;
}

static void FreeLine(Line* line) {
    if (!line || !line->text) return;
    free(line->text);
    line->text = NULL;
    line->length = 0;
}

static DocumentState* CreateDocumentState() {
    DocumentState* state = malloc(sizeof(DocumentState));
    if (!state) return NULL;
    state->numLines = numLines;
    state->cursorLine = cursorLine;
    state->cursorCol = cursorCol;
    state->selStartLine = selStartLine;
    state->selStartCol = selStartCol;
    state->selEndLine = selEndLine;
    state->selEndCol = selEndCol;
    state->isSelecting = isSelecting;
    state->isFileDirty = isFileDirty;
    state->lines = malloc(numLines * sizeof(Line));
    if (!state->lines) {
        free(state);
        return NULL;
    }
    for (int i = 0; i < numLines; i++) {
        state->lines[i] = CopyLine(&lines[i]);
        if (!state->lines[i].text) {
            for (int j = 0; j < i; j++) {
                FreeLine(&state->lines[j]);
            }
            free(state->lines);
            free(state);
            return NULL;
        }
    }
    return state;
}

static UndoNode* CreateUndoNode() {
    UndoNode* node = malloc(sizeof(UndoNode));
    if (!node) return NULL;
    node->next = NULL;
    node->timestamp = window.time;
    node->state.numLines = numLines;
    node->state.cursorLine = cursorLine;
    node->state.cursorCol = cursorCol;
    node->state.selStartLine = selStartLine;
    node->state.selStartCol = selStartCol;
    node->state.selEndLine = selEndLine;
    node->state.selEndCol = selEndCol;
    node->state.isSelecting = isSelecting;
    node->state.isFileDirty = isFileDirty;
    node->state.lines = malloc(numLines * sizeof(Line));
    if (!node->state.lines) {
        free(node);
        return NULL;
    }
    for (int i = 0; i < numLines; i++) {
        node->state.lines[i] = CopyLine(&lines[i]);
        if (!node->state.lines[i].text) {
            for (int j = 0; j < i; j++) FreeLine(&node->state.lines[j]);
            free(node->state.lines);
            free(node);
            return NULL;
        }
    }
    return node;
}

static void FreeDocumentState(DocumentState* state) {
    if (!state) return;
    if (state->lines) {
        for (int i = 0; i < state->numLines; i++) {
            FreeLine(&state->lines[i]);
        }
        free(state->lines);
    }
}

static void FreeUndoNode(UndoNode* node) {
    if (!node) return;
    FreeDocumentState(&node->state);
    free(node);
}

static void PushNode(UndoNode** stack, UndoNode* node) {
    if (!stack || !node) return;
    node->next = *stack;
    *stack = node;
    if (stack == &undoStack) {
        undoLevels++;
        if (undoLevels > maxUndoLevels) {
            UndoNode* current = *stack;
            UndoNode* prev = NULL;
            while (current && current->next) {
                prev = current;
                current = current->next;
            }
            if (prev && current) {
                prev->next = NULL;
                FreeUndoNode(current);
                undoLevels--;
            }
        }
    }
}

static UndoNode* PopNode(UndoNode** stack) {
    if (!stack || !*stack) return NULL;
    UndoNode* node = *stack;
    *stack = node->next;
    node->next = NULL;
    if (stack == &undoStack) {
        undoLevels = (undoLevels > 0) ? undoLevels - 1 : 0;
    }
    return node;
}

static void RestoreDocumentState(DocumentState* state) {
    if (!state) return;
    for (int i = 0; i < numLines; i++) {
        if (lines[i].text) {
            free(lines[i].text);
            lines[i].text = NULL;
        }
    }
    numLines = state->numLines;
    for (int i = 0; i < numLines; i++) {
        if (i >= MAXTEXT) {
            fprintf(stderr, "Warning: Maximum line count exceeded during undo\n");
            numLines = MAXTEXT;
            break;
        }
        lines[i].length = state->lines[i].length;
        lines[i].text = malloc(state->lines[i].length + 1);
        if (!lines[i].text) {
            fprintf(stderr, "Error: Failed to allocate memory during undo\n");
            InitLine(i);
            continue;
        }
        memcpy(lines[i].text, state->lines[i].text, state->lines[i].length);
        lines[i].text[state->lines[i].length] = '\0';
    }
    cursorLine = state->cursorLine;
    cursorCol = state->cursorCol;
    selStartLine = state->selStartLine;
    selStartCol = state->selStartCol;
    selEndLine = state->selEndLine;
    selEndCol = state->selEndCol;
    isSelecting = state->isSelecting;
    isFileDirty = state->isFileDirty;
    if (cursorLine < 0) cursorLine = 0;
    if (cursorLine >= numLines) cursorLine = numLines > 0 ? numLines - 1 : 0;
    if (cursorCol < 0) cursorCol = 0;
    if (cursorLine < numLines && cursorCol > lines[cursorLine].length) {
        cursorCol = lines[cursorLine].length;
    }
}

void ClearUndoHistory() {
    while (undoStack) {
        UndoNode* node = PopNode(&undoStack);
        FreeUndoNode(node);
    }
    while (redoStack) {
        UndoNode* node = PopNode(&redoStack);
        FreeUndoNode(node);
    }
    undoLevels = 0;
}

void UndoSystemInit() {
    ClearUndoHistory();
    undoLevels = 0;
    isUndoRedoing = false;
    if (numLines > 0) {
        UndoNode* initialState = CreateUndoNode();
        if (initialState) {
            initialState->state.isFileDirty = false;
            PushNode(&undoStack, initialState);
        }
    }
}

void SaveUndoState(int firstLine, int linesToSave) {
    if (isUndoRedoing) return;
    bool shouldGroup = false;
    if (undoStack) {
        double elapsed = window.time - undoStack->timestamp;
        if (elapsed < undoGroupThreshold && 
            cursorLine == undoStack->state.cursorLine &&
            abs(cursorCol - undoStack->state.cursorCol) <= 1 &&
            linesToSave == 1) {
            shouldGroup = true;
        }
    }
    if (shouldGroup) {
        undoStack->timestamp = window.time;
    } else {
        UndoNode* node = CreateUndoNode();
        if (!node) {
            fprintf(stderr, "Warning: Failed to create undo node\n");
            return;
        }
        PushNode(&undoStack, node);
        while (redoStack) {
            UndoNode* redo = PopNode(&redoStack);
            FreeUndoNode(redo);
        }
    }
}

bool Undo() {
    if (!undoStack) return false;
    isUndoRedoing = true;
    UndoNode* redoState = CreateUndoNode();
    if (redoState) PushNode(&redoStack, redoState);
    UndoNode* undoState = PopNode(&undoStack);
    if (undoState) {
        RestoreDocumentState(&undoState->state);
        FreeUndoNode(undoState);
    }
    isUndoRedoing = false;
    RecalculateWrappedLines();
    AdjustScrollToCursor();
    return true;
}

bool Redo() {
    if (!redoStack) return false;
    isUndoRedoing = true;
    UndoNode* undoState = CreateUndoNode();
    if (undoState) PushNode(&undoStack, undoState);
    UndoNode* redoState = PopNode(&redoStack);
    if (redoState) {
        RestoreDocumentState(&redoState->state);
        FreeUndoNode(redoState);
    }
    isUndoRedoing = false;
    RecalculateWrappedLines();
    AdjustScrollToCursor();
    return true;
}