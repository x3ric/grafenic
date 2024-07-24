#include "grafenic/init.c"
#include "grafenic/ui.c"
Font font;

Vec3 cube;

void update(void) {
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
            cube.x += (speed / 10) * clampz;
        }
        if (isKeyDown("s")) {
            cube.x -= (speed / 10) * clampz;
        }   
        if (isKeyDown("a")) {
            cube.y += (speed / 10) * clampz; 
        }
        if (isKeyDown("d")) {
            cube.y -= (speed / 10) * clampz;
        }
        if (isKeyDown("r")) {
            speed = 0.0f;
            cube.x = Lerp(cube.x, 0.0f, 0.0003f * clampz);
            cube.y = Lerp(cube.y, 0.0f, 0.0003f * clampz);
            cube.z = Lerp(cube.y, 0.0f, 0.0003f * clampz);
            mousescroll.y = 0.0f;
        } else {
            if (cube.z <= 0) {
                cube.z = Lerp(1.0, mousescroll.y * 0.1f, 0.0003f * clampz);
            } else {
                cube.z = Lerp(cube.z, mousescroll.y * 0.1f + 1.0f , 0.0003f * clampz);
            }
        }
        if (mousescroll.y <= 0) {mousescroll.y = 0;}
        if (isKeyDown("e")) {
           cube.z += (speed / 1000) * clampz;
        } else if (isKeyDown("q")) {
           cube.z -= (speed / 1000) * clampz;
        }
    // 3d envoiriment
        float angleX = deltatime * 0.2f * 100.0f;
        float angleY = deltatime * 0.3f * 100.0f;
        float angleZ = deltatime * 0.4f * 100.0f;
        Cube(0.0f, 0.0f, 0.0f, 1.0f, cube.x + angleX, cube.y + angleY, cube.z + angleZ, shaderdefault);
    // Modular ui.c functions
        Fps(0, 0, font, Scaling(50));
        ExitPromt(font);  
}

int main(int argc, char** argv) {
    WindowInit(1920, 1080, "Grafenic");
    font = LoadFont("./res/fonts/Monocraft.ttf");font.nearest = true;
    ClearColor((Color){75, 75, 75, 100});
    while (!WindowState()) {
        WindowClear();
        update();
        WindowProcess();
    }
    WindowClose();
    return 0;
}
