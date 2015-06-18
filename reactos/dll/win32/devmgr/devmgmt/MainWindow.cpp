/*
 * PROJECT:     ReactOS Device Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/devmgr/devmgr/MainWindow.cpp
 * PURPOSE:     Implements the main container window for the device view
 * COPYRIGHT:   Copyright 2014 - 2015 Ged Murphy <gedmurphy@reactos.org>
 */


#include "stdafx.h"
#include "devmgmt.h"
#include "MainWindow.h"


/* DATA *****************************************************/

#define BTN_PROPERTIES      0
#define BTN_SCAN_HARDWARE   1
#define BTN_ENABLE_DRV      2
#define BTN_DISABLE_DRV     3
#define BTN_UPDATE_DRV      4
#define BTN_UNINSTALL_DRV   5


// menu hints
static const MENU_HINT MainMenuHintTable[] =
{
    // File Menu
    { IDC_EXIT, IDS_HINT_EXIT },

    // Action Menu
    { IDC_PROPERTIES, IDS_HINT_PROPERTIES },
    { IDC_SCAN_HARDWARE, IDS_HINT_SCAN },
    { IDC_ENABLE_DRV, IDS_HINT_ENABLE },
    { IDC_DISABLE_DRV, IDS_HINT_DISABLE },
    { IDC_UPDATE_DRV, IDS_HINT_UPDATE },  
    { IDC_UNINSTALL_DRV, IDS_HINT_UNINSTALL },
    { IDC_ADD_HARDWARE, IDS_HINT_ADD },
    

    // View Menu
    { IDC_DEVBYTYPE, IDS_HINT_DEV_BY_TYPE},
    { IDC_DEVBYCONN, IDS_HINT_DEV_BY_CONN},
    { IDC_RESBYTYPE, IDS_HINT_RES_BY_TYPE},
    { IDC_RESBYCONN, IDS_HINT_RES_BY_TYPE},
    { IDC_SHOWHIDDEN, IDS_HINT_SHOW_HIDDEN },

    { IDC_ABOUT, IDS_HINT_ABOUT }

};




#define IDS_HINT_BLANK          20000
#define IDS_HINT_PROPERTIES     20001
#define IDS_HINT_SCAN           20002
#define IDS_HINT_ENABLE         20003
#define IDS_HINT_DISABLE        20004
#define IDS_HINT_UPDATE         20005
#define IDS_HINT_UNINSTALL      20006
#define IDS_HINT_ADD            20007
#define IDS_HINT_ABOUT          20008
#define IDS_HINT_EXIT           20009

// system menu hints
static const MENU_HINT SystemMenuHintTable[] =
{
    {SC_RESTORE,    IDS_HINT_SYS_RESTORE},
    {SC_MOVE,       IDS_HINT_SYS_MOVE},
    {SC_SIZE,       IDS_HINT_SYS_SIZE},
    {SC_MINIMIZE,   IDS_HINT_SYS_MINIMIZE},
    {SC_MAXIMIZE,   IDS_HINT_SYS_MAXIMIZE},
    {SC_CLOSE,      IDS_HINT_SYS_CLOSE},
};

static TBBUTTON TbButtons[] =
{
    { BTN_PROPERTIES, IDC_PROPERTIES, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0 },
    { BTN_SCAN_HARDWARE, IDC_SCAN_HARDWARE, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0 },
    { 2, IDC_STATIC, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
    { BTN_ENABLE_DRV, IDC_ENABLE_DRV, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0 },
    { BTN_DISABLE_DRV, IDC_DISABLE_DRV, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0 },
    { BTN_UPDATE_DRV, IDC_UPDATE_DRV, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0 },
    { BTN_UNINSTALL_DRV, IDC_UNINSTALL_DRV, TBSTATE_ENABLED, BTNS_BUTTON, 0, 0 }
};


/* PUBLIC METHODS **********************************************/

CMainWindow::CMainWindow(void) :
    m_ToolbarhImageList(NULL),
    m_hMainWnd(NULL),
    m_hStatusBar(NULL),
    m_hToolBar(NULL),
    m_CmdShow(0)
{
    m_szMainWndClass = L"DevMgmtWndClass";
}

CMainWindow::~CMainWindow(void)
{
    // Destroy any previous list
    if (m_ToolbarhImageList) ImageList_Destroy(m_ToolbarhImageList);
}

bool
CMainWindow::Initialize(LPCTSTR lpCaption,
                        int nCmdShow)
{
    CAtlStringW szCaption;
    WNDCLASSEXW wc = {0};

    // Store the show window value
    m_CmdShow = nCmdShow;

    // Setup the window class struct
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
    wc.lpszClassName = m_szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(g_hInstance,
                                  MAKEINTRESOURCE(IDI_MAIN_ICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

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
                                     g_hInstance,
                                     this);
    }

    // Return creation result 
    return !!(m_hMainWnd);
}

void
CMainWindow::Uninitialize()
{
    // Unregister the window class 
    UnregisterClassW(m_szMainWndClass, g_hInstance);
}

int
CMainWindow::Run()
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


/* PRIVATE METHODS **********************************************/

bool
CMainWindow::MainWndMenuHint(WORD CmdId,
                             const MENU_HINT *HintArray,
                             DWORD HintsCount,
                             UINT DefHintId)
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
                        g_hInstance,
                        HintId);

    return Found;
}

void
CMainWindow::UpdateStatusBar(
    _In_ bool InMenuLoop
    )
{
    SendMessageW(m_hStatusBar,
                 SB_SIMPLE,
                 (WPARAM)InMenuLoop,
                 0);
}

bool
CMainWindow::RefreshView(ViewType Type)
{
    UINT CheckId;
    BOOL bSuccess;

    // Refreshed the cached view
    m_DeviceView->Refresh(Type, FALSE, TRUE);

    // Get the menu item id
    switch (Type)
    {
        case DevicesByType: CheckId = IDC_DEVBYTYPE; break;
        case DevicesByConnection: CheckId = IDC_DEVBYCONN; break;
        case ResourcesByType: CheckId = IDC_RESBYTYPE; break;
        case ResourcesByConnection: CheckId = IDC_RESBYCONN; break;
        default: ATLASSERT(FALSE); break;
    }

    // Set the new check item
    bSuccess = CheckMenuRadioItem(m_hMenu,
                                  IDC_DEVBYTYPE,
                                  IDC_RESBYCONN,
                                  CheckId,
                                  MF_BYCOMMAND);

    return TRUE;
}

bool
CMainWindow::ScanForHardwareChanges()
{
    // Refresh the cache and and display
    m_DeviceView->Refresh(m_DeviceView->GetCurrentView(),
                          true,
                          true);
    return true;
}

bool
CMainWindow::CreateToolBar()
{
    TBADDBITMAP TbAddBitmap;
    INT Index;

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
                                 g_hInstance,
                                 NULL);
    if (m_hToolBar == NULL) return FALSE;

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

    TbAddBitmap.hInst = g_hInstance;
    TbAddBitmap.nID = IDB_TOOLBAR;
    Index = SendMessageW(m_hToolBar, TB_ADDBITMAP, _countof(TbButtons), (LPARAM)&TbAddBitmap);

    SendMessageW(m_hToolBar, TB_ADDBUTTONSW, _countof(TbButtons), (LPARAM)TbButtons);
    SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);

    if (TRUE)
    {
        ShowWindow(m_hToolBar, SW_SHOW);
    }

    return TRUE;
}

bool
CMainWindow::CreateStatusBar()
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
                                   g_hInstance,
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

void CMainWindow::UpdateUiContext(_In_ LPTV_ITEMW TvItem)
{
    WORD State;

    // properties button
    if (m_DeviceView->HasProperties(TvItem))
    {
        State = TBSTATE_ENABLED;
    }
    else
    {
        State = TBSTATE_HIDDEN;
    }
    SendMessageW(m_hToolBar, TB_SETSTATE, IDC_PROPERTIES, MAKELPARAM(State, 0));
    SendMessageW(m_hToolBar, TB_SETSTATE, IDC_UPDATE_DRV, MAKELPARAM(State, 0)); //hack
    SendMessageW(m_hToolBar, TB_SETSTATE, IDC_UNINSTALL_DRV, MAKELPARAM(State, 0)); // hack

    // enable driver button
    if (m_DeviceView->IsDisabled(TvItem))
    {
        State = TBSTATE_ENABLED;
    }
    else
    {
        State = TBSTATE_HIDDEN;
    }
    SendMessageW(m_hToolBar, TB_SETSTATE, IDC_ENABLE_DRV, MAKELPARAM(State, 0));

    // disable driver button
    if (m_DeviceView->CanDisable(TvItem) && !m_DeviceView->IsDisabled(TvItem))
    {
        State = TBSTATE_ENABLED;
    }
    else
    {
        State = TBSTATE_HIDDEN;
    }
    SendMessageW(m_hToolBar, TB_SETSTATE, IDC_DISABLE_DRV, MAKELPARAM(State, 0));



    

}



bool
CMainWindow::StatusBarLoadString(IN HWND hStatusBar,
                                 IN INT PartId,
                                 IN HINSTANCE hInstance,
                                 IN UINT uID)
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
CMainWindow::OnCreate(HWND hwnd)
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
            ScanForHardwareChanges();

            // Display the window according to the user request
            ShowWindow(hwnd, m_CmdShow);
            RetCode = 0;
        }
    }

    return RetCode;
}

LRESULT
CMainWindow::OnSize()
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
CMainWindow::OnNotify(LPARAM lParam)
{
    LPNMHDR NmHdr = (LPNMHDR)lParam;
    LRESULT Ret;

    switch (NmHdr->code)
    {
        case TVN_SELCHANGED:
        {
            LPNMTREEVIEW NmTreeView = (LPNMTREEVIEW)lParam;
            UpdateUiContext(&NmTreeView->itemNew);
            break;
        }

        case NM_DBLCLK:
        {
            LPNMTREEVIEW NmTreeView = (LPNMTREEVIEW)lParam;
            m_DeviceView->DisplayPropertySheet();
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

            UINT_PTR idButton = (UINT)lpttt->hdr.idFrom;
            switch (idButton)
            {
                case IDC_PROPERTIES:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_PROPERTIES);
                    break;
                case IDC_SCAN_HARDWARE:
                    lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_SCAN);
                    break;
                case IDC_ENABLE_DRV:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ENABLE);
                    break;
                case IDC_DISABLE_DRV:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_DIABLE);
                    break;
                case IDC_UPDATE_DRV:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_UPDATE);
                    break;
                case IDC_UNINSTALL_DRV:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_UNINSTALL);
                    break;
            }
            break;
        }
    }

    return 0;
}

LRESULT
CMainWindow::OnContext(LPARAM lParam)
{
    return m_DeviceView->OnContextMenu(lParam);
}

LRESULT
CMainWindow::OnCommand(WPARAM wParam,
                       LPARAM lParam)
{
    LRESULT RetCode = 0;
    WORD Msg;

    // Get the message
    Msg = LOWORD(wParam);

    switch (Msg)
    {
        case IDC_PROPERTIES:
        {
            m_DeviceView->DisplayPropertySheet();
            break;
        }

        case IDC_SCAN_HARDWARE:
        {
            ScanForHardwareChanges();
            break;
        }

        case IDC_ENABLE_DRV:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Enable Driver", MB_OK);
            break;
        }

        case IDC_DISABLE_DRV:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Disable Driver", MB_OK);
            break;
        }

        case IDC_UPDATE_DRV:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Update Driver", MB_OK);
            break;
        }

        case IDC_UNINSTALL_DRV:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Uninstall Driver", MB_OK);
            break;
        }

        case IDC_ADD_HARDWARE:
        {
            MessageBox(m_hMainWnd, L"Not yet implemented", L"Add Hardware", MB_OK);
            break;
        }

        case IDC_DEVBYTYPE:
        {
            RefreshView(DevicesByType);
            break;
        }

        case IDC_DEVBYCONN:
        {
            RefreshView(DevicesByConnection);
            break;
        }

        case IDC_SHOWHIDDEN:
        {
            // Get the current state
            UINT CurCheckState = GetMenuState(m_hMenu, IDC_SHOWHIDDEN, MF_BYCOMMAND);
            if (CurCheckState == MF_CHECKED)
            {
                m_DeviceView->SetHiddenDevices(false);
                CheckMenuItem(m_hMenu, IDC_SHOWHIDDEN, MF_BYCOMMAND | MF_UNCHECKED);
            }
            else if (CurCheckState == MF_UNCHECKED)
            {
                m_DeviceView->SetHiddenDevices(true);
                CheckMenuItem(m_hMenu, IDC_SHOWHIDDEN, MF_BYCOMMAND | MF_CHECKED);
            }
            // Refresh the device view
            m_DeviceView->Refresh(m_DeviceView->GetCurrentView(),
                                  false,
                                  true);
            break;
        }

        case IDC_ABOUT:
        {
            // Apportion blame
            MessageBoxW(m_hMainWnd,
                        L"ReactOS Device Manager\r\nCopyright Ged Murphy 2015",
                        L"About",
                        MB_OK | MB_APPLMODAL);

            // Set focus back to the treeview
            m_DeviceView->SetFocus();
            break;
        }

        case IDC_EXIT:
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

LRESULT
CMainWindow::OnDestroy()
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
CMainWindow::MainWndProc(HWND hwnd,
                         UINT msg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    CMainWindow *This;
    LRESULT RetCode = 0;

    // Get the object pointer from window context
    This = (CMainWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
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
            This = (CMainWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams;

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


//////// MOVE ME ////////////////

HINSTANCE g_hInstance = NULL;
HANDLE ProcessHeap = NULL;

BOOL
WINAPI
DeviceManager_ExecuteW(HWND hWndParent,
                       HINSTANCE hInst,
                       LPCWSTR lpMachineName,
                       int nCmdShow)
{
    CMainWindow MainWindow;
    INITCOMMONCONTROLSEX icex;
    CAtlStringW szAppName;
    int Ret = 1;

    // Store the global values
    g_hInstance = hInst;
    ProcessHeap = GetProcessHeap();

    // Initialize common controls
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    // Load the application name
    if (szAppName.LoadStringW(g_hInstance, IDS_APPNAME))
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

    return Ret;
}
