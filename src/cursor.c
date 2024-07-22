#include "grafenic/init.c"

Font font;

float mousecursorx;
float mousecursory;
float timer;

#include "grafenic/ui.c"

void update(void){
    // Input
        if(isKey("M")){ // Change Cursor state
            cursor = false;
        } else {
            cursor = true;
        }
    // Cursor
        double speed;
        double clampz;
        clampz = (deltatime);
        float lerpSpeed = 0.0003f * clampz;
        const float speedcursor = 0.75f;
        static float timer = 0.0f;
        float circleSize = Lerp(0.0f, (float)Scaling(10), timer);
        float circleborderSize = Lerp(0.0f, (float)Scaling(3), timer);
        if (!isMouseButton(0)) {
            if (mousecursorx <= 0){
                mousecursorx = mouse.x;
            } 
            if(mousecursory <= 0) {
                mousecursory = mouse.y;
            }
            mousecursorx = Lerp(mousecursorx, mouse.x, lerpSpeed);
            mousecursory = Lerp(mousecursory, mouse.y, lerpSpeed);
            timer = Lerp(timer, 0.0f, lerpSpeed);
        } else {
            mousecursorx = Lerp(mousecursorx, mouse.x, lerpSpeed);
            mousecursory = Lerp(mousecursory, mouse.y, lerpSpeed);
            timer += deltatime / (speedcursor * 1000.0f);
        }
        if (IsInside(mousecursorx, mousecursory, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 12) && isKey("Space")) {
            DrawCircle(mousecursorx, mousecursory, circleSize, PURPLE);
            DrawCircleBorder(mousecursorx, mousecursory, circleSize, circleborderSize, VIOLET);
        } else {
            DrawCircle(mousecursorx, mousecursory, circleSize, VIOLET);
            DrawCircleBorder(mousecursorx, mousecursory, circleSize, circleborderSize, PURPLE);
        }
    // Modular ui.c functions
        int texts = 9;
        DrawTextColumn(font,1,texts, text("M1 = circle state: %s", isMouseButton(0) ? "ON" : "OFF"));
        DrawTextColumn(font,2,texts, text("Mouse moving: %s", mousemoving ? "ON" : "OFF"));
        DrawTextColumn(font,3,texts, text("M = mouse state: %s", cursor ? "ON" : "OFF"));
        Fps(0, 0, font, Scaling(50));
        ExitPromt(font);  
} 

int main(int arglenght, char** args)
{ 
    WindowInit(1920, 1080, "Grafenic");
    font = LoadFont("./res/fonts/Monocraft.ttf");font.nearest = true;
    ClearColor((Color){75, 75, 75,100});
    while (!WindowState())
    {
        WindowClear();
            update();
        WindowProcess();
    }
    WindowClose();
    return 0;
} 
