#include "imagesoft.h"

#define ID_MDI_FIRSTCHILD 50000

const TCHAR AppClassName[]   = _T("Parent");
const TCHAR ChildClassName[] = _T("Child");


HINSTANCE hInstance;
HWND hMainWnd;
HWND hMDIClient;
HWND hStatus;
HWND hTool;
HWND hwndRebar;
HMENU hShortcutMenu;


/*
 * Initialize the structure and send a message to the MDI
 * frame requesting a new new child window.
 */
HWND CreateNewMDIChild(HWND hMDIClient)
{
    MDICREATESTRUCT mcs;
    HWND hChild;
    TCHAR Buf[15];
    static DWORD MDINum = 1;

    _sntprintf(Buf, sizeof(Buf) / sizeof(TCHAR), _T("Untitled%d"), MDINum);

    mcs.szTitle = Buf;
    mcs.szClass = ChildClassName;
    mcs.hOwner  = hInstance;
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;
    mcs.style = MDIS_ALLCHILDSTYLES;

    hChild = (HWND)SendMessage(hMDIClient, WM_MDICREATE, 0, (LONG)&mcs);
    if(!hChild)
    {
        MessageBox(hMDIClient, _T("MDI Child creation failed."), _T("Error!"),
            MB_ICONEXCLAMATION | MB_OK);
        return hChild;
    }

    MDINum++;
    return hChild;
}


/*
 * Main program message handler
 */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            CLIENTCREATESTRUCT ccs;
            TBADDBITMAP tbab;
            INT iImageOffset;
            INT statwidths[] = {300, 450, 550, -1}; /* widths of status bar */
            TCHAR Buf[6];

            /* Toolbar buttons */
            TBBUTTON tbb [NUM_BUTTONS] =
            {   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
                {STD_FILENEW,  ID_NEW,      TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},    /* new */
                {STD_FILEOPEN, ID_OPEN,     TBSTATE_ENABLED, BTNS_BUTTON,    {0}, 0, 0},    /* open */
                {STD_FILESAVE, ID_SAVE,     TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0},    /* save */

                /* Note: First item for a seperator is its width in pixels */
                {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

                {STD_PRINTPRE, ID_PRINTPRE, TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* print */
                {STD_PRINT,    ID_PRINT,    TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* print preview */

                {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

                {STD_CUT,      ID_CUT,      TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* cut */
                {STD_COPY,     ID_COPY,     TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* copy */
                {STD_PASTE,    ID_PASTE,    TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* paste */

                {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

                {STD_UNDO,     ID_UNDO,    TBSTATE_INDETERMINATE, BTNS_BUTTON,     {0}, 0, 0 },   /* undo */
                {STD_REDOW,    ID_REDO,    TBSTATE_INDETERMINATE, BTNS_BUTTON,     {0}, 0, 0 },   /* redo */
            };


/* ======================== Create Std Toolbar ============================== */

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
                MessageBox(hwnd, _T("Could not create tool bar."), _T("Error!"), MB_OK | MB_ICONERROR);

            /* Send the TB_BUTTONSTRUCTSIZE message, which is required for backward compatibility */
            SendMessage(hTool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

            /* Add custom images */
            tbab.hInst = HINST_COMMCTRL;
            tbab.nID = IDB_STD_SMALL_COLOR;
            iImageOffset = (INT)SendMessage(hTool, TB_ADDBITMAP, NUM_BUTTONS, (LPARAM)&tbab);
 /*           tbb[0].iBitmap += iImageOffset; / * properties * /
            tbb[1].iBitmap += iImageOffset; / * refresh * /
            tbb[2].iBitmap += iImageOffset; / * export * /
            tbb[4].iBitmap += iImageOffset; / * create * /
            tbb[6].iBitmap += iImageOffset; / * start * /
            tbb[7].iBitmap += iImageOffset; / * stop * /
            tbb[8].iBitmap += iImageOffset; / * pause * /
            tbb[9].iBitmap += iImageOffset; / * restart * /
            tbb[11].iBitmap += iImageOffset; / * help * /
            tbb[12].iBitmap += iImageOffset; / * exit * /
*/
            /* Add buttons to toolbar */
            SendMessage(hTool, TB_ADDBUTTONS, NUM_BUTTONS, (LPARAM) &tbb);

            /* Show toolbar */
            ShowWindow(hTool, SW_SHOWNORMAL);



/* ======================== Create Floating Toolbar ============================== */

 


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


/* ======================= Create MDI Client ============================= */

            /* Find window menu where children will be listed */
            ccs.hWindowMenu  = GetSubMenu(GetMenu(hwnd), 4);
            ccs.idFirstChild = ID_MDI_FIRSTCHILD;

            hMDIClient = CreateWindowEx(WS_EX_CLIENTEDGE,
                                        _T("mdiclient"),
                                        NULL,
                                        WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        hwnd,
                                        (HMENU)IDC_MAIN_MDI,
                                        GetModuleHandle(NULL),
                                        (LPVOID)&ccs);


            if(hMDIClient == NULL)
                MessageBox(hwnd, _T("Could not create MDI client."),
                           _T("Error!"), MB_OK | MB_ICONERROR);


/* ======================= Miscelaneous ============================= */

                /* indicate program is ready in the status bar */
                LoadString(hInstance, IDS_READY, Buf, sizeof(Buf) / sizeof(TCHAR));
                SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)Buf);

                /* inilalize file open/save structure */
                FileInitialize(hwnd);

	    }
	    break;

	    case WM_SIZE:
	    {
            RECT rcTool;
            int iToolHeight;

            RECT rcStatus;
            int iStatusHeight;

            HWND hMDI;
            int iMDIHeight;
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

            /* Calculate remaining height and size for the MDI frame */
            GetClientRect(hwnd, &rcClient);

            iMDIHeight = rcClient.bottom - iToolHeight - iStatusHeight;

            hMDI = GetDlgItem(hwnd, IDC_MAIN_MDI);
            SetWindowPos(hMDIClient, NULL, 0, iToolHeight, rcClient.right, iMDIHeight, SWP_NOZORDER);
        }
        break;

        case WM_NOTIFY:
        {
            NMHDR* nm = (NMHDR*) lParam;

            switch (nm->code)
            {
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
                        case ID_NEW:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_NEW);
                        break;

                        case ID_OPEN:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_OPEN);
                        break;

                        case ID_SAVE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SAVE);
                        break;

                        case ID_PRINTPRE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PRINTPRE);
                        break;

                        case ID_PRINT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PRINT);
                        break;

                        case ID_CUT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_CUT);
                        break;

                        case ID_COPY:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_COPY);
                        break;

                        case ID_PASTE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PASTE);
                        break;

                        case ID_UNDO:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_UNDO);
                        break;

                        case ID_REDO:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REDO);
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
                case ID_NEW:
                    CreateNewMDIChild(hMDIClient);
                break;

                case ID_OPEN:
                    DoOpenFile(hwnd);
                break;

                case ID_SAVEAS:
                    DoSaveFile(hwnd);
                break;

                case ID_CLOSE:
                {
                    /* close the active child window */
                    HWND hChild = (HWND)SendMessage(hMDIClient, WM_MDIGETACTIVE,0,0);
                    if(hChild)
                    {
                        SendMessage(hChild, WM_CLOSE, 0, 0);
                    }
                }
				break;

                case ID_CLOSEALL:
                {
                    HWND hChild;
                    /* loop until all windows have been closed */
                    while ((hChild = (HWND)SendMessage(hMDIClient, WM_MDIGETACTIVE,0,0)) != NULL)
                    {
                        SendMessage(hChild, WM_CLOSE, 0, 0);
                    }
                }
				break;

                case ID_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;

                case ID_EDITCOLOURS:
                {
                    /* open up the colour selection dialog */

                    static CHOOSECOLOR cc;
                    static COLORREF crCustColors[16];

                    cc.lStructSize    = sizeof(CHOOSECOLOR);
                    cc.hwndOwner      = hwnd;
                    cc.hInstance      = NULL;
                    cc.rgbResult      = RGB(0x80, 0x80, 0x80);
                    cc.lpCustColors   = crCustColors;
                    cc.Flags          = CC_RGBINIT | CC_FULLOPEN;
                    cc.lCustData      = 0;
                    cc.lpfnHook       = NULL;
                    cc.lpTemplateName = NULL;

                    ChooseColor(&cc);
                }
                break;

				case ID_WINDOW_TILE:
					SendMessage(hMDIClient, WM_MDITILE, 0, 0);
				break;

				case ID_WINDOW_CASCADE:
					SendMessage(hMDIClient, WM_MDICASCADE, 0, 0);
				break;

                case ID_ABOUT:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_ABOUTBOX),
                              hMainWnd,
                              (DLGPROC)AboutDialogProc);
                break;

                default:
                    /* Catch all commands that I didn't process directly and do
                     * a check to see if the value is greater than or equal to
                     * ID_MDI_FIRSTCHILD. If it is, then the user has clicked
                     * on one of the Window menu items and we send the message
                     * on to DefFrameProc() for processing.
                     */
                    if(LOWORD(wParam) >= ID_MDI_FIRSTCHILD)
                        DefFrameProc(hwnd, hMDIClient, WM_COMMAND, wParam, lParam);
                    else
                    {
                        HWND hChild = (HWND)SendMessage(hMDIClient, WM_MDIGETACTIVE,0,0);
                        if(hChild)
                            SendMessage(hChild, WM_COMMAND, wParam, lParam);
                    }
		    }
	    break;

        case WM_CLOSE:
            DestroyMenu(hShortcutMenu);
            DestroyWindow(hwnd);
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
        break;

        default:
            return DefFrameProc(hwnd, hMDIClient, msg, wParam, lParam);
    }
    return 0;
}


/*
 * MDI child window message handler
 */
LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL    fLeftButtonDown, fRightButtonDown;
    static HDC     hdcMem;
    static INT     cxClient, cyClient, xMouse, yMouse;
    HDC            hdc;
    PAINTSTRUCT    ps;

	switch(msg)
	{
        case WM_CREATE:
        break;

        case WM_MDIACTIVATE:
        {
            HMENU hMenu, hFileMenu;
            UINT EnableFlag;

            hMenu = GetMenu(hMainWnd);
            if(hwnd == (HWND)lParam)
            {	/* being activated, enable the menus */
                EnableFlag = MF_ENABLED;
            }
            else
            {
                TCHAR Buf[6];
                /* being de-activated, gray the menus */
                EnableFlag = MF_GRAYED;

                /* indicate program is ready in the status bar */
                LoadString(hInstance, IDS_READY, Buf, sizeof(Buf) / sizeof(TCHAR));
                SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)Buf);
            }

            EnableMenuItem(hMenu, 1, MF_BYPOSITION | EnableFlag); /* edit */
            EnableMenuItem(hMenu, 2, MF_BYPOSITION | EnableFlag); /* image */
            EnableMenuItem(hMenu, 3, MF_BYPOSITION | EnableFlag); /* colours */
            EnableMenuItem(hMenu, 4, MF_BYPOSITION | EnableFlag); /* window */

            hFileMenu = GetSubMenu(hMenu, 0);
            EnableMenuItem(hFileMenu, ID_SAVEAS, MF_BYCOMMAND | EnableFlag);

            EnableMenuItem(hFileMenu, ID_CLOSE, MF_BYCOMMAND | EnableFlag);
            EnableMenuItem(hFileMenu, ID_CLOSEALL, MF_BYCOMMAND | EnableFlag);

            SendMessage(hTool, TB_SETSTATE, ID_COPY,
                   (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));

            DrawMenuBar(hMainWnd);
        }
        break;

        case WM_MOUSEMOVE:
        {
            POINT pt;
            TCHAR Buf[200];
            TCHAR Cur[15];

            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            /* set cursor location in the status bar */
            LoadString(hInstance, IDS_CURPOS, Cur, sizeof(Cur) / sizeof(TCHAR));
            _sntprintf(Buf, sizeof(Buf) / sizeof(TCHAR), Cur, pt.x, pt.y);
            SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)Buf);

            if (!fLeftButtonDown && !fRightButtonDown)
                return 0;

            hdc = GetDC(hwnd);

            SelectObject(hdc,
                GetStockObject(fLeftButtonDown ? BLACK_PEN : WHITE_PEN) );

            SelectObject(hdcMem,
                GetStockObject(fLeftButtonDown ? BLACK_PEN : WHITE_PEN) );

            MoveToEx (hdc, xMouse, yMouse, NULL);
            MoveToEx (hdcMem, xMouse, yMouse, NULL);

            xMouse = (short) LOWORD(lParam);
            yMouse = (short) HIWORD(lParam);

            LineTo(hdc, xMouse, yMouse);
            LineTo(hdcMem, xMouse, yMouse);

            ReleaseDC(hwnd, hdc);
        }
        break;

        case WM_LBUTTONDOWN:
              if (!fRightButtonDown)
                  SetCapture(hwnd);

              xMouse = LOWORD(lParam);
              yMouse = HIWORD(lParam);
              fLeftButtonDown = TRUE;
        break;

        case WM_LBUTTONUP:
            if (fLeftButtonDown)
                SetCapture(NULL);

            fLeftButtonDown = FALSE;
        break;

        case WM_RBUTTONDOWN:
            if (!fLeftButtonDown)
                SetCapture(hwnd);

            xMouse = LOWORD(lParam);
            yMouse = HIWORD(lParam);
            fRightButtonDown = TRUE;
        break;

        case WM_RBUTTONUP:
              if (fRightButtonDown)
                  SetCapture(NULL);

              fRightButtonDown = FALSE;
        break;

        case WM_PAINT:
                hdc = BeginPaint(hwnd, &ps);

                BitBlt(hdc, 0, 0, cxClient, cyClient, hdcMem, 0, 0, SRCCOPY);

                EndPaint(hwnd, &ps);
        break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{

			}
		break;

		case WM_SIZE:
            return DefMDIChildProc(hwnd, msg, wParam, lParam);

		default:
		{
            TCHAR Buf[6];

            /* indicate program is ready in the status bar */
                LoadString(hInstance, IDS_READY, Buf, sizeof(Buf) / sizeof(TCHAR));
                SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)Buf);

			return DefMDIChildProc(hwnd, msg, wParam, lParam);
		}

	}
	return 0;
}

/*
 * Register the MDI child window class
 */
BOOL SetUpMDIChildWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wc;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MDIChildWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = ChildClassName;
    wc.hIconSm       = (HICON)LoadImage(hInstance,
                        MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, 0);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(0, _T("Could Not Register Child Window"), _T("Error!"),
            MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    else
        return TRUE;
}

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    MSG Msg;
    INITCOMMONCONTROLSEX icex;

    hInstance = hThisInstance;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    wc.cbSize		 = sizeof(WNDCLASSEX);
    wc.style		 = 0;
    wc.lpfnWndProc	 = WndProc;
    wc.cbClsExtra	 = 0;
    wc.cbWndExtra	 = 0;
    wc.hInstance	 = hInstance;
    wc.hIcon		 = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = AppClassName;
    wc.hIconSm		 = (HICON)LoadImage(hInstance,
                        MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, 0);

    if(!RegisterClassEx(&wc))
    {
	    MessageBox(NULL, _T("Window Registration Failed!"), _T("Error!"),
		    MB_ICONEXCLAMATION | MB_OK);
	    return 0;
    }

    if(!SetUpMDIChildWindowClass(hInstance))
    return 0;

    hMainWnd = CreateWindowEx(0,
                              AppClassName,
                              _T("ImageSoft"),
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
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
        if (!TranslateMDISysAccel(hMDIClient, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    return (int)Msg.wParam;
}
