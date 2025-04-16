static char* clipboardBuffer=NULL;
bool isInPasteOperation = false;

void FreeClipboardBuffer(){
    if(clipboardBuffer){free(clipboardBuffer);clipboardBuffer=NULL;}
}

void SetClipboardBuffer(const char* text){
    FreeClipboardBuffer();
    if(text)clipboardBuffer=strdup(text);
}

bool IsClipboardOperationSafe(size_t size){
    if(size>MAXTEXT*MAXTEXT){
        snprintf(statusMsg,sizeof(statusMsg),"Clipboard content too large");
        return false;
    }
    void* test=malloc(size);
    if(!test){
        snprintf(statusMsg,sizeof(statusMsg),"Not enough memory for clipboard operation");
        return false;
    }
    free(test);
    return true;
}

void PasteClipboard(){
    if(!clipboardBuffer||!clipboardBuffer[0]){
        snprintf(statusMsg,sizeof(statusMsg),"No clipboard content");
        return;
    }
    size_t contentSize=strlen(clipboardBuffer)+1;
    if(!IsClipboardOperationSafe(contentSize))return;
    SaveUndoPaste();
    isInPasteOperation = true;
    int originalCursorLine=cursorLine;
    int originalCursorCol=cursorCol;
    if(IsSelValid())DeleteSelection();
    char* bufferCopy=strdup(clipboardBuffer);
    if(!bufferCopy){
        snprintf(statusMsg,sizeof(statusMsg),"Failed to allocate memory for paste");
        return;
    }
    char* line=bufferCopy;
    int totalCharsPasted=0;
    bool success=true;
    int lastLineIndex=0;
    while(line&&*line&&numLines<MAXTEXT&&success){
        char* nextLine=strchr(line,'\n');
        size_t lineLen;
        if(nextLine){
            lineLen=nextLine-line;
            *nextLine='\0';
        }else{
            lineLen=strlen(line);
        }
        if(lineLen>MAXTEXT-1){
            lineLen=MAXTEXT-1;
            line[lineLen]='\0';
        }
        if(lines[cursorLine].length+lineLen>=MAXTEXT){
            if(numLines>=MAXTEXT-1){
                snprintf(statusMsg,sizeof(statusMsg),"Maximum lines reached");
                success=false;
                break;
            }
            InsertNewline();
            if(cursorLine>=numLines||!lines[cursorLine].text){
                success=false;
                break;
            }
        }
        if(!lines[cursorLine].text)InitLine(cursorLine);
        if(!lines[cursorLine].text){
            success=false;
            break;
        }
        if(cursorCol<lines[cursorLine].length){
            memmove(&lines[cursorLine].text[cursorCol+lineLen],
                    &lines[cursorLine].text[cursorCol],
                    lines[cursorLine].length-cursorCol+1);
        }
        memcpy(&lines[cursorLine].text[cursorCol],line,lineLen);
        lines[cursorLine].length+=lineLen;
        lines[cursorLine].text[lines[cursorLine].length]='\0';
        cursorCol+=lineLen;
        totalCharsPasted+=lineLen;
        lastLineIndex=cursorLine;
        if(nextLine){
            line=nextLine+1;
            if(numLines<MAXTEXT-1){
                InsertNewline();
                if(cursorLine>=numLines||!lines[cursorLine].text){
                    success=false;
                    break;
                }
            }else{
                break;
            }
        }else{
            break;
        }
    }
    free(bufferCopy);
    isInPasteOperation = false;
    if(success&&totalCharsPasted>0){
        isFileDirty=true;
        if(wordWrap)RecalculateWrappedLines();
        selStartLine=originalCursorLine;
        selStartCol=originalCursorCol;
        selEndLine=lastLineIndex;
        selEndCol=cursorCol;
        isSelecting=true;
        snprintf(statusMsg,sizeof(statusMsg),"Pasted %d characters",totalCharsPasted);
    }else if(totalCharsPasted>0){
        Undo();
        snprintf(statusMsg,sizeof(statusMsg),"Paste operation failed, changes reverted");
    }else{
        snprintf(statusMsg,sizeof(statusMsg),"Nothing pasted");
    }
}

void CopySelection(){
    if(!IsSelValid()){
        snprintf(statusMsg,sizeof(statusMsg),"No selection to copy");
        return;
    }
    int startLine=selStartLine,startCol=selStartCol;
    int endLine=selEndLine,endCol=selEndCol;
    if(startLine>endLine||(startLine==endLine&&startCol>endCol)){
        int tempLine=startLine,tempCol=startCol;
        startLine=endLine;startCol=endCol;
        endLine=tempLine;endCol=tempCol;
    }
    int totalLength=0;
    for(int line=startLine;line<=endLine;line++){
        if(line>=numLines||!lines[line].text){
            snprintf(statusMsg,sizeof(statusMsg),"Invalid selection");
            return;
        }
        int lineStart=(line==startLine)?startCol:0;
        int lineEnd=(line==endLine)?endCol:lines[line].length;
        if(lineStart>lines[line].length)lineStart=lines[line].length;
        if(lineEnd>lines[line].length)lineEnd=lines[line].length;
        totalLength+=(lineEnd-lineStart)+(line<endLine?1:0);
    }
    if(totalLength<=0){
        snprintf(statusMsg,sizeof(statusMsg),"Nothing to copy");
        return;
    }
    if(!IsClipboardOperationSafe(totalLength+1))return;
    char* copiedText=malloc(totalLength+1);
    if(!copiedText){
        snprintf(statusMsg,sizeof(statusMsg),"Failed to allocate memory for clipboard");
        return;
    }
    int pos=0;
    for(int line=startLine;line<=endLine;line++){
        int lineStart=(line==startLine)?startCol:0;
        int lineEnd=(line==endLine)?endCol:lines[line].length;
        if(lineStart>lines[line].length)lineStart=lines[line].length;
        if(lineEnd>lines[line].length)lineEnd=lines[line].length;
        int lineLength=lineEnd-lineStart;
        if(lineLength>0){
            memcpy(copiedText+pos,lines[line].text+lineStart,lineLength);
            pos+=lineLength;
        }
        if(line<endLine)copiedText[pos++]='\n';
    }
    copiedText[pos]='\0';
    FILE* pipe=popen("xclip -selection clipboard","w");
    if(pipe){
        fputs(copiedText,pipe);
        pclose(pipe);
        SetClipboardBuffer(copiedText);
        snprintf(statusMsg,sizeof(statusMsg),"Copied %d characters",pos);
    }else{
        SetClipboardBuffer(copiedText);
        snprintf(statusMsg,sizeof(statusMsg),"Copied %d characters (system clipboard unavailable)",pos);
    }
    free(copiedText);
}

void CutSelection(){
    if(!IsSelValid()){
        snprintf(statusMsg,sizeof(statusMsg),"No selection to cut");
        return;
    }
    SaveUndoCut();
    CopySelection();
    DeleteSelection();
    isFileDirty=true;
    if(wordWrap)RecalculateWrappedLines();
}