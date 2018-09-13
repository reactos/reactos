#include "priv.h"

#define  _NO_DBGMEMORY_REDEFINITION_
#include "dbgmem.h"


#ifdef DEBUG
extern "C" LONG g_cmem = 0;
extern DWORD g_TlsMem;

struct MEMINFO {
    HMODULE hmod;
    void*   pv;
    UINT    cb;
    UINT    nType;
    // DWORD   adwCaller[4];   // for future
    LPCSTR  pszFile;            // file where memory block was allocced
    INT_PTR     iLine;              // line where memory block was allocced
};

void add_to_memlist( HMODULE hmod, void *pv, unsigned int nSize, UINT nType, LPCSTR pszFile, const INT_PTR iLine )
{
    HDSA hdsa = (HDSA)TlsGetValue(g_TlsMem);

    if ((hdsa) && (pv)) {
        MEMINFO memi = { hmod, pv, nSize, nType, pszFile, iLine };

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

    TraceMsg(TF_FTPALLOCS, "operator new() pv=%#08lx, size=%d, file=%s.", pv, nSize, pszFile);
    add_to_memlist( 0, pv, nSize, DBGMEM_OBJECT, pszFile, iLine );

    return pv;
}

void *  __cdecl operator new( size_t nSize )
{
    LPVOID pv;
    CREATE_CALLERS_ADDRESS;

    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

    TraceMsg(TF_FTPALLOCS, "operator new() pv=%#08lx, nSize=%u, caller=%#08lx.", pv, nSize, GET_CALLERS_ADDRESS);
    add_to_memlist( 0, pv, nSize, DBGMEM_UNKNOBJ, "UNKNOWN", (INT_PTR)GET_CALLERS_ADDRESS);

    return pv;
}

int _DSA_GetPtrIndex(HDSA hdsa, void* pv)
{
    for (int i=0; i<DSA_GetItemCount(hdsa); i++) {
        MEMINFO* pmemi = (MEMINFO*)DSA_GetItemPtr(hdsa, i);
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
        g_uMemMove = RegisterWindowMessage(TEXT("MSIEFTP_ThreadMemTransfer"));

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
            MEMINFO *pMemBlock = (MEMINFO*) DSA_GetItemPtr(hdsa, index);
            MEMINFO *pNewBlock = (MEMINFO*) LocalAlloc( LPTR, sizeof(MEMINFO ));
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
    MEMINFO * pMem = (MEMINFO *) pData;
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

        TraceMsg(TF_FTPALLOCS, "operator delete(%#08lx) size=%u.", pv, LocalSize((HLOCAL)pv));

        memset(pv, 0xfe, (UINT) LocalSize((HLOCAL)pv));
        LocalFree((HLOCAL)pv);
    }
}

extern "C" int __cdecl _purecall(void) {return 0;}

void _DumpMemLeak(DWORD wFlags)
{
    HDSA hdsa;
    if (wFlags & DML_END) {
        BOOL fLeaked = FALSE;
        hdsa = (HDSA)TlsGetValue(g_TlsMem);
        if (hdsa) {
            if (DSA_GetItemCount(hdsa))
            {
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
                        MEMINFO* pmemi = (MEMINFO*)DSA_GetItemPtr(hdsa, i);
                        DWORD* pdw = (DWORD*)pmemi->pv;
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
                AssertMsg(0, TEXT("*** ALL MEMORY LEAK MUST BE FIXED BEFORE WE RELEASE ***"));
                fLeaked = TRUE;
            }

            DSA_Destroy(hdsa);
            TlsSetValue(g_TlsMem, NULL);
        }

        if (!fLeaked) {
            TraceMsg(TF_GENERAL, "Thread Terminated: -- No Memory Leak Detected for this thread --");
        }
    } else {
        hdsa = DSA_Create(SIZEOF(MEMINFO),8);
        TlsSetValue(g_TlsMem, (LPVOID)hdsa);
    }
}

void _FTPDebugMemLeak(UINT wFlags, LPCTSTR pszFile, UINT iLine)
{
    LPCTSTR pszSuffix = (wFlags & DML_END) ? TEXT("END") : TEXT("BEGIN");

    switch(wFlags & DML_TYPE_MASK) {
    case DML_TYPE_MAIN:
        _DumpMemLeak(wFlags);
        if (g_cmem) {
            AssertMsg(0, TEXT("MAIN: %s, line %d: %d blocks of C++ objects left in memory. "),
                     PathFindFileName(pszFile), iLine, g_cmem);
        }
        break;

    case DML_TYPE_THREAD:
            _DumpMemLeak(wFlags);
        break;

    case DML_TYPE_NAVIGATE:
        TraceMsg(TF_ALWAYS, "NAVIGATEW_%s: %s, line %d: %d blocks of C++ objects in memory",
                     pszSuffix, PathFindFileName(pszFile), iLine, g_cmem);
        break;
    }
}


// LocalXXXXX functions
HLOCAL _TrcLocalAlloc(
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


LPTSTR  _TrcStrDup(
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

HLOCAL _TrcLocalFree(
    HLOCAL hMem                             // memory to be freed
    )
{
    if ( hMem )
    {
        remove_from_memlist( hMem );
        memset( hMem, 0xfe, (UINT) LocalSize( hMem ));
    }
    return LocalFree( hMem );
}

void  register_intelli_dump(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns)
{
    // BUGBUG:: this is part way to integrating the leaks...
}

#else
#define CPP_FUNCTIONS
#include <crtfree.h>

void _FTPDebugMemLeak(UINT wFlags, LPCTSTR pszFile, UINT iLine)
{
    // Do nothing in RETAIL.
}

#endif


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
        pTable->pfnDebugMemLeak = _FTPDebugMemLeak;
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

// Helper function
int _LoadStringW(HINSTANCE hinst, UINT id, LPWSTR wsz, UINT cchMax)
{
    char szT[512];
    if (LoadStringA(hinst, id, szT, ARRAYSIZE(szT)))
    {
        TraceMsg(0, "LoadStringW just loaded (%s)", szT);
        return SHAnsiToUnicode(szT, wsz, cchMax) - 1;    // -1 for terminator
    }
    else
    {
        TraceMsg(DM_TRACE, "sdv TR LoadStringW(%x) failed", id);
        wsz[0] = L'\0';
    }
    return 0;
}


