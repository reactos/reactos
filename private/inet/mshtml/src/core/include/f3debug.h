//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
// File:      F3DEBUG.H
//
// Contains:  Debugging stuff for use in Forms^3
//            See CORE\DEBUG\F3DEBUG.TXT for more information.
//
//--------------------------------------------------------------------------

#ifndef I_F3DEBUG_H_
#define I_F3DEBUG_H_
#pragma INCMSG("--- Beg 'f3debug.h'")

#ifndef X_MSHTMDBG_H_
#define X_MSHTMDBG_H_
#pragma INCMSG("--- Beg <mshtmdbg.h>")
#include <mshtmdbg.h>
#pragma INCMSG("--- End <mshtmdbg.h>")
#endif

//--------------------------------------------------------------------------
// Assert, Verify && WHEN_DBG
//--------------------------------------------------------------------------

#ifndef _MAC
#if defined(_M_IX86)
    #define F3DebugBreak() _asm { int 3 }
#else
    #define F3DebugBreak() DebugBreak()
#endif
#endif // _MAC

#if DBG != 1

#ifdef _MAC

#ifdef MACDEBUG

#define DebugBreak() Debugger()
#define DEBUGSTR(x)  DebugStr(x)
#define F3DebugBreak() DebugBreak()

#else // MACDEBUG

#define DebugBreak()
#define DEBUGSTR(x)
#define F3DebugBreak()

#endif // MACDEBUG

#endif // _MAC


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

struct THREADSTATE;

#define AssertThreadDisable(fDisable)   DbgExAssertThreadDisable(fDisable)

extern DWORD g_dwFALSE;

#define Verify(x)       Assert(x)
#define Assert(x)       do { if (!((DWORD_PTR)(x)|g_dwFALSE) && DbgExAssertImpl(__FILE__, __LINE__, #x))\
                               F3DebugBreak(); } while (g_dwFALSE)

#define AssertSz(x, sz) do { if (!((DWORD_PTR)(x)|g_dwFALSE) && DbgExAssertImpl(__FILE__, __LINE__, sz))\
                               F3DebugBreak(); } while (g_dwFALSE)

#define AssertLocSz(x, File, Line, sz)   do { if (!((DWORD_PTR)(x)|g_dwFALSE) && DbgExAssertImpl(File, Line, sz))\
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

#if DBG != 1
    #define TraceTag(x)
    #define TraceTagEx(x)
    #define TaggedTraceListEx(tag, usFlags, szFmt, valMarker)
    #define TraceCallers(tag, iStart, cTotal)
    #define DeclareTag(tag, szOwner, szDescription)
    #define DeclareTagOther(tag, szOwner, szDescription)
    #define ExternTag(tag)
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
        TRACETAG tag(DbgExTagRegisterTrace(szOwner, szDescrip, FALSE));
    #define DeclareTagOther(tag, szOwner, szDescrip) \
        TRACETAG tag(DbgExTagRegisterOther(szOwner, szDescrip, FALSE));
    #define ExternTag(tag) extern TRACETAG tag;
    #define PerfDbgTag(tag, szOwner, szDescrip) DeclareTag(tag, szOwner, szDescrip)
    #define PerfDbgExtern(tag) ExternTag(tag)
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

    int __cdecl PerfDbgLogFn(int tag, void * pvObj, char * pchFmt, ...);

    // Tag trace functions

    #define TaggedTrace         DbgExTaggedTrace
    #define TaggedTraceEx       DbgExTaggedTraceEx
    #define TaggedTraceListEx   DbgExTaggedTraceListEx
    #define TaggedTraceCallers  DbgExTaggedTraceCallers

    // TaggedTraceEx usFlags parameter defines

    #define TAG_NONAME      0x01
    #define TAG_NONEWLINE   0x02
    #define TAG_USECONSOLE  0x04
    #define TAG_INDENT      0x08
    #define TAG_OUTDENT     0x10

    // Register a new tag.

    // Standard tags
    #define tagError                DbgExTagError()
    #define tagWarning              DbgExTagWarning()
    #define tagThread               DbgExTagThread()
    #define tagAssertExit           DbgExTagAssertExit()
    #define tagAssertStacks         DbgExTagAssertStacks()
    #define tagMemoryStrict         DbgExTagMemoryStrict()
    #define tagCoMemoryStrict       DbgExTagCoMemoryStrict()
    #define tagMemoryStrictTail     DbgExTagMemoryStrictTail()
    #define tagMemoryStrictAlign    DbgExTagMemoryStrictAlign()
    #define tagOLEWatch             DbgExTagOLEWatch()
    #define tagPerf                 DbgExTagPerf()

    // Get/Set tag enabled status.

    #define IsTagEnabled            DbgExIsTagEnabled
    #define EnableTag               DbgExEnableTag
    #define SetDiskFlag             DbgExSetDiskFlag
    #define SetBreakFlag            DbgExSetBreakFlag
    #define FindTag                 DbgExFindTag

#endif

//--------------------------------------------------------------------------
// Memory Allocation
//--------------------------------------------------------------------------

#if DBG != 1

    #define DbgPreAlloc(cb)             cb
    #define DbgPostAlloc(pv)            pv
    #define DbgPreFree(pv)              pv
    #define DbgPostFree()
    #define DbgPreRealloc(pv, cb, ppv)  cb
    #define DbgPostRealloc(pv)          pv
    #define DbgPreGetSize(pv)           pv
    #define DbgPostGetSize(cb)          cb
    #define DbgPreDidAlloc(pv)          pv
    #define DbgPostDidAlloc(pv, fAct)   fAct

    #define DbgMemoryTrackDisable(fb)
    #define DbgCoMemoryTrackDisable(fb)
    #define DbgMemoryBlockTrackDisable(pv)

    #define CHECK_HEAP()

#else

    #define DbgPreAlloc                 DbgExPreAlloc
    #define DbgPostAlloc                DbgExPostAlloc
    #define DbgPreFree                  DbgExPreFree
    #define DbgPostFree                 DbgExPostFree
    #define DbgPreRealloc               DbgExPreRealloc
    #define DbgPostRealloc              DbgExPostRealloc
    #define DbgPreGetSize               DbgExPreGetSize
    #define DbgPostGetSize              DbgExPostGetSize
    #define DbgPreDidAlloc              DbgExPreDidAlloc
    #define DbgPostDidAlloc             DbgExPostDidAlloc

    #define DbgMemoryTrackDisable       DbgExMemoryTrackDisable
    #define DbgCoMemoryTrackDisable     DbgExCoMemoryTrackDisable
    #define DbgMemoryBlockTrackDisable  DbgExMemoryBlockTrackDisable

    //
    // Use the CHECK_HEAP macro to do thorough heap validation.
    //
    BOOL CheckSmallBlockHeap();
    void WINAPI DbgExCheckHeap();
    #define CHECK_HEAP()                DbgExCheckHeap();

#endif


//+---------------------------------------------------------------------
//  Interface tracing.
//----------------------------------------------------------------------

#if DBG == 1 && !defined(WIN16)
    #define DbgTrackItf     DbgExTrackItf
#else
    #define DbgTrackItf(iid, pch, fTrackOnQi, ppv)
#endif

//--------------------------------------------------------------------------
// Failure testing
//--------------------------------------------------------------------------

#if DBG == 1 && defined(__cplusplus)

#define SetSimFailCounts    DbgExSetSimFailCounts
#define GetFailCount        DbgExGetFailCount
#define TraceFailL          DbgExTraceFailL
#define TraceWin32L         DbgExTraceWin32L
#define TraceHR             DbgExTraceHR
#define TraceOLE            DbgExTraceOLE
#define TraceEnter          DbgExTraceEnter
#define TraceExit           DbgExTraceExit

template <class t> inline t
TraceFail(t errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return (t) DbgExTraceFailL((LONG_PTR) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

template <class t> inline t
TraceWin32(t errExpr, LONG_PTR errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return (t) DbgExTraceWin32L((LONG_PTR) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

// disabled TraceEnter, we don't need it at this time
#undef TraceEnter
#define TraceEnter(x, y, z) NULL

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

    #define SRETURN(hr) \
        return DbgExCheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, -1)
    #define RRETURN(hr) \
        return DbgExCheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 0)
    #define RRETURN1(hr, s1) \
        return DbgExCheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 1, (s1))
    #define RRETURN2(hr, s1, s2) \
        return DbgExCheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 2, (s1), (s2))
    #define RRETURN3(hr, s1, s2, s3) \
        return DbgExCheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 3, (s1), (s2), (s3))
    #define RRETURN4(hr, s1, s2, s3, s4) \
        return DbgExCheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 4, (s1), (s2), (s3), (s4))

    #define SRETURN_NOTRACE(hr) \
        return DbgExCheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, -1)
    #define RRETURN_NOTRACE(hr) \
        return DbgExCheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 0)
    #define RRETURN1_NOTRACE(hr, s1) \
        return DbgExCheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 1, (s1))
    #define RRETURN2_NOTRACE(hr, s1, s2) \
        return DbgExCheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 2, (s1), (s2))
    #define RRETURN3_NOTRACE(hr, s1, s2, s3) \
        return DbgExCheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 3, (s1), (s2), (s3))
    #define RRETURN4_NOTRACE(hr, s1, s2, s3, s4) \
        return DbgExCheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 4, (s1), (s2), (s3), (s4))

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

//+-------------------------------------------------------------------------
//  Stack Spew
//--------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

#pragma INCMSG("--- End 'f3debug.h'")
#else
#pragma INCMSG("*** Dup 'f3debug.h'")
#endif
