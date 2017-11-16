#ifndef DEKOI_PLAYGROUND_TEST_RENDERER_H
#define DEKOI_PLAYGROUND_TEST_RENDERER_H

#include "test.h"

int createRenderer(Window *pWindow,
                   const RendererCreateInfo *pCreateInfo,
                   Renderer **ppRenderer);
void destroyRenderer(Window *pWindow,
                     Renderer *pRenderer);
int resizeRendererSurface(Renderer *pRenderer,
                          unsigned int width,
                          unsigned int height);

#endif /* DEKOI_PLAYGROUND_TEST_RENDERER_H */
