#include "grafenic/init.c"
#include "grafenic/ui.c"

Font font;
Image img;
Shader custom;
Color pixelColor;

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
            camera2d.position.y += (speed / 10) * clampz;
        } else if (isKeyDown("s")) {
            camera2d.position.y -= (speed / 10) * clampz;
        }
        if (isKeyDown("a")) {
            camera2d.position.x += (speed / 10) * clampz;
        } else if (isKeyDown("d")) {
            camera2d.position.x -= (speed / 10) * clampz;
        }
        if (isKeyDown("r")) {
            speed = 0.0f;
            camera2d.position.x = Lerp(camera2d.position.x, 0.0f, 0.0003f * clampz);
            camera2d.position.y = Lerp(camera2d.position.y, 0.0f, 0.0003f * clampz);
            camera2d.position.z = Lerp(camera2d.position.z, 1.0f, 0.0003f * clampz);
            camera2d.angle = Lerp(camera2d.angle, (GLfloat)0.0f, 0.0003f * clampz);
            mousescroll.y = 0.0f;
        } else {
            if (camera2d.position.z <= 0) {
                camera2d.position.z = Lerp(1.0, mousescroll.y * 0.1f, 0.0003f * clampz);
            } else {
                camera2d.position.z = Lerp(camera2d.position.z, mousescroll.y * 0.1f + 1.0f , 0.0003f * clampz);
            }
        }
        if (mousescroll.y <= 0) {mousescroll.y = 0;}
        if (isKeyDown("e")) {
           camera2d.angle += (speed / 1000) * clampz;
        } else if (isKeyDown("q")) {
           camera2d.angle -= (speed / 1000) * clampz;
        }
    // DrawImage
        DrawImageShader(img,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,0,custom);
    // Experimental WorkInProgress
        //debug.wireframe = true; //debug single part
        // TriangleGL 
            //GLfloat x1 = SCREEN_WIDTH/2, y1 = 0;
            //GLfloat x2 = 0.0f, y2 = SCREEN_HEIGHT;
            //GLfloat x3 = SCREEN_WIDTH,  y3 = SCREEN_HEIGHT;
            //Triangle(x1,y1,x2,y2,x3,y3,0,custom);
            //Zelda(x1,y1,x2,y2,x3,y3,0,custom);
        //debug.wireframe = false; //stop debugging
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
    shaderdefault.hotreloading = true;// hot reload on the default pixel shader
    font = LoadFont("./res/fonts/Monocraft.ttf");font.nearest = true;
    img = LoadImage("./res/images/Test.png");
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
