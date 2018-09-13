/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _MEMUTIL_HXX
#define _MEMUTIL_HXX

extern TAG tagDebugMemory;

EXTERN_C DLLEXPORT void * MemAlloc(size_t cb);
EXTERN_C DLLEXPORT void MemFree(void *pv);
EXTERN_C void * MemAllocNe(size_t cb);

EXTERN_C DLLEXPORT void *  MemAllocClear(size_t cb);
EXTERN_C DLLEXPORT void *  MemAllocObject(size_t cb);

EXTERN_C HANDLE g_hProcessHeap;

enum NewNoException { NewNoExceptionEnum };

inline void * __cdecl operator new(size_t cb, NewNoException)
        { return MemAllocNe(cb); }

#define new_ne new(NewNoExceptionEnum)

inline void * __cdecl operator new(size_t cb)
        { return MemAlloc(cb); }

inline void * __cdecl operator new(size_t cb, void * p)
        { return p; }

inline void __cdecl operator delete(void *pv)
        { MemFree(pv); }

#if DBG==1 || defined(MEMSTRESS_ENABLE)
void SeedMemAllocFail( long nSeed, unsigned long ulThresh);
void DisableMemAllocFail();
#endif

extern "C"
{

#ifndef _DEBUG

    #define DbgPreAlloc(cb)             cb
    #define DbgPostAlloc(pv)            pv
    #define DbgPreFree(pv)              pv
    #define DbgPostFree()
    #define DbgPreRealloc(pv, cb, ppv)  cb
    #define DbgPostRealloc(pv)          pv
    #define DbgPreGetSize(pv)           pv
    #define DbgPostGetSize(cb)          cb
    #define DbgPreDidAlloc(pv)          pv
    #define DbgPostDidAlloc(pv, fAct)   fAct
    #define DbgRegisterMallocSpy()
    #define DbgRevokeMallocSpy()
    #define DbgMemoryTrackDisable(fb)
    #define DbgMemoryBlockTrackDisable(pv)
    #define DbgDumpProcessHeaps()
    #define DbgDumpMemory()
    #define DbgTotalAllocated()
    #define PointerRequestFromActual(pv) pv

    #define CHECK_HEAP()

#else

    size_t  __stdcall DbgPreAlloc(size_t cb);
    void *  __stdcall DbgPostAlloc(void *pv);
    void *  __stdcall DbgPreFree(void *pv);
    void    __stdcall DbgPostFree(void);
    size_t  __stdcall DbgPreRealloc(void *pv, size_t cb, void **ppv);
    void *  __stdcall DbgPostRealloc(void *pv);
    void *  __stdcall DbgPreGetSize(void *pv);
    size_t  __stdcall DbgPostGetSize(size_t cb);
    void *  __stdcall DbgPreDidAlloc(void *pv);
    BOOL    __stdcall DbgPostDidAlloc(void *pv, BOOL fActual);

    void    __stdcall DbgRegisterMallocSpy(void);
    void    __stdcall DbgRevokeMallocSpy(void);
    void    __stdcall DbgMemoryTrackDisable(BOOL fDisable);
    void    __stdcall DbgMemoryBlockTrackDisable(void *pv);

    void    __stdcall TraceMemoryLeaks(void);
    BOOL    __stdcall ValidateInternalHeap(void);
    void    __stdcall DbgDumpProcessHeaps();
    void    __stdcall DbgDumpMemory();
    size_t  __stdcall DbgTotalAllocated();
    void *  __stdcall PointerRequestFromActual(void *);

    void * __cdecl DbgMemSetName(void *pv, char * szFmt, ...);
    char * __stdcall DbgMemGetName(void * pv);

    extern BOOL __stdcall CheckSmallBlockHeap();
    extern BOOL __stdcall IsVirtualEnabled();

    //
    // Use the CHECK_HEAP macro to do thorough heap validation.
    //
#define CHECK_HEAP()    if (IsTagEnabled(tagDebugMemory)) {Assert(ValidateInternalHeap() && CheckSmallBlockHeap() && "Corrupted heap!");}

#endif

}

#endif _MEMUTIL_HXX