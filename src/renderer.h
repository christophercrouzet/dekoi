#ifndef DEKOI_RENDERER_H
#define DEKOI_RENDERER_H

#include "dekoi.h"

struct DkRendererCreateInfo {
    void *pDummy;
};

void dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                      const DkAllocator *pAllocator,
                      DkRenderer **ppRenderer);
void dkDestroyRenderer(DkRenderer *pRenderer);

#endif /* DEKOI_RENDERER_H */
