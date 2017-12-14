#ifndef DEKOI_PLAYGROUND_TEST_TEST_H
#define DEKOI_PLAYGROUND_TEST_TEST_H

#if !defined(_WIN32)                                                           \
    && (defined(__unix__) || defined(__unix)                                   \
        || (defined(__APPLE__) && defined(__MACH__)))
#define PL_PLATFORM_UNIX
#endif

#define PL_UNUSED(x) (void)(x)

typedef struct PlApplication PlApplication;
typedef struct PlApplicationCreateInfo PlApplicationCreateInfo;
typedef struct PlDekoiLoggingCallbacksData PlDekoiLoggingCallbacksData;
typedef struct PlLoggingCallbacks PlLoggingCallbacks;
typedef struct PlRenderer PlRenderer;
typedef struct PlRendererCreateInfo PlRendererCreateInfo;
typedef struct PlShaderCreateInfo PlShaderCreateInfo;
typedef struct PlWindow PlWindow;
typedef struct PlWindowCreateInfo PlWindowCreateInfo;

#endif /* DEKOI_PLAYGROUND_TEST_TEST_H */
