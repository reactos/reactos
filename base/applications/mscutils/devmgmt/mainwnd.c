/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/devmgmt/mainwnd.c
 * PURPOSE:     Main window message handler
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static BOOL pCreateToolbar(PMAIN_WND_INFO Info);

static const TCHAR szMainWndClass[] = TEXT("DevMgmtWndClass");


/* Toolbar buttons */
TBBUTTON Buttons [] =
{   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    {TBICON_PROP,    IDC_PROP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0},   /* properties */
    {TBICON_REFRESH, IDC_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},   /* refresh */

    /* Note: First item for a seperator is its width in pixels */
    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                           /* separator */

    {TBICON_HELP,    IDC_PROGHELP,TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },  /* help */
    {TBICON_EXIT,    IDC_EXIT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },  /* exit */

};


/* menu hints */
static const MENU_HINT MainMenuHintTable[] = {
    /* File Menu */
    {IDC_EXIT,     IDS_HINT_EXIT},

    /* Action Menu */
    {IDC_REFRESH,  IDS_HINT_REFRESH},
    {IDC_PROP,     IDS_HINT_PROP},

    /* Help Menu */
    {IDC_PROGHELP, IDS_HINT_HELP},
    {IDC_ABOUT,    IDS_HINT_ABOUT}
};

/* system menu hints */
static const MENU_HINT SystemMenuHintTable[] = {
    {SC_RESTORE,    IDS_HINT_SYS_RESTORE},
    {SC_MOVE,       IDS_HINT_SYS_MOVE},
    {SC_SIZE,       IDS_HINT_SYS_SIZE},
    {SC_MINIMIZE,   IDS_HINT_SYS_MINIMIZE},
    {SC_MAXIMIZE,   IDS_HINT_SYS_MAXIMIZE},
    {SC_CLOSE,      IDS_HINT_SYS_CLOSE},
};


static BOOL
MainWndMenuHint(PMAIN_WND_INFO Info,
                WORD CmdId,
                const MENU_HINT *HintArray,
                DWORD HintsCount,
                UINT DefHintId)
{
    BOOL Found = FALSE;
    const MENU_HINT *LastHint;
    UINT HintId = DefHintId;

    LastHint = HintArray + HintsCount;
    while (HintArray != LastHint)
    {
        if (HintArray->CmdId == CmdId)
        {
            HintId = HintArray->HintId;
            Found = TRUE;
            break;
        }
        HintArray++;
    }

    StatusBarLoadString(Info->hStatus,
                        SB_SIMPLEID,
                        hInstance,
                        HintId);

    return Found;
}


static VOID
UpdateMainStatusBar(PMAIN_WND_INFO Info)
{
    if (Info->hStatus != NULL)
    {
        SendMessage(Info->hStatus,
                    SB_SIMPLE,
                    (WPARAM)Info->InMenuLoop,
                    0);
    }
}


static BOOL
pCreateToolbar(PMAIN_WND_INFO Info)
{
    INT NumButtons = sizeof(Buttons) / sizeof(Buttons[0]);

    Info->hTool = CreateWindowEx(0,
                                 TOOLBARCLASSNAME,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                 0, 0, 0, 0,
                                 Info->hMainWnd,
                                 (HMENU)IDC_TOOLBAR,
                                 hInstance,
                                 NULL);
    if(Info->hTool != NULL)
    {
        HIMAGELIST hImageList;

        SendMessage(Info->hTool,
                    TB_SETEXTENDEDSTYLE,
                    0,
                    TBSTYLE_EX_HIDECLIPPEDBUTTONS);

        SendMessage(Info->hTool,
                    TB_BUTTONSTRUCTSIZE,
                    sizeof(Buttons[0]),
                    0);

        hImageList = InitImageList(IDB_PROP,
                                   IDB_EXIT,
                                   16,
                                   16);
        if (hImageList == NULL)
            return FALSE;

        ImageList_Destroy((HIMAGELIST)SendMessage(Info->hTool,
                                                  TB_SETIMAGELIST,
                                                  0,
                                                  (LPARAM)hImageList));

        SendMessage(Info->hTool,
                    TB_ADDBUTTONS,
                    NumButtons,
                    (LPARAM)Buttons);

        return TRUE;
    }

    return FALSE;
}


static BOOL
CreateTreeView(PMAIN_WND_INFO Info)
{
    Info->hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE,
                                     WC_TREEVIEW,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES |
                                      TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT,
                                     0, 0, 0, 0,
                                     Info->hMainWnd,
                                     (HMENU)IDC_TREEVIEW,
                                     hInstance,
                                     NULL);
    if (Info->hTreeView == NULL)
    {
        DisplayString(_T("Could not create TreeView."));
        return FALSE;
    }

    return TRUE;
}

static BOOL
CreateStatusBar(PMAIN_WND_INFO Info)
{
    INT StatWidths[] = {110, -1}; /* widths of status bar */

    Info->hStatus = CreateWindowEx(0,
                                   STATUSCLASSNAME,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                   0, 0, 0, 0,
                                   Info->hMainWnd,
                                   (HMENU)IDC_STATUSBAR,
                                   hInstance,
                                   NULL);
    if(Info->hStatus == NULL)
        return FALSE;


    SendMessage(Info->hStatus,
                SB_SETPARTS,
                sizeof(StatWidths) / sizeof(INT),
                (LPARAM)StatWidths);

    return TRUE;
}


static DWORD WINAPI
DeviceEnumThread(LPVOID lpParameter)
{
    HTREEITEM hRoot;
    HWND *hTreeView;

    hTreeView = (HWND *)lpParameter;

    if (*hTreeView)
        FreeDeviceStrings(*hTreeView);

    hRoot = InitTreeView(*hTreeView);
    if (hRoot)
    {
        ListDevicesByType(*hTreeView, hRoot);
        return 0;
    }

    return -1;
}


static BOOL
InitMainWnd(PMAIN_WND_INFO Info)
{
    HANDLE DevEnumThread;
    HMENU hMenu;

    if (!pCreateToolbar(Info))
        DisplayString(_T("error creating toolbar"));

    if (!CreateTreeView(Info))
    {
        DisplayString(_T("error creating list view"));
        return FALSE;
    }

    if (!CreateStatusBar(Info))
        DisplayString(_T("error creating status bar"));

    /* make 'properties' bold */
    hMenu = GetMenu(Info->hMainWnd);
    hMenu = GetSubMenu(hMenu, 1);
    SetMenuDefaultItem(hMenu, IDC_PROP, FALSE);

    /* Create Popup Menu */
    Info->hShortcutMenu = LoadMenu(hInstance,
                                   MAKEINTRESOURCE(IDR_POPUP));
    Info->hShortcutMenu = GetSubMenu(Info->hShortcutMenu,
                                     0);
    SetMenuDefaultItem(Info->hShortcutMenu, IDC_PROP, FALSE);

    /* create seperate thread to emum devices */
    DevEnumThread = CreateThread(NULL,
                                 0,
                                 DeviceEnumThread,
                                 &Info->hTreeView,
                                 0,
                                 NULL);
    if (!DevEnumThread)
    {
        DisplayString(_T("Failed to enumerate devices"));
        return FALSE;
    }

    CloseHandle(DevEnumThread);
    return TRUE;
}


static VOID
OnContext(PMAIN_WND_INFO Info,
          LPARAM lParam)
{
    HTREEITEM hSelected;
    POINT pt;
    RECT rc;

    INT xPos = GET_X_LPARAM(lParam);
    INT yPos = GET_Y_LPARAM(lParam);

    hSelected = TreeView_GetSelection(Info->hTreeView);

    if (TreeView_GetItemRect(Info->hTreeView,
                         hSelected,
                         &rc,
                         TRUE))
    {
        if (GetCursorPos(&pt) &&
            ScreenToClient(Info->hTreeView, &pt) &&
            PtInRect(&rc, pt))
        {
            TrackPopupMenuEx(Info->hShortcutMenu,
                             TPM_RIGHTBUTTON,
                             xPos,
                             yPos,
                             Info->hMainWnd,
                             NULL);
        }
    }
}


static VOID
OnNotify(PMAIN_WND_INFO Info,
         LPARAM lParam)
{
    LPNMHDR pnmhdr = (LPNMHDR)lParam;

    switch (pnmhdr->code)
    {
        case TVN_SELCHANGED:
        {
            LPNM_TREEVIEW pnmtv = (LPNM_TREEVIEW)lParam;

            if (!TreeView_GetChild(Info->hTreeView,
                                   pnmtv->itemNew.hItem))
            {
                SendMessage(Info->hTool,
                            TB_SETSTATE,
                            IDC_PROP,
                            (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

                EnableMenuItem(GetMenu(Info->hMainWnd), IDC_PROP, MF_ENABLED);
                EnableMenuItem(Info->hShortcutMenu, IDC_PROP, MF_ENABLED);
            }
            else
            {
                SendMessage(Info->hTool,
                            TB_SETSTATE,
                            IDC_PROP,
                            (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));

                EnableMenuItem(GetMenu(Info->hMainWnd), IDC_PROP, MF_GRAYED);
                EnableMenuItem(Info->hShortcutMenu, IDC_PROP, MF_GRAYED);
            }
        }
        break;

        case NM_DBLCLK:
        {
            HTREEITEM hSelected = TreeView_GetSelection(Info->hTreeView);

            if (!TreeView_GetChild(Info->hTreeView,
                                   hSelected))
            {
                OpenPropSheet(Info->hTreeView,
                              hSelected);
            }
        }
        break;

        case NM_RCLICK:
        {
            TV_HITTESTINFO HitTest;

            if (GetCursorPos(&HitTest.pt) &&
                ScreenToClient(Info->hTreeView, &HitTest.pt))
            {
                if (TreeView_HitTest(Info->hTreeView, &HitTest))
                    (void)TreeView_SelectItem(Info->hTreeView, HitTest.hItem);
            }
        }
        break;

        case TTN_GETDISPINFO:
        {
            LPTOOLTIPTEXT lpttt;
            UINT idButton;

            lpttt = (LPTOOLTIPTEXT)lParam;

            idButton = (UINT)lpttt->hdr.idFrom;
            switch (idButton)
            {
                case IDC_PROP:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PROP);
                break;

                case IDC_REFRESH:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REFRESH);
                break;

                case IDC_PROGHELP:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_HELP);
                break;

                case IDC_EXIT:
                    lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXIT);
                break;
            }
        }
        break;
    }
}


static VOID
MainWndCommand(PMAIN_WND_INFO Info,
               WORD CmdId,
               HWND hControl)
{
    UNREFERENCED_PARAMETER(hControl);

    switch (CmdId)
    {
        case IDC_PROP:
        {
            HTREEITEM hSelected = TreeView_GetSelection(Info->hTreeView);
            OpenPropSheet(Info->hTreeView,
                          hSelected);
        }
        break;

        case IDC_REFRESH:
        {
            HANDLE DevEnumThread;

            SendMessage(Info->hTool,
                        TB_SETSTATE,
                        IDC_PROP,
                        (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));

            EnableMenuItem(GetMenu(Info->hMainWnd), IDC_PROP, MF_GRAYED);
            EnableMenuItem(Info->hShortcutMenu, IDC_PROP, MF_GRAYED);

            /* create seperate thread to emum devices */
            DevEnumThread = CreateThread(NULL,
                                         0,
                                         DeviceEnumThread,
                                         &Info->hTreeView,
                                         0,
                                         NULL);
            if (!DevEnumThread)
            {
                DisplayString(_T("Failed to enumerate devices"));
                break;
            }

            CloseHandle(DevEnumThread);
        }
        break;

        case IDC_PROGHELP:
        {
            DisplayString(_T("Help is not yet implemented\n"));
            SetFocus(Info->hTreeView);
        }
        break;

        case IDC_EXIT:
        {
            PostMessage(Info->hMainWnd,
                        WM_CLOSE,
                        0,
                        0);
        }
        break;

        case IDC_ABOUT:
        {
            DialogBox(hInstance,
                      MAKEINTRESOURCE(IDD_ABOUTBOX),
                      Info->hMainWnd,
                      (DLGPROC)AboutDialogProc);

            SetFocus(Info->hTreeView);
        }
        break;

    }
}


static VOID CALLBACK
MainWndResize(PMAIN_WND_INFO Info,
              WORD cx,
              WORD cy)
{
    RECT rcClient, rcTool, rcStatus;
    int lvHeight, iToolHeight, iStatusHeight;

    /* Size toolbar and get height */
    SendMessage(Info->hTool, TB_AUTOSIZE, 0, 0);
    GetWindowRect(Info->hTool, &rcTool);
    iToolHeight = rcTool.bottom - rcTool.top;

    /* Size status bar and get height */
    SendMessage(Info->hStatus, WM_SIZE, 0, 0);
    GetWindowRect(Info->hStatus, &rcStatus);
    iStatusHeight = rcStatus.bottom - rcStatus.top;

    /* Calculate remaining height and size list view */
    GetClientRect(Info->hMainWnd, &rcClient);
    lvHeight = rcClient.bottom - iToolHeight - iStatusHeight;
    SetWindowPos(Info->hTreeView,
                 NULL,
                 0,
                 iToolHeight,
                 rcClient.right,
                 lvHeight,
                 SWP_NOZORDER);
}


static LRESULT CALLBACK
MainWndProc(HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam)
{
    PMAIN_WND_INFO Info;
    LRESULT Ret = 0;

    /* Get the window context */
    Info = (PMAIN_WND_INFO)GetWindowLongPtr(hwnd,
                                            GWLP_USERDATA);
    if (Info == NULL && msg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch(msg)
    {
        case WM_CREATE:
        {
            Info = (PMAIN_WND_INFO)(((LPCREATESTRUCT)lParam)->lpCreateParams);

            /* Initialize the main window context */
            Info->hMainWnd = hwnd;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             (LONG_PTR)Info);

            if (!InitMainWnd(Info))
                SendMessage(hwnd, WM_CLOSE, 0, 0);

            /* Show the window */
            ShowWindow(hwnd,
                       Info->nCmdShow);

        }
        break;

        case WM_SIZE:
        {
            MainWndResize(Info,
                          LOWORD(lParam),
                          HIWORD(lParam));
        }
        break;

        case WM_NOTIFY:
        {
            OnNotify(Info, lParam);
        }
        break;

        case WM_CONTEXTMENU:
        {
            OnContext(Info, lParam);
        }
        break;

        case WM_COMMAND:
        {
            MainWndCommand(Info,
                           LOWORD(wParam),
                           (HWND)lParam);
            goto HandleDefaultMessage;
        }

        case WM_MENUSELECT:
        {
            if (Info->hStatus != NULL)
            {
                if (!MainWndMenuHint(Info,
                                     LOWORD(wParam),
                                     MainMenuHintTable,
                                     sizeof(MainMenuHintTable) / sizeof(MainMenuHintTable[0]),
                                     IDS_HINT_BLANK))
                {
                    MainWndMenuHint(Info,
                                    LOWORD(wParam),
                                    SystemMenuHintTable,
                                    sizeof(SystemMenuHintTable) / sizeof(SystemMenuHintTable[0]),
                                    IDS_HINT_BLANK);
                }
            }
        }
        break;

        case WM_ENTERMENULOOP:
        {
            Info->InMenuLoop = TRUE;
            UpdateMainStatusBar(Info);
            break;
        }

        case WM_EXITMENULOOP:
        {
            Info->InMenuLoop = FALSE;
            UpdateMainStatusBar(Info);
            break;
        }

        case WM_CLOSE:
        {
            FreeDeviceStrings(Info->hTreeView);
            DestroyMenu(Info->hShortcutMenu);
            DestroyWindow(hwnd);
        }
        break;

        case WM_DESTROY:
        {
            HeapFree(ProcessHeap,
                     0,
                     Info);
            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             0);

            /* Break the message queue loop */
            PostQuitMessage(0);
        }
        break;

        default:
        {
HandleDefaultMessage:

            Ret = DefWindowProc(hwnd,
                                msg,
                                wParam,
                                lParam);
        }
        break;
    }
    return Ret;
}



HWND
CreateMainWindow(LPCTSTR lpCaption,
                 int nCmdShow)
{
    PMAIN_WND_INFO Info;
    HWND hMainWnd = NULL;

    Info = (PMAIN_WND_INFO)HeapAlloc(ProcessHeap,
                                     HEAP_ZERO_MEMORY,
                                     sizeof(MAIN_WND_INFO));

    if (Info != NULL)
    {
        Info->nCmdShow = nCmdShow;

        hMainWnd = CreateWindowEx(WS_EX_WINDOWEDGE,
                                  szMainWndClass,
                                  lpCaption,
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  600,
                                  450,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  Info);
        if (hMainWnd == NULL)
        {
            GetError();
            HeapFree(ProcessHeap,
                     0,
                     Info);
        }
    }

    return hMainWnd;
}

BOOL
InitMainWindowImpl(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_MAIN_ICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

    return RegisterClassEx(&wc) != (ATOM)0;
}


VOID
UninitMainWindowImpl(VOID)
{
    UnregisterClass(szMainWndClass,
                    hInstance);
}


