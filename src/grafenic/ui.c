
void DrawTextRows(Font font,int section,int rows, const char* textContent) { // Text Rows
    int y = SCREEN_HEIGHT / 12;
    int sectionWidth = SCREEN_WIDTH / rows;
    int fontsize = Scaling(50);
    TextSize textSize = GetTextSize(font, fontsize, textContent);
    int textX = section * sectionWidth + (sectionWidth - textSize.width) / 2;
    int textY = SCREEN_HEIGHT - y/2 - textSize.height/2;
    DrawText(textX, textY , font, fontsize, textContent, WHITE);
} 

void DrawTextColumn(Font font, int section, int totalSections, const char* textContent) { // Text Collumns
    int sectionHeight = SCREEN_HEIGHT / totalSections;
    int fontsize = Scaling(40);
    TextSize textSize = GetTextSize(font, fontsize, textContent);
    int textX = 10;
    int textY = section * sectionHeight + (textSize.height);
    DrawText(textX, textY, font, fontsize, textContent, WHITE);
} 

void DrawPopUp(const char* title, Font font, int fontsize, int width, int height) { 
    int x = (SCREEN_WIDTH / 2) - (width / 2);
    int y = (SCREEN_HEIGHT / 2) - (height / 2);
    DrawRect(x, y, width, height, (Color){50, 50, 50, 100});
    DrawRectBorder(x, y, width, height, Scaling(5), (Color){0, 0, 0});
    TextSize text = GetTextSize(font, fontsize, title);
    int textX = x + (width / 2) - (text.width / 2);
    int textY = y + (height / 2) - (text.height / 2);
    DrawText(textX, textY, font, fontsize, title, WHITE);
}

void Fps(int x , int y, Font font, int size) { // FPS info
    DrawText( x, y, font, size, text("FPS: %.0f", fps), WHITE);
} 

void ExitPromt(Font font) { // Escape PopUp
    if (isKey("Esc")) {
        DrawPopUp("Quit? y/n",font,Scaling(17),SCREEN_WIDTH/18, SCREEN_HEIGHT/35);
        if (isKeyDown("Y")) {
            WindowStateSet(true);
        }
        if (isKeyDown("N")) {
            isKeyReset("Esc"); // inverts isKey("Esc") return
        }
    }
} 
