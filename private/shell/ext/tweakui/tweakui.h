/*
 *  tweakui.h
 */

/* WHEN CHANGING PRERELEASE - Rebuild expire.c, change tweakui.rcv */
/* #define PRERELEASE 6		/* REMOVE THIS FOR FINAL BUILD */

#define STRICT
#ifndef WIN32_LEAN_AND_MEAN     /* build.exe will define it for us on NT */
#define WIN32_LEAN_AND_MEAN
#endif
#define WINVER 0x0400		/* Windows 95 compatible */
#define _WIN32_WINDOWS  0x0400  /* Windows 95 compatible */
#include <windows.h>		/* Everybody's favourite */

#ifndef RC_INVOKED
#include <windowsx.h>		/* Message crackers */
typedef const BYTE FAR *LPCBYTE; /* Mysteriously missing from Win32 */
#include <shellapi.h>
#include <shlobj.h>
#include <regstr.h>		/* REGSTR_PATH_DESKTOP */
#include <commdlg.h>
#include <cpl.h>		/* Control panel interface */
#include <shlwapi.h>		/* DLLVERSIONINFO */
#endif				/* !RC_INVOKED */

#include <commctrl.h>		/* For trackbar and imagelist */
#include <prsht.h>		/* Property sheet stuff */

#ifdef WINNT
#include <winuserp.h>
#endif
#undef IDH_OK
#undef IDH_CANCEL
#include "tweakhlp.h"
#include "strings.h"

/*
 * DIDC - delta in IDC
 *
 *  didcEdit - the edit control in charge
 *  didcUd   - the updown control that is associated with it
 */

#define didcEdit 0
#define didcUd   1

/*
 * Dialog boxes and controls.
 */
#define IDD_GENERAL	1
#define IDD_EXPLORER	2
#define IDD_DESKTOP	3
#define IDD_TOOLS	4
#define IDD_ADDREMOVE	5
#define IDD_BOOT	6
#define IDD_REPAIR	7
#define IDD_PARANOIA	8
#define IDD_MYCOMP	9
#define IDD_MOUSE	10
#define IDD_NETWORK	11
#define IDD_IE4         12
#define IDD_CONTROL     13

#define IDD_PICKICON	32
#define IDD_UNINSTALLEDIT 33
#define IDD_SEARCHURL	34

#ifdef	PRERELEASE
#define IDD_BETAPASSWORD 64
#endif

/*
 * Common to multiple
 */
#define IDC_WHATSTHIS  26       /* Display context help */
#define IDC_LVDELETE   27	/* Items can be deleted from LV */
#define IDC_LVTOGGLE   28	/* Toggle the state of the current LV item */
#define IDC_LVRENAME   29	/* Renaming the current LV item */
#define IDC_LISTVIEW   30	/* The listview goes here */
#define IDC_RESET      31

/*
 * IDD_MOUSE
 */

#define IDC_SPEEDTEXT   32
#define IDC_SPEEDFAST   33
#define IDC_SPEEDSLOW   34
#define IDC_SPEEDTRACK  35
#define IDC_SPEEDHELP   36

#define IDC_SENSGROUP	37
#define IDC_DBLCLKTEXT  38
#define IDC_DBLCLK      39
#define IDC_DBLCLKUD    40		/* IDC_DBLCLK + didcUd */
#define IDC_DRAGTEXT    41
#define IDC_DRAG        42
#define IDC_DRAGUD      43		/* IDC_DRAG + didcUd */
#define IDC_SENSHELP    44

#define IDC_XMOUSE      45

#define IDC_TESTGROUP	46
#define IDC_TEST        47
#define IDC_TIPS        48

#define IDC_WHEELFIRST	49
#define IDC_WHEELGROUP	49
#define IDC_WHEELENABLE	50
#define IDC_WHEELPAGE	51
#define IDC_WHEELLINE	52
#define IDC_WHEELLINENO	53
#define IDC_WHEELLINEUD	54		/* IDC_WHEELLINENO + didcUd */
#define IDC_WHEELLINESTXT 55
#define IDC_WHEELLAST	55

//#define IDC_BUGREPORT   56

/*
 * IDD_GENERAL
 */

#define IDC_FLDGROUP    32
#define IDC_FLDNAMETXT  33
#define IDC_FLDNAMELIST 34
#define IDC_FLDLOCTXT   35
#define IDC_FLDLOC      36
#define IDC_FLDCHG      37

#define IDC_EFFECTGROUP 38
#define IDC_ANIMATE     39
#define IDC_SMOOTHSCROLL 40
#define IDC_BEEP        41


#define IDC_IE3FIRST	42
#define IDC_IE3GROUP	42
#define IDC_IE3TXT	43
#define IDC_IE3ENGINETXT 44
#define IDC_IE3ENGINE	45
#define IDC_IE3LAST	45

/*
 * IDD_EXPLORER
 */

#define IDC_LINKGROUP	32
#define IDC_LINKFIRST	33
#define IDC_LINKARROW	33		/* These need to be adjacent */
#define IDC_LIGHTARROW	34
#define IDC_NOARROW	35
#define IDC_CUSTOMARROW 36

#define IDC_LINKBEFORETEXT 37
#define IDC_LINKBEFORE	38
#define IDC_LINKAFTERTEXT 39
#define IDC_LINKAFTER   40
#define IDC_LINKHELP    41

#define IDC_STARTUPGROUP 42
#define IDC_BANNER      43
#define IDC_WELCOME     44

#define IDC_SETGROUP	45
#define IDC_PREFIX      46
#define IDC_EXITSAVE    47
#define IDC_MAKEPRETTY  48
#define IDC_COMPRESSTXT 49
#define IDC_COMPRESSCLR 50
#define IDC_COMPRESSBTN 51

/*
 * IDD_DESKTOP
 */

#define IDC_ICONLV	IDC_LISTVIEW
#define IDC_ICONLVTEXT	32
#define IDC_ICONLVTEXT2	33
#define IDC_UNUSED34	34
#define IDC_CREATENOWTEXT 35
#define IDC_CREATENOW	36
#define IDC_SHOWONDESKTOP 37
#if 0
#define IDC_RENAMELV	38
#endif

/*
 * IDD_NETWORK
 */
#define IDC_LOGONGROUP		32
#define IDC_LOGONAUTO		33
#define IDC_LOGONUSERTXT	34
#define IDC_LOGONUSER   	35
#define IDC_LOGONPASSTXT	36
#define IDC_LOGONPASS		37

/*
 * IDD_TOOLS
 */

#define IDC_TEMPLATE	IDC_LISTVIEW
#define IDC_TEMPLATETEXT 33

/*
 * IDD_ADDREMOVE
 */
#define IDC_UNINSTALL	IDC_LISTVIEW
#define IDC_UNINSTALLTEXT 32
#define IDC_UNINSTALLEDIT 33
#define IDC_UNINSTALLNEW 35
#define IDC_UNINSTALLCHECK      36

/*
 * IDD_BOOT
 */

#define IDC_BOOTGROUP1	32
#define	IDC_BOOTKEYS	33
#define	IDC_BOOTDELAYTEXT 34
#define	IDC_BOOTDELAY   35
#define	IDC_BOOTDELAYUD	36
#define	IDC_BOOTDELAYTEXT2 37
#define	IDC_BOOTGUI	38
#define	IDC_LOGO	39
#define	IDC_BOOTMULTI	40

#define IDC_BOOTMENUGROUP 41
#define	IDC_BOOTMENU	42

//#define IDC_BOOTMENUDEFAULT xx
#define	IDC_BOOTMENUDELAYTEXT 43
#define	IDC_BOOTMENUDELAY 44
#define	IDC_BOOTMENUDELAYUD 45
#define	IDC_BOOTMENUDELAYTEXT2 47

#define IDC_SCANDISK	48
#define IDC_SCANDISKTEXT 49


/*
 * IDD_REPAIR
 */
#define IDC_REPAIRCOMBO         64
#define IDC_REPAIRNOW           65
#define IDC_REPAIRHELP          66
#if 1
#define IDC_REBUILDCACHEICON	32
#define IDC_REBUILDCACHE	33
#define IDC_REPAIRFONTFLDICON	34
#define IDC_REPAIRFONTFLD	35
#define IDC_REPAIRDLLSICON	36
#define IDC_REPAIRDLLS		37
#define IDC_REPAIRADDRMICON     38
#define IDC_REPAIRADDRM         39
#define IDC_REPAIR95ONLYMIN	IDC_REPAIRREGEDITICON
#define IDC_REPAIRREGEDITICON   40
#define IDC_REPAIRREGEDIT       41
#define IDC_REPAIRASSOCICON     42
#define IDC_REPAIRASSOC         43
#define IDC_REPAIR95ONLYMAX     44
#endif

/*
 * IDD_PARANOIA
 */
#define IDC_CLEARGROUP		32
#define IDC_CLEARNOW		37
#define IDC_CDROMGROUP		38
#define	IDC_CDROMAUDIO		39
#define IDC_CDROMDATA		40
#define IDC_PARANOIA95ONLYMIN	IDC_FAULTLOGGROUP
#define IDC_FAULTLOGGROUP	41
#define IDC_FAULTLOG		42
#define IDC_PARANOIA95ONLYMAX	43

/*
 * IDD_IE4
 */
#define IDC_SETTINGSGROUP       32

/*
 * IDD_PICKICON
 */
#define IDC_PICKPATH	32
#define IDC_PICKICON	33
#define IDC_PICKBROWSE	34

/*
 * IDD_UNINSTALLEDIT
 */
#define IDC_UNINSTALLDESCTEXT	32
#define IDC_UNINSTALLDESC	33
#define IDC_UNINSTALLCMDTEXT	34
#define IDC_UNINSTALLCMD	35

/*
 * IDD_SEARCHURL
 */
#define IDC_SEARCHURL	        32

/*
 * Menus
 */
#define IDM_MAIN        1

/*
 * Bitmaps
 */
#define IDB_CHECK       1

/*
 * Strings
 */
#define IDS_NAME	1
#define IDS_DESCRIPTION	2

#define IDS_ALLFILES	3

#define IDS_BADEXT	4
#define IDS_COPYFAIL	5
#define IDS_REGFAIL	6
#define IDS_CONFIRMNEWTEMPLATE 7
#define IDS_TOOMANY	8
#define IDS_CANNOTTEMPLATE 9

#define IDS_NETHOOD    10
#define IDS_LOGONOFF   11

#define IDS_ERRMSDOSSYS	12

/*
 *  Scandisk strings.
 */
#define IDS_SCANDISKFIRST   13
#define IDS_SCANDISKNEVER   13
#define IDS_SCANDISKPROMPT  14
#define IDS_SCANDISKALWAYS  15

#ifdef BOOTMENUDEFAULT
#define IDS_BOOTMENU	    xx
#define IDS_BOOTMENULOGGED  14
#define IDS_BOOTMENUSAFE    15
#define IDS_BOOTMENUSAFENET 16
#define IDS_BOOTMENUSTEP    17
#define IDS_BOOTMENUCMD	    18
#define IDS_BOOTMENUSAFECMD 19
#define IDS_BOOTMENUPREV    20
#define IDS_BOOTMENULAST    20
#endif

#define IDS_DESKTOPRESETOK	21
#define IDS_ICONFILES		22

#define IDS_NOTHINGTOCLEAR  	23

#define IDS_BADRUN  		24
#define IDS_CANTINSTALL		25
#define IDS_NONETHOOD  		26
#define IDS_ADDRMWARN  		27
#define IDS_FIXED  		28
#define IDS_TEMPLATEDELETEWARN  29
#define IDS_MAYBEREBOOT  	30
#define IDS_ICONSREBUILT  	31
#define IDS_RESTRICTED  	32
#define IDS_ASKREPAIRADDRM      33

#define IDS_FOLDER_PATTERN	95

/* WARNING!  These must match the CSIDL values! */
#define IDS_FOLDER_BASE		96
#define IDS_FOLDER_DESKTOP	(IDS_FOLDER_BASE + 0x0010)
#define IDS_FOLDER_PROGRAMS	(IDS_FOLDER_BASE + 0x0002)
#define IDS_FOLDER_PERSONAL	(IDS_FOLDER_BASE + 0x0005)
#define IDS_FOLDER_FAVORITES	(IDS_FOLDER_BASE + 0x0006)
#define IDS_FOLDER_STARTUP	(IDS_FOLDER_BASE + 0x0007)
#define IDS_FOLDER_RECENT	(IDS_FOLDER_BASE + 0x0008)
#define IDS_FOLDER_SENDTO	(IDS_FOLDER_BASE + 0x0009)
#define IDS_FOLDER_STARTMENU	(IDS_FOLDER_BASE + 0x000B)
#define IDS_FOLDER_TEMPLATES	(IDS_FOLDER_BASE + 0x0015)
#define IDS_FOLDER_PROGRAMFILES (IDS_FOLDER_BASE + 0x0016)
#define IDS_FOLDER_COMMONFILES  (IDS_FOLDER_BASE + 0x0017)
#define IDS_FOLDER_SOURCEPATH   (IDS_FOLDER_BASE + 0x0018)

#define IDS_DEFAULT_BASE	128
#define IDS_DEFAULT_DESKTOP	(IDS_DEFAULT_BASE + 0x0010)
#define IDS_DEFAULT_PROGRAMS	(IDS_DEFAULT_BASE + 0x0002)
#define IDS_DEFAULT_PERSONAL	(IDS_DEFAULT_BASE + 0x0005)
#define IDS_DEFAULT_FAVORITES	(IDS_DEFAULT_BASE + 0x0006)
#define IDS_DEFAULT_STARTUP	(IDS_DEFAULT_BASE + 0x0007)
#define IDS_DEFAULT_RECENT	(IDS_DEFAULT_BASE + 0x0008)
#define IDS_DEFAULT_SENDTO	(IDS_DEFAULT_BASE + 0x0009)
#define IDS_DEFAULT_STARTMENU	(IDS_DEFAULT_BASE + 0x000B)
#define IDS_DEFAULT_TEMPLATES	(IDS_DEFAULT_BASE + 0x0015)

#define IDS_ENGINE		160	/* 160 .. 191 */
#define IDS_URL			192	/* 192 .. 223 */

#define IDS_PARANOIA            224     /* 224 .. 239 */

#define IDS_IE4                 240     /* 240 .. 255 */

#define IDS_GENERALEFFECTS      256     /* 256 .. 287 */

#define IDS_CPL_BORING          288     /* 288 .. 319 */
#define IDS_CPL_ADDRM           289     /* Add/Remove Programs */
#define IDS_CPL_DESK            290     /* Desktop settings */
#define IDS_CPL_INTL            291     /* Regional settings */
#define IDS_CPL_MAIN            292     /* Mouse, Keyboard, PCMCIA, etc. */
#define IDS_CPL_TIMEDATE        293     /* Time/Date */

#define IDS_REPAIR              320     /* 320 .. 351 */
#define IDS_REPAIRHELP          352     /* 352 .. 383 */

/*
 * Icons
 */
#define IDI_DEFAULT	1
#define IDI_GEAR1	2
#define IDI_ALTLINK	3	/* This must remain number 3 */
#define IDI_BLANK	4	/* This must remain number 4 */

#define IDI_GEAR2	5

#define IDI_ICONS       6
#define IDI_FONTFOLDER	7
#define IDI_REGEDIT	8
#define IDI_DLL		9
#define IDI_ASSOC      10

/*
 * Binary
 */
#define IDX_SETUP       1	/* Tiny Win16 setup.exe */
#define IDX_FONTFOLDER  2       /* desktop.ini for Font Folder */
#define IDX_TEMPINET    3       /* desktop.ini for Temporary Internet Files */
#define IDX_HISTORY     4       /* desktop.ini for History */
#define IDX_RECYCLE     5       /* desktop.ini for Recycle Bin */

#ifndef RC_INVOKED

/*****************************************************************************
 *
 *  Thinks that depend on which header files you are using.
 *
 *****************************************************************************/

#ifndef SM_MOUSEWHEELPRESENT

#define SM_MOUSEWHEELPRESENT	75

#define SPI_GETWHEELSCROLLLINES	104
#define SPI_SETWHEELSCROLLLINES	105

#endif

#ifndef SPI_GETMENUSHOWDELAY
#define SPI_GETMENUSHOWDELAY	106
#define SPI_SETMENUSHOWDELAY	107
#endif

#ifndef SPI_GETUSERPREFERENCE
#define SPI_GETUSERPREFERENCE   108
#define SPI_SETUSERPREFERENCE   109
#define SPI_UP_ACTIVEWINDOWTRACKING 0
#endif

#ifndef SPI_GETACTIVEWINDOWTRACKING
#define SPI_GETACTIVEWINDOWTRACKING 0x10000000
#define SPI_SETACTIVEWINDOWTRACKING 0x10000001
#define SPI_GETMENUANIMATION                0x10000002
#define SPI_SETMENUANIMATION                0x10000003
#define SPI_GETCOMBOBOXANIMATION            0x10000004
#define SPI_SETCOMBOBOXANIMATION            0x10000005
#define SPI_GETLISTBOXSMOOTHSCROLLING       0x10000006
#define SPI_SETLISTBOXSMOOTHSCROLLING       0x10000007
#define SPI_GETGRADIENTCAPTIONS             0x10000008
#define SPI_SETGRADIENTCAPTIONS             0x10000009
#define SPI_GETMENUUNDERLINES               0x1000000A
#define SPI_SETMENUUNDERLINES               0x1000000B
#define SPI_GETACTIVEWNDTRKZORDER           0x1000000C
#define SPI_SETACTIVEWNDTRKZORDER           0x1000000D
#define SPI_GETHOTTRACKING                  0x1000000E
#define SPI_SETHOTTRACKING                  0x1000000F
#define SPI_GETFOREGROUNDLOCKTIMEOUT        0x20000000
#define SPI_SETFOREGROUNDLOCKTIMEOUT        0x20000001
#endif


#ifndef WHEEL_PAGESCROLL
#define WHEEL_PAGESCROLL	((UINT)-1)
#endif

#ifndef UINT_MAX
#define UINT_MAX                ((UINT)-1)
#endif

/*****************************************************************************
 *
 *  I'm getting tired of writing these out the long way.
 *
 *****************************************************************************/

typedef struct _ITEMIDLIST FAR *PIDL;
typedef LPSHELLFOLDER PSF;
typedef HIMAGELIST HIML;
typedef TCHAR TCH;
typedef const TCH *PCTSTR;
typedef LPVOID PV, *PPV;
typedef LPUNKNOWN PUNK;

#define hkLM HKEY_LOCAL_MACHINE
#define hkCU HKEY_CURRENT_USER
#define hkCR HKEY_CLASSES_ROOT

/*****************************************************************************
 *
 *  Goofy macros
 *
 *****************************************************************************/

#ifdef WIN32
#define EXPORT
#define CODESEG
#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")
#else
#define EXPORT    _loadds _far _pascal
#define CODESEG __based(__segname("_TEXT"))
#define BEGIN_CONST_DATA
#define END_CONST_DATA
#endif
#define INLINE static __inline

#define ConstString(nm, v) const TCH CODESEG nm[] = TEXT(v)
#define ConstString2(nm, v1, v2) const TCH CODESEG nm[] = TEXT(v1) TEXT(v2)
#define ExternConstString(nm) extern const TCH CODESEG nm[]

/*****************************************************************************
 *
 *  Baggage I carry everywhere
 *
 *****************************************************************************/

#define cbX(x) sizeof(x)
#define cA(a) (sizeof(a)/sizeof(a[0]))

#define cbCtch(ctch) ((ctch) * sizeof(TCH))

/*
 * lor -- Logical or.  Evaluate the first.  If the first is nonzero,
 * return it.  Otherwise, return the second.
 *
 * Unfortunately, due to the stupidity of the C language, this can
 * be implemented only with a GNU extension.  In the non-GNU case,
 * we return 1 if the first is nonzero.
 */

#if defined(__GNUC__)
#define fLorFF(f1, f2) ({ typeof (f1) _f = f1; if (!_f) _f = f2; _f; })
#else
#define fLorFF(f1, f2) ((f1) ? 1 : (f2))
#endif

/*
 * limp - logical implication.  True unless the first is nonzero and
 * the second is zero.
 */
#define fLimpFF(f1, f2) fLorFF(!(f1), f2)

/*****************************************************************************
 *
 *  Common dialog wrappers
 *
 *****************************************************************************/

typedef struct COFN {		/* common open file name */
    OPENFILENAME ofn;		/* The thing COMMDLG wants */
    TCH tsz[MAX_PATH];		/* Where we build the name */
    TCH tszFilter[100];		/* File open/save filter */
} COFN, *PCOFN;

void PASCAL InitOpenFileName(HWND hwnd, PCOFN pcofn, UINT ids, LPCSTR pszInit);

/*****************************************************************************
 *
 *  Private stuff
 *
 *****************************************************************************/

#define iErr (-1)
int PASCAL iFromPtsz(LPTSTR ptsz);
LPTSTR PASCAL ptszStrChr(PCTSTR ptsz, TCH tch);
PTSTR PASCAL ptszStrRChr(PCTSTR ptsz, TCH tch);

int PASCAL ParseIconSpec(LPTSTR ptszSrc);

LPTSTR PASCAL ptszFilenameCqn(PCTSTR cqn);

HRESULT PASCAL Ole_Init(void);
void PASCAL Ole_Term(void);

void PASCAL Ole_Free(LPVOID pv);
void PASCAL Ole_Release(LPVOID pv);
BOOL PASCAL Ole_ClsidFromString(LPCTSTR lpsz, LPCLSID pclsid);

#ifdef	UNICODE
#define UnicodeFromPtsz(wsz, ptsz) LPWSTR wsz = ptsz
#else	/* ANSI */
int PASCAL Ole_ToUnicode(LPOLESTR lpos, LPCSTR psz);
#define UnicodeFromPtsz(wsz, ptsz) WCHAR wsz[MAX_PATH]; Ole_ToUnicode(wsz, ptsz)
#endif

void PASCAL SetRestriction(LPCTSTR ptszKey, BOOL f);
BOOL PASCAL GetRestriction(LPCTSTR ptszKey);

PIDL PASCAL pidlFromPath(PSF psf, LPCSTR cqn);
HRESULT PASCAL SetNameOfPidl(PSF psf, PIDL pidl, LPCSTR pszName);
HRESULT PASCAL ComparePidls(PIDL pidl1, PIDL pidl2);
HIML PASCAL GetSystemImageList(DWORD dw);

BOOL PASCAL WithPidl(PSF psf, LPCSTR lqn, BOOL (*pfn)(PIDL, LPVOID), LPVOID pv);
BOOL PASCAL WithPsf(PSF psf, PIDL pidl, BOOL (*pfn)(PSF, LPVOID), LPVOID pv);
BOOL PASCAL EmptyDirectory(LPCSTR pszDir, BOOL (*pfn)(LPCSTR, LPVOID), PV pv);
BOOL PASCAL WithTempDirectory(BOOL (*pfn)(LPCSTR, LPVOID), LPVOID pv);

BOOL EXPORT General_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Mouse_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Explorer_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Desktop_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT MyComp_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Network_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Template_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT AddRm_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Boot_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Repair_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Paranoia_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT IE4_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);
BOOL EXPORT Control_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);

void PASCAL Boot_FindMsdosSys(void);

BOOL PASCAL fCreateNil(LPCSTR cqn);
BOOL PASCAL Link_GetShortcutTo(void);
BOOL PASCAL Link_SetShortcutTo(BOOL f);
void PASCAL Explorer_HackPtui(void);
UINT PASCAL Explorer_GetIconSpecFromRegistry(LPTSTR ptszBuf);
void PASCAL Paranoia_CoverTracks(void);

int PickIcon(HWND, LPSTR, UINT, int *);

int PASCAL MessageBoxId(HWND hwnd, UINT id, LPCSTR pszTitle, UINT fl);

PTSTR PASCAL TweakUi_TrimTrailingBs(LPTSTR ptsz);

typedef BOOL (PASCAL *WITHPROC)(LPVOID pv, LPVOID pvRef);
BOOL PASCAL WithSelector(DWORD lib, UINT cb, WITHPROC wp, LPVOID pvRef,
			 BOOL fWrite);

HICON PASCAL LoadIconId(UINT id);
void PASCAL SafeDestroyIcon(HICON hicon);

/*****************************************************************************
 *
 *  common.c
 *
 *  Common property sheet handling.
 *
 *****************************************************************************/

void PASCAL Common_SetDirty(HWND hdlg);
#define Common_SetClean(hdlg)
void PASCAL Common_NeedLogoff(HWND hdlg);
void PASCAL Common_OnHelp(LPARAM lp, const DWORD CODESEG *pdwHelp);
void PASCAL Common_OnContextMenu(WPARAM wparam, const DWORD CODESEG *pdwHelp);

typedef HMENU (PASCAL *CMCALLBACK)(HWND hwnd, int iItem);
void PASCAL Common_LV_OnContextMenu(HWND hdlg, HWND hwnd, LPARAM lp,
		        CMCALLBACK pfn, const DWORD CODESEG *pdwHelp);

typedef struct LVCI {		/* common listview command info */
    int	id;			/* command identifier */
    void (PASCAL *pfn)(HWND hwnd, int iItem); /* command handler */
} LVCI, *PLVCI;

void PASCAL Common_OnLvCommand(HWND hdlg, int idCmd, PLVCI rglvci, int clvci);

void NEAR PASCAL EnableDlgItem(HWND hdlg, UINT idc, BOOL f);
void NEAR PASCAL
SetDlgItemTextLimit(HWND hdlg, UINT id, LPCTSTR ptsz, UINT ctch);

/*****************************************************************************
 *
 *  misc.c
 *
 *  gxa stuff
 *
 *****************************************************************************/

typedef struct {
    PVOID   pv;
    int	    cx;
    int	    cxAlloc;
    int	    cbx;
} GXA, *PGXA;

#define Declare_Gxa(T, t)					\
    union {							\
	GXA gxa;						\
	struct { P##T p##t; int c##t;				\
	         int c##t##Alloc; int cb##t; };			\
    }								\

PVOID PASCAL Misc_AllocPx(PGXA pgxa);
BOOL PASCAL Misc_InitPgxa(PGXA pgxa, int cbx);
void PASCAL Misc_FreePgxa(PGXA pgxa);

LPTSTR PASCAL Misc_Trim(LPTSTR ptsz);
BOOL PASCAL
Misc_CopyReg(HKEY hkSrcRoot, PCTSTR ptszSrc, HKEY hkDstRoot, PCTSTR ptszDst);

BOOL PASCAL
Misc_RenameReg(HKEY hkRoot, PCTSTR ptszKey, PCTSTR ptszSrc, PCTSTR ptszDst);

void PASCAL lstrcatnBs(PTSTR ptszDst, PCTSTR ptszSrc, int ctch);
#define lstrcatnBsA(a, ptsz) lstrcatnBs(a, ptsz, cA(a))

UINT PASCAL Misc_GetShellIconSize(void);
void PASCAL Misc_SetShellIconSize(UINT ui);
void PASCAL Misc_RebuildIcoCache(void);

void PASCAL Misc_EnableMenuFromHdlgId(HMENU hmenu, HWND hdlg, UINT idc);

/*****************************************************************************
 *
 *  misc.c
 *
 *  Listview stuff
 *
 *****************************************************************************/

void PASCAL Misc_LV_Init(HWND hwnd);
int PASCAL Misc_LV_GetCurSel(HWND hwnd);
void PASCAL Misc_LV_SetCurSel(HWND hwnd, int iIndex);
void PASCAL Misc_LV_EnsureSel(HWND hwnd, int iItem);
void PASCAL Misc_LV_GetItemInfo(HWND, LV_ITEM FAR *, int, UINT);
LPARAM PASCAL Misc_LV_GetParam(HWND hwnd, int iItem);
void PASCAL Misc_LV_HitTest(HWND hwnd, LV_HITTESTINFO FAR *phti, LPARAM lpPos);

/*****************************************************************************
 *
 *  lv.c
 *
 *  Listview property sheet page manager
 *
 *****************************************************************************/

typedef UINT LVVFL;
#define lvvflIcons      1               /* Show icons */
#define lvvflCanCheck   2               /* Show check-box */
#define lvvflCanRename  4               /* Can rename by clicking */
#define lvvflCanDelete  8               /* Parent dialog has IDC_LVDELETE */

typedef struct LVV {
    void (PASCAL *OnCommand)(HWND hdlg, int id, UINT codeNotify);
    void (PASCAL *OnInitContextMenu)(HWND hwnd, int iItem, HMENU hmenu);
    void (PASCAL *Dirtify)(LPARAM lp);
    int (PASCAL *GetIcon)(LPARAM lp);
    BOOL (PASCAL *OnInitDialog)(HWND hwnd);
    void (PASCAL *OnApply)(HWND hdlg);
    void (PASCAL *OnDestroy)(HWND hdlg);
    void (PASCAL *OnSelChange)(HWND hwnd, int iItem);
    int iMenu;
    const DWORD CODESEG *pdwHelp;
    UINT idDblClk;
    LVVFL lvvfl;
    LVCI rglvci[0];
} LVV, *PLVV;

int PASCAL
LV_AddItem(HWND hwnd, int ix, LPCTSTR ptszDesc, int iImage, BOOL fState);

BOOL EXPORT
LV_DlgProc(PLVV plvv, HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);

/* State icon information */

#define isiNil		0		/* No state image */
#define isiUnchecked	1		/* Not checked */
#define isiChecked	2		/* Checked */

#ifndef	STATEIMAGEMASKTOINDEX
#define	STATEIMAGEMASKTOINDEX(i) ((i & LVIS_STATEIMAGEMASK) >> 12)
#endif
#define isiPlvi(plvi) STATEIMAGEMASKTOINDEX((plvi)->state)

/*  Checklist_OnApply assumes this returns exactly 0 or 1 */
#define LV_IsChecked(plvi) (isiPlvi(plvi) == isiChecked)

/*****************************************************************************
 *
 *  lvchk.c
 *
 *  Check-listview property sheet page manager
 *
 *****************************************************************************/

/*
 *  GetCheckValue must return exactly one of these values
 *
 *      -1  not supported on this platform
 *       0  off
 *       1  on
 *
 *  Yes, you must explicitly return 1 for on.  The reason is that if
 *  I let you return arbitrary nonzero values, you might accidentally
 *  return -1 and that would be bad.
 */
typedef BOOL (PASCAL *GETCHECKVALUE)(LPARAM lParam, LPVOID pvRef);
typedef void (PASCAL *SETCHECKVALUE)(BOOL fCheck, LPARAM lParam, LPVOID pvRef);

typedef struct CHECKLISTITEM {
    GETCHECKVALUE   GetCheckValue;
    SETCHECKVALUE   SetCheckValue;
    LPARAM          lParam;
} CHECKLISTITEM, *PCHECKLISTITEM;
typedef const CHECKLISTITEM *PCCHECKLISTITEM;

void PASCAL
Checklist_OnInitDialog(HWND hwnd, PCCHECKLISTITEM rgcli, int ccli,
                       UINT ids, LPVOID pvRef);
void PASCAL Checklist_OnApply(HWND hdlg, PCCHECKLISTITEM rgcli, LPVOID pvRef);

/*****************************************************************************
 *
 *  reg.c
 *
 *  Registry wrappers
 *
 *****************************************************************************/

typedef struct KL {			/* Key location */
    PHKEY phkRoot;			/* E.g., &hkCU, &pcdii->hlCUExplorer */
    PCTSTR ptszKey;			/* E.g., "Software\\Microsoft" */
    PCTSTR ptszSubkey;			/* E.g., "Show" */
} KL;

typedef const KL *PKL;

extern HKEY CODESEG c_hkCR;		/* HKEY_CLASSES_ROOT */
extern HKEY CODESEG c_hkCU;		/* HKEY_CURRENT_USER */
extern HKEY CODESEG c_hkLM;		/* HKEY_LOCAL_MACHINE */
extern HKEY g_hkLMSMWCV;		/* HKLM\Software\MS\Win\CurrentVer */
extern HKEY g_hkCUSMWCV;		/* HKCU\Software\MS\Win\CurrentVer */
extern HKEY g_hkLMSMIE;			/* HKLM\Software\MS\IE */
extern HKEY g_hkCUSMIE;			/* HKCU\Software\MS\IE */

/*
 * The NT version points to HKLM\Software\MS\Win NT\CurrentVersion if we are
 * running on NT; else, it's the same as g_hkLMSMWCV.
 */
extern HKEY g_hkLMSMWNTCV;		/* HKLM\Software\MS\Win( NT)?\CV */

#define phkCU (&c_hkCU)
#define phkLM (&c_hkLM)

typedef HKEY HHK;

#define hhkHkey(hkey) hkey
#define hkeyHhk(hhk)  hhk

#define hhkLM hhkHkey(HKEY_LOCAL_MACHINE)
#define hhkCU hhkHkey(HKEY_CURRENT_USER)
#define hhkCR hhkHkey(HKEY_CLASSES_ROOT)

/*
 *  Special wrappers that are NT-friendly.
 */
BOOL PASCAL RegCanModifyKey(HKEY hkRoot, LPCTSTR ptszSubkey);

LONG PASCAL _RegOpenKey(HKEY hk, LPCTSTR lptszSubKey, PHKEY phkResult);
#undef RegOpenKey
#define RegOpenKey _RegOpenKey

LONG PASCAL _RegCreateKey(HKEY hk, LPCTSTR lptszSubKey, PHKEY phkResult);
#undef RegCreateKey
#define RegCreateKey _RegCreateKey

void PASCAL RegDeleteValues(HKEY hkRoot, LPCTSTR ptszSubkey);
LONG PASCAL RegDeleteTree(HKEY hkRoot, LPCTSTR ptszSubkey);

HKEY PASCAL hkOpenClsid(PCTSTR ptszClsid);

BOOL PASCAL GetRegStr(HKEY hkey, LPCSTR pszKey, LPCTSTR ptszSubkey,
		      LPTSTR ptszBuf, int cbBuf);
BOOL PASCAL GetStrPkl(LPTSTR ptszBuf, int cbBuf, PKL pkl);

DWORD PASCAL GetRegDword(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dw);
DWORD PASCAL GetDwordPkl(PKL pkl, DWORD dw);

UINT PASCAL GetRegInt(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, UINT uiDefault);
UINT PASCAL GetIntPkl(UINT uiDefault, PKL pkl);

void PASCAL SetRegStr(HHK hhk, LPCTSTR ptszKey, LPCTSTR ptszSubkey,
		      LPCTSTR ptszVal);
void PASCAL SetStrPkl(PKL pkl, LPCTSTR ptszVal);
void PASCAL RegSetValuePtsz(HKEY hk, LPCTSTR ptszSubkey, LPCTSTR ptszVal);

void PASCAL SetRegInt(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, UINT ui);
void PASCAL SetIntPkl(UINT ui, PKL pkl);

void PASCAL SetRegDword(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dw);
void PASCAL SetDwordPkl(PKL pkl, DWORD dw);

void PASCAL DelPkl(PKL pkl);

/*****************************************************************************
 *
 *  Secret structure for NT shell
 *
 *****************************************************************************/

typedef struct {
    WORD cLength;
    WORD nVersion;

    BOOL fFullPathTitle            : 1;
    BOOL fSaveLocalView            : 1;
    BOOL fNotShell                 : 1;
    BOOL fSimpleDefault            : 1;
    BOOL fDontShowDescBar          : 1;
    BOOL fNewWindowMode            : 1;
    BOOL fShowCompColor            : 1;  /* Color compressed files */
    BOOL fDontPrettyNames          : 1;  /* 8.3 prettification */
    BOOL fAdminsCreateCommonGroups : 1;  /* Admins create common groups */
    UINT fUnusedFlags : 7;

    UINT fMenuEnumFilter;

} CABINETSTATE, * LPCABINETSTATE;

#define CABINETSTATE_VERSION 1

/*****************************************************************************
 *
 *  Manual import tables
 *
 *  Before expanding MIT_Contents, set MIT_Item either to
 *
 *	#define MIT_Item(a,b) a		// Emit the function type
 *  or	#define MIT_Item(a,b) b		// Emit the ordinal
 *
 *****************************************************************************/

#define MIT_Contents \
MIT_Item(BOOL (*SHGetNewLinkInfo)(LPCSTR, LPCSTR, LPSTR, BOOL *, UINT), 179) \
MIT_Item(int (*Shell_GetCachedImageIndex)(PV, int, UINT), 72) \
MIT_Item(BOOL (*ReadCabinetState)(LPCABINETSTATE, int), MAKELONG(651, 1)) \
MIT_Item(BOOL (*WriteCabinetState)(LPCABINETSTATE), MAKELONG(652, 1)) \
MIT_Item(HRESULT (*SHCoCreateInstance)(LPCSTR, const CLSID *, PUNK, \
				       REFIID, PPV), 102) \

#define MIT_Item(a, b) a;		/* Emit the prototype */

typedef struct MIT {			/* manual import table */
    MIT_Contents
} MIT, *PMIT;

#undef MIT_Item

extern MIT mit;

/*****************************************************************************
 *
 *  Globals
 *
 *****************************************************************************/

extern char g_tszName[80];		/* Program name goes here */
extern char g_tszMsdosSys[];		/* Boot file */

extern char g_tszPathShell32[MAX_PATH];	/* Full path to shell32.dll */
extern char g_tszPathMe[MAX_PATH];	/* Full path to myself */

extern PSF psfDesktop;			/* Desktop IShellFolder */

extern UINT g_flWeirdStuff;
#define flbsComCtl32		1	/* Buggy ComCtl32 */
#define flbsSmoothScroll	2	/* ComCtl32 supports smoothscroll */
#define flbsOPK2		4	/* Windows 95 OPK2 */
#define flbsNT			8	/* Windows NT */
#define flbsBadRun	       16	/* I was run not via Control Panel */
#define flbsShellSz	       32	/* Shell32 supports EXPAND_SZ */
#define flbsIE4                64       /* IE4 is installed */
#define flbsMemphis           128       /* Windows 95 Memphis */

#define g_fBuggyComCtl32	(g_flWeirdStuff & flbsComCtl32)
#define g_fSmoothScroll		(g_flWeirdStuff & flbsSmoothScroll)
#define g_fOPK2			(g_flWeirdStuff & flbsOPK2)
#define g_fNT			(g_flWeirdStuff & flbsNT)
#define g_fBadRun		(g_flWeirdStuff & flbsBadRun)
#define g_fShellSz		(g_flWeirdStuff & flbsShellSz)
#define g_fIE4                  (g_flWeirdStuff & flbsIE4)
#define g_fMemphis              (g_flWeirdStuff & flbsMemphis)

extern HINSTANCE hinstCur;

#define ctchClsid (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

#define ctchKeyMax (1+MAX_PATH)		/* According to the SDK */

/*****************************************************************************
 *
 *  Instanced
 *
 *  We're a cpl so have only one instance, but I declare
 *  all the instance stuff in one place so it's easy to convert this
 *  code to multiple-instance if ever we need to.
 *
 **************************************************************************/

typedef struct CDII {		/* common dialog instance info */
    HMENU hmenu;		/* The one menu everybody uses */
    HKEY  hkClsid;		/* HKCR\CLSID */
    HKEY  hkCUExplorer;		/* HKCU\REGSTR_PATH_EXPLORER */
    HKEY  hkLMExplorer;		/* HKLM\REGSTR_PATH_EXPLORER */
    HIML himlState;		/* Check-box state image imagelist */
    BOOL fRunShellInf;		/* Should I run shell.inf afterwards? */
    PIDL pidlTemplates;		/* Cached templates pidl */
} CDII, *PCDII;

extern CDII cdii;
#define pcdii (&cdii)

/**************************************************************************
 *
 * Local heap stuff
 *
 *  I have to do this because LocalAlloc takes the args in the order
 *  (fl, cb), but LocalReAlloc takes them in the order (cb, fl), and
 *  I invariably get them screwed up.
 *
 **************************************************************************/

#define lAlloc(cb)	(void *)LocalAlloc(LMEM_FIXED, cb)
#define lReAlloc(p, cb) (void *)LocalReAlloc((HLOCAL)(p), (cb), \
					     LMEM_MOVEABLE | LMEM_ZEROINIT)
#define lFree(p)	LocalFree((HLOCAL)(p))

/*****************************************************************************
 *
 *  Set an empty rect, centered at the requested point.
 *
 *****************************************************************************/

INLINE void
SetRectPoint(LPRECT lprc, POINT pt)
{
    lprc->left = lprc->right = pt.x;
    lprc->top = lprc->bottom = pt.y;
}

/*****************************************************************************
 *
 *  Who'd 'a thunk it?
 *
 *****************************************************************************/

LPVOID PASCAL MapSL(DWORD lib);

WINSHELLAPI BOOL PASCAL Shell_GetImageLists(HIMAGELIST FAR *, HIMAGELIST FAR *);

/*****************************************************************************
 *
 *  expire.c
 *
 *****************************************************************************/

#ifdef	PRERELEASE
BOOL PASCAL IsExpired(HWND hwnd);
#endif

#endif				/* !RC_INVOKED */
