//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       Main.CPP
//
//  History:    Nov-10-95   DavePl  Created
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include <htmlhelp.h>

static UINT g_msgTaskbarCreated = 0;
//
// Control IDs
//

#define IDC_STATUSWND   100

//
// Globals - this app is (effectively) single threaded and these values
//           are used by all pages
//

const TCHAR cszStartupMutex[] = TEXT("NTShell Taskman Startup Mutex");
#define FINDME_TIMEOUT 10000                // Wait to to 10 seconds for a response

void MainWnd_OnSize(HWND hwnd, UINT state, int cx, int cy);

HANDLE      g_hStartupMutex = NULL;
BOOL        g_fMenuTracking = FALSE;
HWND        g_hMainWnd      = NULL;
HDESK       g_hMainDesktop  = NULL;
HWND        g_hStatusWnd    = NULL;
HINSTANCE   g_hInstance     = NULL;
HACCEL      g_hAccel        = NULL;
BYTE        g_cProcessors   = (BYTE) 0;
HBITMAP     g_hbmpBack      = NULL;
HBITMAP     g_hbmpForward   = NULL;
HMENU       g_hMenu         = NULL;
BOOL        g_fCantHide     = FALSE;
BOOL        g_fInPopup      = FALSE;
DWORD       g_idTrayThread  = 0;
HANDLE      g_hTrayThread   = NULL;
LONG        g_minWidth      = 0;
LONG        g_minHeight     = 0;
LONG        g_DefSpacing    = 0;
LONG        g_InnerSpacing  = 0;
LONG        g_TopSpacing    = 0;
LONG        g_cxEdge        = 0;

HRGN        g_hrgnView      = NULL;
HRGN        g_hrgnClip      = NULL;

HBRUSH      g_hbrWindow     = NULL;

COptions    g_Options;

static BOOL fAlreadySetPos  = FALSE;

BOOL g_bMirroredOS = FALSE;

#define SHORTSTRLEN         32

//
// Global strings - short strings used too often to be LoadString'd
//                  every time
//

TCHAR       g_szRealtime    [SHORTSTRLEN];
TCHAR       g_szNormal      [SHORTSTRLEN];
TCHAR       g_szHigh        [SHORTSTRLEN];
TCHAR       g_szLow         [SHORTSTRLEN];
TCHAR       g_szUnknown     [SHORTSTRLEN];
TCHAR       g_szAboveNormal [SHORTSTRLEN];
TCHAR       g_szBelowNormal [SHORTSTRLEN];
TCHAR       g_szHung        [SHORTSTRLEN];
TCHAR       g_szRunning     [SHORTSTRLEN];
TCHAR       g_szfmtTasks    [SHORTSTRLEN];
TCHAR       g_szfmtProcs    [SHORTSTRLEN];
TCHAR       g_szfmtCPU      [SHORTSTRLEN];  
TCHAR       g_szfmtMEM      [SHORTSTRLEN];  
TCHAR       g_szfmtCPUNum   [SHORTSTRLEN];
TCHAR       g_szTotalCPU    [SHORTSTRLEN];
TCHAR       g_szKernelCPU   [SHORTSTRLEN];
TCHAR       g_szMemUsage    [SHORTSTRLEN];

TCHAR       g_szK[10];                     // Localized "K"ilobyte symbol

// Page Array
// 
// Each of the page objects is delcared here, and g_pPages is an array
// of pointers to those instantiated objects (at global scope).  The main
// window code can call through the base members of the CPage class to
// do things like sizing, etc., without worrying about whatever specific
// stuff each page might do

CPage * g_pPages[NUM_PAGES] = { NULL };

// to hold pointer to the winsta.dll function

// this function is used to get the username for the given process.

pfnWinStationGetProcessSid gpfnWinStationGetProcessSid = 0;

// to hold the pointer to the utildll function.

// this function hold gets the username from sid.

pfnCachedGetUserFromSid gpfnCachedGetUserFromSid = 0;

// to hold pointer to the winsta.dll function

// this function is used to terminate cross session processes

// NtOpenProcess on cross session processes fails unless you enable
// the debug privilige. This is done by termsrv.exe

pfnWinStationTerminateProcess gpfnWinStationTerminateProcess = 0;


/*
   Superclass of GROUPBOX
 
   We need to turn on clipchildren for our dialog which contains the
   history graphs, so they don't get erased during the repaint cycle.
   Unfortunately, group boxes don't erase their backgrounds, so we
   have to superclass them and provide a control that does.

   This is a lot of extra crap, but the painting is several orders of
   magnitude nicer with it...

*/

/*++ DavesFrameWndProc

Routine Description:

    WndProc for the custom group box class.  Primary difference from
    standard group box is that this one knows how to erase its own
    background, and doesn't rely on the parent to do it for it.
    These controls also have CLIPSIBLINGS turn on so as not to stomp
    on the ownderdraw graphs they surround.
    
Arguments:

    standard wndproc fare

Revision History:

      Nov-29-95 Davepl  Created

--*/

WNDPROC oldButtonWndProc = NULL;
                               
LRESULT DavesFrameWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        //
        // Turn on clipsiblings for the frame
        //

        DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
        dwStyle |= WS_CLIPSIBLINGS;
        SetWindowLong(hWnd, GWL_STYLE, dwStyle);
    }
    else if (msg == WM_ERASEBKGND)
    {
        //
        // Erase our background so that we don't get ugly in a dialog that has
        // clip children on (as our parents will in this app)
        //

        HDC hdc = (HDC) wParam;

        // 
        // If no dc is supplied by the caller, we create our own by building the
        // update region and using GetDCEx to get a DC that corresponds to only
        // the actual area that needs to be erased.
        //
                
        HRGN hRegion;

        if (0 == wParam)
        {
            // Isn't Windows painting self-evident?

            hRegion = CreateRectRgn(0, 0, 0, 0);
            if (hRegion)
            {
                GetUpdateRgn(hWnd, hRegion, TRUE);
                hdc = GetDCEx(hWnd, hRegion, DCX_CACHE | DCX_CLIPSIBLINGS | DCX_INTERSECTRGN);
            }
        }

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        FillRect(hdc, &rcClient, (HBRUSH)(COLOR_3DFACE + 1));

        if (0 == wParam)
        {
            // If your compiler warns about hRegion not being initialized, it's lying

            ReleaseDC(hWnd, hdc);
            if (hRegion)
            {
                DeleteObject(hRegion);
            }
        }
        return TRUE;
    }

    // For anything else, we defer to the standard button class code

    return oldButtonWndProc(hWnd, msg, wParam, lParam);
}

/*++ COptions::Save 

Routine Description:

   Saves current options to the registy
 
Arguments:

Returns:
    
    HRESULT

Revision History:

      Jan-01-95 Davepl  Created

--*/

const TCHAR szTaskmanKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\TaskManager");
const TCHAR szOptionsKey[] = TEXT("Preferences");

HRESULT COptions::Save()
{
    DWORD dwDisposition;
    HKEY  hkSave;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER,
                                        szTaskmanKey,
                                        0,
                                        TEXT("REG_BINARY"),
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_WRITE,
                                        NULL,
                                        &hkSave,
                                        &dwDisposition))
    {
        return GetLastHRESULT();
    }

    if (ERROR_SUCCESS != RegSetValueEx(hkSave,
                                       szOptionsKey,
                                       0,
                                       REG_BINARY,
                                       (LPBYTE) this,
                                       sizeof(COptions)))
    {
        RegCloseKey(hkSave);
        return GetLastHRESULT();
    }

    RegCloseKey(hkSave);
    return S_OK;
}

/*++ COptions::Load

Routine Description:

   Loads current options to the registy
 
Arguments:

Returns:
    
    HRESULT

Revision History:

      Jan-01-95 Davepl  Created

--*/

HRESULT COptions::Load()
{
    HKEY  hkSave;

    // If ctrl-alt-shift is down at startup, "forget" registry settings

    if (GetKeyState(VK_SHIFT) < 0 &&
        GetKeyState(VK_MENU)  < 0 &&
        GetKeyState(VK_CONTROL) < 0   )
    {
        SetDefaultValues();
        return S_FALSE;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                      szTaskmanKey,
                                      0,
                                      KEY_READ,
                                      &hkSave))
    {
        return S_FALSE;
    }

    DWORD dwType;
    DWORD dwSize = sizeof(COptions);
    if (ERROR_SUCCESS       != RegQueryValueEx(hkSave,
                                               szOptionsKey,
                                               0,
                                               &dwType,
                                               (LPBYTE) this,
                                               &dwSize) 
        
        // Validate type and size of options info we got from the registry

        || dwType           != REG_BINARY 
        || dwSize           != sizeof(COptions)

        // Validate options, revert to default if any are invalid (like if
        // the window would be offscreen)

        || MonitorFromRect(&m_rcWindow, MONITOR_DEFAULTTONULL) == NULL
        || m_iCurrentPage    > NUM_PAGES - 1)

    {
        // Reset to default values

        SetDefaultValues();
        RegCloseKey(hkSave);
        return S_FALSE;
    }

    RegCloseKey(hkSave);

    // if machine is updated to/from terminal server
    // we might get wrong columns from the registry.
    // for example if machine was previously hydra and 
    // the last columns displayed contained username.
    // Afterwords hydra is uninstalled. Now when we 
    // load the columns next time we find username there
    // which should not be in case of non hydra systems.
    // lets take care of such situation here.
    // _HYDRA_
    if( !IsTerminalServer( ) )
    {
        // remove username, session id columns if present.

        for( int i = 0; i < NUM_COLUMN + 1 ; i++ )
        {
            if( g_Options.m_ActiveProcCol[ i ] == -1 )
            {
                // this is end of column list.

                break;  
            }

            if( g_Options.m_ActiveProcCol[ i ] == COL_SESSIONID || g_Options.m_ActiveProcCol[ i ] == COL_USERNAME )
            {
                // current values are not good as they contain hydra specific column
                // so lets set the default values.

                SetDefaultValues();

                break;
            }
        }

    }
    //_HYDRA_

    return S_OK;
}

BOOL FPalette(void)
{
    HDC hdc = GetDC(NULL);
    BOOL fPalette = (GetDeviceCaps(hdc, NUMCOLORS) != -1);
    ReleaseDC(NULL, hdc);
    return fPalette;
}

#define CMS_FADE 200

inline void MyShowWindow(HWND hwnd, UINT nShowCmd)
{
#if 0
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int cx = g_Options.m_rcWindow.right-g_Options.m_rcWindow.left;
    int cy = g_Options.m_rcWindow.bottom-g_Options.m_rcWindow.top;
    
    if ((nShowCmd == SW_SHOWNORMAL || nShowCmd == SW_SHOW || 
            nShowCmd == SW_SHOWDEFAULT || nShowCmd == SW_HIDE)          && 
            !FPalette()                                                 && 
            (cx == g_minWidth)                                          && 
            (cy == g_minHeight)                                         &&
            ((si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL
                    &&   si.wProcessorLevel >= 6) ||
            (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA
                    &&   si.wProcessorLevel >= 21164))) {
        if (nShowCmd != SW_HIDE) {
            HWND hwndTab = GetDlgItem(g_hMainWnd, IDC_TABS);
            SetWindowPos(hwndTab, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
            AnimateWindow(g_hMainWnd, CMS_FADE, AW_BLEND | AW_ACTIVATE);
            SetWindowPos(hwndTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        } else {
            AnimateWindow(g_hMainWnd, CMS_FADE, AW_BLEND | AW_HIDE);
        }
   } else {
       ShowWindow(g_hMainWnd, nShowCmd);
   }
#endif

   ShowWindow(g_hMainWnd, nShowCmd);
}

inline void MyDestroyWindow(HWND hwnd)
{
//    MyShowWindow(hwnd, SW_HIDE);
    DestroyWindow(hwnd);
}

/*++ InitDavesControls

Routine Description:

   Superclasses GroupBox for better drawing
 
   Note that I'm not very concerned about failure here, since it
   something goes wrong the dialog creation will fail awayway, and
   it will be handled there
    
Arguments:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void InitDavesControls()
{
    static const TCHAR szControlName[] = TEXT("DavesFrameClass");

    WNDCLASS wndclass;

    // 
    // Get the class info for the Button class (which is what group
    // boxes really are) and create a new class based on it
    //

    GetClassInfo(g_hInstance, TEXT("Button"), &wndclass);
    oldButtonWndProc = wndclass.lpfnWndProc;
    
    wndclass.hInstance = g_hInstance;
    wndclass.lpfnWndProc = DavesFrameWndProc;
    wndclass.lpszClassName = szControlName;

    ATOM atom = RegisterClass(&wndclass);

    return;
}

/*++ SetTitle

Routine Description:

    Sets the app's title in the title bar (we do this on startup and
    when coming out of notitle mode).
    
Arguments:
    
    none

Return Value:

    none

Revision History:

    Jan-24-95 Davepl  Created

--*/

void SetTitle()
{
    TCHAR szTitle[MAX_PATH];
    LoadString(g_hInstance, IDS_APPTITLE, szTitle, MAX_PATH);
    SetWindowText(g_hMainWnd, szTitle);
}

/*++ UpdateMenuStates

Routine Description:

    Updates the menu checks / ghosting based on the
    current settings and options
    
Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void UpdateMenuStates()
{
    HMENU hMenu = GetMenu(g_hMainWnd);
    if (hMenu)
    {
        CheckMenuRadioItem(hMenu, VM_FIRST, VM_LAST, VM_FIRST + (UINT) g_Options.m_vmViewMode, MF_BYCOMMAND);
        CheckMenuRadioItem(hMenu, CM_FIRST, CM_LAST, CM_FIRST + (UINT) g_Options.m_cmHistMode, MF_BYCOMMAND);
        CheckMenuRadioItem(hMenu, US_FIRST, US_LAST, US_FIRST + (UINT) g_Options.m_usUpdateSpeed, MF_BYCOMMAND);
    }

    // REVIEW (davepl) could be table driven

    
    CheckMenuItem(hMenu, IDM_ALWAYSONTOP,       MF_BYCOMMAND | (g_Options.m_fAlwaysOnTop   ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, IDM_MINIMIZEONUSE,     MF_BYCOMMAND | (g_Options.m_fMinimizeOnUse ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, IDM_KERNELTIMES,       MF_BYCOMMAND | (g_Options.m_fKernelTimes   ? MF_CHECKED : MF_UNCHECKED));    
    CheckMenuItem(hMenu, IDM_NOTITLE,           MF_BYCOMMAND | (g_Options.m_fNoTitle       ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, IDM_HIDEWHENMIN,       MF_BYCOMMAND | (g_Options.m_fHideWhenMin   ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, IDM_SHOW16BIT,         MF_BYCOMMAND | (g_Options.m_fShow16Bit     ? MF_CHECKED : MF_UNCHECKED));
#ifdef LATER    // Put back when Sandbox becomes official
    CheckMenuItem(hMenu, IDM_SHOWSANDBOXNAMES,  MF_BYCOMMAND | (g_Options.m_fShowSandboxNames     ? MF_CHECKED : MF_UNCHECKED));
#endif

    // Remove the CPU history style options on single processor machines

    if (g_cProcessors < 2)
    {
        DeleteMenu(hMenu, IDM_ALLCPUS, MF_BYCOMMAND);
    }
}

/*++ SizeChildPage

Routine Description:

    Size the active child page based on the tab control
    
Arguments:

    hwndMain    - Main window

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void SizeChildPage(HWND hwndMain)
{
    if (g_Options.m_iCurrentPage >= 0)
    {
        // If we are in maximum viewing mode, the page gets the whole
        // window area
        
        HWND hwndPage = g_pPages[g_Options.m_iCurrentPage]->GetPageWindow();

        DWORD dwStyle = GetWindowLong (g_hMainWnd, GWL_STYLE);
    
        if (g_Options.m_fNoTitle)
        {
            RECT rcMainWnd;
            GetClientRect(g_hMainWnd, &rcMainWnd);
            SetWindowPos(hwndPage, HWND_TOP, rcMainWnd.left, rcMainWnd.top,
                    rcMainWnd.right - rcMainWnd.left, 
                    rcMainWnd.bottom - rcMainWnd.top, SWP_NOZORDER | SWP_NOACTIVATE);
    
            // remove caption & menu bar, etc.

            dwStyle &= ~(WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            // SetWindowLong (g_hMainWnd, GWL_ID, 0);            
            SetWindowLong (g_hMainWnd, GWL_STYLE, dwStyle);
            SetMenu(g_hMainWnd, NULL);

        }
        else
        {                                
            // If we have a page being displayed, we need to size it also
            // put menu bar & caption back in 

            dwStyle = WS_TILEDWINDOW | dwStyle;
            SetWindowLong (g_hMainWnd, GWL_STYLE, dwStyle);

            if (g_hMenu)
            {
                SetMenu(g_hMainWnd, g_hMenu);
                UpdateMenuStates();
            }

            SetTitle();
              
            if (hwndPage)
            {
                RECT rcCtl;
                HWND hwndCtl = GetDlgItem(hwndMain, IDC_TABS);
                GetClientRect(hwndCtl, &rcCtl);

                MapWindowPoints(hwndCtl, hwndMain, (LPPOINT)&rcCtl, 2);
                TabCtrl_AdjustRect(hwndCtl, FALSE, &rcCtl);

                SetWindowPos(hwndPage, HWND_TOP, rcCtl.left, rcCtl.top,
                        rcCtl.right - rcCtl.left, rcCtl.bottom - rcCtl.top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }
}

/*++ UpdateStatusBar

Routine Description:

    Draws the status bar with test based on data accumulated by all of
    the various pages (basically a summary of most important info)
    
Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void UpdateStatusBar()
{
    //
    // If we're in menu-tracking mode (sticking help text in the stat
    // bar), we don't draw our standard text
    //

    if (FALSE == g_fMenuTracking)
    {
        TCHAR szText[MAX_PATH];
    
        wsprintf(szText, g_szfmtProcs, g_cProcesses);
        SendMessage(g_hStatusWnd, SB_SETTEXT, 0, (LPARAM) szText);

        wsprintf(szText, g_szfmtCPU, g_CPUUsage);
        SendMessage(g_hStatusWnd, SB_SETTEXT, 1, (LPARAM) szText);

        wsprintf(szText, g_szfmtMEM, g_MEMUsage, g_MEMMax);
        SendMessage(g_hStatusWnd, SB_SETTEXT, 2, (LPARAM) szText);
    }
}

/*++ MainWnd_OnTimer

Routine Description:

    Called when the refresh timer fires, we pass a timer event on to
    each of the child pages.  

Arguments:

    hwnd    - window timer was received at
    id      - id of timer that was received

Return Value:

Revision History:

      Nov-30-95 Davepl  Created

--*/

void MainWnd_OnTimer(HWND hwnd, UINT id)
{

    if (GetForegroundWindow() == hwnd && GetAsyncKeyState(VK_CONTROL) < 0)
    {
        // CTRL alone means pause

        return;
    }

    // Notify each of the pages in turn that they need to updatre

    for (int i = 0; i < ARRAYSIZE(g_pPages); i++)
    {
        g_pPages[i]->TimerEvent();
    }

    // Update the tray icon

    UINT iIconIndex = (g_CPUUsage * g_cTrayIcons) / 100;
    if (iIconIndex >= g_cTrayIcons)
    {
        iIconIndex = g_cTrayIcons - 1;      // Handle 100% case
    }

    TCHAR szTipText[MAX_PATH];
    wsprintf(szTipText, g_szfmtCPU, g_CPUUsage);

    CTrayNotification * pNot = new CTrayNotification(hwnd, 
                                                     PWM_TRAYICON, 
                                                     NIM_MODIFY, 
                                                     g_aTrayIcons[iIconIndex], 
                                                     szTipText);
    if (pNot)
    {
        if (FALSE == DeliverTrayNotification(pNot))
        {
            delete pNot;
        }
    }

    UpdateStatusBar();
}

/*++ MainWnd_OnInitDialog

Routine Description:

    Processes WM_INITDIALOG for the main window (a modeless dialog)
    
Revision History:

      Nov-29-95 Davepl  Created

--*/

BOOL MainWnd_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    RECT rcMain;
    GetWindowRect(hwnd, &rcMain);

    g_minWidth  = rcMain.right - rcMain.left;
    g_minHeight = rcMain.bottom - rcMain.top;

    g_DefSpacing   = (DEFSPACING_BASE   * LOWORD(GetDialogBaseUnits())) / DLG_SCALE_X;
    g_InnerSpacing = (INNERSPACING_BASE * LOWORD(GetDialogBaseUnits())) / DLG_SCALE_X; 
    g_TopSpacing   = (TOPSPACING_BASE   * HIWORD(GetDialogBaseUnits())) / DLG_SCALE_Y;

    // Load the user's defaults

    g_Options.Load();

    //
    // On init, save away the window handle for all to see
    //

    g_hMainWnd = hwnd;
    g_hMainDesktop = GetThreadDesktop(GetCurrentThreadId());

    // init some globals

    g_cxEdge = GetSystemMetrics(SM_CXEDGE);
    g_hrgnView = CreateRectRgn(0, 0, 0, 0);
    g_hrgnClip = CreateRectRgn(0, 0, 0, 0);
    g_hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    // If we're supposed to be TOPMOST, start out that way

    if (g_Options.m_fAlwaysOnTop)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
    }

    //        
    // Create the status window
    //

    g_hStatusWnd = CreateStatusWindow(WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBARS_SIZEGRIP,
                                      NULL,
                                      hwnd,
                                      IDC_STATUSWND);
    if (NULL == g_hStatusWnd)
    {
        return FALSE;
    }

    //
    // Base the panes in the status bar off of the LOGPIXELSX system metric
    //

    HDC hdc = GetDC(NULL);
    INT nInch = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    int ciParts[] = {             nInch, 
                     ciParts[0] + (nInch * 5) / 4, 
                     ciParts[1] + (nInch * 5) / 2, 
                     -1};

    if (g_hStatusWnd) 
    {
        SendMessage(g_hStatusWnd, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts);
    }

    //
    // Load our app icon
    //

    HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
    if (hIcon)
    {
        SendMessage(hwnd, WM_SETICON, TRUE, LPARAM(hIcon));
    }

    // Add our tray icon

    CTrayNotification * pNot = new CTrayNotification(hwnd, 
                                                     PWM_TRAYICON, 
                                                     NIM_ADD, 
                                                     g_aTrayIcons[0], 
                                                     NULL);
    if (pNot)
    {
        if (FALSE == DeliverTrayNotification(pNot))
        {
            delete pNot;
        }
    }

    //
    // Turn on TOPMOST for the status bar so it doesn't slide under the
    // tab control
    //

    SetWindowPos(g_hStatusWnd,
                 HWND_TOPMOST,
                 0,0,0,0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);

    //
    // Intialize each of the the pages in turn
    //

    HWND hwndTabs = GetDlgItem(hwnd, IDC_TABS);

    for (int i = 0; i < ARRAYSIZE(g_pPages); i++)
    {
        HRESULT hr;
                
        hr = g_pPages[i]->Initialize(hwndTabs);

        if (SUCCEEDED(hr))
        {
            //
            // Get the title of the new page, and use it as the title of
            // the page which we insert into the tab control
            //

            TCHAR szTitle[MAX_PATH];
            
            g_pPages[i]->GetTitle(szTitle, ARRAYSIZE(szTitle));

            TC_ITEM tcitem =
            {
                TCIF_TEXT,          // value specifying which members to retrieve or set 
                NULL,               // reserved; do not use 
                NULL,               // reserved; do not use 
                szTitle,            // pointer to string containing tab text 
                ARRAYSIZE(szTitle), // size of buffer pointed to by the pszText member 
                0,                  // index to tab control's image 
                NULL                // application-defined data associated with tab 
            };

            if (- 1 == TabCtrl_InsertItem(hwndTabs, i, &tcitem))
            {
                hr = E_FAIL;
            }
        }

        //
        // If a page failed to init, destroy the pages we have created up to this point
        // We don't remove the tabs, on the assumption that the app is going away anyway
        //

        if (FAILED(hr))
        {
            for (int j = i; j >= 0; j--)
            {
                g_pPages[i]->Destroy();
            }
            return FALSE;
        }
        else
        {
            
        }
    }

    //
    // Set the inital menu states
    //

    UpdateMenuStates();

    //
    // Activate a page (pick page 0 if no preference is set)
    //

    if (g_Options.m_iCurrentPage < 0)
    {
        g_Options.m_iCurrentPage = 0;
    }
    
    TabCtrl_SetCurSel(GetDlgItem(g_hMainWnd, IDC_TABS), g_Options.m_iCurrentPage);
    
    g_pPages[g_Options.m_iCurrentPage]->Activate();

    RECT rcMainClient;
    GetClientRect(hwnd, &rcMainClient);
    MainWnd_OnSize(g_hMainWnd, 0, rcMainClient.right - rcMainClient.left, rcMainClient.bottom - rcMainClient.top);

    //
    // Create the update timer
    //

    if (g_Options.m_dwTimerInterval)        // 0 == paused
    {
        SetTimer(g_hMainWnd, 0, g_Options.m_dwTimerInterval, NULL);
    }
    
    // Force at least one intial update so that we don't need to wait
    // for the first timed update to come through

    MainWnd_OnTimer(g_hMainWnd, 0);

    //
    // Disable the MP-specific menu items
    //

    if (g_cProcessors <= 1)
    {
        HMENU hMenu = GetMenu(g_hMainWnd);
        EnableMenuItem(hMenu, IDM_MULTIGRAPH, MF_BYCOMMAND | MF_GRAYED);
    }


    return TRUE;    
}

//
// Draw an edge just below menu bar
//
void MainWnd_Draw(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP);
}

void MainWnd_OnPrintClient(HWND hwnd, HDC hdc)
{
    MainWnd_Draw(hwnd, hdc);
}

/*++ MainWnd_OnPaint

Routine Description:

    Just draws a thin edge just below the main menu bar
    
Arguments:

    hwnd    - Main window

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void MainWnd_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;

    //
    // Don't waste our time if we're minimized
    //

    if (FALSE == IsIconic(hwnd))
    {
        BeginPaint(hwnd, &ps);
    MainWnd_Draw(hwnd, ps.hdc);
        EndPaint(hwnd, &ps);
    }
    else
    {
        FORWARD_WM_PAINT(hwnd, DefWindowProc);
    }
}


/*++ MainWnd_OnMenuSelect

Routine Description:

    As the user browses menus in the app, writes help text to the
    status bar.  Also temporarily sets it to be a plain status bar
    with no panes

Arguments:

Return Value:

Revision History:

      Nov-29-95 Davepl  Created

--*/

void MainWnd_OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
    //
    // If menu is dismissed, restore the panes in the status bar, turn off the
    // global "menu tracking" flag, and redraw the status bar with normal info
    //

    if ((0xFFFF == LOWORD(flags) && NULL == hmenu) ||       // dismissed the menu
        (flags & (MF_SYSMENU | MF_SEPARATOR)))              // sysmenu or separator
    {
        SendMessage(g_hStatusWnd, SB_SIMPLE, FALSE, 0L);    // Restore sb panes
        g_fMenuTracking = FALSE;
        g_fCantHide = FALSE;
        UpdateStatusBar();
        return;
    }
    else
    {
        //
        // If its a popup, go get the submenu item that is selected instead
        //

        if (flags & MF_POPUP)
        {
            MENUITEMINFO miiSubMenu;

            miiSubMenu.cbSize = sizeof(MENUITEMINFO);
            miiSubMenu.fMask = MIIM_ID;
            miiSubMenu.cch = 0;             

            if (FALSE == GetMenuItemInfo(hmenu, item, TRUE, &miiSubMenu))
            {
                return;
            }

            //
            // Change the parameters to simulate a "normal" menu item
            //

            item = miiSubMenu.wID;
            flags &= ~MF_POPUP;
        }

        //
        // Our menus always have the same IDs as the strings that describe
        // their functions... 
        //

        TCHAR szStatusText[MAX_PATH];
        LoadString(g_hInstance, item, szStatusText, ARRAYSIZE(szStatusText));

        g_fMenuTracking = TRUE;

        SendMessage(g_hStatusWnd, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM)szStatusText);
        SendMessage(g_hStatusWnd, SB_SIMPLE, TRUE, 0L);  // Remove sb panes
        SendMessage(g_hStatusWnd, SB_SETTEXT, SBT_NOBORDERS | 0, (LPARAM) szStatusText);
    }
}

/*++ MainWnd_OnTabCtrlNotify

Routine Description:

    Handles WM_NOTIFY messages sent to the main window on behalf of the
    tab control

Arguments:

    pnmhdr - ptr to the notification block's header

Return Value:

    BOOL - depends on message

Revision History:

      Nov-29-95 Davepl  Created

--*/

BOOL MainWnd_OnTabCtrlNotify(LPNMHDR pnmhdr)
{
    HWND hwndTab = pnmhdr->hwndFrom;

    //
    // Selection is changing (new page coming to the front), so activate
    // the appropriate page
    //

    if (TCN_SELCHANGE == pnmhdr->code)
    {
        INT iTab = TabCtrl_GetCurSel(hwndTab);
        
        if (-1 != iTab)
        {
            if (-1 != g_Options.m_iCurrentPage)
            {
                g_pPages[g_Options.m_iCurrentPage]->Deactivate();
            }

            if (FAILED(g_pPages[iTab]->Activate()))
            {
                // If we weren't able to activate the new page,
                // reactivate the old page just to be sure

                if (-1 != g_Options.m_iCurrentPage)
                {
                    g_pPages[iTab]->Activate();                    
                    SizeChildPage(g_hMainWnd);
                }

            }
            else
            {
                g_Options.m_iCurrentPage = iTab;
                SizeChildPage(g_hMainWnd);
                return TRUE;
            }
        }
    }
    return FALSE;    
}

/*++ MainWnd_OnSize

Routine Description:

    Sizes the children of the main window as the size of the main
    window itself changes

Arguments:

    hwnd    - main window
    state   - window state (not used here)
    cx      - new x size
    cy      - new y size

Return Value:

    BOOL - depends on message

Revision History:

      Nov-29-95 Davepl  Created

--*/

void MainWnd_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    if (state == SIZE_MINIMIZED)
    {
        // If there's a tray, we can just hide since we have a
        // tray icon anyway.

        if (GetShellWindow() && g_Options.m_fHideWhenMin)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
    }

    //
    // Let the status bar adjust itself first, and we will work back
    // from its new position
    //

    HDWP hdwp = BeginDeferWindowPos(20);

    FORWARD_WM_SIZE(g_hStatusWnd, state, cx, cy, SendMessage);

    RECT rcStatus;
    GetClientRect(g_hStatusWnd, &rcStatus);
    MapWindowPoints(g_hStatusWnd, g_hMainWnd, (LPPOINT) &rcStatus, 2);

    //
    // Size the tab controls based on where the status bar is
    //

    HWND hwndTabs = GetDlgItem(hwnd, IDC_TABS);
    RECT rcTabs;
    GetWindowRect(hwndTabs, &rcTabs);
    MapWindowPoints(HWND_DESKTOP, g_hMainWnd, (LPPOINT) &rcTabs, 2);
    
    INT dx = cx - 2 * rcTabs.left;
    
    DeferWindowPos(hdwp, hwndTabs, NULL, 0, 0, 
                  dx, 
                  cy - (cy - rcStatus.top) - rcTabs.top * 2,
                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);

    //
    // Now size the front page and its children
    //

    if (cx || cy)               // Don't size in minimized case
        SizeChildPage(hwnd);   
}

/*++ RunDlg

Routine Description:

    Loads shell32.dll and invokes its Run dialog

Arguments:

Return Value:

Revision History:

      Nov-30-95 Davepl  Created

--*/

//
// Prototype for RunFileDlg in shell32 to which we will dynamically bind
//

typedef int (* pfn_RunFileDlg) (HWND hwndParent, 
                                HICON hIcon, 
                                LPCTSTR lpszWorkingDir, 
                                LPCTSTR lpszTitle,
                                LPCTSTR lpszPrompt, 
                                DWORD dwFlags);

DWORD RunDlg()
{
    //
    // Load shell32 and get the entry point for the RunFileDlg function
    //

#ifdef SHELL_DYNA_LINK

    HINSTANCE hShellDll = LoadLibrary(TEXT("shell32.dll"));
    if (NULL == hShellDll)
    {
        return FALSE;
    }

    pfn_RunFileDlg pRunDlg = (pfn_RunFileDlg) GetProcAddress(hShellDll, (LPCSTR) 61);
    if (NULL == pRunDlg)
    {
        FreeLibrary(hShellDll);
        return FALSE;
    }
#endif

    //
    // Put up the RUN dialog for the user (its always a UNICODE
    // entry point, so use WCHAR)
    //

    HICON hIcon = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
    if (hIcon)
    {
        WCHAR szCurDir[MAX_PATH];
        WCHAR szTitle [MAX_PATH];
        WCHAR szPrompt[MAX_PATH];

        LoadStringW(g_hInstance, IDS_RUNTITLE, szTitle, MAX_PATH);
        LoadStringW(g_hInstance, IDS_RUNTEXT, szPrompt, MAX_PATH);
        GetCurrentDirectoryW(MAX_PATH, szCurDir);

#ifdef SHELL_DYNA_LINK

        pRunDlg(g_hMainWnd, hIcon, (LPTSTR) szCurDir, 
                                (LPTSTR) szTitle, 
                                (LPTSTR) szPrompt, RFD_USEFULLPATHDIR | RFD_WOW_APP);

#else

        RunFileDlg(g_hMainWnd, hIcon, (LPTSTR) szCurDir, 
                                (LPTSTR) szTitle, 
                                (LPTSTR) szPrompt, RFD_USEFULLPATHDIR | RFD_WOW_APP);


#endif

        DestroyIcon(hIcon);
    }

#ifdef SHELL_DYNA_LINK
    FreeLibrary(hShellDll);
#endif

    return TRUE;
}


/*++ MainWnd_OnCommand

Routine Description:

    Processes WM_COMMAND messages received at the main window

Revision History:

      Nov-30-95 Davepl  Created

--*/

void MainWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDM_HIDE:
        {
            ShowWindow(hwnd, SW_MINIMIZE);
            break;
        }

        case IDM_HELP:
        {
        HtmlHelpA(GetDesktopWindow(), "taskmgr.chm", HH_DISPLAY_TOPIC, 0);
            break;
        }

        case IDCANCEL:
        case IDM_EXIT:
        {
            MyDestroyWindow(hwnd);
            break;
        }

        case IDM_RESTORETASKMAN:
        {
            ShowRunningInstance();
            break;
        }

        case IDC_NEXTTAB:
        case IDC_PREVTAB:
        {
            INT iPage = g_Options.m_iCurrentPage;
            iPage += (id == IDC_NEXTTAB) ? 1 : -1;

            iPage = iPage < 0 ? NUM_PAGES - 1 : iPage;
            iPage = iPage >= NUM_PAGES ? 0 : iPage;

            // Activate the new page.  If it fails, revert to the current

            TabCtrl_SetCurSel(GetDlgItem(g_hMainWnd, IDC_TABS), iPage);

            // SetCurSel doesn't do the page change (that would make too much
            // sense), so we have to fake up a TCN_SELCHANGE notification

            NMHDR nmhdr;
            nmhdr.hwndFrom = GetDlgItem(g_hMainWnd, IDC_TABS);
            nmhdr.idFrom   = IDC_TABS;
            nmhdr.code     = TCN_SELCHANGE;

            if (MainWnd_OnTabCtrlNotify(&nmhdr))
            {
                g_Options.m_iCurrentPage = iPage;
            }

            break;
        }

        case IDM_ALWAYSONTOP:
        {
            g_Options.m_fAlwaysOnTop = !g_Options.m_fAlwaysOnTop;
            SetWindowPos(hwnd, g_Options.m_fAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 
                            0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
            UpdateMenuStates();

            break;
        }

        case IDM_HIDEWHENMIN:
        {
            g_Options.m_fHideWhenMin = !g_Options.m_fHideWhenMin;
            UpdateMenuStates();
            break;
        }

        case IDM_MINIMIZEONUSE:
        {
            g_Options.m_fMinimizeOnUse = !g_Options.m_fMinimizeOnUse;
            UpdateMenuStates();
            break;
        }

        case IDM_NOTITLE:
        {
            g_Options.m_fNoTitle = !g_Options.m_fNoTitle;
            UpdateMenuStates();
            SizeChildPage(hwnd);

            break;
        }

        case IDM_SHOW16BIT:
        {
            g_Options.m_fShow16Bit = !g_Options.m_fShow16Bit;
            UpdateMenuStates();
            if (g_pPages[PROC_PAGE])
            {
                g_pPages[PROC_PAGE]->TimerEvent();
            }
            break;
        }

#ifdef LATER        // Put back when Sandbox becomes official
    case IDM_SHOWSANDBOXNAMES:
    {
        g_Options.m_fShowSandboxNames = !g_Options.m_fShowSandboxNames;
        UpdateMenuStates();
        if (g_pPages[PROC_PAGE])
        {
        g_pPages[PROC_PAGE]->TimerEvent();
        }
        break;
    }
#endif // LATER

        case IDM_KERNELTIMES:
        {
            g_Options.m_fKernelTimes = !g_Options.m_fKernelTimes;
            UpdateMenuStates();
            if (g_pPages[PERF_PAGE])
            {
                g_pPages[PERF_PAGE]->TimerEvent();
            }
            break;
        }

        case IDM_RUN:
        {
            RunDlg();
            break;
        }

        case IDM_SMALLICONS:
        case IDM_DETAILS:
        case IDM_LARGEICONS:
        {
            g_Options.m_vmViewMode = (VIEWMODE) (id - VM_FIRST);
            UpdateMenuStates();
            if (g_pPages[TASK_PAGE])
            {
                g_pPages[TASK_PAGE]->TimerEvent();
            }
            break;
        }

        // The following few messages get deferred off to the task page

        case IDM_TASK_CASCADE:
        case IDM_TASK_MINIMIZE:
        case IDM_TASK_MAXIMIZE:
        case IDM_TASK_TILEHORZ:
        case IDM_TASK_TILEVERT:
        case IDM_TASK_BRINGTOFRONT:
        {
            SendMessage(g_pPages[TASK_PAGE]->GetPageWindow(), WM_COMMAND, id, NULL);
            break; 
        }

        case IDM_PROCCOLS:
        {
            if (g_pPages[PROC_PAGE])
            {
                ((CProcPage *) (g_pPages[PROC_PAGE]))->PickColumns();
            }
            break;
        }

        case IDM_ALLCPUS:
        case IDM_MULTIGRAPH:
        {
            g_Options.m_cmHistMode = (CPUHISTMODE) (id - CM_FIRST);
            UpdateMenuStates();
            if (g_pPages[PERF_PAGE])
            {
                ((CPerfPage *)(g_pPages[PERF_PAGE]))->UpdateGraphs();
                g_pPages[PERF_PAGE]->TimerEvent();
            }

            break;
        }

        case IDM_REFRESH:
        {
            MainWnd_OnTimer(hwnd, 0);
            break;
        }

        case IDM_HIGH:
        case IDM_NORMAL:
        case IDM_LOW:
        case IDM_PAUSED:
        {
            static const int TimerDelays[] = { 500, 2000, 4000, 0, 0xFFFFFFFF };

            g_Options.m_usUpdateSpeed = (UPDATESPEED) (id - US_FIRST);
            ASSERT(g_Options.m_usUpdateSpeed <= ARRAYSIZE(TimerDelays));

            int cTicks = TimerDelays[ (INT) g_Options.m_usUpdateSpeed ];
            g_Options.m_dwTimerInterval = cTicks;

            KillTimer(g_hMainWnd, 0);
            if (cTicks)
            {
                SetTimer(g_hMainWnd, 0, g_Options.m_dwTimerInterval, NULL);
            }

            UpdateMenuStates();
            break;
        }

        case IDM_ABOUT:
        {
            //
            // Display the "About Task Manager" dialog
            //
            
            HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
            if (hIcon)
            {
                TCHAR szTitle[MAX_PATH];
                LoadString(g_hInstance, IDS_APPTITLE, szTitle, MAX_PATH);
                ShellAbout(hwnd, szTitle, NULL, hIcon);
                DestroyIcon(hIcon);
            }
        }
            
    }
    
}

/*++ CheckParentDeferrals

Routine Description:

    Called by the child pages, each child gives the main parent the
    opportunity to handle certain messages on its behalf

Arguments:

    MSG, WPARAM, LPARAM

Return Value:

    TRUE if parent handle the message on the childs behalf

Revision History:

    Jan-24-95 Davepl  Created

--*/

BOOL CheckParentDeferrals(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_RBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_NCRBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            SendMessage(g_hMainWnd, uMsg, wParam, lParam);
            return TRUE;
        }
    
        default:
            return FALSE;
    }
}

/*++ ShowRunningInstance

Routine Description:

    Brings this running instance to the top, and out of icon state

Revision History:

    Jan-27-95 Davepl  Created

--*/

void ShowRunningInstance()
{
    OpenIcon(g_hMainWnd);
    SetForegroundWindow(g_hMainWnd);
    SetWindowPos(g_hMainWnd, g_Options.m_fAlwaysOnTop ? HWND_TOPMOST : HWND_TOP, 
                 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
}

/*++ MainWindowProc

Routine Description:

    WNDPROC for the main window

Arguments:

    Standard wndproc fare

Return Value:

Revision History:

      Nov-30-95 Davepl  Created

--*/

INT_PTR CALLBACK MainWindowProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,                   // message
                WPARAM      wParam,                 // first message parameter
                LPARAM      lParam                  // second message parameter
                )
{
    static BOOL fIsHidden = FALSE;

    // If this is a size or a move, update the position in the user's options

    if (uMsg == WM_SIZE || uMsg == WM_MOVE)
    {
        // We don't want to start recording the window pos until we've had
        // a chance to set it to the intialial position, or we'll lose the
        // user's preferences

        if (fAlreadySetPos)
            if (!IsIconic(hwnd) && !IsZoomed(hwnd))
                GetWindowRect(hwnd, &g_Options.m_rcWindow);
    }

    if (uMsg == g_msgTaskbarCreated) 
    {
        // This is done async do taskmgr doesn't hand when the shell
        // is hung
            
        CTrayNotification * pNot = new CTrayNotification(hwnd, 
                                                         PWM_TRAYICON, 
                                                         NIM_ADD, 
                                                         g_aTrayIcons[0], 
                                                         NULL);
        if (pNot)
        {
            if (FALSE == DeliverTrayNotification(pNot))
            {
                delete pNot;
            }
        }
    }


    switch(uMsg)
    {
        case WM_PAINT:
            MainWnd_OnPaint(hwnd);
            return TRUE;

        HANDLE_MSG(hwnd, WM_INITDIALOG, MainWnd_OnInitDialog);
        HANDLE_MSG(hwnd, WM_MENUSELECT, MainWnd_OnMenuSelect);
        HANDLE_MSG(hwnd, WM_SIZE,       MainWnd_OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND,    MainWnd_OnCommand);
        HANDLE_MSG(hwnd, WM_TIMER,      MainWnd_OnTimer);

        case WM_PRINTCLIENT:
            MainWnd_OnPrintClient(hwnd, (HDC)wParam);
            break;

        // Don't let the window get too small when the title and
        // menu bars are ON

        case WM_GETMINMAXINFO:
        {
            if (FALSE == g_Options.m_fNoTitle)
            {
                LPMINMAXINFO lpmmi   = (LPMINMAXINFO) lParam;
                lpmmi->ptMinTrackSize.x = g_minWidth;
                lpmmi->ptMinTrackSize.y = g_minHeight;
                return FALSE;
            }
            break;
        }
                
        // Handle notifications from out tray icon

        case PWM_TRAYICON:
        {
            Tray_Notify(hwnd, wParam, lParam);
            break;
        }

        // Someone externally is asking us to wake up and be shown

        case PWM_ACTIVATE:
        {
             ShowRunningInstance();            

             // Return PWM_ACTIVATE to the caller as just a little
             // more assurance that we really did handle this
             // message correctly.
             
             SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PWM_ACTIVATE);
             return TRUE;
        }

        case WM_INITMENU:
        {
            // Don't let the right button hide the window during
            // menu operations

            g_fCantHide = TRUE;
            break;
        }

        // If we're in always-on-top mode, the right mouse button will
        // temporarily hide us so that the user can see whats under us
            
        case WM_RBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        {

            #ifdef ENABLE_HIDING

                if (!g_fInPopup && !fIsHidden && !g_fCantHide && g_Options.m_fAlwaysOnTop)
                {
                    ShowWindow (hwnd, SW_HIDE);

                    // We take the capture so that we're guaranteed to get
                    // the right button up even if its off our window 

                    SetCapture (hwnd);
                    fIsHidden = TRUE;
                }

            #endif

            break;            
        }

        // On right button up, if we were hidden, unhide us.  We have
        // to use the correct state (min/max/restore)

        case WM_RBUTTONUP:
        case WM_NCRBUTTONUP:
        {
            #ifdef ENABLE_HIDING

                if (fIsHidden)
                {
                    ReleaseCapture();
                    if (IsIconic(hwnd))
                        ShowWindow (hwnd, SW_SHOWMINNOACTIVE);
                    else if (IsZoomed (hwnd))
                    {
                        ShowWindow (hwnd, SW_SHOWMAXIMIZED);
                        SetForegroundWindow (hwnd);
                    }
                    else
                    {
                        ShowWindow (hwnd, SW_SHOWNOACTIVATE);
                        SetForegroundWindow (hwnd);
                    }
                    fIsHidden = FALSE;
                }

            #endif

            break;
        }

        case WM_NCHITTEST:
        {
            // If we have no title/menu bar, clicking and dragging the client
            // area moves the window. To do this, return HTCAPTION.
            // Note dragging not allowed if window maximized, or if caption
            // bar is present.
            //

            wParam = DefWindowProc(hwnd, uMsg, wParam, lParam);
            if (g_Options.m_fNoTitle && (wParam == HTCLIENT) && !IsZoomed(g_hMainWnd))
            {
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, HTCAPTION);
                return TRUE;
            }
            else
            {
                return FALSE;       // Not handled
            }
        }

        case WM_NCLBUTTONDBLCLK:
        {
            // If we have no title, an NC dbl click means we should turn
            // them back on

            if (FALSE == g_Options.m_fNoTitle)
            {
                break;
            }

            // Else, fall though
        }

        case WM_LBUTTONDBLCLK:
        {
            g_Options.m_fNoTitle = ~g_Options.m_fNoTitle;

            RECT rcMainWnd;
            GetWindowRect(g_hMainWnd, &rcMainWnd);

            if (g_pPages[PERF_PAGE])
            {
                ((CPerfPage *)(g_pPages[PERF_PAGE]))->UpdateGraphs();
                g_pPages[PERF_PAGE]->TimerEvent();
            }

            // Force a WM_SIZE event so that the window checks the min size
            // when coming out of notitle mode

            MoveWindow(g_hMainWnd, 
                       rcMainWnd.left, 
                       rcMainWnd.top, 
                       rcMainWnd.right - rcMainWnd.left,
                       rcMainWnd.bottom - rcMainWnd.top,
                       TRUE);

            SizeChildPage(hwnd);

            break;
        }

        // Someone (the task page) wants us to look up a process in the 
        // process view.  Switch to that page and send it the FINDPROC
        // message

        case WM_FINDPROC:
        {
            if (-1 != TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_TABS), PROC_PAGE))
            {
                // SetCurSel doesn't do the page change (that would make too much
                // sense), so we have to fake up a TCN_SELCHANGE notification

                NMHDR nmhdr;
                nmhdr.hwndFrom = GetDlgItem(hwnd, IDC_TABS);
                nmhdr.idFrom   = IDC_TABS;
                nmhdr.code     = TCN_SELCHANGE;

                if (MainWnd_OnTabCtrlNotify(&nmhdr))
                {
                    SendMessage(g_pPages[g_Options.m_iCurrentPage]->GetPageWindow(), 
                                    WM_FINDPROC, wParam, lParam);
                }
            }
            else
            {
                MessageBeep(0);
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch(wParam)
            {
                case IDC_TABS:
                    return MainWnd_OnTabCtrlNotify((LPNMHDR) lParam);

                default:
                    break;
            }
            break;
        }

        
        case WM_ENDSESSION:
        {
            if (wParam)
            {
                MyDestroyWindow(g_hMainWnd);
            }
            break;
        }

        case WM_CLOSE:
        {
            MyDestroyWindow(g_hMainWnd);
            break;
        }

        case WM_NCDESTROY:
        {
            // Remove the tray icon

            CTrayNotification * pNot = new CTrayNotification(hwnd, 
                                                             PWM_TRAYICON, 
                                                             NIM_DELETE, 
                                                             NULL, 
                                                             NULL);
            if (pNot)
            {
                if (FALSE == DeliverTrayNotification(pNot))
                {
                    delete pNot;
                }
            }

            // If there's a tray thread, tell is to exit

            EnterCriticalSection(&g_CSTrayThread);
            if (g_idTrayThread)
            {
                PostThreadMessage(g_idTrayThread, PM_QUITTRAYTHREAD, 0, 0);
            }
            LeaveCriticalSection(&g_CSTrayThread);

            // Wait around for some period of time for the tray thread to
            // do its cleanup work.  If the wait times out, worst case we
            // orphan the tray icon.

            if (g_hTrayThread)
            {
                #define TRAY_THREAD_WAIT 3000

                WaitForSingleObject(g_hTrayThread, TRAY_THREAD_WAIT);
                CloseHandle(g_hTrayThread);
            }
            break;
        }

        case WM_SYSCOLORCHANGE:
        case WM_SETTINGCHANGE:
        {
            // pass these to the status bar
            SendMessage(g_hStatusWnd, uMsg, wParam, lParam);
            
            // also pass along to the pages
            for (int i = 0; i < ARRAYSIZE(g_pPages); i++)
            {
                if (g_pPages[i])
                {
                    SendMessage(g_pPages[i]->GetPageWindow(), uMsg, wParam, lParam);
                }
            }   

            if (uMsg == WM_SETTINGCHANGE)
            {
                // force a resizing of the main dialog
                RECT rcMainClient;
                GetClientRect(g_hMainWnd, &rcMainClient);
                MainWnd_OnSize(g_hMainWnd, 0, rcMainClient.right - rcMainClient.left, rcMainClient.bottom - rcMainClient.top);
            }
            
            break; 
        }

        case WM_DESTROY:
        {       

            // Before shutting down, deactivate the current page, then 
            // destroy all pages

            if (g_Options.m_iCurrentPage && g_pPages[g_Options.m_iCurrentPage])
            {
                g_pPages[g_Options.m_iCurrentPage]->Deactivate();
            }

            for (int i = 0; i < ARRAYSIZE(g_pPages); i++)
            {
                if (g_pPages[i])
                {
                    g_pPages[i]->Destroy();
                }
            }

            // Save the current options

            g_Options.Save();

            PostQuitMessage(0);
        }

        default:
            
            return FALSE;       // Not handled here
    }

    return FALSE;
}

/*++ LoadGlobalResources

Routine Description:

    Loads those resources that are used frequently or that are expensive to
    load a single time at program startup

Return Value:

    BOOLEAN success value

Revision History:

      Nov-30-95 Davepl  Created

--*/

static const struct
{
    LPTSTR      psz;
    size_t      len;
    UINT        id;
}
g_aStrings[] =
{
    { g_szK,          ARRAYSIZE(g_szK),          IDS_K          },
    { g_szRealtime,   ARRAYSIZE(g_szRealtime),   IDS_REALTIME   },
    { g_szNormal,     ARRAYSIZE(g_szNormal),     IDS_NORMAL     },
    { g_szLow,        ARRAYSIZE(g_szLow),        IDS_LOW        },
    { g_szHigh,       ARRAYSIZE(g_szHigh),       IDS_HIGH       },
    { g_szUnknown,    ARRAYSIZE(g_szUnknown),    IDS_UNKNOWN    },
    { g_szAboveNormal,ARRAYSIZE(g_szAboveNormal),IDS_ABOVENORMAL},
    { g_szBelowNormal,ARRAYSIZE(g_szBelowNormal),IDS_BELOWNORMAL},
    { g_szRunning,    ARRAYSIZE(g_szRunning),    IDS_RUNNING    },
    { g_szHung,       ARRAYSIZE(g_szHung),       IDS_HUNG       },
    { g_szfmtTasks,   ARRAYSIZE(g_szfmtTasks),   IDS_FMTTASKS   },
    { g_szfmtProcs,   ARRAYSIZE(g_szfmtProcs),   IDS_FMTPROCS   },
    { g_szfmtCPU,     ARRAYSIZE(g_szfmtCPU),     IDS_FMTCPU     },
    { g_szfmtMEM,     ARRAYSIZE(g_szfmtMEM),     IDS_FMTMEM     },
    { g_szfmtCPUNum,  ARRAYSIZE(g_szfmtCPUNum),  IDS_FMTCPUNUM  },
    { g_szTotalCPU,   ARRAYSIZE(g_szTotalCPU),   IDS_TOTALTIME  },
    { g_szKernelCPU,  ARRAYSIZE(g_szKernelCPU),  IDS_KERNELTIME },
    { g_szMemUsage,   ARRAYSIZE(g_szMemUsage),   IDS_MEMUSAGE   },
};

static const UINT idTrayIcons[] =
{
    IDI_TRAY0, IDI_TRAY1, IDI_TRAY2, IDI_TRAY3, IDI_TRAY4, IDI_TRAY5,
    IDI_TRAY6, IDI_TRAY7, IDI_TRAY8, IDI_TRAY9, IDI_TRAY10, IDI_TRAY11
};

HICON g_aTrayIcons[ARRAYSIZE(idTrayIcons)];
UINT  g_cTrayIcons = ARRAYSIZE(idTrayIcons);

BOOL LoadGlobalResources()
{
    // If we don't get accelerators, its not worth failing the load
    
    g_hAccel = (HACCEL) LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    Assert(g_hAccel);

    for (UINT i = 0; i < g_cTrayIcons; i++)
    {
        VERIFY( g_aTrayIcons[i] = (HICON) LoadImage(g_hInstance, 
                                                    MAKEINTRESOURCE(idTrayIcons[i]), 
                                                    IMAGE_ICON, 
                                                    0, 0, 
                                                    LR_DEFAULTCOLOR) );
    }

    for (i = 0; i < ARRAYSIZE(g_aStrings); i++)
    {
        if (FALSE == LoadString(g_hInstance, 
                                g_aStrings[i].id, 
                                g_aStrings[i].psz,
                                g_aStrings[i].len))
        {
            return FALSE;
        }
    }

    g_hbmpBack = (HBITMAP) LoadImage(g_hInstance,
                                     MAKEINTRESOURCE(IDB_BMPBACK),
                                     IMAGE_BITMAP,
                                     0, 0,
                                     LR_LOADMAP3DCOLORS);
    if (NULL == g_hbmpBack)
    {
        return FALSE;
    }

    g_hbmpForward = (HBITMAP) LoadImage(g_hInstance,
                                     MAKEINTRESOURCE(IDB_BMPFORWARD),
                                     IMAGE_BITMAP,
                                     0, 0,
                                     LR_LOADMAP3DCOLORS);
    if (NULL == g_hbmpForward)
    {
        DeleteObject(g_hbmpBack);
        g_hbmpBack = NULL;
        return FALSE;
    }

    
    return TRUE;
}

/*++ WinMain

Routine Description:

    Windows app startup.  Does basic initialization and creates the main window

Arguments:

    Standard winmain

Return Value:

    App exit code

Revision History:

      Nov-30-95 Davepl  Created

--*/


int WINAPI WinMainT(
                HINSTANCE   hInstance,          // handle to current instance
                HINSTANCE   hPrevInstance,          // handle to previous instance (n/a)
                LPTSTR      lpCmdLine,          // pointer to command line
                int         nShowCmd            // show state of window
                )
{
    g_hInstance   = hInstance;
    int retval    = TRUE;
    HKEY hKeyPolicy;
    DWORD dwType, dwData = 0, dwSize;
    int cx, cy;

    g_msgTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));

    InitializeCriticalSection(&g_CSTrayThread);

    // Try to create or grab the startup mutex.  Only in the case
    // where everything goes well and the mutex already existed AND
    // we were able to grab it do we deem ourselves to be a secondary instance

    g_hStartupMutex = CreateMutex(NULL, TRUE, cszStartupMutex);
    if (g_hStartupMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Give the other instance (the one that owns the startup mutex) 10
        // seconds to do its thing

        WaitForSingleObject(g_hStartupMutex, FINDME_TIMEOUT);
    }

        // Load Hydra's extensions

    HINSTANCE hWinstaDLL = LoadLibrary( TEXT( "winsta.dll" ) );

    if( hWinstaDLL != NULL)
    {
        gpfnWinStationGetProcessSid = ( pfnWinStationGetProcessSid )GetProcAddress(hWinstaDLL, "WinStationGetProcessSid");

        if( gpfnWinStationGetProcessSid == NULL )
        {
            dprintf(TEXT("GetProcAddress for WinStationGetProcessSid failed, Error %d\n") , GetLastError() );
        }

        gpfnWinStationTerminateProcess = ( pfnWinStationTerminateProcess )GetProcAddress(hWinstaDLL, "WinStationTerminateProcess");

        if( gpfnWinStationTerminateProcess == NULL )
        {
            dprintf(TEXT("GetProcAddress for WinStationTerminateProcess failed, Error %d\n") , GetLastError() );
        }
    }
    else
    {
        dprintf(TEXT("Cannot Load winsta.dll, Error %d\n"), GetLastError() );
    }

    HINSTANCE hUtilDll = LoadLibrary( TEXT( "utildll.dll" ) ); 

    if( hUtilDll != NULL )
    {
        gpfnCachedGetUserFromSid = ( pfnCachedGetUserFromSid )GetProcAddress(hUtilDll, "CachedGetUserFromSid");

        if( gpfnCachedGetUserFromSid == NULL )
        {
            dprintf( TEXT( "GetProcAddress for CachedGetUserFromSid failed, Error %d\n" ) , GetLastError( ) );
        }
    }
    else
    {
        dprintf( TEXT( "Cannot Load utildll.dll, Error %d\n" ) , GetLastError( ) );
    }

    
    // 
    // Locate and activate a running instance if it exists.  
    //

    TCHAR szTitle[MAX_PATH];
    if (LoadString(hInstance, IDS_APPTITLE, szTitle, ARRAYSIZE(szTitle)))
    {
        HWND hwndOld = FindWindow(WC_DIALOG, szTitle);
        if (hwndOld)
        {
            // Send the other copy of ourselves a PWM_ACTIVATE message.  If that
            // succeeds, and it returns PWM_ACTIVATE back as the return code, it's
            // up and alive and we can exit this instance.

            DWORD dwPid = 0;
            GetWindowThreadProcessId(hwndOld, &dwPid);
            AllowSetForegroundWindow(dwPid);

            ULONG_PTR dwResult;
            if (SendMessageTimeout(hwndOld, 
                                   PWM_ACTIVATE, 
                                   0, 0, 
                                   SMTO_ABORTIFHUNG, 
                                   FINDME_TIMEOUT, 
                                   &dwResult))
            {
                if (dwResult == PWM_ACTIVATE)
                {
                    goto cleanup;
                }
            }
        }
    }


    if (RegOpenKeyEx (HKEY_CURRENT_USER,
                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                      0, KEY_READ, &hKeyPolicy) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwData);

        RegQueryValueEx (hKeyPolicy, TEXT("DisableTaskMgr"), NULL,
                         &dwType, (LPBYTE) &dwData, &dwSize);

        RegCloseKey (hKeyPolicy);

        if (dwData)
        {
            TCHAR szTitle[25];
            TCHAR szMessage[200];

            LoadString (hInstance, IDS_TASKMGR, szTitle, ARRAYSIZE(szTitle));
            LoadString (hInstance, IDS_TASKMGRDISABLED , szMessage, ARRAYSIZE(szMessage));
            MessageBox (NULL, szMessage, szTitle, MB_OK | MB_ICONSTOP);
            retval = FALSE;
            goto cleanup;
        }
    }

    // No running instance found, so we run as normal

    InitCommonControls();

    InitDavesControls();

    // Start the worker thread.  If it fails, you just don't
    // get tray icons

    g_hTrayThread = CreateThread(NULL, 0, TrayThreadMessageLoop, NULL, 0, &g_idTrayThread);
    ASSERT(g_hTrayThread);

    // Init the page table

    g_pPages[0] = new CTaskPage;
    if (NULL == g_pPages[0])
    {
        retval = FALSE;
        goto cleanup;
    }

    g_pPages[1] = new CProcPage;
    if (NULL == g_pPages[1])
    {
        retval = FALSE;
        goto cleanup;
    }

    g_pPages[2] = new CPerfPage;
    if (NULL == g_pPages[2])
    {
        retval = FALSE;
        goto cleanup;
    }
    
    // Load whatever resources that we need available globally

    if (FALSE == LoadGlobalResources())
    {
        retval = FALSE;
        goto cleanup;
    }

    // Initialize the history buffers

    if (0 == InitPerfInfo())
    {
        retval = FALSE;
        goto cleanup;
    }

    // Create the main window (it's a modeless dialog, to be precise)

    g_hMainWnd = CreateDialog(hInstance,                  
                                 MAKEINTRESOURCE(IDD_MAINWND),  
                                 NULL,
                                 MainWindowProc);

    if (NULL == g_hMainWnd)
    {
        retval = FALSE;
        goto cleanup;
    }
    else
    {
        fAlreadySetPos = TRUE;

        cx = g_Options.m_rcWindow.right-g_Options.m_rcWindow.left;
        cy = g_Options.m_rcWindow.bottom-g_Options.m_rcWindow.top;

        SetWindowPos(g_hMainWnd, NULL, 
                         g_Options.m_rcWindow.left,
                         g_Options.m_rcWindow.top,
                         cx,
                         cy,
                         SWP_NOZORDER);

        MyShowWindow(g_hMainWnd, nShowCmd);
    }

    // We're out of the "starting up" phase so release the startup mutex

    if (g_hStartupMutex)
    {
        ReleaseMutex(g_hStartupMutex);
        CloseHandle(g_hStartupMutex);
        g_hStartupMutex = NULL;
    }

    // If we're the one, true, task manager, we can hang around till the
    // bitter end in case the user has problems during shutdown

    SetProcessShutdownParameters(1, SHUTDOWN_NORETRY);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // Give the page a crack at the accelerator
        
        HWND hwndPage = NULL;
        
        if (g_Options.m_iCurrentPage >= 0) 
        {
            g_pPages[g_Options.m_iCurrentPage]->GetPageWindow();
        }

        BOOL bHandled = FALSE;
        
        bHandled = TranslateAccelerator(g_hMainWnd, g_hAccel, &msg);

        if (FALSE == bHandled)
        {
            if (hwndPage)
            {
                bHandled = TranslateAccelerator(hwndPage, g_hAccel, &msg);
            }

            if (FALSE == bHandled && FALSE == IsDialogMessage(g_hMainWnd, &msg))
            {
                TranslateMessage(&msg);          // Translates virtual key codes 
                DispatchMessage(&msg);           // Dispatches message to window 
            }
        }
    }

cleanup:

    // We're no longer "starting up"

    if (g_hStartupMutex)
    {
        ReleaseMutex(g_hStartupMutex);
        CloseHandle(g_hStartupMutex);
        g_hStartupMutex = NULL;
    }

    // Yes, I could use virtual destructors, but I could also poke
    // myself in the eye with a sharp stick.  Either way you wouldn't
    // be able to see what's going on.

    if (g_pPages[TASK_PAGE])
        delete (CTaskPage *) g_pPages[TASK_PAGE];

    if (g_pPages[PROC_PAGE])
        delete (CProcPage *) g_pPages[PROC_PAGE];

    if (g_pPages[PERF_PAGE])
        delete (CPerfPage *) g_pPages[PERF_PAGE];

    ReleasePerfInfo();

    if( hWinstaDLL != NULL )
    {
        FreeLibrary( hWinstaDLL );
    }

    if( hUtilDll != NULL )
    {
        FreeLibrary( hUtilDll );
    }

    return (retval);
}

//
// And now the magic begins.  The normal C++ CRT code walks a set of vectors
// and calls through them to perform global initializations.  Those vectors
// are always in data segments with a particular naming scheme.  By delcaring
// the variables below, I can determine where in my code they get stuck, and
// then call them myself
//

typedef void (__cdecl *_PVFV)(void);

#pragma data_seg(".CRT$XIA")
_PVFV __xi_a[] = { NULL };

#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] = { NULL };

#pragma data_seg(".CRT$XCA")                                 
_PVFV __xc_a[] = { NULL };

#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = { NULL };

#pragma data_seg(".data")

/*++ _initterm

Routine Description:

    Walk the table of function pointers from the bottom up, until
    the end is encountered.  Do not skip the first entry.  The initial
    value of pfbegin points to the first valid entry.  Do not try to
    execute what pfend points to.  Only entries before pfend are valid.

Arguments:

    pfbegin - first pointer
    pfend   - last pointer

Revision History:

      Nov-30-95 Davepl  Created

--*/

static void __cdecl _initterm ( _PVFV * pfbegin, _PVFV * pfend )
{
        while ( pfbegin < pfend )
        {
            
             // if current table entry is non-NULL, call thru it.
             
            if ( *pfbegin != NULL )
            {
                (**pfbegin)();
            }
            ++pfbegin;
        }
}

/*++ WinMain

Routine Description:

    Windows app startup.  Does basic initialization and creates the main window

Arguments:

    Standard winmain

Return Value:

    App exit code

Revision History:

      Nov-30-95 Davepl  Created

--*/

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    //
    // Do runtime startup initializers.
    //

    _initterm( __xi_a, __xi_z );
    
    //
    // do C++ constructors (initializers) specific to this EXE
    //

    _initterm( __xc_a, __xc_z );

    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    g_bMirroredOS = IS_MIRRORING_ENABLED();
    
    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}

// DisplayFailureMsg
//
// Displays a generic error message based on the error code
// and message box title provided

void DisplayFailureMsg(HWND hWnd, UINT idTitle, DWORD dwError)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[MAX_PATH * 2];
    TCHAR szError[MAX_PATH];

    if (0 == LoadString(g_hInstance, idTitle, szTitle, ARRAYSIZE(szTitle)))
    {
        return;
    }

    if (0 == LoadString(g_hInstance, IDS_GENFAILURE, szMsg, ARRAYSIZE(szMsg)))
    {
        return;
    }

    
    // "incorrect paramter" doesn't make a lot of sense for the user, so
    // massage it to be "Operation not allowed on this process".
                                                                             
    if (dwError == ERROR_INVALID_PARAMETER)
    {
        if (0 == LoadString(g_hInstance, IDS_BADPROC, szError, ARRAYSIZE(szError)))
        {
            return;
        }
    }
    else if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                dwError,
                                LANG_USER_DEFAULT,
                                szError,
                                ARRAYSIZE(szError),
                                NULL))
    {
        return;
    }

    lstrcat(szMsg, szError);

    MessageBox(hWnd, szMsg, szTitle, MB_OK | MB_ICONERROR);
}

/*++ NewDeskDlgProc

Routine Description:

    Dialog proc for the NewDesktop dialog
    
Arguments:

Return Value:

    BOOL, TRUE == success

Revision History:

      Nov-29-95 Davepl  Created

--*/

typedef struct _NewDesktopInfo
{
    TCHAR   m_szName[MAX_PATH];
    BOOL    m_fStartExplorer;
} NewDesktopInfo;

BOOL CALLBACK NewDeskDlgProc(HWND  hwndDlg, UINT  uMsg, WPARAM  wParam, LPARAM lParam)
{
    NewDesktopInfo * pndi = (NewDesktopInfo *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pndi = (NewDesktopInfo *) lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
      
            CheckDlgButton(hwndDlg, IDC_STARTEXPLORER, pndi->m_fStartExplorer);
            SetWindowText(GetDlgItem(hwndDlg, IDC_DESKTOPNAME), pndi->m_szName);
            
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                    break;
                }

                case IDOK:
                {
                    pndi->m_fStartExplorer = IsDlgButtonChecked(hwndDlg, IDC_STARTEXPLORER);
                    GetWindowText(GetDlgItem(hwndDlg, IDC_DESKTOPNAME), pndi->m_szName, ARRAYSIZE(pndi->m_szName));
                    EndDialog(hwndDlg, IDOK);
                    break;
                }

                default:
                    break;
            }
        }
            
        default:
        {
            return FALSE;
        }
    }
}


#ifdef DESKTOP_SUPPORT

/*++ CreateNewDesktop

Routine Description:

    Prompts the user for options and starts a new desktop
    
Arguments:

Return Value:

    BOOL, TRUE == success

Revision History:

      Nov-29-95 Davepl  Created

--*/

BOOL CreateNewDesktop()
{
    NewDesktopInfo ndi;
    ndi.m_fStartExplorer = TRUE;

TryAgain:

    //
    // Look for a desktop name that's not in use yet
    //

    for (int i = 0; ; i++)
    {
        wsprintf( ndi.m_szName, TEXT("Desktop%d"), i );
        HDESK hdesk = OpenDesktop( ndi.m_szName, 0, FALSE, DESKTOP_SWITCHDESKTOP );
        if (hdesk)
        {
            CloseDesktop(hdesk);
        }
        else
        {
            break;
        }
    }

    //
    // Allow the user to modify the options
    //

    int iRet = DialogBoxParam(g_hInstance, 
                              MAKEINTRESOURCE(IDD_CREATEDESKTOP), 
                              g_hMainWnd, 
                              NewDeskDlgProc,
                              (LPARAM) &ndi);
    if (iRet != IDOK)
    {
        return FALSE;
    }

    // 
    // Create the new desktop
    //

    HDESK hdesk = CreateDesktop( ndi.m_szName, NULL, NULL, 0, MAXIMUM_ALLOWED, NULL );
    if (NULL == hdesk)
    {
        DWORD dwError = GetLastError();
        DisplayFailureMsg(g_hMainWnd, IDS_CANTCREATEDESKTOP, dwError);
        goto TryAgain;
    }
    
    return S_OK;
}

#endif //DESKTOP_SUPPORT


/*++ LoadPopupMenu

Routine Description:

    Loads a popup menu from a resource.  Needed because USER
    does not support popup menus (yes, really)
    
Arguments:

    hinst       - module instance to look for resource in
    id          - resource id of popup menu

Return Value:

Revision History:

      Nov-22-95 Davepl  Created

--*/

HMENU LoadPopupMenu(HINSTANCE hinst, UINT id)
{
    HMENU hmenuParent = LoadMenu(hinst, MAKEINTRESOURCE(id));

    if (hmenuParent) 
    {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }

    return NULL;
}


/*++ SubclassListView, ListViewWndProc, LV_GetViewRect

Routine Description:

    Routines for flicker-free painting of listview controls.
    
Revision History:

      Dec-23-96 vadimg  Created

--*/

typedef LRESULT (CALLBACK *ListView)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
ListView gfnListView;

void inline _SetRectRgnIndirect(HRGN hrgn, LPRECT prc)
{
    SetRectRgn(hrgn, prc->left, prc->top, prc->right, prc->bottom);
}

void LV_GetViewRgn(HWND hwnd)
{
    RECT rcItem;
    int nCount, i;
    
    SetRectRgn(g_hrgnView, 0, 0, 0, 0);
    nCount = ListView_GetItemCount(hwnd);

    for (i = 0; i < nCount; i++) {
        // exclude the selected item, listview does not erase it
        if (ListView_GetItemState(hwnd, i, LVIS_SELECTED))
            continue;

        ListView_GetItemRect(hwnd, i, &rcItem, LVIR_BOUNDS);
        
        // listview leaves off SM_CXEDGE when painting first column
        rcItem.left += g_cxEdge;

        _SetRectRgnIndirect(g_hrgnClip, &rcItem);
        CombineRgn(g_hrgnView, g_hrgnView, g_hrgnClip, RGN_OR);
    }
}

LRESULT CALLBACK ListViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_SYSCOLORCHANGE:
        if (g_hbrWindow)
            DeleteObject(g_hbrWindow);
        g_hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_ERASEBKGND:
        {
            // To achieve the flicker-free effect only erase the area not
            // taken up by the items. The items will be filled by listview.

            RECT rcAll;
            HDC hdc = (HDC)wParam;
            
            LV_GetViewRgn(hwnd);
            
            GetClientRect(hwnd, &rcAll);
            _SetRectRgnIndirect(g_hrgnClip, &rcAll);
            
            CombineRgn(g_hrgnClip, g_hrgnClip, g_hrgnView, RGN_DIFF);
            FillRgn(hdc, g_hrgnClip, g_hbrWindow);
        }
        return TRUE;
    }
    
    return CallWindowProc(gfnListView, hwnd, msg, wParam, lParam);
}

void SubclassListView(HWND hwnd)
{
    gfnListView = (ListView)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ListViewWndProc);
}
