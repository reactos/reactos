/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/mainwnd.c
 * PURPOSE:     Main window message handler
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static const TCHAR szMainWndClass[] = TEXT("ServManWndClass");

BOOL bSortAscending = TRUE;


/* Toolbar buttons */
TBBUTTON Buttons [NUM_BUTTONS] =
{   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    {TBICON_PROP,    ID_PROP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0},    /* properties */
    {TBICON_REFRESH, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* refresh */
    {TBICON_EXPORT,  ID_EXPORT,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* export */

    /* Note: First item for a seperator is its width in pixels */
    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_CREATE,  ID_CREATE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* create */
    {TBICON_DELETE,  ID_DELETE,  TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* delete */

    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_START,   ID_START,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* start */
    {TBICON_STOP,    ID_STOP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
    {TBICON_PAUSE,   ID_PAUSE,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* pause */
    {TBICON_RESTART, ID_RESTART, TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* restart */

    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_HELP,    ID_HELP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* help */
    {TBICON_EXIT,    ID_EXIT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },          /* exit */

};


/* menu hints */
static const MENU_HINT MainMenuHintTable[] = {
    /* File Menu */
    {ID_EXPORT,    IDS_HINT_EXPORT},
    {ID_EXIT,      IDS_HINT_EXIT},

    /* Action Menu */
    {ID_CONNECT,  IDS_HINT_CONNECT},
    {ID_START,    IDS_HINT_START},
    {ID_STOP,     IDS_HINT_STOP},
    {ID_PAUSE,    IDS_HINT_PAUSE},
    {ID_RESUME,   IDS_HINT_RESUME},
    {ID_RESTART,  IDS_HINT_RESTART},
    {ID_REFRESH,  IDS_HINT_REFRESH},
    {ID_EDIT,     IDS_HINT_EDIT},
    {ID_CREATE,   IDS_HINT_CREATE},
    {ID_DELETE,   IDS_HINT_DELETE},
    {ID_PROP,     IDS_HINT_PROP},

    /* View menu */
    {ID_VIEW_LARGE,   IDS_HINT_LARGE},
    {ID_VIEW_SMALL,   IDS_HINT_SMALL},
    {ID_VIEW_LIST,    IDS_HINT_LIST},
    {ID_VIEW_DETAILS, IDS_HINT_DETAILS},
    {ID_VIEW_CUST,    IDS_HINT_CUST},

    /* Help Menu */
    {ID_HELP,     IDS_HINT_HELP},
    {ID_ABOUT,    IDS_HINT_ABOUT}
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


static VOID
SetListViewStyle(HWND hListView,
                 DWORD View)
{
    DWORD Style = GetWindowLong(hListView, GWL_STYLE);

    if ((Style & LVS_TYPEMASK) != View)
    {
        SetWindowLong(hListView,
                      GWL_STYLE,
                      (Style & ~LVS_TYPEMASK) | View);
    }
}


VOID SetMenuAndButtonStates(PMAIN_WND_INFO Info)
{
    HMENU hMainMenu;
    DWORD Flags, State;

    /* get handle to menu */
    hMainMenu = GetMenu(Info->hMainWnd);

    /* set all to greyed */
    EnableMenuItem(hMainMenu, ID_START, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_STOP, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_PAUSE, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_RESUME, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_RESTART, MF_GRAYED);

    EnableMenuItem(Info->hShortcutMenu, ID_START, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_STOP, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_PAUSE, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_RESUME, MF_GRAYED);
    EnableMenuItem(Info->hShortcutMenu, ID_RESTART, MF_GRAYED);

    SendMessage(Info->hTool, TB_SETSTATE, ID_START,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(Info->hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(Info->hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(Info->hTool, TB_SETSTATE, ID_RESTART,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));

    if (Info->SelectedItem != NO_ITEM_SELECTED)
    {
        /* allow user to delete service */
        SendMessage(Info->hTool, TB_SETSTATE, ID_DELETE,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        EnableMenuItem(hMainMenu, ID_DELETE, MF_ENABLED);
        EnableMenuItem(Info->hShortcutMenu, ID_DELETE, MF_ENABLED);

        Flags = Info->CurrentService->ServiceStatusProcess.dwControlsAccepted;
        State = Info->CurrentService->ServiceStatusProcess.dwCurrentState;

        if (State == SERVICE_STOPPED)
        {
            EnableMenuItem(hMainMenu, ID_START, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_START, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_START,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_STOP, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_STOP, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_PAUSE, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_PAUSE, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_RESTART, MF_ENABLED);
            EnableMenuItem(Info->hShortcutMenu, ID_RESTART, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_RESTART,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }
    }
    else
    {
        /* disable tools which rely on a selected service */
        EnableMenuItem(hMainMenu, ID_PROP, MF_GRAYED);
        EnableMenuItem(hMainMenu, ID_DELETE, MF_GRAYED);
        EnableMenuItem(Info->hShortcutMenu, ID_PROP, MF_GRAYED);
        EnableMenuItem(Info->hShortcutMenu, ID_DELETE, MF_GRAYED);
        SendMessage(Info->hTool, TB_SETSTATE, ID_PROP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
        SendMessage(Info->hTool, TB_SETSTATE, ID_DELETE,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    }

}


static INT CALLBACK
CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    ENUM_SERVICE_STATUS_PROCESS *Param1;
    ENUM_SERVICE_STATUS_PROCESS *Param2;
//    INT iSubItem = (LPARAM)lParamSort;

    if (bSortAscending) {
        Param1 = (ENUM_SERVICE_STATUS_PROCESS *)lParam1;
        Param2 = (ENUM_SERVICE_STATUS_PROCESS *)lParam2;
    }
    else
    {
        Param1 = (ENUM_SERVICE_STATUS_PROCESS *)lParam2;
        Param2 = (ENUM_SERVICE_STATUS_PROCESS *)lParam1;
    }
    return _tcsicmp(Param1->lpDisplayName, Param2->lpDisplayName);
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
                                 0,
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
CreateListView(PMAIN_WND_INFO Info)
{
    LVCOLUMN lvc = { 0 };
    TCHAR szTemp[256];

    Info->hListView = CreateWindowEx(WS_EX_CLIENTEDGE,
                                     WC_LISTVIEW,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER |
                                        LBS_NOTIFY | LVS_SORTASCENDING | LBS_NOREDRAW,
                                     0, 0, 0, 0,
                                     Info->hMainWnd,
                                     (HMENU) IDC_SERVLIST,
                                     hInstance,
                                     NULL);
    if (Info->hListView == NULL)
    {
        MessageBox(Info->hMainWnd,
                   _T("Could not create List View."),
                   _T("Error"),
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    (void)ListView_SetExtendedListViewStyle(Info->hListView,
                                            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);/*LVS_EX_GRIDLINES |*/

    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
    lvc.fmt  = LVCFMT_LEFT;

    /* Add columns to the list-view */

    /* name */
    lvc.iSubItem = 0;
    lvc.cx       = 150;
    LoadString(hInstance,
               IDS_FIRSTCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                0,
                                &lvc);

    /* description */
    lvc.iSubItem = 1;
    lvc.cx       = 240;
    LoadString(hInstance,
               IDS_SECONDCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                1,
                                &lvc);

    /* status */
    lvc.iSubItem = 2;
    lvc.cx       = 55;
    LoadString(hInstance,
               IDS_THIRDCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                2,
                                &lvc);

    /* startup type */
    lvc.iSubItem = 3;
    lvc.cx       = 80;
    LoadString(hInstance,
               IDS_FOURTHCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                3,
                                &lvc);

    /* logon as */
    lvc.iSubItem = 4;
    lvc.cx       = 100;
    LoadString(hInstance,
               IDS_FITHCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                4,
                                &lvc);

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

static VOID
ListViewSelectionChanged(PMAIN_WND_INFO Info,
                         LPNMLISTVIEW pnmv)
{

    HMENU hMainMenu;

    /* get handle to menu */
    hMainMenu = GetMenu(Info->hMainWnd);

    /* activate properties menu item, if not already */
    if (GetMenuState(hMainMenu,
                     ID_PROP,
                     MF_BYCOMMAND) != MF_ENABLED)
    {
        EnableMenuItem(hMainMenu,
                       ID_PROP,
                       MF_ENABLED);
    }

    /* activate delete menu item, if not already */
    if (GetMenuState(hMainMenu,
                     ID_DELETE,
                     MF_BYCOMMAND) != MF_ENABLED)
    {
        EnableMenuItem(hMainMenu,
                       ID_DELETE,
                       MF_ENABLED);
        EnableMenuItem(Info->hShortcutMenu,
                       ID_DELETE,
                       MF_ENABLED);
    }


    /* set selected service */
    Info->SelectedItem = pnmv->iItem;

    /* get pointer to selected service */
    Info->CurrentService = GetSelectedService(Info);

    /* alter options for the service */
    SetMenuAndButtonStates(Info);

    /* set current selected service in the status bar */
    SendMessage(Info->hStatus,
                SB_SETTEXT,
                1,
                (LPARAM)Info->CurrentService->lpDisplayName);

    /* show the properties button */
    SendMessage(Info->hTool,
                TB_SETSTATE,
                ID_PROP,
                (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));


}


static VOID
InitMainWnd(PMAIN_WND_INFO Info)
{
    if (!pCreateToolbar(Info))
        DisplayString(_T("error creating toolbar"));

    if (!CreateListView(Info))
    {
        DisplayString(_T("error creating list view"));
        return;
    }

    if (!CreateStatusBar(Info))
        DisplayString(_T("error creating status bar"));

    /* Create Popup Menu */
    Info->hShortcutMenu = LoadMenu(hInstance,
                                   MAKEINTRESOURCE(IDR_POPUP));
    Info->hShortcutMenu = GetSubMenu(Info->hShortcutMenu,
                                     0);
}


static VOID
MainWndCommand(PMAIN_WND_INFO Info,
               WORD CmdId,
               HWND hControl)
{
    UNREFERENCED_PARAMETER(hControl);

    switch (CmdId)
    {
        case ID_PROP:
        {
            if (Info->SelectedItem != NO_ITEM_SELECTED)
            {
                PPROP_DLG_INFO PropSheet;

                PropSheet = (PROP_DLG_INFO*) HeapAlloc(ProcessHeap,
                                      HEAP_ZERO_MEMORY,
                                      sizeof(PROP_DLG_INFO));
                if (PropSheet != NULL)
                {
                    Info->PropSheet = PropSheet;
                    OpenPropSheet(Info);
                }

                HeapFree(ProcessHeap,
                         0,
                         PropSheet);

                Info->PropSheet = NULL;
            }
        }
        break;

        case ID_REFRESH:
        {
            RefreshServiceList(Info);
            Info->SelectedItem = NO_ITEM_SELECTED;

            /* disable menus and buttons */
            SetMenuAndButtonStates(Info);

            /* clear the service in the status bar */
            SendMessage(Info->hStatus,
                        SB_SETTEXT,
                        1,
                        _T('\0'));
        }
        break;

        case ID_EXPORT:
        {
            ExportFile(Info);
            SetFocus(Info->hListView);
        }
        break;

        case ID_CREATE:
        {
            DialogBoxParam(hInstance,
                           MAKEINTRESOURCE(IDD_DLG_CREATE),
                           Info->hMainWnd,
                           (DLGPROC)CreateDialogProc,
                           (LPARAM)Info);

            SetFocus(Info->hListView);
        }
        break;

        case ID_DELETE:
        {
            if (Info->CurrentService->ServiceStatusProcess.dwCurrentState != SERVICE_RUNNING)
            {
                DialogBoxParam(hInstance,
                               MAKEINTRESOURCE(IDD_DLG_DELETE),
                               Info->hMainWnd,
                               (DLGPROC)DeleteDialogProc,
                               (LPARAM)Info);
            }
            else
            {
                TCHAR Buf[60];
                LoadString(hInstance,
                           IDS_DELETE_STOP,
                           Buf,
                           sizeof(Buf) / sizeof(TCHAR));
                DisplayString(Buf);
            }

            SetFocus(Info->hListView);

        }
        break;

        case ID_START:
        {
            DoStart(Info);
        }
        break;

        case ID_STOP:
        {
            DoStop(Info);
        }
        break;

        case ID_PAUSE:
        {
            Control(Info,
                    SERVICE_CONTROL_PAUSE);
        }
        break;

        case ID_RESUME:
        {
            Control(Info,
                    SERVICE_CONTROL_CONTINUE );
        }
        break;

        case ID_RESTART:
        {
            /* FIXME: remove this hack */
            SendMessage(Info->hMainWnd,
                        WM_COMMAND,
                        0,
                        ID_STOP);
            SendMessage(Info->hMainWnd,
                        WM_COMMAND,
                        0,
                        ID_START);
        }
        break;

        case ID_HELP:
            MessageBox(NULL,
                       _T("Help is not yet implemented\n"),
                       _T("Note!"),
                       MB_OK | MB_ICONINFORMATION);
            SetFocus(Info->hListView);
        break;

        case ID_EXIT:
            PostMessage(Info->hMainWnd,
                        WM_CLOSE,
                        0,
                        0);
        break;

        case ID_VIEW_LARGE:
            SetListViewStyle(Info->hListView,
                             LVS_ICON);
        break;

        case ID_VIEW_SMALL:
            SetListViewStyle(Info->hListView,
                             LVS_SMALLICON);
        break;

        case ID_VIEW_LIST:
            SetListViewStyle(Info->hListView,
                             LVS_LIST);
        break;

        case ID_VIEW_DETAILS:
            SetListViewStyle(Info->hListView,
                             LVS_REPORT);
        break;

        case ID_VIEW_CUST:
        break;

        case ID_ABOUT:
            DialogBox(hInstance,
                      MAKEINTRESOURCE(IDD_ABOUTBOX),
                      Info->hMainWnd,
                      (DLGPROC)AboutDialogProc);
            SetFocus(Info->hListView);
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
    SetWindowPos(Info->hListView,
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
            Info->SelectedItem = NO_ITEM_SELECTED;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             (LONG_PTR)Info);

            InitMainWnd(Info);

            /* Show the window */
            ShowWindow(hwnd,
                       Info->nCmdShow);

            RefreshServiceList(Info);
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
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
	            case NM_DBLCLK:
	            {
	                POINT pt;
	                RECT rect;

	                GetCursorPos(&pt);
	                GetWindowRect(Info->hListView, &rect);

	                if (PtInRect(&rect, pt))
	                {
                        SendMessage(hwnd,
                                    WM_COMMAND,
                                    //ID_PROP,
                                    MAKEWPARAM((WORD)ID_PROP, (WORD)0),
                                    0);
	                }

                    //OpenPropSheet(Info);
	            }
			    break;

			    case LVN_COLUMNCLICK:
			    {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    (void)ListView_SortItems(Info->hListView,
                                             CompareFunc,
                                             pnmv->iSubItem);
                    bSortAscending = !bSortAscending;
			    }
                break;

			    case LVN_ITEMCHANGED:
			    {
			        LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

			        ListViewSelectionChanged(Info, pnmv);

			    }
			    break;

                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT)lParam;

                    /* Specify the resource identifier of the descriptive
                     * text for the given button. */
                    idButton = (UINT)lpttt->hdr.idFrom;
                    switch (idButton)
                    {
                        case ID_PROP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PROP);
                        break;

                        case ID_REFRESH:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REFRESH);
                        break;

                        case ID_EXPORT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXPORT);
                        break;

                        case ID_CREATE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_CREATE);
                        break;

                        case ID_DELETE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_DELETE);
                        break;

                        case ID_START:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_START);
                        break;

                        case ID_STOP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_STOP);
                        break;

                        case ID_PAUSE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PAUSE);
                        break;

                        case ID_RESTART:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_RESTART);
                        break;

                        case ID_HELP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_HELP);
                        break;

                        case ID_EXIT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXIT);
                        break;

                    }
                }
                break;
            }
        }
        break;

        case WM_CONTEXTMENU:
            {
                POINT pt;
                RECT lvRect;

                INT xPos = GET_X_LPARAM(lParam);
                INT yPos = GET_Y_LPARAM(lParam);

                GetCursorPos(&pt);

                /* display popup when cursor is in the list view */
                GetWindowRect(Info->hListView, &lvRect);
                if (PtInRect(&lvRect, pt))
                {
                    TrackPopupMenuEx(Info->hShortcutMenu,
                                     TPM_RIGHTBUTTON,
                                     xPos,
                                     yPos,
                                     Info->hMainWnd,
                                     NULL);
                }
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
            /* Free service array */
            HeapFree(ProcessHeap,
                     0,
                     Info->pServiceStatus);

            DestroyMenu(Info->hShortcutMenu);
		    DestroyWindow(hwnd);
	    }
	    break;

	    case WM_DESTROY:
        {
            //DestroyMainWnd(Info);

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

    Info = (MAIN_WND_INFO*) HeapAlloc(ProcessHeap,
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
                                  650,
                                  450,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  Info);
        if (hMainWnd == NULL)
        {
            int ret;
            ret = GetLastError();
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
                        MAKEINTRESOURCE(IDI_SM_ICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_SM_ICON),
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

