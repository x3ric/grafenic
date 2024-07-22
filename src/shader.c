#include "grafenic/init.c"

Font font;
Image img;
Shader custom;
Color pixelColor;

#include "grafenic/ui.c"

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
        DrawImageShader(img,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0,custom);
    // Getting Pixel Pointed Color 
        // Uncomment "GetPixel Pointed color" in main
        //int fontsize = Scaling(50);
        //const char* textContent = text("R:%d G:%d B:%d A:%d", pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a);
        //TextSize textSize = GetTextSize(font, fontsize, textContent);
        // DrawText(SCREEN_WIDTH/2-(textSize.width/2), 0, font, fontsize, textContent, WHITE);
    // Modular ui.c functions
        Fps(0, 0, font, Scaling(50));
        ExitPromt(font);  
} 

int main(int arglenght, char** args)
{ 
    WindowInit(1920, 1080, "Grafenic");
    custom = LoadShader("./res/shaders/default.vert","./res/shaders/custom.frag");
    custom.hotreloading = true;//hotreloading for the shader or put this in the update "shader = ShaderHotReload(shader);" > in this case > "custom = ShaderHotReload(custom);"
    pixelshaderdefault.hotreloading = true;// hot reload on the default pixel shader
    fontshaderdefault.hotreloading = true;// hot reload on the default font shader
    font = LoadFont("./res/fonts/Monocraft.ttf");font.nearest = true;
    img = LoadImage("./res/images/Arch.png");
    ClearColor((Color){75, 75, 75,100});
    while (!WindowState())
    {
        WindowClear();
            update();
        WindowProcess();
        // GetPixel Pointed color "slow"
            //pixelColor = GetPixel(mouse.x, mouse.y);
        // Screenshot
            if(isKey("F11")){
                SaveScreenshot("Screenshot.png", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                isKeyReset("F11");
            }
    }
    WindowClose();
    DeleteShader(custom);
    return 0;
} 
