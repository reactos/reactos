/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    resinfo.h

Abstract:

    Resource owner info header. Used for tracking resources in debug build

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

#if INET_DEBUG

//
// types
//

//typedef struct {
//    DWORD Tid;
//    DWORD CallersAddress;
//    DWORD CallersCaller;
//    DWORD SourceFileLine;
//    LPSTR SourceFileName;
//} RESOURCE_INFO, *LPRESOURCE_INFO;
typedef struct {
    DWORD Tid;
    LPSTR SourceFileName;
    DWORD SourceFileLine;
} RESOURCE_INFO, *LPRESOURCE_INFO;

//#define GET_RESOURCE_INFO(pResource) \
//    { \
//        (pResource)->Tid = GetCurrentThreadId(); \
//        (pResource)->CallersAddress = 0; \
//        (pResource)->CallersCaller = 0; \
//        (pResource)->SourceFileLine = __LINE__; \
//        (pResource)->SourceFileName = __FILE__; \
//    }
#define GET_RESOURCE_INFO(pResource) \
    { \
        (pResource)->Tid = GetCurrentThreadId(); \
        (pResource)->SourceFileName = __FILE__; \
        (pResource)->SourceFileLine = __LINE__; \
    }

//#define INITIALIZE_RESOURCE_INFO(pResource) \
//    { \
//        (pResource)->Tid = GetCurrentThreadId(); \
//        (pResource)->CallersAddress = 0; \
//        (pResource)->CallersCaller = 0; \
//        (pResource)->SourceFileLine = __LINE__; \
//        (pResource)->SourceFileName = __FILE__; \
//    }
#define INITIALIZE_RESOURCE_INFO(pResource) \
    { \
        (pResource)->Tid = GetCurrentThreadId(); \
        (pResource)->SourceFileName = __FILE__; \
        (pResource)->SourceFileLine = __LINE__; \
    }

#else

#define GET_RESOURCE_INFO(pResource)
#define INITIALIZE_RESOURCE_INFO(pResource)

#endif // INET_DEBUG
