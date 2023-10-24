#ifndef WINDOW_H
#define WINDOW_H

    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;
    GLubyte* framebuffer;
    GLuint framebufferTexture;

    double frametime;
    int fpslimit;
    double fps;

    #include "draw.h"

    bool cursor = true;
    bool oldcursor = true;

    bool vsync = false;
    bool oldvsync = false;

    bool fullscreen = false;
    bool oldfullscreen = false;

    void window_size_callback(GLFWwindow* window, int width, int height)
    {
        const int MIN_WIDTH = 1;
        const int MIN_HEIGHT = 1;
        SCREEN_WIDTH = width < MIN_WIDTH ? MIN_WIDTH : width;
        SCREEN_HEIGHT = height < MIN_HEIGHT ? MIN_HEIGHT : height;
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glfwSetWindowAspectRatio(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    int WindowInit(int width, int height, const char* title)
    {
        SCREEN_WIDTH = width;
        SCREEN_HEIGHT = height;
        glfwSetErrorCallback(ErrorCallback);
        if (!glfwInit()) {
            printf("Failed to initialize GLFW\n");
            return -1;
        }

        glfwWindowHint(GLFW_RESIZABLE , GLFW_FALSE); // Floating
        glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
        glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

        window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title, NULL, NULL);
        if (!window) {
            printf("Failed to open GLFW window.\n");
            glfwTerminate();
            return -1;
        }

        glfwMakeContextCurrent(window);

        glfwSetKeyCallback(window, KeyCallback);
        glfwSetScrollCallback(window, ScrollCallback);
        glfwSetWindowSizeCallback(window, window_size_callback);

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

        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //Textured FrameBuffer
            glGenTextures(1, &framebufferTexture);
            glBindTexture(GL_TEXTURE_2D, framebufferTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glBindTexture(GL_TEXTURE_2D, 0);

        print("Loaded\n");
        return 0;
    }

    int WindowState()
    {
        return glfwWindowShouldClose(window);
    }

    void WindowClear()
    {
        if (framebuffer) {
            free(framebuffer);
        }
        framebuffer = (GLubyte*)malloc((int)SCREEN_WIDTH * (int)SCREEN_HEIGHT * 4 * sizeof(GLubyte));
        memset(framebuffer, 0, (int)SCREEN_WIDTH * (int)SCREEN_HEIGHT * 4 * sizeof(GLubyte)); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        static double previousframetime = 0.0;
        double targetTime = 1.0 / fpslimit;
        frametime = glfwGetTime();
        double elapsedTime = frametime - previousframetime;
        if (fpslimit != 0) {
            while (elapsedTime < targetTime) {
                frametime = glfwGetTime();
                elapsedTime = frametime - previousframetime;
            }
        }
        if (elapsedTime != 0) {
            fps = 1.0 / elapsedTime;
        } else {
            fps = 0.0;
        }
        previousframetime = frametime;
    }

    void WindowStateSet(bool x)
    {
        if (x) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else  {
            glfwSetWindowShouldClose(window, GLFW_FALSE);            
        }
    }

    void DrawPixels(float x, float y, float width, float height) {
        OrthoCam(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
        
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);                   // Top-left
            glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y);          // Top-right
            glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y + height); // Bottom-right
            glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + height);         // Bottom-left
        glEnd();

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);  
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        
    }

    
    void WindowProcess() {
        if (fullscreen != oldfullscreen) {
            if (fullscreen) {
                glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, SCREEN_WIDTH, SCREEN_WIDTH, 0);
            } else {
                glfwSetWindowMonitor(window, NULL, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
            }
            oldfullscreen = fullscreen;
        }
        if (cursor != oldcursor) {
            if (cursor) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
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

        DrawPixels(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    void WindowClose()
    {
        glfwSetKeyCallback(window, NULL);
        print("Exit\n");
        stbi_image_free(img.data);
        free(framebuffer);
        framebuffer = NULL;
        glfwDestroyWindow(window);
        glfwTerminate();
    }

#endif // WINDOW_H