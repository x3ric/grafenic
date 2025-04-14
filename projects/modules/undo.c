void RecalculateWrappedLines();
void AdjustScrollToCursor();

// Structure to store a copy of the entire document
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

// Undo node structure
typedef struct UndoNode {
    DocumentState state;        // Document state
    double timestamp;           // Timestamp for grouping
    struct UndoNode* next;      // Next node in stack
} UndoNode;

// Global stacks
static UndoNode* undoStack = NULL;
static UndoNode* redoStack = NULL;
static int undoLevels = 0;
static int maxUndoLevels = 100;
static bool isUndoRedoing = false;
static double undoGroupThreshold = 0.5;

// Deep copy a single line
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

// Free a single line
static void FreeLine(Line* line) {
    if (!line || !line->text) return;
    free(line->text);
    line->text = NULL;
    line->length = 0;
}

// Create a complete copy of document state
static DocumentState* CreateDocumentState() {
    DocumentState* state = malloc(sizeof(DocumentState));
    if (!state) return NULL;
    
    // Initialize state
    state->numLines = numLines;
    state->cursorLine = cursorLine;
    state->cursorCol = cursorCol;
    state->selStartLine = selStartLine;
    state->selStartCol = selStartCol;
    state->selEndLine = selEndLine;
    state->selEndCol = selEndCol;
    state->isSelecting = isSelecting;
    state->isFileDirty = isFileDirty;
    
    // Allocate and copy all lines
    state->lines = malloc(numLines * sizeof(Line));
    if (!state->lines) {
        free(state);
        return NULL;
    }
    
    // Copy each line
    for (int i = 0; i < numLines; i++) {
        state->lines[i] = CopyLine(&lines[i]);
        if (!state->lines[i].text) {
            // Cleanup on failure
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

// Free document state
static void FreeDocumentState(DocumentState* state) {
    if (!state) return;
    
    if (state->lines) {
        for (int i = 0; i < state->numLines; i++) {
            FreeLine(&state->lines[i]);
        }
        free(state->lines);
    }
    
    free(state);
}

// Create an undo node with current document state
static UndoNode* CreateUndoNode() {
    UndoNode* node = malloc(sizeof(UndoNode));
    if (!node) return NULL;
    
    // Initialize node
    node->next = NULL;
    node->timestamp = window.time;
    
    // Create document state
    DocumentState* state = CreateDocumentState();
    if (!state) {
        free(node);
        return NULL;
    }
    
    // Move state into node
    memcpy(&node->state, state, sizeof(DocumentState));
    free(state); // Only free the container, not the data
    
    return node;
}

// Free an undo node
static void FreeUndoNode(UndoNode* node) {
    if (!node) return;
    
    FreeDocumentState(&node->state);
    free(node);
}

// Push a node onto a stack
static void PushNode(UndoNode** stack, UndoNode* node) {
    if (!stack || !node) return;
    
    node->next = *stack;
    *stack = node;
    
    if (stack == &undoStack) {
        undoLevels++;
        
        // Trim the stack if it gets too large
        if (undoLevels > maxUndoLevels) {
            UndoNode* current = *stack;
            UndoNode* prev = NULL;
            
            // Find the last node
            while (current && current->next) {
                prev = current;
                current = current->next;
            }
            
            // Remove it
            if (prev && current) {
                prev->next = NULL;
                FreeUndoNode(current);
                undoLevels--;
            }
        }
    }
}

// Pop a node from a stack
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

// Restore document state from an undo node
static void RestoreDocumentState(DocumentState* state) {
    if (!state) return;
    
    // Clear current document
    for (int i = 0; i < numLines; i++) {
        if (lines[i].text) {
            free(lines[i].text);
            lines[i].text = NULL;
        }
    }
    
    // Set new line count
    numLines = state->numLines;
    
    // Copy all lines
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
            InitLine(i); // Initialize with empty line
            continue;
        }
        
        memcpy(lines[i].text, state->lines[i].text, state->lines[i].length);
        lines[i].text[state->lines[i].length] = '\0';
    }
    
    // Restore editor state
    cursorLine = state->cursorLine;
    cursorCol = state->cursorCol;
    selStartLine = state->selStartLine;
    selStartCol = state->selStartCol;
    selEndLine = state->selEndLine;
    selEndCol = state->selEndCol;
    isSelecting = state->isSelecting;
    isFileDirty = state->isFileDirty;
    
    // Bounds checking
    if (cursorLine < 0) cursorLine = 0;
    if (cursorLine >= numLines) cursorLine = numLines > 0 ? numLines - 1 : 0;
    if (cursorCol < 0) cursorCol = 0;
    if (cursorLine < numLines && cursorCol > lines[cursorLine].length) {
        cursorCol = lines[cursorLine].length;
    }
}

// Clear all undo/redo history
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

// Initialize the undo system
void UndoSystemInit() {
    // Clear any existing undo history
    ClearUndoHistory();
    undoLevels = 0;
    isUndoRedoing = false;
    
    // Create initial state
    if (numLines > 0) {
        UndoNode* initialState = CreateUndoNode();
        if (initialState) {
            initialState->state.isFileDirty = false;
            PushNode(&undoStack, initialState);
        }
    }
}

// Save current state for undo
void SaveUndoState(int firstLine, int linesToSave) {
    if (isUndoRedoing) return;
    
    // Check if we should group with previous undo
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
        // Update timestamp to continue grouping
        undoStack->timestamp = window.time;
    } else {
        // Create a new undo node with full document state
        UndoNode* node = CreateUndoNode();
        if (!node) {
            fprintf(stderr, "Warning: Failed to create undo node\n");
            return;
        }
        
        // Add to undo stack
        PushNode(&undoStack, node);
        
        // Clear redo stack since we've made a new change
        while (redoStack) {
            UndoNode* redo = PopNode(&redoStack);
            FreeUndoNode(redo);
        }
    }
}

// Perform undo operation
bool Undo() {
    if (!undoStack || undoStack->next == NULL) {
        snprintf(statusMsg, sizeof(statusMsg), "Nothing to undo");
        return false;
    }
    
    if (isUndoRedoing) return false;
    
    isUndoRedoing = true;
    
    // Save current state for redo
    UndoNode* redoNode = CreateUndoNode();
    if (!redoNode) {
        isUndoRedoing = false;
        snprintf(statusMsg, sizeof(statusMsg), "Undo failed: Could not save current state");
        return false;
    }
    
    // Push to redo stack
    PushNode(&redoStack, redoNode);
    
    // Get previous state
    UndoNode* undoNode = PopNode(&undoStack);
    if (!undoNode) {
        // This shouldn't happen since we checked undoStack above
        isUndoRedoing = false;
        snprintf(statusMsg, sizeof(statusMsg), "Undo failed: No previous state available");
        return false;
    }
    
    // Restore document to previous state
    RestoreDocumentState(&undoNode->state);
    
    // Update display
    if (wordWrap) RecalculateWrappedLines();
    AdjustScrollToCursor();
    
    // Clean up
    FreeUndoNode(undoNode);
    isUndoRedoing = false;
    
    snprintf(statusMsg, sizeof(statusMsg), "Undo");
    return true;
}

// Perform redo operation
bool Redo() {
    if (!redoStack) {
        snprintf(statusMsg, sizeof(statusMsg), "Nothing to redo");
        return false;
    }
    
    if (isUndoRedoing) return false;
    
    isUndoRedoing = true;
    
    // Save current state for undo
    UndoNode* undoNode = CreateUndoNode();
    if (!undoNode) {
        isUndoRedoing = false;
        snprintf(statusMsg, sizeof(statusMsg), "Redo failed: Could not save current state");
        return false;
    }
    
    // Push to undo stack
    PushNode(&undoStack, undoNode);
    
    // Get next state
    UndoNode* redoNode = PopNode(&redoStack);
    if (!redoNode) {
        // This shouldn't happen since we checked redoStack above
        isUndoRedoing = false;
        snprintf(statusMsg, sizeof(statusMsg), "Redo failed: No next state available");
        return false;
    }
    
    // Restore document to next state
    RestoreDocumentState(&redoNode->state);
    
    // Update display
    if (wordWrap) RecalculateWrappedLines();
    AdjustScrollToCursor();
    
    // Clean up
    FreeUndoNode(redoNode);
    isUndoRedoing = false;
    
    snprintf(statusMsg, sizeof(statusMsg), "Redo");
    return true;
}