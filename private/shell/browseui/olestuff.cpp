#include "priv.h"

#define  _NO_DBGMEMORY_REDEFINITION_
#include "dbgmem.h"


#define DM_MISC2        0   // misc stuff (verbose)


// The Intelli-leak functionality has been turned OFF because
// we have -tons- of cases where we alloc memory on one thread
// and free it on another.  In these cases, intelli-leak reports
// FAKE leaks.  Removing these from the memlist is non-trivial
// because often a COM object is created that allocs other memory
// so not only do the top level objects need to be removed from
// the memory list, but all child allocations.  This is most
// often hit with cached desktop IShellFolders (g_psfDesktop)
// which gets really bad because of the aggregation.
// 
// In the future, we could report leaks when the process ends.
//
// Tester can and will report bugs on these leaks, even fake leaks
// so either we need to spend time fixing the fake leaks or turn
// this off.  Please fix ALL fake leak reports before turning
// this back on.  -BryanSt
// #define FEATURE_INTELLI_LEAK
//#define FEATURE_INTELLI_LEAK_PER_PROCESS

#ifdef DEBUG // {
extern "C" LONG g_cmem = 0;
extern "C" DWORD g_TlsMem;

// BUGBUG:: Gross but simply a static table of people who registered callbacks...
#define MAX_INTELLI_CALLBACKS 10

// We will psuedo register our own callback as part of this as to remove a lot of special case code...
STDAPI_(void) DumpLeakedMemory(PLEAKMEMINFO pmeminfo);
STDAPI_(LPWSTR) GetLeakSymbolicName(PLEAKMEMINFO pmeminfo, LPWSTR pwszBuf, int cchBuf);

static const INTELLILEAKDUMPCBFUNCTIONS c_ildcbf = {
    DumpLeakedMemory,
    GetLeakSymbolicName
};

int g_cIntelliCallbacks = 1;
struct INTELLILEAKCALLBACKENTRY
{
    HMODULE hmod;
    const INTELLILEAKDUMPCBFUNCTIONS *pfns;
}   g_pfnLeakCallbacks[MAX_INTELLI_CALLBACKS] = {{0, &c_ildcbf}};


STDAPI_(void) _add_to_memlist( HMODULE hmod, void *pv, unsigned int nSize, UINT nType, LPCSTR pszFile, const INT_PTR iLine )
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

void *  __cdecl operator new( size_t nSize, LPCSTR pszFile, const int iLine )
{
    LPVOID pv;
    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

#ifdef DEBUG
    if (pv)
    {
        HMODULE hmod = 0;

        if (g_bUseNewLeakDetection)
            hmod = HINST_THISDLL;
        add_to_memlist( hmod, pv, nSize, DBGMEM_OBJECT, pszFile, iLine );
    }
#endif

    return pv;
}

void *  __cdecl operator new( size_t nSize )
{
    LPVOID pv;
    //
    // HACK!  The compiler is putting hmod as the first local (EBP - 4). So
    // point pstack to &hmod.  We need to get rid of this scumbag code!
    // (edwardp)
    //
#ifdef DEBUG
    HMODULE hmod = 0;
    LPDWORD pstack = (LPDWORD) &hmod;
#endif

    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

#ifdef DEBUG
    if (pv)
    {
        if (g_bUseNewLeakDetection)
            hmod = HINST_THISDLL;
        add_to_memlist( hmod, pv, nSize, DBGMEM_UNKNOBJ, "UNKNOWN", (INT_PTR) &pstack[1] );
    }
#endif

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

STDAPI_(void) _remove_from_memlist(void *pv)
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
STDAPI_(UINT) _mem_thread_message()
{
    if ( !g_uMemMove )
        g_uMemMove = RegisterWindowMessage(TEXT("Shdocvw_ThreadMemTransfer"));

    return g_uMemMove;
}

STDAPI_(void) _transfer_to_thread_memlist(DWORD dwThreadId, void *pv)
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

STDAPI_(void) _remove_from_thread_memlist( DWORD dwThreadId, LPVOID pv )
{
    UINT uMsg = mem_thread_message();

    PostThreadMessage( dwThreadId, uMsg, 1, (LPARAM) pv );
}

STDAPI_(void) _received_for_thread_memlist( DWORD dwFlags, void * pData )
{
    LEAKMEMINFO * pMem = (LEAKMEMINFO *) pData;
    if ( pMem ){
        if ( dwFlags )
        {
            // we are being told to remove it from our thread list because it
            // is actually being freed on the other thread....
            remove_from_memlist( pMem->pv );
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
        memset(pv, 0xfe, (UINT)LocalSize((HLOCAL)pv));
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
#ifdef FEATURE_INTELLI_LEAK
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
            dOff = SearchDWP((DWORD_PTR *)pRef->pv, TRUNCPOW2(pRef->cb, SIZEOF(DWORD)), (DWORD_PTR)pDef->pv);
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
#endif // FEATURE_INTELLI_LEAK
}

// }

void _DoDumpMemLeak(HDSA hdsa, DWORD wFlags)
{
    BOOL fLeaked = FALSE;

#ifdef FEATURE_INTELLI_LEAK
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
                DWORD_PTR* pdw = (DWORD_PTR*)pmemi->pv;

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
        TraceMsg(DM_MISC2, "Thread Terminated: -- No Memory Leak Detected for this thread --");
    }
#endif // FEATURE_INTELLI_LEAK
}

extern void DBDumpQIStats();    // BUGBUG qistub.h

void _DumpMemLeak(DWORD wFlags)
{
    HDSA hdsa;
    if (wFlags & DML_END) {
        DBDumpQIStats();
        hdsa = (HDSA)TlsGetValue(g_TlsMem);
        if (hdsa) {
            _DoDumpMemLeak(hdsa, wFlags);
            _DoDumpMemLeakIntelli(hdsa, wFlags);

            DSA_Destroy(hdsa);
            TlsSetValue(g_TlsMem, NULL);
        }
    } else {
        hdsa = DSA_Create(SIZEOF(LEAKMEMINFO),8);
        TlsSetValue(g_TlsMem, (LPVOID)hdsa);
    }
}

STDAPI_(void) _DebugMemLeak(UINT wFlags, LPCTSTR pszFile, UINT iLine)
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
        if (g_tidParking != GetCurrentThreadId()) {
            _DumpMemLeak(wFlags);
        }
        break;

    case DML_TYPE_NAVIGATE:
        TraceMsg(TF_SHDLIFE, "NAVIGATEW_%s: %s, line %d: %d blocks of C++ objects in memory",
                     pszSuffix, PathFindFileName(pszFile), iLine, g_cmem);
        break;
    }
}


// LocalXXXXX functions
STDAPI_(HLOCAL) _TrcLocalAlloc(
    UINT uFlags,                            // flags used in LocalAlloc
    UINT uBytes,                            // number of bytes to be allocated
    LPCSTR pszFile,                         // file which allocced memory
    const int iLine                         // line which allocced memory
    )
{
    HLOCAL lpv = (HLOCAL) LocalAlloc( uFlags, uBytes );
    if ( lpv )
        add_to_memlist( 0, lpv, uBytes, DBGMEM_MEMORY, pszFile, iLine );

    return lpv;
}


STDAPI_(LPTSTR)  _TrcStrDup(
    LPTSTR lpSrch,                          // pointer to string to StrDup
    LPCSTR pszFile,                         // file which allocced memory
    const int iLine                         // line which allocced memory
    )
{
    UINT uBytes = 0;
    if ( lpSrch )
        uBytes = lstrlen( lpSrch ) + 1;

    LPTSTR lpstr = StrDup( lpSrch );
    if ( lpstr )
        add_to_memlist( 0, lpstr, uBytes, DBGMEM_STRDUP, pszFile, iLine );

    return lpstr;
}

STDAPI_(HLOCAL) _TrcLocalFree(
    HLOCAL hMem                             // memory to be freed
    )
{
    if ( hMem )
    {
        remove_from_memlist( hMem );
        memset( hMem, 0xfe, (UINT)LocalSize( hMem ));
    }
    return LocalFree( hMem );
}

STDAPI_(void) _register_intelli_dump(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns)
{
    if (g_cIntelliCallbacks < (MAX_INTELLI_CALLBACKS - 1))
    {
        g_pfnLeakCallbacks[g_cIntelliCallbacks].hmod = hmod;
        g_pfnLeakCallbacks[g_cIntelliCallbacks].pfns = pfns;
        g_cIntelliCallbacks++;
    }
}

#else // }{
#define CPP_FUNCTIONS
#include <crtfree.h>
#endif // }


BOOL GetLeakDetectionFunctionTable(LEAKDETECTFUNCS *pTable)
{


#ifdef DEBUG

    if(pTable)
    {
        pTable->pfnDumpMemLeak = _DumpMemLeak;
        pTable->pfnTrcLocalAlloc = _TrcLocalAlloc;

        pTable->pfnTrcLocalFree = _TrcLocalFree;

        pTable->pfnTrcStrDup = _TrcStrDup;

        pTable->pfnDumpMemLeak = _DumpMemLeak;
        pTable->pfnDebugMemLeak = MemLeakInit; //_DebugMemLeak;
        pTable->pfnreceived_for_thread_memlist = received_for_thread_memlist;
        pTable->pfnremove_from_thread_memlist = remove_from_thread_memlist;
        pTable->pfnmem_thread_message = mem_thread_message;
        pTable->pfnremove_from_memlist = remove_from_memlist;
        pTable->pfnadd_to_memlist = add_to_memlist;
        pTable->pfnregister_hmod_intelli_dump = register_intelli_dump;


        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
    return FALSE;
#endif


}

// We export all the memory leak detection functions in the form of a table of
// pointers so that all other shell components can share in the same leak
// detection code

// -----------------------------------------------------------------------------
// These two functions are our DLLS callback functions to dump the acutal leaks...
#ifdef DEBUG
//***   GetLeakSymbolicName
// DESCRIPTION
STDAPI_(LPWSTR) GetLeakSymbolicName(PLEAKMEMINFO pmeminfo, LPWSTR pwszBuf, int cchBuf)
{
    LPTSTR pszDef = GetLeakSymbolic(pmeminfo);     // human-readable name

        // they only want the class name...
#ifdef UNICODE
    return pszDef;
#else
    MultiByteToWideChar(CP_ACP, 0, pszDef, -1, pwszBuf, cchBuf);
    return pwszBuf;
#endif
}

//***   Dump LeakedMemory
// DESCRIPTION
STDAPI_(void) DumpLeakedMemory(LEAKMEMINFO* pmeminfo)
{
    LPTSTR pszDef = GetLeakSymbolic(pmeminfo);     // human-readable name

    TraceMsg(TF_ALWAYS, "leak=0x%x,%x(%s)", pmeminfo->pv, pmeminfo->cb, pszDef);

    if (pmeminfo->nType == DBGMEM_UNKNOBJ)
        TraceMsg(TF_ALWAYS, "\tcreated from 0x%x", pmeminfo->iLine);
    else
        TraceMsg(TF_ALWAYS, "\tcreated from %hs:%d",
             PathFindFileNameA(pmeminfo->pszFile), pmeminfo->iLine);

#if ( _X86_) // { QIStub only works for X86
    if (pmeminfo->nType == DBGMEM_UNKNOBJ && DBIsQIStub(pmeminfo->pv)) {
        // it's a QIStub, dump it
        // of particular interest is the 'sequence #' (esp. for roots)
        DBDumpQIStub(pmeminfo->pv);
    }
#endif // }
}

#endif



// -----------------------------------------------------------------------------

IMalloc* g_pmalloc = NULL;

HRESULT _InitMalloc(void)
{
    HRESULT hres = S_OK;
    ENTERCRITICAL;
    if (g_pmalloc == NULL)
        hres = CoGetMalloc(MEMCTX_TASK, &g_pmalloc);
    LEAVECRITICAL;
    return hres;
}

LPVOID OleAlloc(UINT cb)
{
    if (g_pmalloc == NULL)
        _InitMalloc();

    ASSERT(g_pmalloc);
    return g_pmalloc->Alloc(cb);
}

void   OleFree(LPVOID pb)
{
    if (g_pmalloc == NULL)
        _InitMalloc();

    ASSERT(g_pmalloc);
    g_pmalloc->Free(pb);
}
