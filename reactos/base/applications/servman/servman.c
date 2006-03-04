/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/servman.c
 * PURPOSE:     Main window message handler
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

const TCHAR ClassName[] = _T("ServiceManager");

HINSTANCE hInstance;
HWND hMainWnd;
HWND hListView;
HWND hStatus;
HWND hTool;
HWND hProgDlg;
HMENU hShortcutMenu;
INT SelectedItem = -1;

TBBUTTON *ptbb;

extern HWND hwndGenDlg;


INT GetSelectedItem(VOID)
{
    return SelectedItem;
}

VOID SetView(DWORD View)
{
    DWORD Style = GetWindowLong(hListView, GWL_STYLE);

    if ((Style & LVS_TYPEMASK) != View)
        SetWindowLong(hListView, GWL_STYLE, (Style & ~LVS_TYPEMASK) | View);
}


VOID SetMenuAndButtonStates()
{
    HMENU hMainMenu;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    DWORD Flags, State;

    /* get handle to menu */
    hMainMenu = GetMenu(hMainWnd);

    /* set all to greyed */
    EnableMenuItem(hMainMenu, ID_START, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_STOP, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_PAUSE, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_RESUME, MF_GRAYED);
    EnableMenuItem(hMainMenu, ID_RESTART, MF_GRAYED);

    EnableMenuItem(hShortcutMenu, ID_START, MF_GRAYED);
    EnableMenuItem(hShortcutMenu, ID_STOP, MF_GRAYED);
    EnableMenuItem(hShortcutMenu, ID_PAUSE, MF_GRAYED);
    EnableMenuItem(hShortcutMenu, ID_RESUME, MF_GRAYED);
    EnableMenuItem(hShortcutMenu, ID_RESTART, MF_GRAYED);

    SendMessage(hTool, TB_SETSTATE, ID_START,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    SendMessage(hTool, TB_SETSTATE, ID_RESTART,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));

    if (GetSelectedItem() != -1)
    {
        /* get pointer to selected service */
        Service = GetSelectedService();

        Flags = Service->ServiceStatusProcess.dwControlsAccepted;
        State = Service->ServiceStatusProcess.dwCurrentState;

        if (State == SERVICE_STOPPED)
        {
            EnableMenuItem(hMainMenu, ID_START, MF_ENABLED);
            EnableMenuItem(hShortcutMenu, ID_START, MF_ENABLED);
            SendMessage(hTool, TB_SETSTATE, ID_START,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_STOP, MF_ENABLED);
            EnableMenuItem(hShortcutMenu, ID_STOP, MF_ENABLED);
            SendMessage(hTool, TB_SETSTATE, ID_STOP,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_PAUSE, MF_ENABLED);
            EnableMenuItem(hShortcutMenu, ID_PAUSE, MF_ENABLED);
            SendMessage(hTool, TB_SETSTATE, ID_PAUSE,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }

        if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
        {
            EnableMenuItem(hMainMenu, ID_RESTART, MF_ENABLED);
            EnableMenuItem(hShortcutMenu, ID_RESTART, MF_ENABLED);
            SendMessage(hTool, TB_SETSTATE, ID_RESTART,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
        }
    }
    else
    {
        EnableMenuItem(hMainMenu, ID_PROP, MF_GRAYED);
        EnableMenuItem(hMainMenu, ID_DELETE, MF_GRAYED);
        EnableMenuItem(hShortcutMenu, ID_DELETE, MF_GRAYED);
        SendMessage(hTool, TB_SETSTATE, ID_PROP,
                   (LPARAM)MAKELONG(TBSTATE_INDETERMINATE, 0));
    }

}




LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            TBADDBITMAP tbab;
            INT iImageOffset;
            INT statwidths[] = {110, -1}; /* widths of status bar */
            TCHAR szTemp[256];
            LVCOLUMN lvc = { 0 };

            /* Toolbar buttons */
            TBBUTTON tbb [NUM_BUTTONS] =
            {   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
                {TBICON_PROP,    ID_PROP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0},    /* properties */
                {TBICON_REFRESH, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* refresh */
                {TBICON_EXPORT,  ID_EXPORT,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},          /* export */

                /* Note: First item for a seperator is its width in pixels */
                {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

                {TBICON_CREATE,  ID_CREATE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* create */

                {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

                {TBICON_START,   ID_START,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* start */
                {TBICON_STOP,    ID_STOP,    TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
                {TBICON_PAUSE,   ID_PAUSE,   TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* pause */
                {TBICON_RESTART, ID_RESTART, TBSTATE_INDETERMINATE, BTNS_BUTTON, {0}, 0, 0 },   /* restart */

                {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                                  /* separator */

                {TBICON_HELP,    ID_HELP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },         /* help */
                {TBICON_EXIT,    ID_EXIT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },          /* exit */

            };

            ptbb = tbb;

/* ======================== Create Toolbar ============================== */

            /* Create Toolbar */
            hTool = CreateWindowEx(0,
                                   TOOLBARCLASSNAME,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                   0, 0, 0, 0,
                                   hwnd,
                                   (HMENU)IDC_TOOLBAR,
                                   hInstance,
                                   NULL);
            if(hTool == NULL)
                MessageBox(hwnd, _T("Could not create tool bar."), _T("Error"), MB_OK | MB_ICONERROR);

            /* Send the TB_BUTTONSTRUCTSIZE message, which is required for backward compatibility */
            SendMessage(hTool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

            /* Add custom images */
            tbab.hInst = hInstance;
            tbab.nID = IDB_BUTTONS;
            iImageOffset = (INT)SendMessage(hTool, TB_ADDBITMAP, NUM_BUTTONS, (LPARAM)&tbab);
            tbb[0].iBitmap += iImageOffset; /* properties */
            tbb[1].iBitmap += iImageOffset; /* refresh */
            tbb[2].iBitmap += iImageOffset; /* export */
            tbb[4].iBitmap += iImageOffset; /* create */
            tbb[6].iBitmap += iImageOffset; /* start */
            tbb[7].iBitmap += iImageOffset; /* stop */
            tbb[8].iBitmap += iImageOffset; /* pause */
            tbb[9].iBitmap += iImageOffset; /* restart */
            tbb[11].iBitmap += iImageOffset; /* help */
            tbb[12].iBitmap += iImageOffset; /* exit */

            /* Add buttons to toolbar */
            SendMessage(hTool, TB_ADDBUTTONS, NUM_BUTTONS, (LPARAM) &tbb);

            /* Show toolbar */
            ShowWindow(hTool, SW_SHOWNORMAL);



/* ======================== Create List View ============================== */

            hListView = CreateWindowEx(0,
                                       WC_LISTVIEW,
                                       NULL,
                                       WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER |
                                       LBS_NOTIFY | LVS_SORTASCENDING | LBS_NOREDRAW,
                                       0, 0, 0, 0, /* sized via WM_SIZE */
                                       hwnd,
                                       (HMENU) IDC_SERVLIST,
                                       hInstance,
                                       NULL);
            if (hListView == NULL)
                MessageBox(hwnd, _T("Could not create List View."), _T("Error"), MB_OK | MB_ICONERROR);

            (void)ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT |
                    /*LVS_EX_GRIDLINES |*/ LVS_EX_HEADERDRAGDROP);

            lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
            lvc.fmt  = LVCFMT_LEFT;

            /* Add columns to the list-view */

            /* name */
            lvc.iSubItem = 0;
            lvc.cx       = 150;
            LoadString(hInstance, IDS_FIRSTCOLUMN, szTemp,
                sizeof(szTemp) / sizeof(TCHAR));
            lvc.pszText  = szTemp;
            (void)ListView_InsertColumn(hListView, 0, &lvc);

            /* description */
            lvc.iSubItem = 1;
            lvc.cx       = 240;
            LoadString(hInstance, IDS_SECONDCOLUMN, szTemp,
                sizeof(szTemp) / sizeof(TCHAR));
            lvc.pszText  = szTemp;
            (void)ListView_InsertColumn(hListView, 1, &lvc);

            /* status */
            lvc.iSubItem = 2;
            lvc.cx       = 55;
            LoadString(hInstance, IDS_THIRDCOLUMN, szTemp,
                sizeof(szTemp) / sizeof(TCHAR));
            lvc.pszText  = szTemp;
            (void)ListView_InsertColumn(hListView, 2, &lvc);

            /* startup type */
            lvc.iSubItem = 3;
            lvc.cx       = 80;
            LoadString(hInstance, IDS_FOURTHCOLUMN, szTemp,
                sizeof(szTemp) / sizeof(TCHAR));
            lvc.pszText  = szTemp;
            (void)ListView_InsertColumn(hListView, 3, &lvc);

            /* logon as */
            lvc.iSubItem = 4;
            lvc.cx       = 100;
            LoadString(hInstance, IDS_FITHCOLUMN, szTemp,
                sizeof(szTemp) / sizeof(TCHAR));
            lvc.pszText  = szTemp;
            (void)ListView_InsertColumn(hListView, 4, &lvc);



/* ======================== Create Status Bar ============================== */

		    hStatus = CreateWindowEx(0,
                                     STATUSCLASSNAME,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                     0, 0, 0, 0,
                                     hwnd,
                                     (HMENU)IDC_STATUSBAR,
                                     hInstance,
                                     NULL);
            if(hStatus == NULL)
			    MessageBox(hwnd, _T("Could not create status bar."),
                           _T("Error!"), MB_OK | MB_ICONERROR);

		    SendMessage(hStatus, SB_SETPARTS, sizeof(statwidths)/sizeof(int), (LPARAM)statwidths);


/* ======================== Create Popup Menu ============================== */

            hShortcutMenu = LoadMenu(hInstance, MAKEINTRESOURCE (IDR_POPUP));
            hShortcutMenu = GetSubMenu(hShortcutMenu, 0);




/* ================= populate the list view with all services =================== */

		    RefreshServiceList();

	    }
	    break;

	    case WM_SIZE:
	    {
		    RECT rcTool;
		    int iToolHeight;

		    RECT rcStatus;
		    int iStatusHeight;

		    int lvHeight;
		    RECT rcClient;

		    /* Size toolbar and get height */
            hTool = GetDlgItem(hwnd, IDC_TOOLBAR);
		    SendMessage(hTool, TB_AUTOSIZE, 0, 0);

		    GetWindowRect(hTool, &rcTool);
		    iToolHeight = rcTool.bottom - rcTool.top;

		    /* Size status bar and get height */
		    hStatus = GetDlgItem(hwnd, IDC_STATUSBAR);
		    SendMessage(hStatus, WM_SIZE, 0, 0);

		    GetWindowRect(hStatus, &rcStatus);
		    iStatusHeight = rcStatus.bottom - rcStatus.top;

		    /* Calculate remaining height and size list view */
		    GetClientRect(hwnd, &rcClient);

		    lvHeight = rcClient.bottom - iToolHeight - iStatusHeight;

		    hListView = GetDlgItem(hwnd, IDC_SERVLIST);
		    SetWindowPos(hListView, NULL, 0, iToolHeight, rcClient.right, lvHeight, SWP_NOZORDER);
	    }
	    break;

	    case WM_NOTIFY:
        {
            NMHDR* nm = (NMHDR*) lParam;

            switch (nm->code)
            {
	            case NM_DBLCLK:
                    OpenPropSheet(hwnd);
			    break;

			    case LVN_COLUMNCLICK:

                break;

			    case LVN_ITEMCHANGED:
			    {
			        LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
			        ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
			        HMENU hMainMenu;

                    /* get handle to menu */
                    hMainMenu = GetMenu(hMainWnd);

                    /* activate properties menu item, if not already */
                    if (GetMenuState(hMainMenu, ID_PROP, MF_BYCOMMAND) != MF_ENABLED)
                        EnableMenuItem(hMainMenu, ID_PROP, MF_ENABLED);

                    /* activate delete menu item, if not already */
                    if (GetMenuState(hMainMenu, ID_DELETE, MF_BYCOMMAND) != MF_ENABLED)
                    {
                        EnableMenuItem(hMainMenu, ID_DELETE, MF_ENABLED);
                        EnableMenuItem(hShortcutMenu, ID_DELETE, MF_ENABLED);
                    }


                    /* globally set selected service */
			        SelectedItem = pnmv->iItem;

                    /* alter options for the service */
			        SetMenuAndButtonStates();

			        /* get pointer to selected service */
                    Service = GetSelectedService();

			        /* set current selected service in the status bar */
                    SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)Service->lpDisplayName);

                    /* show the properties button */
                    SendMessage(hTool, TB_SETSTATE, ID_PROP,
                        (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

			    }
			    break;

                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT) lParam;

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
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_NEW);
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

                default:
                break;
            }
        }
        break;

        case WM_CONTEXTMENU:
            {
                int xPos, yPos;

                xPos = GET_X_LPARAM(lParam);
                yPos = GET_Y_LPARAM(lParam);

                TrackPopupMenuEx(hShortcutMenu, TPM_RIGHTBUTTON,
                                 xPos, yPos, hwnd, NULL);
            }
        break;

	    case WM_COMMAND:

		    switch(LOWORD(wParam))
		    {
                case ID_PROP:
                    if (GetSelectedItem() != -1)
                        OpenPropSheet(hwnd);

                break;

                case ID_REFRESH:
                    RefreshServiceList();
                    SelectedItem = -1;

                    /* disable menus and buttons */
                    SetMenuAndButtonStates();

                    /* clear the service in the status bar */
                    SendMessage(hStatus, SB_SETTEXT, 1, _T('\0'));

                break;

                case ID_EXPORT:
                    ExportFile(hListView);
                    SetFocus(hListView);
                break;

                case ID_CREATE:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_DLG_CREATE),
                              hMainWnd,
                              (DLGPROC)CreateDialogProc);
                    SetFocus(hListView);
                break;

                case ID_DELETE:
                {
                    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;

                    Service = GetSelectedService();

                    if (Service->ServiceStatusProcess.dwCurrentState !=
                        SERVICE_RUNNING)
                    {
                        DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_DLG_DELETE),
                              hMainWnd,
                              (DLGPROC)DeleteDialogProc);
                    }
                    else
                    {
                        TCHAR Buf[60];
                        LoadString(hInstance, IDS_DELETE_STOP, Buf,
                            sizeof(Buf) / sizeof(TCHAR));
                        DisplayString(Buf);
                    }

                    SetFocus(hListView);

                }
                break;

                case ID_START:
                {
                    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
                    TCHAR ProgDlgBuf[100];

                    /* open the progress dialog */
                    hProgDlg = CreateDialog(GetModuleHandle(NULL),
                                            MAKEINTRESOURCE(IDD_DLG_PROGRESS),
                                            hMainWnd,
                                            (DLGPROC)ProgressDialogProc);
                    if (hProgDlg != NULL)
                    {
                        ShowWindow(hProgDlg, SW_SHOW);

                        /* write the  info to the progress dialog */
                        LoadString(hInstance, IDS_PROGRESS_INFO_START, ProgDlgBuf,
                                   sizeof(ProgDlgBuf) / sizeof(TCHAR));
                        SendDlgItemMessage(hProgDlg, IDC_SERVCON_INFO, WM_SETTEXT,
                                           0, (LPARAM)ProgDlgBuf);

                        /* get pointer to selected service */
                        Service = GetSelectedService();

                        /* write the service name to the progress dialog */
                        SendDlgItemMessage(hProgDlg, IDC_SERVCON_NAME, WM_SETTEXT, 0,
                            (LPARAM)Service->lpServiceName);
                    }

                    if ( DoStartService(hProgDlg) )
                    {
                        LVITEM item;
                        TCHAR szStatus[64];
                        TCHAR buf[25];

                        LoadString(hInstance, IDS_SERVICES_STARTED, szStatus,
                            sizeof(szStatus) / sizeof(TCHAR));
                        item.pszText = szStatus;
                        item.iItem = GetSelectedItem();
                        item.iSubItem = 2;
                        SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);

                        /* change dialog status */
                        if (hwndGenDlg)
                        {
                            LoadString(hInstance, IDS_SERVICES_STARTED, buf,
                                       sizeof(buf) / sizeof(TCHAR));
                            SendDlgItemMessageW(hwndGenDlg, IDC_SERV_STATUS, WM_SETTEXT,
                                                0, (LPARAM)buf);
                        }
                    }

                    SendMessage(hProgDlg, WM_DESTROY, 0, 0);

                }
			    break;

                case ID_STOP:
                {
                    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
                    TCHAR ProgDlgBuf[100];

                    /* open the progress dialog */
                    hProgDlg = CreateDialog(GetModuleHandle(NULL),
                                            MAKEINTRESOURCE(IDD_DLG_PROGRESS),
                                            hMainWnd,
                                            (DLGPROC)ProgressDialogProc);
                    if (hProgDlg != NULL)
                    {
                        ShowWindow(hProgDlg, SW_SHOW);

                        /* write the  info to the progress dialog */
                        LoadString(hInstance, IDS_PROGRESS_INFO_STOP, ProgDlgBuf,
                            sizeof(ProgDlgBuf) / sizeof(TCHAR));
                        SendDlgItemMessage(hProgDlg, IDC_SERVCON_INFO,
                            WM_SETTEXT, 0, (LPARAM)ProgDlgBuf);

                        /* get pointer to selected service */
                        Service = GetSelectedService();

                        /* write the service name to the progress dialog */
                        SendDlgItemMessage(hProgDlg, IDC_SERVCON_NAME, WM_SETTEXT, 0,
                            (LPARAM)Service->lpServiceName);
                    }

                    if( Control(hProgDlg, SERVICE_CONTROL_STOP) )
                    {
                        LVITEM item;
                        TCHAR buf[25];

                        item.pszText = _T('\0');
                        item.iItem = GetSelectedItem();
                        item.iSubItem = 2;
                        SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);

                        /* change dialog status */
                        if (hwndGenDlg)
                        {
                            LoadString(hInstance, IDS_SERVICES_STOPPED, buf,
                                       sizeof(buf) / sizeof(TCHAR));
                            SendDlgItemMessageW(hwndGenDlg, IDC_SERV_STATUS, WM_SETTEXT,
                                                0, (LPARAM)buf);
                        }
                    }

                    SendMessage(hProgDlg, WM_DESTROY, 0, 0);

                }
                break;

                case ID_PAUSE:
                    Control(hProgDlg, SERVICE_CONTROL_PAUSE);
                break;

                case ID_RESUME:
                    Control(hProgDlg, SERVICE_CONTROL_CONTINUE );
                break;

                case ID_RESTART:
                    SendMessage(hMainWnd, WM_COMMAND, 0, ID_STOP);
                    SendMessage(hMainWnd, WM_COMMAND, 0, ID_START);
                break;

                case ID_HELP:
                    MessageBox(NULL, _T("Help is not yet implemented\n"),
                        _T("Note!"), MB_OK | MB_ICONINFORMATION);
                    SetFocus(hListView);
                break;

                case ID_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;

                case ID_VIEW_LARGE:
                    SetView(LVS_ICON);
                break;

                case ID_VIEW_SMALL:
                    SetView(LVS_SMALLICON);
                break;

                case ID_VIEW_LIST:
                    SetView(LVS_LIST);
                break;

                case ID_VIEW_DETAILS:
                    SetView(LVS_REPORT);
                break;

                case ID_VIEW_CUSTOMIZE:
                break;

                case ID_ABOUT:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_ABOUTBOX),
                              hMainWnd,
                              (DLGPROC)AboutDialogProc);
                    SetFocus(hListView);
                break;

		    }
	    break;

	    case WM_CLOSE:
            FreeMemory(); /* free the service array */
            DestroyMenu(hShortcutMenu);
		    DestroyWindow(hwnd);
	    break;

	    case WM_DESTROY:
		    PostQuitMessage(0);
	    break;

	    default:
		    return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    MSG Msg;

    hInstance = hThisInstance;

    InitCommonControls();

    wc.cbSize		 = sizeof(WNDCLASSEX);
    wc.style		 = 0;
    wc.lpfnWndProc	 = WndProc;
    wc.cbClsExtra	 = 0;
    wc.cbWndExtra	 = 0;
    wc.hInstance	 = hInstance;
    wc.hIcon		 = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
    wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = ClassName;
    wc.hIconSm		 = (HICON)LoadImage(hInstance,
                        MAKEINTRESOURCE(IDI_SM_ICON), IMAGE_ICON, 16, 16, 0);

    if(!RegisterClassEx(&wc))
    {
	    MessageBox(NULL, _T("Window Registration Failed!"), _T("Error!"),
		    MB_ICONEXCLAMATION | MB_OK);
	    return 0;
    }

    hMainWnd = CreateWindowEx(
	    0,
	    ClassName,
	    _T("ReactOS Service Manager"),
	    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	    CW_USEDEFAULT, CW_USEDEFAULT, 650, 450,
	    NULL, NULL, hInstance, NULL);

    if(hMainWnd == NULL)
    {
	    MessageBox(NULL, _T("Window Creation Failed!"), _T("Error!"),
		    MB_ICONEXCLAMATION | MB_OK);
	    return 0;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    while( GetMessage( &Msg, NULL, 0, 0 ) )
    {
        if(! IsDialogMessage(hProgDlg, &Msg) )
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }

    }
    return (int)Msg.wParam;
}



