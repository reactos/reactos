#include "shellprv.h"
#pragma  hdrstop

#define  _NO_DBGMEMORY_REDEFINITION_
#include "dbgmem.h"


EXTERN_C BOOL g_fInitTable;
EXTERN_C LEAKDETECTFUNCS LeakDetFunctionTable;
EXTERN_C void GetAndRegisterLeakDetection(void);

BOOL g_fInitTable = FALSE;
LEAKDETECTFUNCS LeakDetFunctionTable;

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


#ifdef DEBUG

TCHAR *GetLeakSymbolic(LEAKMEMINFO *pLeak)
{
    extern TCHAR *DBGetQIStubSymbolic(void* that);
    extern TCHAR *DBGetClassSymbolic(int cbSize);
    TCHAR *pszSym;

#if ( _X86_) // { QIStub only works for X86
    if (pLeak->nType == DBGMEM_UNKNOBJ && DBIsQIStub(pLeak->pv))
        pszSym = DBGetQIStubSymbolic(pLeak->pv);
    else
#endif // }
    if (pszSym = DBGetClassSymbolic(pLeak->cb))
        ;
    else {
        // if you get this string, go into debdump.cpp and add a 
        // DBGetClassSymbolic table entry for the class
        pszSym = TEXT("?debdump!DBGetClassSymbolic");
    }

    return pszSym;
}

LPWSTR GetLeakSymbolicName(PLEAKMEMINFO pmeminfo, LPWSTR pwszBuf, int cchBuf)
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

void DumpLeakedMemory(LEAKMEMINFO* pmeminfo)
{
#ifdef FEATURE_INTELLI_LEAK
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
#endif // _X86_
#endif // FEATURE_INTELLI_LEAK
}

static const INTELLILEAKDUMPCBFUNCTIONS c_ildcbf = {
    DumpLeakedMemory,
    GetLeakSymbolicName
};

void GetAndRegisterLeakDetection(void)
{
#ifdef FEATURE_INTELLI_LEAK
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
        {
            g_fInitTable = TRUE;
            // now tell shdocvw about us...
            LeakDetFunctionTable.pfnregister_hmod_intelli_dump(HINST_THISDLL, 
                     &c_ildcbf);
        }

    }
#endif // FEATURE_INTELLI_LEAK
}
#endif

void *  __cdecl operator new( size_t nSize, LPCSTR pszFile, const int iLine )
{
    LPVOID pv; 
    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

#ifdef DEBUG
    // leak detection
    GetAndRegisterLeakDetection();
    if(g_fInitTable)
        LeakDetFunctionTable.pfnadd_to_memlist(HINST_THISDLL, pv, nSize, DBGMEM_OBJECT, pszFile, iLine);
#endif 
    return pv;
}

void *  __cdecl operator new( size_t nSize )
{
    LPVOID pv; 
    LPDWORD pstack = (LPDWORD) &pv;

    // Zero init just to save some headaches
    pv = ((LPVOID)LocalAlloc(LPTR, nSize));

#ifdef DEBUG
    // leak detection           
    GetAndRegisterLeakDetection();
    if(g_fInitTable)
        LeakDetFunctionTable.pfnadd_to_memlist(HINST_THISDLL, pv, nSize, DBGMEM_UNKNOBJ, "UNKNOWN", (INT_PTR) &pstack[1]);
#endif 

    return pv;
}

void  __cdecl operator delete(void *pv)
{
    if (pv) {
#ifdef DEBUG
        if(g_fInitTable)
            LeakDetFunctionTable.pfnremove_from_memlist(pv);

        memset(pv, 0xfe, (size_t) LocalSize((HLOCAL)pv));
#endif 
        LocalFree((HLOCAL)pv);
    }
}
