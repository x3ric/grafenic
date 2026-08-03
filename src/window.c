#include "window.h"

Window window;

#include "input.c"
#include "utils.c"
#include "audio.c"
#include "render/draw.c"

void WindowFrames() {
    static double lastFPSTime = 0.0;
    static double previousFrameTime = 0.0;
    static int frameCount = 0;
    double currentTime = glfwGetTime();
    if (previousFrameTime == 0.0) previousFrameTime = currentTime;
    double deltaTime = currentTime - previousFrameTime;
    if (window.fpslimit > 0) {
        double targetTime = 1.0 / (double)window.fpslimit;
        double sleepTime = targetTime - deltaTime;
        if (sleepTime > 0.0) {
            struct timespec time = {
                .tv_sec = (time_t)sleepTime,
                .tv_nsec = (long)((sleepTime - (time_t)sleepTime) * 1.0e9)
            };
            while (nanosleep(&time, &time) != 0) {}
            currentTime = glfwGetTime();
            deltaTime = currentTime - previousFrameTime;
        }
    }
    window.deltatime = deltaTime;
    window.time = currentTime;
    previousFrameTime = currentTime;
    frameCount++;
    double elapsed = currentTime - lastFPSTime;
    if (elapsed >= 1.0) {
        window.fps = frameCount / elapsed;
        if (window.debug.fps) printf("FPS: %.0f\n", window.fps);
        frameCount = 0;
        lastFPSTime = currentTime;
    }
}

void WindowClear() {
    WindowFrames();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void WindowChecks() {
    static int windowedX = 0;
    static int windowedY = 0;
    static int windowedWidth = 0;
    static int windowedHeight = 0;
    mouse = MouseInit();
    if (window.opt.fullscreen != window.opt.oldfullscreen) {
        if (window.opt.fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : NULL;
            glfwGetWindowPos(window.w, &windowedX, &windowedY);
            glfwGetWindowSize(window.w, &windowedWidth, &windowedHeight);
            if (monitor && mode) glfwSetWindowMonitor(window.w, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            int width = windowedWidth > 0 ? windowedWidth : window.width;
            int height = windowedHeight > 0 ? windowedHeight : window.height;
            glfwSetWindowMonitor(window.w, NULL, windowedX, windowedY, width, height, GLFW_DONT_CARE);
        }
        window.opt.oldfullscreen = window.opt.fullscreen;
    }
    if (window.opt.hidecursor != window.opt.oldhidecursor || window.opt.disablecursor != window.opt.olddisablecursor) {
        int mode = GLFW_CURSOR_NORMAL;
        if (window.opt.hidecursor) mode = GLFW_CURSOR_HIDDEN;
        else if (window.opt.disablecursor) mode = GLFW_CURSOR_DISABLED;
        glfwSetInputMode(window.w, GLFW_CURSOR, mode);
        window.opt.oldhidecursor = window.opt.hidecursor;
        window.opt.olddisablecursor = window.opt.disablecursor;
    }
    if (window.opt.vsync != window.opt.oldvsync) {
        glfwSwapInterval(window.opt.vsync ? 1 : 0);
        window.opt.oldvsync = window.opt.vsync;
    }
    if (window.opt.hided != window.opt.oldhided) {
        if (window.opt.hided) glfwHideWindow(window.w);
        else glfwShowWindow(window.w);
        window.opt.oldhided = window.opt.hided;
    }
}

void WindowProcess() {
    WindowChecks();
    glfwSwapBuffers(window.w);
    glfwPollEvents();
}

void window_buffersize_callback(GLFWwindow* glfw_window, int width, int height) {
    (void)glfw_window;
    window.screen_width = MaxInt(width, 1);
    window.screen_height = MaxInt(height, 1);
    glViewport(0, 0, window.screen_width, window.screen_height);
}

int WindowInit(int width, int height, char* title) {
    if (width <= 0 || height <= 0 || !title) return -1;
    window.title = title;
    window.width = width;
    window.height = height;
    window.screen_width = width;
    window.screen_height = height;
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    glfwSetErrorCallback(ErrorCallback);
    glfwWindowHint(GLFW_SAMPLES, MaxInt(window.samples, 0));
    glfwWindowHint(GLFW_DEPTH_BITS, window.depthbits > 0 ? window.depthbits : 24);
    glfwWindowHint(GLFW_REFRESH_RATE, window.refresh_rate > 0 ? window.refresh_rate : GLFW_DONT_CARE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, window.opt.transparent ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, window.opt.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, window.opt.floating ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, window.opt.floating ? GLFW_TRUE : GLFW_FALSE);
    GLFWmonitor* monitor = window.opt.fullscreen ? glfwGetPrimaryMonitor() : NULL;
    window.w = glfwCreateWindow(width, height, title, monitor, NULL);
    if (!window.w) {
        printf("Failed to open GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window.w);
    glewExperimental = GL_TRUE;
    GLenum error = glewInit();
    if (error != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(error));
        glfwDestroyWindow(window.w);
        window.w = NULL;
        glfwTerminate();
        return -1;
    }
    glGetError();
    glfwSetCharCallback(window.w, CharCallback);
    glfwSetKeyCallback(window.w, KeyCallback);
    glfwSetScrollCallback(window.w, ScrollCallback);
    glfwSetFramebufferSizeCallback(window.w, window_buffersize_callback);
    glfwGetFramebufferSize(window.w, &window.screen_width, &window.screen_height);
    glViewport(0, 0, window.screen_width, window.screen_height);
    glfwSwapInterval(window.opt.vsync ? 1 : 0);
    window.opt.oldvsync = window.opt.vsync;
    window.opt.oldfullscreen = window.opt.fullscreen;
    window.opt.oldhided = !window.opt.hided;
    window.opt.oldhidecursor = !window.opt.hidecursor;
    window.opt.olddisablecursor = !window.opt.disablecursor;
    if (window.samples > 0) glEnable(GL_MULTISAMPLE);
    InitializeShader();
    LoadJoysticks();
    WindowChecks();
    if (!shaderdefault.Program || !shaderfont.Program) {
        TerminateShader();
        glfwDestroyWindow(window.w);
        window.w = NULL;
        glfwTerminate();
        return -1;
    }
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));
    print("Loaded\n");
    return 0;
}

int WindowState() {
    return window.w ? glfwWindowShouldClose(window.w) : true;
}

void WindowStateSet(bool state) {
    if (window.w) glfwSetWindowShouldClose(window.w, state ? GLFW_TRUE : GLFW_FALSE);
}

void WindowClose() {
    if (!window.w) return;
    print("Exit\n");
    AudioStop();
    TerminateShader();
    glfwDestroyWindow(window.w);
    window.w = NULL;
    glfwTerminate();
}
