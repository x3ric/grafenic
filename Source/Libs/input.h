#ifndef INPUT_H
#define INPUT_H
    
    #include <stdbool.h>
    #include <string.h>
    
    GLFWwindow* window;

    int KeyChar(const char* character) {
        if (!character) {
            return GLFW_KEY_UNKNOWN;
        }
        if (strcmp(character, "Esc") == 0) {
            return GLFW_KEY_ESCAPE;
        }
        if (strlen(character) == 1) {
            char ch = character[0];
            if (ch >= 'A' && ch <= 'Z') {
                return GLFW_KEY_A + (ch - 'A');
            }
            if (ch >= 'a' && ch <= 'z') {
                return GLFW_KEY_A + (ch - 'a');
            }
            if (ch >= '0' && ch <= '9') {
                return GLFW_KEY_0 + (ch - '0');
            }
        }
        return GLFW_KEY_UNKNOWN;
    }

    int isKeyDown(const char* character) {
        int key = KeyChar(character);
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    int isKeyUp(const char* character) {
        int key = KeyChar(character);
        return glfwGetKey(window, key) == GLFW_RELEASE;
    }

    static int lastState[GLFW_KEY_LAST + 1] = {0};
    static int toggleState[GLFW_KEY_LAST + 1] = {0};

    int isKey(const char* character) {
        int key = KeyChar(character);
        if (key == GLFW_KEY_UNKNOWN || key > GLFW_KEY_LAST) {
            return false;
        }
        if (isKeyDown(character) && lastState[key] == GLFW_RELEASE) {
            toggleState[key] = !toggleState[key];
        }
        lastState[key] = glfwGetKey(window, key);
        return toggleState[key];
    }

    void isKeyReset(const char* character) {
        int key = KeyChar(character);
        if (key != GLFW_KEY_UNKNOWN && key <= GLFW_KEY_LAST) {
            toggleState[key] = 0;
        }
    }

    typedef struct {
        double x, y;
    } Mouse;

    Mouse GetMousePos() {
        Mouse mouse = {0,0};
        glfwGetCursorPos(window, &mouse.x, &mouse.y);
        return (Mouse){mouse.x, mouse.y};
    }

    static int mouseLastState[GLFW_MOUSE_BUTTON_LAST + 1] = {0};
    static int mouseToggleState[GLFW_MOUSE_BUTTON_LAST + 1] = {0};

    int isMouseButtonDown(int button) {
        return glfwGetMouseButton(window, button) == GLFW_PRESS;
    }

    int isMouseButtonUp(int button) {
        return glfwGetMouseButton(window, button) == GLFW_RELEASE;
    }

    int isMouseButton(const int button) {
        int currentState = glfwGetMouseButton(window, button);
        if (currentState == GLFW_PRESS && mouseLastState[button] == GLFW_RELEASE) {
            mouseToggleState[button] = !mouseToggleState[button];
        }
        mouseLastState[button] = currentState;
        return mouseToggleState[button];
    }

    void isMouseButtonReset(const int button) {
        if (button <= GLFW_MOUSE_BUTTON_LAST) {
            mouseToggleState[button] = 0;
        }
    }

    typedef struct {
        double x, y;
    } MouseScroll;

    static MouseScroll currentScroll = {0.0, 0.0};

    MouseScroll GetScroll() {
        return currentScroll;
    }

    void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        currentScroll.x += xoffset;
        currentScroll.y += yoffset;
    }

    void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS) {
            printf("Key initially pressed: %d\n", key);
        } else if (action == GLFW_REPEAT) {
            printf("Key repeat: %d\n", key);
        }
    }

#endif // INPUT_H