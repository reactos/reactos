//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       qcore.h
//
//  Contents:   External include file for mshtmdbg.dll
//
// ----------------------------------------------------------------------------

#ifndef _QCOREDBG_H_
#define _QCOREDBG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MSHTMDBG_API_VERSION    (4)     // Increment whenever API changes

typedef INT     TRACETAG;
typedef INT_PTR PERFTAG;
typedef INT_PTR PERFMETERTAG;

DWORD       WINAPI  DbgExGetVersion();
BOOL        WINAPI  DbgExIsFullDebug();
void        WINAPI  DbgExSetDllMain(HANDLE hDllHandle, BOOL (WINAPI *)(HANDLE, DWORD, LPVOID));

void        WINAPI  DbgExDoTracePointsDialog(BOOL fWait);
void        WINAPI  DbgExRestoreDefaultDebugState();

BOOL        WINAPI  DbgExEnableTag(TRACETAG tag, BOOL fEnable);
BOOL        WINAPI  DbgExSetDiskFlag(TRACETAG tag, BOOL fSendToDisk);
BOOL        WINAPI  DbgExSetBreakFlag(TRACETAG tag, BOOL fBreak);
BOOL        WINAPI  DbgExIsTagEnabled(TRACETAG tag);
TRACETAG    WINAPI  DbgExFindTag(char * szTagDesc);

TRACETAG    WINAPI  DbgExTagError();
TRACETAG    WINAPI  DbgExTagWarning();
TRACETAG    WINAPI  DbgExTagThread();
TRACETAG    WINAPI  DbgExTagAssertExit();
TRACETAG    WINAPI  DbgExTagAssertStacks();
TRACETAG    WINAPI  DbgExTagMemoryStrict();
TRACETAG    WINAPI  DbgExTagCoMemoryStrict();
TRACETAG    WINAPI  DbgExTagMemoryStrictTail();
TRACETAG    WINAPI  DbgExTagMemoryStrictAlign();
TRACETAG    WINAPI  DbgExTagOLEWatch();
TRACETAG    WINAPI  DbgExTagRegisterTrace(CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled);
TRACETAG    WINAPI  DbgExTagRegisterOther(CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled);
TRACETAG    WINAPI  DbgExTagPerf();

BOOL        __cdecl DbgExTaggedTrace(TRACETAG tag, CHAR * szFmt, ...);
BOOL        __cdecl DbgExTaggedTraceEx(TRACETAG tag, USHORT usFlags, CHAR * szFmt, ...);
BOOL        WINAPI  DbgExTaggedTraceListEx(TRACETAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker);
void        WINAPI  DbgExTaggedTraceCallers(TRACETAG tag, int iStart, int cTotal);

BOOL        WINAPI  DbgExAssertImpl(char const * szFile, int iLine, char const * szMessage);
void        WINAPI  DbgExAssertThreadDisable(BOOL fDisable);
HRESULT     __cdecl DbgExCheckAndReturnResult(HRESULT hr, BOOL fTrace, LPSTR pstrFile, UINT line, int cHResult, ...);
HRESULT     WINAPI  DbgExCheckAndReturnResultList(HRESULT hr, BOOL fTrace, LPSTR pstrFile, UINT line, int cHResult, va_list valMarker);

size_t      WINAPI  DbgExPreAlloc(size_t cbRequest);
void *      WINAPI  DbgExPostAlloc(void *pv);
void *      WINAPI  DbgExPreFree(void *pv);
void        WINAPI  DbgExPostFree();
size_t      WINAPI  DbgExPreRealloc(void *pvRequest, size_t cbRequest, void **ppv);
void *      WINAPI  DbgExPostRealloc(void *pv);
void *      WINAPI  DbgExPreGetSize(void *pvRequest);
size_t      WINAPI  DbgExPostGetSize(size_t cb);
void *      WINAPI  DbgExPreDidAlloc(void *pvRequest);
BOOL        WINAPI  DbgExPostDidAlloc(void *pvRequest, BOOL fActual);

void        WINAPI  DbgExMemoryTrackDisable(BOOL fDisable);
void        WINAPI  DbgExCoMemoryTrackDisable(BOOL fDisable);
void        WINAPI  DbgExMemoryBlockTrackDisable(void * pv);
void        WINAPI  DbgExMemSetHeader(void * pvRequest, size_t cb, PERFMETERTAG mt);
void *      WINAPI  DbgExGetMallocSpy();
void        WINAPI  DbgExTraceMemoryLeaks();
BOOL        WINAPI  DbgExValidateInternalHeap();

LONG_PTR    WINAPI  DbgExTraceFailL(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
LONG_PTR    WINAPI  DbgExTraceWin32L(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
HRESULT     WINAPI  DbgExTraceHR(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
HRESULT     WINAPI  DbgExTraceOLE(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line, LPVOID lpsite);
void        WINAPI  DbgExTraceEnter(LPSTR pstrExpr, LPSTR pstrFile, int line);
void        WINAPI  DbgExTraceExit(LPSTR pstrExpr, LPSTR pstrFile, int line);
void        WINAPI  DbgExSetSimFailCounts(int firstFailure, int cInterval);
void        WINAPI  DbgExShowSimFailDlg();
BOOL        WINAPI  DbgExFFail();
int         WINAPI  DbgExGetFailCount();
void        WINAPI  DbgExTrackItf(REFIID iid, char * pch, BOOL fTrackOnQI, void **ppv);

void        WINAPI  DbgExOpenViewObjectMonitor(HWND hwndOwner, IUnknown *pUnk, BOOL fUseFrameSize);
void        WINAPI  DbgExOpenMemoryMonitor();
void        WINAPI  DbgExOpenLogFile(LPCSTR szFName);

void *      __cdecl DbgExMemSetName(void *pvRequest, char * szFmt, ...);
void *      WINAPI  DbgExMemSetNameList(void * pvRequest, char * szFmt, va_list valMarker);
char *      WINAPI  DbgExMemGetName(void *pvRequest);

HRESULT     WINAPI  DbgExWsClear(HANDLE hProcess);
HRESULT     WINAPI  DbgExWsTakeSnapshot(HANDLE hProcess);
BSTR        WINAPI  DbgExWsGetModule(long row);
BSTR        WINAPI  DbgExWsGetSection(long row);
long        WINAPI  DbgExWsSize(long row);
long        WINAPI  DbgExWsCount();
long        WINAPI  DbgExWsTotal();
HRESULT     WINAPI  DbgExWsStartDelta(HANDLE hProcess);
long        WINAPI  DbgExWsEndDelta(HANDLE hProcess);

void        WINAPI  DbgExDumpProcessHeaps();

PERFTAG     WINAPI  DbgExPerfRegister(char * szTag, char * szOwner, char * szDescrip);
void        __cdecl DbgExPerfLogFn(PERFTAG tag, void * pvObj, const char * pchFmt, ...);
void        WINAPI  DbgExPerfLogFnList(PERFTAG tag, void * pvObj, const char * pchFmt, va_list valMarker);
void        WINAPI  DbgExPerfDump();
void        WINAPI  DbgExPerfClear();
void        WINAPI  DbgExPerfTags();

char *      WINAPI  DbgExDecodeMessage(UINT msg);

PERFMETERTAG WINAPI  DbgExMtRegister(char * szTag, char * szOwner, char * szDescrip);
void        WINAPI  DbgExMtAdd(PERFMETERTAG mt, LONG lCnt, LONG lVal);
void        WINAPI  DbgExMtSet(PERFMETERTAG mt, LONG lCnt, LONG lVal);
char *      WINAPI  DbgExMtGetName(PERFMETERTAG mt);
char *      WINAPI  DbgExMtGetDesc(PERFMETERTAG mt);
BOOL        WINAPI  DbgExMtSimulateOutOfMemory(PERFMETERTAG mt, LONG lNewValue);
void        WINAPI  DbgExMtOpenMonitor();
void        WINAPI  DbgExMtLogDump(LPSTR pchFile);

void        WINAPI  DbgExSetTopUrl(LPWSTR pstrUrl);
void        WINAPI  DbgExGetSymbolFromAddress(void * pvAddr, char * pszBuf, DWORD cchBuf);

BOOL        WINAPI  DbgExGetChkStkFill(DWORD * pdwFill);

#ifdef __cplusplus
}
#endif

// Performance Logging --------------------------------------------------------

#ifdef PERFTAGS

#define     IsPerfEnabled(tag) (*(BOOL *)tag)
#define     PerfTag(tag, szOwner, szDescrip) PERFTAG tag(DbgExPerfRegister(#tag, szOwner, szDescrip));
#define     PerfExtern(tag) extern PERFTAG tag;
#define     PerfLog(tag,pv,f) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f) : 0
#define     PerfLog1(tag,pv,f,a1) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1) : 0
#define     PerfLog2(tag,pv,f,a1,a2) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2) : 0
#define     PerfLog3(tag,pv,f,a1,a2,a3) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3) : 0
#define     PerfLog4(tag,pv,f,a1,a2,a3,a4) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3,a4) : 0
#define     PerfLog5(tag,pv,f,a1,a2,a3,a4,a5) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3,a4,a5) : 0
#define     PerfLog6(tag,pv,f,a1,a2,a3,a4,a5,a6) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6) : 0
#define     PerfLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) : 0
#define     PerfLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) : 0
#define     PerfLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) IsPerfEnabled(tag) ? DbgExPerfLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) : 0
#define     PerfDump()  DbgExPerfDump()
#define     PerfClear() DbgExPerfClear()
#define     PerfTags()  DbgExPerfTags()

#else

#define     IsPerfEnabled(tag) (FALSE)
#define     PerfTag(tag, szOwner, szDescrip)
#define     PerfExtern(tag)
#define     PerfLog(tag,pv,f)
#define     PerfLog1(tag,pv,f,a1)
#define     PerfLog2(tag,pv,f,a1,a2)
#define     PerfLog3(tag,pv,f,a1,a2,a3)
#define     PerfLog4(tag,pv,f,a1,a2,a3,a4)
#define     PerfLog5(tag,pv,f,a1,a2,a3,a4,a5)
#define     PerfLog6(tag,pv,f,a1,a2,a3,a4,a5,a6)
#define     PerfLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
#define     PerfLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
#define     PerfLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#define     PerfDump()
#define     PerfClear()
#define     PerfTags()

#endif

// Performance Metering -------------------------------------------------------

#ifdef PERFMETER

#define     Mt(x)                               g_mt##x
#define     MtExtern(tag)                       extern PERFMETERTAG g_mt##tag;
#define     MtDefine(tag, tagOwner, szDescrip)  PERFMETERTAG g_mt##tag(DbgExMtRegister("mt" #tag, "mt" #tagOwner, szDescrip));
#define     MtAdd(mt, lCnt, lVal)               DbgExMtAdd(mt, lCnt, lVal)
#define     MtSet(mt, lCnt, lVal)               DbgExMtSet(mt, lCnt, lVal)
#define     MtSimulateOutOfMemory(mt, lNewVal)  DbgExMtSimulateOutOfMemory(mt, lNewVal)
#define     MtOpenMonitor()                     DbgExMtOpenMonitor()

#else

#define     Mt(x)                               ((PERFMETERTAG)0)
#define     MtExtern(tag)
#define     MtDefine(tag, szOwner, szDescrip)
#define     MtAdd(mt, lCnt, lVal)
#define     MtSet(mt, lCnt, lVal)
#define     MtSimulateOutOfMemory(mt, lNewValue)
#define     MtOpenMonitor()

#endif

// MSHTML perf control --------------------------------------------------------

#define HTMPERFCTL_NAME     "#MSHTML#PERF#"

enum
{
    HTMPF_CALLBACK_ONLOAD   = 0x00000001,   // callback when topdoc loaded
    HTMPF_ENABLE_PROFILE    = 0x00000002,   // enable profiling
    HTMPF_ENABLE_MEMWATCH   = 0x00000004,   // enable memwatch sampling
    HTMPF_DISABLE_PADEVENTS = 0x00000008,   // disable firing of events from mshtmpad.exe
    HTMPF_DISABLE_IMGCACHE  = 0x00000010,   // disable image cache in mshtml
    HTMPF_DISABLE_OFFSCREEN = 0x00000020,   // disable offscreen buffering
    HTMPF_DISABLE_ALERTS    = 0x00000040,   // disable alert() and confirm() methods
};

typedef void (WINAPI *HTMPFCBFN)(DWORD dwArg1, void * pvArg2);

typedef struct HTMPERFCTL
{
    DWORD       dwSize;     // set to sizeof(MSHTMLPERF)
    DWORD       dwFlags;    // see HTMPF_*
    HTMPFCBFN   pfnCall;    // Callback function
    void *      pvHost;     // Private data for host
} HTMPERFCTL;

#endif
