/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1992,1993
 *  All Rights Reserved.
 *
 *
 *  PIFMGRP.H
 *  Private PIFMGR include file
 *
 *  History:
 *  Created 31-Jul-1992 3:45pm by Jeff Parsons
 */


#include <windows.h>    // declares NULL the "right" way (as 0)

#ifndef RC_INVOKED
#include <malloc.h>     // misc. C runtime goop
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#ifndef WINNT
#include <setupx.h>     // for Ip (inf) API
#endif
#include <prsht.h>      // for PropertySheet(), pulls in shell.h
#include <commdlg.h>    // for GetOpenFileName(), GetSaveFileName()
#endif /* RC_INVOKED */

#include <commctrl.h>   // for TRACKBAR_CLASS, HOTKEY_CLASS...

#include <regstr.h>
#include <winerror.h>

#ifndef RC_INVOKED
#define PIF_PROPERTY_SHEETS
#include <pif.h>

#include "shellvm.h"    // for struct PIF_Key


#endif /* RC_INVOKED */


/*
 *  Supported app extension types
 */
#define APPTYPE_UNKNOWN         -1
#define APPTYPE_EXE             0
#define APPTYPE_COM             1
#define APPTYPE_BAT             2
#define APPTYPE_CMD             3
#define APPTYPE_PIF             4
#define MAX_APP_TYPES           5


/*
 *  Bitmap IDs
 */

#define DX_TTBITMAP             20
#define DY_TTBITMAP             12

#define MAX_STRING_SIZE         256

#ifndef RC_INVOKED


/*
 * Some typedefs mysteriously missing from windows.h
 */
typedef const WORD * LPCWORD;
typedef const VOID *LPCVOID;// pointer to const void

#define PROP_SIG                0x504A

/*  Property flags
 *
 *  Anyone can set PROP_DIRTY, but only Lock/UnlockPIFData should set
 *  PROP_DIRTYLOCK.  The latter is set/cleared when the last lock is about
 *  to be unlocked (ie, when cLocks is going back to zero).  At that time,
 *  if PROP_DIRTY is set, then we also set PROP_DIRTYLOCK and skip the
 *  call to GlobalUnlock;  on the other hand, if PROP_DIRTY is clear, then
 *  we also clear PROP_DIRTYLOCK and allow the call to GlobalUnlock to proceed.
 *
 *  A consequence is that you must NEVER clear PROP_DIRTY while the data
 *  is unlocked, unless you plan on checking PROP_DIRTYLOCK yourself and
 *  relinquishing that outstanding lock, if it exists.  It is much preferable
 *  to either clear PROP_DIRTY while the data is locked (so that UnlockPIFData
 *  will take care of it), or to simply call FlushPIFData with fDiscard set
 *  appropriately.
 */
#define PROP_DIRTY              0x0001  // memory block modified and unwritten
#define PROP_DIRTYLOCK          0x0002  // memory block locked
#define PROP_TRUNCATE           0x0004  // memory block shrunk, truncate on write
#define PROP_RAWIO              0x0008  // direct access to memory block allowed
#define PROP_NOTIFY             0x0010  // property sheet made changes
#define PROP_IGNOREPIF          0x0020  // entry in [pif] exists, ignore any PIF
#define PROP_SKIPPIF            0x0040  // don't try to open a PIF (various reasons)
#define PROP_NOCREATEPIF        0x0080  // we opened the PIF once, so don't recreate
#define PROP_REGEN              0x0100  // GetPIFData call in progress
#define PROP_DONTWRITE          0x0200  // someone else has flushed, don't write
#define PROP_REALMODE           0x0400  // disable non-real-mode props
#define PROP_PIFDIR             0x0800  // PIF found in PIF directory
#define PROP_NOPIF              0x1000  // no PIF found
#define PROP_DEFAULTPIF         0x2000  // default PIF found
#define PROP_INFSETTINGS        0x4000  // INF settings found
#define PROP_INHIBITPIF         0x8000  // INF or OpenProperties requested no PIF

#if (PROP_NOPIF != PRGINIT_NOPIF || PROP_DEFAULTPIF != PRGINIT_DEFAULTPIF || PROP_INFSETTINGS != PRGINIT_INFSETTINGS || PROP_INHIBITPIF != PRGINIT_INHIBITPIF)
#error Bit mismatch in PIF constants
#endif

#ifndef OF_READ
#define MAXPATHNAME 260
#else
#define MAXPATHNAME 260 // BUGBUG undefined (davepl) (sizeof(OFSTRUCTEX)-9)
#endif


typedef struct PIFOFSTRUCT {
    DWORD   nErrCode;
    TCHAR   szPathName[MAXPATHNAME];
} PIFOFSTRUCT, *LPPIFOFSTRUCT;


typedef struct PROPLINK {       /* pl */
    struct    PROPLINK *ppl;      //
    struct    PROPLINK *pplNext;  //
    struct    PROPLINK *pplPrev;  //
    int       iSig;               // proplink signature
    int       flProp;             // proplink flags (PROP_*)
    int       cbPIFData;          // size of PIF data
    int       cLocks;             // # of locks, if any
    LPPIFDATA lpPIFData;          // pointer (non-NULL if PIF data locked)
    int       ckbMem;             // memory setting from WIN.INI (-1 if none)
    int       iSheetUsage;        // number of prop sheets using this struct
    LPCTSTR   lpszTitle;          // title to use in dialogs (NULL if none)
    HWND      hwndNotify;         // who to notify when PROP_NOTIFY has been set
    UINT      uMsgNotify;         // message number to use when notifying, 0 if none
    DWORD     hVM;                // handle to associated VM (if any)
    HWND      hwndTty;            // handle to associated window (if any)
    LPTSTR    lpArgs;             // pointer to args for this instance (if any)
    HANDLE    hPIF;               // handle to PIF file
    PIFOFSTRUCT ofPIF;            // hacked OpenFile() structure for PIF
    UINT      iFileName;          // offset of base filename in szPathName
    UINT      iFileExt;           // offset of base filename extension in szPathName
    TCHAR     szPathName[MAXPATHNAME];
} PROPLINK;
typedef PROPLINK *PPROPLINK;


#ifndef ROUNDUNITS
#define ROUNDUNITS(a,b)    (((a)+(b)-1)/(b))
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif

#define INI_WORDS       (5 + ROUNDUNITS(sizeof (WINDOWPLACEMENT), 2))
#define MAX_INT_LENGTH  10      // "-32767" + separator + 3 chars of slop
#define MAX_INI_WORDS   20
#define MAX_INI_BUFFER  (MAX_INT_LENGTH * MAX_INI_WORDS)
#define ISVALIDINI(w)   ((w)==1 || (w)==3 || (w)==5 || (w) == INI_WORDS)

/*  Owing to the fact that wFlags was originally defined as a combination
 *  of font and window settings, WIN_SAVESETTINGS and WIN_TOOLBAR should be
 *  considered reserved FNT flags.  It is problematic to assert that no
 *  one ever use those FNT flags for anything, but I will at least try to
 *  catch them sticking those bits into FNT_DEFAULT....
 */

#if (FNT_DEFAULT & (WIN_SAVESETTINGS | WIN_TOOLBAR))
#error Reserved FNT flags incorrectly used
#endif

#if (FNT_TTFONTS - FNT_RASTERFONTS) != (FNT_BOTHFONTS - FNT_TTFONTS)
#error Incorrect bit value(s) for FNT_RASTERFONTS and/or FNT_TTFONTS
#endif

typedef struct INIINFO {
    WORD    wFlags;             // This order is the same as written to file
    WORD    wFontWidth;         // We assume that if zero width, nothing to init
    WORD    wFontHeight;
    WORD    wWinWidth;
    WORD    wWinHeight;
    WINDOWPLACEMENT wp;         // If normalposition.left & right are zero,
    BYTE    szInvokedName[128+1];//   there is no position to restore
} INIINFO;
typedef INIINFO *PINIINFO;
typedef INIINFO *LPINIINFO;


/*
 * Types/structures for GetINIData
 */
#define INIDATA_DECINT      0x0001
#define INIDATA_FIXEDPOINT  0x0002
#define INIDATA_BOOLEAN     0x0004
#define INIDATA_INVERT      0x1000

typedef struct _INIDATA {
    const TCHAR *pszSection;
    const TCHAR *pszKey;
    void *pValue;
    int  iFlags;
    int  iMask;
} INIDATA, *PINIDATA;

/*
 *  Structure used to define bits associated with control IDs
 */
typedef struct _BITINFO {   /* binf */
    WORD id;                /* Control ID (must be edit control) */
    BYTE bBit;              /* bit #; if bit 7 set, sense of bit reversed */
};
typedef const struct _BITINFO BINF;
typedef const struct _BITINFO *PBINF;

#define Z2(m)               ((m)&1?0:(m)&2?1:2)
#define Z4(m)               ((m)&3?Z2(m):Z2((m)>>2)+2)
#define Z8(m)               ((m)&15?Z4(m):Z4((m)>>4)+4)
#define Z16(m)              ((m)&255?Z8(m):Z8((m)>>8)+8)
#define Z32(m)              ((m)&65535?Z16(m):Z16((m)>>16)+16)
#define BITNUM(m)           Z32(m)

/*
 * Warning: There is some evil overloading of these switches because
 * there isn't enough time to do it `right'.
 *
 * VINF_AUTO means that a value of zero means `Auto' and a nonzero
 * value represents itself.
 *
 * VINF_AUTOMINMAX means that there are really two fields, a min and a
 * max.  If the two values are equal to each other, then the field value
 * is the common value, possibly zero for `None'.  Otherwise, the min
 * is iMin and the max is iMax, which indicates `Auto'.
 */

#define VINF_NONE           0x00
#define VINF_AUTO           0x01    /* integer field supports AUTO only */
#define VINF_AUTOMINMAX     0x02    /* integer field supports AUTO and NONE */

/*
 *  Structure used to validate integer parameters in property sheets.
 */
typedef struct _VALIDATIONINFO {/* vinf */
    BYTE off;               /* offset of integer in property structure */
    BYTE fbOpt;             /* See VINF_* constants */
    WORD id;                /* Control ID (must be edit control) */
    INT  iMin;              /* Minimum acceptable value */
    INT  iMax;              /* Maximum acceptable value */
    WORD idMsg;             /* Message resource for error message */
};
typedef const struct _VALIDATIONINFO VINF;
typedef const struct _VALIDATIONINFO *PVINF;

#define NUM_TICKS 20        /* Number of tick marks in slider control */

/*
 *  Macro to dispatch Help subsystem messages.
 */
#define HELP_CASES(rgdwHelp)                                        \
    case WM_HELP:               /* F1 or title-bar help button */   \
        OnWmHelp(lParam, &rgdwHelp[0]);                             \
        break;                                                      \
                                                                    \
    case WM_CONTEXTMENU:        /* right mouse click */             \
        OnWmContextMenu(wParam, &rgdwHelp[0]);                      \
        break;


/*
 *  Internal function prototypes
 */

/* XLATOFF */

#ifndef DEBUG
#define ASSERTFAIL()
#define ASSERTTRUE(exp)
#define ASSERTFALSE(exp)
#define VERIFYTRUE(exp)  (exp)
#define VERIFYFALSE(exp) (exp)
#else
#define ASSERTFAIL()     ASSERT(FALSE)
#define ASSERTTRUE(exp)  ASSERT((exp))
#define ASSERTFALSE(exp) ASSERT((!(exp)))
#define VERIFYTRUE(exp)  ASSERT((exp))
#define VERIFYFALSE(exp) ASSERT((!(exp)))
#endif

/*
 * CTASSERT  -- Assert at compile-time, standalone.
 * CTASSERTF -- Assert at compile-time, inside a function.
 */

#define CTASSERTF(c) switch (0) case 0: case c:
#define CTASSERTPP(c,l) \
    static INLINE void Assert##l(void) { CTASSERTF(c); }
#define CTASSERTP(c,l) CTASSERTPP(c,l)
#define CTASSERT(c) CTASSERTP(c,__LINE__)


/*
 * FunctionName allows us to make something happen on entry to every function.
 *
 * If SWAP_TUNING is defined, then the function name is squirted out the first
 * time it is called.  This is used to decide which functions should go into
 * the PRELOAD segment and which in the RARE segment.
 */

#ifndef DEBUG
#define FunctionName(f)
#else
#ifdef SWAP_TUNING
#define FunctionName(f) \
    static fSeen = 0; if (!fSeen) { OutputDebugString(#f TEXT("\r\n")); fSeen = 1; }
#else
#define FunctionName(f)
#endif
#endif


#ifdef WINNT
#ifdef UNICODE

// NT and UNICODE
#define NUM_DATA_PTRS 4
#else

// NT, but not UNICODE
#define NUM_DATA_PTRS 3
#endif

#else

// Neither NT or UNICODE
#define NUM_DATA_PTRS 2

#endif

#define LP386_INDEX 0
#define LPENH_INDEX 1
#define LPNT31_INDEX 2
#define LPNT40_INDEX 3

#ifdef WINNT
// Macro definitions that handle codepages 
//
#define CP_US       (UINT)437
#define CP_JPN      (UINT)932
#define CP_WANSUNG  (UINT)949
#define CP_TC       (UINT)950
#define CP_SC       (UINT)936

#define IsBilingualCP(cp) ((cp)==CP_JPN || (cp)==CP_WANSUNG)
#define IsFarEastCP(cp) ((cp)==CP_JPN || (cp)==CP_WANSUNG || (cp)==CP_TC || (cp)==CP_SC)
#endif

typedef LPVOID * DATAPTRS;


typedef int (*GETSETFN)(HANDLE hProps, LPCSTR lpszGroup, LPVOID lpProps, int cbProps, UINT flOpt);
typedef int (*DATAGETFN)(PPROPLINK ppl, DATAPTRS aDataPtrs, LPVOID lpData, int cbData, UINT flOpt);
typedef int (*DATASETFN)(PPROPLINK ppl, DATAPTRS aDataPtrs, LPCVOID lpData, int cbData, UINT flOpt);


/*
 *  Constant strings used in multiple places.
 *
 *  The null string is so popular, we keep a copy of it in each segment.
 */

extern const TCHAR c_szNULL[];     // Null string in nonresident code segment
extern const TCHAR r_szNULL[];     // Null string in resident code segment

extern TCHAR g_szNone[16];
extern TCHAR g_szAuto[16];

extern const TCHAR szNoPIFSection[];

extern CHAR szSTDHDRSIG[];
extern CHAR szW286HDRSIG30[];
extern CHAR szW386HDRSIG30[];
extern CHAR szWENHHDRSIG40[];

extern CHAR szCONFIGHDRSIG40[];
extern CHAR szAUTOEXECHDRSIG40[];

extern const TCHAR szDOSAPPDefault[];
extern const TCHAR szDOSAPPINI[];
extern const TCHAR szDOSAPPSection[];

// In alphabetical order, for sanity's sake.

extern const TCHAR sz386EnhSection[];
extern const TCHAR szDisplay[];
extern const TCHAR szTTDispDimKey[];
extern const TCHAR szTTInitialSizes[];

extern const TCHAR szNonWinSection[];
extern const TCHAR szPP4[];
extern const TCHAR szSystemINI[];
extern const TCHAR szWOAFontKey[];
extern const TCHAR szWOADBCSFontKey[];
extern const TCHAR szZero[];

// these are initialized at LoadGlobalFontData()
extern TCHAR szTTCacheSection[2][32];
extern CHAR szTTFaceName[2][LF_FACESIZE];

#ifdef  CUSTOMIZABLE_HEURISTICS
extern const TCHAR szTTHeuristics[];
extern const TCHAR szTTNonAspectMin[];
#endif

extern const TCHAR *apszAppType[];

// pifdll.asm
void GetSetExtendedData(DWORD hVM, WORD wGroup, LPCTSTR lpszGroup, LPVOID lpProps);
WORD GetVxDVersion(WORD wVxdId);
BOOL IsBufferDifferent(LPVOID lpv1, LPVOID lpv2, UINT cb);
#ifndef WIN32
void BZero(LPVOID lpvBuf, UINT cb);
#else
#define BZero(lpvBuf,cb) ZeroMemory(lpvBuf,(DWORD)cb)
#endif
#ifndef WINNT
WORD flEmsSupport(void);
#endif

// pifmgr.c

void GetINIData(void);
void InitProperties(PPROPLINK ppl, BOOL fLocked);

void ReadAdvPrgData(PPROPLINK ppl, int cbMax, LPCSTR lpszName, LPTSTR pszFile, UINT flOpt);
BOOL WriteAdvPrgData(PPROPLINK ppl, int cbMax, LPCSTR lpszName, LPTSTR pszFile, LPTSTR pszOrigFile, BOOL fCreateAnyway, BOOL fForceReboot);

PPROPLINK ValidPropHandle(HANDLE hProps);
int   ResizePIFData(PPROPLINK ppl, int cbResize);
BOOL  GetPIFData(PPROPLINK ppl, BOOL fLocked);
BOOL  FlushPIFData(PPROPLINK ppl, BOOL fDiscard);

LPWENHPIF40 AddEnhancedData(PPROPLINK ppl, LPW386PIF30 lp386);
BOOL        AddGroupData(PPROPLINK ppl, LPCSTR lpszGroup, LPVOID lpGroup, int cbGroup);
BOOL        RemoveGroupData(PPROPLINK ppl, LPCSTR lpszGroup);
LPVOID      GetGroupData(PPROPLINK ppl, LPCSTR lpszGroup, LPINT lpcbGroup, LPPIFEXTHDR *lplpph);

// pifdat.c


int GetPrgData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPPRG lpPrg, int cb, UINT flOpt);
int SetPrgData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPPRG lpPrg, int cb, UINT flOpt);
int GetTskData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPTSK lpTsk, int cb, UINT flOpt);
int SetTskData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPTSK lpTsk, int cb, UINT flOpt);
int GetVidData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPVID lpVid, int cb, UINT flOpt);
int SetVidData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPVID lpVid, int cb, UINT flOpt);
int GetMemData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMEM lpMem, int cb, UINT flOpt);
int SetMemData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMEM lpMem, int cb, UINT flOpt);
int GetKbdData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPKBD lpKbd, int cb, UINT flOpt);
int SetKbdData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPKBD lpKbd, int cb, UINT flOpt);
int GetMseData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMSE lpMse, int cb, UINT flOpt);
int SetMseData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPMSE lpMse, int cb, UINT flOpt);
int GetSndData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPSND lpSnd, int cb, UINT flOpt);
int SetSndData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPSND lpSnd, int cb, UINT flOpt);
int GetFntData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPFNT lpFnt, int cb, UINT flOpt);
int SetFntData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPFNT lpFnt, int cb, UINT flOpt);
int GetWinData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPWIN lpWin, int cb, UINT flOpt);
int SetWinData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPWIN lpWin, int cb, UINT flOpt);
int GetEnvData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPENV lpEnv, int cb, UINT flOpt);
int SetEnvData(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPENV lpEnv, int cb, UINT flOpt);
#ifdef WINNT
int GetNt31Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT31 lpNt31, int cb, UINT flOpt);
int SetNt31Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT31 lpNt31, int cb, UINT flOpt);
#endif
#ifdef UNICODE
int GetNt40Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT40 lpNt40, int cb, UINT flOpt);
int SetNt40Data(PPROPLINK ppl, DATAPTRS aDataPtrs, LPPROPNT40 lpNt40, int cb, UINT flOpt);
#endif


void CopyIniWordsToFntData(LPPROPFNT lpFnt, LPINIINFO lpii, int cWords);
void CopyIniWordsToWinData(LPPROPWIN lpWin, LPINIINFO lpii, int cWords);

// These could be defined as WINAPI if we ever wanted to export them back to WinOldAp

WORD GetIniWords(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPWORD lpwBuf, WORD cwBuf, LPCTSTR lpszFilename);
WORD ParseIniWords(LPCTSTR lpsz, LPWORD lpwBuf, WORD cwBuf, LPTSTR *lplpsz);
BOOL WriteIniWords(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCWORD lpwBuf, WORD cwBuf, LPCTSTR lpszFilename);

// piflib.c

BOOL LoadGlobalEditData(void);
void FreeGlobalEditData(void);
void InitRealModeFlag(PPROPLINK ppl);

// pifprg.c

BOOL_PTR CALLBACK DlgPrgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef UNICODE
HICON LoadPIFIcon(LPPROPPRG lpprg, LPPROPNT40 lpnt40);
#else
HICON LoadPIFIcon(LPPROPPRG lpprg);
#endif

// pifvid.c

BOOL_PTR CALLBACK DlgVidProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// pifmem.c

BOOL_PTR CALLBACK DlgMemProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// pifmsc.c

BOOL_PTR CALLBACK DlgMscProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// pifhot.c
WORD HotKeyWindowsFromOem(LPCPIFKEY lppifkey);
void HotKeyOemFromWindows(LPPIFKEY lppifkey, WORD wHotKey);

// pifsub.c

void  lstrcpytrimA(LPSTR lpszDst, LPCSTR lpszSrc, int cbMax);
void  lstrcpypadA(LPSTR lpszDst, LPCSTR lpszSrc, int cbMax);
int   lstrcpyncharA(LPSTR lpszDst, LPCSTR lpszSrc, int cbMax, CHAR ch);
int   lstrskipcharA(LPCSTR lpszSrc, CHAR ch);
int   lstrskiptocharA(LPCSTR lpszSrc, CHAR ch);
LPSTR lstrrchrA(LPCSTR lpszSrc, CHAR ch);
#ifdef UNICODE
LPWSTR  lstrrchrW(LPCWSTR lpszSrc, WCHAR ch);
int     _fatoiW(LPCWSTR lpszw);
#endif
int lstrcpyfnameA(LPSTR lpszDst, LPCSTR lpszSrc, int cbMax);
int lstrunquotefnameA(LPSTR lpszDst, LPCSTR lpszSrc, int cbMax, BOOL fShort);
int lstrskipfnameA(LPCSTR lpszSrc);
#define _fatoiA atoi

#ifdef UNICODE

#define _fatoi _fatoiW
#define lstrrchr lstrrchrW

#else

#define _fatoi atoi
#define lstrrchr  lstrrchrA

#endif  // UNICODE

int cdecl Warning(HWND hwnd, WORD id, WORD type, ...);
int MemoryWarning(HWND hwnd);
LPTSTR LoadStringSafe(HWND hwnd, UINT id, LPTSTR lpsz, int cbsz);
void SetDlgBits(HWND hDlg, PBINF pbinf, UINT cbinf, WORD wFlags);
void GetDlgBits(HWND hDlg, PBINF pbinf, UINT cbinf, LPWORD lpwFlags);
void SetDlgInts(HWND hDlg, PVINF pvinf, UINT cvinf, LPVOID lp);
void AddDlgIntValues(HWND hDlg, int id, int iMax);
void GetDlgInts(HWND hDlg, PVINF pvinf, int cvinf, LPVOID lp);
BOOL ValidateDlgInts(HWND hDlg, PVINF pvinf, int cvinf);

void LimitDlgItemText(HWND hDlg, int iCtl, UINT uiLimit);
void SetDlgItemPct(HWND hDlg, int iCtl, UINT uiPct);
UINT GetDlgItemPct(HWND hDlg, int iCtl);
void SetDlgItemPosRange(HWND hDlg, int iCtl, UINT uiPos, DWORD dwRange);
UINT GetDlgItemPos(HWND hDlg, int iCtl);
void EnableDlgItems(HWND hDlg, PBINF pbinf, int cbinf, BOOL fEnable);
void DisableDlgItems(HWND hDlg, PBINF pbinf, int cbinf);
BOOL AdjustRealModeControls(PPROPLINK ppl, HWND hDlg);
void BrowsePrograms(HWND hDlg, UINT uiCtl, UINT uiCwd);
void OnWmHelp(LPARAM lparam, const DWORD *pdwHelp);
void OnWmContextMenu(WPARAM wparam, const DWORD *pdwHelp);
#ifdef UNICODE
void PifMgr_WCtoMBPath( LPWSTR lpUniPath, LPSTR lpAnsiPath, UINT cchBuf );
#endif

void PifMgrDLL_Init();
void PifMgrDLL_Term();

#ifdef  DEBUG
void DebugASSERT(TCHAR *pszModule, int line);
#endif

extern TCHAR   *pszNoMemory;

extern CHAR szRasterFaceName[];

#endif /* RC_INVOKED */
