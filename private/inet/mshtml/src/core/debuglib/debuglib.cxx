//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       debuglib.c
//
//  Contents:   Interface to debugging .dll (if available)
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <w4warn.h>
#include <limits.h>
#include <mshtmdbg.h>

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
    { "Perf",       "Perf and size killers",                    FALSE   },  // 11: tagPerf
    { "Peer",       "Provide test host behaviors",              FALSE   },  // 12: tagHostInfoBehaviors
    { "FALSE",      "FALSE",                                    FALSE   },  // 13: tagFALSE
};

#define TAG_NONAME              0x01
#define TAG_NONEWLINE           0x02
#define ARRAY_SIZE(x)           (sizeof(x) / sizeof(x[0]))

#define tagDefault              ((TRACETAG)0)
#define tagError                ((TRACETAG)1)
#define tagWarning              ((TRACETAG)2)
#define tagThread               ((TRACETAG)3)
#define tagAssertExit           ((TRACETAG)4)
#define tagAssertStacks         ((TRACETAG)5)
#define tagMemoryStrict         ((TRACETAG)6)
#define tagCoMemoryStrict       ((TRACETAG)7)
#define tagMemoryStrictTail     ((TRACETAG)8)
#define tagMemoryStrictAlign    ((TRACETAG)9)
#define tagOLEWatch             ((TRACETAG)10)
#define tagPerf                 ((TRACETAG)11)
#define tagHostInfoBehavior     ((TRACETAG)12)
#define tagFALSE                ((TRACETAG)13)

HINSTANCE g_hInstDbg = NULL;
HINSTANCE g_hInstLeak = NULL;

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

void LeakDumpAppend(char * pszMsg, void * pvArg = NULL)
{
    HANDLE hFile;
    char ach[1024];
    DWORD dw;

    lstrcpyA(ach, GetModuleName(g_hInstLeak));
    lstrcatA(ach, ": ");
    wsprintfA(&ach[lstrlenA(ach)], pszMsg, pvArg);

    hFile = CreateFileA("c:\\leakdump.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        WriteFile(hFile, ach, lstrlenA(ach), &dw, NULL);
        WriteFile(hFile, "\r\n", 2, &dw, NULL);
        CloseHandle(hFile);
    }
}

DWORD WINAPI _DbgExGetVersion()
{
    return(MSHTMDBG_API_VERSION);
}

BOOL WINAPI _DbgExIsFullDebug()
{
    return(FALSE);
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

TRACETAG WINAPI _DbgExFindTag(char * szTagDesc)
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

    return(tagFALSE);
}

TRACETAG WINAPI _DbgExTagError()
{
    return(tagError);
}

TRACETAG WINAPI _DbgExTagWarning()
{
    return(tagWarning);
}

TRACETAG WINAPI _DbgExTagThread()
{
    return(tagThread);
}

TRACETAG WINAPI _DbgExTagAssertExit()
{
    return(tagAssertExit);
}

TRACETAG WINAPI _DbgExTagAssertStacks()
{
    return(tagAssertStacks);
}

TRACETAG WINAPI _DbgExTagMemoryStrict()
{
    return(tagMemoryStrict);
}

TRACETAG WINAPI _DbgExTagCoMemoryStrict()
{
    return(tagCoMemoryStrict);
}

TRACETAG WINAPI _DbgExTagMemoryStrictTail()
{
    return(tagMemoryStrictTail);
}

TRACETAG WINAPI _DbgExTagMemoryStrictAlign()
{
    return(tagMemoryStrictAlign);
}

TRACETAG WINAPI _DbgExTagOLEWatch()
{
    return(tagOLEWatch);
}

TRACETAG WINAPI _DbgExTagPerf()
{
    return(tagPerf);
}

TRACETAG WINAPI _DbgExTagRegisterTrace(CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled)
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

    return(tagFALSE);
}

TRACETAG WINAPI _DbgExTagRegisterOther(CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled)
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

    return(tagFALSE);
}

BOOL WINAPI _DbgExTaggedTraceListEx(TRACETAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker)
{
    if (DbgExIsTagEnabled(tag))
    {
        CHAR    achDup[512], *pch;
        CHAR    achBuf[1024];
        LONG    cch = 0;

        lstrcpynA(achDup, szFmt, ARRAY_SIZE(achDup));

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
            strcpy(achBuf, "MSHTML: ");
            cch += 8;
        }

        cch += wvsprintfA(&achBuf[cch], szFmt, valMarker);

        if (!(usFlags & TAG_NONEWLINE))
        {
            strcpy(&achBuf[cch], "\r\n");
        }

        OutputDebugStringA(achBuf);
    }

    return(FALSE);
}

void WINAPI _DbgExTaggedTraceCallers(TRACETAG tag, int iStart, int cTotal)
{
}

BOOL WINAPI _DbgExAssertImpl(char const * szFile, int iLine, char const * szMessage)
{
    CHAR szBuf[4096];
    wsprintfA(szBuf, "MSHTML: Assertion Failure in file %s, line %ld:\r\nMSHTML:   %s\r\n",
              szFile ? szFile : "unknown", iLine, szMessage);
    OutputDebugStringA(szBuf);
    return TRUE;
}

void WINAPI _DbgExAssertThreadDisable(BOOL fDisable)
{
}

HRESULT WINAPI _DbgExCheckAndReturnResultList(HRESULT hr, BOOL fTrace, LPSTR pstrFile, UINT line, int cHResult, va_list valMarker)
{
    return(hr);
}

size_t WINAPI _DbgExPreAlloc(size_t cbRequest)
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

size_t WINAPI _DbgExPreRealloc(void *pvRequest, size_t cbRequest, void **ppv)
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

void * WINAPI _DbgExPreDidAlloc(void *pvRequest)
{
    return(pvRequest);
}

BOOL WINAPI _DbgExPostDidAlloc(void *pvRequest, BOOL fActual)
{
    return(fActual);
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

void WINAPI _DbgExMemSetHeader(void * pvRequest, size_t cb, PERFMETERTAG mt)
{
}

void * WINAPI _DbgExGetMallocSpy()
{
    return(NULL);
}

void WINAPI _DbgExTraceMemoryLeaks()
{
}

BOOL WINAPI _DbgExValidateInternalHeap()
{
    return(TRUE);
}

LONG_PTR WINAPI _DbgExTraceFailL(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return(errExpr);
}

LONG_PTR WINAPI _DbgExTraceWin32L(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return(errExpr);
}

HRESULT WINAPI _DbgExTraceHR(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return(hrTest);
}

HRESULT WINAPI _DbgExTraceOLE(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line, LPVOID lpsite)
{
    return(hrTest);
}

void WINAPI _DbgExTraceEnter(LPSTR pstrExpr, LPSTR pstrFile, int line)
{
}

void WINAPI _DbgExTraceExit(LPSTR pstrExpr, LPSTR pstrFile, int line)
{
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

void WINAPI _DbgExTrackItf(REFIID iid, char * pch, BOOL fTrackOnQI, void **ppv)
{
}

void WINAPI _DbgExOpenViewObjectMonitor(HWND hwndOwner, IUnknown *pUnk, BOOL fUseFrameSize)
{
}

void WINAPI _DbgExOpenMemoryMonitor()
{
}

void WINAPI _DbgExOpenLogFile(LPCSTR szFName)
{
}

void * WINAPI _DbgExMemSetNameList(void * pvRequest, char * szFmt, va_list valMarker)
{
    return(pvRequest);
}

char * WINAPI _DbgExMemGetName(void *pvRequest)
{
    return("");
}

HRESULT WINAPI _DbgExWsClear(HANDLE hProcess)
{
    return(S_OK);
}

HRESULT WINAPI _DbgExWsTakeSnapshot(HANDLE hProcess)
{
    return(S_OK);
}

BSTR WINAPI _DbgExWsGetModule(long row)
{
    return(NULL);
}

BSTR WINAPI _DbgExWsGetSection(long row)
{
    return(NULL);
}

long WINAPI _DbgExWsSize(long row)
{
    return(0);
}

long WINAPI _DbgExWsCount()
{
    return(0);
}

long WINAPI _DbgExWsTotal()
{
    return(0);
}

HRESULT WINAPI _DbgExWsStartDelta(HANDLE hProcess)
{
    return(S_OK);
}

long WINAPI _DbgExWsEndDelta(HANDLE hProcess)
{
    return(-1);
}

void WINAPI _DbgExDumpProcessHeaps()
{
}

PERFTAG WINAPI _DbgExPerfRegister(char * szTag, char * szOwner, char * szDescrip)
{
    return(0);
}

void WINAPI _DbgExPerfLogFnList(PERFTAG tag, void * pvObj, const char * pchFmt, va_list valMarker)
{
}

void WINAPI _DbgExPerfDump()
{
}

void WINAPI _DbgExPerfClear()
{
}

void WINAPI _DbgExPerfTags()
{
}

char * WINAPI _DbgExDecodeMessage(UINT msg)
{
    return("");
}

PERFMETERTAG WINAPI _DbgExMtRegister(char * szTag, char * szOwner, char * szDescrip)
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

BOOL WINAPI _DbgExMtSimulateOutOfMemory(PERFMETERTAG mt, LONG lNewValue)
{
    return(0);
}

void WINAPI _DbgExMtOpenMonitor()
{
}

void WINAPI _DbgExMtLogDump(LPSTR pchFile)
{
}

void WINAPI _DbgExSetTopUrl(LPWSTR pstrUrl)
{
}

void WINAPI _DbgExGetSymbolFromAddress(void * pvAddr, char * pszBuf, DWORD cchBuf)
{
    pszBuf[0] = 0;
}

BOOL WINAPI _DbgExGetChkStkFill(DWORD * pdwFill)
{
    *pdwFill = GetPrivateProfileIntA("chkstk", "fill", 0xCCCCCCCC, "mshtmdbg.ini");
    return(!GetPrivateProfileIntA("chkstk", "disable", FALSE, "mshtmdbg.ini"));
}

// cdecl function "wrappers" to their va_list equivalent ----------------------

BOOL __cdecl
DbgExTaggedTrace(TRACETAG tag, CHAR * szFmt, ...)
{
    va_list va;
    BOOL    f;

    va_start(va, szFmt);
    f = DbgExTaggedTraceListEx(tag, 0, szFmt, va);
    va_end(va);

    return f;
}

BOOL __cdecl
DbgExTaggedTraceEx(TRACETAG tag, USHORT usFlags, CHAR * szFmt, ...)
{
    va_list va;
    BOOL    f;

    va_start(va, szFmt);
    f = DbgExTaggedTraceListEx(tag, usFlags, szFmt, va);
    va_end(va);

    return f;
}

HRESULT __cdecl
DbgExCheckAndReturnResult(HRESULT hr, BOOL fTrace, LPSTR pstrFile, UINT line, int cHResult, ...)
{
    va_list va;
    HRESULT hrResult;

    va_start(va, cHResult);
    hrResult = DbgExCheckAndReturnResultList(hr, fTrace, pstrFile, line, cHResult, va);
    va_end(va);

    return(hrResult);
}

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

void __cdecl
DbgExPerfLogFn(PERFTAG tag, void * pvObj, const char * pchFmt, ...)
{
    va_list va;

    va_start(va, pchFmt);
    DbgExPerfLogFnList(tag, pvObj, pchFmt, va);
    va_end(va);
}

// InitDebugLib ---------------------------------------------------------------

#define DBGEXFUNCTIONS() \
    DBGEXWRAP (DWORD, DbgExGetVersion, (), ()) \
    DBGEXWRAP (BOOL, DbgExIsFullDebug, (), ()) \
    DBGEXWRAP_(void, DbgExSetDllMain, (HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID)), (hDllHandle, pfnDllMain)) \
    DBGEXWRAP_(void, DbgExDoTracePointsDialog, (BOOL fWait), (fWait)) \
    DBGEXWRAP_(void, DbgExRestoreDefaultDebugState, (), ()) \
    DBGEXWRAP (BOOL, DbgExEnableTag, (TRACETAG tag, BOOL fEnable), (tag, fEnable)) \
    DBGEXWRAP (BOOL, DbgExSetDiskFlag, (TRACETAG tag, BOOL fSendToDisk), (tag, fSendToDisk)) \
    DBGEXWRAP (BOOL, DbgExSetBreakFlag, (TRACETAG tag, BOOL fBreak), (tag, fBreak)) \
    DBGEXWRAP (BOOL, DbgExIsTagEnabled, (TRACETAG tag), (tag)) \
    DBGEXWRAP (TRACETAG, DbgExFindTag, (char * szTagDesc), (szTagDesc)) \
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
    DBGEXWRAP (TRACETAG, DbgExTagPerf, (), ()) \
    DBGEXWRAP (TRACETAG, DbgExTagRegisterTrace, (CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled), (szOwner, szDescrip, fEnabled)) \
    DBGEXWRAP (TRACETAG, DbgExTagRegisterOther, (CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled), (szOwner, szDescrip, fEnabled)) \
    DBGEXWRAP (BOOL, DbgExTaggedTraceListEx, (TRACETAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker), (tag, usFlags, szFmt, valMarker)) \
    DBGEXWRAP_(void, DbgExTaggedTraceCallers, (TRACETAG tag, int iStart, int cTotal), (tag, iStart, cTotal)) \
    DBGEXWRAP (BOOL, DbgExAssertImpl, (char const * szFile, int iLine, char const * szMessage), (szFile, iLine, szMessage)) \
    DBGEXWRAP_(void, DbgExAssertThreadDisable, (BOOL fDisable), (fDisable)) \
    DBGEXWRAP (HRESULT, DbgExCheckAndReturnResultList, (HRESULT hr, BOOL fTrace, LPSTR pstrFile, UINT line, int cHResult, va_list valMarker), (hr, fTrace, pstrFile, line, cHResult, valMarker)) \
    DBGEXWRAP (size_t, DbgExPreAlloc, (size_t cbRequest), (cbRequest)) \
    DBGEXWRAP (void *, DbgExPostAlloc, (void *pv), (pv)) \
    DBGEXWRAP (void *, DbgExPreFree, (void *pv), (pv)) \
    DBGEXWRAP_(void, DbgExPostFree, (), ()) \
    DBGEXWRAP (size_t, DbgExPreRealloc, (void *pvRequest, size_t cbRequest, void **ppv), (pvRequest, cbRequest, ppv)) \
    DBGEXWRAP (void *, DbgExPostRealloc, (void *pv), (pv)) \
    DBGEXWRAP (void *, DbgExPreGetSize, (void *pvRequest), (pvRequest)) \
    DBGEXWRAP (size_t, DbgExPostGetSize, (size_t cb), (cb)) \
    DBGEXWRAP (void *, DbgExPreDidAlloc, (void *pvRequest), (pvRequest)) \
    DBGEXWRAP (BOOL, DbgExPostDidAlloc, (void *pvRequest, BOOL fActual), (pvRequest, fActual)) \
    DBGEXWRAP_(void, DbgExMemoryTrackDisable, (BOOL fDisable), (fDisable)) \
    DBGEXWRAP_(void, DbgExCoMemoryTrackDisable, (BOOL fDisable), (fDisable)) \
    DBGEXWRAP_(void, DbgExMemoryBlockTrackDisable, (void * pv), (pv)) \
    DBGEXWRAP_(void, DbgExMemSetHeader, (void * pvRequest, size_t cb, PERFMETERTAG mt), (pvRequest, cb, mt)) \
    DBGEXWRAP (void *, DbgExGetMallocSpy, (), ()) \
    DBGEXWRAP_(void, DbgExTraceMemoryLeaks, (), ()) \
    DBGEXWRAP (BOOL, DbgExValidateInternalHeap, (), ()) \
    DBGEXWRAP (LONG_PTR, DbgExTraceFailL, (LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line), (errExpr, errTest, fIgnore, pstrExpr, pstrFile, line)) \
    DBGEXWRAP (LONG_PTR, DbgExTraceWin32L, (LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line), (errExpr, errTest, fIgnore, pstrExpr, pstrFile, line)) \
    DBGEXWRAP (HRESULT, DbgExTraceHR, (HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line), (hrTest, fIgnore, pstrExpr, pstrFile, line)) \
    DBGEXWRAP (HRESULT, DbgExTraceOLE, (HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line, LPVOID lpsite), (hrTest, fIgnore, pstrExpr, pstrFile, line, lpsite)) \
    DBGEXWRAP_(void, DbgExTraceEnter, (LPSTR pstrExpr, LPSTR pstrFile, int line), (pstrExpr, pstrFile, line)) \
    DBGEXWRAP_(void, DbgExTraceExit, (LPSTR pstrExpr, LPSTR pstrFile, int line), (pstrExpr, pstrFile, line)) \
    DBGEXWRAP_(void, DbgExSetSimFailCounts, (int firstFailure, int cInterval), (firstFailure, cInterval)) \
    DBGEXWRAP_(void, DbgExShowSimFailDlg, (), ()) \
    DBGEXWRAP (BOOL, DbgExFFail, (), ()) \
    DBGEXWRAP (int, DbgExGetFailCount, (), ()) \
    DBGEXWRAP_(void, DbgExTrackItf, (REFIID iid, char * pch, BOOL fTrackOnQI, void **ppv), (iid, pch, fTrackOnQI, ppv)) \
    DBGEXWRAP_(void, DbgExOpenViewObjectMonitor, (HWND hwndOwner, IUnknown *pUnk, BOOL fUseFrameSize), (hwndOwner, pUnk, fUseFrameSize)) \
    DBGEXWRAP_(void, DbgExOpenMemoryMonitor, (), ()) \
    DBGEXWRAP_(void, DbgExOpenLogFile, (LPCSTR szFName), (szFName)) \
    DBGEXWRAP (void *, DbgExMemSetNameList, (void * pvRequest, char * szFmt, va_list valMarker), (pvRequest, szFmt, valMarker)) \
    DBGEXWRAP (char *, DbgExMemGetName, (void *pvRequest), (pvRequest)) \
    DBGEXWRAP (HRESULT, DbgExWsClear, (HANDLE hProcess), (hProcess)) \
    DBGEXWRAP (HRESULT, DbgExWsTakeSnapshot, (HANDLE hProcess), (hProcess)) \
    DBGEXWRAP (BSTR, DbgExWsGetModule, (long row), (row)) \
    DBGEXWRAP (BSTR, DbgExWsGetSection, (long row), (row)) \
    DBGEXWRAP (long, DbgExWsSize, (long row), (row)) \
    DBGEXWRAP (long, DbgExWsCount, (), ()) \
    DBGEXWRAP (long, DbgExWsTotal, (), ()) \
    DBGEXWRAP (HRESULT, DbgExWsStartDelta, (HANDLE hProcess), (hProcess)) \
    DBGEXWRAP (long, DbgExWsEndDelta, (HANDLE hProcess), (hProcess)) \
    DBGEXWRAP_(void, DbgExDumpProcessHeaps, (), ()) \
    DBGEXWRAP (PERFTAG, DbgExPerfRegister, (char * szTag, char * szOwner, char * szDescrip), (szTag, szOwner, szDescrip)) \
    DBGEXWRAP_(void, DbgExPerfLogFnList, (PERFTAG tag, void * pvObj, const char * pchFmt, va_list valMarker), (tag, pvObj, pchFmt, valMarker)) \
    DBGEXWRAP_(void, DbgExPerfDump, (), ()) \
    DBGEXWRAP_(void, DbgExPerfClear, (), ()) \
    DBGEXWRAP_(void, DbgExPerfTags, (), ()) \
    DBGEXWRAP (char *, DbgExDecodeMessage, (UINT msg), (msg)) \
    DBGEXWRAP(PERFMETERTAG, DbgExMtRegister, (char * szTag, char * szOwner, char * szDescrip), (szTag, szOwner, szDescrip)) \
    DBGEXWRAP_(void, DbgExMtAdd, (PERFMETERTAG mt, LONG lCnt, LONG lVal), (mt, lCnt, lVal)) \
    DBGEXWRAP_(void, DbgExMtSet, (PERFMETERTAG mt, LONG lCnt, LONG lVal), (mt, lCnt, lVal)) \
    DBGEXWRAP (char *, DbgExMtGetName, (PERFMETERTAG mt), (mt)) \
    DBGEXWRAP (char *, DbgExMtGetDesc, (PERFMETERTAG mt), (mt)) \
    DBGEXWRAP (BOOL, DbgExMtSimulateOutOfMemory, (PERFMETERTAG mt, LONG lNewValue), (mt, lNewValue)) \
    DBGEXWRAP_(void, DbgExMtOpenMonitor, (), ()) \
    DBGEXWRAP_(void, DbgExMtLogDump, (LPSTR pchFile), (pchFile)) \
    DBGEXWRAP_(void, DbgExSetTopUrl, (LPWSTR pstrUrl), (pstrUrl)) \
    DBGEXWRAP_(void, DbgExGetSymbolFromAddress, (void * pvAddr, char * pszBuf, DWORD cchBuf), (pvAddr, pszBuf, cchBuf)) \
    DBGEXWRAP (BOOL, DbgExGetChkStkFill, (DWORD * pdwFill), (pdwFill)) \

#undef  DBGEXWRAP
#undef  DBGEXWRAP_
#define DBGEXWRAP(ret, fn, formals, params) ret (WINAPI * g_##fn) formals = NULL;
#define DBGEXWRAP_(ret, fn, formals, params) ret (WINAPI * g_##fn) formals = NULL;

DBGEXFUNCTIONS()

#undef  DBGEXWRAP
#undef  DBGEXWRAP_
#define DBGEXWRAP(ret, fn, formals, params) ret WINAPI fn formals { return(g_##fn params); }
#define DBGEXWRAP_(ret, fn, formals, params) ret WINAPI fn formals { g_##fn params; }

DBGEXFUNCTIONS()

BOOL InitDebugProcedure(void ** ppv, char * pchFn)
{
    *ppv = (void *)GetProcAddress(g_hInstDbg, pchFn);

    if (*ppv == NULL)
    {
        char ach[512];
        wsprintfA(ach, "InitDebugLib: Can't find mshtmdbg.dll entrypoint %s\r\n", pchFn);
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

void InitDebugLib(HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID))
{
    g_hInstDbg = LoadLibraryA("mshtmdbg.dll");

    if (g_hInstDbg == NULL)
    {
        OutputDebugStringA("InitDebugLib: Can't find mshtmdbg.dll.  Only partial debug support available.\r\n");
        goto dostubs;
    }

    #undef  DBGEXWRAP
    #undef  DBGEXWRAP_
    #define DBGEXWRAP(ret, fn, formals, params) if (!InitDebugProcedure((void **)&g_##fn, #fn)) goto dostubs;
    #define DBGEXWRAP_(ret, fn, formals, params) if (!InitDebugProcedure((void **)&g_##fn, #fn)) goto dostubs;

    DBGEXFUNCTIONS()

    if (DbgExGetVersion() != MSHTMDBG_API_VERSION)
    {
        char ach[512];
        wsprintfA(ach, "InitDebugLib: Version mismatch for MSHTMDBG.DLL.  Expected %ld but found %ld.\r\n",
                  MSHTMDBG_API_VERSION, DbgExGetVersion());
        OutputDebugStringA(ach);
        FreeLibrary(g_hInstDbg);
        g_hInstDbg = NULL;
        goto dostubs;
    }
    else
    {
        DbgExSetDllMain(hDllHandle, pfnDllMain);
    }

    return;

dostubs:

    InitDebugStubs();
}

void TermDebugLib(HANDLE hDllHandle, BOOL fFinal)
{
    if (g_hInstDbg == NULL)
        return;

    if (fFinal)
    {
        FreeLibrary(g_hInstDbg);
        g_hInstDbg = NULL;
    }
    else
    {
        DbgExSetDllMain(hDllHandle, NULL);
    }
}

#ifdef __cplusplus
    }
#endif
