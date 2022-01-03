/*
 * PROJECT:     ReactOS Device Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/devmgr/devmgmt/MainWindow.cpp
 * PURPOSE:     Implements the main container window for the device view
 * COPYRIGHT:   Copyright 2014 - 2015 Ged Murphy <gedmurphy@reactos.org>
 */


#include "precomp.h"
#include "devmgmt.h"
#include "MainWindow.h"

/* DATA *****************************************************/

#define BTN_PROPERTIES      0
#define BTN_SCAN_HARDWARE   1
#define BTN_ENABLE_DRV      2
#define BTN_DISABLE_DRV     3
#define BTN_UPDATE_DRV      4
#define BTN_UNINSTALL_DRV   5

#define REFRESH_TIMER       1

HINSTANCE g_hThisInstance = NULL;
HINSTANCE g_hParentInstance = NULL;

// menu hints
static const MENU_HINT MainMenuHintTable[] =
{
    // File Menu
    { IDM_EXIT, IDS_HINT_EXIT },

    // Action Menu
    { IDM_PROPERTIES, IDS_HINT_PROPERTIES },
    { IDM_SCAN_HARDWARE, IDS_HINT_SCAN },
    { IDM_ENABLE_DRV, IDS_HINT_ENABLE },
    { IDM_DISABLE_DRV, IDS_HINT_DISABLE },
    { IDM_UPDATE_DRV, IDS_HINT_UPDATE },
    { IDM_UNINSTALL_DRV, IDS_HINT_UNINSTALL },
    { IDM_ADD_HARDWARE, IDS_HINT_ADD },

    // View Menu
    { IDM_DEVBYTYPE, IDS_HINT_DEV_BY_TYPE},
    { IDM_DEVBYCONN, IDS_HINT_DEV_BY_CONN},
    { IDM_RESBYTYPE, IDS_HINT_RES_BY_TYPE},
    { IDM_RESBYCONN, IDS_HINT_RES_BY_TYPE},
    { IDM_SHOWHIDDEN, IDS_HINT_SHOW_HIDDEN },

    { IDM_ABOUT, IDS_HINT_ABOUT }

};


// system menu hints
static const MENU_HINT SystemMenuHintTable[] =
{
    {SC_RESTORE,    IDS_HINT_SYS_RESTORE},
    {SC_MOVE,       IDS_HINT_SYS_MOVE},
    {SC_SIZE,       IDS_HINT_SYS_SIZE},
    {SC_MINIMIZE,   IDS_HINT_SYS_MINIMIZE},
    {SC_MAXIMIZE,   IDS_HINT_SYS_MAXIMIZE},
    {SC_CLOSE,      IDS_HINT_SYS_CLOSE}
};

static TBBUTTON TbButtons[] =
{
    { BTN_PROPERTIES, IDM_PROPERTIES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_SCAN_HARDWARE, IDM_SCAN_HARDWARE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { 2, IDC_STATIC, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
    { BTN_ENABLE_DRV, IDM_ENABLE_DRV, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_DISABLE_DRV, IDM_DISABLE_DRV, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_UPDATE_DRV, IDM_UPDATE_DRV, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
    { BTN_UNINSTALL_DRV, IDM_UNINSTALL_DRV, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 }
};


/* PUBLIC METHODS **********************************************/

CDeviceManager::CDeviceManager(void) :
    m_hMainWnd(NULL),
    m_hStatusBar(NULL),
    m_hToolBar(NULL),
    m_CmdShow(0),
    m_RefreshPending(false)
{
    m_szMainWndClass = L"DevMgmtWndClass";
}

CDeviceManager::~CDeviceManager(void)
{
}

bool
CDeviceManager::Create(_In_ HWND /*hWndParent*/,
                       _In_ HINSTANCE hInst,
                       _In_opt_z_ LPCWSTR /*lpMachineName*/,
                       _In_ int nCmdShow)
{
    CDeviceManager MainWindow;
    INITCOMMONCONTROLSEX icex;
    CAtlStringW szAppName;
    int Ret = 1;

    // Store the instances
    g_hParentInstance = hInst;
    g_hThisInstance = GetModuleHandleW(L"devmgr.dll");

    // Initialize common controls
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    // Load the application name
    if (szAppName.LoadStringW(g_hThisInstance, IDS_APPNAME))
    {
        // Initialize the main window
        if (MainWindow.Initialize(szAppName, nCmdShow))
        {
            // Run the application
            Ret = MainWindow.Run();

            // Uninitialize the main window
            MainWindow.Uninitialize();
        }
    }

    return (Ret == 0);
}


/* PRIVATE METHODS **********************************************/

bool
CDeviceManager::Initialize(_In_z_ LPCTSTR lpCaption,
                           _In_ int nCmdShow)
{
    CAtlStringW szCaption;
    WNDCLASSEXW wc = {0};

    // Store the show window value
    m_CmdShow = nCmdShow;

    // Setup the window class struct
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hThisInstance;
    wc.hIcon = LoadIcon(g_hThisInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDM_MAINMENU);
    wc.lpszClassName = m_szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(g_hThisInstance,
                                  MAKEINTRESOURCE(IDI_MAIN_ICON),
                                  IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON),
                                  0);

    // Register the window
    if (RegisterClassExW(&wc))
    {
        // Create the main window and store the object pointer
        m_hMainWnd = CreateWindowExW(WS_EX_WINDOWEDGE,
                                     m_szMainWndClass,
                                     lpCaption,
                                     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     550,
                                     500,
                                     NULL,
                                     NULL,
                                     g_hThisInstance,
                                     this);
    }

    // Return creation result
    return !!(m_hMainWnd);
}

void
CDeviceManager::Uninitialize(void)
{
    // Unregister the window class
    UnregisterClassW(m_szMainWndClass, g_hThisInstance);
}

int
CDeviceManager::Run(void)
{
    MSG Msg;

    // Pump the message queue
    while (GetMessageW(&Msg, NULL, 0, 0 ) != 0)
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }

    return 0;
}

bool
CDeviceManager::MainWndMenuHint(_In_ WORD CmdId,
                                _In_ const MENU_HINT *HintArray,
                                _In_ DWORD HintsCount,
                                _In_ UINT DefHintId)
{
    bool Found = false;
    const MENU_HINT *LastHint;
    UINT HintId = DefHintId;

    LastHint = HintArray + HintsCount;
    while (HintArray != LastHint)
    {
        if (HintArray->CmdId == CmdId)
        {
            HintId = HintArray->HintId;
            Found = true;
            break;
        }
        HintArray++;
    }

    StatusBarLoadString(m_hStatusBar,
                        SB_SIMPLEID,
                        g_hThisInstance,
                        HintId);

    return Found;
}

void
CDeviceManager::UpdateStatusBar(_In_ bool InMenuLoop)
{
    SendMessageW(m_hStatusBar,
                 SB_SIMPLE,
                 (WPARAM)InMenuLoop,
                 0);
}

bool
CDeviceManager::RefreshView(_In_ ViewType Type,
                            _In_ bool ScanForChanges)
{
    UINT CheckId = 0;

    // Refreshed the cached view
    m_DeviceView->Refresh(Type, ScanForChanges, true);

    // Get the menu item id
    switch (Type)
    {
        case DevicesByType:
            CheckId = IDM_DEVBYTYPE;
            break;

        case DevicesByConnection:
            CheckId = IDM_DEVBYCONN;
            break;

        case ResourcesByType:
            CheckId = IDM_RESBYTYPE;
            break;

        case ResourcesByConnection:
            CheckId = IDM_RESBYCONN;
            break;

        default:
            ATLASSERT(FALSE);
            break;
    }

    // Set the new check item
    CheckMenuRadioItem(m_hMenu,
                       IDM_DEVBYTYPE,
                       IDM_RESBYCONN,
                       CheckId,
                       MF_BYCOMMAND);

    return true;
}

bool
CDeviceManager::CreateToolBar(void)
{
    TBADDBITMAP TbAddBitmap;

    DWORD dwStyles = WS_CHILDWINDOW | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS | CCS_NODIVIDER;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;

    // Create the toolbar window
    m_hToolBar = CreateWindowExW(dwExStyles,
                                 TOOLBARCLASSNAME,
                                 NULL,
                                 dwStyles,
                                 0, 0, 0, 0,
                                 m_hMainWnd,
                                 (HMENU)IDC_TOOLBAR,
                                 g_hThisInstance,
                                 NULL);
    if (m_hToolBar == NULL)
        return FALSE;

    // Don't show clipped buttons
    SendMessageW(m_hToolBar,
                 TB_SETEXTENDEDSTYLE,
                 0,
                 TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    SendMessageW(m_hToolBar, TB_SETBITMAPSIZE, 0, MAKELONG(16, 16));

    // Set the struct size, the toobar needs this...
    SendMessageW(m_hToolBar,
                 TB_BUTTONSTRUCTSIZE,
                 sizeof(TBBUTTON),
                 0);

    TbAddBitmap.hInst = g_hThisInstance;
    TbAddBitmap.nID = IDB_TOOLBAR;
    SendMessageW(m_hToolBar, TB_ADDBITMAP, _countof(TbButtons), (LPARAM)&TbAddBitmap);

    SendMessageW(m_hToolBar, TB_ADDBUTTONSW, _countof(TbButtons), (LPARAM)TbButtons);
    SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);

    if (TRUE)
    {
        ShowWindow(m_hToolBar, SW_SHOW);
    }

    return TRUE;
}

bool
CDeviceManager::CreateStatusBar(void)
{
    int StatWidths[] = {110, -1}; // widths of status bar
    bool bRet = FALSE;

    // Create the status bar
    m_hStatusBar = CreateWindowExW(0,
                                   STATUSCLASSNAME,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                   0, 0, 0, 0,
                                   m_hMainWnd,
                                   (HMENU)IDC_STATUSBAR,
                                   g_hThisInstance,
                                   NULL);
    if (m_hStatusBar)
    {
        // Set the width
        bRet = (SendMessageW(m_hStatusBar,
                             SB_SETPARTS,
                             sizeof(StatWidths) / sizeof(int),
                             (LPARAM)StatWidths) != 0);
    }

    return bRet;
}

void CDeviceManager::UpdateToolbar()
{
    WORD State;

    CNode *Node = m_DeviceView->GetSelectedNode();

    // properties button
    if (Node->HasProperties())
    {
        State = TBSTATE_ENABLED;
    }
    else
    {
        State = TBSTATE_HIDDEN;
    }
    SendMessageW(m_hToolBar, TB_SETSTATE, IDM_PROPERTIES, MAKELPARAM(State, 0));
    SendMessageW(m_hToolBar, TB_SETSTATE, IDM_UPDATE_DRV, MAKELPARAM(State, 0)); //hack
    SendMessageW(m_hToolBar, TB_SETSTATE, IDM_UNINSTALL_DRV, MAKELPARAM(State, 0)); // hack

    // enable driver button
    if (Node->GetNodeType() == DeviceNode &&
        dynamic_cast<CDeviceNode *>(Node)->IsDisabled())
    {
        State = TBSTATE_ENABLED;
    }
    else
    {
        State = TBSTATE_HIDDEN;
    }
    SendMessageW(m_hToolBar, TB_SETSTATE, IDM_ENABLE_DRV, MAKELPARAM(State, 0));

    // disable driver button
    if (Node->GetNodeType() == DeviceNode &&
        dynamic_cast<CDeviceNode *>(Node)->CanDisable() &&
        !dynamic_cast<CDeviceNode *>(Node)->IsDisabled())
    {
        State = TBSTATE_ENABLED;
    }
    else
    {
        State = TBSTATE_HIDDEN;
    }
    SendMessageW(m_hToolBar, TB_SETSTATE, IDM_DISABLE_DRV, MAKELPARAM(State, 0));
}

bool
CDeviceManager::StatusBarLoadString(_In_ HWND hStatusBar,
                                    _In_ INT PartId,
                                    _In_ HINSTANCE hInstance,
                                    _In_ UINT uID)
{
    CAtlStringW szMessage;
    bool bRet = false;

    // Load the string
    if (szMessage.LoadStringW(hInstance, uID))
    {
        // Show the string on the status bar
        bRet = (SendMessageW(hStatusBar,
                             SB_SETTEXT,
                             (WPARAM)PartId,
                             (LPARAM)szMessage.GetBuffer()) != 0);
    }

    return bRet;
}

LRESULT
CDeviceManager::OnCreate(_In_ HWND hwnd)
{
    LRESULT RetCode;

    RetCode = -1;
    m_hMainWnd = hwnd;

    // Store a handle to the main menu
    m_hMenu = GetMenu(m_hMainWnd);

    // Create the toolbar and statusbar
    if (CreateToolBar() && CreateStatusBar())
    {
        // Create the device view object
        m_DeviceView = new CDeviceView(m_hMainWnd);
        if (m_DeviceView->Initialize())
        {
            // Do the initial scan
            RefreshView(m_DeviceView->GetCurrentView(), true);

            // Display the window according to the user request
            ShowWindow(hwnd, m_CmdShow);
            RetCode = 0;
        }
    }

    return RetCode;
}

LRESULT
CDeviceManager::OnSize(void)
{
    RECT rcClient, rcTool, rcStatus;
    INT lvHeight, iToolHeight, iStatusHeight;

    // Autosize the toolbar
    SendMessage(m_hToolBar, TB_AUTOSIZE, 0, 0);

    // Get the toolbar rect and save the height
    GetWindowRect(m_hToolBar, &rcTool);
    iToolHeight = rcTool.bottom - rcTool.top;

    // Resize the status bar
    SendMessage(m_hStatusBar, WM_SIZE, 0, 0);

    // Get the statusbar rect and save the height
    GetWindowRect(m_hStatusBar, &rcStatus);
    iStatusHeight = rcStatus.bottom - rcStatus.top;

    // Get the full client rect
    GetClientRect(m_hMainWnd, &rcClient);

    // Calculate the remaining height for the treeview
    lvHeight = rcClient.bottom - iToolHeight - iStatusHeight;

    // Resize the device view
    m_DeviceView->OnSize(0,
                         iToolHeight,
                         rcClient.right,
                         lvHeight);

    return 0;
}

LRESULT
CDeviceManager::OnNotify(_In_ LPARAM lParam)
{
    LPNMHDR NmHdr = (LPNMHDR)lParam;
    LRESULT Ret = 0;

    switch (NmHdr->code)
    {
        case TVN_SELCHANGED:
        {
            HMENU hMenu = GetSubMenu(m_hMenu, 1);
            for (INT i = GetMenuItemCount(hMenu) - 1; i >= 0; i--)
            {
                DeleteMenu(hMenu, i, MF_BYPOSITION);
            }
            m_DeviceView->CreateActionMenu(hMenu, true);
            UpdateToolbar();
            break;
        }

        case NM_DBLCLK:
        {
            Ret = m_DeviceView->OnDoubleClick(NmHdr);
            break;
        }

        case NM_RCLICK:
        {
            Ret = m_DeviceView->OnRightClick(NmHdr);
            break;
        }

        case NM_RETURN:
        {
            m_DeviceView->DisplayPropertySheet();
            break;
        }

        case TTN_GETDISPINFO:
        {
             LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
             lpttt->hinst = g_hThisInstance;

            UINT_PTR idButton = lpttt->hdr.idFrom;
            switch (idButton)
            {
                case IDM_PROPERTIES:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_PROPERTIES);
                    break;
                case IDM_SCAN_HARDWARE:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_SCAN);
                    break;
                case IDM_ENABLE_DRV:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_ENABLE);
                    break;
                case IDM_DISABLE_DRV:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_DISABLE);
                    break;
                case IDM_UPDATE_DRV:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UPDATE);
                    break;
                case IDM_UNINSTALL_DRV:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UNINSTALL);
                    break;
            }
            break;
        }
    }

    return Ret;
}

LRESULT
CDeviceManager::OnContext(_In_ LPARAM lParam)
{
    return m_DeviceView->OnContextMenu(lParam);
}

LRESULT
CDeviceManager::OnCommand(_In_ WPARAM wParam,
                          _In_ LPARAM /*lParam*/)
{
    LRESULT RetCode = 0;
    WORD Msg;

    // Get the message
    Msg = LOWORD(wParam);

    switch (Msg)
    {
        case IDM_PROPERTIES:
        case IDM_SCAN_HARDWARE:
        case IDM_ENABLE_DRV:
        case IDM_DISABLE_DRV:
        case IDM_UPDATE_DRV:
        case IDM_UNINSTALL_DRV:
        case IDM_ADD_HARDWARE:
        {
            m_DeviceView->OnAction(Msg);
            break;
        }

        case IDM_ACTIONMENU:
        {
            // Create a popup menu with all the actions for the selected node
            HMENU hMenu = CreatePopupMenu();
            m_DeviceView->CreateActionMenu(hMenu, true);

            // Calculate where to put the menu
            RECT rc;
            GetMenuItemRect(m_hMainWnd, m_hMenu, 1, &rc);
            LONG Height = rc.bottom - rc.top;

            // Display the menu
            TrackPopupMenuEx(hMenu,
                             TPM_RIGHTBUTTON,
                             rc.left,
                             rc.top + Height,
                             m_hMainWnd,
                             NULL);

            DestroyMenu(hMenu);
            break;
        }

        case IDM_DEVBYTYPE:
        {
            RefreshView(DevicesByType, false);
            break;
        }

        case IDM_DEVBYCONN:
        {
            RefreshView(DevicesByConnection, false);
            break;
        }

        case IDM_SHOWHIDDEN:
        {
            // Get the current state
            UINT CurCheckState = GetMenuState(m_hMenu, IDM_SHOWHIDDEN, MF_BYCOMMAND);
            if (CurCheckState == MF_CHECKED)
            {
                m_DeviceView->SetHiddenDevices(false);
                CheckMenuItem(m_hMenu, IDM_SHOWHIDDEN, MF_BYCOMMAND | MF_UNCHECKED);
            }
            else if (CurCheckState == MF_UNCHECKED)
            {
                m_DeviceView->SetHiddenDevices(true);
                CheckMenuItem(m_hMenu, IDM_SHOWHIDDEN, MF_BYCOMMAND | MF_CHECKED);
            }
            // Refresh the device view
            RefreshView(m_DeviceView->GetCurrentView(), false);
            break;
        }

        case IDM_ABOUT:
        {
            CAtlStringW szAppName;
            CAtlStringW szAppAuthors;
            HICON hIcon;

            if (!szAppName.LoadStringW(g_hThisInstance, IDS_APPNAME))
                szAppName = L"ReactOS Device Manager";
            if (!szAppAuthors.LoadStringW(g_hThisInstance, IDS_APP_AUTHORS))
                szAppAuthors = L"";
            hIcon = LoadIconW(g_hThisInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
            ShellAboutW(m_hMainWnd, szAppName, szAppAuthors, hIcon);
            if (hIcon)
                DestroyIcon(hIcon);

            // Set focus back to the treeview
            m_DeviceView->SetFocus();
            break;
        }

        case IDM_EXIT:
        {
            // Post a close message to the window
            PostMessageW(m_hMainWnd,
                         WM_CLOSE,
                         0,
                         0);
            break;
        }

        default:
            // We didn't handle it
            RetCode = -1;
            break;
    }

    return RetCode;
}

void
CDeviceManager::OnActivate(void)
{
    m_DeviceView->SetFocus();
}

LRESULT
CDeviceManager::OnDestroy(void)
{
    // Uninitialize the device view
    m_DeviceView->Uninitialize();

    // Kill the object
    delete m_DeviceView;
    m_DeviceView = NULL;

    // Clear the user data pointer
    SetWindowLongPtr(m_hMainWnd, GWLP_USERDATA, 0);

    // Break the message loop
    PostQuitMessage(0);

    return 0;
}

LRESULT CALLBACK
CDeviceManager::MainWndProc(_In_ HWND hwnd,
                            _In_ UINT msg,
                            _In_ WPARAM wParam,
                            _In_ LPARAM lParam)
{
    CDeviceManager *This;
    LRESULT RetCode = 0;

    // Get the object pointer from window context
    This = (CDeviceManager *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (This == NULL)
    {
        // Check that this isn't a create message
        if (msg != WM_CREATE)
        {
            // Don't handle null info pointer
            goto HandleDefaultMessage;
        }
    }

    switch(msg)
    {
        case WM_CREATE:
        {
            // Get the object pointer from the create param
            This = (CDeviceManager *)((LPCREATESTRUCT)lParam)->lpCreateParams;

            // Store the pointer in the window's global user data
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);

            // Call the create handler
            RetCode = This->OnCreate(hwnd);
            break;
        }

        case WM_SIZE:
        {
            RetCode = This->OnSize();
            break;
        }

        case WM_NOTIFY:
        {
            RetCode = This->OnNotify(lParam);
            break;
        }

        case WM_CONTEXTMENU:
        {
            RetCode = This->OnContext(lParam);
            break;
        }

        case WM_MENUSELECT:
        {
            if (This->m_hStatusBar != NULL)
            {
                if (!This->MainWndMenuHint(LOWORD(wParam),
                                           MainMenuHintTable,
                                           sizeof(MainMenuHintTable) / sizeof(MainMenuHintTable[0]),
                                           IDS_HINT_BLANK))
                {
                    This->MainWndMenuHint(LOWORD(wParam),
                                          SystemMenuHintTable,
                                          sizeof(SystemMenuHintTable) / sizeof(SystemMenuHintTable[0]),
                                          IDS_HINT_BLANK);
                }
            }

            break;
        }

        case WM_COMMAND:
        {
            // Handle the command message
            RetCode = This->OnCommand(wParam, lParam);
            if (RetCode == -1)
            {
                // Hand it off to the default message handler
                goto HandleDefaultMessage;
            }
            break;
        }

        case WM_DEVICECHANGE:
        {
            if (wParam == DBT_DEVNODES_CHANGED)
            {
                //
                // The OS can send multiple change messages in quick succession. To avoid
                // refreshing multiple times (and to avoid waiting in the message thread)
                // we set a timer to run in 500ms, which should leave enough time for all
                // the messages to come through. Wrap so we don't set multiple timers
                //
                if (InterlockedCompareExchange((LONG *)&This->m_RefreshPending, TRUE, FALSE) == FALSE)
                {
                    SetTimer(hwnd, REFRESH_TIMER, 500, NULL);
                }
            }
            break;
        }

        case WM_TIMER:
        {
            if (wParam == REFRESH_TIMER)
            {
                // Schedule a refresh (this just creates a thread and returns)
                This->RefreshView(This->m_DeviceView->GetCurrentView(), true);

                // Cleanup the timer
                KillTimer(hwnd, REFRESH_TIMER);

                // Allow more change notifications
                InterlockedExchange((LONG *)&This->m_RefreshPending, FALSE);
            }
            break;
        }

        case WM_ENTERMENULOOP:
        {
            This->UpdateStatusBar(true);
            break;
        }

        case WM_EXITMENULOOP:
        {
            This->UpdateStatusBar(false);
            break;
        }

        case WM_CLOSE:
        {
            // Destroy the main window
            DestroyWindow(hwnd);
            break;
        }

        case WM_ACTIVATE:
        {
            if (LOWORD(hwnd))
                This->OnActivate();
            break;
        }

        case WM_DESTROY:
        {
            // Call the destroy handler
            RetCode = This->OnDestroy();
            break;
        }

        default:
        {
HandleDefaultMessage:
            RetCode = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
        }
    }

    return RetCode;
}
