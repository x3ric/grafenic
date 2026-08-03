
typedef struct {
    const char* name;
    int key;
} KeyName;

static const KeyName keyNames[] = {
    // Keypad
    {"KpDecimal", GLFW_KEY_KP_DECIMAL}, {"KpDivide", GLFW_KEY_KP_DIVIDE},
    {"KpMultiply", GLFW_KEY_KP_MULTIPLY}, {"KpSubtract", GLFW_KEY_KP_SUBTRACT},
    {"KpAdd", GLFW_KEY_KP_ADD}, {"KpEnter", GLFW_KEY_KP_ENTER}, {"KpEqual", GLFW_KEY_KP_EQUAL},
    {"Kp0", GLFW_KEY_KP_0}, {"Kp1", GLFW_KEY_KP_1}, {"Kp2", GLFW_KEY_KP_2},
    {"Kp3", GLFW_KEY_KP_3}, {"Kp4", GLFW_KEY_KP_4}, {"Kp5", GLFW_KEY_KP_5},
    {"Kp6", GLFW_KEY_KP_6}, {"Kp7", GLFW_KEY_KP_7}, {"Kp8", GLFW_KEY_KP_8}, {"Kp9", GLFW_KEY_KP_9},
    // Alphanumeric keys
    {"Space", GLFW_KEY_SPACE}, {"Apostrophe", GLFW_KEY_APOSTROPHE}, {"Comma", GLFW_KEY_COMMA},
    {"Add", GLFW_KEY_KP_ADD}, {"Minus", GLFW_KEY_MINUS}, {"Period", GLFW_KEY_PERIOD},
    {"Slash", GLFW_KEY_SLASH}, {"Semicolon", GLFW_KEY_SEMICOLON}, {"Equal", GLFW_KEY_EQUAL},
    {"LeftBracket", GLFW_KEY_LEFT_BRACKET}, {"Backslash", GLFW_KEY_BACKSLASH},
    {"Brackslash", GLFW_KEY_BACKSLASH}, {"RightBracket", GLFW_KEY_RIGHT_BRACKET},
    {"Grave", GLFW_KEY_GRAVE_ACCENT}, {"Esc", GLFW_KEY_ESCAPE}, {"Enter", GLFW_KEY_ENTER},
    {"Tab", GLFW_KEY_TAB}, {"Backspace", GLFW_KEY_BACKSPACE}, {"Insert", GLFW_KEY_INSERT},
    {"Delete", GLFW_KEY_DELETE}, {"Right", GLFW_KEY_RIGHT}, {"Left", GLFW_KEY_LEFT},
    {"Down", GLFW_KEY_DOWN}, {"Up", GLFW_KEY_UP}, {"PageUp", GLFW_KEY_PAGE_UP},
    {"PageDown", GLFW_KEY_PAGE_DOWN}, {"Home", GLFW_KEY_HOME}, {"End", GLFW_KEY_END},
    {"CapsLock", GLFW_KEY_CAPS_LOCK}, {"ScrollLock", GLFW_KEY_SCROLL_LOCK},
    {"NumLock", GLFW_KEY_NUM_LOCK}, {"PrintScreen", GLFW_KEY_PRINT_SCREEN}, {"Pause", GLFW_KEY_PAUSE},
    // Modifier keys
    {"LeftShift", GLFW_KEY_LEFT_SHIFT}, {"RightShift", GLFW_KEY_RIGHT_SHIFT},
    {"LeftControl", GLFW_KEY_LEFT_CONTROL}, {"RightControl", GLFW_KEY_RIGHT_CONTROL},
    {"LeftAlt", GLFW_KEY_LEFT_ALT}, {"RightAlt", GLFW_KEY_RIGHT_ALT},
    {"LeftSuper", GLFW_KEY_LEFT_SUPER}, {"RightSuper", GLFW_KEY_RIGHT_SUPER}, {"Menu", GLFW_KEY_MENU},
    // Mouse Keys
    {"Mouse1", GLFW_MOUSE_BUTTON_1}, {"Mouse2", GLFW_MOUSE_BUTTON_2},
    {"Mouse3", GLFW_MOUSE_BUTTON_3}, {"Mouse4", GLFW_MOUSE_BUTTON_4},
    {"Mouse5", GLFW_MOUSE_BUTTON_5}, {"Mouse6", GLFW_MOUSE_BUTTON_6},
    {"Mouse7", GLFW_MOUSE_BUTTON_7}, {"Mouse8", GLFW_MOUSE_BUTTON_8},
    // Gamepad Keys
    {"GamepadA", GLFW_GAMEPAD_BUTTON_A}, {"GamepadB", GLFW_GAMEPAD_BUTTON_B},
    {"GamepadX", GLFW_GAMEPAD_BUTTON_X}, {"GamepadY", GLFW_GAMEPAD_BUTTON_Y},
    {"LeftBumper", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER}, {"RightBumper", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER},
    {"Back", GLFW_GAMEPAD_BUTTON_BACK}, {"Start", GLFW_GAMEPAD_BUTTON_START}, {"Guide", GLFW_GAMEPAD_BUTTON_GUIDE},
    {"LeftThumb", GLFW_GAMEPAD_BUTTON_LEFT_THUMB}, {"RightThumb", GLFW_GAMEPAD_BUTTON_RIGHT_THUMB},
    {"L3", GLFW_GAMEPAD_BUTTON_LEFT_THUMB}, {"R3", GLFW_GAMEPAD_BUTTON_RIGHT_THUMB},
    {"DpadUp", GLFW_GAMEPAD_BUTTON_DPAD_UP}, {"DpadRight", GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"DpadDown", GLFW_GAMEPAD_BUTTON_DPAD_DOWN}, {"DpadLeft", GLFW_GAMEPAD_BUTTON_DPAD_LEFT},
    {"Cross", GLFW_GAMEPAD_BUTTON_CROSS}, {"Circle", GLFW_GAMEPAD_BUTTON_CIRCLE},
    {"Square", GLFW_GAMEPAD_BUTTON_SQUARE}, {"Triangle", GLFW_GAMEPAD_BUTTON_TRIANGLE}
};

int KeyChar(const char* character) {
    if (!character || !character[0]) return GLFW_KEY_UNKNOWN;
    if (!character[1]) {
        unsigned char ch = (unsigned char)character[0];
        if (ch >= 'A' && ch <= 'Z') return GLFW_KEY_A + ch - 'A';
        if (ch >= 'a' && ch <= 'z') return GLFW_KEY_A + ch - 'a';
        if (ch >= '0' && ch <= '9') return GLFW_KEY_0 + ch - '0';
        switch (ch) {
            case ' ': return GLFW_KEY_SPACE;
            case '\'': return GLFW_KEY_APOSTROPHE;
            case ',': return GLFW_KEY_COMMA;
            case '-': return GLFW_KEY_MINUS;
            case '.': return GLFW_KEY_PERIOD;
            case '/': return GLFW_KEY_SLASH;
            case ';': return GLFW_KEY_SEMICOLON;
            case '=': case '+': return GLFW_KEY_EQUAL;
            case '[': return GLFW_KEY_LEFT_BRACKET;
            case '\\': return GLFW_KEY_BACKSLASH;
            case ']': return GLFW_KEY_RIGHT_BRACKET;
            case '`': return GLFW_KEY_GRAVE_ACCENT;
            case '*': return GLFW_KEY_KP_MULTIPLY;
            default: return GLFW_KEY_UNKNOWN;
        }
    }
    if (character[0] == 'F' && isdigit((unsigned char)character[1])) {
        char* end = NULL;
        long number = strtol(character + 1, &end, 10);
        if (*end == '\0' && number >= 1 && number <= 25) return GLFW_KEY_F1 + (int)number - 1;
    }
    for (size_t i = 0; i < sizeof(keyNames) / sizeof(keyNames[0]); i++) {
        if (strcmp(character, keyNames[i].name) == 0) return keyNames[i].key;
    }
    return GLFW_KEY_UNKNOWN;
}

// KEYS

static int lastState[GLFW_KEY_LAST + 1] = {0};
static int toggleState[GLFW_KEY_LAST + 1] = {0};
static bool pressedState[GLFW_KEY_LAST + 1] = {0};
static double nextPressTime[GLFW_KEY_LAST + 1] = {0};

int isKeyDown(const char* character) {
    int key = KeyChar(character);
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return glfwGetKey(window.w, key) == GLFW_PRESS;
}

int isKeyUp(const char* character) {
    int key = KeyChar(character);
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return glfwGetKey(window.w, key) == GLFW_RELEASE;
}

bool isKeyPressed(const char* character, double interval) {
    int key = KeyChar(character);
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    bool down = glfwGetKey(window.w, key) == GLFW_PRESS;
    double currentTime = glfwGetTime();
    if (!down) {
        pressedState[key] = false;
        return false;
    }
    if (!pressedState[key]) {
        pressedState[key] = true;
        nextPressTime[key] = currentTime + interval;
        return true;
    }
    if (interval > 0.0 && currentTime >= nextPressTime[key]) {
        nextPressTime[key] = currentTime + interval;
        return true;
    }
    return false;
}

int isKey(const char* character) {
    int key = KeyChar(character);
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    int currentState = glfwGetKey(window.w, key);
    if (currentState == GLFW_PRESS && lastState[key] == GLFW_RELEASE) toggleState[key] = !toggleState[key];
    lastState[key] = currentState;
    return toggleState[key];
}

void isKeyReset(const char* character) {
    int key = KeyChar(character);
    if (key >= 0 && key <= GLFW_KEY_LAST) toggleState[key] = 0;
}

char lastPressedChar = '\0';

void CharCallback(GLFWwindow* glfw_window, unsigned int codepoint) {
    (void)glfw_window;
    if (codepoint < 128) lastPressedChar = (char)codepoint;
}

void KeyCallback(GLFWwindow* glfw_window, int key, int scancode, int action, int mods) {
    (void)glfw_window;
    if (!window.debug.input) return;
    static const char* actionStrings[] = {"RELEASED", "PRESSED", "REPEATED"};
    const char* actionString = action >= GLFW_RELEASE && action <= GLFW_REPEAT ? actionStrings[action] : "UNKNOWN";
    char modString[64] = {0};
    if (mods & GLFW_MOD_SHIFT) strcat(modString, "Shift+");
    if (mods & GLFW_MOD_CONTROL) strcat(modString, "Ctrl+");
    if (mods & GLFW_MOD_ALT) strcat(modString, "Alt+");
    if (mods & GLFW_MOD_SUPER) strcat(modString, "Mod+");
    const char* keyName = glfwGetKeyName(key, scancode);
    if (!keyName) keyName = "Unknown";
    char keyNameBuffer[64];
    snprintf(keyNameBuffer, sizeof(keyNameBuffer), "%s", keyName);
    if (mods & GLFW_MOD_SHIFT) {
        for (int i = 0; keyNameBuffer[i]; i++) keyNameBuffer[i] = (char)toupper((unsigned char)keyNameBuffer[i]);
    }
    printf("%s[%s] %s %d %d %c\n", modString, keyNameBuffer, actionString, scancode, key, lastPressedChar);
}

// MOUSE

Mouse mouse;

Mouse MouseInit() {
    double x, y;
    glfwGetCursorPos(window.w, &x, &y);
    bool moved = x != mouse.x || y != mouse.y;
    mouse.lastx = mouse.x;
    mouse.lasty = mouse.y;
    mouse.x = x;
    mouse.y = y;
    mouse.moving = moved;
    mouse.scroll.scrolling = mouse.scroll.x != mouse.scroll.lastx || mouse.scroll.y != mouse.scroll.lasty;
    mouse.scroll.lastx = mouse.scroll.x;
    mouse.scroll.lasty = mouse.scroll.y;
    return mouse;
}

void ScrollCallback(GLFWwindow* glfw_window, double xoffset, double yoffset) {
    (void)glfw_window;
    mouse.scroll.x += xoffset;
    mouse.scroll.y += yoffset;
}

void SetCursorPos(float x, float y) {
    glfwSetCursorPos(window.w, x, y);
}

static int mouseLastState[GLFW_MOUSE_BUTTON_LAST + 1] = {0};
static int mouseToggleState[GLFW_MOUSE_BUTTON_LAST + 1] = {0};

int isMouseButtonDown(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return glfwGetMouseButton(window.w, button) == GLFW_PRESS;
}

int isMouseButtonUp(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return glfwGetMouseButton(window.w, button) == GLFW_RELEASE;
}

int isMouseButton(const int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    int currentState = glfwGetMouseButton(window.w, button);
    if (currentState == GLFW_PRESS && mouseLastState[button] == GLFW_RELEASE) mouseToggleState[button] = !mouseToggleState[button];
    mouseLastState[button] = currentState;
    return mouseToggleState[button];
}

void isMouseButtonReset(const int button) {
    if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST) mouseToggleState[button] = 0;
}

// GAMEPAD & JOYSTICK

JoystickManager joystickManager;
static int gamepadLastState[GLFW_GAMEPAD_BUTTON_LAST + 1] = {0};
static int gamepadToggleState[GLFW_GAMEPAD_BUTTON_LAST + 1] = {0};

JoystickManager GetJoysticks(void) {
    return joystickManager;
}

static void joystick_callback(int jid, int event) {
    if (event == GLFW_CONNECTED) {
        for (int i = 0; i < joystickManager.count; i++) {
            if (joystickManager.joysticks[i] == jid) return;
        }
        if (joystickManager.count < MAX_JOYSTICKS) joystickManager.joysticks[joystickManager.count++] = jid;
        return;
    }
    if (event == GLFW_DISCONNECTED) {
        for (int i = 0; i < joystickManager.count; i++) {
            if (joystickManager.joysticks[i] != jid) continue;
            joystickManager.joysticks[i] = joystickManager.joysticks[--joystickManager.count];
            break;
        }
    }
}

void LoadJoysticks(void) {
    joystickManager.count = 0;
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST && joystickManager.count < MAX_JOYSTICKS; jid++) {
        if (glfwJoystickPresent(jid)) joystickManager.joysticks[joystickManager.count++] = jid;
    }
    glfwSetJoystickCallback(joystick_callback);
}

const char* GetJoystickName(int jid) {
    const char* name = glfwGetJoystickName(jid);
    return name ? name : "Unknown";
}

bool IsGamepadConnected(int gamepadId) {
    return gamepadId >= GLFW_JOYSTICK_1 && gamepadId <= GLFW_JOYSTICK_LAST && glfwJoystickPresent(gamepadId) && glfwJoystickIsGamepad(gamepadId);
}

int IsGamepadButtonDown(int gamepadId, const char* buttonName) {
    if (!IsGamepadConnected(gamepadId)) return false;
    int button = KeyChar(buttonName);
    if (button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
    GLFWgamepadstate state;
    return glfwGetGamepadState(gamepadId, &state) && state.buttons[button] == GLFW_PRESS;
}

int IsGamepadButtonUp(int gamepadId, const char* buttonName) {
    if (!IsGamepadConnected(gamepadId)) return false;
    int button = KeyChar(buttonName);
    if (button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
    GLFWgamepadstate state;
    return glfwGetGamepadState(gamepadId, &state) && state.buttons[button] == GLFW_RELEASE;
}

int IsGamepadButton(const char* character) {
    int button = KeyChar(character);
    if (button < 0 || button > GLFW_GAMEPAD_BUTTON_LAST) return false;
    int gamepadId = -1;
    for (int i = 0; i < joystickManager.count; i++) {
        if (IsGamepadConnected(joystickManager.joysticks[i])) {
            gamepadId = joystickManager.joysticks[i];
            break;
        }
    }
    if (gamepadId < 0) return false;
    GLFWgamepadstate state;
    if (!glfwGetGamepadState(gamepadId, &state)) return false;
    int currentState = state.buttons[button];
    if (currentState == GLFW_PRESS && gamepadLastState[button] == GLFW_RELEASE) gamepadToggleState[button] = !gamepadToggleState[button];
    gamepadLastState[button] = currentState;
    return gamepadToggleState[button];
}

void ResetGamepadButton(const char* character) {
    int button = KeyChar(character);
    if (button >= 0 && button <= GLFW_GAMEPAD_BUTTON_LAST) gamepadToggleState[button] = 0;
}

int GetGamepadAxisValue(const char* axisName) {
    if (!axisName) return -1;
    static const KeyName axes[] = {
        {"LeftX", GLFW_GAMEPAD_AXIS_LEFT_X}, {"LeftY", GLFW_GAMEPAD_AXIS_LEFT_Y},
        {"RightX", GLFW_GAMEPAD_AXIS_RIGHT_X}, {"RightY", GLFW_GAMEPAD_AXIS_RIGHT_Y},
        {"LeftTrigger", GLFW_GAMEPAD_AXIS_LEFT_TRIGGER}, {"RightTrigger", GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER}
    };
    for (size_t i = 0; i < sizeof(axes) / sizeof(axes[0]); i++) {
        if (strcmp(axisName, axes[i].name) == 0) return axes[i].key;
    }
    return -1;
}

float GetGamepadAxis(int gamepadId, const char* axisName) {
    if (!IsGamepadConnected(gamepadId)) return 0.0f;
    int axis = GetGamepadAxisValue(axisName);
    if (axis < 0 || axis > GLFW_GAMEPAD_AXIS_LAST) return 0.0f;
    GLFWgamepadstate state;
    if (!glfwGetGamepadState(gamepadId, &state)) return 0.0f;
    return state.axes[axis];
}
