#include "window.h"

Window window;

#include "input.c"
#include "utils.c"
#include "audio.c"
#include "render/draw.c"

void WindowFrames() {
    static double __attribute__((aligned(16))) lastFPSTime = 0.0;
    static int __attribute__((aligned(16))) frameCount = 0;
    static double __attribute__((aligned(16))) previousFrameTime = 0.0;
    const double currentTime = glfwGetTime();
    window.deltatime = currentTime - previousFrameTime;
    window.time = currentTime;
    if (window.fpslimit > 0) {
        const double targetTime = 1.0 / window.fpslimit;
        const double sleepTime = targetTime - window.deltatime;
        if (sleepTime > 0.0) {
            const long nsec = (long)(sleepTime * 1.0e9);
            const struct timespec ts = {
                .tv_sec = 0,
                .tv_nsec = nsec
            };
            nanosleep(&ts, NULL);
        }
    }
    ++frameCount;
    const double timeElapsed = currentTime - lastFPSTime;
    if (timeElapsed >= 1.0) {
        window.fps = (double)frameCount / timeElapsed;
        if (window.debug.fps) {
            printf("FPS: %.0f\n", window.fps);
        }
        frameCount = 0;
        lastFPSTime = currentTime;
    }
    previousFrameTime = currentTime;
}

void WindowClear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    WindowFrames();
}

void WindowChecks() {
    mouse = MouseInit();
    if (window.opt.fullscreen != window.opt.oldfullscreen) {
        if (window.opt.fullscreen) {
            glfwSetWindowMonitor(window.w, glfwGetPrimaryMonitor(), 0, 0, window.screen_width, window.screen_width, 0);
        } else {
            glfwSetWindowMonitor(window.w, NULL, 0, 0, window.screen_width, window.screen_height, 0);
        }
        window.opt.oldfullscreen = window.opt.fullscreen;
    }
    if (window.opt.hidecursor != window.opt.oldhidecursor || window.opt.disablecursor != window.opt.olddisablecursor) {
        if (window.opt.hidecursor) {
            glfwSetInputMode(window.w, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        } else if (window.opt.disablecursor) {
            glfwSetInputMode(window.w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window.w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);            
        }
        window.opt.oldhidecursor = window.opt.hidecursor;
        window.opt.disablecursor = window.opt.disablecursor;
    }
    if (window.opt.vsync != window.opt.oldvsync) {
        if (window.opt.vsync) {
            glfwSwapInterval(1);
        } else {
            glfwSwapInterval(0);
        }
        window.opt.oldvsync = window.opt.vsync;
    } 
    if (window.opt.hided != window.opt.oldhided) {
        if (!window.opt.hided) {
            glfwShowWindow(window.w);
        } else {
            glfwHideWindow(window.w);
        }
        window.opt.oldhided = window.opt.hided;
    }
}

void WindowProcess() {
    WindowChecks();
    glfwSwapBuffers(window.w);
    glfwPollEvents();
}

void window_buffersize_callback(GLFWwindow* glfw_window, int width, int height)
{
    const int MIN_PIXEL = 1;
    window.screen_width = width < MIN_PIXEL ? MIN_PIXEL : width;
    window.screen_height = height < MIN_PIXEL ? MIN_PIXEL : height;
    glViewport(0, 0, (GLsizei)window.screen_width, (GLsizei)window.screen_height);
}

int WindowInit(int width, int height, char* title)
{
    window.title = title;
    window.width = width;
    window.height = height;
    window.screen_width = width;
    window.screen_height = height;
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, window.samples);
    //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    if (window.opt.transparent) {
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE); 
    }
    if (!window.opt.decorated) {
        glfwWindowHint(GLFW_DECORATED, GL_FALSE); 
    }
    if (window.opt.floating) {
        glfwWindowHint(GLFW_RESIZABLE , GLFW_FALSE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE , GLFW_TRUE);
    }
    glfwWindowHint(GLFW_REFRESH_RATE, window.refresh_rate);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);//forced new opengl version
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window.w = glfwCreateWindow(window.screen_width, window.screen_height, window.title, NULL, NULL);
    if (!window.w) {
        printf("Failed to open GLFW window.w\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window.w);
    glfwSetErrorCallback(ErrorCallback);
    glfwSetCharCallback(window.w, CharCallback);
    glfwSetKeyCallback(window.w, KeyCallback);
    glfwSetScrollCallback(window.w, ScrollCallback);
    glfwSetFramebufferSizeCallback(window.w, window_buffersize_callback);
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
    }
    if (window.opt.vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
    //glfwSetInputMode(window.w, GLFW_STICKY_KEYS, GLFW_TRUE);
    glEnable(GL_MULTISAMPLE);
    if(window.depthbits != 0) {
        glfwWindowHint(GLFW_DEPTH_BITS, window.depthbits);
    } else {
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
    }
    InitializeShader();
    print("Loaded\n");
    return 0;
}

int WindowState() {
    return glfwWindowShouldClose(window.w);
}

void WindowStateSet(bool state) {
    if (state) {
        glfwSetWindowShouldClose(window.w, GLFW_TRUE);
    } else  {
        glfwSetWindowShouldClose(window.w, GLFW_FALSE);            
    }
}

void WindowClose()
{
    print("Exit\n");
    AudioStop();
    TerminateShader();
    glfwDestroyWindow(window.w);
    glfwTerminate();
}
