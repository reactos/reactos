#include "cabinet.h"
#include "cabwnd.h"
#include "rcids.h"
#include <shellapi.h>
#include <shlapip.h>
#include "trayclok.h"
#include <fsmenu.h>
#include <shguidp.h>
#include "traynot.h"
#include <help.h>       // help ids
#include <desktray.h>

#if defined(FE_IME)
#include <immp.h>
#endif

#include <trayp.h>
#include <regstr.h>

#ifdef WINNT
// NT APM support
#include <ntddapmt.h>
#else
// Win95 APM support
#include <vmm.h>
#include <bios.h>
#define NOPOWERSTATUSDEFINES
#include <pwrioctl.h>
#include <wshioctl.h>
#include <dbt.h>
#include <pbt.h>

#define Not_VxD
#define No_CM_Calls
#include <configmg.h>
#endif

#include "bandsite.h"
#include "mmhelper.h" // Multimonitor helper functions

#include <shdguid.h>

#include "startmnu.h"
#include "apithk.h"
#include "uemapp.h"
#include "deskconf.h"

#define DM_FOCUS        0           // focus
#define DM_SHUTDOWN     TF_TRAY     // shutdown
#define DM_UEMTRACE     TF_TRAY     // timer service, other UEM stuff
#define DM_MISC         0           // miscellany

#define POLLINTERVAL    (15*1000)       // 15 seconds
#define MINPOLLINTERVAL (3*1000)        // 3 seconds
#define ERRTIMEOUT      (5*1000)        // 5 seconds

#define SECOND 1000
#define RUNWAITSECS 5

#define DOCKSTATE_DOCKED            0
#define DOCKSTATE_UNDOCKED          1
#define DOCKSTATE_UNKNOWN           2

#ifdef WINNT
#define MAXPRINTERBUFFER (MAX_PATH+18+1)
#define JOB_STATUS_ERROR_BITS (JOB_STATUS_USER_INTERVENTION|JOB_STATUS_ERROR)
#else
#define MAXPRINTERBUFFER 32
#define JOB_STATUS_ERROR_BITS JOB_STATUS_USER_INTERVENTION
#endif

#define RDM_ACTIVATE                (WM_USER + 0x101)

// import the WIN31 Compatibility HACKs from the shell32.dll
WINSHELLAPI void WINAPI CheckWinIniForAssocs(void);

HWND v_hwndDesktop = NULL;

#ifdef WINNT
// event to tell the services on NT5 that we are done with boot
// and they can do their stuff
HANDLE g_hShellReadyEvent = NULL;
#endif

UINT g_uStartButtonAllowPopup = WM_NULL;
UINT g_uStartButtonBalloonTip = WM_NULL;

// Users and Passwords must send this message to get the "real" logged on user to log off.
// This is required since sometimes U&P runs in the context of a different user and logging this
// other user off does no good. See ext\netplwiz for the other half of this...-dsheldon.
UINT g_uLogoffUser = WM_NULL;

// DSA haPrinterNames structure:
typedef struct _PRINTERNAME {
    TCHAR szPrinterName[MAXPRINTERBUFFER]; // we know max printer name length is 32
    LPITEMIDLIST pidlPrinter; // relative pidl to printer
    BOOL fInErrorState;
} PRINTERNAME, * LPPRINTERNAME;

void RaiseDesktop();
void PrintNotify_Init(HWND hwnd);
void PrintNotify_AddToQueue(LPHANDLE phaPrinterNames, LPSHELLFOLDER *ppsf, LPCITEMIDLIST pidlPrinter);
BOOL PrintNotify_StartThread(HWND hwnd);
void PrintNotify_HandleFSNotify(HWND hwnd, LPCITEMIDLIST *ppidl, LONG lEvent);
void PrintNotify_Exit(void);
void PrintNotify_IconNotify(HWND hwnd, LPARAM uMsg);

void RevertMenu(HMENU hmenuSub, UINT idMenu);

void _PropagateMessage(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

void Tray_HandleShellServiceObject(WPARAM wParam, LPARAM lParam);
STDMETHODIMP CShellTray_AddViewPropertySheetPages(DWORD dwReserved, LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam);
HMONITOR Tray_GetDisplayRectFromRect(LPRECT prcDisplay, LPCRECT prcIn, UINT uFlags);
HMONITOR Tray_GetDisplayRectFromPoint(LPRECT prcDisplay, POINT pt, UINT uFlags);
void Tray_MakeStuckRect(LPRECT prcStick, LPCRECT prcBound, SIZE size, UINT uStick);
void Tray_ScreenSizeChange(HWND hwnd);
void Tray_SizeWindows();
void Tray_ContextMenu(DWORD dwPos, BOOL fSetTime);
void Tray_DesktopMenu(DWORD dwPos);
void StuckAppChange(HWND hwnd, LPCRECT prcOld, LPCRECT prcNew, BOOL bTray);
void Tray_StuckTrayChange();
void SetWindowStyleBit(HWND hwnd, DWORD dwBit, DWORD dwValue);
void CStartDropTarget_Register();
void CStartDropTarget_Revoke();
void DestroySavedWindowPositions(LPWINDOWPOSITIONS pPositions);
void Tray_ResetZorder();
void Cabinet_InitGlobalMetrics(WPARAM, LPTSTR);
void StartMenuFolder_ContextMenu(DWORD dwPos);
void Tray_HandleSize();
void Tray_HandleSizing(WPARAM code, LPRECT lprc, UINT uStuckPlace);
void MinimizeAll(HWND hwndView);
void Tray_RegisterGlobalHotkeys();
void Tray_UnregisterGlobalHotkeys();
void Tray_HandleGlobalHotkey(WPARAM wParam);
void Tray_Unhide();
void Tray_SetAutoHideTimer();
void Tray_ComputeHiddenRect(LPRECT prc, UINT uStuck);
UINT Tray_GetDockedRect(LPRECT prc, BOOL fMoving);
void Tray_CalcClipCoords(RECT *prcClip, const RECT *prcMonitor, const RECT *prcNew);
void Tray_ClipInternal(const RECT *prcClip);
void Tray_ClipWindow(BOOL fEnableClipping);
UINT Tray_CalcDragPlace(POINT pt);
UINT Tray_RecalcStuckPos(LPRECT prc);
void Tray_AutoHideCollision();
void StartMenu_Build();
int HotkeyList_Restore(HWND hwnd);
LRESULT Tray_RegisterHotkey(HWND hwnd, int i);
void RecordStartButtonSize(HWND hwndStart);
LRESULT Tray_HandleMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpmi);
void Tray_OnDesktopState(LPARAM lParam);
#ifdef DESKBTN
void Tray_CreateDesktopButton();
HWND g_hwndDesktopTB = NULL;
#else
#define Tray_CreateDesktopButton()  0
#define KillConfigDesktopDlg() 0
#define g_hwndDesktopTB NULL
#endif
void ToggleDesktop();
void LowerDesktop();
void DoTrayProperties(INT nStartPage);
void RunStartupApps();
void ClearRecentDocumentsAndMRUStuff(BOOL fBroadcastChange);
void WriteCleanShutdown(DWORD dwValue);
void RefreshStartMenu();

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
void _DragMove(HWND hwndTarget, const POINTL ptStart);

BOOL SetWindowZorder(HWND hwnd, HWND hwndInsertAfter);

//
// Settings UI entry point types.
//
typedef VOID (WINAPI *PTRAYPROPSHEETCALLBACK)(DWORD nStartPage);
typedef VOID (WINAPI *PSETTINGSUIENTRY)(PTRAYPROPSHEETCALLBACK);

DWORD SettingsUI_ThreadProc(void *pv);
VOID WINAPI SettingsUI_TrayPropSheetCallback(DWORD nStartPage);


extern void Cabinet_RefreshAll(void);

#if defined(DBCS) || defined(FE_IME)
// b#11258-win95d
#if !defined(WINNT)
DWORD WINAPI ImmGetAppIMECompatFlags(DWORD dwThreadID);
#endif
#endif

// Shell perf automation
extern DWORD g_dwShellStartTime;
extern DWORD g_dwShellStopTime;

// appbar stuff
BOOL IAppBarSetAutoHideBar(HWND hwnd, BOOL fAutoHide, UINT uEdge);
void IAppBarActivationChange(HWND hwnd, UINT uEdge);
HWND AppBarGetAutoHideBar(UINT uEdge);
// BOGUS: nuke this (multiple monitors...)
HWND g_hwndAutoHide[ABE_MAX] = { NULL, NULL, NULL, NULL };
extern BOOL g_fUseMerge;

BOOL IsChildOrHWND(HWND hwnd, HWND hwndChild)
{
    return (hwnd == hwndChild || IsChild(hwnd, hwndChild));
}

void ClockCtl_HandleTrayHide(BOOL fHiding)
{
    g_ts.fLastHandleTrayHide = fHiding ? TRUE : FALSE;
    SendMessage(g_ts.hwndNotify, TNM_TRAYHIDE, 0, fHiding);
}


// dyna-res change for multi-config hot/warm-doc
void HandleDisplayChange(int x, int y, BOOL fCritical);
#ifdef WINNT
#define IsDisplayChangeSafe() TRUE
#else
BOOL IsDisplayChangeSafe(void);
#endif
DWORD GetMinDisplayRes(void);

// appbar stuff
void AppBarNotifyAll(HMONITOR hmon, UINT uMsg, HWND hwndExclude, LPARAM lParam);

// Button subclass.
WNDPROC g_ButtonProc = NULL;

// timer IDs
#define IDT_AUTOHIDE            2
#define IDT_AUTOUNHIDE          3
#ifdef DELAYWININICHANGE
#define IDT_DELAYWININICHANGE   5
#endif
#define IDT_DESKTOP             6
#define IDT_PROGRAMS            IDM_PROGRAMS
#define IDT_RECENT              IDM_RECENT
#define IDT_REBUILDMENU         7
#define IDT_HANDLEDELAYBOOTSTUFF 8
#define IDT_REVERTPROGRAMS      9
#define IDT_REVERTRECENT        10
#define IDT_REVERTFAVORITES     11

#define IDT_STARTMENU           12

#define IDT_ENDUNHIDEONTRAYNOTIFY 13

#define IDT_SERVICE0            14
#define IDT_SERVICE1            15
#define IDT_SERVICELAST         IDT_SERVICE1
#define IDT_SAVESETTINGS        17
#define IDT_ENABLEUNDO          18

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#define SZ_RECENTKEY        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs\\Menu")
#define SZ_STARTMENUKEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Start Menu\\Menu")
#define REGSTR_EXPLORER_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")

int g_cyTrayBorders = -1; // the amount of Y difference between the window and client height;

//
// amount of time to show/hide the tray
// to turn sliding off set these to 0
//
int g_dtSlideHide;
int g_dtSlideShow;

typedef enum
{
    // Sequence commands
    SMCT_NONE                   = 0x0,
    SMCT_PRIMARY                = 0x1,
    SMCT_WININIASSOCS           = 0x2,
    SMCT_DESKTOPHOTKEYS         = 0x3,
    SMCT_INITPROGRAMS           = 0x4,
    SMCT_INITRECENT             = 0x5,
    SMCT_INITFAVORITES          = 0x6,
    SMCT_PARTFILLPROGRAMS       = 0x7,
    SMCT_FILLRECENT             = 0x8,
    SMCT_FILLPROGRAMS           = 0x9,
    SMCT_FILLFASTITEMS          = 0xa,
    SMCT_FILLFAVORITES          = 0xb,
    SMCT_BUILDLISTOFPATHS       = 0xc,  // Must be last item in list before SMCT_DONE
    SMCT_DONE                   = 0xd,

    // Single execution commands
    SMCT_FILLRECENTONLY         = 0xe,
    SMCT_FILLPROGRAMSONLY       = 0xf,
    SMCT_DESKTOPHOTKEYSONLY     = 0x10,
    SMCT_FILLFAVORITESONLY      = 0x11,
    SMCT_STOP                   = 0x12,
    SMCT_RESTART                = 0x13,
    SMCT_STOPNOWAITBLOP         = 0x14, // Stop, but don't wait for BuildListOfPaths to complete
} STARTMENUCONTROLTHREAD;

// INSTRUMENTATION WARNING: If you change anything here, make sure to update instrument.c
// we need to start at 500 because we're now sharing the hotkey handler
// with shortcuts..  they use an index array so they need to be 0 based
// NOTE, this constant is also in desktop.cpp, so that we can forward hotkeys from the desktop for
// NOTE, app compatibility.
#define GHID_FIRST 500
enum
{
    GHID_RUN = GHID_FIRST,
    GHID_MINIMIZEALL,
    GHID_UNMINIMIZEALL,
    GHID_HELP,
    GHID_EXPLORER,
    GHID_FINDFILES,
    GHID_FINDCOMPUTER,
    GHID_TASKTAB,
    GHID_TASKSHIFTTAB,
    GHID_SYSPROPERTIES,
    GHID_DESKTOP,
    GHID_MAX
};

const DWORD GlobalKeylist[] =
{ MAKELONG(TEXT('R'), MOD_WIN),
      MAKELONG(TEXT('M'), MOD_WIN),
      MAKELONG(TEXT('M'), MOD_SHIFT|MOD_WIN),
      MAKELONG(VK_F1,MOD_WIN),
      MAKELONG(TEXT('E'),MOD_WIN),
      MAKELONG(TEXT('F'),MOD_WIN),
      MAKELONG(TEXT('F'), MOD_CONTROL|MOD_WIN),
      MAKELONG(VK_TAB, MOD_WIN),
      MAKELONG(VK_TAB, MOD_WIN|MOD_SHIFT),
      MAKELONG(VK_PAUSE,MOD_WIN),
      MAKELONG(TEXT('D'),MOD_WIN),
};


void DoExitWindows(HWND hwnd);
void Tray_Command(UINT idCmd);
LONG Tray_SetAutoHideState(BOOL fAutoHide);

LRESULT CALLBACK Tray_WndProc(HWND, UINT, WPARAM, LPARAM);

//---------------------------------------------------------------------------
// Global to this file only.
// NB None of the tray stuff needs thread serialising because
// we only ever have one tray.

TRAYSTUFF g_ts = {0};

// g_fDockingFlags bits
#define DOCKFLAG_WARMEJECTABLENOW       (1<<0)

// TVSD Flags.
#define TVSD_NULL               0x0000
#define TVSD_AUTOHIDE           0x0001
#define TVSD_TOPMOST            0x0002
#define TVSD_SMSMALLICONS       0x0004
#define TVSD_HIDECLOCK          0x0008
#define TVSD_SHOWDESKBTN        0x0010

// old Win95 TVSD struct
typedef struct _TVSD95
{
    DWORD   dwSize;
    LONG    cxScreen;
    LONG    cyScreen;
    LONG    dxLeft;
    LONG    dxRight;
    LONG    dyTop;
    LONG    dyBottom;
    DWORD   uAutoHide;
    RECTL   rcAutoHide;
    DWORD   uStuckPlace;
    DWORD   dwFlags;
} TVSD95;

// Nashville tray save data
typedef struct _TVSD
{
    DWORD   dwSize;
    LONG    lSignature;     // signature (must be negative)

    DWORD   dwFlags;        // TVSD_ flags

    DWORD   uStuckPlace;    // current stuck edge
    SIZE    sStuckWidths;   // widths of stuck rects (BUGBUG: in tbd units)
    RECT    rcLastStuck;    // last stuck position in pixels

} TVSD;

// convenient union for reading either
typedef union _TVSDCOMPAT
{
    TVSD;           // new format
    TVSD95 w95;     // old format

} TVSDCOMPAT;
#define TVSDSIG_CURRENT     (-1L)
#define IS_CURRENT_TVSD(t)  ((t.dwSize >= sizeof(TVSD)) && (t.lSignature < 0))
#define MAYBE_WIN95_TVSD(t) (t.dwSize == sizeof(TVSD95))

BOOL ShouldWeShowTheStartButtonBalloon()
{
    DWORD dwType;
    DWORD dwData = 0;
    DWORD cbSize = sizeof(DWORD);
    SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, 
            TEXT("StartButtonBalloonTip"), &dwType, (BYTE*)&dwData, &cbSize);
    return (dwData == 0);       // if StartButtonBalloonTip == 1, don't show
}

void DontShowTheStartButtonBalloonAnyMore()
{
    DWORD dwData = 1;
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, 
        TEXT("StartButtonBalloonTip"), REG_DWORD, (BYTE*)&dwData, sizeof(DWORD));
}

void DestroyStartButtonBalloon()
{
    if (g_ts.hwndStartBalloon)
    {
        DestroyWindow(g_ts.hwndStartBalloon);
        g_ts.hwndStartBalloon = NULL;
    }
}

void ShowStartButtonToolTip()
{
    if (!ShouldWeShowTheStartButtonBalloon())
        return;

    if (!g_ts.hwndStartBalloon)
    {

        g_ts.hwndStartBalloon = CreateWindow(TOOLTIPS_CLASS, NULL,
                                             WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             CW_USEDEFAULT, CW_USEDEFAULT,
                                             NULL, NULL, hinstCabinet,
                                             NULL);

        if (g_ts.hwndStartBalloon) 
        {
            // set the version so we can have non buggy mouse event forwarding
            SendMessage(g_ts.hwndStartBalloon, CCM_SETVERSION, COMCTL32_VERSION, 0);
            SendMessage(g_ts.hwndStartBalloon, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
        }
    }

    if (g_ts.hwndStartBalloon)
    {
        TCHAR szTip[MAX_PATH];
        szTip[0] = TEXT('\0');
        LoadString(hinstCabinet, IDS_STARTMENUBALLOON_TIP, szTip, ARRAYSIZE(szTip));
        if (szTip[0])
        {
            RECT rc;
            TOOLINFO ti = {0};

            ti.cbSize = SIZEOF(ti);
            ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_TRANSPARENT;
            ti.hwnd = v_hwndTray;
            ti.uId = (UINT_PTR)g_ts.hwndStart;
            //ti.lpszText = NULL;
            SendMessage(g_ts.hwndStartBalloon, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
            SendMessage(g_ts.hwndStartBalloon, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)0);

            ti.lpszText = szTip;
            SendMessage(g_ts.hwndStartBalloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

            LoadString(hinstCabinet, IDS_STARTMENUBALLOON_TITLE, szTip, ARRAYSIZE(szTip));
            if (szTip[0])
            {
                SendMessage(g_ts.hwndStartBalloon, TTM_SETTITLE, TTI_INFO, (LPARAM)szTip);
            }

            GetWindowRect(g_ts.hwndStart, &rc);

            SendMessage(g_ts.hwndStartBalloon, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.top));

            SetWindowZorder(g_ts.hwndStartBalloon, HWND_TOPMOST);

            SendMessage(g_ts.hwndStartBalloon, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
        }
    }

}

// Mirror a bitmap in a DC (mainly a text object in a DC)
//
// [samera]
//
void MirrorBitmapInDC( HDC hdc , HBITMAP hbmOrig )
{
  HDC     hdcMem;
  HBITMAP hbm;
  BITMAP  bm;


  if( !GetObject( hbmOrig , sizeof(BITMAP) , &bm ))
    return;

  hdcMem = CreateCompatibleDC( hdc );

  if( !hdcMem )
    return;

  hbm = CreateCompatibleBitmap( hdc , bm.bmWidth , bm.bmHeight );

  if( !hbm )
  {
    DeleteDC( hdcMem );
    return;
  }

  //
  // Flip the bitmap
  //
  SelectObject( hdcMem , hbm );
  SET_DC_RTL_MIRRORED(hdcMem);

  BitBlt( hdcMem , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdc , 0 , 0 , SRCCOPY );

  SET_DC_LAYOUT(hdcMem,0);

  //
  // BUGBUG : The offset by 1 in hdcMem is to solve the off-by-one problem. Removed.
  // [samera]
  //
  BitBlt( hdc , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdcMem , 0 , 0 , SRCCOPY );


  DeleteDC( hdcMem );
  DeleteObject( hbm );

  return;
}


BOOL Tray_CreateClockWindow()
{
    g_ts.hwndNotify = TrayNotifyCreate(v_hwndTray, IDC_CLOCK, hinstCabinet);
    return BOOLFROMPTR(g_ts.hwndNotify);
}

BOOL InitTrayClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    ZeroMemory(&wc, SIZEOF(WNDCLASS));

    wc.lpszClassName = TEXT(WNDCLASS_TRAYNOTIFY);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = Tray_WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);

    return RegisterClass(&wc);
}


HMENU LoadMenuPopup(LPCTSTR id)
{
    HMENU hMenuSub = NULL;
    HMENU hMenu = LoadMenu(hinstCabinet, id);
    if (hMenu) {
        hMenuSub = GetSubMenu(hMenu, 0);
        if (hMenuSub) {
            RemoveMenu(hMenu, 0, MF_BYPOSITION);
        }
        DestroyMenu(hMenu);
    }

    return hMenuSub;
}

#define CXGAP 4
//----------------------------------------------------------------------------
// Compose the Start bitmap out of a flag and some text.
// REVIEW UNDONE - Put the up/down arrow back in.
HBITMAP CreateStartBitmap(HWND hwndTray)
{
    HBITMAP hbmpStart = NULL;
    HBITMAP hbmpStartOld = NULL;
    HICON hiconFlag = NULL;
    int cx, cy, cySmIcon;
    TCHAR szStart[256];
    SIZE size;
    HDC hdcStart;
    HDC hdcScreen;
    HFONT hfontStart = NULL;
    HFONT hfontStartOld = NULL;
    NONCLIENTMETRICS ncm;
    RECT rcStart;
    WORD wLang;

    // DebugMsg(DM_TRACE, "c.csb: Creating start bitmap.");

    hdcScreen = GetDC(NULL);
    hdcStart = CreateCompatibleDC(hdcScreen);
    if (hdcStart)
    {
        ncm.cbSize = SIZEOF(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE))
        {
            wLang = GetUserDefaultLangID();
            // Select normal weight font for chinese language.
            if ( PRIMARYLANGID(wLang) == LANG_CHINESE &&
               ( (SUBLANGID(wLang) == SUBLANG_CHINESE_TRADITIONAL) ||
                 (SUBLANGID(wLang) == SUBLANG_CHINESE_SIMPLIFIED) ))
                ncm.lfCaptionFont.lfWeight = FW_NORMAL;
            else
                ncm.lfCaptionFont.lfWeight = FW_BOLD;

            hfontStart = CreateFontIndirect(&ncm.lfCaptionFont);
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("c.csb: Can't create font."));
        }

        // Get an idea about how big we need everyhting to be.
        LoadString(hinstCabinet, IDS_START, szStart, ARRAYSIZE(szStart));

        hfontStartOld = SelectObject(hdcScreen, hfontStart);
        GetTextExtentPoint(hdcScreen, szStart, lstrlen(szStart), &size);
        SelectObject(hdcScreen, hfontStartOld);

// Mimick the tray and ignore the font size for determining the height.
#if 0
        cy = max(g_cySize, size.cy);
#else
        cy = g_cySize;
#endif
        cySmIcon = GetSystemMetrics(SM_CYSMICON);
        cx = (cy + size.cx) + CXGAP;
        hbmpStart = CreateCompatibleBitmap(hdcScreen, cx, cy);
        hbmpStartOld = SelectObject(hdcStart, hbmpStart);

        hfontStartOld = SelectObject(hdcStart , hfontStart);

        //
        // Let's mirror the DC, so that text won't get mirrored
        // if the window ir mirrored
        //
        if (IS_WINDOW_RTL_MIRRORED(hwndTray))
        {
            //
            // Mirror the DC, so that drawing goes from the visual right
            // edge. [samera]
            //
            SET_DC_RTL_MIRRORED(hdcStart);
        }

        rcStart.left = -g_cxEdge; // subtract this off because drawcaptiontemp adds it on
        rcStart.top = 0;
        rcStart.right = cx;
        rcStart.bottom = cy;
        hiconFlag = (HICON)LoadImage(NULL, MAKEINTRESOURCE(OIC_WINLOGO_DEFAULT ), IMAGE_ICON, cySmIcon, cySmIcon, 0);
        // Get User to draw everything for us.
        DrawCaptionTemp(hwndTray, hdcStart, &rcStart, hfontStart, hiconFlag, szStart, DC_INBUTTON | DC_TEXT | DC_ICON | DC_NOSENDMSG);

        //
        // Now we have the image ready to maintained by USER, let's mirror
        // it so that when it is bitblt'ed to the hwndTray DC it will be mirrored
        // and the bitmap image is maintained. [samera]
        //
        if (IS_WINDOW_RTL_MIRRORED(hwndTray))
        {
            MirrorBitmapInDC(hdcStart, hbmpStart);
        }

        // Clean up Start stuff.
        SelectObject(hdcStart, hbmpStartOld);
        if (hfontStart)
        {
            SelectObject(hdcStart, hfontStartOld);
            DeleteObject(hfontStart);
        }
        DeleteDC(hdcStart);
        DestroyIcon(hiconFlag);
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("c.csb: Can't create Start bitmap."));
    }

    ReleaseDC(NULL, hdcScreen);
    return hbmpStart;
}

// Set the stuck monitor for the tray window
void Tray_SetStuckMonitor()
{
    // use STICK_LEFT because most of the multi-monitors systems are set up
    // side by side. use DEFAULTTONULL because we don't want to get the wrong one
    // use the center point to call again in case we failed the first time.
    g_ts.hmonStuck = MonitorFromRect(&g_ts.arStuckRects[STICK_LEFT],
                                     MONITOR_DEFAULTTONULL);
    if (!g_ts.hmonStuck)
    {
        POINT pt;
        pt.x = (g_ts.arStuckRects[STICK_LEFT].left + g_ts.arStuckRects[STICK_LEFT].right)/2;
        pt.y = (g_ts.arStuckRects[STICK_LEFT].top + g_ts.arStuckRects[STICK_LEFT].bottom)/2;
        g_ts.hmonStuck = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    }

    g_ts.hmonOld = g_ts.hmonStuck;
}

VOID Tray_GetSaveStateAndInitRects()
{
    TVSDCOMPAT tvsd;
    DWORD cbData1, cbData2;
    RECT rcDisplay;
    DWORD dwTrayFlags;
    UINT uStick;
    SIZE size;

    //
    // first fill in the defaults
    //
    SetRect(&rcDisplay, 0, 0, g_cxPrimaryDisplay, g_cyPrimaryDisplay);

    // size gets defaults
    size.cx = g_ts.sizeStart.cx + 2 * (g_cxDlgFrame + g_cxBorder);
    size.cy = g_ts.sizeStart.cy + 2 * (g_cyDlgFrame + g_cyBorder);

    // sStuckWidths gets minimum
    g_ts.sStuckWidths.cx = 2 * (g_cxDlgFrame + g_cxBorder);
    g_ts.sStuckWidths.cy = g_ts.sizeStart.cy + 2 * (g_cyDlgFrame + g_cyBorder);

    g_ts.uStuckPlace = STICK_BOTTOM;
    dwTrayFlags = TVSD_TOPMOST;

#ifdef WINNT
    //  if we are on a remote hydra session
    //  and if there is no previous saved value,
    //  do not display the clock.
    if (IsRemoteSession())
    {
        g_ts.fHideClock = TRUE;
        dwTrayFlags |= TVSD_HIDECLOCK;
    }
#endif

    g_ts.uAutoHide = 0;

    //
    // now try to load saved vaules
    //

    
    // BUG : 231077
    // Since Tasbar properties don't roam from NT5 to NT4, (NT4 -> NT5 yes) 
    // Allow roaming from NT4 to NT5 only for the first time the User logs
    // on to NT5, so that future changes to NT5 are not lost when the user
    // logs on to NT4 after customizing the taskbar properties on NT5.

    cbData1 = SIZEOF(tvsd);
    cbData2 = SIZEOF(tvsd);
    if (Reg_GetStruct(g_hkeyExplorer, TEXT("StuckRects2"), TEXT("Settings"), 
        &tvsd, &cbData1) 
        ||
        Reg_GetStruct(g_hkeyExplorer, TEXT("StuckRects"), TEXT("Settings"),
        &tvsd, &cbData2))
    {
        if (IS_CURRENT_TVSD(tvsd) && IsValidSTUCKPLACE(tvsd.uStuckPlace))
        {
            Tray_GetDisplayRectFromRect(&rcDisplay, &tvsd.rcLastStuck,
                MONITOR_DEFAULTTONEAREST);

            // BUGBUG: scale sStuckWidths.cy here?
            size = tvsd.sStuckWidths;
            g_ts.uStuckPlace = tvsd.uStuckPlace;

            dwTrayFlags = tvsd.dwFlags;
        }
        else if (MAYBE_WIN95_TVSD(tvsd) &&
                 IsValidSTUCKPLACE(tvsd.w95.uStuckPlace))
        {
            g_ts.uStuckPlace = tvsd.w95.uStuckPlace;
            dwTrayFlags = tvsd.w95.dwFlags;
            if (tvsd.w95.uAutoHide & AH_ON)
                dwTrayFlags |= TVSD_AUTOHIDE;

            switch (g_ts.uStuckPlace)
            {
            case STICK_LEFT:
                size.cx = tvsd.w95.dxLeft;
                break;

            case STICK_RIGHT:
                size.cx = tvsd.w95.dxRight;
                break;

            case STICK_BOTTOM:
                size.cy = tvsd.w95.dyBottom;
                break;

            case STICK_TOP:
                size.cy = tvsd.w95.dyTop;
                break;
            }
        }
    }
    

    ASSERT(IsValidSTUCKPLACE(g_ts.uStuckPlace));

    //
    // use the size only if it is not bogus
    //
    if (g_ts.sStuckWidths.cx < size.cx)
        g_ts.sStuckWidths.cx = size.cx;

    if (g_ts.sStuckWidths.cy < size.cy)
        g_ts.sStuckWidths.cy = size.cy;

    //
    // set the tray flags
    //
    g_ts.fAlwaysOnTop  = dwTrayFlags & TVSD_TOPMOST      ? 1     : 0;
    g_ts.fSMSmallIcons = dwTrayFlags & TVSD_SMSMALLICONS ? 1     : 0;

    g_ts.fHideClock    = dwTrayFlags & TVSD_HIDECLOCK    ? 1     : 0;

    g_ts.uAutoHide     = dwTrayFlags & TVSD_AUTOHIDE     ? (AH_ON | AH_HIDING) : 0;

    g_ts.fShowDeskBtn  = dwTrayFlags & TVSD_SHOWDESKBTN  ? 1 : 0;

    //
    // initialize stuck rects
    //
    for (uStick = STICK_LEFT; uStick <= STICK_BOTTOM; uStick++)
        Tray_MakeStuckRect(&g_ts.arStuckRects[uStick], &rcDisplay, g_ts.sStuckWidths, uStick);

    // Determine which monitor the tray is on using its stuck rectangles
    Tray_SetStuckMonitor();
}

extern HRESULT Tray_SaveView();

void _SaveTrayStuff(void)
{
    TVSD tvsd;

    tvsd.dwSize = SIZEOF(tvsd);
    tvsd.lSignature = TVSDSIG_CURRENT;

    // position
    CopyRect(&tvsd.rcLastStuck, &g_ts.arStuckRects[g_ts.uStuckPlace]);
    tvsd.sStuckWidths = g_ts.sStuckWidths;
    tvsd.uStuckPlace = g_ts.uStuckPlace;

    tvsd.dwFlags = 0;
    if (g_ts.fAlwaysOnTop)      tvsd.dwFlags |= TVSD_TOPMOST;
    if (g_ts.fSMSmallIcons)     tvsd.dwFlags |= TVSD_SMSMALLICONS;
    if (g_ts.fHideClock)        tvsd.dwFlags |= TVSD_HIDECLOCK;
    if (g_ts.uAutoHide & AH_ON) tvsd.dwFlags |= TVSD_AUTOHIDE;
    if (g_ts.fShowDeskBtn)      tvsd.dwFlags |= TVSD_SHOWDESKBTN;

    // Save for now in Stuck rects.
    // BUGBUG: really want to save rcLastStuck per user/maching/config
    Reg_SetStruct(g_hkeyExplorer, TEXT("StuckRects2"), TEXT("Settings"), &tvsd, SIZEOF(tvsd));

    Tray_SaveView(g_ts.ptbs);

    return;
}

/*------------------------------------------------------------------
** align toolbar so that buttons are flush with client area
** and make toolbar's buttons to be MENU style
**------------------------------------------------------------------*/
void Tray_AlignStartButton()
{
    HWND hwndStart = g_ts.hwndStart;
    if (hwndStart)
    {
        RECT rcClient;
        if (g_ts.sizeStart.cy == 0)
        {
            BITMAP bm;
            HBITMAP hbm = (HBITMAP)SendMessage(hwndStart, BM_GETIMAGE, IMAGE_BITMAP, 0);

            if (hbm)
            {
                GetObject(hbm, SIZEOF(bm), &bm);

                g_ts.sizeStart.cx = bm.bmWidth  + 2 * g_cxEdge;
                g_ts.sizeStart.cy = bm.bmHeight + 2 * g_cyEdge;
                if (g_ts.sizeStart.cy < g_cySize + 2*g_cyEdge)
                    g_ts.sizeStart.cy = g_cySize + 2*g_cyEdge;
            }
            else
            {
                // BUGBUG: New user may have caused this to fail...
                // Setup some size for it that wont be too bad...
                g_ts.sizeStart.cx = g_cxMinimized;
                g_ts.sizeStart.cy = g_cySize + 2*g_cyEdge;
            }
        }
        GetClientRect(g_ts.hwndMain, &rcClient);
        SetWindowPos(hwndStart, NULL, 0, 0, (rcClient.right < g_ts.sizeStart.cx) ? rcClient.right : g_ts.sizeStart.cx, g_ts.sizeStart. cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

//----------------------------------------------------------------------------
// Allow us to do stuff on a "button-down".
HWND g_hwndPrevFocus = NULL;

LRESULT CALLBACK StartButtonSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    static UINT uDown = 0;
    static BOOL fAllowUp = FALSE;       // Is the start button allowed to be in the up position?

    ASSERT(g_ButtonProc)

    // Is the button going down?
    if (uMsg == BM_SETSTATE)
    {
        // Is it going Down?
        if (wParam) {
            // DebugMsg(DM_TRACE, "c.stswp: Set state %d", wParam);
            // Yes, Is it already down?
            if (!uDown)
            {
                // Nope.
                INSTRUMENT_STATECHANGE(SHCNFI_STATE_START_DOWN);
                uDown = 1;

                // If we are going down, then we do not want to popup again until the Start Menu is collapsed
                fAllowUp = FALSE;

                SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);

                // Show the button down.
                lRet = CallWindowProc(g_ButtonProc, hwnd, uMsg, wParam, lParam );
                // Notify the parent.
                SendMessage(GetParent(hwnd), WM_COMMAND, (WPARAM)LOWORD(GetDlgCtrlID(hwnd)), (LPARAM)hwnd);
                return lRet;
            }
            else
            {
                // Yep. Do nothing.
                // fDown = FALSE;
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
        }
        else
        {
            // DebugMsg(DM_TRACE, "c.stswp: Set state %d", wParam);
            // Nope, buttons coming up.

            // Is it supposed to be down?   Is it not allowed to be up?
            if (uDown == 1 || !fAllowUp)
            {
                INSTRUMENT_STATECHANGE(SHCNFI_STATE_START_UP);

                // Yep, do nothing.
                uDown = 2;
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
            else
            {
                SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
                // Nope, Forward it on.
                uDown = 0;
                return CallWindowProc(g_ButtonProc, hwnd, uMsg, wParam, lParam );
            }
        }
    }
    else
    {
        switch (uMsg) {
        case WM_LBUTTONDOWN:
            // The button was clicked on, then we don't need no stink'n focus rect.
            SendMessage(GetParent(hwnd), WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, 
                UISF_HIDEFOCUS), 0);

            goto ProcessCapture;
            break;


        case WM_KEYDOWN:
            // The user pressed enter or return or some other bogus key combination when
            // the start button had keyboard focus, so show the rect....
            SendMessage(GetParent(hwnd), WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, 
                UISF_HIDEFOCUS), 0);

            if (wParam == VK_RETURN)
                PostMessage(g_ts.hwndMain, WM_COMMAND, IDC_KBSTART, 0);

            // We do not need the capture, because we do all of our button processing
            // on the button down. In fact taking capture for no good reason screws with
            // drag and drop into the menus. We're overriding user.
ProcessCapture:
            lRet = CallWindowProc(g_ButtonProc, hwnd, uMsg, wParam, lParam );
            SetCapture(NULL);
            return lRet;
            break;

        case WM_MOUSEMOVE:
        {
            MSG msg;

            msg.lParam = lParam;
            msg.wParam = wParam;
            msg.message = uMsg;
            msg.hwnd = hwnd;
            SendMessage(g_ts.hwndTrayTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)& msg);

            break;
        }

        case WM_NULL:
                break;
        default:
            if (uMsg == g_uStartButtonAllowPopup)
            {
                fAllowUp = TRUE;
            }
            break;
        }

        return CallWindowProc(g_ButtonProc, hwnd, uMsg, wParam, lParam);
    }
}


const TCHAR c_szButton[] = TEXT("button");

/*------------------------------------------------------------------
** create the toolbar with the three buttons and align windows
**------------------------------------------------------------------*/
HWND Tray_CreateStartButton()
{
    HWND hwnd;
    DWORD dwStyle = BS_BITMAP;

    g_uStartButtonBalloonTip = RegisterWindowMessage(TEXT("Welcome Finished")); 

    g_uLogoffUser = RegisterWindowMessage(TEXT("Logoff User"));

    // BUGBUG: BS_CENTER | VS_VCENTER required, user bug?

    hwnd = CreateWindowEx(0, c_szButton, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        BS_PUSHBUTTON | BS_LEFT | BS_VCENTER | dwStyle,
        0, 0, 0, 0, v_hwndTray, (HMENU)IDC_START, hinstCabinet, NULL);

    if (hwnd)
    {
        // Subclass it.
        g_ts.hwndStart = hwnd;
        g_ButtonProc = SubclassWindow(hwnd, StartButtonSubclassWndProc);

        {
            HBITMAP hbm = CreateStartBitmap(v_hwndTray);
            if (hbm)
            {
                SendMessage(hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
                Tray_AlignStartButton();
                return hwnd;
            }
        }

        DestroyWindow(hwnd);
    }

    return NULL;
}


void Tray_GetWindowSizes(PRECT prcClient, PRECT prcView, PRECT prcClock, PRECT prcDesktop)
{
    int xFSView, yFSView, cxFSView, cyFSView;
    int xClock, yClock, cxClock, cyClock;
    DWORD_PTR dwClockMinSize;
    int cxDesktop;
    int cyDesktop;
    BOOL fDesktopBottom = FALSE;
    RECT rcDesktop;

    if (!g_hwndDesktopTB || !g_ts.fShowDeskBtn || SHRestricted(REST_CLASSICSHELL))
    {
        cxDesktop = cyDesktop = 0;
    }
    else
    {
        SendMessage(g_hwndDesktopTB, TB_GETITEMRECT, 0, (LPARAM)&rcDesktop);
        cxDesktop = RECTWIDTH(rcDesktop) + g_cxEdge;
        cyDesktop = RECTHEIGHT(rcDesktop);
    }

    // size to fill either horizontally or vertically
    if ((prcClient->right - g_ts.sizeStart.cx) > (prcClient->bottom - g_ts.sizeStart.cy))
    {
        //
        // Horizontal - Two cases.  One where we have room below the
        // toolbar to display the clock. so display it there, else
        // display it on the right hand side...
        //
        yFSView = 0;
        cyFSView = prcClient->bottom - yFSView;

        // Recalc the min size for horizontal arrangement
        dwClockMinSize = SendMessage(g_ts.hwndNotify, WM_CALCMINSIZE,
                                     0x7fff, prcClient->bottom);
        cxClock = LOWORD(dwClockMinSize);
        cyClock = HIWORD(dwClockMinSize);

        // xClock = prcClient->right - cxClock - g_cxFrame;  // A little gap...
        xClock = prcClient->right - cxClock - cxDesktop;
#ifdef VCENTER_CLOCK
        yClock = (prcClient->bottom-cyClock)/2;     // Center it vertically.
#else
        yClock = 0;
#endif

        xFSView = g_ts.sizeStart.cx + g_cxFrame + 1; // NT5 VFREEZE: one more pixel here
        cxFSView = xClock - xFSView - g_cxFrame/2; // NT5 VFREEZE: we removed the right etch, so reclaim the space


    }
    else
    {
        // Vertical - Again two cases.  One where we have room to the
        // right to display the clock.  Note: we want some gap here between
        // the clock and toolbar.  If it does not fit, then we will center
        // it at the bottom...
        xFSView = 0;
        cxFSView = prcClient->right - xFSView;

        dwClockMinSize = SendMessage(g_ts.hwndNotify, WM_CALCMINSIZE,
                0x7fff, g_ts.sizeStart.cy);
        cxClock = LOWORD(dwClockMinSize);
        cyClock = HIWORD(dwClockMinSize);

        if ((g_ts.sizeStart.cx + cxClock + 4 * g_cyTabSpace + cxDesktop)  < prcClient->right)
        {
            int cyMax = g_ts.sizeStart.cy;;

            // Can fit on the same row!
            xClock = prcClient->right - cxClock - (2 * g_cxEdge) - cxDesktop;  // A little gap...
            yClock = 0;

            if (cyClock > cyMax)
                cyMax = cyClock;

            yFSView = cyMax + g_cyTabSpace;
            cyFSView = prcClient->bottom - yFSView;
        }
        else
        {
            // Nope put at bottom

            // Recalc the min size for vertical arrangement
            dwClockMinSize = SendMessage(g_ts.hwndNotify, WM_CALCMINSIZE,
                cxFSView, 0x7fff);

            if (LOWORD(dwClockMinSize) + cyDesktop >= prcClient->right)
                fDesktopBottom = TRUE;

            if (fDesktopBottom) {
                cxClock = min(LOWORD(dwClockMinSize), prcClient->right);
            } else {
                cxClock = LOWORD(dwClockMinSize);
            }

            cyClock = HIWORD(dwClockMinSize);

            xClock = (prcClient->right - cxClock) / 2;
            yClock = prcClient->bottom - cyClock - g_cyTabSpace;
            // if we'ren ot slowing the clock, we still need to make
            // room for hte desktop button
            if (!cyClock)
                yClock -= (cyDesktop - g_cyEdge);

            if (fDesktopBottom) {
                yClock -= cyDesktop;
            } else {
                xClock -= (cxDesktop/2);
            }

            yFSView = g_ts.sizeStart.cy + g_cyTabSpace;
            cyFSView = yClock - yFSView - g_cyTabSpace;
        }
    }


    prcView->left = xFSView;
    prcView->top = yFSView;
    prcView->right = xFSView + cxFSView;
    prcView->bottom = yFSView + cyFSView;

    prcClock->left = xClock;
    prcClock->top = yClock;
    prcClock->right = xClock + cxClock;
    prcClock->bottom = yClock + cyClock;

    if (fDesktopBottom) {
        prcDesktop->top = prcClock->bottom + g_cyEdge;
        prcDesktop->left = (prcClient->right - cxDesktop)/2;
    } else {
        *prcDesktop = *prcClock;
        prcDesktop->left = prcClock->right + g_cxEdge;
    }
    prcDesktop->bottom = prcDesktop->top + cyDesktop;
    prcDesktop->right = prcDesktop->left + cxDesktop + g_cxEdge;
}

void Tray_RestoreWindowPos()
{
    WINDOWPLACEMENT wp;

    //first restore the stuck postitions
    Tray_GetSaveStateAndInitRects();

    wp.length = SIZEOF(wp);
    wp.showCmd = SW_HIDE;

    g_ts.uMoveStuckPlace = (UINT)-1;
    Tray_GetDockedRect(&wp.rcNormalPosition, FALSE);

    ClockCtl_HandleTrayHide(g_ts.fHideClock);
    SetWindowPlacement(v_hwndTray, &wp);
}

//----------------------------------------------------------------------------
// Sort of a registry equivalent of the profile API's.
BOOL Reg_GetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, void *pData, DWORD *pcbData)
{
    BOOL fRet = FALSE;
 
    if (!g_fCleanBoot)
    {
        fRet = ERROR_SUCCESS == SHGetValue(hkey, pszSubKey, pszValue, NULL, pData, pcbData);
    }

    return fRet;
}

//----------------------------------------------------------------------------
// Sort of a registry equivalent of the profile API's.
BOOL Reg_SetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, void *lpData, DWORD cbData)
{
    HKEY hkeyNew = hkey;
    BOOL fRet = FALSE;

    if (pszSubKey)
    {
        if (RegCreateKey(hkey, pszSubKey, &hkeyNew) != ERROR_SUCCESS)
        {
            return fRet;
        }
    }

    if (RegSetValueEx(hkeyNew, pszValue, 0, REG_BINARY, lpData, cbData) == ERROR_SUCCESS)
    {
        fRet = TRUE;
    }

    if (pszSubKey)
        RegCloseKey(hkeyNew);

    return fRet;
}

//----------------------------------------------------------------------------
// Get the display (monitor) rectangle from the given arbitrary point
HMONITOR Tray_GetDisplayRectFromPoint(LPRECT prcDisplay, POINT pt, UINT uFlags)
{
    HMONITOR hmon;
    RECT rcEmpty = {0};
    hmon = MonitorFromPoint(pt, uFlags);
    if (hmon && prcDisplay)
        GetMonitorRect(hmon, prcDisplay);
    else if (prcDisplay)
        *prcDisplay = rcEmpty;

    return hmon;
}

//----------------------------------------------------------------------------
// Get the display (monitor) rectangle from the given arbitrary rectangle
HMONITOR Tray_GetDisplayRectFromRect(LPRECT prcDisplay, LPCRECT prcIn,
    UINT uFlags)
{
    HMONITOR hmon;
    RECT rcEmpty = {0};
    hmon = MonitorFromRect(prcIn, uFlags);

    if (hmon && prcDisplay)
        GetMonitorRect(hmon, prcDisplay);
    else if (prcDisplay)
        *prcDisplay = rcEmpty;

    return hmon;
}
//----------------------------------------------------------------------------
// Get the display (monitor) rectangle where the taskbar is currently on,
// if that monitor is invalid, get the nearest one.
void Tray_GetStuckDisplayRect(UINT uStuckPlace, LPRECT prcDisplay)
{
    BOOL fValid;
    ASSERT(prcDisplay);
    fValid = GetMonitorRect(g_ts.hmonStuck, prcDisplay);

    if (!fValid)
        Tray_GetDisplayRectFromRect(prcDisplay, &g_ts.arStuckRects[uStuckPlace], MONITOR_DEFAULTTONEAREST);
}

//----------------------------------------------------------------------------
// Snap a StuckRect to the edge of a containing rectangle
// fClip determines whether to clip the rectangle if it's off the display or move it onto the screen
void Tray_MakeStuckRect(LPRECT prcStick, LPCRECT prcBound, SIZE size,
                        UINT uStick)
{
    CopyRect(prcStick, prcBound);
    InflateRect(prcStick, g_cxEdge, g_cyEdge);

    if (size.cx < 0) size.cx *= -1;
    if (size.cy < 0) size.cy *= -1;

    switch (uStick)
    {
    case STICK_LEFT:   prcStick->right  = (prcStick->left   + size.cx); break;
    case STICK_TOP:    prcStick->bottom = (prcStick->top    + size.cy); break;
    case STICK_RIGHT:  prcStick->left   = (prcStick->right  - size.cx); break;
    case STICK_BOTTOM: prcStick->top    = (prcStick->bottom - size.cy); break;
    }
}

/*-------------------------------------------------------------------
** the screen size has changed, so the docked rectangles need to be
** adjusted to the new screen.
**-------------------------------------------------------------------*/
void ResizeStuckRects(RECT *arStuckRects)
{
    UINT uStick;
    RECT rcDisplay;
    Tray_GetStuckDisplayRect(g_ts.uStuckPlace, &rcDisplay);
    for (uStick = STICK_LEFT; uStick <= STICK_BOTTOM; uStick++)
        Tray_MakeStuckRect(&arStuckRects[uStick], &rcDisplay, g_ts.sStuckWidths, uStick);
}


void Tray_UpdateDockingFlags()
{
#ifndef WINNT
    BOOL fIoSuccess;
    BIOSPARAMS bp;
    DWORD cbOut;
#endif
    static BOOL fFirstTime=TRUE;

    if ((!fFirstTime) && (g_ts.hBIOS == INVALID_HANDLE_VALUE)) {
        return;
    }
#ifdef WINNT
    g_ts.hBIOS = INVALID_HANDLE_VALUE;
    fFirstTime = FALSE;
    return;

#else

    g_ts.hBIOS = CreateFile(TEXT("\\\\.\\BIOS"),
        GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);

    if (g_ts.hBIOS == INVALID_HANDLE_VALUE) {
        fFirstTime = FALSE;
        return;
    }

    bp.bp_ret=0;

    fIoSuccess=DeviceIoControl(
        g_ts.hBIOS,
        PNPBIOS_SERVICE_GETDOCKCAPABILITIES,
        &bp,
        SIZEOF(bp),
        &bp,
        SIZEOF(bp),
        &cbOut,
        NULL);

    if (fFirstTime) {

        // first time through, if a warm dock change is not possible,
        // don't bother with the menu item.

        fFirstTime=FALSE;

        if (!fIoSuccess) {

            // problem getting the dock capabilities:
            // if problem wasn't "undocked now" or "can't identify dock"
            // then warm ejecting isn't possible.

            if ((bp.bp_ret!=PNPBIOS_ERR_SYSTEM_NOT_DOCKED) &&
                (bp.bp_ret!=PNPBIOS_ERR_CANT_DETERMINE_DOCKING)) {
                CloseHandle(g_ts.hBIOS);
                g_ts.hBIOS=INVALID_HANDLE_VALUE;
                return;
            }
        } else {

            // success getting the dock capabilities:
            // if the dock isn't capable of warm or hot docking
            // then warm ejecting isn't possible

            if (!(bp.bp_ret&PNPBIOS_DOCK_CAPABILITY_TEMPERATURE)) {
                CloseHandle(g_ts.hBIOS);
                g_ts.hBIOS=INVALID_HANDLE_VALUE;
                return;
            }
        }
    }

    // on each call we update WARMEJECTABLENOW
    // depending on whether the dock is capable of warm ejecting right now

    if ((fIoSuccess) && (bp.bp_ret&PNPBIOS_DOCK_CAPABILITY_TEMPERATURE)) {
        g_ts.fDockingFlags|=DOCKFLAG_WARMEJECTABLENOW;
    } else {
        g_ts.fDockingFlags&=~DOCKFLAG_WARMEJECTABLENOW;
    }

    return;
#endif
}

/* Check if system is currently in a docking station.
 *
 * Returns: TRUE if docked, FALSE if undocked or can't tell.
 *
 */

// BUGBUG (lamadio): This should be moved into it's own file with the other
// API and ACPI routines.
BOOL GetDockedState(void)
{
#ifndef WINNT                       // BUGBUG - Fix this when NT gets docking capability
    struct _ghwpi {                 // Get_Hardware_Profile_Info parameter blk
        CMAPI   cmApi;
        ULONG   ulIndex;
        PFARHWPROFILEINFO pHWProfileInfo;
        ULONG   ulFlags;
        HWPROFILEINFO HWProfileInfo;
    } *pghwpi;

    HANDLE hCMHeap;
    UINT Result = DOCKSTATE_UNKNOWN;
    DWORD dwRecipients = BSM_VXDS;

#define HEAP_SHARED     0x04000000      /* put heap in shared memory--undoc'd */

    // Create a shared heap for CONFIGMG parameters

    if ((hCMHeap = HeapCreate(HEAP_SHARED, 1, 4096)) == NULL)
        return DOCKSTATE_UNKNOWN;

#undef HEAP_SHARED

    // Allocate parameter block in shared memory

    pghwpi = (struct _ghwpi *)HeapAlloc(hCMHeap, HEAP_ZERO_MEMORY,
                                        SIZEOF(*pghwpi));
    if (pghwpi == NULL)
    {
        HeapDestroy(hCMHeap);
        return DOCKSTATE_UNKNOWN;
    }

    pghwpi->cmApi.dwCMAPIRet     = 0;
    pghwpi->cmApi.dwCMAPIService = GetVxDServiceOrdinal(_CONFIGMG_Get_Hardware_Profile_Info);
    pghwpi->cmApi.pCMAPIStack    = (DWORD)(((LPBYTE)pghwpi) + SIZEOF(pghwpi->cmApi));
    pghwpi->ulIndex              = 0xFFFFFFFF;
    pghwpi->pHWProfileInfo       = &pghwpi->HWProfileInfo;
    pghwpi->ulFlags              = 0;

    // "Call" _CONFIGMG_Get_Hardware_Profile_Info service

    BroadcastSystemMessage(0, &dwRecipients, WM_DEVICECHANGE, DBT_CONFIGMGAPI32,
                           (LPARAM)pghwpi);

    if (pghwpi->cmApi.dwCMAPIRet == CR_SUCCESS) {

        switch (pghwpi->HWProfileInfo.HWPI_dwFlags) {

            case CM_HWPI_DOCKED:
                Result = DOCKSTATE_DOCKED;
                break;

            case CM_HWPI_UNDOCKED:
                Result = DOCKSTATE_UNDOCKED;
                break;

            default:
                Result = DOCKSTATE_UNKNOWN;
                break;

        }

    }

    HeapDestroy(hCMHeap);

    return Result;
#else
    return(DOCKSTATE_DOCKED);
#endif
}

int g_cHided;

#ifdef DEBUG
BOOL g_dbNoShow;
#endif

void Tray_Hide();

//***   TrayShowWindow -- temporary 'invisible' un-autohide
// DESCRIPTION
//  various tray resize routines need the tray to be un-autohide'd for
// stuff to be calculated correctly.  so we un-autohide it (invisibly...)
// here.  note the WM_SETREDRAW to prevent flicker (nt5:182340).
//  note that this is kind of a hack -- ideally the tray code would do
// stuff correctly even if hidden.
// NOTES
//  separate routine (vs. inline) to aid debugging.  turn this func off
// to 'see' resize stuff happening
//  BUGBUG what happens if rehide kicks off before we're done?
void TrayShowWindow(int nCmdShow)
{
    if (nCmdShow == SW_HIDE) {
        if (g_cHided++ == 0) {
#ifdef DEBUG
            if (!g_dbNoShow)
#endif
            SendMessage(v_hwndTray, WM_SETREDRAW, FALSE, 0);
            ShowWindow(v_hwndTray, nCmdShow);
            Tray_Unhide();
        }
    }
    else if (nCmdShow == SW_SHOWNA) {
        ASSERT(g_cHided > 0);       // must be push/pop
        if (--g_cHided == 0) {
            Tray_Hide();
#ifdef DEBUG
            if (!g_dbNoShow)
#endif
            ShowWindow(v_hwndTray, nCmdShow);
            SendMessage(v_hwndTray, WM_SETREDRAW, TRUE, 0);
        }
    }
    else {
        ASSERT(0);
    }

    return;
}

void Tray_VerifySize(BOOL fWinIni)
{
    RECT rc;
    BOOL fHiding;

    fHiding = (g_ts.uAutoHide & AH_HIDING);
    if (fHiding) {
        // force it visible so various calculations will happen relative
        // to unhidden size/position.
        //
        // fixes (e.g.) ie5:154536, where dropping a large-icon ISFBand
        // onto hidden tray didn't do size negotiation.
        //
        // BUGBUG what happens if rehide kicks off before we're done?.
        TrayShowWindow(SW_HIDE);
    }

    rc = g_ts.arStuckRects[g_ts.uStuckPlace];
    Tray_HandleSizing(0, NULL, g_ts.uStuckPlace);

    //
    // if the old view had a height, and now it won't...
    // push it up to at least one height
    //
    // do this only on win ini if we're on the top or bottom
    if (fWinIni && STUCK_HORIZONTAL(g_ts.uStuckPlace)) {
        RECT rcView;
        // stash the old view size;
        GetClientRect(g_ts.hwndView, &rcView);

        if (RECTHEIGHT(rcView) && (RECTHEIGHT(rc) == g_cyTrayBorders)) {
            int cyOneRow = g_cySize + 2 * g_cyEdge;
            if (g_ts.uStuckPlace == STICK_TOP) {
                rc.bottom = rc.top + cyOneRow;
            } else {
                rc.top = rc.bottom - cyOneRow;
            }

            // now snap this size.
            Tray_HandleSizing(0, NULL, g_ts.uStuckPlace);
        }
    }

    if (!EqualRect(&rc, &g_ts.arStuckRects[g_ts.uStuckPlace]))
    {
        if (fWinIni) {
            // if we're changing size or position, we need to be unhidden
            Tray_Unhide();
            Tray_SizeWindows();
        }
        rc = g_ts.arStuckRects[g_ts.uStuckPlace];

        if ((g_ts.uAutoHide & (AH_ON | AH_HIDING)) != (AH_ON | AH_HIDING))
        {
            g_ts.fSelfSizing = TRUE;
            SetWindowPos(g_ts.hwndMain, NULL,
                rc.left, rc.top,
                RECTWIDTH(rc),RECTHEIGHT(rc),
                SWP_NOZORDER | SWP_NOACTIVATE);

            g_ts.fSelfSizing = FALSE;
        }
        else
            ASSERT(0);      // tmp unhide above

        Tray_StuckTrayChange();
    }

    if (fWinIni)
        Tray_SizeWindows();

    if (fHiding) {
        TrayShowWindow(SW_SHOWNA);
    }
}

//----------------------------------------------------------------------------
TCHAR const c_szCheckAssociations[] = TEXT("CheckAssociations");

//----------------------------------------------------------------------------
// Returns true if GrpConv says we should check extensions again (and then
// clears the flag).
// The assumption here is that runonce gets run before we call this (so
// GrpConv -s can set this).
BOOL Tray_CheckAssociations(void)
{
    DWORD dw = 0;
    DWORD cb = SIZEOF(dw);

    if (Reg_GetStruct(g_hkeyExplorer, NULL, c_szCheckAssociations,
        &dw, &cb) && dw)
    {
        dw = 0;
        Reg_SetStruct(g_hkeyExplorer, NULL, c_szCheckAssociations, &dw, SIZEOF(dw));
        return TRUE;
    }

    return FALSE;
}

ULONG _RegisterNotify(HWND hwnd, UINT nMsg, LPITEMIDLIST pidl, BOOL fRecursive );


//----------------------------------------------------------------------------
void Tray_RegisterDesktopNotify()
{
    LPITEMIDLIST pidl;

    TraceMsg(TF_TRAY, "c.rdn: Notify for desktop.");

    if (!g_ts.uDesktopNotify)
    {
        pidl = SHCloneSpecialIDList(NULL, CSIDL_DESKTOPDIRECTORY, TRUE);
        if (pidl)
        {
            g_ts.uDesktopNotify = _RegisterNotify(g_ts.hwndMain, WMTRAY_DESKTOPCHANGE, pidl, FALSE);
            ILFree(pidl);
        }
    }

    if (!SHRestricted(REST_NOCOMMONGROUPS) && !g_ts.uCommonDesktopNotify)
    {
        pidl = SHCloneSpecialIDList(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, TRUE);
        if (pidl)
        {
            g_ts.uCommonDesktopNotify = _RegisterNotify(g_ts.hwndMain, WMTRAY_DESKTOPCHANGE, pidl, FALSE);
            ILFree(pidl);
        }
    }
}

HWND Tray_GetClockWindow(void)
{
    return (HWND)SendMessage(g_ts.hwndNotify, TNM_GETCLOCK, 0, 0L);
}

UINT Tray_GetStartIDB()
{
    UINT id;

#ifdef WINNT
    if (IsOS(OS_TERMINALCLIENT))
    {
        id = IDB_TERMINALSERVICESBKG;
    }
    else if (IsOS(OS_WIN2000DATACENTER))
    {
        id = IDB_DCSERVERSTARTBKG;
    }
    else if (IsOS(OS_SERVERAPPLIANCE))
    {
        id = IDB_SRVAPPSTARTBKG;
    }
    else if (IsOS(OS_WIN2000ADVSERVER))
    {
        id = IDB_ADVSERVERSTARTBKG;
    }
    else if (IsOS(OS_WIN2000SERVER))
    {
        id = IDB_SERVERSTARTBKG;
    }
    else if (IsOS(OS_WIN2000EMBED))
    {
        id = IDB_EMBEDDED;
    }
    else
    {
        id = IDB_STARTBKG;
    }
#else
    if (IsOS(OS_MEMPHIS_GOLD))
    {
        id = IDB_STARTBKG;
    }
    else
    {
        id = IDB_START95BK;
    }
#endif

    return id;
}

void Tray_CreateTrayTips()
{
    g_ts.hwndTrayTips = CreateWindow(TOOLTIPS_CLASS, NULL,
                                     WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     NULL, NULL, hinstCabinet,
                                     NULL);

    SetWindowZorder(g_ts.hwndTrayTips, HWND_TOPMOST);

    if (g_ts.hwndTrayTips)
    {
        HWND hwndClock;
        TOOLINFO ti;

        ti.cbSize = SIZEOF(ti);
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd = v_hwndTray;
        ti.uId = (UINT_PTR)g_ts.hwndStart;
        ti.lpszText = (LPTSTR)MAKEINTRESOURCE(IDS_STARTBUTTONTIP);
        ti.hinst = hinstCabinet;
        SendMessage(g_ts.hwndTrayTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        if (NULL != (hwndClock = Tray_GetClockWindow()))
        {
            ti.uFlags = 0;
            ti.uId = (UINT_PTR)hwndClock;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;
            SendMessage(g_ts.hwndTrayTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        }
    }
}

IUnknown*  Tray_CreateView();

LRESULT Tray_CreateWindows()
{
    if (Tray_CreateStartButton())
    {
        if (Tray_CreateClockWindow())
        {
            //
            //  We need to set the tray position, before creating
            // the view window, because it will call back our
            // GetWindowRect member functions.
            //
            Tray_RestoreWindowPos();

            Tray_CreateTrayTips();

            g_ts.ptbs = Tray_CreateView();

            SendMessage(g_ts.hwndNotify, TNM_HIDECLOCK, 0, g_ts.fHideClock);

            if (g_ts.ptbs)
            {
                Tray_CreateDesktopButton();

                Tray_VerifySize(FALSE);
                Tray_SizeWindows();      // size after all windows created

                return 1;
            }
        }
    }

    return -1;
}


LRESULT Tray_InitStartButtonEtc()
{
    // NOTE: This bitmap is used as a flag in CTaskBar::OnPosRectChangeDB to
    // tell when we are done initializing, so we don't resize prematurely
    g_ts.hbmpStartBkg = LoadBitmap(hinstCabinet, MAKEINTRESOURCE(Tray_GetStartIDB()));

    if (g_ts.hbmpStartBkg)
    {
        UpdateWindow(v_hwndTray);
        StartMenu_Build();
        CStartDropTarget_Register();
        PrintNotify_Init(v_hwndTray);

        if (Tray_CheckAssociations())
            CheckWinIniForAssocs();

        Tray_RegisterDesktopNotify();

        SendNotifyMessage(HWND_BROADCAST,
                          RegisterWindowMessage(TEXT("TaskbarCreated")), 0, 0);
        return 1;
    }

    return -1;
}

void Tray_AdjustMinimizedMetrics()
{
    MINIMIZEDMETRICS mm;

    mm.cbSize = SIZEOF(mm);
    SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, SIZEOF(mm), &mm, FALSE);
    mm.iArrange |= ARW_HIDE;
    SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, SIZEOF(mm), &mm, FALSE);
}

LRESULT Tray_OnCreateAsync()
{
    LRESULT lres;

    if (g_dwProfileCAP & 0x00000004)
    {
        StartCAP();
    }

    lres = Tray_InitStartButtonEtc();

    if (g_dwProfileCAP & 0x00000004)
    {
        StopCAP();
    }

    g_ts.hMainAccel = LoadAccelerators(hinstCabinet, MAKEINTRESOURCE(ACCEL_TRAY));

    Tray_RegisterGlobalHotkeys();

    HotkeyList_Restore(v_hwndTray);

    // we run the tray thread that handles Ctrl-Esc with a high priority
    // class so that it can respond even on a stressed system.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    return lres;
}

LRESULT Tray_OnCreate(HWND hwnd)
{
    LRESULT lres;

    v_hwndTray = hwnd;

    SendMessage(v_hwndTray, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);

    Tray_AdjustMinimizedMetrics();
    Tray_UpdateDockingFlags();
    lres = Tray_CreateWindows();

    return lres;
}

void Tray_DestroyShellView()
{
    DestroyWindow(g_ts.hwndView);
    g_ts.hwndView = NULL;
}

BOOL CALLBACK Tray_FullScreenEnumCallback(HMONITOR hmon, HDC hdc, LPRECT prc, LPARAM dwData)
{
    BOOL fFullScreen;   // Is there a rude app on this monitor?

    LPRECT prcRude = (LPRECT)dwData;
    if (prcRude)
    {
        RECT rc, rcMon;
        GetMonitorRect(hmon, &rcMon);
        IntersectRect(&rc, &rcMon, prcRude);
        fFullScreen = EqualRect(&rc, &rcMon);
    }
    else
    {
        fFullScreen = FALSE;
    }

    if (hmon == g_ts.hmonStuck)
    {
        g_ts.fStuckRudeApp = fFullScreen;
    }

    //
    // Tell all the appbars on the same display to get out of the way too
    //
    AppBarNotifyAll(hmon, ABN_FULLSCREENAPP, NULL, fFullScreen);

    return TRUE;
}

void Tray_HandleFullScreenApp(HWND hwnd)
{
    //
    // First check to see if something has actually changed
    //
    if (g_ts.hwndRude != hwnd)
    {
        g_ts.hwndRude = hwnd;

        //
        // Enumerate all the monitors, see if the app is rude on each, adjust
        // app bars and g_ts.fStuckRudeApp as necessary.  (Some rude apps, such
        // as the NT Logon Screen Saver, span multiple monitors.)
        //
        {
            LPRECT prc;
            RECT rc;
            if (hwnd && GetWindowRect(hwnd, &rc))
            {
                prc = &rc;
            }
            else
            {
                prc = NULL;
            }

            EnumDisplayMonitors(NULL, NULL, Tray_FullScreenEnumCallback, (LPARAM)prc);
        }

        //
        // Now that we've set g_ts.fStuckRudeApp, update the tray's z-order position
        //
        Tray_ResetZorder();

        //
        // stop the clock so we don't eat cycles and keep tons of code paged in
        //
        ClockCtl_HandleTrayHide(g_ts.fStuckRudeApp);

        //
        // Finally, let traynot know about whether the tray is hiding
        //
        SendMessage(g_ts.hwndNotify, TNM_RUDEAPP, g_ts.fStuckRudeApp, 0);
    }
}

BOOL Tray_IsTopmost()
{
    return BOOLIFY(GetWindowLong(v_hwndTray, GWL_EXSTYLE) & WS_EX_TOPMOST);
}

BOOL Tray_IsStartMenuVisible()
{
    HWND hwnd;
    if (SUCCEEDED(IUnknown_GetWindow((IUnknown*)g_ts._pmpStartMenu, &hwnd)))
    {
        return IsWindowVisible(hwnd);
    }

    return FALSE;
}

BOOL Tray_IsActive()
{
    //
    // We say the tray is "active" iff:
    //
    // (a) the foreground window is the tray or a window owned by the tray, or
    // (b) the start menu is showing
    //

    BOOL fActive = FALSE;
    HWND hwnd = GetForegroundWindow();

    if (hwnd != NULL &&
        (hwnd == v_hwndTray || (GetWindowOwner(hwnd) == v_hwndTray)))
    {
        fActive = TRUE;
    }
    else if (Tray_IsStartMenuVisible())
    {
        fActive = TRUE;
    }

    return fActive;
}

void Tray_ResetZorder()
{
    HWND hwndZorder, hwndZorderCurrent;

    if (g_fDesktopRaised || (g_ts.fAlwaysOnTop && !g_ts.fStuckRudeApp))
    {
        hwndZorder = HWND_TOPMOST;
    }
    else if (Tray_IsActive())
    {
        hwndZorder = HWND_TOP;
    }
    else if (g_ts.fStuckRudeApp)
    {
        hwndZorder = HWND_BOTTOM;
    }
    else
    {
        hwndZorder = HWND_NOTOPMOST;
    }

    //
    // We don't have to worry about the HWND_BOTTOM current case -- it's ok
    // to keep moving ourselves down to the bottom when there's a rude app.
    //
    // Nor do we have to worry about the HWND_TOP current case -- it's ok
    // to keep moving ourselves up to the top when we're active.
    //
    hwndZorderCurrent = Tray_IsTopmost() ? HWND_TOPMOST : HWND_NOTOPMOST;

    if (hwndZorder != hwndZorderCurrent)
    {
        // only do this if somehting has changed.
        // this keeps us from popping up over menus as desktop async
        // notifies us of it's state

        SHForceWindowZorder(v_hwndTray, hwndZorder);
    }
}

DWORD CALLBACK Tray_SyncThreadProc(void *hInst);
DWORD CALLBACK Tray_MainThreadProc(void *hInst);

void Tray_MessageLoop()
{
    for (;;)
    {
        MSG  msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                if (v_hwndTray && IsWindow(v_hwndTray))
                {
                    // Tell the tray to save everything off if we got here
                    // without it being destroyed.
                    SendMessage(v_hwndTray, WM_ENDSESSION, 1, 0);
                }
                return;  // break all the way out of the main loop
            }

            if (g_ts._pmbStartMenu &&
                g_ts._pmbStartMenu->lpVtbl->IsMenuMessage(g_ts._pmbStartMenu, &msg) == S_OK)
            {
                continue;
            }

            if (g_ts.hMainAccel && TranslateAccelerator(g_ts.hwndMain, g_ts.hMainAccel, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            WaitMessage();
        }
    }
}

BOOL InitTray(HINSTANCE hInst )
{
    // put the tray on a separate thread
    return SHCreateThread(Tray_MainThreadProc, hInst, 0, Tray_SyncThreadProc);
}

void Tray_InitBandsite()
{
    ASSERT(v_hwndTray);

    // we initilize the contents after all the infrastructure is created and sized properly
    // need to notify which side we're on.
    // nt5:211881: set mode *before* load, o.w. Update->RBAutoSize screwed up
    BandSite_SetMode(g_ts.ptbs, STUCK_HORIZONTAL(g_ts.uStuckPlace) ? 0 : DBIF_VIEWMODE_VERTICAL);
    BandSite_Load();
    // now that the mode is set, we need to force an update because we
    // explicitly avoided the update during BandSite_Load
    BandSite_Update(g_ts.ptbs);
    BandSite_UIActivateDBC(g_ts.ptbs, DBC_SHOW);
}

void Tray_KickStartAutohide()
{
    if (g_ts.uAutoHide & AH_ON)
    {
        // tray always starts out hidden on autohide
        g_ts.uAutoHide = AH_ON | AH_HIDING;

        // we and many apps rely upon us having calculated the size correctly
        Tray_Unhide();

        // register it
        if (!IAppBarSetAutoHideBar(v_hwndTray, TRUE, g_ts.uStuckPlace))
        {
            // don't bother putting up UI in this case
            // if someone is there just silently convert to normal
            // (the shell is booting who would be there anyway?)
            Tray_SetAutoHideState(FALSE);
        }
    }
}

void Tray_InitNonzeroGlobals()
{
    // initalize globals that need to be non-zero
    g_ts.wThreadCmd = SMCT_DONE;
    g_ts.hBIOS = INVALID_HANDLE_VALUE;

    if (GetSystemMetrics(SM_SLOWMACHINE))
    {
        g_dtSlideHide = 0;       // dont slide the tray out
        g_dtSlideShow = 0;
    }
    else
    {
        //BUGBUG: we should read from registry.
        g_dtSlideHide = 400;
        g_dtSlideShow = 200;
    }
}

void Tray_CreateTrayWindow(HINSTANCE hinst)
{
    DWORD dwExStyle = WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW;
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    v_hwndTray = CreateWindowEx(dwExStyle,
                                TEXT(WNDCLASS_TRAYNOTIFY), NULL,
                                WS_CLIPCHILDREN | WS_POPUP | WS_BORDER | WS_THICKFRAME,
                                0, 0, 0, 0, NULL, NULL, hinst, NULL);
}

DWORD CALLBACK Tray_SyncThreadProc(void *pv)
{
    MSG msg;

    if (g_dwProfileCAP & 0x00000002)
    {
        StartCAP();
    }

    // make sure the message queue has been created...
    PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );

    OleInitialize( NULL );

    Tray_InitNonzeroGlobals();

    Tray_CreateTrayWindow((HINSTANCE)pv);

    if (!v_hwndTray || !g_ts.ptbs)
    {
        OleUninitialize();
        return FALSE;
    }

    //
    // obey the "always on top" flag
    //
    Tray_ResetZorder();
    Tray_KickStartAutohide();

    Tray_InitBandsite();

    //
    // make sure we clip the taskbar to the current monitor before showing it
    //
    Tray_ClipWindow(TRUE);

    //
    // it looks really dorky for the tray to pop up and rehide at logon
    // if we are autohide don't activate the tray when we show it
    // if we aren't autohide do what Win95 did (tray is active by default)
    //
    ShowWindow(v_hwndTray,
        ((g_ts.uAutoHide & AH_HIDING)? SW_SHOWNA : SW_SHOW));

    UpdateWindow(v_hwndTray);
    Tray_StuckTrayChange();

    SetTimer(v_hwndTray, IDT_HANDLEDELAYBOOTSTUFF, 10 * 1000, NULL);

    if (g_dwProfileCAP & 0x00020000)
    {
        StopCAP();
    }

    return FALSE;
}

// the rest of the thread proc that includes the message loop
DWORD CALLBACK Tray_MainThreadProc(void *pv)
{
    if (!v_hwndTray)
        return FALSE;

    Tray_OnCreateAsync();

    Tray_MessageLoop();

    OleUninitialize();

    return FALSE;
}

#define DM_IANELHK 0


//---------------------------------------------------------------------------
ULONG _RegisterNotify(HWND hwnd, UINT nMsg, LPITEMIDLIST pidl, BOOL fRecursive)
{
    SHChangeNotifyEntry fsne;
    ULONG lReturn;

    fsne.fRecursive = fRecursive;
    fsne.pidl = pidl;

    //
    // Don't watch for attribute changes since we just want the
    // name and icon.  For example, if a printer is paused, we don't
    // want to re-enumerate everything.
    //
    lReturn =  SHChangeNotifyRegister(hwnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
        ((SHCNE_DISKEVENTS | SHCNE_UPDATEIMAGE) & ~SHCNE_ATTRIBUTES), nMsg, 1, &fsne);
    return lReturn;
}

//---------------------------------------------------------------------------
ULONG RegisterNotify(HWND hwnd, UINT nMsg, LPITEMIDLIST pidl)
{
    return _RegisterNotify(hwnd, nMsg, pidl, TRUE);
}

//---------------------------------------------------------------------------
void UnregisterNotify(ULONG nNotify)
{
    if (nNotify)
        SHChangeNotifyDeregister(nNotify);
}

//----------------------------------------------------------------------------
#define HKIF_NULL               0
#define HKIF_CACHED             1
#define HKIF_FREEPIDLS          2

typedef struct
{
    LPITEMIDLIST pidlFolder;
    LPITEMIDLIST pidlItem;
    WORD wGHotkey;
    // BOOL fCached;
    WORD wFlags;
} HOTKEYITEM, *PHOTKEYITEM;


const TCHAR c_szSlashCLSID[] = TEXT("\\CLSID");

//
// like OLE GetClassFile(), but it only works on ProgID\CLSID type registration
// not real doc files or pattern matched files
//

HRESULT _CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid)
{
    TCHAR szProgID[80];
    ULONG cb = SIZEOF(szProgID);
    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szProgID, &cb) == ERROR_SUCCESS)
    {
        TCHAR szCLSID[80];

        lstrcat(szProgID, c_szSlashCLSID);
        cb = SIZEOF(szCLSID);

        if (RegQueryValue(HKEY_CLASSES_ROOT, szProgID, szCLSID, &cb) == ERROR_SUCCESS)
            return SHCLSIDFromString(szCLSID, pclsid);
    }
    return E_FAIL;
}


//----------------------------------------------------------------------------
// this gets hotkeys for files given a folder and a pidls  it is much faster
// than _GetHotkeyFromPidls since it does not need to bind to an IShellFolder
// to interrogate it.  if you have access to the item's IShellFolder, call this
// one, especially in a loop.
//
WORD _GetHotkeyFromFolderItem(LPSHELLFOLDER psf, LPCITEMIDLIST pidl)
{
//    TCHAR szPath[MAX_PATH];
    WORD wHotkey = 0;
    DWORD dwAttrs = SFGAO_LINK;

    //
    // Make sure it is an SFGAO_LINK so we don't load a big handler dll
    // just to get back E_NOINTERFACE...
    //
    if (SUCCEEDED(psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &dwAttrs)) &&
        (dwAttrs & SFGAO_LINK))
    {
        IShellLink * pLink;
        UINT rgfInOut = 0;
        if ( SUCCEEDED( psf->lpVtbl->GetUIObjectOf(psf, NULL, 1, &pidl, &IID_IShellLink, &rgfInOut, &pLink )))
        {
            pLink->lpVtbl->GetHotkey(pLink, &wHotkey);
            pLink->lpVtbl->Release(pLink);
        }
    }

    return wHotkey;
}

//----------------------------------------------------------------------------
UINT HotkeyList_GetFreeItemIndex(void)
{
    int i, cItems;
    PHOTKEYITEM phki;

    ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

    cItems = DSA_GetItemCount(g_ts.hdsaHKI);
    for (i=0; i<cItems; i++)
    {
        phki = DSA_GetItemPtr(g_ts.hdsaHKI, i);
        if (!phki->wGHotkey)
        {
            ASSERT(!phki->pidlFolder);
            ASSERT(!phki->pidlItem);
            break;
        }
    }
    return i;
}

//----------------------------------------------------------------------------
// Weird, Global hotkeys use different flags for modifiers than window hotkeys
// (and hotkeys returned by the hotkey control)
WORD MapHotkeyToGlobalHotkey(WORD wHotkey)
{
    UINT nVirtKey;
    UINT nMod = 0;

    // Map the modifiers.
    if (HIBYTE(wHotkey) & HOTKEYF_SHIFT)
        nMod |= MOD_SHIFT;
    if (HIBYTE(wHotkey) & HOTKEYF_CONTROL)
        nMod |= MOD_CONTROL;
    if (HIBYTE(wHotkey) & HOTKEYF_ALT)
        nMod |= MOD_ALT;
    nVirtKey = LOBYTE(wHotkey);
    return (WORD)((nMod*256) + nVirtKey);
}

//----------------------------------------------------------------------------
// NB This takes a regular window hotkey not a global hotkey (it does
// the convertion for you).
int HotkeyList_Add(WORD wHotkey, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, BOOL fClone)
{
    LPCITEMIDLIST pidl1, pidl2;

    if (wHotkey)
    {
        HOTKEYITEM hki;
        int i = HotkeyList_GetFreeItemIndex();

        ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

        // DebugMsg(DM_IANELHK, "c.hl_a: Hotkey %x with id %d.", wHotkey, i);

        if (fClone)
        {
            pidl1 = ILClone(pidlFolder);
            pidl2 = ILClone(pidlItem);
            hki.wFlags = HKIF_FREEPIDLS;
        }
        else
        {
            pidl1 = pidlFolder;
            pidl2 = pidlItem;
            hki.wFlags = HKIF_NULL;
        }

        hki.pidlFolder = (LPITEMIDLIST)pidl1;
        hki.pidlItem = (LPITEMIDLIST)pidl2;
        hki.wGHotkey = MapHotkeyToGlobalHotkey(wHotkey);
        DSA_SetItem(g_ts.hdsaHKI, i, &hki);
        return i;
    }

    return -1;
}

//----------------------------------------------------------------------------
// NB Cached hotkeys have their own pidls that need to be free but
// regular hotkeys just keep a pointer to pidls used by the startmenu and
// so don't.
int HotkeyList_AddCached(WORD wGHotkey, LPITEMIDLIST pidl)
{
    int i = -1;

    if (wGHotkey)
    {
        LPITEMIDLIST pidlItem = ILClone(ILFindLastID(pidl));

        ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

        if (pidlItem)
        {
            if (ILRemoveLastID(pidl))
            {
                HOTKEYITEM hki;

                i = HotkeyList_GetFreeItemIndex();

                // DebugMsg(DM_IANELHK, "c.hl_ac: Hotkey %x with id %d.", wGHotkey, i);

                hki.pidlFolder = pidl;
                hki.pidlItem = pidlItem;
                hki.wGHotkey = wGHotkey;
                hki.wFlags = HKIF_CACHED | HKIF_FREEPIDLS;
                DSA_SetItem(g_ts.hdsaHKI, i, &hki);
            }
        }
    }

    return i;
}

const TCHAR c_szHotkeys[] = TEXT("Hotkeys");

//----------------------------------------------------------------------------
// NB Must do this before destroying the startmenu since the hotkey list
// uses it's pidls in a lot of cases.
int HotkeyList_Save(void)
{
    int cCached = 0;

    TraceMsg(TF_TRAY, "HotkeyList_Save: Saving global hotkeys.");

    if (EVAL(g_ts.hdsaHKI))
    {
        int i, cItems;
        PHOTKEYITEM phki;
        LPITEMIDLIST pidl;
        int cbData = 0;
        TCHAR szValue[32];
        LPBYTE pData;

        ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

        cItems = DSA_GetItemCount(g_ts.hdsaHKI);
        for (i=0; i<cItems; i++)
        {
            phki = DSA_GetItemPtr(g_ts.hdsaHKI, i);
            // Non cached item?
            if (phki->wGHotkey && !(phki->wFlags & HKIF_CACHED))
            {
                // Yep, save it.
                pidl = ILCombine(phki->pidlFolder, phki->pidlItem);
                if (pidl)
                {
                    cbData = SIZEOF(WORD) + ILGetSize(pidl);
                    pData = LocalAlloc(GPTR, cbData);
                    if (pData)
                    {
                        // DebugMsg(DM_TRACE, "c.hl_s: Saving %x.", phki->wGHotkey );
                        *((LPWORD)pData) = phki->wGHotkey;
                        memcpy(pData+SIZEOF(WORD), pidl, cbData-SIZEOF(DWORD));
                        wsprintf(szValue, TEXT("%d"), cCached);
                        Reg_SetStruct(g_hkeyExplorer, c_szHotkeys, szValue, pData , cbData);
                        cCached++;
                        LocalFree(pData);
                    }
                    ILFree(pidl);
                }
            }
        }
    }

    return cCached;
}


//----------------------------------------------------------------------------
HKEY Reg_EnumValueCreate(HKEY hkey, LPCTSTR pszSubkey)
{
    HKEY hkeyOut;

    if (RegOpenKeyEx(hkey, pszSubkey, 0L, KEY_ALL_ACCESS, &hkeyOut) == ERROR_SUCCESS)
    {
        return hkeyOut;
    }
    return(NULL);
}

//----------------------------------------------------------------------------
int Reg_EnumValue(HKEY hkey, int i, LPBYTE pData, int cbData)
{
    TCHAR szValue[MAX_PATH];
    DWORD cchValue;
    DWORD dw;
    DWORD dwType;

    cchValue = ARRAYSIZE(szValue);
    if (RegEnumValue(hkey, i, szValue, &cchValue, &dw, &dwType, pData,
        &cbData) == ERROR_SUCCESS)
    {
        return cbData;
    }

    return 0;
}

//----------------------------------------------------------------------------
BOOL Reg_EnumValueDestroy(HKEY hkey)
{
    return RegCloseKey(hkey) == ERROR_SUCCESS;
}

//----------------------------------------------------------------------------
int HotkeyList_Restore(HWND hwnd)
{
    int i = 0;
    HKEY hkey;
    LPBYTE pData;
    int cbData;
    WORD wGHotkey;
    int id;
    LPITEMIDLIST pidl;

    // If shdocvw fails to load for some reason, we will GP-fault if we
    // don't validate hdsaHKI.
    if (EVAL(g_ts.hdsaHKI))
    {
        TraceMsg(TF_TRAY, "HotkeyList_Restore: Restoring global hotkeys...");

        hkey = Reg_EnumValueCreate(g_hkeyExplorer, c_szHotkeys);
        if (hkey)
        {
            cbData = Reg_EnumValue(hkey, i, NULL, 0);
            while (cbData)
            {
                pData = LocalAlloc(GPTR, cbData);
                if (pData)
                {
                    Reg_EnumValue(hkey, i, pData, cbData);
                    // Get the hotkey and the pidl components.
                    wGHotkey = *((LPWORD)pData);
                    pidl = ILClone((LPITEMIDLIST)(pData+SIZEOF(WORD)));
                    // DebugMsg(DM_TRACE, "c.hl_r: Restoring %x", wGHotkey);
                    id = HotkeyList_AddCached(wGHotkey, pidl);
                    if (id != -1)
                        Tray_RegisterHotkey(hwnd, id);
                    LocalFree(pData);
                }
                i++;
                cbData = Reg_EnumValue(hkey, i, NULL, 0);
            }
            Reg_EnumValueDestroy(hkey);
            // Nuke the cached stuff.
            RegDeleteKey(g_hkeyExplorer, c_szHotkeys);
        }
    }

    return i;
}

//----------------------------------------------------------------------------
// NB Again, this takes window hotkey not a Global one.
// NB This doesn't delete cached hotkeys.
int HotkeyList_Remove(WORD wHotkey)
{
    if (EVAL(g_ts.hdsaHKI))
    {
        int i, cItems;
        PHOTKEYITEM phki;
        WORD wGHotkey;

        ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

        // DebugMsg(DM_IANELHK, "c.hl_r: Remove hotkey for %x" , wHotkey);

        // Unmap the modifiers.
        wGHotkey = MapHotkeyToGlobalHotkey(wHotkey);
        cItems = DSA_GetItemCount(g_ts.hdsaHKI);
        for (i=0; i<cItems; i++)
        {
            phki = DSA_GetItemPtr(g_ts.hdsaHKI, i);
            if (phki && !(phki->wFlags & HKIF_CACHED) && (phki->wGHotkey == wGHotkey))
            {
                // DebugMsg(DM_IANELHK, "c.hl_r: Invalidating %d", i);
                if (phki->wFlags & HKIF_FREEPIDLS)
                {
                    if (phki->pidlFolder)
                        ILFree(phki->pidlFolder);
                    if (phki->pidlItem)
                        ILFree(phki->pidlItem);
                }
                phki->wGHotkey = 0;
                phki->pidlFolder = NULL;
                phki->pidlItem = NULL;
                phki->wFlags &= ~HKIF_FREEPIDLS;
                return i;
            }
        }
    }

    return -1;
}

//----------------------------------------------------------------------------
// NB This takes a global hotkey.
int HotkeyList_RemoveCached(WORD wGHotkey)
{
    int i, cItems;
    PHOTKEYITEM phki;

    ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

    // DebugMsg(DM_IANELHK, "c.hl_rc: Remove hotkey for %x" , wGHotkey);

    cItems = DSA_GetItemCount(g_ts.hdsaHKI);
    for (i=0; i<cItems; i++)
    {
        phki = DSA_GetItemPtr(g_ts.hdsaHKI, i);
        if (phki && (phki->wFlags & HKIF_CACHED) && (phki->wGHotkey == wGHotkey))
        {
            // DebugMsg(DM_IANELHK, "c.hl_r: Invalidating %d", i);
            if (phki->wFlags & HKIF_FREEPIDLS)
            {
                if (phki->pidlFolder)
                    ILFree(phki->pidlFolder);
                if (phki->pidlItem)
                    ILFree(phki->pidlItem);
            }
            phki->pidlFolder = NULL;
            phki->pidlItem = NULL;
            phki->wGHotkey = 0;
            phki->wFlags &= ~(HKIF_CACHED | HKIF_FREEPIDLS);
            return i;
        }
    }

    return -1;
}


//----------------------------------------------------------------------------
// NB Some (the ones not marked HKIF_FREEPIDLS) of the items in the list of hotkeys
// have pointers to idlists used by the filemenu so they are only valid for
// the lifetime of the filemenu.
BOOL HotkeyList_Create(void)
{
    if (!g_ts.hdsaHKI)
    {
        // DebugMsg(DM_TRACE, "c.hkl_c: Creating global hotkey list.");
        g_ts.hdsaHKI = DSA_Create(SIZEOF(HOTKEYITEM), 0);
    }

    if (g_ts.hdsaHKI)
        return TRUE;

    return FALSE;
}

void StartMenu_AddTask(IShellTaskScheduler* pSystemScheduler, int nFolder, int iPriority)
{
    LPITEMIDLIST pidlFolder;
    IShellHotKey * pshk;

    if (FAILED(CHotKey_Create(&pshk)))
        return;

    SHGetSpecialFolderLocation(NULL, nFolder, &pidlFolder);

    if (pidlFolder)
    {
        LPITEMIDLIST pidlParent = ILClone(pidlFolder);
        if (pidlParent)
        {
            IShellFolder* psf;
            ILRemoveLastID(pidlParent);

            psf = BindToFolder(pidlParent);
            if (psf)
            {
                HRESULT hres;
                IStartMenuTask * psmt;
                BOOL bDesktop = (CSIDL_DESKTOPDIRECTORY == nFolder || CSIDL_COMMON_DESKTOPDIRECTORY == nFolder);

                hres = CoCreateInstance(bDesktop ? &CLSID_DesktopTask : &CLSID_StartMenuTask,
                    NULL, CLSCTX_INPROC, &IID_IStartMenuTask, (void **) &psmt);
                if (SUCCEEDED(hres))
                {
                    DWORD  dwFlags = bDesktop ? 0 : ITSFT_RECURSE;
                    LONG   nMaxRecursion = bDesktop ? 32 : 2 ;

                    // Initialize the task.  Set task priority according to max
                    // folder depth.  We'll assume 32 is a reasonable number.
                    hres = psmt->lpVtbl->InitTaskSFT(psmt, psf, pidlFolder, nMaxRecursion, dwFlags, 32);
                    if (SUCCEEDED(hres))
                    {
                        IRunnableTask * ptask;

                        psmt->lpVtbl->InitTaskSMT(psmt, pshk, iPriority);

                        hres = psmt->lpVtbl->QueryInterface(psmt, &IID_IRunnableTask, (void **)&ptask);
                        if (SUCCEEDED(hres))
                        {
                            // Add it to the scheduler
                            hres = pSystemScheduler->lpVtbl->AddTask(pSystemScheduler, ptask,
                                                              &CLSID_StartMenuTask,
                                                              0, 32);
                            ptask->lpVtbl->Release(ptask);
                        }
                    }
                    psmt->lpVtbl->Release(psmt);
                }

                if (FAILED(hres))
                    TraceMsg(TF_ERROR, "StartMenu_AddTask: failed to create start menu task");

                psf->lpVtbl->Release(psf);
            }
            ILFree(pidlParent);
        }
        ILFree(pidlFolder);
    }

    pshk->lpVtbl->Release(pshk);
}



//----------------------------------------------------------------------------
void StartMenu_Build()
{
    HRESULT hres;

    ATOMICRELEASET(g_ts._pmpStartMenu, IMenuPopup);
    ATOMICRELEASET(g_ts._pmbStartMenu, IMenuBand);

    g_uStartButtonAllowPopup = RegisterWindowMessage(TEXT("StartButtonAllowPopup"));

    hres = StartMenuHost_Create(&g_ts._pmpStartMenu, &g_ts._pmbStartMenu);
    if (SUCCEEDED(hres))
    {
        IBanneredBar* pbb;

        hres = g_ts._pmpStartMenu->lpVtbl->QueryInterface(g_ts._pmpStartMenu,
                                        &IID_IBanneredBar, (void**)&pbb);
        if (SUCCEEDED(hres))
        {
            pbb->lpVtbl->SetBitmap(pbb, g_ts.hbmpStartBkg);
            if (g_ts.fSMSmallIcons)
                pbb->lpVtbl->SetIconSize(pbb, BMICON_SMALL);
            else
                pbb->lpVtbl->SetIconSize(pbb, BMICON_LARGE);

            pbb->lpVtbl->Release(pbb);
        }

        HotkeyList_Create();
    }
    else
        TraceMsg(TF_ERROR, "Could not create StartMenu");
}

//----------------------------------------------------------------------------
void StartMenu_Destroy()
{
    IUnknown_SetSite((IUnknown*)g_ts._pmpStartMenu, NULL);
    ATOMICRELEASET(g_ts._pmpStartMenu, IMenuPopup);
    ATOMICRELEASET(g_ts._pmbStartMenu, IMenuBand);
}

//----------------------------------------------------------------------------
void _ForceStartButtonUp()
{
    MSG msg;
    // don't do that check message pos because it gets screwy with
    // keyboard cancel.  and besides, we always want it cleared after
    // track menu popup is done.
    // do it twice to be sure it's up due to the uDown cycling twice in
    // the subclassing stuff
    // pull off any button downs
    PeekMessage(&msg, g_ts.hwndStart, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
    SendMessage(g_ts.hwndStart, BM_SETSTATE, FALSE, 0);
    SendMessage(g_ts.hwndStart, BM_SETSTATE, FALSE, 0);

    // The user cancelled the Start menu, so a new press of the Windows key
    // should be considered "Reopen the Start Menu" instead of "Cancel the
    // Start Menu and return to the previous window."
    g_hwndPrevFocus = NULL;
}


int Tray_TrackMenu(HMENU hmenu)
{
    TPMPARAMS tpm;
    int iret;

    tpm.cbSize = SIZEOF(tpm);
    GetClientRect(g_ts.hwndStart, &tpm.rcExclude);
    MapWindowPoints(g_ts.hwndStart, NULL, (LPPOINT)&tpm.rcExclude, 2);

    SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);
    iret = TrackPopupMenuEx(hmenu, TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                            tpm.rcExclude.left, tpm.rcExclude.bottom, v_hwndTray, &tpm);
    SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
    return iret;
}

//----------------------------------------------------------------------------
// Keep track of the menu font name and weight so we can redo the startmenu if
// these change before getting notified via win.ini.
BOOL StartMenu_FontChange(void)
{
    NONCLIENTMETRICS ncm;
    static TCHAR lfFaceName[LF_FACESIZE] = TEXT("");
    static long lfHeight = 0;

    ncm.cbSize = SIZEOF(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE ))
    {
        if (!*lfFaceName)
        {
            // DebugMsg(DM_TRACE, "sm_fc: No menu font - initing.");
            lstrcpy(lfFaceName, ncm.lfMenuFont.lfFaceName);
            lfHeight = ncm.lfMenuFont.lfHeight;
            return FALSE;
        }
        else if ((lstrcmpi(ncm.lfMenuFont.lfFaceName, lfFaceName) == 0) &&
            (ncm.lfMenuFont.lfHeight == lfHeight))
        {
            // DebugMsg(DM_TRACE, "sm_fc: Menu font unchanged.");
            return FALSE;
        }
        else
        {
            // DebugMsg(DM_TRACE, "sm_fc: Menu font changed.");
            lstrcpy(lfFaceName, ncm.lfMenuFont.lfFaceName);
            lfHeight = ncm.lfMenuFont.lfHeight;
            return TRUE;
        }
    }
    return FALSE;
}

/*------------------------------------------------------------------
** Respond to a button's pressing by bringing up the appropriate menu.
** Clean up the button depression when the menu is dismissed.
**------------------------------------------------------------------*/
void ToolbarMenu()
{
    RECTL    rcExclude;
    POINTL   ptPop;
    DWORD dwFlags = MPPF_KEYBOARD;      // Assume that we're popuping
                                        // up because of the keyboard
                                        // This is for the underlines on NT5

    if (g_ts.hwndStartBalloon)
    {
        DontShowTheStartButtonBalloonAnyMore();
        ShowWindow(g_ts.hwndStartBalloon, SW_HIDE);
        DestroyStartButtonBalloon();
    }

    SetActiveWindow(v_hwndTray);
    g_ts.bMainMenuInit = TRUE;
    GetClientRect(g_ts.hwndStart, (RECT *)&rcExclude);
    MapWindowRect(g_ts.hwndStart, HWND_DESKTOP, &rcExclude);
    ptPop.x = rcExclude.left;
    ptPop.y = rcExclude.top;

    // Close any Context Menus
    SendMessage(v_hwndTray, WM_CANCELMODE, 0, 0);

    // Is the "Activate" button down (If the buttons are swapped, then it's the
    // right button, otherwise the left button)
    if (GetKeyState(GetSystemMetrics(SM_SWAPBUTTON)?VK_RBUTTON:VK_LBUTTON) < 0)
    {
        dwFlags = 0;    // Then set to the default
    }
    else
    {
        // Since the user has launched the start button by Ctrl-Esc, or some other worldly
        // means, then turn the rect on.
        SendMessage(g_ts.hwndStart, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
            UISF_HIDEFOCUS), 0);
    }


    if (g_ts._pmpStartMenu) {
        g_ts._pmpStartMenu->lpVtbl->Popup(g_ts._pmpStartMenu, &ptPop, &rcExclude, dwFlags);
        TraceMsg(DM_MISC, "e.tbm: dwFlags=%x (0=mouse 1=key)", dwFlags);
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UIINPUT, dwFlags ? UIBL_INPMENU : UIBL_INPMOUSE);
    }
}


//
// can't use SubtractRect sometimes because of inclusion limitations
//
void AppBarSubtractRect(PAPPBAR pab, LPRECT lprc)
{
    switch (pab->uEdge) {
        case ABE_TOP:
            if (pab->rc.bottom > lprc->top)
                lprc->top = pab->rc.bottom;
            break;

        case ABE_LEFT:
            if (pab->rc.right > lprc->left)
                lprc->left = pab->rc.right;
            break;

        case ABE_BOTTOM:
            if (pab->rc.top < lprc->bottom)
                lprc->bottom = pab->rc.top;
            break;

        case ABE_RIGHT:
            if (pab->rc.left < lprc->right)
                lprc->right = pab->rc.left;
            break;
    }
}

void AppBarSubtractRects(HMONITOR hmon, LPRECT lprc)
{
    int i;

    if (!g_ts.hdpaAppBars)
        return;

    i = DPA_GetPtrCount(g_ts.hdpaAppBars);

    while (i--)
    {
        PAPPBAR pab = (PAPPBAR)DPA_GetPtr(g_ts.hdpaAppBars, i);

        //
        // autohide bars are not in our DPA or live on the edge
        // BUGBUG: don't subtract the appbar if it is not always on top????
        // don't subtract the appbar if it's on a different display
        //
        if (hmon == MonitorFromRect(&pab->rc, MONITOR_DEFAULTTONULL))
            AppBarSubtractRect(pab, lprc);
    }
}

#define RWA_NOCHANGE      0
#define RWA_CHANGED       1
#define RWA_BOTTOMMOSTTRAY 2
// BUGBUG: (dli) This is a hack put in because bottommost tray is wierd, once
// it becomes a toolbar, this code should go away.
// In the bottommost tray case, even though the work area has not changed,
// we should notify the desktop.


int RecomputeWorkArea(HWND hwndCause, HMONITOR hmon, LPRECT prcWork)
{
    int iRet = RWA_NOCHANGE;
    MONITORINFO mi;

    //
    // tell everybody that this window changed positions _on_this_monitor_
    // note that this notify happens even if we don't change the work area
    // since it may cause another app to change the work area...
    //
    PostMessage(v_hwndTray, TM_RELAYPOSCHANGED, (WPARAM)hwndCause,
        (LPARAM)hmon);

    //
    // get the current info for this monitor
    // we subtract down from the display rectangle to build the work area
    //
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(hmon, &mi))
    {
        //
        // don't subtract the tray if it is autohide
        // don't subtract the tray if it is not always on top
        // don't subtract the tray if it's on a different display
        //
        if (!(g_ts.uAutoHide & AH_ON) && g_ts.fAlwaysOnTop &&
            (hmon == g_ts.hmonStuck))
        {
            SubtractRect(prcWork, &mi.rcMonitor,
                         &g_ts.arStuckRects[g_ts.uStuckPlace]);
        }
        else
            *prcWork = mi.rcMonitor;

        //
        // now subtract off all the appbars on this display
        //
        AppBarSubtractRects(hmon, prcWork);

        //
        // return whether we changed anything
        //
        if (!EqualRect(prcWork, &mi.rcWork))
            iRet = RWA_CHANGED;
        else if (!(g_ts.uAutoHide & AH_ON) && (!g_ts.fAlwaysOnTop) &&
                 (!IsRectEmpty(&g_ts.arStuckRects[g_ts.uStuckPlace])))
            // NOTE: This is the bottommost case, it only applies for the tray.
            // this should be taken out when bottommost tray becomes toolbar
            iRet = RWA_BOTTOMMOSTTRAY;
    }
    else
    {
        // NOTE: This should never happen, if it does because of USER problem,
        // we just say NO CHANGE!!!!
        ASSERTMSG(FALSE, "GetMonitorInfo should never fail! We should have updated our hmonOld or hmonStuck on time");
        iRet = RWA_NOCHANGE;
    }
    
    return iRet;
}

//
// Nashville's debug USER draws silly "Monitor N CXxCYxBPP" crud on the display
//
#if !defined(WINNT) && defined(DEBUG)
#define FORCE_INVALIDATE_WORKAREA TRUE
#else
#define FORCE_INVALIDATE_WORKAREA FALSE
#endif

void RedrawDesktop(LPRECT prcWork)
{
    // This rect point should always be valid (dli)
    RIP(prcWork);
    
    if (v_hwndDesktop && (FORCE_INVALIDATE_WORKAREA || g_fCleanBoot))
    {
        MapWindowPoints(NULL, v_hwndDesktop, (LPPOINT)prcWork, 2);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac invalidating desktop rect {%d,%d,%d,%d}"), prcWork->left, prcWork->top, prcWork->right, prcWork->bottom);
        RedrawWindow(v_hwndDesktop, prcWork, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
}

void StuckAppChange(HWND hwndCause, LPCRECT prcOld, LPCRECT prcNew, BOOL bTray)
{
    RECT rcWork1, rcWork2;
    HMONITOR hmon1, hmon2 = 0;
    int iChange = 0;

    //
    // BUGBUG:
    // there are cases where we end up setting the work area multiple times
    // we need to keep a static array of displays that have changed and a
    //  reenter count so we can avoid pain of sending notifies to the whole
    //  planet...
    //
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac from AppBar %08X"), hwndCause);

    //
    // see if the work area changed on the display containing prcOld
    //
    if (prcOld)
    {
        if (bTray)
            hmon1 = g_ts.hmonOld;
        else
            hmon1 = MonitorFromRect(prcOld, MONITOR_DEFAULTTONEAREST);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac old pos {%d,%d,%d,%d} on monitor %08X"), prcOld->left, prcOld->top, prcOld->right, prcOld->bottom, hmon1);

        if (hmon1)
        {
            int iret = RecomputeWorkArea(hwndCause, hmon1, &rcWork1);
            if (iret == RWA_CHANGED)
                iChange = 1;
            if (iret == RWA_BOTTOMMOSTTRAY)
                iChange = 4;
        }
    }
    else
        hmon1 = NULL;

    //
    // see if the work area changed on the display containing prcNew
    //
    if (prcNew)
    {
        hmon2 = MonitorFromRect(prcNew, MONITOR_DEFAULTTONULL);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac new pos {%d,%d,%d,%d} on monitor %08X"), prcNew->left, prcNew->top, prcNew->right, prcNew->bottom, hmon2);

        if (hmon2 && (hmon2 != hmon1))
        {
            int iret = RecomputeWorkArea(hwndCause, hmon2, &rcWork2);
            if (iret == RWA_CHANGED)
                iChange |= 2;
            else if (iret == RWA_BOTTOMMOSTTRAY && (!iChange))
                iChange = 4;
        }
    }

    //
    // did the prcOld's display's work area change?
    //
    if (iChange & 1)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac changing work area for monitor %08X"), hmon1);

        // only send SENDWININICHANGE if the desktop has been created (otherwise
        // we will hang the explorer because the main thread is currently blocked)
        SystemParametersInfo(SPI_SETWORKAREA, TRUE, &rcWork1,
                             (iChange == 1 && v_hwndDesktop)? SPIF_SENDWININICHANGE : 0);

        RedrawDesktop(&rcWork1);
    }

    //
    // did the prcOld's display's work area change?
    //
    if (iChange & 2)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sac changing work area for monitor %08X"), hmon2);

        // only send SENDWININICHANGE if the desktop has been created (otherwise
        // we will hang the explorer because the main thread is currently blocked)
        SystemParametersInfo(SPI_SETWORKAREA, TRUE, &rcWork2,
                             v_hwndDesktop ? SPIF_SENDWININICHANGE : 0);

        RedrawDesktop(&rcWork2);
    }

    // only send if the desktop has been created...
    // need to send if it's from the tray or any outside app that causes size change
    // from the tray because autohideness will affect desktop size even if it's not always on top
    if ((bTray || iChange == 4) && v_hwndDesktop )
        SendMessage(v_hwndDesktop, WM_SIZE, 0, 0);
}

void Tray_StuckTrayChange()
{
    // We used to blow off the StuckAppChange when the tray was in autohide
    // mode, since moving or resizing an autohid tray doesn't change the
    // work area.  Now we go ahead with the StuckAppChange in this case
    // too.  The reason is that we can get into a state where the work area
    // size is incorrect, and we want the taskbar to always be self-repairing
    // in this case (so that resizing or moving the taskbar will correct the
    // work area size).

    //
    // pass a NULL window here since we don't want to hand out our window and
    // the tray doesn't get these anyway (nobody cares as long as its not them)
    //
    StuckAppChange(NULL, &g_ts.rcOldTray,
        &g_ts.arStuckRects[g_ts.uStuckPlace], TRUE);

    //
    // save off the new tray position...
    //
    g_ts.rcOldTray = g_ts.arStuckRects[g_ts.uStuckPlace];
}

UINT Tray_RecalcStuckPos(LPRECT prc)
{
    RECT rcDummy;
    POINT pt;

    if (!prc)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_rsp no rect supplied, using window rect"));

        prc = &rcDummy;
        GetWindowRect(v_hwndTray, prc);
    }

    // use the center of the original drag rect as a staring point
    pt.x = prc->left + RECTWIDTH(*prc) / 2;
    pt.y = prc->top + RECTHEIGHT(*prc) / 2;

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_rsp rect is {%d, %d, %d, %d} point is {%d, %d}"), prc->left, prc->top, prc->right, prc->bottom, pt.x, pt.y);

    // reset this so the drag code won't give it preference
    g_ts.uMoveStuckPlace = (UINT)-1;

    // simulate a drag back to figure out where we originated from
    // you may be tempted to remove this.  before you do think about dragging
    // the tray across monitors and then hitting ESC...
    return Tray_CalcDragPlace(pt);
}

/*------------------------------------------------------------------
** the position is changing in response to a move operation.
**
** if the docking status changed, we need to get a new size and
** maybe a new frame style.  change the WINDOWPOS to reflect
** these changes accordingly.
**------------------------------------------------------------------*/
void Tray_DoneMoving(LPWINDOWPOS lpwp)
{
    RECT rc, *prc;

    if (g_ts.uMoveStuckPlace == (UINT)-1)
        return;

    if (g_ts.fSysSizing)
        g_ts._fDeferedPosRectChange = TRUE;

    rc.left   = lpwp->x;
    rc.top    = lpwp->y;
    rc.right  = lpwp->x + lpwp->cx;
    rc.bottom = lpwp->y + lpwp->cy;

    prc = &g_ts.arStuckRects[g_ts.uMoveStuckPlace];

    if (!EqualRect(prc, &rc))
    {
        g_ts.uMoveStuckPlace = Tray_RecalcStuckPos(&rc);
        prc = &g_ts.arStuckRects[g_ts.uMoveStuckPlace];
    }

    // Get the new hmonitor
    g_ts.hmonStuck = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);

    if (g_ts.hwndView)
        Tray_HandleSizing(0, prc, g_ts.uMoveStuckPlace);

    lpwp->x = prc->left;
    lpwp->y = prc->top;
    lpwp->cx = RECTWIDTH(*prc);
    lpwp->cy = RECTHEIGHT(*prc);

    lpwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);

    // if we were autohiding, we need to update our appbar autohide rect
    if (g_ts.uAutoHide & AH_ON)
    {
        // unregister us from the old side
        IAppBarSetAutoHideBar(v_hwndTray, FALSE, g_ts.uStuckPlace);
    }

    // All that work might've changed g_ts.uMoveStuckPlace (since there
    // was a lot of message traffic), so check one more time.
    // Somehow, NT Stress manages to get us in here with an invalid
    // uMoveStuckPlace.
    if (IsValidSTUCKPLACE(g_ts.uMoveStuckPlace))
    {
        // remember the new state
        g_ts.uStuckPlace = g_ts.uMoveStuckPlace;
    }
    g_ts.uMoveStuckPlace = (UINT)-1;

    BandSite_SetMode(g_ts.ptbs, STUCK_HORIZONTAL(g_ts.uStuckPlace) ? 0 : DBIF_VIEWMODE_VERTICAL);
    if ((g_ts.uAutoHide & AH_ON) &&
        !IAppBarSetAutoHideBar(v_hwndTray, TRUE, g_ts.uStuckPlace))
    {
        Tray_AutoHideCollision();
    }
}

UINT Tray_CalcDragPlace(POINT pt)
{
    UINT uPlace = g_ts.uMoveStuckPlace;

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp starting point is {%d, %d}"), pt.x, pt.y);

    //
    // if the mouse is currently over the tray position leave it alone
    //
    if ((uPlace == (UINT)-1) || !PtInRect(&g_ts.arStuckRects[uPlace], pt))
    {
        HMONITOR hmonDrag;
        SIZE screen, error;
        UINT uHorzEdge, uVertEdge;
        RECT rcDisplay, *prcStick;

        //
        // which display is the mouse on?
        //
        hmonDrag = Tray_GetDisplayRectFromPoint(&rcDisplay, pt,
            MONITOR_DEFAULTTOPRIMARY);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp monitor is %08X"), hmonDrag);

        //
        // re-origin at zero to make calculations simpler
        //
        screen.cx =  RECTWIDTH(rcDisplay);
        screen.cy = RECTHEIGHT(rcDisplay);
        pt.x -= rcDisplay.left;
        pt.y -= rcDisplay.top;

        //
        // are we closer to the left or right side of this display?
        //
        if (pt.x < (screen.cx / 2))
        {
            uVertEdge = STICK_LEFT;
            error.cx = pt.x;
        }
        else
        {
            uVertEdge = STICK_RIGHT;
            error.cx = screen.cx - pt.x;
        }

        //
        // are we closer to the top or bottom side of this display?
        //
        if (pt.y < (screen.cy / 2))
        {
            uHorzEdge = STICK_TOP;
            error.cy = pt.y;
        }
        else
        {
            uHorzEdge = STICK_BOTTOM;
            error.cy = screen.cy - pt.y;
        }

        //
        // closer to a horizontal or vertical edge?
        //
        uPlace = ((error.cy * screen.cx) > (error.cx * screen.cy))?
            uVertEdge : uHorzEdge;

        // which StuckRect should we use?
        prcStick = &g_ts.arStuckRects[uPlace];

        //
        // need to recalc stuck rect for new monitor?
        //
        if ((hmonDrag != Tray_GetDisplayRectFromRect(NULL, prcStick,
            MONITOR_DEFAULTTONULL)))
        {
            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp re-snapping rect for new display"));
            Tray_MakeStuckRect(prcStick, &rcDisplay, g_ts.sStuckWidths, uPlace);
        }
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_cdp edge is %d, rect is {%d, %d, %d, %d}"), uPlace, g_ts.arStuckRects[uPlace].left, g_ts.arStuckRects[uPlace].top, g_ts.arStuckRects[uPlace].right, g_ts.arStuckRects[uPlace].bottom);
    ASSERT(IsValidSTUCKPLACE(uPlace));
    return uPlace;
}

//------------------------------------------------------------------
void Tray_HandleMoving(WPARAM wParam, LPRECT lprc)
{
    POINT ptCursor;

    GetCursorPos(&ptCursor);
    g_ts.uMoveStuckPlace = Tray_CalcDragPlace(ptCursor);
    *lprc = g_ts.arStuckRects[g_ts.uMoveStuckPlace];

    Tray_HandleSizing(wParam, lprc, g_ts.uMoveStuckPlace);

}

//------------------------------------------------------------------
// store the tray size when dragging is finished
void Tray_SnapshotStuckRectSize(UINT uPlace)
{
    RECT rcDisplay, *prc = &g_ts.arStuckRects[uPlace];

    //
    // record the width of this stuck rect
    //
    if (STUCK_HORIZONTAL(uPlace))
        g_ts.sStuckWidths.cy = RECTHEIGHT(*prc);
    else
        g_ts.sStuckWidths.cx = RECTWIDTH(*prc);

    //
    // we only present a horizontal or vertical size to the end user
    // so update the StuckRect on the other side of the screen to match
    //
    Tray_GetStuckDisplayRect(uPlace, &rcDisplay);

    uPlace += 2;
    uPlace %= 4;
    prc = &g_ts.arStuckRects[uPlace];

    Tray_MakeStuckRect(prc, &rcDisplay, g_ts.sStuckWidths, uPlace);
}


//------------------------------------------------------------------
// Size the icon area to fill as much of the tray window as it can.
//------------------------------------------------------------------
void Tray_SizeWindows()
{
    RECT rcView, rcClock, rcClient, rcDesktop;
    int fHiding;

    if (!g_ts.hwndRebar || !g_ts.hwndMain || !g_ts.hwndNotify)
        return;

    fHiding = (g_ts.uAutoHide & AH_HIDING);
    if (fHiding)
    {
        TrayShowWindow(SW_HIDE);
    }

    // remember our current size
    Tray_SnapshotStuckRectSize(g_ts.uStuckPlace);

    GetClientRect(g_ts.hwndMain, &rcClient);
    Tray_AlignStartButton();
    Tray_GetWindowSizes(&rcClient, &rcView, &rcClock, &rcDesktop);

    InvalidateRect(g_ts.hwndStart, NULL, TRUE);

    // position the view
    SetWindowPos(g_ts.hwndRebar, NULL, rcView.left, rcView.top,
                 RECTWIDTH(rcView), RECTHEIGHT(rcView),
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // And the clock
    SetWindowPos(g_ts.hwndNotify, NULL, rcClock.left, rcClock.top,
                 RECTWIDTH(rcClock), RECTHEIGHT(rcClock),
                 SWP_NOZORDER | SWP_NOACTIVATE);

    // And the desktop button
    if (g_hwndDesktopTB)
        SetWindowPos(g_hwndDesktopTB, NULL, rcDesktop.left, rcDesktop.top,
                     RECTWIDTH(rcDesktop), RECTHEIGHT(rcDesktop),
                     SWP_NOZORDER | SWP_NOACTIVATE);


    {
        TOOLINFO ti;
        HWND hwndClock = Tray_GetClockWindow();

        ti.cbSize = SIZEOF(ti);
        ti.uFlags = 0;
        ti.hwnd = g_ts.hwndMain;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        ti.uId = (UINT_PTR)hwndClock;
        GetWindowRect(hwndClock, &ti.rect);
        MapWindowPoints(HWND_DESKTOP, g_ts.hwndMain, (LPPOINT)&ti.rect, 2);
        SendMessage(g_ts.hwndTrayTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
    }

    if (fHiding)
    {
        TrayShowWindow(SW_SHOWNA);
    }
}


#define MAX_SIZING_RECUR 7000

void Tray_NegotiateSizing(WPARAM code, PRECT prcClient)
{
    RECT rcView, rcClock, rcDesktop;
    int iHeight;
    UINT uRecur = 0;

    while (uRecur++ < MAX_SIZING_RECUR) // guard against infinite loop
    {
        // given this sizing, what would our children's sizes be?
        // use Client coords
        prcClient->bottom -= prcClient->top;
        prcClient->right -= prcClient->left;
        prcClient->top = prcClient->left = 0;
        Tray_GetWindowSizes(prcClient, &rcView, &rcClock, &rcDesktop);
        iHeight = RECTHEIGHT(rcView);

        // maybe we should translate this  to screen coordinates?
        // that would allow our children to prevent being in certain
        // physical locations, but we're more concerned with size, not loc

        if (g_ts.hwndView)
            // Avoid rips because at WM_CREATE time, this is null.
            SendMessage(g_ts.hwndView, WM_SIZING, code, (LPARAM)(LPRECT)&rcView);

        // did it change?
        if (RECTHEIGHT(rcView) != iHeight) {
            // grow the prcClient by the same amound rcView grew;
            prcClient->bottom += RECTHEIGHT(rcView) - iHeight;
        }
        else
            return;
    }
}


int Window_GetClientGapHeight(HWND hwnd)
{
    RECT rc;

    SetRectEmpty(&rc);
    AdjustWindowRectEx(&rc, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
    return RECTHEIGHT(rc);
}

void Tray_HandleSize()
{
    //
    // if somehow we got minimized go ahead and un-minimize
    //
    if (((GetWindowLong(v_hwndTray, GWL_STYLE)) & WS_MINIMIZE))
    {
        ASSERT(FALSE);
        ShowWindow(v_hwndTray, SW_RESTORE);
    }

    //
    // if we are in the move/size loop and are visible then
    // re-snap the current stuck rect to the new window size
    //
#ifdef DEBUG
    if (g_ts.fSysSizing && (g_ts.uAutoHide & AH_HIDING)) {
        TraceMsg(DM_TRACE, "fSysSize && hiding");
        ASSERT(0);
    }
#endif
    if (g_ts.fSysSizing &&
        ((g_ts.uAutoHide & (AH_ON | AH_HIDING)) != (AH_ON | AH_HIDING)))
    {
        g_ts.uStuckPlace = Tray_RecalcStuckPos(NULL);
    }

    //
    // if we are in fulldrag or we are not in the middle of a move/size then
    // we should resize all our child windows to reflect our new size
    //
    if (g_fDragFullWindows || !g_ts.fSysSizing)
        Tray_SizeWindows();

    //
    // if we are just plain resized and we are visible we may need re-dock
    //
    if (!g_ts.fSysSizing && !g_ts.fSelfSizing && IsWindowVisible(v_hwndTray))
    {
        if (g_ts.uAutoHide & AH_ON)
        {
            UINT uPlace = g_ts.uStuckPlace;
            HWND hwndOther = AppBarGetAutoHideBar(uPlace);

            //
            // we sometimes defer checking for this until after a move
            // so as to avoid interrupting a full-window-drag in progress
            // if there is a different autohide window in our slot then whimper
            //
            if (hwndOther?
                (hwndOther != v_hwndTray) :
                !IAppBarSetAutoHideBar(v_hwndTray, TRUE, uPlace))
            {
                Tray_AutoHideCollision();
            }
        }

        Tray_StuckTrayChange();

        //
        // make sure we clip to tray to the current monitor (if necessary)
        //
        Tray_ClipWindow(TRUE);
    }

    if (g_ts.hwndStartBalloon)
    {
        RECT rc;
        GetWindowRect(g_ts.hwndStart, &rc);
        SendMessage(g_ts.hwndStartBalloon, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.top));
        SetWindowZorder(g_ts.hwndStartBalloon, HWND_TOPMOST);
    }
}

void Tray_HandleSizing(WPARAM code, LPRECT lprc, UINT uStuckPlace)
{
    RECT rc, rcDisplay;
    int iHeight, iMax;
    int iYExtra;        // Y difference between the window and client height;
    SIZE sNewWidths;
    RECT rcTemp;
    BOOL fHiding;

    if (!lprc) {
        rcTemp = g_ts.arStuckRects[uStuckPlace];
        lprc = &rcTemp;
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs starting rect is {%d, %d, %d, %d}"), lprc->left, lprc->top, lprc->right, lprc->bottom);
#if 1
    fHiding = (g_ts.uAutoHide & AH_HIDING);
    if (fHiding) {
        TrayShowWindow(SW_HIDE);
    }
#else
    // old code pre-980504
    if (g_ts.uAutoHide & AH_HIDING) {
        TraceMsg(DM_TRACE, "e.t_hs AH_HIDING => early-out");
        return;
    }
#endif

    KillConfigDesktopDlg();

    //
    // get the a bunch of relevant dimensions
    //
    // (dli) need to change this funciton or get rid of it
    Tray_GetDisplayRectFromRect(&rcDisplay, lprc, MONITOR_DEFAULTTONEAREST);

    iYExtra = Window_GetClientGapHeight(g_ts.hwndMain);
    if (g_cyTrayBorders == -1)
        g_cyTrayBorders = iYExtra; // on initial boot

    if (code) {
        // if code != 0, this is the user sizing.
        // make sure they clip it to the screen.
        RECT rcTemp = rcDisplay;
        InflateRect(&rcTemp, g_cxEdge, g_cyEdge);
        // don't do intersect rect because of sizing up from the bottom (when taskbar docked on bottom)
        // confuses people
        switch (uStuckPlace)
        {
        case STICK_LEFT:   lprc->left = rcTemp.left; break;
        case STICK_TOP:    lprc->top = rcTemp.top; break;
        case STICK_RIGHT:  lprc->right = rcTemp.right; break;
        case STICK_BOTTOM: lprc->bottom = rcTemp.bottom; break;
        }
    }

    //
    // compute the new widths
    // don't let either be more than half the screen
    //
    sNewWidths.cx = RECTWIDTH(*lprc);
    iMax = RECTWIDTH(rcDisplay) / 2;
    if (sNewWidths.cx > iMax)
        sNewWidths.cx = iMax;

    sNewWidths.cy = RECTHEIGHT(*lprc);
    iMax = RECTHEIGHT(rcDisplay) / 2;
    if (sNewWidths.cy > iMax)
        sNewWidths.cy = iMax;

    if (STUCK_HORIZONTAL(uStuckPlace))
        g_cyTrayBorders = iYExtra;

    //
    // compute an initial size
    //
    Tray_MakeStuckRect(lprc, &rcDisplay, sNewWidths, uStuckPlace);
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs starting rect is {%d, %d, %d, %d}"), lprc->left, lprc->top, lprc->right, lprc->bottom);

    //
    // negotiate the exact size with our children
    //
    rc = *lprc;
    rc.bottom -= g_cyTrayBorders;
    Tray_NegotiateSizing(code, &rc);
    iHeight = RECTHEIGHT(rc);

    //
    // was there a change?
    //
    if ((iHeight + iYExtra) != sNewWidths.cy)
    {
        //
        // yes, update the final rectangle
        //
        sNewWidths.cy = iHeight + iYExtra;
        Tray_MakeStuckRect(lprc, &rcDisplay, sNewWidths, uStuckPlace);
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hs final rect is {%d, %d, %d, %d}"), lprc->left, lprc->top, lprc->right, lprc->bottom);

    //
    // store the new size in the appropriate StuckRect
    //
    g_ts.arStuckRects[uStuckPlace] = *lprc;

    //
    // remember the current client gap
    //
    g_cyTrayBorders = iYExtra;

    if (fHiding) {
        TrayShowWindow(SW_SHOWNA);
    }

    if (g_ts.hwndStartBalloon)
    {
        RECT rc;
        GetWindowRect(g_ts.hwndStart, &rc);
        SendMessage(g_ts.hwndStartBalloon, TTM_TRACKPOSITION, 0, MAKELONG((rc.left + rc.right)/2, rc.top));
        SetWindowZorder(g_ts.hwndStartBalloon, HWND_TOPMOST);
    }
}

/*-------------------------------------------------------------------
** the screen size changed, and we need to adjust some stuff, mostly
** globals.  if the tray was docked, it needs to be resized, too.
**
** TRICKINESS: the handling of WM_WINDOWPOSCHANGING is used to
** actually do all the real sizing work.  this saves a bit of
** extra code here.
**-------------------------------------------------------------------*/
STDAPI_(BOOL) SHIsTempDisplayMode();

BOOL CALLBACK Tray_MonitorEnumCallBack(HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM lData)
{
    RECT rcWork;
    int iRet = RecomputeWorkArea(NULL, hMonitor, &rcWork);

    if (iRet == RWA_CHANGED)
    {
        // only send SENDWININICHANGE if the desktop has been created (otherwise
        // we will hang the explorer because the main thread is currently blocked)

        // BUGBUG: it will be nice to send WININICHANGE only once, but we can't
        // because each time the rcWork is different, and there is no way to do it all
        // bug opened on this. 
        SystemParametersInfo(SPI_SETWORKAREA, TRUE, &rcWork, v_hwndDesktop ? SPIF_SENDWININICHANGE : 0);
        RedrawDesktop(&rcWork);
    }

    return TRUE;
}

void Tray_RecomputeAllWorkareas()
{
    EnumDisplayMonitors(NULL, NULL, Tray_MonitorEnumCallBack, (LPARAM)NULL);
}

void Tray_ScreenSizeChange(HWND hwnd)
{
    if (SHIsTempDisplayMode())
    {
        // this will suppress updates so warn to be nice
        TraceMsg(DM_TRACE, "t_ssc: IsTempDisplayMode=1 => nop!");
        return;
    }

    // Set our new HMONITOR in case there is some change
    {
        MONITORINFO mi = {0};
        mi.cbSize = sizeof(mi);

        // Is our old HMONITOR still valid?
        // NOTE: This test is used to tell whether somethings happened to the
        // HMONITOR's or just the screen size changed
        if (!GetMonitorInfo(g_ts.hmonStuck, &mi))
        {
            // No, this means the HMONITORS changed, our monitor might have gone away
            Tray_SetStuckMonitor();
            Tray_RecomputeAllWorkareas();
        }
    }

    // screen size changed, so we need to adjust globals
    g_cxPrimaryDisplay = GetSystemMetrics(SM_CXSCREEN);
    g_cyPrimaryDisplay = GetSystemMetrics(SM_CYSCREEN);

    ResizeStuckRects(g_ts.arStuckRects);

    if (hwnd)
    {
        //
        // set a bogus windowpos and actually repaint with the right
        // shape/size in handling the WINDOWPOSCHANGING message
        //
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    Tray_SizeWindows();
    
    // In the multi-monitor case, we need to turn on clipping for the dynamic
    // monitor changes i.e. when the user add monitors or remove them from the
    // control panel.
    Tray_ClipWindow(TRUE);
}

BOOL _UpdateAlwaysOnTop(BOOL fAlwaysOnTop)
{
    BOOL fChanged = ((g_ts.fAlwaysOnTop == 0) != (fAlwaysOnTop == 0));
    //
    // The user clicked on the AlwaysOnTop menu item, we should now toggle
    // the state and update the window accordingly...
    //
    g_ts.fAlwaysOnTop = fAlwaysOnTop;
    Tray_ResetZorder();

    // Make sure the screen limits are update to the new state.
    Tray_StuckTrayChange();
    return fChanged;
}


void ViewOptionsUpdateDisplay(HWND hDlg);

void _ApplyViewOptionsFromDialog(LPPROPSHEETPAGE psp, HWND hDlg)
{
    // We need to get the Cabinet structure from the property sheet info.
    BOOL fSmallPrev;
    BOOL fSmallNew;
    BOOL fAutoHide;
    BOOL fChanged;
#ifdef DESKBTN
    BOOL fShowDeskBtn;
#endif
    LONG lRet;

    // First check for Always on Top
    fChanged = _UpdateAlwaysOnTop(IsDlgButtonChecked(hDlg, IDC_TRAYOPTONTOP));

    //
    // And change the Autohide state
    fAutoHide = IsDlgButtonChecked(hDlg, IDC_TRAYOPTAUTOHIDE);
    lRet = Tray_SetAutoHideState(fAutoHide);
    if (!HIWORD(lRet) && fAutoHide) {
        // we tried and failed.
        if (!(g_ts.uAutoHide & AH_ON)) {
            CheckDlgButton(hDlg, IDC_TRAYOPTAUTOHIDE, FALSE);
            ViewOptionsUpdateDisplay(hDlg);
        }
    }
    fChanged |= LOWORD(lRet);

    if (fChanged)
        AppBarNotifyAll(NULL, ABN_STATECHANGE, NULL, 0);

    // show/hide the clock
    g_ts.fHideClock = !IsDlgButtonChecked(hDlg, IDC_TRAYOPTSHOWCLOCK);
    SendMessage(g_ts.hwndNotify, TNM_HIDECLOCK, 0, g_ts.fHideClock);
#ifdef DESKBTN
    // show/hide the desktop button
    fShowDeskBtn = IsDlgButtonChecked(hDlg, IDC_TRAYOPTSHOWDESKBTN);
    ShowHideDeskBtnOnQuickLaunch(fShowDeskBtn);
    g_ts.fShowDeskBtn = fShowDeskBtn;
#endif
    Tray_SizeWindows();

    // check if it's changed
    fSmallPrev = (g_ts.fSMSmallIcons ? TRUE : FALSE);
    fSmallNew = (IsDlgButtonChecked(hDlg, IDC_SMSMALLICONS) ? TRUE : FALSE);

    if (fSmallPrev != fSmallNew)
    {
        g_ts.fSMSmallIcons = fSmallNew;
        IMenuPopup_SetIconSize(g_ts._pmpStartMenu, g_ts.fSMSmallIcons ? BMICON_SMALL : BMICON_LARGE);
    }


    if (!SHRestricted(REST_INTELLIMENUS))
    {
        LPTSTR psz = IsDlgButtonChecked(hDlg, IDC_PERSONALIZEDMENUS)? TEXT("YES") : TEXT("No");

        SHRegSetUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("IntelliMenus"),
            REG_SZ, psz, lstrlen(psz) * sizeof(TCHAR), SHREGSET_FORCE_HKCU);

        PostMessage(v_hwndTray, TM_REFRESH, 0, 0);
    }
}

//---------------------------------------------------------------------------
void ViewOptionsInitBitmaps(HWND hDlg)
{
    int i;

    for (i = 0; i <= (IDB_VIEWOPTIONSLAST - IDB_VIEWOPTIONSFIRST); i++) {
        HBITMAP hbm;

        hbm = LoadImage(hinstCabinet,
                        MAKEINTRESOURCE(IDB_VIEWOPTIONSFIRST + i),
                        IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS);
        hbm = (HBITMAP)SendDlgItemMessage(hDlg, IDC_VIEWOPTIONSICONSFIRST + i,
                                          STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
        if (hbm)
            DeleteBitmap(hbm);
    }
}

void ViewOptionsDestroyBitmaps(HWND hDlg)
{
    int i;

    for (i = 0; i <= (IDB_VIEWOPTIONSLAST - IDB_VIEWOPTIONSFIRST); i++) {
        HBITMAP hbmOld;
        hbmOld = (HBITMAP)SendDlgItemMessage(hDlg, IDC_VIEWOPTIONSICONSFIRST + i,
                                             STM_GETIMAGE, 0,0);
        if (hbmOld)
            DeleteBitmap(hbmOld);
    }
}

void ViewOptionsUpdateDisplay(HWND hDlg)
{
    HWND hwnd;
    BOOL fShown;
    BOOL fAutoHide, fShowDeskBtn, fShowClock;

    // this works by having the background bitmap having the
    // small menu, the tray in autohide/ontop mode and we lay other statics
    // over it to give the effect we want.
    SetWindowZorder(GetDlgItem(hDlg, IDC_VOBASE), HWND_BOTTOM);

    ShowWindow(GetDlgItem(hDlg, IDC_VOLARGEMENU),
               !IsDlgButtonChecked(hDlg, IDC_SMSMALLICONS) ? SW_SHOWNA : SW_HIDE);

    fAutoHide = IsDlgButtonChecked(hDlg, IDC_TRAYOPTAUTOHIDE);
#ifdef DESKBTN
    fShowDeskBtn = IsDlgButtonChecked(hDlg, IDC_TRAYOPTSHOWDESKBTN);
#else
    fShowDeskBtn = FALSE;
#endif
    ShowWindow(GetDlgItem(hDlg, IDC_VOTRAY), (!fAutoHide && !fShowDeskBtn) ? SW_SHOWNA : SW_HIDE);
#ifdef DESKBTN
    ShowWindow(GetDlgItem(hDlg, IDC_VOTRAYD), (!fAutoHide && fShowDeskBtn) ? SW_SHOWNA : SW_HIDE);
#endif
    fShowClock = IsDlgButtonChecked(hDlg, IDC_TRAYOPTSHOWCLOCK);
    ShowWindow(GetDlgItem(hDlg, IDC_VOTRAYNOCLOCK), (!fAutoHide && !fShowClock && !fShowDeskBtn) ? SW_SHOWNA : SW_HIDE);
#ifdef DESKBTN
    ShowWindow(GetDlgItem(hDlg, IDC_VOTRAYNOCLOCKD), (!fAutoHide && !fShowClock && fShowDeskBtn) ? SW_SHOWNA : SW_HIDE);
#endif
    ShowWindow(hwnd = GetDlgItem(hDlg, IDC_VOWINDOW),
               (FALSE != (fShown = !IsDlgButtonChecked(hDlg, IDC_TRAYOPTONTOP)) ? SW_SHOWNA : SW_HIDE));

    if (fShown)
        SetWindowZorder(hwnd, HWND_TOP);
}


#define CXVOBASE 274
#define CYVOBASE 130

#define XVOMENU 2
#define YVOMENU 0
#define CXVOMENU 150
#define CYVOMENU 98

#define XVOTRAY 2
#define YVOTRAY 100
#define CXVOTRAY 268
#define CYVOTRAY 28

#define XVOTRAYNOCLOCK (XVOTRAY + 184)
#define YVOTRAYNOCLOCK (YVOTRAY + 3)
#define CXVOTRAYNOCLOCK 79
#define CYVOTRAYNOCLOCK 22

#define XVOTRAYNOCLOCKD (XVOTRAY + 148)
#define YVOTRAYNOCLOCKD YVOTRAYNOCLOCK
#define CXVOTRAYNOCLOCKD 119
#define CYVOTRAYNOCLOCKD CYVOTRAYNOCLOCK

#define XVOWINDOW 212
#define YVOWINDOW 100
#define CXVOWINDOW 61
#define CYVOWINDOW 6

// need to do this by hand because dialog units to pixels will change,
// but the bitmaps won't
void ViewOptionsSizeControls(HWND hDlg)
{
    POINT ptBase; // coordinates of origin of VOBase.bmp
    HWND hwnd;

    ptBase.x = ptBase.y = 0;
    hwnd = GetDlgItem(hDlg, IDC_VOBASE);
    MapWindowPoints(hwnd, hDlg, &ptBase, 1);

    // over it to give the effect we want.
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, CXVOBASE, CYVOBASE,
                 SWP_NOMOVE | SWP_NOACTIVATE);

    SetWindowPos(GetDlgItem(hDlg, IDC_VOLARGEMENU), NULL,
                 ptBase.x + XVOMENU, ptBase.y + YVOMENU, CXVOMENU, CYVOMENU, SWP_NOACTIVATE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hDlg, IDC_VOTRAY), NULL,
                 ptBase.x + XVOTRAY, ptBase.y + YVOTRAY, CXVOTRAY, CYVOTRAY, SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hDlg, IDC_VOTRAYNOCLOCK), HWND_TOP,
                 ptBase.x + XVOTRAYNOCLOCK, ptBase.y + YVOTRAYNOCLOCK, CXVOTRAYNOCLOCK, CYVOTRAYNOCLOCK, SWP_NOACTIVATE);
#ifdef DESKBTN
    SetWindowPos(GetDlgItem(hDlg, IDC_VOTRAYD), NULL,
                 ptBase.x + XVOTRAY, ptBase.y + YVOTRAY, CXVOTRAY, CYVOTRAY, SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hDlg, IDC_VOTRAYNOCLOCKD), HWND_TOP,
                 ptBase.x + XVOTRAYNOCLOCKD, ptBase.y + YVOTRAYNOCLOCKD, CXVOTRAYNOCLOCKD, CYVOTRAYNOCLOCKD, SWP_NOACTIVATE);
#endif
    {
        BITMAP bmp;
        HBITMAP hbmpVoBase = LoadBitmap(hinstCabinet, MAKEINTRESOURCE(IDB_VOBASE));
        GetObject(hbmpVoBase, sizeof(BITMAP), &bmp);
        DeleteObject(hbmpVoBase);

        SetWindowPos(GetDlgItem(hDlg, IDC_VOWINDOW), NULL,
                     ptBase.x + bmp.bmWidth - CXVOWINDOW -1,
                     ptBase.y + YVOWINDOW,
                     CXVOWINDOW, CYVOWINDOW, SWP_NOACTIVATE);
    }
}


const static DWORD aTaskOptionsHelpIDs[] = {  // Context Help IDs
    IDC_VOBASE,           IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_VOLARGEMENU,      IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_VOTRAY,           IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_VOWINDOW,         IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_VOTRAYD,          IDH_TASKBAR_OPTIONS_BITMAP,
    IDC_TRAYOPTONTOP,     IDH_TRAY_TASKBAR_ONTOP,
    IDC_TRAYOPTAUTOHIDE,  IDH_TRAY_TASKBAR_AUTOHIDE,
    IDC_SMSMALLICONS,     IDH_STARTMENU_SMALLICONS,
    IDC_TRAYOPTSHOWCLOCK, IDH_TRAY_SHOW_CLOCK,
    IDC_PERSONALIZEDMENUS,IDH_TRAY_PERSONALIZED_MENUS,

    0, 0
};

#define REGSTR_EXPLORER_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")

BOOL_PTR CALLBACK TrayViewOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE psp = GetWindowPtr(hDlg, DWLP_USER);

    INSTRUMENT_WNDPROC(SHCNFI_TRAYVIEWOPTIONS_DLGPROC, hDlg, uMsg, wParam, lParam);

    switch (uMsg) {

    case WM_COMMAND:
        ViewOptionsUpdateDisplay(hDlg);
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        break;

    case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            if (g_ts.fAlwaysOnTop)
                CheckDlgButton(hDlg, IDC_TRAYOPTONTOP, TRUE);
            if (g_ts.uAutoHide & AH_ON)
                CheckDlgButton(hDlg, IDC_TRAYOPTAUTOHIDE, TRUE);
            if (g_ts.fSMSmallIcons)
                CheckDlgButton(hDlg, IDC_SMSMALLICONS, TRUE);
            if (!g_ts.fHideClock)
                CheckDlgButton(hDlg, IDC_TRAYOPTSHOWCLOCK, TRUE);

            if (SHRestricted(REST_INTELLIMENUS))
            {
                // If there is a restriction of any kine, hide the window
                ShowWindow(GetDlgItem(hDlg, IDC_PERSONALIZEDMENUS), FALSE);
            }
            else if (SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("IntelliMenus"),
                                   FALSE, TRUE))
            {
                CheckDlgButton(hDlg, IDC_PERSONALIZEDMENUS, TRUE);
            }

    #ifdef DESKBTN
            if (g_ts.fShowDeskBtn)
                CheckDlgButton(hDlg, IDC_TRAYOPTSHOWDESKBTN, TRUE);
    #endif
            ViewOptionsSizeControls(hDlg);
            ViewOptionsInitBitmaps(hDlg);
            ViewOptionsUpdateDisplay(hDlg);
        }
        break;

    case WM_SYSCOLORCHANGE:
        ViewOptionsInitBitmaps(hDlg);
        return TRUE;

    case WM_DESTROY:
        SetForegroundWindow(v_hwndTray);
        ViewOptionsDestroyBitmaps(hDlg);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_APPLY:
            // save settings here
            _ApplyViewOptionsFromDialog(psp, hDlg);
            return TRUE;

        case PSN_KILLACTIVE:
        case PSN_SETACTIVE:
            return TRUE;

        }
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aTaskOptionsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aTaskOptionsHelpIDs);
        break;
    }

    return FALSE;
}


BOOL_PTR CALLBACK InitStartMenuDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);    // cfgstart.c

/*-------------------------------------------------------------
** Method to add view property sheet pages to the tray property
** sheet
**-------------------------------------------------------------*/
STDMETHODIMP CShellTray_AddViewPropertySheetPages(
        DWORD dwReserved, LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
    HPROPSHEETPAGE hpage;
    PROPSHEETPAGE psp;

    psp.dwSize = SIZEOF(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hinstCabinet;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_TRAY_VIEW_OPTIONS);
    psp.pfnDlgProc = TrayViewOptionsDlgProc;
    psp.lParam = 0;
    hpage = CreatePropertySheetPage(&psp);
    if (hpage)
        lpfn(hpage, lParam);

    psp.pszTemplate = MAKEINTRESOURCE(DLG_STARTMENU_CONFIG);
    psp.pfnDlgProc = InitStartMenuDlgProc;
    psp.lParam = 0;
    hpage = CreatePropertySheetPage(&psp);
    if (hpage)
        lpfn(hpage, lParam);

    return S_OK;
}

void _SaveTray(void)
{
    if (SHRestricted(REST_NOSAVESET)) 
        return;

    if (SHRestricted(REST_CLEARRECENTDOCSONEXIT))
        ClearRecentDocumentsAndMRUStuff(FALSE);

    HotkeyList_Save();

    _SaveTrayStuff();
}

void _SaveTrayAndDesktop(void)
{
    _SaveTray();

    if (v_hwndDesktop)
        SendMessage(v_hwndDesktop, DTM_SAVESTATE, 0, 0);
}

void SlideStep(HWND hwnd,
    const RECT *prcMonitor, const RECT *prcOld, const RECT *prcNew)
{
    SIZE sizeOld = {prcOld->right - prcOld->left, prcOld->bottom - prcOld->top};
    SIZE sizeNew = {prcNew->right - prcNew->left, prcNew->bottom - prcNew->top};
    BOOL fClipFirst = FALSE;
    UINT flags;

    DAD_ShowDragImage(FALSE);   // Make sure this is off - client function must turn back on!!!
    if (prcMonitor)
    {
        RECT rcClip, rcClipSafe, rcClipTest;

        Tray_CalcClipCoords(&rcClip, prcMonitor, prcNew);

        rcClipTest = rcClip;

        OffsetRect(&rcClipTest, prcOld->left, prcOld->top);
        IntersectRect(&rcClipSafe, &rcClipTest, prcMonitor);

        fClipFirst = EqualRect(&rcClipTest, &rcClipSafe);

        if (fClipFirst)
            Tray_ClipInternal(&rcClip);
    }

    flags = SWP_NOZORDER|SWP_NOACTIVATE;
    if ((sizeOld.cx == sizeNew.cx) && (sizeOld.cy == sizeNew.cy))
        flags |= SWP_NOSIZE;

    SetWindowPos(hwnd, NULL,
        prcNew->left, prcNew->top, sizeNew.cx, sizeNew.cy, flags);

    if (prcMonitor && !fClipFirst)
    {
        RECT rcClip;

        Tray_CalcClipCoords(&rcClip, prcMonitor, prcNew);
        Tray_ClipInternal(&rcClip);
    }
}

void Tray_SlideWindow(HWND hwnd, RECT *prc, BOOL fShow)
{
    RECT rcLast;
    RECT rcMonitor;
    const RECT *prcMonitor;
    int dt;
    BOOL fAnimate;

    if (!IsWindowVisible(hwnd))
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sw window is hidden, just moving"));
        MoveWindow(v_hwndTray, prc->left, prc->top, RECTWIDTH(*prc), RECTHEIGHT(*prc), FALSE);
        return;
    }

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sw -----------------BEGIN-----------------"));

    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        Tray_GetStuckDisplayRect(g_ts.uStuckPlace, &rcMonitor);
        prcMonitor = &rcMonitor;
    }
    else
        prcMonitor = NULL;

    GetWindowRect(hwnd, &rcLast);

    dt = fShow? g_dtSlideShow : g_dtSlideHide;

    // See if we can use animation effects.
    SystemParametersInfo(SPI_GETMENUANIMATION, 0, &fAnimate, 0);
	
    if (g_fDragFullWindows && fAnimate && (dt > 0))
    {
        RECT rcOld, rcNew, rcMove;
        int  dx, dy, t, t2, t0, priority;
        HANDLE me;

        rcOld = rcLast;
        rcNew = *prc;

        dx = ((rcNew.left + rcNew.right) - (rcOld.left + rcOld.right)) / 2;
        dy = ((rcNew.top + rcNew.bottom) - (rcOld.top + rcOld.bottom)) / 2;
        ASSERT(dx == 0 || dy == 0);

        me = GetCurrentThread();
        priority = GetThreadPriority(me);
        SetThreadPriority(me, THREAD_PRIORITY_HIGHEST);

        t2 = t0 = GetTickCount();

        rcMove = rcOld;
        while ((t = GetTickCount()) < (t0 + dt))
        {
            int dtdiff;
            LRESULT lres;
            if (t != t2)
            {
                rcMove.right -= rcMove.left;
                rcMove.left = rcOld.left + (dx) * (t - t0) / dt;
                rcMove.right += rcMove.left;

                rcMove.bottom -= rcMove.top;
                rcMove.top  = rcOld.top  + (dy) * (t - t0) / dt;
                rcMove.bottom += rcMove.top;

                SlideStep(hwnd, prcMonitor, &rcLast, &rcMove);

                if (fShow)
                    UpdateWindow(hwnd);

                rcLast = rcMove;
                t2 = t;
            }

            // don't draw frames faster than user can see, e.g. 20ms
            #define ONEFRAME 20
            dtdiff = GetTickCount();
            if ((dtdiff - t) < ONEFRAME)
                Sleep(ONEFRAME - (dtdiff - t));

            // try to give desktop a chance to update
            // only do it on hide because desktop doesn't need to paint on taskbar show
            if (!fShow)
                SendMessageTimeout(v_hwndDesktop, DTM_UPDATENOW, 0, 0, SMTO_ABORTIFHUNG, 50, &lres);
        }

        SetThreadPriority(me, priority);
    }

    SlideStep(hwnd, prcMonitor, &rcLast, prc);

    if (fShow)
        UpdateWindow(hwnd);

    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sw ------------------END------------------"));
}

void Tray_UnhideNow()
{
    if (g_ts.uModalMode == MM_SHUTDOWN)
        return;

    g_ts.fSelfSizing = TRUE;
    DAD_ShowDragImage(FALSE);   // unlock the drag sink if we are dragging.
    Tray_SlideWindow(v_hwndTray, &g_ts.arStuckRects[g_ts.uStuckPlace], g_dtSlideShow);
    DAD_ShowDragImage(TRUE);    // restore the lock state.
    g_ts.fSelfSizing = FALSE;

    ClockCtl_HandleTrayHide(FALSE);
}

void Tray_ComputeHiddenRect(LPRECT prc, UINT uStuck)
{
    int dwh;
    HMONITOR hMon;
    RECT rcMon;
    
    hMon = MonitorFromRect(prc, MONITOR_DEFAULTTONULL);
    if (!hMon)
        return;
    GetMonitorRect(hMon, &rcMon);
 
    if (STUCK_HORIZONTAL(uStuck))
        dwh = prc->bottom - prc->top;
    else
        dwh = prc->right - prc->left;

    switch (uStuck)
    {
    case STICK_LEFT:
        prc->right = rcMon.left + (g_cxFrame / 2);
        prc->left = prc->right - dwh;
        break;

    case STICK_RIGHT:
        prc->left = rcMon.right - (g_cxFrame / 2);
        prc->right = prc->left + dwh;
        break;

    case STICK_TOP:
        prc->bottom = rcMon.top + (g_cyFrame / 2);
        prc->top = prc->bottom - dwh;
        break;

    case STICK_BOTTOM:
        prc->top = rcMon.bottom - (g_cyFrame / 2);
        prc->bottom = prc->top + dwh;
        break;
    }
}

UINT Tray_GetDockedRect(LPRECT prc, BOOL fMoving)
{
    UINT uPos;

    if (fMoving && (g_ts.uMoveStuckPlace != (UINT)-1))
        uPos = g_ts.uMoveStuckPlace;
    else
        uPos = g_ts.uStuckPlace;

    *prc = g_ts.arStuckRects[uPos];

    if ((g_ts.uAutoHide & (AH_ON | AH_HIDING)) == (AH_ON | AH_HIDING))
    {
        Tray_ComputeHiddenRect(prc, uPos);
    }

    return uPos;
}

void Tray_CalcClipCoords(RECT *prcClip, const RECT *prcMonitor, const RECT *prcNew)
{
    RECT rcMonitor;
    RECT rcWindow;

    if (!prcMonitor)
    {
        Tray_GetStuckDisplayRect(g_ts.uStuckPlace, &rcMonitor);
        prcMonitor = &rcMonitor;
    }

    if (!prcNew)
    {
        GetWindowRect(v_hwndTray, &rcWindow);
        prcNew = &rcWindow;
    }

    IntersectRect(prcClip, prcMonitor, prcNew);
    OffsetRect(prcClip, -prcNew->left, -prcNew->top);
}

void Tray_ClipInternal(const RECT *prcClip)
{
    HRGN hrgnClip;

    // don't worry about clipping if there's only one monitor
    if (GetSystemMetrics(SM_CMONITORS) <= 1)
        prcClip = NULL;

    if (prcClip)
    {
        g_ts.fMonitorClipped = TRUE;
        hrgnClip = CreateRectRgnIndirect(prcClip);
    }
    else
    {
        // SetWindowRgn is expensive, skip ones that are NOPs
        if (!g_ts.fMonitorClipped)
            return;

        g_ts.fMonitorClipped = FALSE;
        hrgnClip = NULL;
    }

    SetWindowRgn(v_hwndTray, hrgnClip, TRUE);
}

void Tray_ClipWindow(BOOL fClipState)
{
    RECT rcClip;
    RECT *prcClip;

    if (g_ts.fSelfSizing || g_ts.fSysSizing)
    {
        TraceMsg(TF_WARNING, "Tray_ClipWindow: g_ts.fSelfSizing %x, g_ts.fSysSizing %x", g_ts.fSelfSizing, g_ts.fSysSizing);
        return;
    }

    if (GetSystemMetrics(SM_CMONITORS) <= 1)
        fClipState = FALSE;

    if (fClipState)
    {
        prcClip = &rcClip;
        Tray_CalcClipCoords(prcClip, NULL, NULL);
    }
    else
        prcClip = NULL;

    Tray_ClipInternal(prcClip);
}

void Tray_Hide()
{
    RECT rcNew;

    // if we're in shutdown or if we're on boot up
    // don't hide
    if (g_ts.uModalMode == MM_SHUTDOWN)
    {
        TraceMsg(TF_TRAY, "e.th: suppress hide (shutdown || Notify)");
        return;
    }

    KillTimer(v_hwndTray, IDT_AUTOHIDE);

    g_ts.fSelfSizing = TRUE;

    //
    // update the flags here to prevent race conditions
    //
    g_ts.uAutoHide = AH_ON | AH_HIDING;
    Tray_GetDockedRect(&rcNew, FALSE);

    DAD_ShowDragImage(FALSE);   // unlock the drag sink if we are dragging.
    Tray_SlideWindow(v_hwndTray, &rcNew, g_dtSlideHide);
    DAD_ShowDragImage(FALSE);   // Another thread could have locked while we were gone
    DAD_ShowDragImage(TRUE);    // restore the lock state.

    ClockCtl_HandleTrayHide(TRUE);

    g_ts.fSelfSizing = FALSE;
}

void Tray_AutoHideCollision()
{
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_ahc COLLISION! (posting UI request)"));

    PostMessage(v_hwndTray, TM_WARNNOAUTOHIDE, ((g_ts.uAutoHide & AH_ON) != 0),
        0L);
}

LONG Tray_SetAutoHideState(BOOL fAutoHide)
{
    //
    // make sure we have something to do
    //
    if ((fAutoHide != 0) == ((g_ts.uAutoHide & AH_ON) != 0))
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.sahs nothing to do"));
        return MAKELONG(FALSE, TRUE);
    }

    //
    // make sure we can do it
    //
    if (!IAppBarSetAutoHideBar(v_hwndTray, fAutoHide, g_ts.uStuckPlace))
    {
        Tray_AutoHideCollision();
        return MAKELONG(FALSE, FALSE);
    }

    //
    // do it
    //
    if (fAutoHide)
    {
        Tray_Hide();
#ifdef DEBUG
        // Tray_Hide updates the flags for us (sanity)
        if (!(g_ts.uAutoHide & AH_ON))
        {
            TraceMsg(DM_WARNING, "e.sahs: !AH_ON"); // ok to fail on boot/shutdown
        }
#endif
    }
    else
    {
        g_ts.uAutoHide = 0;
        KillTimer(v_hwndTray, IDT_AUTOHIDE);
        Tray_UnhideNow();
    }

    //
    // brag about it
    //
    Tray_StuckTrayChange();
    return MAKELONG(TRUE, TRUE);
}

void Tray_HandleEnterMenuLoop()
{
    // kill the timer when we're in the menu loop so that we don't
    // pop done while browsing the menus.
    if (g_ts.uAutoHide & AH_ON)
    {
        KillTimer(v_hwndTray,  IDT_AUTOHIDE);
    }
}

void Tray_SetAutoHideTimer()
{
    if (g_ts.uAutoHide & AH_ON)
    {
        SetTimer(v_hwndTray, IDT_AUTOHIDE, 500, NULL);
    }
}

__inline void Tray_HandleExitMenuLoop()
{
    // when we leave the menu stuff, start checking again.
    Tray_SetAutoHideTimer();
}

void Tray_Unhide()
{
    // handle autohide
    if ((g_ts.uAutoHide & AH_ON) &&
        (g_ts.uAutoHide & AH_HIDING))
    {
        Tray_UnhideNow();
        g_ts.uAutoHide &= ~AH_HIDING;
        Tray_SetAutoHideTimer();

        if (g_ts.fShouldResize)
        {
            ASSERT(0);
            ASSERT(!(g_ts.uAutoHide & AH_HIDING));
            Tray_SizeWindows();
            g_ts.fShouldResize = FALSE;
        }
    }
}

void TraySetUnhideTimer(LONG x, LONG y)
{
    // handle autohide
    if ((g_ts.uAutoHide & AH_ON) &&
        (g_ts.uAutoHide & AH_HIDING))
    {
        LONG dx = x-g_ts.ptLastHittest.x;
        LONG dy = y-g_ts.ptLastHittest.y;
        LONG rr = dx*dx + dy*dy;
        LONG dd = GetSystemMetrics(SM_CXDOUBLECLK) * GetSystemMetrics(SM_CYDOUBLECLK);

        if (rr > dd) 
        {
            SetTimer(v_hwndTray, IDT_AUTOUNHIDE, 50, NULL);
            g_ts.ptLastHittest.x = x;
            g_ts.ptLastHittest.y = y;
        }
    }
}

void StartButton_Reset()
{
    HBITMAP hbmOld, hbmNew;

    DebugMsg(DM_IANELHK, TEXT("c.sb_rs: Recreating start button."));
    hbmOld = (HBITMAP)SendMessage(g_ts.hwndStart, BM_GETIMAGE, IMAGE_BITMAP, 0 );
    if (hbmOld)
    {
        hbmNew = CreateStartBitmap(g_ts.hwndMain);
        if (hbmNew)
        {
            SendMessage(g_ts.hwndStart, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbmNew);
            DeleteObject(hbmOld);
        }
    }
    g_ts.sizeStart.cy = 0;
}


//---------------------------------------------------------------------------
void Tray_OnNewSystemSizes()
{
    TraceMsg(TF_TRAY, "Handling win ini change.");
    StartButton_Reset();
    Tray_VerifySize(TRUE);
}

//***   CheckWindowPositions -- flag which windows actually changed
// ENTRY/EXIT
//  g_ts.pPositions->hdsaWP[*]->fRestore modified
// NOTES
//  in order to correctly implement 'Undo Minimize-all(Cascade/Tile)',
// we need to tell which windows were changed by the 'Do' operation.
// (nt5:183421: we used to restore *every* top-level window).
int CALLBACK CWPCallback(void *pItem, void *pData)
{
    HWNDANDPLACEMENT *pI2 = (HWNDANDPLACEMENT *)pItem;
    WINDOWPLACEMENT wp;

    wp.length = SIZEOF(wp);
    pI2->fRestore = TRUE;
    if (GetWindowPlacement(pI2->hwnd, &wp)) {
        if (memcmp(&pI2->wp, &wp, SIZEOF(wp)) == 0)
            pI2->fRestore = FALSE;
    }

    TraceMsg(TF_TRAY, "cwp: (hwnd=0x%x) fRestore=%d", pI2->hwnd, pI2->fRestore);

    return 1;   // 1:continue enum
}

void CheckWindowPositions()
{
    ENTERCRITICAL;      // i think this is needed...
    if (g_ts.pPositions) {
        if (g_ts.pPositions->hdsaWP) {
            DSA_EnumCallback(g_ts.pPositions->hdsaWP, CWPCallback, NULL);
        }
    }
    LEAVECRITICAL;

    return;
}

//---------------------------------------------------------------------------
void Tray_HandleTimer(WPARAM wTimerID)
{
    if (wTimerID == IDT_HANDLEDELAYBOOTSTUFF)
    {
        KillTimer(v_hwndTray, IDT_HANDLEDELAYBOOTSTUFF);
        PostMessage(v_hwndTray, TM_HANDLEDELAYBOOTSTUFF, 0, 0);
        return;
    }

    if (wTimerID == IDT_STARTMENU)
    {
        SetForegroundWindow(v_hwndTray);
        KillTimer(v_hwndTray, wTimerID);
        DAD_ShowDragImage(FALSE);       // unlock the drag sink if we are dragging.
        SendMessage(g_ts.hwndStart, BM_SETSTATE, TRUE, 0 );
        UpdateWindow(g_ts.hwndStart);
        DAD_ShowDragImage(TRUE);        // restore the lock state.
    }

    if (wTimerID == IDT_SAVESETTINGS)
    {
        KillTimer(v_hwndTray, IDT_SAVESETTINGS);
        _SaveTray();
    }
    
    if (wTimerID == IDT_ENABLEUNDO)
    {
        KillTimer(v_hwndTray, IDT_SAVESETTINGS);
        CheckWindowPositions();
        g_ts.fUndoEnabled = TRUE;
    }

    if  (g_ts.uAutoHide & AH_ON)
    {
        if (g_ts.fSysSizing)
            return;

        switch (wTimerID)
        {
            POINT pt;
            RECT rc;

            case IDT_AUTOHIDE:
            {
                // Don't hide if we're already hiding or if any apps are flashing.
                if (!(g_ts.uAutoHide & AH_HIDING) && !g_ts.fFlashing && !g_ts.fBalloonUp)
                {
                    // Get the cursor position.
                    GetCursorPos(&pt);

                    // Get the tray rect and inflate it a bit.
                    rc = g_ts.arStuckRects[g_ts.uStuckPlace];
                    InflateRect(&rc, g_cxEdge * 4, g_cyEdge*4);

                    // Don't hide if cursor is within inflated tray rect.
                    if (!PtInRect(&rc, pt))
                    {
                        // Don't hide if the tray is active
                        if (!Tray_IsActive())
                        {
                            // Don't hide if the view has a system menu up.
                            if (!SendMessage(g_ts.hwndView, TM_SYSMENUCOUNT, 0, 0L))
                            {
                                // Phew!  We made it.  Hide the tray.
                                Tray_Hide();
                            }
                        }
                    }
                }
                break;
            }

            case IDT_AUTOUNHIDE:
            {
                KillTimer(v_hwndTray, wTimerID);
                g_ts.ptLastHittest.x = -0x0fff;
                g_ts.ptLastHittest.y = -0x0fff;
                GetWindowRect(v_hwndTray, &rc);
                if (g_ts.uAutoHide & AH_HIDING)
                {
                    GetCursorPos(&pt);
                    if (PtInRect(&rc, pt))
                        Tray_Unhide();
                }
                break;
            }
        }
    }
}


//---------------------------------------------------------------------------
// Search for the first sub menu of the given menu, who's first item's ID
// is id. Returns NULL, if nothing is found.
HMENU Menu_FindSubMenuByFirstID(HMENU hmenu, UINT id)
{
    int cMax, c;
    HMENU hmenuSub;
    MENUITEMINFO mii;

    ASSERT(hmenu);

    // REVIEW There's a bug in the thunks such that GMIC() returns
    // 0x0000ffff for an invalid menu which gets us confused.
    if (!IsMenu(hmenu))
        return NULL;

    // Search all items.
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_ID;
    cMax = GetMenuItemCount(hmenu);
    for (c=0; c < cMax; c++)
    {
        // Is this item a submenu?
        hmenuSub = GetSubMenu(hmenu, c);
        if (hmenuSub && GetMenuItemInfo(hmenuSub, 0, TRUE, &mii))
        {
            if (mii.wID == id)
            {
                // Found it!
                return hmenuSub;
            }
        }
    }

    return NULL;
}


BOOL ExecItemByPidls(HWND hwnd, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem)
{
    BOOL fRes = FALSE;

    if (pidlFolder && pidlItem)
    {
        IShellFolder *psf = BindToFolder(pidlFolder);
        if (psf)
        {
            fRes = SUCCEEDED(SHInvokeDefaultCommand(hwnd, psf, pidlItem));
        }
        else
        {
            TCHAR szPath[MAX_PATH];
            SHGetPathFromIDList(pidlFolder, szPath);
            ShellMessageBox(hinstCabinet, hwnd, MAKEINTRESOURCE( IDS_CANTFINDSPECIALDIR),
                            NULL, MB_ICONEXCLAMATION, szPath);
        }
    }
    return fRes;
}


LRESULT Tray_HandleDestroy()
{
#ifdef OLDTSTARTMENU
    HMENU hmenuSub;
#endif
    MINIMIZEDMETRICS mm;

    TraceMsg(DM_SHUTDOWN, "Tray_HD: enter");

    mm.cbSize = SIZEOF(mm);
    SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, SIZEOF(mm), &mm, FALSE);
    mm.iArrange &= ~ARW_HIDE;
    SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, SIZEOF(mm), &mm, FALSE);

    PrintNotify_Exit();
    CStartDropTarget_Revoke();
    StartMenu_Destroy();

    ENTERCRITICAL;
    if (g_ts.pPositions)
    {
        DestroySavedWindowPositions(g_ts.pPositions);
        g_ts.pPositions = NULL;
    }
    LEAVECRITICAL;

    if (g_ts.hBIOS!=INVALID_HANDLE_VALUE)
        CloseHandle(g_ts.hBIOS);

    Tray_UnregisterGlobalHotkeys();

    if (g_ts.ptbs) {
        g_ts.ptbs->lpVtbl->Release(g_ts.ptbs);
        g_ts.ptbs = NULL;
    }

    // Cleanup notify handlers.
    UnregisterNotify(g_ts.uProgNotify);
    UnregisterNotify(g_ts.uFastNotify);
    UnregisterNotify(g_ts.uRecentNotify);
    UnregisterNotify(g_ts.uFavoritesNotify);
    UnregisterNotify(g_ts.uDesktopNotify);
    UnregisterNotify(g_ts.uCommonDesktopNotify);
    UnregisterNotify(g_ts.uCommonProgNotify);
    UnregisterNotify(g_ts.uCommonFastNotify);

    if (g_ts.pcmFind)
        g_ts.pcmFind->lpVtbl->Release(g_ts.pcmFind);
    Tray_DestroyShellView();


    if (g_ts.hwndTrayTips) {
        DestroyWindow(g_ts.hwndTrayTips);
        g_ts.hwndTrayTips = NULL;
    }

    DestroyStartButtonBalloon();

    // REVIEW
    PostQuitMessage(0);

    if (g_ts.hbmpStartBkg)
        DeleteBitmap(g_ts.hbmpStartBkg);

    Tray_HandleShellServiceObject(SSOCMDID_CLOSE, 0);

#ifdef WINNT
    if ( g_hShellReadyEvent )
    {
        CloseHandle( g_hShellReadyEvent );
        g_hShellReadyEvent = NULL;
    }
#endif
    v_hwndTray = NULL;
    g_ts.hwndStart = NULL;


    TraceMsg(DM_SHUTDOWN, "Tray_HD: leave");
    return 0;
}

void Tray_SetFocus(HWND hwnd)
{
    UnkUIActivateIO(g_ts.ptbs, FALSE, NULL);
    SetFocus(hwnd);
}

#define TRIEDTOOMANYTIMES 100

void Tray_ActAsSwitcher()
{
    if (g_ts.uModalMode) {

        if (g_ts.uModalMode != MM_SHUTDOWN) {
            SwitchToThisWindow(GetLastActivePopup(g_ts.hwndMain), TRUE);
        }
        MessageBeep(0);

    } else {
        HWND hwndForeground;
        HWND hwndActive;

#if (defined(WINNT) || defined(DEBUG))
        static int s_iRecurse = 0;
        s_iRecurse++;
#endif

#ifdef DEBUG
        ASSERT(s_iRecurse < TRIEDTOOMANYTIMES);
        TraceMsg(TF_TRAY, "s_iRecurse = %d", s_iRecurse);
#endif

        hwndForeground = GetForegroundWindow();
        hwndActive = GetActiveWindow();
        // only do the button once we're the foreground dude.
        if ((hwndForeground == v_hwndTray) && (hwndActive == v_hwndTray))
        {
            // This code path causes the start button to do something because
            // of the keyboard. So reflect that with the focus rect.
            SendMessage(g_ts.hwndStart, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);

            if (SendMessage(g_ts.hwndStart, BM_GETSTATE, 0, 0) & BST_PUSHED)
            {
                if (g_ts._pmpStartMenu)
                    g_ts._pmpStartMenu->lpVtbl->OnSelect(g_ts._pmpStartMenu, MPOS_FULLCANCEL);
                _ForceStartButtonUp();
            }
            else
            {
                // This pushes the start button and causes the start menu to popup.
                SendMessage(GetDlgItem(v_hwndTray, IDC_START), BM_SETSTATE, TRUE, 0);
            }
#if (defined(WINNT) || defined(DEBUG))
            s_iRecurse = 0;
#endif
        } else {
#ifdef WINNT
            // On NT we don't want to loop endlessly trying to become
            // foreground.  With NT's new SetForegroundWindow rules, it would
            // be pointless to try and hopefully we won't need to anyhow.
            // Randomly, I picked a quarter as many times as the debug squirty would indicate
            // as the number of times to try on NT.
            // Hopefully that is enough on most machines.
            if (s_iRecurse > (TRIEDTOOMANYTIMES / 4)) {
                s_iRecurse = 0;
                return;
            }
#endif
            // until then, try to come forward.
            Tray_HandleFullScreenApp(NULL);
            if (hwndForeground == v_hwndDesktop) {
                Tray_SetFocus(g_ts.hwndStart);
                if (GetFocus() != g_ts.hwndStart)
                    return;
            }

            SwitchToThisWindow(g_ts.hwndMain, TRUE);
            SetForegroundWindow(v_hwndTray);
            Sleep(20); // give some time for other async activation messages to get posted
            PostMessage(v_hwndTray, TM_ACTASTASKSW, 0, 0);
        }

    }
}

//----------------------------------------------------------------------------
void Tray_OnWinIniChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    Cabinet_InitGlobalMetrics(0, (LPTSTR)NULL);
    // Reset the programs menu.
    // REVIEW IANEL - We should only need to listen to the SPI_SETNONCLIENT stuff
    // but deskcpl doesn't send one.
    if (wParam == SPI_SETNONCLIENTMETRICS || (!wParam && (!lParam || (lstrcmpi((LPTSTR)lParam, TEXT("WindowMetrics")) == 0))))
    {
#ifdef DEBUG
        if (wParam == SPI_SETNONCLIENTMETRICS)
            TraceMsg(TF_TRAY, "c.t_owic: Non-client metrics (probably) changed.");
        else
            TraceMsg(TF_TRAY, "c.t_owic: Window metrics changed.");
#endif

        Tray_OnNewSystemSizes();
    }

    // Handle old extensions.
    if (!lParam || (lParam && (lstrcmpi((LPTSTR)lParam, TEXT("Extensions")) == 0)))
    {
        TraceMsg(TF_TRAY, "t_owic: Extensions section change.");
        CheckWinIniForAssocs();
    }

    // Get the printer thread to re-do the icon.
    if (g_ts.htPrinterPoll && g_ts.idPrinterPoll)
    {
        PostThreadMessage(g_ts.idPrinterPoll, WM_USER+SHCNE_UPDATEITEM, 0, 0);
    }

    // Tell shell32 to refresh its cache
    SHSettingsChanged(wParam, lParam);
}


//---------------------------------------------------------------------------
HWND HotkeyList_HotkeyInUse(WORD wHK)
{
    HWND hwnd;
    LRESULT lrHKInUse = 0;
    int nMod;
    WORD wHKNew;
#ifdef DEBUG
    TCHAR sz[MAX_PATH];
#endif

    // Map the modifiers back.
    nMod = 0;
    if (HIBYTE(wHK) & MOD_SHIFT)
        nMod |= HOTKEYF_SHIFT;
    if (HIBYTE(wHK) & MOD_CONTROL)
        nMod |= HOTKEYF_CONTROL;
    if (HIBYTE(wHK) & MOD_ALT)
        nMod |= HOTKEYF_ALT;

    wHKNew = (WORD)((nMod*256)+LOBYTE(wHK));

    DebugMsg(DM_IANELHK, TEXT("c.hkl_hiu: Checking for %x"), wHKNew);
    hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
    while (hwnd)
    {
        SendMessageTimeout(hwnd, WM_GETHOTKEY, 0, 0, SMTO_ABORTIFHUNG| SMTO_BLOCK, 3000, &lrHKInUse);
        if (wHKNew == (WORD)lrHKInUse)
        {
#ifdef DEBUG
            GetWindowText(hwnd, sz, ARRAYSIZE(sz));
            DebugMsg(DM_IANELHK, TEXT("c.hkl_hiu: %s (%x) is using %x"), sz, hwnd, lrHKInUse);
#endif
            return hwnd;
        }
#ifdef DEBUG
        else if (lrHKInUse)
        {
            GetWindowText(hwnd, sz, ARRAYSIZE(sz));
            DebugMsg(DM_IANELHK, TEXT("c.hkl_hiu: %s (%x) is using %x"), sz, hwnd, lrHKInUse);
        }
#endif
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return NULL;
}

//---------------------------------------------------------------------------
void HotkeyList_HandleHotkey(int nID)
{
    PHOTKEYITEM phki;
    BOOL fRes;
    HWND hwnd;

    TraceMsg(TF_TRAY, "c.hkl_hh: Handling hotkey (%d).", nID);

    // Find it in the list.
    ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

    phki = DSA_GetItemPtr(g_ts.hdsaHKI, nID);
    if (phki && phki->wGHotkey)
    {
        TraceMsg(TF_TRAY, "c.hkl_hh: Hotkey listed.");

        // Are global hotkeys enabled?
        if (!g_ts.fGlobalHotkeyDisable)
        {
            // Yep.
            hwnd = HotkeyList_HotkeyInUse(phki->wGHotkey);
            // Make sure this hotkey isn't already in use by someone.
            if (hwnd)
            {
                TraceMsg(TF_TRAY, "c.hkl_hh: Hotkey is already in use.");
                // Activate it.
                SwitchToThisWindow(GetLastActivePopup(hwnd), TRUE);
            }
            else
            {
                DECLAREWAITCURSOR;
                // Exec the item.
                SetWaitCursor();
                TraceMsg(TF_TRAY, "c.hkl_hh: Hotkey is not in use, execing item.");
                ASSERT(phki->pidlFolder && phki->pidlItem);
                    fRes = ExecItemByPidls(g_ts.hwndMain, phki->pidlFolder, phki->pidlItem);
                ResetWaitCursor();
#ifdef DEBUG
                if (!fRes)
                {
                    DebugMsg(DM_ERROR, TEXT("c.hkl_hh: Can't exec command ."));
                }
#endif
            }
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("c.hkl_hh: Global hotkeys have been disabled."));
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("c.hkl_hh: Hotkey not listed."));
    }
}

//---------------------------------------------------------------------------
LRESULT Tray_UnregisterHotkey(HWND hwnd, int i)
{
    TraceMsg(TF_TRAY, "c.t_uh: Unregistering hotkey (%d).", i);

    if (!UnregisterHotKey(hwnd, i))
    {
        DebugMsg(DM_ERROR, TEXT("c.t_rh: Unable to unregister hotkey %d."), i);
    }
    return TRUE;
}

//---------------------------------------------------------------------------
// Add hotkey to the shell's list of global hotkeys.
LRESULT Tray_ShortcutRegisterHotkey(HWND hwnd, WORD wHotkey, ATOM atom)
{
    int i;
    LPITEMIDLIST pidl;
    TCHAR szPath[MAX_PATH];
    ASSERT(atom);

    if (GlobalGetAtomName(atom, szPath, MAX_PATH))
    {
        TraceMsg(TF_TRAY, "c.t_srh: Hotkey %d for %s", wHotkey, szPath);

        pidl = ILCreateFromPath(szPath);
        if (pidl)
        {
            i = HotkeyList_AddCached(MapHotkeyToGlobalHotkey(wHotkey), pidl);
            if (i != -1)
            {
                Tray_RegisterHotkey(v_hwndTray, i);
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//---------------------------------------------------------------------------
// Remove hotkey from shell's list.
LRESULT Tray_ShortcutUnregisterHotkey(HWND hwnd, WORD wHotkey)
{
    int i;

    // DebugMsg(DM_TRACE, "c.t_suh: Hotkey %d", wHotkey);

    i = HotkeyList_Remove(wHotkey);
    if (i == -1)
        i = HotkeyList_RemoveCached(MapHotkeyToGlobalHotkey(wHotkey));

    if (i != -1)
        Tray_UnregisterHotkey(hwnd, i);

    return TRUE;
}

//---------------------------------------------------------------------------
LRESULT Tray_RegisterHotkey(HWND hwnd, int i)
{
    PHOTKEYITEM phki;
    WORD wGHotkey;
    int iCached;

    ASSERT(IS_VALID_HANDLE(g_ts.hdsaHKI, DSA));

    TraceMsg(TF_TRAY, "c.t_rh: Registering hotkey (%d).", i);

    phki = DSA_GetItemPtr(g_ts.hdsaHKI, i);
    ASSERT(phki);
    if (phki)
    {
        wGHotkey = phki->wGHotkey;
        if (wGHotkey)
        {
            // Is the hotkey available?
            if (RegisterHotKey(hwnd, i, HIBYTE(wGHotkey), LOBYTE(wGHotkey)))
            {
                // Yes.
                return TRUE;
            }
            else
            {
                // Delete any cached items that might be using this
                // hotkey.
                iCached = HotkeyList_RemoveCached(wGHotkey);
                ASSERT(iCached != i);
                if (iCached != -1)
                {
                    // Free up the hotkey for us.
                    Tray_UnregisterHotkey(hwnd, iCached);
                    // Yep, nuked the cached item. Try again.
                    if (RegisterHotKey(hwnd, i, HIBYTE(wGHotkey), LOBYTE( wGHotkey)))
                    {
                        return TRUE;
                    }
                }
            }

            // Can't set hotkey for this item.
            DebugMsg(DM_ERROR, TEXT("c.t_rh: Unable to register hotkey %d."), i);
            // Null out this item.
            phki->wGHotkey = 0;
            phki->pidlFolder = NULL;
            phki->pidlItem = NULL;
        }
        else
        {
            DebugMsg(DM_ERROR, TEXT("c.t_rh: Hotkey item is invalid."));
        }
    }
    return FALSE;
}


void AppBarGetTaskBarPos(PTRAYAPPBARDATA ptabd)
{
    APPBARDATA *pabd;

    pabd = (APPBARDATA*)SHLockShared(ptabd->hSharedABD, ptabd->dwProcId);
    if (pabd)
    {
        pabd->rc = g_ts.arStuckRects[g_ts.uStuckPlace];
        pabd->uEdge = g_ts.uStuckPlace;     // compat: new to ie4
        SHUnlockShared(pabd);
    }
}

void NukeAppBar(int i)
{
    LocalFree(DPA_GetPtr(g_ts.hdpaAppBars, i));
    DPA_DeletePtr(g_ts.hdpaAppBars, i);
}

void AppBarRemove(PTRAYAPPBARDATA ptabd)
{
    int i;

    if (!g_ts.hdpaAppBars)
        return;

    i = DPA_GetPtrCount(g_ts.hdpaAppBars);

    while (i--)
    {
        PAPPBAR pab = (PAPPBAR)DPA_GetPtr(g_ts.hdpaAppBars, i);

        if (ptabd->abd.hWnd == pab->hwnd)
        {
            RECT rcNuke = pab->rc;

            NukeAppBar(i);
            StuckAppChange(ptabd->abd.hWnd, &rcNuke, NULL, FALSE);
        }
    }
}

PAPPBAR FindAppBar(HWND hwnd)
{
    if (g_ts.hdpaAppBars)
    {
        int i = DPA_GetPtrCount(g_ts.hdpaAppBars);

        while (i--)
        {
            PAPPBAR pab = (PAPPBAR)DPA_GetPtr(g_ts.hdpaAppBars, i);

            if (hwnd == pab->hwnd)
                return pab;
        }
    }

    return NULL;
}

void AppBarNotifyAll(HMONITOR hmon, UINT uMsg, HWND hwndExclude, LPARAM lParam)
{
    int i;

    if (!g_ts.hdpaAppBars)
        return;

    i = DPA_GetPtrCount(g_ts.hdpaAppBars);

    while (i--)
    {
        PAPPBAR pab = (PAPPBAR)DPA_GetPtr(g_ts.hdpaAppBars, i);

        // We need to check pab here as an appbar can delete other
        // appbars on the callback.
        if (pab && (hwndExclude != pab->hwnd))
        {
            if (!IsWindow(pab->hwnd))
            {
                NukeAppBar(i);
                continue;
            }

            //
            // if a monitor was specified only tell appbars on that display
            //
            if (hmon &&
                (hmon != MonitorFromWindow(pab->hwnd, MONITOR_DEFAULTTONULL)))
            {
                continue;
            }

            SendMessage(pab->hwnd, pab->uCallbackMessage, uMsg, lParam);
        }
    }
}

BOOL AppBarNew(PTRAYAPPBARDATA ptabd)
{
    PAPPBAR pab;
    if (!g_ts.hdpaAppBars) {
        g_ts.hdpaAppBars = DPA_Create(4);
        if (!g_ts.hdpaAppBars)
            return FALSE;

    } else if (FindAppBar(ptabd->abd.hWnd)) {

        // already have this hwnd
        return FALSE;
    }

    pab = (PAPPBAR)LocalAlloc(LPTR, SIZEOF(APPBAR));
    if (!pab)
        return FALSE;

    pab->hwnd = ptabd->abd.hWnd;
    pab->uCallbackMessage = ptabd->abd.uCallbackMessage;
    pab->uEdge = (UINT)-1;

    return ( DPA_AppendPtr(g_ts.hdpaAppBars, pab) != -1);
}


UINT CDeskTray_AppBarGetState(IDeskTray* pdt)
{
    return (g_ts.uAutoHide ?  ABS_AUTOHIDE    : 0) |
        (g_ts.fAlwaysOnTop ?  ABS_ALWAYSONTOP : 0);
}

BOOL AppBarOutsideOf(PAPPBAR pabReq, PAPPBAR pab)
{
    if (pabReq->uEdge == pab->uEdge) {
        switch (pab->uEdge) {
        case ABE_RIGHT:
            return (pab->rc.right >= pabReq->rc.right);

        case ABE_BOTTOM:
            return (pab->rc.bottom >= pabReq->rc.bottom);

        case ABE_TOP:
            return (pab->rc.top <= pabReq->rc.top);

        case ABE_LEFT:
            return (pab->rc.left <= pabReq->rc.left);
        }
    }
    return FALSE;
}

void AppBarQueryPos(PTRAYAPPBARDATA ptabd)
{
    int i;
    PAPPBAR pabReq = FindAppBar(ptabd->abd.hWnd);

    if (pabReq)
    {
        APPBARDATA *pabd;

        pabd = (APPBARDATA*)SHLockShared(ptabd->hSharedABD, ptabd->dwProcId);
        if (pabd)
        {
            HMONITOR hmon;

            pabd->rc = ptabd->abd.rc;

            //
            // default to the primary display for this call because old appbars
            // sometimes pass a huge rect and let us pare it down.  if they do
            // something like that they don't support multiple displays anyway
            // so just put them on the primary display...
            //
            hmon = MonitorFromRect(&pabd->rc, MONITOR_DEFAULTTOPRIMARY);

            //
            // always subtract off the tray if it's on the same display
            //
            if (!g_ts.uAutoHide && (hmon == g_ts.hmonStuck))
            {
                APPBAR ab;

                ab.uEdge = Tray_GetDockedRect(&ab.rc, FALSE);
                AppBarSubtractRect(&ab, &pabd->rc);
            }

            i = DPA_GetPtrCount(g_ts.hdpaAppBars);

            while (i--)
            {
                PAPPBAR pab = (PAPPBAR)DPA_GetPtr(g_ts.hdpaAppBars, i);

                //
                // give top and bottom preference
                // ||
                // if we're not changing edges,
                //          subtract anything currently on the outside of us
                // ||
                // if we are changing sides,
                //          subtract off everything on the new side.
                //
                // of course ignore appbars which are not on the same display...
                //
                if ((((pabReq->hwnd != pab->hwnd) &&
                    STUCK_HORIZONTAL(pab->uEdge) &&
                    !STUCK_HORIZONTAL(ptabd->abd.uEdge)) ||
                    ((pabReq->hwnd != pab->hwnd) &&
                    (pabReq->uEdge == ptabd->abd.uEdge) &&
                    AppBarOutsideOf(pabReq, pab)) ||
                    ((pabReq->hwnd != pab->hwnd) &&
                    (pabReq->uEdge != ptabd->abd.uEdge) &&
                    (pab->uEdge == ptabd->abd.uEdge))) &&
                    (hmon == MonitorFromRect(&pab->rc, MONITOR_DEFAULTTONULL)))
                {
                    AppBarSubtractRect(pab, &pabd->rc);
                }
            }
            SHUnlockShared(pabd);
        }
    }
}

void AppBarSetPos(PTRAYAPPBARDATA ptabd)
{
    PAPPBAR pab = FindAppBar(ptabd->abd.hWnd);

    if (pab)
    {
        RECT rcOld;
        APPBARDATA *pabd;
        BOOL fChanged = FALSE;

        AppBarQueryPos(ptabd);

        pabd = (APPBARDATA*)SHLockShared(ptabd->hSharedABD, ptabd->dwProcId);
        if (pabd)
        {
            if (!EqualRect(&pab->rc, &pabd->rc)) {
                rcOld = pab->rc;
                pab->rc = pabd->rc;
                pab->uEdge = ptabd->abd.uEdge;
                fChanged = TRUE;
            }
            SHUnlockShared(pabd);
        }

        if (fChanged)
            StuckAppChange(ptabd->abd.hWnd, &rcOld, &pab->rc, FALSE);
    }
}

//
// BUGBUG: need to get rid of this array-based implementation to allow autohide
// appbars on secondary display (or a/h tray on 2nd with a/h appbar on primary)
// change it to an AppBarFindAutoHideBar that keeps flags on the appbardata...
//
HWND AppBarGetAutoHideBar(UINT uEdge)
{
    if (uEdge >= ABE_MAX)
        return FALSE;
    else {
        HWND hwndAutoHide = g_hwndAutoHide[uEdge];
        if (!IsWindow(hwndAutoHide)) {
            g_hwndAutoHide[uEdge] = NULL;
        }
        return g_hwndAutoHide[uEdge];
    }
}

BOOL IAppBarSetAutoHideBar(HWND hwnd, BOOL fAutoHide, UINT uEdge)
{
    HWND hwndAutoHide = g_hwndAutoHide[uEdge];
    if (!IsWindow(hwndAutoHide))
    {
        g_hwndAutoHide[uEdge] = NULL;
    }

    if (fAutoHide)
    {
        // register
        if (!g_hwndAutoHide[uEdge])
        {
            g_hwndAutoHide[uEdge] = hwnd;
        }

        return g_hwndAutoHide[uEdge] == hwnd;
    }
    else
    {
        // unregister
        if (g_hwndAutoHide[uEdge] == hwnd)
        {
            g_hwndAutoHide[uEdge] = NULL;
        }

        return TRUE;
    }
}

BOOL AppBarSetAutoHideBar(PTRAYAPPBARDATA ptabd)
{
    UINT uEdge = ptabd->abd.uEdge;
    if (uEdge >= ABE_MAX)
        return FALSE;
    else {
        return IAppBarSetAutoHideBar(ptabd->abd.hWnd, BOOLFROMPTR(ptabd->abd.lParam), uEdge);
    }
}

void IAppBarActivationChange(HWND hwnd, UINT uEdge)
{
    //
    // BUGBUG: make this multi-monitor cool
    //
    HWND hwndAutoHide = AppBarGetAutoHideBar(uEdge);

    if (hwndAutoHide && (hwndAutoHide != hwnd))
    {
        //
        // the AppBar got this notification inside a SendMessage from USER
        // and is now in a SendMessage to us.  don't try to do a SetWindowPos
        // right now...
        //
        PostMessage(v_hwndTray, TM_BRINGTOTOP, (WPARAM)hwndAutoHide, uEdge);
    }
}

void AppBarActivationChange(PTRAYAPPBARDATA ptabd)
{
    PAPPBAR pab = FindAppBar(ptabd->abd.hWnd);

    if (pab) {
        UINT i;
        // if this is an autohide bar and they're claiming to be on an edge not the same as their autohide edge,
        // we don't do any activation of other autohides
        for (i = 0; i < ABE_MAX; i++) {
            if (g_hwndAutoHide[i] == ptabd->abd.hWnd &&
                i != pab->uEdge)
                return;
        }
        IAppBarActivationChange(ptabd->abd.hWnd, pab->uEdge);
    }
}

LRESULT TrayAppBarMessage(PCOPYDATASTRUCT pcds)
{
    PTRAYAPPBARDATA ptabd = (PTRAYAPPBARDATA)pcds->lpData;

    ASSERT(pcds->cbData == SIZEOF(TRAYAPPBARDATA));
    ASSERT(ptabd->abd.cbSize == SIZEOF(APPBARDATA));

    switch (ptabd->dwMessage) {
    case ABM_NEW:
        return AppBarNew(ptabd);

    case ABM_REMOVE:
        AppBarRemove(ptabd);
        break;

    case ABM_QUERYPOS:
        AppBarQueryPos(ptabd);
        break;

    case ABM_SETPOS:
        AppBarSetPos(ptabd);
        break;

    case ABM_GETSTATE:
        return CDeskTray_AppBarGetState(NULL);

    case ABM_GETTASKBARPOS:
        AppBarGetTaskBarPos(ptabd);
        break;

    case ABM_WINDOWPOSCHANGED:
    case ABM_ACTIVATE:
        AppBarActivationChange(ptabd);
        break;

    case ABM_GETAUTOHIDEBAR:
        return (LRESULT)AppBarGetAutoHideBar(ptabd->abd.uEdge);

    case ABM_SETAUTOHIDEBAR:
        return AppBarSetAutoHideBar(ptabd);

    default:
        return FALSE;
    }

    return TRUE;

}

// EA486701-7F92-11cf-9E05-444553540000
const GUID CLSID_HIJACKINPROC = {0xEA486701, 0x7F92, 0x11cf, 0x9E, 0x05, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

UINT TrayLoadInProc(PCOPYDATASTRUCT pcds)
{
    IUnknown *punk;
    HRESULT hres;

    // Hack to allow us to kill W95 shell extensions that do reall hacky things that
    // we can not support.    In this case Hijack pro

    if (IsEqualIID((CLSID *)pcds->lpData, &CLSID_HIJACKINPROC))
        return (UINT)E_FAIL;

    hres = SHCoCreateInstance(NULL, (CLSID *)pcds->lpData, NULL, &IID_IUnknown, &punk);
    if (SUCCEEDED(hres))
    {
        punk->lpVtbl->Release(punk);
    }
    return (UINT)hres;
}


//---------------------------------------------------------------------------
// Allow the trays global hotkeys to be disabled for a while.
LRESULT Tray_SetHotkeyEnable(HWND hwnd, BOOL fEnable)
{
    g_ts.fGlobalHotkeyDisable = (fEnable ? FALSE : TRUE);
    return TRUE;
}


BOOL IsPosInHwnd(LPARAM lParam, HWND hwnd)
{
    RECT r1;
    POINT pt;

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    GetWindowRect(hwnd, &r1);
    return PtInRect(&r1, pt);
}

void Tray_HandleWindowPosChanging(LPWINDOWPOS lpwp)
{
    DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc"));

    if (g_ts.uMoveStuckPlace != (UINT)-1)
    {
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc handling pending move"));
        Tray_DoneMoving(lpwp);
    }
    else if (g_ts.fSysSizing || !g_ts.fSelfSizing)
    {
        RECT rc;

        if (g_ts.fSysSizing)
        {
            GetWindowRect(v_hwndTray, &rc);
            if (!(lpwp->flags & SWP_NOMOVE))
            {
                rc.left = lpwp->x;
                rc.top = lpwp->y;
            }
            if (!(lpwp->flags & SWP_NOSIZE))
            {
                rc.right = rc.left + lpwp->cx;
                rc.bottom = rc.top + lpwp->cy;
            }

            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc sys sizing to rect {%d, %d, %d, %d}"), rc.left, rc.top, rc.right, rc.bottom);

            g_ts.uStuckPlace = Tray_RecalcStuckPos(&rc);
        }

        Tray_GetDockedRect(&rc, g_ts.fSysSizing);

        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.t_hwpc using rect {%d, %d, %d, %d}"), rc.left, rc.top, rc.right, rc.bottom);

        lpwp->x = rc.left;
        lpwp->y = rc.top;
        lpwp->cx = RECTWIDTH(rc);
        lpwp->cy = RECTHEIGHT(rc);

        lpwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
    }
}


//---------------------------------------------------------------------------
void Tray_HandlePowerStatus(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResetDisplay = FALSE;

    //
    // always reset the display when the machine wakes up from a
    // suspend.  NOTE: we don't need this for a standby suspend.
    //
    // a critical resume does not generate a WM_POWERBROADCAST
    // to windows for some reason, but it does generate an old
    // WM_POWER message.
    //
    switch (uMsg)
    {
    case WM_POWER:
        fResetDisplay = (wParam == PWR_CRITICALRESUME);
        break;

#ifndef WINNT       // BUGBUG - Fix this when NT gets APM
    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
            fResetDisplay = TRUE;
            break;
        }
        break;
#endif
    }

    if (fResetDisplay)
        ChangeDisplaySettings(NULL, CDS_RESET);
}

BOOL LoadShellServiceObjects(HKEY hkeyParent, LPCTSTR szSubkey);

//---------------------------------------------------------------------------
void Tray_HandleShellServiceObject(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case SSOCMDID_LOAD:
            LoadShellServiceObjects(HKEY_LOCAL_MACHINE, REGSTR_PATH_SHELLSERVICEOBJECTDELAYED);
            LoadShellServiceObjects(HKEY_CURRENT_USER,  REGSTR_PATH_SHELLSERVICEOBJECTDELAYED);
            break;

        case SSOCMDID_OPEN:
            CTExecShellServiceObjects(NULL, SSOCMDID_OPEN, 0, 0);
            break;

        case SSOCMDID_CLOSE:
            // This can be called multiple times (because we can quit Explorer
            // in a number of places) so we need to make sure we don't send
            // this message multiple times so free the DSA when done.
            if (g_hdsaShellServiceObjects)
            {
                CTExecShellServiceObjects(NULL, SSOCMDID_CLOSE, 0, CTEXECSSOF_REVERSE);
                DSA_Destroy(g_hdsaShellServiceObjects);
                g_hdsaShellServiceObjects=NULL;
            }
            break;
    }
}



// 
// Try tacking a 1, 2, 3 or whatever on to a file or
// directory name until it is unique.  When on a file, 
// stick it before the extension.
// 
void   MakeBetterUniqueName(LPTSTR pszPathName)
{
    TCHAR   szNewPath[MAX_PATH];
    int     i = 1;

    if (PathIsDirectory(pszPathName))
    {
        do 
        {
            wsprintf(szNewPath, TEXT("%s%d"), pszPathName, i++);
        } while (-1 != GetFileAttributes(szNewPath));

        lstrcpy(pszPathName, szNewPath);
    }
    else
    {
        TCHAR   szExt[MAX_PATH];
        LPTSTR  pszExt;

        pszExt = PathFindExtension(pszPathName);
        lstrcpy(szExt, pszExt);
        *pszExt = 0;

        do 
        {
            wsprintf(szNewPath, TEXT("%s%d%s"), pszPathName, i++,szExt);
        } while (-1 != GetFileAttributes(szNewPath));

        lstrcpy(pszPathName, szNewPath);
    }
}

static BOOL_PTR CALLBACK RogueProgramFileDlgProc(HWND       hWnd,
                                   UINT     iMsg, 
                                   WPARAM   wParam, 
                                   LPARAM   lParam)
{
    TCHAR   szBuffer[MAX_PATH*2];
    TCHAR   szBuffer2[MAX_PATH*2];
    static TCHAR   szBetterPath[MAX_PATH];
    static TCHAR  *pszPath = NULL;
    
    switch (iMsg)
    {
    case WM_INITDIALOG:
        pszPath = (TCHAR *)lParam;
        lstrcpy(szBetterPath, pszPath);
        MakeBetterUniqueName(szBetterPath);
        SendDlgItemMessage(hWnd, IDC_MSG, WM_GETTEXT, (WPARAM)(MAX_PATH*2), (LPARAM)szBuffer);
        wsprintf(szBuffer2, szBuffer, pszPath, szBetterPath);
        SendDlgItemMessage(hWnd, IDC_MSG, WM_SETTEXT, (WPARAM)0, (LPARAM)szBuffer2);

        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_RENAME:
            //rename and fall through
            if (pszPath)
            {
                MoveFile(pszPath, szBetterPath);
            }
            EndDialog(hWnd, IDC_RENAME);
            return TRUE;

        case IDIGNORE:
            EndDialog(hWnd, IDIGNORE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

//
// Check to see if there are any files or folders that could interfere
// with the fact that Program Files has a space in it.
//
// An example would be a directory called: "C:\Program" or a file called"C:\Program.exe".
//
// This can prevent apps that dont quote strings in the registry or call CreateProcess with 
// unquoted strings from working properly since CreateProcess wont know what the real exe is.
//
void Tray_CheckForRogueProgramFile()
{
    TCHAR szProgramFilesPath[MAX_PATH];
    TCHAR szProgramFilesShortName[MAX_PATH];

    if (S_OK == SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szProgramFilesPath))
    {
        LPTSTR pszRoguePattern;

        pszRoguePattern = StrChr(szProgramFilesPath, TEXT(' '));
        
        if (pszRoguePattern)
        {
            HANDLE  hFind;
            WIN32_FIND_DATA wfd;

            // Remember short name for folder name comparison below
            *pszRoguePattern = TEXT('\0');
            lstrcpy(szProgramFilesShortName, szProgramFilesPath);

            // turn "C:\program files" into "C:\program.*"
            lstrcpy(pszRoguePattern, TEXT(".*"));
            pszRoguePattern = szProgramFilesPath;

            hFind = FindFirstFile(pszRoguePattern, &wfd);

            while (hFind != INVALID_HANDLE_VALUE)
            {
                int iRet;
                TCHAR szRogueFileName[MAX_PATH];

                // we found a file (eg "c:\Program.txt")
                lstrcpyn(szRogueFileName, pszRoguePattern, ARRAYSIZE(szRogueFileName));
                PathRemoveFileSpec(szRogueFileName);
                lstrcatn(szRogueFileName, wfd.cFileName, ARRAYSIZE(szRogueFileName));

                // don't worry about folders unless they are called "Program"
                if (!((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    lstrcmpi(szProgramFilesShortName, szRogueFileName) != 0))
                {
                        
                    iRet = SHMessageBoxCheckEx(GetDesktopWindow(),
                                               hinstCabinet,
                                               MAKEINTRESOURCE(DLG_PROGRAMFILECONFLICT),
                                               RogueProgramFileDlgProc,
                                               (LPVOID)szRogueFileName,
                                               IDIGNORE,
                                               TEXT("RogueProgramName"));
                }
                
                if ((iRet == IDIGNORE) || !FindNextFile(hFind, &wfd))
                {
                    // user hit ignore or we are done, so don't keep going
                    break;
                }
            }

            if (hFind != INVALID_HANDLE_VALUE)
            {
                FindClose(hFind);
            }
        }
    }
}


void Tray_OnWaitCursorNotify(LPNMHDR pnm)
{
    g_ts.iWaitCount += (pnm->code == NM_STARTWAIT ? 1 :-1);
    ASSERT(g_ts.iWaitCount >= 0);
    // Don't let it go negative or we'll never get rid of it.
    if (g_ts.iWaitCount < 0)
        g_ts.iWaitCount = 0;
    // what we really want is for user to simulate a mouse move/setcursor
    SetCursor(LoadCursor(NULL, g_ts.iWaitCount ? IDC_APPSTARTING : IDC_ARROW));
}

void Tray_HandlePrivateCommand(LPARAM lParam)
{
    LPSTR psz = (LPSTR) lParam; // lParam always ansi.

    if (!lstrcmpiA(psz, "ToggleDesktop")) {
        ToggleDesktop();
    } else if (!lstrcmpiA(psz, "Explorer")) {
        // Fast way to bring up explorer window on root of
        // windows dir.
        SHELLEXECUTEINFO shei = {0};
        TCHAR szPath[MAX_PATH];

        GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
        PathStripToRoot(szPath);

        shei.lpIDList = ILCreateFromPath(szPath);
        if (shei.lpIDList)
        {
            shei.cbSize     = sizeof(shei);
            shei.fMask      = SEE_MASK_IDLIST;
            shei.nShow      = SW_SHOWNORMAL;
            shei.lpVerb     = TEXT("explore");
            ShellExecuteEx(&shei);
            ILFree(shei.lpIDList);
        }
    }

    LocalFree(psz);
}

//***
//
void Tray_OnFocusMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fActivate = (BOOL) wParam;

    switch (uMsg)
    {
    case TM_UIACTIVATEIO:
    {
        int dtb = (int) lParam;

        TraceMsg(DM_FOCUS, "tiois: TM_UIActIO fAct=%d dtb=%d", fActivate, dtb);

        ASSERT(dtb == 1 || dtb == -1);

        if (fActivate)
        {
            // Since we are tabbing into the tray, turn the focus rect on.
            SendMessage(g_ts.hwndMain, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);

            SendMessage(v_hwndDesktop, DTM_ONFOCUSCHANGEIS, TRUE, (LPARAM) v_hwndTray);
            SetForegroundWindow(v_hwndTray);

            // fake an UnkUIActivateIO(g_ts.ptbs, TRUE, &msg);
            TraceMsg(DM_FOCUS, "tiois: TM_UIActIO fake UIAct");
            if (GetAsyncKeyState(VK_SHIFT))
            {
                if (g_hwndDesktopTB && g_ts.fShowDeskBtn)
                    Tray_SetFocus(g_hwndDesktopTB);
                else
                    Tray_SetFocus(g_ts.hwndNotify);
            }
            else
            {
                Tray_SetFocus(g_ts.hwndStart);
            }
        }
        else
        {
Ldeact:
            UnkUIActivateIO(g_ts.ptbs, FALSE, NULL);
            SetForegroundWindow(v_hwndDesktop);
        }

        break;
    }

    case TM_ONFOCUSCHANGEIS:
    {
        HWND hwnd = (HWND) lParam;

        TraceMsg(DM_FOCUS, "tiois: TM_OnFocChgIS hwnd=%x fAct=%d", hwnd, fActivate);

        if (fActivate)
        {
            // someone else is activating, so we need to deactivate
            goto Ldeact;
        }

        break;
    }

    default:
        ASSERT(0);
        break;
    }

    return;
}

#define TSVC_NTIMER     (IDT_SERVICELAST - IDT_SERVICE0 + 1)

struct {
#ifdef DEBUG
    UINT_PTR    idtWin;
#endif
    TIMERPROC   pfnSvc;
} g_timerService[TSVC_NTIMER];

#define TSVC_IDToIndex(id)    ((id) - IDT_SERVICE0)
#define TSVC_IndexToID(i)     ((i) + IDT_SERVICE0)

int Tray_OnTimerService(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int i;
    UINT_PTR idt;
    TIMERPROC pfn;
    BOOL b;

    switch (uMsg) {
    case TM_SETTIMER:
        TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: wP=0x%x lP=%x", wParam, lParam);
        ASSERT(IS_VALID_CODE_PTR(lParam, TIMERPROC));
        for (i = 0; i < TSVC_NTIMER; i++) {
            if (g_timerService[i].pfnSvc == 0) {
                g_timerService[i].pfnSvc = (TIMERPROC)lParam;
                idt = SetTimer(v_hwndTray, TSVC_IndexToID(i), (UINT)wParam, 0);
                if (idt == 0) {
                    TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: ST()=%d (!)", idt);
                    break;
                }
                ASSERT(idt == (UINT_PTR)TSVC_IndexToID(i));
                DBEXEC(TRUE, (g_timerService[i].idtWin = idt));
                TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: ret=0x%x", TSVC_IndexToID(i));
                return TSVC_IndexToID(i);   // idtWin
            }
        }
        TraceMsg(DM_UEMTRACE, "e.TM_SETTIMER: ret=0 (!)");
        return 0;

    case TM_KILLTIMER:  // lP=idtWin
        TraceMsg(DM_UEMTRACE, "e.TM_KILLTIMER: wP=0x%x lP=%x", wParam, lParam);
        if (EVAL(IDT_SERVICE0 <= lParam && lParam <= IDT_SERVICE0 + TSVC_NTIMER - 1)) {
            i = (int)TSVC_IDToIndex(lParam);
            if (g_timerService[i].pfnSvc) {
                ASSERT(g_timerService[i].idtWin == (UINT)lParam);
                b = KillTimer(v_hwndTray, lParam);
                ASSERT(b);
                g_timerService[i].pfnSvc = 0;
                DBEXEC(TRUE, (g_timerService[i].idtWin = 0));
                return TRUE;
            }
        }
        return 0;

    case WM_TIMER:      // wP=idtWin lP=0
        TraceMsg(DM_UEMTRACE, "e.TM_TIMER: wP=0x%x lP=%x", wParam, lParam);
        if (EVAL(IDT_SERVICE0 <= wParam && wParam <= IDT_SERVICE0 + TSVC_NTIMER - 1)) {
            i = (int)TSVC_IDToIndex(wParam);
            pfn = g_timerService[i].pfnSvc;
            if (EVAL(IS_VALID_CODE_PTR(pfn, TIMERPROC)))
                (*pfn)(v_hwndTray, WM_TIMER, wParam, GetTickCount());
        }
        return 0;
    }

    ASSERT(0);      /*NOTREACHED*/
    return 0;
}

int g_fInSizeMove;

UINT GetDDEExecMsg()
{
    static UINT uDDEExec = 0;

    if (!uDDEExec)
        uDDEExec = RegisterWindowMessage(TEXT("DDEEXECUTESHORTCIRCUIT"));

    return uDDEExec;
}

extern void Task_RealityCheck(HWND hwndView);

void Tray_RealityCheck()
{
    //
    // Make sure that the tray's actual z-order position agrees with what we think
    // it is.  We need to do this because there's a recurring bug where the tray
    // gets bumped out of TOPMOST position.  (Lots of things, like a tray-owned
    // window moving itself to non-TOPMOST or a random app messing with the tray
    // window position, can cause this.)
    //
    Tray_ResetZorder();

    //
    // The taskbar needs to do some reality-checking as well (for things like
    // ghost app buttons).
    //
    Task_RealityCheck(g_ts.hwndView);
}

void Tray_HandleDelayBootStuff()
{
    // This posted message is the last one processed by the primary
    // thread (tray thread) when we boot.  At this point we will
    // want to load the shell services (which usually create threads)
    // and resume both the background start menu thread and the fs_notfiy
    // thread.

    if (!g_fHandledDelayBootStuff)
    {
        IShellTaskScheduler* pScheduler;

        g_fHandledDelayBootStuff = TRUE;
        PostMessage(v_hwndTray, TM_SHELLSERVICEOBJECTS, SSOCMDID_LOAD, 0);
        PostMessage(v_hwndTray, TM_SHELLSERVICEOBJECTS, SSOCMDID_OPEN, 0);

        BandSite_HandleDelayBootStuff(g_ts.ptbs);

        // get the system background scheduler thread
        if (SUCCEEDED(CoCreateInstance(&CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC,
                                       &IID_IShellTaskScheduler, (void **) &pScheduler)))
        {
            const static c_rgcsidl[] =
            {
                CSIDL_STARTMENU,
                CSIDL_COMMON_STARTMENU,
                // Don't do programs, since normally programs is already
                // under startmenu
                // CSIDL_PROGRAMS,
                // CSIDL_COMMON_PROGRAMS,
                // Don't do favorites since it is huge and better done on demand.
                // CSIDL_FAVORITES,
                CSIDL_RECENT
            };
            int i;

            StartMenu_AddTask(pScheduler, CSIDL_DESKTOPDIRECTORY, THREAD_PRIORITY_NORMAL);

            if (!SHRestricted(REST_NOCOMMONGROUPS))
                StartMenu_AddTask(pScheduler, CSIDL_COMMON_DESKTOPDIRECTORY, THREAD_PRIORITY_NORMAL);


            // Add the tasks that enumerate to set the hotkeys and
            // build the startmenu
            for (i = 0; i < ARRAYSIZE(c_rgcsidl); i++)
            {
                StartMenu_AddTask(pScheduler, c_rgcsidl[i], THREAD_PRIORITY_BELOW_NORMAL);
            }

            pScheduler->lpVtbl->Release(pScheduler);
        }
        
        //check to see if there are any files or folders that could interfere
        //with the fact that Program Files has a space in it.  An example would
        //be a folder called "C:\Program" or a file called "C:\Program.exe"
        Tray_CheckForRogueProgramFile();

        // We spin a thread that will process "Load=", "Run=", CU\Run, and CU\RunOnce
        RunStartupApps();

#ifdef WINNT
        // NT5 hack, create a named event and fire it so that the services can
        // go to work, giving the illusion that we booted faster
        g_hShellReadyEvent = CreateEvent(0, FALSE, FALSE, TEXT("ShellReadyEvent"));
        if ( g_hShellReadyEvent )
            SetEvent( g_hShellReadyEvent );
#endif
    }
}

//---------------------------------------------------------------------------
LRESULT CALLBACK Tray_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static  UINT uDDEExec = 0;
    LRESULT lres = 0;
    RECT r1;
    POINT pt;
    DWORD dw;
    MSG msg;

    msg.hwnd = hwnd;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;

    if (g_ts._pmbStartMenu &&
        g_ts._pmbStartMenu->lpVtbl->TranslateMenuMessage(g_ts._pmbStartMenu, &msg, &lres) == S_OK)
        return lres;

    wParam = msg.wParam;
    lParam = msg.lParam;

    INSTRUMENT_WNDPROC(SHCNFI_TRAY_WNDPROC, hwnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WMTRAY_PRINTCHANGE:
        {
            LPSHChangeNotificationLock pshcnl;
            LPITEMIDLIST *ppidl;
            LONG lEvent;

            pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
            if (pshcnl)
            {
                // BUGBUG (scotth): shouldn't need to filter out update images,
                //                  but to avoid regressions so close to beta 1...

                if (SHCNE_UPDATEIMAGE != lEvent)
                    PrintNotify_HandleFSNotify(g_ts.hwndMain, ppidl, lEvent);
                lres = 1;
                SHChangeNotification_Unlock(pshcnl);
            }
            break;
        }

    case WMTRAY_PRINTICONNOTIFY:
        PrintNotify_IconNotify(hwnd, lParam);
        break;

    case WMTRAY_REGISTERHOTKEY:
        return Tray_RegisterHotkey(hwnd, (int)wParam);

    case WMTRAY_UNREGISTERHOTKEY:
        return Tray_UnregisterHotkey(hwnd, (int)wParam);

    case WMTRAY_SCREGISTERHOTKEY:
        return Tray_ShortcutRegisterHotkey(hwnd, (WORD)wParam, (ATOM)lParam);

    case WMTRAY_SCUNREGISTERHOTKEY:
        return Tray_ShortcutUnregisterHotkey(hwnd, (WORD)wParam);

    case WMTRAY_SETHOTKEYENABLE:
        return Tray_SetHotkeyEnable(hwnd, (BOOL)wParam);

    case WMTRAY_QUERY_MENU:
        return (LRESULT)g_ts.hmenuStart;

    case WMTRAY_QUERY_VIEW:
        return (LRESULT)g_ts.hwndView;

    case WM_COPYDATA:
        // Check for NULL it can happen if user runs out of selectors or memory...
        if (lParam)
        {
            switch (((PCOPYDATASTRUCT)lParam)->dwData) {
            case TCDM_NOTIFY:
            {
                BOOL bRefresh = TRUE;

                lres = TrayNotify(g_ts.hwndNotify, (HWND)wParam, (PCOPYDATASTRUCT)lParam, &bRefresh);
                if(bRefresh)
                {
                    Tray_SizeWindows();
                }
                return(lres);
            }
            
            case TCDM_APPBAR:
                return TrayAppBarMessage((PCOPYDATASTRUCT)lParam);

            case TCDM_LOADINPROC:
                return TrayLoadInProc((PCOPYDATASTRUCT)lParam);
            }
        }
        return FALSE;



    case WM_NCLBUTTONDBLCLK:
        if (IsPosInHwnd(lParam, g_ts.hwndNotify)) {
            Tray_Command(IDM_SETTIME);

            // Hack!  If you click on the tray clock, this tells the tooltip
            // "Hey, I'm using this thing; stop putting up a tip for me."
            // You can get the tooltip to lose track of when it needs to
            // reset the "stop it!" flag and you get stuck in "stop it!" mode.
            // It's particularly easy to make happen on Terminal Server.
            //
            // So let's assume that the only reason people click on the
            // tray clock is to change the time.  when they change the time,
            // kick the tooltip in the head to reset the "stop it!" flag.

            SendMessage(g_ts.hwndTrayTips, TTM_POP, 0, 0);
        }
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMOUSEMOVE:
        if (IsPosInHwnd(lParam, g_ts.hwndNotify)) {
            MSG msg;
            msg.lParam = lParam;
            msg.wParam = wParam;
            msg.message = uMsg;
            msg.hwnd = hwnd;
            SendMessage(g_ts.hwndTrayTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
            if (uMsg == WM_NCLBUTTONDOWN)
                Tray_SetFocus(g_ts.hwndNotify);
        }

        goto DoDefault;

    case WM_CREATE:
        return Tray_OnCreate(hwnd);
        break;

    case WM_DESTROY:
        return Tray_HandleDestroy();

#ifdef DEBUG
    case WM_QUERYENDSESSION:
        TraceMsg(DM_SHUTDOWN, "Tray.wp WM_QUERYENDSESSION");
        goto DoDefault;
#endif

    case WM_ENDSESSION:
        // save our settings if we are shutting down
        TraceMsg(DM_SHUTDOWN, "Tray.wp WM_ENDSESSION wP=%d lP=%d", wParam, lParam);
        if (wParam) 
        {
            TraceMsg(TF_TRAY, "Tray WM_ENDSESSION");
            _SaveTrayAndDesktop();

            ShowWindow(v_hwndTray, SW_HIDE);
            ShowWindow(v_hwndDesktop, SW_HIDE);
            RegisterShellHook(g_ts.hwndView, FALSE);

            Tray_HandleShellServiceObject(SSOCMDID_CLOSE, 0);

            TraceMsg(DM_SHUTDOWN, "Tray.wp WM_ENDSESSION call DestroyWindow v_hwndTray=%x", v_hwndTray);
            DestroyWindow(v_hwndTray);
            v_hwndTray = NULL;
        }
        TraceMsg(DM_SHUTDOWN, "Tray.wp WM_ENDSESSION return 0");
        break;

    case WM_PAINT:
        // draw etched line around on either side of the bandsite
        if (g_ts.fCoolTaskbar) {
            RECT rc;
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            GetWindowRect(g_ts.hwndRebar, &rc);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rc, 2);
            InflateRect(&rc, g_cxEdge, g_cyEdge);

            DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOPLEFT); // NT5 VFREEZE: remove right & bottom etch

            EndPaint(hwnd, &ps);
        } else
            goto DoDefault;
        break;

    case WM_POWER:
    case WM_POWERBROADCAST:
        _PropagateMessage(hwnd, uMsg, wParam, lParam);
        Tray_HandlePowerStatus(uMsg, wParam, lParam);
        goto DoDefault;

    case WM_DEVICECHANGE:
            if (wParam == DBT_CONFIGCHANGED)
            {
                Tray_UpdateDockingFlags();

                // Set this bit, so that the API
                if (IsEjectAllowed(TRUE))
                {
                    HandleDisplayChange(0, 0, TRUE);
                }

                // We got an update. Refresh.
                RefreshStartMenu();
            }
            else if (wParam == DBT_QUERYCHANGECONFIG)
            {
                if (IsEjectAllowed(FALSE) && !IsDisplayChangeSafe())
                {
                    if (ShellMessageBox(hinstCabinet, hwnd,
                        MAKEINTRESOURCE(IDS_DISPLAY_WARN),
                        MAKEINTRESOURCE(IDS_WINDOWS),
                        MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2) == IDNO)
                    {
                        return BROADCAST_QUERY_DENY;
                    }
                }

                //
                // drop down to 640x480 (or the lowest res for all configs)
                // before the config change.  this makes sure we dont screw
                // up some laptop display panels.
                //
                dw = GetMinDisplayRes();
                HandleDisplayChange(LOWORD(dw), HIWORD(dw), FALSE);
            }
            else if (wParam == DBT_MONITORCHANGE)
            {
                //
                // handle monitor change
                //
                HandleDisplayChange(LOWORD(lParam), HIWORD(lParam), TRUE);
            }
            else if (wParam == DBT_CONFIGCHANGECANCELED)
            {
                //
                // if the config change was canceled go back
                //
                HandleDisplayChange(0, 0, FALSE);
            }
        goto DoDefault;

    case WM_NOTIFY:
    {
        NMHDR *pnm = (NMHDR*)lParam;
        if (!BandSite_HandleMessage(g_ts.ptbs, hwnd, uMsg, wParam, lParam, &lres)) {
            switch (pnm->code)
            {
            case SEN_DDEEXECUTE:
                if (((LPNMHDR)lParam)->idFrom == 0)
                {
                    LPNMVIEWFOLDER pnmPost = DDECreatePostNotify((LPNMVIEWFOLDER)pnm);

                    if (pnmPost)
                    {
                        PostMessage(hwnd, GetDDEExecMsg(), 0, (LPARAM)pnmPost);
                        return TRUE;
                    }
                }
                break;

            case NM_STARTWAIT:
            case NM_ENDWAIT:
                Tray_OnWaitCursorNotify((NMHDR *)lParam);
                PostMessage(v_hwndDesktop, ((NMHDR*)lParam)->code == NM_STARTWAIT ? DTM_STARTWAIT : DTM_ENDWAIT,
                            0, 0); // forward it along
                break;

            case TTN_NEEDTEXT:
                {
#ifdef DESKBTN
                    #define lpttt ((LPTOOLTIPTEXT)lParam)
                    POINT pt;
                    GetCursorPos(&pt);
                    if (WindowFromPoint(pt) == g_hwndDesktopTB) {
                        if (g_hdlgDesktopConfig)
                            lpttt->szText[0] = TEXT('\0');
                        else
                            LoadString(hinstCabinet, IDS_SHOWDESKTOP, lpttt->szText, ARRAYSIZE(lpttt->szText));
                    } else 
#endif
                    {
                        //
                        //  Make the clock manage its own tooltip.
                        //
                        return SendMessage(Tray_GetClockWindow(), WM_NOTIFY, wParam, lParam);
                    }

                    return TRUE;
                }
            case TTN_SHOW:
                SetWindowZorder(g_ts.hwndTrayTips, HWND_TOP);
                break;

            }
        }
        break;
    }

    case WM_CLOSE:
        DoExitWindows(v_hwndDesktop);
        return 0;

    case WM_NCHITTEST:
        GetClientRect(hwnd, &r1);
        MapWindowPoints(hwnd, NULL, (LPPOINT)&r1, 2);

        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        TraySetUnhideTimer(pt.x, pt.y);

        // allow dragging if mouse is in client area of v_hwndTray
        if (PtInRect(&r1, pt)) {
            return HTCAPTION;
        }
        else {
            switch (g_ts.uStuckPlace) {
                case STICK_LEFT:
                    dw = ((pt.x > r1.right) ? HTRIGHT : HTBORDER);
                    break;
                case STICK_TOP:
                    dw = ((pt.y > r1.bottom) ? HTBOTTOM : HTBORDER);
                    break;
                case STICK_RIGHT:
                    dw = ((pt.x < r1.left) ? HTLEFT : HTBORDER);
                    break;
                case STICK_BOTTOM:
                default:
                    dw = ((pt.y < r1.top) ? HTTOP : HTBORDER);
                    break;
            }
            return dw;
        }
        break;

    case WM_WINDOWPOSCHANGING:
        Tray_HandleWindowPosChanging((LPWINDOWPOS)lParam);
        break;

    case WM_ENTERSIZEMOVE:
        DebugMsg(DM_TRAYDOCK, TEXT("Tray -- WM_ENTERSIZEMOVE"));
        g_fInSizeMove = TRUE;

        //
        // unclip the tray from the current monitor.
        // keeping the tray properly clipped in the MoveSize loop is extremely
        // hairy and provides almost no benefit.  we'll re-clip when it lands.
        //
        Tray_ClipWindow(FALSE);

        // Remember the old monitor we were on
        g_ts.hmonOld = g_ts.hmonStuck;

        // set up for WM_MOVING/WM_SIZING messages
        g_ts.uMoveStuckPlace = (UINT)-1;
        g_ts.fSysSizing = TRUE;
        break;

    case WM_EXITSIZEMOVE:
        DebugMsg(DM_TRAYDOCK, TEXT("Tray -- WM_EXITSIZEMOVE"));

        // done sizing
        g_ts.fSysSizing = FALSE;
        g_ts._fDeferedPosRectChange = FALSE;

        //
        // kick the size code one last time after the loop is done.
        // NOTE: we rely on the WM_SIZE code re-clipping the tray.
        //
        PostMessage(hwnd, WM_SIZE, 0, 0L);
        g_fInSizeMove = FALSE;
        break;

    case WM_MOVING:
        Tray_HandleMoving(wParam, (LPRECT)lParam);
        break;

    case WM_ENTERMENULOOP:
        // DebugMsg(DM_TRACE, "c.twp: Enter menu loop.");
        Tray_HandleEnterMenuLoop();
        break;

    case WM_EXITMENULOOP:
        // DebugMsg(DM_TRACE, "c.twp: Exit menu loop.");
        Tray_HandleExitMenuLoop();
        break;

    case WM_TIMER:
        if (IDT_SERVICE0 <= wParam && wParam <= IDT_SERVICELAST)
            return Tray_OnTimerService(uMsg, wParam, lParam);
        Tray_HandleTimer(wParam);
        break;

    case WM_SIZING:
        Tray_HandleSizing(wParam, (LPRECT)lParam, g_ts.uStuckPlace);
        break;

    case WM_SIZE:
        Tray_HandleSize();
        break;

    case WM_DISPLAYCHANGE:
        // NOTE: we get WM_DISPLAYCHANGE in the below two situations
        // 1. a display size changes (HMON will not change in USER)
        // 2. a display goes away or gets added (HMON will change even if
        // the monitor that went away has nothing to do with our hmonStuck)

        // In the above two situations we actually need to do different things
        // because in 1, we do not want to update our hmonStuck because we might
        // end up on another monitor, but in 2 we do want to update hmonStuck because
        // our hmon is invalid

        // The way we handle this is to call GetMonitorInfo on our old HMONITOR
        // and see if it's still valid, if not, we update it by calling Tray_SetStuckMonitor 
        // all these code is in Tray_ScreenSizeChange;

        Tray_ScreenSizeChange(hwnd);
        break;
    
    // Don't go to default wnd proc for this one...
    case WM_INPUTLANGCHANGEREQUEST:
        return(LRESULT)0L;

    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = g_cxFrame;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = g_cyFrame;
        break;

    case WM_WININICHANGE:
        if (lParam && (0 == lstrcmpi((LPCTSTR)lParam, TEXT("SaveTaskbar"))))
        {
            _SaveTrayAndDesktop();
        }
        else
        {
            BandSite_HandleMessage(g_ts.ptbs, hwnd, uMsg, wParam, lParam, NULL);
            _PropagateMessage(hwnd, uMsg, wParam, lParam);
            Tray_OnWinIniChange(hwnd, wParam, lParam);
        }

        if (lParam)
            TraceMsg(TF_TRAY, "Tray Got: lParam=%s", (LPCSTR)lParam);

        break;

    case WM_TIMECHANGE:
        _PropagateMessage(hwnd, uMsg, wParam, lParam);
        break;

    case WM_SYSCOLORCHANGE:
        Tray_OnNewSystemSizes();
        BandSite_HandleMessage(g_ts.ptbs, hwnd, uMsg, wParam, lParam, NULL);
        _PropagateMessage(hwnd, uMsg, wParam, lParam);
        break;

    case WM_SETCURSOR:
        if (g_ts.iWaitCount) {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        } else
            goto DoDefault;

    case WM_SETFOCUS:
        if (g_ts.hwndView)
            SetFocus(g_ts.hwndView);
        break;


    case WM_SYSCHAR:
        if (wParam == TEXT(' ')) {
            HMENU hmenu;
            int idCmd;

            SetWindowStyleBit(hwnd, WS_SYSMENU, WS_SYSMENU);
            hmenu = GetSystemMenu(hwnd, FALSE);
            if (hmenu) {
                EnableMenuItem(hmenu, SC_RESTORE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MAXIMIZE, MFS_GRAYED | MF_BYCOMMAND);
                EnableMenuItem(hmenu, SC_MINIMIZE, MFS_GRAYED | MF_BYCOMMAND);
                idCmd = Tray_TrackMenu(hmenu);
                if (idCmd)
                    SendMessage(v_hwndTray, WM_SYSCOMMAND, idCmd, 0L);
            }
            SetWindowStyleBit(hwnd, WS_SYSMENU, 0L);
        }
        break;

    case WM_SYSCOMMAND:

        // if we are sizing, make the full screen accessible
        switch (wParam & 0xFFF0) {
        case SC_CLOSE:
            DoExitWindows(v_hwndDesktop);
            break;

        default:
            goto DoDefault;
        }
        break;


    case TM_DESKTOPSTATE:
        Tray_OnDesktopState(lParam);
        break;

#ifdef DEBUG
    case TM_NEXTCTL:
#endif
    case TM_UIACTIVATEIO:
    case TM_ONFOCUSCHANGEIS:
        Tray_OnFocusMsg(uMsg, wParam, lParam);
        break;

    case TM_MARSHALBS:  // wParam=IID lRes=pstm
        return Tray_OnMarshalBS(wParam, lParam);

    case TM_SETTIMER:
    case TM_KILLTIMER:
        return Tray_OnTimerService(uMsg, wParam, lParam);
        break;

    case TM_REFRESH:        // cross-component FCIDM_REFRESH
        TraceMsg(DM_TRACE, "TM_REFRESH");
        Tray_Command(FCIDM_REFRESH);
        break;

    case TM_ACTASTASKSW:
        Tray_ActAsSwitcher();
        break;

    case TM_RELAYPOSCHANGED:
        AppBarNotifyAll((HMONITOR)lParam, ABN_POSCHANGED, (HWND)wParam, 0);
        break;

    case TM_BRINGTOTOP:
        SetWindowZorder((HWND)wParam, HWND_TOP);
        break;

    case TM_WARNNOAUTOHIDE:
        DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.twp collision UI request"));

        //
        // this may look a little funny but what we do is post this message all
        // over the place and ignore it when we think it is a bad time to put
        // up a message (like the middle of a fulldrag...)
        //
        // wParam tells us if we need to try to clear the state
        // the lowword of Tray_SetAutoHideState's return tells if anything changed
        //
        if ((!g_ts.fSysSizing || !g_fDragFullWindows) &&
            (!wParam || LOWORD(Tray_SetAutoHideState(FALSE))))
        {
            ShellMessageBox(hinstCabinet, hwnd,
                MAKEINTRESOURCE(IDS_ALREADYAUTOHIDEBAR),
                MAKEINTRESOURCE(IDS_TASKBAR), MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            DebugMsg(DM_TRAYDOCK, TEXT("TRAYDOCK.twp blowing off extraneous collision UI request"));
        }
        break;

    case TM_WARNNODROP:
        //
        // tell the user they can't drop objects on the taskbar
        //
        ShellMessageBox(hinstCabinet, v_hwndTray,
            MAKEINTRESOURCE(IDS_TASKDROP_ERROR), MAKEINTRESOURCE(IDS_TASKBAR),
            MB_ICONHAND | MB_OK);
        break;

    case TM_PRIVATECOMMAND:
        Tray_HandlePrivateCommand(lParam);
        break;

    case TM_HANDLEDELAYBOOTSTUFF:
        Tray_HandleDelayBootStuff();
        break;

    case TM_SHELLSERVICEOBJECTS:
        Tray_HandleShellServiceObject(wParam, lParam);
        break;

    case TM_GETHMONITOR:
        *((HMONITOR *)lParam) = g_ts.hmonStuck;
        break;

    case WM_NCRBUTTONUP:
        uMsg = WM_CONTEXTMENU;
        wParam = (WPARAM)g_ts.hwndView;
        goto L_WM_CONTEXTMENU;

    case WM_CONTEXTMENU:
    L_WM_CONTEXTMENU:

        if (SHRestricted(REST_NOTRAYCONTEXTMENU)) 
        {
            break;
        }

        if (((HWND)wParam) == g_ts.hwndStart)
        {
            // Don't display of the Start Menu is up.
            if (SendMessage(g_ts.hwndStart, BM_GETSTATE, 0, 0) & BST_PUSHED)
                break;
            StartMenuFolder_ContextMenu((DWORD)lParam);
        } 
        else if (g_hwndDesktopTB && g_hwndDesktopTB == (HWND)wParam)
        {
            Tray_DesktopMenu((DWORD)-1);
        } 
        else if (IsPosInHwnd(lParam, g_ts.hwndNotify)) 
        {
            // if click was inthe clock, include
            // the time
            Tray_ContextMenu((DWORD)lParam, TRUE);
        } 
        else 
        {
            BandSite_HandleMessage(g_ts.ptbs, hwnd, uMsg, wParam, lParam, &lres);
        }
        break;

    case TM_DOEXITWINDOWS:
        DoExitWindows(v_hwndDesktop);
        break;

    case WM_HOTKEY:
        if (wParam < GHID_FIRST)
        {
            HotkeyList_HandleHotkey((WORD)wParam);
        }
        else
        {
            Tray_HandleGlobalHotkey(wParam);
        }
        break;

    case WM_COMMAND:
        if (!BandSite_HandleMessage(g_ts.ptbs, hwnd, uMsg, wParam, lParam, &lres))
            Tray_Command(GET_WM_COMMAND_ID(wParam, lParam));
        break;

    case SBM_CANCELMENU:
        if (g_ts._pmpStartMenu)
            g_ts._pmpStartMenu->lpVtbl->OnSelect(g_ts._pmpStartMenu, MPOS_FULLCANCEL);
        break;

    case WM_WINDOWPOSCHANGED:
        IAppBarActivationChange(hwnd, g_ts.uStuckPlace);
        SendMessage(g_ts.hwndNotify, TNM_TRAYPOSCHANGED, 0, 0);
        goto DoDefault;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (g_ts.hwndStartBalloon)
        {
            RECT rc;
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            GetWindowRect(g_ts.hwndStartBalloon, &rc);
            MapWindowRect(HWND_DESKTOP, v_hwndTray, &rc);

            if (PtInRect(&rc, pt))
            {
                ShowWindow(g_ts.hwndStartBalloon, SW_HIDE);
                DontShowTheStartButtonBalloonAnyMore();
                DestroyStartButtonBalloon();
            }
        }
        break;

    case WM_ACTIVATE:
        IAppBarActivationChange(hwnd, g_ts.uStuckPlace);
        if (wParam != WA_INACTIVE)
        {
            Tray_Unhide();
        }
        else
        {
            // When tray is deactivated, remove our keyboard cues:
            //
            SendMessage(hwnd, WM_CHANGEUISTATE,
                MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);

            UnkUIActivateIO(g_ts.ptbs, FALSE, NULL);
        }
        //
        // Tray activation is a good time to do a reality check
        // (make sure "always-on-top" agrees with actual window
        // position, make sure there are no ghost buttons, etc).
        //
        Tray_RealityCheck();
        goto L_default;

    default:
    L_default:
        if (uMsg == GetDDEExecMsg())
        {
            ASSERT(lParam && 0 == ((LPNMHDR)lParam)->idFrom);
            DDEHandleViewFolderNotify(NULL, g_ts.hwndMain, (LPNMVIEWFOLDER)lParam);
            LocalFree((LPNMVIEWFOLDER)lParam);
            return TRUE;
        }
        else if (uMsg == g_uStartButtonBalloonTip)
        {
            ShowStartButtonToolTip();
        }
        else if (uMsg == g_uLogoffUser)
        {
            // Log off the current user (message from U&P control panel)
            ExitWindowsEx(EWX_LOGOFF, 0);
        }
DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return lres;
}

//----------------------------------------------------------------------------
// Just like shells SHRestricted() only this put up a message if the restricion
// is in effect.
BOOL _Restricted(HWND hwnd, RESTRICTIONS rest)
{
    if (SHRestricted(rest))
    {
        ShellMessageBox(hinstCabinet, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
            MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
        return TRUE;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
void DoExitWindows(HWND hwnd)
{
    static BOOL g_fShellShutdown = FALSE;

    if (!g_fShellShutdown)
    {

        if (_Restricted(hwnd, REST_NOCLOSE))
            return;

        {
            UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, FALSE, -1);
            // really #ifdef DEBUG, but want for testing
            // however can't do unconditionally due to perf
            if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuForceRefresh"), 
                NULL, NULL, NULL) || GetAsyncKeyState(VK_SHIFT) < 0)
            {
                RefreshStartMenu();
            }
        }

        _SaveTrayAndDesktop();

        g_ts.uModalMode = MM_SHUTDOWN;
        ExitWindowsDialog(hwnd);

        // NB User can have problems if the focus is forcebly changed to the desktop while
        // shutting down since it tries to serialize the whole process by making windows sys-modal.
        // If we hit this code at just the wrong moment (ie just after the sharing dialog appears)
        // the desktop will become sys-modal so you can't switch back to the sharing dialog
        // and you won't be able to shutdown.
        // SetForegroundWindow(hwnd);
        // SetFocus(hwnd);

        g_ts.uModalMode = 0;
        if ((GetKeyState(VK_SHIFT) < 0) && (GetKeyState(VK_CONTROL) < 0) && (GetKeyState(VK_MENU) < 0))
        {
            // User cancelled...
            // The shift key means exit the tray...
            // ??? - Used to destroy all cabinets...
            // PostQuitMessage(0);
            g_fFakeShutdown = TRUE; // Don't blow away session state; the session will survive
            TraceMsg(TF_TRAY, "c.dew: Posting quit message for tid=%#08x hwndDesk=%x(IsWnd=%d) hwndTray=%x(IsWnd=%d)", GetCurrentThreadId(),
            v_hwndDesktop,IsWindow(v_hwndDesktop), v_hwndTray,IsWindow(v_hwndTray));
            // 1 means close all the shell windows too
            PostMessage(v_hwndDesktop, WM_QUIT, 0, 1);
            PostMessage(v_hwndTray, WM_QUIT, 0, 0);

            g_fShellShutdown = TRUE;
        }
    }
}

#define MAX_FILE_PROP_PAGES 5  // More than enough

BOOL CALLBACK _TrayAddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_FILE_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}

void RealTrayProperties(HWND hwndParent, INT nStartPage)
{
    HPROPSHEETPAGE ahpage[MAX_FILE_PROP_PAGES];
    TCHAR szPath[MAX_PATH];
    PROPSHEETHEADER psh;

    LoadString(hinstCabinet, IDS_STARTMENUANDTASKBAR, szPath, ARRAYSIZE(szPath));

    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hInstance = hinstCabinet;
    psh.hwndParent = hwndParent;
    psh.pszCaption = szPath;
    psh.nPages = 0;     // incremented in callback
    psh.nStartPage = nStartPage;
    psh.phpage = ahpage;

    CShellTray_AddViewPropertySheetPages(0L, _TrayAddPropSheetPage, (LPARAM)(LPPROPSHEETHEADER)&psh);

    // Open the property sheet, only if we have some pages.
    if (psh.nPages > 0)
    {
        //g_ts.uModalMode = MM_OTHER;
        PropertySheet(&psh);
        //g_ts.uModalMode = 0;
    }
}

DWORD WINAPI TrayPropertiesThread(void *lpData)
{
    HWND hwnd;
    RECT rc;
    DWORD dwExStyle = WS_EX_TOOLWINDOW;
    INT nStartPage = PtrToUlong(lpData);
    CoInitialize(0);

    GetWindowRect(g_ts.hwndStart, &rc);
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;

    g_ts.hwndProp = hwnd = CreateWindowEx(dwExStyle, TEXT("static"), NULL, 0   ,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinstCabinet, NULL);

    if (g_ts.hwndProp) {
        // SwitchToThisWindow(hwnd, TRUE);
        // SetForegroundWindow(hwnd);
        RealTrayProperties(hwnd, nStartPage);
        g_ts.hwndProp = NULL;
        DestroyWindow(hwnd);
    }

    CoUninitialize();
    return TRUE;
}

void DoTrayProperties(INT nStartPage)
{
    if (!_Restricted(g_ts.hwndMain, REST_NOSETTASKBAR))
    {
        int i = RUNWAITSECS;
        while (g_ts.hwndProp == ((HWND)-1) &&i--) {
            // we're in the process of coming up. wait
            Sleep(SECOND);
        }

        // failed!  blow it off.
        if (g_ts.hwndProp == (HWND)-1)
            g_ts.hwndProp = NULL;

        if (g_ts.hwndProp)
        {
            // there's a window out there... activate it
            SwitchToThisWindow(GetLastActivePopup(g_ts.hwndProp), TRUE);
        } else {
            g_ts.hwndProp = (HWND)-1;
            if (!SHCreateThread(TrayPropertiesThread, (void *)nStartPage, 0, NULL)) {
                g_ts.hwndProp = 0;
            }
        }
    }
}

void ShowFolder(HWND hwnd, UINT csidl, UINT uFlags)
{
    SHELLEXECUTEINFO shei = { 0 };

    shei.cbSize     = sizeof(shei);
    shei.fMask      = SEE_MASK_IDLIST | SEE_MASK_INVOKEIDLIST;
    shei.nShow      = SW_SHOWNORMAL;

    if (_Restricted(hwnd, REST_NOSETFOLDERS))
        return;

    if (uFlags & COF_EXPLORE)
        shei.lpVerb = TEXT("explore");

    shei.lpIDList = SHCloneSpecialIDList(NULL, csidl, FALSE);
    if (shei.lpIDList)
    {
        ShellExecuteEx(&shei);
        ILFree(shei.lpIDList);
    }
}

BOOL Tray_IsAutoHide()
{
    return g_ts.uAutoHide & AH_ON;
}

DWORD Tray_GetStuckPlace()
{
    return g_ts.uStuckPlace;
}

BOOL CanMinimizeAll(HWND hwndView);

BOOL CanTileWindow(HWND hwnd, LPARAM lParam)
{
    BOOL *pf = (BOOL*)lParam;

    if (IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
        ((GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION) &&
        (hwnd != v_hwndTray) && hwnd != v_hwndDesktop) {
        *pf = TRUE;
    }
    return !*pf;
}

BOOL CanTileAnyWindows()
{
    BOOL f = FALSE;
    EnumWindows(CanTileWindow, (LPARAM)&f);
    return f;
}

HMENU Tray_BuildContextMenu(BOOL fIncludeTime)
{
    HKEY hKeyPolicy;
    HMENU hmContext = LoadMenuPopup(MAKEINTRESOURCE(MENU_TRAYCONTEXT));
    if (!hmContext)
        return NULL;

    if (fIncludeTime) {
        INSTRUMENT_STATECHANGE(SHCNFI_STATE_TRAY_CONTEXT_CLOCK);
        SetMenuDefaultItem(hmContext, IDM_SETTIME, MF_BYCOMMAND);
    } else {
        INSTRUMENT_STATECHANGE(SHCNFI_STATE_TRAY_CONTEXT);
        DeleteMenu(hmContext, IDM_SETTIME, MF_BYCOMMAND);
    }

    if (!g_ts.fUndoEnabled || !g_ts.pPositions) {
        DeleteMenu(hmContext, IDM_UNDO, MF_BYCOMMAND);
    } else {
        TCHAR szTemplate[30];
        TCHAR szCommand[30];
        TCHAR szMenu[64];
        LoadString(hinstCabinet, IDS_UNDOTEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
        LoadString(hinstCabinet, g_ts.pPositions->idRes, szCommand, ARRAYSIZE(szCommand));
        wsprintf(szMenu, szTemplate, szCommand);
        ModifyMenu(hmContext, IDM_UNDO, MF_BYCOMMAND | MF_STRING, IDM_UNDO, szMenu);
    }

    if (!CanMinimizeAll(g_ts.hwndView))
        EnableMenuItem(hmContext, IDM_MINIMIZEALL, MFS_GRAYED | MF_BYCOMMAND);

    if (!CanTileAnyWindows()) {
        EnableMenuItem(hmContext, IDM_CASCADE, MFS_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmContext, IDM_HORIZTILE, MFS_GRAYED | MF_BYCOMMAND);
        EnableMenuItem(hmContext, IDM_VERTTILE, MFS_GRAYED | MF_BYCOMMAND);

    }

    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                      0, KEY_READ, &hKeyPolicy) == ERROR_SUCCESS)
    {
        DWORD dwType, dwData = 0, dwSize;
        dwSize = sizeof(dwData);
        RegQueryValueEx (hKeyPolicy, TEXT("DisableTaskMgr"), NULL,
                         &dwType, (LPBYTE) &dwData, &dwSize);
        RegCloseKey (hKeyPolicy);
        if (dwData)
            EnableMenuItem(hmContext, IDM_SHOWTASKMAN, MFS_GRAYED | MF_BYCOMMAND);
    }


    BandSite_AddMenus(g_ts.ptbs, hmContext, 0, 0, IDM_TRAYCONTEXTFIRST);

    return hmContext;
}

void Tray_ContextMenuInvoke(int idCmd)
{
    if (idCmd) {
        if (idCmd < IDM_TRAYCONTEXTFIRST)
            BandSite_HandleMenuCommand(g_ts.ptbs, idCmd);
        else
            Tray_Command(idCmd);
    }
}

//
//  Tray_AsyncSaveSettings
//
//  We need to save our tray settings, but there may be a bunch
//  of these calls coming, (at startup time or when dragging 
//  items in the task bar) so gather them up into one save that
//  will happen in at least 2 seconds.
//
STDAPI_(void) Tray_AsyncSaveSettings()
{
    if (!g_fHandledDelayBootStuff)        // no point in saving if we're not done booting
        return;

    KillTimer(v_hwndTray, IDT_SAVESETTINGS);
    SetTimer(v_hwndTray, IDT_SAVESETTINGS, 2000, NULL); 
}


void Tray_ContextMenu(DWORD dwPos, BOOL fIncludeTime)
{
    HMENU hmContext;
    int idCmd;
    POINT pt = {LOWORD(dwPos), HIWORD(dwPos)};

    SwitchToThisWindow(g_ts.hwndMain, TRUE);
    SetForegroundWindow(g_ts.hwndMain);
    SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);

    if (dwPos != (DWORD)-1 &&
        IsChildOrHWND(g_ts.hwndRebar, WindowFromPoint(pt))) {

        // if the context menu came from below us, reflect down
        BandSite_HandleMessage(g_ts.ptbs, v_hwndTray, WM_CONTEXTMENU, 0, dwPos, NULL);

    } else {

        if (dwPos == (DWORD)-1) {
            pt.x = pt.y = 0;
            ClientToScreen(g_ts.hwndView, &pt);
            dwPos = MAKELONG(pt.x, pt.y);
        }

        hmContext = Tray_BuildContextMenu(fIncludeTime);
        if (!hmContext)
            return;

        idCmd = TrackPopupMenu(hmContext, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                               GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, v_hwndTray, NULL);
        DestroyMenu(hmContext);

        Tray_ContextMenuInvoke(idCmd);
    }

    SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
}

void Tray_DesktopMenu(DWORD dwPos)
{
    POINT pt;
    RECT rc;

    if (dwPos == (DWORD)-1)
    {
        if (g_hwndDesktopTB && g_ts.fShowDeskBtn)
        {
            GetClientRect(g_hwndDesktopTB, &rc);
            pt.x = rc.right;
            pt.y = rc.bottom;
            ClientToScreen(g_hwndDesktopTB, &pt);
        }
        else
        {
            GetCursorPos(&pt);
        }
        dwPos = MAKELONG(pt.x, pt.y);
    }

    PostMessage(v_hwndDesktop, DTM_DESKTOPCONTEXTMENU, 0, dwPos);
}

//----------------------------------------------------------------------------
void _RunFileDlg(HWND hwnd, UINT idIcon, LPCITEMIDLIST pidlWorkingDir,
        UINT idTitle, UINT idPrompt, DWORD dwFlags)
{
    HICON hIcon;
    LPCTSTR lpszTitle;
    LPCTSTR lpszPrompt;
    TCHAR szTitle[256];
    TCHAR szPrompt[256];
    TCHAR szWorkingDir[MAX_PATH];

    dwFlags |= RFD_USEFULLPATHDIR;
    szWorkingDir[0] = 0;

    hIcon = idIcon ? LoadIcon(hinstCabinet, MAKEINTRESOURCE(idIcon)) : NULL;

    if (!pidlWorkingDir || !SHGetPathFromIDList(pidlWorkingDir, szWorkingDir))
    {
        // This is either the Tray, or some non-file system folder, so
        // we will "suggest" the Desktop as a working dir, but if the
        // user types a full path, we will use that instead.  This is
        // what WIN31 Progman did (except they started in the Windows
        // dir instead of the Desktop).
        goto UseDesktop;
    }

    // if it's a removable dir, make sure it's still there
    if (szWorkingDir[0])
    {
        int idDrive = PathGetDriveNumber(szWorkingDir);
        if ((idDrive != -1))
        {
            UINT dtype = DriveType(idDrive);
            if ( ( (dtype == DRIVE_REMOVABLE) || (dtype == DRIVE_CDROM) )
                 && !PathFileExists(szWorkingDir) )
            {
                goto UseDesktop;
            }
        }
    }

    //
    // Check if this is a directory. Notice that it could be a in-place
    // navigated document.
    //
    if (PathIsDirectory(szWorkingDir)) {
        goto UseWorkingDir;
    }

UseDesktop:
    SHGetSpecialFolderPath(hwnd, szWorkingDir, CSIDL_DESKTOPDIRECTORY, FALSE);

UseWorkingDir:

    if (idTitle)
    {
        LoadString(hinstCabinet, idTitle, szTitle, ARRAYSIZE(szTitle));
        lpszTitle = szTitle;
    }
    else
        lpszTitle = NULL;
    if (idPrompt)
    {
        LoadString(hinstCabinet, idPrompt, szPrompt, ARRAYSIZE(szPrompt));
        lpszPrompt = szPrompt;
    }
    else
        lpszPrompt = NULL;

    RunFileDlg(hwnd, hIcon, szWorkingDir, lpszTitle, lpszPrompt, dwFlags);
}

#ifdef DEBUG
BOOL ShouldMinMax(HWND hwnd, BOOL fMin);
#endif

BOOL SavePosEnumProc(HWND hwnd, LPARAM lParam)
{
    // dont need to entercritical here since we are only ever
    // called from SaveWindowPositions, which as already entered the critical secion
    // for g_ts.pPositions

    ASSERT(g_ts.pPositions);

    if (IsWindowVisible(hwnd) &&
        (hwnd != v_hwndTray) &&
        (hwnd != v_hwndDesktop))
    {
        HWNDANDPLACEMENT hap;

        hap.wp.length = SIZEOF(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &hap.wp);
#ifdef DEBUG
        TraceMsg(TF_TRAY, "SaveWindowPostitionsCallback(hwnd=0x%x lP=%x) sc=%d par=0x%x own=0x%x", hwnd, (int)lParam, hap.wp.showCmd, GetParent(hwnd), GetWindow(hwnd, GW_OWNER));
#endif
        if (hap.wp.showCmd != SW_SHOWMINIMIZED) {
            hap.hwnd = hwnd;
            hap.fRestore = TRUE;
#ifdef DEBUG
            TraceMsg(TF_TRAY, "SaveWindowPostitionsCallback: sm()=%d add hwnd=0x%x sc=%d fRestore=%d", ShouldMinMax(hwnd, TRUE), hwnd, hap.wp.showCmd, hap.fRestore);
#endif
            DSA_AppendItem(g_ts.pPositions->hdsaWP, &hap);
        }
    }
    return TRUE;
}

void SaveWindowPositions(UINT idRes)
{
    ENTERCRITICAL;
    if (g_ts.pPositions)
    {
        if (g_ts.pPositions->hdsaWP)
            DSA_DeleteAllItems(g_ts.pPositions->hdsaWP);
    }
    else
    {
        g_ts.pPositions = (LPWINDOWPOSITIONS)LocalAlloc(LPTR, SIZEOF(WINDOWPOSITIONS));
        if (!g_ts.pPositions)
        {
            LEAVECRITICAL;
            return;
        }

        g_ts.pPositions->hdsaWP = DSA_Create(SIZEOF(HWNDANDPLACEMENT), 4);
    }
    g_ts.pPositions->idRes = idRes;

    // CheckWindowPositions tested for these...
    ASSERT(idRes == IDS_MINIMIZEALL || idRes == IDS_CASCADE || idRes == IDS_TILE);
    EnumWindows(SavePosEnumProc, 0);
    LEAVECRITICAL;
}

DWORD RestoreWindowPositionsThread( LPWINDOWPOSITIONS pPositions )
{
    int i;
    LPHWNDANDPLACEMENT phap;
    LONG iAnimate;
    ANIMATIONINFO ami;

    ami.cbSize = SIZEOF(ANIMATIONINFO);
    SystemParametersInfo(SPI_GETANIMATION, SIZEOF(ami), &ami, FALSE);
    iAnimate = ami.iMinAnimate;
    ami.iMinAnimate = FALSE;
    SystemParametersInfo(SPI_SETANIMATION, SIZEOF(ami), &ami, FALSE);

    i = DSA_GetItemCount(pPositions->hdsaWP) - 1;
    for ( ; i >= 0; i--) {
        phap = DSA_GetItemPtr(pPositions->hdsaWP, i);
        if (IsWindow(phap->hwnd)) {
#if (defined(DBCS) || defined(FE_IME))
        // Word6/J relies on WM_SYSCOMMAND/SC_RESTORE to invalidate their
        // MDI childrens' client area. If we just do SetWindowPlacement
        // the app leaves its MDI children unpainted. I wanted to patch
        // the app but couldn't afford to because the patch will remove
        // its optimizing code (will affect performance).
        // bug#11258-win95d.
        //
#if !defined(WINNT)
            if (ImmGetAppIMECompatFlags(GetWindowThreadProcessId(phap->hwnd, NULL)) & IMECOMPAT_SENDSC_RESTORE)
            {
                SendMessage(phap->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                continue;
            }
#endif
#endif

#ifndef WPF_ASYNCWINDOWPLACEMENT
#define WPF_ASYNCWINDOWPLACEMENT 0x0004
#endif
#ifdef WINNT
            // pass this async.
            if (!IsHungAppWindow(phap->hwnd))
#endif
            {
                phap->wp.length = sizeof(WINDOWPLACEMENT);
                phap->wp.flags |= WPF_ASYNCWINDOWPLACEMENT;
                TraceMsg(TF_TRAY, "rwpt: hwnd=0x%x fRestore=%d ?SWP", phap->hwnd, phap->fRestore);
                if (phap->fRestore) {
                    // only restore those guys we've actually munged.
                    SetWindowPlacement(phap->hwnd, &phap->wp);
                }
            }
        }
    }

    ami.iMinAnimate = iAnimate;
    SystemParametersInfo(SPI_SETANIMATION, SIZEOF(ami), &ami, FALSE);

    DestroySavedWindowPositions(pPositions);

    return 1;
}


void RestoreWindowPositions()
{
    ENTERCRITICAL;
    if (g_ts.pPositions)
    {
        if (SHCreateThread(RestoreWindowPositionsThread, g_ts.pPositions, 0, NULL))
            g_ts.pPositions = NULL;
    }
    LEAVECRITICAL;
}

void DestroySavedWindowPositions(LPWINDOWPOSITIONS pPositions)
{
    ENTERCRITICAL;
    if (pPositions)
    {
        // free the global struct
        DSA_Destroy(pPositions->hdsaWP);
        LocalFree(pPositions);
    }
    LEAVECRITICAL;
}

void TrayHandleWindowDestroyed(HWND hwnd)
{
    // enter critical section so we dont corrupt the hdsaWP
    ENTERCRITICAL;
    if (g_ts.pPositions)
    {
        int i = DSA_GetItemCount(g_ts.pPositions->hdsaWP) - 1;
        for ( ; i >= 0; i--) {
            LPHWNDANDPLACEMENT phap = DSA_GetItemPtr(g_ts.pPositions->hdsaWP, i);
            if (phap->hwnd == hwnd || !IsWindow(phap->hwnd)) {
                DSA_DeleteItem(g_ts.pPositions->hdsaWP, i);
            }
        }

        if (!DSA_GetItemCount(g_ts.pPositions->hdsaWP))
        {
            DestroySavedWindowPositions(g_ts.pPositions);
            g_ts.pPositions = NULL;
        }
    }
    LEAVECRITICAL;
}


//----------------------------------------------------------------------------
// Allow us to bump the activation of the run dlg hidden window.
// Certain lame apps (Norton Desktop setup) use the active window at RunDlg time
// as the parent for their dialogs. If that window disappears then they fault.
// We don't want the tray to get the activation coz it will cause it to appeare
// if you're in auto-hide mode.
LRESULT CALLBACK RunDlgStaticSubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == RDM_ACTIVATE) {

        // there's a window out there... activate it
        SwitchToThisWindow(GetLastActivePopup(hwnd), TRUE);
        return 0;
    } else if ((uMsg == WM_ACTIVATE) && (wParam == WA_ACTIVE))
        {
                // Bump the activation to the desktop.
                if (v_hwndDesktop)
                {
                        SetForegroundWindow(v_hwndDesktop);
                        return 0;
                }
        } else if (uMsg == WM_NOTIFY) {
            // relay it to the tray
            return SendMessage(v_hwndTray, uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD CALLBACK RunDlgThread(void *data)
{
    HWND            hwnd;
    RECT            rc, rcTemp;
    MONITORINFO     monitorInfo;

    CoInitialize(0);

    // 99/04/12 #316424 vtan: Get the rectangle for the "Start" button.
    // If this is off the screen the tray is probably in auto hide mode.
    // In this case offset the rectangle into the monitor where it should
    // belong. This may be up, down, left or right depending on the
    // position of the tray.

    // First thing to do is to establish the dimensions of the monitor on
    // which the tray resides. If no monitor can be found then use the
    // primary monitor.

    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(g_ts.hmonStuck, &monitorInfo) == 0)
    {
        TBOOL(SystemParametersInfo(SPI_GETWORKAREA, 0, &monitorInfo.rcMonitor, 0));
    }

    // Get the co-ordinates of the "Start" button.

    GetWindowRect(g_ts.hwndStart, &rc);

    // Look for an intersection in the monitor.

    if (IntersectRect(&rcTemp, &rc, &monitorInfo.rcMonitor) == 0)
    {
        LONG    lDeltaX, lDeltaY;

        // Does not exist in the monitor. Move the co-ordinates by the
        // width or height of the tray so that it does.

        // This bizarre arithmetic is used because Tray_ComputeHiddenRect()
        // takes into account the frame and that right/bottom of RECT
        // is exclusive in GDI.

        lDeltaX = g_ts.sStuckWidths.cx - g_cxFrame;
        lDeltaY = g_ts.sStuckWidths.cy - g_cyFrame;
        if (rc.left < monitorInfo.rcMonitor.left)
        {
            --lDeltaX;
            lDeltaY = 0;
        }
        else if (rc.top < monitorInfo.rcMonitor.top)
        {
            lDeltaX = 0;
            --lDeltaY;
        }
        else if (rc.right > monitorInfo.rcMonitor.right)
        {
            lDeltaX = -lDeltaX;
            lDeltaY = 0;
        }
        else if (rc.bottom > monitorInfo.rcMonitor.bottom)
        {
            lDeltaX = 0;
            lDeltaY = -lDeltaY;
        }
        TBOOL(OffsetRect(&rc, lDeltaX, lDeltaY));
    }

    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, TEXT("static"), NULL, 0   ,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinstCabinet, NULL);
    if (hwnd) {
        // SwitchToThisWindow(hwnd, TRUE);
        // SetForegroundWindow(hwnd);
        // UINT idPidl;
        HANDLE hMemWorkDir = NULL;
        LPITEMIDLIST pidlWorkingDir = NULL;
#ifdef WINNT
        TCHAR szDir[MAX_PATH];
        TCHAR szPath[MAX_PATH];
        BOOL fSimple;
#endif

        // Subclass it.
        SubclassWindow(hwnd, RunDlgStaticSubclassWndProc);

        if (data)
            SetProp(hwnd, TEXT("WaitingThreadID"), (HANDLE)data);

#ifdef WINNT
        // On NT, we like to start apps in the HOMEPATH directory.  This
        // should be the current directory for the current process.
        fSimple = FALSE;
        GetEnvironmentVariable(TEXT("HOMEDRIVE"), szDir, ARRAYSIZE(szDir));
        GetEnvironmentVariable(TEXT("HOMEPATH"), szPath, ARRAYSIZE(szPath));

        if (PathAppend(szDir, szPath) && PathIsDirectory(szDir))
        {
            pidlWorkingDir = SHSimpleIDListFromPath(szDir);
            fSimple = TRUE;
        }
#endif
        if (!pidlWorkingDir)
        {
            // If the last active window was a folder/explorer window with the
            // desktop as root, use its as the current dir
            if (g_ts.hwndLastActive)
            {
                ENTERCRITICAL;
                if (g_ts.hwndLastActive && !IsMinimized(g_ts.hwndLastActive) &&
                    (Cabinet_IsExplorerWindow(g_ts.hwndLastActive) || Cabinet_IsFolderWindow(g_ts.hwndLastActive)))
                {
// BUGBUG: BobDay - If the window is hung, shell hangs!
// Should ask OTRegister for appropriate COMPAREROOT data
                    hMemWorkDir = (HANDLE)SendMessage(g_ts.hwndLastActive, CWM_CLONEPIDL, GetCurrentProcessId(), 0L);
                    pidlWorkingDir = SHLockShared(hMemWorkDir, GetCurrentProcessId());
                }
                LEAVECRITICAL;
            }
        }

        _RunFileDlg(hwnd, 0, pidlWorkingDir, 0, 0, 0);
        if (pidlWorkingDir)
        {
#ifdef WINNT
            if (fSimple)
                ILFree(pidlWorkingDir);
            else
#endif
            {
                SHUnlockShared(pidlWorkingDir);
                SHFreeShared(hMemWorkDir, GetCurrentProcessId());
            }
        }

        if (data)
            RemoveProp(hwnd, TEXT("WaitingThreadID"));
        DestroyWindow(hwnd);
    }

    CoUninitialize();
    return TRUE;
}

void Tray_RunDlg()
{
    HANDLE hEvent;
    void *pvThreadParam;

    if (!_Restricted(v_hwndTray, REST_NORUN))
    {
        TCHAR szRunDlgTitle[MAX_PATH];
        HWND  hwndOldRun;
        LoadString(hinstCabinet, IDS_RUNDLGTITLE, szRunDlgTitle, ARRAYSIZE(szRunDlgTitle));

        // See if there is already a run dialog up, and if so, try to activate it

        hwndOldRun = FindWindow(WC_DIALOG, szRunDlgTitle);
        if (hwndOldRun)
        {
            DWORD dwPID;

            GetWindowThreadProcessId(hwndOldRun, &dwPID);
            if (dwPID == GetCurrentProcessId())
            {
                if (IsWindowVisible(hwndOldRun))
                {
                    SetForegroundWindow(hwndOldRun);
                    return;
                }
            }
        }

        // Create an event so we can wait for the run dlg to appear before
        // continue - this allows it to capture any type-ahead.
        hEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("MSShellRunDlgReady"));
        if (hEvent)
            pvThreadParam = (void *)GetCurrentThreadId();
        else
            pvThreadParam = NULL;

        if (SHQueueUserWorkItem(RunDlgThread, pvThreadParam, 0, (DWORD_PTR)0, (DWORD_PTR)NULL, NULL, TPS_LONGEXECTIME | TPS_DEMANDTHREAD))
        {
            if (hEvent)
            {
                MsgWaitForMultipleObjectsLoop(hEvent, 10*1000);
                DebugMsg(DM_TRACE, TEXT("c.t_rd: Done waiting."));
            }
        }

        if (hEvent)
            CloseHandle(hEvent);
    }
}

DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout)
{
    while (1)
    {
        MSG msg;
        // NB We need to let the run dialog become active so we have to handle sent
        // messages but we don't want to handle any input events or we'll swallow the
        // type-ahead.

        // NOTE: If hEvent never signals and we get at least one message every dwTimeout
        // milliseconds then we will never timeout and could hang in this function.  If
        // this ever becomes a problem then see the shell32 function SHProcessMessagesUntilEvent
        // of one of many similar examples for the solution.

        DWORD dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_SENDMESSAGE);
        // Are we done waiting?
        switch (dwObject) {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            return dwObject;

        case WAIT_TIMEOUT:
            return WAIT_TIMEOUT;

        case WAIT_OBJECT_0 + 1:
            // This PeekMessage has the side effect of processing any broadcast messages.
            // It doesn't matter what message we actually peek for but if we don't peek
            // then other threads that have sent broadcast sendmessages will hang until
            // hEvent is signaled.  Since the process we're waiting on could be the one
            // that sent the broadcast message that could cause a deadlock otherwise.
            PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
            break;
        }
    }
    // never gets here
    // return dwObject;
}

void ExploreCommonStartMenu(BOOL bExplore)
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   TCHAR szPath[MAX_PATH];
   TCHAR szCmdLine[MAX_PATH + 50];

   //
   // Get the common start menu path.
   //
// On NT we want to force the directory to exist, but not on W95 machines
#ifdef WINNT
   if (!SHGetSpecialFolderPath(NULL, szPath,  CSIDL_COMMON_STARTMENU, FALSE)) {
#else
   if (!SHGetSpecialFolderPath(NULL, szPath, CSIDL_COMMON_STARTMENU, TRUE)) {
#endif
       return;
   }


   //
   // If we are starting in explorer view, then the command line
   // has a "/e, " before the quoted diretory.
   //

   if (bExplore) {
       lstrcpy (szCmdLine, TEXT("explorer.exe /e, \""));
   } else {
       lstrcpy (szCmdLine, TEXT("explorer.exe \""));
   }

   lstrcat (szCmdLine, szPath);
   lstrcat (szCmdLine, TEXT("\""));



   //
   // Initialize process startup info
   //

   si.cb = sizeof(STARTUPINFO);
   si.lpReserved = NULL;
   si.lpTitle = NULL;
   si.lpDesktop = NULL;
   si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
   si.dwFlags = STARTF_USESHOWWINDOW;
   si.wShowWindow = SW_SHOWNORMAL;
   si.lpReserved2 = NULL;
   si.cbReserved2 = 0;


   //
   // Start explorer
   //
   if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE,
                     NORMAL_PRIORITY_CLASS, NULL, NULL,
                     &si, &pi)) {

       //
       // Close the process and thread handles
       //

       CloseHandle(pi.hProcess);
       CloseHandle(pi.hThread);
   }

}

#ifdef FULL_DEBUG

typedef struct tagDCTPFINO
{
    UINT idCmd;
    HWND hwndOwner;
} DCTPINFO;


DWORD WINAPI DebugCommandThreadProc(void *pv)
{
    DCTPINFO * pinfo = (DCTPINFO *)pv;

    switch (pinfo->idCmd)
    {
    case IDM_DEBUGURLDB:
        // Inproc debugging of URL database
        InvokeURLDebugDlg(pinfo->hwndOwner);
        break;
    }

    LocalFree(pinfo);
    return 0;
}


void Tray_OnDebugCommand(UINT idCmd)
{
    DCTPINFO * pinfo;

    switch (idCmd)
    {
    case IDM_DEBUGURLDB:
        pinfo = LocalAlloc(LPTR, sizeof(*pinfo));
        if (pinfo)
        {
            pinfo->hwndOwner = NULL;
            pinfo->idCmd = idCmd;

            SHCreateThread(DebugCommandThreadProc, pinfo, CTF_INSIST, NULL);
        }
        break;
    }
}


void Tray_AddDebugMenu(HMENU hmenu)
{
    HMENU hmenuSrc = LoadMenu(hinstCabinet, MAKEINTRESOURCE(MENU_DEBUG));

    if (hmenuSrc)
    {
        Shell_MergeMenus(hmenu, hmenuSrc, (UINT)-1, 0, (UINT)-1, MM_ADDSEPARATOR);
        DestroyMenu(hmenuSrc);
    }
}

#endif  // FULL_DEBUG


void StartMenuFolder_ContextMenu(DWORD dwPos)
{
    LPITEMIDLIST pidlStart = SHCloneSpecialIDList(v_hwndTray, CSIDL_STARTMENU, TRUE);
    INSTRUMENT_STATECHANGE(SHCNFI_STATE_TRAY_CONTEXT_START);
    Tray_HandleFullScreenApp(NULL);

    SetForegroundWindow(g_ts.hwndMain);

    if (pidlStart)
    {
        LPITEMIDLIST pidlLast = ILClone(ILFindLastID(pidlStart));
        ILRemoveLastID(pidlStart);

        if (pidlLast)
        {
            IShellFolder *psf = BindToFolder(pidlStart);
            if (psf)
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    IContextMenu *pcm;
                    HRESULT hres = psf->lpVtbl->GetUIObjectOf(psf, v_hwndTray,
                        1, &pidlLast, &IID_IContextMenu, NULL, &pcm);
                    if (SUCCEEDED(hres))
                    {
                        hres = pcm->lpVtbl->QueryContextMenu(pcm, hmenu, 0,
                                            IDSYSPOPUP_FIRST, IDSYSPOPUP_LAST, CMF_VERBSONLY);
                        if (SUCCEEDED(hres))
                        {
                            int idCmd;

                            if (!SHRestricted(REST_NOCOMMONGROUPS))
                            {
                                // If the user has access to the Common Start Menu, then we can add those items. If not,
                                // then we should not.
                                TCHAR szCommon[MAX_PATH];
                                BOOL fAddCommon = (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, szCommon));

                                if (fAddCommon)
                                    fAddCommon = IsUserAnAdmin();


                                // Since we don't show this on the start button when the user is not an admin, don't show it here... I guess...
                                if (fAddCommon)
                                {
                                   AppendMenu (hmenu, MF_SEPARATOR, 0, NULL);
                                   LoadString (hinstCabinet, IDS_OPENCOMMON, szCommon, ARRAYSIZE(szCommon));
                                   AppendMenu (hmenu, MF_STRING, IDSYSPOPUP_OPENCOMMON, szCommon);
                                   LoadString (hinstCabinet, IDS_EXPLORECOMMON, szCommon, ARRAYSIZE(szCommon));
                                   AppendMenu (hmenu, MF_STRING, IDSYSPOPUP_EXPLORECOMMON, szCommon);
                                }
                            }

#ifdef FULL_DEBUG
                            // Add the Debug cascaded menu to the context
                            // menu
                            Tray_AddDebugMenu(hmenu);
#endif

                            if (dwPos == (DWORD)-1)
                            {
                                idCmd = Tray_TrackMenu(hmenu);
                            }
                            else
                            {
                                SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);
                                idCmd = TrackPopupMenu(hmenu,
                                                       TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                                       GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, v_hwndTray, NULL);
                                SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
                            }


                            if (idCmd)
                            {
                                if (idCmd == IDSYSPOPUP_OPENCOMMON)
                                {
                                    ExploreCommonStartMenu(FALSE);
                                }
                                else if (idCmd == IDSYSPOPUP_EXPLORECOMMON)
                                {
                                    ExploreCommonStartMenu(TRUE);
                                }
#ifdef FULL_DEBUG
                                else if (InRange(idCmd, IDSYSPOPUP_DEBUGFIRST, IDSYSPOPUP_DEBUGLAST))
                                {
                                    Tray_OnDebugCommand(idCmd);
                                }
#endif
                                else
                                {
                                    TCHAR szPath[MAX_PATH];
                                    CMINVOKECOMMANDINFOEX ici;
#ifdef UNICODE
                                    CHAR szPathAnsi[MAX_PATH];
#endif
                                    ZeroMemory(&ici, SIZEOF(CMINVOKECOMMANDINFOEX));
                                    ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
                                    // ici.fMask = 0;
                                    ici.hwnd = v_hwndTray;
                                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - IDSYSPOPUP_FIRST);
                                    // ici.lpParameters = NULL;
                                    // ici.lpDirectory = NULL;
                                    ici.nShow = SW_NORMAL;
#ifdef UNICODE
                                    SHGetPathFromIDListA(pidlStart, szPathAnsi);
                                    SHGetPathFromIDList(pidlStart, szPath);
                                    ici.lpDirectory = szPathAnsi;
                                    ici.lpDirectoryW = szPath;
                                    ici.fMask |= CMIC_MASK_UNICODE;
#else
                                    SHGetPathFromIDList(pidlStart, szPath);
                                    ici.lpDirectory = szPath;
#endif
                                    pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&ici);
                                }
                            }
                        }
                        pcm->lpVtbl->Release(pcm);
                    }
                    DestroyMenu(hmenu);
                }
                psf->lpVtbl->Release(psf);
            }
            ILFree(pidlLast);
        }
        ILFree(pidlStart);
    }
}
//----------------------------------------------------------------------------
#ifdef WINNT
// RunSystemMonitor
//
// Launches system monitor (taskmgr.exe), which is expected to be able
// to find any currently running instances of itself

void RunSystemMonitor(void)
{
    STARTUPINFO startup;
    PROCESS_INFORMATION pi;
    TCHAR szName[] = TEXT("taskmgr.exe");

    startup.cb = SIZEOF(startup);
    startup.lpReserved = NULL;
    startup.lpDesktop = NULL;
    startup.lpTitle = NULL;
    startup.dwFlags = 0L;
    startup.cbReserved2 = 0;
    startup.lpReserved2 = NULL;
    startup.wShowWindow = SW_SHOWNORMAL;

    // Used to pass "taskmgr.exe" here, but NT faulted in CreateProcess
    // Probably they are wacking on the command line, which is bogus, but
    // then again the paremeter is not marked const...
    if (CreateProcess(NULL, szName, NULL, NULL, FALSE, 0,
                      NULL, NULL, &startup, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
#endif

void GiveDesktopFocus()
{
    SetForegroundWindow(v_hwndDesktop);
    SendMessage(v_hwndDesktop, DTM_UIACTIVATEIO, (WPARAM) TRUE, /*dtb*/0);
}

void FlushMessages()
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


/*----------------------------------------------------------
Purpose: loads the given resource string and executes it.
         The resource string should follow this format:

         "program.exe>parameters"

         If there are no parameters, the format should simply be:

         "program.exe"

*/
void _ExecResourceCmd(UINT ids)
{
    TCHAR szCmd[2*MAX_PATH];

    if (LoadString(hinstCabinet, ids, szCmd, SIZECHARS(szCmd)))
    {
        SHELLEXECUTEINFO sei = {0};

        // Find list of parameters (if any)
        LPTSTR pszParam = StrChr(szCmd, TEXT('>'));

        if (pszParam)
        {
            // Replace the '>' with a null terminator
            *pszParam = 0;
            pszParam++;
        }

        sei.cbSize = sizeof(sei);
        sei.nShow = SW_SHOWNORMAL;
        sei.lpFile = szCmd;
        sei.lpParameters = pszParam;
        ShellExecuteEx(&sei);
    }
}

void RefreshStartMenu()
{
    // BUGBUG (lamadio): We should use IUnknown_Exec, but we need to extern "c" it...
    IOleCommandTarget* poct;
    if (g_ts._pmbStartMenu &&
        SUCCEEDED(g_ts._pmbStartMenu->lpVtbl->QueryInterface(g_ts._pmbStartMenu,
                  &IID_IOleCommandTarget, (void**)&poct) ))
    {
        poct->lpVtbl->Exec(poct, &CLSID_MenuBand, MBANDCID_REFRESH, 0, NULL, NULL);
        poct->lpVtbl->Release(poct);
    }
}

void Tray_Command(UINT idCmd)
{
    INSTRUMENT_ONCOMMAND(SHCNFI_TRAYCOMMAND, g_ts.hwndMain, idCmd);

    UEMFireEvent(&UEMIID_SHELL, UEME_RUNWMCMD, UEMF_XEVENT, UIM_EXPLORER, idCmd);

    switch (idCmd) {

    case IDM_CONTROLS:
    case IDM_PRINTERS:
        ShowFolder(g_ts.hwndMain,
            idCmd == IDM_CONTROLS ? CSIDL_CONTROLS : CSIDL_PRINTERS, COF_USEOPENSETTINGS);
        break;

    case IDM_EJECTPC:
        DoEjectPC();    //From apithk.c
        break;

    case IDM_LOGOFF:
        // Let the desktop get a chance to repaint to get rid of the
        // start menu bits before bringing up the logoff dialog box.
        Sleep(100);

        _SaveTrayAndDesktop();
        LogoffWindowsDialog(v_hwndDesktop);
        break;

    case IDM_EXITWIN:
        DoExitWindows(v_hwndDesktop);
        break;

    case IDM_RAISEDESKTOP:
        ToggleDesktop();
        break;

    case IDM_FILERUN:
        Tray_RunDlg();
        break;

    case IDM_MINIMIZEALLHOTKEY:
        Tray_HandleGlobalHotkey(GHID_MINIMIZEALL);
        break;

#ifdef DEBUG
    case IDM_SIZEUP:
    {
        RECT rcView;
        GetWindowRect(g_ts.hwndRebar, &rcView);
        MapWindowPoints(HWND_DESKTOP, g_ts.hwndMain, (LPPOINT)&rcView, 2);
        rcView.bottom -= 18;
        SetWindowPos(g_ts.hwndRebar, NULL, 0, 0, RECTWIDTH(rcView), RECTHEIGHT(rcView), SWP_NOMOVE | SWP_NOZORDER);
    }
        break;

    case IDM_SIZEDOWN:
    {
        RECT rcView;
        GetWindowRect(g_ts.hwndRebar, &rcView);
        MapWindowPoints(HWND_DESKTOP, g_ts.hwndMain, (LPPOINT)&rcView, 2);
        rcView.bottom += 18;
        SetWindowPos(g_ts.hwndRebar, NULL, 0, 0, RECTWIDTH(rcView), RECTHEIGHT(rcView), SWP_NOMOVE | SWP_NOZORDER);
    }
        break;
#endif

    case IDM_MINIMIZEALL:
        // minimize all window
        MinimizeAll(g_ts.hwndView);
        g_ts.fUndoEnabled = TRUE;
        break;

    case IDM_UNDO:
        RestoreWindowPositions();
        break;

    case IDM_SETTIME:
        // run the default applet in timedate.cpl
        SHRunControlPanel( TEXT("timedate.cpl"), g_ts.hwndMain );
        break;

#ifdef WINNT
    case IDM_SHOWTASKMAN:
        RunSystemMonitor();
        break;
#endif

    case IDM_CASCADE:
    case IDM_VERTTILE:
    case IDM_HORIZTILE:
        if (CanTileAnyWindows())
        {
            SaveWindowPositions((idCmd == IDM_CASCADE)?
                IDS_CASCADE : IDS_TILE);

            AppBarNotifyAll(NULL, ABN_WINDOWARRANGE, NULL, TRUE);

            if (idCmd == IDM_CASCADE)
            {
                CascadeWindows(GetDesktopWindow(), 0, NULL, 0, NULL);
            }
            else
            {
                TileWindows(GetDesktopWindow(), ((idCmd == IDM_VERTTILE)?
                    MDITILE_VERTICAL : MDITILE_HORIZONTAL), NULL, 0, NULL);
            }

            // do it *before* ABN_xxx so don't get 'indirect' moves
            // BUGBUG or should it be after?
            // CheckWindowPositions();
            g_ts.fUndoEnabled = FALSE;
            SetTimer(v_hwndTray, IDT_ENABLEUNDO, 500, NULL);

            AppBarNotifyAll(NULL, ABN_WINDOWARRANGE, NULL, FALSE);
        }
        break;

    case IDM_TRAYPROPERTIES:
        DoTrayProperties(0);
        break;

    case IDM_SETTINGSASSIST:
        SHCreateThread(SettingsUI_ThreadProc, NULL, 0, NULL);
        break;

    case IDM_HELPSEARCH:
        if (g_bRunOnMemphis || g_bRunOnNT5)
            _ExecResourceCmd(IDS_HELP_CMD);
        else
            WinHelp(g_ts.hwndMain, TEXT("windows.hlp"), HELP_FINDER, 0);
        break;

    // NB The Alt-s comes in here.
    case IDC_KBSTART:
        SetForegroundWindow(g_ts.hwndMain);
        // This pushes the start button and causes the start menu to popup.
            SendMessage(g_ts.hwndStart, BM_SETSTATE, TRUE, 0 );
        // This forces the button back up.
            SendMessage(g_ts.hwndStart, BM_SETSTATE, FALSE, 0);
        break;

    case IDC_ASYNCSTART:
        KillConfigDesktopDlg();
#if 0 // (for testing UAssist locking code)
        UEMFireEvent(&UEMIID_SHELL, UEME_DBSLEEP, UEMF_XEVENT, -1, (LPARAM)10000);
#endif
#ifdef DEBUG
        if (GetAsyncKeyState(VK_SHIFT) < 0)
        {
            UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, TRUE, -1);
            RefreshStartMenu();
        }
#endif

        // Make sure the button is down.
        // DebugMsg(DM_TRACE, "c.twp: IDC_START.");

        // Make sure the Start button is down.
        if (!g_ts.bMainMenuInit && SendMessage(g_ts.hwndStart, BM_GETSTATE, 0, 0) & BST_PUSHED)
        {
            // DebugMsg(DM_TRACE, "c.twp: Start button down.");
            // Set the focus.
            Tray_SetFocus(g_ts.hwndStart);
            ToolbarMenu();
        }
        break;

    // NB LButtonDown on the Start button come in here.
    // Space-bar stuff also comes in here.
    case IDC_START:
        // User gets a bit confused with space-bar tuff (the popup ends up
        // getting the key-up and beeps).
        PostMessage(g_ts.hwndMain, WM_COMMAND, IDC_ASYNCSTART, 0);
        break;

    case FCIDM_FINDFILES:
        SHFindFiles(NULL, NULL);
        break;

    case FCIDM_FINDCOMPUTER:
        SHFindComputer(NULL, NULL);
        break;

    case FCIDM_REFRESH:
        RefreshStartMenu();
        break;

    case FCIDM_NEXTCTL:
        {
            MSG msg = { 0, WM_KEYDOWN, VK_TAB };
            HWND hwndFocus = GetFocus();
            int dtb;

            // Since we are Tab or Shift Tab we should turn the focus rect on.
            //
            // Note: we don't need to do this in the GiveDesktopFocus cases below,
            // but in those cases we're probably already in the UIS_CLEAR UISF_HIDEFOCUS
            // state so this message is cheap to send.
            //
            SendMessage(g_ts.hwndMain, WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR,
                UISF_HIDEFOCUS), 0);

            dtb = GetAsyncKeyState(VK_SHIFT) < 0 ? -1 : 1;

            if (hwndFocus && (IsChildOrHWND(g_ts.hwndStart, hwndFocus))) {
                if (dtb == -1) {
                    TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL start => deact");
                    // gotta deactivate manually
                    TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL deact");
                    GiveDesktopFocus();
                }
                else {
                    TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL start -> bs");
                    UnkUIActivateIO(g_ts.ptbs, TRUE, &msg);
                }
            } else if (hwndFocus && (IsChildOrHWND(g_ts.hwndNotify, hwndFocus))) {
                if (dtb == -1) {
                    TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL notifies=>bs");
                    UnkUIActivateIO(g_ts.ptbs, TRUE, &msg);
                } else {
                    if (g_hwndDesktopTB && g_ts.fShowDeskBtn)
                    {
                        TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL notifies->desk btn");
                        Tray_SetFocus(g_hwndDesktopTB);
                    }
                    else
                    {
                        TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL notifies->deact");
                        GiveDesktopFocus();
                    }
                }
            } else if (hwndFocus && g_hwndDesktopTB && (IsChildOrHWND(g_hwndDesktopTB, hwndFocus))) {
                if (dtb == -1) {
                    TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL desk btn=>notifies");
                    Tray_SetFocus(g_ts.hwndNotify);
                } else {
                    TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL desk btn->deact");
                    GiveDesktopFocus();
                }
            } else {
                if (UnkTranslateAcceleratorIO(g_ts.ptbs, &msg) != S_OK) {
                    if (dtb == -1) {
                        TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL bs => start");
                        TraceMsg(DM_FOCUS, "tiois: TM_UIActIO fake UIAct (shift NYI)");

                        Tray_SetFocus(g_ts.hwndStart);
                    }
                    else {
                        // if you tab forward out of the bands, the next focus guy is the tray notify set
                        TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL bs -> notify");
                        Tray_SetFocus(g_ts.hwndNotify);
                    }
                }
                else
                {
                    if (dtb == -1)
                        TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL bs => bs");
                    else
                        TraceMsg(DM_TRACE, "t FCIDM_NEXTCTL bs -> bs");
                }
            }

            break;
        }

#ifdef WINNT // hydra specific menu items
    case IDM_MU_SECURITY:
        MuSecurity();
        break;
#endif

    default:
        if (IsInRange(idCmd, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST) && ! _Restricted(v_hwndTray, REST_NOFIND))
        {
            CMINVOKECOMMANDINFOEX ici;
            ZeroMemory(&ici, SIZEOF(CMINVOKECOMMANDINFOEX));
            ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
            ici.hwnd = g_ts.hwndMain;
            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - TRAY_IDM_FINDFIRST);
            ici.nShow = SW_NORMAL;

            g_ts.pcmFind->lpVtbl->InvokeCommand(g_ts.pcmFind,
                                                (LPCMINVOKECOMMANDINFO)&ici);

        }
        else
        {
            DebugMsg(TF_WARNING, TEXT("tray Unknown command (%x)"), idCmd);
        }
        break;
    }
}

//// Start menu/Tray tab as a drop target
extern const IDropTarget c_dtgtStart;  // forward
extern const IDropTarget c_dtgtTray;   // forward

// globals used to all of the 3 drop targets to do switching
int g_iDropItem = -2;
DWORD g_dwTriggerTime = 0;

HRESULT GetStartMenuDropTarget(IDropTarget** pptgt)
{
    HRESULT hres = E_FAIL;
    LPITEMIDLIST pidlStart = SHCloneSpecialIDList(NULL, CSIDL_STARTMENU, TRUE);

    *pptgt = NULL;

    if (pidlStart)
    {
        IShellFolder *psf = BindToFolder(pidlStart);
        if (psf)
        {
            hres = psf->lpVtbl->CreateViewObject(psf, v_hwndTray, &IID_IDropTarget, pptgt);
            psf->lpVtbl->Release(psf);
        }

        ILFree(pidlStart);
    }
    return hres;
}

STDMETHODIMP CStartDropTarget_QueryInterface(LPDROPTARGET pdtgt, REFIID riid, void ** ppvObj)
{
    if (IsEqualIID(riid, &IID_IDropTarget) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = pdtgt;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    // AddRef() not needed, static object.
    return S_OK;
}

STDMETHODIMP_(ULONG) CStartDropTarget_AddRef(LPDROPTARGET pdtgt)
{
    return 2;
}

STDMETHODIMP_(ULONG) CStartDropTarget_Release(LPDROPTARGET pdtgt)
{
    return 1;
}

STDMETHODIMP CStartDropTarget_DragEnter(LPDROPTARGET pdtgt, LPDATAOBJECT pdtobj, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    HWND hwndLock = v_hwndTray;  // no clippy
    POINT pt;
    RECT rc;
    IDropTarget* ptgt;

    if (SUCCEEDED(GetStartMenuDropTarget(&ptgt)))
    {
        TraceMsg(TF_TRAY, "CStartDropTarget::DragEnter called");
        TraySetUnhideTimer(ptl.x, ptl.y);
        pt.x = ptl.x;
        pt.y = ptl.y;

        // Check to make sure that we're going to accept the drop before we expand the start menu.
        ptgt->lpVtbl->DragEnter(ptgt, pdtobj, grfKeyState, ptl,
                                 pdwEffect);

        // DROPEFFECT_NONE means it ain't gonna work, so don't popup the Start Menu.
        // BUGBUG (lamadio): I assume that all shell folders benieth the Start Menu are
        // the same type of shell folder as the Fast items portion. This might not be true.
        if (*pdwEffect != DROPEFFECT_NONE)
        {
            GetWindowRect(g_ts.hwndStart, &rc);
            if (PtInRect(&rc,pt))
            {
                //Make sure it really is in the start menu..
                SetTimer(v_hwndTray, IDT_STARTMENU, 1000, NULL);
            }
        }

        ptgt->lpVtbl->DragLeave(ptgt);
        ptgt->lpVtbl->Release(ptgt);
    }

    _DragEnter(hwndLock, ptl, pdtobj);

    g_iDropItem = -2;    // reset to no target

    return S_OK;
}

STDMETHODIMP CStartDropTarget_DragOver(LPDROPTARGET pdtgt, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    TraySetUnhideTimer(ptl.x, ptl.y);
    _DragMove(g_ts.hwndStart, ptl);

    *pdwEffect = DROPEFFECT_LINK;

    return S_OK;
}

extern int Task_HitTest(HWND hwndTask, POINTL ptl);
extern void Task_SetCurSel(HWND hwndTask, int i);

STDMETHODIMP CTabDropTarget_DragEnter(LPDROPTARGET pdtgt, LPDATAOBJECT pdtobj, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    DWORD dwEffect = *pdwEffect;
    BandSite_DragEnter(g_ts.ptbs, pdtobj, grfKeyState, ptl, &dwEffect);
    return CStartDropTarget_DragEnter( pdtgt, pdtobj, grfKeyState, ptl, pdwEffect);
}

STDMETHODIMP CTabDropTarget_DragLeave(LPDROPTARGET pdtgt)
{
    BandSite_DragLeave(g_ts.ptbs);
    DAD_DragLeave();
    return S_OK;
}

STDMETHODIMP CTabDropTarget_DragOver(LPDROPTARGET pdtgt, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    int iHitNew;

    iHitNew = Task_HitTest(g_ts.hwndView, ptl);
    if (iHitNew == -1) {
        DWORD dwEffect = *pdwEffect;
        BandSite_DragOver(g_ts.ptbs, grfKeyState, ptl, &dwEffect);
    }

    CStartDropTarget_DragOver(pdtgt, grfKeyState, ptl, pdwEffect);

    if (g_iDropItem != iHitNew)
    {
        TraceMsg(TF_TRAY, "CTDT::HitTest new target (%d->%d)", g_iDropItem, iHitNew);

        g_iDropItem = iHitNew;
        g_dwTriggerTime = GetCurrentTime() + 1000;
        if (iHitNew == -1) {
            g_dwTriggerTime += 1000;    // make a little longer for minimize all
        }
    }
    else if (GetCurrentTime() > g_dwTriggerTime)
    {
        TraceMsg(TF_TRAY, "CTDT::doing event (%d)", g_iDropItem);

        DAD_ShowDragImage(FALSE);       // unlock the drag sink if we are dragging.

        if (g_iDropItem == -1)
            RaiseDesktop();
        else
        {
            Task_SetCurSel(g_ts.hwndView, g_iDropItem);
            UpdateWindow(v_hwndTray);
        }

        DAD_ShowDragImage(TRUE);        // restore the lock state.

        g_dwTriggerTime += 10000;   // don't let this happen again for 10 seconds
                                    // simulate a single shot event
    }

    if (g_iDropItem != -1)
        *pdwEffect = DROPEFFECT_MOVE;   // try to get the move cursor
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

STDMETHODIMP CStartDropTarget_DragLeave(LPDROPTARGET pdtgt)
{
    TraceMsg(TF_TRAY, "CStartDropTarget::DragLeave called");
    DAD_DragLeave();
    KillTimer(v_hwndTray, IDT_STARTMENU);
    return S_OK;
}

STDMETHODIMP CStartDropTarget_Drop(LPDROPTARGET pdtgt, LPDATAOBJECT pdtobj,
                             DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HRESULT hres = E_OUTOFMEMORY;
    IDropTarget* pdrop;
    KillTimer(v_hwndTray, IDT_STARTMENU);

    hres = GetStartMenuDropTarget(&pdrop);
    if (SUCCEEDED(hres))
    {
        POINTL pt = { 0, 0 };
        DWORD grfKeyState = 0;

        *pdwEffect &= DROPEFFECT_LINK;

        pdrop->lpVtbl->DragEnter(pdrop, pdtobj, grfKeyState, pt,
                                 pdwEffect);

        hres = pdrop->lpVtbl->Drop(pdrop, pdtobj, grfKeyState, pt,
                                   pdwEffect);

        pdrop->lpVtbl->DragLeave(pdrop);
        pdrop->lpVtbl->Release(pdrop);
    }

    DAD_DragLeave();

    return hres;
}

STDMETHODIMP CTabDropTarget_Drop(LPDROPTARGET pdtgt, LPDATAOBJECT pdtobj,
                             DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    HWND hwndPt;
    POINT pt = {ptl.x, ptl.y};

    BandSite_DragLeave(g_ts.ptbs);
    DAD_DragLeave();

    hwndPt = WindowFromPoint(pt);
    if (!IsChildOrHWND(g_ts.hwndRebar, hwndPt)) {
        // if it was dropped on us somewhere other than in the tasks window
        // then send it to bandsite
        return BandSite_SimulateDrop(g_ts.ptbs, pdtobj, grfKeyState, ptl, pdwEffect);
    }

    //
    // post ourselves a message to have the tray put up a message box to
    // explain that you can't drag to the taskbar.  we need to return from
    // the Drop method now so the DragSource isn't hung while our box is up
    //
    PostMessage(v_hwndTray, TM_WARNNODROP, 0, 0L);

    // be sure to clear DROPEFFECT_MOVE so apps don't delete their data
    *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

IDropTarget* Tray_GetDropTarget()
{
    return (IDropTarget*)&c_dtgtTray;
}

// store our drop target in pdtgtTree
//=============================================================================
// CStartDropTarget : Register and Revoke
//=============================================================================
void CStartDropTarget_Register()
{
    THR(RegisterDragDrop(g_ts.hwndStart, (IDropTarget *)&c_dtgtStart));
    THR(RegisterDragDrop(g_ts.hwndMain, (IDropTarget*)&c_dtgtTray));
}

void CStartDropTarget_Revoke()
{
    RevokeDragDrop(g_ts.hwndMain);
    RevokeDragDrop(g_ts.hwndStart);
}

//=============================================================================
// CStartDropTarget : VTable
//=============================================================================
const IDropTargetVtbl c_CStartDropTargetVtbl = {
    CStartDropTarget_QueryInterface, CStartDropTarget_AddRef, CStartDropTarget_Release,
    CStartDropTarget_DragEnter,
    CStartDropTarget_DragOver,
    CStartDropTarget_DragLeave,
    CStartDropTarget_Drop
};

const IDropTargetVtbl c_CTabDropTargetVtbl = {
    CStartDropTarget_QueryInterface, CStartDropTarget_AddRef, CStartDropTarget_Release,
    CTabDropTarget_DragEnter,
    CTabDropTarget_DragOver,
    CTabDropTarget_DragLeave,
    CTabDropTarget_Drop
};

const IDropTarget c_dtgtStart = { (IDropTargetVtbl *)&c_CStartDropTargetVtbl };
const IDropTarget c_dtgtTray = { (IDropTargetVtbl *)&c_CTabDropTargetVtbl };

//// end Start Drop target


////////////////////////////////////////////////////////////////////////////
// Begin Print Notify stuff
////////////////////////////////////////////////////////////////////////////

typedef BOOL (* PFNENUMJOBS) (HANDLE, DWORD, DWORD, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef BOOL (* PFNOPENPRINTER) (LPTSTR, LPHANDLE, void *);
typedef BOOL (* PFNCLOSEPRINTER) (HANDLE);

PFNENUMJOBS g_pfnEnumJobs = NULL;
PFNOPENPRINTER g_pfnOpenPrinter = NULL;
PFNCLOSEPRINTER g_pfnClosePrinter = NULL;

void PrintNotify_Init(HWND hwnd)
{
    g_ts.pidlPrintersFolder = SHCloneSpecialIDList(hwnd, CSIDL_PRINTERS, FALSE);
    if (g_ts.pidlPrintersFolder)
    {
        SHChangeNotifyEntry fsne;
        HWND hwndSpool;

        fsne.pidl = g_ts.pidlPrintersFolder;
        fsne.fRecursive = TRUE;

        TraceMsg(TF_TRAY, "PRINTNOTIFY - registering print notify icon");
        g_ts.uPrintNotify = SHChangeNotifyRegister(hwnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel,
                                SHCNE_CREATE | SHCNE_UPDATEITEM | SHCNE_DELETE,
                                WMTRAY_PRINTCHANGE, 1, &fsne);

        // Tell the spool subsystem to stop waiting for the printnotify icon..
        // After this notification, the subsystem can start printing and our
        // little printer icon will work. We need this so that deferred
        // print jobs don't start printing before the icon is listening.
        hwndSpool = FindWindow(TEXT("SpoolProcessClass"), NULL);
        if (hwndSpool)
        {
            NMHDR nmhdr = {hwnd, 0, NM_ENDWAIT};
            TraceMsg(TF_TRAY, "PRINTNOTIFY - Telling print subsystem we're here");
            SendMessage(hwndSpool, WM_NOTIFY, (WPARAM)NM_ENDWAIT, (LPARAM)&nmhdr);
        }
    }
}


typedef BOOL (*ENUMPROP)(void *lpData, HANDLE hPrinter, DWORD dwLevel,
        LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum);

void *Printer_EnumProps(HANDLE hPrinter, DWORD dwLevel, DWORD *lpdwNum,
    ENUMPROP lpfnEnum, void *lpData)
{
    DWORD dwSize, dwNeeded;
    LPBYTE pEnum;

    dwSize = 0;
    SetLastError(0);
    lpfnEnum(lpData, hPrinter, dwLevel, NULL, 0, &dwSize, lpdwNum);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        pEnum = NULL;
        goto Error1;
    }

    ASSERT(dwSize < 0x100000L);

TryAgain:
    pEnum = LocalAlloc(LPTR, dwSize);
    if (!pEnum)
    {
        goto Error1;
    }

    SetLastError(0);
    if (!lpfnEnum(lpData, hPrinter, dwLevel, pEnum, dwSize, &dwNeeded, lpdwNum))
    {
        LocalFree(pEnum);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            dwSize = dwNeeded;
            goto TryAgain;
        }

        pEnum = NULL;
    }

Error1:
    return(pEnum);
}


BOOL _EnumJobsCB(void *lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return(g_pfnEnumJobs(hPrinter, 0, 0xffff, dwLevel, pEnum, dwSize, lpdwNeeded, lpdwNum));
}


DWORD _EnumJobs(HANDLE hPrinter, DWORD dwLevel, void **ppJobs)
{
    DWORD dwNum = 0L;
    *ppJobs = Printer_EnumProps(hPrinter, dwLevel, &dwNum, _EnumJobsCB, NULL);
    if (*ppJobs==NULL)
    {
        dwNum=0;
    }
    return(dwNum);
}


// Thread_PrinterPoll is the worker thread for PrintNotify_HandleFSNotify.
// It keeps track of print jobs, polls the pinter, updates the icon, etc.

typedef struct _PRINTERPOLLDATA
{
    HWND hwnd;
    HANDLE heWakeUp;
} PRINTERPOLLDATA, *PPRINTERPOLLDATA;

DWORD WINAPI Thread_PrinterPoll(PPRINTERPOLLDATA pData)
{
    PRINTERPOLLDATA data;

    HANDLE hmodWinspool;

    TCHAR szUserName[40];
    TCHAR szFormat[60];
    DWORD dwSize;
    DWORD_PTR aArgs[2];

    NOTIFYICONDATA IconData = {0};
    DWORD dwNotifyMessage = NIM_ADD;
    UINT nIconShown = 0;
    HICON hIconNormal = NULL;
    HICON hIconError = NULL;

    HANDLE haPrinterNames = NULL;
    LPSHELLFOLDER psf = NULL;
    DWORD dwTimeOut;

    DWORD dwStatus;
    int nFound;

    MSG msg;

    LPITEMIDLIST pidlFree = NULL;

    static int s_cxSmIcon = 0;
    static int s_cySmIcon = 0;
    int cxSmIcon = 0;
    int cySmIcon = 0;

    // notify PrintNotify_StartThread that our message queue is up
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    data = *pData;
    GlobalFree(pData);
    SetEvent(data.heWakeUp);

    // load winspool functions we use
    hmodWinspool = LoadLibrary(TEXT("winspool.drv"));
    if (!hmodWinspool)
    {
        goto Error1;
    }

    // These are globals just so I don't have to pass them around
#ifdef UNICODE
    g_pfnEnumJobs     = (PFNENUMJOBS)    GetProcAddress(hmodWinspool, "EnumJobsW");
    g_pfnOpenPrinter  = (PFNOPENPRINTER) GetProcAddress(hmodWinspool, "OpenPrinterW");
#else
    g_pfnEnumJobs     = (PFNENUMJOBS)    GetProcAddress(hmodWinspool, "EnumJobsA");
    g_pfnOpenPrinter  = (PFNOPENPRINTER) GetProcAddress(hmodWinspool, "OpenPrinterA");
#endif
    g_pfnClosePrinter = (PFNCLOSEPRINTER)GetProcAddress(hmodWinspool, "ClosePrinter");
    if (!g_pfnEnumJobs
        || !g_pfnOpenPrinter || !g_pfnClosePrinter)
    {
        ASSERT(FALSE);
        goto Error2;
    }

    // Get fixed data
    dwSize = ARRAYSIZE(szUserName);
    if (!GetUserName(szUserName, &dwSize))
    {
        // non repro bug 6853/5592 is probably caused by GetUserName failing.
        // since this bug gets reported occasionally, see if this fixes it.
        // Note: we may want to change the tip message to not include this
        // empty user name.
        TraceMsg(TF_TRAY, "GetUserName failed!");
        ASSERT(0); // why would this fail?
        szUserName[0] = TEXT('\0');
    }
    LoadString(hinstCabinet, IDS_NUMPRINTJOBS, szFormat, ARRAYSIZE(szFormat));
    aArgs[0] = (DWORD_PTR)-1;
    aArgs[1] = (DWORD_PTR)szUserName;

    // Set up initial data

    IconData.cbSize = SIZEOF(IconData);
    IconData.hWnd = data.hwnd;
    //IconData.uID = 0;
    IconData.uFlags = NIF_MESSAGE;
    IconData.uCallbackMessage = WMTRAY_PRINTICONNOTIFY;
    // IconData.hIcon filled in when message sent
    // IconData.szTip filled in when message sent

    dwNotifyMessage = NIM_ADD;  // next operation for the icon

    nIconShown = 0;             // icon id of printer icon currently displayed
    hIconNormal = NULL;         // handle of ICO_PRINTER
    hIconError = NULL;          // handle of ICO_PRINTER_ERROR

    haPrinterNames = NULL;      // list of currently polled printers
    psf = NULL;                 // IShellFolder of Printers Folder
    dwTimeOut = POLLINTERVAL;   // time before next poll

    nFound = 0;                 // # jobs found after most recent poll
    dwStatus = 0;               // Status after most recent poll

    // process messages

    for ( ; TRUE ; )
    {
        int iPrinter, iJob;
        JOB_INFO_1 *pJobs;
        HANDLE hPrinter;
        DWORD dwTime;  // elapsed time for this wait
        DWORD dw;      // return value from this wait

        dwTime = GetTickCount();
        dw = MsgWaitForMultipleObjects(1, &(data.heWakeUp),
                FALSE, dwTimeOut, QS_ALLINPUT);
        dwTime = GetTickCount() - dwTime;

        switch (dw)
        {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            // the tray is shutting down
            TraceMsg(TF_TRAY, "PRINTNOTIFY - Thread_PrinterPoll shutting down");
            goto exit;

        case WAIT_TIMEOUT:
#ifndef WINNT
            if (g_ts.uAutoHide & AH_HIDING)
            {
                // don't poll if the tray is hidden
                dwTimeOut = MINPOLLINTERVAL;
                break;
            }
#endif

            TraceMsg(TF_TRAY, "PRINTNOTIFY - Thread_PrinterPoll timeout polling");

            // we need to poll -- jump into the message loop
            pidlFree = NULL;
            goto poll;

        default:
            // we have messages
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                switch (msg.message - WM_USER)
                {
                // NOTE: These can't (& don't) conflict w/ our SHCNE_ messages
                case WM_LBUTTONDBLCLK:
                case WM_RBUTTONUP:
                {
                    int idCmd, idDefCmd = IDM_PRINTNOTIFY_FOLDER;
                    HMENU hmContext = LoadMenuPopup(MAKEINTRESOURCE(MENU_PRINTNOTIFYCONTEXT));
                    if (hmContext)
                    {
                        // Append each printer we're polling
                        if (haPrinterNames)
                        {
                            MENUITEMINFO mii;
                            int i;
                            TCHAR szTemplate[60];

                            // If error, we'll need szTemplate
                            if (dwStatus & JOB_STATUS_ERROR_BITS)
                            {
                                LoadString(hinstCabinet, IDS_PRINTER_ERROR, szTemplate, ARRAYSIZE(szTemplate));
                            }

                            mii.cbSize = SIZEOF(MENUITEMINFO);
                            mii.fMask = MIIM_TYPE | MIIM_ID;
                            mii.fType = MF_STRING;

                            InsertMenu(hmContext, (UINT)-1, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

                            for (i = DSA_GetItemCount(haPrinterNames) - 1 ;
                                 i >= 0 ; --i)
                            {
                                TCHAR szBuf[MAXPRINTERBUFFER+60];
                                LPTSTR pName;
                                LPPRINTERNAME pPrinter = DSA_GetItemPtr(haPrinterNames, i);

                                // give it an id
                                mii.wID = i+IDM_PRINTNOTIFY_FOLDER+1;

                                // indicate error state via name
                                pName = pPrinter->szPrinterName;
                                if (pPrinter->fInErrorState)
                                {
                                    wsprintf(szBuf, szTemplate, pName);
                                    pName = szBuf;

                                    // default to first error state printer
                                    if (idDefCmd == IDM_PRINTNOTIFY_FOLDER)
                                        idDefCmd = mii.wID;
                                }

                                // Name of the printer
                                mii.dwTypeData = pName;
                                mii.cch = lstrlen(pName);

                                InsertMenuItem(hmContext, (UINT)-1, MF_BYPOSITION, &mii);
                            }
                        } // if (haPrinterNames)

                        // show the context menu
                        if (msg.message == WM_USER + WM_RBUTTONUP) 
                        {
                            // We need an hwnd owned by this thread.
                            HWND hwndOwner = CreateWindow(TEXT("static"), NULL,
                                                WS_DISABLED, 0, 0, 0, 0,
                                                NULL, NULL, hinstCabinet, 0L);
                            DWORD dwPos = GetMessagePos();
                            SetMenuDefaultItem(hmContext, idDefCmd, MF_BYCOMMAND);
                            SetForegroundWindow(hwndOwner);
                            SetFocus(hwndOwner);
                            idCmd = TrackPopupMenu(hmContext,
                                TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, hwndOwner, NULL);

                            DestroyWindow(hwndOwner);
                        }
                        else
                        {
                            idCmd = idDefCmd;
                        }

                        if (idCmd != 0)
                        {
                            int iStart, iStop;

                            if (idCmd == IDM_PRINTNOTIFY_FOLDER)
                            {
                                iStart = 0;
                                iStop = DSA_GetItemCount(haPrinterNames);
                            }
                            else
                            {
                                iStart = idCmd - IDM_PRINTNOTIFY_FOLDER - 1;
                                iStop = iStart + 1;
                            }

                            for ( ; iStart < iStop ; iStart++)
                            {
                                LPPRINTERNAME pPrinter = DSA_GetItemPtr(haPrinterNames, iStart);
                                IContextMenu *pcm;


                                // open selected queue view
                                TraceMsg(TF_TRAY, "PRINTNOTIFY - Open '%s'", pPrinter->szPrinterName);
                                psf->lpVtbl->GetUIObjectOf(psf, NULL, 1, &pPrinter->pidlPrinter,
                                    &IID_IContextMenu, NULL, &pcm);
                                if (pcm)
                                {
                                    //
                                    //  We need to call QueryContextMenu() so that CDefFolderMenu
                                    // can initialize its dispatch table correctly.
                                    //
                                    HMENU hmenuTmp = CreatePopupMenu();
                                    if (hmenuTmp)
                                    {
                                        HRESULT hres;

                                        hres = pcm->lpVtbl->QueryContextMenu(pcm, hmenuTmp, 0,
                                                    0, 0x7FFF, CMF_DEFAULTONLY);
                                        if (SUCCEEDED(hres))
                                        {
                                            // BUGBUG:: compiler bug VC4 will not zero out remaining
                                            // fields...
                                            CMINVOKECOMMANDINFOEX ici;
                                            ZeroMemory(&ici, SIZEOF(CMINVOKECOMMANDINFOEX));
                                            ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
                                            // ici.fMask = 0;
                                            ici.hwnd = NULL;
                                            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(GetMenuDefaultItem(hmenuTmp, MF_BYCOMMAND, 0));
                                            // ici.lpParameters = NULL;
                                            // ici.lpDirectory = NULL;
                                            ici.nShow = SW_NORMAL;

                                            pcm->lpVtbl->InvokeCommand(pcm,
                                                     (LPCMINVOKECOMMANDINFO)&ici);
                                        }

                                        DestroyMenu(hmenuTmp);
                                    }

                                    pcm->lpVtbl->Release(pcm);
                                }
                            }
                        }

                        DestroyMenu(hmContext);

                    } // if (hmenu)
                    break;
                }

                case SHCNE_CREATE:
                case SHCNE_DELETE:
                    TraceMsg(TF_TRAY, "PRINTNOTIFY - Thread_PrinterPoll SHCNE_CREATE/DELETE");

                    // Bug 405510
                    // BUGBUG: Somebody is passing us messages with a bogus PIDL as the lParam.
                    // We don't know who it is, but we double-check if the PIDL is okay here.
                    // If we can find the culprit, then this check won't be necessary.
                    if (!IsValidPIDL((LPCITEMIDLIST)msg.lParam))
                    {
                        break;
                    }
                    PrintNotify_AddToQueue(&haPrinterNames, &psf, (LPCITEMIDLIST)msg.lParam);

                    //
                    // We need to free the pidl passed in.
                    //
                    pidlFree = (LPITEMIDLIST)msg.lParam;
poll:
                    dwTimeOut = POLLINTERVAL;

                    // just in case we failed AddToQueue
                    if (!haPrinterNames)
                    {
                        ASSERT(FALSE);
                        break;
                    }

                    nFound = 0;
                    dwStatus = 0;

                    for (iPrinter=DSA_GetItemCount(haPrinterNames)-1 ;
                         iPrinter>=0 ; --iPrinter)
                    {
                        LPPRINTERNAME pPrinter;
                        BOOL fFound;
                        BOOL fNetDown;

                        pPrinter = DSA_GetItemPtr(haPrinterNames,iPrinter);

                        // if the tray is shutting down while we're in the
                        // middle of polling, we might want to exit this loop

                        if (!g_pfnOpenPrinter(pPrinter->szPrinterName, &hPrinter, NULL))
                        {
                            // Can't open the printer?  Remove it so we don't keep trying
                            TraceMsg(TF_TRAY, "PRINTNOTIFY - can't open [%s]", pPrinter->szPrinterName);
                            goto remove_printer;
                        }

                        TraceMsg(TF_TRAY, "PRINTNOTIFY - polling [%s]", pPrinter->szPrinterName);

                        fFound = FALSE;
                        iJob = _EnumJobs(hPrinter, 1, &pJobs) - 1;
                        fNetDown = GetLastError() == ERROR_BAD_NET_RESP;
                        pPrinter->fInErrorState = FALSE;
                        for ( ; iJob>=0 ; --iJob)
                        {
                            // if GetUserName failed, match all jobs
#ifdef WINNT
                            // Exclude jobs that are status PRINTED.
                            if ((!lstrcmpi(pJobs[iJob].pUserName, szUserName) ||
                                !szUserName[0]) &&
                                !(pJobs[iJob].Status & JOB_STATUS_PRINTED))
#else
                            if (!lstrcmpi(pJobs[iJob].pUserName, szUserName) ||
                                !szUserName[0])
#endif
                            {
                                // We found a job owned by the user,
                                // so continue to show the icon
                                ++nFound;
                                dwStatus |= pJobs[iJob].Status;
                                if (pJobs[iJob].Status & JOB_STATUS_USER_INTERVENTION)
                                    pPrinter->fInErrorState = TRUE;
                                fFound = TRUE;
                            }
                        }

                        g_pfnClosePrinter(hPrinter);

                        if (pJobs)
                        {
                            LocalFree(pJobs);
                        }

                        // there may be local jobs (fFound), but if the net is down
                        // then we don't want to poll this printer again -- we have to
                        // wait for the net to timeout which is SLOW.
                        if (!fFound || fNetDown)
                        {
#ifdef DEBUG
                            if (fNetDown)
                                TraceMsg(TF_TRAY, "PRINTNOTIFY - net is down for [%s]", pPrinter->szPrinterName);
                            else
                                TraceMsg(TF_TRAY, "PRINTNOTIFY - no jobs for [%s]", pPrinter->szPrinterName);
#endif
remove_printer:
                            // no need to poll this printer any more
                            ILFree(pPrinter->pidlPrinter);
                            DSA_DeleteItem(haPrinterNames, iPrinter);
                        }
                    } // polling loop

#ifndef WINNT
update_icon:
#endif
                    if (nFound)
                    {
                        UINT nIcon = (dwStatus & JOB_STATUS_ERROR_BITS) ?
                                        ICO_PRINTER_ERROR : ICO_PRINTER;

                        // Keep track of small icon sizes so we can redraw the
                        // icon whenever this changes.
                        cxSmIcon = GetSystemMetrics(SM_CXSMICON);
                        cySmIcon = GetSystemMetrics(SM_CYSMICON);

                        if ((nIconShown != nIcon) || cxSmIcon != s_cxSmIcon || cySmIcon != s_cySmIcon)
                        {
                            nIconShown = nIcon;

                            s_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
                            s_cySmIcon = GetSystemMetrics(SM_CYSMICON);

                            // show an error printer icon if the print job is in
                            // an error state which "requires user intervention"
                            if (dwStatus & JOB_STATUS_ERROR_BITS)
                            {
                                if (hIconError == NULL)
                                    hIconError = (HICON)LoadImage(hinstCabinet,
                                        MAKEINTRESOURCE(nIcon), IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON), 0);
                                IconData.hIcon = hIconError;
                            }
                            else
                            {
                                if (hIconNormal == NULL)
                                    hIconNormal = (HICON)LoadImage(hinstCabinet,
                                        MAKEINTRESOURCE(nIcon), IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON), 0);
                                IconData.hIcon = hIconNormal;
                            }

                            IconData.uFlags |= NIF_ICON;
                        }
                    }
                    else
                    {
                        if (pidlFree)
                        {
                            ILGlobalFree(pidlFree);
                            pidlFree = NULL;
                        }
                        goto exit;
                    }

                    if (aArgs[0] != (DWORD_PTR)nFound)
                    {
                        aArgs[0] = nFound;

                        if (!FormatMessage(
                                FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                szFormat, 0, 0, IconData.szTip, ARRAYSIZE(IconData.szTip),
                                (va_list*)aArgs))
                        {
                            IconData.szTip[0] = TEXT('\0');
                        }

                        IconData.uFlags |= NIF_TIP;
                    }

                    // if the state has changed, update the tray notification area
                    if (IconData.uFlags)
                    {
                        TraceMsg(TF_TRAY, "PRINTNOTIFY - updating icon area");
                        Shell_NotifyIcon(dwNotifyMessage, &IconData);
                        dwNotifyMessage = NIM_MODIFY;
                        IconData.uFlags = 0;
                    }
                    break;

#ifndef WINNT
                //
                // WINNT is hacked only to send SHCNE_CREATE messages.
                //
                case SHCNE_UPDATEITEM:
                    TraceMsg(TF_TRAY, "PRINTNOTIFY - Thread_PrinterPoll SHCNE_UPDATEITEM");

                    //
                    // We need to free the pidl if one is passed in.
                    // (Currently one is not.)
                    //
                    pidlFree = (LPITEMIDLIST)msg.lParam;

                    // if the JOB_STATUS_ERROR_BITS status bit has
                    // changed, update the printer icon
                    if (((DWORD)msg.wParam ^ dwStatus) & JOB_STATUS_ERROR_BITS)
                    {
                        if ((DWORD)msg.wParam & JOB_STATUS_ERROR_BITS)
                        {
                            // A job just went into the error state, turn the
                            // icon on right away.
                            dwStatus |= JOB_STATUS_ERROR_BITS;
                            goto update_icon;
                        }
                        else
                        {
                            // Maybe the last job in an error state went
                            // out of the error state. Poll to find out.
                            // (But not more frequently than MINPOLLINTERVAL.)
                            if (dwTimeOut - dwTime < POLLINTERVAL - MINPOLLINTERVAL)
                                goto poll;
                        }
                    }

                    // Or if the icon size has changed.
                    if (s_cxSmIcon != cxSmIcon || s_cySmIcon != cySmIcon)
                        goto update_icon;

                    // don't wait as long next time
                    dwTimeOut -= dwTime;
                    if (dwTimeOut > POLLINTERVAL)
                    {
                        // oops, rollover; time to poll
                        goto poll;
                    }

                    break;
#endif
#ifdef DEBUG
                default:
                    TraceMsg(TF_TRAY, "PRINTNOTIFY - Bogus PeekMessage 0x%X in Thread_PrinterPoll", msg.message);
                    break;
#endif
                }

                if (pidlFree)
                {
                    ILGlobalFree(pidlFree);
                    pidlFree = NULL;
                }

            } // while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        } // switch (dw)
    } // for ( ; TRUE ; )

exit:
Error2:
    FreeLibrary(hmodWinspool);
Error1:
    {
        // Set g_ts.htPrinterPoll to null here and only here. we have a timing
        // bug where someone sets this to null and we hang.
        // Because g_ts.htPrinterPoll was null, we couldn't trace down what
        // happened to the polling thread (it was lost).

        HANDLE htPrinterPoll = (HANDLE)InterlockedExchangePointer((void **)&g_ts.htPrinterPoll, 0);

        // assuming no race condition these should be true
        ASSERT(htPrinterPoll);
        ASSERT(g_ts.htPrinterPoll == NULL);

        CloseHandle(htPrinterPoll);
    }

    if (psf)
    {
        psf->lpVtbl->Release(psf);
    }

    if (haPrinterNames)
    {
        int i;

        for (i = DSA_GetItemCount(haPrinterNames) - 1 ; i >= 0 ; i--)
        {
            LPPRINTERNAME pPrinter = DSA_GetItemPtr(haPrinterNames, i);
            ILFree(pPrinter->pidlPrinter);
        }

        DSA_Destroy(haPrinterNames);
    }

    if (nIconShown)
    {
        Shell_NotifyIcon(NIM_DELETE, &IconData);
    }

    if (hIconNormal)
    {
        DestroyIcon(hIconNormal);
    }
    if (hIconError)
    {
        DestroyIcon(hIconError);
    }

    return(0);
}

IShellFolder* BindToFolder(LPCITEMIDLIST pidl)
{
    IShellFolder *psfDesktop;
    if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
    {
        IShellFolder* psf;
        psfDesktop->lpVtbl->BindToObject(psfDesktop, pidl, NULL, &IID_IShellFolder, &psf);
        psfDesktop->lpVtbl->Release(psfDesktop);    // not really needed
        return psf;
    }
    return NULL;
}

// This functions adds the printer name pointed to by pidl to the list
// of printers to poll by Thread_PrinterPoll
void PrintNotify_AddToQueue(LPHANDLE phaPrinterNames, LPSHELLFOLDER *ppsf, LPCITEMIDLIST pidlPrinter)
{
    STRRET strret;
    LPCTSTR pNewPrinter;
    LPPRINTERNAME pPrinter;
    int i;
    LPSHELLFOLDER psf;
    PRINTERNAME pn;

#ifdef UNICODE
    LPCTSTR pszFree = NULL;
#endif

    // We need the IShellFolder for the Printers Folder in order to get
    // the display name of the printer
    psf = *ppsf;
    if (psf == NULL)
    {
        psf = BindToFolder(g_ts.pidlPrintersFolder);
        if (!psf)
        {
            goto exit;
        }
        *ppsf = psf;
    }

    // all this work to get the name of the printer:
    psf->lpVtbl->GetDisplayNameOf(psf, pidlPrinter, SHGDN_FORPARSING, &strret);

    // we know how we implemented the printers GetDisplayNameOf function
#ifdef UNICODE

    //
    // pszFree saves a copy that will be freed on exit.
    //
    ASSERT(strret.uType == STRRET_OLESTR);
    pszFree = pNewPrinter = strret.pOleStr;
#else
    ASSERT(strret.uType == STRRET_OFFSET);
    pNewPrinter = STRRET_OFFPTR(pidlPrinter, &strret);
#endif

    if (*phaPrinterNames == NULL)
    {
        *phaPrinterNames = DSA_Create(SIZEOF(PRINTERNAME), 4);
        if (*phaPrinterNames == NULL)
            goto exit;
    }

    for (i=DSA_GetItemCount(*phaPrinterNames)-1 ; i>=0 ; --i)
    {
        pPrinter = DSA_GetItemPtr(*phaPrinterNames, i);

        if (!lstrcmp(pPrinter->szPrinterName, pNewPrinter))
        {
            // printer already in list, no need to add it
            TraceMsg(TF_TRAY, "PRINTNOTIFY - [%s] already in poll list", pNewPrinter);
            goto exit;
        }
    }

    TraceMsg(TF_TRAY, "PRINTNOTIFY - adding [%s] to poll list", pNewPrinter);

    lstrcpy(pn.szPrinterName, pNewPrinter);
    pn.fInErrorState = FALSE;
    pn.pidlPrinter = ILClone(pidlPrinter);
    DSA_AppendItem(*phaPrinterNames, &pn);

exit:

#ifdef UNICODE
    //
    // Free the allocated string from GetDisplayNameOf in strret.
    //
    if (pszFree)
    {
        SHFree((void *)pszFree);
    }
#endif
    return;
}


BOOL PrintNotify_StartThread(HWND hwnd)
{
    PPRINTERPOLLDATA pPrinterPollData;
    BOOL fRet;

    if (g_ts.htPrinterPoll)
    {
        return TRUE;
    }

    ENTERCRITICAL;

    // We check this again so that the above check will go very quickly,
    // and almost always do the return

    fRet = g_ts.htPrinterPoll != NULL;
    if (fRet)
    {
        goto Error_Exit;
    }

    // We keep one event around to communicate with the polling thread so
    // we don't need to keep re-creating it whenever the user prints.
    // On the down side: this thing never does get freed up... Oh well.
    if (!g_ts.heWakeUp)
    {
        g_ts.heWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!g_ts.heWakeUp)
            goto Error_Exit;
    }

    pPrinterPollData = (PPRINTERPOLLDATA)GlobalAlloc(GPTR, SIZEOF(PRINTERPOLLDATA));
    if (!pPrinterPollData)
    {
        goto Error_Exit;
    }
    pPrinterPollData->hwnd = hwnd;
    pPrinterPollData->heWakeUp = g_ts.heWakeUp;

    g_ts.htPrinterPoll = CreateThread(NULL, 0, Thread_PrinterPoll,
                        pPrinterPollData, 0, &g_ts.idPrinterPoll);

    fRet = g_ts.htPrinterPoll != NULL;

    LEAVECRITICAL;

    if (fRet)
    {
        // wait until the new thread's message queue is ready
        // I had INFINITE here, but something funky happened once on a build
        // machine which caused MSGSRV32 to hang waiting for this thread to
        // unblock. If we timeout, the worst that will happen is our icon won't
        // show and/or we'll keep destroying re-creating the polling thread
        // until everything works.
        WaitForSingleObject(g_ts.heWakeUp, ERRTIMEOUT);
        TraceMsg(TF_TRAY, "PRINTNOTIFY - Thread_PrinterPoll started");
    }
    else
    {
        GlobalFree(pPrinterPollData);
    }

    goto Exit;

Error_Exit:
    LEAVECRITICAL;
Exit:
    return fRet;
}

void PrintNotify_HandleFSNotify(HWND hwnd, LPCITEMIDLIST *ppidl, LONG lEvent)
{
    // Look at the pidl to determine if it's a Print Job notification. If it
    // is, we may also need a copy of the relative Printer pidl. Pass the
    // notification on to our polling thread so the tray isn't stuck waiting
    // for a (net) print subsystem query (up to 15 seconds if net is funky).
    LPITEMIDLIST pidl = ILClone(ppidl[0]);
    if (pidl)
    {
        LPITEMIDLIST pidlPrintJob;
        LPITEMIDLIST pidlPrinter;
        LPITEMIDLIST pidlPrintersFolder;
        USHORT cbPidlPrinter;

        pidlPrintJob = ILFindLastID(pidl);
        ILRemoveLastID(pidl);
        pidlPrinter = ILFindLastID(pidl);
        cbPidlPrinter = pidlPrinter->mkid.cb;
        ILRemoveLastID(pidl);
        pidlPrintersFolder = pidl;

        if (ILIsEqual(pidlPrintersFolder, g_ts.pidlPrintersFolder))
        {
            if (PrintNotify_StartThread(hwnd))
            {
                LPCITEMIDLIST pidlPrinterName = NULL;
                LPSHCNF_PRINTJOB_DATA pData;

                // HACK: we know the format of this (internal) "temporary" pidl
                // which exists only between SHChangeNotify and here
                pData = (LPSHCNF_PRINTJOB_DATA)(pidlPrintJob->mkid.abID);

                // PERFORMANCE: SHCNE_CREATE can just add one to the local count
                // and SHCNE_DELETE will sub one from the local count and start
                // polling this printer to get the net count.
                // GOOD ENOUGH: poll on create and delete.
                if (SHCNE_UPDATEITEM != lEvent)
                {
                    // this was whacked with one of the above ILRemoveLastID()s
                    pidlPrinter->mkid.cb = cbPidlPrinter;

                    // this is freed in the polling thread
                    pidlPrinterName = ILGlobalClone(pidlPrinter);
                }

                // REVIEW: What if we PostThreadMessage to a dead thread?
                // PrintNotify_StartThread can start the thread and give us
                // the idPrinterPoll, but then under certain error conditions
                // the new thread will terminate itself. This can happen
                // after the above if and before this post.

                PostThreadMessage(g_ts.idPrinterPoll, WM_USER+(UINT)lEvent,
                        (WPARAM)pData->Status, (LPARAM)pidlPrinterName);
            }
        }

        ILFree(pidl);
    }
}

void PrintNotify_Exit(void)
{
    // PrintNotify_Init only initializes if it can get the pidlPrintsFolder
    if (g_ts.pidlPrintersFolder)
    {
        if (g_ts.uPrintNotify)
        {
            SHChangeNotifyDeregister(g_ts.uPrintNotify);
        }

        if (g_ts.htPrinterPoll)
        {
            // Signal the PrinterPoll thread to exit
            if (g_ts.heWakeUp)
            {
                SetEvent(g_ts.heWakeUp);
            }

            WaitForSingleObject(g_ts.htPrinterPoll, ERRTIMEOUT);
        }

        // Free this last just in case the htPrinterPoll thread was active
        ILFree((LPITEMIDLIST)g_ts.pidlPrintersFolder);
    }
}


void PrintNotify_IconNotify(HWND hwnd, LPARAM uMsg)
{
    switch (uMsg)
    {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        // call off to worker thread to bring up the context menu --
        // we need to access the list of polling printers for this
        if (g_ts.htPrinterPoll && g_ts.idPrinterPoll)
        {
            PostThreadMessage(g_ts.idPrinterPoll, (UINT)(WM_USER+uMsg), 0, 0);
        }
        else if (uMsg == WM_LBUTTONDBLCLK)
        {
            ShowFolder(hwnd, CSIDL_PRINTERS, COF_USEOPENSETTINGS);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////
// End Print Notify stuff
////////////////////////////////////////////////////////////////////////////

#ifndef WINNT           // All NT drivers are required to support reschange

////////////////////////////////////////////////////////////////////////////
//
// IsDisplayChangeSafe()
//
// make sure the display change is a safe thing to do.
//
// NOTE this code can easily be fooled if we are running on HW that will be
// gone after a undoc.
//
////////////////////////////////////////////////////////////////////////////

BOOL IsDisplayChangeSafe()
{
    HDC  hdc;
    BOOL fSafe;
    BOOL fVGA;

    hdc = GetDC(NULL);

    fVGA = GetDeviceCaps(hdc, PLANES) == 4 &&
           GetDeviceCaps(hdc, BITSPIXEL) == 1 &&
           GetDeviceCaps(hdc, HORZRES) == 640 &&
           GetDeviceCaps(hdc, VERTRES) == 480;

    fSafe = fVGA || (GetDeviceCaps(hdc, CAPS1) & C1_REINIT_ABLE);

    ReleaseDC(NULL, hdc);
    return fSafe;
}

#endif

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

int BlueScreenMesssgeBox(TCHAR *szTitle, TCHAR *szText, int flags)
{
#ifndef WINNT
    BLUESCREENINFO bsi = {szText, szTitle, flags};
    int cb;
    HANDLE h;

//BUGBUG
//  AnsiToOEM(szTitle, szTitle);
//  AnsiToOEM(szText, szText);

    h = CreateFile(SHELLFILENAME, GENERIC_WRITE, FILE_SHARE_WRITE,
            0, OPEN_EXISTING, 0, 0);

    if (h != INVALID_HANDLE_VALUE)
    {
        DeviceIoControl(h, WSHIOCTL_BLUESCREEN, &bsi, SIZEOF(bsi),
            &flags, SIZEOF(flags), &cb, 0);
        CloseHandle(h);
    }
    return flags;
#else
    // BUGBUG - We need to figure out what to do for NT here
    return 0;
#endif
}

const TCHAR c_szConfig[] = REGSTR_KEY_CONFIG;
const TCHAR c_szSlashDisplaySettings[] = TEXT("\\") REGSTR_PATH_DISPLAYSETTINGS;
const TCHAR c_szResolution[] = REGSTR_VAL_RESOLUTION;

//
// GetMinDisplayRes
//
// walk all the configs and find the minimum display resolution.
//
// when doing a hot undock we have no idea what config we are
// going to undock into.
//
// we want to put the display into a "common" mode that all configs
// can handle so we dont "fry" the display when we wake up in the
// new mode.
//
DWORD GetMinDisplayRes(void)
{
    TCHAR ach[128];
    UINT cb;
    HKEY hkey;
    HKEY hkeyT;
    int i, n;
    int xres=0;
    int yres=0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szConfig, &hkey) == ERROR_SUCCESS)
    {
        for (n=0; RegEnumKey(hkey, n, ach, ARRAYSIZE(ach)) == ERROR_SUCCESS; n++)
        {
            lstrcat(ach, c_szSlashDisplaySettings);  // 0000\Display\Settings

            TraceMsg(TF_TRAY, "GetMinDisplayRes: found config %s", ach);

            if (RegOpenKey(hkey, ach, &hkeyT) == ERROR_SUCCESS)
            {
                cb = SIZEOF(ach);
                ach[0] = 0;
                RegQueryValueEx(hkeyT, c_szResolution, 0, NULL, (LPBYTE) &ach[0], &cb);

                TraceMsg(TF_TRAY, "GetMinDisplayRes: found res %s", ach);

                if (ach[0])
                {
                    i = StrToInt(ach);

                    if (i < xres || xres == 0)
                        xres = i;

                    for (i=1;ach[i] && ach[i-1]!=TEXT(','); i++)
                        ;

                    i = StrToInt(ach + i);

                    if (i < yres || yres == 0)
                        yres = i;
                }
                else
                {
                    xres = 640;
                    yres = 480;
                }

                RegCloseKey(hkeyT);
            }
        }
        RegCloseKey(hkey);
    }

    TraceMsg(TF_TRAY, "GetMinDisplayRes: xres=%d yres=%d", xres, yres);

    if (xres == 0 || yres == 0)
        return MAKELONG(640, 480);
    else
        return MAKELONG(xres, yres);
}

//
//  the user has done a un-doc or re-doc we may need to switch
//  to a new display mode.
//
//  if fCritical is set the mode switch is critical, show a error
//  if it does not work.
//
void HandleDisplayChange(int x, int y, BOOL fCritical)
{
    TCHAR ach[256];
    DEVMODE dm;
    LONG err;
    HDC hdc;

    //
    //  try to change into the mode specific to this config
    //  HKEY_CURRENT_CONFIG has already been updated by PnP
    //  so all we have to do is re-init the current display
    //
    //  we cant default to current bpp because we may have changed configs.
    //  and the bpp may be different in the new config.
    //
    dm.dmSize   = SIZEOF(dm);
    dm.dmFields = DM_BITSPERPEL;

    hdc = GetDC(NULL);
    dm.dmBitsPerPel = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    if (x + y)
    {
        dm.dmFields    |= DM_PELSWIDTH|DM_PELSHEIGHT;
        dm.dmPelsWidth  = x;
        dm.dmPelsHeight = y;
    }

    err = ChangeDisplaySettings(&dm, 0);

    if (err != 0 && fCritical)
    {
        //
        //  if it fails make a panic atempt to try 640x480, if
        //  that fails also we should put up a big error message
        //  in text mode and tell the user he is screwed.
        //
        dm.dmFields     = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;
        dm.dmPelsWidth  = 640;
        dm.dmPelsHeight = 480;

        err = ChangeDisplaySettings(&dm, 0);

        if (err != 0)
        {
            //
            //  if 640x480 fails we should put up a big error message
            //  in text mode and tell the user he is screwed. and
            //  offer to reboot his machine.
            //
            MessageBeep(0);

            LoadString(hinstCabinet, IDS_DISPLAY_ERROR, ach, ARRAYSIZE(ach));
            BlueScreenMesssgeBox(NULL, ach, MB_OK);
        }
    }
}



void Tray_HandleGlobalHotkey(WPARAM wParam)
{
    INSTRUMENT_HOTKEY(SHCNFI_GLOBALHOTKEY, wParam);

    UEMFireEvent(&UEMIID_SHELL, UEME_UIHOTKEY, UEMF_XEVENT, -1, wParam);     // NYI FEATURE_UASSIST

    switch(wParam)
    {
    case GHID_RUN:
        Tray_RunDlg();
        break;

    case GHID_MINIMIZEALL:
        if (CanMinimizeAll(g_ts.hwndView))
            MinimizeAll(g_ts.hwndView);
        SetForegroundWindow(v_hwndDesktop);
        break;

    case GHID_UNMINIMIZEALL:
        RestoreWindowPositions();
        break;

    case GHID_HELP:
        Tray_Command(IDM_HELPSEARCH);
        break;

    case GHID_DESKTOP:
        ToggleDesktop();
        break;

    case GHID_EXPLORER:
        ShowFolder(g_ts.hwndMain, CSIDL_DRIVES, COF_CREATENEWWINDOW | COF_EXPLORE);
        break;

    case GHID_FINDFILES:
        if (!SHRestricted(REST_NOFIND))
            Tray_Command(FCIDM_FINDFILES);
        break;

    case GHID_FINDCOMPUTER:
        if (!SHRestricted(REST_NOFIND))
            Tray_Command(FCIDM_FINDCOMPUTER);
        break;

    case GHID_TASKTAB:
    case GHID_TASKSHIFTTAB:
        if (GetForegroundWindow() != v_hwndTray)
            SetForegroundWindow(v_hwndTray);
        SendMessage(g_ts.hwndView, TM_TASKTAB, wParam == GHID_TASKTAB ? 1 : -1, 0L);
        break;

    case GHID_SYSPROPERTIES:
#define IDS_SYSDMCPL            0x2334  // from shelldll
        SHRunControlPanel(MAKEINTRESOURCE(IDS_SYSDMCPL), g_ts.hwndMain);
        break;
    }
}

void Tray_UnregisterGlobalHotkeys()
{
    int i;

    for (i = GHID_FIRST ; i < GHID_MAX; i++) {
        UnregisterHotKey(v_hwndTray, i);
    };
}

void Tray_RegisterGlobalHotkeys()
{
    int i;
    // Are the Windows keys restricted?
    DWORD dwRestricted = SHRestricted(REST_NOWINKEYS);

    for (i = GHID_FIRST ; i < GHID_MAX; i++) 
    {
        // If the Windows Keys are Not restricted or it's not a Windows key
        if (!( (HIWORD(GlobalKeylist[i - GHID_FIRST]) & MOD_WIN) && dwRestricted) )
        {
            // Then register it.
            RegisterHotKey(v_hwndTray, i, HIWORD(GlobalKeylist[i - GHID_FIRST]), LOWORD(GlobalKeylist[i - GHID_FIRST]));
        }
    }
}

void RaiseDesktop()
{
    if (v_hwndDesktop) {
        if (CanMinimizeAll(g_ts.hwndView))
             MinimizeAll(g_ts.hwndView);
        PostMessage(v_hwndDesktop, DTM_RAISE, (WPARAM)v_hwndTray, DTRF_RAISE);
#ifdef CONFIG_DESKTOP
        if (!g_hdlgDesktopConfig)
        {
            g_hdlgDesktopConfig = CreateDialog(hinstCabinet, MAKEINTRESOURCE(DLG_CONFIGDESKTOP), HWND_DESKTOP, (DLGPROC)ConfigDesktopDlgProc);
            g_fShouldShowConfigDesktop = TRUE;
        }
#endif
    }
}

HWND g_hwndFocusBeforeRaise = NULL;

void LowerDesktop()
{
    if (v_hwndDesktop && g_fDesktopRaised) {
        PostMessage(v_hwndDesktop, DTM_RAISE, (WPARAM)v_hwndTray, DTRF_LOWER);
        g_hwndFocusBeforeRaise = NULL;
        g_fShouldShowConfigDesktop = FALSE;
        KillConfigDesktopDlg();
    }
}

void QueryDesktop()
{
    if (v_hwndDesktop)
        PostMessage(v_hwndDesktop, DTM_RAISE, (WPARAM)v_hwndTray, DTRF_QUERY);
}

void ToggleDesktop()
{
    if (g_fDesktopRaised) {
        HWND hwndLastActive;
        HWND hwnd = g_hwndFocusBeforeRaise;
        LowerDesktop();

        if (!CanMinimizeAll(g_ts.hwndView))
            RestoreWindowPositions();

        if (!hwnd)
            hwnd = v_hwndTray;

        // use GetLastActivePopup (if it's a visible window) so we don't change
        // what child had focus all the time
        hwndLastActive = GetLastActivePopup(hwnd);

        if (IsWindowVisible(hwndLastActive))
            hwnd = hwndLastActive;

        SetForegroundWindow(hwnd);

        if (hwnd == v_hwndTray) {
            Tray_SetFocus(g_ts.hwndStart);
        }

    } else {
        g_hwndFocusBeforeRaise = GetForegroundWindow();
        RaiseDesktop();
    }
}

void Tray_OnDesktopState(LPARAM lParam)
{
    g_fDesktopRaised = (!(lParam & DTRF_LOWER));

    DAD_ShowDragImage(FALSE);       // unlock the drag sink if we are dragging.

    if (g_hwndDesktopTB)
        SendMessage(g_hwndDesktopTB, TB_CHECKBUTTON, IDM_RAISEDESKTOP, g_fDesktopRaised);

    if (!g_fDesktopRaised)
    {
        Tray_HandleFullScreenApp(g_ts.hwndRude);
    }
    else
    {
        // if the desktop is raised, we need to force the tray to be always on top
        // until it's lowered again
        Tray_ResetZorder();
    }

    DAD_ShowDragImage(TRUE);       // unlock the drag sink if we are dragging.
}

#ifdef DESKBTN
void Tray_CreateDesktopButton()
{
    HWND hwnd = g_ts.hwndMain;
    TBBUTTON tb = { 0, IDM_RAISEDESKTOP,         TBSTATE_ENABLED,               BTNS_CHECK, {0,0}, 0, -1 };

    if (!SHRestricted(REST_CLASSICSHELL))
        g_hwndDesktopTB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                         TBSTYLE_FLAT| CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS, // | TBSTYLE_TOOLTIPS
                                         0, 0, 100, 30, hwnd, (HMENU)IDC_RAISEDESKTOP, hinstCabinet, NULL);
    if (g_hwndDesktopTB) {
        LPITEMIDLIST pidlDesktop;

        g_DesktopTBProc = SubclassWindow(g_hwndDesktopTB, DesktopTBSubclass);

        // this tells the toolbar what version we are
        SendMessage(g_hwndDesktopTB, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(g_hwndDesktopTB, TB_SETBITMAPSIZE, 0, MAKELONG(16,16));
        SendMessage(g_hwndDesktopTB, TB_SETBUTTONWIDTH, 0, MAKELONG(g_ts.sizeStart.cy, g_ts.sizeStart.cy));
        SendMessage(g_hwndDesktopTB, TB_SETTOOLTIPS, (WPARAM)g_ts.hwndTrayTips, 0);
        SendMessage(g_hwndDesktopTB, TB_SETEXTENDEDSTYLE,    0, TBSTYLE_EX_DRAWDDARROWS);

        SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlDesktop);
        if (pidlDesktop) {
            HIMAGELIST himl;
            SHFILEINFO sfi;

            Shell_GetImageLists(NULL, &himl);
            SHGetFileInfo((LPCTSTR)pidlDesktop, 0, &sfi ,sizeof(sfi), SHGFI_SYSICONINDEX |SHGFI_PIDL);
            tb.iBitmap = sfi.iIcon;

            SendMessage(g_hwndDesktopTB, TB_SETIMAGELIST, 0, (LPARAM)himl);

            SendMessage(g_hwndDesktopTB, TB_ADDBUTTONS, 1, (LPARAM)&tb);
            ILFree(pidlDesktop);
        }
    }
}
#endif



//---------------------------------------------------------------------------
// Process the message by propagating it to all of our child windows
struct _CabPM
{
    UINT uMsg;
    WPARAM wP;
    LPARAM lP;
};


BOOL _PropagateProc(HWND hwnd, struct _CabPM *ppm)
{
    if (SHIsChildOrSelf(g_ts.hwndRebar, hwnd) == S_OK) {
        return TRUE;
    }
    SendMessage(hwnd, ppm->uMsg, ppm->wP, ppm->lP);
    return TRUE;
}


void _PropagateMessage(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    struct _CabPM pm = {uMessage, wParam, lParam};

    ASSERT(hwnd != g_ts.hwndRebar);
    EnumChildWindows(hwnd, (WNDENUMPROC)_PropagateProc, (LPARAM)&pm);
}


BOOL CALLBACK Cabinet_RefreshEnum(HWND hwnd, LPARAM lParam)
{
    if (Cabinet_IsFolderWindow(hwnd) || Cabinet_IsExplorerWindow(hwnd))
    {
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);
    }

    return(TRUE);
}


void Cabinet_RefreshAll(void)
{
    PostMessage(v_hwndDesktop, WM_COMMAND, FCIDM_REFRESH, 0L);
    PostMessage(v_hwndTray, WM_COMMAND, FCIDM_REFRESH, 0L);

    EnumWindows(Cabinet_RefreshEnum, 0);
}

//
// Called from SETTINGS.DLL when the tray property sheet needs
// to be activated.  See SettingsUI_ThreadProc below.
//
VOID WINAPI SettingsUI_TrayPropSheetCallback(DWORD nStartPage)
{
    DoTrayProperties(nStartPage);
}


DWORD SettingsUI_ThreadProc(void *pv)
{
    //
    // Open up the "Settings Wizards" UI.
    //
    HMODULE hmodSettings = LoadLibrary(TEXT("settings.dll"));
    if (NULL != hmodSettings)
    {
        //
        // Entry point in SETTINGS.DLL is ordinal 1.
        // Don't want to export this entry point by name.
        //
        PSETTINGSUIENTRY pfDllEntry = (PSETTINGSUIENTRY)GetProcAddress(hmodSettings, (LPCSTR)1);
        if (NULL != pfDllEntry)
        {
            //
            // This call will open and run the UI.
            // The thread's message loop is inside settings.dll.
            // This call will not return until the settings UI has been closed.
            //
            (*pfDllEntry)(SettingsUI_TrayPropSheetCallback);
        }

        FreeLibrary(hmodSettings);
    }
    return 0;
}

HRESULT CDeskTray_QueryInterface(IDeskTray* this, REFIID riid, void ** ppvObj)
{
    *ppvObj = NULL;
    return E_NOTIMPL;
}

ULONG CDeskTray_AddRef(IDeskTray* this)
{
    return 2;
}

ULONG CDeskTray_Release(IDeskTray* this)
{
    return 1;
}

HRESULT CDeskTray_GetTrayWindow(IDeskTray* this, HWND* phwndTray)
{
    ASSERT(v_hwndTray);
    *phwndTray = v_hwndTray;
    return S_OK;
}

HRESULT CDeskTray_SetDesktopWindow(IDeskTray* this, HWND hwndDesktop)
{
    ASSERT(v_hwndDesktop == NULL);
    v_hwndDesktop = hwndDesktop;
    return S_OK;
}

//***   CDeskTray::SetVar -- set an explorer variable (var#i := value)
// ENTRY/EXIT
//  var     id# of variable to be changed
//  value   value to be assigned
// NOTES
//  WARNING: thread safety is up to caller!
//  notes: currently only called in 1 place, but extra generality is cheap
// minimal cost
HRESULT CDeskTray_SetVar(IDeskTray* this, int var, DWORD value)
{
    extern BOOL g_fExitExplorer;

    TraceMsg(DM_TRACE, "c.cdt_sv: set var(%d):=%d", var, value);
    switch (var) {
    case SVTRAY_EXITEXPLORER:
        TraceMsg(DM_TRACE, "c.cdt_sv: set g_fExitExplorer:=%d", value);
        g_fExitExplorer = value;
        WriteCleanShutdown(1);
        break;

    default:
        ASSERT(0);
        return S_FALSE;
    }
    return S_OK;
}

//
// *** WARNING ***
//
//  This is a private interface EXPLORER.EXE exposes to SHDOCVW, which
// allows SHDOCVW (mostly desktop) to access tray. All member must be
// thread safe!
//
const IDeskTrayVtbl c_DeskTrayVtbl =
{
    CDeskTray_QueryInterface, CDeskTray_AddRef, CDeskTray_Release,
    CDeskTray_AppBarGetState,
    CDeskTray_GetTrayWindow,
    CDeskTray_SetDesktopWindow,
    CDeskTray_SetVar,               // thread safe iff var in question is!
    // READ above warning before you are any member here
};

const struct {
    IDeskTray dtray;
} c_CDeskTray = { (IDeskTrayVtbl*)&c_DeskTrayVtbl };

IDeskTray* const c_pdtray = (IDeskTray*)&c_CDeskTray.dtray;


// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject)
{
    RECT    rc;
    POINT   pt;

    GetWindowRect(hwndTarget, &rc);
    pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragEnterEx2(hwndTarget, pt, pdtObject);
    return;
}

void _DragMove(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    POINT pt;

    GetWindowRect(hwndTarget, &rc);
    pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragMove(pt);
    return;
}
