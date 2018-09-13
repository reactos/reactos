/*
 *  M A P I D B G . H
 *
 *  Debugging support for MAPI service providers.
 *  Support functions are implemented in MAPIDBG.C.
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef __MAPIDBG_H_
#define __MAPIDBG_H_

#if defined (WIN32) && !defined (_WIN32)
#define _WIN32
#endif

/*
 * Debugging Macros -------------------------------------------------------
 *
 *      IFDBG(x)        Results in the expression x if DEBUG is defined, or
 *                      to nothing if DEBUG is not defined
 *  
 *      IFNDBG(x)       Results in the expression x if DEBUG is not defined,
 *                      or to nothing if DEBUG is defined
 *  
 *      Unreferenced(a) Causes a to be referenced so that the compiler
 *                      doesn't issue warnings about unused local variables
 *                      which exist but are reserved for future use (eg
 *                      ulFlags in many cases)
 */

#if defined(DEBUG)
#define IFDBG(x)            x
#define IFNDBG(x)
#else
#define IFDBG(x)
#define IFNDBG(x)           x
#endif

#ifdef __cplusplus
#define EXTERN_C_BEGIN      extern "C" {
#define EXTERN_C_END        }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

#define dimensionof(a)      (sizeof(a)/sizeof(*(a)))

#define Unreferenced(a)     ((void)(a))

typedef long SCODE;
typedef unsigned long ULONG;
typedef unsigned long DWORD;

/*
 *   Assert Macros ---------------------------------------------------------
 *  
 *      Assert(a)       Displays a message indicating the file and line number
 *                      of this Assert() if a == 0.  OK'ing an assert traps
 *                      into the debugger.
 *  
 *      AssertSz(a,sz)  Works like an Assert(), but displays the string sz
 *                      along with the file and line number.
 *  
 *      Side asserts    A side assert works like an Assert(), but evaluates
 *                      'a' even when asserts are not enabled.
 *  
 *      NF asserts      A NF (Non-Fatal) assert works like an Assert(), but
 *                      continues instead of trapping into the debugger when
 *                      OK'ed.
 */

#if defined(DEBUG) || defined(ASSERTS_ENABLED)
#define IFTRAP(x)           x
#else
#define IFTRAP(x)           0
#endif

#define Trap()                                          IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,"Trap"))
#define TrapSz(psz)                                     IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz))
#define TrapSz1(psz,a1)                                 IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1))
#define TrapSz2(psz,a1,a2)                              IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2))
#define TrapSz3(psz,a1,a2,a3)                           IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3))
#define TrapSz4(psz,a1,a2,a3,a4)                        IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4))
#define TrapSz5(psz,a1,a2,a3,a4,a5)                     IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5))
#define TrapSz6(psz,a1,a2,a3,a4,a5,a6)                  IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6))
#define TrapSz7(psz,a1,a2,a3,a4,a5,a6,a7)               IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7))
#define TrapSz8(psz,a1,a2,a3,a4,a5,a6,a7,a8)            IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8))
#define TrapSz9(psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)         IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9))

#define Assert(t)                                       IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t),0))
#define AssertSz(t,psz)                                 IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz),0))
#define AssertSz1(t,psz,a1)                             IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1),0))
#define AssertSz2(t,psz,a1,a2)                          IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2),0))
#define AssertSz3(t,psz,a1,a2,a3)                       IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3),0))
#define AssertSz4(t,psz,a1,a2,a3,a4)                    IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4),0))
#define AssertSz5(t,psz,a1,a2,a3,a4,a5)                 IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5),0))
#define AssertSz6(t,psz,a1,a2,a3,a4,a5,a6)              IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6),0))
#define AssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)           IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7),0))
#define AssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)        IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8),0))
#define AssertSz9(t,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)     IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9),0))

#define SideAssert(t)                                   ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t)),0)
#define SideAssertSz(t,psz)                             ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz)),0)
#define SideAssertSz1(t,psz,a1)                         ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1)),0)
#define SideAssertSz2(t,psz,a1,a2)                      ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2)),0)
#define SideAssertSz3(t,psz,a1,a2,a3)                   ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3)),0)
#define SideAssertSz4(t,psz,a1,a2,a3,a4)                ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4)),0)
#define SideAssertSz5(t,psz,a1,a2,a3,a4,a5)             ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5)),0)
#define SideAssertSz6(t,psz,a1,a2,a3,a4,a5,a6)          ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6)),0)
#define SideAssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)       ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7)),0)
#define SideAssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)    ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8)),0)
#define SideAssertSz9(t,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9) ((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)),0)

#define NFAssert(t)                                     IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,"Assertion Failure: " #t),0))
#define NFAssertSz(t,psz)                               IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz),0))
#define NFAssertSz1(t,psz,a1)                           IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1),0))
#define NFAssertSz2(t,psz,a1,a2)                        IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2),0))
#define NFAssertSz3(t,psz,a1,a2,a3)                     IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3),0))
#define NFAssertSz4(t,psz,a1,a2,a3,a4)                  IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4),0))
#define NFAssertSz5(t,psz,a1,a2,a3,a4,a5)               IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5),0))
#define NFAssertSz6(t,psz,a1,a2,a3,a4,a5,a6)            IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6),0))
#define NFAssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)         IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7),0))
#define NFAssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)      IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8),0))
#define NFAssertSz9(t,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)   IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9),0))

#define NFSideAssert(t)                                 ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,"Assertion Failure: " #t)),0)
#define NFSideAssertSz(t,psz)                           ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz)),0)
#define NFSideAssertSz1(t,psz,a1)                       ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1)),0)
#define NFSideAssertSz2(t,psz,a1,a2)                    ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2)),0)
#define NFSideAssertSz3(t,psz,a1,a2,a3)                 ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3)),0)
#define NFSideAssertSz4(t,psz,a1,a2,a3,a4)              ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4)),0)
#define NFSideAssertSz5(t,psz,a1,a2,a3,a4,a5)           ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5)),0)
#define NFSideAssertSz6(t,psz,a1,a2,a3,a4,a5,a6)        ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6)),0)
#define NFSideAssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)     ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7)),0)
#define NFSideAssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)  ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8)),0)
#define NFSideAssertSz9(t,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)   ((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)),0)

/*
 *   Trace Macros ------------------------------------------------------------
 *  
 *      DebugTrace          Use for arbitrary formatted output. It
 *                          takes exactly the same arguments as the
 *                          Windows wsprintf() function.
 *      DebugTraceResult    Shorthand for error tracing with an
 *                          HRESULT. Arguments are the name of the
 *                          function (not quoted) and the HRESULT.
 *      DebugTraceSc        Shorthand for error tracing with an
 *                          SCODE. Arguments are the name of the
 *                          function (not quoted) and the SCODE.
 *      DebugTraceArg       Shorthand for invalid parameter
 *                          tracing. Arguments are the name of the
 *                          function (not quoted) and a quoted
 *                          string describing the bad parameter.
 */

#if defined(DEBUG) || defined(TRACES_ENABLED)
#define IFTRACE(x)          x
#define DebugTrace          DebugTraceFn
#else
#define IFTRACE(x)          0
#define DebugTrace          1?0:DebugTraceFn
#endif

#define DebugTraceResult(f,hr)                          IFTRACE(((hr) ? DebugTraceFn(#f " returns 0x%08lX %s\n", GetScode(hr), SzDecodeScode(GetScode(hr))) : 0))
#define DebugTraceSc(f,sc)                              IFTRACE(((sc) ? DebugTraceFn(#f " returns 0x%08lX %s\n", sc, SzDecodeScode(sc)) : 0))
#define DebugTraceArg(f,s)                              IFTRACE(DebugTraceFn(#f ": bad parameter: " s "\n"))
#define DebugTraceLine()                                IFTRACE(DebugTraceFn("File %s, Line %i  \n",__FILE__,__LINE__))
#define DebugTraceProblems(sz, rgprob)                  IFTRACE(DebugTraceProblemsFn(sz, rgprob))

#define TraceSz(psz)                                    IFTRACE(DebugTraceFn("~" psz))
#define TraceSz1(psz,a1)                                IFTRACE(DebugTraceFn("~" psz,a1))
#define TraceSz2(psz,a1,a2)                             IFTRACE(DebugTraceFn("~" psz,a1,a2))
#define TraceSz3(psz,a1,a2,a3)                          IFTRACE(DebugTraceFn("~" psz,a1,a2,a3))
#define TraceSz4(psz,a1,a2,a3,a4)                       IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4))
#define TraceSz5(psz,a1,a2,a3,a4,a5)                    IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5))
#define TraceSz6(psz,a1,a2,a3,a4,a5,a6)                 IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6))
#define TraceSz7(psz,a1,a2,a3,a4,a5,a6,a7)              IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6,a7))
#define TraceSz8(psz,a1,a2,a3,a4,a5,a6,a7,a8)           IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6,a7,a8))
#define TraceSz9(psz,a1,a2,a3,a4,a5,a6,a7,a8,a9)        IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6,a7,a8,a9))

/* Debugging Functions ---------------------------------------------------- */

EXTERN_C_BEGIN

#ifdef WIN16
#define EXPORTDBG   __export
#else
#define EXPORTDBG
#endif

int EXPORTDBG __cdecl       DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...);
int EXPORTDBG __cdecl       DebugTraceFn(char *pszFormat, ...);
void EXPORTDBG __cdecl      DebugTraceProblemsFn(char *sz, void *rgprob);
char * EXPORTDBG __cdecl    SzDecodeScodeFn(SCODE sc);
char * EXPORTDBG __cdecl    SzDecodeUlPropTypeFn(unsigned long ulPropType);
char * EXPORTDBG __cdecl    SzDecodeUlPropTagFn(unsigned long ulPropTag);
unsigned long EXPORTDBG __cdecl UlPropTagFromSzFn(char *psz);
SCODE EXPORTDBG __cdecl     ScodeFromSzFn(char *psz);
void * EXPORTDBG __cdecl    DBGMEM_EncapsulateFn(void * pmalloc, char *pszSubsys, int fCheckOften);
void EXPORTDBG __cdecl      DBGMEM_ShutdownFn(void * pmalloc);
void EXPORTDBG __cdecl      DBGMEM_CheckMemFn(void * pmalloc, int fReportOrphans);
#if defined(_WIN32) && defined(_X86_)
void EXPORTDBG __cdecl      DBGMEM_LeakHook(FARPROC pfn);
void EXPORTDBG __cdecl      GetCallStack(DWORD *, int, int);
#endif
void EXPORTDBG __cdecl      DBGMEM_NoLeakDetectFn(void * pmalloc, void *pv);
void EXPORTDBG __cdecl      DBGMEM_SetFailureAtFn(void * pmalloc, ULONG ulFailureAt);
SCODE EXPORTDBG __cdecl     ScCheckScFn(SCODE, SCODE *, char *, char *, int);
void * EXPORTDBG __cdecl    VMAlloc(ULONG);
void * EXPORTDBG __cdecl    VMAllocEx(ULONG, ULONG);
void * EXPORTDBG __cdecl    VMRealloc(void *, ULONG);
void * EXPORTDBG __cdecl    VMReallocEx(void *, ULONG, ULONG);
ULONG EXPORTDBG __cdecl     VMGetSize(void *);
ULONG EXPORTDBG __cdecl     VMGetSizeEx(void *, ULONG);
void EXPORTDBG __cdecl      VMFree(void *);
void EXPORTDBG __cdecl      VMFreeEx(void *, ULONG);

EXTERN_C_END

/*
 *  Debugging Macros -------------------------------------------------------
 *  
 *      SzDecodeScode           Returns the string name of an SCODE
 *      SzDecodeUlPropTag       Returns the string name of a property
 *                              tag
 *      UlPropTagFromSz         Given a property tag's name, returns
 *                              its value
 *      ScodeFromSz             Given an SCODE's name, returns its
 *                              value
 *  
 *      DBGMEM_Encapsulate      Given an IMalloc interface, adds heap-
 *                              checking functionality and returns a
 *                              wrapped interface
 *      DBGMEM_Shutdown         Undoes DBGMEM_Encapsulate, and prints
 *                              out information on any allocations made
 *                              since the interface was encapsulated
 *                              that have not yet been released.
 *      DBGMEM_CheckMem         Checks all memory allocated on the heap,
 *                              and optionally reports leaked blocks.
 *      DBGMEM_NoLeakDetect     Prevents a block from appearing on the leak
 *                              report.  Pass NULL for pv to inhibit leak
 *                              reports at all from this heap.
 */

#ifdef DEBUG

#define SzDecodeScode(_sc)              SzDecodeScodeFn(_sc)
#define SzDecodeUlPropType(_ulPropType) SzDecodeUlPropTypeFn(_ulPropType)
#define SzDecodeUlPropTag(_ulPropTag)   SzDecodeUlPropTagFn(_ulPropTag)
#define UlPropTagFromSz(_sz)            UlPropTagFromSzFn(_sz)
#define ScodeFromSz(_sz)                ScodeFromSzFn(_sz)
#define DBGMEM_Encapsulate(pm, psz, f)  DBGMEM_EncapsulateFn(pm, psz, f)
#define DBGMEM_Shutdown(pm)             DBGMEM_ShutdownFn(pm)
#define DBGMEM_CheckMem(pm, f)          DBGMEM_CheckMemFn(pm, f)
#define DBGMEM_NoLeakDetect(pm, pv)     DBGMEM_NoLeakDetectFn(pm, pv)
#define DBGMEM_SetFailureAt(pm, ul)     DBGMEM_SetFailureAtFn(pm, ul)

#else

#define SzDecodeScode(_sc)              (0)
#define SzDecodeUlPropType(_ulPropType) (0)
#define SzDecodeUlPropTag(_ulPropTag)   (0)
#define UlPropTagFromSz(_sz)            (0)
#define ScodeFromSz(_sz)                (0)

#if defined(__cplusplus) && !defined(CINTERFACE)
#define DBGMEM_Encapsulate(pmalloc, pszSubsys, fCheckOften) \
    ((pmalloc)->AddRef(), (pmalloc))
#define DBGMEM_Shutdown(pmalloc) \
    ((pmalloc)->Release())
#else
#define DBGMEM_Encapsulate(pmalloc, pszSubsys, fCheckOften) \
    ((pmalloc)->lpVtbl->AddRef(pmalloc), (pmalloc))
#define DBGMEM_Shutdown(pmalloc) \
    ((pmalloc)->lpVtbl->Release(pmalloc))
#endif
#define DBGMEM_CheckMem(pm, f)
#define DBGMEM_NoLeakDetect(pm, pv)
#define DBGMEM_SetFailureAt(pm, ul)

#endif

/*
 *  SCODE maps -------------------------------------------------------------
 *  
 *      ScCheckSc       Given an SCODE and method name, verifies
 *                      that the SCODE can legally be returned from
 *                      thet method. Prints out a debug string if
 *                      it cannot.
 *      HrCheckHr       As ScCheckSc, for functions that return
 *                      HRESULT.
 */

#if defined(DEBUG) && !defined(DOS)
#define ScCheckSc(sc,fn)                ScCheckScFn(sc,fn##_Scodes,#fn,__FILE__, __LINE__)
#define HrCheckHr(hr,fn)                HrCheckSc(GetScode(hr),fn)
#else
#define ScCheckSc(sc,fn)                (sc)
#define HrCheckHr(hr,fn)                (hr)
#endif

#define HrCheckSc(sc,fn)                ResultFromScode(ScCheckSc(sc,fn))

#if defined(DEBUG) && !defined(DOS)
extern SCODE Common_Scodes[];
extern SCODE MAPILogon_Scodes[];
extern SCODE MAPIAllocateBuffer_Scodes[];
extern SCODE MAPIAllocateMore_Scodes[];
extern SCODE MAPIFreeBuffer_Scodes[];

extern SCODE IUnknown_QueryInterface_Scodes[];
extern SCODE IUnknown_AddRef_Scodes[];
extern SCODE IUnknown_Release_Scodes[];
extern SCODE IUnknown_GetLastError_Scodes[];

extern SCODE IMAPIProp_CopyTo_Scodes[];
extern SCODE IMAPIProp_CopyProps_Scodes[];
extern SCODE IMAPIProp_DeleteProps_Scodes[];
extern SCODE IMAPIProp_GetIDsFromNames_Scodes[];
extern SCODE IMAPIProp_GetLastError_Scodes[];
extern SCODE IMAPIProp_GetNamesFromIDs_Scodes[];
extern SCODE IMAPIProp_GetPropList_Scodes[];
extern SCODE IMAPIProp_GetProps_Scodes[];
extern SCODE IMAPIProp_OpenProperty_Scodes[];
extern SCODE IMAPIProp_SetProps_Scodes[];
extern SCODE IMAPIProp_SaveChanges_Scodes[];

extern SCODE IStream_Read_Scodes[];
extern SCODE IStream_Write_Scodes[];
extern SCODE IStream_Seek_Scodes[];
extern SCODE IStream_SetSize_Scodes[];
extern SCODE IStream_Tell_Scodes[];
extern SCODE IStream_LockRegion_Scodes[];
extern SCODE IStream_UnlockRegion_Scodes[];
extern SCODE IStream_Clone_Scodes[];
extern SCODE IStream_CopyTo_Scodes[];
extern SCODE IStream_Revert_Scodes[];
extern SCODE IStream_Stat_Scodes[];
extern SCODE IStream_Commit_Scodes[];

extern SCODE IMAPITable_GetLastError_Scodes[];
extern SCODE IMAPITable_Advise_Scodes[];
extern SCODE IMAPITable_Unadvise_Scodes[];
extern SCODE IMAPITable_GetStatus_Scodes[];
extern SCODE IMAPITable_SetColumns_Scodes[];
extern SCODE IMAPITable_QueryColumns_Scodes[];
extern SCODE IMAPITable_GetRowCount_Scodes[];
extern SCODE IMAPITable_SeekRow_Scodes[];
extern SCODE IMAPITable_SeekRowApprox_Scodes[];
extern SCODE IMAPITable_QueryPosition_Scodes[];
extern SCODE IMAPITable_FindRow_Scodes[];
extern SCODE IMAPITable_Restrict_Scodes[];
extern SCODE IMAPITable_CreateBookmark_Scodes[];
extern SCODE IMAPITable_FreeBookmark_Scodes[];
extern SCODE IMAPITable_SortTable_Scodes[];
extern SCODE IMAPITable_QuerySortOrder_Scodes[];
extern SCODE IMAPITable_QueryRows_Scodes[];
extern SCODE IMAPITable_Abort_Scodes[];
extern SCODE IMAPITable_ExpandRow_Scodes[];
extern SCODE IMAPITable_CollapseRow_Scodes[];
extern SCODE IMAPITable_WaitForCompletion_Scodes[];
extern SCODE IMAPITable_GetCollapseState_Scodes[];
extern SCODE IMAPITable_SetCollapseState_Scodes[];

extern SCODE IMAPISession_LogOff_Scodes[];
extern SCODE IMAPISession_Release_Scodes[];
extern SCODE IMAPISession_GetLastError_Scodes[];
extern SCODE IMAPISession_GetMsgStoresTable_Scodes[];
extern SCODE IMAPISession_GetStatusTable_Scodes[];
extern SCODE IMAPISession_OpenMsgStore_Scodes[];
extern SCODE IMAPISession_OpenAddressBook_Scodes[];
extern SCODE IMAPISession_OpenEntry_Scodes[];
extern SCODE IMAPISession_OpenProfileSection_Scodes[];
extern SCODE IMAPISession_Advise_Scodes[];
extern SCODE IMAPISession_Unadvise_Scodes[];
extern SCODE IMAPISession_CompareEntryIDs_Scodes[];
extern SCODE IMAPISession_MessageOptions_Scodes[];
extern SCODE IMAPISession_QueryDefaultMessageOpt_Scodes[];
extern SCODE IMAPISession_EnumAdrTypes_Scodes[];
extern SCODE IMAPISession_QueryIdentity_Scodes[];
extern SCODE IMAPISession_OpenProfileSection_Scodes[];
extern SCODE IMAPISession_GetStatusTable_Scodes[];

extern SCODE IMsgStore_Advise_Scodes[] ;
extern SCODE IMsgStore_Unadvise_Scodes[] ;
extern SCODE IMsgStore_CompareEntryIDs_Scodes[] ;
extern SCODE IMsgStore_OpenEntry_Scodes[] ;
extern SCODE IMsgStore_SetReceiveFolder_Scodes[];
extern SCODE IMsgStore_GetReceiveFolder_Scodes[];
extern SCODE IMsgStore_GetReceiveFolderTable_Scodes[];
extern SCODE IMsgStore_StoreLogoff_Scodes[];
extern SCODE IMsgStore_AbortSubmit_Scodes[];
extern SCODE IMsgStore_GetOutgoingQueue_Scodes[] ;
extern SCODE IMsgStore_SetLockState_Scodes[] ;
extern SCODE IMsgStore_FinishedMsg_Scodes[] ;
extern SCODE IMsgStore_NotifyNewMail_Scodes[];

extern SCODE IMAPIFolder_GetContentsTable_Scodes[];
extern SCODE IMAPIFolder_GetHierarchyTable_Scodes[];
extern SCODE IMAPIFolder_SaveContentsSort_Scodes[];
extern SCODE IMAPIFolder_OpenEntry_Scodes[];
extern SCODE IMAPIFolder_CreateMessage_Scodes[];
extern SCODE IMAPIFolder_CopyMessages_Scodes[];
extern SCODE IMAPIFolder_DeleteMessages_Scodes[];
extern SCODE IMAPIFolder_CreateFolder_Scodes[];
extern SCODE IMAPIFolder_CopyFolder_Scodes[];
extern SCODE IMAPIFolder_DeleteFolder_Scodes[];
extern SCODE IMAPIFolder_SetSearchCriteria_Scodes[];
extern SCODE IMAPIFolder_GetSearchCriteria_Scodes[];
extern SCODE IMAPIFolder_SetReadFlags_Scodes[];
extern SCODE IMAPIFolder_GetMessageStatus_Scodes[];
extern SCODE IMAPIFolder_SetMessageStatus_Scodes[];
extern SCODE IMAPIFolder_EmptyFolder_Scodes[];

extern SCODE IMessage_SaveChanges_Scodes[];
extern SCODE IMessage_GetAttachmentTable_Scodes[];
extern SCODE IMessage_OpenAttach_Scodes[];
extern SCODE IMessage_CreateAttach_Scodes[];
extern SCODE IMessage_DeleteAttach_Scodes[];
extern SCODE IMessage_GetRecipientTable_Scodes[];
extern SCODE IMessage_ModifyRecipients_Scodes[];
extern SCODE IMessage_SubmitMessage_Scodes[];
extern SCODE IMessage_SetReadFlag_Scodes[];

extern SCODE IAttach_SaveChanges_Scodes[];

extern SCODE IAddrBook_OpenEntry_Scodes[];
extern SCODE IAddrBook_CompareEntryIDs_Scodes[];
extern SCODE IAddrBook_CreateOneOff_Scodes[];
extern SCODE IAddrBook_ResolveName_Scodes[];
extern SCODE IAddrBook_Address_Scodes[];
extern SCODE IAddrBook_Details_Scodes[];
extern SCODE IAddrBook_RecipOptions_Scodes[];
extern SCODE IAddrBook_QueryDefaultRecipOpt_Scodes[];
extern SCODE IAddrBook_Address_Scodes[];
extern SCODE IAddrBook_ButtonPress_Scodes[];

extern SCODE IABContainer_GetContentsTable_Scodes[];
extern SCODE IABContainer_GetHierarchyTable_Scodes[];

extern SCODE INotifObj_ChangeEvMask_Scodes[];

extern SCODE IMAPIStatus_ChangePassword_Scodes[];
extern SCODE IMAPIStatus_FlushQueues_Scodes[];
extern SCODE IMAPIStatus_SettingsDialog_Scodes[];
extern SCODE IMAPIStatus_ValidateState_Scodes[];

extern SCODE IMSProvider_Logon_Scodes[];
extern SCODE IMSProvider_Deinit_Scodes[];
extern SCODE IMSProvider_Init_Scodes[];
extern SCODE IMSProvider_SpoolerLogon_Scodes[];
extern SCODE IMSProvider_Shutdown_Scodes[];

extern SCODE SMAPI_MAPILogon_Scodes[];
extern SCODE SMAPI_MAPILogoff_Scodes[];
extern SCODE SMAPI_MAPIFreeBuffer_Scodes[];
extern SCODE SMAPI_MAPISendMail_Scodes[];
extern SCODE SMAPI_MAPISendDocuments_Scodes[];
extern SCODE SMAPI_MAPIFindNext_Scodes[];
extern SCODE SMAPI_MAPIReadMail_Scodes[];
extern SCODE SMAPI_MAPISaveMail_Scodes[];
extern SCODE SMAPI_MAPIDeleteMail_Scodes[];
extern SCODE SMAPI_MAPIAddress_Scodes[];
extern SCODE SMAPI_MAPIResolveName_Scodes[];
extern SCODE SMAPI_MAPIDetails_Scodes[];

extern SCODE IMSLogon_OpenEntry_Scodes[];
extern SCODE IMSLogon_OpenStatusEntry_Scodes[];
extern SCODE IMSLogon_CompareEntryIDs_Scodes[];
extern SCODE IMSLogon_Advise_Scodes[];
extern SCODE IMSLogon_Unadvise_Scodes[];
extern SCODE IMSLogon_Logoff_Scodes[];
#endif

/* ------------------------------------------------------------------------ */

#endif
