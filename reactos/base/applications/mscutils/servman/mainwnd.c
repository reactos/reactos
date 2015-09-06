/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/mainwnd.c
 * PURPOSE:     Main window message handler
 * COPYRIGHT:   Copyright 2006-20015 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

#include <windowsx.h>

static const WCHAR szMainWndClass[] = L"ServManWndClass";

BOOL bSortAscending = TRUE;

/* Temporary copy for access from list-view sort CompareFunc */
HWND hListView;

/* Toolbar buttons */
static const TBBUTTON Buttons [] =
{   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    {TBICON_PROP,    ID_PROP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0},    /* properties */
    {TBICON_REFRESH, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* refresh */
    {TBICON_EXPORT,  ID_EXPORT,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* export */

    /* Note: First item for a separator is its width in pixels */
    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_CREATE,  ID_CREATE,  TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },         /* create */
    {TBICON_DELETE,  ID_DELETE,  TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* delete */

    {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

    {TBICON_START,   ID_START,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* start */
    {TBICON_STOP,    ID_STOP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
    {TBICON_PAUSE,   ID_PAUSE,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* pause */
    {TBICON_RESTART, ID_RESTART, TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* restart */
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
                    (WPARAM)Info->bInMenuLoop,
                    0);
    }
}

VOID
UpdateServiceCount(PMAIN_WND_INFO Info)
{
    LPWSTR lpNumServices;

    if (AllocAndLoadString(&lpNumServices,
                           hInstance,
                           IDS_NUM_SERVICES))
    {
        WCHAR szNumServices[32];

        INT NumListedServ = ListView_GetItemCount(Info->hListView);

        _snwprintf(szNumServices,
                   31,
                   lpNumServices,
                   NumListedServ);

        SendMessage(Info->hStatus,
                    SB_SETTEXT,
                    0,
                    (LPARAM)szNumServices);

        LocalFree(lpNumServices);
    }
}


VOID SetMenuAndButtonStates(PMAIN_WND_INFO Info)
{
    HMENU hMainMenu;
    UINT i;

    /* get handle to menu */
    hMainMenu = GetMenu(Info->hMainWnd);

    /* set all to greyed */
    for (i = ID_START; i <= ID_RESTART; i++)
    {
        EnableMenuItem(hMainMenu, i, MF_GRAYED);
        EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), i, MF_GRAYED);
        SendMessage(Info->hTool, TB_SETSTATE, i,
                    (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    }

    if (Info->SelectedItem != NO_ITEM_SELECTED)
    {
        LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;
        DWORD Flags, State;

        /* allow user to delete service */
        if (Info->bIsUserAnAdmin)
        {
            SendMessage(Info->hTool, TB_SETSTATE, ID_DELETE,
                       (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
            EnableMenuItem(hMainMenu, ID_DELETE, MF_ENABLED);
            EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_DELETE, MF_ENABLED);
        }

        Flags = Info->pCurrentService->ServiceStatusProcess.dwControlsAccepted;
        State = Info->pCurrentService->ServiceStatusProcess.dwCurrentState;

        lpServiceConfig = GetServiceConfig(Info->pCurrentService->lpServiceName);

        if (lpServiceConfig && lpServiceConfig->dwStartType != SERVICE_DISABLED)
        {
            if (State == SERVICE_STOPPED)
            {
                EnableMenuItem(hMainMenu, ID_START, MF_ENABLED);
                EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_START, MF_ENABLED);
                SendMessage(Info->hTool, TB_SETSTATE, ID_START,
                       (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
            }

            if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
            {
                EnableMenuItem(hMainMenu, ID_RESTART, MF_ENABLED);
                EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_RESTART, MF_ENABLED);
                SendMessage(Info->hTool, TB_SETSTATE, ID_RESTART,
                       (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
            }
        }

        if(lpServiceConfig)
            HeapFree(GetProcessHeap(), 0, lpServiceConfig);

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_STOP, MF_ENABLED);
            EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_STOP, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_PAUSE, MF_ENABLED);
            EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_PAUSE, MF_ENABLED);
            SendMessage(Info->hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }
    }
    else
    {
        /* disable tools which rely on a selected service */
        EnableMenuItem(hMainMenu, ID_PROP, MF_GRAYED);
        EnableMenuItem(hMainMenu, ID_DELETE, MF_GRAYED);
        EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_PROP, MF_GRAYED);
        EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0), ID_DELETE, MF_GRAYED);
        SendMessage(Info->hTool, TB_SETSTATE, ID_PROP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
        SendMessage(Info->hTool, TB_SETSTATE, ID_DELETE,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    }

}


static INT CALLBACK
CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    WCHAR Item1[256], Item2[256];
    LVFINDINFO IndexInfo;
    INT Index;

    IndexInfo.flags = LVFI_PARAM;

    IndexInfo.lParam = lParam1;
    Index = ListView_FindItem(hListView, -1, &IndexInfo);
    ListView_GetItemText(hListView, Index, (INT)lParamSort, Item1, sizeof(Item1) / sizeof(WCHAR));

    IndexInfo.lParam = lParam2;
    Index = ListView_FindItem(hListView, -1, &IndexInfo);
    ListView_GetItemText(hListView, Index, (INT)lParamSort, Item2, sizeof(Item2) / sizeof(WCHAR));

    return bSortAscending ? wcscmp(Item1, Item2) : wcscmp(Item2, Item1);
}


static BOOL
pCreateToolbar(PMAIN_WND_INFO Info)
{
    INT numButtons = sizeof(Buttons) / sizeof(Buttons[0]);

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
                                   IDB_RESTART,
                                   GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CXSMICON),
                                   IMAGE_BITMAP);
        if (hImageList == NULL)
            return FALSE;

        ImageList_Destroy((HIMAGELIST)SendMessage(Info->hTool,
                                                  TB_SETIMAGELIST,
                                                  0,
                                                  (LPARAM)hImageList));

        SendMessage(Info->hTool,
                    TB_ADDBUTTONS,
                    numButtons,
                    (LPARAM)Buttons);

        return TRUE;
    }

    return FALSE;
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


static BOOL
InitMainWnd(PMAIN_WND_INFO Info)
{
    if (!pCreateToolbar(Info))
    {
        DisplayString(L"error creating toolbar");
        return FALSE;
    }

    if (!CreateListView(Info))
    {
        DisplayString(L"error creating list view");
        return FALSE;
    }

    if (!CreateStatusBar(Info))
        DisplayString(L"error creating status bar");

    /* Create Popup Menu */
    Info->hShortcutMenu = LoadMenu(hInstance,
                                   MAKEINTRESOURCE(IDR_POPUP));

    Info->bIsUserAnAdmin = TRUE;// IsUserAnAdmin();
    if (Info->bIsUserAnAdmin)
    {
        HMENU hMainMenu = GetMenu(Info->hMainWnd);

        SendMessage(Info->hTool,
                    TB_SETSTATE,
                    ID_CREATE,
                    (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        if (hMainMenu)
        {
            EnableMenuItem(hMainMenu,
                           ID_CREATE,
                           MF_ENABLED);
        }
        EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0),
                       ID_CREATE,
                       MF_ENABLED);
    }

    return TRUE;
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
                Info->bDlgOpen = TRUE;
                OpenPropSheet(Info);
                Info->bDlgOpen = FALSE;
                SetMenuAndButtonStates(Info);
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
                        L'\0');
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
            INT ret;

            ret = DialogBoxParam(hInstance,
                                 MAKEINTRESOURCE(IDD_DLG_CREATE),
                                 Info->hMainWnd,
                                 CreateDialogProc,
                                 (LPARAM)Info);
            if (ret == IDOK)
                RefreshServiceList(Info);

            SetFocus(Info->hListView);
        }
        break;

        case ID_DELETE:
        {
            if (Info->pCurrentService->ServiceStatusProcess.dwCurrentState != SERVICE_RUNNING)
            {
                DialogBoxParam(hInstance,
                               MAKEINTRESOURCE(IDD_DLG_DELETE),
                               Info->hMainWnd,
                               DeleteDialogProc,
                               (LPARAM)Info);
            }
            else
            {
                WCHAR Buf[60];
                LoadString(hInstance,
                           IDS_DELETE_STOP,
                           Buf,
                           sizeof(Buf) / sizeof(WCHAR));
                DisplayString(Buf);
            }

            SetFocus(Info->hListView);

        }
        break;

        case ID_START:
        {
            RunActionWithProgress(Info->hMainWnd,
                                  Info->pCurrentService->lpServiceName,
                                  Info->pCurrentService->lpDisplayName,
                                  ACTION_START,
                                  NULL); //FIXME: Add start params

            UpdateServiceStatus(Info->pCurrentService);
            ChangeListViewText(Info, Info->pCurrentService, LVSTATUS);
            SetMenuAndButtonStates(Info);
            SetFocus(Info->hListView);

        }
        break;

        case ID_STOP:
            RunActionWithProgress(Info->hMainWnd,
                                  Info->pCurrentService->lpServiceName,
                                  Info->pCurrentService->lpDisplayName,
                                  ACTION_STOP,
                                  NULL);

            UpdateServiceStatus(Info->pCurrentService);
            ChangeListViewText(Info, Info->pCurrentService, LVSTATUS);
            SetMenuAndButtonStates(Info);
            SetFocus(Info->hListView);

        break;

        case ID_PAUSE:
            RunActionWithProgress(Info->hMainWnd,
                                  Info->pCurrentService->lpServiceName,
                                  Info->pCurrentService->lpDisplayName,
                                  ACTION_PAUSE,
                                  NULL);

            UpdateServiceStatus(Info->pCurrentService);
            ChangeListViewText(Info, Info->pCurrentService, LVSTATUS);
            SetMenuAndButtonStates(Info);
            SetFocus(Info->hListView);
        break;

        case ID_RESUME:
            RunActionWithProgress(Info->hMainWnd,
                                  Info->pCurrentService->lpServiceName,
                                  Info->pCurrentService->lpDisplayName,
                                  ACTION_RESUME,
                                  NULL);

            UpdateServiceStatus(Info->pCurrentService);
            ChangeListViewText(Info, Info->pCurrentService, LVSTATUS);
            SetMenuAndButtonStates(Info);
            SetFocus(Info->hListView);
        break;

        case ID_RESTART:
            RunActionWithProgress(Info->hMainWnd,
                                  Info->pCurrentService->lpServiceName,
                                  Info->pCurrentService->lpDisplayName,
                                  ACTION_RESTART,
                                  NULL);

            UpdateServiceStatus(Info->pCurrentService);
            ChangeListViewText(Info, Info->pCurrentService, LVSTATUS);
            SetMenuAndButtonStates(Info);
            SetFocus(Info->hListView);
        break;

        case ID_HELP:
            MessageBoxW(NULL,
                        L"Help is not yet implemented\n",
                        L"Note!",
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
            CheckMenuRadioItem(GetMenu(Info->hMainWnd),
                               ID_VIEW_LARGE,
                               ID_VIEW_DETAILS,
                               ID_VIEW_LARGE,
                               MF_BYCOMMAND);
        break;

        case ID_VIEW_SMALL:
            SetListViewStyle(Info->hListView,
                             LVS_SMALLICON);
            CheckMenuRadioItem(GetMenu(Info->hMainWnd),
                               ID_VIEW_LARGE,
                               ID_VIEW_DETAILS,
                               ID_VIEW_SMALL,
                               MF_BYCOMMAND);
        break;

        case ID_VIEW_LIST:
            SetListViewStyle(Info->hListView,
                             LVS_LIST);
            CheckMenuRadioItem(GetMenu(Info->hMainWnd),
                               ID_VIEW_LARGE,
                               ID_VIEW_DETAILS,
                               ID_VIEW_LIST,
                               MF_BYCOMMAND);
        break;

        case ID_VIEW_DETAILS:
            SetListViewStyle(Info->hListView,
                             LVS_REPORT);
            CheckMenuRadioItem(GetMenu(Info->hMainWnd),
                               ID_VIEW_LARGE,
                               ID_VIEW_DETAILS,
                               ID_VIEW_DETAILS,
                               MF_BYCOMMAND);
        break;

        case ID_VIEW_CUST:
        break;

        case ID_ABOUT:
            DialogBox(hInstance,
                      MAKEINTRESOURCE(IDD_ABOUTBOX),
                      Info->hMainWnd,
                      AboutDialogProc);
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

            if (!InitMainWnd(Info))
                return -1;

            /* Fill the list-view before showing the main window */
            RefreshServiceList(Info);

            /* Show the window */
            ShowWindow(hwnd,
                       Info->nCmdShow);

            SetFocus(Info->hListView);
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

                case NM_RETURN:
                {
                    SendMessage(hwnd,
                                WM_COMMAND,
                                //ID_PROP,
                                MAKEWPARAM((WORD)ID_PROP, (WORD)0),
                                0);
                }
                break;

                case LVN_COLUMNCLICK:
                {
                    static int iLastSortColumn = 0;
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    /* get new sort parameters */
                    if (pnmv->iSubItem == iLastSortColumn)
                        bSortAscending = !bSortAscending;
                    else
                    {
                        iLastSortColumn = pnmv->iSubItem;
                        bSortAscending = TRUE;
                    }

                    /* store a copy to have access from callback */
                    hListView = Info->hListView;
                    (void)ListView_SortItems(Info->hListView,
                                             CompareFunc,
                                             pnmv->iSubItem);
                }
                break;
                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    if (pnmv->uNewState != 0)
                    {
                        ListViewSelectionChanged(Info, pnmv);
                        SetMenuAndButtonStates(Info);
                    }
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
                    TrackPopupMenuEx(GetSubMenu(Info->hShortcutMenu, 0),
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
            Info->bInMenuLoop = TRUE;
            UpdateMainStatusBar(Info);
            break;
        }

        case WM_EXITMENULOOP:
        {
            Info->bInMenuLoop = FALSE;
            UpdateMainStatusBar(Info);
            break;
        }

        case WM_CLOSE:
        {
            HeapFree(ProcessHeap,
                     0,
                     Info->pAllServices);

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
                                  680,
                                  450,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  Info);
        if (hMainWnd == NULL)
        {
            //int ret;
            //ret = GetLastError();
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
