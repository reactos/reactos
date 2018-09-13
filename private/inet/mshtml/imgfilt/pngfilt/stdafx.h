// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef UNIX
#define _ATL_MIN_CRT
#endif

#define _ATL_NO_FLTUSED
#define _MERGE_PROXYSTUB
#define USE_IERT

#if DBG
#if !defined(_DEBUG)
#define _DEBUG
#endif /* !_DEBUG */
#define _ATL_NO_DEBUG_CRT   // Don't use ATL CRT stuff.  As a result, we need to define our own ASSERTE per altbase.h

#define _ASSERTE(expr) \
{ \
    if(!(expr)) \
    { \
        TCHAR sz[256]; \
        wsprintf(sz, TEXT("ASSERT PNGFILT: %s %d %s\n"), __FILE__, __LINE__, TEXT(#expr)); \
        OutputDebugString(sz); \
        DebugBreak(); \
    } \
} \

#define _ASSERT(expr) \
{ \
    if(!(expr)) \
    { \
        TCHAR sz[256]; \
        wsprintf(sz, TEXT("ASSERT PNGFILT: %s %d\n"), __FILE__, __LINE__); \
        OutputDebugString(sz); \
        DebugBreak(); \
    } \
} \

#else
#define _ASSERTE(expr) ((void)0)
#define _ASSERT(expr) ((void)0)
#endif

#include <ddraw.h>
#include "atlbase.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include "atlcom.h"

#include "imgutil.h"

extern "C" {
#ifdef UNIX
#  include "zlib/zlib.h"
#else
#  include "zlib\zlib.h"
#endif
}
