void RecalculateWrappedLines(void);
void AdjustScrollToCursor(void);
void InitLine(int idx);
bool IsSelValid(void);
static void ValidateCursor(void);
static void ValidateSelection(void);

typedef struct {
    int numLines;
    Line* lines;
    int cursorLine, cursorCol;
    int selStartLine, selStartCol;
    int selEndLine, selEndCol;
    bool isSelecting, isFileDirty;
    int operationType;
} DocumentState;

#define OP_OTHER 0
#define OP_CHAR_INSERTION 1
#define OP_CHAR_DELETION 2
#define OP_LINE_BREAK 3
#define OP_PASTE 4
#define OP_CUT 5
#define OP_REPLACE 6

typedef struct UndoNode {
    DocumentState state;
    double timestamp;
    struct UndoNode* next;
    bool isGrouped;
    int groupId;
} UndoNode;

static UndoNode *undoStack=NULL,*redoStack=NULL;
static int undoLevels=0,maxUndoLevels=500;
static bool isUndoRedoing=false;
static bool undoStateChanged=true;
static double undoGroupThreshold=1.0;
static int currentGroupId=0;
static int lastUndoOperation=OP_OTHER;

static Line CopyLine(const Line* src){
    Line dst={NULL,0};
    if(!src)return dst;
    if(src->text){
        dst.length=src->length;
        dst.text=malloc(dst.length+1);
        if(dst.text)memcpy(dst.text,src->text,dst.length+1);
        else dst.length=0;
    }
    return dst;
}

static void FreeLine(Line* line){
    if(!line)return;
    if(line->text){free(line->text);line->text=NULL;}
    line->length=0;
}

static UndoNode* CreateUndoNode(bool grouped, int opType){
    UndoNode* node=malloc(sizeof(UndoNode));
    if(!node){snprintf(statusMsg,sizeof(statusMsg),"Undo: Memory allocation failed");return NULL;}
    memset(node,0,sizeof(UndoNode));
    node->timestamp=window.time;
    node->isGrouped=grouped;
    node->groupId=grouped?currentGroupId:++currentGroupId;
    node->state.cursorLine=cursorLine;
    node->state.cursorCol=cursorCol;
    node->state.selStartLine=selStartLine;
    node->state.selStartCol=selStartCol;
    node->state.selEndLine=selEndLine;
    node->state.selEndCol=selEndCol;
    node->state.isSelecting=isSelecting;
    node->state.isFileDirty=isFileDirty;
    node->state.operationType=opType;
    if(numLines>0){
        node->state.lines=calloc(numLines,sizeof(Line));
        if(!node->state.lines){free(node);snprintf(statusMsg,sizeof(statusMsg),"Undo: Line allocation failed");return NULL;}
        node->state.numLines=numLines;
        for(int i=0;i<numLines;i++){
            if(lines[i].text){
                node->state.lines[i].text=strdup(lines[i].text);
                node->state.lines[i].length=lines[i].length;
                if(!node->state.lines[i].text){
                    for(int j=0;j<i;j++)free(node->state.lines[j].text);
                    free(node->state.lines);free(node);
                    snprintf(statusMsg,sizeof(statusMsg),"Undo: Text copy failed");
                    return NULL;
                }
            }
        }
    }
    return node;
}

static void FreeUndoNode(UndoNode* node){
    if(!node)return;
    if(node->state.lines){
        for(int i=0;i<node->state.numLines;i++)FreeLine(&node->state.lines[i]);
        free(node->state.lines);
    }
    free(node);
}

static void PurgeUndoStack(UndoNode** stack){
    if(!stack)return;
    UndoNode* current;
    while(*stack){
        current=*stack;
        *stack=current->next;
        FreeUndoNode(current);
    }
}

static void PushNode(UndoNode** stack,UndoNode* node){
    if(!stack||!node)return;
    node->next=*stack;
    *stack=node;
    if(stack==&undoStack){
        undoLevels++;
        if(undoLevels>maxUndoLevels){
            UndoNode* current=*stack;
            UndoNode* prev=NULL;
            while(current&&current->next){
                prev=current;
                current=current->next;
            }
            if(prev&&current){
                prev->next=NULL;
                FreeUndoNode(current);
                undoLevels--;
            }
        }
    }
}

static UndoNode* PopNode(UndoNode** stack){
    if(!stack||!*stack)return NULL;
    UndoNode* node=*stack;
    *stack=node->next;
    node->next=NULL;
    if(stack==&undoStack&&undoLevels>0)undoLevels--;
    return node;
}

static bool CanGroupOperations(UndoNode* prevNode,int opType,int line,int col){
    if(!prevNode)return false;
    if(prevNode->state.operationType!=opType)return false;
    if(prevNode->state.cursorLine!=line)return false;
    double elapsed=window.time-prevNode->timestamp;
    if(elapsed>undoGroupThreshold)return false;
    switch(opType){
        case OP_CHAR_INSERTION:case OP_CHAR_DELETION:
            return abs(col-prevNode->state.cursorCol)<=2;
        default:return false;
    }
}

static void ValidateCursor(void){
    if(numLines<=0){cursorLine=cursorCol=0;return;}
    cursorLine=(cursorLine<0)?0:(cursorLine>=numLines)?numLines-1:cursorLine;
    if(!lines[cursorLine].text)InitLine(cursorLine);
    cursorCol=(cursorCol<0)?0:(cursorCol>lines[cursorLine].length)?lines[cursorLine].length:cursorCol;
}

static void ValidateSelection(void){
    if(!isSelecting){selStartLine=selStartCol=selEndLine=selEndCol=-1;return;}
    if(numLines<=0){isSelecting=false;selStartLine=selStartCol=selEndLine=selEndCol=-1;return;}
    selStartLine=(selStartLine<0)?0:(selStartLine>=numLines)?numLines-1:selStartLine;
    selEndLine=(selEndLine<0)?0:(selEndLine>=numLines)?numLines-1:selEndLine;
    if(selStartLine>=0&&selStartLine<numLines){
        if(!lines[selStartLine].text)InitLine(selStartLine);
        selStartCol=(selStartCol<0)?0:(selStartCol>lines[selStartLine].length)?lines[selStartLine].length:selStartCol;
    }else selStartCol=0;
    if(selEndLine>=0&&selEndLine<numLines){
        if(!lines[selEndLine].text)InitLine(selEndLine);
        selEndCol=(selEndCol<0)?0:(selEndCol>lines[selEndLine].length)?lines[selEndLine].length:selEndCol;
    }else selEndCol=0;
    if(selStartLine==selEndLine&&selStartCol==selEndCol){
        isSelecting=false;
        selStartLine=selStartCol=selEndLine=selEndCol=-1;
    }
    if(isSelecting&&(selStartLine>selEndLine||(selStartLine==selEndLine&&selStartCol>selEndCol))){
        int tempLine=selStartLine,tempCol=selStartCol;
        selStartLine=selEndLine;selStartCol=selEndCol;
        selEndLine=tempLine;selEndCol=tempCol;
    }
}

static bool RestoreDocumentState(const DocumentState* state){
    if(!state||state->numLines<=0){
        snprintf(statusMsg,sizeof(statusMsg),"Invalid undo state");
        return false;
    }
    for(int i=0;i<numLines;i++)FreeLine(&lines[i]);
    numLines=(state->numLines>MAXTEXT)?MAXTEXT:state->numLines;
    bool fullyRestored=true;
    for(int i=0;i<numLines;i++){
        if(state->lines[i].text){
            lines[i].text=strdup(state->lines[i].text);
            lines[i].length=state->lines[i].length;
            if(!lines[i].text){InitLine(i);fullyRestored=false;}
        }else InitLine(i);
    }
    isFileDirty=state->isFileDirty;
    if(!fullyRestored)snprintf(statusMsg,sizeof(statusMsg),"Partial undo state restoration");
    return fullyRestored;
}

void ClearUndoHistory(){
    PurgeUndoStack(&undoStack);
    PurgeUndoStack(&redoStack);
    undoLevels=0;
    undoStateChanged=true;
    currentGroupId=0;
}

void UndoSystemInit(){
    ClearUndoHistory();
    isUndoRedoing=false;
    if(numLines>0){
        UndoNode* initialState=CreateUndoNode(false,OP_OTHER);
        if(initialState){
            initialState->state.isFileDirty=false;
            PushNode(&undoStack,initialState);
        }
    }
}

void ForceUndoStateBreak(){
    undoStateChanged=true;
    currentGroupId++;
}

void SaveUndoState(int opType,int firstLine,int lastLine){
    if(isUndoRedoing)return;
    if(!undoStateChanged)PurgeUndoStack(&redoStack);
    bool shouldGroup=!undoStateChanged&&CanGroupOperations(undoStack,opType,firstLine,cursorCol);
    UndoNode* node=CreateUndoNode(shouldGroup,opType);
    if(!node)return;
    if(shouldGroup&&undoStack){
        undoStack->timestamp=window.time;
        UndoNode* oldTop=undoStack;
        node->next=oldTop->next;
        undoStack=node;
        FreeUndoNode(oldTop);
    }else PushNode(&undoStack,node);
    undoStateChanged=false;
}

void SaveUndoCharInsert(){
    if(isUndoRedoing)return;
    SaveUndoState(OP_CHAR_INSERTION,cursorLine,cursorLine);
}

void SaveUndoCharDelete(){
    if(isUndoRedoing)return;
    bool atLineEnd=(cursorCol>=lines[cursorLine].length);
    if(atLineEnd&&cursorLine<numLines-1)
        SaveUndoState(OP_CHAR_DELETION,cursorLine,cursorLine+1);
    else
        SaveUndoState(OP_CHAR_DELETION,cursorLine,cursorLine);
}

void SaveDeleteWordForward(){
    if(isUndoRedoing)return;
    ForceUndoStateBreak();
    if(cursorCol>=lines[cursorLine].length&&cursorLine<numLines-1)
        SaveUndoState(OP_CHAR_DELETION,cursorLine,cursorLine+1);
    else
        SaveUndoState(OP_CHAR_DELETION,cursorLine,cursorLine);
}

void SaveUndoLineBreak(){
    if(isUndoRedoing)return;
    ForceUndoStateBreak();
    SaveUndoState(OP_LINE_BREAK,cursorLine,cursorLine);
}

void SaveUndoPaste(){
    if(isUndoRedoing)return;
    ForceUndoStateBreak();
    bool hasSelection=IsSelValid();
    if(hasSelection){
        int startLine=selStartLine,startCol=selStartCol;
        int endLine=selEndLine,endCol=selEndCol;
        if(startLine>endLine||(startLine==endLine&&startCol>endCol)){
            int tempLine=startLine,tempCol=startCol;
            startLine=endLine;startCol=endCol;
            endLine=tempLine;endCol=tempCol;
        }
        SaveUndoState(OP_PASTE,startLine,endLine);
    }else{
        SaveUndoState(OP_PASTE,cursorLine,cursorLine);
    }
}

void SaveUndoCut(){
    if(isUndoRedoing||!IsSelValid())return;
    ForceUndoStateBreak();
    int startLine=selStartLine,startCol=selStartCol;
    int endLine=selEndLine,endCol=selEndCol;
    if(startLine>endLine||(startLine==endLine&&startCol>endCol)){
        int tempLine=startLine,tempCol=startCol;
        startLine=endLine;startCol=endCol;
        endLine=tempLine;endCol=tempCol;
    }
    SaveUndoState(OP_CUT,startLine,endLine);
}

void SaveUndoReplace(){
    if(isUndoRedoing)return;
    ForceUndoStateBreak();
    if(IsSelValid()){
        int startLine=selStartLine,startCol=selStartCol;
        int endLine=selEndLine,endCol=selEndCol;
        if(startLine>endLine||(startLine==endLine&&startCol>endCol)){
            int tempLine=startLine,tempCol=startCol;
            startLine=endLine;startCol=endCol;
            endLine=tempLine;endCol=tempCol;
        }
        SaveUndoState(OP_REPLACE,startLine,endLine);
    }else{
        SaveUndoState(OP_REPLACE,cursorLine,cursorLine);
    }
}

bool Undo(){
    if(!undoStack||!undoStack->next){snprintf(statusMsg,sizeof(statusMsg),"Nothing to undo");return false;}
    isUndoRedoing=true;
    UndoNode* saveState=CreateUndoNode(false,OP_OTHER);
    if(saveState)PushNode(&redoStack,saveState);
    UndoNode* currentState=PopNode(&undoStack);
    if(!currentState){isUndoRedoing=false;return false;}
    lastUndoOperation=currentState->state.operationType;
    bool success=RestoreDocumentState(&currentState->state);
    switch(lastUndoOperation){
        case OP_PASTE:case OP_CUT:
            cursorLine=currentState->state.cursorLine;
            cursorCol=currentState->state.cursorCol;
            selStartLine=currentState->state.selStartLine;
            selStartCol=currentState->state.selStartCol;
            selEndLine=currentState->state.selEndLine;
            selEndCol=currentState->state.selEndCol;
            isSelecting=currentState->state.isSelecting;
            break;
        case OP_CHAR_INSERTION:
            cursorLine=currentState->state.cursorLine;
            cursorCol=currentState->state.cursorCol;
            break;
        case OP_CHAR_DELETION:
            cursorLine=currentState->state.cursorLine;
            cursorCol=currentState->state.cursorCol;
            break;
        case OP_LINE_BREAK:
            cursorLine=currentState->state.cursorLine;
            cursorCol=currentState->state.cursorCol;
            break;
        default:
            cursorLine=currentState->state.cursorLine;
            cursorCol=currentState->state.cursorCol;
            break;
    }
    ValidateCursor();
    ValidateSelection();
    FreeUndoNode(currentState);
    if(success){
        RecalculateWrappedLines();
        AdjustScrollToCursor();
        if(undoStack&&
           (lastUndoOperation==OP_CHAR_INSERTION||lastUndoOperation==OP_CHAR_DELETION)&&
           undoStack->state.operationType==lastUndoOperation&&
           undoStack->isGrouped){
            Undo();
        }else{
            snprintf(statusMsg,sizeof(statusMsg),"Undo successful");
        }
    }else{
        snprintf(statusMsg,sizeof(statusMsg),"Undo partially failed");
    }
    undoStateChanged=true;
    isUndoRedoing=false;
    return success;
}

bool Redo(){
    if(!redoStack){snprintf(statusMsg,sizeof(statusMsg),"Nothing to redo");return false;}
    isUndoRedoing=true;
    UndoNode* saveState=CreateUndoNode(false,OP_OTHER);
    if(saveState){
        PushNode(&undoStack,saveState);
    }
    UndoNode* redoState=PopNode(&redoStack);
    if(!redoState){isUndoRedoing=false;return false;}
    int opType=redoState->state.operationType;
    bool success=RestoreDocumentState(&redoState->state);
    switch(opType) {
        case OP_CHAR_INSERTION:
            {
                cursorLine=redoState->state.cursorLine;
                cursorCol=redoState->state.cursorCol+1;
                if(lines[cursorLine].text && cursorCol > lines[cursorLine].length && cursorLine < numLines-1) {
                    cursorLine++;
                    cursorCol=0;
                }
            }
            break;
        case OP_CHAR_DELETION:
            cursorLine=redoState->state.cursorLine;
            cursorCol=redoState->state.cursorCol;
            break;
        case OP_LINE_BREAK:
            cursorLine=redoState->state.cursorLine+1;
            cursorCol=0;
            break;
        case OP_PASTE:
            {
                cursorLine=redoState->state.cursorLine;
                cursorCol=redoState->state.cursorCol;
            }
            break;
        case OP_CUT:
            {
                cursorLine=redoState->state.selStartLine;
                cursorCol=redoState->state.selStartCol;
                isSelecting=false;
                selStartLine=selStartCol=selEndLine=selEndCol=-1;
            }
            break;
        default:
            cursorLine=redoState->state.cursorLine;
            cursorCol=redoState->state.cursorCol;
            selStartLine=redoState->state.selStartLine;
            selStartCol=redoState->state.selStartCol;
            selEndLine=redoState->state.selEndLine;
            selEndCol=redoState->state.selEndCol;
            isSelecting=redoState->state.isSelecting;
            break;
    }
    ValidateCursor();
    ValidateSelection();
    FreeUndoNode(redoState);
    if(success){
        RecalculateWrappedLines();
        AdjustScrollToCursor();
        if(redoStack&&(opType==OP_CHAR_INSERTION||opType==OP_CHAR_DELETION)&&
           redoStack->state.operationType==opType&&redoStack->isGrouped){
            Redo();
        }else{
            snprintf(statusMsg,sizeof(statusMsg),"Redo successful");
        }
    }else{
        snprintf(statusMsg,sizeof(statusMsg),"Redo partially failed");
    }
    undoStateChanged=true;
    isUndoRedoing=false;
    return success;
}