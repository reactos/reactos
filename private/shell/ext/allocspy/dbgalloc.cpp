//+------------------------------------------------------------------------
//  
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//  
//  File:       dbgalloc.cpp
//  
//  Contents:   IMallocSpy implementation.
//  
//  History:
//     10/15/97: t-saml based on MSHTML's magic.cxx and dbgalloc.cxx
//     12/02/97: t-saml moved to allocspy.dll
//
//
//  Todo:
//     Dump leaks per thread
//
//-------------------------------------------------------------------------
#define CC_INTERNAL

#include "windows.h"
#include "shlwapi.h"
#include "ccstock.h"
#include "port32.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shsemip.h"        // for _ILNext
#include "shguidp.h"
#include "shellp.h"
#include "..\..\shell32\shitemid.h"
#include "imagehlp.h"
#include "debug.h"

#include "dbgalloc.h"

CMallocSpy *g_pms=NULL;

// DONT USE #define'd versions!
#undef LocalAlloc
#undef LocalFree

#ifdef DEBUG
EXTERN_C const CHAR FAR c_szCcshellIniFile[];
EXTERN_C const CHAR FAR c_szCcshellIniSecDebug[];
#else
#define c_szCcshellIniFile      "CCSHELL.INI"
#define c_szCcshellIniSecDebug  "ALLOCSPY"
#endif

#define IS_STACKTRACE_ON() (_dwLeakSetting & 0x02)
#define LEAK_DUMPTOSCREEN 0x04
#define LEAK_OVERWRITELOG 0x08


#define SPYSIG 0x66600666
#define INVALID_TLS_VALUE 0xFFFFFFFF


#define TLS(x)      (GetThreadState()->x)


#ifdef _ALPHA_
#define PAGE_SIZE       8192
#else
#define PAGE_SIZE		4096
#endif
#define PvToVMBase(pv)	((void *)((ULONG)pv & 0xFFFF0000))


// is per-instance the default?
#pragma data_seg(DATASEG_PERINSTANCE)

// Globals

static DWORD g_dwTls         = INVALID_TLS_VALUE;

// use our private, low overhead crit sections on NT only (win95 doesn't support InterlockedCompareExchange)
#ifdef USEPRIVATECRIT
static LONG g_lCrit=0, g_lCritCount=0;  // for private critical section
#else
static CRITICAL_SECTION g_csSpy;
#endif

pfnImgHlp_StackWalk         g_pfnStackWalk=NULL;
pfnImgHlp_SymGetModuleInfo  g_pfnSymGetModuleInfo=NULL;
pfnImgHlp_SymLoadModule     g_pfnSymLoadModule=NULL;
pfnImgHlp_SymGetSymFromAddr g_pfnSymGetSymFromAddr=NULL;
pfnImgHlp_SymUnDName        g_pfnSymUnDName=NULL;
pfnImgHlp_SymGetLineFromAddr g_pfnSymGetLineFromAddr=NULL;
PFUNCTION_TABLE_ACCESS_ROUTINE  g_pfnFunctionTableAccessRoutine;

HINSTANCE g_ImgHlp = NULL;

BOOL g_bRegistered = FALSE;

static THREADSTATE * s_pts=NULL;

SYSTEMTIME          g_st; // time we were registered

#pragma data_seg()

void GetSymbolName(LPSTR pszSymName, HANDLE hProcess, DWORD dwAddr);


void EnterSpyCrit()
{
#ifdef USEPRIVATECRIT
    long tid = GetCurrentThreadId();
    // if we don't already own it
    if (g_lCrit != tid)
    {
        // wait for no-one to own it
        while(tid != (LONG)InterlockedCompareExchange((LPVOID*)&g_lCrit, (LPVOID)tid, 0)) // if its zero, make it threadid
            Sleep(0);
    }
    InterlockedIncrement(&g_lCritCount);
#else
    EnterCriticalSection(&g_csSpy);
#endif
}

void LeaveSpyCrit()
{
#ifdef USEPRIVATECRIT
    long tid = GetCurrentThreadId();
    ASSERT(g_lCrit == tid);
    if (0 == InterlockedDecrement(&g_lCritCount))
        g_lCrit = 0;
#else
    LeaveCriticalSection(&g_csSpy);
#endif
}

HRESULT SpyDllThreadAttach()
{
    THREADSTATE *   pts;
    // Allocate directly from the heap, rather than through new, since new
    // requires that the THREADSTATE is established
    
    if (g_dwTls == INVALID_TLS_VALUE)
    {
        TraceMsg(TF_WARNING, "SpyDllThreadAttatch: IMallocSpy not initialized");
        return E_FAIL;
    }

    pts = (THREADSTATE *)LocalAlloc(LPTR, sizeof(THREADSTATE));
    if (!pts)
    {
        TraceMsg(TF_ERROR, "Debug Thread initialization failed");
        return E_OUTOFMEMORY;
    }
    pts->dwThreadId = GetCurrentThreadId();

    EnterSpyCrit();

    pts->ptsNext = s_pts;
    if (s_pts)
        s_pts->ptsPrev = pts;
    s_pts = pts;
    LeaveSpyCrit();

    TlsSetValue(g_dwTls, pts);
    TraceMsg(TF_ALWAYS, "-- SpyDllThreadAttach %d",GetCurrentThreadId());

    return S_OK;
}

void _DllThreadDetach(THREADSTATE *pts)
{
    THREADSTATE **  ppts;

    if (!pts)
        return;

    EnterSpyCrit();

    for (ppts = &s_pts; *ppts && *ppts != pts; ppts = &((*ppts)->ptsNext));
    if (*ppts)
    {
        *ppts = pts->ptsNext;
        if (pts->ptsNext)
        {
            pts->ptsNext->ptsPrev = pts->ptsPrev;
        }
    }
    LeaveSpyCrit();

   LocalFree(pts);
}

inline HRESULT EnsureThreadState()
{
    if (!TlsGetValue(g_dwTls))
        return SpyDllThreadAttach();
    return S_OK;
}

inline THREADSTATE *GetThreadState()
{
    THREADSTATE * pts = (THREADSTATE *)TlsGetValue(g_dwTls);
    return pts;

}

void SpyDllThreadDetach()
{
    if (g_dwTls == INVALID_TLS_VALUE)
    {
//        TraceMsg(TF_WARNING, "SpyDllThreadDetach: IMallocSpy not initialized");
        return;
    }

    TraceMsg(TF_ALWAYS, "-- SpyDllThreadDetach %d",GetCurrentThreadId());
    _DllThreadDetach(GetThreadState());
}


BOOL VMValidatePv(void *pv)
{
	void *	pvBase = PvToVMBase(pv);
	BYTE *	pb;

	pb = (BYTE *)pvBase + sizeof(ULONG);

	while (pb < (BYTE *)pv)
    {
		if (*pb++ != 0xAD)
        {
			TraceMsg(TF_ERROR, "Block leader has been overwritten for %x",pv);
			return(FALSE);
		}
	}

	return(TRUE);
}

void * VMAlloc(ULONG cb)
{
	ULONG	cbAlloc;
	void *	pvR;
	void *	pvC;

	if (cb > 0x100000)
		return(0);

	cbAlloc	= sizeof(ULONG) + cb + PAGE_SIZE - 1;
	cbAlloc -= cbAlloc % PAGE_SIZE;
	cbAlloc	+= PAGE_SIZE;

	pvR = VirtualAlloc(0, cbAlloc, MEM_RESERVE, PAGE_NOACCESS);

	if (pvR == 0)
		return(0);

	pvC = VirtualAlloc(pvR, cbAlloc - PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

	if (pvC != pvR)
	{
        TraceMsg(TF_ERROR, "MallocSpy: Problem allocating resrved memory (pvC=%x, pvR=%x)",pvC,pvR);
		VirtualFree(pvR, 0, MEM_RELEASE);
		return(0);
	}

	*(ULONG *)pvC = cb;

	memset((BYTE *)pvC + sizeof(ULONG), 0xAD,
		(UINT) cbAlloc - cb - sizeof(ULONG) - PAGE_SIZE);

	return((BYTE *)pvC + (cbAlloc - cb - PAGE_SIZE));
}

void * VMAllocClear(ULONG cb)
{
    void *pv = VMAlloc(cb);

    if (pv)
    {
        memset(pv, 0, cb);
    }

    return(pv);
}

void VMFree(void *pv)
{
	VMValidatePv(pv);
	VirtualFree(PvToVMBase(pv), 0, MEM_RELEASE);
}

HRESULT VMRealloc(void **ppv, ULONG cb)
{
    void *  pvOld  = *ppv;
	void *	pvNew  = 0;
	ULONG	cbCopy = 0;

    if (pvOld)
    {
        VMValidatePv(pvOld);
        cbCopy = *(ULONG *)PvToVMBase(pvOld);
        if (cbCopy > cb)
            cbCopy = cb;
    }

    if (cb)
    {
        pvNew = VMAlloc(cb);

        if (pvNew == 0)
            return(E_OUTOFMEMORY);

        if (cbCopy)
        {
            memcpy(pvNew, pvOld, cbCopy);
        }
    }

    if (pvOld)
    {
        VMFree(pvOld);
    }

    *ppv = pvNew;
    return(S_OK);
}

ULONG VMGetSize(void *pv)
{
    VMValidatePv(pv);
	return(*(ULONG *)PvToVMBase(pv));
}

#if 0
    void EnterSpyAlloc()
    {
        EnsureThreadState();
        TLS(fSpyAlloc) += 1; 
    }

    void LeaveSpyAlloc()
    {
        TLS(fSpyAlloc) -= 1;
    }
#else
#define EnterSpyAlloc()
#define LeaveSpyAlloc()
#endif

void CMallocSpy::SpyEnqueue(SPYBLK * psb)
{
    EnterSpyCrit();

    _iAllocs++;
    _iBytes += psb->cbRequest;

    psb->psbNext  = _psbHead;
    psb->dwSig = SPYSIG;
    _psbHead     = psb;

    LeaveSpyCrit();
}

SPYBLK * CMallocSpy::SpyDequeue(void * pvRequest)
{
    SPYBLK ** ppsb, * psb;

    EnterSpyCrit();

    for (ppsb = &_psbHead; (psb = *ppsb) != NULL; ppsb = &psb->psbNext)
    {
        if (psb->pvRequest == pvRequest)
        {
            *ppsb = psb->psbNext;
            break;
        }
    }

    if (psb)
    {
        _iAllocs--;
        _iBytes -= psb->cbRequest;
    }

    LeaveSpyCrit();

    return(psb);
}

SPYBLK * CMallocSpy::_SpyFindBlock(void * pvRequest)
{
    SPYBLK ** ppsb, * psb;

    EnterSpyCrit();

    for (ppsb = &_psbHead; (psb = *ppsb) != NULL; ppsb = &psb->psbNext)
        if (psb->pvRequest == pvRequest)
            break;

    LeaveSpyCrit();

    return(psb);
}

LPVOID CMallocSpy::SpyPostAlloc(void * pvActual)
{
    //EnsureThreadState();
    SPYBLK * psb = (SPYBLK *)pvActual;
    THREADSTATE *pts = GetThreadState();

    if (!psb)
        return NULL;

    psb->cbRequest = pts->cbRequest;
    psb->pvRequest = VMAlloc(psb->cbRequest);
    psb->dwThreadId = pts->dwThreadId;
    psb->bOKToLeak = pts->cTrackDisable;
    psb->cRealloc = 0;

    if (_fRegistered && IS_STACKTRACE_ON() && !psb->bOKToLeak)
        GetStackBacktrace(4 /*level to start*/, ARRAYSIZE(psb->rdwStack), psb->rdwStack, psb->szStackSym);

    if (psb->pvRequest)
    {
        SpyEnqueue(psb);
    }

    return(psb->pvRequest);
}

LPVOID CMallocSpy::SpyPreFree(void * pvRequest)
{
    if (!pvRequest)
        return NULL;

    SPYBLK * psb = SpyDequeue(pvRequest);

    if (psb)
    {
        ASSERT(psb->dwSig == SPYSIG);

        VMFree(pvRequest);
    }
    else
        TraceMsg(TF_ERROR, "SpyPreFree - can't find supposedly allocated block");

    return(psb);
}

size_t CMallocSpy::SpyPreRealloc(void *pvRequest, size_t cbRequest, void **ppv)
{
    EnsureThreadState();
    size_t          cb;
    THREADSTATE *   pts = GetThreadState();
    SPYBLK *        psb = SpyDequeue(pvRequest);

    pts->cbRequest = cbRequest;
    pts->pvRequest = pvRequest;

    if (pvRequest == NULL)
    {
        *ppv = NULL;
        cb   = sizeof(SPYBLK);
    }
    else if (cbRequest == 0)
    {
        *ppv = SpyPreFree(pvRequest);
        cb   = 0;
    }
    else
    {
        if (!psb)
        {
            TraceMsg(TF_ERROR, "SpyPreRealloc - can't find supposedly allocated block");
            ASSERT(0);
        }

        ASSERT(psb->dwSig == SPYSIG);

        *ppv = psb;
        cb   = sizeof(SPYBLK);
    }

    return cb;
}

void * CMallocSpy::SpyPostRealloc(void * pvActual)
{
    //EnsureThreadState();
    void *          pvReturn;
    THREADSTATE *   pts = GetThreadState();
    SPYBLK *        psb = (SPYBLK *)pvActual;

    if (pts->pvRequest == NULL)
    {
        pvReturn = SpyPostAlloc(pvActual);
    }
    else if (pts->cbRequest == 0)
    {
        ASSERT(pvActual == NULL);
        pvReturn = NULL;
    }
    else
    {
        if (pvActual == NULL)
        {
            TraceMsg(TF_ERROR, "IMallocSpy: realloc coulnd't get passed pointer!");
            // The realloc failed.  Hook the block back into the list.

            SpyEnqueue(psb);
            pvReturn = NULL;
        }
        else
        {
//            TraceMsg(TF_ALWAYS, "ims::postrealloc- resizing %x from %d to %d",psb->pvRequest, psb->cbRequest, pts->cbRequest);
            psb->cbRequest = pts->cbRequest;
            psb->cRealloc++;

            if (FAILED(VMRealloc(&psb->pvRequest, psb->cbRequest)))
            {
                TraceMsg(TF_ERROR, "IMallocSpy: realloc FAILED");
                VMFree(psb->pvRequest);
                psb->pvRequest = NULL;
            }

            if (psb->pvRequest)
            {
                SpyEnqueue(psb);
            }

            pvReturn = psb->pvRequest;
        }
    }

    return pvReturn;
}


// note- never called, because someone always has a ref to us.
CMallocSpy::~CMallocSpy()
{
    SPYBLK *psb, *psbLast;
    TraceMsg(TF_ALWAYS, "CMallocSpy::dtor");
    TlsFree(g_dwTls);
    g_dwTls = INVALID_TLS_VALUE;

    CloseHandle(_hProcess);
#ifndef USEPRIVATECRIT
    DeleteCriticalSection(&g_csSpy);
#endif
    DeleteCriticalSection(&_csImgHlp);

    for (psb = _psbHead;psb;)
    {
        psbLast=psb;
        psb=psb->psbNext;
        LocalFree(psbLast);
    }
}

STDMETHODIMP CMallocSpy::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IUnknown || riid == IID_IMallocSpy)
    {
        *ppv = SAFECAST(this, IMallocSpy*);
    }
    else if (riid == IID_IShellMallocSpy)
    {
        *ppv = SAFECAST(this, IShellMallocSpy *);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;

}

STDMETHODIMP_(ULONG) CMallocSpy::AddRef()
{
    return 2; // InterlockedIncrement((LONG *)&_ulRef);
}

STDMETHODIMP_(ULONG) CMallocSpy::Release()
{
    return 1;
#if 0
    if (InterlockedDecrement((LONG *)&_ulRef))
        return _ulRef;
    else
    {
        delete this;
        g_pms = NULL;
        return 0;
    }
#endif
}

STDMETHODIMP_(ULONG) CMallocSpy::PreAlloc(ULONG cbRequest)
{
    EnsureThreadState();
    TLS(cbRequest) = cbRequest;
    return sizeof(SPYBLK);
}

STDMETHODIMP_(void *) CMallocSpy::PostAlloc(void *pvActual)
{
    void * pv;

    EnterSpyAlloc();

    pv = SpyPostAlloc(pvActual);

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(void *) CMallocSpy::PreFree(void *pvRequest, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (fSpyed)
        pv = SpyPreFree(pvRequest);
    else 
        pv = pvRequest;
    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(void) CMallocSpy::PostFree(BOOL fSpyed)
{
    EnterSpyAlloc();

    if (!fSpyed)
        TraceMsg(TF_WARNING, "CMallocSpy freeing a block alloced before we started");

    LeaveSpyAlloc();
}

STDMETHODIMP_(ULONG) CMallocSpy::PreRealloc(
    void *pvRequest, 
    ULONG cbRequest, 
    void **ppvActual, 
    BOOL fSpyed)
{
    ULONG cb;

    EnterSpyAlloc();

    if (fSpyed)
        cb = SpyPreRealloc(pvRequest, cbRequest, ppvActual);
    else 
    {
        *ppvActual = pvRequest;
        cb = cbRequest;
    }

    LeaveSpyAlloc();

    return cb;
}

STDMETHODIMP_(void *) CMallocSpy::PostRealloc(void *pvActual, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (fSpyed)
        pv = SpyPostRealloc(pvActual);
    else 
        pv = pvActual;

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(void *) CMallocSpy::PreGetSize(void *pvRequest, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (fSpyed)
    {
        SPYBLK * psb = _SpyFindBlock(pvRequest);
        EnsureThreadState();
        TLS(pvRequest) = psb;
        pv = psb;
    }
    else
        pv = pvRequest;

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(ULONG) CMallocSpy::PostGetSize(ULONG cbActual, BOOL fSpyed)
{
    ULONG cb;

    EnterSpyAlloc();

    if (fSpyed)
    {
#define psb ((SPYBLK *)pts->pvRequest)
        THREADSTATE * pts = GetThreadState();

        if (psb)
        {
            ASSERT(psb->dwSig == SPYSIG);
            if (psb->pvRequest)
                cb = psb->cbRequest;
            else
                cb = 0;
        }
#undef psb
    }
    else 
        cb = cbActual;

    LeaveSpyAlloc();

    return cb;
}

STDMETHODIMP_(void *) CMallocSpy::PreDidAlloc(void *pvRequest, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (fSpyed)
    {
        pv = _SpyFindBlock(pvRequest);
    }
    else 
        pv = pvRequest;

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(BOOL) CMallocSpy::PostDidAlloc(void *pvRequest, BOOL fSpyed, BOOL fActual)
{
    return fActual;
}

STDMETHODIMP_(void) CMallocSpy::PreHeapMinimize()
{
}

STDMETHODIMP_(void) CMallocSpy::PostHeapMinimize()
{
}

BOOL CMallocSpy::WriteToFile(HANDLE hFile, LPCSTR szFormat, ...)
{
    va_list vaArgs;
    DWORD dwResult;
    CHAR szTemp[MAX_PATH];

    va_start(vaArgs, szFormat);
    wvsprintfA(szTemp, szFormat, vaArgs); // note A version
    va_end(vaArgs);

    if (_dwLeakSetting & LEAK_DUMPTOSCREEN)
    {
        TraceMsg(TF_ALWAYS, szTemp);
        return TRUE;
    }
    else
    {
        lstrcatA(szTemp, "\r\n");
        return (WriteFile(hFile, szTemp, lstrlenA(szTemp), &dwResult, NULL) && (dwResult == (DWORD)lstrlenA(szTemp)));
    }
}

BOOL IsGoodPIDL(LPCITEMIDLIST pidl)
{
    // less 'secure' than IsValidPIDL, but it doens't break into debugger.
    return (pidl->mkid.cb <= 512 && (0 == _ILNext(pidl)->mkid.cb || IsGoodPIDL(_ILNext(pidl))) );
}

BOOL IsAsciiString(LPSTR pv, int cb)
{
    BOOL bRet = TRUE;
    while(cb && *pv && bRet)
    {
        if (*pv > '~' || *pv < ' ')
            bRet = FALSE;
        cb--;
        pv++;
    }
    return (!cb && bRet && !*pv); // we've only had ascii characters, and we're null terminated
}

BOOL CMallocSpy::DumpLeaks(LPCTSTR szFile)
{
    HANDLE hFile;
    int i,iBytes, iStack, iUniFlags;
    SPYBLK *psbWalk;
    BOOL bRet=FALSE;
    DWORD dwOpenFlag = (_dwLeakSetting & LEAK_OVERWRITELOG) ? CREATE_ALWAYS : OPEN_ALWAYS;
    DWORD dwPID = GetCurrentProcessId();

    if (!_psbHead)
    {
        return TRUE;
    }
    hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwOpenFlag, FILE_ATTRIBUTE_NORMAL, NULL);

    if (!hFile || hFile == INVALID_HANDLE_VALUE)
    {
        TraceMsg(TF_WARNING, "IMallocSpy: could not open ALLOCSPY.LOG");
        return FALSE;
    }

    SetFilePointer(hFile, 0, NULL, FILE_END); // get to end of file

    WriteToFile(hFile, "-===  Leak tracking begun at %d/%d/%d %02d:%02d:%02d  ===-",
        g_st.wMonth, g_st.wDay, g_st.wYear,
        g_st.wHour, g_st.wMinute, g_st.wSecond);

    EnterSpyCrit();
    i=0; iBytes=0;
    psbWalk = _psbHead;

    while(psbWalk)
    {
        __try
        {
            if (psbWalk->bOKToLeak)
            {
                goto Loop;
            }
            iUniFlags = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_STATISTICS;

            WriteToFile(hFile, "\r\n* Leaked %4d bytes at 0x%X, from thread %d", psbWalk->cbRequest, psbWalk->pvRequest, psbWalk->dwThreadId);
            if (psbWalk->cRealloc)
                WriteToFile(hFile, "  Data was re-alloced %d times", psbWalk->cRealloc);

            if (psbWalk->cbRequest >= *((USHORT*)psbWalk->pvRequest) &&
                IsGoodPIDL((LPCITEMIDLIST) psbWalk->pvRequest) )
            {
                if (FS_IsValidID((LPITEMIDLIST)psbWalk->pvRequest))
                {
                    TCHAR szTemp[MAX_PATH];
                    SHGetPathFromIDList((LPCITEMIDLIST) psbWalk->pvRequest, szTemp);
                    if (szTemp[0])
                    {
#ifdef UNICODE
                        WriteToFile(hFile, "  Data is pidl for '%ls'",szTemp);
#else
                        WriteToFile(hFile, "  Data is pidl for '%s'",szTemp);
#endif
                    }
                    else if (psbWalk->cbRequest > 16 && SIL_GetType((LPITEMIDLIST)psbWalk->pvRequest)== SHID_FS_FILE/*0x32*/)
                        WriteToFile(hFile, "  Data may be a relative pidl for '%s'", ((LPBYTE)psbWalk->pvRequest)+14);
                }
            }
            else 
                if (psbWalk->cbRequest > 8 && IsTextUnicode((LPWSTR)psbWalk->pvRequest, psbWalk->cbRequest - 2, &iUniFlags))
            {
                WriteToFile(hFile, "  Data is UNICODE string '%ls'", (LPWSTR)psbWalk->pvRequest);
            }
            else if (IsAsciiString((LPSTR)psbWalk->pvRequest, psbWalk->cbRequest))
                WriteToFile(hFile, "  Data is ASCII string '%s'", psbWalk->pvRequest);

            if (IS_STACKTRACE_ON())
            {
                for (iStack=0;iStack < ARRAYSIZE(psbWalk->rdwStack) && psbWalk->rdwStack[iStack];iStack++)
                {
                    WriteToFile(hFile, "   %s", &psbWalk->szStackSym[iStack*SYM_MAXNAMELEN]);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            bRet = FALSE;
            TraceMsg(TF_ERROR, "Faulted trying to print out leaks!!!! (i=%d)",i);
            break;
        }

        iBytes += psbWalk->cbRequest;
        i++;
Loop:
//        psbLast = psbWalk;
        psbWalk = psbWalk->psbNext;
//        LocalFree(psbLast); // free these when the object goes away
    }
    LeaveSpyCrit();

    WriteToFile(hFile, "****** Leaked %d blocks totalling %d bytes\r\n", i, iBytes);
    if (!(_dwLeakSetting & LEAK_DUMPTOSCREEN))
        TraceMsg(TF_ERROR, "*** IMallocSpy: %d blocks (%d bytes) leaked, see ALLOCSPY.LOG", i, iBytes);
    
    CloseHandle(hFile);
    return bRet;
}


DWORD GetLeakSetting()
{
    static DWORD s_dwLeak = 0;
    static BOOL  s_bInit  = FALSE;

    if (!s_bInit)
    {
        CHAR szRHS[16]; // space for 0x12345678\0
        int val;
        GetPrivateProfileStringA(c_szCcshellIniSecDebug,
                                "LeakDetect",
                                "0",
                                szRHS,
                                ARRAYSIZE(szRHS),
                                c_szCcshellIniFile);

        if (StrToIntExA(szRHS, STIF_SUPPORT_HEX, &val))
            s_dwLeak = (DWORD)val;
        s_bInit = TRUE;
    }
    return s_dwLeak;
}


// Stack stuff

DWORD GetModuleBase(HANDLE hProcess, DWORD ReturnAddress)
{
    IMAGEHLP_MODULE          ModuleInfo;

//    ZeroMemory(&ModuleInfo,sizeof(ModuleInfo));
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    if (g_pfnSymGetModuleInfo(hProcess, ReturnAddress, &ModuleInfo))
    {
        return ModuleInfo.BaseOfImage;
    }
    else
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPVOID)ReturnAddress, &mbi, sizeof(mbi)))
        {
#ifdef UNICODE
            if (mbi.Type & MEM_IMAGE)
#endif
            {
                int cch;
                char achFile[MAX_PATH];

                cch = GetModuleFileNameA((HINSTANCE)mbi.AllocationBase,
                                         achFile,
                                         ARRAYSIZE(achFile));

                // Ignore the return code since we can't do anything with it.
                g_pfnSymLoadModule(hProcess,
                               NULL,
                               ((cch) ? achFile : NULL),
                               NULL,
                               (DWORD)mbi.AllocationBase,
                               0);

                return (DWORD)mbi.AllocationBase;
            }
        }
    }

    return 0;
}

int CMallocSpy::GetStackBacktrace(int ifrStart,
                                  int cfrTotal,
                                  DWORD *pdwEip,
                                  LPSTR szSym)
{
    HANDLE        hThread;
    CONTEXT       context;
    STACKFRAME    stkfrm;
    DWORD         dwMachType;
    int           i;
    DWORD       * pdw        = pdwEip;

    ZeroMemory(pdwEip, cfrTotal * sizeof(DWORD));

    if (!g_pfnStackWalk)
    {
        TraceMsg(TF_WARNING, "Not getting StackBacktrace, because no imghlp functions");
        return 0;
    }
    hThread  = GetCurrentThread();

    context.ContextFlags = CONTEXT_FULL;

    if (GetThreadContext(hThread, &context))
    {
        ZeroMemory(&stkfrm, sizeof(STACKFRAME));

        stkfrm.AddrPC.Mode      = AddrModeFlat;

#if defined(_M_IX86)
        dwMachType              = IMAGE_FILE_MACHINE_I386;
        stkfrm.AddrPC.Offset    = context.Eip;  // Program Counter

        stkfrm.AddrStack.Offset = context.Esp;  // Stack Pointer
        stkfrm.AddrStack.Mode   = AddrModeFlat;
        stkfrm.AddrFrame.Offset = context.Ebp;  // Frame Pointer
        stkfrm.AddrFrame.Mode   = AddrModeFlat;
#elif defined(_M_MRX000)
        dwMachType              = IMAGE_FILE_MACHINE_R4000;
        stkfrm.AddrPC.Offset    = context.Fir;  // Program Counter
#elif defined(_M_ALPHA)
        dwMachType              = IMAGE_FILE_MACHINE_ALPHA;
        stkfrm.AddrPC.Offset    = (unsigned long) context.Fir;  // Program Counter
#elif defined(_M_PPC)
        dwMachType              = IMAGE_FILE_MACHINE_POWERPC;
        stkfrm.AddrPC.Offset    = context.Iar;  // Program Counter
#elif
#error("Unknown Target Machine");
#endif

        //
        // We have to use a critical section because MSPDB50.DLL (and
        // maybe imagehlp.dll) is not reentrant and simultaneous calls
        // to StackWalk cause it to tromp on its own memory.
        //

        EnterCriticalSection(&_csImgHlp);
        for (i = 0; i < ifrStart + cfrTotal; i++)
        {
            if (!g_pfnStackWalk(dwMachType,
                            _hProcess,
                            hThread,
                            &stkfrm,
                            &context,
                            NULL,
                            g_pfnFunctionTableAccessRoutine,
                            GetModuleBase,
                            NULL))
            {
                break;
            }

            if (i >= ifrStart)
            {
                *pdw++ = stkfrm.AddrPC.Offset;
                // Yes, its lame to get the symbol here, but we don't have access to
                // ImageHlp when we are printing out the leaks.
                // note that this is NOT a slow call, ImageHlp already loads the symbols for the stackwalk
                if (szSym)
                    GetSymbolName(&szSym[(i-ifrStart)*SYM_MAXNAMELEN], _hProcess, stkfrm.AddrPC.Offset);
            }
        }
        LeaveCriticalSection(&_csImgHlp);
    }
    return pdw - pdwEip;
}

void GetSymbolName(LPSTR pszSymName, HANDLE hProcess, DWORD dwAddr)
{
    union {
        IMAGEHLP_SYMBOL  sym;
        CHAR szOverflow[sizeof(IMAGEHLP_SYMBOL) + SYM_MAXNAMELEN + 32]; // extra room for decoration
    };
    DWORD dwOffset;
    CHAR szUndecorated[SYM_MAXNAMELEN - 16]; // leave some room for module
    CHAR szModule[16];
    IMAGEHLP_MODULE  mi;

    ZeroMemory(&mi, sizeof(mi));
    mi.SizeOfStruct = sizeof(mi);


    if (!g_pfnSymGetModuleInfo(hProcess, dwAddr, &mi))
    {
        lstrcpyA(szModule, "<UNKNOWN>");
    }
    else
    {
        lstrcpynA(szModule, mi.ModuleName, ARRAYSIZE(szModule)-1);
    }

    sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    sym.Address = dwAddr;
    sym.MaxNameLength = SYM_MAXNAMELEN;

    if (g_pfnSymGetSymFromAddr(hProcess, dwAddr, &dwOffset, &sym))
    {
        CHAR *pszSymbol;

        pszSymbol = sym.Name;
        if (g_pfnSymUnDName(&sym, szUndecorated, ARRAYSIZE(szUndecorated)-1))
            pszSymbol = szUndecorated;

/*      
        IMAGEHLP_LINE ilLine;
        ZeroMemory(&ilLine, sizeof(ilLine));
        ilLine.SizeOfStruct = sizeof(ilLine);

        if (g_pfnSymGetLineFromAddr(g_hProcess, dwAddr, &dwDisplacement, &ilLine))
        {
            wsprintfA(pszSymName, "%s!%s+0x%X    (%s, line %d + %d)", szModule, pszSymbol, dwOffset,
                ilLine.FileName, ilLine.LineNumber, dwDisplacement);
        }
        else
*/
        {
            wsprintfA(pszSymName, "%s!%s+0x%X", szModule, pszSymbol, dwOffset);
        }

    }
    else
    {
        wsprintfA(pszSymName, "%s+0x%X", szModule, dwAddr - mi.BaseOfImage);
    }
}

typedef BOOL (__stdcall *pfnImgHlp_SymInitialize)(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     fInvadeProcess
    );

#define LOAD_FUNCTION(fn) g_pfn##fn = (pfnImgHlp_##fn) GetProcAddress(g_ImgHlp, #fn);

// Set things up so we can get symbols
BOOL CMallocSpy::SymInit()
{
    pfnImgHlp_SymInitialize _SymInitialize;

    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &_hProcess,
        PROCESS_ALL_ACCESS, TRUE, 0))
        _hProcess = GetCurrentProcess(); // will just put 0xfffffff here

    if(IS_STACKTRACE_ON())
    {
        if ( !(g_ImgHlp=LoadLibrary(TEXT("IMAGEHLP.DLL"))) )
        {
            return FALSE;
        }

        LOAD_FUNCTION(StackWalk);           //g_pfnStackWalk = (pfnImgHlp_StackWalk) GetProcAddress(g_ImgHlp, "StackWalk");
        ASSERT(g_pfnStackWalk);
        LOAD_FUNCTION(SymGetModuleInfo);    //g_pfnSymGetModuleInfo = (pfnImgHlp_SymGetModuleInfo) GetProcAddress(g_ImgHlp, "SymGetModuleInfo");
        LOAD_FUNCTION(SymLoadModule);       //g_pfnSymLoadModule = (pfnImgHlp_SymLoadModule)GetProcAddress(g_ImgHlp, "SymLoadModule");
        LOAD_FUNCTION(SymGetSymFromAddr);   //g_pfnSymGetSymFromAddr = (pfnImgHlp_SymGetSymFromAddr) GetProcAddress(g_ImgHlp, "SymGetSymFromAddr");
        LOAD_FUNCTION(SymUnDName);          //g_pfnSymUnDName = (pfnImgHlp_SymUnDName) GetProcAddress(g_ImgHlp, "SymUnDName");
        LOAD_FUNCTION(SymGetLineFromAddr);  //g_pfnSymGetLineFromAddr = (pfnImgHlp_SymGetLineFromAddr) GetProcAddress(g_ImgHlp, "SymGetLineFromAddr");
        g_pfnFunctionTableAccessRoutine = (PFUNCTION_TABLE_ACCESS_ROUTINE)GetProcAddress(g_ImgHlp, "SymFunctionTableAccess");
        _SymInitialize = (pfnImgHlp_SymInitialize) GetProcAddress(g_ImgHlp, "SymInitialize");

        return _SymInitialize(_hProcess, NULL, FALSE);
    }
    else
        return TRUE;
}

BOOL DoActualRegister(IMallocSpy *pms)
{
    // Note GetModuleHandle, not LoadLibrary!  We are trying not to load OLE
    HMODULE hmod =GetModuleHandle(TEXT("ole32.dll"));
    if (hmod)
    {
        pfnCoRegisterMallocSpy CoRegisterMallocSpy = (pfnCoRegisterMallocSpy) GetProcAddress(hmod, "CoRegisterMallocSpy");

        if (CoRegisterMallocSpy)
        {
            if (SUCCEEDED(CoRegisterMallocSpy(pms)) )
            {
                TraceMsg(TF_ALWAYS, "Registered MallocSpy");
                return TRUE;
            }
            else
            {
                TraceMsg(TF_ERROR, "Could NOT register MallocSpy");
            }
        }
    }
    return FALSE;
}

CMallocSpy::CMallocSpy()
{

#ifdef USEPRIVATECRIT
    g_lCrit = 0;
    g_lCritCount=0;
#else
    InitializeCriticalSection(&g_csSpy);
#endif

    // Set up some globals
    g_dwTls = TlsAlloc();
    ASSERT(g_dwTls != INVALID_TLS_VALUE);
    EnsureThreadState();

    InitializeCriticalSection(&_csImgHlp);

    _dwLeakSetting = GetLeakSetting();
    TraceMsg(TF_ALWAYS, "AllocSpy loading with leak detect flags 0x%X",_dwLeakSetting);

    if (!SymInit())
        TraceMsg(TF_ERROR, "Could not load ImageHlp functions for IMallocSpy");
}


STDMETHODIMP CMallocSpy::RegisterSpy()
{
    if (!g_bRegistered)
        g_bRegistered = DoActualRegister(SAFECAST(this, IMallocSpy*));

    if (_fRegistered)
        return HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING);

    if (!_dwLeakSetting)
        return S_FALSE;

    // We only want to attatch to explorer
//    if (StrNCmpI(GetCurrentApp(), TEXT("EXPLORER"), 8) )
//        return;

    _fRegistered = TRUE;

    return S_OK;
}

STDMETHODIMP CMallocSpy::RevokeSpy(void)
{
    if (!_dwLeakSetting)
        return S_FALSE;

    if (_fRegistered)
    {
        HRESULT hres;

        SetTracking(FALSE); // avoid re-entrancy in imagehlp

        DumpLeaks(TEXT("\\ALLOCSPY.LOG"));
        SetTracking(TRUE);
        _fRegistered = FALSE;

        HMODULE hmod = GetModuleHandle(TEXT("ole32.dll"));
        pfnCoRevokeMallocSpy CoRevokeMallocSpy = (pfnCoRevokeMallocSpy) GetProcAddress(hmod, "CoRevokeMallocSpy");
        if (CoRevokeMallocSpy)
        {
            hres = CoRevokeMallocSpy();
            if (hres == E_ACCESSDENIED)
                TraceMsg(TF_WARNING, "MallocSpy still active, since blocks are still allocated");
            TraceMsg(TF_ALWAYS, "Revoked MallocSpy (_psbHead==%x)",_psbHead);
        }
        else
            TraceMsg(TF_WARNING, "could not delay-load CoRevokeMallocSpy");
    }
    else
        TraceMsg(TF_ERROR, "DbgRevokeMallocSpy: spy at not registered");

    if (g_ImgHlp)
        FreeLibrary(g_ImgHlp);
    return S_OK;
}

HRESULT CMallocSpy::SetTracking(BOOL bDetect)
{
    THREADSTATE *pts;

    EnsureThreadState();
    pts = GetThreadState();

    if (bDetect)
        pts->cTrackDisable--;
    else
        pts->cTrackDisable++;
    ASSERT(pts->cTrackDisable >= 0);
    return S_OK;
}

HRESULT CMallocSpy::AddToList(void*pv, UINT cb)
{
    if (!_dwLeakSetting)
        return S_FALSE;

    EnsureThreadState();

    SPYBLK * psb= (SPYBLK*)LocalAlloc(LPTR, sizeof(SPYBLK));
    THREADSTATE *pts = GetThreadState();

    if (!psb)
        return E_OUTOFMEMORY;

    psb->cbRequest = cb;
    psb->pvRequest = pv;
    psb->dwThreadId = pts->dwThreadId;
    psb->bOKToLeak = pts->cTrackDisable;
//    psb->cRealloc = 0; // zero allocator

    if (IS_STACKTRACE_ON() && !psb->bOKToLeak)
        GetStackBacktrace(3 /*level to start*/, ARRAYSIZE(psb->rdwStack), psb->rdwStack, psb->szStackSym);

    if (psb->pvRequest)
    {
        SpyEnqueue(psb);
    }

    return S_OK;
}

HRESULT CMallocSpy::RemoveFromList(void *pv)
{
    if (!_dwLeakSetting)
        return S_FALSE;

    SPYBLK * psb = SpyDequeue(pv);
    if (psb)
    {
        ASSERT(psb->dwSig == SPYSIG);
    }
    else
        TraceMsg(TF_ERROR, "MallocSpy - can't find supposedly allocated block at %x",pv);

    return S_OK;
}

void __cdecl operator delete(void *pv)
{
    LocalFree(pv);
}

void * __cdecl operator new(unsigned int cb)
{
    return LocalAlloc(LPTR, cb);
}

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvRes)
{
    switch(ulReason)
    {
        case DLL_PROCESS_ATTACH:
            TCHAR szTemp[MAX_PATH];

            GetModuleFileName(GetModuleHandle(NULL), szTemp, ARRAYSIZE(szTemp));

            TraceMsg(TF_ALWAYS, "PROCESS_ATTACH: %s %d",szTemp, GetCurrentProcessId());
            g_pms = new CMallocSpy;
            CcshellGetDebugFlags();

            GetLocalTime(&g_st); // per-instance
            break;

        case DLL_PROCESS_DETACH:
            GetModuleFileName(GetModuleHandle(NULL), szTemp, ARRAYSIZE(szTemp));

            TraceMsg(TF_ALWAYS, "PROCESS_DETACH: %s %d",szTemp, GetCurrentProcessId());
            if (g_pms)
                if (g_pms->_fRegistered)
                {
                    g_pms->RevokeSpy();
                }
            else
                delete g_pms;
            break;
    }
    return TRUE;
}

STDAPI GetShellMallocSpy(IShellMallocSpy **ppout)
{
    if (g_pms)
    {
        *ppout = g_pms;
        (*ppout)->AddRef();
        return TRUE;
    }
    *ppout = NULL;
    return FALSE;
}

