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


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
             //HFONT hfDefault;

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
                {25, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                             /* separator */

                {TBICON_START,   ID_START,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* start */ 
                {TBICON_STOP,    ID_STOP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
                {TBICON_PAUSE,   ID_PAUSE,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* pause */   
                {TBICON_RESTART, ID_RESTART, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* restart */

                {25, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                             /* separator */

                {TBICON_NEW,     ID_NEW,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* start */
                {TBICON_HELP,    ID_HELP,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* stop */
                {TBICON_EXIT,    ID_EXIT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* pause */

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

            /* Add standard image list */
            tbab.hInst = HINST_COMMCTRL;
            tbab.nID = IDB_STD_SMALL_COLOR;
            SendMessage(hTool, TB_ADDBITMAP, 0, (LPARAM) &tbab);

            /* Add custom images */
            tbab.hInst = hInstance;
            tbab.nID = IDB_BUTTONS;
            iImageOffset = (INT)SendMessage(hTool, TB_ADDBITMAP, 11, (LPARAM)&tbab);
            tbb[0].iBitmap += iImageOffset; /* properties */
            tbb[1].iBitmap += iImageOffset; /* refresh */
            tbb[2].iBitmap += iImageOffset; /* export */
            tbb[4].iBitmap += iImageOffset; /* start */
            tbb[5].iBitmap += iImageOffset; /* stop */
            tbb[6].iBitmap += iImageOffset; /* pause */
            tbb[7].iBitmap += iImageOffset; /* restart */
            tbb[9].iBitmap += iImageOffset; /* new */
            tbb[10].iBitmap += iImageOffset; /* help */
            tbb[11].iBitmap += iImageOffset; /* exit */

            /* Add buttons to toolbar */
            SendMessage(hTool, TB_ADDBUTTONS, NUM_BUTTONS, (LPARAM) &tbb);

            /* Show toolbar */
            ShowWindow(hTool, SW_SHOWNORMAL);



/* ======================== Create List View ============================== */

            hListView = CreateWindow(WC_LISTVIEW,
                                     NULL,
                                     WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | 
                                     LVS_EDITLABELS | LVS_SORTASCENDING,
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
            lvc.cx       = 160;
            LoadString(hInstance, IDS_FIRSTCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 0, &lvc);

            /* description */
            lvc.iSubItem = 1;
            lvc.cx       = 260;
            LoadString(hInstance, IDS_SECONDCOLUMN, szTemp, 256);
            lvc.pszText  = szTemp;
            ListView_InsertColumn(hListView, 1, &lvc);

            /* status */
            lvc.iSubItem = 2;
            lvc.cx       = 75;
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

		    /* populate the list view with all services */
		    if (! RefreshServiceList() )
                GetError();

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
            LPNMITEMACTIVATE item;

            switch (((LPNMHDR) lParam)->code)
            {

	            case NM_DBLCLK:
			    item = (LPNMITEMACTIVATE) lParam;
			    PropSheets(hwnd);

			    break;
            
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT) lParam;
                    lpttt->hinst = hInstance;

                    // Specify the resource identifier of the descriptive
                    // text for the given button.
                    idButton = lpttt->hdr.idFrom;
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

	    case WM_CLOSE:
            /* free the service array */
            FreeMemory();
		    DestroyWindow(hwnd);
	    break;

	    case WM_DESTROY:
		    PostQuitMessage(0);
	    break;

	    case WM_COMMAND:
		    switch(LOWORD(wParam))
		    {
                case ID_PROP:
                    PropSheets(hwnd);
                break;
                
                case ID_REFRESH:
                    if (! RefreshServiceList() )
                        GetError();

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
                break;

                case ID_EXIT:
				    PostMessage(hwnd, WM_CLOSE, 0, 0);
			    break;

                case ID_VIEW_CUSTOMIZE:
                break;

                case ID_HELP_ABOUT:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_ABOUTBOX),
                              hwnd,
                              AboutDialogProc);
                 break;

		    }
	    break;

	    default:
		    return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


//#pragma warning(disable : 4100)
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
	    CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
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
    return Msg.wParam;
}


