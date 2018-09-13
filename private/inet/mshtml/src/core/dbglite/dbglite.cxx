//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       dbglite.c
//
//  Contents:   functions for debug lite version of mshtml.dll
//
//----------------------------------------------------------------------------

#if DBG == 1

#include "headers.hxx"

#ifdef tagError
#undef tagError
#endif

#ifdef tagAssertExit
#undef tagAssertExit
#endif

#define tagNull     ((TAG) 0)

//
//  Global variables
//
TAG     tagDefault                  = TRUE;
TAG     tagError                    = TRUE;
TAG     tagWarn_                    = FALSE;
TAG     tagAssertExit               = FALSE;
TAG     tagThrd_                    = FALSE;
TAG     tagMemoryStrict_            = FALSE;
TAG     tagMemoryStrictTail_        = FALSE;
TAG     tagMemoryStrictAlign_       = FALSE;
TAG     tagOLEWatchvar_             = FALSE;
TAG     tagPerf_                    = FALSE;

BOOL    g_fBreakOnAssert            = TRUE;


//+------------------------------------------------------------------------
//
//  Function:   AssertImpl
//
//  Synopsis:   Function called for all asserts.  Outputs szMessage to
//              standart output
//
//  Arguments:
//              szFile
//              iLine
//              szMessage
//
//-------------------------------------------------------------------------

BOOL
AssertImpl(char const * szFile, int iLine, char const * szMessage)
{
    OutputDebugStringA("MSHTML: ");
    OutputDebugStringA(szMessage);
    OutputDebugStringA("\n");
    return g_fBreakOnAssert;
}


BOOL
IsFullDebug(void)
{
    return FALSE;
}

void AssertThreadDisable(BOOL fDisable)
{
}

void TaggedTraceCallers(TAG tag, int iStart, int cTotal)
{
}

TAG FindTag(char * szTagDesc)
{
    return 0;
}

size_t DbgPreAlloc(size_t cbRequest)
{
    return cbRequest;
}


void * DbgPostAlloc(void *pv)
{
    return pv;
}

void * DbgPreFree(void *pv)
{
    return pv;
}

void DbgPostFree(void)
{
}

size_t DbgPreRealloc(void *pvRequest, size_t cbRequest, void **ppv)
{
    *ppv = pvRequest;
    return cbRequest;
}

void * DbgPostRealloc(void *pv)
{
    return pv;
}

void * DbgPreGetSize(void *pvRequest)
{
    return pvRequest;
}

size_t DbgPostGetSize(size_t cb)
{
    return cb;
}

void * DbgPreDidAlloc(void *pvRequest)
{
    return pvRequest;
}

BOOL DbgPostDidAlloc(void *pvRequest, BOOL fActual)
{
    return fActual;
}

void DbgRegisterMallocSpy(void)
{
}

void DbgRevokeMallocSpy(void)
{
}

void DbgMemoryTrackDisable(BOOL fDisable)
{
}

void DbgMemoryBlockTrackDisable(void * pv)
{
}

void TraceMemoryLeaks()
{
}

BOOL ValidateInternalHeap()
{
    return TRUE;
}

int GetTotalAllocated(void)
{
    return 0;
}

void DbgDumpProcessHeaps()
{
}

void * __cdecl DbgMemSetName(void *pvRequest, char * szFmt, ...)
{
    return pvRequest;
}

char * DbgMemGetName(void *pvRequest)
{
    return NULL;
}

void
DbgSetTopUrl(LPWSTR pstrUrl)
{
}

void SetSimFailCounts(int firstFailure, int cInterval)
{
}

void ShowSimFailDlg(void)
{
}

TAG TagError( void )
{
    return tagError;
}

TAG TagAssertExit( void )
{
    return tagAssertExit;
}

TAG TagWarning( void )
{
    return tagWarn_;
}

TAG TagThread( void )
{
    return tagThrd_;
}

TAG TagMemoryStrict( void )
{
    return tagMemoryStrict_;
}

TAG TagMemoryStrictTail( void )
{
    return tagMemoryStrictTail_;
}

TAG TagMemoryStrictAlign( void )
{
    return tagMemoryStrictAlign_;
}

TAG TagOLEWatch( void )
{
    return tagOLEWatchvar_;
}

TAG TagPerf( void )
{
    return tagPerf_;
}

BOOL IsTagEnabled(TAG tag)
{
    return  tag;
}

BOOL SetDiskFlag (TAG tag, BOOL fSendToDisk)
{
    return FALSE;
}

BOOL SetBreakFlag (TAG tag, BOOL fBreak)
{
    return FALSE;
}

BOOL EnableTag( TAG tag, BOOL fEnable )
{
    return FALSE;
}

#undef TraceEnter
void TraceEnter(LPSTR pstrExpr, LPSTR pstrFile, int line)
{
}

void TraceExit(LPSTR pstrExpr, LPSTR pstrFile, int line)
{
}

BOOL __cdecl TaggedTrace(TAG tag, CHAR * szFmt, ...)
{
    BOOL    f;

    va_list valMarker;

    va_start(valMarker, szFmt);
    f = TaggedTraceListEx(tag, 0, szFmt, valMarker);
    va_end(valMarker);

    return f;
}

BOOL __cdecl TaggedTraceEx(TAG tag, USHORT usFlags, CHAR * szFmt, ...)
{
    BOOL    f;

    va_list valMarker;

    va_start(valMarker, szFmt);
    f = TaggedTraceListEx(tag, 0, szFmt, valMarker);
    va_end(valMarker);

    return f;
}

BOOL EXPORT WINAPI TaggedTraceListEx(TAG tag,
                                     USHORT usFlags,
                                     CHAR * szFmt,
                                     va_list valMarker)
{
    int         cch;
    CHAR        achBuf[4096];

    if (tag)
    {
        cch = wsprintfA(
                achBuf,
                szFmt,
                valMarker);

    OutputDebugStringA("MSHTML trace: ");
    OutputDebugStringA(achBuf);
    OutputDebugStringA("\n");
    }

    return FALSE;
}

TAG TagRegisterTrace( CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled )
{
    return tagNull;
}

TAG TagRegisterOther( CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled )
{
    return tagNull;
}

#ifndef WIN16
void DbgTrackItf(REFIID iid, char * pch, BOOL fTrackOnQI, void **ppv)
{
}
#endif

void DoTracePointsDialog( BOOL fWait )
{
}

void RestoreDefaultDebugState( void )
{
}

int GetFailCount( )
{
    return INT_MIN;
}

extern "C" long TraceFailL(long errExpr,
                           long errTest,
                           BOOL fIgnore,
                           LPSTR pstrExpr,
                           LPSTR pstrFile,
                           int line)
{
    return errExpr;
}

extern "C" long TraceWin32L(long errExpr,
                            long errTest,
                            BOOL fIgnore,
                            LPSTR pstrExpr,
                            LPSTR pstrFile,
                            int line)
{
    return errExpr;
}

extern "C" HRESULT TraceHR(HRESULT hrTest,
                           BOOL fIgnore,
                           LPSTR pstrExpr,
                           LPSTR pstrFile,
                           int line)
{
    return hrTest;
}

extern "C" HRESULT TraceOLE(HRESULT hrTest,
                            BOOL fIgnore,
                            LPSTR pstrExpr,
                            LPSTR pstrFile,
                            int line,
                            LPVOID lpsite)
{
    return hrTest;
}

HRESULT EXPORT CDECL CheckAndReturnResult(HRESULT hr,
                                        BOOL    fTrace,
                                        LPSTR   pstrFile,
                                        UINT    line,
                                        int     cHResult,
                                        ...)
{
    return hr;
}

void OpenViewObjectMonitor(HWND hwndOwner, IUnknown *pUnk, BOOL fUseFrameSize)
{
}

void OpenMemoryMonitor()
{
}

void OpenLogFile(LPCSTR szFName)
{
}

HRESULT WsClear(HANDLE hProcess)
{
    return S_OK;
}

HRESULT WsTakeSnapshot(HANDLE hProcess)
{
    return S_OK;
}

BSTR WsGetModule(long row)
{
    return NULL;
}

BSTR WsGetSection(long row)
{
    return NULL;
}

long WsSize(long row)
{
    return 0;
}

long WsCount()
{
    return 0;
}

long WsTotal()
{
    return 0;
}

HRESULT WsStartDelta(HANDLE hProcess)
{
    return S_OK;
}

long WsEndDelta(HANDLE hProcess)
{
    return -1;
}

void WINAPI InitStackSpew(BOOL * pfEnabled, DWORD * pdwSpew)
{
    Assert( pfEnabled && pdwSpew );
    pfEnabled = FALSE;
}

#endif  //  DBG == 1