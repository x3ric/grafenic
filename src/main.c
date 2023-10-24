#include "libs/graphene.h"

void DrawCenteredText(Font myFont,int section, const char* textContent) {
    int y = SCREEN_HEIGHT / 12;
    int sectionWidth = SCREEN_WIDTH / 3; // Divide screen into 3 equal sections
    int fontSize = SCREEN_HEIGHT / 40;
    TextSize textSize = GetTextSize(myFont, fontSize, textContent);
    int textX = section * sectionWidth + (sectionWidth - textSize.width) / 2;
    int textY = y/2 + textSize.height/2;
    DrawText(textX, textY , myFont, fontSize, textContent, (Color){255, 255, 255});
}

void Draw(Font myFont) {
    int y = SCREEN_HEIGHT / 12;
    DrawRect(0, 0, SCREEN_WIDTH, y, (Color){50, 50, 50,100});
    DrawRectBorder(0, 0, SCREEN_WIDTH, y, 5, (Color){0, 0, 0,175});
    // Mouse info
    Mouse mouse = GetMousePos();
    DrawCenteredText( myFont,0, text("Mouse = X: %.0f Y: %.0f", mouse.x, mouse.y));
    // Scroll info
    MouseScroll scrollData = GetScroll();
    DrawCenteredText( myFont,1, text("Scroll = X: %.0f Y: %.0f", scrollData.x, scrollData.y));
    // Window info 
    DrawCenteredText( myFont,2, text("Size = X: %d Y: %d", SCREEN_WIDTH, SCREEN_HEIGHT));

    //Input example and mouse hide
        //if(isKey("M")){
        //    cursor = false;
        //} else {
        //    cursor = true;
        //}

        //if(isKey("V")){
        //    vsync = true;
        //} else {
        //    vsync = false;
        //}

        // Built-in
            // fullscreen = false;
            // vsync = false;
            // cursor = true;
            // fpslimit = 60;
            // fps = ReadOnly; 
            // frametime = ReadOnly; 

    //Input mouse example
        //if (isMouseButton(0)) {
        //    DrawText(300, SCREEN_HEIGHT / 2 + 50, myFont, 16,  "1 pressed = Mouse 1 pressed", (Color){0, 0, 0});
        //}

    //DrawLine Examples
        //DrawLine(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){120, 120, 120}, 5);
        //DrawLine(0, SCREEN_HEIGHT, SCREEN_WIDTH, 0, (Color){120, 120, 120}, 5);
}

void DrawPopUp(const char* title, Font myFont, int width, int height) { // Simple popup example function
    int x = (SCREEN_WIDTH / 2) - (width / 2);
    int y = (SCREEN_HEIGHT / 2) - (height / 2);
    DrawRect(x, y, width, height, (Color){100, 100, 100,100});
    DrawRectBorder(x, y, width, height, 5, (Color){0, 0, 0});
    TextSize text = GetTextSize(myFont, 16, title);
    DrawText((SCREEN_WIDTH / 2) - (text.width / 2), (SCREEN_HEIGHT / 2) + (text.height / 2) + 3, myFont, 16, title, (Color){255, 255, 255});
}

void Fps(int x , int y, Font myFont, int size) {
    DrawText( x, y, myFont, SCREEN_HEIGHT/size, text("FPS: %.0f", fps), (Color){255, 255, 255});
}

void ExitPromt(Font myFont) { // Draw your quit prompt
    if (isKey("Esc")) {
        DrawPopUp("Quit?Yes/No",myFont,135,25);
        if (isKeyDown("Y")) {
            WindowStateSet(true);
        }
        if (isKeyDown("N")) {
            isKeyReset("Esc");
        }
    }
}

int main(void)
{
    WindowInit(1920, 1080, "Hello World");
    //Image myImage = LoadImage("./res/Arch.png");
    Font myFont = LoadFont("./res/Monocraft.ttf");
    Texture img1 = LoadTexture("./res/box1.png");
    Texture img2 = LoadTexture("./res/Arch.png");

    fpslimit = 60;
    
    while (!WindowState())
    {
        ClearColor((Color){100, 100, 100});
        WindowClear();
                OrthoCam(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
                    //DrawRectGL(100, 100, 200, 200, (Color){0, 0, 0,150});
                    //DrawImage(0,0,myImage);
                    DrawTexture(img2,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0);

                //int camsize = 2;
                //int camfar = 25;
                //OrthoCam(-camsize, camsize, -camsize, camsize, -camfar, camfar);
                //PerspectiveCam(45.0f, SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
                    //DrawCube(img1);
            Draw(myFont);
            Fps(0, SCREEN_HEIGHT, myFont, 35);
            ExitPromt(myFont);
        WindowProcess();
    }
    WindowClose();
    return 0;
}
