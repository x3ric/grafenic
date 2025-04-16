void handleModifiers(int key, int action, int mods) {
    static double keyTimes[GLFW_KEY_LAST+1]={0};
    static bool keyState[GLFW_KEY_LAST+1]={0};
    static double initDelay=0.02, repeatInt=0.02;
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
}

void handleWrappedNavigation(int key, int ctrl, int* targCol) {
    int wl,wc; OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
    switch(key){
        case GLFW_KEY_LEFT:
            if(ctrl){
                if(cursorCol>0){
                    while(cursorCol>0&&isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                    while(cursorCol>0&&!isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                }else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
            }else{
                int oldWl=wl,oldWc=wc;
                if(oldWc>0)oldWc--;
                else if(oldWl>0){oldWl--;oldWc=wrappedLines[oldWl].length;}
                int newLine,newCol;
                WrappedToOriginal(oldWl,oldWc,&newLine,&newCol);
                if(newLine==cursorLine&&newCol==cursorCol&&!(oldWl==0&&oldWc==0)){
                    if(cursorCol>0)cursorCol--;
                    else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
                }else{cursorLine=newLine;cursorCol=newCol;}
            }
            break;
        case GLFW_KEY_RIGHT:
            if(ctrl){
                if(cursorCol<lines[cursorLine].length){
                    while(cursorCol<lines[cursorLine].length&&!isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                }else if(cursorLine<numLines-1){cursorLine++;cursorCol=0;}
            }else{
                if(wc<wrappedLines[wl].length)wc++;
                else if(wl<numWrappedLines-1){wl++;wc=0;}
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
            }
            break;
        case GLFW_KEY_UP:
            if(*targCol<0)*targCol=wc;
            bool wasAtEOL=(wc==wrappedLines[wl].length);
            if(wl>0){
                wl=fmax(0,ctrl?wl-3:wl-1);
                wc=fmin(*targCol,wrappedLines[wl].length);
                bool nowAtEOL=(wc==wrappedLines[wl].length);
                if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;
                else consecutiveEOLMoves=0;
                if(consecutiveEOLMoves>=2){wc=wrappedLines[wl].length;*targCol=INT_MAX;}
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
            }
            break;
        case GLFW_KEY_DOWN:
            if(*targCol<0)*targCol=wc;
            wasAtEOL=(wc==wrappedLines[wl].length);
            if(wl<numWrappedLines-1){
                wl=fmin(numWrappedLines-1,ctrl?wl+3:wl+1);
                wc=fmin(*targCol,wrappedLines[wl].length);
                bool nowAtEOL=(wc==wrappedLines[wl].length);
                if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;
                else consecutiveEOLMoves=0;
                if(consecutiveEOLMoves>=2){wc=wrappedLines[wl].length;*targCol=INT_MAX;}
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
            }
            break;
        case GLFW_KEY_HOME:
            if(ctrl){cursorLine=0;cursorCol=0;scroll.targetY=0;}
            else{wc=0;WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);}
            break;
        case GLFW_KEY_END:
            if(ctrl){cursorLine=numLines-1;cursorCol=lines[cursorLine].length;scroll.targetY=MaxScroll();}
            else{wc=wrappedLines[wl].length;WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);}
            break;
        case GLFW_KEY_PAGE_UP:{
            int visLines=window.screen_height/lineHeight;
            wl=fmax(0,wl-visLines);
            wc=fmin(*targCol>=0?*targCol:wc,wrappedLines[wl].length);
            WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
            scroll.targetY=fmax(0,scroll.targetY-window.screen_height+lineHeight);
            break;
        }
        case GLFW_KEY_PAGE_DOWN:{
            int visLines=window.screen_height/lineHeight;
            wl=fmin(numWrappedLines-1,wl+visLines);
            wc=fmin(*targCol>=0?*targCol:wc,wrappedLines[wl].length);
            WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
            scroll.targetY=fmin(MaxScroll(),scroll.targetY+window.screen_height-lineHeight);
            break;
        }
    }
}

void handleNormalNavigation(int key, int ctrl, int* goalColumn) {
    switch(key){
        case GLFW_KEY_LEFT:
            if(ctrl){
                if(cursorCol>0){
                    while(cursorCol>0&&isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                    while(cursorCol>0&&!isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                }else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
            }else{
                if(cursorCol>0)cursorCol--;
                else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
            }
            nowAtEOL=false;*goalColumn=-1;consecutiveEOLMoves=0;
            break;
        case GLFW_KEY_RIGHT:
            if(ctrl){
                if(cursorCol<lines[cursorLine].length){
                    while(cursorCol<lines[cursorLine].length&&!isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                }else if(cursorLine<numLines-1){cursorLine++;cursorCol=0;}
            }else{
                if(cursorCol<lines[cursorLine].length)cursorCol++;
                else if(cursorLine<numLines-1){cursorLine++;cursorCol=0;}
            }
            *goalColumn=-1;consecutiveEOLMoves=0;
            break;
        case GLFW_KEY_UP:
            if(*goalColumn<0)*goalColumn=cursorCol;
            bool wasAtEOL=(cursorCol==lines[cursorLine].length);
            if(ctrl)cursorLine=MaxInt(cursorLine-3,0);
            else if(cursorLine>0)cursorLine--;
            cursorCol=MinInt(*goalColumn,lines[cursorLine].length);
            bool nowAtEOL=(cursorCol==lines[cursorLine].length);
            if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;
            else consecutiveEOLMoves=0;
            if(consecutiveEOLMoves>=2){cursorCol=lines[cursorLine].length;*goalColumn=INT_MAX;}
            break;
        case GLFW_KEY_DOWN:
            if(*goalColumn<0)*goalColumn=cursorCol;
            wasAtEOL=(cursorCol==lines[cursorLine].length);
            if(ctrl)cursorLine=MinInt(cursorLine+3,numLines-1);
            else if(cursorLine<numLines-1)cursorLine++;
            cursorCol=MinInt(*goalColumn,lines[cursorLine].length);
            nowAtEOL=(cursorCol==lines[cursorLine].length);
            if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;
            else consecutiveEOLMoves=0;
            if(consecutiveEOLMoves>=2){cursorCol=lines[cursorLine].length;*goalColumn=INT_MAX;}
            break;
        case GLFW_KEY_HOME:
            if(ctrl){cursorLine=0;cursorCol=0;scroll.targetY=0;scroll.targetX=0;}
            else{
                int origCol=cursorCol;cursorCol=0;
                if(origCol==0){
                    while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    if(cursorCol==lines[cursorLine].length||cursorCol==origCol)cursorCol=0;
                }
                scroll.targetX=0;
            }
            break;
        case GLFW_KEY_END:
            if(ctrl){cursorLine=numLines-1;cursorCol=lines[cursorLine].length;scroll.targetY=MaxScroll();scroll.targetX=MaxHorizontalScroll();}
            else cursorCol=lines[cursorLine].length;
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
    }
}



void updateCursorAndScroll(int key, int pLine, int pCol, int shift, int ctrl, int action) {
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
        int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
        curY=wl*lineHeight;
        curX=0;
    }else{
        curX=GetCursorXPosition();
        curY=cursorLine*lineHeight;
    }
    float vTop=scroll.currentY,vBot=vTop+visH,buf=lineHeight;
    if(curY<vTop+buf)scroll.targetY=curY-buf;
    else if(curY+lineHeight>vBot-buf)scroll.targetY=curY-visH+lineHeight+buf;
    if(!wordWrap){
        float vLeft=scroll.currentX,vRight=vLeft+visW;
        float hBuf=GetTextSizeCached(font,fontSize,"MM").width;
        if(curX<vLeft+hBuf)scroll.targetX=curX-hBuf;
        else if(curX>vRight-hBuf)scroll.targetX=curX-visW+hBuf;
        scroll.targetX=fmax(0,fmin(MaxHorizontalScroll(),scroll.targetX));
        float cw=GetTextSizeCached(font,fontSize,"M").width;
        scroll.targetX=roundf(scroll.targetX/cw)*cw;
    }else scroll.targetX=0;
    scroll.targetY=fmax(0,fmin(MaxScroll(),scroll.targetY));
    lastVerticalScrollTime=lastHorizontalScrollTime=window.time;
    static int targCol=-1;
    if(key==GLFW_KEY_UP||key==GLFW_KEY_DOWN||key==GLFW_KEY_PAGE_UP||key==GLFW_KEY_PAGE_DOWN){
        if(!(key==GLFW_KEY_UP||key==GLFW_KEY_DOWN)||(!shift&&!ctrl))targCol=-1;
    }else targCol=-1;
    if(wordWrap&&isFileDirty&&(
        key==GLFW_KEY_BACKSPACE||
        key==GLFW_KEY_DELETE||
        key==GLFW_KEY_ENTER||
        key==GLFW_KEY_TAB||
        ((action==GLFW_PRESS||action==GLFW_REPEAT)&&isprint(key))
    )){
        RecalculateWrappedLines();
    }
}

void KeyCallback(GLFWwindow* win,int key,int scan,int action,int mods){
    if(lsp.active)EditorLspKeyHandler(key,action,mods);
    handleModifiers(key,action,mods);
    if(action==GLFW_RELEASE)return;
    bool ctrl=(mods&GLFW_MOD_CONTROL)!=0,shift=(mods&GLFW_MOD_SHIFT)!=0;
    static int targCol=-1;
    static bool hadSelection=false;
    int pLine=cursorLine,pCol=cursorCol;
    bool wasAtEOL,nowAtEOL;
    bool isNavigationKey=(key==GLFW_KEY_LEFT||key==GLFW_KEY_RIGHT||key==GLFW_KEY_UP||key==GLFW_KEY_DOWN||
                         key==GLFW_KEY_HOME||key==GLFW_KEY_END||key==GLFW_KEY_PAGE_UP||key==GLFW_KEY_PAGE_DOWN);
    bool isCursorMoved=false;
    if(shift&&!isSelecting){selStartLine=cursorLine;selStartCol=cursorCol;isSelecting=1;}
    switch(key){
        case GLFW_KEY_ESCAPE:
            if(isSelecting){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_W:
            if(ctrl){wordWrap=!wordWrap;snprintf(statusMsg,sizeof(statusMsg),"Word Wrap: %s",wordWrap?"ON":"OFF");
                if(wordWrap){RecalculateWrappedLines();int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
                    float curY=wl*lineHeight;int visH=window.screen_height-(showStatusBar?statusBarHeight:0)-(showModeline?modeLineHeight:0);
                    if(curY<scroll.currentY||curY>=scroll.currentY+visH-lineHeight)scroll.targetY=fmax(0,curY-visH/2);scroll.targetX=0;}}
            break;
        case GLFW_KEY_X:
            if(ctrl&&IsSelValid())CutSelection();
            break;
        case GLFW_KEY_C:
            if(ctrl){
                if(IsSelValid())CopySelection();
                else{hadSelection=isSelecting;selStartLine=selEndLine=cursorLine;selStartCol=0;selEndCol=lines[cursorLine].length;
                    isSelecting=true;CopySelection();isSelecting=hadSelection;
                    if(!hadSelection){selStartLine=selEndLine=-1;selStartCol=selEndCol=-1;}}
            }
            break;
        case GLFW_KEY_V:
            if(ctrl){if(IsSelValid())DeleteSelection();PasteClipboard();}
            break;
        case GLFW_KEY_Z:
            if(ctrl)Undo();
            break;
        case GLFW_KEY_Y:
            if(ctrl)Redo();
            break;
        case GLFW_KEY_INSERT:
            insertMode=!insertMode;
            break;
        case GLFW_KEY_LEFT:
            if(wordWrap){
                if(ctrl){
                    if(cursorCol>0){
                        while(cursorCol>0&&isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                        while(cursorCol>0&&!isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                    }else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
                }else{
                    int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);int oldWl=wl,oldWc=wc;
                    if(oldWc>0)oldWc--;else if(oldWl>0){oldWl--;oldWc=wrappedLines[oldWl].length;}
                    int newLine,newCol;WrappedToOriginal(oldWl,oldWc,&newLine,&newCol);
                    if(newLine==cursorLine&&newCol==cursorCol&&!(oldWl==0&&oldWc==0)){
                        if(cursorCol>0)cursorCol--;
                        else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
                    }else{cursorLine=newLine;cursorCol=newCol;}
                }
            }else{
                if(ctrl){
                    if(cursorCol>0){
                        while(cursorCol>0&&isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                        while(cursorCol>0&&!isspace((unsigned char)lines[cursorLine].text[cursorCol-1]))cursorCol--;
                    }else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
                }else{
                    if(cursorCol>0)cursorCol--;
                    else if(cursorLine>0){cursorLine--;cursorCol=lines[cursorLine].length;}
                }
                targCol=-1;consecutiveEOLMoves=0;
            }
            break;
        case GLFW_KEY_RIGHT:
            if(wordWrap){
                if(ctrl){
                    if(cursorCol<lines[cursorLine].length){
                        while(cursorCol<lines[cursorLine].length&&!isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                        while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    }else if(cursorLine<numLines-1){cursorLine++;cursorCol=0;}
                }else{
                    int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
                    if(wc<wrappedLines[wl].length)wc++;
                    else if(wl<numWrappedLines-1){wl++;wc=0;}
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
            }else{
                if(ctrl){
                    if(cursorCol<lines[cursorLine].length){
                        while(cursorCol<lines[cursorLine].length&&!isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                        while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                    }else if(cursorLine<numLines-1){cursorLine++;cursorCol=0;}
                }else{
                    if(cursorCol<lines[cursorLine].length)cursorCol++;
                    else if(cursorLine<numLines-1){cursorLine++;cursorCol=0;}
                }
                targCol=-1;consecutiveEOLMoves=0;
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_UP:
            if(wordWrap){
                int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
                if(targCol<0)targCol=wc;
                wasAtEOL=(wc==wrappedLines[wl].length);
                if(wl>0){
                    wl=fmax(0,ctrl?wl-3:wl-1);
                    wc=fmin(targCol,wrappedLines[wl].length);
                    nowAtEOL=(wc==wrappedLines[wl].length);
                    if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;else consecutiveEOLMoves=0;
                    if(consecutiveEOLMoves>=2){wc=wrappedLines[wl].length;targCol=INT_MAX;}
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
            }else{
                if(targCol<0)targCol=cursorCol;
                wasAtEOL=(cursorCol==lines[cursorLine].length);
                if(ctrl)cursorLine=fmax(cursorLine-3,0);
                else if(cursorLine>0)cursorLine--;
                cursorCol=fmin(targCol,lines[cursorLine].length);
                nowAtEOL=(cursorCol==lines[cursorLine].length);
                if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;else consecutiveEOLMoves=0;
                if(consecutiveEOLMoves>=2){cursorCol=lines[cursorLine].length;targCol=INT_MAX;}
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_DOWN:
            if(wordWrap){
                int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
                if(targCol<0)targCol=wc;
                wasAtEOL=(wc==wrappedLines[wl].length);
                if(wl<numWrappedLines-1){
                    wl=fmin(numWrappedLines-1,ctrl?wl+3:wl+1);
                    wc=fmin(targCol,wrappedLines[wl].length);
                    nowAtEOL=(wc==wrappedLines[wl].length);
                    if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;else consecutiveEOLMoves=0;
                    if(consecutiveEOLMoves>=2){wc=wrappedLines[wl].length;targCol=INT_MAX;}
                    WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                }
            }else{
                if(targCol<0)targCol=cursorCol;
                wasAtEOL=(cursorCol==lines[cursorLine].length);
                if(ctrl)cursorLine=fmin(cursorLine+3,numLines-1);
                else if(cursorLine<numLines-1)cursorLine++;
                cursorCol=fmin(targCol,lines[cursorLine].length);
                nowAtEOL=(cursorCol==lines[cursorLine].length);
                if(wasAtEOL&&nowAtEOL)consecutiveEOLMoves++;else consecutiveEOLMoves=0;
                if(consecutiveEOLMoves>=2){cursorCol=lines[cursorLine].length;targCol=INT_MAX;}
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_HOME:
            if(wordWrap){
                if(ctrl){cursorLine=0;cursorCol=0;scroll.targetY=0;}
                else{int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);wc=0;WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);}
            }else{
                if(ctrl){cursorLine=0;cursorCol=0;scroll.targetY=0;scroll.targetX=0;}
                else{
                    int origCol=cursorCol;cursorCol=0;
                    if(origCol==0){
                        while(cursorCol<lines[cursorLine].length&&isspace((unsigned char)lines[cursorLine].text[cursorCol]))cursorCol++;
                        if(cursorCol==lines[cursorLine].length||cursorCol==origCol)cursorCol=0;}scroll.targetX=0;
                }
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_END:
            if(wordWrap){
                if(ctrl){cursorLine=numLines-1;cursorCol=lines[cursorLine].length;scroll.targetY=MaxScroll();}
                else{int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);wc=wrappedLines[wl].length;WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);}
            }else{
                if(ctrl){cursorLine=numLines-1;cursorCol=lines[cursorLine].length;scroll.targetY=MaxScroll();scroll.targetX=MaxHorizontalScroll();}
                else cursorCol=lines[cursorLine].length;
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_PAGE_UP:
            if(wordWrap){
                int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
                int visLines=window.screen_height/lineHeight;
                wl=fmax(0,wl-visLines/2);
                wc=fmin(targCol>=0?targCol:wc,wrappedLines[wl].length);
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                scroll.targetY=fmax(0,scroll.targetY-window.screen_height/2);
            }else{
                int visLines=window.screen_height/lineHeight;
                cursorLine=fmax(0,cursorLine-visLines/2);
                cursorCol=fmin(cursorCol,lines[cursorLine].length);
                scroll.targetY=fmax(0,scroll.targetY-window.screen_height/2);
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_PAGE_DOWN:
            if(wordWrap){
                int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);
                int visLines=window.screen_height/lineHeight;
                wl=fmin(numWrappedLines-1,wl+visLines/2);
                wc=fmin(targCol>=0?targCol:wc,wrappedLines[wl].length);
                WrappedToOriginal(wl,wc,&cursorLine,&cursorCol);
                scroll.targetY=fmin(MaxScroll(),scroll.targetY+window.screen_height/2);
            }else{
                int visLines=window.screen_height/lineHeight;
                cursorLine=fmin(numLines-1,cursorLine+visLines/2);
                cursorCol=fmin(cursorCol,lines[cursorLine].length);
                scroll.targetY=fmin(MaxScroll(),scroll.targetY+window.screen_height/2);
            }
            if(!shift&&isSelecting&&!ctrl){isSelecting=0;selStartLine=selEndLine=selStartCol=selEndCol=-1;}
            break;
        case GLFW_KEY_TAB:
            if(IsSelValid())DeleteSelection();
            for(int i=0;i<3;i++)InsertChar(' ');
            break;
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
        case GLFW_KEY_ENTER:
            if(IsSelValid())DeleteSelection();
            InsertNewline();
            break;
        case GLFW_KEY_A:
            if(ctrl){
                selStartLine=0;selStartCol=0;
                selEndLine=numLines-1;selEndCol=lines[numLines-1].length;
                cursorLine=selEndLine;cursorCol=selEndCol;
                isSelecting=1;
                snprintf(statusMsg,sizeof(statusMsg),"Selected all");
            }
            break;
        case GLFW_KEY_H:
            if(ctrl){
                if(shift){
                    if(SyntaxCycleLanguage())snprintf(statusMsg,sizeof(statusMsg),"Syntax: %s",SyntaxGetLanguage());
                    else snprintf(statusMsg,sizeof(statusMsg),"Syntax highlighting disabled");
                }else{
                    SyntaxToggle();
                    if(SyntaxIsEnabled())snprintf(statusMsg,sizeof(statusMsg),"Syntax: %s",SyntaxGetLanguage());
                    else snprintf(statusMsg,sizeof(statusMsg),"Syntax highlighting disabled");
                }
            }
            break;
        case 47:
            if(ctrl&&fontSize>=22.0f)Zoom(-2.0f);
            break;
        case 93:
            if(ctrl&&fontSize<=40.0f)Zoom(2.0f);
            break;
        case GLFW_KEY_L:
            if(ctrl){
                showLineNumbers=!showLineNumbers;
                snprintf(statusMsg,sizeof(statusMsg),"LineNumbers= %s",showLineNumbers?"ON":"OFF");
            }
            break;
        case GLFW_KEY_R:
            if(ctrl&&!showMinimap){
                showScrollbar=!showScrollbar;
                snprintf(statusMsg,sizeof(statusMsg),"Scrollbar = %s",showScrollbar?"ON":"OFF");
            }
            break;
        case GLFW_KEY_M:
            if(ctrl){
                if(!showScrollbar)showScrollbar=!showScrollbar;
                showMinimap=!showMinimap;
                snprintf(statusMsg,sizeof(statusMsg),"Minimap = %s",showMinimap?"ON":"OFF");
            }
            break;
        case GLFW_KEY_N:
            if(ctrl){
                showModeline=!showModeline;
                scroll.targetY=fmin(scroll.targetY,MaxScroll());
            }
            break;
        case GLFW_KEY_P:
            if(ctrl){
                autoParingEnabled=!autoParingEnabled;
                snprintf(statusMsg,sizeof(statusMsg),"Auto-pairing: %s",autoParingEnabled?"ON":"OFF");
            }
            break;
        case GLFW_KEY_S:
            if(ctrl){
                if(shift)snprintf(statusMsg,sizeof(statusMsg),"Save As not implemented");
                else if(isFileDirty){
                    char* text=ConstructTextFromLines();
                    if(FileSave(filename,text)==NULL)snprintf(statusMsg,sizeof(statusMsg),"Error saving file -> %s",filename);
                    else{snprintf(statusMsg,sizeof(statusMsg),"File saved -> %s",filename);isFileDirty=0;}
                    free(text);
                }else{
                    scroll.smoothScroll=!scroll.smoothScroll;
                    snprintf(statusMsg,sizeof(statusMsg),"Smooth Scroll: %s",scroll.smoothScroll?"ON":"OFF");
                }
            }
            break;
    }
    cursorLine=fmax(0,fmin(numLines-1,cursorLine));
    cursorCol=fmax(0,fmin(lines[cursorLine].length,cursorCol));
    if(isSelecting){selEndLine=cursorLine;selEndCol=cursorCol;}
    isCursorMoved = (pLine!=cursorLine || pCol!=cursorCol);
    if(!shift && isCursorMoved && !(key==GLFW_KEY_BACKSPACE||key==GLFW_KEY_DELETE)) {
        isSelecting=0;
        selStartLine=selEndLine=selStartCol=selEndCol=-1;
    }
    int visH=window.screen_height-(showStatusBar?statusBarHeight:0)-(showModeline?modeLineHeight:0);
    int visW=window.screen_width-(showLineNumbers?gutterWidth:0)-(showScrollbar?scrollbarWidth:0);
    float curX,curY;
    if(wordWrap){int wl,wc;OriginalToWrapped(cursorLine,cursorCol,&wl,&wc);curY=wl*lineHeight;curX=0;}
    else{curX=GetCursorXPosition();curY=cursorLine*lineHeight;}
    float vTop=scroll.currentY,vBot=vTop+visH,buf=lineHeight;
    if(curY<vTop+buf)scroll.targetY=curY-buf;
    else if(curY+lineHeight>vBot-buf)scroll.targetY=curY-visH+lineHeight+buf;
    if(!wordWrap){
        float vLeft=scroll.currentX,vRight=vLeft+visW;
        float hBuf=GetTextSizeCached(font,fontSize,"MM").width;
        if(curX<vLeft+hBuf)scroll.targetX=curX-hBuf;
        else if(curX>vRight-hBuf)scroll.targetX=curX-visW+hBuf;
        scroll.targetX=fmax(0,fmin(MaxHorizontalScroll(),scroll.targetX));
        float cw=GetTextSizeCached(font,fontSize,"M").width;
        scroll.targetX=roundf(scroll.targetX/cw)*cw;
    }else scroll.targetX=0;
    scroll.targetY=fmax(0,fmin(MaxScroll(),scroll.targetY));
    lastVerticalScrollTime=lastHorizontalScrollTime=window.time;
    if(wordWrap&&isFileDirty&&(key==GLFW_KEY_BACKSPACE||key==GLFW_KEY_DELETE||key==GLFW_KEY_ENTER||
                          key==GLFW_KEY_TAB||(action==GLFW_PRESS||action==GLFW_REPEAT)&&isprint(key))){
        RecalculateWrappedLines();
    }
}

void MouseCallback(GLFWwindow* win,int button,int action,int mods){
    static double lastClickTime=0;static int clickCount=0;double x,y;
    glfwGetCursorPos(win,&x,&y);
    if(button==GLFW_MOUSE_BUTTON_LEFT){
        if(action==GLFW_PRESS){
            if(showMinimap){
                int minimapX=window.screen_width-minimapWidth-(showScrollbar?scrollbarWidth:0);
                if(x>=minimapX&&x<=minimapX+minimapWidth&&y>=0&&y<=window.screen_height){
                    isMinimapDragging=true;HandleMinimapDrag(x,y);return;
                }
            }
            if(window.deltatime-lastClickTime<0.4)clickCount++;else clickCount=1;
            lastClickTime=window.deltatime;
            if(IsInScrollbar(x,y)){isScrollbarDragging=true;HandleScrollDrag(y);return;}
            else if(IsInHorizontalScrollbar(x,y)){isHorizontalScrollbarDragging=true;HandleHorizontalScrollDrag(x);return;}
            UpdateCursor(x,y);
            if(clickCount==1){selStartLine=selEndLine=cursorLine;selStartCol=selEndCol=cursorCol;isSelecting=true;}
            else if(clickCount==2){
                int lineLen=lines[cursorLine].length;const char* lineText=lines[cursorLine].text;
                int wordStart=cursorCol;
                while(wordStart>0&&(isalnum(lineText[wordStart-1])||lineText[wordStart-1]=='_'))wordStart--;
                int wordEnd=cursorCol;
                while(wordEnd<lineLen&&(isalnum(lineText[wordEnd])||lineText[wordEnd]=='_'))wordEnd++;
                selStartLine=selEndLine=cursorLine;selStartCol=wordStart;selEndCol=wordEnd;cursorCol=wordEnd;isSelecting=true;
            }else if(clickCount>=3){
                selStartLine=selEndLine=cursorLine;selStartCol=0;selEndCol=lines[cursorLine].length;
                cursorCol=selEndCol;isSelecting=true;clickCount=0;
            }
        }else if(action==GLFW_RELEASE){
            isMinimapDragging=isScrollbarDragging=isHorizontalScrollbarDragging=false;
            if(selStartLine==selEndLine&&selStartCol==selEndCol){
                selStartLine=selEndLine=-1;selStartCol=selEndCol=-1;isSelecting=false;
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
        int visibleHeight = GetVisibleHeight();
        int linesPerScreen = visibleHeight / lineHeight;
        int currentTopLine = (int)(scroll.targetY / lineHeight);
        int edge = lineHeight * 2;
        if (y < edge) {
            scroll.targetY = fmax(0, (currentTopLine - 1) * lineHeight);
        }
        else if (y > window.screen_height - edge) {
            int maxTopLine = fmax(0, numLines - linesPerScreen);
            scroll.targetY = fmin(maxTopLine * lineHeight, (currentTopLine + 1) * lineHeight);
        }
        int viewWidth = window.screen_width - textX - (showScrollbar ? scrollbarWidth : 0);
        float charWidth = GetTextSizeCached(font, fontSize, "A").width;
        int currentTopChar = (int)(scroll.currentX / charWidth);
        int horizEdge = 50;
        if (x < horizEdge) {
            scroll.targetX = fmax(0, (currentTopChar - 1) * charWidth);
        }
        else if (x > window.screen_width - horizEdge) {
            int maxTopChar = fmax(0, (GetDocumentMaxWidth() - viewWidth) / charWidth);
            scroll.targetX = fmin(MaxHorizontalScroll(), (currentTopChar + 1) * charWidth);
        }
    }
}

void ScrollCallback(GLFWwindow* win, double x, double y) {
    int ctrlPressed = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL)==GLFW_PRESS;
    int shiftPressed = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    if(ctrlPressed && y!=0) {
        float zoomAmount = (y<0) ? -2.0f : 2.0f;
        if((zoomAmount<0 && fontSize>=22.0f) || (zoomAmount>0 && fontSize<=40.0f)) {
            Zoom(zoomAmount);
            UpdateGutterWidth();
            lastVerticalScrollTime = lastHorizontalScrollTime = window.time;
        }
        return;
    }
    if(x!=0 || shiftPressed) {
        double amount = (x!=0) ? x : y;
        float charWidth = GetTextSizeCached(font, fontSize, "M").width;
        scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), scroll.targetX - amount*charWidth*2));
    } else {
        scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY - y*lineHeight));
    }
    if(scroll.smoothScroll) scroll.velocity = 0;
}

void CharCallback(GLFWwindow* win, unsigned int c) {
    if (lsp.active) {EditorLspCharHandler(c);}
    if (isprint(c)) {
        if (IsSelValid() && autoParingEnabled) {
            char closingBracket = 0;
            bool shouldWrapSelection = false;
            for (size_t i = 0; i < sizeof(bracket_pairs)/sizeof(bracket_pairs[0]); i++) {
                if (bracket_pairs[i].open == c) {
                    closingBracket = bracket_pairs[i].close;
                    shouldWrapSelection = true;
                    break;
                }
            }
            if (!shouldWrapSelection) {
                for (size_t i = 0; i < sizeof(bracket_pairs)/sizeof(bracket_pairs[0]); i++) {
                    if (bracket_pairs[i].close == c) {
                        closingBracket = c;
                        c = bracket_pairs[i].open;
                        shouldWrapSelection = true;
                        break;
                    }
                }
            }
            if (shouldWrapSelection) {
                int startLine = selStartLine, startCol = selStartCol;
                int endLine = selEndLine, endCol = selEndCol;
                int origCursorLine = cursorLine;
                int origCursorCol = cursorCol;
                if (startLine > endLine || (startLine == endLine && startCol > endCol)) {
                    int tmp = startLine; startLine = endLine; endLine = tmp;
                    tmp = startCol; startCol = endCol; endCol = tmp;
                }
                cursorLine = endLine;
                cursorCol = endCol;
                InsertChar(closingBracket);
                cursorLine = startLine;
                cursorCol = startCol;
                InsertChar(c);
                if (origCursorLine == startLine && origCursorCol == startCol) {
                    cursorLine = startLine;
                    cursorCol = startCol;
                    selStartLine = startLine;
                    selStartCol = startCol + 1;
                    selEndLine = endLine;
                    selEndCol = endCol + 1;
                } else {
                    cursorLine = endLine;
                    cursorCol = endCol + 2;
                    selStartLine = startLine;
                    selStartCol = startCol + 1;
                    selEndLine = endLine;
                    selEndCol = endCol + 1;
                }
                if (wordWrap) {
                    RecalculateWrappedLines();
                }
                isFileDirty = true;
                isSelecting = true;
                return;
            }
        }
        if (IsSelValid()) DeleteSelection();
        bool skipInsert = false;
        if (insertMode && autoParingEnabled) {
            bool isClosing = false;
            bool isOpeningOrSame = false;
            char closing = 0, opening = 0;
            for (size_t i = 0; i < sizeof(bracket_pairs)/sizeof(bracket_pairs[0]); i++) {
                if (bracket_pairs[i].close == c) {
                    isClosing = true;
                    opening = bracket_pairs[i].open;
                } 
                if (bracket_pairs[i].open == c) {
                    isOpeningOrSame = true;
                    closing = bracket_pairs[i].close;
                }
            }
            if (isClosing) {
                if (cursorCol < lines[cursorLine].length && 
                    lines[cursorLine].text[cursorCol] == c) {
                    cursorCol++;
                    skipInsert = true;
                }
            }
        }
        if (insertMode) {
            bool isOpening = false;
            char closing = 0;
            for (size_t i = 0; i < sizeof(bracket_pairs)/sizeof(bracket_pairs[0]); i++) {
                if (bracket_pairs[i].open == c) {
                    isOpening = true;
                    closing = bracket_pairs[i].close;
                    break;
                }
            }
            int originalLength = lines[cursorLine].length;
            if (isOpening) {
                selStartLine = selEndLine = -1;
                selStartCol = selEndCol = -1;
                isSelecting = false;
            }
            if (!skipInsert) {
                if (insertMode) {
                    int newLengthAfterOpen = 0;
                    InsertChar((char)c);
                    newLengthAfterOpen = lines[cursorLine].length;
                    if (newLengthAfterOpen > originalLength) {
                        if (isOpening && autoParingEnabled) {
                            InsertChar(closing);
                            if (lines[cursorLine].length > newLengthAfterOpen) {
                                cursorCol--;
                                if (numAutoPairs < MAX_AUTO_PAIRS) {
                                    autoPairs[numAutoPairs].line = cursorLine;
                                    autoPairs[numAutoPairs].col = cursorCol - 1;
                                    autoPairs[numAutoPairs].open = (char)c;
                                    autoPairs[numAutoPairs].close = closing;
                                    autoPairs[numAutoPairs].wasAutoInserted = true;
                                    numAutoPairs++;
                                }
                            }
                        }
                    }
                } else {
                    if (cursorCol < lines[cursorLine].length) {
                        lines[cursorLine].text[cursorCol] = (char)c;
                        cursorCol++;
                    } else {
                        if (lines[cursorLine].length < MAXTEXT - 1) {
                            lines[cursorLine].text[lines[cursorLine].length] = (char)c;
                            lines[cursorLine].length++;
                            cursorCol = lines[cursorLine].length;
                        }
                    }
                    if (lines[cursorLine].length >= MAXTEXT - 1) {
                        lines[cursorLine].length = MAXTEXT - 1;
                        lines[cursorLine].text[lines[cursorLine].length] = '\0';
                    }
                }
                isFileDirty = true;
            }
        }
    }
}