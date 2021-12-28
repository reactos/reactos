/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 * Copyright 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * this library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * this library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"
#include <commoncontrols.h>

HRESULT TrayWindowCtxMenuCreator(ITrayWindow * TrayWnd, IN HWND hWndOwner, IContextMenu ** ppCtxMenu);
LRESULT appbar_message(COPYDATASTRUCT* cds);
void appbar_notify_all(HMONITOR hMon, UINT uMsg, HWND hwndExclude, LPARAM lParam);

#define WM_APP_TRAYDESTROY  (WM_APP + 0x100)

#define TIMER_ID_AUTOHIDE 1
#define TIMER_ID_MOUSETRACK 2
#define MOUSETRACK_INTERVAL 100
#define AUTOHIDE_DELAY_HIDE 2000
#define AUTOHIDE_DELAY_SHOW 50
#define AUTOHIDE_INTERVAL_ANIMATING 10

#define AUTOHIDE_SPEED_SHOW 10
#define AUTOHIDE_SPEED_HIDE 1

#define AUTOHIDE_HIDDEN 0
#define AUTOHIDE_SHOWING 1
#define AUTOHIDE_SHOWN 2
#define AUTOHIDE_HIDING 3

#define IDHK_RUN 0x1f4
#define IDHK_MINIMIZE_ALL 0x1f5
#define IDHK_RESTORE_ALL 0x1f6
#define IDHK_HELP 0x1f7
#define IDHK_EXPLORE 0x1f8
#define IDHK_FIND 0x1f9
#define IDHK_FIND_COMPUTER 0x1fa
#define IDHK_NEXT_TASK 0x1fb
#define IDHK_PREV_TASK 0x1fc
#define IDHK_SYS_PROPERTIES 0x1fd
#define IDHK_DESKTOP 0x1fe
#define IDHK_PAGER 0x1ff

static const WCHAR szTrayWndClass[] = L"Shell_TrayWnd";

struct EFFECTIVE_INFO
{
    HWND hwndFound;
    HWND hwndDesktop;
    HWND hwndProgman;
    HWND hTrayWnd;
    BOOL bMustBeInMonitor;
};

static BOOL CALLBACK
FindEffectiveProc(HWND hwnd, LPARAM lParam)
{
    EFFECTIVE_INFO *pei = (EFFECTIVE_INFO *)lParam;

    if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
        return TRUE;    // continue

    if (pei->hTrayWnd == hwnd || pei->hwndDesktop == hwnd ||
        pei->hwndProgman == hwnd)
    {
        return TRUE;    // continue
    }

    if (pei->bMustBeInMonitor)
    {
        // is the window in the nearest monitor?
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (hMon)
        {
            MONITORINFO info;
            ZeroMemory(&info, sizeof(info));
            info.cbSize = sizeof(info);
            if (GetMonitorInfoW(hMon, &info))
            {
                RECT rcWindow, rcMonitor, rcIntersect;
                rcMonitor = info.rcMonitor;

                GetWindowRect(hwnd, &rcWindow);

                if (!IntersectRect(&rcIntersect, &rcMonitor, &rcWindow))
                    return TRUE;    // continue
            }
        }
    }

    pei->hwndFound = hwnd;
    return FALSE;   // stop if found
}

static BOOL
IsThereAnyEffectiveWindow(BOOL bMustBeInMonitor)
{
    EFFECTIVE_INFO ei;
    ei.hwndFound = NULL;
    ei.hwndDesktop = GetDesktopWindow();
    ei.hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    ei.hwndProgman = FindWindowW(L"Progman", NULL);
    ei.bMustBeInMonitor = bMustBeInMonitor;

    EnumWindows(FindEffectiveProc, (LPARAM)&ei);
    if (ei.hwndFound && FALSE)
    {
        WCHAR szClass[64], szText[64];
        GetClassNameW(ei.hwndFound, szClass, _countof(szClass));
        GetWindowTextW(ei.hwndFound, szText, _countof(szText));
        MessageBoxW(NULL, szText, szClass, 0);
    }
    return ei.hwndFound != NULL;
}

CSimpleArray<HWND>  g_MinimizedAll;

/*
 * ITrayWindow
 */

const GUID IID_IShellDesktopTray = { 0x213e2df9, 0x9a14, 0x4328, { 0x99, 0xb1, 0x69, 0x61, 0xf9, 0x14, 0x3c, 0xe9 } };

class CStartButton
    : public CWindowImpl<CStartButton>
{
    HIMAGELIST m_ImageList;
    SIZE       m_Size;
    HFONT      m_Font;

public:
    CStartButton()
        : m_ImageList(NULL),
          m_Font(NULL)
    {
        m_Size.cx = 0;
        m_Size.cy = 0;
    }

    virtual ~CStartButton()
    {
        if (m_ImageList != NULL)
            ImageList_Destroy(m_ImageList);

        if (m_Font != NULL)
            DeleteObject(m_Font);
    }

    SIZE GetSize()
    {
        return m_Size;
    }

    VOID UpdateSize()
    {
        SIZE Size = { 0, 0 };

        if (m_ImageList == NULL ||
            !SendMessageW(BCM_GETIDEALSIZE, 0, (LPARAM) &Size))
        {
            Size.cx = 2 * GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CYCAPTION) * 3;
        }

        Size.cy = max(Size.cy, GetSystemMetrics(SM_CYCAPTION));

        /* Save the size of the start button */
        m_Size = Size;
    }

    VOID UpdateFont()
    {
        /* Get the system fonts, we use the caption font, always bold, though. */
        NONCLIENTMETRICS ncm = {sizeof(ncm)};
        if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            return;

        if (m_Font)
            DeleteObject(m_Font);

        ncm.lfCaptionFont.lfWeight = FW_BOLD;
        m_Font = CreateFontIndirect(&ncm.lfCaptionFont);

        SetFont(m_Font, FALSE);
    }

    VOID Initialize()
    {
        // HACK & FIXME: CORE-17505
        SubclassWindow(m_hWnd);

        SetWindowTheme(m_hWnd, L"Start", NULL);

        m_ImageList = ImageList_LoadImageW(hExplorerInstance,
                                           MAKEINTRESOURCEW(IDB_START),
                                           0, 0, 0,
                                           IMAGE_BITMAP,
                                           LR_LOADTRANSPARENT | LR_CREATEDIBSECTION);

        BUTTON_IMAGELIST bil = {m_ImageList, {1,1,1,1}, BUTTON_IMAGELIST_ALIGN_LEFT};
        SendMessageW(BCM_SETIMAGELIST, 0, (LPARAM) &bil);
        UpdateSize();
    }

    // Hack:
    // Use DECLARE_WND_SUPERCLASS instead!
    HWND Create(HWND hwndParent)
    {
        WCHAR szStartCaption[32];
        if (!LoadStringW(hExplorerInstance,
                         IDS_START,
                         szStartCaption,
                         _countof(szStartCaption)))
        {
            wcscpy(szStartCaption, L"Start");
        }

        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_LEFT | BS_VCENTER;

        m_hWnd = CreateWindowEx(
            0,
            WC_BUTTON,
            szStartCaption,
            dwStyle,
            0, 0, 0, 0,
            hwndParent,
            (HMENU) IDC_STARTBTN,
            hExplorerInstance,
            NULL);

        if (m_hWnd)
            Initialize();

        return m_hWnd;
    }

    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (uMsg == WM_KEYUP && wParam != VK_SPACE)
            return 0;

        GetParent().PostMessage(TWM_OPENSTARTMENU);
        return 0;
    }

    BEGIN_MSG_MAP(CStartButton)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    END_MSG_MAP()

};

class CTrayWindow :
    public CComCoClass<CTrayWindow>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayWindow, CWindow, CControlWinTraits >,
    public ITrayWindow,
    public IShellDesktopTray,
    public IOleWindow,
    public IContextMenu
{
    CStartButton m_StartButton;

    CComPtr<IMenuBand>  m_StartMenuBand;
    CComPtr<IMenuPopup> m_StartMenuPopup;

    CComPtr<IDeskBand> m_TaskBand;
    CComPtr<IContextMenu> m_ContextMenu;
    HTHEME m_Theme;

    HFONT m_Font;

    HWND m_DesktopWnd;
    HWND m_Rebar;
    HWND m_TaskSwitch;
    HWND m_TrayNotify;

    CComPtr<IUnknown> m_TrayNotifyInstance;

    DWORD    m_Position;
    HMONITOR m_Monitor;
    HMONITOR m_PreviousMonitor;
    DWORD    m_DraggingPosition;
    HMONITOR m_DraggingMonitor;

    RECT m_TrayRects[4];
    SIZE m_TraySize;

    HWND m_TrayPropertiesOwner;
    HWND m_RunFileDlgOwner;

    UINT m_AutoHideState;
    SIZE m_AutoHideOffset;
    TRACKMOUSEEVENT m_MouseTrackingInfo;

    HDPA m_ShellServices;

public:
    CComPtr<ITrayBandSite> m_TrayBandSite;

    union
    {
        DWORD Flags;
        struct
        {
            /* UI Status */
            DWORD InSizeMove : 1;
            DWORD IsDragging : 1;
            DWORD NewPosSize : 1;
        };
    };

public:
    CTrayWindow() :
        m_StartButton(),
        m_Theme(NULL),
        m_Font(NULL),
        m_DesktopWnd(NULL),
        m_Rebar(NULL),
        m_TaskSwitch(NULL),
        m_TrayNotify(NULL),
        m_Position(0),
        m_Monitor(NULL),
        m_PreviousMonitor(NULL),
        m_DraggingPosition(0),
        m_DraggingMonitor(NULL),
        m_TrayPropertiesOwner(NULL),
        m_RunFileDlgOwner(NULL),
        m_AutoHideState(NULL),
        m_ShellServices(NULL),
        Flags(0)
    {
        ZeroMemory(&m_TrayRects, sizeof(m_TrayRects));
        ZeroMemory(&m_TraySize, sizeof(m_TraySize));
        ZeroMemory(&m_AutoHideOffset, sizeof(m_AutoHideOffset));
        ZeroMemory(&m_MouseTrackingInfo, sizeof(m_MouseTrackingInfo));
    }

    virtual ~CTrayWindow()
    {
        if (m_ShellServices != NULL)
        {
            ShutdownShellServices(m_ShellServices);
            m_ShellServices = NULL;
        }

        if (m_Font != NULL)
        {
            DeleteObject(m_Font);
            m_Font = NULL;
        }

        if (m_Theme)
        {
            CloseThemeData(m_Theme);
            m_Theme = NULL;
        }

        PostQuitMessage(0);
    }





    /**********************************************************
     *    ##### command handling #####
     */

    HRESULT ExecResourceCmd(int id)
    {
        WCHAR szCommand[256];
        WCHAR *pszParameters;

        if (!LoadStringW(hExplorerInstance,
                         id,
                         szCommand,
                         _countof(szCommand)))
        {
            return E_FAIL;
        }

        pszParameters = wcschr(szCommand, L'>');
        if (pszParameters)
        {
            *pszParameters = 0;
            pszParameters++;
        }

        ShellExecuteW(m_hWnd, NULL, szCommand, pszParameters, NULL, SW_SHOWNORMAL);
        return S_OK;
    }

    LRESULT DoExitWindows()
    {
        /* Display the ReactOS Shutdown Dialog */
        ExitWindowsDialog(m_hWnd);

        /*
         * If the user presses CTRL+ALT+SHIFT while exiting
         * the shutdown dialog, exit the shell cleanly.
         */
        if ((GetKeyState(VK_CONTROL) & 0x8000) &&
            (GetKeyState(VK_SHIFT)   & 0x8000) &&
            (GetKeyState(VK_MENU)    & 0x8000))
        {
            PostMessage(WM_QUIT, 0, 0);
        }
        return 0;
    }

    DWORD WINAPI RunFileDlgThread()
    {
        HWND hwnd;
        RECT posRect;

        m_StartButton.GetWindowRect(&posRect);

        hwnd = CreateWindowEx(0,
                              WC_STATIC,
                              NULL,
                              WS_OVERLAPPED | WS_DISABLED | WS_CLIPSIBLINGS | WS_BORDER | SS_LEFT,
                              posRect.left,
                              posRect.top,
                              posRect.right - posRect.left,
                              posRect.bottom - posRect.top,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

        m_RunFileDlgOwner = hwnd;

        // build the default directory from two environment variables
        CStringW strDefaultDir, strHomePath;
        strDefaultDir.GetEnvironmentVariable(L"HOMEDRIVE");
        strHomePath.GetEnvironmentVariable(L"HOMEPATH");
        strDefaultDir += strHomePath;

        RunFileDlg(hwnd, NULL, (LPCWSTR)strDefaultDir, NULL, NULL, RFF_CALCDIRECTORY);

        m_RunFileDlgOwner = NULL;
        ::DestroyWindow(hwnd);

        return 0;
    }

    static DWORD WINAPI s_RunFileDlgThread(IN OUT PVOID pParam)
    {
        CTrayWindow * This = (CTrayWindow*) pParam;
        return This->RunFileDlgThread();
    }

    void DisplayRunFileDlg()
    {
        HWND hRunDlg;
        if (m_RunFileDlgOwner)
        {
            hRunDlg = ::GetLastActivePopup(m_RunFileDlgOwner);
            if (hRunDlg != NULL &&
                hRunDlg != m_RunFileDlgOwner)
            {
                SetForegroundWindow(hRunDlg);
                return;
            }
        }

        CloseHandle(CreateThread(NULL, 0, s_RunFileDlgThread, this, 0, NULL));
    }

    DWORD WINAPI TrayPropertiesThread()
    {
        HWND hwnd;
        RECT posRect;

        m_StartButton.GetWindowRect(&posRect);
        hwnd = CreateWindowEx(0,
                              WC_STATIC,
                              NULL,
                              WS_OVERLAPPED | WS_DISABLED | WS_CLIPSIBLINGS | WS_BORDER | SS_LEFT,
                              posRect.left,
                              posRect.top,
                              posRect.right - posRect.left,
                              posRect.bottom - posRect.top,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

        m_TrayPropertiesOwner = hwnd;

        DisplayTrayProperties(hwnd, m_hWnd);

        m_TrayPropertiesOwner = NULL;
        ::DestroyWindow(hwnd);

        return 0;
    }

    static DWORD WINAPI s_TrayPropertiesThread(IN OUT PVOID pParam)
    {
        CTrayWindow *This = (CTrayWindow*) pParam;

        return This->TrayPropertiesThread();
    }

    HWND STDMETHODCALLTYPE DisplayProperties()
    {
        HWND hTrayProp;

        if (m_TrayPropertiesOwner)
        {
            hTrayProp = ::GetLastActivePopup(m_TrayPropertiesOwner);
            if (hTrayProp != NULL &&
                hTrayProp != m_TrayPropertiesOwner)
            {
                SetForegroundWindow(hTrayProp);
                return NULL;
            }
        }

        CloseHandle(CreateThread(NULL, 0, s_TrayPropertiesThread, this, 0, NULL));
        return NULL;
    }

    VOID OpenCommonStartMenuDirectory(IN HWND hWndOwner, IN LPCTSTR lpOperation)
    {
        WCHAR szDir[MAX_PATH];

        if (SHGetSpecialFolderPath(hWndOwner,
            szDir,
            CSIDL_COMMON_STARTMENU,
            FALSE))
        {
            ShellExecute(hWndOwner,
                         lpOperation,
                         szDir,
                         NULL,
                         NULL,
                         SW_SHOWNORMAL);
        }
    }

    VOID OpenTaskManager(IN HWND hWndOwner)
    {
        ShellExecute(hWndOwner,
                     TEXT("open"),
                     TEXT("taskmgr.exe"),
                     NULL,
                     NULL,
                     SW_SHOWNORMAL);
    }

    VOID ToggleDesktop()
    {
        if (::IsThereAnyEffectiveWindow(TRUE))
        {
            ShowDesktop();
        }
        else
        {
            RestoreAll();
        }
    }

    BOOL STDMETHODCALLTYPE ExecContextMenuCmd(IN UINT uiCmd)
    {
        switch (uiCmd)
        {
        case ID_SHELL_CMD_PROPERTIES:
            DisplayProperties();
            break;

        case ID_SHELL_CMD_OPEN_ALL_USERS:
            OpenCommonStartMenuDirectory(m_hWnd,
                                         TEXT("open"));
            break;

        case ID_SHELL_CMD_EXPLORE_ALL_USERS:
            OpenCommonStartMenuDirectory(m_hWnd,
                                         TEXT("explore"));
            break;

        case ID_LOCKTASKBAR:
            if (SHRestricted(REST_CLASSICSHELL) == 0)
            {
                Lock(!g_TaskbarSettings.bLock);
            }
            break;

        case ID_SHELL_CMD_OPEN_TASKMGR:
            OpenTaskManager(m_hWnd);
            break;

        case ID_SHELL_CMD_UNDO_ACTION:
            break;

        case ID_SHELL_CMD_SHOW_DESKTOP:
            ShowDesktop();
            break;

        case ID_SHELL_CMD_TILE_WND_H:
            appbar_notify_all(NULL, ABN_WINDOWARRANGE, NULL, TRUE);
            TileWindows(NULL, MDITILE_HORIZONTAL, NULL, 0, NULL);
            appbar_notify_all(NULL, ABN_WINDOWARRANGE, NULL, FALSE);
            break;

        case ID_SHELL_CMD_TILE_WND_V:
            appbar_notify_all(NULL, ABN_WINDOWARRANGE, NULL, TRUE);
            TileWindows(NULL, MDITILE_VERTICAL, NULL, 0, NULL);
            appbar_notify_all(NULL, ABN_WINDOWARRANGE, NULL, FALSE);
            break;

        case ID_SHELL_CMD_CASCADE_WND:
            appbar_notify_all(NULL, ABN_WINDOWARRANGE, NULL, TRUE);
            CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, 0, NULL);
            appbar_notify_all(NULL, ABN_WINDOWARRANGE, NULL, FALSE);
            break;

        case ID_SHELL_CMD_CUST_NOTIF:
            ShowCustomizeNotifyIcons(hExplorerInstance, m_hWnd);
            break;

        case ID_SHELL_CMD_ADJUST_DAT:
            //FIXME: Use SHRunControlPanel
            ShellExecuteW(m_hWnd, NULL, L"timedate.cpl", NULL, NULL, SW_NORMAL);
            break;

        case ID_SHELL_CMD_RESTORE_ALL:
            RestoreAll();
            break;

        default:
            TRACE("ITrayWindow::ExecContextMenuCmd(%u): Unhandled Command ID!\n", uiCmd);
            return FALSE;
        }

        return TRUE;
    }

    LRESULT HandleHotKey(DWORD id)
    {
        switch (id)
        {
        case IDHK_RUN:
            DisplayRunFileDlg();
            break;
        case IDHK_HELP:
            ExecResourceCmd(IDS_HELP_COMMAND);
            break;
        case IDHK_EXPLORE:
            //FIXME: We don't support this yet:
            //ShellExecuteW(0, L"explore", NULL, NULL, NULL, 1);
            ShellExecuteW(0, NULL, L"explorer.exe", L"/e ,", NULL, 1);
            break;
        case IDHK_FIND:
            SHFindFiles(NULL, NULL);
            break;
        case IDHK_FIND_COMPUTER:
            SHFindComputer(NULL, NULL);
            break;
        case IDHK_SYS_PROPERTIES:
            //FIXME: Use SHRunControlPanel
            ShellExecuteW(m_hWnd, NULL, L"sysdm.cpl", NULL, NULL, SW_NORMAL);
            break;
        case IDHK_NEXT_TASK:
            break;
        case IDHK_PREV_TASK:
            break;
        case IDHK_MINIMIZE_ALL:
            MinimizeAll();
            break;
        case IDHK_RESTORE_ALL:
            RestoreAll();
            break;
        case IDHK_DESKTOP:
            ToggleDesktop();
            break;
        case IDHK_PAGER:
            break;
        }

        return 0;
    }

    LRESULT HandleCommand(UINT uCommand)
    {
        switch (uCommand)
        {
            case TRAYCMD_STARTMENU:
                // TODO:
                break;
            case TRAYCMD_RUN_DIALOG:
                DisplayRunFileDlg();
                break;
            case TRAYCMD_LOGOFF_DIALOG:
                LogoffWindowsDialog(m_hWnd); // FIXME: Maybe handle it in a similar way as DoExitWindows?
                break;
            case TRAYCMD_CASCADE:
                CascadeWindows(NULL, MDITILE_SKIPDISABLED, NULL, 0, NULL);
                break;
            case TRAYCMD_TILE_H:
                TileWindows(NULL, MDITILE_HORIZONTAL, NULL, 0, NULL);
                break;
            case TRAYCMD_TILE_V:
                TileWindows(NULL, MDITILE_VERTICAL, NULL, 0, NULL);
                break;
            case TRAYCMD_TOGGLE_DESKTOP:
                ToggleDesktop();
                break;
            case TRAYCMD_DATE_AND_TIME:
                ShellExecuteW(m_hWnd, NULL, L"timedate.cpl", NULL, NULL, SW_NORMAL);
                break;
            case TRAYCMD_TASKBAR_PROPERTIES:
                DisplayProperties();
                break;
            case TRAYCMD_MINIMIZE_ALL:
                MinimizeAll();
                break;
            case TRAYCMD_RESTORE_ALL:
                RestoreAll();
                break;
            case TRAYCMD_SHOW_DESKTOP:
                ShowDesktop();
                break;
            case TRAYCMD_SHOW_TASK_MGR:
                OpenTaskManager(m_hWnd);
                break;
            case TRAYCMD_CUSTOMIZE_TASKBAR:
                break;
            case TRAYCMD_LOCK_TASKBAR:
                if (SHRestricted(REST_CLASSICSHELL) == 0)
                {
                    Lock(!g_TaskbarSettings.bLock);
                }
                break;
            case TRAYCMD_HELP_AND_SUPPORT:
                ExecResourceCmd(IDS_HELP_COMMAND);
                break;
            case TRAYCMD_CONTROL_PANEL:
                // TODO:
                break;
            case TRAYCMD_SHUTDOWN_DIALOG:
                DoExitWindows();
                break;
            case TRAYCMD_PRINTERS_AND_FAXES:
                // TODO:
                break;
            case TRAYCMD_LOCK_DESKTOP:
                // TODO:
                break;
            case TRAYCMD_SWITCH_USER_DIALOG:
                // TODO:
                break;
            case IDM_SEARCH:
            case TRAYCMD_SEARCH_FILES:
                SHFindFiles(NULL, NULL);
                break;
            case TRAYCMD_SEARCH_COMPUTERS:
                SHFindComputer(NULL, NULL);
                break;

            default:
                break;
        }

        return FALSE;
    }


    UINT TrackMenu(
        IN HMENU hMenu,
        IN POINT *ppt OPTIONAL,
        IN HWND hwndExclude OPTIONAL,
        IN BOOL TrackUp,
        IN BOOL IsContextMenu)
    {
        TPMPARAMS tmp, *ptmp = NULL;
        POINT pt;
        UINT cmdId;
        UINT fuFlags;

        if (hwndExclude != NULL)
        {
            /* Get the client rectangle and map it to screen coordinates */
            if (::GetClientRect(hwndExclude,
                &tmp.rcExclude) &&
                ::MapWindowPoints(hwndExclude,
                NULL,
                (LPPOINT) &tmp.rcExclude,
                2) != 0)
            {
                ptmp = &tmp;
            }
        }

        if (ppt == NULL)
        {
            if (ptmp == NULL &&
                GetClientRect(&tmp.rcExclude) &&
                MapWindowPoints(
                NULL,
                (LPPOINT) &tmp.rcExclude,
                2) != 0)
            {
                ptmp = &tmp;
            }

            if (ptmp != NULL)
            {
                /* NOTE: TrackPopupMenuEx will eventually align the track position
                         for us, no need to take care of it here as long as the
                         coordinates are somewhere within the exclusion rectangle */
                pt.x = ptmp->rcExclude.left;
                pt.y = ptmp->rcExclude.top;
            }
            else
                pt.x = pt.y = 0;
        }
        else
            pt = *ppt;

        tmp.cbSize = sizeof(tmp);

        fuFlags = TPM_RETURNCMD | TPM_VERTICAL;
        fuFlags |= (TrackUp ? TPM_BOTTOMALIGN : TPM_TOPALIGN);
        if (IsContextMenu)
            fuFlags |= TPM_RIGHTBUTTON;
        else
            fuFlags |= (TrackUp ? TPM_VERNEGANIMATION : TPM_VERPOSANIMATION);

        cmdId = TrackPopupMenuEx(hMenu,
                                 fuFlags,
                                 pt.x,
                                 pt.y,
                                 m_hWnd,
                                 ptmp);

        return cmdId;
    }

    HRESULT TrackCtxMenu(
        IN IContextMenu * contextMenu,
        IN POINT *ppt OPTIONAL,
        IN HWND hwndExclude OPTIONAL,
        IN BOOL TrackUp,
        IN PVOID Context OPTIONAL)
    {
        POINT pt;
        TPMPARAMS params;
        RECT rc;
        HRESULT hr;
        UINT uCommand;
        HMENU popup = CreatePopupMenu();

        if (popup == NULL)
            return E_FAIL;

        if (ppt)
        {
            pt = *ppt;
        }
        else
        {
            ::GetWindowRect(m_hWnd, &rc);
            pt.x = rc.left;
            pt.y = rc.top;
        }

        TRACE("Before Query\n");
        hr = contextMenu->QueryContextMenu(popup, 0, 0, UINT_MAX, CMF_NORMAL);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            TRACE("Query failed\n");
            DestroyMenu(popup);
            return hr;
        }

        TRACE("Before Tracking\n");
        ::SetForegroundWindow(m_hWnd);
        if (hwndExclude)
        {
            ::GetWindowRect(hwndExclude, &rc);
            ZeroMemory(&params, sizeof(params));
            params.cbSize = sizeof(params);
            params.rcExclude = rc;
            uCommand = ::TrackPopupMenuEx(popup, TPM_RETURNCMD, pt.x, pt.y, m_hWnd, &params);
        }
        else
        {
            uCommand = ::TrackPopupMenuEx(popup, TPM_RETURNCMD, pt.x, pt.y, m_hWnd, NULL);
        }
        ::PostMessage(m_hWnd, WM_NULL, 0, 0);

        if (uCommand != 0)
        {
            TRACE("Before InvokeCommand\n");
            CMINVOKECOMMANDINFO cmi = { 0 };
            cmi.cbSize = sizeof(cmi);
            cmi.lpVerb = MAKEINTRESOURCEA(uCommand);
            cmi.hwnd = m_hWnd;
            hr = contextMenu->InvokeCommand(&cmi);
        }
        else
        {
            TRACE("TrackPopupMenu failed. Code=%d, LastError=%d\n", uCommand, GetLastError());
            hr = S_FALSE;
        }

        DestroyMenu(popup);
        return hr;
    }





    /**********************************************************
     *    ##### moving and sizing handling #####
     */

    void UpdateFonts()
    {
        /* There is nothing to do if themes are enabled */
        if (m_Theme)
            return;

        m_StartButton.UpdateFont();

        NONCLIENTMETRICS ncm = {sizeof(ncm)};
        if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
        {
            ERR("SPI_GETNONCLIENTMETRICS failed\n");
            return;
        }

        if (m_Font != NULL)
            DeleteObject(m_Font);

        ncm.lfCaptionFont.lfWeight = FW_NORMAL;
        m_Font = CreateFontIndirect(&ncm.lfCaptionFont);
        if (!m_Font)
        {
            ERR("CreateFontIndirect failed\n");
            return;
        }

        SendMessage(m_Rebar, WM_SETFONT, (WPARAM) m_Font, TRUE);
        SendMessage(m_TaskSwitch, WM_SETFONT, (WPARAM) m_Font, TRUE);
        SendMessage(m_TrayNotify, WM_SETFONT, (WPARAM) m_Font, TRUE);
    }

    HMONITOR GetScreenRectFromRect(
        IN OUT RECT *pRect,
        IN DWORD dwFlags)
    {
        MONITORINFO mi;
        HMONITOR hMon;

        mi.cbSize = sizeof(mi);
        hMon = MonitorFromRect(pRect, dwFlags);
        if (hMon != NULL &&
            GetMonitorInfo(hMon, &mi))
        {
            *pRect = mi.rcMonitor;
        }
        else
        {
            pRect->left = 0;
            pRect->top = 0;
            pRect->right = GetSystemMetrics(SM_CXSCREEN);
            pRect->bottom = GetSystemMetrics(SM_CYSCREEN);

            hMon = NULL;
        }

        return hMon;
    }

    HMONITOR GetMonitorFromRect(
        IN const RECT *pRect)
    {
        HMONITOR hMon;

        /* In case the monitor sizes or saved sizes differ a bit (probably
           not a lot, only so the tray window overlaps into another monitor
           now), minimize the risk that we determine a wrong monitor by
           using the center point of the tray window if we can't determine
           it using the rectangle. */
        hMon = MonitorFromRect(pRect, MONITOR_DEFAULTTONULL);
        if (hMon == NULL)
        {
            POINT pt;

            pt.x = pRect->left + ((pRect->right - pRect->left) / 2);
            pt.y = pRect->top + ((pRect->bottom - pRect->top) / 2);

            /* be less error-prone, find the nearest monitor */
            hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        }

        return hMon;
    }

    HMONITOR GetScreenRect(
        IN HMONITOR hMonitor,
        IN OUT RECT *pRect)
    {
        HMONITOR hMon = NULL;

        if (hMonitor != NULL)
        {
            MONITORINFO mi;

            mi.cbSize = sizeof(mi);
            if (!GetMonitorInfo(hMonitor, &mi))
            {
                /* Hm, the monitor is gone? Try to find a monitor where it
                   could be located now */
                hMon = GetMonitorFromRect(pRect);
                if (hMon == NULL ||
                    !GetMonitorInfo(hMon, &mi))
                {
                    hMon = NULL;
                    goto GetPrimaryRect;
                }
            }

            *pRect = mi.rcMonitor;
        }
        else
        {
GetPrimaryRect:
            pRect->left = 0;
            pRect->top = 0;
            pRect->right = GetSystemMetrics(SM_CXSCREEN);
            pRect->bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        return hMon;
    }

    VOID AdjustSizerRect(RECT *rc, DWORD pos)
    {
        int iSizerPart[4] = {TBP_SIZINGBARLEFT, TBP_SIZINGBARTOP, TBP_SIZINGBARRIGHT, TBP_SIZINGBARBOTTOM};
        SIZE size;

        if (pos > ABE_BOTTOM)
            pos = ABE_BOTTOM;

        HRESULT hr = GetThemePartSize(m_Theme, NULL, iSizerPart[pos], 0, NULL, TS_TRUE, &size);
        if (FAILED_UNEXPECTEDLY(hr))
            return;

        switch (pos)
        {
            case ABE_TOP:
                rc->bottom -= size.cy;
                break;
            case ABE_BOTTOM:
                rc->top += size.cy;
                break;
            case ABE_LEFT:
                rc->right -= size.cx;
                break;
            case ABE_RIGHT:
                rc->left += size.cx;
                break;
        }
    }

    VOID MakeTrayRectWithSize(IN DWORD Position,
                              IN const SIZE *pTraySize,
                              IN OUT RECT *pRect)
    {
        switch (Position)
        {
        case ABE_LEFT:
            pRect->right = pRect->left + pTraySize->cx;
            break;

        case ABE_TOP:
            pRect->bottom = pRect->top + pTraySize->cy;
            break;

        case ABE_RIGHT:
            pRect->left = pRect->right - pTraySize->cx;
            break;

        case ABE_BOTTOM:
        default:
            pRect->top = pRect->bottom - pTraySize->cy;
            break;
        }
    }

    VOID GetTrayRectFromScreenRect(IN DWORD Position,
                                   IN const RECT *pScreen,
                                   IN const SIZE *pTraySize OPTIONAL,
                                   OUT RECT *pRect)
    {
        if (pTraySize == NULL)
            pTraySize = &m_TraySize;

        *pRect = *pScreen;

        if(!m_Theme)
        {
            /* Move the border outside of the screen */
            InflateRect(pRect,
                        GetSystemMetrics(SM_CXEDGE),
                        GetSystemMetrics(SM_CYEDGE));
        }

        MakeTrayRectWithSize(Position, pTraySize, pRect);
    }

    BOOL IsPosHorizontal()
    {
        return m_Position == ABE_TOP || m_Position == ABE_BOTTOM;
    }

    HMONITOR CalculateValidSize(
        IN DWORD Position,
        IN OUT RECT *pRect)
    {
        RECT rcScreen;
        //BOOL Horizontal;
        HMONITOR hMon;
        SIZE szMax, szWnd;

        //Horizontal = IsPosHorizontal();

        szWnd.cx = pRect->right - pRect->left;
        szWnd.cy = pRect->bottom - pRect->top;

        rcScreen = *pRect;
        hMon = GetScreenRectFromRect(
            &rcScreen,
            MONITOR_DEFAULTTONEAREST);

        /* Calculate the maximum size of the tray window and limit the window
           size to half of the screen's size. */
        szMax.cx = (rcScreen.right - rcScreen.left) / 2;
        szMax.cy = (rcScreen.bottom - rcScreen.top) / 2;
        if (szWnd.cx > szMax.cx)
            szWnd.cx = szMax.cx;
        if (szWnd.cy > szMax.cy)
            szWnd.cy = szMax.cy;

        /* FIXME - calculate */

        GetTrayRectFromScreenRect(Position,
                                  &rcScreen,
                                  &szWnd,
                                  pRect);

        return hMon;
    }

#if 0
    VOID
        GetMinimumWindowSize(
        OUT RECT *pRect)
    {
        RECT rcMin = {0};

        AdjustWindowRectEx(&rcMin,
                           GetWindowLong(m_hWnd,
                           GWL_STYLE),
                           FALSE,
                           GetWindowLong(m_hWnd,
                           GWL_EXSTYLE));

        *pRect = rcMin;
    }
#endif


    DWORD GetDraggingRectFromPt(
        IN POINT pt,
        OUT RECT *pRect,
        OUT HMONITOR *phMonitor)
    {
        HMONITOR hMon, hMonNew;
        DWORD PosH, PosV, Pos;
        SIZE DeltaPt, ScreenOffset;
        RECT rcScreen;

        rcScreen.left = 0;
        rcScreen.top = 0;

        /* Determine the screen rectangle */
        hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
        if (hMon != NULL)
        {
            MONITORINFO mi;

            mi.cbSize = sizeof(mi);
            if (!GetMonitorInfo(hMon, &mi))
            {
                hMon = NULL;
                goto GetPrimaryScreenRect;
            }

            /* make left top corner of the screen zero based to
               make calculations easier */
            pt.x -= mi.rcMonitor.left;
            pt.y -= mi.rcMonitor.top;

            ScreenOffset.cx = mi.rcMonitor.left;
            ScreenOffset.cy = mi.rcMonitor.top;
            rcScreen.right = mi.rcMonitor.right - mi.rcMonitor.left;
            rcScreen.bottom = mi.rcMonitor.bottom - mi.rcMonitor.top;
        }
        else
        {
GetPrimaryScreenRect:
            ScreenOffset.cx = 0;
            ScreenOffset.cy = 0;
            rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
            rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        /* Calculate the nearest screen border */
        if (pt.x < rcScreen.right / 2)
        {
            DeltaPt.cx = pt.x;
            PosH = ABE_LEFT;
        }
        else
        {
            DeltaPt.cx = rcScreen.right - pt.x;
            PosH = ABE_RIGHT;
        }

        if (pt.y < rcScreen.bottom / 2)
        {
            DeltaPt.cy = pt.y;
            PosV = ABE_TOP;
        }
        else
        {
            DeltaPt.cy = rcScreen.bottom - pt.y;
            PosV = ABE_BOTTOM;
        }

        Pos = (DeltaPt.cx * rcScreen.bottom < DeltaPt.cy * rcScreen.right) ? PosH : PosV;

        /* Fix the screen origin to be relative to the primary monitor again */
        OffsetRect(&rcScreen,
                   ScreenOffset.cx,
                   ScreenOffset.cy);

        RECT rcPos = m_TrayRects[Pos];

        hMonNew = GetMonitorFromRect(&rcPos);
        if (hMon != hMonNew)
        {
            SIZE szTray;

            /* Recalculate the rectangle, we're dragging to another monitor.
               We don't need to recalculate the rect on single monitor systems. */
            szTray.cx = rcPos.right - rcPos.left;
            szTray.cy = rcPos.bottom - rcPos.top;

            GetTrayRectFromScreenRect(Pos, &rcScreen, &szTray, pRect);
            hMon = hMonNew;
        }
        else
        {
            /* The user is dragging the tray window on the same monitor. We don't need
               to recalculate the rectangle */
            *pRect = rcPos;
        }

        *phMonitor = hMon;

        return Pos;
    }

    DWORD GetDraggingRectFromRect(
        IN OUT RECT *pRect,
        OUT HMONITOR *phMonitor)
    {
        POINT pt;

        /* Calculate the center of the rectangle. We call
           GetDraggingRectFromPt to calculate a valid
           dragging rectangle */
        pt.x = pRect->left + ((pRect->right - pRect->left) / 2);
        pt.y = pRect->top + ((pRect->bottom - pRect->top) / 2);

        return GetDraggingRectFromPt(
            pt,
            pRect,
            phMonitor);
    }

    VOID ChangingWinPos(IN OUT LPWINDOWPOS pwp)
    {
        RECT rcTray;

        if (IsDragging)
        {
            rcTray.left = pwp->x;
            rcTray.top = pwp->y;
            rcTray.right = rcTray.left + pwp->cx;
            rcTray.bottom = rcTray.top + pwp->cy;

            if (!EqualRect(&rcTray,
                &m_TrayRects[m_DraggingPosition]))
            {
                /* Recalculate the rectangle, the user dragged the tray
                   window to another monitor or the window was somehow else
                   moved or resized */
                m_DraggingPosition = GetDraggingRectFromRect(
                    &rcTray,
                    &m_DraggingMonitor);
                //m_TrayRects[DraggingPosition] = rcTray;
            }

            //Monitor = CalculateValidSize(DraggingPosition,
            //                             &rcTray);

            m_Monitor = m_DraggingMonitor;
            m_Position = m_DraggingPosition;
            IsDragging = FALSE;

            m_TrayRects[m_Position] = rcTray;
            goto ChangePos;
        }
        else if (GetWindowRect(&rcTray))
        {
            if (InSizeMove)
            {
                if (!(pwp->flags & SWP_NOMOVE))
                {
                    rcTray.left = pwp->x;
                    rcTray.top = pwp->y;
                }

                if (!(pwp->flags & SWP_NOSIZE))
                {
                    rcTray.right = rcTray.left + pwp->cx;
                    rcTray.bottom = rcTray.top + pwp->cy;
                }

                m_Position = GetDraggingRectFromRect(&rcTray, &m_Monitor);

                if (!(pwp->flags & (SWP_NOMOVE | SWP_NOSIZE)))
                {
                    SIZE szWnd;

                    szWnd.cx = pwp->cx;
                    szWnd.cy = pwp->cy;

                    MakeTrayRectWithSize(m_Position, &szWnd, &rcTray);
                }

                m_TrayRects[m_Position] = rcTray;
            }
            else if (m_Position != (DWORD)-1)
            {
                /* If the user isn't resizing the tray window we need to make sure the
                   new size or position is valid. this is to prevent changes to the window
                   without user interaction. */
                rcTray = m_TrayRects[m_Position];

                if (g_TaskbarSettings.sr.AutoHide)
                {
                    rcTray.left += m_AutoHideOffset.cx;
                    rcTray.right += m_AutoHideOffset.cx;
                    rcTray.top += m_AutoHideOffset.cy;
                    rcTray.bottom += m_AutoHideOffset.cy;
                }

            }

ChangePos:
            m_TraySize.cx = rcTray.right - rcTray.left;
            m_TraySize.cy = rcTray.bottom - rcTray.top;

            pwp->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
            pwp->x = rcTray.left;
            pwp->y = rcTray.top;
            pwp->cx = m_TraySize.cx;
            pwp->cy = m_TraySize.cy;
        }
    }

    VOID ApplyClipping(IN BOOL Clip)
    {
        RECT rcClip, rcWindow;
        HRGN hClipRgn;

        if (GetWindowRect(&rcWindow))
        {
            /* Disable clipping on systems with only one monitor */
            if (GetSystemMetrics(SM_CMONITORS) <= 1)
                Clip = FALSE;

            if (Clip)
            {
                rcClip = rcWindow;

                GetScreenRect(m_Monitor, &rcClip);

                if (!IntersectRect(&rcClip, &rcClip, &rcWindow))
                {
                    rcClip = rcWindow;
                }

                OffsetRect(&rcClip,
                           -rcWindow.left,
                           -rcWindow.top);

                hClipRgn = CreateRectRgnIndirect(&rcClip);
            }
            else
                hClipRgn = NULL;

            /* Set the clipping region or make sure the window isn't clipped
               by disabling it explicitly. */
            SetWindowRgn(hClipRgn, TRUE);
        }
    }

    VOID ResizeWorkArea()
    {
#if !WIN7_DEBUG_MODE
        RECT rcTray, rcWorkArea;

        /* If monitor has changed then fix the previous monitors work area */
        if (m_PreviousMonitor != m_Monitor)
        {
            GetScreenRect(m_PreviousMonitor, &rcWorkArea);
            SystemParametersInfoW(SPI_SETWORKAREA,
                                  1,
                                  &rcWorkArea,
                                  SPIF_SENDCHANGE);
        }

        rcTray = m_TrayRects[m_Position];

        GetScreenRect(m_Monitor, &rcWorkArea);
        m_PreviousMonitor = m_Monitor;

        /* If AutoHide is false then change the workarea to exclude
           the area that the taskbar covers. */
        if (!g_TaskbarSettings.sr.AutoHide)
        {
            switch (m_Position)
            {
            case ABE_TOP:
                rcWorkArea.top = rcTray.bottom;
                break;
            case ABE_LEFT:
                rcWorkArea.left = rcTray.right;
                break;
            case ABE_RIGHT:
                rcWorkArea.right = rcTray.left;
                break;
            case ABE_BOTTOM:
                rcWorkArea.bottom = rcTray.top;
                break;
            }
        }

        /*
         * Resize the current monitor work area. Win32k will also send
         * a WM_SIZE message to automatically resize the desktop.
         */
        SystemParametersInfoW(SPI_SETWORKAREA,
                              1,
                              &rcWorkArea,
                              SPIF_SENDCHANGE);
#endif
    }

    VOID CheckTrayWndPosition()
    {
        /* Force the rebar bands to resize */
        IUnknown_Exec(m_TrayBandSite,
                      IID_IDeskBand,
                      DBID_BANDINFOCHANGED,
                      0,
                      NULL,
                      NULL);

        /* Calculate the size of the taskbar based on the rebar */
        FitToRebar(&m_TrayRects[m_Position]);

        /* Move the tray window */
        /* The handler of WM_WINDOWPOSCHANGING will override whatever size
         * and position we use here with m_TrayRects */
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
        ResizeWorkArea();
        ApplyClipping(TRUE);
    }

    VOID RegLoadSettings()
    {
        DWORD Pos;
        RECT rcScreen;
        SIZE WndSize, EdgeSize, DlgFrameSize;
        SIZE StartBtnSize = m_StartButton.GetSize();

        EdgeSize.cx = GetSystemMetrics(SM_CXEDGE);
        EdgeSize.cy = GetSystemMetrics(SM_CYEDGE);
        DlgFrameSize.cx = GetSystemMetrics(SM_CXDLGFRAME);
        DlgFrameSize.cy = GetSystemMetrics(SM_CYDLGFRAME);

        m_Position = g_TaskbarSettings.sr.Position;
        rcScreen = g_TaskbarSettings.sr.Rect;
        GetScreenRectFromRect(&rcScreen, MONITOR_DEFAULTTONEAREST);

        if (!g_TaskbarSettings.sr.Size.cx || !g_TaskbarSettings.sr.Size.cy)
        {
            /* Use the minimum size of the taskbar, we'll use the start
               button as a minimum for now. Make sure we calculate the
               entire window size, not just the client size. However, we
               use a thinner border than a standard thick border, so that
               the start button and bands are not stuck to the screen border. */
            if(!m_Theme)
            {
                g_TaskbarSettings.sr.Size.cx = StartBtnSize.cx + (2 * (EdgeSize.cx + DlgFrameSize.cx));
                g_TaskbarSettings.sr.Size.cy = StartBtnSize.cy + (2 * (EdgeSize.cy + DlgFrameSize.cy));
            }
            else
            {
                g_TaskbarSettings.sr.Size.cx = StartBtnSize.cx - EdgeSize.cx;
                g_TaskbarSettings.sr.Size.cy = StartBtnSize.cy - EdgeSize.cy;
                if(!g_TaskbarSettings.bLock)
                    g_TaskbarSettings.sr.Size.cy += GetSystemMetrics(SM_CYSIZEFRAME);
            }
        }
        /* Determine a minimum tray window rectangle. The "client" height is
           zero here since we cannot determine an optimal minimum width when
           loaded as a vertical tray window. We just need to make sure the values
           loaded from the registry are at least. The windows explorer behaves
           the same way, it allows the user to save a zero width vertical tray
           window, but not a zero height horizontal tray window. */
        if(!m_Theme)
        {
            WndSize.cx = 2 * (EdgeSize.cx + DlgFrameSize.cx);
            WndSize.cy = StartBtnSize.cy + (2 * (EdgeSize.cy + DlgFrameSize.cy));
        }
        else
        {
            WndSize.cx = StartBtnSize.cx;
            WndSize.cy = StartBtnSize.cy - EdgeSize.cy;
        }

        if (WndSize.cx < g_TaskbarSettings.sr.Size.cx)
            WndSize.cx = g_TaskbarSettings.sr.Size.cx;
        if (WndSize.cy < g_TaskbarSettings.sr.Size.cy)
            WndSize.cy = g_TaskbarSettings.sr.Size.cy;

        /* Save the calculated size */
        m_TraySize = WndSize;

        /* Calculate all docking rectangles. We need to do this here so they're
           initialized and dragging the tray window to another position gives
           usable results */
        for (Pos = ABE_LEFT; Pos <= ABE_BOTTOM; Pos++)
        {
            GetTrayRectFromScreenRect(Pos,
                                      &rcScreen,
                                      &m_TraySize,
                                      &m_TrayRects[Pos]);
            // TRACE("m_TrayRects[%d(%d)]: %d,%d,%d,%d\n", Pos, Position, m_TrayRects[Pos].left, m_TrayRects[Pos].top, m_TrayRects[Pos].right, m_TrayRects[Pos].bottom);
        }

        /* Determine which monitor we are on. It shouldn't matter which docked
           position rectangle we use */
        m_Monitor = GetMonitorFromRect(&m_TrayRects[ABE_LEFT]);
    }

    VOID AlignControls(IN PRECT prcClient OPTIONAL)
    {
        RECT rcClient;
        SIZE TraySize, StartSize;
        POINT ptTrayNotify = { 0, 0 };
        BOOL Horizontal;
        HDWP dwp;

        m_StartButton.UpdateSize();
        if (prcClient != NULL)
        {
            rcClient = *prcClient;
        }
        else
        {
            if (!GetClientRect(&rcClient))
            {
                ERR("Could not get client rect lastErr=%d\n", GetLastError());
                return;
            }
        }

        Horizontal = IsPosHorizontal();

        /* We're about to resize/move the start button, the rebar control and
           the tray notification control */
        dwp = BeginDeferWindowPos(3);
        if (dwp == NULL)
        {
            ERR("BeginDeferWindowPos failed. lastErr=%d\n", GetLastError());
            return;
        }

        /* Limit the Start button width to the client width, if necessary */
        StartSize = m_StartButton.GetSize();
        if (StartSize.cx > rcClient.right)
            StartSize.cx = rcClient.right;

        HWND hwndTaskToolbar = ::GetWindow(m_TaskSwitch, GW_CHILD);
        if (hwndTaskToolbar)
        {
            DWORD size = SendMessageW(hwndTaskToolbar, TB_GETBUTTONSIZE, 0, 0);

            /* Themed button covers Edge area as well */
            StartSize.cy = HIWORD(size) + (m_Theme ? GetSystemMetrics(SM_CYEDGE) : 0);
        }

        if (m_StartButton.m_hWnd != NULL)
        {
            /* Resize and reposition the button */
            dwp = m_StartButton.DeferWindowPos(dwp,
                                               NULL,
                                               0,
                                               0,
                                               StartSize.cx,
                                               StartSize.cy,
                                               SWP_NOZORDER | SWP_NOACTIVATE);
            if (dwp == NULL)
            {
                ERR("DeferWindowPos for start button failed. lastErr=%d\n", GetLastError());
                return;
            }
        }

        /* Determine the size that the tray notification window needs */
        if (Horizontal)
        {
            TraySize.cx = 0;
            TraySize.cy = rcClient.bottom;
        }
        else
        {
            TraySize.cx = rcClient.right;
            TraySize.cy = 0;
        }

        if (m_TrayNotify != NULL &&
            SendMessage(m_TrayNotify,
                        TNWM_GETMINIMUMSIZE,
                        (WPARAM)Horizontal,
                        (LPARAM)&TraySize))
        {
            /* Move the tray notification window to the desired location */
            if (Horizontal)
                ptTrayNotify.x = rcClient.right - TraySize.cx;
            else
                ptTrayNotify.y = rcClient.bottom - TraySize.cy;

            dwp = ::DeferWindowPos(dwp,
                                   m_TrayNotify,
                                   NULL,
                                   ptTrayNotify.x,
                                   ptTrayNotify.y,
                                   TraySize.cx,
                                   TraySize.cy,
                                   SWP_NOZORDER | SWP_NOACTIVATE);
            if (dwp == NULL)
            {
                ERR("DeferWindowPos for notification area failed. lastErr=%d\n", GetLastError());
                return;
            }
        }

        /* Resize/Move the rebar control */
        if (m_Rebar != NULL)
        {
            POINT ptRebar = { 0, 0 };
            SIZE szRebar;

            SetWindowStyle(m_Rebar, CCS_VERT, Horizontal ? 0 : CCS_VERT);

            if (Horizontal)
            {
                ptRebar.x = StartSize.cx + GetSystemMetrics(SM_CXSIZEFRAME);
                szRebar.cx = ptTrayNotify.x - ptRebar.x;
                szRebar.cy = rcClient.bottom;
            }
            else
            {
                ptRebar.y = StartSize.cy + GetSystemMetrics(SM_CYSIZEFRAME);
                szRebar.cx = rcClient.right;
                szRebar.cy = ptTrayNotify.y - ptRebar.y;
            }

            dwp = ::DeferWindowPos(dwp,
                                   m_Rebar,
                                   NULL,
                                   ptRebar.x,
                                   ptRebar.y,
                                   szRebar.cx,
                                   szRebar.cy,
                                   SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (dwp != NULL)
            EndDeferWindowPos(dwp);

        if (m_TaskSwitch != NULL)
        {
            /* Update the task switch window configuration */
            SendMessage(m_TaskSwitch, TSWM_UPDATETASKBARPOS, 0, 0);
        }
    }

    void FitToRebar(PRECT pRect)
    {
        /* Get the rect of the rebar */
        RECT rebarRect, taskbarRect, clientRect;
        ::GetWindowRect(m_Rebar, &rebarRect);
        ::GetWindowRect(m_hWnd, &taskbarRect);
        ::GetClientRect(m_hWnd, &clientRect);
        OffsetRect(&rebarRect, -taskbarRect.left, -taskbarRect.top);

        /* Calculate the difference of size of the taskbar and the rebar */
        SIZE margins;
        margins.cx = taskbarRect.right - taskbarRect.left - clientRect.right + clientRect.left;
        margins.cy = taskbarRect.bottom - taskbarRect.top - clientRect.bottom + clientRect.top;

        /* Calculate the new size of the rebar and make it resize, then change the new taskbar size */
        switch (m_Position)
        {
        case ABE_TOP:
            rebarRect.bottom = rebarRect.top + pRect->bottom - pRect->top - margins.cy;
            ::SendMessageW(m_Rebar, RB_SIZETORECT, RBSTR_CHANGERECT,  (LPARAM)&rebarRect);
            pRect->bottom = pRect->top + rebarRect.bottom - rebarRect.top + margins.cy;
            break;
        case ABE_BOTTOM:
            rebarRect.top = rebarRect.bottom - (pRect->bottom - pRect->top - margins.cy);
            ::SendMessageW(m_Rebar, RB_SIZETORECT, RBSTR_CHANGERECT,  (LPARAM)&rebarRect);
            pRect->top = pRect->bottom - (rebarRect.bottom - rebarRect.top + margins.cy);
            break;
        case ABE_LEFT:
            rebarRect.right = rebarRect.left + (pRect->right - pRect->left - margins.cx);
            ::SendMessageW(m_Rebar, RB_SIZETORECT, RBSTR_CHANGERECT,  (LPARAM)&rebarRect);
            pRect->right = pRect->left + (rebarRect.right - rebarRect.left + margins.cx);
            break;
        case ABE_RIGHT:
            rebarRect.left = rebarRect.right - (pRect->right - pRect->left - margins.cx);
            ::SendMessageW(m_Rebar, RB_SIZETORECT, RBSTR_CHANGERECT,  (LPARAM)&rebarRect);
            pRect->left = pRect->right - (rebarRect.right - rebarRect.left + margins.cx);
            break;
        }

        CalculateValidSize(m_Position, pRect);
    }

    void PopupStartMenu()
    {
        if (m_StartMenuPopup != NULL)
        {
            POINTL pt;
            RECTL rcExclude;
            DWORD dwFlags = 0;

            if (m_StartButton.GetWindowRect((RECT*) &rcExclude))
            {
                switch (m_Position)
                {
                case ABE_BOTTOM:
                    pt.x = rcExclude.left;
                    pt.y = rcExclude.top;
                    dwFlags |= MPPF_TOP;
                    break;
                case ABE_TOP:
                    pt.x = rcExclude.left;
                    pt.y = rcExclude.bottom;
                    dwFlags |= MPPF_BOTTOM;
                    break;
                case ABE_LEFT:
                    pt.x = rcExclude.right;
                    pt.y = rcExclude.top;
                    dwFlags |= MPPF_RIGHT;
                    break;
                case ABE_RIGHT:
                    pt.x = rcExclude.left;
                    pt.y = rcExclude.top;
                    dwFlags |= MPPF_LEFT;
                    break;
                }

                m_StartMenuPopup->Popup(&pt, &rcExclude, dwFlags);

                m_StartButton.SendMessageW(BM_SETSTATE, TRUE, 0);
            }
        }
    }

    void ProcessMouseTracking()
    {
        RECT rcCurrent;
        POINT pt;
        BOOL over;
        UINT state = m_AutoHideState;

        GetCursorPos(&pt);
        GetWindowRect(&rcCurrent);
        over = PtInRect(&rcCurrent, pt);

        if (m_StartButton.SendMessage( BM_GETSTATE, 0, 0) != BST_UNCHECKED)
        {
            over = TRUE;
        }

        if (over)
        {
            if (state == AUTOHIDE_HIDING)
            {
                TRACE("AutoHide cancelling hide.\n");
                m_AutoHideState = AUTOHIDE_SHOWING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
            }
            else if (state == AUTOHIDE_HIDDEN)
            {
                TRACE("AutoHide starting show.\n");
                m_AutoHideState = AUTOHIDE_SHOWING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_DELAY_SHOW, NULL);
            }
        }
        else
        {
            if (state == AUTOHIDE_SHOWING)
            {
                TRACE("AutoHide cancelling show.\n");
                m_AutoHideState = AUTOHIDE_HIDING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
            }
            else if (state == AUTOHIDE_SHOWN)
            {
                TRACE("AutoHide starting hide.\n");
                m_AutoHideState = AUTOHIDE_HIDING;
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_DELAY_HIDE, NULL);
            }

            KillTimer(TIMER_ID_MOUSETRACK);
        }
    }

    void ProcessAutoHide()
    {
        INT w = m_TraySize.cx - GetSystemMetrics(SM_CXSIZEFRAME);
        INT h = m_TraySize.cy - GetSystemMetrics(SM_CYSIZEFRAME);

        switch (m_AutoHideState)
        {
        case AUTOHIDE_HIDING:
            switch (m_Position)
            {
            case ABE_LEFT:
                m_AutoHideOffset.cy = 0;
                m_AutoHideOffset.cx -= AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cx < -w)
                    m_AutoHideOffset.cx = w;
                break;
            case ABE_TOP:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy -= AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cy < -h)
                    m_AutoHideOffset.cy = h;
                break;
            case ABE_RIGHT:
                m_AutoHideOffset.cy = 0;
                m_AutoHideOffset.cx += AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cx > w)
                    m_AutoHideOffset.cx = w;
                break;
            case ABE_BOTTOM:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy += AUTOHIDE_SPEED_HIDE;
                if (m_AutoHideOffset.cy > h)
                    m_AutoHideOffset.cy = h;
                break;
            }

            if (m_AutoHideOffset.cx != w && m_AutoHideOffset.cy != h)
            {
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
                break;
            }

            /* fallthrough */
        case AUTOHIDE_HIDDEN:

            switch (m_Position)
            {
            case ABE_LEFT:
                m_AutoHideOffset.cx = -w;
                m_AutoHideOffset.cy = 0;
                break;
            case ABE_TOP:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy = -h;
                break;
            case ABE_RIGHT:
                m_AutoHideOffset.cx = w;
                m_AutoHideOffset.cy = 0;
                break;
            case ABE_BOTTOM:
                m_AutoHideOffset.cx = 0;
                m_AutoHideOffset.cy = h;
                break;
            }

            KillTimer(TIMER_ID_AUTOHIDE);
            m_AutoHideState = AUTOHIDE_HIDDEN;
            break;

        case AUTOHIDE_SHOWING:
            if (m_AutoHideOffset.cx >= AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cx -= AUTOHIDE_SPEED_SHOW;
            }
            else if (m_AutoHideOffset.cx <= -AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cx += AUTOHIDE_SPEED_SHOW;
            }
            else
            {
                m_AutoHideOffset.cx = 0;
            }

            if (m_AutoHideOffset.cy >= AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cy -= AUTOHIDE_SPEED_SHOW;
            }
            else if (m_AutoHideOffset.cy <= -AUTOHIDE_SPEED_SHOW)
            {
                m_AutoHideOffset.cy += AUTOHIDE_SPEED_SHOW;
            }
            else
            {
                m_AutoHideOffset.cy = 0;
            }

            if (m_AutoHideOffset.cx != 0 || m_AutoHideOffset.cy != 0)
            {
                SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_INTERVAL_ANIMATING, NULL);
                break;
            }

            /* fallthrough */
        case AUTOHIDE_SHOWN:

            KillTimer(TIMER_ID_AUTOHIDE);
            m_AutoHideState = AUTOHIDE_SHOWN;
            break;
        }

        SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER);
    }





    /**********************************************************
     *    ##### taskbar drawing #####
     */

    LRESULT EraseBackgroundWithTheme(HDC hdc)
    {
        RECT rect;
        int iSBkgndPart[4] = {TBP_BACKGROUNDLEFT, TBP_BACKGROUNDTOP, TBP_BACKGROUNDRIGHT, TBP_BACKGROUNDBOTTOM};

        ASSERT(m_Position <= ABE_BOTTOM);

        if (m_Theme)
        {
            GetClientRect(&rect);
            DrawThemeBackground(m_Theme, hdc, iSBkgndPart[m_Position], 0, &rect, 0);
        }

        return 0;
    }

    int DrawSizerWithTheme(IN HRGN hRgn)
    {
        HDC hdc;
        RECT rect;
        int iSizerPart[4] = {TBP_SIZINGBARLEFT, TBP_SIZINGBARTOP, TBP_SIZINGBARRIGHT, TBP_SIZINGBARBOTTOM};
        SIZE size;

        ASSERT(m_Position <= ABE_BOTTOM);

        HRESULT hr = GetThemePartSize(m_Theme, NULL, iSizerPart[m_Position], 0, NULL, TS_TRUE, &size);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;

        GetWindowRect(&rect);
        OffsetRect(&rect, -rect.left, -rect.top);

        hdc = GetWindowDC();

        switch (m_Position)
        {
        case ABE_LEFT:
            rect.left = rect.right - size.cx;
            break;
        case ABE_TOP:
            rect.top = rect.bottom - size.cy;
            break;
        case ABE_RIGHT:
            rect.right = rect.left + size.cx;
            break;
        case ABE_BOTTOM:
        default:
            rect.bottom = rect.top + size.cy;
            break;
        }

        DrawThemeBackground(m_Theme, hdc, iSizerPart[m_Position], 0, &rect, 0);

        ReleaseDC(hdc);
        return 0;
    }





    /*
     * ITrayWindow
     */
    HRESULT STDMETHODCALLTYPE Open()
    {
        RECT rcWnd;

        /* Check if there's already a window created and try to show it.
           If it was somehow destroyed just create a new tray window. */
        if (m_hWnd != NULL && IsWindow())
        {
            return S_OK;
        }

        DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE;
        if (g_TaskbarSettings.sr.AlwaysOnTop)
            dwExStyle |= WS_EX_TOPMOST;

        DWORD dwStyle = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        if(!m_Theme)
        {
            dwStyle |= WS_THICKFRAME | WS_BORDER;
        }

        ZeroMemory(&rcWnd, sizeof(rcWnd));
        if (m_Position != (DWORD) -1)
            rcWnd = m_TrayRects[m_Position];

        if (!Create(NULL, rcWnd, NULL, dwStyle, dwExStyle))
            return E_FAIL;

        /* Align all controls on the tray window */
        AlignControls(NULL);

        /* Move the tray window to the right position and resize it if necessary */
        CheckTrayWndPosition();

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Close()
    {
        if (m_hWnd != NULL)
        {
            SendMessage(m_hWnd,
                        WM_APP_TRAYDESTROY,
                        0,
                        0);
        }

        return S_OK;
    }

    HWND STDMETHODCALLTYPE GetHWND()
    {
        return m_hWnd;
    }

    BOOL STDMETHODCALLTYPE IsSpecialHWND(IN HWND hWnd)
    {
        return (m_hWnd == hWnd ||
                (m_DesktopWnd != NULL && m_hWnd == m_DesktopWnd));
    }

    BOOL STDMETHODCALLTYPE IsHorizontal()
    {
        return IsPosHorizontal();
    }

    BOOL STDMETHODCALLTYPE Lock(IN BOOL bLock)
    {
        BOOL bPrevLock = g_TaskbarSettings.bLock;

        if (g_TaskbarSettings.bLock != bLock)
        {
            g_TaskbarSettings.bLock = bLock;

            if (m_TrayBandSite != NULL)
            {
                if (!SUCCEEDED(m_TrayBandSite->Lock(bLock)))
                {
                    /* Reset?? */
                    g_TaskbarSettings.bLock = bPrevLock;
                    return bPrevLock;
                }
            }

            if (m_Theme)
            {
                /* Update cached tray sizes */
                for(DWORD Pos = ABE_LEFT; Pos <= ABE_BOTTOM; Pos++)
                {
                    RECT rcGripper = {0};
                    AdjustSizerRect(&rcGripper, Pos);

                    if(g_TaskbarSettings.bLock)
                    {
                        m_TrayRects[Pos].top += rcGripper.top;
                        m_TrayRects[Pos].left += rcGripper.left;
                        m_TrayRects[Pos].bottom += rcGripper.bottom;
                        m_TrayRects[Pos].right += rcGripper.right;
                    }
                    else
                    {
                        m_TrayRects[Pos].top -= rcGripper.top;
                        m_TrayRects[Pos].left -= rcGripper.left;
                        m_TrayRects[Pos].bottom -= rcGripper.bottom;
                        m_TrayRects[Pos].right -= rcGripper.right;
                    }
                }
            }
            SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
            ResizeWorkArea();
            ApplyClipping(TRUE);
        }

        return bPrevLock;
    }


    /*
     *  IContextMenu
     */
    HRESULT STDMETHODCALLTYPE QueryContextMenu(HMENU hPopup,
                                               UINT indexMenu,
                                               UINT idCmdFirst,
                                               UINT idCmdLast,
                                               UINT uFlags)
    {
        if (!m_ContextMenu)
        {
            HRESULT hr = TrayWindowCtxMenuCreator(this, m_hWnd, &m_ContextMenu);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }

        return m_ContextMenu->QueryContextMenu(hPopup, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }

    HRESULT STDMETHODCALLTYPE InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
    {
        if (!m_ContextMenu)
            return E_INVALIDARG;

        return m_ContextMenu->InvokeCommand(lpici);
    }

    HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR idCmd,
                                               UINT uType,
                                               UINT *pwReserved,
                                               LPSTR pszName,
                                               UINT cchMax)
    {
        if (!m_ContextMenu)
            return E_INVALIDARG;

        return m_ContextMenu->GetCommandString(idCmd, uType, pwReserved, pszName, cchMax);
    }

    /**********************************************************
     *    ##### message handling #####
     */

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HRESULT hRet;

        ((ITrayWindow*)this)->AddRef();

        SetWindowTheme(m_hWnd, L"TaskBar", NULL);

        /* Create the Start button */
        m_StartButton.Create(m_hWnd);

        /* Load the saved tray window settings */
        RegLoadSettings();

        /* Create and initialize the start menu */
        HBITMAP hbmBanner = LoadBitmapW(hExplorerInstance, MAKEINTRESOURCEW(IDB_STARTMENU));
        m_StartMenuPopup = CreateStartMenu(this, &m_StartMenuBand, hbmBanner, 0);

        /* Create the task band */
        hRet = CTaskBand_CreateInstance(this, m_StartButton.m_hWnd, IID_PPV_ARG(IDeskBand, &m_TaskBand));
        if (FAILED_UNEXPECTEDLY(hRet))
            return FALSE;

        /* Create the rebar band site. This actually creates the rebar and the tasks toolbar. */
        hRet = CTrayBandSite_CreateInstance(this, m_TaskBand, &m_TrayBandSite);
        if (FAILED_UNEXPECTEDLY(hRet))
            return FALSE;

        /* Create the tray notification window */
        hRet = CTrayNotifyWnd_CreateInstance(m_hWnd, IID_PPV_ARG(IUnknown, &m_TrayNotifyInstance));
        if (FAILED_UNEXPECTEDLY(hRet))
            return FALSE;

        /* Get the hwnd of the rebar */
        hRet = IUnknown_GetWindow(m_TrayBandSite, &m_Rebar);
        if (FAILED_UNEXPECTEDLY(hRet))
            return FALSE;

        /* Get the hwnd of the tasks toolbar */
        hRet = IUnknown_GetWindow(m_TaskBand, &m_TaskSwitch);
        if (FAILED_UNEXPECTEDLY(hRet))
            return FALSE;

        /* Get the hwnd of the tray notification window */
        hRet = IUnknown_GetWindow(m_TrayNotifyInstance, &m_TrayNotify);
        if (FAILED_UNEXPECTEDLY(hRet))
            return FALSE;

        SetWindowTheme(m_Rebar, L"TaskBar", NULL);

        UpdateFonts();

        InitShellServices(&m_ShellServices);

        if (g_TaskbarSettings.sr.AutoHide)
        {
            m_AutoHideState = AUTOHIDE_HIDING;
            SetTimer(TIMER_ID_AUTOHIDE, AUTOHIDE_DELAY_HIDE, NULL);
        }

        /* Set the initial lock state in the band site */
        m_TrayBandSite->Lock(g_TaskbarSettings.bLock);

        RegisterHotKey(m_hWnd, IDHK_RUN, MOD_WIN, 'R');
        RegisterHotKey(m_hWnd, IDHK_MINIMIZE_ALL, MOD_WIN, 'M');
        RegisterHotKey(m_hWnd, IDHK_RESTORE_ALL, MOD_WIN|MOD_SHIFT, 'M');
        RegisterHotKey(m_hWnd, IDHK_HELP, MOD_WIN, VK_F1);
        RegisterHotKey(m_hWnd, IDHK_EXPLORE, MOD_WIN, 'E');
        RegisterHotKey(m_hWnd, IDHK_FIND, MOD_WIN, 'F');
        RegisterHotKey(m_hWnd, IDHK_FIND_COMPUTER, MOD_WIN|MOD_CONTROL, 'F');
        RegisterHotKey(m_hWnd, IDHK_NEXT_TASK, MOD_WIN, VK_TAB);
        RegisterHotKey(m_hWnd, IDHK_PREV_TASK, MOD_WIN|MOD_SHIFT, VK_TAB);
        RegisterHotKey(m_hWnd, IDHK_SYS_PROPERTIES, MOD_WIN, VK_PAUSE);
        RegisterHotKey(m_hWnd, IDHK_DESKTOP, MOD_WIN, 'D');
        RegisterHotKey(m_hWnd, IDHK_PAGER, MOD_WIN, 'B');

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_Theme)
            CloseThemeData(m_Theme);

        m_Theme = OpenThemeData(m_hWnd, L"TaskBar");

        if (m_Theme)
        {
            SetWindowStyle(m_hWnd, WS_THICKFRAME | WS_BORDER, 0);
        }
        else
        {
            SetWindowStyle(m_hWnd, WS_THICKFRAME | WS_BORDER, WS_THICKFRAME | WS_BORDER);
        }
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

        return TRUE;
    }

    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            SendMessage(m_TrayNotify, uMsg, wParam, lParam);
            SendMessage(m_TaskSwitch, uMsg, wParam, lParam);
            UpdateFonts();
            AlignControls(NULL);
            CheckTrayWndPosition();
        }

        return 0;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!m_Theme)
        {
            bHandled = FALSE;
            return 0;
        }

        return EraseBackgroundWithTheme(hdc);
    }

    LRESULT OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Load the saved tray window settings */
        RegLoadSettings();

        /* Move the tray window to the right position and resize it if necessary */
        CheckTrayWndPosition();

        return TRUE;
    }

    LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        COPYDATASTRUCT *pCopyData = reinterpret_cast<COPYDATASTRUCT *>(lParam);
        switch (pCopyData->dwData)
        {
            case TABDMC_APPBAR:
                return appbar_message(pCopyData);
            case TABDMC_NOTIFY:
            case TABDMC_LOADINPROC:
                return ::SendMessageW(m_TrayNotify, uMsg, wParam, lParam);
        }
        return FALSE;
    }

    LRESULT OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (!m_Theme)
        {
            bHandled = FALSE;
            return 0;
        }
        else if (g_TaskbarSettings.bLock)
        {
            return 0;
        }

        return DrawSizerWithTheme((HRGN) wParam);
    }

    LRESULT OnCtlColorBtn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetBkMode((HDC) wParam, TRANSPARENT);
        return (LRESULT) GetStockObject(HOLLOW_BRUSH);
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT rcClient;
        POINT pt;

        if (g_TaskbarSettings.bLock)
        {
            /* The user may not be able to resize the tray window.
            Pretend like the window is not sizeable when the user
            clicks on the border. */
            return HTBORDER;
        }

        SetLastError(ERROR_SUCCESS);
        if (GetClientRect(&rcClient) &&
            (MapWindowPoints(NULL, (LPPOINT) &rcClient, 2) != 0 || GetLastError() == ERROR_SUCCESS))
        {
            pt.x = (SHORT) LOWORD(lParam);
            pt.y = (SHORT) HIWORD(lParam);

            if (PtInRect(&rcClient, pt))
            {
                /* The user is trying to drag the tray window */
                return HTCAPTION;
            }

            /* Depending on the position of the tray window, allow only
            changing the border next to the monitor working area */
            switch (m_Position)
            {
            case ABE_TOP:
                if (pt.y > rcClient.bottom)
                    return HTBOTTOM;
                break;
            case ABE_LEFT:
                if (pt.x > rcClient.right)
                    return HTRIGHT;
                break;
            case ABE_RIGHT:
                if (pt.x < rcClient.left)
                    return HTLEFT;
                break;
            case ABE_BOTTOM:
            default:
                if (pt.y < rcClient.top)
                    return HTTOP;
                break;
            }
        }
        return HTBORDER;
    }

    LRESULT OnMoving(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT ptCursor;
        PRECT pRect = (PRECT) lParam;

        /* We need to ensure that an application can not accidently
        move the tray window (using SetWindowPos). However, we still
        need to be able to move the window in case the user wants to
        drag the tray window to another position or in case the user
        wants to resize the tray window. */
        if (!g_TaskbarSettings.bLock && GetCursorPos(&ptCursor))
        {
            IsDragging = TRUE;
            m_DraggingPosition = GetDraggingRectFromPt(ptCursor, pRect, &m_DraggingMonitor);
        }
        else
        {
            *pRect = m_TrayRects[m_Position];
        }
        return TRUE;
    }

    LRESULT OnSizing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PRECT pRect = (PRECT) lParam;

        if (!g_TaskbarSettings.bLock)
        {
            FitToRebar(pRect);
        }
        else
        {
            *pRect = m_TrayRects[m_Position];
        }
        return TRUE;
    }

    LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        ChangingWinPos((LPWINDOWPOS) lParam);
        return TRUE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT rcClient;
        if (wParam == SIZE_RESTORED && lParam == 0)
        {
            ResizeWorkArea();
            /* Clip the tray window on multi monitor systems so the edges can't
            overlap into another monitor */
            ApplyClipping(TRUE);

            if (!GetClientRect(&rcClient))
            {
                return FALSE;
            }
        }
        else
        {
            rcClient.left = rcClient.top = 0;
            rcClient.right = LOWORD(lParam);
            rcClient.bottom = HIWORD(lParam);
        }

        AlignControls(&rcClient);
        return TRUE;
    }

    LRESULT OnEnterSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        InSizeMove = TRUE;
        IsDragging = FALSE;
        if (!g_TaskbarSettings.bLock)
        {
            /* Remove the clipping on multi monitor systems while dragging around */
            ApplyClipping(FALSE);
        }
        return TRUE;
    }

    LRESULT OnExitSizeMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        InSizeMove = FALSE;
        if (!g_TaskbarSettings.bLock)
        {
            FitToRebar(&m_TrayRects[m_Position]);

            /* Apply clipping */
            PostMessage(WM_SIZE, SIZE_RESTORED, 0);
        }
        return TRUE;
    }

    LRESULT OnSysChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        switch (wParam)
        {
        case TEXT(' '):
        {
            /* The user pressed Alt+Space, this usually brings up the system menu of a window.
            The tray window needs to handle this specially, since it normally doesn't have
            a system menu. */

            static const UINT uidDisableItem [] = {
                SC_RESTORE,
                SC_MOVE,
                SC_SIZE,
                SC_MAXIMIZE,
                SC_MINIMIZE,
            };
            HMENU hSysMenu;
            UINT i, uId;

            /* temporarily enable the system menu */
            SetWindowStyle(m_hWnd, WS_SYSMENU, WS_SYSMENU);

            hSysMenu = GetSystemMenu(FALSE);
            if (hSysMenu != NULL)
            {
                /* Disable all items that are not relevant */
                for (i = 0; i < _countof(uidDisableItem); i++)
                {
                    EnableMenuItem(hSysMenu,
                                   uidDisableItem[i],
                                   MF_BYCOMMAND | MF_GRAYED);
                }

                EnableMenuItem(hSysMenu,
                               SC_CLOSE,
                               MF_BYCOMMAND |
                               (SHRestricted(REST_NOCLOSE) ? MF_GRAYED : MF_ENABLED));

                /* Display the system menu */
                uId = TrackMenu(
                    hSysMenu,
                    NULL,
                    m_StartButton.m_hWnd,
                    m_Position != ABE_TOP,
                    FALSE);
                if (uId != 0)
                {
                    SendMessage(m_hWnd, WM_SYSCOMMAND, (WPARAM) uId, 0);
                }
            }

            /* revert the system menu window style */
            SetWindowStyle(m_hWnd, WS_SYSMENU, 0);
            break;
        }

        default:
            bHandled = FALSE;
        }
        return TRUE;
    }

    LRESULT OnNcLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* This handler implements the trick that makes  the start button to
           get pressed when the user clicked left or below the button */

        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        WINDOWINFO wi = {sizeof(WINDOWINFO)};
        RECT rcStartBtn;

        bHandled = FALSE;

        m_StartButton.GetWindowRect(&rcStartBtn);
        GetWindowInfo(m_hWnd, &wi);

        switch (m_Position)
        {
            case ABE_TOP:
            case ABE_LEFT:
            {
                if (pt.x > rcStartBtn.right || pt.y > rcStartBtn.bottom)
                    return 0;
                break;
            }
            case ABE_RIGHT:
            {
                if (pt.x < rcStartBtn.left || pt.y > rcStartBtn.bottom)
                    return 0;

                if (rcStartBtn.right + (int)wi.cxWindowBorders * 2 + 1 < wi.rcWindow.right &&
                    pt.x > rcStartBtn.right)
                {
                    return 0;
                }
                break;
            }
            case ABE_BOTTOM:
            {
                if (pt.x > rcStartBtn.right || pt.y < rcStartBtn.top)
                {
                    return 0;
                }

                if (rcStartBtn.bottom + (int)wi.cyWindowBorders * 2 + 1 < wi.rcWindow.bottom &&
                    pt.y > rcStartBtn.bottom)
                {
                    return 0;
                }

                break;
            }
        }

        bHandled = TRUE;
        PopupStartMenu();
        return 0;
    }

    LRESULT OnNcRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* We want the user to be able to get a context menu even on the nonclient
        area (including the sizing border)! */
        uMsg = WM_CONTEXTMENU;
        wParam = (WPARAM) m_hWnd;

        return OnContextMenu(uMsg, wParam, lParam, bHandled);
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = FALSE;
        POINT pt, *ppt = NULL;
        HWND hWndExclude = NULL;

        /* Check if the administrator has forbidden access to context menus */
        if (SHRestricted(REST_NOTRAYCONTEXTMENU))
            return FALSE;

        pt.x = (SHORT) LOWORD(lParam);
        pt.y = (SHORT) HIWORD(lParam);

        if (pt.x != -1 || pt.y != -1)
            ppt = &pt;
        else
            hWndExclude = m_StartButton.m_hWnd;

        if ((HWND) wParam == m_StartButton.m_hWnd)
        {
            /* Make sure we can't track the context menu if the start
            menu is currently being shown */
            if (!(m_StartButton.SendMessage(BM_GETSTATE, 0, 0) & BST_PUSHED))
            {
                CComPtr<IContextMenu> ctxMenu;
                CStartMenuBtnCtxMenu_CreateInstance(this, m_hWnd, &ctxMenu);
                TrackCtxMenu(ctxMenu, ppt, hWndExclude, m_Position == ABE_BOTTOM, this);
            }
        }
        else
        {
            /* See if the context menu should be handled by the task band site */
            if (ppt != NULL && m_TrayBandSite != NULL)
            {
                HWND hWndAtPt;
                POINT ptClient = *ppt;

                /* Convert the coordinates to client-coordinates */
                ::MapWindowPoints(NULL, m_hWnd, &ptClient, 1);

                hWndAtPt = ChildWindowFromPoint(ptClient);
                if (hWndAtPt != NULL &&
                    (hWndAtPt == m_Rebar || ::IsChild(m_Rebar, hWndAtPt)))
                {
                    /* Check if the user clicked on the task switch window */
                    ptClient = *ppt;
                    ::MapWindowPoints(NULL, m_Rebar, &ptClient, 1);

                    hWndAtPt = ::ChildWindowFromPointEx(m_Rebar, ptClient, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
                    if (hWndAtPt == m_TaskSwitch)
                        goto HandleTrayContextMenu;

                    /* Forward the message to the task band site */
                    m_TrayBandSite->ProcessMessage(m_hWnd, uMsg, wParam, lParam, &Ret);
                }
                else
                    goto HandleTrayContextMenu;
            }
            else
            {
HandleTrayContextMenu:
                /* Tray the default tray window context menu */
                TrackCtxMenu(this, ppt, NULL, FALSE, this);
            }
        }
        return Ret;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = FALSE;
        /* FIXME: We can't check with IsChild whether the hwnd is somewhere inside
        the rebar control! But we shouldn't forward messages that the band
        site doesn't handle, such as other controls (start button, tray window */

        HRESULT hr = E_FAIL;

        if (m_TrayBandSite)
        {
            hr = m_TrayBandSite->ProcessMessage(m_hWnd, uMsg, wParam, lParam, &Ret);
            if (SUCCEEDED(hr))
                return Ret;
        }

        if (m_TrayBandSite == NULL || FAILED(hr))
        {
            const NMHDR *nmh = (const NMHDR *) lParam;

            if (nmh->hwndFrom == m_TrayNotify)
            {
                switch (nmh->code)
                {
                case NTNWM_REALIGN:
                    /* Cause all controls to be aligned */
                    PostMessage(WM_SIZE, SIZE_RESTORED, 0);
                    break;
                }
            }
        }
        return Ret;
    }

    LRESULT OnNcLButtonDblClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Let the clock handle the double click */
        ::SendMessageW(m_TrayNotify, uMsg, wParam, lParam);

        /* We "handle" this message so users can't cause a weird maximize/restore
        window animation when double-clicking the tray window! */
        return TRUE;
    }

    LRESULT OnAppTrayDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DestroyWindow();
        return TRUE;
    }

    LRESULT OnOpenStartMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HWND hwndStartMenu;
        HRESULT hr = IUnknown_GetWindow(m_StartMenuPopup, &hwndStartMenu);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        if (::IsWindowVisible(hwndStartMenu))
        {
            m_StartMenuPopup->OnSelect(MPOS_CANCELLEVEL);
        }
        else
        {
            PopupStartMenu();
        }

        return TRUE;
    }

    LRESULT OnDoExitWindows(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /*
         * TWM_DOEXITWINDOWS is send by the CDesktopBrowser to us
         * to show the shutdown dialog. Also a WM_CLOSE message sent
         * by apps should show the dialog.
         */
        return DoExitWindows();
    }

    LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == SC_CLOSE)
        {
            return DoExitWindows();
        }

        bHandled = FALSE;
        return TRUE;
    }

    LRESULT OnGetTaskSwitch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        bHandled = TRUE;
        return (LRESULT)m_TaskSwitch;
    }

    LRESULT OnHotkey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return HandleHotKey(wParam);
    }

    struct MINIMIZE_INFO
    {
        HWND hwndDesktop;
        HWND hTrayWnd;
        HWND hwndProgman;
        BOOL bRet;
        CSimpleArray<HWND> *pMinimizedAll;
        BOOL bShowDesktop;
    };

    static BOOL IsDialog(HWND hwnd)
    {
        WCHAR szClass[32];
        GetClassNameW(hwnd, szClass, _countof(szClass));
        return wcscmp(szClass, L"#32770") == 0;
    }

    static BOOL CALLBACK MinimizeWindowsProc(HWND hwnd, LPARAM lParam)
    {
        MINIMIZE_INFO *info = (MINIMIZE_INFO *)lParam;
        if (hwnd == info->hwndDesktop || hwnd == info->hTrayWnd ||
            hwnd == info->hwndProgman)
        {
            return TRUE;
        }
        if (!info->bShowDesktop)
        {
            if (!::IsWindowEnabled(hwnd) || IsDialog(hwnd))
                return TRUE;
            HWND hwndOwner = ::GetWindow(hwnd, GW_OWNER);
            if (hwndOwner && !::IsWindowEnabled(hwndOwner))
                return TRUE;
        }
        if (::IsWindowVisible(hwnd) && !::IsIconic(hwnd))
        {
            ::ShowWindowAsync(hwnd, SW_MINIMIZE);
            info->bRet = TRUE;
            info->pMinimizedAll->Add(hwnd);
        }
        return TRUE;
    }

    VOID MinimizeAll(BOOL bShowDesktop = FALSE)
    {
        MINIMIZE_INFO info;
        info.hwndDesktop = GetDesktopWindow();;
        info.hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
        info.hwndProgman = FindWindowW(L"Progman", NULL);
        info.bRet = FALSE;
        info.pMinimizedAll = &g_MinimizedAll;
        info.bShowDesktop = bShowDesktop;
        EnumWindows(MinimizeWindowsProc, (LPARAM)&info);

        // invalid handles should be cleared to avoid mismatch of handles
        for (INT i = 0; i < g_MinimizedAll.GetSize(); ++i)
        {
            if (!::IsWindow(g_MinimizedAll[i]))
                g_MinimizedAll[i] = NULL;
        }

        ::SetForegroundWindow(m_DesktopWnd);
        ::SetFocus(m_DesktopWnd);
    }

    VOID ShowDesktop()
    {
        MinimizeAll(TRUE);
    }

    VOID RestoreAll()
    {
        for (INT i = g_MinimizedAll.GetSize() - 1; i >= 0; --i)
        {
            HWND hwnd = g_MinimizedAll[i];
            if (::IsWindowVisible(hwnd) && ::IsIconic(hwnd))
            {
                ::ShowWindowAsync(hwnd, SW_RESTORE);
            }
        }
        g_MinimizedAll.RemoveAll();
    }

    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = FALSE;

        if ((HWND) lParam == m_StartButton.m_hWnd)
        {
            return FALSE;
        }

        if (m_TrayBandSite == NULL || FAILED_UNEXPECTEDLY(m_TrayBandSite->ProcessMessage(m_hWnd, uMsg, wParam, lParam, &Ret)))
        {
            return HandleCommand(LOWORD(wParam));
        }
        return Ret;
    }

    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (g_TaskbarSettings.sr.AutoHide)
        {
            SetTimer(TIMER_ID_MOUSETRACK, MOUSETRACK_INTERVAL, NULL);
        }

        return TRUE;
    }

    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == TIMER_ID_MOUSETRACK)
        {
            ProcessMouseTracking();
        }
        else if (wParam == TIMER_ID_AUTOHIDE)
        {
            ProcessAutoHide();
        }

        bHandled = FALSE;
        return TRUE;
    }

    LRESULT OnNcCalcSize(INT code, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        RECT *rc = NULL;
        /* Ignore WM_NCCALCSIZE if we are not themed or locked */
        if(!m_Theme || g_TaskbarSettings.bLock)
        {
            bHandled = FALSE;
            return 0;
        }
        if(!wParam)
        {
            rc = (RECT*)wParam;
        }
        else
        {
            NCCALCSIZE_PARAMS *prms = (NCCALCSIZE_PARAMS*)lParam;
            if(prms->lppos->flags & SWP_NOSENDCHANGING)
            {
                bHandled = FALSE;
                return 0;
            }
            rc = &prms->rgrc[0];
        }

        AdjustSizerRect(rc, m_Position);

        return 0;
    }

    LRESULT OnInitMenuPopup(INT code, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HMENU hMenu = (HMENU)wParam;
        if (::IsThereAnyEffectiveWindow(FALSE))
        {
            ::EnableMenuItem(hMenu, ID_SHELL_CMD_CASCADE_WND, MF_BYCOMMAND | MF_ENABLED);
            ::EnableMenuItem(hMenu, ID_SHELL_CMD_TILE_WND_H, MF_BYCOMMAND | MF_ENABLED);
            ::EnableMenuItem(hMenu, ID_SHELL_CMD_TILE_WND_V, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            ::EnableMenuItem(hMenu, ID_SHELL_CMD_CASCADE_WND, MF_BYCOMMAND | MF_GRAYED);
            ::EnableMenuItem(hMenu, ID_SHELL_CMD_TILE_WND_H, MF_BYCOMMAND | MF_GRAYED);
            ::EnableMenuItem(hMenu, ID_SHELL_CMD_TILE_WND_V, MF_BYCOMMAND | MF_GRAYED);
        }
        return 0;
    }

    LRESULT OnRebarAutoSize(INT code, LPNMHDR nmhdr, BOOL& bHandled)
    {
#if 0
        LPNMRBAUTOSIZE as = (LPNMRBAUTOSIZE) nmhdr;

        if (!as->fChanged)
            return 0;

        RECT rc;
        ::GetWindowRect(m_hWnd, &rc);

        SIZE szWindow = {
            rc.right - rc.left,
            rc.bottom - rc.top };
        SIZE szTarget = {
            as->rcTarget.right - as->rcTarget.left,
            as->rcTarget.bottom - as->rcTarget.top };
        SIZE szActual = {
            as->rcActual.right - as->rcActual.left,
            as->rcActual.bottom - as->rcActual.top };

        SIZE borders = {
            szWindow.cx - szTarget.cx,
            szWindow.cy - szTarget.cx,
        };

        switch (m_Position)
        {
        case ABE_LEFT:
            szWindow.cx = szActual.cx + borders.cx;
            break;
        case ABE_TOP:
            szWindow.cy = szActual.cy + borders.cy;
            break;
        case ABE_RIGHT:
            szWindow.cx = szActual.cx + borders.cx;
            rc.left = rc.right - szWindow.cy;
            break;
        case ABE_BOTTOM:
            szWindow.cy = szActual.cy + borders.cy;
            rc.top = rc.bottom - szWindow.cy;
            break;
        }

        SetWindowPos(NULL, rc.left, rc.top, szWindow.cx, szWindow.cy, SWP_NOACTIVATE | SWP_NOZORDER);
#else
        bHandled = FALSE;
#endif
        return 0;
    }

    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TaskbarSettings* newSettings = (TaskbarSettings*)lParam;

        /* Propagate the new settings to the children */
        ::SendMessageW(m_TaskSwitch, uMsg, wParam, lParam);
        ::SendMessageW(m_TrayNotify, uMsg, wParam, lParam);

        /* Toggle autohide */
        if (newSettings->sr.AutoHide != g_TaskbarSettings.sr.AutoHide)
        {
            g_TaskbarSettings.sr.AutoHide = newSettings->sr.AutoHide;
            memset(&m_AutoHideOffset, 0, sizeof(m_AutoHideOffset));
            m_AutoHideState = AUTOHIDE_SHOWN;
            if (!newSettings->sr.AutoHide)
                SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER);
            else
                SetTimer(TIMER_ID_MOUSETRACK, MOUSETRACK_INTERVAL, NULL);
        }

        /* Toggle lock state */
        Lock(newSettings->bLock);

        /* Toggle OnTop state */
        if (newSettings->sr.AlwaysOnTop != g_TaskbarSettings.sr.AlwaysOnTop)
        {
            g_TaskbarSettings.sr.AlwaysOnTop = newSettings->sr.AlwaysOnTop;
            HWND hWndInsertAfter = newSettings->sr.AlwaysOnTop ? HWND_TOPMOST : HWND_BOTTOM;
            SetWindowPos(hWndInsertAfter, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }

        g_TaskbarSettings.Save();
        return 0;
    }

    DECLARE_WND_CLASS_EX(szTrayWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayWindow)
        if (m_StartMenuBand != NULL)
        {
            MSG Msg;
            LRESULT lRet;

            Msg.hwnd = m_hWnd;
            Msg.message = uMsg;
            Msg.wParam = wParam;
            Msg.lParam = lParam;

            if (m_StartMenuBand->TranslateMenuMessage(&Msg, &lRet) == S_OK)
            {
                return lRet;
            }

            wParam = Msg.wParam;
            lParam = Msg.lParam;
        }
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        NOTIFY_CODE_HANDLER(RBN_AUTOSIZE, OnRebarAutoSize) // Doesn't quite work ;P
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        /*MESSAGE_HANDLER(WM_DESTROY, OnDestroy)*/
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
        MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
        MESSAGE_HANDLER(WM_NCPAINT, OnNcPaint)
        MESSAGE_HANDLER(WM_CTLCOLORBTN, OnCtlColorBtn)
        MESSAGE_HANDLER(WM_MOVING, OnMoving)
        MESSAGE_HANDLER(WM_SIZING, OnSizing)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_NCLBUTTONDOWN, OnNcLButtonDown)
        MESSAGE_HANDLER(WM_SYSCHAR, OnSysChar)
        MESSAGE_HANDLER(WM_NCRBUTTONUP, OnNcRButtonUp)
        MESSAGE_HANDLER(WM_NCLBUTTONDBLCLK, OnNcLButtonDblClick)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_NCMOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_APP_TRAYDESTROY, OnAppTrayDestroy)
        MESSAGE_HANDLER(WM_CLOSE, OnDoExitWindows)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotkey)
        MESSAGE_HANDLER(WM_NCCALCSIZE, OnNcCalcSize)
        MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        MESSAGE_HANDLER(TWM_OPENSTARTMENU, OnOpenStartMenu)
        MESSAGE_HANDLER(TWM_DOEXITWINDOWS, OnDoExitWindows)
        MESSAGE_HANDLER(TWM_GETTASKSWITCH, OnGetTaskSwitch)
    ALT_MSG_MAP(1)
    END_MSG_MAP()

    /*****************************************************************************/

    VOID TrayProcessMessages()
    {
        MSG Msg;

        /* FIXME: We should keep a reference here... */

        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            if (Msg.message == WM_QUIT)
                break;

            if (m_StartMenuBand == NULL ||
                m_StartMenuBand->IsMenuMessage(&Msg) != S_OK)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    VOID TrayMessageLoop()
    {
        MSG Msg;
        BOOL Ret;

        /* FIXME: We should keep a reference here... */

        while (true)
        {
            Ret = GetMessage(&Msg, NULL, 0, 0);

            if (!Ret || Ret == -1)
                break;

            if (m_StartMenuBand == NULL ||
                m_StartMenuBand->IsMenuMessage(&Msg) != S_OK)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    /*
     * IShellDesktopTray
     *
     * NOTE: this is a very windows-specific COM interface used by SHCreateDesktop()!
     *       These are the calls I observed, it may be wrong/incomplete/buggy!!!
     *       The reason we implement it is because we have to use SHCreateDesktop() so
     *       that the shell provides the desktop window and all the features that come
     *       with it (especially positioning of desktop icons)
     */

    virtual ULONG STDMETHODCALLTYPE GetState()
    {
        /* FIXME: Return ABS_ flags? */
        TRACE("IShellDesktopTray::GetState() unimplemented!\n");
        return 0;
    }

    virtual HRESULT STDMETHODCALLTYPE GetTrayWindow(OUT HWND *phWndTray)
    {
        TRACE("IShellDesktopTray::GetTrayWindow(0x%p)\n", phWndTray);
        *phWndTray = m_hWnd;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE RegisterDesktopWindow(IN HWND hWndDesktop)
    {
        TRACE("IShellDesktopTray::RegisterDesktopWindow(0x%p)\n", hWndDesktop);

        m_DesktopWnd = hWndDesktop;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Unknown(IN DWORD dwUnknown1, IN DWORD dwUnknown2)
    {
        TRACE("IShellDesktopTray::Unknown(%u,%u) unimplemented!\n", dwUnknown1, dwUnknown2);
        return S_OK;
    }

    virtual HRESULT RaiseStartButton()
    {
        m_StartButton.SendMessageW(BM_SETSTATE, FALSE, 0);
        return S_OK;
    }

    HRESULT WINAPI GetWindow(HWND* phwnd)
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode)
    {
        return E_NOTIMPL;
    }

    void _Init()
    {
        m_Position = (DWORD) -1;
    }

    DECLARE_NOT_AGGREGATABLE(CTrayWindow)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTrayWindow)
        /*COM_INTERFACE_ENTRY_IID(IID_ITrayWindow, ITrayWindow)*/
        COM_INTERFACE_ENTRY_IID(IID_IShellDesktopTray, IShellDesktopTray)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};

class CTrayWindowCtxMenu :
    public CComCoClass<CTrayWindowCtxMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu
{
    HWND hWndOwner;
    CComPtr<CTrayWindow> TrayWnd;
    CComPtr<IContextMenu> pcm;
    UINT m_idCmdCmFirst;

public:
    HRESULT Initialize(ITrayWindow * pTrayWnd, IN HWND hWndOwner)
    {
        this->TrayWnd = (CTrayWindow *) pTrayWnd;
        this->hWndOwner = hWndOwner;
        this->m_idCmdCmFirst = 0;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE
        QueryContextMenu(HMENU hPopup,
                         UINT indexMenu,
                         UINT idCmdFirst,
                         UINT idCmdLast,
                         UINT uFlags)
    {
        HMENU hMenuBase;

        hMenuBase = LoadPopupMenu(hExplorerInstance, MAKEINTRESOURCEW(IDM_TRAYWND));

        if (g_MinimizedAll.GetSize() != 0 && !::IsThereAnyEffectiveWindow(TRUE))
        {
            CStringW strRestoreAll(MAKEINTRESOURCEW(IDS_RESTORE_ALL));
            MENUITEMINFOW mii = { sizeof(mii) };
            mii.fMask = MIIM_ID | MIIM_TYPE;
            mii.wID = ID_SHELL_CMD_RESTORE_ALL;
            mii.fType = MFT_STRING;
            mii.dwTypeData = const_cast<LPWSTR>(&strRestoreAll[0]);
            SetMenuItemInfoW(hMenuBase, ID_SHELL_CMD_SHOW_DESKTOP, FALSE, &mii);
        }

        if (!hMenuBase)
            return HRESULT_FROM_WIN32(GetLastError());

        if (SHRestricted(REST_CLASSICSHELL) != 0)
        {
            DeleteMenu(hPopup,
                       ID_LOCKTASKBAR,
                       MF_BYCOMMAND);
        }

        CheckMenuItem(hMenuBase,
                      ID_LOCKTASKBAR,
                      MF_BYCOMMAND | (g_TaskbarSettings.bLock ? MF_CHECKED : MF_UNCHECKED));

        UINT idCmdNext;
        idCmdNext = Shell_MergeMenus(hPopup, hMenuBase, indexMenu, idCmdFirst, idCmdLast, MM_SUBMENUSHAVEIDS | MM_ADDSEPARATOR);
        m_idCmdCmFirst = idCmdNext - idCmdFirst;

        ::DestroyMenu(hMenuBase);

        if (TrayWnd->m_TrayBandSite != NULL)
        {
            if (FAILED(TrayWnd->m_TrayBandSite->AddContextMenus(
                hPopup,
                indexMenu,
                idCmdNext,
                idCmdLast,
                CMF_NORMAL,
                &pcm)))
            {
                WARN("AddContextMenus failed.\n");
                pcm = NULL;
            }
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE
        InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
    {
        UINT uiCmdId = PtrToUlong(lpici->lpVerb);
        if (uiCmdId != 0)
        {
            if (uiCmdId >= m_idCmdCmFirst)
            {
                CMINVOKECOMMANDINFO cmici = { 0 };

                if (pcm != NULL)
                {
                    /* Setup and invoke the shell command */
                    cmici.cbSize = sizeof(cmici);
                    cmici.hwnd = hWndOwner;
                    cmici.lpVerb = (LPCSTR) MAKEINTRESOURCEW(uiCmdId - m_idCmdCmFirst);
                    cmici.nShow = SW_NORMAL;

                    pcm->InvokeCommand(&cmici);
                }
            }
            else
            {
                TrayWnd->ExecContextMenuCmd(uiCmdId);
            }
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE
        GetCommandString(UINT_PTR idCmd,
        UINT uType,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax)
    {
        return E_NOTIMPL;
    }

    CTrayWindowCtxMenu()
    {
    }

    virtual ~CTrayWindowCtxMenu()
    {
    }

    BEGIN_COM_MAP(CTrayWindowCtxMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};

HRESULT TrayWindowCtxMenuCreator(ITrayWindow * TrayWnd, IN HWND hWndOwner, IContextMenu ** ppCtxMenu)
{
    CTrayWindowCtxMenu * mnu = new CComObject<CTrayWindowCtxMenu>();
    mnu->Initialize(TrayWnd, hWndOwner);
    *ppCtxMenu = mnu;
    return S_OK;
}

HRESULT CreateTrayWindow(ITrayWindow ** ppTray)
{
    CComPtr<CTrayWindow> Tray = new CComObject<CTrayWindow>();
    if (Tray == NULL)
        return E_OUTOFMEMORY;

    Tray->_Init();
    Tray->Open();

    *ppTray = (ITrayWindow *) Tray;

    return S_OK;
}

HRESULT
Tray_OnStartMenuDismissed(ITrayWindow* Tray)
{
    CTrayWindow * TrayWindow = static_cast<CTrayWindow *>(Tray);
    return TrayWindow->RaiseStartButton();
}

VOID TrayProcessMessages(ITrayWindow *Tray)
{
    CTrayWindow * TrayWindow = static_cast<CTrayWindow *>(Tray);
    TrayWindow->TrayProcessMessages();
}

VOID TrayMessageLoop(ITrayWindow *Tray)
{
    CTrayWindow * TrayWindow = static_cast<CTrayWindow *>(Tray);
    TrayWindow->TrayMessageLoop();
}
