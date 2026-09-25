// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       AvalonDebugP.h
//------------------------------------------------------------------------------


#pragma once
#ifdef _MANAGED
#pragma unmanaged
#endif


#if defined(_PREFIX_) || defined(_PREFAST_)
    #define ANALYSIS    1
    #define ANALYSIS_PARAM(x)        x
    #define ANALYSIS_COMMA_PARAM(x)  , x
    #define ANALYSIS_PARAM_COMMA(x)  x,
#else
    #define ANALYSIS_PARAM(x)
    #define ANALYSIS_COMMA_PARAM(x)
    #define ANALYSIS_PARAM_COMMA(x)
#endif

#if ANALYSIS || DBG
    #define DBG_ANALYSIS    1
    #define DBG_ANALYSIS_PARAM(x)        x
    #define DBG_ANALYSIS_COMMA_PARAM(x)  , x
    #define DBG_ANALYSIS_PARAM_COMMA(x)  x,
    #define WHEN_DBG_ANALYSIS(x)         x
#else
    #define DBG_ANALYSIS_PARAM(x)
    #define DBG_ANALYSIS_COMMA_PARAM(x)
    #define DBG_ANALYSIS_PARAM_COMMA(x)
    #define WHEN_DBG_ANALYSIS(x)
#endif
    

//
// UNCONDITIONAL_EXPR suppresses warning 4127
//   C4127: conditional expression is constant
// Use when you know that the test may be unconditional but that is okay as in
// templates with conditional behavior.  For templates do consider __if_exists
// with a trait map as an alternative.
//
// Note: PREfast doesn't like comma operator when left expression doesn't have
//       a side-effect - see defects 319 and 5430.  So, for PREfast just use
//       the expression as is.
//
#if !defined(_PREFAST_)
    #define UNCONDITIONAL_EXPR(Exp) (0,Exp)
#else
    #define UNCONDITIONAL_EXPR(Exp) (Exp)
#endif


// DebugDLL interface ---------------------------------------------------------
struct IUnknown;
typedef INT     TRACETAG;
typedef INT_PTR PERFTAG;
typedef INT_PTR PERFMETERTAG;


#ifdef __cplusplus
extern "C" {
#endif

#define AVALON_DEBUG_API_VERSION    (2)     // Increment whenever API changes

// Debug DLL management --------------------------------------------------------
DWORD       WINAPI  DbgExGetVersion();
BOOL        WINAPI  DbgExIsFullDebug();
void        WINAPI  DbgExAddRefDebugLibrary();
void        WINAPI  DbgExReleaseDebugLibrary();

// DllMain registration (for DLL reference leak tracking) ----------------------
void        WINAPI  DbgExSetDllMain(HANDLE hDllHandle, BOOL (WINAPI *)(HANDLE, DWORD, LPVOID));

// Trace Tag support -----------------------------------------------------------
void        WINAPI  DbgExOpenLogFile(LPCSTR szFName);

void        WINAPI  DbgExDoTracePointsDialog(BOOL fWait);
void        WINAPI  DbgExRestoreDefaultDebugState();

BOOL        WINAPI  DbgExEnableTag(TRACETAG tag, BOOL fEnable);
BOOL        WINAPI  DbgExSetDiskFlag(TRACETAG tag, BOOL fSendToDisk);
BOOL        WINAPI  DbgExSetBreakFlag(TRACETAG tag, BOOL fBreak);
BOOL        WINAPI  DbgExIsTagEnabled(TRACETAG tag);
TRACETAG    WINAPI  DbgExFindTag(__in PCSTR szTagDesc);

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
TRACETAG    WINAPI  DbgExTagRegisterTrace(__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, BOOL fEnabled); 

BOOL        __cdecl DbgExTaggedTrace(TRACETAG tag, __in PCSTR szFmt, ...);
BOOL        __cdecl DbgExTaggedTraceEx(TRACETAG tag, USHORT usFlags, __in PCSTR szFmt, ...);
BOOL        WINAPI  DbgExTaggedTraceListEx(TRACETAG tag, USHORT usFlags, __in PCSTR szFmt, __in va_list valMarker);
void        WINAPI  DbgExTaggedTraceCallers(TRACETAG tag, int iStart, int cTotal);

// Tracing --------------------------------------------------

LONG_PTR    WINAPI  DbgExTraceFailL(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line);
LONG_PTR    WINAPI  DbgExTraceWin32L(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line);
HRESULT     WINAPI  DbgExTraceHR(HRESULT hrTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line);
void        WINAPI  DbgExSetSimFailCounts(int firstFailure, int cInterval);
void        WINAPI  DbgExShowSimFailDlg();
BOOL        WINAPI  DbgExFFail();
int         WINAPI  DbgExGetFailCount();

// Assert support --------------------------------------------------------------
void        WINAPI  DbgExAssertThreadDisable(BOOL fDisable);

// Heap monitoring support -----------------------------------------------------
void        WINAPI  DbgExOpenMemoryMonitor();

size_t      WINAPI  DbgExPreAlloc(size_t cbRequest, PERFMETERTAG mt);
void *      WINAPI  DbgExPostAlloc(void *pv);
void *      WINAPI  DbgExPreFree(void *pv);
void        WINAPI  DbgExPostFree();
size_t      WINAPI  DbgExPreRealloc(__in void *pvRequest, size_t cbRequest, __deref_out void **ppv, PERFMETERTAG mt);
void *      WINAPI  DbgExPostRealloc(void *pv);
void *      WINAPI  DbgExPreGetSize(void *pvRequest);
size_t      WINAPI  DbgExPostGetSize(size_t cb);

size_t      WINAPI  DbgExMtPreAlloc(size_t cbRequest, PERFMETERTAG mt);
void *      WINAPI  DbgExMtPostAlloc(void *pv);
void *      WINAPI  DbgExMtPreFree(void *pv);
void        WINAPI  DbgExMtPostFree();
size_t      WINAPI  DbgExMtPreRealloc(void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt);
void *      WINAPI  DbgExMtPostRealloc(void *pv);
void *      WINAPI  DbgExMtPreGetSize(void *pvRequest);
size_t      WINAPI  DbgExMtPostGetSize(size_t cb);

void        WINAPI  DbgExMemoryTrackDisable(BOOL fDisable);
void        WINAPI  DbgExCoMemoryTrackDisable(BOOL fDisable);
void        WINAPI  DbgExMemoryBlockTrackDisable(void * pv);
void        WINAPI  DbgExTraceMemoryLeaks();
BOOL        WINAPI  DbgExValidateKnownAllocations();

void        WINAPI  DbgExDumpProcessHeaps();

// Metering support ------------------------------------------------------------
PERFMETERTAG WINAPI  DbgExMtRegister(__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, DWORD dwFlags);
void        WINAPI  DbgExMtAdd(PERFMETERTAG mt, LONG lCnt, LONG lVal);
void        WINAPI  DbgExMtSet(PERFMETERTAG mt, LONG lCnt, LONG lVal);
char *      WINAPI  DbgExMtGetName(PERFMETERTAG mt);
char *      WINAPI  DbgExMtGetDesc(PERFMETERTAG mt);
PERFMETERTAG WINAPI DbgExMtGetParent(PERFMETERTAG mt);
DWORD       WINAPI  DbgExMtGetFlags(PERFMETERTAG mt);
void        WINAPI  DbgExMtSetFlags(PERFMETERTAG mt, DWORD dwFlags);
BOOL        WINAPI  DbgExMtSimulateOutOfMemory(PERFMETERTAG mt, LONG lNewValue);
void        WINAPI  DbgExMtOpenMonitor();
void        WINAPI  DbgExMtLogDump(__in PCSTR pchFile);
PERFMETERTAG WINAPI DbgExMtLookupMeter(__in PCSTR szTag);
long        WINAPI  DbgExMtGetMeterCnt(PERFMETERTAG mt, BOOL fExclusive);
long        WINAPI  DbgExMtGetMeterVal(PERFMETERTAG mt, BOOL fExclusive);
PERFMETERTAG WINAPI DbgExMtGetDefaultMeter();
PERFMETERTAG WINAPI DbgExMtSetDefaultMeter(PERFMETERTAG mtDefault);

// Stack walking helpers -------------------------------------------------------
void        WINAPI  DbgExGetStackAddresses(void ** ppvAddr, int iStart, int cTotal);

// Stack spew fill function ----------------------------------------------------
BOOL        WINAPI  DbgExGetChkStkFill(DWORD * pdwFill);

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------
//------------------------ Performance Metering Macros -------------------------
//------------------------------------------------------------------------------
enum
{
    METER_NO_MEMALLOC   = 0x00000001,   // don't allow allocation on this meter
    METER_MT_VERIFIED   = 0x00000002,   // This tag has been verified as cool for alloc
};

#ifdef PERFMETER

class CSetDefaultMeter
{
public:
    CSetDefaultMeter(PERFMETERTAG mtDefault)
    {   mtDefaultOld = DbgExMtSetDefaultMeter(mtDefault);    }
    ~CSetDefaultMeter()
    {   DbgExMtSetDefaultMeter(mtDefaultOld);   }
private:
    PERFMETERTAG mtDefaultOld;
};

class CMemoryTrackDisable
{
public:
    CMemoryTrackDisable()
    { DbgExMemoryTrackDisable(TRUE); }
    ~CMemoryTrackDisable()
    { DbgExMemoryTrackDisable(FALSE); }
};

#define     Mt(x)                               g_mt##x
#define     MtWrap(mt)                          (mt)
#define     MtExtern(tag)                       extern PERFMETERTAG g_mt##tag;
#define     MtExport(exp, tag, tagOwner, szDescrip) PERFMETERTAG g_mt##exp(DbgExMtRegister("mt" #tag, "mt" #tagOwner, szDescrip, 0));
#define     MtDefine(tag, tagOwner, szDescrip)  PERFMETERTAG g_mt##tag(DbgExMtRegister("mt" #tag, "mt" #tagOwner, szDescrip, 0));
#define     MtExportF(exp, tag, tagOwner, szDescrip, dwFlags) PERFMETERTAG g_mt##exp(DbgExMtRegister("mt" #tag, "mt" #tagOwner, szDescrip, dwFlags));
#define     MtDefineF(tag, tagOwner, szDescrip, dwFlags)  PERFMETERTAG g_mt##tag(DbgExMtRegister("mt" #tag, "mt" #tagOwner, szDescrip, dwFlags));
#define     MtAdd(mt, lCnt, lVal)               DbgExMtAdd(mt, lCnt, lVal)
#define     MtSet(mt, lCnt, lVal)               DbgExMtSet(mt, lCnt, lVal)
#define     MtSimulateOutOfMemory(mt, lNewVal)  DbgExMtSimulateOutOfMemory(mt, lNewVal)
#define     MtOpenMonitor()                     DbgExMtOpenMonitor()
#define     MtSetDefault(mt)                    CSetDefaultMeter setdefaultmeter(mt)
#define     MtMemoryTrackDisable                CMemoryTrackDisable memoryTrackDisable
#define     MtBlockTrackDisable(pv)             DbgExMemoryBlockTrackDisable(pv)

#else

#define     Mt(x)                               (#x,0)
#define     MtWrap(mt)                          ((PERFMETERTAG)0)
#define     MtExtern(tag)
#define     MtExport(exp, tag, tagOwner, szDescrip)
#define     MtDefine(tag, szOwner, szDescrip)
#define     MtExportF(exp, tag, tagOwner, szDescrip, dwFlags)
#define     MtDefineF(tag, tagOwner, szDescrip, dwFlags)
#define     MtAdd(mt, lCnt, lVal)
#define     MtSet(mt, lCnt, lVal)
#define     MtSimulateOutOfMemory(mt, lNewValue)
#define     MtOpenMonitor()
#define     MtSetDefault(mt)
#define     MtMemoryTrackDisable
#define     MtBlockTrackDisable(pv)

#endif


BOOL IsKernelDebuggerEnabled();
BOOL IsKernelDebuggerPresent();


//------------------------------------------------------------------------------
//------------------------ Assert, Verify, and WHEN_DBG ------------------------
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// The following methods and macros should not be used directly.  Instead their
// CamelCased couterparts should be used.  See below for general Asserts/RIPs.
//

#if (NTDDI_VERSION >= NTDDI_WINXP)   

//
// Assert related procedures and macros that will work on XP RTM and above.
//

VOID
AssertA(
    __in_opt PCSTR Message,
    __in_opt PCWSTR FailedAssertion,
    __in PCWSTR Function,
    __in PCWSTR FileName,
    ULONG LineNumber
    );

VOID
AssertW(
    __in_opt PCWSTR Message,
    __in_opt PCWSTR FailedAssertion,
    __in PCWSTR Function,
    __in PCWSTR FileName,
    ULONG LineNumber
    );

// Macros to widen a string macro
#define __WIDEN2(x) L ## x
#define __WIDEN(x)  __WIDEN2(x)

#define __WFILE__ __WIDEN(__FILE__)
#define __WFUNCTION__ __WIDEN(__FUNCTION__)

#define FRERIPA(_msg) \
    FRERIPW(L##_msg)

#define FRERIPW(_msg) \
    (AssertW(_msg, NULL, __WFUNCTION__, __WFILE__, __LINE__))

#define FREASSERT(_exp) \
    ((!(_exp)) ? \
        (AssertW(NULL, L#_exp, __WFUNCTION__, __WFILE__, __LINE__),FALSE) : \
        TRUE)

#define FREASSERTMSGA(_msg, _exp) \
    ((!(_exp)) ? \
        (AssertA(_msg, L#_exp, __WFUNCTION__, __WFILE__, __LINE__),FALSE) : \
        TRUE)

#define FREASSERTMSGW(_msg, _exp) \
    ((!(_exp)) ? \
        (AssertW(_msg, L#_exp, __WFUNCTION__, __WFILE__, __LINE__),FALSE) : \
        TRUE)

#endif  // (NTDDI_VERSION >= NTDDI_WINXP)   


#if (NTDDI_VERSION >= NTDDI_VISTA)
#if !defined(NT_FREASSERT)

#define NT_FREASSERT(_exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L#_exp), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

#define NT_FREASSERTMSGA(_msg, _exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L##_msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

#define NT_FREASSERTMSGW(_msg, _exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", _msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)

#endif  // !defined(NT_FREASSERT)
#endif  // (NTDDI_VERSION >= NTDDI_VISTA)

//
//  End of internal assert macros and methods.
//-----------------------------------------------------------------------------


#if defined(_M_IX86)
    #define AvalonDebugBreak() _asm { int 3 }
#else
    #define AvalonDebugBreak() DebugBreak()
#endif


//-----------------------------------------------------------------------------
//
// Asserts and RIPs available for general use.
//
//   Fre prefix indicates it will be live in retail builds.  However expect to
//   have to remove all Fre asserts before product release.
//
//  RIPA(sz), FreRIPA(sz),
//  RIPW(sz), FreRIPW(sz)
//    - Always break when excuted.
//    - sz message is displayed.
//
//  Assert(x), FreAssert(x)
//    - Breaks when x evaluates to false.
//    - x is displayed.
//
//  AssertConstMsgA(x, sz), FreAssertConstMsgA(x, sz),
//  AssertConstMsgW(x, sz), FreAssertConstMsgW(x, sz)
//    - Breaks when x evaluates to false.
//    - sz message is displayed.  sz must be a string literal.
//    - Preferred over non-Const version as there is less impact to binary.
//
//  AssertMsgA(x, sz), FreAssertMsgA(x, sz),
//  AssertMsgW(x, sz), FreAssertMsgW(x, sz)
//    - Breaks when x evaluates to false.
//    - sz message is displayed.  sz may be generated/selected at runtime.
//
//  SetDbgPrintFilterID
//    - Controls DbgPrintEx ComponentID used for all non-Longhorn asserts' and
//      Longhorn non-Const AssertMsgs' debug output.
//
//-----------------------------------------------------------------------------


extern ULONG g_uDPFltrID;

inline void
SetDbgPrintFilterID(
    ULONG uDPFltrID
    )
{
    g_uDPFltrID = uDPFltrID;
}


// Note: For PREF* runs, we need to make the compiler believe that we're going
//       to exit upon a RIP or have it check/assume Asserted conditions. To do
//       this, we redefine the *RIP() and *Assert*() macros when _PREFIX_ or
//       _PREFAST_ is defined.
#if defined(ANALYSIS)

#define FreRIPA(sz)                 exit(1)
#define FreRIPW(sz)                 FreRIPA(sz)

// Since retail build expands out to a real assert, its safe to assume ?x? for the purposes of analysis
#define FreAssert(x)  __analysis_assume((x))

#define FreAssertConstMsgA(x, sz)   FreAssertMsgA(x, sz)
#define FreAssertConstMsgW(x, sz)   FreAssertMsgA(x, sz)
#define FreAssertMsgA(x, sz)        FreAssert(x)
#define FreAssertMsgW(x, sz)        FreAssert(x)

#define Verify(x)                   FreAssert(x)

#define RIPA(sz)                    FreRIPA(sz)
#define RIPW(sz)                    FreRIPW(sz)
//
//[pfx_parse]
//  Before changing Assert definition please contact owners (see codetags.txt)
// Assert is used under static analysis tools to help verify code correctness
// and avoid a number of false positives (see FreAssert above).
//
// If build errors are introduced by use of debug only variables or methods,
// please fix those instances or contact respective owner.
#define Assert(x)                   FreAssert(x)
#define AssertConstMsgA(x, sz)      FreAssertConstMsgA(x, sz)
#define AssertConstMsgW(x, sz)      FreAssertConstMsgW(x, sz)
#define AssertMsgA(x, sz)           FreAssertMsgA(x, sz)
#define AssertMsgW(x, sz)           FreAssertMsgW(x, sz)

#else

//
// We want to use NT_ASSERT style assertions because of the embedded support
// in the OS and debuggers, but this is only available in Longhorn.
//
// For the moment forcible disable using NT_ASSERT until the target version
// build macros can be properly set to indicate we aren't always targeting
// Longhorn.
#if defined(NT_FREASSERT) && 0

#define FreRIPA(sz)                 NT_FREASSERTMSGA(sz, FALSE)
#define FreRIPW(sz)                 NT_FREASSERTMSGW(sz, FALSE)

#define FreAssert(x)                NT_FREASSERT(x)

// FreAssertConstMsg uses NT_FREASSERTMSG, which places constant inline strings
// in the PDB file.  Strings passed to this macro cannot be generated at
// runtime.
#define FreAssertConstMsgA(x, sz)   NT_FREASSERTMSGA(sz, x)
#define FreAssertConstMsgW(x, sz)   NT_FREASSERTMSGW(sz, x)

// FreAssertMsg uses DbgPrint.  These methods can accept strings that are
// generated at runtime.
#define FreAssertMsgA(x, sz) \
    ((!(x)) ?                                               \
        (DbgPrintEx(g_uDPFltrID, DPFLTR_ERROR_LEVEL, sz),   \
         FreRIPW(L#x L"\nSee debug output for details."),   \
         FALSE) :                                           \
        TRUE)
#define FreAssertMsgW(x, sz) \
    ((!(x)) ?                                               \
        (DbgPrintEx(g_uDPFltrID, DPFLTR_ERROR_LEVEL, "%ls", sz), \
         FreRIPW(L#x L"\nSee debug output for details."),   \
         FALSE) :                                           \
        TRUE)

#else   // defined(NT_FREASSERT)

#define FreRIPA(sz)                 FRERIPA(sz)
// Use __annotation to enforce string literal, rather than runtime string,
// because NT version requires a string literal.  See NT_FREASSERTMSG
#define FreRIPW(sz)                 (__annotation(L"Debug", L"RIP", sz), \
                                     FRERIPW(sz))

#define FreAssert(x)                FREASSERT(x)

// FreAssertConstMsg wants to use NT_FREASSERTMSG, which places constant inline
// strings in the PDB file.  Strings passed to this macro cannot be generated
// at runtime.
#define FreAssertConstMsgA(x, sz)   (__annotation(L"Debug", L"AssertFail", L##sz), \
                                     FREASSERTMSGW(L##sz, x))
#define FreAssertConstMsgW(x, sz)   (__annotation(L"Debug", L"AssertFail", sz), \
                                     FREASSERTMSGW(sz, x))

// These methods can accept strings that are generated at runtime.
#define FreAssertMsgA(x, sz)        FREASSERTMSGA(sz, x)
#define FreAssertMsgW(x, sz)        FREASSERTMSGW(sz, x)

#endif  // defined(NT_FREASSERT)

#endif


// For message accepting versions that don't specify a character size assume
// ANSI.  (Referenced macros definitions are below.)
#define RIP(sz)                     RIPA(sz)
#define AssertConstMsg(x, sz)       AssertConstMsgA(x, sz)
#define AssertMsg(x, sz)            AssertMsgA(x, sz)


#if !DBG && !RETAILDEBUGLIB

#if !defined(ANALYSIS)

#define Verify(x)   ((x) ? 0 : 0)

#define RIPA(x)
#define RIPW(sz)
#define Assert(x)
#define AssertMsgA(x, sz)
#define AssertMsgW(x, sz)
#define AssertConstMsgA(x, sz)
#define AssertConstMsgW(x, sz)
#endif

#define IF_DBG(x)
#define WHEN_DBG(x)
#define IF_NOT_DBG(x) x
#define WHEN_NOT_DBG(x) x
#define DBG_COMMA

#else // !DBG && !RETAILDEBUGLIB

#if !defined(ANALYSIS)

#define Verify(x)               Assert(x)

#define RIPA(sz)                FreRIPA(sz)
#define RIPW(sz)                FreRIPW(sz)
#define Assert(x)               FreAssert(x)
// AssertConstMsg uses NT_ASSERTMSG, which places constant inline strings in
// the PDB file.  Strings passed to this macro cannot be generated at runtime.
#define AssertConstMsgA(x, sz)  FreAssertConstMsgA(x, sz)
#define AssertConstMsgW(x, sz)  FreAssertConstMsgW(x, sz)
// AssertMsg uses DbgPrint.  This method can accept strings that are generated
// at runtime.
#define AssertMsgA(x, sz)       \
    do {\
        FreAssertMsgA(((DWORD_PTR)(x)), (sz));\
    } while (UNCONDITIONAL_EXPR(false))
#define AssertMsgW(x, sz)       \
    do {\
        FreAssertMsgW(((DWORD_PTR)(x)), (sz));\
    } while (UNCONDITIONAL_EXPR(false))
#endif

#define IF_DBG(x) x
#define WHEN_DBG(x) x
#define IF_NOT_DBG(x)
#define WHEN_NOT_DBG(x)
#define DBG_COMMA ,

#endif // !DBG && !RETAILDEBUGLIB

//------------------------------------------------------------------------------
//-------------------------- Trace Tag MACRO wrappers --------------------------
//------------------------------------------------------------------------------
#if DBG != 1 && RETAILDEBUGLIB != 1
    #define TraceTag(x)
    #define TraceTagEx(x)
    #define TaggedTraceListEx(tag, usFlags, szFmt, valMarker)
    #define TraceCallers(tag, iStart, cTotal)
    #define DeclareTag(tag, szOwner, szDescription)
    #define DeclareTagEx(tag, szOwner, szDescription, fEnabled)
    #define DeclareTagOther(tag, szOwner, szDescription)
    #define ExternTag(tag)
    #define IsTagEnabled(tag) (UNCONDITIONAL_EXPR(FALSE))
    #define EnableTag(tag, fEnable)
    #define SetDiskFlag(tag, fSendToDisk)
    #define SetBreakFlag(tag, fBreak)
    #define FindTag(szTagDesc) NULL
    #define PerfDbgTag(tag, szOwner, szDescrip) \
            PerfTag(tag, szOwner, szDescrip)
    #define PerfDbgTagOther(tag, szOwner, szDescrip) \
            PerfTag(tag, szOwner, szDescrip)
    #define PerfDbgExtern(tag) \
            PerfExtern(tag)
    #define IsPerfDbgEnabled(tag) IsPerfEnabled(tag)

#else
    #define TraceTag(x)                         \
        do                                      \
        {                                       \
            if (DbgExTaggedTrace x) {                \
                AvalonDebugBreak();             \
            }                                   \
        } while (UNCONDITIONAL_EXPR(false))

    #define TraceTagEx(x)                       \
        do                                      \
        {                                       \
            if (DbgExTaggedTraceEx x) {              \
                AvalonDebugBreak();             \
            }                                   \
        } while (UNCONDITIONAL_EXPR(false))

    #define TraceCallers(tag, iStart, cTotal)   \
        DbgExTaggedTraceCallers(tag, iStart, cTotal)

    #define DeclareTag(tag, szOwner, szDescrip) \
        TRACETAG tag(DbgExTagRegisterTrace(#tag, szOwner, szDescrip, FALSE));
    #define DeclareTagEx(tag, szOwner, szDescrip, fEnabled) \
        TRACETAG tag(DbgExTagRegisterTrace(#tag, szOwner, szDescrip, fEnabled));
    #define ExternTag(tag) extern TRACETAG tag;

    #define PerfDbgTag(tag, szOwner, szDescrip) DeclareTag(tag, szOwner, szDescrip)
    #define PerfDbgTagOther(tag, szOwner, szDescrip) DeclareTagOther(tag, szOwner, szDescrip)
    #define PerfDbgExtern(tag) ExternTag(tag)
    #define IsPerfDbgEnabled(tag) IsTagEnabled(tag)

    // DbgExTaggedTraceEx usFlags parameter defines
    #define TAG_NONAME      0x01
    #define TAG_NONEWLINE   0x02
    #define TAG_USECONSOLE  0x04
    #define TAG_INDENT      0x08
    #define TAG_OUTDENT     0x10

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
    
    // Get/Set tag enabled status.
    #define IsTagEnabled            DbgExIsTagEnabled
    #define EnableTag               DbgExEnableTag
    #define SetDiskFlag             DbgExSetDiskFlag
    #define SetBreakFlag            DbgExSetBreakFlag
    #define FindTag                 DbgExFindTag

#endif

//------------------------------------------------------------------------------
//------------------------------ Failure Testing -------------------------------
//------------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Macro:     ASSIGN_*
//
//  Synopsis:  Assigns errExpr result to destVar and traces result for debug
//
//             Suffix  Types Handled
//              FAIL    bool, BOOL, + (see TraceFail)
//              W32     BOOL+ returned from Win32 API
//              HR      HRESULT
//              NT      NTSTATUS
//
//  Returns:   nothing, because using result may upset PreFast since it won't
//             understand that the return value is exactly errExpr and
//             therefore can assume that a code path is taken or not
//             incorrectly.  For example if a method is supposed to set an out
//             parameter when it returns success, but there was a failure along
//             the way and PreFast thought success could be returned it will
//             trigger a failure.
//

#define ASSIGN_FAIL(destVar, errTest, errExpr)  ((void) ASSIGN_FAIL_PREFASTOKAY(destVar, errTest, errExpr))
#define ASSIGN_W32(destVar, errTest, errExpr)   ((void) ASSIGN_W32_PREFASTOKAY(destVar, errTest, errExpr))
#define ASSIGN_HR(destVar, errExpr)             ((void) ASSIGN_HR_PREFASTOKAY(destVar, errExpr))
#define ASSIGN_NT(destVar, errExpr)             ((void) ASSIGN_NT_PREFASTOKAY(destVar, errExpr))


#if (DBG == 1 || RETAILDEBUGLIB == 1) && defined(__cplusplus)

#if defined(_PREFIX_)

// On a prefix run we don't want prefix to assume a default model for the trace functions.
// Instead, we want to be explicit about the fact that these macros do some work irrelevant
// to the prefix simulation and then return one of their parameters unchanged.

#define SetSimFailCounts(a, b)
#define GetFailCount                    0
#define TraceFailL(a, b, c, d, e, f)    (a)
#define TraceWin32L(a, b, c, d, e, f)   (a)
#define TraceHR(a, b, c, d, e)          (a)
#define TraceFail(a, b, c, d, e, f)     (a)
#define TraceWin32(a, b, c, d, e, f)    (a)

#else // _PREFIX_

#define SetSimFailCounts    DbgExSetSimFailCounts
#define GetFailCount        DbgExGetFailCount
#define TraceFailL          DbgExTraceFailL
#define TraceWin32L         DbgExTraceWin32L
#define TraceHR             DbgExTraceHR

template <class t> inline t
TraceFail(t errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return (t) DbgExTraceFailL((LONG_PTR) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

// Specialization for T = bool that would prevent warning 
// C4800: type': forcing value to bool 'true' or 'false' (performance warning)
template<> inline bool
TraceFail<bool>(bool errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return DbgExTraceFailL((LONG_PTR)errExpr, errTest, fIgnore, pstrExpr, pstrFile, line) ? true : false;
}

template <class t> inline t
TraceWin32(t errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return (t) DbgExTraceWin32L((LONG_PTR) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

// Specialization for T = bool that would prevent warning 
// C4800: type': forcing value to bool 'true' or 'false' (performance warning)
template <> inline bool
TraceWin32<bool>(bool errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    return DbgExTraceWin32L((LONG_PTR)errExpr, errTest, fIgnore, pstrExpr, pstrFile, line) ? true : false;
}

#endif // _PREFIX_

#if defined(_PREFAST_)

// Prefast doesn't explore calls and assumes any value maybe returned despite
// our knowledge (hope) that the DbgExTrace* methods don't change the result.
#define TFAIL(e, x)             ((e), (x))
#define TW32(e, x)              ((e), (x))
#define THR(x)                  (x)
#define TNT(x)                  (x)

#else

#define TFAIL(e, x)             (TraceFail((x), (e), FALSE, #x, __FILE__, __LINE__))
#define TW32(e, x)              (TraceWin32((x), (e), FALSE, #x, __FILE__, __LINE__))
#define THR(x)                  (TraceHR((x), FALSE, #x, __FILE__, __LINE__))
#define TNT(x)                  (TraceFail((x), -1, FALSE, #x, __FILE__, __LINE__))

#endif


//+---------------------------------------------------------------------------
//
//  Macro:     ASSIGN_*_PREFASTOKAY
//
//  Synopsis:  Assigns errExpr result to destVar and traces result for debug
//
//  Returns:   Result of errExpr
//
//  Notes:     _PREFASTOKAY suffix means caller understands that PreFast
//             doesn't understand the return value is the same as errExpr.
//

#define ASSIGN_FAIL_PREFASTOKAY(destVar, errTest, errExpr)    (TraceFail((destVar = (errExpr)), (errTest), FALSE, #errExpr, __FILE__, __LINE__))
#define ASSIGN_W32_PREFASTOKAY(destVar, errTest, errExpr)     (TraceWin32((destVar = (errExpr)), (errTest), FALSE, #errExpr, __FILE__, __LINE__))
#define ASSIGN_HR_PREFASTOKAY(destVar, errExpr)         (TraceHR((destVar = (errExpr)), FALSE, #errExpr, __FILE__, __LINE__))
#define ASSIGN_NT_PREFASTOKAY(destVar, errExpr)         (TraceFail((destVar = (errExpr)), -1, FALSE, #errExpr, __FILE__, __LINE__))

#define TFAIL_NOTRACE(e, x)     (x)
#define TW32_NOTRACE(e, x)      (x)
#define THR_NOTRACE(x)          (x)
#define TNT_NOTRACE(x)          (x)

#define IGNORE_FAIL(e, x)       ((void) TraceFail((x), (e), TRUE, #x, __FILE__, __LINE__))
#define IGNORE_W32(e,x)         ((void) TraceWin32((x), (e), TRUE, #x, __FILE__, __LINE__))
#define IGNORE_HR(x)            ((void) TraceHR((x), TRUE, #x, __FILE__, __LINE__))
#define IGNORE_NT(x)            ((void) TraceFail((x), -1, TRUE, #x, __FILE__, __LINE__))

#else // #if DBG == 1

#define SetSimFailCounts(firstFailure, cInterval)

#define TFAIL(e, x)             (x)
#define TW32(e, x)              (x)
#define THR(x)                  (x)
#define TNT(x)                  (x)

#define ASSIGN_FAIL_PREFASTOKAY(destVar, errTest, errExpr)  (destVar = (errExpr))
#define ASSIGN_W32_PREFASTOKAY(destVar, errTest, errExpr)   (destVar = (errExpr))
#define ASSIGN_HR_PREFASTOKAY(destVar, errExpr)             (destVar = (errExpr))
#define ASSIGN_NT_PREFASTOKAY(destVar, errExpr)             (destVar = (errExpr))

#define TFAIL_NOTRACE(e, x)     (x)
#define TW32_NOTRACE(e, x)      (x)
#define THR_NOTRACE(x)          (x)
#define TNT_NOTRACE(x)          (x)

#define IGNORE_FAIL(e, x)       ((void)(x))
#define IGNORE_W32(e,x)         ((void)(x))
#define IGNORE_HR(x)            ((void)(x))
#define IGNORE_NT(x)            ((void)(x))

#endif // #if DBG == 1

// Success verification ------------------------------------------------------
#if DBG_ANALYSIS
    #define VerifySUCCEEDED(x) Assert(SUCCEEDED(x));
#else
    #define VerifySUCCEEDED(x) IGNORE_HR(x);
#endif

// Debug only parameters -------------------------------------------------------
#if DBG
    #define DBG_PARAM(x)        x
    #define DBG_COMMA_PARAM(x)  , x
    #define DBG_PARAM_COMMA(x)  x,
#else
    #define DBG_PARAM(x)
    #define DBG_COMMA_PARAM(x)
    #define DBG_PARAM_COMMA(x)
#endif




