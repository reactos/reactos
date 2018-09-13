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
///#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
    LocalFree(pv);
}

void* __cdecl malloc(size_t cb)
{
    // The semantics for malloc is the memory is not zero-inited.  We 
    // defy this on purpose.
    return(LocalAlloc(LPTR, cb));
}

void* __cdecl calloc(size_t cb, size_t size)
{
    return(LocalAlloc(LPTR, cb * size));
}

void* __cdecl realloc(void* p, size_t n)
{
    return (NULL == p) ? malloc(n) : LocalReAlloc((HLOCAL)p, n, LMEM_MOVEABLE | LMEM_ZEROINIT);
}

void __cdecl _exit (int code);

void __cdecl abort( void )
{
    _exit(3);
}

#ifdef _M_ALPHA
// stub for alpha ehunwind.obj
void __cdecl _write(void) {}
#endif


