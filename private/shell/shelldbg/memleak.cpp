#include "priv.h"

#define  _NO_DBGMEMORY_REDEFINITION_
#include "dbgmem.h"


#define DM_MISC2        0   // misc stuff (verbose)


#ifdef DEBUG
EXTERN_C LONG g_cmem = 0;
EXTERN_C DWORD g_TlsMem;
EXTERN_C DWORD g_TlsCount;

// BUGBUG:: Gross but simply a static table of people who registered callbacks...
#define MAX_INTELLI_CALLBACKS 10

int g_cIntelliCallbacks = 0;
struct INTELLILEAKCALLBACKENTRY
{
    HMODULE hmod;
    const INTELLILEAKDUMPCBFUNCTIONS *pfns;
}   g_pfnLeakCallbacks[MAX_INTELLI_CALLBACKS] = {0};


void add_to_memlist( HMODULE hmod, void *pv, unsigned int nSize, UINT nType, LPCSTR pszFile, const int iLine )
{
    HDSA hdsa = (HDSA)TlsGetValue(g_TlsMem);

    if ((hdsa) && (pv)) {
        LEAKMEMINFO memi = { hmod, pv, nSize, nType, pszFile, iLine };
        LEAKMEMINFO *pmemi = &memi;

        if (pmemi->nType == DBGMEM_UNKNOBJ) {
#if ( _X86_) // { QIStub (and GetStackBack) only work for X86
            if (IsFlagSet(g_dwDumpFlags, DF_DEBUGQI)) {
            extern LPVOID QIStub_CreateInstance(void* that, IUnknown* punk, REFIID riid);
            HRESULT QISearch(void* that, LPCQITAB pqitab, REFIID riid, LPVOID* ppv);
            // if QIStub is on, our frame should look like:
            //  ... -> leaker -> [SUPER::QI]* -> xxx::QI -> QISearch ->
            //      -> QIStub_CreateInst -> ::new
            // we do our best to follow it back to get to leaker.  getting
            // to xxx::QI is pretty easy, getting thru the SUPER::QI's a
            // bit more iffy (but usually works).
            //
            // note that we need to allow for some fudge since a child
            // returns into the middle of a func (i.e. func+FUDGE), not
            // to the beginning (i.e. func+0).
#define FUDGE(n)    (n)
            //
            // TODO: eventually we may hook up to imagehlp's GetStackBacktrace
            // to give a deeper backtrace, but it's a bit heavyweight.

            int fp, n, i, pfunc;
            struct DBstkback sbtab[6];  // enough for a couple SUPER's

            fp = pmemi->iLine;
            n = DBGetStackBack(&fp, sbtab, ARRAYSIZE(sbtab));
#define INFUNC(p, pfunc, cb)    ((pfunc) <= (p) && (p) <= (pfunc) + (cb))
            // skip over QIStub_CI and QISearch explicitly
            pfunc = (int) QIStub_CreateInstance;
            i = 0;
            if (INFUNC(sbtab[0].ret, (int)QIStub_CreateInstance, FUDGE(16))) {
                i++;
                if (INFUNC(sbtab[1].ret, (int)QISearch, FUDGE(512)))
                    i++;
            }

            // skip over any known/suspected QI's
            for ( ; i < n; i++) {
                if (! DBIsQIFunc(sbtab[i].ret, FUDGE(64)))
                    break;
            }

            TraceMsg(DM_MISC2, "v.atm: n=%d i=%d ret[]=%x,%x,%x,%x,%x,%x", n, i,
                sbtab[0].ret, sbtab[1].ret, sbtab[2].ret, sbtab[3].ret,
                sbtab[4].ret, sbtab[5].ret);

            // this should be the leaker
            pmemi->iLine = sbtab[i].ret;
            }
            else
#endif // }
            {
                // beats me if/why this works for non-x86,
                // but it's what the old code did
                pmemi->iLine = *((int *)iLine + 1);     // aka BP_GETRET(iLine)
            }
        }

        if (DSA_AppendItem(hdsa, &memi) >=0) {
            InterlockedIncrement(&g_cmem);
        }
    }
}

void *  __cdecl operator new( unsigned int nSize, LPCSTR pszFile, const int 
iLine )
{
    LPVOID pv;
    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

    //add_to_memlist( 0, pv, nSize, DBGMEM_OBJECT, pszFile, iLine );

    return pv;
}

void *  __cdecl operator new( unsigned int nSize )
{
    LPVOID pv;
    LPDWORD pstack = (LPDWORD) &pv;

    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

    //add_to_memlist( 0, pv, nSize, DBGMEM_UNKNOBJ, "UNKNOWN", (int) &pstack[1] );

    return pv;
}

int _DSA_GetPtrIndex(HDSA hdsa, void* pv)
{
    for (int i=0; i<DSA_GetItemCount(hdsa); i++) {
        LEAKMEMINFO* pmemi = (LEAKMEMINFO*)DSA_GetItemPtr(hdsa, i);
        if (pmemi->pv == pv) {
            return i;
        }
    }

    return -1;
}

void remove_from_memlist(void *pv)
{
    HDSA hdsa = (HDSA)TlsGetValue(g_TlsMem);
    if (hdsa) {
        int index = _DSA_GetPtrIndex(hdsa, pv);
        if (index >=0) {
            DSA_DeleteItem(hdsa, index);
            InterlockedDecrement(&g_cmem);
        }
    }
}

static UINT g_uMemMove = 0;
UINT mem_thread_message()
{
    if ( !g_uMemMove )
        g_uMemMove = RegisterWindowMessage(TEXT("Shdocvw_ThreadMemTransfer"));

    return g_uMemMove;
}

void transfer_to_thread_memlist(DWORD dwThreadId, void *pv)
{
    UINT uMsg = mem_thread_message();

    HDSA hdsa = (HDSA)TlsGetValue(g_TlsMem);
    if ( hdsa )
    {
        int index = _DSA_GetPtrIndex( hdsa, pv );
        if ( index >= 0)
        {
            LEAKMEMINFO *pMemBlock = (LEAKMEMINFO*) DSA_GetItemPtr(hdsa, index);
            LEAKMEMINFO *pNewBlock = (LEAKMEMINFO*) LocalAlloc( LPTR, sizeof(LEAKMEMINFO ));
            if ( pNewBlock )
            {
                *pNewBlock = *pMemBlock;

                // post a message to the thread giving it the memblock
                PostThreadMessage( dwThreadId, uMsg, 0, (LPARAM) pNewBlock );
            }

            // remove from the current thread's list...
            DSA_DeleteItem( hdsa, index );
            InterlockedDecrement(&g_cmem);
        }
    }
}

void remove_from_thread_memlist( DWORD dwThreadId, LPVOID pv )
{
    UINT uMsg = mem_thread_message();

    PostThreadMessage( dwThreadId, uMsg, 1, (LPARAM) pv );
}

void received_for_thread_memlist( DWORD dwFlags, void * pData )
{
    LEAKMEMINFO * pMem = (LEAKMEMINFO *) pData;
    if ( pMem ){
        if ( dwFlags )
        {
            // we are being told to remove it from our thread list because it
            // is actually being freed on the other thread....
            remove_from_memlist( pData );
            return;
        }

        HDSA hdsa = (HDSA)TlsGetValue(g_TlsMem);

        if (hdsa) {
            if (DSA_AppendItem(hdsa, pMem) >=0) {
                InterlockedIncrement(&g_cmem);
            }
        }
        LocalFree( pMem );
    }
}

void  __cdecl operator delete(void *pv)
{
    if (pv) {
        remove_from_memlist(pv);
        memset(pv, 0xfe, LocalSize((HLOCAL)pv));
        LocalFree((HLOCAL)pv);
    }
}

extern "C" int __cdecl _purecall(void) {return 0;}

//***   heuristics {

//***   GetLeakSymbolic -- get human-readable name for object
//
LPTSTR GetLeakSymbolic(LEAKMEMINFO *pLeak)
{
    extern LPTSTR DBGetQIStubSymbolic(void* that);
    extern LPTSTR DBGetClassSymbolic(int cbSize);
    LPTSTR pszSym;

#if ( _X86_) // { QIStub only works for X86
    if (pLeak->nType == DBGMEM_UNKNOBJ && DBIsQIStub(pLeak->pv))
        pszSym = DBGetQIStubSymbolic(pLeak->pv);
    else
#endif // }
    if (pszSym = DBGetClassSymbolic(pLeak->cb))
        NULL;
    else {
        // if you get this string, go into debdump.cpp and add a
        // DBGetClassSymbolic table entry for the class
        pszSym = TEXT("?debdump!DBGetClassSymbolic");
    }

    return pszSym;
}

//***   _ConvertHModToIntelliDumpIndex
// DESCRIPTION
//  main heuristics:
int _ConvertHModToIntelliDumpIndex(HMODULE hmod)
{
    for (int i = 0; i < g_cIntelliCallbacks; i++) {
        if ((hmod == g_pfnLeakCallbacks[i].hmod) &&
                !IsBadCodePtr((FARPROC)g_pfnLeakCallbacks[i].pfns->pfnDumpLeakedMemory)) {
            return i;
        }
    }

    return -1;

}


//***   _DoDumpMemLeakIntelli -- attempt to interpret leak info
// DESCRIPTION
//  main heuristics:
//      try to detect refs from one object to another by walking all DWORDs
//          this gives us a graph description
//          more importantly, it tells us which are the 'root' leaks
//      identify classes so we can label them w/ a symbolic name
//      identify QIStub's so we can dump them nicely
//          e.g. symbolic IID and a sequence # for that IID
//          the sequence # can be used to find the exact QI call
//  others:
//      see the code (sorry)...
// NOTES
//  algorithm is n^2, so sue me...
//  we assume (hopefully correctly...) that all memory is aligned to a
//  'strict' boundary so we'll never fault dereferencing DWORDs.  if that
//  turns out to be wrong we can wrap in a __try...__except.
void _DoDumpMemLeakIntelli(HDSA hdsa, DWORD wFlags)
{
    int cLeak;

    ASSERT(wFlags & DML_END);
    ASSERT(hdsa);

    cLeak = DSA_GetItemCount(hdsa);
    if (!cLeak)
        return;

    TraceMsg(TF_ALWAYS, "intelli-leak heuristics...");

    for (int iDef = 0; iDef < cLeak; iDef++) {
        LEAKMEMINFO* pDef = (LEAKMEMINFO*)DSA_GetItemPtr(hdsa, iDef);
        BOOL fHasRef;
        int iDumpModule;

        if (IsBadReadPtr(pDef->pv, pDef->cb))
            continue;

        // see if we should hand this one off to a different dumper?
        if ((iDumpModule = _ConvertHModToIntelliDumpIndex(pDef->hmod)) >= 0) {
            g_pfnLeakCallbacks[iDumpModule].pfns->pfnDumpLeakedMemory(pDef);
        }
        else {
            ASSERT(FALSE);
        }
        // find all (likely) refs to this leak.  we do this by walking
        // the full set of leaks and looking for DWORDs that point to
        // the base of this leak.  we only look at the base on the
        // assumption that ptrs will go thru QIStub's (and this bounds
        // the search which is nice).
        fHasRef = FALSE;
        for (int iRef = 0; iRef < cLeak; iRef++) {
            LEAKMEMINFO* pRef = (LEAKMEMINFO*)DSA_GetItemPtr(hdsa, iRef);
            int dOff;

            if (IsBadReadPtr(pRef->pv, pRef->cb))
                continue;

#define TRUNCPOW2(n, p)     ((n) & ~((p) - 1))
            dOff = SearchDW((DWORD *)pRef->pv, TRUNCPOW2(pRef->cb, SIZEOF(DWORD)), (DWORD)pDef->pv);
            if (dOff != -1) {
                // found a ref to the leak def!
                if ((iDumpModule = _ConvertHModToIntelliDumpIndex(pRef->hmod)) >= 0) {
                    WCHAR szBuf[80];
                    LPWSTR pwszRef = g_pfnLeakCallbacks[iDumpModule].pfns->pfnGetLeakSymbolicName(pDef,
                            szBuf, ARRAYSIZE(szBuf));

                    TraceMsg(TF_ALWAYS, "\tref=%x,%x+%x(%ls)",
                        pRef->pv, pRef->cb, dOff, pwszRef);
                }
                else {
                    ASSERT(FALSE);
                }


                // a fair number of objects actually point back to themselves
                // print out the ref just in case, but don't count it as a
                // real one (which will make us look like a non-root)
                if (pRef->pv != pDef->pv)
                    fHasRef = TRUE;
                else
                    TraceMsg(TF_ALWAYS, "\tref=self");

                // (we could iterate on pRef->pv+dOff+SIZEOF(DWORD),
                // but probably the 1st hit is all we need)
            }
        }
        if (!fHasRef) {
            // these are the guys we really care about
            TraceMsg(TF_ALWAYS, "\tref=root ***");
        }
    }

    ASSERT(0);      // go to caller to see what is wrong
}

// }

void _DoDumpMemLeak(HDSA hdsa, DWORD wFlags)
{
    BOOL fLeaked = FALSE;

    ASSERT(wFlags & DML_END);
    ASSERT(hdsa);

    if (DSA_GetItemCount(hdsa)) {
        // Let's always dump them.
        TraceMsg(TF_ALWAYS, "****************************************************");
        TraceMsg(TF_ALWAYS, "*    !!!!! WARNING : MEMORY LEAK DETECTED !!!!!    *");
        TraceMsg(TF_ALWAYS, "****************************************************");
        TraceMsg(TF_ALWAYS, "* For Object: address: Vtbl, ...Vtbl, _cRef        *");
        TraceMsg(TF_ALWAYS, "* For StrDup: address: 'text'                      *");
        TraceMsg(TF_ALWAYS, "* For Traced: address:                             *");
        TraceMsg(TF_ALWAYS, "* For Memory: address:                             *");
        TraceMsg(TF_ALWAYS, "****************************************************");
        {
            for (int i=0; i<DSA_GetItemCount(hdsa); i++) {
                LEAKMEMINFO* pmemi = (LEAKMEMINFO*)DSA_GetItemPtr(hdsa, i);
                DWORD* pdw = (DWORD*)pmemi->pv;

                // sometimes we think we have leaked something and its really been freed,
                // so check here first
                if (IsBadReadPtr((LPVOID)pdw, pmemi->cb))
                    continue;

                switch( pmemi->nType ) {
                case DBGMEM_STRDUP:
                    TraceMsg(TF_ALWAYS, "StrDup: %8x: \"%s\"\n\t\t size=%d (%hs, line %d)",
                             pdw, pdw, pmemi->cb, PathFindFileNameA(pmemi->pszFile), pmemi->iLine);
                    break;

                case DBGMEM_UNKNOBJ:
                case DBGMEM_OBJECT:
                    {
                        if ( pmemi->cb >= 32 )
                        {
                            TraceMsg(TF_ALWAYS, "Object: %8x: %8x %8x %8x %8x",
                                pdw, pdw[0], pdw[1], pdw[2], pdw[3] );
                        }
                        else
                        {
                            TraceMsg(TF_ALWAYS, "Object: %8x: Size<32 bytes. No Vtbl.", pdw );
                        }
                    }
                    break;

                case DBGMEM_TRACED:
                    TraceMsg(TF_ALWAYS, "Traced: %8x:", pdw );
                    break;

                case DBGMEM_MEMORY:
                default:
                    TraceMsg(TF_ALWAYS, "Memory: %8x:", pdw );
                    break;
                }
                if ( pmemi->nType == DBGMEM_UNKNOBJ )
                {
                    TraceMsg(TF_ALWAYS, "\t size=%d, created from %8x (return address)",
                                     pmemi->cb, pmemi->iLine);
                }
                else
                {
                    TraceMsg(TF_ALWAYS, "\t size=%d  (%hs, line %d)",
                                     pmemi->cb, PathFindFileNameA(pmemi->pszFile), pmemi->iLine);
                }


            }
        }
        TraceMsg(TF_ALWAYS, "*************************************************");
        AssertMsg(0, TEXT("*** ALL MEMORY LEAK MUST BE FIXED BEFORE WE RELEASE (continue for intelli-leak on x86) ***"));
        fLeaked = TRUE;
    }

    if (!fLeaked) {
        TraceMsg(TF_GENERAL, "Thread Terminated: -- No Memory Leak Detected for this thread --");
    }
}

void _DumpMemLeak(DWORD wFlags)
{
    HDSA hdsa;
    if (wFlags & DML_END) 
    {
        DWORD dwCount = (DWORD)TlsGetValue(g_TlsCount);
        if (EVAL(g_TlsCount > 0))
        {
            dwCount--;
            TlsSetValue(g_TlsCount, (LPVOID)dwCount);
        }
        else
        {   // just to make things work below
            dwCount = 0;
        }

        if (dwCount == 0)
        {
            hdsa = (HDSA)TlsGetValue(g_TlsMem);
            if (hdsa) {
                _DoDumpMemLeak(hdsa, wFlags);
                _DoDumpMemLeakIntelli(hdsa, wFlags);

                DSA_Destroy(hdsa);
                TlsSetValue(g_TlsMem, NULL);
            }
        }
    } 
    else 
    {
        hdsa = (HDSA)TlsGetValue(g_TlsMem);
        // don't init twice
        if (!hdsa)
        {
            hdsa = DSA_Create(SIZEOF(LEAKMEMINFO),8);
            TlsSetValue(g_TlsMem, (LPVOID)hdsa);
            TlsSetValue(g_TlsCount, (LPVOID)1);
        }
        else
        {
            DWORD dwCount = (DWORD)TlsGetValue(g_TlsCount);
            if (EVAL(g_TlsCount > 0))
            {
                dwCount++;
                TlsSetValue(g_TlsCount, (LPVOID)dwCount);
            }
        }
    }
}

void _DebugMemLeak(UINT wFlags, LPCTSTR pszFile, UINT iLine)
{
    LPCTSTR pszSuffix = (wFlags & DML_END) ? TEXT("END") : TEXT("BEGIN");

    switch(wFlags & DML_TYPE_MASK) {
    case DML_TYPE_MAIN:
        _DumpMemLeak(wFlags);
        if (g_cmem) {
            AssertMsg(0, TEXT("MAIN: %s, line %d: %d blocks of C++ objects left in memory. Read shdocvw\\memleak.txt for detail"),
                     PathFindFileName(pszFile), iLine, g_cmem);
        }
        break;

    case DML_TYPE_THREAD:
        //if (g_tidParking != GetCurrentThreadId()) {
            _DumpMemLeak(wFlags);
        //}
        break;

    case DML_TYPE_NAVIGATE:
        TraceMsg(TF_SHDLIFE, "NAVIGATEW_%s: %s, line %d: %d blocks of C++ objects in memory",
                     pszSuffix, PathFindFileName(pszFile), iLine, g_cmem);
        break;
    }
}

void  register_intelli_dump(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns)
{
    if (g_cIntelliCallbacks < (MAX_INTELLI_CALLBACKS - 1))
    {
        g_pfnLeakCallbacks[g_cIntelliCallbacks].hmod = hmod;
        g_pfnLeakCallbacks[g_cIntelliCallbacks].pfns = pfns;
        g_cIntelliCallbacks++;
    }
}

#else // DEBUG
#define CPP_FUNCTIONS
#include <crtfree.h>
#endif // DEBUG

void ShdbgMemLeakInit(UINT flag, LPCTSTR pszFile, UINT iLine)
{
#ifdef DEBUG
    _DebugMemLeak(flag, pszFile, iLine);
#endif
}

void ShdbgAddToMemList(HMODULE hmod, void *pv, UINT nSize, UINT nType, LPCSTR pszFile, const int iLine)
{
#ifdef DEBUG
    add_to_memlist(hmod, pv, nSize, nType, pszFile, iLine);
#endif
}

void ShdbgRemoveFromMemList(void *pv)
{
#ifdef DEBUG
    remove_from_memlist(pv);
#endif
}

void ShdbgRemoveFromThreadMemList(DWORD dwThreadId, LPVOID pv)
{
#ifdef DEBUG
    remove_from_thread_memlist(dwThreadId, pv);
#endif
}

void ShdbgTransferToThreadMemList(DWORD dwThread, LPVOID pv)
{
#ifdef DEBUG
    transfer_to_thread_memlist(dwThread, pv);
#endif
}

UINT ShdbgMemThreadMessage(void)
{
    UINT uiRet = 0;
#ifdef DEBUG
    uiRet = mem_thread_message();
#endif
    return uiRet;
}

void ShdbgReceivedForThreadMemList(DWORD dwFlags, void * pData)
{
#ifdef DEBUG
    received_for_thread_memlist(dwFlags, pData);
#endif
}

void ShdbgRegisterIntelliLeakDump(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns)
{
#ifdef DEBUG
    register_intelli_dump(hmod, pfns);
#endif
}

