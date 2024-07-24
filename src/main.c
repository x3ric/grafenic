#include "grafenic/init.c"
#include "grafenic/ui.c"

Font font;

void update(void){
    // Input Example
        //      Key                    MouseButton                      Info
        // isKey,isKeyReset  // isMouseButton,isMouseButtonReset  // key Bool
        // isKeyDown,isKeyUp // isMouseButtonDown,isMouseButtonUp // key Press/Relase 
        // isKeyPressed      //                                   // key Mantain 
        //if (isKeyPressed("A", 0.06f)) { // Repeat key every 0.06 seconds
        //    DrawCircle(mouse.x,mouse.y, Scaling(35), (Color){0, 0, 0, 75});
        //}
        //if(isKey("V")){ // Change Vsync state
        //    vsync = true;
        //} else {
        //    vsync = false;
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
    // Modular ui.c functions
        // Left Text Bar
            //int texts = 9;
            //DrawTextColumn(font,1,texts, text("V = vsync state: %s", vsync ? "ON" : "OFF"));
            //DrawTextColumn(font,2,texts, text("Esc = exitbar state: %s", isKey("Esc") ? "ON" : "OFF"));
            //DrawTextColumn(font,3,texts, text("Space = bar state: %s", !isKey("Space") ? "ON" : "OFF"));
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
      // camera.rx = rotation x;
      // camera.ry = rotation y;
      // camera.rz = rotation z;
      // debug.input = true;
      // debug.wireframe = true;
      // debug.fps = true;
    WindowInit(1920, 1080, "Grafenic");
    font = LoadFont("./res/fonts/Monocraft.ttf");font.nearest = true;
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
    }
    WindowClose();
    return 0;
} 
