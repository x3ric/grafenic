#include "grafenic/init.c"
#include "grafenic/ui.c"

Font font;
float mousecursorx;
float mousecursory;
float timer;
int bary;
bool cursor;

void update(void){
    // Input   
        if(isKey("M")){ // Change Cursor state
            window.opt.hidecursor = false;
        } else {
            window.opt.hidecursor = true;
        }
    // Cursor
        double speed;
        double clampz;
        clampz = (window.deltatime);
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
            timer += window.deltatime / (speedcursor * 1000.0f);
        }
        if (IsInside(mousecursorx, mousecursory, 0, window.screen_height - bary, window.screen_width, window.screen_height / 12) && isKey("Space")) {
            DrawCircle(mousecursorx, mousecursory, circleSize, PURPLE);
            DrawCircleBorder(mousecursorx, mousecursory, circleSize, circleborderSize, VIOLET);
        } else {
            DrawCircle(mousecursorx, mousecursory, circleSize, VIOLET);
            DrawCircleBorder(mousecursorx, mousecursory, circleSize, circleborderSize, PURPLE);
        }
    // Bottom Bar Info
        if(isKey("Space")){
            bary = window.screen_height / 12;
            DrawRect(0, window.screen_height - bary, window.screen_width, bary, (Color){50, 50, 50,100});
            DrawRectBorder(0, window.screen_height - bary, window.screen_width, bary, Scaling(5), (Color){0, 0, 0,175});
            int texts = 3;
            // Mouse info
                DrawTextRows( font,0,texts, text("Mouse = X: %.0f Y: %.0f", mouse.x, mouse.y));
            // Scroll info
                DrawTextRows( font,1,texts, text("Scroll = X: %.0f Y: %.0f", mouse.scroll.x, mouse.scroll.y));
            // Window info 
                DrawTextRows( font,2,texts, text("Size = X: %d Y: %d", window.screen_width, window.screen_height));
        }
    // Modular ui.c functions
        int texts = 9;
        DrawTextColumn(font,1,texts, text("M1 = circle state: %s", isMouseButton(0) ? "ON" : "OFF"));
        DrawTextColumn(font,2,texts, text("Mouse moving: %s", mouse.moving ? "ON" : "OFF"));
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
