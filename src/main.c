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
        double speed;
        double clampz;
        clampz = (deltatime);
        //print(text("Cam lerp: %.5f\n", clampz));
        if (isKeyDown("LeftShift")) {
            speed = 0.15f;
        } else {
            speed = 0.05f;
        }
        if (isKeyDown("w")) {
            camera.y += (speed / 10) * clampz;
        }
        if (isKeyDown("s")) {
            camera.y -= (speed / 10) * clampz;
        }
        if (isKeyDown("a")) {
            camera.x += (speed / 10) * clampz;
        }
        if (isKeyDown("d")) {
            camera.x -= (speed / 10) * clampz;
        }
        if (isKeyDown("r")) {
            speed = 0.0f;
            camera.x = Lerp(camera.x, 0.0f, 0.0003f * clampz);
            camera.y = Lerp(camera.y, 0.0f, 0.0003f * clampz);
            camera.z = Lerp(camera.z, 1.0f, 0.0003f * clampz);
            camera.angle = Lerp(camera.angle, (GLfloat)0.0f, 0.0003f * clampz);
            mousescroll.y = 0.0f;
        } else {
            if (camera.z <= 0) {
                camera.z = Lerp(1.0, mousescroll.y * 0.1f, 0.0003f * clampz);
            } else {
                camera.z = Lerp(camera.z, mousescroll.y * 0.1f + 1.0f , 0.0003f * clampz);
            }
        }
        if (mousescroll.y <= 0) {mousescroll.y = 0;}
        if (isKeyDown("e")) {
           camera.angle += (speed / 1000) * clampz;
        } else if (isKeyDown("q")) {
           camera.angle -= (speed / 1000) * clampz;
        }
    // DrawImage
        //DrawImageShader(img,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0,custom);
        DrawImage(img,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0);
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
            //float lerpSpeed = 0.0003f * clampz;
            //const float speedcursor = 0.75f;
            //static float timer = 0.0f;
            //float circleSize = Lerp(0.0f, (float)Scaling(10), timer);
            //float circleborderSize = Lerp(0.0f, (float)Scaling(3), timer);
            //if (!isMouseButton(0)) {
            //    if (mousecursorx <= 0){
            //        mousecursorx = mouse.x;
            //    } 
            //    if(mousecursory <= 0) {
            //        mousecursory = mouse.y;
            //    }
            //    mousecursorx = Lerp(mousecursorx, mouse.x, lerpSpeed);
            //    mousecursory = Lerp(mousecursory, mouse.y, lerpSpeed);
            //    timer = Lerp(timer, 0.0f, lerpSpeed);
            //} else {
            //    mousecursorx = Lerp(mousecursorx, mouse.x, lerpSpeed);
            //    mousecursory = Lerp(mousecursory, mouse.y, lerpSpeed);
            //    timer += deltatime / (speedcursor * 1000.0f);
            //}
            //if (IsInside(mousecursorx, mousecursory, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 12) && isKey("Space")) {
            //    DrawCircle(mousecursorx, mousecursory, circleSize, PURPLE);
            //    DrawCircleBorder(mousecursorx, mousecursory, circleSize, circleborderSize, VIOLET);
            //} else {
            //    DrawCircle(mousecursorx, mousecursory, circleSize, VIOLET);
            //    DrawCircleBorder(mousecursorx, mousecursory, circleSize, circleborderSize, PURPLE);
            //}
    // Easing Lerp
        //int sizeball  = Lerp(0, 50,  Easing(Motion(1.0,0.5), "Linear"));
        //int positionx = Lerp(0, SCREEN_WIDTH,  Easing(Motion(1.0,1.0), "CubicInOut"));
        //int positiony = Lerp(0, SCREEN_HEIGHT, Easing(Motion(1.0,1.0), "CubicInOut"));
        //DrawCircle(positionx, positiony, sizeball, VIOLET);
    // DrawLine Examples "Cross Screen"
        //DrawLine(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Scaling(5), BLACK);
        //DrawLine(0, SCREEN_HEIGHT, SCREEN_WIDTH, 0, Scaling(5), BLACK);
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
    // Getting Pixel Pointed Color 
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
            //Triangle(x1,y1,x2,y2,x3,y3,0,custom);
            //Zelda(x1,y1,x2,y2,x3,y3,0,custom);
        //debug.wireframe = false; //stop debugging
    // Modular Utils Funcitons
        if(isKey("Space")){DrawBar(font);}
        //TextInfo();
        Fps(0, 0, font, Scaling(50));
        ExitPromt(font);  
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
      // mousescrolling = Bool output; 
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
    img = LoadImage("./res/images/Arch.png");
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
