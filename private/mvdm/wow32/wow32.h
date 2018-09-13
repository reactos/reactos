/*++ BUILD Version: 0003
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOW32.H
 *  WOW32 16-bit API support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Changed 12-May-1992 by Mike Tricker (MikeTri) Added MultiMedia header includes
 *  Changed 30-Jul-1992 by Mike Tricker (MikeTri) Removed all Multimedia includes
--*/
#ifndef _DEF_WOW32_   // if this hasn't already been included
#define _DEF_WOW32_


#define HACK32

#if DBG
#define DEBUG 1
#endif

#ifdef i386
     #define PMODE32
     #define FASTCALL _fastcall
#else
     #define FASTCALL
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winuserp.h>
#include <shellapi.h>
#include <tsappcmp.h>

/***** ifdef( DEBUG || WOWPROFILE ) *****/
#ifdef DEBUG
#ifndef WOWPROFILE
#define WOWPROFILE   // DEBUG => WOWPROFILE
#endif // !WOWPROFILE
#endif // DEBUG

#ifdef WOWPROFILE
#ifndef DEBUG_OR_WOWPROFILE
#define DEBUG_OR_WOWPROFILE
#endif
#endif // WOWPROFILE


#include <wow.h>

#include "walias.h"
#include "wstruc.h"
#include "wheap.h"
#include "wowcmpat.h"


//
// Enable warnings that are turned off by sdk\inc\warning.h but we want on
//
#pragma warning(error:4101)   // Unreferenced local variable


/* Constants
 */
#define CIRC_BUFFERS        100             // Number of Saved in Circular Buffer for debug logging only
#define TMP_LINE_LEN        200             // maxlen of Circular Buffer strings
#define FILTER_FUNCTION_MAX 10              // Number of Calls you can filter on

#define WOWPRIVATEMSG   0x00010000  // this gets OR'd into certain 16-bit msg's
                                    // for handling special message cases

#define CLR_BLACK       0x00000000
#define CLR_RED         0x007F0000
#define CLR_GREEN       0x00007F00
#define CLR_BROWN       0x007F7F00
#define CLR_BLUE        0x0000007F
#define CLR_MAGENTA     0x007F007F
#define CLR_CYAN        0x00007F7F
#define CLR_LT_GRAY     0x00BFBFBF

#define CLR_DK_GRAY     0x007F7F7F
#define CLR_BR_RED      0x00FF0000
#define CLR_BR_GREEN        0x0000FF00
#define CLR_YELLOW      0x00FFFF00
#define CLR_BR_BLUE     0x000000FF
#define CLR_BR_MAGENTA      0x00FF00FF
#define CLR_BR_CYAN     0x0000FFFF
#define CLR_WHITE       0x00FFFFFF

#define WM_CTLCOLOR     0x0019

#define WM_WOWDESTROYCLIPBOARD  0x0
#define WM_WOWSETCBDATA     0x0

#define WOWVDM          TRUE


/*                        DO NOT CHANGE THE SIZES OF THESE TABLES WITHOUT
**                        CHANGING I386\FASTWOW.ASM!
*/
typedef struct _PA32 {
    PW32 lpfnA32;       // Array Address
#ifdef DEBUG_OR_WOWPROFILE
    LPSZ    lpszW32;    // Table Name (DEBUG version only)
    INT *lpiFunMax;     // Pointer # of table entries (DEBUG version only)
#endif // DEBUG_OR_WOWPROFILE
} PA32, *PPA32;




#ifdef DEBUG_OR_WOWPROFILE
#define  W32FUN(fn,name,mod,size)   fn,name,size,0L,0L
#define  W32MSGFUN(fn,name)         fn,name,0L,0L
#else              // non-profile RETAIL
#define  W32FUN(fn,name,mod,size)   fn
#define  W32MSGFUN(fn,name)         fn
#endif



#ifdef DEBUG_OR_WOWPROFILE
#define W32TAB(fn,name,size)    fn,name,&size
#else  // RETAIL ONLY
#define W32TAB(fn,name,size)    fn
#endif


/* Per-thread data
 */
#define CURRENTPTD()        ((PTD)(NtCurrentTeb()->WOW32Reserved))
#define PTEBTOPTD(pteb)     ((PTD)((pteb)->WOW32Reserved))


//
// Internal flags used in the COMMDLGTD Flags element
//

#define WOWCD_ISCHOOSEFONT 1
#define WOWCD_ISOPENFILE   2
#define WOWCD_NOSSYNC      4

//
// Used for COMMDLG thunk support
//

typedef struct _COMMDLGTD {
    HWND16  hdlg;              // hwnd of dialog & hwndOwner for Find/Replace 
    VPVOID  vpData;            // vp to 16-bit struct passed to ComDlg API
    PVOID   pData32;           // ptr to 32-bit ANSI version of above struct
    VPVOID  vpfnHook;          // vp to 16-bit hook proc specified by app
    union {
        VPVOID  vpfnSetupHook; // vp to 16-bit hook proc (print setup only)
        PVOID   pRes;          // ptr to 16-bit template resource
    };
    HWND16  SetupHwnd;            // for Print Setup Dialogs only
    struct  _COMMDLGTD *Previous; // for Find/Replace & nested dlg situations
    ULONG   Flags;
} COMMDLGTD, *PCOMMDLGTD;

//
// WOAINST
//

typedef struct _WOAINST {
    struct _WOAINST *pNext;
    struct _TD      *ptdWOA;            // TD of associated WinOldAp task
    DWORD            dwChildProcessID;
    HANDLE           hChildProcess;
    CHAR             szModuleName[1];   // As provided to LoadModule
} WOAINST, *PWOAINST;


//
// TD.dwFlags bit definitions
//

// #define TDF_INITCALLBACKSTACK  0x00000001  // no longer needed
#define TDF_IGNOREINPUT        0x00000002
#define TDF_FORCETASKEXIT      0x00000004
#define TDF_TASKCLEANUPDONE    0x00000008

// NOTE:  vpCBStack must not be referenced outside of CallBack16(), 
//        stackalloc16(), & stackfree16()!!!! 
//        See NOTES in walloc16.c\stackalloc16()
typedef struct _TD {           /* td */
    VPVOID      vpStack;           // 16-bit stack  MUST BE FIRST!!!
    VPVOID      vpCBStack;         // 16-bit callback frame (see NOTE above)
    DWORD       FastWowEsp;        // offset must match private\inc\vdmtib.inc
    PCOMMDLGTD  CommDlgTd;         // offset must match mvdm\inc\wowtd.h
    struct _TD *ptdNext;           // Pointer to Next PTD
    DWORD       dwFlags;           // TDF_ values above
    INT         VDMInfoiTaskID;    // SCS Task ID != 0 if task Exec'd form 32 bit program
    DWORD       dwWOWCompatFlags;  // WOW Compatibility flags
    DWORD       dwWOWCompatFlagsEx;// Extended WOW Compatibility flags
#ifdef FE_SB
    DWORD       dwWOWCompatFlags2; // Extended WOW Compatibility flags2
#endif // FE_SB
    DWORD       dwThreadID;        // ID of the thread
    HANDLE      hThread;           // Thread Handle
    HHOOK       hIdleHook;         // Hook handle for USER idle notification
    HRGN        hrgnClip;          // used by GetClipRgn()
    ULONG       ulLastDesktophDC;  // remembers last desktop DC for GetDC(0)
    INT         cStackAlloc16;     // for tracking stackalloc16() memory alloc's
    PWOAINST    pWOAList;          // One per active winoldap child
    HAND16      htask16;           // 16-bit kernel task handle - unique across VDMs
    HAND16      hInst16;           // 16-bit instance handle for this task
    HAND16      hMod16;            // 16-bit module handle for this task
    CRITICAL_SECTION csTD;         // protects this particular TD, esp. WOA list
} TD, *PTD;


/* Options (for flOptions)
 *
 * Bits 0-15 are RESERVED for use by x86,
 * so it must match the x86 definition, if any! -JTP
 */
#define OPT_DEBUG   0x00008 // shadow all log output on debug terminal (/d)
#define OPT_BREAKONNEWTASK 0x00010 // breakpoint on new task start
#define OPT_DONTPATCHCODE 0x00020 // doesnt patch wcallid with lpfnw32
#define OPT_DEBUGRETURN 0x10000 // convert next WOW16 return to debug return
#define OPT_FAKESUCCESS 0x20000 // convert selected failures into successes

/* Logging Filtering Options (fLogFilter)
 *
 * To Log all output set fLogFilter = -1
 */

#define FILTER_KERNEL   0x00000001
#define FILTER_USER     0x00000002
#define FILTER_GDI      0x00000004
#define FILTER_KEYBOARD 0x00000008
#define FILTER_SOUND    0x00000010
#define FILTER_KERNEL16 0X00000020
#define FILTER_MMEDIA   0x00000040
#define FILTER_WINSOCK  0x00000080
#define FILTER_VERBOSE  0x00000100
#define FILTER_COMMDLG  0x00000200
#ifdef FE_IME
#define FILTER_WINNLS   0x00000400
#endif
#ifdef FE_SB
#define FILTER_WIFEMAN  0x00000800
#endif

/* Global data
 */
#ifdef DEBUG
extern UCHAR  gszAssert[256]; // Buffer for assertion text (could be eliminated with restructuring)
int _cdecl sprintf_gszAssert(PSZ pszFmt, ...);
extern HANDLE hfLog;        // log file handle, if any
#endif
extern INT    flOptions;    // command-line options (see OPT_*)
#ifdef DEBUG
extern INT    iLogLevel;    // logging level;  0 implies none
extern INT    fDebugWait;   // Single Step; 0 = No Single Step
#endif
extern HANDLE hHostInstance;
#ifdef DEBUG
extern INT    fLogFilter;   // Filter Catagories of Functions
extern WORD   fLogTaskFilter;   // Filter Specific TaskID only
#endif
#ifdef i386
extern PX86CONTEXT pIntelRegisters; // x86 Only - Pointer to Intel Register Block
#endif

#ifdef DEBUG
extern INT    iReqLogLevel;         // Current Output LogLevel
extern INT    iCircBuffer;          // Current Buffer
extern CHAR   achTmp[CIRC_BUFFERS][TMP_LINE_LEN];    // Circular Buffer
extern WORD   awfLogFunctionFilter[FILTER_FUNCTION_MAX]; // Specific Filter API Array
extern INT    iLogFuncFiltIndex;        // Index Into Specific Array for Debugger Extensions
#endif


/* WOW global data
 */
extern UINT   iW32ExecTaskId;   // Base Task ID of Task Being Exec'd
extern UINT   nWOWTasks;    // # of WOW tasks running
extern BOOL   fBoot;        // TRUE During Boot Process
extern HANDLE  ghevWaitCreatorThread; // Used to Syncronize creation of a new thread
extern BOOL   fWowMode;     // see comment in wow32.c
extern HANDLE hWOWHeap;
extern DECLSPEC_IMPORT BOOL fSeparateWow;   // imported from ntvdm, FALSE if shared WOW VDM.
extern HANDLE ghProcess;       // WOW Process Handle
extern PFNWOWHANDLERSOUT pfnOut; // USER secret API pointers
extern DECLSPEC_IMPORT DWORD FlatAddress[];    // Base address of each selector in LDT
extern DECLSPEC_IMPORT LPDWORD SelectorLimit;  // Limit of each selector in LDT (x86 only)
extern PTD *  pptdWOA;
extern PTD    gptdShell;
extern char szWINFAX[];
extern char szINSTALL[];
extern char szModem[];
extern char szWINFAXCOMx[];
extern BOOL gbWinFaxHack;
extern char szEmbedding[];
extern char szServerKey[];
extern char szPicture[];
extern char szPostscript[];
extern char szZapfDingbats[];
extern char szZapf_Dingbats[];
extern char szSymbol[];
extern char szTmsRmn[];
extern char szHelv[];
extern char szMavisCourier[];
extern char szDevices[];
extern char szBoot[];
extern char szShell[];
extern char szWinDotIni[];
extern char szSystemDotIni[];
extern char szExplorerDotExe[];
extern PSTR pszWinIniFullPath;
extern PSTR pszWindowsDirectory;
extern PSTR pszSystemDirectory;
extern BOOL gfIgnoreInputAssertGiven;
#ifdef FE_SB
extern char szSystemMincho[];
extern char szMsMincho[];
#endif
extern DWORD dwSharedWowTimeout;
extern DWORD gpfn16GetProcModule;



#ifndef _X86_
extern PUCHAR IntelMemoryBase;  // Start of emulated CPU's memory
#define pNtVDMState   ((ULONG *)(IntelMemoryBase+FIXED_NTVDMSTATE_LINEAR))
#endif


/* WOW32 assertion/warning macros
 *
 * Take care where you put ASSERTs and where you put VERIFYs;  ASSERT
 * expressions go away in the retail product, VERIFYs don't, so if an essential
 * calculation or function call is taking place, put it in WOW32VERIFY().
 *
 * WOW32ASSERT(exp)  - prints module and line number and breakpoints
 * WOW32VERIFY(exp)  - like WOW32ASSERT but expression evaluated on free build
 * WOW32ASSERTMSG(exp, msg) - print the string and breakpoint
 * WOW32ASSERTMSGF(exp, (fmt, args...)) - print the formatted string and
 *                                        breakpoint
 * WOW32WARNMSG(exp, msg) - print the string but don't breakpoint
 * WOW32WARNMSGF(exp, (fmt, args, ...)) - print the formatted string but don't
 *                                        breakpoint
 * WOW32APIWARN(exp, msg) - specific to API thunks, msg must be API name,
 *                          does not breakpoint at all.
 */

#define EXCEPTION_WOW32_ASSERTION   0x9898

#ifdef DEBUG
#undef  MODNAME
#define MODNAME(module)     static char szModule[] = __FILE__

int DoAssert(PSZ szAssert, PSZ szModule, UINT line, UINT loglevel);

#define WOW32ASSERT(exp)                                                     \
{                                                                            \
    if (!(exp))                                                              \
    {                                                                        \
        DoAssert(NULL, szModule, __LINE__, LOG_ALWAYS);                      \
    }                                                                        \
}

#define WOW32VERIFY(exp)    WOW32ASSERT(exp)

#define WOW32ASSERTMSG(exp,msg)                                              \
{                                                                            \
    if (!(exp)) {                                                            \
        DoAssert(msg, szModule, __LINE__, LOG_ALWAYS);                       \
    }                                                                        \
}

#define WOW32ASSERTMSGF(exp, printf_args)                                    \
(                                                                            \
    (!(exp)) ? (                                                             \
        sprintf_gszAssert printf_args,                                       \
        DoAssert(gszAssert, szModule, __LINE__, LOG_ALWAYS)                  \
    ) : 0                                                                    \
)

#define WOW32WARNMSG(exp,msg)                                                \
{                                                                            \
    if (!(exp)) {                                                            \
        LOGDEBUG(LOG_ALWAYS, ("%s", (msg)));                                 \
    }                                                                        \
}

#define WOW32WARNMSGF(exp, printf_args)                                      \
{                                                                            \
    if (!(exp)) {                                                            \
        LOGDEBUG(LOG_ALWAYS, printf_args);                                   \
    }                                                                        \
}

#define WOW32APIWARN(exp,msg)                                                \
{                                                                            \
    if (!(exp)) {                                                            \
        LOGDEBUG(1,("    WOW32 WARNING: %s failed", (msg)));                 \
        if (flOptions & OPT_FAKESUCCESS) {                                   \
            LOGDEBUG(1,(" (but returning fake success)\n"));                 \
            (ULONG)exp = TRUE;                                               \
        }                                                                    \
        else {                                                               \
            LOGDEBUG(1,("\n"));                                              \
        }                                                                    \
    }                                                                        \
}

#else
#undef  MODNAME
#define MODNAME(module)
#define WOW32ASSERT(exp)
#define WOW32VERIFY(exp) (exp)
#define WOW32ASSERTMSG(exp,msg)
#define WOW32ASSERTMSGF(exp,msg)
#define WOW32WARNMSG(exp,msg)
#define WOW32WARNMSGF(exp,msg)
#define WOW32APIWARN(exp,msg)
#endif

#ifdef DEBUG
#define LOGARGS(l,v)    logargs(l,v)
#else
#define LOGARGS(l,v)
#endif

#ifdef DEBUG
#define LOGRETURN(l,v,r)    logreturn(l,v,r)
#else
#define LOGRETURN(l,v,r)
#endif

//
// Macros used to eliminate compiler warning generated when formal
// parameters or local variables are not declared.
//
// Use DBG_UNREFERENCED_PARAMETER() when a parameter is not yet
// referenced but will be once the module is completely developed.
//
// Use DBG_UNREFERENCED_LOCAL_VARIABLE() when a local variable is not yet
// referenced but will be once the module is completely developed.
//
// Use UNREFERENCED_PARAMETER() if a parameter will never be referenced.
//
// DBG_UNREFERENCED_PARAMETER and DBG_UNREFERENCED_LOCAL_VARIABLE will
// eventually be made into a null macro to help determine whether there
// is unfinished work.
//

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)          (P)
#define DBG_UNREFERENCED_PARAMETER(P)      (P)
#define DBG_UNREFERENCED_LOCAL_VARIABLE(V) (V)
#endif

#define SIZE_BOGUS      256

#define SIZETO64K(s)        (s?(INT)s:(INT)(64*K))  // return 64K if zero

#define CHAR32(b)       ((CHAR)(b))
#define BYTE32(b)       ((BYTE)(b))
#define INT32(i)        ((INT)(INT16)(i))
#define UINT32(i)       ((unsigned int)(i))
#define BOOL32(f)       ((BOOL)(f))
#define WORD32(w)       ((WORD)(w))
#define LONG32(l)       FETCHLONG(l)
#define DWORD32(dw)     FETCHDWORD(dw)
#define VPFN32(fn)      FETCHDWORD(fn)
#define INT32DEFAULT(i)     ((WORD)i==(WORD)CW_USEDEFAULT16?(UINT)(WORD)i:INT32(i))

#define GETBYTE16(v)        (v)
#define GETINT16(v)     ((INT16)(v))
#define GETBOOL16(v)        ((BOOL16)(v))
#define GETWORD16(v)        (v)
#define GETLONG16(v)        (v)
#define GETDWORD16(v)       (v)
#define GETUINT16(v)        ((WORD)(v))


#define ATOM32(a16)     (a16)           // bogus
#define PROC32(vpfn16)      ((PROC)FETCHDWORD(vpfn16))
#define NPSTR32(np16)       ((NPSTR)(np16))     // bogus

#define GETATOM16(v)        (v)         // bogus
#define GETPROC16(v)        ((ULONG)(v))        // bogus
#define GETWNDPROC16(v)     ((ULONG)(v))        // bogus
#define GETNPSTRBOGUS(v)    ((ULONG)(INT)(v))   // bogus
#define GETLPSTRBOGUS(v)    ((ULONG)(v))        // bogus
#define GETLPWORDBOGUS(v)   ((ULONG)(v))        // bogus


/* Simulator wrapper macros
 */
#ifndef _X86_  // emulated CPU
#define VDMSTACK()      (((ULONG)getSS()<<16)|getSP())
#define SETVDMSTACK(vp)     {setSS(HIW(vp)); setSP(LOW(vp));}
#else          // X86
#define VDMSTACK()      ((USHORT)pIntelRegisters->SegSs << 16 | (USHORT)pIntelRegisters->Esp)
#define SETVDMSTACK(vp)      pIntelRegisters->SegSs = HIW(vp); pIntelRegisters->Esp = LOW(vp);
#endif

// Use FlatAddress array exported by ntvdm instead of Sim32GetVDMPointer.

#ifndef _X86_
#define INTEL_MEMORY_BASE ((DWORD)IntelMemoryBase)
#else
#define INTEL_MEMORY_BASE (0)
#endif

#define GetPModeVDMPointerMacro(Address, Count)                               \
    (                                                                         \
        FlatAddress[(Address) >> 19]                                          \
        ? (void *)(FlatAddress[(Address) >> 19] + ((Address) & 0xFFFF))       \
        : NULL                                                                \
    )

#define SetPModeVDMPointerBase(Selector, Base)                                \
    {                                                                         \
        FlatAddress[Selector >> 3] = Base;                                    \
    }                                                                         \


#define GetRModeVDMPointer(Address)                                           \
        (void *)(INTEL_MEMORY_BASE + (((Address) & 0xFFFF0000) >> 12) +       \
                 ((Address) & 0xFFFF))

#ifdef DEBUG
    PVOID FASTCALL GetPModeVDMPointerAssert(DWORD Address, DWORD Count);
    #define GetPModeVDMPointer(vp, count) GetPModeVDMPointerAssert((vp), (count))
#else
    PVOID FASTCALL GetPModeVDMPointerAssert(DWORD Address);
    #define GetPModeVDMPointer(vp, count) GetPModeVDMPointerAssert((vp))
#endif


#define SEGPTR(seg,off)     GetPModeVDMPointer(((ULONG)seg<<16)|off, 0)
#define FRAMEPTR(vp)        ((PVDMFRAME)GetPModeVDMPointer(vp, 0))
#define CBFRAMEPTR(vp)      ((PCBVDMFRAME)GetPModeVDMPointer(vp, 0))

#define GETFRAMEPTR(vp,p)   {p=FRAMEPTR(vp); }
#define GETARGPTR(p,cb,parg)  parg=(PVOID)((ULONG)p+OFFSETOF(VDMFRAME,bArgs));

#define VDMPTR(vp,cb)       (PVOID)GetPModeVDMPointer(FETCHDWORD(vp),(cb))
#define GETVDMPTR(vp,cb,p)  ((p)=VDMPTR((vp),(cb)))
#define GETOPTPTR(vp,cb,p)  {(p)=NULL; if (FETCHDWORD(vp)) GETVDMPTR(vp,cb,p);}
#define GETSTRPTR(vp,cb,p)  {GETVDMPTR(vp,cb,p); LOGDEBUG(11,("        String @%08lx: \"%.*s\"\n",vp,min((cb),80),(p)));}
#define GETVARSTRPTR(vp,cb,p)   {GETVDMPTR(vp,(((cb)==-1)?1:(cb)),p); LOGDEBUG(11,("        String @%08lx: \"%.*s\"\n",(vp),min(((cb)==-1)?strlen(p):(cb),80),(p)));}
#define GETPSZPTR(vp,p)     {GETOPTPTR(vp,1,p);  LOGDEBUG(11,("        String @%08lx: \"%.80s\"\n",(FETCHDWORD(vp)),(p)));}
#define GETPSZPTRNOLOG(vp,p)    GETOPTPTR(vp,1,p)
#define GETPSZIDPTR(vp,p)   {p=(LPSZ)FETCHDWORD(vp); if (HIW16(vp)) GETPSZPTR(vp,p);}
#define GETMISCPTR(vp,p)    GETOPTPTR(vp,1,p)   // intended for non-string variable-length pointers
#define ALLOCVDMPTR(vp,cb,p)    GETVDMPTR(vp,cb,p)  // intended for output-only pointers

//
// Macros to "flush" VDM pointers after modifying 16-bit memory.
// Use FLUSHVDMCODEPTR when the 16-bit memory contains x86 code.
// Use FLUSHVDMPTR when the 16-bit memory does not contain x86 code.
//
// On x86, these macros are NOPs.  On RISC, FLUSHVDMPTR is a NOP, while
// FLUSHVDMCODEPTR actually calls the emulator so it can recompile any
// code affected.
//

#define FLUSHVDMCODEPTR(vp,cb,p) Sim32FlushVDMPointer( (vp), (USHORT)(cb), (PBYTE)(p), (fWowMode))
//#define FLUSHVDMPTR(vp,cb,p)     TRUE          // BUGBUG! davehart
#define FLUSHVDMPTR(vp,cb,p)     FLUSHVDMCODEPTR(vp,cb,p)

#define LOG_ALWAYS          0x00
#define LOG_ERROR           0x01
#define LOG_IMPORTANT       LOG_ERROR
#define LOG_WARNING         0x02
#define LOG_TRACE           0x04
#define LOG_PRIVATE         0x08
#define LOG_API             0x10
#define LOG_MSG             0x20
#define LOG_CALLBACK        0x40
#define LOG_STRING          0x80


#ifndef i386
#ifdef DEBUG
static CHAR *pszLogNull = "<null>";
#undef  GETPSZPTR
#define GETPSZPTR(vp,p)         {GETOPTPTR(vp,0,p);  LOGDEBUG(11,("        String @%08lx: \"%.80s\"\n",(FETCHDWORD(vp)),p ? p : pszLogNull));}
#endif
#endif

#ifndef DEBUG
#define FREEARGPTR(p)
#define FREEOPTPTR(p)
#define FREESTRPTR(p)
#define FREEPSZPTR(p)
#define FREEPSZIDPTR(p)
#define FREEMISCPTR(p)
#define FREEVDMPTR(p)
#define FREEOPTPTR(p)
#else
#define FREEARGPTR(p)       p=NULL
#define FREEOPTPTR(p)       p=NULL
#define FREESTRPTR(p)       p=NULL
#define FREEPSZPTR(p)       p=NULL
#define FREEPSZIDPTR(p)     p=NULL
#define FREEMISCPTR(p)      p=NULL
#define FREEVDMPTR(p)       p=NULL
#define FREEOPTPTR(p)       p=NULL
#endif

#define RETURN(ul)      return ul


#ifdef DBCS // MUST fix for FE NT
#define FIX_318197_NOW
#endif


#ifdef FIX_318197_NOW 

#define WOW32_strupr(psz)             CharUpperA(psz)
#define WOW32_strlwr(psz)             CharLowerA(psz)
#define WOW32_strcmp(psz1, psz2)      lstrcmpA(psz1, psz2)
#define WOW32_stricmp(psz1, psz2)     lstrcmpiA(psz1, psz2)
#define WOW32_strncpy(psz1, psz2, n)  lstrcpyn(psz1, psz2, n)

char* WOW32_strchr(const char* psz, int c);
char* WOW32_strrchr(const char* psz, int c);
char* WOW32_strstr(const char* str1, const char* str2);
int   WOW32_strncmp(const char* str1, const char* str2, size_t n);
int   WOW32_strnicmp(const char* str1, const char* str2, size_t n);

#else

#define WOW32_strupr(psz)             _strupr(psz)
#define WOW32_strlwr(psz)             _strlwr(psz)
#define WOW32_strcmp(psz1, psz2)      strcmp(psz1, psz2)
#define WOW32_stricmp(psz1, psz2)     _stricmp(psz1, psz2)
#define WOW32_strncpy(psz1, psz2, n)  strncpy(psz1, psz2, n)

#define WOW32_strchr(psz,c)           strchr(psz,c)
#define WOW32_strrchr(psz,c)          strrchr(psz,c)
#define WOW32_strstr(psz1, psz2)      strstr(psz1, psz2)
#define WOW32_strncmp(psz1, psz2, n)  strncmp(psz1, psz2, n)
#define WOW32_strnicmp(psz1, psz2, n) _strnicmp(psz1, psz2, n)

#endif




/* Function prototypes
 */
BOOL    W32Init(VOID);
VOID    W32Dispatch(VOID);
INT     W32Exception(DWORD dwException, PEXCEPTION_POINTERS pexi);
BOOLEAN W32DllInitialize(PVOID DllHandle,ULONG Reason,PCONTEXT Context);
BOOL IsDebuggerAttached(VOID);

ULONG FASTCALL   WK32WOWGetFastAddress( PVDMFRAME pFrame );
ULONG FASTCALL   WK32WOWGetFastCbRetAddress( PVDMFRAME pFrame );
ULONG FASTCALL   WK32WOWGetTableOffsets( PVDMFRAME pFrame );
ULONG FASTCALL   WK32WOWGetFlatAddressArray( PVDMFRAME pFrame );
PTD     ThreadProcID32toPTD(DWORD ThreadID, DWORD dwProcessID);
PTD     Htask16toPTD( HAND16 );
HTASK16 ThreadID32toHtask16(DWORD ThreadID32);
PVOID   WOWStartupFailed(VOID);
LPSTR  ThunkStr16toStr32(LPSTR pdst32, VPVOID vpsrc16, int cChars, BOOL bMulti);

#ifdef DEBUG
VOID    logprintf(PSZ psz, ...);
VOID    logargs(INT iLog, PVDMFRAME pFrame);
VOID    logreturn(INT iLog, PVDMFRAME pFrame, ULONG ulReturn);
BOOL    checkloging(register PVDMFRAME pFrame);
#endif

#ifdef DEBUG_OR_WOWPROFILE
DWORD   GetWOWTicDiff(DWORD dwPrevCount);
INT     GetFuncId(DWORD iFun);
#endif

BOOL    IsDebuggerAttached(VOID);

//
// Thunk table stub functions and aliases.
//

ULONG FASTCALL   WOW32UnimplementedAPI(PVDMFRAME pFrame);
ULONG FASTCALL   WOW32Unimplemented95API(PVDMFRAME pFrame);

// for tracking memory leaks
#ifdef DEBUG
#define DEBUG_MEMLEAK 1
#else  // non-DEBUG
#ifdef MEMLEAK
#define DEBUG_MEMLEAK 1
#endif // MEMLEAK
#endif // DEBUG

#ifdef DEBUG_MEMLEAK
VOID  WOW32DebugMemLeak(PVOID lp, ULONG size, DWORD fHow);
VOID  WOW32DebugReMemLeak(PVOID lpNew, PVOID lpOrig, ULONG size, DWORD fHow);
VOID  WOW32DebugFreeMem(PVOID lp);
VOID  WOW32DebugCorruptionCheck(PVOID lp, DWORD size);
DWORD WOW32DebugGetMemSize(PVOID lp);
HGLOBAL WOW32DebugGlobalAlloc(UINT flags, DWORD dwSize);
HGLOBAL WOW32DebugGlobalReAlloc(HGLOBAL h32, DWORD dwSize, UINT flags);
HGLOBAL WOW32DebugGlobalFree(HGLOBAL h32);
#define WOWGLOBALALLOC(f,s)        WOW32DebugGlobalAlloc(f,(s))
#define WOWGLOBALREALLOC(h,s,f)    WOW32DebugGlobalReAlloc(h,(s),f)
#define WOWGLOBALFREE(h)           WOW32DebugGlobalFree(h)
#define ML_MALLOC_W      0x00000001
#define ML_MALLOC_W_ZERO 0x00000002
#define ML_REALLOC_W     0x00000004
#define ML_MALLOC_WTYPE  (ML_MALLOC_W | ML_MALLOC_W_ZERO | ML_REALLOC_W)
#define ML_GLOBALALLOC   0x00000010
#define ML_GLOBALREALLOC 0x00000020
#define ML_GLOBALTYPE    (ML_GLOBALREALLOC | ML_GLOBALALLOC)
#define TAILCHECK        (4 * sizeof(CHAR))  // for heap tail corruption check
typedef struct _tagMEMLEAK {
    struct _tagMEMLEAK *lpmlNext;
    PVOID               lp;
    DWORD               size;
    UINT                fHow;
    ULONG               Count;
    PVOID               CallersAddress;
} MEMLEAK, *LPMEMLEAK;
#else  // non-DEBUG_MEMLEAK
#define TAILCHECK                  0
#define WOWGLOBALALLOC(f,s)        GlobalAlloc(f,(s))
#define WOWGLOBALREALLOC(h,f,s)    GlobalReAlloc(h, f,(s))
#define WOWGLOBALFREE(h)           GlobalFree(h)
#endif // DEBUG_MEMLEAK

#ifdef DEBUG
    ULONG FASTCALL   WOW32NopAPI(PVDMFRAME pFrame);
    ULONG FASTCALL   WOW32LocalAPI(PVDMFRAME pFrame);
    ULONG FASTCALL   WK32WowPartyByNumber(PVDMFRAME pFrame);

    #define LOCALAPI              WOW32LocalAPI
    #define NOPAPI                WOW32NopAPI
    #define UNIMPLEMENTEDAPI      WOW32UnimplementedAPI
    #define UNIMPLEMENTED95API    WOW32Unimplemented95API
    #define WK32WOWPARTYBYNUMBER  WK32WowPartyByNumber
#else
    #define LOCALAPI              WOW32UnimplementedAPI
    #define NOPAPI                WOW32UnimplementedAPI
    #define UNIMPLEMENTEDAPI      WOW32UnimplementedAPI
    #define UNIMPLEMENTED95API    WOW32UnimplementedAPI
    #define WK32WOWPARTYBYNUMBER  UNIMPLEMENTEDAPI
#endif

//Terminal Server
PTERMSRVCORINIFILE gpfnTermsrvCORIniFile;





#endif // ifndef _DEF_WOW32_  THIS SHOULD BE THE LAST LINE IN THIS FILE
