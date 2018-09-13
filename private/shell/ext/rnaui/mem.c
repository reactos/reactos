#include "rnaui.h"

//========== Memory Management =============================================

#ifndef WIN32

#define MAX_WORD    0xffff

DECLARE_HANDLE(HHEAP);

typedef struct {		//  maps to the bottom of a 16bit DS
    WORD reserved[8];
    WORD cAlloc;
    WORD cbAllocFailed;
    HHEAP hhpFirst;
    HHEAP hhpNext;
} HEAP;

#define PHEAP(hhp)          ((HEAP FAR*)MAKELP(hhp, 0))
#define MAKEHP(sel, off)    ((void _huge*)MAKELP((sel), (off)))

#define CBSUBALLOCMAX   0x0000f000L

HHEAP g_hhpFirst = NULL;

BOOL NEAR DestroyHeap(HHEAP hhp);

void Mem_Terminate()
{
    while (g_hhpFirst)
        DestroyHeap(g_hhpFirst);
}

BOOL NEAR CreateHeap(WORD cbInitial)
{
    HHEAP hhp;

    if (cbInitial < 1024)
        cbInitial = 1024;

    hhp = (HHEAP)GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_SHARE, cbInitial);

    if (!hhp)
        return FALSE;

    if (!LocalInit((WORD)hhp, sizeof(HEAP), cbInitial - 1))
    {
        GlobalFree(hhp);
        return FALSE;
    }

    PHEAP(hhp)->cAlloc = 0;
    PHEAP(hhp)->cbAllocFailed = MAX_WORD;
    PHEAP(hhp)->hhpNext = g_hhpFirst;
    g_hhpFirst = hhp;

    DebugMsg(DM_TRACE, "CreateHeap: added new local heap %x", hhp);

    return TRUE;
}

#pragma optimize("o", off)		// linked list removals don't optimize correctly
BOOL NEAR DestroyHeap(HHEAP hhp)
{
    ASSERT(hhp);
    ASSERT(g_hhpFirst);

    if (g_hhpFirst == hhp)
    {
        g_hhpFirst = PHEAP(hhp)->hhpNext;
    }
    else
    {
        HHEAP hhpT = g_hhpFirst;

        while (PHEAP(hhpT)->hhpNext != hhp)
        {
            hhpT = PHEAP(hhpT)->hhpNext;
            if (!hhpT)
                return FALSE;
        }

        PHEAP(hhpT)->hhpNext = PHEAP(hhp)->hhpNext;
    }
    if (GlobalFree((HGLOBAL)hhp) != NULL)
        return FALSE;

    return TRUE;
}
#pragma optimize("", on)	// back to default optimizations

#pragma optimize("lge", off) // Suppress warnings associated with use of _asm...
void NEAR* NEAR HeapAlloc(HHEAP hhp, WORD cb)
{
    void NEAR* pb;

    _asm {
        push    ds
        mov     ds,hhp
    }

    pb = (void NEAR*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cb);

    if (pb)
        ((HEAP NEAR*)0)->cAlloc++;

    _asm {
        pop     ds
    }

    return pb;
}
#pragma optimize("o", off)		// linked list removals don't optimize correctly

void _huge* WINAPI SharedAlloc(long cb)
{
    void NEAR* pb;
    HHEAP hhp;
    HHEAP hhpPrev;

    // If this is a big allocation, just do a global alloc.
    //
    if (cb > CBSUBALLOCMAX)
    {
        void FAR* lpb = MAKEHP(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_SHARE, cb), 0);
        if (!lpb)
            DebugMsg(DM_ERROR, "Alloc: out of memory");
        return lpb;
    }

    hhp = g_hhpFirst;

    while (TRUE)
    {
        if (hhp == NULL)
        {
            if (!CreateHeap(0))
            {
                DebugMsg(DM_ERROR, "Alloc: out of memory");
                return NULL;
            }

            hhp = g_hhpFirst;
        }

        pb = HeapAlloc(hhp, (WORD)cb);
        if (pb)
            return MAKEHP(hhp, pb);

        // Record the size of the allocation that failed.
        // Later attempts to allocate more than this amount
        // will not succeed.  This gets reset anytime anything
        // is freed in the heap.
        //
        PHEAP(hhp)->cbAllocFailed = (WORD)cb;

        // First heap is full... see if there's room in any other heap...
        //
        for (hhpPrev = hhp; hhp = PHEAP(hhp)->hhpNext; hhpPrev = hhp)
        {
            // If the last allocation to fail in this heap
            // is not larger than cb, don't even try an allocation.
            //
            if ((WORD)cb >= PHEAP(hhp)->cbAllocFailed)
                continue;

            pb = HeapAlloc(hhp, (WORD)cb);
            if (pb)
            {
                // This heap had room: move it to the front...
                //
                PHEAP(hhpPrev)->hhpNext = PHEAP(hhp)->hhpNext;
                PHEAP(hhp)->hhpNext = g_hhpFirst;
                g_hhpFirst = hhp;

                return MAKEHP(hhp, pb);
            }
            else
            {
                // The alloc failed.  Set cbAllocFailed...
                //
                PHEAP(hhp)->cbAllocFailed = (WORD)cb;
            }
        }
    }
}
#pragma optimize("", on)	// back to default optimizations

#pragma optimize("lge", off) // Suppress warnings associated with use of _asm...

void _huge* WINAPI SharedReAlloc(void _huge* pb, long cb)
{
    void NEAR* pbNew;
    void _huge* lpbNew;
    UINT cbOld;

    // BUGBUG, does not work with cb > 64k
    if (!pb)
        return SharedAlloc(cb);

    if (OFFSETOF(pb) == 0)
        return MAKEHP(GlobalReAlloc((HGLOBAL)SELECTOROF(pb), cb, GMEM_MOVEABLE | GMEM_ZEROINIT), 0);

    _asm {
        push    ds
        mov     ds,word ptr [pb+2]
    }

    pbNew = (void NEAR*)LocalReAlloc((HLOCAL)OFFSETOF(pb), (int)cb, LMEM_MOVEABLE | LMEM_ZEROINIT);
    if (!pbNew)
        cbOld = LocalSize((HLOCAL)OFFSETOF(pb));

    _asm {
        pop     ds
    }

    if (pbNew)
        return MAKEHP(SELECTOROF(pb), pbNew);

    lpbNew = SharedAlloc(cb);
    if (lpbNew)
    {
        hmemcpy((void FAR*)lpbNew, (void FAR*)pb, cbOld);
        Free(pb);
    }
    else
    {
        DebugMsg(DM_ERROR, "ReAlloc: out of memory");
    }
    return lpbNew;
}

BOOL WINAPI SharedFree(void _huge* FAR * ppb)
{
    BOOL fSuccess;
    UINT cAlloc;
    void _huge * pb = *ppb;

    if (!pb)
        return FALSE;

    *ppb = 0;

    if (OFFSETOF(pb) == 0)
        return (GlobalFree((HGLOBAL)SELECTOROF(pb)) == NULL);

    _asm {
        push    ds
        mov     ds,word ptr [pb+2]
    }

    fSuccess = (LocalFree((HLOCAL)OFFSETOF(pb)) ? FALSE : TRUE);

    cAlloc = 1;
    if (fSuccess)
    {
        cAlloc = --((HEAP NEAR*)0)->cAlloc;
        ((HEAP NEAR*)0)->cbAllocFailed = MAX_WORD;
    }

    _asm {
        pop     ds
    }

    if (cAlloc == 0)
        DestroyHeap((HHEAP)SELECTOROF(pb));

    return fSuccess;
}


DWORD WINAPI SharedGetSize(void _huge* pb)
{
    WORD wSize;

    if (OFFSETOF(pb) == 0)
        return GlobalSize((HGLOBAL)SELECTOROF(pb));

    _asm {
        push    ds
        mov     ds,word ptr [pb+2]
    }

    wSize = LocalSize((HLOCAL)OFFSETOF(pb));

    _asm {
        pop     ds
    }

    return (DWORD)wSize;
}

#pragma optimize("", on)

#else // WIN32

#pragma data_seg("SHAREDATA")
// Define a Global Shared Heap that we use allocate memory out of that we
// Need to share between multiple instances.
HANDLE g_hSharedHeap = NULL;
#pragma data_seg()
#define MAXHEAPSIZE 2097152
#define HEAP_SHARED	0x04000000		/* put heap in shared memory */

//----------------------------------------------------------------------------
void PUBLIC Mem_Terminate()
{
    // Assuming that everything else has exited
    //
    if (g_hSharedHeap != NULL)
        HeapDestroy(g_hSharedHeap);
    g_hSharedHeap = NULL;
}

//----------------------------------------------------------------------------
void * WINAPI SharedAlloc(long cb)
{
    // I will assume that this is the only one that needs the checks to
    // see if the heap has been previously created or not

    if (g_hSharedHeap == NULL)
    {
        ENTEREXCLUSIVE()
        if (g_hSharedHeap == NULL)
        {
            g_hSharedHeap = HeapCreate(HEAP_SHARED, 1, MAXHEAPSIZE);
        }
        LEAVEEXCLUSIVE()

        // If still NULL we have problems!
        if (g_hSharedHeap == NULL)
            return(NULL);
    }

    return HeapAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, cb);
}

//----------------------------------------------------------------------------
void * WINAPI SharedReAlloc(void * pb, long cb)
{
    if (pb==NULL)
    {
        return SharedAlloc(cb);
    }
    return HeapReAlloc(g_hSharedHeap, HEAP_ZERO_MEMORY, pb, cb);
}

//----------------------------------------------------------------------------
BOOL WINAPI SharedFree(void ** ppb)
{
    void * pb = *ppb;

    if (!pb)
        return FALSE;

    *ppb = 0;

    return HeapFree(g_hSharedHeap, 0, pb);
}

//----------------------------------------------------------------------------
DWORD WINAPI SharedGetSize(void * pb)
{
    return HeapSize(g_hSharedHeap, 0, pb);
}

#if 0
//----------------------------------------------------------------------------
// The following functions are for debug only and are used to try to
// calculate memory usage.
//
#ifdef DEBUG

#pragma data_seg("SHAREDATA")
typedef struct _HEAPTRACE
{
    DWORD   cAlloc;
    DWORD   cFailure;
    DWORD   cReAlloc;
    DWORD   cbMaxTotal;
    DWORD   cCurAlloc;
    DWORD   cbCurTotal;
} HEAPTRACE;

HEAPTRACE g_htSync = {0};      // Start of zero...
#pragma data_seg()

LPVOID WINAPI MemAlloc(HANDLE hheap, DWORD cb)
{
    LPVOID lp;

    lp = HeapAlloc(hheap, HEAP_ZERO_MEMORY, cb);
    if (lp == NULL)
    {
        g_htSync.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htSync.cAlloc++;
    g_htSync.cCurAlloc++;
    g_htSync.cbCurTotal += cb;
    if (g_htSync.cbCurTotal > g_htSync.cbMaxTotal)
        g_htSync.cbMaxTotal = g_htSync.cbCurTotal;

    return lp;
}

LPVOID WINAPI MemReAlloc(HANDLE hheap, LPVOID pb, DWORD cb)
{
    LPVOID lp;
    DWORD cbOld;

    cbOld = HeapSize(hheap, 0, pb);

    lp = HeapReAlloc(hheap, HEAP_ZERO_MEMORY, pb,cb);
    if (lp == NULL)
    {
        g_htSync.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htSync.cReAlloc++;
    g_htSync.cbCurTotal += cb - cbOld;
    if (g_htSync.cbCurTotal > g_htSync.cbMaxTotal)
        g_htSync.cbMaxTotal = g_htSync.cbCurTotal;

    return lp;
}

BOOL  WINAPI MemFree(HANDLE hheap, LPVOID pb)
{
    BOOL fRet;

    DWORD cbOld;

    cbOld = HeapSize(hheap, 0, pb);

    fRet = HeapFree(hheap, 0, pb);
    if (fRet)
    {
        // Update counts.
        g_htSync.cCurAlloc--;
        g_htSync.cbCurTotal -= cbOld;
    }

    return(fRet);
}

DWORD WINAPI MemSize(HANDLE hheap, LPVOID pb)
{
    return HeapSize(hheap, 0, pb);
}
#endif
#endif

#endif // WIN32

