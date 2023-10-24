#include "libs/graphene.h"

void DrawCenteredText(Font myFont,int section, const char* textContent) {
    int y = SCREEN_HEIGHT / 12;
    int sectionWidth = SCREEN_WIDTH / 3;
    int fontSize = SCREEN_HEIGHT / 40;
    TextSize textSize = GetTextSize(myFont, fontSize, textContent);
    int textX = section * sectionWidth + (sectionWidth - textSize.width) / 2;
    int textY = y/2 + textSize.height/2;
    DrawText(textX, textY , myFont, fontSize, textContent, HexToColor("#FFFFFF"));
}

void Draw(Font myFont) {
    int y = SCREEN_HEIGHT / 12;
    DrawRect(0, 0, SCREEN_WIDTH, y, HexToColor("#32323264"));
    DrawRectBorder(0, 0, SCREEN_WIDTH, y, 5, HexToColor("#000000AF"));

    Mouse mouse = GetMousePos();
    DrawCenteredText(myFont, 0, text("Mouse = X: %.0f Y: %.0f", mouse.x, mouse.y));

    MouseScroll scrollData = GetScroll();
    DrawCenteredText(myFont, 1, text("Scroll = X: %.0f Y: %.0f", scrollData.x, scrollData.y));

    DrawCenteredText(myFont, 2, text("Size = X: %d Y: %d", SCREEN_WIDTH, SCREEN_HEIGHT));

    // Rest of your commented code remains unchanged
}

void DrawPopUp(const char* title, Font myFont, int width, int height) {
    int x = (SCREEN_WIDTH / 2) - (width / 2);
    int y = (SCREEN_HEIGHT / 2) - (height / 2);
    DrawRect(x, y, width, height, HexToColor("#64646464"));
    DrawRectBorder(x, y, width, height, 5, HexToColor("#000000"));
    TextSize text = GetTextSize(myFont, 16, title);
    DrawText((SCREEN_WIDTH / 2) - (text.width / 2), (SCREEN_HEIGHT / 2) + (text.height / 2) + 3, myFont, 16, title, HexToColor("#FFFFFF"));
}

void Fps(int x, int y, Font myFont, int size) {
    DrawText(x, y, myFont, SCREEN_HEIGHT/size, text("FPS: %.0f", fps), HexToColor("#FFFFFF"));
}

void ExitPromt(Font myFont) {
    if (isKey("Esc")) {
        DrawPopUp("Quit?Yes/No", myFont, 135, 25);
        if (isKeyDown("Y")) {
            WindowStateSet(true);
        }
        if (isKeyDown("N")) {
            isKeyReset("Esc");
        }
    }
}

int main(void) {
    // Built-in attributes remain unchanged
    WindowInit(1920, 1080, "Hello World");

    Font myFont = LoadFont("./res/Monocraft.ttf");
    Texture img1 = LoadTexture("./res/box1.png");
    Texture img2 = LoadTexture("./res/Arch.png");
    while (!WindowState()) {
        ClearColor(HexToColor("#646464"));
        WindowClear();
        OrthoCam(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
        DrawTexture(img2,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0);

        DrawCube(img1);
        Draw(myFont);
        Fps(0, SCREEN_HEIGHT, myFont, 35);
        ExitPromt(myFont);

        WindowProcess();
    }
    WindowClose();
    return 0;
}
