//+-------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*///
// File:      MSXMLDBG.H
//
// Contains:  Debugging stuff for use in Forms^3
//            See CORE\DEBUG\MSXMLDBG.TXT for more information.
//
//--------------------------------------------------------------------------

#ifndef _MSXMLDBG_H
#define _MSXMLDBG_H

#if !defined( EXPORT )
#ifdef WIN32
#define EXPORT
#else
#define EXPORT  __export
#endif  // WIN32
#endif  // !EXPORT


#ifdef __cplusplus      // BUGBUG: Is extern "C" really needed here?
extern "C"
{
#endif

//--------------------------------------------------------------------------
// Assert, Verify && WHEN_DBG
//--------------------------------------------------------------------------

#if DBG != 1

#define Verify(x)   x
#define Assert(x)
#define AssertSz(x, sz)
#define IF_DBG(x)
#define WHEN_DBG(x)
#define IF_NOT_DBG(x) x
#define WHEN_NOT_DBG(x) x
#define StartupAssert(x)
#define AssertThreadDisable(fb)

#else // #if DBG != 1

#if defined(_M_IX86)
    #define F3DebugBreak() _asm { int 3 }
#else
    #define F3DebugBreak() DebugBreak()
#endif

BOOL EXPORT WINAPI AssertImpl(char const * szFile, int iLine, char const * szMessage);
void EXPORT WINAPI AssertThreadDisable(BOOL fDisable);

extern DWORD g_dwFALSE;

#define Verify(x)       Assert(x)
#define Assert(x)       do { if (!((DWORD_PTR)(x)|g_dwFALSE) && AssertImpl(__FILE__, __LINE__, #x))\
                               F3DebugBreak(); } while (g_dwFALSE)

#define AssertSz(x, sz) do { if (!((DWORD_PTR)(x)|g_dwFALSE) && AssertImpl(__FILE__, __LINE__, sz))\
                               F3DebugBreak(); } while (g_dwFALSE)

#define AssertLocSz(x, File, Line, sz)   do { if (!((DWORD_PTR)(x)|g_dwFALSE) && AssertImpl(File, Line, sz))\
                               F3DebugBreak(); } while (g_dwFALSE)
#define IF_DBG(x) x
#define WHEN_DBG(x) x
#define IF_NOT_DBG(x)
#define WHEN_NOT_DBG(x)

//
// Startup assertion:
// The assertion is called by initializing a global variable with
// a function that performs the assertion and returns 1. The name
// of the global variable and function name are suffixed with the
// line number to make them unique. Unfortunatly, one cannot just
// write StartupAssert_##__LINE__, because __LINE__ is not an
// argument to the macro and so the expansion is, e.g. StartupAssert__##53.
// So we indirect through another macro which concatenates its
// two arguments.
//

#define concat_name(x, y) x##y
#define concat_line_impl(x, y) concat_name(x, y)
#define concat_LINE(x) concat_line_impl(x, __LINE__)

#define StartupAssert(x)                                                    \
static int                                                                  \
concat_LINE(StartupAssert_) ()                                              \
{                                                                           \
    Assert(x);                                                              \
    return 1;                                                               \
}                                                                           \
                                                                            \
static int concat_LINE(g_StartupAssert_) = concat_LINE(StartupAssert_)()    \

#endif // #if DBG != 1

//--------------------------------------------------------------------------
// Trace Tags
//--------------------------------------------------------------------------

typedef int TAG;

#if DBG != 1
    #define TraceTag(x)
    #define TraceTagEx(x)
    #define TaggedTraceListEx(tag, usFlags, szFmt, valMarker)
    #define TraceCallers(tag, iStart, cTotal)
    #define DeclareTag(tag, szOwner, szDescription)
    #define DeclareTagOther(tag, szOwner, szDescription)
    #define IsTagEnabled(tag) FALSE
    #define EnableTag(tag, fEnable)
    #define SetDiskFlag(tag, fSendToDisk)
    #define SetBreakFlag(tag, fBreak)
    #define FindTag(szTagDesc) NULL
    #define PerfDbgTag(tag, szOwner, szDescrip) \
            PerfTag(tag, szOwner, szDescrip)
    #define PerfDbgExtern(tag) \
            PerfExtern(tag)
    #define PerfDbgLog(tag,pv,f) PerfLog(tag,pv,f)
    #define PerfDbgLog1(tag,pv,f,a1) PerfLog1(tag,pv,f,a1)
    #define PerfDbgLog2(tag,pv,f,a1,a2) PerfLog2(tag,pv,f,a1,a2)
    #define PerfDbgLog3(tag,pv,f,a1,a2,a3) PerfLog3(tag,pv,f,a1,a2,a3)
    #define PerfDbgLog4(tag,pv,f,a1,a2,a3,a4) PerfLog4(tag,pv,f,a1,a2,a3,a4)
    #define PerfDbgLog5(tag,pv,f,a1,a2,a3,a4,a5) PerfLog5(tag,pv,f,a1,a2,a3,a4,a5)
    #define PerfDbgLog6(tag,pv,f,a1,a2,a3,a4,a5,a6) PerfLog6(tag,pv,f,a1,a2,a3,a4,a5,a6)
    #define PerfDbgLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) PerfLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
    #define PerfDbgLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) PerfLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
    #define PerfDbgLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) PerfLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
    #define PerfDbgLogN(x) PerfLogFn x
    #define IsPerfDbgEnabled(tag) IsPerfEnabled(tag)

#else
    #define TraceTag(x)                         \
        do                                      \
        {                                       \
            if (TaggedTrace x)                  \
                F3DebugBreak();                 \
        } while  (g_dwFALSE)                   

    #define TraceTagEx(x)                       \
        do                                      \
        {                                       \
            if (TaggedTraceEx x)                \
                F3DebugBreak();                 \
        } while  (g_dwFALSE)                    

    #define TraceCallers(tag, iStart, cTotal)   \
        TaggedTraceCallers(tag, iStart, cTotal)

    #define DeclareTag(tag, szOwner, szDescrip) \
        TAG tag(TagRegisterTrace(szOwner, szDescrip));

    #define DeclareTagOther(tag, szOwner, szDescrip) \
        TAG tag(TagRegisterOther(szOwner, szDescrip));

    #define PerfDbgTag(tag, szOwner, szDescrip) \
            DeclareTag(tag, szOwner, szDescrip)
    #define PerfDbgExtern(tag) \
            extern TAG tag;
    #define PerfDbgLog(tag,pv,f) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f)
    #define PerfDbgLog1(tag,pv,f,a1) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1)
    #define PerfDbgLog2(tag,pv,f,a1,a2) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2)
    #define PerfDbgLog3(tag,pv,f,a1,a2,a3) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3)
    #define PerfDbgLog4(tag,pv,f,a1,a2,a3,a4) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4)
    #define PerfDbgLog5(tag,pv,f,a1,a2,a3,a4,a5) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5)
    #define PerfDbgLog6(tag,pv,f,a1,a2,a3,a4,a5,a6) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6)
    #define PerfDbgLog7(tag,pv,f,a1,a2,a3,a4,a5,a6,a7) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7)
    #define PerfDbgLog8(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8)
    #define PerfDbgLog9(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9) if (IsPerfDbgEnabled(tag)) PerfDbgLogFn(tag,pv,f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
    #define PerfDbgLogN(x) PerfDbgLogFn x
    #define IsPerfDbgEnabled(tag) IsTagEnabled(tag)

    // Tag trace functions

    BOOL EXPORT CDECL TaggedTrace(TAG tag, CHAR * szFmt, ...);
    BOOL EXPORT CDECL TaggedTraceEx(TAG tag, USHORT usFlags, CHAR * szFmt, ...);
    BOOL EXPORT WINAPI TaggedTraceListEx(TAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker);
    void __cdecl PerfDbgLogFn(TAG tag, void * pvObj, CHAR * szFmt, ...);
    void EXPORT WINAPI TaggedTraceCallers(TAG tag, int iStart, int cTotal);

    // TaggedTraceEx usFlags parameter defines

    #define TAG_NONAME      0x01
    #define TAG_NONEWLINE   0x02
    #define TAG_USECONSOLE  0x04
    #define TAG_INDENT      0x08
    #define TAG_OUTDENT     0x10

    // Register a new tag.

    TAG EXPORT WINAPI TagRegisterTrace(
            CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled = FALSE);

    TAG EXPORT WINAPI TagRegisterOther(
            CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled = FALSE);

    // Standard tags
    #define tagError                TagError()
    #define tagWarning              TagWarning()
    #define tagThread               TagThread()
    #define tagAssertExit           TagAssertExit()
    #define tagAssertPop            TagAssertPop()
    #define tagMemoryStrict         TagMemoryStrict()
    #define tagMemoryStrictAlign    TagMemoryStrictAlign()
    #define tagOLEWatch             TagOLEWatch()
    #define tagPerf                 TagPerf()

    TAG EXPORT WINAPI TagError( void );
    TAG EXPORT WINAPI TagWarning( void );
    TAG EXPORT WINAPI TagThread( void );
    TAG EXPORT WINAPI TagAssertExit(void);
    TAG EXPORT WINAPI TagAssertPop(void);
    TAG EXPORT WINAPI TagMemoryStrict(void);
    TAG EXPORT WINAPI TagMemoryStrictAlign(void);
    TAG EXPORT WINAPI TagOLEWatch(void);
    TAG EXPORT WINAPI TagPerf(void);

    // Get/Set tag enabled status.

    BOOL EXPORT WINAPI IsTagEnabled(TAG tag);
    BOOL EXPORT WINAPI EnableTag(TAG tag, BOOL fEnable);
    BOOL EXPORT WINAPI SetDiskFlag(TAG tag, BOOL fSendToDisk);
    BOOL EXPORT WINAPI SetBreakFlag(TAG tag, BOOL fBreak);

    TAG  EXPORT WINAPI FindTag(char * szTagDesc);

#endif

//+---------------------------------------------------------------------
//  Interface tracing.
//----------------------------------------------------------------------

#if DBG == 1 && !defined(WIN16)
    void DbgTrackItf(REFIID iid, char *pch, BOOL fTrackOnQI, void **ppv);
#else
    #define DbgTrackItf(iid, pch, fTrackOnQi, ppv)
#endif

//--------------------------------------------------------------------------
// Miscelleanous
//--------------------------------------------------------------------------

#if DBG==1
    void EXPORT WINAPI DoTracePointsDialog(BOOL fWait);
    void EXPORT WINAPI RestoreDefaultDebugState(void);
    BOOL EXPORT WINAPI IsFullDebug(void);
#endif

//--------------------------------------------------------------------------
// Failure testing
//--------------------------------------------------------------------------

#if DBG == 1 && defined(__cplusplus)

void    EXPORT WINAPI SetSimFailCounts(int firstFailure, int cInterval);
void    EXPORT WINAPI ShowSimFailDlg(void);

int     EXPORT WINAPI GetFailCount();
long    EXPORT WINAPI TraceFailL( long errTest, long errExpr, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
long    EXPORT WINAPI TraceWin32L(long errTest, long errExpr, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
HRESULT EXPORT WINAPI TraceHR(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
HRESULT EXPORT WINAPI TraceOLE(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line, LPVOID lpsite);
void    EXPORT WINAPI TraceEnter(LPSTR pstrExpr, LPSTR pstrFile, int line);
void    EXPORT WINAPI TraceExit(LPSTR pstrExpr, LPSTR pstrFile, int line);

}       // extern "C" close

template <class t> inline t
TraceFail(t errExpr, int errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return (t) TraceFailL((long) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

template <class t> inline t
TraceWin32(t errExpr, int errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return (t) TraceWin32L((long) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

extern "C" {

#define TFAIL(e, x)             (TraceEnter(#x, __FILE__, __LINE__), TraceFail( (x), (e), FALSE, #x, __FILE__, __LINE__))
#define TW32(e, x)              (TraceEnter(#x, __FILE__, __LINE__), TraceWin32((x), (e), FALSE, #x, __FILE__, __LINE__))
#define THR(x)                  (TraceEnter(#x, __FILE__, __LINE__), TraceHR((x), FALSE, #x, __FILE__, __LINE__))

#define TFAIL_NOTRACE(e, x)     (x)
#define TW32_NOTRACE(e, x)      (x)
#define THR_NOTRACE(x)          (x)

#define IGNORE_FAIL(e, x)       (TraceEnter(#x, __FILE__, __LINE__), (void) TraceFail((x), (e), TRUE, #x, __FILE__, __LINE__))
#define IGNORE_W32(e,x)         (TraceEnter(#x, __FILE__, __LINE__), (void) TraceWin32((x), (e), TRUE, #x, __FILE__, __LINE__))
#define IGNORE_HR(x)            (TraceEnter(#x, __FILE__, __LINE__), (void) TraceHR((x), TRUE, #x, __FILE__, __LINE__))

#else // #if DBG == 1

#define SetSimFailCounts(firstFailure, cInterval)
#define ShowSimFailDlg()

#define TFAIL(e, x)             (x)
#define TW32(e, x)              (x)
#define THR(x)                  (x)

#define TFAIL_NOTRACE(e, x)     (x)
#define TW32_NOTRACE(e, x)      (x)
#define THR_NOTRACE(x)          (x)

#define IGNORE_FAIL(e, x)       (x)
#define IGNORE_W32(e,x)         (x)
#define IGNORE_HR(x)            (x)

#endif // #if DBG == 1

//+-------------------------------------------------------------------------
//  Return tracing
//--------------------------------------------------------------------------

#if DBG == 1


        HRESULT EXPORT CDECL
            CheckAndReturnResult(
                HRESULT hr,
                BOOL    fTrace,
                LPSTR   lpstrFile,
                UINT    line,
                int     cSuccess,
                ...);

    #define SRETURN(hr) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, -1)
    #define RRETURN(hr) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 0)
    #define RRETURN1(hr, s1) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 1, (s1))
    #define RRETURN2(hr, s1, s2) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 2, (s1), (s2))
    #define RRETURN3(hr, s1, s2, s3) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 3, (s1), (s2), (s3))
    #define RRETURN4(hr, s1, s2, s3, s4) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 4, (s1), (s2), (s3), (s4))

    #define SRETURN_NOTRACE(hr) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, -1)
    #define RRETURN_NOTRACE(hr) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 0)
    #define RRETURN1_NOTRACE(hr, s1) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 1, (s1))
    #define RRETURN2_NOTRACE(hr, s1, s2) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 2, (s1), (s2))
    #define RRETURN3_NOTRACE(hr, s1, s2, s3) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 3, (s1), (s2), (s3))
    #define RRETURN4_NOTRACE(hr, s1, s2, s3, s4) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 4, (s1), (s2), (s3), (s4))

#else   // DBG == 0

    #define SRETURN(hr)                 return (hr)
    #define RRETURN(hr)                 return (hr)
    #define RRETURN1(hr, s1)            return (hr)
    #define RRETURN2(hr, s1, s2)        return (hr)
    #define RRETURN3(hr, s1, s2, s3)    return (hr)
    #define RRETURN4(hr, s1, s2, s3, s4)return (hr)

    #define SRETURN_NOTRACE(hr)                 return (hr)
    #define RRETURN_NOTRACE(hr)                 return (hr)
    #define RRETURN1_NOTRACE(hr, s1)            return (hr)
    #define RRETURN2_NOTRACE(hr, s1, s2)        return (hr)
    #define RRETURN3_NOTRACE(hr, s1, s2, s3)    return (hr)
    #define RRETURN4_NOTRACE(hr, s1, s2, s3, s4)return (hr)

#endif  // DBG

void EXPORT WINAPI OpenViewObjectMonitor(HWND hwndOwner, IUnknown *pUnk, BOOL fUseFrameSize);
void EXPORT WINAPI OpenMemoryMonitor();


void EXPORT WINAPI OpenLogFile(LPCSTR szFName);

//
// Working Set Hooks
//

HRESULT EXPORT WINAPI WsClear(HANDLE hProcess);
HRESULT EXPORT WINAPI WsTakeSnapshot(HANDLE hProcess);
BSTR EXPORT WINAPI WsGetModule(LONG_PTR row);
BSTR EXPORT WINAPI WsGetSection(LONG_PTR row);
LONG_PTR EXPORT WINAPI WsSize(LONG_PTR row);
LONG_PTR EXPORT WINAPI WsCount();
LONG_PTR EXPORT WINAPI WsTotal();
HRESULT EXPORT WINAPI WsStartDelta(HANDLE hProcess);
LONG_PTR EXPORT WINAPI WsEndDelta(HANDLE hProcess);

#ifdef __cplusplus
}
#endif

#endif //_MSXMLDBG_H

