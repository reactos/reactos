/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsys/system/servman/servman.c
 * PURPOSE:     Main window message handler
 * COPYRIGHT:   Copyright 2005 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

const TCHAR ClassName[] = _T("ServiceManager");

HINSTANCE hInstance;
HWND hMainWnd;
HWND hListView;
HWND hStatus;
HWND hTool;
HMENU hShortcutMenu;
INT SelectedItem;


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
                {TBICON_PROP,    ID_PROP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* properties */
                {TBICON_REFRESH, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* refresh */
                {TBICON_EXPORT,  ID_EXPORT,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* export */

                /* Note: First item for a seperator is its width in pixels */
                {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                            /* separator */

                {TBICON_NEW,     ID_NEW,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },     /* create */

                {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                            /* separator */

                {TBICON_START,   ID_START,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* start */
                {TBICON_STOP,    ID_STOP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
                {TBICON_PAUSE,   ID_PAUSE,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* pause */
                {TBICON_RESTART, ID_RESTART, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* restart */

                {15, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                            /* separator */

                {TBICON_HELP,    ID_HELP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* help */
                {TBICON_EXIT,    ID_EXIT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },    /* exit */

            };

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
            tbb[4].iBitmap += iImageOffset; /* new */
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

            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT |
                    LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

            lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
            lvc.fmt  = LVCFMT_LEFT;

            /* Add columns to the list-view */

            /* name */
            lvc.iSubItem = 0;
            lvc.cx       = 150;
            LoadString(hInstance, IDS_FIRSTCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 0, &lvc);

            /* description */
            lvc.iSubItem = 1;
            lvc.cx       = 240;
            LoadString(hInstance, IDS_SECONDCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 1, &lvc);

            /* status */
            lvc.iSubItem = 2;
            lvc.cx       = 55;
            LoadString(hInstance, IDS_THIRDCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 2, &lvc);

            /* startup type */
            lvc.iSubItem = 3;
            lvc.cx       = 80;
            LoadString(hInstance, IDS_FOURTHCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 3, &lvc);

            /* logon as */
            lvc.iSubItem = 4;
            lvc.cx       = 100;
            LoadString(hInstance, IDS_FITHCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 4, &lvc);



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

			    case LVN_ITEMCHANGED:
			    {
			        LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

			        SelectedItem = pnmv->iItem;

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

                        case ID_NEW:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_NEW);
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
                    OpenPropSheet(hwnd);
                break;

                case ID_REFRESH:
                    RefreshServiceList();
                break;

                case ID_EXPORT:
                break;

                case ID_START:
			    break;

                case ID_STOP:
                break;

                case ID_PAUSE:
                break;

                case ID_RESUME:
                break;

                case ID_RESTART:
                break;

                case ID_NEW:
                break;

                case ID_HELP:
                    MessageBox(NULL, _T("Help is not yet implemented\n"),
                        _T("Note!"), MB_OK | MB_ICONINFORMATION);
                break;

                case ID_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;

                case ID_VIEW_CUSTOMIZE:
                break;

                case ID_ABOUT:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_ABOUTBOX),
                              hwnd,
                              AboutDialogProc);
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
    BOOL bRet;

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

    while( (bRet = GetMessage( &Msg, NULL, 0, 0 )) != 0)
    {
        if (bRet == -1)
        {
            /* handle the error and possibly exit */
        }
        else
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    return (int)Msg.wParam;
}


