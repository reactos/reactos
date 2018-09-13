#define STRICT
#define OEMRESOURCE

#define OVERRIDE_SHLWAPI_PATH_FUNCTIONS     // see comment in shsemip.h

#ifdef WINNT
#include <nt.h>         // Some of the NT specific code calls Rtl functions
#include <ntrtl.h>      // which requires all of these header files...
#include <nturtl.h>
#endif

// This stuff must run on Win95
#ifndef WINVER
#define WINVER              0x0400
#endif

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <ole2.h>
#include <shlobj.h>     // Includes <fcext.h>
#include <shsemip.h>
#include <shellapi.h>
#include <cpl.h>
#include <ddeml.h>

#ifdef UNICODE
#define CP_WINNATURAL   CP_WINUNICODE
#else
#define CP_WINNATURAL   CP_WINANSI
#endif

#define DISALLOW_Assert
#include <debug.h>          // our version of Assert etc.
#include <port32.h>
#include <heapaloc.h>
#include <shellp.h>
#include <ccstock.h>
#include <crtfree.h>
#include "apithk.h"
#include "svcs.h"
#include <shlobjp.h>
#include <shlwapi.h>
#include "dbt.h"

#ifdef __cplusplus
#include <shstr.h>
#endif

#include "uastrfnc.h"
#include "vdate.h"      // buffer validation (debug only)

#include "multimon.h"   // for multiple-monitor APIs on pre-Nashville OSes
#include <desktopp.h>

// Note that this "works" even if the main window is not visible, unlike
// IsWindowVisible
#define Cabinet_IsVisible(hwnd)  ((GetWindowStyle(hwnd) & WS_VISIBLE) == WS_VISIBLE)

//
// Trace/dump/break flags specific to explorer.
//   (Standard flags defined in shellp.h)
//

// Trace flags
#define TF_DDE              0x00000100      // DDE traces
#define TF_TARGETFRAME      0x00000200      // Target frame
#define TF_TRAYDOCK         0x00000400      // Tray dock
#define TF_TRAY             0x00000800      // Tray 

// "Olde names"
#define DM_DDETRACE         TF_DDE
#define DM_TARGETFRAME      TF_TARGETFRAME
#define DM_TRAYDOCK         TF_TRAYDOCK

// Function trace flags
#define FTF_DDE             0x00000001      // DDE functions
#define FTF_TARGETFRAME     0x00000002      // Target frame methods

// Dump flags
#define DF_DDE              0x00000001      // DDE package
#define DF_DELAYLOADDLL     0x00000002      // Delay load

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//---------------------------------------------------------------------------
// Globals
extern HINSTANCE hinstCabinet;  // Instance handle of the app.

extern HWND v_hwndDesktop;

extern HIMAGELIST g_himlSysSmall;
extern HIMAGELIST g_himlSysLarge;
extern int g_nDefOpenSysIndex;
extern int g_nDefNormalSysIndex;
extern COLORREF g_crAltColor;
extern HCURSOR g_hcurWait;
extern HKEY g_hkeyExplorer;

#ifdef WINNT
#define g_bRunOnNT          TRUE
#define g_bRunOnMemphis     FALSE
#else
#define g_bRunOnNT          FALSE
#define g_bRunOnNT5         FALSE
#endif

//
// Is Mirroring APIs enabled (BiDi Memphis and NT5 only)
//
extern BOOL g_bMirroredOS;

// Global System metrics.  the desktop wnd proc will be responsible
// for watching wininichanges and keeping these up to date.

extern int g_fCleanBoot;
extern BOOL g_fFakeShutdown;
extern int g_fDragFullWindows;
extern int g_cxEdge;
extern int g_cyEdge;
extern int g_cySize;
extern int g_cyTabSpace;
extern int g_cxTabSpace;
extern int g_cxBorder;
extern int g_cyBorder;
extern int g_cxPrimaryDisplay;
extern int g_cyPrimaryDisplay;
extern int g_cxDlgFrame;
extern int g_cyDlgFrame;
extern int g_cxFrame;
extern int g_cyFrame;
extern int g_cxMinimized;
extern int g_cxVScroll;
extern int g_cyHScroll;
extern BOOL g_fNoDesktop;

typedef struct _CABVIEW
{
    WINDOWPLACEMENT wp;
    FOLDERSETTINGS fs;
    UINT wHotkey;
    UINT UNUSED;       // unused
    WINVIEW wv;

    SHELLVIEWID m_vidRestore;
    BOOL m_bRestoreView;
} CABVIEW, *PCABVIEW;

typedef struct _hWndAndPlacement {
    HWND hwnd;
    BITBOOL fRestore : 1;
    WINDOWPLACEMENT wp;
} HWNDANDPLACEMENT, *LPHWNDANDPLACEMENT;


typedef struct _appbar {

    HWND hwnd;
    UINT uCallbackMessage;
    RECT rc;
    UINT uEdge;

} APPBAR, *PAPPBAR;


typedef struct _WINDOWPOSITIONS {
    UINT idRes;
    HDSA hdsaWP;
} WINDOWPOSITIONS, *LPWINDOWPOSITIONS;

#define v_hwndTray g_ts.hwndMain

typedef struct {
    HWND hwndMain;
    HWND hwndView;
    HWND hwndRebar;
    
    HWND hwndNotify;     // clock window
    HMENU hmenuStart;
    HWND hwndTrayTips;
    HWND hwndStart;
    HWND hwndStartBalloon;

    HWND hwndLastActive;
    HWND hwndRude;

    HBITMAP hbmpStartBkg;
    
    IUnknown *ptbs;
    IContextMenu* pcmFind;

    SIZE sizeStart;  // height/width of the start button

    BITBOOL fAlwaysOnTop : 1;
    BITBOOL fSMSmallIcons : 1;
    BITBOOL fStuckRudeApp : 1;
    BITBOOL fGlobalHotkeyDisable : 1;
    BITBOOL fThreadTerminate : 1;
    BITBOOL fSysSizing : 1;      // being sized by user; hold off on recalc
    BITBOOL fSelfSizing: 1;
    BITBOOL fDockingFlags : 1;
    BITBOOL bMainMenuInit : 1;
    BITBOOL fFlashing : 1;      // currently flashing (HSHELL_FLASH)
    
#define MM_OTHER    0x01
#define MM_SHUTDOWN 0x02
    UINT uModalMode : 2;

    BITBOOL fHideClock : 1;
    BITBOOL fShouldResize : 1;
    BITBOOL fMonitorClipped : 1;
    BITBOOL fLastHandleTrayHide : 1;
    BITBOOL bDelayRebuilding : 1;
    BITBOOL fCoolTaskbar :1;
    BITBOOL _fDeferedPosRectChange :1;
    BITBOOL _fDesktopRaised :1;
#define g_fDesktopRaised g_ts._fDesktopRaised
    BITBOOL _fHandledDelayBootStuff :1;
#define g_fHandledDelayBootStuff g_ts._fHandledDelayBootStuff
    BITBOOL _fOnOS5:1;
#ifdef WINNT
#define g_bRunOnNT5 g_ts._fOnOS5
#else
#define g_bRunOnMemphis g_ts._fOnOS5
#endif

    BITBOOL fShowDeskBtn : 1;
    BITBOOL fIgnoreTaskbarActivate :1;
    BITBOOL fBalloonUp : 1; // true if balloon notification is up
    BITBOOL fUndoEnabled : 1;
            
    POINT ptLastHittest;

    HWND hwndRun;
    HWND hwndProp;

    DWORD wThreadCmd;                           // for rebuilding the menus

    HANDLE hThread;

    HACCEL              hMainAccel;     // Main accel table
    int iWaitCount;

    // docking stuff
    HANDLE hBIOS;

    // PRINTNOTIFY stuff
    HANDLE heWakeUp;                    // event polling thread sleeps on
    HANDLE htPrinterPoll;               // polling thread handle
    DWORD idPrinterPoll;                // polling thread id
    LPCITEMIDLIST pidlPrintersFolder;   // so we don't keep allocating it

    HDPA hdpaAppBars;  // app bar info
    HDSA hdsaHKI;  // hotkey info

    // Keep track of notification.
    ULONG uProgNotify;
    ULONG uRecentNotify;
    ULONG uFavoritesNotify;
    ULONG uFastNotify;
    ULONG uDesktopNotify;
    ULONG uExtendedEventNotify;     //When theme changes (like HTML templates change), we get notified.

    ULONG uCommonProgNotify;
    ULONG uCommonFastNotify;
    ULONG uCommonDesktopNotify;

    UINT uPrintNotify;

    LPWINDOWPOSITIONS pPositions;  // saved windows positions (for undo of minimize all)

#define AH_ON           0x01
#define AH_HIDING       0x02

    RECT arStuckRects[4];   // temporary for hit-testing
    SIZE sStuckWidths;      // width/height of tray
    UINT uStuckPlace;       // the stuck place
    UINT uMoveStuckPlace;   // stuck status during a move operation

    // these two must  go together for save reasons
    UINT uAutoHide;     // AH_HIDING , AH_ON
    RECT rcOldTray;     // last place we stuck ourselves (for work area diffs)
    HMONITOR hmonStuck; // The current HMONITOR we are on
    HMONITOR hmonOld;   // The last hMonitor we were on 
    IMenuBand*  _pmbStartMenu;  //For Message translation.
    IMenuPopup* _pmpStartMenu;  //For start menu cache
} TRAYSTUFF, *PTRAYSTUFF;

// the order of these is IMPORTANT for move-tracking and profile stuff
// also for the STUCK_HORIZONTAL macro
#define STICK_FIRST     ABE_LEFT
#define STICK_LEFT      ABE_LEFT
#define STICK_TOP       ABE_TOP
#define STICK_RIGHT     ABE_RIGHT
#define STICK_BOTTOM    ABE_BOTTOM
#define STICK_LAST      ABE_BOTTOM
#define STICK_MAX       ABE_MAX
#define STUCK_HORIZONTAL(x)     (x & 0x1)

#if STUCK_HORIZONTAL(STICK_LEFT) || STUCK_HORIZONTAL(STICK_RIGHT) || \
   !STUCK_HORIZONTAL(STICK_TOP)  || !STUCK_HORIZONTAL(STICK_BOTTOM)
#error Invalid STICK_* constants
#endif

#define IsValidSTUCKPLACE(stick) IsInRange(stick, STICK_FIRST, STICK_LAST)

extern TRAYSTUFF g_ts;

extern CABINETSTATE g_CabState;


// ShellServiceObject data structs
extern HDSA g_hdsaShellServiceObjects;
typedef struct
{
    DWORD              flags;
    TCHAR              szName[32];
    CLSID              clsid;
    LPOLECOMMANDTARGET pct;
} SHELLSERVICEOBJECT, *PSHELLSERVICEOBJECT;

enum {
    CTEXECSSOF_REVERSE=0x0001,
};
   
void CTExecShellServiceObjects(const CLSID *pclsid, DWORD nCmdID, DWORD nCmdexecopt, DWORD flags);

// initcab.c
void DoDaylightCheck(BOOL fStartupInit);
HKEY GetSessionKey(REGSAM samDesired);

//
// Debug helper functions
//

void InvokeURLDebugDlg(HWND hwnd);

#define DESKTOP_ACCELERATORS 1

BOOL InitTrayClass(HINSTANCE);
BOOL InitTray(HINSTANCE);

// tray.c/desktop.c

DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout);
void TrayHandleWindowDestroyed(HWND hwnd);
BOOL InitDesktopClass(HINSTANCE hInstance);
BOOL CreateDesktopWindows(HINSTANCE hInstance);
void DeskTray_DestroyShellView();
STDMETHODIMP CDeskTray_GetViewStateStream(IShellBrowser * psb, DWORD grfMode, LPSTREAM *pStrm);
BOOL Reg_SetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPVOID lpData, DWORD cbData);
BOOL Reg_GetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPVOID pData, DWORD *pcbData);
HBITMAP CreateStartBitmap(HWND hwnd);
HMENU LoadMenuPopup(LPCTSTR id);

void _RunFileDlg(HWND hwnd, UINT idIcon, LPCITEMIDLIST pidlWorkingDir,
        UINT idTitle, UINT idPrompt, DWORD dwFlags);

/* message/c */

DWORD FormatMessageWithArgs( DWORD    dwFlags,
                             LPCVOID  lpSource,
                             DWORD    dwMessageId,
                             DWORD    dwLanguageId,
                             LPTSTR   lpBuffer,
                             DWORD    nSize,
                             ... );



BOOL ExecItemByPidls(HWND hwnd, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem);

typedef BOOL (*PFNENUMFOLDERCALLBACK)(LPSHELLFOLDER psf, HWND hwndOwner, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem);
void EnumFolder(HWND hwndOwner, LPITEMIDLIST pidlFolder, DWORD grfFlags, PFNENUMFOLDERCALLBACK pfn);

IShellFolder* BindToFolder(LPCITEMIDLIST pidl);

#undef WinHelp
#define WinHelp SHWinHelp

#ifdef WINNT
// until we implement this in NT, map to our code in nothunk.c
#undef ChangeDisplaySettings
#define ChangeDisplaySettings NoThkChangeDisplaySettings

LONG WINAPI NoThkChangeDisplaySettings(LPDEVMODE lpdv, DWORD dwFlags);

#endif


#ifdef __cplusplus
};       /* End of extern "C" { */
#endif // __cplusplus
