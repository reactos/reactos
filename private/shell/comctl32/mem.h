#ifndef _INC_MEM
#define _INC_MEM

// wrappers for private allocations, near in 16 bits

#define NearAlloc(cb)       ((void NEAR*)LocalAlloc(LPTR, (cb)))
#define NearReAlloc(pb, cb) ((void NEAR*)LocalReAlloc((HLOCAL)(pb), (cb), LMEM_MOVEABLE | LMEM_ZEROINIT))
#define NearFree(pb)        (LocalFree((HLOCAL)(pb)) ? FALSE : TRUE)
#define NearSize(pb)        LocalSize(pb)

#ifdef WIN32
//
// These macros are used in our controls, that in 32 bits we simply call
// LocalAlloc as to have the memory associated with the process that created
// it and as such will be cleaned up if the process goes away.
//
#ifdef DEBUG
LPVOID WINAPI ControlAlloc(HANDLE hheap, DWORD cb);
LPVOID WINAPI ControlReAlloc(HANDLE hheap, LPVOID pb, DWORD cb);
BOOL   WINAPI ControlFree(HANDLE hheap, LPVOID pb);
SIZE_T WINAPI ControlSize(HANDLE hheap, LPVOID pb);
#else // DEBUG
#define ControlAlloc(hheap, cb)       HeapAlloc((hheap), HEAP_ZERO_MEMORY, (cb))
#define ControlReAlloc(hheap, pb, cb) HeapReAlloc((hheap), HEAP_ZERO_MEMORY, (pb),(cb))
#define ControlFree(hheap, pb)        HeapFree((hheap), 0, (pb))
#define ControlSize(hheap, pb)        HeapSize((hheap), 0, (LPCVOID)(pb))
#endif // DEBUG

BOOL Str_Set(LPTSTR *ppsz, LPCTSTR psz);  // in the process heap

#else // WIN32

//
// In 16 bit code we need the Allocs to go from our heap code as we do not
// want to limit them to 64K of data.  If we have some type of notification of
// 16 bit application termination, We may want to see if we can
// dedicate different heaps for different processes to cleanup...
//
#define ControlAlloc(hheap, cb)       Alloc(cb)  /* calls to verify heap exists */
#define ControlReAlloc(hheap, pb, cb) ReAlloc(pb, cb)
#define ControlFree(hheap, pb)        Free(pb)
#define ControlSize(hheap, pb)        GetSize((LPCVOID)pb)
#define Str_Set(p, s)                 Str_SetPtr(p, s)  // use shared heap for win16

#endif // WIN32

#ifndef WINNT

extern HANDLE g_hSharedHeap;

HANDLE InitSharedHeap(void);

__inline HANDLE
GetSharedHeapHandle(void)
{
    if (g_hSharedHeap)
    {
        return g_hSharedHeap;
    }
    else
    {
        return InitSharedHeap();
    }
}

#endif

#endif  // !_INC_MEM
