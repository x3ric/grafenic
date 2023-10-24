#ifndef UTILS_H
#define UTILS_H

    #include <stdarg.h>
    #include <stdio.h>
    #include <stdlib.h>

    const char* text(const char* format, ...) {
        static char buffer[100];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return buffer;
    }

    void print(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    double GetTime() {
        return glfwGetTime();
    }

    void SetTime(double time) {
        glfwSetTime(time);
    }

    void ErrorCallback(int error, const char* description) {
        fprintf(stderr, "Error: %s\n", description);
    }

#endif // UTILS_H