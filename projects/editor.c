#include "../src/window.h"
//#include <grafenic/window.h>

Font font;
Shader shaderfontcursor;

#include "modules/syntax.c"
#include "modules/editor.c"

int main(int argc, char *argv[]) {
    window.opt.floating = true;
    //window.opt.transparent = true;glClearColor(0.0f, 0.0f, 0.0f, 0.75f);
    WindowInit(1920, 1080, "Text Editor");
    font = LoadFont("./res/fonts/Monocraft.ttf");//font.nearest = false;
    //shaderfont.hotreloading = true;//shaderdefault.hotreloading = true;
    Init(argc, argv);
    while (!WindowState()) {
        WindowClear();
        Draw();
        WindowProcess();
    }
    Close();
    WindowClose();
    return 0;
}

// Features
//  markdown rendering
//  modline with git status
//  file manager buffer
//  terminal buffer, pipe commands to buffer
//  command line with to eval commads inside of the editor
//  shortcut save as,recent file,open,ident,comment,undo,redo
//  buffer window managment and tabs
//  timeline of files old 7 days
//  sudo save using like pkexec
//  code folding
//  use paywall colors and refine colors in modline and minimap
//  code snippets
//  unicode support
//  refine zooming animation to be more smooth and also scrolling
//  term escape sequences support
//  jupyter notebooks like cells with whatever interpreter you want
// External Modules
//  debugger like to debug itself and change variables introspectivly
//  diff folder,clipboard,git,timeline
//  search & replace & regex 
//  autopair brackets,quotes 
//  color picker
//  lsp for autocompletion
//  git info
//  copilot like ai
//  snyk anlysis
