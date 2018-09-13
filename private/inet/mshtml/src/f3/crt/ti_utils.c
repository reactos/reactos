//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       ti_utils.c
//
//  Contents:   Helper utilities to satisfy link requirements for typinfo.obj
//
//----------------------------------------------------------------------------

#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4244)
#pragma warning(disable:4514)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

EXTERN_C HANDLE g_hProcessHeap;

static CRITICAL_SECTION g_critsecRTTI;
static BOOL             g_fInit = FALSE;

//
// utility function used below
//
void BreakHere()
{
#if DBG == 1
    #ifdef _M_IX86
        _asm int 3;
    #else
        DebugBreak();
    #endif // _M_IX86
#endif // DBG == 1
}

void __cdecl free(void *pv)
{
    if (pv)
    {
        HeapFree(g_hProcessHeap, 0, pv);
    }
}

void * __cdecl malloc(size_t cb)
{
    return HeapAlloc(g_hProcessHeap, 0, cb);
}

void __cdecl _lock (int locknum)
{
    if (locknum != 27)  // We should only be called by the Run-Time Type
    {                   //   Information code, which uses lock 27.
        BreakHere();
    }

    if (!g_fInit)
    {
        InitializeCriticalSection(&g_critsecRTTI);

        g_fInit = TRUE;
    }

    EnterCriticalSection(&g_critsecRTTI);
}

void __cdecl _unlock(int locknum)
{
    if (locknum != 27)  // We should only be called by the Run-Time Type
    {                   //   Information code, which uses lock 27.
        BreakHere();
    }

    LeaveCriticalSection(&g_critsecRTTI);
}

void lock_cleanup()
{
    if (g_fInit)
    {
        DeleteCriticalSection(&g_critsecRTTI);
    }
}

//
// These functions should never get called. If they do, it means the
// run-time type information code threw an exception, which should never
// happen in fm30.dll. Remove the code that caused it.
//
void __stdcall _CxxThrowException(long a, long b)
{
    a; b;
    BreakHere();
}

#ifdef _ALPHA_
void __cdecl _CxxFrameHandler()
#else
void __cdecl __CxxFrameHandler()
#endif
{
    BreakHere();
}

