/*****************************************************************************\
*                                                                             *
* windows.h -   Windows functions, types, and definitions                     *
*                                                                             *
*               Version 4.00                                                  *
*                                                                             *
*               Copyright (c) 1985-1994, Microsoft Corp. All rights reserved. *
*                                                                             *
*******************************************************************************
*
* The following symbols control inclusion of various parts of this file:
*
* WINVER            Windows version number (0x0400).  To exclude
*                   definitions introduced in version 4.0 (or above)
*                   #define WINVER 0x0300
*                   or 0x0310 before #including <windows.h>
*
* #define:          To prevent inclusion of:
*
* NOKERNEL          KERNEL APIs and definitions
* NOGDI             GDI APIs and definitions
* NOUSER            USER APIs and definitions
* NOSOUND           Sound APIs and definitions
* NOCOMM            Comm driver APIs and definitions
* NODRIVERS         Installable driver APIs and definitions
* NOIMT             Installable messge thunk APIs and definitions
* NOMINMAX          min() and max() macros
* NOLOGERROR        LogError() and related definitions
* NOPROFILER        Profiler APIs
* NOMEMMGR          Local and global memory management
* NOLFILEIO         _l* file I/O routines
* NOOPENFILE        OpenFile and related definitions
* NORESOURCE        Resource management
* NOATOM            Atom management
* NOLANGUAGE        Character test routines
* NOLSTRING         lstr* string management routines
* NODBCS            Double-byte character set routines
* NOKEYBOARDINFO    Keyboard driver routines
* NOGDICAPMASKS     GDI device capability constants
* NOCOLOR           COLOR_* color values
* NOGDIOBJ          GDI pens, brushes, fonts
* NODRAWTEXT        DrawText() and related definitions
* NOTEXTMETRIC      TEXTMETRIC and related APIs
* NOSCALABLEFONT    Truetype scalable font support
* NOBITMAP          Bitmap support
* NORASTEROPS       GDI Raster operation definitions
* NOMETAFILE        Metafile support
* NOSYSMETRICS      GetSystemMetrics() and related SM_* definitions
* NOSYSTEMPARAMSINFO SystemParametersInfo() and SPI_* definitions
* NOMSG             APIs and definitions that use MSG structure
* NOWINSTYLES       Window style definitions
* NOWINOFFSETS      Get/SetWindowWord/Long offset definitions
* NOSHOWWINDOW      ShowWindow and related definitions
* NODEFERWINDOWPOS  DeferWindowPos and related definitions
* NOVIRTUALKEYCODES VK_* virtual key codes
* NOKEYSTATES       MK_* message key state flags
* NOWH              SetWindowsHook and related WH_* definitions
* NOMENUS           Menu APIs
* NOSCROLL          Scrolling APIs and scroll bar control
* NOCLIPBOARD       Clipboard APIs and definitions
* NOICONS           IDI_* icon IDs
* NOMB              MessageBox and related definitions
* NOSYSCOMMANDS     WM_SYSCOMMAND SC_* definitions
* NOMDI             MDI support
* NOCTLMGR          Control management and controls
* NOWINMESSAGES     WM_* window messages
* NOHELP            Help support
*
\****************************************************************************/

#ifndef _INC_WINDOWS
#define _INC_WINDOWS    /* #defined if windows.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/* If WINVER is not defined, assume version 4.0 */
#ifndef WINVER
#define WINVER  0x0400
#endif

#ifdef RC_INVOKED
/* Don't include definitions that RC.EXE can't parse */
#define NOATOM
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NORASTEROPS
#define NOSCROLL
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NODBCS
#define NOSYSTEMPARAMSINFO
#define NOCOMM
#define NOOEMRESOURCE
#endif  /* RC_INVOKED */

/* Handle OEMRESOURCE for 3.0 compatibility */
#if (WINVER < 0x030a)
#define NOOEMRESOURCE
#ifdef OEMRESOURCE
#undef NOOEMRESOURCE
#endif
#endif

/******* Common definitions and typedefs ***********************************/

#define VOID        void

#define FAR         _far
#define NEAR        _near
#define PASCAL      _pascal
#define CDECL       _cdecl
#define WINAPI      _far _pascal
#define CALLBACK    _far _pascal
#define WINCAPI     _far _cdecl

/****** Simple types & common helper macros *********************************/

typedef int             BOOL;
#define FALSE           0
#define TRUE            1

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;

typedef WORD            WCHAR;

typedef unsigned int    UINT;

#ifdef STRICT
typedef signed long     LONG;
#else
#define LONG            long
#endif

typedef short           SHORT;
typedef unsigned long   ULONG;

#define LOBYTE(w)       ((BYTE)(w))
#define HIBYTE(w)       ((BYTE)(((UINT)(w) >> 8) & 0xFF))

#define LOWORD(l)       ((WORD)(DWORD)(l))
#define HIWORD(l)       ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))

#define MAKELONG(low, high)     ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))

#ifndef NOMINMAX
#ifndef max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif
#endif  /* NOMINMAX */

/* Types use for passing & returning polymorphic values */
typedef UINT            WPARAM;
typedef LONG            LPARAM;
typedef LONG            LRESULT;

#define MAKELPARAM(low, high)   ((LPARAM)MAKELONG(low, high))
#define MAKELRESULT(low, high)  ((LRESULT)MAKELONG(low, high))

/****** Common pointer types ************************************************/

#ifndef NULL
#define NULL            0
#endif

typedef char NEAR*      PSTR;
typedef char NEAR*      NPSTR;

typedef char FAR*       LPSTR;
typedef const char FAR* LPCSTR;

typedef BYTE NEAR*      PBYTE;
typedef BYTE FAR*       LPBYTE;
typedef const BYTE FAR* LPCBYTE;

typedef int NEAR*       PINT;
typedef int FAR*        LPINT;

typedef WORD NEAR*      PWORD;
typedef WORD FAR*       LPWORD;

typedef long NEAR*      PLONG;
typedef long FAR*       LPLONG;

typedef DWORD NEAR*     PDWORD;
typedef DWORD FAR*      LPDWORD;

typedef void FAR*       LPVOID;

#define MAKELP(sel, off)    ((void FAR*)MAKELONG((off), (sel)))
#define SELECTOROF(lp)      HIWORD(lp)
#define OFFSETOF(lp)        LOWORD(lp)

#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)

/****** Common handle types *************************************************/

#ifdef STRICT
typedef const void NEAR*        HANDLE;
#define DECLARE_HANDLE(name)    struct name##__ { int unused; }; \
                                typedef const struct name##__ NEAR* name
#define DECLARE_HANDLE32(name)  struct name##__ { int unused; }; \
                                typedef const struct name##__ FAR* name
#else   /* STRICT */
typedef UINT                    HANDLE;
#define DECLARE_HANDLE(name)    typedef UINT name
#define DECLARE_HANDLE32(name)  typedef DWORD name
#endif  /* !STRICT */

typedef HANDLE*         PHANDLE;
typedef HANDLE NEAR*    SPHANDLE;
typedef HANDLE FAR*     LPHANDLE;

typedef HANDLE          HGLOBAL;
typedef HANDLE          HLOCAL;

typedef HANDLE          GLOBALHANDLE;
typedef HANDLE          LOCALHANDLE;

typedef UINT            ATOM;

#ifdef STRICT
typedef void (CALLBACK*     FARPROC)(void);
typedef void (NEAR PASCAL*  NEARPROC)(void);
#else
typedef int (CALLBACK*      FARPROC)();
typedef int (NEAR PASCAL*   NEARPROC)();
#endif

DECLARE_HANDLE(HSTR);

/****** KERNEL typedefs, structures, and functions **************************/

DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;  /* HMODULEs can be used in place of HINSTANCEs */

#ifndef NOKERNEL

/****** Application entry point function ************************************/

#ifdef STRICT
int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
#endif

/****** System Information **************************************************/

DWORD   WINAPI GetVersion(void);

DWORD   WINAPI GetFreeSpace(UINT);
UINT    WINAPI GetCurrentPDB(void);

UINT    WINAPI GetWindowsDirectory(LPSTR, UINT);
UINT    WINAPI GetSystemDirectory(LPSTR, UINT);

#if (WINVER >= 0x030a)
UINT    WINAPI GetFreeSystemResources(UINT);
#define GFSR_SYSTEMRESOURCES   0x0000
#define GFSR_GDIRESOURCES      0x0001
#define GFSR_USERRESOURCES     0x0002
#endif  /* WINVER >= 0x030a */

DWORD   WINAPI GetWinFlags(void);

#define WF_PMODE        0x0001
#define WF_CPU286       0x0002
#define WF_CPU386       0x0004
#define WF_CPU486       0x0008
#define WF_STANDARD     0x0010
#define WF_WIN286       0x0010
#define WF_ENHANCED     0x0020
#define WF_WIN386       0x0020
#define WF_CPU086       0x0040
#define WF_CPU186       0x0080
#if (WINVER < 0x030a)
#define WF_LARGEFRAME   0x0100
#define WF_SMALLFRAME   0x0200
#endif
#define WF_80x87        0x0400
#define WF_PAGING       0x0800
#define WF_DOSPAGING    0x1000
#define WF_WINNT        0x4000
#define WF_WLO          0x8000
#define WF_CPUMASK      0xFC000000
#define WF_CPU_X86	0
#define WF_CPU_R4000	1
#define WF_CPU_ALPHA	2
#define WF_CPU_CLIPPER	3


LPSTR   WINAPI GetDOSEnvironment(void);

DWORD   WINAPI GetCurrentTime(void);
DWORD   WINAPI GetTickCount(void);
DWORD   WINAPI GetTimerResolution(void);

/****** Error handling ******************************************************/

#if (WINVER >= 0x030a)
#ifndef NOLOGERROR

void    WINAPI LogError(UINT err, void FAR* lpInfo);
void    WINAPI LogParamError(UINT err, FARPROC lpfn, void FAR* param);

/****** LogParamError/LogError values */

/* Error modifier bits */

#define ERR_WARNING     0x8000
#define ERR_PARAM       0x4000

#define ERR_SIZE_MASK   0x3000
#define ERR_BYTE        0x1000
#define ERR_WORD        0x2000
#define ERR_DWORD       0x3000

/****** LogParamError() values */

/* Generic parameter values */
#define ERR_BAD_VALUE       0x6001
#define ERR_BAD_FLAGS       0x6002
#define ERR_BAD_INDEX       0x6003
#define ERR_BAD_DVALUE      0x7004
#define ERR_BAD_DFLAGS      0x7005
#define ERR_BAD_DINDEX      0x7006
#define ERR_BAD_PTR         0x7007
#define ERR_BAD_FUNC_PTR    0x7008
#define ERR_BAD_SELECTOR    0x6009
#define ERR_BAD_STRING_PTR  0x700a
#define ERR_BAD_HANDLE      0x600b

/* KERNEL parameter errors */
#define ERR_BAD_HINSTANCE       0x6020
#define ERR_BAD_HMODULE         0x6021
#define ERR_BAD_GLOBAL_HANDLE   0x6022
#define ERR_BAD_LOCAL_HANDLE    0x6023
#define ERR_BAD_ATOM            0x6024
#define ERR_BAD_HFILE           0x6025

/* USER parameter errors */
#define ERR_BAD_HWND            0x6040
#define ERR_BAD_HMENU           0x6041
#define ERR_BAD_HCURSOR         0x6042
#define ERR_BAD_HICON           0x6043
#define ERR_BAD_HDWP            0x6044
#define ERR_BAD_CID             0x6045
#define ERR_BAD_HDRVR           0x6046

/* GDI parameter errors */
#define ERR_BAD_COORDS          0x7060
#define ERR_BAD_GDI_OBJECT      0x6061
#define ERR_BAD_HDC             0x6062
#define ERR_BAD_HPEN            0x6063
#define ERR_BAD_HFONT           0x6064
#define ERR_BAD_HBRUSH          0x6065
#define ERR_BAD_HBITMAP         0x6066
#define ERR_BAD_HRGN            0x6067
#define ERR_BAD_HPALETTE        0x6068
#define ERR_BAD_HMETAFILE       0x6069

/**** LogError() values */

/* KERNEL errors */
#define ERR_GALLOC              0x0001
#define ERR_GREALLOC            0x0002
#define ERR_GLOCK               0x0003
#define ERR_LALLOC              0x0004
#define ERR_LREALLOC            0x0005
#define ERR_LLOCK               0x0006
#define ERR_ALLOCRES            0x0007
#define ERR_LOCKRES             0x0008
#define ERR_LOADMODULE          0x0009

/* USER errors */
#define ERR_CREATEDLG           0x0040
#define ERR_CREATEDLG2          0x0041
#define ERR_REGISTERCLASS       0x0042
#define ERR_DCBUSY              0x0043
#define ERR_CREATEWND           0x0044
#define ERR_STRUCEXTRA          0x0045
#define ERR_LOADSTR             0x0046
#define ERR_LOADMENU            0x0047
#define ERR_NESTEDBEGINPAINT    0x0048
#define ERR_BADINDEX            0x0049
#define ERR_CREATEMENU          0x004a

/* GDI errors */
#define ERR_CREATEDC            0x0080
#define ERR_CREATEMETA          0x0081
#define ERR_DELOBJSELECTED      0x0082
#define ERR_SELBITMAP           0x0083

/* Debugging support (DEBUG SYSTEM ONLY) */
typedef struct tagWINDEBUGINFO
{
    UINT    flags;
    DWORD   dwOptions;
    DWORD   dwFilter;
    char    achAllocModule[8];
    DWORD   dwAllocBreak;
    DWORD   dwAllocCount;
#if (WINVER >= 0x0400)
    WORD    chDefRIP;
#endif /* WINVER >= 0x0400 */
} WINDEBUGINFO;

BOOL    WINAPI GetWinDebugInfo(WINDEBUGINFO FAR* lpwdi, UINT flags);
BOOL    WINAPI SetWinDebugInfo(const WINDEBUGINFO FAR* lpwdi);

void    FAR _cdecl DebugOutput(UINT flags, LPCSTR lpsz, ...);

/* WINDEBUGINFO flags values */
#define WDI_OPTIONS             0x0001
#define WDI_FILTER              0x0002
#define WDI_ALLOCBREAK          0x0004
#define WDI_DEFRIP              0x0008

/* dwOptions values */
#define DBO_CHECKHEAP           0x0001
#define DBO_BUFFERFILL          0x0004
#define DBO_DISABLEGPTRAPPING   0x0010
#define DBO_CHECKFREE           0x0020

#define DBO_SILENT              0x8000

#define DBO_TRACEBREAK          0x2000
#define DBO_WARNINGBREAK        0x1000
#define DBO_NOERRORBREAK        0x0800
#define DBO_NOFATALBREAK        0x0400
#define DBO_INT3BREAK           0x0100

/* DebugOutput flags values */
#define DBF_TRACE               0x0000
#define DBF_WARNING             0x4000
#define DBF_ERROR               0x8000
#define DBF_FATAL               0xc000

/* dwFilter values */
#define DBF_KERNEL              0x1000
#define DBF_KRN_MEMMAN          0x0001
#define DBF_KRN_LOADMODULE      0x0002
#define DBF_KRN_SEGMENTLOAD     0x0004
#define DBF_USER                0x0800
#define DBF_GDI                 0x0400
#define DBF_MMSYSTEM            0x0040
#define DBF_PENWIN              0x0020
#define DBF_APPLICATION         0x0008
#define DBF_DRIVER              0x0010

#endif  /* NOLOGERROR */
#endif  /* WINVER >= 0x030a */

void    WINAPI FatalExit(int);
void    WINAPI FatalAppExit(UINT, LPCSTR);

BOOL    WINAPI ExitWindows(DWORD dwReturnCode, UINT wReserved);

#define EW_RESTARTWINDOWS 0x42

#if (WINVER >= 0x030a)
#define EW_REBOOTSYSTEM         0x43
BOOL    WINAPI ExitWindowsExec(LPCSTR, LPCSTR);
#endif  /* WINVER >= 0x030a */



void    WINAPI DebugBreak(void);
void    WINAPI OutputDebugString(LPCSTR);

/* SetErrorMode() constants */
#define SEM_FAILCRITICALERRORS  0x0001
#define SEM_NOGPFAULTERRORBOX   0x0002
#define SEM_NOOPENFILEERRORBOX  0x8000

UINT    WINAPI SetErrorMode(UINT);

#if (WINVER >= 0x0400)
void    WINAPI SetLastError(DWORD);
DWORD   WINAPI GetLastError(void);
#endif /* WINVER >= 0x0400 */


/****** Pointer validation **************************************************/

#if (WINVER >= 0x030a)

BOOL    WINAPI IsBadReadPtr(const void FAR* lp, UINT cb);
BOOL    WINAPI IsBadWritePtr(void FAR* lp, UINT cb);
BOOL    WINAPI IsBadHugeReadPtr(const void _huge* lp, DWORD cb);
BOOL    WINAPI IsBadHugeWritePtr(void _huge* lp, DWORD cb);
BOOL    WINAPI IsBadCodePtr(FARPROC lpfn);
BOOL    WINAPI IsBadStringPtr(const void FAR* lpsz, UINT cchMax);
#endif  /* WINVER >= 0x030a */

/****** Profiling support ***************************************************/

#ifndef NOPROFILER

int     WINAPI ProfInsChk(void);
void    WINAPI ProfSetup(int,int);
void    WINAPI ProfSampRate(int,int);
void    WINAPI ProfStart(void);
void    WINAPI ProfStop(void);
void    WINAPI ProfClear(void);
void    WINAPI ProfFlush(void);
void    WINAPI ProfFinish(void);

#if (WINVER >= 0x0400)
void    WINAPI ProfStart32(void);
void    WINAPI ProfStop32(void);
void    WINAPI ProfClear32(void);
void    WINAPI ProfFlush32(void);
void    WINAPI ProfFinish32(void);
#endif /* WINVER >= 0x0400) */
#endif  /* NOPROFILER */

/****** Catch/Throw and stack management ************************************/

typedef int CATCHBUF[9];
typedef int FAR* LPCATCHBUF;

int     WINAPI Catch(int FAR*);
void    WINAPI Throw(const int FAR*, int);

void    WINAPI SwitchStackBack(void);
void    WINAPI SwitchStackTo(UINT, UINT, UINT);

/****** Module Management ***************************************************/

#define HINSTANCE_ERROR ((HINSTANCE)32)

HINSTANCE   WINAPI LoadModule(LPCSTR, LPVOID);
BOOL        WINAPI FreeModule(HINSTANCE);

HINSTANCE   WINAPI LoadLibrary(LPCSTR);
void        WINAPI FreeLibrary(HINSTANCE);

UINT    WINAPI WinExec(LPCSTR, UINT);

HMODULE WINAPI GetModuleHandle(LPCSTR);

int     WINAPI GetModuleUsage(HINSTANCE);
int     WINAPI GetModuleFileName(HINSTANCE, LPSTR, int);

#if (WINVER >= 0x0400)
BOOL    WINAPI GetModuleName(HINSTANCE, LPSTR, int);
#endif /* WINVER >= 0x0400 */

FARPROC WINAPI GetProcAddress(HINSTANCE, LPCSTR);

int     WINAPI GetInstanceData(HINSTANCE, BYTE NEAR *, int);

HGLOBAL WINAPI GetCodeHandle(FARPROC);

typedef struct tagSEGINFO
{
    UINT    offSegment;
    UINT    cbSegment;
    UINT    flags;
    UINT    cbAlloc;
    HGLOBAL h;
    UINT    alignShift;
    UINT    reserved[2];
} SEGINFO;
typedef SEGINFO FAR* LPSEGINFO;

void    WINAPI GetCodeInfo(FARPROC lpProc, SEGINFO FAR* lpSegInfo);

FARPROC WINAPI MakeProcInstance(FARPROC, HINSTANCE);
void    WINAPI FreeProcInstance(FARPROC);

LONG    WINAPI SetSwapAreaSize(UINT);
void    WINAPI SwapRecording(UINT);
void    WINAPI ValidateCodeSegments(void);

/* Windows Exit Procedure flag values */
#define WEP_SYSTEM_EXIT 1
#define WEP_FREE_DLL 0

/****** Task Management *****************************************************/

#endif /* NOKERNEL */

DECLARE_HANDLE(HTASK);

#ifndef NOKERNEL

UINT    WINAPI GetNumTasks(void);

#if (WINVER >= 0x030a)
BOOL    WINAPI IsTask(HTASK);
#endif  /* WINVER >= 0x030a */

HTASK   WINAPI GetCurrentTask(void);

void    WINAPI Yield(void);
void    WINAPI DirectedYield(HTASK);


/****** Global memory management ********************************************/

#ifndef NOMEMMGR

/* Global Memory Flags */

#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)

#define GlobalDiscard(h)    GlobalReAlloc(h, 0L, GMEM_MOVEABLE)

HGLOBAL WINAPI GlobalAlloc(UINT, DWORD);
HGLOBAL WINAPI GlobalReAlloc(HGLOBAL, DWORD, UINT);
HGLOBAL WINAPI GlobalFree(HGLOBAL);

DWORD   WINAPI GlobalDosAlloc(DWORD);
UINT    WINAPI GlobalDosFree(UINT);

#ifdef STRICT
void FAR* WINAPI GlobalLock(HGLOBAL);
#else
char FAR* WINAPI GlobalLock(HGLOBAL);
#endif

BOOL    WINAPI GlobalUnlock(HGLOBAL);

DWORD   WINAPI GlobalSize(HGLOBAL);
DWORD   WINAPI GlobalHandle(UINT);

/* GlobalFlags return flags (in addition to GMEM_DISCARDABLE) */
#define GMEM_DISCARDED     0x4000
#define GMEM_LOCKCOUNT     0x00FF
UINT    WINAPI GlobalFlags(HGLOBAL);

#ifdef STRICT
void FAR* WINAPI GlobalWire(HGLOBAL);
#else
char FAR* WINAPI GlobalWire(HGLOBAL);
#endif

BOOL    WINAPI GlobalUnWire(HGLOBAL);

UINT    WINAPI GlobalPageLock(HGLOBAL);
UINT    WINAPI GlobalPageUnlock(HGLOBAL);

void    WINAPI GlobalFix(HGLOBAL);
void    WINAPI GlobalUnfix(HGLOBAL);

HGLOBAL WINAPI GlobalLRUNewest(HGLOBAL);
HGLOBAL WINAPI GlobalLRUOldest(HGLOBAL);

DWORD   WINAPI GlobalCompact(DWORD);

#ifdef STRICT
typedef BOOL (CALLBACK* GNOTIFYPROC)(HGLOBAL);
#else
typedef FARPROC GNOTIFYPROC;
#endif

void    WINAPI GlobalNotify(GNOTIFYPROC);

HGLOBAL WINAPI LockSegment(UINT);
void    WINAPI UnlockSegment(UINT);

#define LockData(dummy)     LockSegment((UINT)-1)
#define UnlockData(dummy)   UnlockSegment((UINT)-1)

UINT    WINAPI AllocSelector(UINT);
UINT    WINAPI FreeSelector(UINT);
UINT    WINAPI AllocDStoCSAlias(UINT);
UINT    WINAPI PrestoChangoSelector(UINT sourceSel, UINT destSel);
DWORD   WINAPI GetSelectorBase(UINT);
UINT    WINAPI SetSelectorBase(UINT, DWORD);
DWORD   WINAPI GetSelectorLimit(UINT);
UINT    WINAPI SetSelectorLimit(UINT, DWORD);

void    WINAPI LimitEmsPages(DWORD);

void    WINAPI ValidateFreeSpaces(void);

/***** Local Memory Management */

/* Local Memory Flags */
#define LMEM_FIXED          0x0000
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00

#define LHND                (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)

#define NONZEROLHND         (LMEM_MOVEABLE)
#define NONZEROLPTR         (LMEM_FIXED)

#define LocalDiscard(h)     LocalReAlloc(h, 0, LMEM_MOVEABLE)


HLOCAL  WINAPI LocalAlloc(UINT, UINT);
HLOCAL  WINAPI LocalReAlloc(HLOCAL, UINT, UINT);
HLOCAL  WINAPI LocalFree(HLOCAL);

#ifdef STRICT
void NEAR* WINAPI LocalLock(HLOCAL);
#else
char NEAR* WINAPI LocalLock(HLOCAL);
#endif

BOOL    WINAPI LocalUnlock(HLOCAL);

UINT    WINAPI LocalSize(HLOCAL);
#ifdef STRICT
HLOCAL  WINAPI LocalHandle(void NEAR*);
#else
HLOCAL  WINAPI LocalHandle(UINT);
#endif

/* LocalFlags return flags (in addition to LMEM_DISCARDABLE) */
#define LMEM_DISCARDED     0x4000
#define LMEM_LOCKCOUNT     0x00FF

UINT    WINAPI LocalFlags(HLOCAL);

BOOL    WINAPI LocalInit(UINT, UINT, UINT);
UINT    WINAPI LocalCompact(UINT);
UINT    WINAPI LocalShrink(HLOCAL, UINT);
#endif  /* NOMEMMGR */

/****** File I/O ************************************************************/

#ifndef NOLFILEIO

typedef int HFILE;      /* Polymorphic with C runtime file handle type */

#define HFILE_ERROR ((HFILE)-1)

#ifndef NOOPENFILE

/* OpenFile() Structure */
typedef struct tagOFSTRUCT
{
    BYTE cBytes;
    BYTE fFixedDisk;
    UINT nErrCode;
    BYTE reserved[4];
    char szPathName[128];
} OFSTRUCT;
typedef OFSTRUCT*       POFSTRUCT;
typedef OFSTRUCT NEAR* NPOFSTRUCT;
typedef OFSTRUCT FAR*  LPOFSTRUCT;

/* OpenFile() Flags */
#define OF_READ             0x0000
#define OF_WRITE            0x0001
#define OF_READWRITE        0x0002
#define OF_SHARE_COMPAT     0x0000
#define OF_SHARE_EXCLUSIVE  0x0010
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_DENY_READ  0x0030
#define OF_SHARE_DENY_NONE  0x0040
#define OF_PARSE            0x0100
#define OF_DELETE           0x0200
#define OF_VERIFY           0x0400  /* Used with OF_REOPEN */
#define OF_SEARCH           0x0400  /* Used without OF_REOPEN */
#define OF_CANCEL           0x0800
#define OF_CREATE           0x1000
#define OF_PROMPT           0x2000
#define OF_EXIST            0x4000
#define OF_REOPEN           0x8000

HFILE   WINAPI OpenFile(LPCSTR, OFSTRUCT FAR*, UINT);

#endif  /* NOOPENFILE */

/* _lopen() flags */
#define READ                0
#define WRITE               1
#define READ_WRITE          2

HFILE   WINAPI _lopen(LPCSTR, int);
HFILE   WINAPI _lcreat(LPCSTR, int);

HFILE   WINAPI _lclose(HFILE);

LONG    WINAPI _llseek(HFILE, LONG, int);

/* _llseek origin values */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

UINT    WINAPI _lread(HFILE, void _huge*, UINT);
UINT    WINAPI _lwrite(HFILE, const void _huge*, UINT);

#if (WINVER >= 0x030a)
long WINAPI _hread(HFILE, void _huge*, long);
long WINAPI _hwrite(HFILE, const void _huge*, long);
#endif /* WINVER >= 0x030a */

#endif /* NOLFILEIO */

#if (WINVER >= 0x0400)
DWORD   WINAPI GetCurrentDirectory(DWORD, LPSTR);
BOOL    WINAPI SetCurrentDirectory(LPCSTR);

#define MAX_PATH 260

#define _FILETIME_
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, NEAR *PFILETIME, FAR *LPFILETIME;

typedef struct _WIN32_FIND_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    char   cFileName[ MAX_PATH ];
    char   cAlternateFileName[ 14 ];
} WIN32_FIND_DATA, NEAR *PWIN32_FIND_DATA, FAR *LPWIN32_FIND_DATA;

#define FILE_ATTRIBUTE_READONLY         0x00000001
#define FILE_ATTRIBUTE_HIDDEN           0x00000002
#define FILE_ATTRIBUTE_SYSTEM           0x00000004
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020
#define FILE_ATTRIBUTE_NORMAL           0x00000080
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100
#define FILE_ATTRIBUTE_ATOMIC_WRITE     0x00000200
#define FILE_ATTRIBUTE_XACTION_WRITE    0x00000400

DECLARE_HANDLE(HFINDFILE);
#define INVALID_HFINDFILE       ((HFINDFILE)-1)
#define INVALID_HANDLE_VALUE    ((HANDLE)-1)

HFINDFILE WINAPI FindFirstFile(LPCSTR, LPWIN32_FIND_DATA);
BOOL      WINAPI FindNextFile(HFINDFILE, LPWIN32_FIND_DATA);
BOOL      WINAPI FindClose(HFINDFILE);

DWORD     WINAPI GetFileAttributes(LPCSTR);
BOOL      WINAPI SetFileAttributes(LPCSTR, DWORD);
BOOL      WINAPI GetDiskFreeSpace(LPCSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD);
BOOL      WINAPI CreateDirectory(LPCSTR, LPVOID);
BOOL      WINAPI RemoveDirectory(LPCSTR);
BOOL      WINAPI DeleteFile(LPCSTR);
#endif /* WINVER >= 0x0400 */

/* GetTempFileName() Flags */
#define TF_FORCEDRIVE     (BYTE)0x80

int     WINAPI GetTempFileName(BYTE, LPCSTR, UINT, LPSTR);
BYTE    WINAPI GetTempDrive(char);

/* GetDriveType return values */
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED     3
#define DRIVE_REMOTE    4
UINT    WINAPI GetDriveType(int);

UINT    WINAPI SetHandleCount(UINT);

/****** Network support *****************************************************/
UINT WINAPI WNetAddConnection(LPSTR, LPSTR, LPSTR);
UINT WINAPI WNetGetConnection(LPSTR, LPSTR, UINT FAR*);
UINT WINAPI WNetCancelConnection(LPSTR, BOOL);
/* Errors */
#define WN_SUCCESS          0x0000
#define WN_NOT_SUPPORTED    0x0001
#define WN_NET_ERROR        0x0002
#define WN_MORE_DATA        0x0003
#define WN_BAD_POINTER      0x0004
#define WN_BAD_VALUE        0x0005
#define WN_BAD_PASSWORD     0x0006
#define WN_ACCESS_DENIED    0x0007
#define WN_FUNCTION_BUSY    0x0008
#define WN_WINDOWS_ERROR    0x0009
#define WN_BAD_USER         0x000A
#define WN_OUT_OF_MEMORY    0x000B
#define WN_CANCEL           0x000C
#define WN_CONTINUE         0x000D

/* Connection errors */
#define WN_NOT_CONNECTED    0x0030
#define WN_OPEN_FILES       0x0031
#define WN_BAD_NETNAME      0x0032
#define WN_BAD_LOCALNAME    0x0033
#define WN_ALREADY_CONNECTED    0x0034
#define WN_DEVICE_ERROR         0x0035
#define WN_CONNECTION_CLOSED    0x0036

/****** Resource Management *************************************************/

DECLARE_HANDLE(HRSRC);

HRSRC   WINAPI FindResource(HINSTANCE, LPCSTR, LPCSTR);
HGLOBAL WINAPI LoadResource(HINSTANCE, HRSRC);
BOOL    WINAPI FreeResource(HGLOBAL);

#ifdef STRICT
void FAR* WINAPI LockResource(HGLOBAL);
#else
char FAR* WINAPI LockResource(HGLOBAL);
#endif

#define     UnlockResource(h)     GlobalUnlock(h)

DWORD   WINAPI SizeofResource(HINSTANCE, HRSRC);

int     WINAPI AccessResource(HINSTANCE, HRSRC);

HGLOBAL WINAPI AllocResource(HINSTANCE, HRSRC, DWORD);

#ifdef STRICT
typedef HGLOBAL (CALLBACK* RSRCHDLRPROC)(HGLOBAL, HINSTANCE, HRSRC);
#else
typedef FARPROC RSRCHDLRPROC;
#endif

RSRCHDLRPROC WINAPI SetResourceHandler(HINSTANCE, LPCSTR, RSRCHDLRPROC);

#define MAKEINTRESOURCE(i)  ((LPCSTR)MAKELP(0, (i)))

#ifndef NORESOURCE

/* Predefined Resource Types */
#define RT_CURSOR       MAKEINTRESOURCE(1)
#define RT_BITMAP       MAKEINTRESOURCE(2)
#define RT_ICON         MAKEINTRESOURCE(3)
#define RT_MENU         MAKEINTRESOURCE(4)
#define RT_DIALOG       MAKEINTRESOURCE(5)
#define RT_STRING       MAKEINTRESOURCE(6)
#define RT_FONTDIR      MAKEINTRESOURCE(7)
#define RT_FONT         MAKEINTRESOURCE(8)
#define RT_ACCELERATOR  MAKEINTRESOURCE(9)
#define RT_RCDATA       MAKEINTRESOURCE(10)
#define RT_GROUP_CURSOR MAKEINTRESOURCE(12)
#define RT_GROUP_ICON   MAKEINTRESOURCE(14)
#define RT_PLUGPLAY     MAKEINTRESOURCE(19)
#define RT_VXD          MAKEINTRESOURCE(20)

#endif  /* NORESOURCE */


#ifdef OEMRESOURCE

/* OEM Resource Ordinal Numbers */
#define OBM_CLOSE           32754
#define OBM_UPARROW         32753
#define OBM_DNARROW         32752
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750
#define OBM_REDUCE          32749
#define OBM_ZOOM            32748
#define OBM_RESTORE         32747
#define OBM_REDUCED         32746
#define OBM_ZOOMD           32745
#define OBM_RESTORED        32744
#define OBM_UPARROWD        32743
#define OBM_DNARROWD        32742
#define OBM_RGARROWD        32741
#define OBM_LFARROWD        32740
#define OBM_MNARROW         32739
#define OBM_COMBO           32738
#if (WINVER >= 0x030a)
#define OBM_UPARROWI        32737
#define OBM_DNARROWI        32736
#define OBM_RGARROWI        32735
#define OBM_LFARROWI        32734
#endif  /* WINVER >= 0x030a */

#define OBM_OLD_CLOSE       32767
#define OBM_SIZE            32766
#define OBM_OLD_UPARROW     32765
#define OBM_OLD_DNARROW     32764
#define OBM_OLD_RGARROW     32763
#define OBM_OLD_LFARROW     32762
#define OBM_BTSIZE          32761
#define OBM_CHECK           32760
#define OBM_CHECKBOXES      32759
#define OBM_BTNCORNERS      32758
#define OBM_OLD_REDUCE      32757
#define OBM_OLD_ZOOM        32756
#define OBM_OLD_RESTORE     32755

#define OCR_NORMAL          32512
#define OCR_IBEAM           32513
#define OCR_WAIT            32514
#define OCR_CROSS           32515
#define OCR_UP              32516
#define OCR_SIZE            32640
#define OCR_ICON            32641
#define OCR_SIZENWSE        32642
#define OCR_SIZENESW        32643
#define OCR_SIZEWE          32644
#define OCR_SIZENS          32645
#define OCR_SIZEALL         32646
#define OCR_ICOCUR          32647
#define OCR_NO              32648   
#if (WINVER >= 0x0400)
#endif  /* WINVER >= 0x400 */

#define OIC_SAMPLE          32512
#define OIC_HAND            32513
#define OIC_QUES            32514
#define OIC_BANG            32515
#define OIC_NOTE            32516

#endif  /* OEMRESOURCE */

/****** Atom Management *****************************************************/

#define MAKEINTATOM(i)      ((LPCSTR)MAKELP(0, (i)))

#ifndef NOATOM

BOOL    WINAPI InitAtomTable(int);
ATOM    WINAPI AddAtom(LPCSTR);
ATOM    WINAPI DeleteAtom(ATOM);
ATOM    WINAPI FindAtom(LPCSTR);
UINT    WINAPI GetAtomName(ATOM, LPSTR, int);
ATOM    WINAPI GlobalAddAtom(LPCSTR);
ATOM    WINAPI GlobalDeleteAtom(ATOM);
ATOM    WINAPI GlobalFindAtom(LPCSTR);
UINT    WINAPI GlobalGetAtomName(ATOM, LPSTR, int);
HLOCAL  WINAPI GetAtomHandle(ATOM);

#endif  /* NOATOM */

/****** WIN.INI Support *****************************************************/

/* User Profile Routines */
UINT    WINAPI GetProfileInt(LPCSTR, LPCSTR, int);
int     WINAPI GetProfileString(LPCSTR, LPCSTR, LPCSTR, LPSTR, int);
BOOL    WINAPI WriteProfileString(LPCSTR, LPCSTR, LPCSTR);

UINT    WINAPI GetPrivateProfileInt(LPCSTR, LPCSTR, int, LPCSTR);
int     WINAPI GetPrivateProfileString(LPCSTR, LPCSTR, LPCSTR, LPSTR, int, LPCSTR);
BOOL    WINAPI WritePrivateProfileString(LPCSTR, LPCSTR, LPCSTR, LPCSTR);

#if (WINVER >= 0x0400)
BOOL    WINAPI GetPrivateProfileStruct(LPCSTR, LPCSTR, LPVOID, UINT, LPCSTR);
BOOL    WINAPI WritePrivateProfileStruct(LPCSTR, LPCSTR, LPVOID, UINT, LPCSTR);
UINT    WINAPI GetProfileSectionNames(LPSTR, UINT);
UINT    WINAPI GetPrivateProfileSectionNames(LPSTR, UINT, LPCSTR);
int     WINAPI GetPrivateProfileSection(LPCSTR, LPSTR, UINT, LPCSTR);
int     WINAPI GetProfileSection(LPCSTR, LPSTR, UINT);
BOOL    WINAPI WritePrivateProfileSection(LPCSTR, LPCSTR, LPCSTR);
BOOL    WINAPI WriteProfileSection(LPCSTR, LPCSTR);
#endif /* WINVER >= 0x0400 */

/****** Registration Database Support ***************************************/
/* return codes from Registration functions */
/* Retained for win 3.x compatibility  and not needed 4.0 onwards */
/* For 4.0 onwards you should be including winerror.h */

#if (WINVER < 0x0400)

#define ERROR_SUCCESS			0L
#define ERROR_BADDB                     1L
#define ERROR_BADKEY			2L
#define ERROR_CANTOPEN			3L
#define ERROR_CANTREAD			4L
#define ERROR_CANTWRITE			5L
#define ERROR_INSUFFICIENT_MEMORY	6L
#define ERROR_INVALID_PARAMETER         7L
#define	ERROR_ACCESS_DENIED		8L

#endif /* WINVER < 0x400 */

#define REG_SZ				0x0001

#if (WINVER >= 0x0400)
#define REG_BINARY			0x0003
#endif

typedef DWORD HKEY;
typedef HKEY FAR* LPHKEY;

#if (WINVER < 0x0400)		// Win 3.1 compatibility
#define HKEY_CLASSES_ROOT		(( HKEY) 0x1)
#else				// Win 4 upwards
#define HKEY_CLASSES_ROOT		(( HKEY) 0x80000000)
#define HKEY_CURRENT_USER		(( HKEY) 0x80000001)
#define HKEY_LOCAL_MACHINE		(( HKEY) 0x80000002)
#define HKEY_USERS			(( HKEY) 0x80000003)
#define HKEY_PERFORMANCE_DATA		(( HKEY) 0x80000004)
#define HKEY_CURRENT_CONFIG		(( HKEY) 0x80000005)
#define HKEY_DYN_DATA			(( HKEY) 0x80000006)
#define HKEY_PREDEF_KEYS		7
#endif

LONG    WINAPI RegOpenKey(HKEY, LPCSTR, LPHKEY);
LONG    WINAPI RegCreateKey(HKEY, LPCSTR, LPHKEY);
LONG    WINAPI RegCloseKey(HKEY);
LONG    WINAPI RegDeleteKey(HKEY, LPCSTR);
LONG    WINAPI RegSetValue(HKEY, LPCSTR, DWORD, LPCSTR, DWORD);
LONG    WINAPI RegQueryValue(HKEY, LPCSTR, LPSTR, LONG FAR*);
LONG    WINAPI RegEnumKey(HKEY, DWORD, LPSTR, DWORD);

#if (WINVER >= 0x0400)
LONG    WINAPI RegDeleteValue(HKEY, LPCSTR);
LONG    WINAPI RegEnumValue(HKEY, DWORD, LPCSTR,
                         LONG FAR *, DWORD, LONG FAR *, LPBYTE,
                         LONG FAR *);
LONG    WINAPI RegQueryValueEx(HKEY, LPCSTR, LONG FAR *, LONG FAR *,
			    LPBYTE, LONG FAR *);
LONG    WINAPI RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, LPBYTE, DWORD);
LONG    WINAPI RegFlushKey(HKEY);
LONG	WINAPI RegSaveKey(HKEY, LPCSTR,LPVOID);
LONG	WINAPI RegLoadKey(HKEY, LPCSTR,LPCSTR);
LONG	WINAPI RegUnLoadKey(HKEY, LPCSTR);

#endif		// WINVER >= 0x0400

/****** International & Char Translation Support ****************************/

void    WINAPI AnsiToOem(const char _huge*, char _huge*);
void    WINAPI OemToAnsi(const char _huge*, char _huge*);

void    WINAPI AnsiToOemBuff(LPCSTR, LPSTR, UINT);
void    WINAPI OemToAnsiBuff(LPCSTR, LPSTR, UINT);

LPSTR   WINAPI AnsiNext(LPCSTR);
LPSTR   WINAPI AnsiPrev(LPCSTR, LPCSTR);

LPSTR   WINAPI AnsiUpper(LPSTR);
LPSTR   WINAPI AnsiLower(LPSTR);

UINT    WINAPI AnsiUpperBuff(LPSTR, UINT);
UINT    WINAPI AnsiLowerBuff(LPSTR, UINT);

#ifndef  NOLANGUAGE
BOOL    WINAPI IsCharAlpha(char);
BOOL    WINAPI IsCharAlphaNumeric(char);
BOOL    WINAPI IsCharUpper(char);
BOOL    WINAPI IsCharLower(char);
#endif

#ifndef NOLSTRING
int     WINAPI lstrcmp(LPCSTR, LPCSTR);
int     WINAPI lstrcmpi(LPCSTR, LPCSTR);
LPSTR   WINAPI lstrcpy(LPSTR, LPCSTR);
LPSTR   WINAPI lstrcat(LPSTR, LPCSTR);
int     WINAPI lstrlen(LPCSTR);
#if (WINVER >= 0x030a)
LPSTR   WINAPI lstrcpyn(LPSTR, LPCSTR, int);
void    WINAPI hmemcpy(void _huge*, const void _huge*, DWORD);
#if (WINVER >= 0x0400)
LPSTR   WINAPI lstrcatn(LPSTR, LPCSTR, int);
#endif /* WINVER >= 0x0400 */
#endif /* WINVER >= 0x030a */
#endif /* NOLSTRING */

#if (WINVER >= 0x030a)
#ifndef NODBCS
BOOL    WINAPI IsDBCSLeadByte(BYTE);
#endif /* NODBCS */
#endif  /* WINVER >= 0x030a */

int     WINAPI LoadString(HINSTANCE, UINT, LPSTR, int);


/****** Keyboard Driver Functions *******************************************/

#ifndef NOKEYBOARDINFO

DWORD   WINAPI OemKeyScan(UINT);
UINT    WINAPI VkKeyScan(UINT);
int     WINAPI GetKeyboardType(int);
UINT    WINAPI MapVirtualKey(UINT, UINT);
int     WINAPI GetKBCodePage(void);
int     WINAPI GetKeyNameText(LONG, LPSTR, int);
int     WINAPI ToAscii(UINT wVirtKey, UINT wScanCode, BYTE FAR* lpKeyState, DWORD FAR* lpChar, UINT wFlags);

#endif

#endif  /* NOKERNEL */

/****** GDI typedefs, structures, and functions *****************************/

DECLARE_HANDLE(HDC);

#ifndef NOGDI

#ifdef STRICT
typedef const void NEAR* HGDIOBJ;
#else
DECLARE_HANDLE(HGDIOBJ);
#endif

#endif /* NOGDI */

DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HFONT);

typedef struct tagRECT
{
    int left;
    int top;
    int right;
    int bottom;
} RECT;
typedef RECT*      PRECT;
typedef RECT NEAR* NPRECT;
typedef RECT FAR*  LPRECT;
typedef const RECT FAR * LPCRECT;

typedef struct tagPOINT
{
    int x;
    int y;
} POINT;
typedef POINT*       PPOINT;
typedef POINT NEAR* NPPOINT;
typedef POINT FAR*  LPPOINT;
typedef const POINT FAR * LPCPOINT;

#if (WINVER >= 0x030a)
typedef struct tagSIZE
{
    int cx;
    int cy;
} SIZE;
typedef SIZE*       PSIZE;
typedef SIZE NEAR* NPSIZE;
typedef SIZE FAR*  LPSIZE;
#endif  /* WINVER >= 0x030a */

#if (WINVER >= 0x0400)

/* Common structures for 4.X/Win32 GDI APIs */

typedef struct tagPOINTS
{
   SHORT    x;
   SHORT    y;

} POINTS;
typedef POINTS*       PPOINTS;
typedef POINTS NEAR*  NPPOINTS;
typedef POINTS FAR*   LPPOINTS;
typedef const POINTS FAR* LPCPOINTS;

typedef struct tagPOINTL
{
   LONG     x;
   LONG     y;

} POINTL;
typedef POINTL*       PPOINTL;
typedef POINTL NEAR*  NPPOINTL;
typedef POINTL FAR*   LPPOINTL;
typedef const POINTL FAR* LPCPOINTL;

typedef struct tagSIZEL
{
   LONG     cx;
   LONG     cy;

} SIZEL;
typedef SIZEL*       PSIZEL;
typedef SIZEL NEAR*  NPSIZEL;
typedef SIZEL FAR*   LPSIZEL;
typedef const SIZEL FAR* LPCSIZEL;

typedef struct tagRECTL
{                      
   LONG     left;      
   LONG     top;       
   LONG     right;     
   LONG     bottom;    
                       
} RECTL;               
typedef RECTL*       PRECTL; 
typedef RECTL NEAR*  NPRECTL; 
typedef RECTL FAR*   LPRECTL;  
typedef const RECTL FAR* LPCRECTL;
#endif /* WINVER >= 0x0400 */

#define MAKEPOINT(l)     (*((POINT FAR*)&(l)))

#ifndef NOGDI

/****** DC Management *******************************************************/

HDC     WINAPI CreateDC(LPCSTR, LPCSTR, LPCSTR, const void FAR*);
HDC     WINAPI CreateIC(LPCSTR, LPCSTR, LPCSTR, const void FAR*);
HDC     WINAPI CreateCompatibleDC(HDC);

BOOL    WINAPI DeleteDC(HDC);

DWORD   WINAPI GetDCOrg(HDC);

int     WINAPI SaveDC(HDC);
BOOL    WINAPI RestoreDC(HDC, int);

int     WINAPI SetEnvironment(LPCSTR, const void FAR*, UINT);
int     WINAPI GetEnvironment(LPCSTR, void FAR*, UINT);

int     WINAPI MulDiv(int, int, int);

#if (WINVER >= 0x030a)
/* Drawing bounds accumulation APIs */
UINT    WINAPI SetBoundsRect(HDC hDC, LPCRECT lprcBounds, UINT flags);
UINT    WINAPI GetBoundsRect(HDC hDC, LPRECT lprcBounds, UINT flags);

#define DCB_RESET       0x0001
#define DCB_ACCUMULATE  0x0002
#define DCB_DIRTY       DCB_ACCUMULATE
#define DCB_SET         (DCB_RESET | DCB_ACCUMULATE)
#define DCB_ENABLE      0x0004
#define DCB_DISABLE     0x0008
#endif  /* WINVER >= 0x030a */

/****** Device Capabilities *************************************************/

int WINAPI GetDeviceCaps(HDC, int);

/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0
#define TECHNOLOGY    2
#define HORZSIZE      4
#define VERTSIZE      6
#define HORZRES       8
#define VERTRES       10
#define BITSPIXEL     12
#define PLANES        14
#define NUMBRUSHES    16
#define NUMPENS       18
#define NUMMARKERS    20
#define NUMFONTS      22
#define NUMCOLORS     24
#define PDEVICESIZE   26
#define CURVECAPS     28
#define LINECAPS      30
#define POLYGONALCAPS 32
#define TEXTCAPS      34
#define CLIPCAPS      36
#define RASTERCAPS    38
#define ASPECTX       40
#define ASPECTY       42
#define ASPECTXY      44

#define LOGPIXELSX    88
#define LOGPIXELSY    90

#define SIZEPALETTE  104
#define NUMRESERVED  106
#define COLORRES     108

#ifndef NOGDICAPMASKS

/* GetDeviceCaps() return value masks */

/* TECHNOLOGY */
#define DT_PLOTTER          0
#define DT_RASDISPLAY       1
#define DT_RASPRINTER       2
#define DT_RASCAMERA        3
#define DT_CHARSTREAM       4
#define DT_METAFILE         5
#define DT_DISPFILE         6

/* CURVECAPS */
#define CC_NONE             0x0000
#define CC_CIRCLES          0x0001
#define CC_PIE              0x0002
#define CC_CHORD            0x0004
#define CC_ELLIPSES         0x0008
#define CC_WIDE             0x0010
#define CC_STYLED           0x0020
#define CC_WIDESTYLED       0x0040
#define CC_INTERIORS        0x0080
#define CC_ROUNDRECT        0x0100

/* LINECAPS */
#define LC_NONE             0x0000
#define LC_POLYLINE         0x0002
#define LC_MARKER           0x0004
#define LC_POLYMARKER       0x0008
#define LC_WIDE             0x0010
#define LC_STYLED           0x0020
#define LC_WIDESTYLED       0x0040
#define LC_INTERIORS        0x0080

/* POLYGONALCAPS */
#define PC_NONE             0x0000
#define PC_POLYGON          0x0001
#define PC_RECTANGLE        0x0002
#define PC_WINDPOLYGON      0x0004
#define PC_SCANLINE         0x0008
#define PC_WIDE             0x0010
#define PC_STYLED           0x0020
#define PC_WIDESTYLED       0x0040
#define PC_INTERIORS        0x0080
#define PC_POLYPOLYGON      0x0100

/* TEXTCAPS */
#define TC_OP_CHARACTER     0x0001
#define TC_OP_STROKE        0x0002
#define TC_CP_STROKE        0x0004
#define TC_CR_90            0x0008
#define TC_CR_ANY           0x0010
#define TC_SF_X_YINDEP      0x0020
#define TC_SA_DOUBLE        0x0040
#define TC_SA_INTEGER       0x0080
#define TC_SA_CONTIN        0x0100
#define TC_EA_DOUBLE        0x0200
#define TC_IA_ABLE          0x0400
#define TC_UA_ABLE          0x0800
#define TC_SO_ABLE          0x1000
#define TC_RA_ABLE          0x2000
#define TC_VA_ABLE          0x4000
#define TC_RESERVED         0x8000

/* CLIPCAPS */
#define CP_NONE             0x0000
#define CP_RECTANGLE        0x0001
#define CP_REGION           0x0002

/* RASTERCAPS */
#define RC_NONE
#define RC_BITBLT           0x0001
#define RC_BANDING          0x0002
#define RC_SCALING          0x0004
#define RC_BITMAP64         0x0008
#define RC_GDI20_OUTPUT     0x0010
#define RC_GDI20_STATE      0x0020
#define RC_SAVEBITMAP       0x0040
#define RC_DI_BITMAP        0x0080
#define RC_PALETTE          0x0100
#define RC_DIBTODEV         0x0200
#define RC_BIGFONT          0x0400
#define RC_STRETCHBLT       0x0800
#define RC_FLOODFILL        0x1000
#define RC_STRETCHDIB       0x2000
#define RC_OP_DX_OUTPUT     0x4000
#define RC_DEVBITS          0x8000

#endif  /* NOGDICAPMASKS */

/****** Coordinate transformation support ***********************************/

int     WINAPI SetMapMode(HDC, int);
int     WINAPI GetMapMode(HDC);

/* Map modes */
#define MM_TEXT         1
#define MM_LOMETRIC     2
#define MM_HIMETRIC     3
#define MM_LOENGLISH    4
#define MM_HIENGLISH    5
#define MM_TWIPS        6
#define MM_ISOTROPIC    7
#define MM_ANISOTROPIC  8

DWORD   WINAPI SetWindowOrg(HDC, int, int);
DWORD   WINAPI GetWindowOrg(HDC);

DWORD   WINAPI SetWindowExt(HDC, int, int);
DWORD   WINAPI GetWindowExt(HDC);

DWORD   WINAPI OffsetWindowOrg(HDC, int, int);
DWORD   WINAPI ScaleWindowExt(HDC, int, int, int, int);

DWORD   WINAPI SetViewportOrg(HDC, int, int);
DWORD   WINAPI GetViewportOrg(HDC);

DWORD   WINAPI SetViewportExt(HDC, int, int);
DWORD   WINAPI GetViewportExt(HDC);

DWORD   WINAPI OffsetViewportOrg(HDC, int, int);
DWORD   WINAPI ScaleViewportExt(HDC, int, int, int, int);

#if (WINVER >= 0x030a)
BOOL    WINAPI SetWindowOrgEx(HDC, int, int, POINT FAR*);
BOOL    WINAPI GetWindowOrgEx(HDC, POINT FAR*);

BOOL    WINAPI SetWindowExtEx(HDC, int, int, SIZE FAR*);
BOOL    WINAPI GetWindowExtEx(HDC, SIZE FAR*);

BOOL    WINAPI OffsetWindowOrgEx(HDC, int, int, POINT FAR*);
BOOL    WINAPI ScaleWindowExtEx(HDC, int, int, int, int, SIZE FAR*);

BOOL    WINAPI SetViewportExtEx(HDC, int, int, SIZE FAR*);
BOOL    WINAPI GetViewportExtEx(HDC, SIZE FAR*);

BOOL    WINAPI SetViewportOrgEx(HDC, int, int, POINT FAR*);
BOOL    WINAPI GetViewportOrgEx(HDC, POINT FAR*);

BOOL    WINAPI OffsetViewportOrgEx(HDC, int, int, POINT FAR*);
BOOL    WINAPI ScaleViewportExtEx(HDC, int, int, int, int, SIZE FAR*);
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI DPtoLP(HDC, POINT FAR*, int);
BOOL    WINAPI LPtoDP(HDC, POINT FAR*, int);

/* Coordinate Modes */
#define ABSOLUTE    1
#define RELATIVE    2

/****** Color support *******************************************************/

typedef DWORD COLORREF;

#define RGB(r,g,b)      ((COLORREF)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)))

#define GetRValue(rgb)  ((BYTE)(rgb))
#define GetGValue(rgb)  ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)  ((BYTE)((rgb)>>16))

COLORREF WINAPI GetNearestColor(HDC, COLORREF);

#ifndef NOCOLOR

HBRUSH   WINAPI GetSysColorBrush(int);
COLORREF WINAPI GetSysColor(int);
void     WINAPI SetSysColors(int, const int FAR*, const COLORREF FAR*);

#define COLOR_SCROLLBAR             0
#define COLOR_DESKTOP               1
#define COLOR_ACTIVECAPTION         2
#define COLOR_INACTIVECAPTION       3
#define COLOR_MENU                  4
#define COLOR_WINDOW                5
#define COLOR_WINDOWFRAME           6
#define COLOR_MENUTEXT              7
#define COLOR_WINDOWTEXT            8
#define COLOR_CAPTIONTEXT           9
#define COLOR_ACTIVEBORDER          10
#define COLOR_INACTIVEBORDER        11
#define COLOR_APPWORKSPACE          12
#define COLOR_HIGHLIGHT             13
#define COLOR_HIGHLIGHTTEXT         14
#define COLOR_3DFACE                15
#define COLOR_3DSHADOW              16
#define COLOR_GRAYTEXT              17
#define COLOR_BTNTEXT               18
#if (WINVER >= 0x030a)
#define COLOR_INACTIVECAPTIONTEXT   19
#define COLOR_3DHILIGHT             20
#if (WINVER >= 0x0400)
#define COLOR_3DDKSHADOW            21
#define COLOR_3DLIGHT               22
#define COLOR_MSGBOX                23
#define COLOR_MSGBOXTEXT            24
#endif /* WINVER >= 0x0400 */
#endif  /* WINVER >= 0x030a */

#define COLOR_BACKGROUND            COLOR_DESKTOP
#define COLOR_BTNFACE               COLOR_3DFACE
#define COLOR_BTNSHADOW             COLOR_3DSHADOW
#define COLOR_BTNHIGHLIGHT          COLOR_3DHILIGHT

#endif  /* NOCOLOR */

/****** GDI Object Support **************************************************/

#ifndef NOGDIOBJ

HGDIOBJ WINAPI GetStockObject(int);

BOOL    WINAPI IsGDIObject(HGDIOBJ);

BOOL    WINAPI DeleteObject(HGDIOBJ);
HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
int     WINAPI GetObject(HGDIOBJ, int, void FAR*);
BOOL    WINAPI UnrealizeObject(HGDIOBJ);

#ifdef STRICT
typedef (CALLBACK* GOBJENUMPROC)(void FAR*, LPARAM);
#else
typedef FARPROC GOBJENUMPROC;
#endif

#ifdef STRICT
int     WINAPI EnumObjects(HDC, int, GOBJENUMPROC, LPARAM);
#else
int     WINAPI EnumObjects(HDC, int, GOBJENUMPROC, LPSTR);
#endif

/* Object types for EnumObjects() */
#define OBJ_PEN         1
#define OBJ_BRUSH       2

#if (WINVER >= 0x0400)

HANDLE WINAPI SetObjectOwner(HGDIOBJ, HANDLE);

#endif

/****** Pen support *********************************************************/

/* Logical Pen */
typedef struct tagLOGPEN
{
    UINT    lopnStyle;
    POINT   lopnWidth;
    COLORREF lopnColor;
} LOGPEN;
typedef LOGPEN*       PLOGPEN;
typedef LOGPEN NEAR* NPLOGPEN;
typedef LOGPEN FAR*  LPLOGPEN;

/* Pen Styles */
#define PS_SOLID        0
#define PS_DASH         1
#define PS_DOT          2
#define PS_DASHDOT      3
#define PS_DASHDOTDOT   4
#define PS_NULL         5
#define PS_INSIDEFRAME  6

HPEN    WINAPI CreatePen(int, int, COLORREF);
HPEN    WINAPI CreatePenIndirect(LOGPEN FAR*);

/* Stock pens for use with GetStockObject(); */
#define WHITE_PEN       6
#define BLACK_PEN       7
#define NULL_PEN        8

/****** Brush support *******************************************************/

/* Brush Styles */
#define BS_SOLID        0
#define BS_NULL         1
#define BS_HOLLOW       BS_NULL
#define BS_HATCHED      2
#define BS_PATTERN      3
#define BS_INDEXED      4
#define BS_DIBPATTERN   5

/* Hatch Styles */
#define HS_HORIZONTAL   0
#define HS_VERTICAL     1
#define HS_FDIAGONAL    2
#define HS_BDIAGONAL    3
#define HS_CROSS        4
#define HS_DIAGCROSS    5
#define HS_API_MAX      6

/* Logical Brush (or Pattern) */
typedef struct tagLOGBRUSH
{
    UINT     lbStyle;
    COLORREF lbColor;
    int      lbHatch;
} LOGBRUSH;
typedef LOGBRUSH*       PLOGBRUSH;
typedef LOGBRUSH NEAR* NPLOGBRUSH;
typedef LOGBRUSH FAR*  LPLOGBRUSH;

typedef LOGBRUSH        PATTERN;
typedef PATTERN*       PPATTERN;
typedef PATTERN NEAR* NPPATTERN;
typedef PATTERN FAR*  LPPATTERN;

HBRUSH  WINAPI CreateSolidBrush(COLORREF);
HBRUSH  WINAPI CreateHatchBrush(int, COLORREF);
HBRUSH  WINAPI CreatePatternBrush(HBITMAP);
HBRUSH  WINAPI CreateDIBPatternBrush(HGLOBAL, UINT);
HBRUSH  WINAPI CreateBrushIndirect(LOGBRUSH FAR*);

/* Stock brushes for use with GetStockObject() */
#define WHITE_BRUSH    0
#define LTGRAY_BRUSH   1
#define GRAY_BRUSH     2
#define DKGRAY_BRUSH   3
#define BLACK_BRUSH    4
#define NULL_BRUSH     5
#define HOLLOW_BRUSH   NULL_BRUSH

DWORD   WINAPI SetBrushOrg(HDC, int, int);
DWORD   WINAPI GetBrushOrg(HDC);

#if (WINVER >= 0x030a)
BOOL    WINAPI GetBrushOrgEx(HDC, POINT FAR*);
#endif  /* WINVER >= 0x030a */
#endif /* NOGDIOBJ */

/****** Region support ******************************************************/

HRGN    WINAPI CreateRectRgn(int, int, int, int);
HRGN    WINAPI CreateRectRgnIndirect(LPCRECT);
HRGN    WINAPI CreateEllipticRgnIndirect(LPCRECT);
HRGN    WINAPI CreateEllipticRgn(int, int, int, int);
HRGN    WINAPI CreatePolygonRgn(LPCPOINT, int, int);
HRGN    WINAPI CreatePolyPolygonRgn(LPCPOINT, const int FAR*, int, int);
HRGN    WINAPI CreateRoundRectRgn(int, int, int, int, int, int);

/* Region type flags */
#define ERROR           0
#define NULLREGION      1
#define SIMPLEREGION    2
#define COMPLEXREGION   3

void    WINAPI SetRectRgn(HRGN, int, int, int, int);

int     WINAPI CombineRgn(HRGN, HRGN, HRGN, int);

/* CombineRgn() command values */
#define RGN_AND         1
#define RGN_OR          2
#define RGN_XOR         3
#define RGN_DIFF        4
#define RGN_COPY        5

BOOL    WINAPI EqualRgn(HRGN, HRGN);
int     WINAPI OffsetRgn(HRGN, int, int);

int     WINAPI GetRgnBox(HRGN, LPRECT);

BOOL    WINAPI RectInRegion(HRGN, LPCRECT);
BOOL    WINAPI PtInRegion(HRGN, int, int);


/****** Color palette Support ************************************************/

#define PALETTERGB(r,g,b)   (0x02000000L | RGB(r,g,b))
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000L | (DWORD)(WORD)(i)))

typedef struct tagPALETTEENTRY
{
    BYTE    peRed;
    BYTE    peGreen;
    BYTE    peBlue;
    BYTE    peFlags;
} PALETTEENTRY;
typedef PALETTEENTRY FAR* LPPALETTEENTRY;

/* Palette entry flags */
#define PC_RESERVED 0x01 /* palette index used for animation */
#define PC_EXPLICIT 0x02 /* palette index is explicit to device */
#define PC_NOCOLLAPSE 0x04 /* do not match color to system palette */

/* Logical Palette */
typedef struct tagLOGPALETTE
{
    WORD    palVersion;
    WORD    palNumEntries;
    PALETTEENTRY palPalEntry[1];
} LOGPALETTE;
typedef LOGPALETTE*       PLOGPALETTE;
typedef LOGPALETTE NEAR* NPLOGPALETTE;
typedef LOGPALETTE FAR*  LPLOGPALETTE;

HPALETTE WINAPI CreatePalette(const LOGPALETTE FAR*);

HPALETTE WINAPI SelectPalette(HDC, HPALETTE, BOOL);

UINT    WINAPI RealizePalette(HDC);

int     WINAPI UpdateColors(HDC);
void    WINAPI AnimatePalette(HPALETTE, UINT, UINT, const PALETTEENTRY FAR*);

UINT    WINAPI SetPaletteEntries(HPALETTE, UINT, UINT, const PALETTEENTRY FAR*);
UINT    WINAPI GetPaletteEntries(HPALETTE, UINT, UINT, PALETTEENTRY FAR*);

UINT    WINAPI GetNearestPaletteIndex(HPALETTE, COLORREF);

BOOL    WINAPI ResizePalette(HPALETTE, UINT);

UINT    WINAPI GetSystemPaletteEntries(HDC, UINT, UINT, PALETTEENTRY FAR*);

UINT    WINAPI GetSystemPaletteUse(HDC);
UINT    WINAPI SetSystemPaletteUse(HDC, UINT);

/* Get/SetSystemPaletteUse() values */
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2

/****** Clipping support *****************************************************/

int     WINAPI SelectClipRgn(HDC, HRGN);
int     WINAPI GetClipBox(HDC, LPRECT);

int     WINAPI IntersectClipRect(HDC, int, int, int, int);
int     WINAPI OffsetClipRgn(HDC, int, int);
int     WINAPI ExcludeClipRect(HDC, int, int, int, int);

BOOL    WINAPI PtVisible(HDC, int, int);
BOOL    WINAPI RectVisible(HDC, LPCRECT);


/****** General drawing support ********************************************/

DWORD   WINAPI MoveTo(HDC, int, int);
DWORD   WINAPI GetCurrentPosition(HDC);

#if (WINVER >= 0x030a)
BOOL    WINAPI MoveToEx(HDC, int, int, POINT FAR*);
BOOL    WINAPI GetCurrentPositionEx(HDC, POINT FAR*);
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI LineTo(HDC, int, int);
BOOL    WINAPI Polyline(HDC, LPCPOINT, int);

#ifdef STRICT
typedef void (CALLBACK* LINEDDAPROC)(int, int, LPARAM);
#else
typedef FARPROC LINEDDAPROC;
#endif

void    WINAPI LineDDA(int, int, int, int, LINEDDAPROC, LPARAM);

BOOL    WINAPI Rectangle(HDC, int, int, int, int);
BOOL    WINAPI RoundRect(HDC, int, int, int, int, int, int);

BOOL    WINAPI Ellipse(HDC, int, int, int, int);
BOOL    WINAPI Arc(HDC, int, int, int, int, int, int, int, int);
BOOL    WINAPI Chord(HDC, int, int, int, int, int, int, int, int);
BOOL    WINAPI Pie(HDC, int, int, int, int, int, int, int, int);

BOOL    WINAPI Polygon(HDC, LPCPOINT, int);
BOOL    WINAPI PolyPolygon(HDC, LPCPOINT, int FAR*, int);

/* PolyFill Modes */
#define ALTERNATE   1
#define WINDING     2

int     WINAPI SetPolyFillMode(HDC, int);
int     WINAPI GetPolyFillMode(HDC);

BOOL    WINAPI FloodFill(HDC, int, int, COLORREF);
BOOL    WINAPI ExtFloodFill(HDC, int, int, COLORREF, UINT);

/* ExtFloodFill style flags */
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1

BOOL    WINAPI FillRgn(HDC, HRGN, HBRUSH);
BOOL    WINAPI FrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL    WINAPI InvertRgn(HDC, HRGN);
BOOL    WINAPI PaintRgn(HDC, HRGN);

/* Rectangle output routines */
int     WINAPI FillRect(HDC, LPCRECT, HBRUSH);
int     WINAPI FrameRect(HDC, LPCRECT, HBRUSH);
BOOL    WINAPI InvertRect(HDC, LPCRECT);

void    WINAPI DrawFocusRect(HDC, LPCRECT);

/****** Text support ********************************************************/

BOOL    WINAPI TextOut(HDC, int, int, LPCSTR, int);
LONG    WINAPI TabbedTextOut(HDC, int, int, LPCSTR, int, int, int FAR*, int);
BOOL    WINAPI ExtTextOut(HDC, int, int, UINT, LPCRECT, LPCSTR, UINT, int FAR*);

#define ETO_GRAYED      0x0001
#define ETO_OPAQUE      0x0002
#define ETO_CLIPPED     0x0004

DWORD   WINAPI GetTextExtent(HDC, LPCSTR, int);
DWORD   WINAPI GetTabbedTextExtent(HDC, LPCSTR, int, int, int FAR*);

#if (WINVER >= 0x030a)
BOOL    WINAPI GetTextExtentPoint(HDC, LPCSTR, int, SIZE FAR*);
#endif  /* WINVER >= 0x030a */

/* DrawText() Format Flags */
#ifndef NODRAWTEXT
#define DT_LEFT             0x0000
#define DT_CENTER           0x0001
#define DT_RIGHT            0x0002
#define DT_TOP              0x0000
#define DT_VCENTER          0x0004
#define DT_BOTTOM           0x0008
#define DT_WORDBREAK        0x0010
#define DT_SINGLELINE       0x0020
#define DT_EXPANDTABS       0x0040
#define DT_TABSTOP          0x0080
#define DT_NOCLIP           0x0100
#define DT_EXTERNALLEADING  0x0200
#define DT_CALCRECT         0x0400
#define DT_NOPREFIX         0x0800
#define DT_INTERNAL         0x1000
#define DT_EDITCONTROL      0x00002000

int     WINAPI DrawText(HDC, LPCSTR, int, LPRECT, UINT);

#if (WINVER >= 0x0400)
typedef struct tagDRAWTEXTPARAMS
{
    DWORD   cbSize;
    int     iTabLength;
    int     iLeftMargin;
    int     iRightMargin;
    UINT    uiLengthDrawn;
}   DRAWTEXTPARAMS, FAR *LPDRAWTEXTPARAMS;

int     WINAPI DrawTextEx(HDC, LPCSTR, int, LPRECT, DWORD, LPDRAWTEXTPARAMS);
#endif /* WINVER >= 0x0400 */

#endif  /* NODRAWTEXT */

#ifdef STRICT
typedef BOOL (CALLBACK* GRAYSTRINGPROC)(HDC, LPARAM, int);
#else
typedef FARPROC GRAYSTRINGPROC;
#endif

BOOL    WINAPI GrayString(HDC, HBRUSH, GRAYSTRINGPROC, LPARAM, int, int, int, int, int);

/* Monolithic state-drawing routine */

#if (WINVER >= 0x0400)
/* Image type */
#define DST_COMPLEX     0x0000
#define DST_TEXT        0x0001
#define DST_PREFIXTEXT  0x0002
#define DST_ICON        0x0003
#define DST_BITMAP      0x0004

/* State type */
#define DSS_NORMAL      0x0000
#define DSS_UNION       0x0010  /* Old gray string appearance */
#define DSS_DISABLED    0x0020
#define DSS_MONO        0x0080

#ifdef STRICT
typedef BOOL (CALLBACK* DRAWSTATEPROC)(HDC, LPARAM, WPARAM, int, int);
#else
typedef FARPROC DRAWSTATEPROC;
#endif
BOOL    WINAPI DrawState(HDC, HBRUSH, DRAWSTATEPROC, LPARAM, WPARAM, int, int, int, int, UINT);
#endif /* WINVER >= 0x0400 */

BOOL    WINAPI GetCharWidth(HDC, UINT, UINT, int FAR*);

COLORREF WINAPI SetTextColor(HDC, COLORREF);
COLORREF WINAPI GetTextColor(HDC);

COLORREF WINAPI SetBkColor(HDC, COLORREF);
COLORREF WINAPI GetBkColor(HDC);

int     WINAPI SetBkMode(HDC, int);
int     WINAPI GetBkMode(HDC);

/* Background Modes */
#define TRANSPARENT     1
#define OPAQUE          2

UINT    WINAPI SetTextAlign(HDC, UINT);
UINT    WINAPI GetTextAlign(HDC);

/* Text Alignment Options */
#define TA_NOUPDATECP   0x0000
#define TA_UPDATECP     0x0001
#define TA_LEFT         0x0000
#define TA_RIGHT        0x0002
#define TA_CENTER       0x0006
#define TA_TOP          0x0000
#define TA_BOTTOM       0x0008
#define TA_BASELINE     0x0018

int     WINAPI SetTextCharacterExtra(HDC, int);
int     WINAPI GetTextCharacterExtra(HDC);

int     WINAPI SetTextJustification(HDC, int, int);

/****** Font support ********************************************************/

/* Logical Font */
#define LF_FACESIZE     32

#ifndef NOGDIOBJ
typedef struct tagLOGFONT
{
    int     lfHeight;
    int     lfWidth;
    int     lfEscapement;
    int     lfOrientation;
    int     lfWeight;
    BYTE    lfItalic;
    BYTE    lfUnderline;
    BYTE    lfStrikeOut;
    BYTE    lfCharSet;
    BYTE    lfOutPrecision;
    BYTE    lfClipPrecision;
    BYTE    lfQuality;
    BYTE    lfPitchAndFamily;
    char    lfFaceName[LF_FACESIZE];
} LOGFONT;
typedef LOGFONT*       PLOGFONT;
typedef LOGFONT NEAR* NPLOGFONT;
typedef LOGFONT FAR*  LPLOGFONT;

/* weight values */
#define FW_DONTCARE     0
#define FW_THIN         100
#define FW_EXTRALIGHT   200
#define FW_LIGHT        300
#define FW_NORMAL       400
#define FW_MEDIUM       500
#define FW_SEMIBOLD     600
#define FW_BOLD         700
#define FW_EXTRABOLD    800
#define FW_HEAVY        900

#define FW_ULTRALIGHT   FW_EXTRALIGHT
#define FW_REGULAR      FW_NORMAL
#define FW_DEMIBOLD     FW_SEMIBOLD
#define FW_ULTRABOLD    FW_EXTRABOLD
#define FW_BLACK        FW_HEAVY

/* CharSet values */
#define ANSI_CHARSET        0
#define DEFAULT_CHARSET     1
#define SYMBOL_CHARSET      2
#if (WINVER >= 0x0400)
#define	MAC_CHARSET         77
#endif /* WINVER >= 0x0400 */
#define SHIFTJIS_CHARSET    128
#define HANGEUL_CHARSET     129
#define GB2312_CHARSET      134
#define CHINESEBIG5_CHARSET 136
#define OEM_CHARSET         255
#define HEBREW_CHARSET		 177
#define ARABIC_CHARSET		 178
#define GREEK_CHARSET		 161
#define TURKISH_CHARSET		 162
#define EASTEUROPE_CHARSET	 238
#define RUSSIAN_CHARSET		 204

/* OutPrecision values */
#define OUT_DEFAULT_PRECIS      0
#define OUT_STRING_PRECIS       1
#define OUT_CHARACTER_PRECIS    2
#define OUT_STROKE_PRECIS       3
#if (WINVER >= 0x030a)
#define OUT_TT_PRECIS           4
#define OUT_DEVICE_PRECIS       5
#define OUT_RASTER_PRECIS       6
#define OUT_TT_ONLY_PRECIS      7
#endif  /* WINVER >= 0x030a */

/* ClipPrecision values */
#define CLIP_DEFAULT_PRECIS     0x00
#define CLIP_CHARACTER_PRECIS   0x01
#define CLIP_STROKE_PRECIS      0x02
#define CLIP_MASK               0x0F
#if (WINVER >= 0x030a)
#define CLIP_LH_ANGLES          0x10
#define CLIP_TT_ALWAYS          0x20
#define CLIP_EMBEDDED           0x80
#endif  /* WINVER >= 0x030a */

/* Quality values */
#define DEFAULT_QUALITY     0
#define DRAFT_QUALITY       1
#define PROOF_QUALITY       2

/* PitchAndFamily pitch values (low 4 bits) */
#define DEFAULT_PITCH       0x00
#define FIXED_PITCH         0x01
#define VARIABLE_PITCH      0x02

/* PitchAndFamily family values (high 4 bits) */
#define FF_DONTCARE         0x00
#define FF_ROMAN            0x10
#define FF_SWISS            0x20
#define FF_MODERN           0x30
#define FF_SCRIPT           0x40
#define FF_DECORATIVE       0x50

HFONT   WINAPI CreateFont(int, int, int, int, int, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, LPCSTR);
HFONT   WINAPI CreateFontIndirect(const LOGFONT FAR*);

/* Stock fonts for use with GetStockObject() */
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16


DWORD   WINAPI SetMapperFlags(HDC, DWORD);
#define ASPECT_FILTERING    0x00000001L

int     WINAPI AddFontResource(LPCSTR);
BOOL    WINAPI RemoveFontResource(LPCSTR);

int     WINAPI GetTextFace(HDC, int, LPSTR);

DWORD   WINAPI GetAspectRatioFilter(HDC);
#if (WINVER >= 0x030a)
BOOL    WINAPI GetAspectRatioFilterEx(HDC, SIZE FAR*);
#endif  /* WINVER >= 0x030a */

#endif /* NOGDIOBJ */


typedef struct tagPANOSE
{
    BYTE    bFamilyType;
    BYTE    bSerifStyle;
    BYTE    bWeight;
    BYTE    bProportion;
    BYTE    bContrast;
    BYTE    bStrokeVariation;
    BYTE    bArmStyle;
    BYTE    bLetterform;
    BYTE    bMidline;
    BYTE    bXHeight;
} PANOSE, FAR* LPPANOSE;


#ifndef NOTEXTMETRIC

typedef struct tagTEXTMETRIC
{
    int     tmHeight;
    int     tmAscent;
    int     tmDescent;
    int     tmInternalLeading;
    int     tmExternalLeading;
    int     tmAveCharWidth;
    int     tmMaxCharWidth;
    int     tmWeight;
    BYTE    tmItalic;
    BYTE    tmUnderlined;
    BYTE    tmStruckOut;
    BYTE    tmFirstChar;
    BYTE    tmLastChar;
    BYTE    tmDefaultChar;
    BYTE    tmBreakChar;
    BYTE    tmPitchAndFamily;
    BYTE    tmCharSet;
    int     tmOverhang;
    int     tmDigitizedAspectX;
    int     tmDigitizedAspectY;
} TEXTMETRIC;
typedef TEXTMETRIC*       PTEXTMETRIC;
typedef TEXTMETRIC NEAR* NPTEXTMETRIC;
typedef TEXTMETRIC FAR*  LPTEXTMETRIC;

/* tmPitchAndFamily values */
#define TMPF_VARIABLE_PITCH 0x01
#define TMPF_VECTOR         0x02
#define TMPF_DEVICE         0x08

/*
 * Yes, this looks bogus.  But it's because we got confused and named the
 * flag wrong.  LOGFONTs have an lfPitchAndFamily field, where FIXED_PITCH
 * is 0x01.  But TEXTMETRICs have a tmPitchAndFamily field, where
 * VARIABLE_PITCH is 0x01.  It's easy to confuse the two.  We're sorry.
*/
#define TMPF_FIXED_PITCH    TMPF_VARIABLE_PITCH

#if (WINVER >= 0x030a)
#define TMPF_TRUETYPE       0x04
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI GetTextMetrics(HDC, TEXTMETRIC FAR*);

#if (WINVER >= 0x030a)

#ifndef NOSCALABLEFONT
#if (WINVER >= 0x0400)

#define PANOSE_COUNT               10
#define PAN_FAMILYTYPE_INDEX        0
#define PAN_SERIFSTYLE_INDEX        1
#define PAN_WEIGHT_INDEX            2
#define PAN_PROPORTION_INDEX        3
#define PAN_CONTRAST_INDEX          4
#define PAN_STROKEVARIATION_INDEX   5
#define PAN_ARMSTYLE_INDEX          6
#define PAN_LETTERFORM_INDEX        7
#define PAN_MIDLINE_INDEX           8
#define PAN_XHEIGHT_INDEX           9

#define PAN_CULTURE_LATIN           0

#define PAN_ANY                         0 /* Any                            */
#define PAN_NO_FIT                      1 /* No Fit                         */

#define PAN_FAMILY_TEXT_DISPLAY         2 /* Text and Display               */
#define PAN_FAMILY_SCRIPT               3 /* Script                         */
#define PAN_FAMILY_DECORATIVE           4 /* Decorative                     */
#define PAN_FAMILY_PICTORIAL            5 /* Pictorial                      */

#define PAN_SERIF_COVE                  2 /* Cove                           */
#define PAN_SERIF_OBTUSE_COVE           3 /* Obtuse Cove                    */
#define PAN_SERIF_SQUARE_COVE           4 /* Square Cove                    */
#define PAN_SERIF_OBTUSE_SQUARE_COVE    5 /* Obtuse Square Cove             */
#define PAN_SERIF_SQUARE                6 /* Square                         */
#define PAN_SERIF_THIN                  7 /* Thin                           */
#define PAN_SERIF_BONE                  8 /* Bone                           */
#define PAN_SERIF_EXAGGERATED           9 /* Exaggerated                    */
#define PAN_SERIF_TRIANGLE             10 /* Triangle                       */
#define PAN_SERIF_NORMAL_SANS          11 /* Normal Sans                    */
#define PAN_SERIF_OBTUSE_SANS          12 /* Obtuse Sans                    */
#define PAN_SERIF_PERP_SANS            13 /* Prep Sans                      */
#define PAN_SERIF_FLARED               14 /* Flared                         */
#define PAN_SERIF_ROUNDED              15 /* Rounded                        */

#define PAN_WEIGHT_VERY_LIGHT           2 /* Very Light                     */
#define PAN_WEIGHT_LIGHT                3 /* Light                          */
#define PAN_WEIGHT_THIN                 4 /* Thin                           */
#define PAN_WEIGHT_BOOK                 5 /* Book                           */
#define PAN_WEIGHT_MEDIUM               6 /* Medium                         */
#define PAN_WEIGHT_DEMI                 7 /* Demi                           */
#define PAN_WEIGHT_BOLD                 8 /* Bold                           */
#define PAN_WEIGHT_HEAVY                9 /* Heavy                          */
#define PAN_WEIGHT_BLACK               10 /* Black                          */
#define PAN_WEIGHT_NORD                11 /* Nord                           */

#define PAN_PROP_OLD_STYLE              2 /* Old Style                      */
#define PAN_PROP_MODERN                 3 /* Modern                         */
#define PAN_PROP_EVEN_WIDTH             4 /* Even Width                     */
#define PAN_PROP_EXPANDED               5 /* Expanded                       */
#define PAN_PROP_CONDENSED              6 /* Condensed                      */
#define PAN_PROP_VERY_EXPANDED          7 /* Very Expanded                  */
#define PAN_PROP_VERY_CONDENSED         8 /* Very Condensed                 */
#define PAN_PROP_MONOSPACED             9 /* Monospaced                     */

#define PAN_CONTRAST_NONE               2 /* None                           */
#define PAN_CONTRAST_VERY_LOW           3 /* Very Low                       */
#define PAN_CONTRAST_LOW                4 /* Low                            */
#define PAN_CONTRAST_MEDIUM_LOW         5 /* Medium Low                     */
#define PAN_CONTRAST_MEDIUM             6 /* Medium                         */
#define PAN_CONTRAST_MEDIUM_HIGH        7 /* Mediim High                    */
#define PAN_CONTRAST_HIGH               8 /* High                           */
#define PAN_CONTRAST_VERY_HIGH          9 /* Very High                      */

#define PAN_STROKE_GRADUAL_DIAG         2 /* Gradual/Diagonal               */
#define PAN_STROKE_GRADUAL_TRAN         3 /* Gradual/Transitional           */
#define PAN_STROKE_GRADUAL_VERT         4 /* Gradual/Vertical               */
#define PAN_STROKE_GRADUAL_HORZ         5 /* Gradual/Horizontal             */
#define PAN_STROKE_RAPID_VERT           6 /* Rapid/Vertical                 */
#define PAN_STROKE_RAPID_HORZ           7 /* Rapid/Horizontal               */
#define PAN_STROKE_INSTANT_VERT         8 /* Instant/Vertical               */

#define PAN_STRAIGHT_ARMS_HORZ          2 /* Straight Arms/Horizontal       */
#define PAN_STRAIGHT_ARMS_WEDGE         3 /* Straight Arms/Wedge            */
#define PAN_STRAIGHT_ARMS_VERT          4 /* Straight Arms/Vertical         */
#define PAN_STRAIGHT_ARMS_SINGLE_SERIF  5 /* Straight Arms/Single-Serif     */
#define PAN_STRAIGHT_ARMS_DOUBLE_SERIF  6 /* Straight Arms/Double-Serif     */
#define PAN_BENT_ARMS_HORZ              7 /* Non-Straight Arms/Horizontal   */
#define PAN_BENT_ARMS_WEDGE             8 /* Non-Straight Arms/Wedge        */
#define PAN_BENT_ARMS_VERT              9 /* Non-Straight Arms/Vertical     */
#define PAN_BENT_ARMS_SINGLE_SERIF     10 /* Non-Straight Arms/Single-Serif */
#define PAN_BENT_ARMS_DOUBLE_SERIF     11 /* Non-Straight Arms/Double-Serif */

#define PAN_LETT_NORMAL_CONTACT         2 /* Normal/Contact                 */
#define PAN_LETT_NORMAL_WEIGHTED        3 /* Normal/Weighted                */
#define PAN_LETT_NORMAL_BOXED           4 /* Normal/Boxed                   */
#define PAN_LETT_NORMAL_FLATTENED       5 /* Normal/Flattened               */
#define PAN_LETT_NORMAL_ROUNDED         6 /* Normal/Rounded                 */
#define PAN_LETT_NORMAL_OFF_CENTER      7 /* Normal/Off Center              */
#define PAN_LETT_NORMAL_SQUARE          8 /* Normal/Square                  */
#define PAN_LETT_OBLIQUE_CONTACT        9 /* Oblique/Contact                */
#define PAN_LETT_OBLIQUE_WEIGHTED      10 /* Oblique/Weighted               */
#define PAN_LETT_OBLIQUE_BOXED         11 /* Oblique/Boxed                  */
#define PAN_LETT_OBLIQUE_FLATTENED     12 /* Oblique/Flattened              */
#define PAN_LETT_OBLIQUE_ROUNDED       13 /* Oblique/Rounded                */
#define PAN_LETT_OBLIQUE_OFF_CENTER    14 /* Oblique/Off Center             */
#define PAN_LETT_OBLIQUE_SQUARE        15 /* Oblique/Square                 */

#define PAN_MIDLINE_STANDARD_TRIMMED    2 /* Standard/Trimmed               */
#define PAN_MIDLINE_STANDARD_POINTED    3 /* Standard/Pointed               */
#define PAN_MIDLINE_STANDARD_SERIFED    4 /* Standard/Serifed               */
#define PAN_MIDLINE_HIGH_TRIMMED        5 /* High/Trimmed                   */
#define PAN_MIDLINE_HIGH_POINTED        6 /* High/Pointed                   */
#define PAN_MIDLINE_HIGH_SERIFED        7 /* High/Serifed                   */
#define PAN_MIDLINE_CONSTANT_TRIMMED    8 /* Constant/Trimmed               */
#define PAN_MIDLINE_CONSTANT_POINTED    9 /* Constant/Pointed               */
#define PAN_MIDLINE_CONSTANT_SERIFED   10 /* Constant/Serifed               */
#define PAN_MIDLINE_LOW_TRIMMED        11 /* Low/Trimmed                    */
#define PAN_MIDLINE_LOW_POINTED        12 /* Low/Pointed                    */
#define PAN_MIDLINE_LOW_SERIFED        13 /* Low/Serifed                    */

#define PAN_XHEIGHT_CONSTANT_SMALL      2 /* Constant/Small                 */
#define PAN_XHEIGHT_CONSTANT_STD        3 /* Constant/Standard              */
#define PAN_XHEIGHT_CONSTANT_LARGE      4 /* Constant/Large                 */
#define PAN_XHEIGHT_DUCKING_SMALL       5 /* Ducking/Small                  */
#define PAN_XHEIGHT_DUCKING_STD         6 /* Ducking/Standard               */
#define PAN_XHEIGHT_DUCKING_LARGE       7 /* Ducking/Large                  */

#endif   /* WINVER >= 0x0400 */

typedef struct tagOUTLINETEXTMETRIC
{
    UINT    otmSize;
    TEXTMETRIC otmTextMetrics;
    BYTE    otmFiller;
    PANOSE  otmPanoseNumber;
    UINT    otmfsSelection;
    UINT    otmfsType;
    int     otmsCharSlopeRise;
    int     otmsCharSlopeRun;
    int     otmItalicAngle;
    UINT    otmEMSquare;
    int     otmAscent;
    int     otmDescent;
    UINT    otmLineGap;
    UINT    otmsCapEmHeight;
    UINT    otmsXHeight;
    RECT    otmrcFontBox;
    int     otmMacAscent;
    int     otmMacDescent;
    UINT    otmMacLineGap;
    UINT    otmusMinimumPPEM;
    POINT   otmptSubscriptSize;
    POINT   otmptSubscriptOffset;
    POINT   otmptSuperscriptSize;
    POINT   otmptSuperscriptOffset;
    UINT    otmsStrikeoutSize;
    int     otmsStrikeoutPosition;
    int     otmsUnderscorePosition;
    int     otmsUnderscoreSize;
    PSTR    otmpFamilyName;
    PSTR    otmpFaceName;
    PSTR    otmpStyleName;
    PSTR    otmpFullName;
} OUTLINETEXTMETRIC, FAR* LPOUTLINETEXTMETRIC;

WORD    WINAPI GetOutlineTextMetrics(HDC, UINT, OUTLINETEXTMETRIC FAR*);

#endif  /* NOSCALABLEFONT */
#endif  /* WINVER >= 0x030a */


#ifndef NOGDIOBJ
#if (WINVER >= 0x030a)

/* Structure passed to FONTENUMPROC */
/* NOTE: NEWTEXTMETRIC is the same as TEXTMETRIC plus 4 new fields */
typedef struct tagNEWTEXTMETRIC
{
    int     tmHeight;
    int     tmAscent;
    int     tmDescent;
    int     tmInternalLeading;
    int     tmExternalLeading;
    int     tmAveCharWidth;
    int     tmMaxCharWidth;
    int     tmWeight;
    BYTE    tmItalic;
    BYTE    tmUnderlined;
    BYTE    tmStruckOut;
    BYTE    tmFirstChar;
    BYTE    tmLastChar;
    BYTE    tmDefaultChar;
    BYTE    tmBreakChar;
    BYTE    tmPitchAndFamily;
    BYTE    tmCharSet;
    int     tmOverhang;
    int     tmDigitizedAspectX;
    int     tmDigitizedAspectY;
    DWORD   ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRIC;
typedef NEWTEXTMETRIC*       PNEWTEXTMETRIC;
typedef NEWTEXTMETRIC NEAR* NPNEWTEXTMETRIC;
typedef NEWTEXTMETRIC FAR*  LPNEWTEXTMETRIC;

/* ntmFlags field flags */
#define NTM_REGULAR 0x00000040L
#define NTM_BOLD 0x00000020L
#define NTM_ITALIC 0x00000001L

#define LF_FULLFACESIZE     64

/* Structure passed to FONTENUMPROC */
typedef struct tagENUMLOGFONT
{
    LOGFONT elfLogFont;
    char    elfFullName[LF_FULLFACESIZE];
    char    elfStyle[LF_FACESIZE];
} ENUMLOGFONT, FAR* LPENUMLOGFONT;

#endif  /* WINVER >= 0x030a */
#endif  /* NOGDIOBJ */

#ifdef STRICT
#ifndef NOGDIOBJ

typedef int (CALLBACK* OLDFONTENUMPROC)(const LOGFONT FAR*, const TEXTMETRIC FAR*, int, LPARAM);

#if (WINVER >= 0x030a)
typedef int (CALLBACK* FONTENUMPROC)(const ENUMLOGFONT FAR*, const NEWTEXTMETRIC FAR*, int, LPARAM);

int     WINAPI EnumFontFamilies(HDC, LPCSTR, FONTENUMPROC, LPARAM);
int     WINAPI EnumFontFamiliesEx(HDC, LPLOGFONT, FONTENUMPROC, LPARAM,DWORD);

#else   /* WINVER >= 0x030a */
typedef OLDFONTENUMPROC FONTENUMPROC;
#endif  /* WINVER >= 0x030a) */

int     WINAPI EnumFonts(HDC, LPCSTR, OLDFONTENUMPROC, LPARAM);
#endif  /* NOGDIOBJ */

#else   /* STRICT */

typedef FARPROC OLDFONTENUMPROC;
typedef FARPROC FONTENUMPROC;

int     WINAPI EnumFonts(HDC, LPCSTR, OLDFONTENUMPROC, LPSTR);

#if (WINVER >= 0x030a)
int     WINAPI EnumFontFamilies(HDC, LPCSTR, FONTENUMPROC, LPSTR);
#endif  /* WINVER >= 0x030a */

#endif  /* !STRICT */

/* EnumFonts font type values */
#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0X0002
#if (WINVER >= 0x030a)
#define TRUETYPE_FONTTYPE   0x0004
#endif  /* WINVER >= 0x030a */

#endif  /* NOTEXTMETRIC */

#ifndef NOSCALABLEFONT
#if (WINVER >= 0x030a)

DWORD   WINAPI GetFontData(HDC, DWORD, DWORD, void FAR*, DWORD);
BOOL WINAPI CreateScalableFontResource(UINT, LPCSTR, LPCSTR, LPCSTR);

typedef struct tagGLYPHMETRICS
{
    UINT    gmBlackBoxX;
    UINT    gmBlackBoxY;
    POINT   gmptGlyphOrigin;
    int     gmCellIncX;
    int     gmCellIncY;
} GLYPHMETRICS, FAR* LPGLYPHMETRICS;

typedef struct tagFIXED
{
    UINT    fract;
    int     value;
} FIXED, FAR* LPFIXED;

typedef struct tagMAT2
{
    FIXED  eM11;
    FIXED  eM12;
    FIXED  eM21;
    FIXED  eM22;
} MAT2, FAR* LPMAT2;

DWORD   WINAPI GetGlyphOutline(HDC, UINT, UINT, GLYPHMETRICS FAR*, DWORD, void FAR*, const MAT2 FAR*);

/* GetGlyphOutline constants */
#define GGO_METRICS        0
#define GGO_BITMAP         1
#define GGO_NATIVE         2
#if (WINVER >= 0x0400)
#define	GGO_GRAY2_BITMAP   4
#define	GGO_GRAY4_BITMAP   5
#define	GGO_GRAY8_BITMAP   6
#endif /* WINVER >= 0x0400 */


#define TT_POLYGON_TYPE   24

#define TT_PRIM_LINE       1
#define TT_PRIM_QSPLINE    2

typedef struct tagPOINTFX
{
    FIXED x;
    FIXED y;
} POINTFX, FAR* LPPOINTFX;

typedef struct tagTTPOLYCURVE
{
    UINT    wType;
    UINT    cpfx;
    POINTFX apfx[1];
} TTPOLYCURVE, FAR* LPTTPOLYCURVE;

typedef struct tagTTPOLYGONHEADER
{
    DWORD   cb;
    DWORD   dwType;
    POINTFX pfxStart;
} TTPOLYGONHEADER, FAR* LPTTPOLYGONHEADER;

typedef struct tagABC
{
    int   abcA;
    UINT  abcB;
    int   abcC;
} ABC;
typedef ABC FAR* LPABC;

BOOL    WINAPI GetCharABCWidths(HDC, UINT, UINT, ABC FAR*);

typedef struct tagKERNINGPAIR
{
    WORD wFirst;
    WORD wSecond;
    int  iKernAmount;
} KERNINGPAIR, FAR* LPKERNINGPAIR;

int     WINAPI GetKerningPairs(HDC, int, KERNINGPAIR FAR*);

typedef struct tagRASTERIZER_STATUS
{
    int   nSize;
    int   wFlags;
    int   nLanguageID;
} RASTERIZER_STATUS;
typedef RASTERIZER_STATUS FAR* LPRASTERIZER_STATUS;

/* bits defined in wFlags of RASTERIZER_STATUS */
#define TT_AVAILABLE 0x0001
#define TT_ENABLED 0x0002

BOOL    WINAPI GetRasterizerCaps(RASTERIZER_STATUS FAR*, int);

#endif  /* WINVER >= 0x030a */
#endif  /* NOSCALABLEFONT */

/****** Bitmap support ******************************************************/

#ifndef NOBITMAP
typedef struct tagBITMAP
{
    int     bmType;
    int     bmWidth;
    int     bmHeight;
    int     bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    void FAR* bmBits;
} BITMAP;
typedef BITMAP*       PBITMAP;
typedef BITMAP NEAR* NPBITMAP;
typedef BITMAP FAR*  LPBITMAP;

/* Bitmap Header structures */
typedef struct tagRGBTRIPLE
{
    BYTE    rgbtBlue;
    BYTE    rgbtGreen;
    BYTE    rgbtRed;
} RGBTRIPLE;
typedef RGBTRIPLE FAR* LPRGBTRIPLE;

typedef struct tagRGBQUAD
{
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;
typedef RGBQUAD FAR* LPRGBQUAD;

/* structures for defining DIBs */
typedef struct tagBITMAPCOREHEADER
{
    DWORD   bcSize;
    short   bcWidth;
    short   bcHeight;
    WORD    bcPlanes;
    WORD    bcBitCount;
} BITMAPCOREHEADER;
typedef BITMAPCOREHEADER*      PBITMAPCOREHEADER;
typedef BITMAPCOREHEADER FAR* LPBITMAPCOREHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD   biSize;
    LONG    biWidth;
    LONG    biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biCompression;
    DWORD   biSizeImage;
    LONG    biXPelsPerMeter;
    LONG    biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
} BITMAPINFOHEADER;
typedef BITMAPINFOHEADER*      PBITMAPINFOHEADER;
typedef BITMAPINFOHEADER FAR* LPBITMAPINFOHEADER;

/* constants for the biCompression field */
#define BI_RGB       0L
#define BI_RLE8      1L
#define BI_RLE4      2L
#define BI_BITFIELDS 3L

typedef struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD      bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO*     PBITMAPINFO;
typedef BITMAPINFO FAR* LPBITMAPINFO;

typedef struct tagBITMAPCOREINFO
{
    BITMAPCOREHEADER bmciHeader;
    RGBTRIPLE      bmciColors[1];
} BITMAPCOREINFO;
typedef BITMAPCOREINFO*      PBITMAPCOREINFO;
typedef BITMAPCOREINFO FAR* LPBITMAPCOREINFO;

typedef struct tagBITMAPFILEHEADER
{
    UINT    bfType;
    DWORD   bfSize;
    UINT    bfReserved1;
    UINT    bfReserved2;
    DWORD   bfOffBits;
} BITMAPFILEHEADER;
typedef BITMAPFILEHEADER*      PBITMAPFILEHEADER;
typedef BITMAPFILEHEADER FAR* LPBITMAPFILEHEADER;

HBITMAP WINAPI CreateBitmap(int, int, UINT, UINT, const void FAR*);
HBITMAP WINAPI CreateBitmapIndirect(BITMAP FAR* );
HBITMAP WINAPI CreateCompatibleBitmap(HDC, int, int);
HBITMAP WINAPI CreateDiscardableBitmap(HDC, int, int);
HBITMAP WINAPI CreateDIBitmap(HDC, BITMAPINFOHEADER FAR*, DWORD, const void FAR*, BITMAPINFO FAR*, UINT);

HBITMAP WINAPI LoadBitmap(HINSTANCE, LPCSTR);

/* DIB color table identifiers */
#define DIB_RGB_COLORS  0
#define DIB_PAL_COLORS  1

/* constants for CreateDIBitmap */
#define CBM_INIT        0x00000004L
#endif /* NOBITMAP */

#ifndef NORASTEROPS

/* Binary raster ops */
#define R2_BLACK            1
#define R2_NOTMERGEPEN      2
#define R2_MASKNOTPEN       3
#define R2_NOTCOPYPEN       4
#define R2_MASKPENNOT       5
#define R2_NOT              6
#define R2_XORPEN           7
#define R2_NOTMASKPEN       8
#define R2_MASKPEN          9
#define R2_NOTXORPEN        10
#define R2_NOP              11
#define R2_MERGENOTPEN      12
#define R2_COPYPEN          13
#define R2_MERGEPENNOT      14
#define R2_MERGEPEN         15
#define R2_WHITE            16

/* Ternary raster operations */
#define SRCCOPY             0x00CC0020L
#define SRCPAINT            0x00EE0086L
#define SRCAND              0x008800C6L
#define SRCINVERT           0x00660046L
#define SRCERASE            0x00440328L
#define NOTSRCCOPY          0x00330008L
#define NOTSRCERASE         0x001100A6L
#define MERGECOPY           0x00C000CAL
#define MERGEPAINT          0x00BB0226L
#define PATCOPY             0x00F00021L
#define PATOR               0x00FA0089L
#define PATPAINT            0x00FB0A09L
#define PATINVERT           0x005A0049L
#define DSTINVERT           0x00550009L
#define BLACKNESS           0x00000042L
#define WHITENESS           0x00FF0062L
#define SRCSTENCIL          0x00B8074AL
#define SRCINVSTENCIL       0x00E20746L

#endif  /* NORASTEROPS */

#ifndef NOBITMAP
BOOL    WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);

BOOL    WINAPI PatBlt(HDC, int, int, int, int, DWORD);

BOOL    WINAPI StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
int     WINAPI StretchDIBits(HDC, int, int, int, int, int,
                        int, int, int, const void FAR*, LPBITMAPINFO, UINT, DWORD);

COLORREF WINAPI SetPixel(HDC, int, int, COLORREF);
COLORREF WINAPI GetPixel(HDC, int, int);

/* StretchBlt() Modes */
#define BLACKONWHITE 1
#define WHITEONBLACK 2
#define COLORONCOLOR 3

/* new StretchBlt() Modes (simpler names) */
#define STRETCH_ANDSCANS        1
#define STRETCH_ORSCANS         2
#define STRETCH_DELETESCANS     3


#if (WINVER >= 0x0400)
#define AVERAGE             5
#define STRETCH_AVESCANS    5
#endif /* WINVER >= 0x0400 */

int     WINAPI SetStretchBltMode(HDC, int);
int     WINAPI GetStretchBltMode(HDC);

DWORD   WINAPI SetBitmapDimension(HBITMAP, int, int);
DWORD   WINAPI GetBitmapDimension(HBITMAP);
#if (WINVER >= 0x030a)
BOOL    WINAPI SetBitmapDimensionEx(HBITMAP, int, int, SIZE FAR*);
BOOL    WINAPI GetBitmapDimensionEx(HBITMAP, SIZE FAR*);
#endif  /* WINVER >= 0x030a */
int     WINAPI SetROP2(HDC, int);
int     WINAPI GetROP2(HDC);

LONG    WINAPI SetBitmapBits(HBITMAP, DWORD, const void FAR*);
LONG    WINAPI GetBitmapBits(HBITMAP, LONG, void FAR*);

int     WINAPI SetDIBits(HDC, HBITMAP, UINT, UINT, const void FAR*, BITMAPINFO FAR*, UINT);
int     WINAPI GetDIBits(HDC, HBITMAP, UINT, UINT, void FAR*, BITMAPINFO FAR*, UINT);

int     WINAPI SetDIBitsToDevice(HDC, int, int, int, int, int, int, UINT, UINT,
                    void FAR*, BITMAPINFO FAR*, UINT);
#endif /* NOBITMAP */

/****** Metafile support ****************************************************/

#ifndef NOMETAFILE

DECLARE_HANDLE(HMETAFILE);

HDC     WINAPI CreateMetaFile(LPCSTR);
HMETAFILE WINAPI CloseMetaFile(HDC);

HMETAFILE WINAPI GetMetaFile(LPCSTR);
BOOL      WINAPI DeleteMetaFile(HMETAFILE);
HMETAFILE WINAPI CopyMetaFile(HMETAFILE, LPCSTR);

BOOL    WINAPI PlayMetaFile(HDC, HMETAFILE);

HGLOBAL WINAPI GetMetaFileBits(HMETAFILE);
HMETAFILE WINAPI SetMetaFileBits(HGLOBAL);
#if (WINVER >= 0x030a)
HMETAFILE WINAPI SetMetaFileBitsBetter(HGLOBAL);
#endif  /* WINVER >= 0x030a */

/* Clipboard Metafile Picture Structure */
typedef struct tagMETAFILEPICT
{
    int     mm;
    int     xExt;
    int     yExt;
    HMETAFILE hMF;
} METAFILEPICT;
typedef METAFILEPICT FAR* LPMETAFILEPICT;

typedef struct tagMETAHEADER
{
    UINT    mtType;
    UINT    mtHeaderSize;
    UINT    mtVersion;
    DWORD   mtSize;
    UINT    mtNoObjects;
    DWORD   mtMaxRecord;
    UINT    mtNoParameters;
} METAHEADER;

typedef struct tagHANDLETABLE
{
    HGDIOBJ objectHandle[1];
} HANDLETABLE;
typedef HANDLETABLE*      PHANDLETABLE;
typedef HANDLETABLE FAR* LPHANDLETABLE;

typedef struct tagMETARECORD
{
    DWORD   rdSize;
    UINT    rdFunction;
    UINT    rdParm[1];
} METARECORD;
typedef METARECORD*      PMETARECORD;
typedef METARECORD FAR* LPMETARECORD;

/* Metafile Functions */
#define META_SETBKCOLOR             0x0201
#define META_SETBKMODE              0x0102
#define META_SETMAPMODE             0x0103
#define META_SETROP2                0x0104
#define META_SETRELABS              0x0105
#define META_SETPOLYFILLMODE        0x0106
#define META_SETSTRETCHBLTMODE      0x0107
#define META_SETTEXTCHAREXTRA       0x0108
#define META_SETTEXTCOLOR           0x0209
#define META_SETTEXTJUSTIFICATION   0x020A
#define META_SETWINDOWORG           0x020B
#define META_SETWINDOWEXT           0x020C
#define META_SETVIEWPORTORG         0x020D
#define META_SETVIEWPORTEXT         0x020E
#define META_OFFSETWINDOWORG        0x020F
#define META_SCALEWINDOWEXT         0x0410
#define META_OFFSETVIEWPORTORG      0x0211
#define META_SCALEVIEWPORTEXT       0x0412
#define META_LINETO                 0x0213
#define META_MOVETO                 0x0214
#define META_EXCLUDECLIPRECT        0x0415
#define META_INTERSECTCLIPRECT      0x0416
#define META_ARC                    0x0817
#define META_ELLIPSE                0x0418
#define META_FLOODFILL              0x0419
#define META_PIE                    0x081A
#define META_RECTANGLE              0x041B
#define META_ROUNDRECT              0x061C
#define META_PATBLT                 0x061D
#define META_SAVEDC                 0x001E
#define META_SETPIXEL               0x041F
#define META_OFFSETCLIPRGN          0x0220
#define META_TEXTOUT                0x0521
#define META_BITBLT                 0x0922
#define META_STRETCHBLT             0x0B23
#define META_POLYGON                0x0324
#define META_POLYLINE               0x0325
#define META_ESCAPE                 0x0626
#define META_RESTOREDC              0x0127
#define META_FILLREGION             0x0228
#define META_FRAMEREGION            0x0429
#define META_INVERTREGION           0x012A
#define META_PAINTREGION            0x012B
#define META_SELECTCLIPREGION       0x012C
#define META_SELECTOBJECT           0x012D
#define META_SETTEXTALIGN           0x012E
#define META_DRAWTEXT               0x062F

#define META_CHORD                  0x0830
#define META_SETMAPPERFLAGS         0x0231
#define META_EXTTEXTOUT             0x0a32
#define META_SETDIBTODEV            0x0d33
#define META_SELECTPALETTE          0x0234
#define META_REALIZEPALETTE         0x0035
#define META_ANIMATEPALETTE         0x0436
#define META_SETPALENTRIES          0x0037
#define META_POLYPOLYGON            0x0538
#define META_RESIZEPALETTE          0x0139

#define META_DIBBITBLT              0x0940
#define META_DIBSTRETCHBLT          0x0b41
#define META_DIBCREATEPATTERNBRUSH  0x0142
#define META_STRETCHDIB             0x0f43

#define META_EXTFLOODFILL           0x0548

#define META_RESETDC                0x014C
#define META_STARTDOC               0x014D
#define META_STARTPAGE              0x004F
#define META_ENDPAGE                0x0050
#define META_ABORTDOC               0x0052
#define META_ENDDOC                 0x005E

#define META_DELETEOBJECT           0x01f0

#define META_CREATEPALETTE          0x00f7
#define META_CREATEBRUSH            0x00F8
#define META_CREATEPATTERNBRUSH     0x01F9
#define META_CREATEPENINDIRECT      0x02FA
#define META_CREATEFONTINDIRECT     0x02FB
#define META_CREATEBRUSHINDIRECT    0x02FC
#define META_CREATEBITMAPINDIRECT   0x02FD
#define META_CREATEBITMAP           0x06FE
#define META_CREATEREGION           0x06FF

void    WINAPI PlayMetaFileRecord(HDC, HANDLETABLE FAR*, METARECORD FAR*, UINT);

#ifdef STRICT
typedef int (CALLBACK* MFENUMPROC)(HDC, HANDLETABLE FAR*, METARECORD FAR*, int, LPARAM);
#else
typedef FARPROC MFENUMPROC;
#endif

BOOL    WINAPI EnumMetaFile(HDC, HMETAFILE, MFENUMPROC, LPARAM);

#endif  /* NOMETAFILE */

/****** Printing support ****************************************************/

#ifdef STRICT
typedef BOOL (CALLBACK* ABORTPROC)(HDC, int);
#else
typedef FARPROC ABORTPROC;
#endif

#if (WINVER >= 0x030a)

int WINAPI SetAbortProc(HDC, ABORTPROC);

HANDLE  WINAPI SpoolFile(LPSTR, LPSTR, LPSTR, LPSTR);

#endif  /* WINVER >= 0x030a */

typedef struct {
    short   cbSize;
    LPSTR   lpszDocName;
    LPSTR   lpszOutput;
    WORD    fwType;
    }	DOCINFO, FAR * LPDOCINFO;

#define DI_RAW		    0x0001
#define DI_DIRECT	    0x0002

int     WINAPI StartDoc(HDC, DOCINFO FAR*);
int     WINAPI StartPage(HDC);
int     WINAPI EndPage(HDC);
int     WINAPI EndDoc(HDC);
int     WINAPI AbortDoc(HDC);
BOOL    WINAPI QueryAbort(HDC, int);

/* Spooler Error Codes */
#define SP_NOTREPORTED      0x4000
#define SP_ERROR            (-1)
#define SP_APPABORT         (-2)
#define SP_USERABORT        (-3)
#define SP_OUTOFDISK        (-4)
#define SP_OUTOFMEMORY      (-5)

#define PR_JOBSTATUS        0x0000

#endif  /* NOGDI  */

#ifndef NOGDI

/******* GDI Escape support *************************************************/

int     WINAPI Escape(HDC, int, int, LPCSTR, void FAR*);

/* GDI Escapes */
#define NEWFRAME                1
#define ABORTDOC                2
#define NEXTBAND                3
#define SETCOLORTABLE           4
#define GETCOLORTABLE           5
#define FLUSHOUTPUT             6
#define DRAFTMODE               7
#define QUERYESCSUPPORT         8
#define SETABORTPROC            9
#define STARTDOC                10
#define ENDDOC                  11
#define GETPHYSPAGESIZE         12
#define GETPRINTINGOFFSET       13
#define GETSCALINGFACTOR        14
#define MFCOMMENT               15
#define GETPENWIDTH             16
#define SETCOPYCOUNT            17
#define SELECTPAPERSOURCE       18
#define DEVICEDATA              19
#define PASSTHROUGH             19
#define GETTECHNOLGY            20
#define GETTECHNOLOGY           20
#define SETLINECAP              21
#define SETLINEJOIN             22
#define SETMITERLIMIT           23
#define BANDINFO                24
#define DRAWPATTERNRECT         25
#define GETVECTORPENSIZE        26
#define GETVECTORBRUSHSIZE      27
#define ENABLEDUPLEX            28
#define GETSETPAPERBINS         29
#define GETSETPRINTORIENT       30
#define ENUMPAPERBINS           31
#define SETDIBSCALING           32
#define EPSPRINTING             33
#define ENUMPAPERMETRICS        34
#define GETSETPAPERMETRICS      35
#define POSTSCRIPT_DATA         37
#define POSTSCRIPT_IGNORE       38
#define MOUSETRAILS             39

#define GETEXTENDEDTEXTMETRICS  256
#define GETEXTENTTABLE          257
#define GETPAIRKERNTABLE        258
#define GETTRACKKERNTABLE       259
#define EXTTEXTOUT              512
#define GETFACENAME             513
#define ENABLERELATIVEWIDTHS    768
#define ENABLEPAIRKERNING       769
#define SETKERNTRACK            770
#define SETALLJUSTVALUES        771
#define SETCHARSET              772

#define STRETCHBLT              2048
#define GETSETSCREENPARAMS      3072
#define QUERYDIBSUPPORT         3073
#define QDI_SETDIBITS           0x0001
#define QDI_GETDIBITS           0x0002
#define QDI_DIBTOSCREEN         0x0004
#define QDI_STRETCHDIB          0x0008

#define BEGIN_PATH              4096
#define CLIP_TO_PATH            4097
#define END_PATH                4098
#define EXT_DEVICE_CAPS         4099
#define RESTORE_CTM             4100
#define SAVE_CTM                4101
#define SET_ARC_DIRECTION       4102
#define SET_BACKGROUND_COLOR    4103
#define SET_POLY_MODE           4104
#define SET_SCREEN_ANGLE        4105
#define SET_SPREAD              4106
#define TRANSFORM_CTM           4107
#define SET_CLIP_BOX            4108
#define SET_BOUNDS              4109

#endif  /* NOGDI */

/****** USER typedefs, structures, and functions *****************************/

DECLARE_HANDLE(HWND);

#ifndef NOUSER

DECLARE_HANDLE(HMENU);

DECLARE_HANDLE(HICON);
typedef HICON HCURSOR;     /* HICONs & HCURSORs are polymorphic */


/****** System Metrics *******************************************************/

#ifndef NOSYSMETRICS

int WINAPI GetSystemMetrics(int);

/* GetSystemMetrics() codes */
#define SM_CXSCREEN             0
#define SM_CYSCREEN             1
#define SM_CXVSCROLL            2
#define SM_CYHSCROLL            3
#define SM_CYCAPTION            4
#define SM_CXBORDER             5
#define SM_CYBORDER             6
#define SM_CXFIXEDFRAME         7
#define SM_CYFIXEDFRAME         8
#define SM_CYVTHUMB             9
#define SM_CXHTHUMB             10
#define SM_CXICON               11
#define SM_CYICON               12
#define SM_CXCURSOR             13
#define SM_CYCURSOR             14
#define SM_CYMENU               15
#define SM_CXFULLSCREEN         16
#define SM_CYFULLSCREEN         17
#define SM_CYKANJIWINDOW        18
#define SM_MOUSEPRESENT         19
#define SM_CYVSCROLL            20
#define SM_CXHSCROLL            21
#define SM_DEBUG                22
#define SM_SWAPBUTTON           23
#define SM_RESERVED1            24
#define SM_RESERVED2            25
#define SM_RESERVED3            26
#define SM_RESERVED4            27
#define SM_CXMIN                28
#define SM_CYMIN                29
#define SM_CXSIZE               30
#define SM_CYSIZE               31
#define SM_CXSIZEFRAME          32
#define SM_CYSIZEFRAME          33
#define SM_CXMINTRACK           34
#define SM_CYMINTRACK           35

#if (WINVER >= 0x030a)
#define SM_CXDOUBLECLK          36
#define SM_CYDOUBLECLK          37
#define SM_CXICONSPACING        38
#define SM_CYICONSPACING        39
#define SM_MENUDROPALIGNMENT    40
#define SM_PENWINDOWS           41
#define SM_DBCSENABLED          42
#endif  /* WINVER >= 0x030a */

#define SM_CXDLGFRAME           SM_CXFIXEDFRAME
#define SM_CYDLGFRAME           SM_CYFIXEDFRAME
#define SM_CXFRAME              SM_CXSIZEFRAME
#define SM_CYFRAME              SM_CYSIZEFRAME
#define SM_CMETRICS             81

#endif  /* NOSYSMETRICS */

UINT    WINAPI GetDoubleClickTime(void);
void    WINAPI SetDoubleClickTime(UINT);


/****** System Parameters support ********************************************/

#if (WINVER >= 0x030a)

#ifndef NOSYSTEMPARAMSINFO

BOOL    WINAPI SystemParametersInfo(UINT, UINT, VOID FAR*, UINT);

#define SPI_GETBEEP                 1
#define SPI_SETBEEP                 2
#define SPI_GETMOUSE                3
#define SPI_SETMOUSE                4
#define SPI_GETBORDER               5
#define SPI_SETBORDER               6
#define SPI_GETKEYBOARDSPEED        10
#define SPI_SETKEYBOARDSPEED        11
#define SPI_LANGDRIVER              12
#define SPI_ICONHORIZONTALSPACING   13
#define SPI_GETSCREENSAVETIMEOUT    14
#define SPI_SETSCREENSAVETIMEOUT    15
#define SPI_GETSCREENSAVEACTIVE     16
#define SPI_SETSCREENSAVEACTIVE     17
#define SPI_GETGRIDGRANULARITY      18
#define SPI_SETGRIDGRANULARITY      19
#define SPI_SETDESKWALLPAPER        20
#define SPI_SETDESKPATTERN          21
#define SPI_GETKEYBOARDDELAY        22
#define SPI_SETKEYBOARDDELAY        23
#define SPI_ICONVERTICALSPACING     24
#define SPI_GETICONTITLEWRAP        25
#define SPI_SETICONTITLEWRAP        26
#define SPI_GETMENUDROPALIGNMENT    27
#define SPI_SETMENUDROPALIGNMENT    28
#define SPI_SETDOUBLECLKWIDTH       29
#define SPI_SETDOUBLECLKHEIGHT      30
#define SPI_GETICONTITLELOGFONT     31
#define SPI_SETDOUBLECLICKTIME      32
#define SPI_SETMOUSEBUTTONSWAP      33
#define SPI_SETICONTITLELOGFONT     34
#define SPI_GETFASTTASKSWITCH       35
#define SPI_SETFASTTASKSWITCH       36

/* SystemParametersInfo flags */
#define SPIF_UPDATEINIFILE          0x0001
#define SPIF_SENDCHANGE             0x0002
#define SPIF_SENDWININICHANGE       SPIF_SENDCHANGE

#endif  /* NOSYSTEMPARAMSINFO  */
#endif  /* WINVER >= 0x030a */

/****** Rectangle support ****************************************************/

void    WINAPI SetRect(LPRECT, int, int, int, int);
void    WINAPI SetRectEmpty(LPRECT);

void    WINAPI CopyRect(LPRECT, LPCRECT);

BOOL    WINAPI IsRectEmpty(LPCRECT);

BOOL    WINAPI EqualRect(LPCRECT, LPCRECT);

BOOL    WINAPI IntersectRect(LPRECT, LPCRECT, LPCRECT);
BOOL    WINAPI UnionRect(LPRECT, LPCRECT, LPCRECT);
BOOL    WINAPI SubtractRect(LPRECT, LPCRECT, LPCRECT);

void    WINAPI OffsetRect(LPRECT, int, int);
void    WINAPI InflateRect(LPRECT, int, int);

BOOL    WINAPI PtInRect(LPCRECT, POINT);

/****** Window message support ***********************************************/

UINT WINAPI RegisterWindowMessage(LPCSTR);

#ifndef NOMSG

/* Queued message structure */
typedef struct tagMSG
{
    HWND    hwnd;
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
    DWORD   time;
    POINT   pt;
} MSG, * PMSG, NEAR* NPMSG, FAR* LPMSG;

BOOL    WINAPI GetMessage(LPMSG, HWND, UINT, UINT);
BOOL    WINAPI PeekMessage(LPMSG, HWND, UINT, UINT, UINT);

/* PeekMessage() options */
#define PM_NOREMOVE     0x0000
#define PM_REMOVE       0x0001
#define PM_NOYIELD      0x0002

void    WINAPI WaitMessage(void);

DWORD   WINAPI GetMessagePos(void);
LONG    WINAPI GetMessageTime(void);
#if (WINVER >= 0x030a)
LPARAM  WINAPI GetMessageExtraInfo(void);
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI TranslateMessage(const MSG FAR*);
LONG    WINAPI DispatchMessage(const MSG FAR*);

BOOL    WINAPI SetMessageQueue(int);

BOOL    WINAPI GetInputState(void);

#if (WINVER >= 0x030a)
DWORD   WINAPI GetQueueStatus(UINT flags);

/* GetQueueStatus flags */
#define QS_KEY          0x0001
#define QS_MOUSEMOVE    0x0002
#define QS_MOUSEBUTTON  0x0004
#define QS_MOUSE        (QS_MOUSEMOVE | QS_MOUSEBUTTON)
#define QS_POSTMESSAGE  0x0008
#define QS_TIMER        0x0010
#define QS_PAINT        0x0020
#define QS_SENDMESSAGE  0x0040
#define QS_ALLINPUT     0x007f
#endif  /* WINVER >= 0x030a */

#endif  /* NOMSG */

BOOL    WINAPI PostMessage(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI SendMessage(HWND, UINT, WPARAM, LPARAM);

#if (WINVER >= 0x0400)
long  WINAPI  BroadcastSystemMessage(DWORD, LPDWORD, UINT, WPARAM, LPARAM);
                      
#define BSM_ALLCOMPONENTS       0x00000000
#define BSM_VXDS                0x00000001
#define BSM_NETDRIVER           0x00000002
#define BSM_INSTALLABLEDRIVERS  0x00000004
#define BSM_APPLICATIONS        0x00000008
                                          
#define BSF_QUERY               0x00000001
#define BSF_IGNORECURRENTTASK   0x00000002
#define BSF_FLUSHDISK           0x00000004
#define BSF_NOHANG              0x00000008
#define BSF_POSTMESSAGE         0x00000010
#define BSF_FORCEIFHUNG         0x00000020

/* Flags for wParam of WM_DEVICEBROADCAST message */
#define DBWF_LPARAMPOINTER	0x8000		
#define DBWF_LPARAMSTRING 	0x4000		
                                                
typedef struct tagBROADCASTSYSMSG
{
    UINT    uiMessage;                          
    WPARAM  wParam;                             
    LPARAM  lParam;                             
} BROADCASTSYSMSG;                              
                                                
typedef BROADCASTSYSMSG  FAR *LPBROADCASTSYSMSG;

#endif /* WINVER >= 0x0400 */
#ifndef NOMSG

BOOL    WINAPI PostAppMessage(HTASK, UINT, WPARAM, LPARAM);

void    WINAPI ReplyMessage(LRESULT);
BOOL    WINAPI InSendMessage(void);

/* Special HWND value for use with PostMessage() and SendMessage() */
#define HWND_BROADCAST ((HWND)-1)


BOOL WINAPI CallMsgFilter(LPMSG, int);

#define WH_GETMESSAGE           3

#define WH_CALLWNDPROC          4

#define WH_MSGFILTER            (-1)
#define WH_SYSMSGFILTER         6

/* CallMsgFilter() and WH_SYS/MSGFILTER context codes */
#define MSGF_DIALOGBOX          0
#define MSGF_MENU               2
#define MSGF_MOVE               3
#define MSGF_SIZE               4
#define MSGF_SCROLLBAR          5
#define MSGF_NEXTWINDOW         6
#define MSGF_MAINLOOP           8
#define MSGF_USER               4096
#endif  /* NOMSG */

/* Standard window messages */
/* NOTE: All messages below 0x0400 are RESERVED by Windows */
#define WM_NULL                 0x0000
#define WM_CREATE               0x0001
#define WM_DESTROY              0x0002
#define WM_MOVE                 0x0003
#define WM_SIZE                 0x0005
#define WM_ACTIVATE             0x0006
#define WM_SETFOCUS             0x0007
#define WM_KILLFOCUS            0x0008
#define WM_ENABLE               0x000A
#define WM_SETREDRAW            0x000B
#define WM_SETTEXT              0x000C
#define WM_GETTEXT              0x000D
#define WM_GETTEXTLENGTH        0x000E
#define WM_PAINT                0x000F

#define WM_CLOSE                0x0010
#define WM_QUERYENDSESSION      0x0011
#define WM_QUIT                 0x0012
#define WM_QUERYOPEN            0x0013
#define WM_ERASEBKGND           0x0014
#define WM_SYSCOLORCHANGE       0x0015
#define WM_ENDSESSION           0x0016
#define WM_SHOWWINDOW           0x0018
#define WM_CTLCOLOR             0x0019
#define WM_SETTINGCHANGE        0x001A
#define WM_WININICHANGE         WM_SETTINGCHANGE
#define WM_DEVMODECHANGE        0x001B
#define WM_ACTIVATEAPP          0x001C
#define WM_FONTCHANGE           0x001D
#define WM_TIMECHANGE           0x001E
#define WM_CANCELMODE           0x001F

#define WM_SETCURSOR            0x0020
#define WM_MOUSEACTIVATE        0x0021
#define WM_CHILDACTIVATE        0x0022
#define WM_QUEUESYNC            0x0023
#define WM_GETMINMAXINFO        0x0024
#define WM_ICONERASEBKGND       0x0027
#define WM_NEXTDLGCTL           0x0028
#define WM_SPOOLERSTATUS        0x002A
#define WM_DRAWITEM             0x002B
#define WM_MEASUREITEM          0x002C
#define WM_DELETEITEM           0x002D
#define WM_VKEYTOITEM           0x002E
#define WM_CHARTOITEM           0x002F

#define WM_SETFONT              0x0030
#define WM_GETFONT              0x0031
#define WM_QUERYDRAGICON        0x0037
#define WM_COMPAREITEM          0x0039

#define WM_COMPACTING           0x0041
#define WM_COMMNOTIFY           0x0044
#define WM_WINDOWPOSCHANGING    0x0046
#define WM_WINDOWPOSCHANGED     0x0047
#define WM_POWER                0x0048
#define WM_NOTIFY               0x004E

#define WM_TCARD		        0x0052
#define WM_HELP                 0x0053
#define WM_USERCHANGED		    0x0054



#define WM_NCCREATE             0x0081
#define WM_NCDESTROY            0x0082
#define WM_NCCALCSIZE           0x0083
#define WM_NCHITTEST            0x0084
#define WM_NCPAINT              0x0085
#define WM_NCACTIVATE           0x0086
#define WM_GETDLGCODE           0x0087

#define WM_NCMOUSEMOVE          0x00A0
#define WM_NCLBUTTONDOWN        0x00A1
#define WM_NCLBUTTONUP          0x00A2
#define WM_NCLBUTTONDBLCLK      0x00A3
#define WM_NCRBUTTONDOWN        0x00A4
#define WM_NCRBUTTONUP          0x00A5
#define WM_NCRBUTTONDBLCLK      0x00A6
#define WM_NCMBUTTONDOWN        0x00A7
#define WM_NCMBUTTONUP          0x00A8
#define WM_NCMBUTTONDBLCLK      0x00A9


#define WM_KEYFIRST             0x0100
#define WM_KEYDOWN              0x0100
#define WM_KEYUP                0x0101
#define WM_CHAR                 0x0102
#define WM_DEADCHAR             0x0103
#define WM_SYSKEYDOWN           0x0104
#define WM_SYSKEYUP             0x0105
#define WM_SYSCHAR              0x0106
#define WM_SYSDEADCHAR          0x0107
#define WM_KEYLAST              0x0108

#define WM_INITDIALOG           0x0110
#define WM_COMMAND              0x0111
#define WM_SYSCOMMAND           0x0112
#define WM_TIMER                0x0113
#define WM_HSCROLL              0x0114
#define WM_VSCROLL              0x0115
#define WM_INITMENU             0x0116
#define WM_INITMENUPOPUP        0x0117
#define WM_MENUSELECT           0x011F
#define WM_MENUCHAR             0x0120

#define WM_ENTERIDLE            0x0121

#define WM_MOUSEFIRST           0x0200
#define WM_MOUSEMOVE            0x0200
#define WM_LBUTTONDOWN          0x0201
#define WM_LBUTTONUP            0x0202
#define WM_LBUTTONDBLCLK        0x0203
#define WM_RBUTTONDOWN          0x0204
#define WM_RBUTTONUP            0x0205
#define WM_RBUTTONDBLCLK        0x0206
#define WM_MBUTTONDOWN          0x0207
#define WM_MBUTTONUP            0x0208
#define WM_MBUTTONDBLCLK        0x0209
#define WM_MOUSELAST            0x0209

#define WM_PARENTNOTIFY         0x0210

#define WM_MDICREATE            0x0220
#define WM_MDIDESTROY           0x0221
#define WM_MDIACTIVATE          0x0222
#define WM_MDIRESTORE           0x0223
#define WM_MDINEXT              0x0224
#define WM_MDIMAXIMIZE          0x0225
#define WM_MDITILE              0x0226
#define WM_MDICASCADE           0x0227
#define WM_MDIICONARRANGE       0x0228
#define WM_MDIGETACTIVE         0x0229

#define WM_MDISETMENU           0x0230
#define WM_DROPFILES            0x0233

#define WM_CUT                  0x0300
#define WM_COPY                 0x0301
#define WM_PASTE                0x0302
#define WM_CLEAR                0x0303
#define WM_UNDO                 0x0304
#define WM_RENDERFORMAT         0x0305
#define WM_RENDERALLFORMATS     0x0306
#define WM_DESTROYCLIPBOARD     0x0307
#define WM_DRAWCLIPBOARD        0x0308
#define WM_PAINTCLIPBOARD       0x0309
#define WM_SIZECLIPBOARD        0x030B
#define WM_VSCROLLCLIPBOARD     0x030A
#define WM_ASKCBFORMATNAME      0x030C
#define WM_CHANGECBCHAIN        0x030D
#define WM_HSCROLLCLIPBOARD     0x030E
#define WM_QUERYNEWPALETTE      0x030F

#define WM_PALETTEISCHANGING    0x0310
#define WM_PALETTECHANGED       0x0311
#define WM_PRINT                0x0317
#define WM_PRINTCLIENT          0x0318

/* WinPad specific messages */
#define WM_HANDHELDFIRST        0x0358
#define WM_HANDHELDLAST         0x035F

/* PenWindows specific messages */
#define WM_PENWINFIRST          0x0380
#define WM_PENWINLAST           0x038F

/* Coalescing messages */
#define WM_COALESCE_FIRST           0x0390
#define WM_COALESCE_LAST            0x039F

#define WM_USER                 0x0400

/* NOTE: All messages between WM_APP and 0xBFFF are for exclusive use by apps */
#define WM_APP                  0x8000

#if (WINVER >= 0x030a)
/****** Power management ****************************************************/

/* wParam for WM_POWER window message and DRV_POWER driver notification */
#define PWR_OK              1
#define PWR_FAIL            (-1)
#define PWR_SUSPENDREQUEST  1
#define PWR_SUSPENDRESUME   2
#define PWR_CRITICALRESUME  3
#endif  /* WINVER >= 0x030a */

#if (WINVER >= 0x0400)
/*  wParam for WM_SIZING message  */
#define WMSZ_LEFT           1
#define WMSZ_RIGHT          2
#define WMSZ_TOP            3
#define WMSZ_TOPLEFT        4
#define WMSZ_TOPRIGHT       5
#define WMSZ_BOTTOM         6
#define WMSZ_BOTTOMLEFT     7
#define WMSZ_BOTTOMRIGHT    8

#endif  /* WINVER >= 0x400 */

/****** Application termination *********************************************/

void    WINAPI PostQuitMessage(int);

/****** Window class management *********************************************/

typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagWNDCLASS
{
    UINT        style;
    WNDPROC     lpfnWndProc;
    int         cbClsExtra;
    int         cbWndExtra;
    HINSTANCE   hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
} WNDCLASS;
typedef WNDCLASS* PWNDCLASS;
typedef WNDCLASS NEAR* NPWNDCLASS;
typedef WNDCLASS FAR* LPWNDCLASS;

ATOM    WINAPI RegisterClass(const WNDCLASS FAR*);
BOOL    WINAPI UnregisterClass(LPCSTR, HINSTANCE);

BOOL    WINAPI GetClassInfo(HINSTANCE, LPCSTR, LPWNDCLASS);
int     WINAPI GetClassName(HWND, LPSTR, int);

#ifndef NOWINSTYLES

/* Class styles */
#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_KEYCVTWINDOW     0x0004
#define CS_DBLCLKS          0x0008

#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080

#define CS_NOKEYCVT         0x0100
#define CS_NOCLOSE          0x0200
#define CS_SAVEBITS         0x0800

#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_GLOBALCLASS      0x4000

#endif /* NOWINSTYLES */

#ifndef NOWINOFFSETS

WORD    WINAPI GetClassWord(HWND, int);
WORD    WINAPI SetClassWord(HWND, int, WORD);
LONG    WINAPI GetClassLong(HWND, int);
LONG    WINAPI SetClassLong(HWND, int, LONG);

/* Class field offsets for GetClassLong() and GetClassWord() */
#define GCL_MENUNAME        (-8)
#define GCW_HBRBACKGROUND   (-10)
#define GCW_HCURSOR         (-12)
#define GCW_HICON           (-14)
#define GCW_HMODULE         (-16)
#define GCW_CBWNDEXTRA      (-18)
#define GCW_CBCLSEXTRA      (-20)
#define GCL_WNDPROC         (-24)
#define GCW_STYLE           (-26)

#if (WINVER >= 0x030a)
#define GCW_ATOM            (-32)
#endif  /* WINVER >= 0x030a */
#endif /* NOWINOFFSETS */

/****** Window creation/destroy *********************************************/

/* Window Styles */
#ifndef NOWINSTYLES

/* Basic window types */
#define WS_OVERLAPPED           0x00000000L
#define WS_POPUP                0x80000000L
#define WS_CHILD                0x40000000L

/* Clipping styles */
#define WS_CLIPSIBLINGS         0x04000000L
#define WS_CLIPCHILDREN         0x02000000L

/* Generic window states */
#define WS_VISIBLE              0x10000000L
#define WS_DISABLED             0x08000000L

/* Main window states */
#define WS_MINIMIZE             0x20000000L
#define WS_MAXIMIZE             0x01000000L

/* Main window styles */
#define WS_CAPTION              0x00C00000L     /* WS_BORDER | WS_DLGFRAME */
#define WS_BORDER               0x00800000L
#define WS_DLGFRAME             0x00400000L
#define WS_VSCROLL              0x00200000L
#define WS_HSCROLL              0x00100000L
#define WS_SYSMENU              0x00080000L
#define WS_THICKFRAME           0x00040000L
#define WS_MINIMIZEBOX          0x00020000L
#define WS_MAXIMIZEBOX          0x00010000L

/* Control window styles */
#define WS_GROUP                0x00020000L
#define WS_TABSTOP              0x00010000L

/* Common Window Styles */
#define WS_OVERLAPPEDWINDOW     (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW          (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW          (WS_CHILD)

/* Extended Window Styles */
#define WS_EX_DLGMODALFRAME     0x00000001L
#define WS_EX_NOPARENTNOTIFY    0x00000004L

#if (WINVER >= 0x030a)
#define WS_EX_TOPMOST           0x00000008L
#define WS_EX_ACCEPTFILES       0x00000010L
#define WS_EX_TRANSPARENT       0x00000020L
#endif  /* WINVER >= 0x030a */

/* Obsolete style names */
#define WS_TILED                WS_OVERLAPPED
#define WS_ICONIC               WS_MINIMIZE
#define WS_SIZEBOX              WS_THICKFRAME
#define WS_TILEDWINDOW          WS_OVERLAPPEDWINDOW

#endif  /* NOWINSTYLES */


/* Special value for CreateWindow, et al. */
#define HWND_DESKTOP        ((HWND)0)

BOOL    WINAPI IsWindow(HWND);

HWND    WINAPI CreateWindow(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void FAR*);
HWND    WINAPI CreateWindowEx(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void FAR*);

/* WM_CREATE/WM_NCCREATE lParam struct */
typedef struct tagCREATESTRUCT
{
    void FAR* lpCreateParams;
    HINSTANCE hInstance;
    HMENU     hMenu;
    HWND      hwndParent;
    int       cy;
    int       cx;
    int       y;
    int       x;
    LONG      style;
    LPCSTR    lpszName;
    LPCSTR    lpszClass;
    DWORD     dwExStyle;
} CREATESTRUCT, FAR * LPCREATESTRUCT;

BOOL    WINAPI DestroyWindow(HWND);

/* Basic window attributes */

HTASK   WINAPI GetWindowTask(HWND);

BOOL    WINAPI IsChild(HWND, HWND);

HWND    WINAPI GetParent(HWND);
HWND    WINAPI SetParent(HWND, HWND);

BOOL    WINAPI IsWindowVisible(HWND);

BOOL    WINAPI ShowWindow(HWND, int);

#ifndef NOSHOWWINDOW

#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           SW_SHOWNORMAL
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         SW_SHOWMAXIMIZED
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9

/* Obsolete ShowWindow() command names */
#define HIDE_WINDOW         SW_HIDE
#define SHOW_OPENWINDOW     SW_SHOWNORMAL
#define SHOW_ICONWINDOW     SW_SHOWMINIMIZED
#define SHOW_FULLSCREEN     SW_SHOWMAXIMIZED
#define SHOW_OPENNOACTIVATE SW_SHOWNOACTIVATE

/* WM_SHOWWINDOW wParam codes */
#define SW_PARENTCLOSING    1
#define SW_OTHERMAXIMIZED   2
#define SW_PARENTOPENING    3
#define SW_OTHERRESTORED    4

/* Obsolete constant names */
#define SW_OTHERZOOM        SW_OTHERMAXIMIZED
#define SW_OTHERUNZOOM      SW_OTHERRESTORED
#endif /* NOSHOWWINDOW */

/* Enabled state */
BOOL    WINAPI EnableWindow(HWND,BOOL);
BOOL    WINAPI IsWindowEnabled(HWND);

/* Window text */
void    WINAPI SetWindowText(HWND, LPCSTR);
int     WINAPI GetWindowText(HWND, LPSTR, int);
int     WINAPI GetWindowTextLength(HWND);

/* Window words */
WORD    WINAPI GetWindowWord(HWND, int);
WORD    WINAPI SetWindowWord(HWND, int, WORD);
LONG    WINAPI GetWindowLong(HWND, int);
LONG    WINAPI SetWindowLong(HWND, int, LONG);

/* Window field offsets for GetWindowLong() and GetWindowWord() */
#ifndef NOWINOFFSETS
#define GWL_WNDPROC     (-4)
#define GWW_HINSTANCE   (-6)
#define GWW_HWNDPARENT  (-8)
#define GWW_ID          (-12)
#define GWL_STYLE       (-16)
#define GWL_EXSTYLE     (-20)
#endif  /* NOWINOFFSETS */

/****** Window size, position, Z-order, and visibility **********************/

#define CW_USEDEFAULT     ((int)0x8000)

void    WINAPI GetClientRect(HWND, LPRECT);
void    WINAPI GetWindowRect(HWND, LPRECT);


#if (WINVER >= 0x030a)
typedef struct tagWINDOWPLACEMENT
{
    UINT  length;
    UINT  flags;
    UINT  showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT  rcNormalPosition;
} WINDOWPLACEMENT;
typedef WINDOWPLACEMENT     *PWINDOWPLACEMENT;
typedef WINDOWPLACEMENT FAR* LPWINDOWPLACEMENT;

#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002

BOOL    WINAPI GetWindowPlacement(HWND, WINDOWPLACEMENT FAR*);
BOOL    WINAPI SetWindowPlacement(HWND, const WINDOWPLACEMENT FAR*);
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI SetWindowPos(HWND, HWND, int, int, int, int, UINT);
/* SetWindowPos() and WINDOWPOS flags */
#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
#define SWP_NOREDRAW        0x0008
#define SWP_NOACTIVATE      0x0010
#define SWP_FRAMECHANGED    0x0020  /* The frame changed: send WM_NCCALCSIZE */
#define SWP_SHOWWINDOW      0x0040
#define SWP_HIDEWINDOW      0x0080
#define SWP_NOCOPYBITS      0x0100
#define SWP_NOOWNERZORDER   0x0200  /* Don't do owner Z ordering */

#define SWP_DRAWFRAME       SWP_FRAMECHANGED
#define SWP_NOREPOSITION    SWP_NOOWNERZORDER

#define SWP_NOSENDCHANGING  0x0400
#define SWP_DEFERERASE      0x2000

/* SetWindowPos() hwndInsertAfter field values */
#define HWND_TOP            ((HWND)0)
#define HWND_BOTTOM         ((HWND)1)
#define HWND_TOPMOST        ((HWND)-1)
#define HWND_NOTOPMOST      ((HWND)-2)


#ifndef NODEFERWINDOWPOS

DECLARE_HANDLE(HDWP);

HDWP    WINAPI BeginDeferWindowPos(int);
HDWP    WINAPI DeferWindowPos(HDWP, HWND, HWND, int, int, int, int, UINT);
BOOL    WINAPI EndDeferWindowPos(HDWP);

#endif  /* NODEFERWINDOWPOS */

BOOL    WINAPI MoveWindow(HWND, int, int, int, int, BOOL);
BOOL    WINAPI BringWindowToTop(HWND);

#if (WINVER >= 0x030a)

/* WM_WINDOWPOSCHANGING/CHANGED struct pointed to by lParam */
typedef struct tagWINDOWPOS
{
    HWND    hwnd;
    HWND    hwndInsertAfter;
    int     x;
    int     y;
    int     cx;
    int     cy;
    UINT    flags;
} WINDOWPOS;
typedef WINDOWPOS FAR* LPWINDOWPOS;
#endif  /* WINVER >= 0x030a */

/* WM_SIZE message wParam values */
#define SIZE_RESTORED       0
#define SIZE_MINIMIZED      1
#define SIZE_MAXIMIZED      2
#define SIZE_MAXSHOW        3
#define SIZE_MAXHIDE        4

/* Obsolete constant names */
#define SIZENORMAL          SIZE_RESTORED
#define SIZEICONIC          SIZE_MINIMIZED
#define SIZEFULLSCREEN      SIZE_MAXIMIZED
#define SIZEZOOMSHOW        SIZE_MAXSHOW
#define SIZEZOOMHIDE        SIZE_MAXHIDE

/****** Window proc implementation & subclassing support *********************/

LRESULT WINAPI DefWindowProc(HWND, UINT, WPARAM, LPARAM);

#ifdef STRICT
LRESULT WINAPI CallWindowProc(WNDPROC, HWND, UINT, WPARAM, LPARAM);
#else
LRESULT WINAPI CallWindowProc(FARPROC, HWND, UINT, WPARAM, LPARAM);
#endif

/****** Main window support **************************************************/

void    WINAPI AdjustWindowRect(LPRECT, DWORD, BOOL);
void    WINAPI AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD);

/* Struct pointed to by WM_GETMINMAXINFO lParam */
typedef struct tagMINMAXINFO
{
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
} MINMAXINFO;

BOOL    WINAPI FlashWindow(HWND, BOOL);

void    WINAPI ShowOwnedPopups(HWND, BOOL);

/* Obsolete functions */
BOOL    WINAPI OpenIcon(HWND);
void    WINAPI CloseWindow(HWND);
BOOL    WINAPI AnyPopup(void);
BOOL    WINAPI IsIconic(HWND);
BOOL    WINAPI IsZoomed(HWND);

/****** Window coordinate mapping and hit-testing ***************************/

void    WINAPI ClientToScreen(HWND, POINT FAR*);
void    WINAPI ScreenToClient(HWND, POINT FAR*);

#if (WINVER >= 0x030a)
void    WINAPI MapWindowPoints(HWND hwndFrom, HWND hwndTo, POINT FAR* lppt, UINT cpt);
#endif  /* WINVER >= 0x030a */

HWND    WINAPI WindowFromPoint(POINT);
HWND    WINAPI ChildWindowFromPoint(HWND, POINT);

/****** Window query and enumeration ****************************************/

HWND    WINAPI GetDesktopWindow(void);
HWND    WINAPI FindWindow(LPCSTR, LPCSTR);

#ifdef STRICT
typedef BOOL (CALLBACK* WNDENUMPROC)(HWND, LPARAM);
#else
typedef FARPROC WNDENUMPROC;
#endif

BOOL    WINAPI EnumWindows(WNDENUMPROC, LPARAM);
BOOL    WINAPI EnumChildWindows(HWND, WNDENUMPROC, LPARAM);
BOOL    WINAPI EnumTaskWindows(HTASK, WNDENUMPROC, LPARAM);


HWND    WINAPI GetTopWindow(HWND);
HWND    WINAPI GetWindow(HWND, UINT);
HWND    WINAPI GetNextWindow(HWND, UINT);

/* GetWindow() constants */
#define GW_HWNDFIRST        0
#define GW_HWNDLAST         1
#define GW_HWNDNEXT         2
#define GW_HWNDPREV         3
#define GW_OWNER            4
#define GW_CHILD            5

/****** Window property support *********************************************/

BOOL    WINAPI SetProp(HWND, LPCSTR, HANDLE);
HANDLE  WINAPI GetProp(HWND, LPCSTR);
HANDLE  WINAPI RemoveProp(HWND, LPCSTR);

#ifdef STRICT
typedef BOOL (CALLBACK* PROPENUMPROC)(HWND, LPCSTR, HANDLE);
#else
typedef FARPROC PROPENUMPROC;
#endif

int     WINAPI EnumProps(HWND, PROPENUMPROC);

/****** Window drawing support **********************************************/

HDC     WINAPI GetDC(HWND);
int     WINAPI ReleaseDC(HWND, HDC);

HDC     WINAPI GetWindowDC(HWND);

#if (WINVER >= 0x030a)
HDC     WINAPI GetDCEx(register HWND hwnd, HRGN hrgnClip, DWORD flags);

#define DCX_WINDOW          0x00000001L
#define DCX_CACHE           0x00000002L
#define DCX_CLIPCHILDREN    0x00000008L
#define DCX_CLIPSIBLINGS    0x00000010L
#define DCX_PARENTCLIP      0x00000020L
#define DCX_EXCLUDERGN      0x00000040L
#define DCX_INTERSECTRGN    0x00000080L
#define DCX_LOCKWINDOWUPDATE 0x00000400L
#define DCX_USESTYLE        0x00010000L
#endif  /* WINVER >= 0x030a */

/****** Window repainting ***************************************************/


/* BeginPaint() return structure */
typedef struct tagPAINTSTRUCT
{
    HDC  hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[16];
} PAINTSTRUCT;
typedef PAINTSTRUCT* PPAINTSTRUCT;
typedef PAINTSTRUCT NEAR* NPPAINTSTRUCT;
typedef PAINTSTRUCT FAR* LPPAINTSTRUCT;

HDC     WINAPI BeginPaint(HWND, LPPAINTSTRUCT);
void    WINAPI EndPaint(HWND, const PAINTSTRUCT FAR*);

void    WINAPI UpdateWindow(HWND);

int     WINAPI ExcludeUpdateRgn(HDC, HWND);

#if (WINVER >= 0x030a)
BOOL    WINAPI LockWindowUpdate(HWND hwndLock);
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI GetUpdateRect(HWND, LPRECT, BOOL);
int     WINAPI GetUpdateRgn(HWND, HRGN, BOOL);

void    WINAPI InvalidateRect(HWND, LPCRECT, BOOL);
void    WINAPI ValidateRect(HWND, LPCRECT);

void    WINAPI InvalidateRgn(HWND, HRGN, BOOL);
void    WINAPI ValidateRgn(HWND, HRGN);

#if (WINVER >= 0x030a)
BOOL    WINAPI RedrawWindow(HWND hwnd, LPCRECT lprcUpdate, HRGN hrgnUpdate, UINT flags);

#define RDW_INVALIDATE      0x0001
#define RDW_INTERNALPAINT   0x0002
#define RDW_ERASE           0x0004
#define RDW_VALIDATE        0x0008

#define RDW_NOINTERNALPAINT 0x0010
#define RDW_NOERASE         0x0020
#define RDW_NOCHILDREN      0x0040
#define RDW_ALLCHILDREN     0x0080

#define RDW_UPDATENOW       0x0100
#define RDW_ERASENOW        0x0200
#define RDW_FRAME           0x0400
#define RDW_NOFRAME         0x0800

#endif  /* WINVER >= 0x030a */

/****** Window scrolling ****************************************************/

void    WINAPI ScrollWindow(HWND, int, int, LPCRECT, LPCRECT);
BOOL    WINAPI ScrollDC(HDC, int, int, LPCRECT, LPCRECT, HRGN, LPRECT);

#if (WINVER >= 0x030a)
int WINAPI ScrollWindowEx(HWND, int, int, LPCRECT, LPCRECT, HRGN, LPRECT, UINT);

#define SW_SCROLLCHILDREN       0x0001
#define SW_INVALIDATE           0x0002
#define SW_ERASE                0x0004
#endif  /* WINVER >= 0x030a */

/****** Non-client window area management ************************************/

#if (WINVER >= 0x030a)
/* WM_NCCALCSIZE return flags */
#define WVR_ALIGNTOP        0x0010
#define WVR_ALIGNLEFT       0x0020
#define WVR_ALIGNBOTTOM     0x0040
#define WVR_ALIGNRIGHT      0x0080
#define WVR_HREDRAW         0x0100
#define WVR_VREDRAW         0x0200
#define WVR_REDRAW          (WVR_HREDRAW | WVR_VREDRAW)
#define WVR_VALIDRECTS      0x0400

/* WM_NCCALCSIZE parameter structure */
typedef struct tagNCCALCSIZE_PARAMS
{
    RECT    rgrc[3];
    WINDOWPOS FAR* lppos;
} NCCALCSIZE_PARAMS;
#else   /* WINVER >= 0x030a */
typedef struct tagNCCALCSIZE_PARAMS
{
    RECT    rgrc[2];
} NCCALCSIZE_PARAMS;
#endif  /* WINVER >= 0x030a */
typedef NCCALCSIZE_PARAMS FAR* LPNCCALCSIZE_PARAMS;


/* WM_NCHITTEST return codes */
#define HTERROR             (-2)
#define HTTRANSPARENT       (-1)
#define HTNOWHERE           0
#define HTCLIENT            1
#define HTCAPTION           2
#define HTSYSMENU           3
#define HTSIZE              4
#define HTMENU              5
#define HTHSCROLL           6
#define HTVSCROLL           7
#define HTMINBUTTON         8
#define HTMAXBUTTON         9
#define HTLEFT              10
#define HTRIGHT             11
#define HTTOP               12
#define HTTOPLEFT           13
#define HTTOPRIGHT          14
#define HTBOTTOM            15
#define HTBOTTOMLEFT        16
#define HTBOTTOMRIGHT       17
#define HTBORDER            18
#define HTGROWBOX           HTSIZE
#define HTREDUCE            HTMINBUTTON
#define HTZOOM              HTMAXBUTTON

/****** Window activation ***************************************************/

HWND    WINAPI SetActiveWindow(HWND);
HWND    WINAPI GetActiveWindow(void);
HWND    WINAPI GetLastActivePopup(HWND);

/* WM_ACTIVATE state values */
#define WA_INACTIVE         0
#define WA_ACTIVE           1
#define WA_CLICKACTIVE      2

/****** Keyboard input support **********************************************/

HWND    WINAPI SetFocus(HWND);
HWND    WINAPI GetFocus(void);

int     WINAPI GetKeyState(int);
int     WINAPI GetAsyncKeyState(int);

void    WINAPI GetKeyboardState(BYTE FAR* );
void    WINAPI SetKeyboardState(BYTE FAR* );

/* WM_KEYUP/DOWN/CHAR HIWORD(lParam) flags */
#define KF_EXTENDED     0x0100
#define KF_DLGMODE      0x0800
#define KF_MENUMODE     0x1000
#define KF_ALTDOWN      0x2000
#define KF_REPEAT       0x4000
#define KF_UP           0x8000

/* Virtual key codes */
#ifndef NOVIRTUALKEYCODES
#define VK_LBUTTON      0x01
#define VK_RBUTTON      0x02
#define VK_CANCEL       0x03
#define VK_MBUTTON      0x04
#define VK_BACK         0x08
#define VK_TAB          0x09
#define VK_CLEAR        0x0C
#define VK_RETURN       0x0D
#define VK_SHIFT        0x10
#define VK_CONTROL      0x11
#define VK_MENU         0x12
#define VK_PAUSE        0x13
#define VK_CAPITAL      0x14
#define VK_ESCAPE       0x1B
#define VK_SPACE        0x20
#define VK_PRIOR        0x21
#define VK_NEXT         0x22
#define VK_END          0x23
#define VK_HOME         0x24
#define VK_LEFT         0x25
#define VK_UP           0x26
#define VK_RIGHT        0x27
#define VK_DOWN         0x28
#define VK_SELECT       0x29
#define VK_PRINT        0x2A
#define VK_EXECUTE      0x2B
#define VK_SNAPSHOT     0x2C
#define VK_INSERT       0x2D
#define VK_DELETE       0x2E
#define VK_HELP         0x2F
#define VK_NUMPAD0      0x60
#define VK_NUMPAD1      0x61
#define VK_NUMPAD2      0x62
#define VK_NUMPAD3      0x63
#define VK_NUMPAD4      0x64
#define VK_NUMPAD5      0x65
#define VK_NUMPAD6      0x66
#define VK_NUMPAD7      0x67
#define VK_NUMPAD8      0x68
#define VK_NUMPAD9      0x69
#define VK_MULTIPLY     0x6A
#define VK_ADD          0x6B
#define VK_SEPARATOR    0x6C
#define VK_SUBTRACT     0x6D
#define VK_DECIMAL      0x6E
#define VK_DIVIDE       0x6F
#define VK_F1           0x70
#define VK_F2           0x71
#define VK_F3           0x72
#define VK_F4           0x73
#define VK_F5           0x74
#define VK_F6           0x75
#define VK_F7           0x76
#define VK_F8           0x77
#define VK_F9           0x78
#define VK_F10          0x79
#define VK_F11          0x7A
#define VK_F12          0x7B
#define VK_F13          0x7C
#define VK_F14          0x7D
#define VK_F15          0x7E
#define VK_F16          0x7F
#define VK_F17          0x80
#define VK_F18          0x81
#define VK_F19          0x82
#define VK_F20          0x83
#define VK_F21          0x84
#define VK_F22          0x85
#define VK_F23          0x86
#define VK_F24          0x87
#define VK_NUMLOCK      0x90
#define VK_SCROLL       0x91

/* VK_A thru VK_Z are the same as their ASCII equivalents: 'A' thru 'Z' */
/* VK_0 thru VK_9 are the same as their ASCII equivalents: '0' thru '9' */

#endif  /* NOVIRTUALKEYCODES */


/* SetWindowsHook() keyboard hook */
#define WH_KEYBOARD     2

/****** Mouse input support *************************************************/

HWND    WINAPI SetCapture(HWND);
HWND    WINAPI GetCapture(void);
void    WINAPI ReleaseCapture(void);

BOOL    WINAPI SwapMouseButton(BOOL);

/* Mouse message wParam key states */
#ifndef NOKEYSTATES
#define MK_LBUTTON      0x0001
#define MK_RBUTTON      0x0002
#define MK_SHIFT        0x0004
#define MK_CONTROL      0x0008
#define MK_MBUTTON      0x0010
#endif  /* NOKEYSTATES */

/* WM_MOUSEACTIVATE return codes */
#define MA_ACTIVATE         1
#define MA_ACTIVATEANDEAT   2
#define MA_NOACTIVATE       3
#if (WINVER >= 0x030a)
#define MA_NOACTIVATEANDEAT 4
#endif  /* WINVER >= 0x030a */

/* SetWindowsHook() mouse hook */
#ifndef NOWH
#define WH_MOUSE            7

typedef struct tagMOUSEHOOKSTRUCT
{
    POINT   pt;
    HWND    hwnd;
    UINT    wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT;
typedef MOUSEHOOKSTRUCT  FAR* LPMOUSEHOOKSTRUCT;
#endif /* NOWH */

/****** Mode control ********************************************************/

HWND    WINAPI GetSysModalWindow(void);
HWND    WINAPI SetSysModalWindow(HWND);


/****** Timer support *******************************************************/

#ifdef STRICT
typedef void (CALLBACK* TIMERPROC)(HWND, UINT, UINT, DWORD);
#else
typedef FARPROC TIMERPROC;
#endif

UINT    WINAPI SetTimer(HWND, UINT, UINT, TIMERPROC);

BOOL    WINAPI KillTimer(HWND, UINT);

/****** Accelerator support *************************************************/

DECLARE_HANDLE(HACCEL);

HACCEL  WINAPI LoadAccelerators(HINSTANCE, LPCSTR);


#ifndef NOMSG
int     WINAPI TranslateAccelerator(HWND, HACCEL, LPMSG);
#endif

/****** Menu support ********************************************************/

#ifndef NOMENUS

/* Menu template header */
typedef struct tagMENUITEMTEMPLATEHEADER
{
    UINT    versionNumber;
    UINT    offset;
} MENUITEMTEMPLATEHEADER;

/* Menu template item struct */
typedef struct tagMENUITEMTEMPLATE
{
    UINT    mtOption;
    UINT    mtID;
    char    mtString[1];
} MENUITEMTEMPLATE;

BOOL    WINAPI IsMenu(HMENU);

HMENU   WINAPI CreateMenu(void);
HMENU   WINAPI CreatePopupMenu(void);
HMENU   WINAPI LoadMenu(HINSTANCE, LPCSTR);
HMENU   WINAPI LoadMenuIndirect(const void FAR*);
HMENU   WINAPI GetMenu(HWND);
HMENU   WINAPI GetSystemMenu(HWND, BOOL);
HMENU   WINAPI GetSubMenu(HMENU, UINT);
BOOL    WINAPI InsertMenu(HMENU, UINT, UINT, UINT, LPCSTR);
BOOL    WINAPI AppendMenu(HMENU, UINT, UINT, LPCSTR);
BOOL    WINAPI ModifyMenu(HMENU, UINT, UINT, UINT, LPCSTR);
BOOL    WINAPI ChangeMenu(HMENU, UINT, LPCSTR, UINT, UINT);
BOOL    WINAPI SetMenuItemBitmaps(HMENU, UINT, UINT, HBITMAP, HBITMAP);
int     WINAPI GetMenuString(HMENU, UINT, LPSTR, int, UINT);

BOOL    WINAPI DestroyMenu(HMENU);
BOOL    WINAPI SetMenu(HWND, HMENU);
void    WINAPI DrawMenuBar(HWND);
BOOL    WINAPI HiliteMenuItem(HWND, HMENU, UINT, UINT);
BOOL    WINAPI RemoveMenu(HMENU, UINT, UINT);
BOOL    WINAPI DeleteMenu(HMENU, UINT, UINT);

#define MF_INSERT           0x0000  /* Obsolete -- only used by ChangeMenu */
#define MF_CHANGE           0x0080  /* Obsolete -- only used by ChangeMenu */
#define MF_APPEND           0x0100  /* Obsolete -- only used by ChangeMenu */
#define MF_DELETE           0x0200  /* Obsolete -- only used by ChangeMenu */
#define MF_REMOVE           0x1000  /* Obsolete -- only used by ChangeMenu */

#define MF_BYCOMMAND        0x0000  /* Obsolete -- only old APIs use these */
#define MF_BYPOSITION       0x0400  /* Obsolete -- only old APIs use these */
#define MF_USECHECKBITMAPS  0x0200  /* Obsolete -- replaced by MF_RADIOCHECK */
#define MF_HELP             0x4000  /* Obsolete -- use MF_RIGHTJUSTIFY */

#define MF_POPUP            0x0010  /* Obsolete -- replace by hSubMenu field */
#define MF_END              0x0080  /* Obsolete -- only used by old RES files */
#define MF_SYSMENU          0x2000  /* Passed back during WM_MENUSELECT */
#define MF_MOUSESELECT      0x8000  /* Passed back during WM_MENUSELECT */

#define MFT_STRING          0x0000
#define MFT_BITMAP          0x0004
#define MFT_MENUBARBREAK    0x0020
#define MFT_MENUBREAK       0x0040
#define MFT_OWNERDRAW       0x0100
#define MFT_RADIOCHECK      0x0200
#define MFT_SEPARATOR       0x0800
#define MFT_RIGHTJUSTIFY    0x4000
#define MFT_MASK            0x4B64

#define MFS_GRAYED          0x0003
#define MFS_DISABLED        0x0002
#define MFS_CHECKED         0x0008
#define MFS_HILITE          0x0080
#define MFS_ENABLED         0x0000
#define MFS_UNCHECKED       0x0000
#define MFS_UNHILITE        0x0000
#define MFS_DEFAULT         0x1000
#define MFS_MASK            0x108B

#define MFR_POPUP           0x01
#define MFR_END             0x80

/* old names for menu types & states */
#define MF_SEPARATOR        MFT_SEPARATOR

#define MF_GRAYED           0x0001
#define MF_DISABLED         0x0002

#define MF_CHECKED          MFS_CHECKED
#define MF_ENABLED          MFS_ENABLED
#define MF_UNCHECKED        MFS_UNCHECKED
#define MF_UNHILITE         MFS_UNHILITE

#define MF_STRING           MFT_STRING
#define MF_BITMAP           MFT_BITMAP
#define MF_OWNERDRAW        MFT_OWNERDRAW

#define MF_MENUBARBREAK     MFT_MENUBARBREAK
#define MF_MENUBREAK        MFT_MENUBREAK

#define MF_HILITE           MFS_HILITE

#define MF_DEFAULT          MFS_DEFAULT
#define MF_RIGHTJUSTIFY     MFT_RIGHTJUSTIFY


UINT    WINAPI EnableMenuItem(HMENU, UINT, UINT);
UINT    WINAPI CheckMenuItem(HMENU, UINT, UINT);

int     WINAPI GetMenuItemCount(HMENU);
UINT    WINAPI GetMenuItemID(HMENU, UINT);
UINT    WINAPI GetMenuState(HMENU, UINT, UINT);

DWORD   WINAPI GetMenuCheckMarkDimensions(void);

BOOL    WINAPI TrackPopupMenu(HMENU, UINT, int, int, int, HWND, LPCRECT);

/* Flags for TrackPopupMenu */
#define TPM_LEFTBUTTON      0x0000

#if (WINVER >= 0x030a)
#define TPM_RIGHTBUTTON     0x0002
#define TPM_LEFTALIGN       0x0000
#define TPM_CENTERALIGN     0x0004
#define TPM_RIGHTALIGN      0x0008
#endif  /* WINVER >= 0x030a */

#endif  /* NOMENUS */

/****** Scroll bar support **************************************************/

#ifndef NOSCROLL

/* WM_H/VSCROLL commands */
#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8

/* Scroll bar selection constants */
#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3

int     WINAPI SetScrollPos(HWND, int, int, BOOL);
int     WINAPI GetScrollPos(HWND, int);
void    WINAPI SetScrollRange(HWND, int, int, int, BOOL);
void    WINAPI GetScrollRange(HWND, int, int FAR*, int FAR*);
void    WINAPI ShowScrollBar(HWND, int, BOOL);
BOOL    WINAPI EnableScrollBar(HWND, int, UINT);

/* EnableScrollBar() flags */
#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003

#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002

#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002

#define ESB_DISABLE_LTUP    ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN    ESB_DISABLE_RIGHT

#endif  /* NOSCROLL */

/******* Clipboard manager **************************************************/

#ifndef NOCLIPBOARD

/* Predefined Clipboard Formats */
#define CF_TEXT             1
#define CF_BITMAP           2
#define CF_METAFILEPICT     3
#define CF_SYLK             4
#define CF_DIF              5
#define CF_TIFF             6
#define CF_OEMTEXT          7
#define CF_DIB              8
#define CF_PALETTE          9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12

#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083

/* "Private" formats don't get GlobalFree()'d */
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF

/* "GDIOBJ" formats do get DeleteObject()'d */
#define CF_GDIOBJFIRST      0x0300
#define CF_GDIOBJLAST       0x03FF

/* Clipboard Manager Functions */
BOOL    WINAPI OpenClipboard(HWND);
BOOL    WINAPI CloseClipboard(void);
BOOL    WINAPI EmptyClipboard(void);

#if (WINVER >= 0x030a)
HWND    WINAPI GetOpenClipboardWindow(void);
#endif  /* WINVER >= 0x030a */

HWND    WINAPI GetClipboardOwner(void);

HWND    WINAPI SetClipboardViewer(HWND);
HWND    WINAPI GetClipboardViewer(void);

HANDLE  WINAPI SetClipboardData(UINT, HANDLE);
HANDLE  WINAPI GetClipboardData(UINT);

BOOL    WINAPI IsClipboardFormatAvailable(UINT);
int     WINAPI GetPriorityClipboardFormat(UINT FAR*, int);

UINT    WINAPI RegisterClipboardFormat(LPCSTR);
int     WINAPI CountClipboardFormats(void);
UINT    WINAPI EnumClipboardFormats(UINT);
int     WINAPI GetClipboardFormatName(UINT, LPSTR, int);

BOOL    WINAPI ChangeClipboardChain(HWND, HWND);

#endif  /* NOCLIPBOARD */

/****** Mouse cursor support *************************************************/

HCURSOR WINAPI LoadCursor(HINSTANCE, LPCSTR);
HCURSOR WINAPI CreateCursor(HINSTANCE, int, int, int, int, const void FAR*, const void FAR*);
BOOL    WINAPI DestroyCursor(HCURSOR);

#if (WINVER >= 0x030a)
HCURSOR WINAPI CopyCursor(HINSTANCE, HCURSOR);
#endif /* WINVER >= 0x030a */

int     WINAPI ShowCursor(BOOL);

void    WINAPI SetCursorPos(int, int);
void    WINAPI GetCursorPos(POINT FAR*);

HCURSOR WINAPI SetCursor(HCURSOR);

#if (WINVER >= 0x030a)
HCURSOR WINAPI GetCursor(void);
#endif  /* WINVER >= 0x030a */

void    WINAPI ClipCursor(LPCRECT);
#if (WINVER >= 0x030a)
void    WINAPI GetClipCursor(LPRECT);
#endif  /* WINVER >= 0x030a */

/* Standard cursor resource IDs */
#define IDC_ARROW       MAKEINTRESOURCE(32512)
#define IDC_IBEAM       MAKEINTRESOURCE(32513)
#define IDC_WAIT        MAKEINTRESOURCE(32514)
#define IDC_CROSS       MAKEINTRESOURCE(32515)
#define IDC_UPARROW     MAKEINTRESOURCE(32516)
#define IDC_SIZE        MAKEINTRESOURCE(32640)
#define IDC_ICON        MAKEINTRESOURCE(32641)
#define IDC_SIZENWSE    MAKEINTRESOURCE(32642)
#define IDC_SIZENESW    MAKEINTRESOURCE(32643)
#define IDC_SIZEWE      MAKEINTRESOURCE(32644)
#define IDC_SIZENS      MAKEINTRESOURCE(32645)
#define IDC_NO          MAKEINTRESOURCE(32648)


/****** Icon support *********************************************************/


HICON   WINAPI LoadIcon(HINSTANCE, LPCSTR);
HICON   WINAPI CreateIcon(HINSTANCE, int, int, BYTE, BYTE, const void FAR*, const void FAR*);
BOOL    WINAPI DestroyIcon(HICON);

#if (WINVER >= 0x030a)
HICON   WINAPI CopyIcon(HINSTANCE, HICON);

typedef struct tagNEWHEADER
{
    WORD    Reserved;
    WORD    ResType;        /* RES_ICON or RES_CURSOR */
    WORD    ResCount;
} NEWHEADER, FAR *LPNEWHEADER;

#if (WINVER >= 0x0400)
/* Icon/Cursor header */
typedef struct tagCURSORSHAPE
{
    int     xHotSpot;
    int     yHotSpot;
    int     cx;
    int     cy;
    int     cbWidth;
    BYTE    Planes;
    BYTE    BitsPixel;
} CURSORSHAPE, FAR *LPCURSORSHAPE;
                                  
#endif /* WINVER >= 0x0400 */
#endif  /* WINVER >= 0x030a */

BOOL    WINAPI DrawIcon(HDC, int, int, HICON);

#ifndef NOICONS

/* Standard icon resource IDs */
#ifdef RC_INVOKED
#define IDI_APPLICATION     32512
#define IDI_HAND            32513
#define IDI_QUESTION        32514
#define IDI_EXCLAMATION     32515
#define IDI_ASTERISK        32516
#else
#define IDI_APPLICATION     MAKEINTRESOURCE(32512)
#define IDI_HAND            MAKEINTRESOURCE(32513)
#define IDI_QUESTION        MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION     MAKEINTRESOURCE(32515)
#define IDI_ASTERISK        MAKEINTRESOURCE(32516)
#endif /* RC_INVOKED */

#endif  /* NOICONS */


/****** Caret support ********************************************************/

void    WINAPI CreateCaret(HWND, HBITMAP, int, int);
void    WINAPI DestroyCaret(void);
void    WINAPI ShowCaret(HWND);
void    WINAPI HideCaret(HWND);
void    WINAPI SetCaretPos(int, int);
void    WINAPI SetCaretBlinkTime(UINT);
void    WINAPI GetCaretPos(LPPOINT);
UINT    WINAPI GetCaretBlinkTime(void);

/****** WM_SYSCOMMAND support ***********************************************/

#ifndef NOSYSCOMMANDS

/* System Menu Command Values */
#define SC_SIZE         0xF000
#define SC_MOVE         0xF010
#define SC_MINIMIZE     0xF020
#define SC_MAXIMIZE     0xF030
#define SC_NEXTWINDOW   0xF040
#define SC_PREVWINDOW   0xF050
#define SC_CLOSE        0xF060
#define SC_VSCROLL      0xF070
#define SC_HSCROLL      0xF080
#define SC_MOUSEMENU    0xF090
#define SC_KEYMENU      0xF100
#define SC_ARRANGE      0xF110
#define SC_RESTORE      0xF120
#define SC_TASKLIST     0xF130
#define SC_SCREENSAVE   0xF140
#define SC_HOTKEY       0xF150

/* Obsolete names */
#define SC_ICON         SC_MINIMIZE
#define SC_ZOOM         SC_MAXIMIZE

#endif  /* NOSYSCOMMANDS */

/****** MDI Support *********************************************************/

#ifndef NOMDI

/* CreateWindow lpParams structure for creating MDI client */
typedef struct tagCLIENTCREATESTRUCT
{
    HMENU  hWindowMenu;
    UINT   idFirstChild;
} CLIENTCREATESTRUCT;

typedef CLIENTCREATESTRUCT FAR* LPCLIENTCREATESTRUCT;

/* MDI client style bits */
#if (WINVER >= 0x030a)
#define MDIS_ALLCHILDSTYLES 0x0001
#endif  /* WINVER >= 0x030a */

/* WM_MDICREATE message structure */
typedef struct tagMDICREATESTRUCT
{
    LPCSTR  szClass;
    LPCSTR  szTitle;
    HINSTANCE hOwner;
    int     x;
    int     y;
    int     cx;
    int     cy;
    DWORD   style;
    LPARAM  lParam;
} MDICREATESTRUCT;
typedef MDICREATESTRUCT FAR*  LPMDICREATESTRUCT;



#if (WINVER >= 0x030a)
/* wParam values for WM_MDITILE and WM_MDICASCADE messages. */
#define MDITILE_VERTICAL        0x0000
#define MDITILE_HORIZONTAL      0x0001
#endif  /* WINVER >= 0x030a */

LRESULT WINAPI DefFrameProc(HWND, HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI DefMDIChildProc(HWND, UINT, WPARAM, LPARAM);

#ifndef NOMSG
BOOL    WINAPI TranslateMDISysAccel(HWND, LPMSG);
#endif

UINT    WINAPI ArrangeIconicWindows(HWND);
BOOL    WINAPI TileChildWindows(HWND, UINT);
BOOL    WINAPI CascadeChildWindows(HWND, UINT);
#endif  /* NOMDI */

/****** Dialog and Control Management ***************************************/

#ifndef NOCTLMGR

/* Dialog window class */
#define WC_DIALOG (MAKEINTATOM(0x8002))

/* cbWndExtra bytes needed by dialog manager for dialog classes */
#define DLGWINDOWEXTRA 30

/* Dialog styles */
#define DS_ABSALIGN         0x0001L
#define DS_SYSMODAL         0x0002L
#define DS_LOCALEDIT        0x0020L
#define DS_MODALFRAME       0x0080L
#define DS_NOIDLEMSG        0x0100L

/* Dialog messages */
#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)

/* Returned in HIWORD() of DM_GETDEFID result if msg is supported */
#define DC_HASDEFID         0x534B

#ifdef STRICT
typedef BOOL (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
#else
typedef FARPROC DLGPROC;
#endif

/* Get/SetWindowWord/Long offsets for use with WC_DIALOG windows */
#define DWL_MSGRESULT       0
#define DWL_DLGPROC         4
#define DWL_USER            8

#ifndef NOMSG
BOOL    WINAPI IsDialogMessage(HWND, LPMSG);
#endif

LRESULT WINAPI DefDlgProc(HWND, UINT, WPARAM, LPARAM);

HWND    WINAPI CreateDialog(HINSTANCE, LPCSTR, HWND, DLGPROC);
HWND    WINAPI CreateDialogIndirect(HINSTANCE, const void FAR*, HWND, DLGPROC);
HWND    WINAPI CreateDialogParam(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
HWND    WINAPI CreateDialogIndirectParam(HINSTANCE, const void FAR*, HWND, DLGPROC, LPARAM);

int     WINAPI DialogBox(HINSTANCE, LPCSTR, HWND, DLGPROC);
int     WINAPI DialogBoxIndirect(HINSTANCE, HGLOBAL, HWND, DLGPROC);
int     WINAPI DialogBoxParam(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);
int     WINAPI DialogBoxIndirectParam(HINSTANCE, HGLOBAL, HWND, DLGPROC, LPARAM);

void    WINAPI EndDialog(HWND, int);

int     WINAPI GetDlgCtrlID(HWND);
HWND    WINAPI GetDlgItem(HWND, int);
LRESULT WINAPI SendDlgItemMessage(HWND, int, UINT, WPARAM, LPARAM);

void    WINAPI SetDlgItemInt(HWND, int, UINT, BOOL);
UINT    WINAPI GetDlgItemInt(HWND, int, BOOL FAR *, BOOL);

void    WINAPI SetDlgItemText(HWND, int, LPCSTR);
int     WINAPI GetDlgItemText(HWND, int, LPSTR, int);

void    WINAPI CheckDlgButton(HWND, int, UINT);
void    WINAPI CheckRadioButton(HWND, int, int, int);
UINT    WINAPI IsDlgButtonChecked(HWND, int);

HWND    WINAPI GetNextDlgGroupItem(HWND, HWND, BOOL);
HWND    WINAPI GetNextDlgTabItem(HWND, HWND, BOOL);

void    WINAPI MapDialogRect(HWND, LPRECT);
DWORD   WINAPI GetDialogBaseUnits(void);

/* dialog codes */
#define DLGC_WANTARROWS         0x0001
#define DLGC_WANTTAB            0x0002
#define DLGC_WANTALLKEYS        0x0004
#define DLGC_WANTMESSAGE        0x0004
#define DLGC_HASSETSEL          0x0008
#define DLGC_DEFPUSHBUTTON      0x0010
#define DLGC_UNDEFPUSHBUTTON    0x0020
#define DLGC_RADIOBUTTON        0x0040
#define DLGC_WANTCHARS          0x0080
#define DLGC_STATIC             0x0100
#define DLGC_BUTTON             0x2000

/* WM_CTLCOLOR control IDs */
#define CTLCOLOR_MSGBOX         0
#define CTLCOLOR_EDIT           1
#define CTLCOLOR_LISTBOX        2
#define CTLCOLOR_BTN            3
#define CTLCOLOR_DLG            4
#define CTLCOLOR_SCROLLBAR      5
#define CTLCOLOR_STATIC         6

#endif  /* NOCTLMGR */

/* Standard dialog button IDs */
#define IDOK            1
#define IDCANCEL        2
#define IDABORT         3
#define IDRETRY         4
#define IDIGNORE        5
#define IDYES           6
#define IDNO            7


typedef struct tagNMHDR
{
#ifdef tagWND
    HWND_16 hwndFrom;
#else
    HWND  hwndFrom;
#endif
    WORD  _wDummy1;						
    UINT  idFrom;
    WORD  _wDummy2;
    UINT  code;
    WORD  _wDummy3;
}   NMHDR;
typedef NMHDR FAR * LPNMHDR;

/****** Owner draw control support ******************************************/

/* Owner draw control types */
#define ODT_MENU            1
#define ODT_LISTBOX         2
#define ODT_COMBOBOX        3
#define ODT_BUTTON          4

/* Owner draw actions */
#define ODA_DRAWENTIRE      0x0001
#define ODA_SELECT          0x0002
#define ODA_FOCUS           0x0004

/* Owner draw state */
#define ODS_SELECTED        0x0001
#define ODS_GRAYED          0x0002
#define ODS_DISABLED        0x0004
#define ODS_CHECKED         0x0008
#define ODS_FOCUS           0x0010

/* LPARAM of WM_DRAWITEM is LPDRAWITEMSTRUCT */
typedef struct tagDRAWITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemAction;
    UINT        itemState;
    HWND        hwndItem;
    HDC         hDC;
    RECT        rcItem;
    DWORD       itemData;
} DRAWITEMSTRUCT;
typedef DRAWITEMSTRUCT NEAR* PDRAWITEMSTRUCT;
typedef DRAWITEMSTRUCT FAR* LPDRAWITEMSTRUCT;

/* LPARAM of WM_MEASUREITEM is LPMEASUREITEMSTRUCT */
typedef struct tagMEASUREITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemWidth;
    UINT        itemHeight;
    DWORD       itemData;
} MEASUREITEMSTRUCT;
typedef MEASUREITEMSTRUCT NEAR* PMEASUREITEMSTRUCT;
typedef MEASUREITEMSTRUCT FAR* LPMEASUREITEMSTRUCT;

/* LPARAM of WM_DELETEITEM is LPDELETEITEMSTRUCT */
typedef struct tagDELETEITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    HWND        hwndItem;
    DWORD       itemData;
} DELETEITEMSTRUCT;
typedef DELETEITEMSTRUCT NEAR* PDELETEITEMSTRUCT;
typedef DELETEITEMSTRUCT FAR* LPDELETEITEMSTRUCT;

/* LPARAM of WM_COMPAREITEM is LPCOMPAREITEMSTRUCT */
typedef struct tagCOMPAREITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    HWND hwndItem;
    UINT        itemID1;
    DWORD       itemData1;
    UINT        itemID2;
    DWORD       itemData2;
} COMPAREITEMSTRUCT;
typedef COMPAREITEMSTRUCT NEAR* PCOMPAREITEMSTRUCT;
typedef COMPAREITEMSTRUCT FAR* LPCOMPAREITEMSTRUCT;

/****** Static control ******************************************************/

#ifndef NOCTLMGR

/* Static Control Styles */
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL

#define SS_NOPREFIX         0x00000080L

#if (WINVER >= 0x030a)
#ifndef NOWINMESSAGES
/* Static Control Mesages */
#define STM_SETICON         (WM_USER+0)
#define STM_GETICON         (WM_USER+1)
#endif  /* NOWINMESSAGES */
#endif  /* WINVER >= 0x030a */

#endif  /* NOCTLMGR */

/****** Button control *****************************************************/

#ifndef NOCTLMGR

/* Button Control Styles */
#define BS_PUSHBUTTON       0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX         0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_RADIOBUTTON      0x00000004L
#define BS_3STATE           0x00000005L
#define BS_AUTO3STATE       0x00000006L
#define BS_GROUPBOX         0x00000007L
#define BS_USERBUTTON       0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_OWNERDRAW        0x0000000BL

#define BS_RIGHTBUTTON      0x00000020L

#define BS_LEFTTEXT         BS_RIGHTBUTTON

#ifndef NOWINMESSAGES
/* Button Control Messages  */
#define BM_GETCHECK         (WM_USER+0)
#define BM_SETCHECK         (WM_USER+1)
#define BM_GETSTATE         (WM_USER+2)
#define BM_SETSTATE         (WM_USER+3)
#define BM_SETSTYLE         (WM_USER+4)
#define BM_CLICK            (WM_USER+5)

/* User Button Notification Codes */
#define BN_CLICKED          0
#define BN_PAINT            1
#define BN_HILITE           2
#define BN_UNHILITE         3
#define BN_DISABLE          4
#define BN_DBLCLK           5
#define BN_DOUBLECLICKED    BN_DBLCLK
#endif /* NOWINMESSAGES */

#endif  /* NOCTLMGR */

/****** Edit control *******************************************************/

#ifndef NOCTLMGR

/* Edit control styles */
#ifndef NOWINSTYLES
#define ES_LEFT                 0x0000L
#define ES_CENTER               0x0001L
#define ES_RIGHT                0x0002L
#define ES_MULTILINE            0x0004L
#define ES_UPPERCASE            0x0008L
#define ES_LOWERCASE            0x0010L
#define ES_PASSWORD             0x0020L
#define ES_AUTOVSCROLL          0x0040L
#define ES_AUTOHSCROLL          0x0080L
#define ES_NOHIDESEL            0x0100L
#define ES_OEMCONVERT           0x0400L
#if (WINVER >= 0x030a)
#define ES_READONLY             0x0800L
#define ES_WANTRETURN           0x1000L
#endif  /* WINVER >= 0x030a */
#endif  /* NOWINSTYLES */

/* Edit control messages */
#ifndef NOWINMESSAGES
#define EM_GETSEL               (WM_USER+0)
#define EM_SETSEL               (WM_USER+1)
#define EM_GETRECT              (WM_USER+2)
#define EM_SETRECT              (WM_USER+3)
#define EM_SETRECTNP            (WM_USER+4)
#define EM_LINESCROLL           (WM_USER+6)
#define EM_SCROLLCARET          (WM_USER+7)
#define EM_GETMODIFY            (WM_USER+8)
#define EM_SETMODIFY            (WM_USER+9)
#define EM_GETLINECOUNT         (WM_USER+10)
#define EM_LINEINDEX            (WM_USER+11)
#define EM_SETHANDLE            (WM_USER+12)
#define EM_GETHANDLE            (WM_USER+13)
#define EM_LINELENGTH           (WM_USER+17)
#define EM_REPLACESEL           (WM_USER+18)
#define EM_SETFONT              (WM_USER+19)    /* NOT IMPLEMENTED: use WM_SETFONT */
#define EM_GETLINE              (WM_USER+20)
#define EM_SETLIMITTEXT         (WM_USER+21)
#define EM_CANUNDO              (WM_USER+22)
#define EM_UNDO                 (WM_USER+23)
#define EM_FMTLINES             (WM_USER+24)
#define EM_LINEFROMCHAR         (WM_USER+25)
#define EM_SETWORDBREAK         (WM_USER+26)    /* NOT IMPLEMENTED: use EM_SETWORDBREAK */
#define EM_SETTABSTOPS          (WM_USER+27)
#define EM_SETPASSWORDCHAR      (WM_USER+28)
#define EM_EMPTYUNDOBUFFER      (WM_USER+29)

#if (WINVER >= 0x030a)
#define EM_GETFIRSTVISIBLELINE  (WM_USER+30)
#define EM_SETREADONLY          (WM_USER+31)
#define EM_SETWORDBREAKPROC     (WM_USER+32)
#define EM_GETWORDBREAKPROC     (WM_USER+33)
#define EM_GETPASSWORDCHAR      (WM_USER+34)
#endif  /* WINVER >= 0x030a */
#define EM_LIMITTEXT            EM_SETLIMITTEXT
#endif  /* NOWINMESSAGES */

#if (WINVER >= 0x030a)
typedef int   (CALLBACK* EDITWORDBREAKPROC)(LPSTR lpch, int ichCurrent, int cch, int code);

/* EDITWORDBREAKPROC code values */
#define WB_LEFT                 0
#define WB_RIGHT                1
#define WB_ISDELIMITER          2
#endif  /* WINVER >= 0x030a */

/* Edit control notification codes */
#define EN_SETFOCUS             0x0100
#define EN_KILLFOCUS            0x0200
#define EN_CHANGE               0x0300
#define EN_UPDATE               0x0400
#define EN_ERRSPACE             0x0500
#define EN_MAXTEXT              0x0501
#define EN_HSCROLL              0x0601
#define EN_VSCROLL              0x0602
#endif  /* NOCTLMGR */

/****** Scroll bar control *************************************************/
/* Also see scrolling support */

#ifndef NOCTLMGR

#ifndef NOWINSTYLES

/* Scroll bar styles */
#define SBS_HORZ                0x0000L
#define SBS_VERT                0x0001L
#define SBS_TOPALIGN            0x0002L
#define SBS_LEFTALIGN           0x0002L
#define SBS_BOTTOMALIGN         0x0004L
#define SBS_RIGHTALIGN          0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN 0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX             0x0008L
#define SBS_FLAT                0x0010L

#endif  /* NOWINSTYLES */

#endif  /* NOCTLMGR */

/****** Listbox control ****************************************************/

#ifndef NOCTLMGR

/* Listbox styles */
#ifndef NOWINSTYLES
#define LBS_NOTIFY              0x0001L
#define LBS_SORT                0x0002L
#define LBS_NOREDRAW            0x0004L
#define LBS_MULTIPLESEL         0x0008L
#define LBS_OWNERDRAWFIXED      0x0010L
#define LBS_OWNERDRAWVARIABLE   0x0020L
#define LBS_HASSTRINGS          0x0040L
#define LBS_USETABSTOPS         0x0080L
#define LBS_NOINTEGRALHEIGHT    0x0100L
#define LBS_MULTICOLUMN         0x0200L
#define LBS_WANTKEYBOARDINPUT   0x0400L
#define LBS_EXTENDEDSEL         0x0800L
#if (WINVER >= 0x030a)
#define LBS_DISABLENOSCROLL     0x1000L
#endif  /* WINVER >= 0x030a */
#define LBS_STANDARD            (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)
#endif  /* NOWINSTYLES */

/* Listbox messages */
#ifndef NOWINMESSAGES
#define LB_ADDSTRING            (WM_USER+1)
#define LB_INSERTSTRING         (WM_USER+2)
#define LB_DELETESTRING         (WM_USER+3)
#define LB_RESETCONTENT         (WM_USER+5)
#define LB_SETSEL               (WM_USER+6)
#define LB_SETCURSEL            (WM_USER+7)
#define LB_GETSEL               (WM_USER+8)
#define LB_GETCURSEL            (WM_USER+9)
#define LB_GETTEXT              (WM_USER+10)
#define LB_GETTEXTLEN           (WM_USER+11)
#define LB_GETCOUNT             (WM_USER+12)
#define LB_SELECTSTRING         (WM_USER+13)
#define LB_DIR                  (WM_USER+14)
#define LB_GETTOPINDEX          (WM_USER+15)
#define LB_FINDSTRING           (WM_USER+16)
#define LB_GETSELCOUNT          (WM_USER+17)
#define LB_GETSELITEMS          (WM_USER+18)
#define LB_SETTABSTOPS          (WM_USER+19)
#define LB_GETHORIZONTALEXTENT  (WM_USER+20)
#define LB_SETHORIZONTALEXTENT  (WM_USER+21)
#define LB_SETCOLUMNWIDTH       (WM_USER+22)
#define LB_SETTOPINDEX          (WM_USER+24)
#define LB_GETITEMRECT          (WM_USER+25)
#define LB_GETITEMDATA          (WM_USER+26)
#define LB_SETITEMDATA          (WM_USER+27)
#define LB_SELITEMRANGE         (WM_USER+28)
#define LB_SETANCHORINDEX       (WM_USER+29)
#define LB_GETANCHORINDEX       (WM_USER+30)
#define LB_SETCARETINDEX        (WM_USER+31)
#define LB_GETCARETINDEX        (WM_USER+32)

#if (WINVER >= 0x030a)
#define LB_SETITEMHEIGHT        (WM_USER+33)
#define LB_GETITEMHEIGHT        (WM_USER+34)
#define LB_FINDSTRINGEXACT      (WM_USER+35)
#endif  /* WINVER >= 0x030a */

#endif  /* NOWINMESSAGES */

/* Listbox notification codes */
#define LBN_ERRSPACE            (-2)
#define LBN_SELCHANGE           1
#define LBN_DBLCLK              2
#define LBN_SELCANCEL           3
#define LBN_SETFOCUS            4
#define LBN_KILLFOCUS           5

/* Listbox message return values */
#define LB_OKAY                 0
#define LB_ERR                  (-1)
#define LB_ERRSPACE             (-2)

#define LB_CTLCODE              0L

/****** Dialog directory support ********************************************/

int     WINAPI DlgDirList(HWND, LPSTR, int, int, UINT);
BOOL    WINAPI DlgDirSelect(HWND, LPSTR, int);

int     WINAPI DlgDirListComboBox(HWND, LPSTR, int, int, UINT);
BOOL    WINAPI DlgDirSelectComboBox(HWND, LPSTR, int);

#if (WINVER >= 0x030a)
BOOL    WINAPI DlgDirSelectEx(HWND, LPSTR, int, int);
BOOL    WINAPI DlgDirSelectComboBoxEx(HWND, LPSTR, int, int);
#endif  /* WINVER >= 0x030a */

/* DlgDirList, DlgDirListComboBox flags values */
#define DDL_READWRITE           0x0000
#define DDL_READONLY            0x0001
#define DDL_HIDDEN              0x0002
#define DDL_SYSTEM              0x0004
#define DDL_DIRECTORY           0x0010
#define DDL_ARCHIVE             0x0020

#define DDL_POSTMSGS            0x2000
#define DDL_DRIVES              0x4000
#define DDL_EXCLUSIVE           0x8000

#endif  /* NOCTLMGR */

/****** Combo box control **************************************************/

#ifndef NOCTLMGR

/* Combo box styles */
#ifndef NOWINSTYLES
#define CBS_SIMPLE              0x0001L
#define CBS_DROPDOWN            0x0002L
#define CBS_DROPDOWNLIST        0x0003L
#define CBS_OWNERDRAWFIXED      0x0010L
#define CBS_OWNERDRAWVARIABLE   0x0020L
#define CBS_AUTOHSCROLL         0x0040L
#define CBS_OEMCONVERT          0x0080L
#define CBS_SORT                0x0100L
#define CBS_HASSTRINGS          0x0200L
#define CBS_NOINTEGRALHEIGHT    0x0400L
#if (WINVER >= 0x030a)
#define CBS_DISABLENOSCROLL     0x0800L
#endif  /* WINVER >= 0x030a */
#endif  /* NOWINSTYLES */

/* Combo box messages */
#ifndef NOWINMESSAGES
#define CB_GETEDITSEL           (WM_USER+0)
#define CB_LIMITTEXT            (WM_USER+1)
#define CB_SETEDITSEL           (WM_USER+2)
#define CB_ADDSTRING            (WM_USER+3)
#define CB_DELETESTRING         (WM_USER+4)
#define CB_DIR                  (WM_USER+5)
#define CB_GETCOUNT             (WM_USER+6)
#define CB_GETCURSEL            (WM_USER+7)
#define CB_GETLBTEXT            (WM_USER+8)
#define CB_GETLBTEXTLEN         (WM_USER+9)
#define CB_INSERTSTRING         (WM_USER+10)
#define CB_RESETCONTENT         (WM_USER+11)
#define CB_FINDSTRING           (WM_USER+12)
#define CB_SELECTSTRING         (WM_USER+13)
#define CB_SETCURSEL            (WM_USER+14)
#define CB_SHOWDROPDOWN         (WM_USER+15)
#define CB_GETITEMDATA          (WM_USER+16)
#define CB_SETITEMDATA          (WM_USER+17)
#if (WINVER >= 0x030a)
#define CB_GETDROPPEDCONTROLRECT (WM_USER+18)
#define CB_SETITEMHEIGHT        (WM_USER+19)
#define CB_GETITEMHEIGHT        (WM_USER+20)
#define CB_SETEXTENDEDUI        (WM_USER+21)
#define CB_GETEXTENDEDUI        (WM_USER+22)
#define CB_GETDROPPEDSTATE      (WM_USER+23)
#define CB_FINDSTRINGEXACT      (WM_USER+24)
#endif  /* WINVER >= 0x030a */

#endif  /* NOWINMESSAGES */

/* Combo box notification codes */
#define CBN_ERRSPACE            (-1)
#define CBN_SELCHANGE           1
#define CBN_DBLCLK              2
#define CBN_SETFOCUS            3
#define CBN_KILLFOCUS           4
#define CBN_EDITCHANGE          5
#define CBN_EDITUPDATE          6
#define CBN_DROPDOWN            7
#if (WINVER >= 0x030a)
#define CBN_CLOSEUP             8
#define CBN_SELENDOK            9
#define CBN_SELENDCANCEL        10
#endif  /* WINVER >= 0x030a */

/* Combo box message return values */
#define CB_OKAY                 0
#define CB_ERR                  (-1)
#define CB_ERRSPACE             (-2)

#endif /* NOCTLMGR */

/******* Windows hook support **********************************************/

#ifndef NOWH

DECLARE_HANDLE32(HHOOK);

#ifdef STRICT
typedef LRESULT (CALLBACK* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);
#else
typedef FARPROC HOOKPROC;
#endif

#ifdef STRICT
HHOOK   WINAPI SetWindowsHook(int, HOOKPROC);
LRESULT WINAPI DefHookProc(int, WPARAM, LPARAM, HHOOK FAR*);
#else
HOOKPROC WINAPI SetWindowsHook(int, HOOKPROC);
LRESULT WINAPI DefHookProc(int, WPARAM, LPARAM, HOOKPROC FAR*);
#endif
BOOL    WINAPI UnhookWindowsHook(int, HOOKPROC);

#if (WINVER >= 0x030a)

HHOOK   WINAPI SetWindowsHookEx(int idHook, HOOKPROC lpfn, HINSTANCE hInstance, HTASK hTask);
BOOL    WINAPI UnhookWindowsHookEx(HHOOK hHook);
LRESULT WINAPI CallNextHookEx(HHOOK hHook, int code, WPARAM wParam, LPARAM lParam);

#endif  /* WINVER >= 0x030a */

/* Standard hook code */
#define HC_ACTION               0

/* Obsolete hook codes (NO LONGER SUPPORTED) */
#define HC_GETLPLPFN            (-3)
#define HC_LPLPFNNEXT           (-2)
#define HC_LPFNNEXT             (-1)

#endif /* NOWH */

/****** Computer-based-training (CBT) support *******************************/

#ifndef NOWH

/* SetWindowsHook() code */
#define WH_CBT              5

#define HCBT_MOVESIZE        0
#define HCBT_MINMAX          1
#define HCBT_QS              2
#define HCBT_CREATEWND       3
#define HCBT_DESTROYWND      4
#define HCBT_ACTIVATE        5
#define HCBT_CLICKSKIPPED    6
#define HCBT_KEYSKIPPED      7
#define HCBT_SYSCOMMAND      8
#define HCBT_SETFOCUS        9
#define HCBT_SETWINDOWTEXT  10

#if (WINVER >= 0x030a)
/* HCBT_CREATEWND parameters pointed to by lParam */
typedef struct tagCBT_CREATEWND
{
    LPCREATESTRUCT lpcs;
    HWND    hwndInsertAfter;
} CBT_CREATEWND;
typedef CBT_CREATEWND FAR* LPCBT_CREATEWND;

/* HCBT_ACTIVATE structure pointed to by lParam */
typedef struct tagCBTACTIVATESTRUCT
{
    BOOL    fMouse;
    HWND    hWndActive;
} CBTACTIVATESTRUCT;

#endif  /* WINVER >= 0x030a */
#endif /* NOWH */

/****** Hardware hook support ***********************************************/

#ifndef NOWH
#if (WINVER >= 0x030a)
#define WH_HARDWARE         8

typedef struct tagHARDWAREHOOKSTRUCT
{
    HWND    hWnd;
    UINT    wMessage;
    WPARAM  wParam;
    LPARAM  lParam;
} HARDWAREHOOKSTRUCT;
#endif  /* WINVER >= 0x030a */
#endif  /* NOWH */

/****** Shell support *******************************************************/

#ifndef NOWH
#if (WINVER >= 0x030a)
/* SetWindowsHook() Shell hook code */
#define WH_SHELL           10

#define HSHELL_WINDOWCREATED       1
#define HSHELL_WINDOWDESTROYED     2
#define HSHELL_ACTIVATESHELLWINDOW 3
#if (WINVER >= 0x0400)
#define HSHELL_WINDOWACTIVATED     4
#define HSHELL_GETMINRECT          5
#define HSHELL_REDRAW              6
#define HSHELL_TASKMAN             7
#define HSHELL_LANGUAGE            8 
#endif  /* WINVER >= 0x0400 */
#endif  /* WINVER >= 0x030a */
#endif  /* NOWH */

/****** Journalling support *************************************************/

#ifndef NOWH
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1

/* Journalling hook codes */
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREMOVE         3
#define HC_NOREM            HC_NOREMOVE
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

/* Journalling message structure */
typedef struct tagEVENTMSG
{
    UINT    message;
    UINT    paramL;
    UINT    paramH;
    DWORD   time;
} EVENTMSG;
typedef EVENTMSG *PEVENTMSG;
typedef EVENTMSG NEAR* NPEVENTMSG;
typedef EVENTMSG FAR* LPEVENTMSG;

BOOL    WINAPI EnableHardwareInput(BOOL);

#endif /* NOWH */


/****** Debugger support ****************************************************/

#if (WINVER >= 0x030a)
/* SetWindowsHook debug hook support */
#define WH_DEBUG        9

typedef struct tagDEBUGHOOKINFO
{
    HMODULE     hModuleHook;
    LPARAM      reserved;
    LPARAM      lParam;
    WPARAM      wParam;
    int         code;
} DEBUGHOOKINFO;
typedef DEBUGHOOKINFO FAR* LPDEBUGHOOKINFO;

#ifndef NOMSG
BOOL WINAPI QuerySendMessage(HANDLE h1, HANDLE h2, HANDLE h3, LPMSG lpmsg);
#endif  /* NOMSG */

BOOL WINAPI LockInput(HANDLE h1, HWND hwndInput, BOOL fLock);

LONG WINAPI GetSystemDebugState(void);
/* Flags returned by GetSystemDebugState.
 */
#define SDS_MENU        0x0001
#define SDS_SYSMODAL    0x0002
#define SDS_NOTASKQUEUE 0x0004
#define SDS_DIALOG      0x0008
#define SDS_TASKLOCKED  0x0010
#endif  /* WINVER >= 0x030a */

/****** Help support ********************************************************/

#ifndef NOHELP

BOOL WINAPI WinHelp(HWND hwndMain, LPCSTR lpszHelp, UINT usCommand, DWORD ulData);

/* WinHelp() commands */
#define HELP_CONTEXT        0x0001
#define HELP_QUIT           0x0002
#define HELP_INDEX          0x0003
#define HELP_CONTENTS       0x0003
#define HELP_HELPONHELP     0x0004
#define HELP_SETINDEX       0x0005
#define HELP_SETCONTENTS    0x0005
#define HELP_CONTEXTPOPUP   0x0008
#define HELP_FORCEFILE      0x0009
#define HELP_KEY            0x0101
#define HELP_COMMAND        0x0102
#define HELP_PARTIALKEY     0x0105
#define HELP_MULTIKEY       0x0201
#define HELP_SETWINPOS      0x0203

#if (WINVER >= 0x0400)

#define HELP_CONTEXTMENU    0x000a
#define HELP_FINDER         0x000b
#define HELP_WM_HELP        0x000c

#define HELP_TCARD          0x8000
#define HELP_TCARD_DATA     0x0010
#define HELP_TCARD_NEXT     0x0011
#define HELP_TCARD_OTHER_CALLER 0x0011

#endif  /* WINVER >= 0x0400 */

typedef struct tagMULTIKEYHELP
{
    UINT    mkSize;
    BYTE    mkKeylist;
    BYTE    szKeyphrase[1];
} MULTIKEYHELP;


typedef struct
{
    int  wStructSize;
    int  x;
    int  y;
    int  dx;
    int  dy;
    int  wMax;
    char rgchMember[2];
} HELPWININFO;
typedef HELPWININFO NEAR* PHELPWININFO;
typedef HELPWININFO FAR* LPHELPWININFO;

#endif  /* NOHELP */


/****** Message Box support *************************************************/

#ifndef NOMB

int     WINAPI MessageBox(HWND, LPCSTR, LPCSTR, UINT);
void    WINAPI MessageBeep(UINT);

#define MB_OK               0x0000
#define MB_OKCANCEL         0x0001
#define MB_ABORTRETRYIGNORE 0x0002
#define MB_YESNOCANCEL      0x0003
#define MB_YESNO            0x0004
#define MB_RETRYCANCEL      0x0005
#define MB_TYPEMASK         0x000F

#define MB_ICONHAND         0x0010
#define MB_ICONQUESTION     0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040
#define MB_ICONMASK         0x00F0

#define MB_ICONINFORMATION  MB_ICONASTERISK
#define MB_ICONSTOP         MB_ICONHAND

#define MB_DEFBUTTON1       0x0000
#define MB_DEFBUTTON2       0x0100
#define MB_DEFBUTTON3       0x0200
#if (WINVER >= 0x0400)
#define MB_DEFBUTTON4       0x0300
#endif
#define MB_DEFMASK          0x0F00

#define MB_APPLMODAL        0x0000
#define MB_SYSTEMMODAL      0x1000
#define MB_TASKMODAL        0x2000
#define MB_NOFOCUS          0x8000
#endif  /* NOMB */

/****** Sound support ******************************************************/

#ifndef NOSOUND

int     WINAPI OpenSound(void);
void    WINAPI CloseSound(void);

int     WINAPI StartSound(void);
int     WINAPI StopSound(void);

int     WINAPI SetVoiceQueueSize(int, int);
int     WINAPI SetVoiceNote(int, int, int, int);
int     WINAPI SetVoiceAccent(int, int, int, int, int);
int     WINAPI SetVoiceEnvelope(int, int, int);
int     WINAPI SetVoiceSound(int, DWORD, int);

int     WINAPI SetVoiceThreshold(int, int);
LPINT   WINAPI GetThresholdEvent(void);
int     WINAPI GetThresholdStatus(void);

int     WINAPI SetSoundNoise(int, int);

/* SetSoundNoise() Sources */
#define S_PERIOD512     0
#define S_PERIOD1024    1
#define S_PERIOD2048    2
#define S_PERIODVOICE   3
#define S_WHITE512      4
#define S_WHITE1024     5
#define S_WHITE2048     6
#define S_WHITEVOICE    7

int     WINAPI WaitSoundState(int);

/* WaitSoundState() constants */
#define S_QUEUEEMPTY    0
#define S_THRESHOLD     1
#define S_ALLTHRESHOLD  2

int     WINAPI SyncAllVoices(void);
int     WINAPI CountVoiceNotes(int);

/* Accent Modes */
#define S_NORMAL        0
#define S_LEGATO        1
#define S_STACCATO      2

/* Error return values */
#define S_SERDVNA       (-1)
#define S_SEROFM        (-2)
#define S_SERMACT       (-3)
#define S_SERQFUL       (-4)
#define S_SERBDNT       (-5)
#define S_SERDLN        (-6)
#define S_SERDCC        (-7)
#define S_SERDTP        (-8)
#define S_SERDVL        (-9)
#define S_SERDMD        (-10)
#define S_SERDSH        (-11)
#define S_SERDPT        (-12)
#define S_SERDFQ        (-13)
#define S_SERDDR        (-14)
#define S_SERDSR        (-15)
#define S_SERDST        (-16)

#endif  /* NOSOUND */

/****** Comm support ******************************************************/

#ifndef NOCOMM

#define NOPARITY        0
#define ODDPARITY       1
#define EVENPARITY      2
#define MARKPARITY      3
#define SPACEPARITY     4

#define ONESTOPBIT      0
#define ONE5STOPBITS    1
#define TWOSTOPBITS     2

#define IGNORE          0
#define INFINITE        0xFFFF

/* Error Flags */
#define CE_RXOVER       0x0001
#define CE_OVERRUN      0x0002
#define CE_RXPARITY     0x0004
#define CE_FRAME        0x0008
#define CE_BREAK        0x0010
#define CE_CTSTO        0x0020
#define CE_DSRTO        0x0040
#define CE_RLSDTO       0x0080
#define CE_TXFULL       0x0100
#define CE_PTO          0x0200
#define CE_IOE          0x0400
#define CE_DNS          0x0800
#define CE_OOP          0x1000
#define CE_MODE         0x8000

#define IE_BADID        (-1)
#define IE_OPEN         (-2)
#define IE_NOPEN        (-3)
#define IE_MEMORY       (-4)
#define IE_DEFAULT      (-5)
#define IE_HARDWARE     (-10)
#define IE_BYTESIZE     (-11)
#define IE_BAUDRATE     (-12)

/* Events */
#define EV_RXCHAR       0x0001
#define EV_RXFLAG       0x0002
#define EV_TXEMPTY      0x0004
#define EV_CTS          0x0008
#define EV_DSR          0x0010
#define EV_RLSD         0x0020
#define EV_BREAK        0x0040
#define EV_ERR          0x0080
#define EV_RING         0x0100
#define EV_PERR         0x0200
#define EV_CTSS         0x0400
#define EV_DSRS         0x0800
#define EV_RLSDS        0x1000
#define EV_RingTe       0x2000
#define EV_RINGTE       EV_RingTe

/* Escape Functions */
#define SETXOFF         1
#define SETXON          2
#define SETRTS          3
#define CLRRTS          4
#define SETDTR          5
#define CLRDTR          6
#define RESETDEV        7

#define LPTx            0x80

#if (WINVER >= 0x030a)

/* new escape functions */
#define GETMAXLPT       8
#define GETMAXCOM       9
#define GETBASEIRQ      10

/* Comm Baud Rate indices */
#define CBR_110         0xFF10
#define CBR_300         0xFF11
#define CBR_600         0xFF12
#define CBR_1200        0xFF13
#define CBR_2400        0xFF14
#define CBR_4800        0xFF15
#define CBR_9600        0xFF16
#define CBR_14400       0xFF17
#define CBR_19200       0xFF18
#define CBR_38400       0xFF1B
#define CBR_56000       0xFF1F
#define CBR_128000      0xFF23
#define CBR_256000      0xFF27

/* notifications passed in low word of lParam on WM_COMMNOTIFY messages */
#define CN_RECEIVE      0x0001
#define CN_TRANSMIT     0x0002
#define CN_EVENT        0x0004

#endif  /* WINVER >= 0x030a */

typedef struct tagDCB
{
    BYTE Id;
    UINT BaudRate;
    BYTE ByteSize;
    BYTE Parity;
    BYTE StopBits;
    UINT RlsTimeout;
    UINT CtsTimeout;
    UINT DsrTimeout;

    UINT fBinary        :1;
    UINT fRtsDisable    :1;
    UINT fParity        :1;
    UINT fOutxCtsFlow   :1;
    UINT fOutxDsrFlow   :1;
    UINT fDummy         :2;
    UINT fDtrDisable    :1;

    UINT fOutX          :1;
    UINT fInX           :1;
    UINT fPeChar        :1;
    UINT fNull          :1;
    UINT fChEvt         :1;
    UINT fDtrflow       :1;
    UINT fRtsflow       :1;
    UINT fDummy2        :1;

    char XonChar;
    char XoffChar;
    UINT XonLim;
    UINT XoffLim;
    char PeChar;
    char EofChar;
    char EvtChar;
    UINT TxDelay;
} DCB;
typedef DCB FAR* LPDCB;

#if (defined(STRICT) | (WINVER >= 0x030a))

typedef struct tagCOMSTAT
{
    BYTE status;
    UINT cbInQue;
    UINT cbOutQue;
} COMSTAT;

#define CSTF_CTSHOLD    0x01
#define CSTF_DSRHOLD    0x02
#define CSTF_RLSDHOLD   0x04
#define CSTF_XOFFHOLD   0x08
#define CSTF_XOFFSENT   0x10
#define CSTF_EOF        0x20
#define CSTF_TXIM       0x40

#else   /* (STRICT | WINVER >= 0x030a) */

/* NOTE: This structure declaration is not ANSI compatible! */
typedef struct tagCOMSTAT
{
    BYTE fCtsHold  :1;
    BYTE fDsrHold  :1;
    BYTE fRlsdHold :1;
    BYTE fXoffHold :1;
    BYTE fXoffSent :1;
    BYTE fEof      :1;
    BYTE fTxim     :1;
    UINT cbInQue;
    UINT cbOutQue;
} COMSTAT;

#endif  /* !(STRICT | WINVER >= 0x030a */

int     WINAPI BuildCommDCB(LPCSTR, DCB FAR*);

int     WINAPI OpenComm(LPCSTR, UINT, UINT);
int     WINAPI CloseComm(int);

int     WINAPI ReadComm(int, void FAR*, int);
int     WINAPI WriteComm(int, const void FAR*, int);
int     WINAPI UngetCommChar(int, char);
int     WINAPI FlushComm(int, int);
int     WINAPI TransmitCommChar(int, char);

int     WINAPI SetCommState(const DCB FAR*);
int     WINAPI GetCommState(int, DCB FAR*);
int     WINAPI GetCommError(int, COMSTAT FAR* );

int     WINAPI SetCommBreak(int);
int     WINAPI ClearCommBreak(int);

UINT FAR* WINAPI SetCommEventMask(int, UINT);
UINT    WINAPI GetCommEventMask(int, int);

LONG    WINAPI EscapeCommFunction(int, int);

#if (WINVER >= 0x030a)
BOOL    WINAPI EnableCommNotification(int, HWND, int, int);

#endif  /* WINVER >= 0x030a */

#endif  /* NOCOMM */

/****** String formatting support *******************************************/

int     WINAPI wvsprintf(LPSTR lpszOut, LPCSTR lpszFmt, const void FAR* lpParams);

int FAR CDECL wsprintf(LPSTR lpszOut, LPCSTR lpszFmt, ...);


/****** Driver support ******************************************************/

#if (WINVER >= 0x030a)

#ifndef NODRIVERS

DECLARE_HANDLE(HDRVR);

typedef LRESULT (CALLBACK* DRIVERPROC)(DWORD, HDRVR, UINT, LPARAM, LPARAM);

/* Driver messages */
#define DRV_LOAD            0x0001
#define DRV_ENABLE          0x0002
#define DRV_OPEN            0x0003
#define DRV_CLOSE           0x0004
#define DRV_DISABLE         0x0005
#define DRV_FREE            0x0006
#define DRV_CONFIGURE       0x0007
#define DRV_QUERYCONFIGURE  0x0008
#define DRV_INSTALL         0x0009
#define DRV_REMOVE          0x000A
#define DRV_EXITSESSION     0x000B
#define DRV_EXITAPPLICATION 0x000C
#define DRV_POWER           0x000F
#define DRV_NOTIFYMSG	      0x0010
/*#define DRV_QUERYENDSESSION 0x0010      */
/*#define DRV_EXITSESSIONCANCELLED 0x0011 */

#define DRV_RESERVED        0x0800
#define DRV_USER            0x4000

/* LPARAM of DRV_CONFIGURE message */
typedef struct tagDRVCONFIGINFO
{
    DWORD   dwDCISize;
    LPCSTR  lpszDCISectionName;
    LPCSTR  lpszDCIAliasName;
} DRVCONFIGINFO;
typedef DRVCONFIGINFO NEAR* PDRVCONFIGINFO;
typedef DRVCONFIGINFO FAR* LPDRVCONFIGINFO;

/* Supported return values for DRV_CONFIGURE message */
#define DRVCNF_CANCEL       0x0000
#define DRVCNF_OK           0x0001
#define DRVCNF_RESTART      0x0002

/* Supported lParam1 of DRV_EXITAPPLICATION notification */
#define DRVEA_NORMALEXIT    0x0001
#define DRVEA_ABNORMALEXIT  0x0002

LRESULT WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR driverID, UINT message, LPARAM lParam1, LPARAM lParam2);

HDRVR   WINAPI OpenDriver(LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam2);
LRESULT WINAPI CloseDriver(HDRVR hDriver, LPARAM lParam1, LPARAM lParam2);

LRESULT WINAPI SendDriverMessage(HDRVR hDriver, UINT message, LPARAM lParam1, LPARAM lParam2);

HINSTANCE WINAPI GetDriverModuleHandle(HDRVR hDriver);

HDRVR   WINAPI GetNextDriver(HDRVR, DWORD);

/* GetNextDriver flags */
#define GND_FIRSTINSTANCEONLY   0x00000001

#define GND_FORWARD             0x00000000
#define GND_REVERSE             0x00000002

typedef struct tagDRIVERINFOSTRUCT
{
    UINT    length;
    HDRVR   hDriver;
    HINSTANCE hModule;
    char    szAliasName[128];
} DRIVERINFOSTRUCT;
typedef DRIVERINFOSTRUCT FAR* LPDRIVERINFOSTRUCT;

BOOL    WINAPI GetDriverInfo(HDRVR, DRIVERINFOSTRUCT FAR*);

#endif  /* !NODRIVERS */
#endif  /* WINVER >= 0x030a */

/****** Installable Message Thunk support ***********************************/

#if (WINVER > 0x030a)
#ifndef NOIMT

/* wReason codes */
#define IMT_REASON_ASYNC    0x00001
#define IMT_REASON_LS       0x00002
#define IMT_REASON_AFTER    0x00004

typedef struct tagIMTMSG
{
    HWND    hwnd;
    UINT    message;
    DWORD   wParam;
    LPARAM  lParam;
} IMTMSG, FAR* LPIMTMSG;

typedef struct tagIMTBUFFER
{
    DWORD   dwT1;
    DWORD   dwT2;
} IMTBUFFER, FAR* LPIMTBUFFER;

typedef BOOL (CALLBACK *IMTDISP)(WORD wReason, LPDWORD lpRetVal,
                                 LPIMTMSG lpMsg, LPIMTBUFFER lpBuffer);
BOOL WINAPI InstallIMT(LPSTR lpszClassName, IMTDISP pfnDispatcher,
                       WORD msgLo, WORD msgHi);
BOOL WINAPI UnInstallIMT(LPSTR lpszClassName, IMTDISP pfnDispatcher,
                         WORD msgLo, WORD msgHi);

#endif  /* !NOIMT */
#endif  /* WINVER > 0x030a */

#endif  /* NOUSER */

DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwServiceType);
#define RSP_UNREGISTER_SERVICE	0x00000000
#define RSP_SIMPLE_SERVICE	0x00000001

VOID WINAPI RegisterNetworkCapabilities(DWORD dwBitsToSet, DWORD dwValues);
#define RNC_NETWORKS  		0x00000001





#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif /* __cplusplus */

#endif  /* _INC_WINDOWS */
