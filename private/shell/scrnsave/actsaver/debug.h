//====== Assertion/Debug output APIs =================================

#if defined(DECLARE_DEBUG) && defined(DEBUG)

//
// Declare module-specific debug strings
//
//   When including this header in your private header file, do not
//   define DECLARE_DEBUG.  But do define DECLARE_DEBUG in one of the
//   source files in your project, and then include this header file.
//
//   You may also define the following:
//
//      SZ_DEBUGINI     - the .ini file used to set debug flags
//      SZ_DEBUGSECTION - the section in the .ini file specific to
//                        the module component.
//      SZ_MODULE       - ansi version of the name of your module.
//
//

// (These are deliberately CHAR)
EXTERN_C const CHAR FAR c_szCcshellIniFile[] = SZ_DEBUGINI;
EXTERN_C const CHAR FAR c_szCcshellIniSecDebug[] = SZ_DEBUGSECTION;

EXTERN_C const WCHAR FAR c_wszTrace[] = L"t " TEXTW(SZ_MODULE) L"  ";
EXTERN_C const WCHAR FAR c_wszErrorDbg[] = L"err " TEXTW(SZ_MODULE) L"  ";
EXTERN_C const WCHAR FAR c_wszWarningDbg[] = L"wn " TEXTW(SZ_MODULE) L"  ";
EXTERN_C const WCHAR FAR c_wszAssertMsg[] = TEXTW(SZ_MODULE) L"  Assert: ";
EXTERN_C const WCHAR FAR c_wszAssertFailed[] = TEXTW(SZ_MODULE) L"  Assert %s, line %d: (%s)\r\n";

// (These are deliberately CHAR)
EXTERN_C const CHAR  FAR c_szTrace[] = "t " SZ_MODULE "  ";
EXTERN_C const CHAR  FAR c_szErrorDbg[] = "err " SZ_MODULE "  ";
EXTERN_C const CHAR  FAR c_szWarningDbg[] = "wn " SZ_MODULE "  ";
EXTERN_C const CHAR  FAR c_szAssertMsg[] = SZ_MODULE "  Assert: ";
EXTERN_C const CHAR  FAR c_szAssertFailed[] = SZ_MODULE "  Assert %s, line %d: (%s)\r\n";

#endif  // DECLARE_DEBUG && DEBUG


#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DECLARE_DEBUG)

//
// Debug macros and validation code
//

#undef Assert
#undef AssertE
#undef AssertMsg
#undef AssertStrLen
#undef DebugMsg
#undef FullDebugMsg
#undef ASSERT
#undef EVAL

// Access these globals to determine which debug flags are set.
// These globals are modified by CcshellGetDebugFlags(), which
// reads an .ini file and sets the appropriate flags.
//
//   g_dwDumpFlags  - bits are application specific.  Typically 
//                    used for dumping structures.
//   g_dwBreakFlags - uses BF_* flags.  The remaining bits are
//                    application specific.  Used to determine
//                    when to break into the debugger.
//   g_dwTraceFlags - uses TF_* flags.  The remaining bits are
//                    application specific.  Used to display
//                    debug trace messages.
//   g_dwFuncTraceFlags - bits are application specific.  When
//                    TF_FUNC is set, CcshellFuncMsg uses this
//                    value to determine which function traces
//                    to display.
//   g_dwProtoype   - bits are application specific.  Use it for
//                    anything.
//

extern DWORD g_dwDumpFlags;
extern DWORD g_dwBreakFlags;
extern DWORD g_dwTraceFlags;
#ifdef DEBUG
extern DWORD g_dwPrototype;
#else
#define g_dwPrototype   0
#endif
extern DWORD g_dwFuncTraceFlags;

BOOL CcshellGetDebugFlags(void);

// Break flags for g_dwBreakFlags
#define BF_ONVALIDATE       0x00000001      // Break on assertions or validation
#define BF_ONAPIENTER       0x00000002      // Break on entering an API
#define BF_ONERRORMSG       0x00000004      // Break on TF_ERROR
#define BF_ONWARNMSG        0x00000008      // Break on TF_WARNING

// Trace flags for g_dwTraceFlags
#define TF_ALWAYS           0xFFFFFFFF
#define TF_NEVER            0x00000000
#define TF_WARNING          0x00000001
#define TF_ERROR            0x00000002
#define TF_GENERAL          0x00000004      // Standard messages
#define TF_FUNC             0x00000008      // Trace function calls
// (Upper 28 bits reserved for custom use per-module)

// Old, archaic debug flags.  
// BUGBUG (scotth): the following flags will be phased out over time.
#ifdef DM_TRACE
#undef DM_TRACE
#undef DM_WARNING
#undef DM_ERROR
#endif
#define DM_TRACE            TF_GENERAL      // OBSOLETE Trace messages
#define DM_WARNING          TF_WARNING      // OBSOLETE Warning
#define DM_ERROR            TF_ERROR        // OBSOLETE Error


// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg)      /* ;Internal */ \
    static const TCHAR sz[] = msg;


#ifndef NOSHELLDEBUG    // Others have own versions of these.
#ifdef DEBUG

#ifdef _X86_
// Use int 3 so we stop immediately in the source
#define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
#define DEBUG_BREAK        do { _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#endif

// Prototypes for debug functions

void CcshellStackEnter(void);
void CcshellStackLeave(void);

BOOL CcshellAssertFailedA(LPCSTR szFile, int line, LPCSTR pszEval, BOOL bBreak);
BOOL CcshellAssertFailedW(LPCWSTR szFile, int line, LPCWSTR pwszEval, BOOL bBreak);

void CDECL CcshellDebugMsgW(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellDebugMsgA(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellFuncMsgW(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellFuncMsgA(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellAssertMsgW(BOOL bAssert, LPCSTR pszMsg, ...);
void CDECL CcshellAssertMsgA(BOOL bAssert, LPCSTR pszMsg, ...);

void CDECL _AssertMsgA(BOOL f, LPCSTR pszMsg, ...);
void CDECL _AssertMsgW(BOOL f, LPCWSTR pszMsg, ...);
void CDECL _DebugMsgA(DWORD flag, LPCSTR psz, ...);
void CDECL _DebugMsgW(DWORD flag, LPCWSTR psz, ...);

void _AssertStrLenA(LPCSTR pszStr, int iLen);
void _AssertStrLenW(LPCWSTR pwzStr, int iLen);


#ifdef UNICODE
#define CcshellAssertFailed     CcshellAssertFailedW
#define CcshellDebugMsg         CcshellDebugMsgW
#define CcshellFuncMsg          CcshellFuncMsgW
#define CcshellAssertMsg        CcshellAssertMsgW
#define _AssertMsg              _AssertMsgW
#define _AssertStrLen           _AssertStrLenW
#define _DebugMsg               _DebugMsgW
#else
#define CcshellAssertFailed     CcshellAssertFailedA
#define CcshellDebugMsg         CcshellDebugMsgA
#define CcshellFuncMsg          CcshellFuncMsgA
#define CcshellAssertMsg        CcshellAssertMsgA
#define _AssertMsg              _AssertMsgA
#define _AssertStrLen           _AssertStrLenA
#define _DebugMsg               _DebugMsgA
#endif



// Explanation of debug macros:
//
// ----
// Assert(f)
// ASSERT(f)
//
//   Generates a "Assert file.c, line x (eval)" message if f is NOT true.
//   The g_dwBreakFlags global governs whether the function DebugBreaks.
//
// ----
// AssertE(f)
//
//   Works like Assert, except (f) is also executed in the retail 
//   version as well.
//
// ----
// EVAL(f)
//
//   Evaluates the expression (f).  The expression is always evaluated,
//   even in retail builds.  But the macro only asserts in the debug
//   build.  This macro may only be used on logical expressions, eg:
//
//          if (EVAL(exp))
//              // do something
//
// ----
// TraceMsg(mask, sz, args...) 
//
//   Generate wsprintf-formatted msg using specified trace mask.  
//   The g_dwTraceFlags global governs whether message is displayed.
//
//   The sz parameter is always ANSI; TraceMsg correctly converts it
//   to unicode if necessary.  This is so you don't have to wrap your
//   debug strings with TEXT().
//
// ----
// DebugMsg(mask, sz, args...) 
//
//   OBSOLETE!  
//   Like TraceMsg, except you must wrap the sz parameter with TEXT().
//
// ----
// AssertMsg(bAssert, sz, args...)
//
//   Generate wsprintf-formatted msg if the assertion is false.  
//   The g_dwBreakFlags global governs whether the function DebugBreaks.
//
//   The sz parameter is always ANSI; AssertMsg correctly converts it
//   to unicode if necessary.  This is so you don't have to wrap your
//   debug strings with TEXT().
//


#define ASSERT(f)                                 \
    {                                             \
        DEBUGTEXT(szFile, TEXT(__FILE__));              \
        if (!(f) && CcshellAssertFailed(szFile, __LINE__, TEXT(#f), FALSE)) \
            DEBUG_BREAK;       \
    }

#ifdef DISALLOW_Assert
#define Assert(f)        Dont_use_Assert___Use_ASSERT
#else
#define Assert(f)           ASSERT(f)
#endif


#define AssertE(f)          ASSERT(f)
#define EVAL(exp)   \
    ((exp) || (CcshellAssertFailed(TEXT(__FILE__), __LINE__, TEXT(#exp), TRUE), 0))

// Use TraceMsg instead of DebugMsg.  DebugMsg is obsolete.
#define AssertMsg           _AssertMsg
#define AssertStrLen        _AssertStrLen
#define AssertStrLenA       _AssertStrLenA
#define AssertStrLenW       _AssertStrLenW

#ifdef DISALLOW_DebugMsg
#define DebugMsg            Dont_use_DebugMsg___Use_TraceMsg
#else
#define DebugMsg            _DebugMsg
#endif

#ifdef FULL_DEBUG
#define FullDebugMsg        _DebugMsg
#else
#define FullDebugMsg        1 ? (void)0 : (void)
#endif

#define Dbg_SafeStrA(psz)   (SAFECAST(psz, LPCSTR), (psz) ? (psz) : "NULL string")
#define Dbg_SafeStrW(psz)   (SAFECAST(psz, LPCWSTR), (psz) ? (psz) : L"NULL string")
#ifdef UNICODE
#define Dbg_SafeStr         Dbg_SafeStrW
#else
#define Dbg_SafeStr         Dbg_SafeStrA
#endif

#define ASSERT_MSGW         CcshellAssertMsgW
#define ASSERT_MSGA         CcshellAssertMsgA
#define ASSERT_MSG          CcshellAssertMsg

#define TraceMsgW           CcshellDebugMsgW
#define TraceMsgA           CcshellDebugMsgA
#define TraceMsg            CcshellDebugMsg

#define FUNC_MSG            CcshellFuncMsg

// Debug function enter


// DBG_ENTER(flag, fn)  -- Generates a function entry debug spew for
//                          a function
//
#define DBG_ENTER(flagFTF, fn)                  \
        (FUNC_MSG(flagFTF, " > " #fn "()"), \
         CcshellStackEnter())

// DBG_ENTER_TYPE(flag, fn, dw, pfnStrFromType)  -- Generates a function entry debug
//                          spew for functions that accept <type>.
//
#define DBG_ENTER_TYPE(flagFTF, fn, dw, pfnStrFromType)                   \
        (FUNC_MSG(flagFTF, " < " #fn "(..., %s, ...)", (LPCTSTR)pfnStrFromType(dw)), \
         CcshellStackEnter())

// DBG_ENTER_SZ(flag, fn, sz)  -- Generates a function entry debug spew for
//                          a function that accepts a string as one of its
//                          parameters.
//
#define DBG_ENTER_SZ(flagFTF, fn, sz)                  \
        (FUNC_MSG(flagFTF, " > " #fn "(..., \"%s\",...)", Dbg_SafeStr(sz)), \
         CcshellStackEnter())


// Debug function exit


// DBG_EXIT(flag, fn)  -- Generates a function exit debug spew
//
#define DBG_EXIT(flagFTF, fn)                              \
        (CcshellStackLeave(), \
         FUNC_MSG(flagFTF, " < " #fn "()"))

// DBG_EXIT_TYPE(flag, fn, dw, pfnStrFromType)  -- Generates a function exit debug
//                          spew for functions that return <type>.
//
#define DBG_EXIT_TYPE(flagFTF, fn, dw, pfnStrFromType)                   \
        (CcshellStackLeave(), \
         FUNC_MSG(flagFTF, " < " #fn "() with %s", (LPCTSTR)pfnStrFromType(dw)))

// DBG_EXIT_INT(flag, fn, us)  -- Generates a function exit debug spew for
//                          functions that return an INT.
//
#define DBG_EXIT_INT(flagFTF, fn, n)                       \
        (CcshellStackLeave(), \
         FUNC_MSG(flagFTF, " < " #fn "() with %d", (int)(n)))

// DBG_EXIT_BOOL(flag, fn, b)  -- Generates a function exit debug spew for
//                          functions that return a boolean.
//
#define DBG_EXIT_BOOL(flagFTF, fn, b)                      \
        (CcshellStackLeave(), \
         FUNC_MSG(flagFTF, " < " #fn "() with %s", (b) ? (LPTSTR)TEXT("TRUE") : (LPTSTR)TEXT("FALSE")))

// DBG_EXIT_UL(flag, fn, ul)  -- Generates a function exit debug spew for
//                          functions that return a ULONG.
//
#define DBG_EXIT_UL(flagFTF, fn, ul)                   \
        (CcshellStackLeave(), \
         FUNC_MSG(flagFTF, " < " #fn "() with %#08lx", (ULONG)(ul)))

#define DBG_EXIT_DWORD      DBG_EXIT_UL

// DBG_EXIT_HRES(flag, fn, hres)  -- Generates a function exit debug spew for
//                          functions that return an HRESULT.
//
#define DBG_EXIT_HRES(flagFTF, fn, hres)     DBG_EXIT_TYPE(flagFTF, fn, hres, Dbg_GetHRESULTName)



#else   // DEBUG


#define Assert(f)
#define AssertE(f)      (f)
#define ASSERT(f)
#define AssertMsg       1 ? (void)0 : (void)
#define AssertStrLen(lpStr, iLen)
#define DebugMsg        1 ? (void)0 : (void)
#define FullDebugMsg    1 ? (void)0 : (void)
#define EVAL(exp)       ((exp) != 0)


#define Dbg_SafeStr     1 ? (void)0 : (void)

#define TraceMsgA       1 ? (void)0 : (void)
#define TraceMsgW       1 ? (void)0 : (void)
#ifdef UNICODE
#define TraceMsg        TraceMsgW
#else
#define TraceMsg        TraceMsgA
#endif

#define FUNC_MSG        1 ? (void)0 : (void)

#define ASSERT_MSGA     TraceMsgA
#define ASSERT_MSGW     TraceMsgW
#define ASSERT_MSG      TraceMsg

#define DBG_ENTER(flagFTF, fn)
#define DBG_ENTER_TYPE(flagFTF, fn, dw, pfn)
#define DBG_ENTER_SZ(flagFTF, fn, sz)
#define DBG_EXIT(flagFTF, fn)
#define DBG_EXIT_INT(flagFTF, fn, n)
#define DBG_EXIT_BOOL(flagFTF, fn, b)
#define DBG_EXIT_UL(flagFTF, fn, ul)
#define DBG_EXIT_DWORD      DBG_EXIT_UL
#define DBG_EXIT_TYPE(flagFTF, fn, dw, pfn)
#define DBG_EXIT_HRES(flagFTF, fn, hres)

#endif  // DEBUG
#endif  // NOSHELLDEBUG


// 
// Debug dump helper functions
//

#ifdef DEBUG

LPCTSTR DebugGUIDName(REFGUID rguid, LPCTSTR szKey, LPTSTR pchName);
LPCTSTR DebugIIDName(REFIID riid, LPTSTR pchName);
LPCTSTR DebugCLSIDName(REFIID riid, LPTSTR pchName);

#else

#endif // DEBUG


// Parameter validation macros
#include "validate.h"

#endif // DECLARE_DEBUG

#ifdef __cplusplus
};
#endif
