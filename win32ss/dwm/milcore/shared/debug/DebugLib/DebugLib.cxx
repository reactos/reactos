// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  File:       DebugLib.cxx
//  Contents:   Interface to debugging .dll (if available)
//------------------------------------------------------------------------------
#include "Always.h"
#include "strsafe.h"

//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

struct TAGINFO
{
    CHAR *  pchOwner;
    CHAR *  pchDesc;
    BOOL    fEnabled;
};

TAGINFO g_rgtaginfo[] =
{
    { "Debug",      "General debug output",                     TRUE    },  //  0: tagDefault
    { "Trace",      "Errors",                                   TRUE    },  //  1: tagError
    { "Trace",      "Warnings",                                 FALSE   },  //  2: tagWarning
    { "Thread",     "Thread related tracing",                   FALSE   },  //  3: tagThread
    { "Assert",     "Exit on asserts",                          FALSE   },  //  4: tagAssertExit
    { "Assert",     "Stacktraces on asserts",                   TRUE    },  //  5: tagAssertStacks
    { "Memory",     "Use VMem for MemAlloc",                    FALSE   },  //  6: tagMemoryStrict
    { "Memory",     "Use VMem for CoTaskMemAlloc",              FALSE   },  //  7: tagCoMemoryStrict
    { "Memory",     "Use VMem strict at end (vs beginning)",    FALSE   },  //  8: tagMemoryStrictTail
    { "Memory",     "VMem pad to quadword at end",              FALSE   },  //  9: tagMemoryStrictAlign
    { "Trace",      "All calls to OCX interfaces",              FALSE   },  // 10: tagOLEWatch 
    { "FALSE",      "FALSE",                                    FALSE   },  // 12: tagFALSE
};

#define TAG_NONAME              0x01
#define TAG_NONEWLINE           0x02
#define ARRAY_SIZE(x)           (sizeof(x) / sizeof(x[0]))

#define local_tagDefault              ((TRACETAG)0)
#define local_tagError                ((TRACETAG)1)
#define local_tagWarning              ((TRACETAG)2)
#define local_tagThread               ((TRACETAG)3)
#define local_tagAssertExit           ((TRACETAG)4)
#define local_tagAssertStacks         ((TRACETAG)5)
#define local_tagMemoryStrict         ((TRACETAG)6)
#define local_tagCoMemoryStrict       ((TRACETAG)7)
#define local_tagMemoryStrictTail     ((TRACETAG)8)
#define local_tagMemoryStrictAlign    ((TRACETAG)9)
#define local_tagOLEWatch             ((TRACETAG)10)
#define local_tagFALSE                ((TRACETAG)12)

// Data ------------------------------------------------------------------------
HINSTANCE g_hInstDbg = NULL;
HINSTANCE g_hInstLeak = NULL;

//------------------------------------------------------------------------------

char * GetModuleName(HINSTANCE hInst)
{
    static char achMod[MAX_PATH];
    achMod[0] = 0;
    GetModuleFileNameA(hInst, achMod, sizeof(achMod));
    char * psz = &achMod[lstrlenA(achMod)];
    while (psz > achMod && *psz != '\\' && *psz != '//') --psz;
    if (*psz == '\\' || *psz == '//') ++psz;
    return(psz);
}

void LeakDumpAppend(__in PSTR pszMsg, void * pvArg = NULL)
{
    HANDLE hFile;
    char ach[1024];
    char *pEnd = NULL;
    size_t iRemaining = 0;
    DWORD dw;

    StringCchCopyA(ach, ARRAY_SIZE(ach), GetModuleName(g_hInstLeak));
    StringCchCatExA(ach, ARRAY_SIZE(ach), ": ", &pEnd, &iRemaining, 0);
    StringCchVPrintfA(pEnd, iRemaining, pszMsg, (va_list)pvArg);

    hFile = CreateFileA("c:\\leakdump.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        WriteFile(hFile, ach, lstrlenA(ach), &dw, NULL);
        WriteFile(hFile, "\r\n", 2, &dw, NULL);
        CloseHandle(hFile);
    }
}

// Many stub ignore their parameters.
#pragma warning(push)
#pragma warning(disable:4100) /* unreferenced formal parameter               */


DWORD WINAPI _DbgExGetVersion()
{
    return(AVALON_DEBUG_API_VERSION);
}

BOOL WINAPI _DbgExIsFullDebug()
{
    return(FALSE);
}

void WINAPI _DbgExAddRefDebugLibrary()
{
}

void WINAPI _DbgExReleaseDebugLibrary()
{
}

void WINAPI _DbgExSetDllMain(HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID))
{
}

void WINAPI _DbgExDoTracePointsDialog(BOOL fWait)
{
}

void WINAPI _DbgExRestoreDefaultDebugState()
{
}

BOOL WINAPI _DbgExEnableTag(TRACETAG tag, BOOL fEnable)
{
    BOOL fOld = FALSE;

    if (tag > 0 && tag < ARRAY_SIZE(g_rgtaginfo) - 1)
    {
        fOld = g_rgtaginfo[tag].fEnabled;
        g_rgtaginfo[tag].fEnabled = fEnable;
    }

    return(fOld);
}

BOOL WINAPI _DbgExSetDiskFlag(TRACETAG tag, BOOL fSendToDisk)
{
    return(FALSE);
}

BOOL WINAPI _DbgExSetBreakFlag(TRACETAG tag, BOOL fBreak)
{
    return(FALSE);
}

BOOL WINAPI _DbgExIsTagEnabled(TRACETAG tag)
{
    return(tag >= 0 && tag < ARRAY_SIZE(g_rgtaginfo) && g_rgtaginfo[tag].fEnabled);
}

TRACETAG WINAPI _DbgExFindTag(__in PCSTR szTagDesc)
{
    TAGINFO * pti = g_rgtaginfo;
    TRACETAG tag;

    for (tag = 0; tag < ARRAY_SIZE(g_rgtaginfo); ++tag, ++pti)
    {
        if (!lstrcmpiA(pti->pchDesc, szTagDesc))
        {
            return(tag);
        }
    }

    return(local_tagFALSE);
}

TRACETAG WINAPI _DbgExTagError()
{
    return(local_tagError);
}

TRACETAG WINAPI _DbgExTagWarning()
{
    return(local_tagWarning);
}

TRACETAG WINAPI _DbgExTagThread()
{
    return(local_tagThread);
}

TRACETAG WINAPI _DbgExTagAssertExit()
{
    return(local_tagAssertExit);
}

TRACETAG WINAPI _DbgExTagAssertStacks()
{
    return(local_tagAssertStacks);
}

TRACETAG WINAPI _DbgExTagMemoryStrict()
{
    return(local_tagMemoryStrict);
}

TRACETAG WINAPI _DbgExTagCoMemoryStrict()
{
    return(local_tagCoMemoryStrict);
}

TRACETAG WINAPI _DbgExTagMemoryStrictTail()
{
    return(local_tagMemoryStrictTail);
}

TRACETAG WINAPI _DbgExTagMemoryStrictAlign()
{
    return(local_tagMemoryStrictAlign);
}

TRACETAG WINAPI _DbgExTagOLEWatch()
{
    return(local_tagOLEWatch);
}

TRACETAG WINAPI _DbgExTagRegisterTrace(__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, BOOL fEnabled)
{
    TAGINFO * pti = g_rgtaginfo;
    TRACETAG tag;

    for (tag = 0; tag < ARRAY_SIZE(g_rgtaginfo) - 1; ++tag, ++pti)
    {
        if (!lstrcmpiA(pti->pchDesc, szDescrip) && !lstrcmpiA(pti->pchOwner, szOwner))
        {
            return(tag);
        }
    }

    return(local_tagFALSE);
}


BOOL WINAPI _DbgExTaggedTraceListEx(TRACETAG tag, USHORT usFlags, __in PCSTR szFmt, va_list valMarker)
{
    if (DbgExIsTagEnabled(tag))
    {
        CHAR    achDup[512], *pch;
        CHAR    achBuf[1024];
        CHAR    *pStart = achBuf;
        size_t  cch = ARRAY_SIZE(achBuf);
        achBuf[0] = '\0';

        StringCchCopyA(achDup, ARRAY_SIZE(achDup), szFmt);

        for (pch = achDup; *pch; ++pch)
        {
            if (*pch == '%')
            {
                if (pch[1] == '%')
                {
                    ++pch;
                    continue;
                }

                if (pch[1] == 'h' && pch[2] == 'r')
                {
                    pch[1] = 'l';
                    pch[2] = 'X';
                    continue;
                }
            }
        }

        if (!(usFlags & TAG_NONAME))
        {
            StringCchCopyExA(achBuf, ARRAY_SIZE(achBuf), "WPF: ", &pStart, &cch, 0);
        }

        StringCchVPrintfA(pStart, cch, szFmt, valMarker);

        if (!(usFlags & TAG_NONEWLINE))
        {
            StringCchCatA(achBuf, ARRAY_SIZE(achBuf), "\r\n");
        }

        OutputDebugStringA(achBuf);
    }

    return(FALSE);
}

void WINAPI _DbgExTaggedTraceCallers(TRACETAG tag, int iStart, int cTotal)
{
}

void WINAPI _DbgExAssertThreadDisable(BOOL fDisable)
{
}

size_t WINAPI _DbgExPreAlloc(size_t cbRequest, PERFMETERTAG mt)
{
    return(cbRequest);
}

void * WINAPI _DbgExPostAlloc(void *pv)
{
    return(pv);
}

void * WINAPI _DbgExPreFree(void *pv)
{
    if (g_hInstDbg)
    {
        LeakDumpAppend("DbgExPreFree: freeing memory at %08lX", pv);
        pv = NULL;
    }

    return(pv);
}

void WINAPI _DbgExPostFree()
{
}

size_t WINAPI _DbgExPreRealloc(void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt)
{
    *ppv = pvRequest;
    return(cbRequest);
}

void * WINAPI _DbgExPostRealloc(void *pv)
{
    return(pv);
}

void * WINAPI _DbgExPreGetSize(void *pvRequest)
{
    return(pvRequest);
}

size_t WINAPI _DbgExPostGetSize(size_t cb)
{
    return(cb);
}

size_t WINAPI _DbgExMtPreAlloc(size_t cbRequest, PERFMETERTAG mt)
{
    return(cbRequest);
}

void * WINAPI _DbgExMtPostAlloc(void *pv)
{
    return(pv);
}

void * WINAPI _DbgExMtPreFree(void *pv)
{
    if (g_hInstDbg)
    {
        LeakDumpAppend("DbgExMtPreFree: freeing memory at %08lX", pv);
        pv = NULL;
    }

    return(pv);
}

void WINAPI _DbgExMtPostFree()
{
}

size_t WINAPI _DbgExMtPreRealloc(void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt)
{
    *ppv = pvRequest;
    return(cbRequest);
}

void * WINAPI _DbgExMtPostRealloc(void *pv)
{
    return(pv);
}

void * WINAPI _DbgExMtPreGetSize(void *pvRequest)
{
    return(pvRequest);
}

size_t WINAPI _DbgExMtPostGetSize(size_t cb)
{
    return(cb);
}


void WINAPI _DbgExMemoryTrackDisable(BOOL fDisable)
{
}

void WINAPI _DbgExCoMemoryTrackDisable(BOOL fDisable)
{
}

void WINAPI _DbgExMemoryBlockTrackDisable(void * pv)
{
}

void * WINAPI _DbgExGetMallocSpy()
{
    return(NULL);
}

void WINAPI _DbgExTraceMemoryLeaks()
{
}

BOOL WINAPI _DbgExValidateKnownAllocations()
{
    return(TRUE);
}

LONG_PTR WINAPI _DbgExTraceFailL(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return(errExpr);
}

LONG_PTR WINAPI _DbgExTraceWin32L(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return(errExpr);
}

HRESULT WINAPI _DbgExTraceHR(HRESULT hrTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return(hrTest);
}

HRESULT WINAPI _DbgExTraceOLE(HRESULT hrTest, BOOL fIgnore, __in LPSTR pstrExpr, __in LPSTR pstrFile, int line, LPVOID lpsite)
{
    return(hrTest);
}

void WINAPI _DbgExSetSimFailCounts(int firstFailure, int cInterval)
{
}

void WINAPI _DbgExShowSimFailDlg()
{
}

BOOL WINAPI _DbgExFFail()
{
    return(FALSE);
}

int WINAPI _DbgExGetFailCount()
{
    return(INT_MIN);
}

void WINAPI _DbgExOpenMemoryMonitor()
{
}

void WINAPI _DbgExOpenLogFile(LPCSTR szFName)
{
}

void WINAPI _DbgExDumpProcessHeaps()
{
} 

PERFMETERTAG WINAPI _DbgExMtRegister(__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, DWORD dwFlags)
{
    return(0);
}

void WINAPI _DbgExMtAdd(PERFMETERTAG mt, LONG lCnt, LONG lVal)
{
}

void WINAPI _DbgExMtSet(PERFMETERTAG mt, LONG lCnt, LONG lVal)
{
}

char * WINAPI _DbgExMtGetName(PERFMETERTAG mt)
{
    return("");
}

char * WINAPI _DbgExMtGetDesc(PERFMETERTAG mt)
{
    return("");
}

PERFMETERTAG WINAPI _DbgExMtGetParent(PERFMETERTAG mt)
{
    return NULL;
}

DWORD WINAPI _DbgExMtGetFlags(PERFMETERTAG mt)
{
    return 0;
}

void WINAPI _DbgExMtSetFlags(PERFMETERTAG mt, DWORD dwFlags)
{
    return;
}

BOOL WINAPI _DbgExMtSimulateOutOfMemory(PERFMETERTAG mt, LONG lNewValue)
{
    return(0);
}

void WINAPI _DbgExMtOpenMonitor()
{
}

void WINAPI _DbgExMtLogDump(__in PCSTR pchFile)
{
}

PERFMETERTAG WINAPI _DbgExMtLookupMeter(__in PCSTR szTag)
{
    return 0;
}

long WINAPI _DbgExMtGetMeterCnt(PERFMETERTAG mt, BOOL fExclusive)
{
    return 0;
}

long WINAPI _DbgExMtGetMeterVal(PERFMETERTAG mt, BOOL fExclusive)
{
    return 0;
}

PERFMETERTAG WINAPI _DbgExMtGetDefaultMeter()
{
    return NULL;
}

PERFMETERTAG WINAPI _DbgExMtSetDefaultMeter(PERFMETERTAG mtDefault)
{
    return NULL;
}

void WINAPI _DbgExSetTopUrl(__in LPWSTR pstrUrl)
{
}

void WINAPI _DbgExGetSymbolFromAddress(void * pvAddr,  __out_ecount(cchBuf) char * pszBuf, DWORD cchBuf)
{
    pszBuf[0] = 0;
}

void WINAPI _DbgExGetStackAddresses(void ** ppvAddr, int iStart, int cTotal)
{
    memset( ppvAddr, 0, sizeof(void *) * cTotal );
}


#pragma warning(pop)    /* reenable disabled warnings */



BOOL WINAPI _DbgExGetChkStkFill(DWORD * pdwFill)
{
    *pdwFill = GetPrivateProfileIntA("chkstk", "fill", 0xCCCCCCCC, "avalndbg.ini");
    return(!GetPrivateProfileIntA("chkstk", "disable", FALSE, "avalndbg.ini"));
}

// cdecl function "wrappers" to their va_list equivalent ----------------------

BOOL __cdecl
DbgExTaggedTrace(TRACETAG tag, __in PCSTR szFmt, ...)
{
    va_list va;
    BOOL    f;

    va_start(va, szFmt);
    f = DbgExTaggedTraceListEx(tag, 0, szFmt, va);
    va_end(va);

    return f;
}

BOOL __cdecl
DbgExTaggedTraceEx(TRACETAG tag, USHORT usFlags, __in PCSTR szFmt, ...)
{
    va_list va;
    BOOL    f;

    va_start(va, szFmt);
    f = DbgExTaggedTraceListEx(tag, usFlags, szFmt, va);
    va_end(va);

    return f;
}

// InitDebugLib ---------------------------------------------------------------

#define DBGEXFUNCTIONS() \
    DBGEXWRAP (DWORD, DbgExGetVersion, (), ()) \
    DBGEXWRAP (BOOL, DbgExIsFullDebug, (), ()) \
    DBGEXWRAP_(void, DbgExAddRefDebugLibrary, (), ()) \
    DBGEXWRAP_(void, DbgExReleaseDebugLibrary, (), ()) \
    DBGEXWRAP_(void, DbgExSetDllMain, (HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID)), (hDllHandle, pfnDllMain)) \
    DBGEXWRAP_(void, DbgExDoTracePointsDialog, (BOOL fWait), (fWait)) \
    DBGEXWRAP_(void, DbgExRestoreDefaultDebugState, (), ()) \
    DBGEXWRAP (BOOL, DbgExEnableTag, (TRACETAG tag, BOOL fEnable), (tag, fEnable)) \
    DBGEXWRAP (BOOL, DbgExSetDiskFlag, (TRACETAG tag, BOOL fSendToDisk), (tag, fSendToDisk)) \
    DBGEXWRAP (BOOL, DbgExSetBreakFlag, (TRACETAG tag, BOOL fBreak), (tag, fBreak)) \
    DBGEXWRAP (BOOL, DbgExIsTagEnabled, (TRACETAG tag), (tag)) \
    DBGEXWRAP (TRACETAG, DbgExFindTag, (__in PCSTR szTagDesc), (szTagDesc)) \
    DBGEXWRAP (TRACETAG, DbgExTagError, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagWarning, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagThread, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagAssertExit, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagAssertStacks, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagMemoryStrict, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagCoMemoryStrict, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagMemoryStrictTail, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagMemoryStrictAlign, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagOLEWatch, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagRegisterTrace, (__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, BOOL fEnabled), (szTag, szOwner, szDescrip, fEnabled)) \
    DBGEXWRAP (BOOL, DbgExTaggedTraceListEx, (TRACETAG tag, USHORT usFlags, __in PCSTR szFmt, __in va_list valMarker), (tag, usFlags, szFmt, valMarker)) \
    DBGEXWRAP_(void, DbgExTaggedTraceCallers, (TRACETAG tag, int iStart, int cTotal), (tag, iStart, cTotal)) \
    DBGEXWRAP_(void, DbgExAssertThreadDisable, (BOOL fDisable), (fDisable)) \
    DBGEXWRAP (size_t, DbgExPreAlloc, (size_t cbRequest, PERFMETERTAG mt), (cbRequest, mt)) \
    DBGEXWRAP (void *, DbgExPostAlloc, (void *pv), (pv)) \
    DBGEXWRAP (void *, DbgExPreFree, (void *pv), (pv)) \
    DBGEXWRAP_(void, DbgExPostFree, (), ()) \
    DBGEXWRAP (size_t, DbgExPreRealloc, (void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt), (pvRequest, cbRequest, ppv, mt)) \
    DBGEXWRAP (void *, DbgExPostRealloc, (void *pv), (pv)) \
    DBGEXWRAP (void *, DbgExPreGetSize, (void *pvRequest), (pvRequest)) \
    DBGEXWRAP (size_t, DbgExPostGetSize, (size_t cb), (cb)) \
    DBGEXWRAP (size_t, DbgExMtPreAlloc, (size_t cbRequest, PERFMETERTAG mt), (cbRequest, mt)) \
    DBGEXWRAP (void *, DbgExMtPostAlloc, (void *pv), (pv)) \
    DBGEXWRAP (void *, DbgExMtPreFree, (void *pv), (pv)) \
    DBGEXWRAP_(void, DbgExMtPostFree, (), ()) \
    DBGEXWRAP (size_t, DbgExMtPreRealloc, (void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt), (pvRequest, cbRequest, ppv, mt)) \
    DBGEXWRAP (void *, DbgExMtPostRealloc, (void *pv), (pv)) \
    DBGEXWRAP (void *, DbgExMtPreGetSize, (void *pvRequest), (pvRequest)) \
    DBGEXWRAP (size_t, DbgExMtPostGetSize, (size_t cb), (cb)) \
    DBGEXWRAP_(void, DbgExMemoryTrackDisable, (BOOL fDisable), (fDisable)) \
    DBGEXWRAP_(void, DbgExCoMemoryTrackDisable, (BOOL fDisable), (fDisable)) \
    DBGEXWRAP_(void, DbgExMemoryBlockTrackDisable, (void * pv), (pv)) \
    DBGEXWRAP_(void, DbgExTraceMemoryLeaks, (), ()) \
    DBGEXWRAP (BOOL, DbgExValidateKnownAllocations, (), ()) \
    DBGEXWRAP (LONG_PTR, DbgExTraceFailL, (LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line), (errExpr, errTest, fIgnore, pstrExpr, pstrFile, line)) \
    DBGEXWRAP (LONG_PTR, DbgExTraceWin32L, (LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line), (errExpr, errTest, fIgnore, pstrExpr, pstrFile, line)) \
    DBGEXWRAP (HRESULT, DbgExTraceHR, (HRESULT hrTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line), (hrTest, fIgnore, pstrExpr, pstrFile, line)) \
    DBGEXWRAP_(void, DbgExSetSimFailCounts, (int firstFailure, int cInterval), (firstFailure, cInterval)) \
    DBGEXWRAP_(void, DbgExShowSimFailDlg, (), ()) \
    DBGEXWRAP (BOOL, DbgExFFail, (), ()) \
    DBGEXWRAP (int, DbgExGetFailCount, (), ()) \
    DBGEXWRAP_(void, DbgExOpenMemoryMonitor, (), ()) \
    DBGEXWRAP_(void, DbgExOpenLogFile, (LPCSTR szFName), (szFName)) \
    DBGEXWRAP_(void, DbgExDumpProcessHeaps, (), ()) \
    DBGEXWRAP(PERFMETERTAG, DbgExMtRegister, (__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, DWORD dwFlags), (szTag, szOwner, szDescrip, dwFlags)) \
    DBGEXWRAP_(void, DbgExMtAdd, (PERFMETERTAG mt, LONG lCnt, LONG lVal), (mt, lCnt, lVal)) \
    DBGEXWRAP_(void, DbgExMtSet, (PERFMETERTAG mt, LONG lCnt, LONG lVal), (mt, lCnt, lVal)) \
    DBGEXWRAP (char *, DbgExMtGetName, (PERFMETERTAG mt), (mt)) \
    DBGEXWRAP (char *, DbgExMtGetDesc, (PERFMETERTAG mt), (mt)) \
    DBGEXWRAP (PERFMETERTAG, DbgExMtGetParent, (PERFMETERTAG mt), (mt)) \
    DBGEXWRAP (DWORD, DbgExMtGetFlags, (PERFMETERTAG mt), (mt)) \
    DBGEXWRAP_(void, DbgExMtSetFlags, (PERFMETERTAG mt, DWORD dwFlags), (mt, dwFlags)) \
    DBGEXWRAP (BOOL, DbgExMtSimulateOutOfMemory, (PERFMETERTAG mt, LONG lNewValue), (mt, lNewValue)) \
    DBGEXWRAP_(void, DbgExMtOpenMonitor, (), ()) \
    DBGEXWRAP_(void, DbgExMtLogDump, (__in PCSTR pchFile), (pchFile)) \
    DBGEXWRAP (PERFMETERTAG, DbgExMtLookupMeter, (__in PCSTR szTag), (szTag)) \
    DBGEXWRAP (long, DbgExMtGetMeterCnt, (PERFMETERTAG mt, BOOL fExclusive), (mt, fExclusive)) \
    DBGEXWRAP (long, DbgExMtGetMeterVal, (PERFMETERTAG mt, BOOL fExclusive), (mt, fExclusive)) \
    DBGEXWRAP (PERFMETERTAG, DbgExMtGetDefaultMeter, (), ()) \
    DBGEXWRAP (PERFMETERTAG, DbgExMtSetDefaultMeter, (PERFMETERTAG mtDefault), (mtDefault)) \
    DBGEXWRAP_(void, DbgExGetStackAddresses, (void ** ppvAddr, int iStart, int cTotal), (ppvAddr, iStart, cTotal)) \
    DBGEXWRAP (BOOL, DbgExGetChkStkFill, (DWORD * pdwFill), (pdwFill)) \

#undef  DBGEXWRAP
#undef  DBGEXWRAP_
#define DBGEXWRAP(ret, fn, formals, params) ret (WINAPI * g_##fn) formals = _##fn;
#define DBGEXWRAP_(ret, fn, formals, params) ret (WINAPI * g_##fn) formals = _##fn;

#pragma prefast (suppress: __WARNING_ENCODE_GLOBAL_FUNCTION_POINTER)
DBGEXFUNCTIONS()

#undef  DBGEXWRAP
#undef  DBGEXWRAP_
#define DBGEXWRAP(ret, fn, formals, params) ret WINAPI fn formals { return(g_##fn params); }
#define DBGEXWRAP_(ret, fn, formals, params) ret WINAPI fn formals { g_##fn params; }

DBGEXFUNCTIONS()

BOOL InitDebugProcedure(void ** ppv, __in PSTR pchFn)
{
    *ppv = (void *)GetProcAddress(g_hInstDbg, pchFn);

    if (*ppv == NULL)
    {
        char ach[512];
        StringCchVPrintfA(ach, ARRAY_SIZE(ach), "InitDebugLib: Can't find PresentationDebug.dll entrypoint %s\r\n", pchFn);
        OutputDebugStringA(ach);
        return(FALSE);
    }

    return(TRUE);
}

void InitDebugStubs()
{
    #undef  DBGEXWRAP
    #undef  DBGEXWRAP_
    #define DBGEXWRAP(ret, fn, formals, params) g_##fn = _##fn;
    #define DBGEXWRAP_(ret, fn, formals, params) g_##fn = _##fn;

    DBGEXFUNCTIONS()
}

void InitDebugLib(
    __in_ecount_opt(1) HANDLE hDllHandle, 
    __in_ecount_opt(1) BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID), 
    BOOL fExe
    )
{
    UNREFERENCED_PARAMETER(hDllHandle);
    UNREFERENCED_PARAMETER(pfnDllMain);

    g_hInstDbg = LoadLibraryA("PresentationDebug.dll");

    if (g_hInstDbg == NULL)
    {
//        OutputDebugStringA("InitDebugLib: Can't find PresentationDebug.dll.  Only partial debug support available.\r\n");
        goto dostubs;
    }

    #undef  DBGEXWRAP
    #undef  DBGEXWRAP_
    #define DBGEXWRAP(ret, fn, formals, params) if (!InitDebugProcedure((void **)&g_##fn, #fn)) goto dostubs;
    #define DBGEXWRAP_(ret, fn, formals, params) if (!InitDebugProcedure((void **)&g_##fn, #fn)) goto dostubs;

    DBGEXFUNCTIONS()

    if (DbgExGetVersion() != AVALON_DEBUG_API_VERSION)
    {
        char ach[512];
        StringCchPrintfA(ach, ARRAY_SIZE(ach), "InitDebugLib: Version mismatch for PresentationDebug.DLL.  Expected %ld but found %ld.\r\n",
                  AVALON_DEBUG_API_VERSION, DbgExGetVersion());
        OutputDebugStringA(ach);
        FreeLibrary(g_hInstDbg);
        g_hInstDbg = NULL;
        goto dostubs;
    }
    else
    {
        if (!fExe)
            DbgExAddRefDebugLibrary();

        // 
        // This doesn't work on dll used by the URT because it holds a reference to our dll and the
        // callback is no longer safe to do.
        //
        // DbgExSetDllMain(hDllHandle, pfnDllMain);
    }

    return;

dostubs:

    InitDebugStubs();
}

void TermDebugLib(__in_ecount(1) HANDLE hDllHandle, BOOL fFinal)
{
    UNREFERENCED_PARAMETER(hDllHandle);

    if (g_hInstDbg == NULL)
        return;

    if (fFinal)
    {
        DbgExReleaseDebugLibrary();
        FreeLibrary(g_hInstDbg);
        g_hInstDbg = NULL;
    }
    else
    {
        // 
        // This doesn't work on dll used by the URT because it holds a reference to our dll and the
        // callback is no longer safe to do.
        //
        // DbgExSetDllMain(hDllHandle, NULL);
    }
}

#ifdef __cplusplus
    }
#endif




