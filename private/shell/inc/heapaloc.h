//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       heapaloc.h
//
//  Contents:   Macros which wrap the standard memory API calls, redirecting
//              them to HeapAlloc.
//
//  Functions:  __inline HLOCAL  HeapLocalAlloc   (fuFlags, cbBytes)
//              __inline HLOCAL  HeapLocalReAlloc (hMem, cbBytes, fuFlags)
//              __inline HLOCAL  HeapLocalFree    (HLOCAL hMem)
//
//  History:    2-01-95   davepl   Created
//              12-15-97  t-saml   changed to be used only for leak tracking
//
//--------------------------------------------------------------------------

#ifndef DEBUG
#define IMSAddToList(bAdd, pv, cb)
#else

// Function to add/remove from leak detection list
// In stocklib (shell\lib\debug.c)
STDAPI_(void) IMSAddToList(BOOL bAdd, void*pv, SIZE_T cb);
#ifdef _SHELLP_H_
// Function to call in allocspy.dll (GetShellMallocSpy)
typedef BOOL (__stdcall *PFNGSMS) (IShellMallocSpy **ppout);
#endif


#ifndef ASSERT
#define ASSERT Assert
#endif

#ifdef LocalAlloc
#error "HEAPALOC.H(42): LocalAlloc shouldn't be defined"
#endif

//
// These are functions normally in comctl32, but there's no good reason to call
// that dll, so handle them here.  Since Chicago may still want to use these
// shared memory routines, only "forward" them under NT.
//

#if defined(WINNT) && defined(_COMCTL32_)
#define Alloc(cb)                             HeapLocalAlloc(LMEM_ZEROINIT | LMEM_FIXED, cb)
#define ReAlloc(pb, cb)                       HeapLocalReAlloc(pb, cb, LMEM_ZEROINIT | LMEM_FIXED)
//
// Free() in comctl32 is just HeapFree(), so the return code reversing
// in HeapLocalFree is the opposite of what we want.  Reverse it
// again here for now, and consider redefining Free() as just
// HeapFree(g_hProcessHeap) if the compiler isn't smart enough
// to generate the same code already.  (BUGBUG investigate)
// REVIEW: who checks the return value from a free?  What do you do if it fails?
//
#define Free(pb)                              (!HeapLocalFree(pb))
#define GetSize(pb)                           HeapLocalSize(pb)
#endif


#if 0

// GlobalAllocs cannot be trivially replaced since they are used for DDE, OLE,
// and GDI operations.  However, on a case-by-case version we can switch them
// over to HeapGlobalAlloc as we identify instances that don't _really_ require
// GlobalAllocs.

#define GlobalAlloc(fuFlags, cbBytes)         HeapGlobalAlloc(fuFlags, cbBytes)
#define GlobalReAlloc(hMem, cbBytes, fuFlags) HeapGlobalReAlloc(hMem, cbBytes, fuFlags)
#define GlobalSize(hMem)                      HeapGlobalSize(hMem)
#define GlobalFree(hMem)                      HeapGlobalFree(hMem)
#define GlobalCompact                         InvalidMemoryCall
#define GlobalDiscard                         InvalidMemoryCall
#define GlobalFlags                           InvalidMemoryCall
#define GlobalHandle                          InvalidMemoryCall
#define GlobalLock                            InvalidMemoryCall
#define GlobalUnlock                          InvalidMemoryCall

#endif


__inline HLOCAL HeapLocalAlloc(IN UINT fuFlags, IN SIZE_T cbBytes)
{
    void * pv;

    pv = LocalAlloc(fuFlags, cbBytes);

    IMSAddToList(TRUE, pv, cbBytes); // Add to leak tracking

    return (HLOCAL) pv;
}

__inline HLOCAL HeapLocalFree(HLOCAL hMem)
{
    IMSAddToList(FALSE, hMem, 0); // Free leak tracking

    return LocalFree(hMem);
}

__inline HLOCAL HeapLocalReAlloc(IN HGLOBAL hMem,
                                 IN SIZE_T  cbBytes,
                                 IN UINT    fuFlags)
{
    void * pv;

    // BUGBUG (DavePl) Why can we realloc on a null ptr?

    if (NULL == hMem)
    {
        return LocalAlloc(fuFlags, cbBytes);
    }

    pv = LocalReAlloc((void *) hMem, cbBytes, fuFlags);

    IMSAddToList(FALSE, hMem, 0);    // Take out the old
    IMSAddToList(TRUE, pv, cbBytes);  // And bring in the new

    return (HGLOBAL) pv;
}

// Redefine the standard memory APIs to thunk over to our Heap-based funcs


#define LocalAlloc(fuFlags, cbBytes)          HeapLocalAlloc(fuFlags, cbBytes)
#define LocalReAlloc(hMem, cbBytes, fuFlags)  HeapLocalReAlloc(hMem, cbBytes, fuFlags)
#define LocalFree(hMem)                       HeapLocalFree(hMem)

#endif
