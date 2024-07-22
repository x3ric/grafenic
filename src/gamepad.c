#include "grafenic/init.c"

Font font;

void DrawGamepadInfo(int x, int *y, int jid) {
    char buffer[256];
    const char* joystickName = GetJoystickName(jid);
    if (!isGamepadConnected(jid))
        return; // Only proceed if the joystick is actually a gamepad
    // Display gamepad info
        snprintf(buffer, sizeof(buffer), "Gamepad %d (%s)", jid, joystickName);
        DrawText(x, *y, font, Scaling(24), buffer, WHITE);
        *y += Scaling(30);
    // Gamepad connected status
        DrawText(x, *y, font, Scaling(20), "  Status: Connected", GREEN);
        *y += Scaling(20);
    // Buttons information
        DrawText(x, *y, font, Scaling(22), "  Buttons:", WHITE);
        *y += Scaling(20);
        static const char* buttonNames[] = {
            "A", "B", "X", "Y", "LeftBumper", "RightBumper", "Back", "Start",
            "Guide", "LeftThumb", "RightThumb", "DpadUp", "DpadRight",
            "DpadDown", "DpadLeft", "Cross", "Circle", "Square", "Triangle"
        };
        for (int b = 0; b < sizeof(buttonNames) / sizeof(buttonNames[0]); b++) {
            snprintf(buffer, sizeof(buffer), "    %s: %s", buttonNames[b], isGamepadButtonDown(jid, buttonNames[b]) ? "Pressed" : "Released");
            DrawText(x, *y, font, Scaling(20), buffer, WHITE);
            *y += Scaling(20);
        }
    // Axes information
        DrawText(x, *y, font, Scaling(22), "  Axes:", WHITE);
        *y += Scaling(20);
        static const char* axisNames[] = {
            "LeftX", "LeftY", "RightX", "RightY", "LeftTrigger", "RightTrigger"
        };
        for (int a = 0; a < sizeof(axisNames) / sizeof(axisNames[0]); a++) {
            snprintf(buffer, sizeof(buffer), "    %s: %.2f", axisNames[a], GamepadAxis(jid, axisNames[a]));
            DrawText(x, *y, font, Scaling(20), buffer, WHITE);
            *y += Scaling(20);
        }
    // Extra spacing between gamepads
        *y += Scaling(30);
}

void update(void) {
    int x = 10;
    int y = 10;
    for (int i = 0; i < joystick_count; i++) {
        DrawGamepadInfo(x, &y, joysticks[i]);
    }
}

int main(int argc, char** args) { 
    WindowInit(1920, 1080, "Grafenic");
    font = LoadFont("./res/fonts/Monocraft.ttf");
    font.nearest = true;
    LoadJoysticks();
    ClearColor((Color){75, 75, 75, 100});
    while (!WindowState()) {
        WindowClear();
        update();
        WindowProcess();
    }
    WindowClose();
    return 0;
}
