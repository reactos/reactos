#include "ctlspriv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"

#ifndef UNIX
#define SZ_DEBUGSECTION     "comctl32"
#define SZ_MODULE           "COMCTL32"
#else
#define SZ_DEBUGSECTION     "comctrl"
#define SZ_MODULE           "COMCTRL"
#endif

#define DECLARE_DEBUG
#include <debug.h>

//========== Memory Management =============================================


//----------------------------------------------------------------------------
// Define a Global Shared Heap that we use allocate memory out of that we
// Need to share between multiple instances.
#ifndef WINNT
HANDLE g_hSharedHeap = NULL;
#define GROWABLE        0
#define MAXHEAPSIZE     GROWABLE
#define HEAP_SHARED	0x04000000		/* put heap in shared memory */
#endif

void Mem_Terminate()
{
#ifndef WINNT
    // Assuming that everything else has exited
    //
    if (g_hSharedHeap != NULL)
        HeapDestroy(g_hSharedHeap);
    g_hSharedHeap = NULL;
#endif
}

#ifndef WINNT
HANDLE InitSharedHeap(void)
{
    ENTERCRITICAL;
    if (g_hSharedHeap == NULL)
    {
        g_hSharedHeap = HeapCreate(HEAP_SHARED, 1, MAXHEAPSIZE);
    }
    LEAVECRITICAL;
    return g_hSharedHeap;
}
#endif

void * WINAPI Alloc(long cb)
{
    // I will assume that this is the only one that needs the checks to
    // see if the heap has been previously created or not
#if defined(WINNT) || defined(MAINWIN)
    return (void *)LocalAlloc(LPTR, cb);
#else
    HANDLE hHeap = GetSharedHeapHandle();

    // If still NULL we have problems!
    if (hHeap == NULL)
        return(NULL);

    return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cb);
#endif
}

void * WINAPI ReAlloc(void * pb, long cb)
{
    if (pb == NULL)
        return Alloc(cb);
#if defined(WINNT) || defined(MAINWIN)
    return (void *)LocalReAlloc((HLOCAL)pb, cb, LMEM_ZEROINIT | LMEM_MOVEABLE);
#else
    return HeapReAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, pb, cb);
#endif
}

BOOL WINAPI Free(void * pb)
{
#if defined(WINNT) || defined(MAINWIN)
    return (LocalFree((HLOCAL)pb) == NULL);
#else
    return HeapFree(g_hSharedHeap, 0, pb);
#endif
}

DWORD_PTR WINAPI GetSize(void * pb)
{
#if defined(WINNT) || defined(MAINWIN)
    return LocalSize((HLOCAL)pb);
#else
    return HeapSize(g_hSharedHeap, 0, pb);
#endif
}

//----------------------------------------------------------------------------
// The following functions are for debug only and are used to try to
// calculate memory usage.
//
#ifdef DEBUG
typedef struct _HEAPTRACE
{
    DWORD   cAlloc;
    DWORD   cFailure;
    DWORD   cReAlloc;
    ULONG_PTR cbMaxTotal;
    DWORD   cCurAlloc;
    ULONG_PTR cbCurTotal;
} HEAPTRACE;

HEAPTRACE g_htShell = {0};      // Start of zero...

LPVOID WINAPI ControlAlloc(HANDLE hheap, DWORD cb)
{
    LPVOID lp = HeapAlloc(hheap, HEAP_ZERO_MEMORY, cb);;
    if (lp == NULL)
    {
        g_htShell.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htShell.cAlloc++;
    g_htShell.cCurAlloc++;
    g_htShell.cbCurTotal += cb;
    if (g_htShell.cbCurTotal > g_htShell.cbMaxTotal)
        g_htShell.cbMaxTotal = g_htShell.cbCurTotal;

    return lp;
}

LPVOID WINAPI ControlReAlloc(HANDLE hheap, LPVOID pb, DWORD cb)
{
    LPVOID lp;
    SIZE_T cbOld;

    cbOld = HeapSize(hheap, 0, pb);

    lp = HeapReAlloc(hheap, HEAP_ZERO_MEMORY, pb,cb);
    if (lp == NULL)
    {
        g_htShell.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htShell.cReAlloc++;
    g_htShell.cbCurTotal += cb - cbOld;
    if (g_htShell.cbCurTotal > g_htShell.cbMaxTotal)
        g_htShell.cbMaxTotal = g_htShell.cbCurTotal;

    return lp;
}

BOOL  WINAPI ControlFree(HANDLE hheap, LPVOID pb)
{
    SIZE_T cbOld = HeapSize(hheap, 0, pb);
    BOOL fRet = HeapFree(hheap, 0, pb);
    if (fRet)
    {
        // Update counts.
        g_htShell.cCurAlloc--;
        g_htShell.cbCurTotal -= cbOld;
    }

    return(fRet);
}

SIZE_T WINAPI ControlSize(HANDLE hheap, LPVOID pb)
{
    return (DWORD) HeapSize(hheap, 0, pb);
}
#endif  // DEBUG

#if defined(FULL_DEBUG) && defined(WIN32)
#include "../inc/deballoc.c"
#endif // defined(FULL_DEBUG) && defined(WIN32)
