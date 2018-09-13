//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dbgalloc.cxx
//
//  Contents:   Debug allocation routines and leak tracing
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifdef WIN16
#define SetDlgItemTextA SetDlgItemText
#define SetWindowTextA SetWindowText
#define SendDlgItemMessageA SendDlgItemMessage
#define CharUpperBuffA(x,y)
#define GetStringFromSymbolInfo(dwAddr, psi, pszString) wsprintfA(pszString, "  %08x <symbols not available>", dwAddr)
#define GetRunTimeType(x)  "Run-time type unavailable in win16"
#define AreSymbolsEnabled() FALSE
#else
#if !defined(UNIX) || !defined(_HPUX_SOURCE)
#ifndef X_TYPEINFO_H_
#define X_TYPEINFO_H_

#include <typeinfo.h>
#endif
#endif // !UNIX
#endif // !WIN16

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

ExternTag(tagLeaksExpected);
ExternTag(tagMemTrace);
ExternTag(tagHexDumpLeaks);
ExternTag(tagNoLeakAssert);

const BYTE  FILL_CLEAN      = 0x9D;
const BYTE  FILL_DEAD       = 0xAD;
const DWORD FILL_HEAD_GUARD = 0xBDBDBDBD;
const DWORD FILL_FOOT_GUARD = 0xCDCDCDCD;

extern BOOL g_fOSIsNT;
LONG g_cCoTrackDisable = 0;

// We read from the ini file a block size.  We will then only
// keep stack info for blocks of that size.  -2 means that 
// we haven't looked at the ini file yet and -1 means track
// all block sizes -- allowing us to track 0 size allocations.
// (will this ever happen?)
static LONG g_lStackTrackSize = -2;

#undef SNDMSG
#define SNDMSG ::SendMessageA

void TraceLeak(TRACETAG tag, HANDLE * phFile, char * pch, BOOL fOpenFile)
{
    TraceTag((tag, "%s", pch));

    if (*phFile == INVALID_HANDLE_VALUE && fOpenFile)
    {
        *phFile = CreateFileA("c:\\leakdump.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (*phFile != INVALID_HANDLE_VALUE)
    {
        _lwrite((HFILE)(DWORD_PTR)*phFile, pch, lstrlenA(pch));
        _lwrite((HFILE)(DWORD_PTR)*phFile, "\r\n", 2);
    }
}

void DumpHex(HANDLE hFile, BYTE *pb, UINT cb, DWORD_PTR dwAddr)
{
        BYTE *  pbRow;
        BYTE    b;
        UINT    cbRow;
        int             i;
    char    ach[256], *pch;

        while (cb > 0)
        {
        pch = ach;
        ach[0] = 0;

        pch += wsprintfA(pch, "   %08lX: ", (DWORD)dwAddr);
                pbRow = pb;
                cbRow = (cb > 16) ? 16 : cb;
                for (i = 0; i < 16; ++i)
                {
                        if (cbRow > 0)
                        {
                                b = *pbRow++;
                                --cbRow;
                                pch += wsprintfA(pch, "%02X ", b);
                        }
                        else
                                pch += wsprintfA(pch, "   ");
                        if (i == 7) pch += wsprintfA(pch, " ");
                }
                pch += wsprintfA(pch, "  ");
                pbRow = pb;
                cbRow = (cb > 16) ? 16 : cb;
                for (i = 0; i < 16; ++i)
                {
                        if (cbRow > 0)
                        {
                                b = *pbRow++;
                                --cbRow;
                        }
                        else
                                b = 32;

                        if (b > 31 && b < 127)
                                pch += wsprintfA(pch, "%c", b);
                        else
                                pch += wsprintfA(pch, ".");

                        if (i == 7) pch += wsprintfA(pch, " ");
                }
                cbRow = (cb > 16) ? 16 : cb;
                pb   += cbRow;
                cb   -= cbRow;
        TraceLeak(tagHexDumpLeaks, &hFile, ach, FALSE);
        dwAddr += cbRow;
        }
}

//+------------------------------------------------------------------------
//
//  Prefix struct stuck at the beginning of every allocated block.
//  Keeps track of all allocated blocks, plus saves a stack trace
//  from the allocation
//
//-------------------------------------------------------------------------


struct DBGALLOCHDR
{
    DBGALLOCHDR *   pdbgahPrev;
    DBGALLOCHDR *   pdbgahNext;
    DWORD           dwEip[STACK_WALK_DEPTH];
    DWORD           iAllocated;
    DWORD           tid;
    size_t          cbRequest;
    BOOL            fSymbols;
    BOOL            fLeaked;
    BOOL            fCoFlag;
    PERFMETERTAG    mt;
    size_t          cbHeader;
    char            szName[64];
    DWORD           adwGuard[4];
};


//+------------------------------------------------------------------------
//
//  Suffix struct stuck after the end of every allocated block.  Filled
//  with a known pattern to help detect pointers walking outside their
//  block.
//
//-------------------------------------------------------------------------

struct DBGALLOCFOOT
{
    DWORD           adwGuard[4];
};


//+------------------------------------------------------------------------
//
//  Byte sequence used to fill guard sections
//
//-------------------------------------------------------------------------

DWORD g_adwHeadGuardFill[4] =
        { FILL_HEAD_GUARD, FILL_HEAD_GUARD, FILL_HEAD_GUARD, FILL_HEAD_GUARD };

DWORD g_adwFootGuardFill[4] =
        { FILL_FOOT_GUARD, FILL_FOOT_GUARD, FILL_FOOT_GUARD, FILL_FOOT_GUARD };



inline size_t
ActualSizeFromRequestSize(size_t cb, BOOL fSymbols)
{
    return cb + sizeof(DBGALLOCHDR) + sizeof(DBGALLOCFOOT) +
           ((fSymbols) ? STACK_WALK_DEPTH * sizeof(SYMBOL_INFO) : 0);
}


inline size_t
RequestSizeFromActualSize(size_t cb, BOOL fSymbols)
{
    return cb - sizeof(DBGALLOCHDR) - sizeof(DBGALLOCFOOT) -
           ((fSymbols) ? STACK_WALK_DEPTH * sizeof(SYMBOL_INFO) : 0);
}


inline void *
RequestFromActual(DBGALLOCHDR * pdbgah)
{
    return pdbgah + 1;
}

inline void *
ClientFromActual(DBGALLOCHDR * pdbgah)
{
    return (BYTE *)RequestFromActual(pdbgah) + pdbgah->cbHeader;
}

inline DBGALLOCHDR *
ActualFromRequest(void * pv)
{
    return ((DBGALLOCHDR *) pv) - 1;
}


inline DBGALLOCFOOT *
FooterFromBlock(DBGALLOCHDR * pdbgah)
{
    return (DBGALLOCFOOT *)
            (((BYTE *) pdbgah) + sizeof(DBGALLOCHDR) + pdbgah->cbRequest);
}

inline SYMBOL_INFO *
SymbolsFromBlock(DBGALLOCHDR *pdbgah)
{
    return (SYMBOL_INFO *)
            (((BYTE *) pdbgah) + sizeof(DBGALLOCHDR) +
                                 sizeof(DBGALLOCFOOT) +
                                 pdbgah->cbRequest);
}

//+------------------------------------------------------------------------
//
//  Globals
//
//-------------------------------------------------------------------------

size_t g_cbTotalAllocated  = 0; // Total bytes current allocated.
size_t g_cbMaxAllocated = 0;    // Peak allocation size.
ULONG g_cAllocated = 0;         // Total number of allocations.

//
// Grep for a DWORD:
// If g_pvMemSearch is found inside a block, it is displayed with a 'F'.
//

void *g_pvMemSearch;


//
//  Allocation list is kept in a doubly-linked ring; this is the root
//  block in the ring.
//

DBGALLOCHDR g_dbgahRoot =
    {
        &g_dbgahRoot,
        &g_dbgahRoot,
        { 0, 0, 0, 0, 0 },
        0,
        (DWORD)-1
    };

//+---------------------------------------------------------------------------
//
//  Function:   DbgTraceAlloc
//
//  Synopsis:   Traces the result of an allocation or free
//
//  Arguments:
//
//  Returns:    void
//
//----------------------------------------------------------------------------

void DbgTraceAlloc(char * szTag, size_t cbAlloc, size_t cbFree)
{
    if (DbgExIsTagEnabled(tagMemTrace) && (cbAlloc || cbFree))
    {
        if (cbAlloc && cbFree)
        {
            TraceTag((tagMemTrace, "%s +%5d -%5d = [%7d]", szTag, cbAlloc,
                cbFree, g_cbTotalAllocated));
        }
        else if (cbAlloc)
        {
            TraceTag((tagMemTrace, "%s +%5d        = [%7d]", szTag, cbAlloc,
                g_cbTotalAllocated));
        }
        else
        {
            TraceTag((tagMemTrace, "%s        -%5d = [%7d]", szTag, cbFree,
                g_cbTotalAllocated));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgExMemoryTrackDisable
//
//  Synopsis:   Disables or Enables memory leak tracking. Allocations that
//              occur while tracking is disabled will never be reported as
//              leaks, even if they are.
//
//  Arguments:  [fDisable] -- TRUE if tracking should be disabled. FALSE to
//                            re-enable it.
//
//  Returns:    void
//
//  Notes:      Multiple calls giving TRUE require the same amount of calls
//              giving FALSE before tracking is actually re-enabled.
//
//----------------------------------------------------------------------------

void WINAPI
DbgExMemoryTrackDisable(BOOL fDisable)
{
    EnsureThreadState();
    if (fDisable)
    {
        TLS(cTrackDisable)++;
    }
    else
    {
        Assert(TLS(cTrackDisable) > 0);

        TLS(cTrackDisable)--;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgExCoMemoryTrackDisable
//
//  Synopsis:   Disables or Enables Co memory leak tracking. Allocations that
//              occur while tracking is disabled will never be reported as
//              leaks, even if they are.
//
//  Arguments:  [fDisable] -- TRUE if tracking should be disabled. FALSE to
//                            re-enable it.
//
//  Returns:    void
//
//  Notes:      Multiple calls giving TRUE require the same amount of calls
//              giving FALSE before tracking is actually re-enabled.
//
//----------------------------------------------------------------------------

void WINAPI
DbgExCoMemoryTrackDisable(BOOL fDisable)
{
    if (fDisable)
    {
        InterlockedIncrement(&g_cCoTrackDisable);
    }
    else
    {
        InterlockedDecrement(&g_cCoTrackDisable);
    }
}

void WINAPI
DbgExMemSetHeader(void * pvRequest, size_t cb, PERFMETERTAG mt)
{
    if (pvRequest)
    {
        DBGALLOCHDR * pdbgah = ActualFromRequest(pvRequest);
        pdbgah->cbHeader = cb;
        pdbgah->mt = mt;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRunTimeType
//
//  Synopsis:   Returns the type of the object stored in the block.  If
//              it cannot be determined, the empty string is returned.
//
//  Arguments:  [fDisable] -- TRUE if tracking should be disabled. FALSE to
//                            re-enable it.
//
//  Returns:    char *
//
//----------------------------------------------------------------------------

#ifndef WIN16
#pragma warning(disable: 4541)

class CType
{
    virtual MethodFoo() { return 0; }
};

char *
GetRunTimeType(LPVOID pv)
{
    CType * pType = (CType *)pv;
    char * psz = NULL;
    static char achBuf[256];

    //
    // The typeid operator will throw an exception or blow up if the memory
    // doesn't point to an object.
    //
#ifndef NO_RTTI
    __try
    {
        psz = const_cast<char*>(typeid(*pType).name());
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        psz = NULL;
    }
    __endexcept
#else
      psz = NULL; // Added only for this #ifdef
#endif

    if (psz == NULL || IsBadStringPtrA(psz, 1024))
        return("");

    // Remove "class " wherever it occurs

    strncpy(achBuf, psz, ARRAY_SIZE(achBuf));

    for (;;)
    {
        char * szClass = strstr(achBuf, "class ");

        if (szClass == NULL)
            break;

        memmove(szClass, szClass + 6, lstrlenA(szClass + 6) + 1);
    }

    return(achBuf);
}
#endif //!WIN16

//+------------------------------------------------------------------------
//
//  Function:   BlockIsValid
//
//  Synopsis:   Returns TRUE iff the block's guard structures are valid.
//
//  Arguments:  [pdbgah]
//
//  Returns:    BOOL
//
//-------------------------------------------------------------------------

BOOL
BlockIsValid(DBGALLOCHDR * pdbgah)
{
    DBGALLOCFOOT *  pdbgft;

    Assert(sizeof(pdbgah->adwGuard) == sizeof(g_adwHeadGuardFill));
    if (memcmp(pdbgah->adwGuard,
            g_adwHeadGuardFill,
            sizeof(g_adwHeadGuardFill)))
        return FALSE;

    pdbgft = FooterFromBlock(pdbgah);
    Assert(sizeof(pdbgft->adwGuard) == sizeof(g_adwFootGuardFill));
    if (memcmp(pdbgft->adwGuard,
            g_adwFootGuardFill,
            sizeof(g_adwFootGuardFill)))
        return FALSE;

    return TRUE;
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExPreGetSize
//
//  Synopsis:   Hook called before memory get-size function
//
//  Arguments:  pvRequest  Request pointer
//
//  Returns:    Actual pointer.
//
//-------------------------------------------------------------------------

void * WINAPI
DbgExPreGetSize(void *pvRequest)
{
    EnsureThreadState();
    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());
    TLS(pvRequest) = pvRequest;
    return (pvRequest) ? ActualFromRequest(pvRequest) : NULL;
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExPostGetSize
//
//  Synopsis:   Hook called after memory get-size function
//
//  Arguments:  cb  Actual size.
//
//  Returns:    Request Size.
//
//-------------------------------------------------------------------------

size_t WINAPI
DbgExPostGetSize(size_t cb)
{
    DBGTHREADSTATE *   pts;
    pts = DbgGetThreadState();
    return (pts->pvRequest) ? ActualFromRequest(pts->pvRequest)->cbRequest : 0;
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExPreAlloc
//
//  Synopsis:   Hook called before memory allocation function.
//
//  Arguments:  cb      Request size.
//
//  Returns:    Actual size to allocate.
//
//-------------------------------------------------------------------------

size_t WINAPI
DbgExPreAlloc(size_t cbRequest)
{
    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());
    EnsureThreadState();
    TLS(cbRequest) = cbRequest;

    if (AreSymbolsEnabled())
    {
        if (g_lStackTrackSize == -2)
        {
            g_lStackTrackSize = GetPrivateProfileIntA("SymbolLeakTrack", "Size", -1, "mshtmdbg.ini");

            if (g_lStackTrackSize < -1)
                g_lStackTrackSize = -1;
        }

        TLS(fSymbols) = (g_lStackTrackSize == -1 || (size_t)g_lStackTrackSize == cbRequest);
    }
    else
        TLS(fSymbols) = FALSE;

    return ActualSizeFromRequestSize(cbRequest, TLS(fSymbols));
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExPostAlloc
//
//  Synopsis:   Hook called after memory allocation function.
//
//  Arguments:  pv      Pointer to actual allocation.
//
//  Returns:    Pointer requested allocation.
//
//-------------------------------------------------------------------------

void * WINAPI
DbgExPostAlloc(void *pv)
{
    DBGTHREADSTATE *   pts;
    DBGALLOCHDR *   pdbgah;
    DBGALLOCFOOT *  pdbgft;

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    pts = DbgGetThreadState();

    pdbgah = (DBGALLOCHDR *)pv;
    if (!pdbgah)
        return NULL;

    DbgTraceAlloc("A", pts->cbRequest, 0);

    //  Keep track of total amount of memory allocated
    pdbgah->cbRequest = pts->cbRequest;
    pdbgah->cbHeader  = 0;
    pdbgah->fSymbols  = pts->fSymbols;
    pdbgah->fLeaked   = (pts->cTrackDisable > 0) ||
                        (pts->fSpyAlloc && g_cCoTrackDisable > 0);
    pdbgah->fCoFlag   = pts->fSpyAlloc;
    pdbgah->mt        = pdbgah->fCoFlag ? pts->mtSpy : 0;

    if (pdbgah->fCoFlag && pdbgah->mt)
    {
        DbgExMtAdd(pdbgah->mt, +1, pts->cbRequest);
    }

    //  Note the thread which made the allocation
    pdbgah->tid = GetCurrentThreadId();

#ifndef WIN16
    if (pdbgah->fSymbols)
    {
        //  Snapshot stack
        GetStackBacktrace(3 + (pts->fSpyAlloc ? 2 : 0),
                          ARRAY_SIZE(pdbgah->dwEip),
                          pdbgah->dwEip,
                          SymbolsFromBlock(pdbgah));
    }
    else
#endif
    {
        pdbgah->dwEip[0] = NULL;
    }

    // Default the name to the empty string

    pdbgah->szName[0] = 0;

    //  Fill in guard blocks
    Assert(sizeof(pdbgah->adwGuard) == sizeof(g_adwHeadGuardFill));
    memcpy(pdbgah->adwGuard, g_adwHeadGuardFill, sizeof(g_adwHeadGuardFill));
    pdbgft = FooterFromBlock(pdbgah);
    Assert(sizeof(pdbgft->adwGuard) == sizeof(g_adwFootGuardFill));
    memcpy(pdbgft->adwGuard, g_adwFootGuardFill, sizeof(g_adwFootGuardFill));

    // Update globals.

    {
        LOCK_GLOBALS;

        g_cbTotalAllocated += pts->cbRequest;
        if (g_cbMaxAllocated < g_cbTotalAllocated)
            g_cbMaxAllocated = g_cbTotalAllocated;

        //  Keep track of allocation number.
        pdbgah->iAllocated = g_cAllocated++;

        //  Hook into allocated blocks chain
        pdbgah->pdbgahPrev = &g_dbgahRoot;
        pdbgah->pdbgahNext = g_dbgahRoot.pdbgahNext;
        g_dbgahRoot.pdbgahNext->pdbgahPrev = pdbgah;
        g_dbgahRoot.pdbgahNext = pdbgah;
    }

    pv = RequestFromActual(pdbgah);

    //  BUGBUG we should modify this to a changing, yet predictable pattern.
    //  Fill logical block with clean byte
    memset(pv, FILL_CLEAN, pts->cbRequest);

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    return pv;
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExPreFree
//
//  Synopsis:   Hook called before memory free function.
//
//  Arguments:  pv  Pointer to request allocation.
//
//  Returns:    Pointer to actual allocation.
//
//-------------------------------------------------------------------------

void * WINAPI
DbgExPreFree(void *pv)
{
    DBGALLOCHDR *   pdbgah;

    EnsureThreadState();
    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    if (!pv)
        return NULL;

    pdbgah = ActualFromRequest(pv);

    Assert(BlockIsValid(pdbgah));

    {
        // Keep scope of locked globals to a minimum.  There's a
        // potential for deadlock if the assert above fires with
        // locked globals.

        LOCK_GLOBALS;

        pdbgah->pdbgahPrev->pdbgahNext = pdbgah->pdbgahNext;
        pdbgah->pdbgahNext->pdbgahPrev = pdbgah->pdbgahPrev;

        g_cbTotalAllocated -= pdbgah->cbRequest;
    }

    DbgTraceAlloc("F", 0, pdbgah->cbRequest);

    if (pdbgah->fCoFlag && pdbgah->mt)
    {
        DbgExMtAdd(pdbgah->mt, -1, -(LONG)pdbgah->cbRequest);
    }

    //  Fill entire block (including debug additions) with fill dead
    memset(pdbgah,
           FILL_DEAD,
           ActualSizeFromRequestSize(pdbgah->cbRequest, pdbgah->fSymbols));

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    return pdbgah;
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExPostFree
//
//  Synopsis:   Hook called after memory free function.
//
//-------------------------------------------------------------------------

void WINAPI
DbgExPostFree(void)
{
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExPreRealloc
//
//  Synopsis:   Hook called before memory reallocation function.
//
//  Arguments:  pv  Pointer to the request allocation.
//              cb  New requested size.
//              ppv Pointer to the actual allocation.
//
//  Returns:    New actual size.
//
//-------------------------------------------------------------------------

size_t WINAPI
DbgExPreRealloc(void *pvRequest, size_t cbRequest, void **ppv)
{
    size_t          cb;
    DBGTHREADSTATE *   pts;
    DBGALLOCHDR *   pdbgah = ActualFromRequest(pvRequest);

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());
    EnsureThreadState();

    pts = DbgGetThreadState();

    pts->cbRequest = cbRequest;
    pts->pvRequest = pvRequest;

    if (pvRequest == NULL)
    {
        *ppv = NULL;
        cb = ActualSizeFromRequestSize(cbRequest, pdbgah->fSymbols);
    }
    else if (cbRequest == 0)
    {
        *ppv = DbgExPreFree(pvRequest);
        cb = 0;
    }
    else
    {
        Assert(BlockIsValid(pdbgah));

        // copy tail data now; it will be invalid later on a shrinking realloc
        if (pdbgah->fSymbols)
        {
            memcpy(pts->aSymbols, SymbolsFromBlock(pdbgah),
                   STACK_WALK_DEPTH * sizeof(SYMBOL_INFO));
        }

        {
            LOCK_GLOBALS;

            pdbgah->pdbgahPrev->pdbgahNext = pdbgah->pdbgahNext;
            pdbgah->pdbgahNext->pdbgahPrev = pdbgah->pdbgahPrev;

            g_cbTotalAllocated -= pdbgah->cbRequest;
        }

        *ppv = pdbgah;
        cb = ActualSizeFromRequestSize(cbRequest, pdbgah->fSymbols);
    }

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    return cb;
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExPostRealloc
//
//  Synopsis:   Hook called after the memory allocation function
//
//  Arguments:  pv  Pointer to the actual allocation.
//
//  Returns:    Pointer to the requested allocation.
//
//-------------------------------------------------------------------------

void * WINAPI
DbgExPostRealloc(void *pv)
{
    void *          pvReturn;
    DBGTHREADSTATE *   pts;
    DBGALLOCHDR *   pdbgah;
    DBGALLOCFOOT *  pdbgft;

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    pts = DbgGetThreadState();

    if (pts->pvRequest == NULL)
    {
        pvReturn = DbgExPostAlloc(pv);
    }
    else if (pts->cbRequest == 0)
    {
        Assert(pv == NULL);
        pvReturn = NULL;
    }
    else
    {
        LOCK_GLOBALS;

        if (pv == NULL)
        {

            // The realloc failed.  Hook the the block back
            // into the list.

            Assert(pts->pvRequest);
            pdbgah = ActualFromRequest(pts->pvRequest);

            pdbgah->pdbgahPrev = &g_dbgahRoot;
            pdbgah->pdbgahNext = g_dbgahRoot.pdbgahNext;
            g_dbgahRoot.pdbgahNext->pdbgahPrev = pdbgah;
            g_dbgahRoot.pdbgahNext = pdbgah;

            g_cbTotalAllocated += pts->cbRequest;
            if (g_cbMaxAllocated < g_cbTotalAllocated)
                g_cbMaxAllocated = g_cbTotalAllocated;

            pvReturn = NULL;
        }
        else
        {
            pdbgah = (DBGALLOCHDR *)pv;

            DbgTraceAlloc("R", pts->cbRequest, pdbgah->cbRequest);

            if (pdbgah->fCoFlag && pdbgah->mt)
            {
                DbgExMtAdd(pdbgah->mt, 0, (LONG)(pts->cbRequest - pdbgah->cbRequest));
            }

            //  Keep track of total amount of memory allocated
            pdbgah->cbRequest = pts->cbRequest;
            g_cbTotalAllocated += pts->cbRequest;
            if (g_cbMaxAllocated < g_cbTotalAllocated)
                g_cbMaxAllocated = g_cbTotalAllocated;

            //  Hook into allocated blocks chain
            pdbgah->pdbgahPrev = &g_dbgahRoot;
            pdbgah->pdbgahNext = g_dbgahRoot.pdbgahNext;
            g_dbgahRoot.pdbgahNext->pdbgahPrev = pdbgah;
            g_dbgahRoot.pdbgahNext = pdbgah;

            if (pdbgah->fSymbols)
            {
                // recall saved symbols
                memcpy(SymbolsFromBlock(pdbgah), pts->aSymbols,
                       STACK_WALK_DEPTH * sizeof(SYMBOL_INFO));
            }

            //  Fill in guard blocks
            pdbgft = FooterFromBlock(pdbgah);
            Assert(sizeof(pdbgft->adwGuard) == sizeof(g_adwFootGuardFill));
            memcpy(pdbgft->adwGuard, g_adwFootGuardFill, sizeof(g_adwFootGuardFill));

            pvReturn = RequestFromActual(pdbgah);
        }
    }

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    return pvReturn;
}

//+------------------------------------------------------------------------
//
//  Function:   DbgPreDidAlloc
//
//  Synopsis:   Hook called before memory did allocate function.
//
//  Arguments:  pv  Pointer to the requested allocation.
//
//  Returns:    Pointer to the actual allocatino.
//
//-------------------------------------------------------------------------

void * WINAPI
DbgExPreDidAlloc(void *pvRequest)
{
    return ActualFromRequest(pvRequest);
}

//+------------------------------------------------------------------------
//
//  Function:   DbgPreDidAlloc
//
//  Synopsis:   Hook called after memory did allocate function.
//
//  Arguments:  pvRequest Pointer to the requested allocation.
//              fActual   Actual return value for the block
//
//  Returns:    Did we allocate this block?.
//
//-------------------------------------------------------------------------

BOOL WINAPI
DbgExPostDidAlloc(void *pvRequest, BOOL fActual)
{
    return fActual;
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExMemSetName
//
//  Synopsis:   Sets the descriptive name of a memory block
//
//  Arguments:  pv  Pointer to the requested allocation
//
//  Returns:    Pointer to the requested allocation
//
//-------------------------------------------------------------------------
void * __cdecl
DbgExMemSetName(void *pvRequest, char * szFmt, ...)
{
    va_list va;
    void * pv;

    va_start(va, szFmt);
    pv = DbgExMemSetNameList(pvRequest, szFmt, va);
    va_end(va);

    return(pv);
}

void * WINAPI
DbgExMemSetNameList(void * pvRequest, char * szFmt, va_list va)
{
    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    DBGALLOCHDR * pdbgah = ActualFromRequest(pvRequest);

    char szBuf[1024];

    if (!BlockIsValid(pdbgah))
        return pvRequest;

    if (pvRequest)
    {
#ifndef WIN16
        wvsprintfA(szBuf, szFmt, va);
#else
        wvsprintf(szBuf, szFmt, va);
#endif

        szBuf[ARRAY_SIZE(pdbgah->szName) - 1] = 0;
        lstrcpyA(pdbgah->szName, szBuf);
    }

    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    return pvRequest;
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExMemGetName
//
//  Synopsis:   Gets the descriptive name of a memory block
//
//  Arguments:  pv  Pointer to the requested allocation
//
//  Returns:    Pointer to the requested allocation
//
//-------------------------------------------------------------------------

char * WINAPI
DbgExMemGetName(void *pvRequest)
{
    return(ActualFromRequest(pvRequest)->szName);
}

//+------------------------------------------------------------------------
//
//  Function:   DbgSetAutoLeak
//
//  Synopsis:   Tests the contents of the block to determine if it is
//              known to have been leaked.  If so, turns on the fLeaked flag.
//
//  Arguments:  pv  Pointer to the requested allocation
//
//  Returns:    Pointer to the requested allocation
//
//-------------------------------------------------------------------------

void DbgSetAutoLeak(DBGALLOCHDR * pdbgah)
{
    return;

#if 0 // For example ...
    size_t cbClient = pdbgah->cbRequest - pdbgah->cbHeader;

    if (pdbgah->fLeaked || cbClient < 5 || cbClient > 32)
        return;

    if (    memcmp(ClientFromActual(pdbgah), "image/", 6) == 0
        ||  memcmp(ClientFromActual(pdbgah), "text/", 5) == 0
        ||  memcmp(ClientFromActual(pdbgah), "application/", 12) == 0)
    {
        pdbgah->fLeaked = TRUE;
    }
#endif
}

//+------------------------------------------------------------------------
//
//  Function:   DbgExTraceMemoryLeaks
//
//  Synopsis:   Traces all allocated blocks, along with stack backtraces
//              from their allocation, to tagLeaks.
//
//-------------------------------------------------------------------------

void WINAPI
DbgExTraceMemoryLeaks()
{
    DBGALLOCHDR *   pdbgah;
#ifndef _MAC
    int             i;
    SYMBOL_INFO *   psi;
    CHAR            achSymbol[256];
#endif
    int             iPass;
    int             acLeaks[4] = { 0, 0, 0, 0 };
    BOOL            fLeakBanner;
    TRACETAG        tag;
    char            achBuf[512];
    HANDLE          hFile = INVALID_HANDLE_VALUE;

    EnsureThreadState();

    LOCK_GLOBALS;

    for (iPass = 0; iPass < 2; ++iPass)
    {
        fLeakBanner = FALSE;
        tag = iPass == 0 ? tagLeaks : tagLeaksExpected;

        for (pdbgah = g_dbgahRoot.pdbgahNext;
             pdbgah != &g_dbgahRoot;
             pdbgah = pdbgah->pdbgahNext)
        {
            if (iPass == 0)
            {
                DbgSetAutoLeak(pdbgah);
            }

            if (!!pdbgah->fLeaked == (iPass == 0))
                continue;

            if (!fLeakBanner)
            {
                fLeakBanner = TRUE;

                wsprintfA(achBuf, "---------- Leaked Memory Blocks %s----------",
                    iPass == 0 ? "" : "(Expected) ");

                if (DbgExIsTagEnabled(tag))
                {
                    TraceLeak(tag, &hFile, achBuf, TRUE);
                }
            }

            if (DbgExIsTagEnabled(tag))
            {
                wsprintfA(achBuf, "%c%c p=0x%08x  cb=%-4d #=%-4d TID:0x%x %s",
                        pdbgah->fCoFlag ? 'C' : ' ',
                        pdbgah->fLeaked ? 'L' : ' ',
                        ClientFromActual(pdbgah),
                        pdbgah->cbRequest - pdbgah->cbHeader,
                        pdbgah->iAllocated,
                        pdbgah->tid,
                        pdbgah->szName);

                TraceLeak(tag, &hFile, achBuf, TRUE);

                if (pdbgah->mt)
                {
                    wsprintfA(achBuf, "   %s (%s)", DbgExMtGetName(pdbgah->mt), DbgExMtGetDesc(pdbgah->mt));
                    TraceLeak(tag, &hFile, achBuf, TRUE);
                }

                psi = (pdbgah->fSymbols) ? SymbolsFromBlock(pdbgah) : NULL;

                for (i = 0; i < ARRAY_SIZE(pdbgah->dwEip); i++)
                {
                    if (!pdbgah->dwEip[i])
                        break;

                    GetStringFromSymbolInfo(pdbgah->dwEip[i],
                                            (psi) ? psi + i : NULL,
                                            achSymbol);

                    if (achSymbol[0])
                    {
                        wsprintfA(achBuf, " %s", achSymbol);
                        TraceLeak(tag, &hFile, achBuf, TRUE);
                    }
                }

                if (hFile != INVALID_HANDLE_VALUE || DbgExIsTagEnabled(tagHexDumpLeaks))
                {
                    ULONG cb = pdbgah->cbRequest - pdbgah->cbHeader;

                    if (cb > 4096)
                        cb = 4096;

                    DumpHex(hFile, (BYTE *)ClientFromActual(pdbgah), cb, (DWORD_PTR)ClientFromActual(pdbgah));
                }
            }

            if (pdbgah->fCoFlag)
                acLeaks[iPass + 2]++;
            else
                acLeaks[iPass]++;
        }
    }

    wsprintfA(achBuf, "%d+%d leaks (plus %d+%d expected), total size %d, peak size %d",
            acLeaks[0], acLeaks[2], acLeaks[1], acLeaks[3],
            g_cbTotalAllocated, g_cbMaxAllocated);
    TraceLeak(tagNull, &hFile, achBuf, FALSE);


    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    //
    // Don't fire the assert if the exit-on-assert flag is on - calling
    // TerminateProcess when we're already shutting down the process causes
    // Win95 to blow up.
    //
    if (!DbgExIsTagEnabled(tagNoLeakAssert))
    {
        char achMsg[512];

        wsprintfA(achMsg, "Memory Leaked! %d+%d block%s (plus %d+%d expected)\n(Leaks were dumped to c:\\leakdump.txt so there is no excuse to ignore this message!)",
            acLeaks[0], acLeaks[2], (acLeaks[0] + acLeaks[2] == 1) ? "" : "s",
            acLeaks[1], acLeaks[3]);

        AssertSz(acLeaks[0] + acLeaks[2] == 0, achMsg);
    }
}


//+------------------------------------------------------------------------
//
//  Function:   DbgExValidateInternalHeap
//
//  Synopsis:   Cruises through the heap, validating each allocated
//              block.  Any invalid blocks are traced to tagLeaks.
//              Function returns TRUE iff all blocks are valid.
//
//-------------------------------------------------------------------------

BOOL WINAPI
DbgExValidateInternalHeap()
{
    DBGALLOCHDR *   pdbgah;
    int             i;
    BOOL            fHeapValid  = TRUE;
    CHAR            achSymbol[256];
    SYMBOL_INFO    *psi;

    EnsureThreadState();

    LOCK_GLOBALS;

    for (pdbgah = g_dbgahRoot.pdbgahNext;
         pdbgah != &g_dbgahRoot;
         pdbgah = pdbgah->pdbgahNext)
    {
        if (!BlockIsValid(pdbgah))
        {
            if (fHeapValid && !AreSymbolsEnabled())
            {
                TraceTag((tagLeaks, ""));

                TraceTag((tagLeaks, "Symbol loading is not enabled. Turn on the '!Symbols'"));
                TraceTag((tagLeaks, "   tag to enable it and then restart your application. Make sure"));
                TraceTag((tagLeaks, "   imagehlp.dll and mspdb41.dll are on your path."));
                TraceTag((tagLeaks, "See the Forms3 Development Handbook, Tips, Tip04 for details.\n"));
            }

            fHeapValid = FALSE;

            TraceTag((
                    tagLeaks,
                    "Invalid block at 0x%08x  Size:%-4d Alloc #:%-4d TID:0x%x",
                    ClientFromActual(pdbgah),
                    pdbgah->cbRequest - pdbgah->cbHeader,
                    pdbgah->iAllocated,
                    pdbgah->tid));

#ifndef WIN16
            if (DbgExIsTagEnabled(tagLeaks))
            {
                psi = (pdbgah->fSymbols) ? SymbolsFromBlock(pdbgah) : NULL;

                for (i = 0; i < ARRAY_SIZE(pdbgah->dwEip); i++)
                {
                    if (!pdbgah->dwEip[i])
                        break;

                    GetStringFromSymbolInfo(pdbgah->dwEip[i],
                                            (psi) ? psi + i : NULL,
                                            achSymbol);

                    TraceTag((tagLeaks, "%s", achSymbol));
                }
            }
#endif //!WIN16
        }
    }

#if !defined(_MAC) && !defined(WIN16)
    //
    // Then, check the system's process heap.
    //
    if (fHeapValid && g_fOSIsNT)
    {
        //
        // If running under a debugger, this call will generate a breakpoint
        // and print information to the debugger if the heap is corrupt.
        //
        EnterCriticalSection(&g_csHeapHack);
        fHeapValid = HeapValidate(GetProcessHeap(), 0, NULL);
        LeaveCriticalSection(&g_csHeapHack);
    }
#endif

    // check

    return fHeapValid;
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgExMemoryBlockTrackDisable
//
//  Synopsis:   Disables memory leak tracking for a previously allocated block.
//
//  Arguments:  [pvRequest] -- The allocated memory block
//
//  Returns:    void
//
//----------------------------------------------------------------------------

void WINAPI
DbgExMemoryBlockTrackDisable(void * pv)
{
    DBGALLOCHDR * pdbgah;
    DBGALLOCHDR * pdbgahT;

    EnsureThreadState();

    if (!pv)
        return;

    LOCK_GLOBALS;

    pdbgahT = ActualFromRequest(pv);

    for (pdbgah = g_dbgahRoot.pdbgahNext;
         pdbgah != &g_dbgahRoot;
         pdbgah = pdbgah->pdbgahNext)
    {
        if (pdbgah == pdbgahT)
        {
            pdbgah->fLeaked = TRUE;
            return;
        }
    }

    TraceTag((tagLeaks, "DbgExMemoryBlockTrackDisable: Block not found: %08lX\n", pv));
}

// Heap Monitor ---------------------------------------------------------------

/* We have no bitmaps in our owner-drawn controls, but  */
/* if we did, these constants would need to be updated. */

#define BMWIDTH             0
#define BMHEIGHT            0
#define NUMBMPS             0

typedef DBGALLOCHDR DBGAH, * PDBGAH;

/* Function Prototypes */

/* Heap Monitor Dialog Thread and Dialog Proc */

DWORD WINAPI
HeapMonitorThread(LPVOID lpv);

INT_PTR WINAPI
HeapMonDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Command and Support Routines for the HeapMonDlgProc */

void    BltIcon(HWND hWnd);
void    SetWindowTitle(HWND hWnd, LPSTR lpcText);
void    RefreshHeapView(HWND hWnd);
void    SetSumSelection(void);
void    BlockListNotify(WORD wNotify, HWND  hWnd);

BOOL    FBlockStillValid(PDBGAH pdbgah);

void    MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis);
int     CompareItem(UINT idCtl, LPCOMPAREITEMSTRUCT pcis);
void    DrawItem(WORD wId, LPDRAWITEMSTRUCT pdis);
void    OutTextFormat(WORD wId, LPDRAWITEMSTRUCT pdis);
void    SetRGBValues(void);

/* Memory Block Editor Dialog Proc and Support Routines */

INT_PTR WINAPI
BlockDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void    FormatBlockRow(LPSTR psz, LPBYTE lpbRow, LPBYTE lpbHead, UINT cb, LPBYTE pbReal);
void    OutBlockRowFormat(WORD wId, LPDRAWITEMSTRUCT pdis, PDBGAH pdbgah);
void    DrawBlockRow(WORD wId, LPDRAWITEMSTRUCT pdis, PDBGAH pdbgah);
void    SetBlockEditData(HWND hWnd, PDBGAH pdbgah);

//  Global Data

HWND        ghWnd           = NULL;
HWND        ghBlockWnd      = NULL;
HICON       ghIcon          = NULL;
HBRUSH      ghbrBkgnd       = NULL;
UINT        idSort          = IDC_SORTADDRESS;
UINT        idDataType      = IDC_BYTE;
BOOL        fDlgUp          = FALSE;
BOOL        g_fShowMeters   = FALSE;

//  Globals used by the Owner-Draw code.

DWORD       rgbWindowColor  = 0xFF000000;    // variables for the current
DWORD       rgbHiliteColor  = 0xFF000000;    // system color settings.
DWORD       rgbWindowText   = 0xFF000000;    // on a WM_SYSCOLORCHANGE
DWORD       rgbHiliteText   = 0xFF000000;    // we check to see if we need
DWORD       rgbGrayText     = 0xFF000000;    // to reload our bitmap.
DWORD       rgbDDWindow     = 0xFF000000;    //
DWORD       rgbDDHilite     = 0xFF000000;    // 0xFF000000 is an invalid RGB

//  Tabs for the Block List - Address  AllocNum  Size  Description

//  Tabs for Memory Block Edit

//  hdc to hold listbox bitmaps (for speed)

HDC     hdcMemory = 0;

/* Memory browsing threads */
DWORD   WINAPI  DoBrowseThread(LPVOID lpParam);
DWORD   WINAPI  DoMemStats(LPVOID lpParam);


void WINAPI DoMemoryBrowse(HINSTANCE phInst, HWND phWnd, DWORD dwProcess);


void WINAPI
DbgExOpenMemoryMonitor()
{
    if (fDlgUp)
    {
        ShowWindow(ghWnd, SW_RESTORE);
        SetForegroundWindow(ghWnd);
    }
    else
    {
        DWORD dwThreadId;
        THREAD_HANDLE hThread;

        hThread = CreateThread((LPSECURITY_ATTRIBUTES) NULL, 0,
            (LPTHREAD_START_ROUTINE)HeapMonitorThread,
            NULL, 0, &dwThreadId);

        CloseThread(hThread);
    }
}

/*
 -  HeapMonitorThread
 -
 *  Purpose:
 *      This thread is here for the sole purpose of putting up a
 *      dialog who gets messages on this threads time slice.
 */

DWORD WINAPI HeapMonitorThread(LPVOID lpv)
{
    ghbrBkgnd = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
    ghIcon = LoadIconA(g_hinstMain, MAKEINTRESOURCEA(IIC_ICONIC));

    DialogBoxA(g_hinstMain, MAKEINTRESOURCEA(IDD_HEAPMON), NULL, HeapMonDlgProc);

    DeleteObject(ghbrBkgnd);

    return 0;
}


/*
 -  FBlockStillValid
 -
 *  Purpose:
 *      Searches for the specified block in the specified heaps.
 *      This must be done before using a pdbgah object since they
 *      are used and added/removed on two different threads.
 */

BOOL FBlockStillValid(PDBGAH pdbgah)
{
    PDBGAH pdbgahT = pdbgah;

    for (pdbgah = g_dbgahRoot.pdbgahNext;
         pdbgah != &g_dbgahRoot;
         pdbgah = pdbgah->pdbgahNext)
    {
        if (pdbgah == pdbgahT)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}


#define PvPlhblkEnd(pdbgah)     ((LPBYTE)RequestFromActual(pdbgah) + (pdbgah)->cbRequest - 1)

/*
 -  RefreshHeapView
 -
 *  Purpose:
 *      This updates our currently displayed heap.  Does all the calculations
 *      and fills in all the edit controls on the HeapMon dialog.
 */

void RefreshHeapView(HWND hWnd)
{
    char    szOutBuff[128];
    ULONG   cLive = 0;
    PDBGAH  pdbgah;
    ULONG   cbLive = 0;
    LONG    idxTop;

    LOCK_GLOBALS;

    idxTop = SendDlgItemMessageA(hWnd, IDC_BLOCKLIST, LB_GETTOPINDEX, 0, 0L);

    SendDlgItemMessageA(hWnd, IDC_BLOCKLIST, LB_RESETCONTENT, 0, 0L);

    pdbgah = g_dbgahRoot.pdbgahNext;

    while (pdbgah != &g_dbgahRoot)
    {
        DbgSetAutoLeak(pdbgah);
        cLive++;
        cbLive += (ULONG)(pdbgah->cbRequest - pdbgah->cbHeader);
        SendDlgItemMessageA(hWnd, IDC_BLOCKLIST, LB_ADDSTRING, 0, (LPARAM)pdbgah);
        pdbgah = pdbgah->pdbgahNext;
    }

    if (idxTop != CB_ERR)
        SendDlgItemMessageA(hWnd, IDC_BLOCKLIST, LB_SETTOPINDEX, (WPARAM)idxTop, 0L);

    wsprintfA(szOutBuff, "%7ld Bytes in %ld Blocks", cbLive, cLive);
    SetDlgItemTextA(hWnd, IDE_LIVEBLOCK, szOutBuff);

    SetSumSelection();
}

/*
 -  BlockListNotify
 -
 *  Purpose:
 *      We capture double click notifications on the block list and
 *      bring up a hex editor (that is read-only) for the memory in
 *      that block.  A nasty side effect of this is that we grab the
 *      critical section on the heap before putting up the dialog and
 *      we don't release it until we return from the dialog.  We have
 *      to do this to prevent the memory from going away out from under
 *      us.
 */

void BlockListNotify(WORD wNotify, HWND  hWnd)
{
    LONG    idx;
    PDBGAH  pdbgah = NULL;
    PDBGAH  pdbgahCopy = NULL;
    ULONG   cb;
    BOOL    fDoDialog = FALSE;

    if (wNotify == LBN_DBLCLK)
    {
        idx = SendMessageA(hWnd, LB_GETCURSEL, 0, 0L);

        if (idx != CB_ERR)
            pdbgah = (PDBGAH)SendMessageA(hWnd, LB_GETITEMDATA, (WPARAM)idx, 0L);
        if ((LONG_PTR)pdbgah == (LONG_PTR)CB_ERR)
            goto ret;

        if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
            {
                LOCK_GLOBALS;

                if (FBlockStillValid(pdbgah))
                {
                    cb = ActualSizeFromRequestSize(pdbgah->cbRequest, pdbgah->fSymbols);
                    pdbgahCopy = (PDBGAH)LocalAlloc(LPTR, cb);

                    if (pdbgahCopy != NULL)
                    {
                        memcpy(pdbgahCopy, pdbgah, cb);
                        *(DWORD_PTR *)pdbgahCopy->adwGuard = (DWORD_PTR)ClientFromActual(pdbgah);
                        fDoDialog = TRUE;
                    }
                }
            }

            if (fDoDialog)
            {
                DialogBoxParamA(g_hinstMain,
                        MAKEINTRESOURCEA(IDD_BLOCKEDIT),
                        hWnd, BlockDlgProc, (LPARAM)pdbgahCopy);

                LocalFree((HGLOBAL)pdbgahCopy);
            }
        }
        else
        {
            idx = SendMessageA(hWnd, LB_GETCURSEL, 0, 0L);

            if (idx != CB_ERR)
                pdbgah = (PDBGAH)SendMessageA(hWnd, LB_GETITEMDATA, (WPARAM)idx, 0L);
            if ((LONG_PTR)pdbgah == (LONG_PTR)CB_ERR)
                goto ret;

            g_pvMemSearch = ClientFromActual(pdbgah);

            RefreshHeapView(ghWnd);
        }
    }
    else if (wNotify == LBN_SELCHANGE)
    {
        SetSumSelection();
    }

ret:
    return;
}


/*
 -  SetWindowTitle
 -
 *  Purpose:
 *      Updates the dialog window title to include the process
 *      on whose behalf we were invoked.
 */

void SetWindowTitle(HWND hWnd, LPSTR lpcText)
{
    LPSTR   pszModule;
    char    szTitle[80];
    char    szModule[MAX_PATH];

    GetModuleFileNameA(NULL, szModule, MAX_PATH);

    pszModule = (LPSTR)(szModule + lstrlenA(szModule));

    while (*pszModule-- != '\\') ;

    pszModule += 2;

    CharUpperBuffA(pszModule, lstrlenA(pszModule));

    wsprintfA(szTitle, lpcText, pszModule);
    SetWindowTextA(hWnd, szTitle);
}


/*
 -  BltIcon
 -
 *  Purpose:
 *      BitBlts a bitmap in the client area of the dialog when the dialog
 *      is minimized (IsIconic).  This is to get around a Windows limitation
 *      of not being able to associate an icon with a dialog.
 */

void BltIcon(HWND hWnd)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT rc;

    hdc = BeginPaint(hWnd, &ps);

    if (hdc)
    {
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, ghbrBkgnd);
        DrawIcon(hdc, 0, 0, ghIcon);
    }

    EndPaint(hWnd, &ps);
}

/*
 -  HeapMonDlgProc
 -
 *  Purpose:
 *      This function (which only executes on Win32 in debug builds) is
 *      used to monitor heap usage on the any heap created by glheap.  We
 *      will attempt to calculate total heap usage, heap fragmentation,
 *      and percentage per heap of total allocations.
 */

INT_PTR WINAPI
HeapMonDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            ghWnd = hWnd;
            CheckRadioButton(hWnd, IDC_SORTCFLAG, IDC_SORTSIZE, idSort);
            SetRGBValues();
            SetWindowTitle(hWnd, "Heap Monitor - %s");
            fDlgUp = TRUE;
            RefreshHeapView(hWnd);
            ShowWindow(hWnd, SW_SHOW);
            break;

        case WM_SYSCOLORCHANGE:
            SetRGBValues();
            RefreshHeapView(hWnd);
            break;

        case WM_MEASUREITEM:
            MeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam);
            break;

        case WM_DRAWITEM:
            DrawItem((WORD)(((LPDRAWITEMSTRUCT)lParam)->CtlID),
                    (LPDRAWITEMSTRUCT)lParam);
            break;

#ifndef WIN16
        case WM_COMPAREITEM:
            return CompareItem((UINT)wParam, (COMPAREITEMSTRUCT *)lParam);
#endif //ndef WIN16

        case WM_PAINT:
            if (IsIconic(hWnd))
                BltIcon(hWnd);

            return FALSE;

        case WM_ERASEBKGND:
            return IsIconic(hWnd);

        case WM_CLOSE:
            if (ghBlockWnd)
                SendMessageA(ghBlockWnd, WM_CLOSE, 0, 0L);
            fDlgUp = FALSE;
#ifdef WIN16
            EndDialog(hWnd, 0);
            return TRUE;
#else
            return EndDialog(hWnd, 0);
#endif

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_REFRESH:
                    RefreshHeapView(hWnd);
                    break;

                case IDC_VIRTUAL:
#ifndef WIN16
                    DoMemoryBrowse(g_hinstMain, hWnd, GetCurrentProcessId());
#else
                    MessageBox(hWnd, "No Virtual Memory browser on Win16", "MSHTMDBG info", MB_OK);
#endif // ndef WIN16 else
                    break;

                case IDC_DUMPHEAPS:
#ifndef WIN16
                    DbgExDumpProcessHeaps();
#else
                    MessageBox(hWnd, "Dump heaps not available on win16", "MSHTMDBG info", MB_OK);
#endif // ndef WIN16 else
                    break;

                case IDC_SORTCFLAG:
                case IDC_SORTTYPE:
                case IDC_SORTNAME:
                case IDC_SORTADDRESS:
                case IDC_SORTALLOC:
                case IDC_SORTSIZE:
                    idSort = GET_WM_COMMAND_ID(wParam, lParam);
                    CheckRadioButton(hWnd, IDC_SORTCFLAG, IDC_SORTSIZE, idSort);
                    RefreshHeapView(hWnd);
                    break;

                case IDC_BLOCKLIST:
                    BlockListNotify(GET_WM_COMMAND_CMD(wParam, lParam), GET_WM_COMMAND_HWND(wParam, lParam));
                    break;

                case IDC_SHOWMETERS:
                    g_fShowMeters = !!IsDlgButtonChecked(hWnd, IDC_SHOWMETERS);
                    RefreshHeapView(hWnd);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/*
 -  MeasureItem
 -
 *  Purpose:
 *      Returns the height of the system font since that's what we draw with.
 *
 *  Parameters:
 *      hwnd        hwnd of dialog
 *      pmis        measureitemstruct from WM_MEASUREITEM call
 */

void MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis)
{
    HDC         hDC = GetDC(hwnd);
    HANDLE      hFont;
    TEXTMETRICA tm;

    hFont = GetStockObject(SYSTEM_FONT);

    hFont = SelectObject(hDC, hFont);
    GetTextMetricsA(hDC, &tm);
    SelectObject(hDC, hFont);
    ReleaseDC(hwnd, hDC);

    pmis->itemHeight = tm.tmHeight;
}

/*
 -  CompareItem
 -
 *  Purpose:
 *      Based on the specified sort order, we evaluate the two blocks
 *      handed to us.  We return -1 if the first one goes before the
 *      second one, zero if they are equal (never happens), and 1 if
 *      the first one goes after the second one.  We apply a secondary
 *      sort to the 2 fields that could potentially be equal: Name &
 *      Size.  The secondary sort is on ulAllocNum which is always
 *      unique.  This just in: we now do a desending sort on Size and
 *      AllocNum since the user is most likely interested in seeing the
 *      largest blocks or the most recently allocated blocks at the top.
 */
#ifndef WIN16
int CompareItem(UINT idCtl, LPCOMPAREITEMSTRUCT pcis)
{
    INT_PTR iOrder = 0, i1, i2;
    PDBGAH  pdbgah1, pdbgah2;
    char    achType[1024];

    if (idCtl != IDC_BLOCKLIST)
        goto ret;

    {
        LOCK_GLOBALS;

        pdbgah1 = (PDBGAH)pcis->itemData1;
        pdbgah2 = (PDBGAH)pcis->itemData2;

        if (!FBlockStillValid(pdbgah1) || !FBlockStillValid(pdbgah2))
            goto ret;

        switch (idSort)
        {
            case IDC_SORTCFLAG:
                i1 = ((!!pdbgah1->fCoFlag) << 1) + !!pdbgah1->fLeaked;
                i2 = ((!!pdbgah2->fCoFlag) << 1) + !!pdbgah2->fLeaked;
                iOrder = i1 - i2;
                break;

            case IDC_SORTTYPE:
                if (g_fShowMeters)
                    iOrder = lstrcmpA(DbgExMtGetName(pdbgah1->mt), DbgExMtGetName(pdbgah2->mt));
                else
                {
                    lstrcpynA(achType, GetRunTimeType(ClientFromActual(pdbgah1)), ARRAY_SIZE(achType));
                    iOrder = lstrcmpA(achType, GetRunTimeType(ClientFromActual(pdbgah2)));
                }
                break;

            case IDC_SORTNAME:
                if (g_fShowMeters)
                    iOrder = lstrcmpA(DbgExMtGetDesc(pdbgah1->mt), DbgExMtGetDesc(pdbgah2->mt));
                else
                    iOrder = lstrcmpA(pdbgah1->szName, pdbgah2->szName);
                break;

            case IDC_SORTADDRESS:
                iOrder = (INT_PTR)ClientFromActual(pdbgah1) - (INT_PTR)ClientFromActual(pdbgah2);
                break;

            case IDC_SORTSIZE:
                iOrder =    ((int)pdbgah2->cbRequest - (int)pdbgah2->cbHeader)
                         -  ((int)pdbgah1->cbRequest - (int)pdbgah1->cbHeader);
                break;
        }

        if (iOrder == 0)
        {
            iOrder = (int)pdbgah2->iAllocated - (int)pdbgah1->iAllocated;
        }

        if (iOrder < 0)
            iOrder = -1;
        else if (iOrder > 0)
            iOrder = 1;
        else
            iOrder = 0;
    }

ret:
    return (int)iOrder;
}
#endif

/*
 -  DrawItem
 -
 *  Purpose:
 *      Handles WM_DRAWITEM for both Heap combo-box and Block List.
 *
 *  Parameters:
 *      wId         Control Id of the control we are drawing into
 *      pdis        LPDRAWITEMSTRUCT passed from the WM_DRAWITEM message.
 */

VOID DrawItem(WORD wId, LPDRAWITEMSTRUCT pdis)
{
    COLORREF    crText = 0, crBack = 0;

    if((int)pdis->itemID < 0)
        return;

    if((ODA_DRAWENTIRE | ODA_SELECT) & pdis->itemAction)
    {
        if(pdis->itemState & ODS_SELECTED)
        {
            // Select the appropriate text colors
            crText = SetTextColor(pdis->hDC, rgbHiliteText);
            crBack = SetBkColor(pdis->hDC, rgbHiliteColor);
        }

        // parse and spit out bmps and text
        OutTextFormat(wId, pdis);

        // Restore original colors if we changed them above.
        if(pdis->itemState & ODS_SELECTED)
        {
            SetTextColor(pdis->hDC, crText);
            SetBkColor(pdis->hDC,   crBack);
        }
    }

    if((ODA_FOCUS & pdis->itemAction) || (ODS_FOCUS & pdis->itemState))
        DrawFocusRect(pdis->hDC, &pdis->rcItem);
}


/*
 -  OutTextFormat
 -
 *  Purpose:
 *      Used to format and 'TextOut' both Heap combo-box items and
 *      Block List items.  Parsing evaluates the string as follows:
 *      otherwise, outtext the line
 *
 *  Parameters:
 *      wId         Control Id of the control we are drawing into
 *      pdis        from DrawItem from WM_DRAWITEM msg
 */

void OutTextFormat(WORD wId, LPDRAWITEMSTRUCT pdis)
{
    char szItem[512];

    LOCK_GLOBALS;

    if (wId == IDC_HEAPLIST)
    {
        wsprintfA(szItem, "%s", "Process Heap");
        ExtTextOutA(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top + 1,
                    ETO_CLIPPED|ETO_OPAQUE, &pdis->rcItem, szItem, lstrlenA(szItem), NULL);
    }
    else if (wId == IDC_BLOCKLIST)
    {
        PDBGAH  pdbgah  = (PDBGAH)pdis->itemData;
        BOOL    fFound  = FALSE;
        BOOL    fSearch = FALSE;

        if (FBlockStillValid(pdbgah))
        {
            if (g_pvMemSearch)
            {
                int    cb  = pdbgah->cbRequest - pdbgah->cbHeader;
                void **ppv = (void**)ClientFromActual(pdbgah);

                if (ppv == (void**)g_pvMemSearch)
                    fSearch = TRUE;

                while (cb >= sizeof(void*))
                {
                    if (*ppv == g_pvMemSearch)
                    {
                        fFound = TRUE;
                        break;
                    }
                    cb -= sizeof(void*);
                    ppv++;
                }
            }
            wsprintfA(szItem, "%c%c%c%c%7ld %8lX %6ld %-32.32s %s",
                pdbgah->fCoFlag ? 'C' : ' ',
                pdbgah->fLeaked ? 'L' : ' ',
                fFound          ? 'F' : ' ',
                fSearch         ? '>' : ' ',
                pdbgah->iAllocated,
                ClientFromActual(pdbgah),
                pdbgah->cbRequest - pdbgah->cbHeader,
                g_fShowMeters ? DbgExMtGetName(pdbgah->mt) : GetRunTimeType(ClientFromActual(pdbgah)),
                g_fShowMeters ? DbgExMtGetDesc(pdbgah->mt) : pdbgah->szName);

            ExtTextOutA(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top + 1,
                        ETO_CLIPPED|ETO_OPAQUE, &pdis->rcItem, szItem, lstrlenA(szItem), NULL);
        }
    }
}


/*
 -  SetRGBValues
 -
 *  Purpose:
 *      To set various system colors in static variables.  Called at
 *      init time and when system colors change.
 */

VOID SetRGBValues(VOID)
{
    rgbWindowColor = GetSysColor(COLOR_WINDOW);
    rgbHiliteColor = GetSysColor(COLOR_HIGHLIGHT);
    rgbWindowText  = GetSysColor(COLOR_WINDOWTEXT);
    rgbHiliteText  = GetSysColor(COLOR_HIGHLIGHTTEXT);
    rgbGrayText    = GetSysColor(COLOR_GRAYTEXT);
}


/*
 -  SetSumSelection
 -
 *  Purpose:
 *      Calcualtes to total size of the blocks selected in the Block
 *      List list-box (which is a multi-selction listbox).
 */

void SetSumSelection(void)
{
    int     i;
    int     *rgIdx = NULL;
    LONG    cSel;
    size_t  cbTotal = 0;
    char    szBuff[64];
    PDBGAH  pdbgah;

    LOCK_GLOBALS;

    cSel = SendDlgItemMessageA(ghWnd, IDC_BLOCKLIST, LB_GETSELCOUNT, 0, 0L);

    if (cSel > 0)
    {
        rgIdx = (int *)LocalAlloc(LPTR, cSel*sizeof(int));
        if (rgIdx)
            cSel = SendDlgItemMessageA(ghWnd, IDC_BLOCKLIST, LB_GETSELITEMS, cSel, (LPARAM)rgIdx);
    }

    // Note that rgIdx must be non-null if cSel > 0
    for (i = 0; i < cSel; i++)
    {
        pdbgah = (PDBGAH)SendDlgItemMessageA(ghWnd, IDC_BLOCKLIST,
                LB_GETITEMDATA, rgIdx[i], 0L);

        if (FBlockStillValid(pdbgah))
            cbTotal += pdbgah->cbRequest - pdbgah->cbHeader;
    }

    if (cbTotal == 0)
        szBuff[0] = 0;
    else
        wsprintfA(szBuff, "%7ld Bytes Selected", cbTotal);
    SetDlgItemTextA(ghWnd, IDE_SUMSEL, szBuff);

    if (rgIdx)
    {
        LocalFree((HGLOBAL)rgIdx);
    }
}

/*
 -  FormatBlockRow
 -
 *  Purpose:
 *      Converts the 16 bytes following the memory address at lpbRow
 *      as 4 hex DWORDs, or 8 hex WORDs, or 16 hex BYTEs and as 16
 *      printable characters.  A string is than constructed as follows:
 *
 *      ADDRESS \t DWORD1 \t DWORD2 \t DWORD3 \t DWORD4 \t CH1 \t ... \t CH16
 *  or
 *      ADDRESS \t WORD1  \t WORD2  \t ...    \t WORD8  \t CH1 \t ... \t CH16
 *  or
 *      ADDRESS \t BYTE1  \t BYTE2  \t ...    \t BYTE16 \t CH1 \t ... \t CH16
 *
 *      This string is then output to the list box in the BlockEditDlg.
 */

void FormatBlockRow(LPSTR psz, BYTE * pbRow, BYTE * pbBase, UINT cbRequest, BYTE * pbReal)
{
    UINT   cbRow = min((UINT)16, (UINT)(cbRequest - (pbRow - pbBase)));
    UINT   cbLine;
    BYTE * pb;
    UINT   ib, ich;
    int    c;

    psz += wsprintfA(psz, "%08lX [+%04lX] ", pbReal + (pbRow - pbBase), pbRow - pbBase);

    if (idDataType == IDC_DWORD)
        cbLine = (8*4) + 3;
    else if (idDataType == IDC_WORD)
        cbLine = (4*8) + 7;
    else
        cbLine = (2*16) + 15;

    memset(psz, ' ', cbLine);

    for (ib = 0, pb = pbRow; ib < cbRow; ++ib, ++pb)
    {
        if (idDataType == IDC_DWORD)
            ich = 2*(3 - (ib%4)) + 9*(ib/4);
        else if (idDataType == IDC_WORD)
            ich = 2*(1 - (ib%2)) + 5*(ib/2);
        else
            ich = 3*ib;

        c = (*pb & 0xF);
        psz[ich+1] = (char) (c + ((c < 10) ? '0' : ('A' - 10)));
        c = (*pb & 0xF0) >> 4;
        psz[ich] = (char) (c + ((c < 10) ? '0' : ('A' - 10)));
    }

    psz += cbLine;
    memset(psz, ' ', 2 + 16);
    psz += 2;

    psz[16] = 0;

    for (ib = 0, pb = pbRow; ib < cbRow; ++ib, ++pb)
    {
        if (*pb > 31 && *pb < 128)
            *psz++ = *pb;
        else
            *psz++ = '.';
    }
}


/*
 -  OutBlockRowFormat
 -
 *  Purpose:
 *      Used to format and 'TextOut' the items in the Block Memory
 *      Edit list-box.  Parsing evaluates the string as follows:
 *      otherwise, outtext the line
 *
 *  Parameters:
 *      wId         Control Id of the control we are drawing into
 *      pdis        from DrawItem from WM_DRAWITEM msg
 *      pdbgah      Pointer to the block we are editing
 */

void OutBlockRowFormat(WORD wId, LPDRAWITEMSTRUCT pdis, PDBGAH pdbgah)
{
    char szItem[256];

    if (wId != IDC_BE_MEMORY)
        return;

    FormatBlockRow(szItem, (LPBYTE)pdis->itemData,
            (LPBYTE)(pdbgah + 1) + pdbgah->cbHeader,
            pdbgah->cbRequest - pdbgah->cbHeader,
            (LPBYTE)(*(DWORD_PTR *)pdbgah->adwGuard));

    //  Draw the text
    ExtTextOutA(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top + 1,
        ETO_OPAQUE, &pdis->rcItem, szItem, lstrlenA(szItem), NULL);
}


/*
 -  DrawBlockRow
 -
 *  Purpose:
 *      Handles WM_DRAWITEM for a row in the Block Memory Edit list-box.
 *
 *  Parameters:
 *      wId         Control Id of the control we are drawing into
 *      pdis        LPDRAWITEMSTRUCT passed from the WM_DRAWITEM message.
 *      pdbgah      Pointer to the block we are editing
 */

void DrawBlockRow(WORD wId, LPDRAWITEMSTRUCT pdis, PDBGAH pdbgah)
{
    COLORREF    crText = 0, crBack = 0;

    if((int)pdis->itemID < 0)
        return;

    if((ODA_DRAWENTIRE | ODA_SELECT) & pdis->itemAction)
    {
        if(pdis->itemState & ODS_SELECTED)
        {
            // Select the appropriate text colors
            crText = SetTextColor(pdis->hDC, rgbHiliteText);
            crBack = SetBkColor(pdis->hDC, rgbHiliteColor);
        }

        // parse and spit out bmps and text
        OutBlockRowFormat(wId, pdis, pdbgah);

        // Restore original colors if we changed them above.
        if(pdis->itemState & ODS_SELECTED)
        {
            SetTextColor(pdis->hDC, crText);
            SetBkColor(pdis->hDC,   crBack);
        }
    }

    if((ODA_FOCUS & pdis->itemAction) || (ODS_FOCUS & pdis->itemState))
        DrawFocusRect(pdis->hDC, &pdis->rcItem);
}


/*
 -  SetBlockEditData
 -
 *  Purpose:
 *      Fills in all the fields on the Block Memory Edit dialog.  Adds
 *      data items to the owner-drawn memory list-box.  Data items are
 *      pointers into every 16 bytes of the memory in the block.  The
 *      owner-draw code then formats each data item in a human readable
 *      format much like a memory window in a debugger.
 *
 *  NOTE:
 *      The initialization of lpb is (pdbgah + 1) instead of
 *      RequestFromActual(pdbgah) because this is not the real block, it is a
 *      copy of the real block and the data is appended to the end of
 *      this copy (see BlockListNotify() function).  THIS IS BY DESIGN!
 */

void SetBlockEditData(HWND hWnd, PDBGAH pdbgah)
{
    UINT    i;
    char    szOutBuff[1024];
    ULONG   cb = pdbgah->cbRequest - pdbgah->cbHeader;
    LPBYTE  lpb = (LPBYTE)(pdbgah + 1) + pdbgah->cbHeader;
    LPBYTE  lpbEnd;
    LPSTR   pszType;

    SendDlgItemMessageA(hWnd, IDC_BE_MEMORY, LB_RESETCONTENT, 0, 0L);
    SendDlgItemMessageA(hWnd, IDC_CALLSTACK, LB_RESETCONTENT, 0, 0L);

    pszType = GetRunTimeType(ClientFromActual(pdbgah));

    if (*pszType == 0 && *pdbgah->szName == 0)
        wsprintfA(szOutBuff, "Memory - 0x%08lX", pdbgah->adwGuard[0]);
    else
        wsprintfA(szOutBuff, "Memory - %s%s%s", pszType, pszType ? " " : "",
            pdbgah->szName);
    SetWindowTextA(hWnd, szOutBuff);

    if (DbgExIsTagEnabled(tagLeaks))
    {
        SYMBOL_INFO * psi = (pdbgah->fSymbols) ? SymbolsFromBlock(pdbgah) : NULL;
        CHAR achSymbol[256];

        for (i = 0; i < ARRAY_SIZE(pdbgah->dwEip); i++)
        {
            if (!pdbgah->dwEip[i])
                break;

            GetStringFromSymbolInfo(pdbgah->dwEip[i],
                                    (psi) ? psi + i : NULL,
                                    achSymbol);

            LPSTR psz = achSymbol;

            while (*psz == ' ')
                ++psz;

            SendDlgItemMessageA(hWnd, IDC_CALLSTACK, LB_ADDSTRING, 0, (LPARAM)psz);
        }
    }

    for (lpbEnd = lpb + cb; lpb < lpbEnd; lpb += 16)
        SendDlgItemMessageA(hWnd, IDC_BE_MEMORY, LB_ADDSTRING, 0, (LPARAM)lpb);
}

/*
 -  BlockDlgProc
 -
 *  Purpose:
 *      This is the dialog procedure for the Block Memory Edit dialog
 *      box.  This dialog displays a block of memory much like a
 *      debugger would.  This is a read-only display of the memory.
 */

INT_PTR WINAPI
BlockDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PDBGAH   pdbgah;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            ghBlockWnd = hWnd;
            pdbgah = (PDBGAH)lParam;
            CheckRadioButton(hWnd, IDC_DWORD, IDC_BYTE, idDataType);
            SetBlockEditData(hWnd, pdbgah);
            break;

        case WM_MEASUREITEM:
            MeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam);
            break;

        case WM_DRAWITEM:
            DrawBlockRow((WORD)(((LPDRAWITEMSTRUCT)lParam)->CtlID),
                    (LPDRAWITEMSTRUCT)lParam, pdbgah);
            break;

        case WM_CLOSE:
            ghBlockWnd = NULL;
            EndDialog(hWnd, 0);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_DWORD:
                case IDC_WORD:
                case IDC_BYTE:
                    idDataType = LOWORD(wParam);
                    CheckRadioButton(hWnd, IDC_DWORD, IDC_BYTE, idDataType);
                    SetBlockEditData(hWnd, pdbgah);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


void    ViewBlock(HWND phWnd, LPVOID lpcBlock)
{
    DialogBoxParamA(g_hinstMain,
            MAKEINTRESOURCEA(IDD_BLOCKEDIT),
            phWnd, BlockDlgProc, (LPARAM) lpcBlock);
}

#ifndef WIN16
// Virtual Memory Browser -----------------------------------------------------

struct LONGDATA
{
   DWORD dwValue;
   LPCSTR szText;
};

#ifndef MEM_IMAGE
#define SEC_FILE           0x800000
#define SEC_IMAGE         0x1000000
#define SEC_RESERVE       0x4000000
#define SEC_COMMIT        0x8000000
#define SEC_NOCACHE      0x10000000
#define MEM_IMAGE         SEC_IMAGE
#endif

#define TABLEENTRY(Prefix, Value)        Prefix##Value, #Value

LONGDATA PageFlags[] = {
   TABLEENTRY(PAGE_, NOACCESS),
   TABLEENTRY(PAGE_, READONLY),
   TABLEENTRY(PAGE_, READWRITE),
   TABLEENTRY(PAGE_, WRITECOPY),
   TABLEENTRY(PAGE_, EXECUTE),
   TABLEENTRY(PAGE_, EXECUTE_READ),
   TABLEENTRY(PAGE_, EXECUTE_READWRITE),
   TABLEENTRY(PAGE_, EXECUTE_WRITECOPY),
   TABLEENTRY(PAGE_, GUARD),
   TABLEENTRY(PAGE_, NOCACHE),
   TABLEENTRY(0, 0)
};

LONGDATA MemFlags[] = {
   TABLEENTRY(MEM_, COMMIT),
   TABLEENTRY(MEM_, RESERVE),
   TABLEENTRY(MEM_, DECOMMIT),
   TABLEENTRY(MEM_, RELEASE),
   TABLEENTRY(MEM_, FREE),
   TABLEENTRY(MEM_, PRIVATE),
   TABLEENTRY(MEM_, MAPPED),
   TABLEENTRY(MEM_, TOP_DOWN),
   TABLEENTRY(MEM_, IMAGE),
   TABLEENTRY(0, 0)
};

#define COLS        16


extern DWORD   rgbWindowColor;
extern DWORD   rgbHiliteColor;
extern DWORD   rgbWindowText;
extern DWORD   rgbHiliteText;
extern DWORD   rgbGrayText;
extern DWORD   rgbDDWindow;
extern DWORD   rgbDDHilite;

int         tabs[COLS];
HANDLE      hProcess;

void    ViewBlock(HWND phWnd, LPVOID lpcBlock);
extern  void SetRGBValues(VOID);

int         cols[COLS] = {70, 13, 70, 11, 11, 11, 11, 65, 11, 11, 11, 11, 11, 65, 80, 92};

/////////////////////////////////////////////////////////////


LPCSTR GetAllocFlagStr(DWORD dwFlag)
{
    static char sszBuffer[20];

    sszBuffer[0] = '\0';

    if (dwFlag & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
        lstrcatA(sszBuffer, "R");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        lstrcatA(sszBuffer, "W");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        lstrcatA(sszBuffer, "X");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
        lstrcatA(sszBuffer, "C");
    else
        lstrcatA(sszBuffer, ".");

    return sszBuffer;
}

LPCSTR GetCurrentFlagStr(DWORD dwFlag)
{
    static char sszBuffer[20];

    sszBuffer[0] = '\0';

    if (dwFlag & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))
        lstrcatA(sszBuffer, "R");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        lstrcatA(sszBuffer, "W");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        lstrcatA(sszBuffer, "X");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
        lstrcatA(sszBuffer, "C");
    else
        lstrcatA(sszBuffer, ".");

    lstrcatA(sszBuffer, "\t");

    if (dwFlag & (PAGE_GUARD))
        lstrcatA(sszBuffer, "G");
    else
        lstrcatA(sszBuffer, ".");

    return sszBuffer;
}


LPCSTR GetStateStr (DWORD dwFlag)
{
   if (dwFlag == MEM_COMMIT) return "C";
   if (dwFlag == MEM_RESERVE) return "R";
   if (dwFlag == MEM_FREE) return "F";

   return "?";
}


LPCSTR  GetFlagStr(DWORD dwFlag)
{
    if (dwFlag == MEM_PRIVATE) return "Private";
    if (dwFlag == MEM_MAPPED) return "Mapped";
    if (dwFlag == MEM_TOP_DOWN) return "Top Down";
    if (dwFlag == MEM_IMAGE) return "Image";

    return "Unknown";
}


/*
 * IsGlobalLocalHeap
 *
 *  Takes a Base Address of a chunk of memory, returns TRUE if this Base
 *  is in a Local or Global heap
 *
 */

UINT IsGlobalLocalHeap(LPVOID   lpBaseAddress)
{
    if (lpBaseAddress == GetProcessHeap())
        return 1;

    return 0;
}

/////////////////////////////////////////////////////////////

void ConstructMemInfoLine (PMEMORY_BASIC_INFORMATION pMBI,
   LPSTR szLine) {

   LPCSTR sz;

   // BaseAddress
   wsprintfA(szLine, "%08X\t", pMBI->BaseAddress);

   // State
   wsprintfA(strchr(szLine, 0), "%s\t", GetStateStr(pMBI->State));

   // AllocationBase
   if ((pMBI->BaseAddress != pMBI->AllocationBase) && pMBI->State != MEM_FREE)
        wsprintfA(strchr(szLine, 0), "%08X\t", pMBI->AllocationBase);
    else
        lstrcatA(szLine, "\t");

   // AllocationProtect
   if (pMBI->State != MEM_FREE)
      sz = GetAllocFlagStr(pMBI->AllocationProtect);
   else
      sz = "\t\t\t";

   wsprintfA(strchr(szLine, 0), "%s\t", sz);

   // RegionSize
   wsprintfA(strchr(szLine, 0), "%05lX\t",
      pMBI->RegionSize / 4096);


   // Protect
   if ((pMBI->State != MEM_FREE) &&
       (pMBI->State != MEM_RESERVE))
   {
      sz = GetCurrentFlagStr(pMBI->Protect);
   }
   else
      sz = "\t\t\t\t";

   wsprintfA(strchr(szLine, 0), "%s\t", sz);

   // Type
   if (pMBI->State != MEM_FREE)
   {
      sz = GetFlagStr(pMBI->Type);
   }
   else
      sz = "";

   wsprintfA(strchr(szLine, 0), "%s", sz);
}

UINT        IsAppBasedOk(PMEMORY_BASIC_INFORMATION pMBI, LPSTR lpText, HANDLE hProc)
{
    UINT                        iRet = 0;
    LPBYTE                      lpBase;
    PIMAGE_DOS_HEADER           dosHeader;
    PIMAGE_NT_HEADERS           pNTHeader;
    PIMAGE_SECTION_HEADER       pSection;
    PIMAGE_SECTION_HEADER       pEData = NULL;
    PIMAGE_EXPORT_DIRECTORY     pExport;
    LPBYTE                      lpMyBase;
    DWORD                       dwRead;
    char                        lsDLLName[256];

    lpBase = (LPBYTE)pMBI->AllocationBase;

    lpMyBase = (LPBYTE)VirtualAlloc(NULL, pMBI->RegionSize, MEM_COMMIT, PAGE_READWRITE);
    if (ReadProcessMemory(hProc, lpBase, lpMyBase, pMBI->RegionSize, &dwRead))
    {
        dosHeader = (PIMAGE_DOS_HEADER) lpMyBase;

        if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE)
        {
            pNTHeader = (PIMAGE_NT_HEADERS) (lpMyBase + dosHeader->e_lfanew);
            if (!IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)) && (pNTHeader->Signature == IMAGE_NT_SIGNATURE))
            {
                UINT        liI;

                /* Are we at the right base? */
                if (pNTHeader->OptionalHeader.ImageBase != (ULONG_PTR) lpBase)
                    iRet = 2;
                else
                    iRet = 1;

                lstrcatA(lpText, "\t");
                pSection = IMAGE_FIRST_SECTION(pNTHeader);
                for (liI = 0; liI < pNTHeader->FileHeader.NumberOfSections; liI++)
                {
                    if (    lstrlenA((char *)pSection->Name) == 6
                        &&  !lstrcmpA(".edata", (char *)pSection->Name))
                        pEData = pSection;

                    if ((lpBase + pSection->VirtualAddress) == pMBI->BaseAddress)
                    {
                        char        lsTemp[16];

                        wsprintfA(lsTemp, "%-8.8s", pSection->Name);
                        lstrcatA(lpText, lsTemp);
                    }

                    pSection++;
                }


                /* Is this a DLL? */
                if (pEData)
                {

                    pExport = (PIMAGE_EXPORT_DIRECTORY)
                        VirtualAlloc(NULL,
                            sizeof(IMAGE_EXPORT_DIRECTORY),
                            MEM_COMMIT, PAGE_READWRITE);

                    if (pExport != NULL)
                    {
                        ReadProcessMemory(hProc,
                            (lpBase + pEData->VirtualAddress),
                            pExport,
                            sizeof(IMAGE_EXPORT_DIRECTORY),
                            &dwRead);

                        ReadProcessMemory(hProc,
                            (lpBase + pExport->Name),
                            lsDLLName,
                            256,
                            &dwRead);

                        lstrcatA(lpText, "\t");
                        lstrcatA(lpText, lsDLLName);

                        if (iRet == 2)
                        {
                            wsprintfA(lsDLLName, "\t(%08lX)", pNTHeader->OptionalHeader.ImageBase);
                            lstrcatA(lpText, lsDLLName);
                        }

                        VirtualFree(pExport, 0, MEM_RELEASE);
                    }

                }



            }

        }
    }

    VirtualFree(lpMyBase, 0, MEM_RELEASE);

    return iRet;
}


void    SetTheColour(LPDRAWITEMSTRUCT lpdis, PMEMORY_BASIC_INFORMATION pMBI, LPSTR  lpText)
{
    UINT        iHeapType;

    if (pMBI->State == MEM_FREE)
    {
        SetTextColor(lpdis->hDC, GetSysColor(COLOR_BTNFACE));
        return;
    }

    if (pMBI->State == MEM_RESERVE)
    {
        PMEMORY_BASIC_INFORMATION   pNext = (PMEMORY_BASIC_INFORMATION) ListBox_GetItemData(lpdis->hwndItem, lpdis->itemID + 1);
        if ((((ULONG_PTR)pNext) != (ULONG_PTR)(-1)) && (pNext->Protect & PAGE_GUARD))
        {
            SetTextColor(lpdis->hDC, RGB(0, 0, 128));   // Stack
            return;
        }
    }

    if (pMBI->State == MEM_COMMIT)
    {
        UINT        iIsApp;

        if (pMBI->Protect & PAGE_GUARD)
        {
            SetTextColor(lpdis->hDC, RGB(0, 0, 128));   // Stack
            return;
        }
        else
        {
            PMEMORY_BASIC_INFORMATION   pPrev = (PMEMORY_BASIC_INFORMATION) ListBox_GetItemData(lpdis->hwndItem, lpdis->itemID - 1);
            if (pPrev->Protect & PAGE_GUARD)
            {
                SetTextColor(lpdis->hDC, RGB(0, 0, 128));   // Stack
                return;
            }
        }

        iIsApp = IsAppBasedOk(pMBI, lpText, hProcess);

        if (iIsApp == 1)
        {
            SetTextColor(lpdis->hDC, RGB(0, 128, 0));   // Image
            return;
        }
        else if (iIsApp == 2)
        {
            SetTextColor(lpdis->hDC, RGB(255, 0, 0));   // Image based badly
            return;
        }
    }

    iHeapType = IsGlobalLocalHeap(pMBI->AllocationBase);
    if (iHeapType == 1)
    {
        SetTextColor(lpdis->hDC, RGB(255, 0, 255)); // Our Heaps - Magenta
        return;
    }
    else if (iHeapType == 2)
    {
        SetTextColor(lpdis->hDC, RGB(0, 255, 255)); // Our Debug Heaps - Cyan
        return;
    }


    SetTextColor(lpdis->hDC, GetSysColor(COLOR_BTNTEXT));
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DrawEntireItem(LPDRAWITEMSTRUCT, int)                      *
 *                                                                          *
 ****************************************************************************/
VOID APIENTRY DrawEntireItem(
        LPDRAWITEMSTRUCT        lpdis,
        INT                     inflate)
{
    char     szLine[300];
    COLORREF crText = 0, crBack = 0;

    /* Draw line here */
    if (lpdis->itemState & ODS_SELECTED)
    {
        // Select the appropriate text colors
        crText = SetTextColor(lpdis->hDC, rgbHiliteText);
        crBack = SetBkColor(lpdis->hDC, rgbHiliteColor);
    }
    memset(szLine, ' ', sizeof(szLine) - 1);
    TextOutA(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, szLine, sizeof(szLine));

    if (lpdis->itemState & ODS_SELECTED)
    {
        SetTextColor(lpdis->hDC, crText);
        SetBkColor(lpdis->hDC,   crBack);
    }

    ConstructMemInfoLine((PMEMORY_BASIC_INFORMATION) lpdis->itemData, szLine);

    SetTheColour(lpdis, (PMEMORY_BASIC_INFORMATION) lpdis->itemData, szLine);
    if (lpdis->itemState & ODS_SELECTED)
    {
        // Select the appropriate text colors
        crText = SetTextColor(lpdis->hDC, rgbHiliteText);
        crBack = SetBkColor(lpdis->hDC, rgbHiliteColor);
    }

    TabbedTextOutA(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, szLine, lstrlenA(szLine), COLS, tabs, 0);

    // Restore original colors if we changed them above.
    if (lpdis->itemState & ODS_SELECTED)
    {
        SetTextColor(lpdis->hDC, crText);
        SetBkColor(lpdis->hDC,   crBack);
    }
}


LPSTR       ReverseString(LPSTR  lpcString)
{
    LPSTR   lpcEnd = lpcString + lstrlenA(lpcString) / 2;
    LPSTR   lpcStart;
    char    lcChar;

    lpcStart = lpcEnd - !(lstrlenA(lpcString) % 2);

    while (*lpcEnd)
    {
        lcChar = *lpcStart;
        *lpcStart = *lpcEnd;
        *lpcEnd = lcChar;

        lpcStart--;
        lpcEnd++;
    }

    return(lpcString);
}


LPSTR       DoCommas(DWORD      dwVal)
{
    static  char    szTemp[20];
    UINT            iDigitCount = 0;
    UINT            iCharCount = 0;

    while (dwVal)
    {
        szTemp[iCharCount++] = (char) ((dwVal % 10) + '0');
        iDigitCount++;
        dwVal /= 10;
        if (dwVal && ((iDigitCount % 3) == 0))
            szTemp[iCharCount++] = ',';
    }

    szTemp[iCharCount] = '\0';
    return ReverseString(szTemp);
}


void            RemoveTabs(LPSTR lpcTabbed, LPSTR lpcNoTab)
{
    UINT        iTabs = 0;
    UINT        iCharsSinceLast = 0;

    while (*lpcTabbed)
    {
        if (*lpcTabbed == '\t')
        {
            UINT        iCharsToTab = (cols[iTabs] / 4);

            if (iCharsToTab > iCharsSinceLast)
                iCharsToTab -= iCharsSinceLast;
            else
                iCharsToTab = 1;

            while (iCharsToTab--)
                *lpcNoTab++ = ' ';

            iTabs++;
            iCharsSinceLast = 0;
        }
        else
        {
            *lpcNoTab++ = *lpcTabbed;
            iCharsSinceLast++;
        }

        lpcTabbed++;
    }
    *lpcNoTab = '\0';
}

void    WINAPI  DumpMemoryList(HWND phList)
{
    OFSTRUCT        ofFile;
    char            szMemLine[300];
    char            szMemLineNoTabs[350];
    HFILE           lhFile;
    UINT            liLines = ListBox_GetCount(phList);
    UINT            liI;
    DWORD_PTR       itemData;

    lhFile = OpenFile("c:\\virtual.dmp", &ofFile, OF_CREATE);

    AssertSz(lhFile != NULL, "Failed to open file.");
    if (lhFile != NULL)
    {
        for (liI = 0; liI < liLines; liI++)
        {
            itemData = ListBox_GetItemData(phList, liI);

            ConstructMemInfoLine((PMEMORY_BASIC_INFORMATION) itemData, szMemLine);
            IsAppBasedOk((PMEMORY_BASIC_INFORMATION) itemData, szMemLine, hProcess);

            RemoveTabs(szMemLine, szMemLineNoTabs);

            _lwrite((HFILE) lhFile, szMemLineNoTabs, lstrlenA(szMemLineNoTabs));
            _lwrite((HFILE) lhFile, "\r\n", 2);
        }

        _lclose(lhFile);
    }
}


BOOL    WINAPI MemBrowseProc(HWND phDlg, UINT puMessage, WPARAM pwParam, LPARAM plParam)
{
    static DWORD    dwProcessID;

    switch (puMessage)
    {

        case WM_INITDIALOG:
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                                    TRUE, plParam);

            SetWindowTitle(phDlg, "Virtual Memory Viewer - %s");
            SendMessageA(phDlg, WM_COMMAND, GET_WM_COMMAND_MPS(0, 0, ID_REFRESH));
            break;

        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT    lpdis;

                /* Get pointer to the DRAWITEMSTRUCT */
                lpdis = (LPDRAWITEMSTRUCT) plParam;

                if (lpdis->itemID >= 0)
                {
                    if ((ODA_FOCUS & lpdis->itemAction) || (ODS_FOCUS & lpdis->itemState))
                        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
                    else
                        DrawEntireItem(lpdis, -2);
                }

                /* Return TRUE meaning that we processed this message. */
                return(TRUE);
            }
            break;

        case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT lpmis;
                lpmis = (LPMEASUREITEMSTRUCT) plParam;

                /* All the items are the same height since the list box style is
                 * LBS_OWNERDRAWFIXED
                 */
                lpmis->itemHeight = 16;
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(pwParam, plParam))
            {
                case ID_REFRESH:
                    {
                        ULONG ulTotalMem = 0, ulCodeMem = 0, ulDataMem = 0;
                        ULONG ulReserveMem = 0, ulReadOnly = 0, ulReadWrite = 0;

                        dwProcessID = plParam;

                        {
                           MEMORY_BASIC_INFORMATION     MemoryBasicInfo;
                           PMEMORY_BASIC_INFORMATION    lpMem;
                           PVOID lpAddress = 0;
                           char szLine[200];
                           int          cols[COLS] = {70, 13, 70, 11, 11, 11, 11, 65, 11, 11, 11, 11, 11, 65, 80, 92};
                           HWND         lhList = GetDlgItem(phDlg, IDC_TRACE_LIST);
                           UINT         i, j, nCol = 0;

                            for (i = 0; i < COLS; i++)
                            {
                                tabs[i] = 5 + nCol + cols[i];
                                nCol += cols[i] + 5;
                            }

                            /* Free up the list memory */
                            i = ListBox_GetCount(lhList);
                            for (j = 0; j < i; j++)
                            {
                                lpMem = (PMEMORY_BASIC_INFORMATION) ListBox_GetItemData(lhList, j);
                                LocalFree(lpMem);
                            }

                            ListBox_ResetContent(lhList);

                            ListBox_SetTabStops(lhList, 6, &tabs);

                           // Walk the virtual address space, adding
                           // entries to the list box.

                           do {
                              int x = VirtualQueryEx(hProcess, lpAddress, &MemoryBasicInfo,
                                 sizeof(MemoryBasicInfo));

                              if (x != sizeof(MemoryBasicInfo)) {
                                 // Attempt to walk beyond the range
                                 // that Windows NT allows.
                                 break;
                              }

                              szLine[0] = '\0';
                              lpMem = (PMEMORY_BASIC_INFORMATION)LocalAlloc(LMEM_FIXED, sizeof(MEMORY_BASIC_INFORMATION));
                              if (lpMem == NULL)
                              {
                                  AssertSz (0, "Insufficient memory for debugging.");
                                  continue;
                              }

                              *lpMem = MemoryBasicInfo;

                              // Construct the line to be displayed, and
                              // add it to the list box.

                              if (MemoryBasicInfo.State != MEM_FREE)
                              {
                                  if (MemoryBasicInfo.State == MEM_RESERVE)
                                      ulReserveMem += (ULONG)MemoryBasicInfo.RegionSize;
                                  else
                                  {
                                      if (IsAppBasedOk(lpMem, szLine, hProcess))
                                          ulCodeMem += (ULONG)MemoryBasicInfo.RegionSize;
                                      else
                                          ulDataMem += (ULONG)MemoryBasicInfo.RegionSize;

                                      ulTotalMem += (ULONG)MemoryBasicInfo.RegionSize;
                                  }

                                  if (MemoryBasicInfo.Protect & (PAGE_READONLY | PAGE_EXECUTE_READ))
                                      ulReadOnly += (ULONG)MemoryBasicInfo.RegionSize;
                                  else
                                      if (MemoryBasicInfo.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
                                          ulReadWrite += (ULONG)MemoryBasicInfo.RegionSize;
                              }

                              ListBox_AddString(lhList, (ULONG_PTR)lpMem);

                              // Get the address of the next region to test.
                              lpAddress = ((BYTE *) MemoryBasicInfo.BaseAddress) +
                                 MemoryBasicInfo.RegionSize;

                           } while (MemoryBasicInfo.RegionSize >= (4 * 1024));

                           wsprintfA(szLine, "%s", DoCommas(ulTotalMem));
                           SetWindowTextA(GetDlgItem(phDlg, IDC_TOTALMEM), szLine);
                           wsprintfA(szLine, "%s", DoCommas(ulCodeMem));
                           SetWindowTextA(GetDlgItem(phDlg, IDC_CODEMEM), szLine);
                           wsprintfA(szLine, "%s", DoCommas(ulDataMem));
                           SetWindowTextA(GetDlgItem(phDlg, IDC_DATAMEM), szLine);
                           wsprintfA(szLine, "%s", DoCommas(ulReserveMem));
                           SetWindowTextA(GetDlgItem(phDlg, IDC_RESERVEMEM), szLine);

                           wsprintfA(szLine, "%s", DoCommas(ulReadOnly));
                           SetWindowTextA(GetDlgItem(phDlg, IDC_READONLY), szLine);
                           wsprintfA(szLine, "%s", DoCommas(ulReadWrite));
                           SetWindowTextA(GetDlgItem(phDlg, IDC_READWRITE), szLine);
                        }
                    }
                    break;

                case ID_ADD:
                    {
                        int     liItemList[700];
                        HWND    lhList = GetDlgItem(phDlg, IDC_TRACE_LIST);
                        int     liItems = ListBox_GetSelCount(lhList);
                        int     liI;
                        ULONG   ulSize = 0;
                        char    szText[150];

                        if (liItems)
                        {
                            ListBox_GetSelItems(lhList, liItems < 700 ? liItems : 700, liItemList);

                            for (liI = 0; liI < (liItems < 700 ? liItems : 700); liI++)
                            {
                                PMEMORY_BASIC_INFORMATION   pInfo;

                                pInfo = (PMEMORY_BASIC_INFORMATION) ListBox_GetItemData(lhList, liItemList[liI]);
                                ulSize += (ULONG)pInfo->RegionSize;
                            }

                            wsprintfA(szText, "%ld (%08lX) bytes", ulSize, ulSize);
                            MessageBoxA(phDlg, szText, "Add Memory", MB_OK);
                        }
                    }
                    break;

                case ID_VIEW:
                    {
                        int     liItemList[1];
                        HWND    lhList = GetDlgItem(phDlg, IDC_TRACE_LIST);
                        int     liItems = ListBox_GetSelCount(lhList);

                        if (liItems && (liItems == 1))
                        {
                            PMEMORY_BASIC_INFORMATION   pInfo;
                            ListBox_GetSelItems(lhList, 1, liItemList);

                            pInfo = (PMEMORY_BASIC_INFORMATION) ListBox_GetItemData(lhList, liItemList[0]);

                            if (pInfo->State == MEM_COMMIT)
                            {
                                PDBGAH  pdbgahCopy;
                                ULONG   cb;
                                DWORD   dwRead;

                                /* Construct a 4K pseudo Local block to pass to memory viewer */
                                cb = ActualSizeFromRequestSize(4000, FALSE);
                                pdbgahCopy = (PDBGAH) LocalAlloc(LPTR, cb);

                                if (pdbgahCopy != NULL)
                                {
                                    memset(pdbgahCopy, 0, cb);
                                    lstrcpyA(pdbgahCopy->szName, "Virtual Page");
                                    pdbgahCopy->cbRequest = 4000;

                                    ReadProcessMemory(hProcess,
                                        pInfo->BaseAddress,
                                        RequestFromActual(pdbgahCopy),
                                        4000,
                                        &dwRead);

                                    ViewBlock(phDlg, pdbgahCopy);
                                    LocalFree(pdbgahCopy);
                                }
                                else
                                    AssertSz(0, "Insufficient memory to debug.");
                            }
                            else
                                MessageBoxA(phDlg, "Only commited memory may be viewed", "View Memory", MB_OK);
                        }
                        else
                            MessageBoxA(phDlg, "Only one memory block may be viewed at a time", "View Memory", MB_OK);

                    }
                    break;

                case ID_DUMP:
                    DumpMemoryList(GetDlgItem(phDlg, IDC_TRACE_LIST));
                    MessageBoxA(phDlg, "Virtual Memory list dumped to 'C:\\VIRTUAL.DMP'", "Dump Memory", MB_OK);
                    break;

                case IDOK:
                case IDCANCEL:
                {
                    PMEMORY_BASIC_INFORMATION   lpMem;
                    UINT                        i, j;
                    HWND                        lhList = GetDlgItem(phDlg, IDC_TRACE_LIST);

                    /* Free up the list memory */
                    i = ListBox_GetCount(lhList);
                    for (j = 0; j < i; j++)
                    {
                        lpMem = (PMEMORY_BASIC_INFORMATION) ListBox_GetItemData(lhList, j);
                        LocalFree(lpMem);
                    }

                    EndDialog(phDlg, 0);
                    break;
                }
            }
            break;

        default: return FALSE; break;
    }
    return TRUE;
}


void WINAPI DoMemoryBrowse(HINSTANCE phInst, HWND phWnd, DWORD dwProcess)
{
    static BOOL     fInDialog = FALSE;

    if (!fInDialog)
    {
        fInDialog = TRUE;
        SetRGBValues();
        DialogBoxParamA(phInst, MAKEINTRESOURCEA(BROWSE_DLG), phWnd, (DLGPROC) MemBrowseProc, dwProcess);
        fInDialog = FALSE;
    }
}


DWORD   WINAPI  DoBrowseThread(LPVOID lpParam)
{
    Sleep(2000);
    DoMemoryBrowse(NULL, NULL, GetCurrentProcessId());
    return 0;
}


void        GetMemStats(LPSTR   lpcLine, HANDLE hProc)
{
    ULONG                       ulTotalMem = 0, ulCodeMem = 0, ulDataMem = 0;
    ULONG                       ulReserveMem = 0;
    MEMORY_BASIC_INFORMATION    MemoryBasicInfo;
    PVOID                       lpAddress = 0;
    char                        szTemp[250];

    // Walk the virtual address space, adding
    // entries to the list box.

    do
    {

        int x = VirtualQueryEx(hProc, lpAddress, &MemoryBasicInfo,
             sizeof(MemoryBasicInfo));

        szTemp[0] = '\0';
        if (x != sizeof(MemoryBasicInfo))
        {
             // Attempt to walk beyond the range
             // that Windows NT allows.
             break;
        }

        // Construct the line to be displayed, and
        // add it to the list box.

        if (MemoryBasicInfo.State != MEM_FREE)
        {
            if (MemoryBasicInfo.State == MEM_RESERVE)
                ulReserveMem += (ULONG)MemoryBasicInfo.RegionSize;
            else
            {
                if (IsAppBasedOk(&MemoryBasicInfo, szTemp, hProc))
                    ulCodeMem += (ULONG)MemoryBasicInfo.RegionSize;
                else
                    ulDataMem += (ULONG)MemoryBasicInfo.RegionSize;

                ulTotalMem += (ULONG)MemoryBasicInfo.RegionSize;
            }
        }

        // Get the address of the next region to test.
        lpAddress = ((BYTE *) MemoryBasicInfo.BaseAddress) +
             MemoryBasicInfo.RegionSize;

    } while (lpAddress != 0);

    wsprintfA(lpcLine, "%lu, %lu, %lu, %lu", ulTotalMem, ulCodeMem, ulDataMem, ulReserveMem);
}

#pragma warning(disable:4702)   //  Unreachable code below

DWORD   WINAPI  DoMemStats(LPVOID lpParam)
{
    char        szLogFile[256];
    OFSTRUCT        ofFile;
    char            szMemLine[300];
    HFILE           lhFile;
    HANDLE          lhHFile;
    HANDLE      hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                                    TRUE, GetCurrentProcessId());


    wsprintfA(szLogFile, "c:\\%08lX.MEM", GetCurrentProcessId());

    lhFile = OpenFile(szLogFile, &ofFile, OF_CREATE);
    AssertSz(lhFile != NULL, "Failed to open log file.");
    _lwrite(lhFile, "Total,Code,Data,Reserve\r\n", 25);
    _lclose(lhFile);

    for (;;)
    {
        GetMemStats(szMemLine, hProc);

        lhHFile = CreateFileA(szLogFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        AssertSz(lhHFile != NULL, "Failed to open log file.");
        SetFilePointer(lhHFile, 0, NULL, FILE_END);
        _lwrite((HFILE) (DWORD_PTR)lhHFile, szMemLine, lstrlenA(szMemLine));
        _lwrite((HFILE) (DWORD_PTR)lhHFile, "\r\n", 2);

        CloseHandle(lhHFile);
        Sleep(10000);       // 10 second pause
    }
    return 0;
}

#pragma warning(default:4702)   // re-enable

// ----------------------------------------------------------------------------

void __cdecl hprintf(HANDLE hfile, char * pchFmt, ...)
{
    char    ach[2048];
    UINT    cb;
    DWORD   dw;
    va_list vl;

    va_start(vl, pchFmt);
    cb = wvsprintfA(ach, pchFmt, vl);
    va_end(vl);

    WriteFile(hfile, ach, cb, &dw, NULL);
}

void  WINAPI
DbgExDumpProcessHeaps()
{
    HANDLE hfile;
    HANDLE ah[256];
    DWORD i, cHeaps, dwBusyTotal = 0, dwFreeTotal = 0;

#ifdef UNIX
    CHAR szHeapDumpFile[] = "heapdump.txt";
#else
    CHAR szHeapDumpFile[] = "\\heapdump.txt";
#endif

    cHeaps = GetProcessHeaps(ARRAY_SIZE(ah), ah);

    hfile = CreateFileA(szHeapDumpFile, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfile == INVALID_HANDLE_VALUE)
        return;

    for (i = 0; i < cHeaps; ++i)
    {
        PROCESS_HEAP_ENTRY he;
        DWORD dwFree = 0, dwBusy = 0;

        memset(&he, 0, sizeof(he));

        hprintf(hfile, "+++ Heap %08lX %s\r\n\r\n", ah[i],
            ah[i] == GetProcessHeap() ? "(Process Heap)" : "");

        while (HeapWalk(ah[i], &he))
        {
            if (he.wFlags & PROCESS_HEAP_REGION)
            {
                hprintf(hfile, "R %08lX [%ld] (cb=%ld+%ld, c/r=%ld/%ld, "
                    "f/l=%08lX/%08lX)\r\n", he.lpData, he.iRegionIndex,
                    he.cbData, he.cbOverhead, he.Region.dwCommittedSize,
                    he.Region.dwUnCommittedSize, he.Region.lpFirstBlock,
                    he.Region.lpLastBlock);
            }
            else if (he.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            {
                hprintf(hfile, "U %08lX [%ld] (cb=%ld+%ld)\r\n",
                    he.lpData, he.iRegionIndex, he.cbData, he.cbOverhead);
            }
            else
            {
                hprintf(hfile, "%c %08lX %6ld (+%ld) [%ld]",
                    (he.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) ? 'M' :
                    (he.wFlags & PROCESS_HEAP_ENTRY_DDESHARE) ? 'S' :
                    (he.wFlags & PROCESS_HEAP_ENTRY_BUSY) ? ' ' : 'F',
                    he.lpData, he.cbData, he.cbOverhead, he.iRegionIndex);

                if (he.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE)
                    hprintf(hfile, " (hMem=%08lX)", he.Block.hMem);

                if (he.wFlags & PROCESS_HEAP_ENTRY_BUSY)
                    dwBusy += he.cbData;
                else
                    dwFree += he.cbData;

                hprintf(hfile, "\r\n");
            }
        }

        hprintf(hfile, "\r\nTotal of %ld bytes allocated (%ld free)\r\n\r\n",
            dwBusy, dwFree);

        dwBusyTotal += dwBusy;
        dwFreeTotal += dwFree;
    }

    hprintf(hfile, "Grand total of %ld bytes allocated (%ld free)\r\n",
        dwBusyTotal, dwFreeTotal);

    CloseHandle(hfile);
}
#endif // !WIN16
