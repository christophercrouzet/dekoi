#ifndef DEKOI_PLAYGROUND_TEST_WINDOW_H
#define DEKOI_PLAYGROUND_TEST_WINDOW_H

#include "test.h"

typedef struct GLFWwindow GLFWwindow;

struct WindowCreateInfo {
    unsigned int width;
    unsigned int height;
    const char *title;
};

int createWindow(Application *pApplication,
                 const WindowCreateInfo *pCreateInfo,
                 Window **ppWindow);
void destroyWindow(Application *pApplication,
                   Window *pWindow);
void getWindowCloseFlag(const Window *pWindow,
                        int *pCloseFlag);
int pollWindowEvents(const Window *pWindow);

#endif /* DEKOI_PLAYGROUND_TEST_WINDOW_H */
