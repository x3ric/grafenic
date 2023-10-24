#include <GL/glew.h>
#include <GL/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    GLFWwindow* w;
} Window;

Window window;

typedef struct {
    bool input;
    bool wireframe;
    bool fps;
} Debug;

Debug debug;

const char* TITLE = "\0";
int SCREEN_WIDTH;
int SCREEN_HEIGHT;
int WIDTH;
int HEIGHT;

GLubyte* framebuffer;

#include "grafenic/input.c"
#include "grafenic/utils.c"
#include "grafenic/audio.c"
#include "grafenic/draw/init.c"

double deltatime;
int fpslimit;
double fps;

void WindowFrames(){
    static double previousframetime = 0.0;
    double targetTime = 1.0 / fpslimit;
    deltatime = glfwGetTime();
    double elapsedTime = deltatime - previousframetime;
    if (fpslimit != 0) {
        while (elapsedTime < targetTime) {
            deltatime = glfwGetTime();
            elapsedTime = deltatime - previousframetime;
        }
    }
    if (elapsedTime != 0) {
        fps = 1.0 / elapsedTime;
    } else {
        fps = 0.0;
    }
    previousframetime = deltatime;
    if(debug.fps){
        print("FPS: %.0f\n",fps);
    }
}

void WindowClear() {
    if(framebuffer){free(framebuffer);}
    if(pixels){framebuffer = (GLubyte*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, 4 * sizeof(GLubyte));}
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    WindowFrames();
}

bool cursor = true;
bool oldcursor = true;
bool vsync = false;
bool oldvsync = false;
bool fullscreen = false;
bool oldfullscreen = false;
bool visible = true;
bool oldvisible = true;

void WindowProcess() {
    MouseInit();
    if (fullscreen != oldfullscreen) {
        if (fullscreen) {
            glfwSetWindowMonitor(window.w, glfwGetPrimaryMonitor(), 0, 0, SCREEN_WIDTH, SCREEN_WIDTH, 0);
        } else {
            glfwSetWindowMonitor(window.w, NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        }
        oldfullscreen = fullscreen;
    }
    if (cursor != oldcursor) {
        if (cursor) {
            glfwSetInputMode(window.w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window.w, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        oldcursor = cursor;
    }
    if (vsync != oldvsync) {
        if (vsync) {
            glfwSwapInterval(1);
        } else {
            glfwSwapInterval(0);
        }
        oldvsync = vsync;
    } 
    if (visible != oldvisible) {
        if (visible) {
            glfwShowWindow(window.w);
        } else {
            glfwHideWindow(window.w);
        }
        oldvisible = visible;
    }
    if(framebuffer){DrawPixels(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);}
    glfwSwapBuffers(window.w);
    glfwPollEvents();
}

const int MIN_PIXEL = 1;

void window_buffersize_callback(GLFWwindow* window, int width, int height)
{
    SCREEN_WIDTH = width < MIN_PIXEL ? MIN_PIXEL : width;
    SCREEN_HEIGHT = height < MIN_PIXEL ? MIN_PIXEL : height;
    //if(!pixels){glfwSetWindowAspectRatio(window.w, SCREEN_WIDTH, SCREEN_HEIGHT);}
    glViewport(0, 0, (GLsizei)SCREEN_WIDTH, (GLsizei)SCREEN_HEIGHT);
}

void update(void);

void window_refresh_callback(GLFWwindow* window)
{
    WindowClear();
    update();
    WindowProcess();   
}

bool floating = false;
bool transparent = false;
bool decorated = false;
int SAMPLES = 0;
int REFRESH_RATE = 0;

int WindowInit(int width, int height, const char* title)
{
    TITLE = title;
    WIDTH = width;
    HEIGHT = height;
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
    glfwSetErrorCallback(ErrorCallback);
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, SAMPLES);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    if (transparent) {
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE); 
    }
    if (!decorated) {
        glfwWindowHint(GLFW_DECORATED, GL_FALSE); 
    }
    if (floating) {
        glfwWindowHint(GLFW_RESIZABLE , GLFW_FALSE);
    } else {
        glfwWindowHint(GLFW_RESIZABLE , GLFW_TRUE);
    }
    glfwWindowHint(GLFW_REFRESH_RATE, REFRESH_RATE);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);//forced new opengl version
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window.w = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title, NULL, NULL);
    if (!window.w) {
        printf("Failed to open GLFW window.w\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window.w);
    glfwSetCharCallback(window.w, CharCallback);
    glfwSetKeyCallback(window.w, KeyCallback);
    glfwSetScrollCallback(window.w, ScrollCallback);
    glfwSetFramebufferSizeCallback(window.w, window_buffersize_callback);
    glfwSetWindowRefreshCallback(window.w, window_refresh_callback);
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));
    //glewExperimental=true;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
    }
    if (vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
    glfwSetInputMode(window.w, GLFW_STICKY_KEYS, GLFW_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_MULTISAMPLE);
    InitializeOpenGL();
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
    glfwSetKeyCallback(window.w, NULL);
    print("Exit\n");
    AudioStop();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(2, PBO);
    glDeleteProgram(fontshaderdefault.Program);
    glDeleteProgram(pixelshaderdefault.Program);
    stbi_image_free(img.data);
    free(framebuffer);
    framebuffer = NULL;
    glfwDestroyWindow(window.w);
    glfwTerminate();
}
