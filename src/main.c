#include "grafenic/init.c"

Font font;
Image img;
Shader custom;

float mousecursorx;
float mousecursory;
float timer;

Color pixelColor;

#include "ui.c"

void update(void){
    // Movement Camera
        //double speed;
        //if (isKeyDown("LeftShift")) {
        //    speed = 0.2f;
        //} else {
        //    speed = 0.1f;
        //}
        //if (isKeyDown("w")) {
        //    camera.y += speed * deltatime;
        //}
        //if (isKeyDown("s")) {
        //    camera.y -= speed * deltatime;
        //}
        //if (isKeyDown("a")) {
        //    camera.x += speed * deltatime;
        //}
        //if (isKeyDown("d")) {
        //    camera.x -= speed * deltatime;
        //}
        //if (isKeyDown("r")) {
        //    camera.x = 0.0;
        //    camera.y = 0.0;
        //    camera.angle = 0.0;
        //}
        //if (mousescroll.y < 0) {mousescroll.y = 0;}
        //camera.z = mousescroll.y;
        //if (isKeyDown("e")) {
        //   camera.angle += 0.001f * deltatime;
        //} else if (isKeyDown("q")) {
        //   camera.angle -= 0.001f * deltatime;
        //}
    // DrawImage
        //DrawImageShader(img,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0,custom);
        //DrawImage(img,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0);
    // Input Example
        //      Key                    MouseButton                      Info
        // isKey,isKeyReset  // isMouseButton,isMouseButtonReset  // key Bool
        // isKeyDown,isKeyUp // isMouseButtonDown,isMouseButtonUp // key Press/Relase 
        // isKeyPressed      //                                   // key Mantain 
        //if (isKeyPressed("A", 0.06f)) { // Repeat key every 0.06 seconds
        //    DrawCircle(mouse.x,mouse.y, Scaling(35), (Color){0, 0, 0, 75});
        //}
        //if(isKey("M")){ // Change Cursor state
        //    cursor = false;
        //} else {
        //    cursor = true;
        //}
        //if(isKey("V")){ // Change Vsync state
        //    vsync = true;
        //} else {
        //    vsync = false;
        //}
        // cursor
            //if (!isMouseButton(0)) { 
            //    // gradually reset ball to center pos when not visible
            //        const int speed = 3;
            //        float lerpOutSpeed = (0.0001*speed) * GetTime();
            //        mousecursorx = Lerp(mousecursorx, SCREEN_WIDTH / 2,  lerpOutSpeed);
            //        mousecursory = Lerp(mousecursory, SCREEN_HEIGHT / 2, lerpOutSpeed);
            //        timer = 0;
            //} else {
            //    float lerpInSpeed = 0.05;
            //    mousecursorx = Lerp(mousecursorx, mouse.x, lerpInSpeed);
            //    mousecursory = Lerp(mousecursory, mouse.y, lerpInSpeed);
            //    const float speed = 0.75;
            //    timer += GetTime()/(speed*1000);
            //    float circleSize = Lerp(0, (float)Scaling(25), timer);
            //    //if (IsInside(mouse.x,mouse.y,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 12)) { // Simple 2d collision Mouse based
            //    if(IsInside(mousecursorx,mousecursory,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 12)) { // Simple 2d collision X and Y cursor based
            //        DrawCircle(mousecursorx, mousecursory, circleSize, PURPLE);
            //        DrawCircleBorder(mousecursorx, mousecursory, circleSize, 2, VIOLET);
            //    } else {
            //        DrawCircle(mousecursorx, mousecursory, circleSize, VIOLET);
            //        DrawCircleBorder(mousecursorx, mousecursory, circleSize, 2, PURPLE);
            //    }
            //}
    // Easing Lerp
        //int sizeball  = Lerp(0, 50,  Easing(Motion(1.0,0.5), "Linear"));
        //int positionx = Lerp(0, SCREEN_WIDTH,  Easing(Motion(1.0,1.0), "CubicInOut"));
        //int positiony = Lerp(0, SCREEN_HEIGHT, Easing(Motion(1.0,1.0), "CubicInOut"));
        //DrawCircle(positionx, positiony, sizeball, VIOLET);
    // DrawLine Examples "Cross Screen"
        //DrawLine(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 5, BLACK);
        //DrawLine(0, SCREEN_HEIGHT, SCREEN_WIDTH, 0, 5, BLACK);
    // DrawTriangle Examples
        //int x = SCREEN_WIDTH/2;
        //int y = 0;
        //static int size = 50;
        //    if(isKeyDown("+")){
        //        size += 1;
        //    }
        //    if(isKeyDown("-") && size > 0){
        //        size -= 1;
        //    }
        //DrawTriangle(x - size, y, x + size, y, x, y + size, PURPLE);
    // Drawing Pixel Pointed Color 
        // Uncomment "GetPixel Pointed color" in main
        //int fontsize = Scaling(50);
        //const char* textContent = text("R:%d G:%d B:%d A:%d", pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a);
        //TextSize textSize = GetTextSize(font, fontsize, textContent);
        //DrawText(SCREEN_WIDTH/2-(textSize.width/2), 0, font, fontsize, textContent, WHITE);
    // Experimental WorkInProgress
        //debug.wireframe = true; //debug single part
        // TriangleGL 
            //GLfloat x1 = SCREEN_WIDTH/2, y1 = 0;
            //GLfloat x2 = 0.0f, y2 = SCREEN_HEIGHT;
            //GLfloat x3 = SCREEN_WIDTH,  y3 = SCREEN_HEIGHT;
            //Triangle(custom,0, x1, y1, x2, y2, x3, y3);
            //Zelda(custom,0, x1, y1, x2, y2, x3, y3);
        //debug.wireframe = false; //stop debuggings
    // Modular Utils Funcitons
        //if(!isKey("Space")){DrawBar(font);}
        //TextInfo();
        Fps(0, 0, font, Scaling(50));
        //ExitPromt(font);  
} 

int main(int arglenght, char** args)
{ 
    // Built-in "!default"
      // floating = true;
      // fullscreen = true;
      // vsync = true;
      // cursor = false;
      // transparent = true;
      // decorated = true;
      // visible = false;
      // fpslimit = 60;
      // fps = output; 
      // deltatime = output; 
      // mouse = {x,y} output; 
      // mousescroll = {x,y} output;
      // mousemoving = Bool output; 
      // camera.x = float;
      // camera.y = float;
      // camera.z = float;
      // camera.angle = angle in degres;
      // debug.input = true;
      // debug.wireframe = true;
      // debug.fps = true;
    WindowInit(1920, 1080, "Grafenic");
    custom = LoadShader("./res/shaders/default.vert","./res/shaders/custom.frag");
    //custom.hotreloading = true;//hotreloading for the shader or put this in the update "shader = ShaderHotReload(shader);" > in this case > "custom = ShaderHotReload(custom);"
    //pixelshaderdefault.hotreloading = true;// hot reload on the default pixel shader
    //fontshaderdefault.hotreloading = true;// hot reload on the default font shader
    font = LoadFont("./res/fonts/Monocraft.ttf");font.nearest = true;
    //font = LoadFont("/home/e/.local/share/fonts/Ocr/OCRA.ttf");
    //img = LoadImage("./res/images/Arch.png");
    ClearColor((Color){75, 75, 75,100});

    // Saving how many times you boot
        //char* path = "./data.txt";
        //char* boot = FileLoad(path);
        //boot = FileSave(path, text("%d", (textint(boot) + 1)));
        //printf("Booted times: %s\n", boot);
        ////FileClear(path); // clear all data
    // Audio inizialization
        //AudioInit();
        // Play a stream with audio
            //AudioPlay("./res/sounds/sound.wav");
        // Load a file edit and play
            //Sound* sound = SoundLoad("./res/sounds/sound.wav");
            //SetSoundPitchSemitones(sound,-24.0);
            //SoundPlay(sound);//SoundStop(sound);
    while (!WindowState())
    {
        WindowClear();
            update();
        WindowProcess();
        // GetPixel Pointed color "slow"
            //pixelColor = GetPixel(mouse.x, mouse.y);
        // Screenshot
            //if(isKey("F11")){
            //    SaveScreenshot("Screenshot.png", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            //    isKeyReset("F11");
            //}
    }
    WindowClose();
    DeleteShader(custom);
    return 0;
} 