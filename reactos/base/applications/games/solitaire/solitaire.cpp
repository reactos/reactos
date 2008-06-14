#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdlib.h>

#include "resource.h"
#include "cardlib/cardlib.h"

#include "solitaire.h"

TCHAR szHelpPath[MAX_PATH];

DWORD        dwAppStartTime;
HWND        hwndMain;
HWND        hwndStatus;
HINSTANCE    hInstance;

TCHAR szAppName[128];
TCHAR MsgQuit[128];
TCHAR MsgAbout[128];
TCHAR MsgWin[128];
DWORD dwOptions = 8;

CardWindow SolWnd;

typedef struct _CardBack
{
    HWND hSelf;
    WNDPROC hOldProc;
    INT hdcNum;
    INT imgNum;
    BOOL bSelected;
} CARDBACK, *PCARDBACK;

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

void MakePath(TCHAR *szDest, UINT nDestLen, const TCHAR *szExt)
{
    TCHAR *ptr;

    ptr = szDest + GetModuleFileName(GetModuleHandle(0), szDest, nDestLen) - 1;
    while(*ptr-- != '.');
    lstrcpy(ptr + 1, szExt);
}

VOID LoadSettings(VOID)
{
    DWORD dwDisposition;
    DWORD dwSize;
    DWORD dwBack;
    HKEY hKey;

    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       _T("Software\\ReactOS\\Solitaire"),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ,
                       NULL,
                       &hKey,
                       &dwDisposition))
        return;

    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKey,
                    _T("Options"),
                    NULL,
                    NULL,
                    (LPBYTE)&dwOptions,
                    &dwSize);

    dwSize = sizeof(DWORD);
    RegQueryValueEx(hKey,
                    _T("Back"),
                    NULL,
                    NULL,
                    (LPBYTE)&dwBack,
                    &dwSize);
    SolWnd.SetBackCardIdx(dwBack);

    RegCloseKey(hKey);
}

VOID SaveSettings(VOID)
{
    DWORD dwDisposition;
    DWORD dwBack;
    HKEY hKey;

    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       _T("Software\\ReactOS\\Solitaire"),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       &dwDisposition))
        return;

    RegSetValueEx(hKey,
                  _T("Options"),
                  0,
                  REG_DWORD,
                  (CONST BYTE *)&dwOptions,
                  sizeof(DWORD));

    dwBack = SolWnd.GetBackCardIdx();
    RegSetValueEx(hKey,
                  _T("Back"),
                  0,
                  REG_DWORD,
                  (CONST BYTE *)&dwBack,
                  sizeof(DWORD));

    RegCloseKey(hKey);
}

//
//    Main entry point
//
int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR szCmdLine, int iCmdShow)
{
    HWND        hwnd;
    MSG            msg;
    WNDCLASS    wndclass;
    INITCOMMONCONTROLSEX ice;
    HACCEL        hAccelTable;

    hInstance = hInst;

    // Load application title
    LoadString(hInst, IDS_SOL_NAME, szAppName, sizeof(szAppName) / sizeof(szAppName[0]));
    // Load MsgBox() texts here to avoid loading them many times later
    LoadString(hInst, IDS_SOL_ABOUT, MsgAbout, sizeof(MsgAbout) / sizeof(MsgAbout[0]));
    LoadString(hInst, IDS_SOL_QUIT, MsgQuit, sizeof(MsgQuit) / sizeof(MsgQuit[0]));
    LoadString(hInst, IDS_SOL_WIN, MsgWin, sizeof(MsgWin) / sizeof(MsgWin[0]));

    //Window class for the main application parent window
    wndclass.style            = 0;//CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra        = 0;
    wndclass.cbWndExtra        = 0;
    wndclass.hInstance        = hInst;
    wndclass.hIcon            = LoadIcon (hInst, MAKEINTRESOURCE(IDI_SOLITAIRE));
    wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground    = (HBRUSH)NULL;
    wndclass.lpszMenuName    = MAKEINTRESOURCE(IDR_MENU1);
    wndclass.lpszClassName    = szAppName;

    RegisterClass(&wndclass);

    ice.dwSize = sizeof(ice);
    ice.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&ice);

    srand((unsigned)GetTickCount());//timeGetTime());

//    InitCardLib();

    LoadSettings();

    //Construct the path to our help file
    MakePath(szHelpPath, MAX_PATH, _T(".hlp"));

    hwnd = CreateWindow(szAppName,        // window class name
                szAppName,                // window caption
                WS_OVERLAPPEDWINDOW
                ,//|WS_CLIPCHILDREN,      // window style
                CW_USEDEFAULT,            // initial x position
                CW_USEDEFAULT,            // initial y position
                0,                        // The real size will be computed in WndProc through WM_GETMINMAXINFO
                0,                        // The real size will be computed in WndProc through WM_GETMINMAXINFO
                NULL,                     // parent window handle
                NULL,                     // use window class menu
                hInst,                    // program instance handle
                NULL);                    // creation parameters

    hwndMain = hwnd;

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    while(GetMessage(&msg, NULL,0,0))
    {
        if(!TranslateAccelerator(hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    SaveSettings();

    return msg.wParam;
}


BOOL CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_OPT_DRAWONE, IDC_OPT_DRAWTHREE,
                         (dwOptions & OPTION_THREE_CARDS) ? IDC_OPT_DRAWTHREE : IDC_OPT_DRAWONE);

        CheckDlgButton(hDlg,
                       IDC_OPT_STATUSBAR,
                       (dwOptions & OPTION_SHOW_STATUS) ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            dwOptions &= ~OPTION_THREE_CARDS;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_DRAWTHREE) == BST_CHECKED)
                dwOptions |= OPTION_THREE_CARDS;

            if (IsDlgButtonChecked(hDlg, IDC_OPT_STATUSBAR) == BST_CHECKED)
                dwOptions |= OPTION_SHOW_STATUS;
            else
                dwOptions &= ~OPTION_SHOW_STATUS;

            EndDialog(hDlg, TRUE);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

VOID ShowGameOptionsDlg(HWND hwnd)
{
    DWORD dwOldOptions = dwOptions;
    RECT rcMain, rcStatus;
    int nWidth, nHeight, nStatusHeight;

    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPTIONS), hwnd, OptionsDlgProc))
    {
        if ((dwOldOptions & OPTION_THREE_CARDS) != (dwOptions & OPTION_THREE_CARDS))
            NewGame();

        if ((dwOldOptions & OPTION_SHOW_STATUS) != (dwOptions & OPTION_SHOW_STATUS))
        {
            GetClientRect(hwndMain, &rcMain);
            nHeight = rcMain.bottom - rcMain.top;
            nWidth = rcMain.right - rcMain.left;

            if (dwOptions & OPTION_SHOW_STATUS)
            {
                RECT rc;

                ShowWindow(hwndStatus, SW_SHOW);
                GetWindowRect(hwndStatus, &rcStatus);
                nStatusHeight = rcStatus.bottom - rcStatus.top;
                MoveWindow(SolWnd, 0, 0, nWidth, nHeight-nStatusHeight, TRUE);
                MoveWindow(hwndStatus, 0, nHeight-nStatusHeight, nWidth, nHeight, TRUE);

                // Force the window to process WM_GETMINMAXINFO again
                GetWindowRect(hwndMain, &rc);
                SetWindowPos(hwndMain, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
            }
            else
            {
                ShowWindow(hwndStatus, SW_HIDE);
                MoveWindow(SolWnd, 0, 0, nWidth, nHeight, TRUE);
            }
        }
    }
}


LRESULT CALLBACK
CardImageWndProc(HWND hwnd,
                 UINT msg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PCARDBACK pCardBack = (PCARDBACK)GetWindowLongPtr(hwnd,
                                                      GWL_USERDATA);
    static WNDPROC hOldProc = NULL;

    if (!hOldProc && pCardBack)
        hOldProc = pCardBack->hOldProc;

    switch (msg)
    {
    case WM_PAINT:
    {
        HDC hdc;
        PAINTSTRUCT ps;
        HPEN hPen, hOldPen;
        HBRUSH hBrush, hOldBrush;
        RECT rc;

        hdc = BeginPaint(hwnd, &ps);

        if (pCardBack->bSelected)
        {
            hPen = CreatePen(PS_SOLID, 2, RGB(0,0,0));
        }
        else
        {
            DWORD Face = GetSysColor(COLOR_3DFACE);
            hPen = CreatePen(PS_SOLID, 2, Face);
        }

        GetClientRect(hwnd, &rc);
        hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        hOldPen = (HPEN)SelectObject(hdc, hPen);
        hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        Rectangle(hdc,
                  rc.left+1,
                  rc.top+1,
                  rc.right,
                  rc.bottom);

        StretchBlt(hdc,
                   2,
                   2,
                   CARDBACK_OPTIONS_WIDTH,
                   CARDBACK_OPTIONS_HEIGHT,
                   __hdcCardBitmaps,
                   pCardBack->hdcNum * __cardwidth,
                   0,
                   __cardwidth,
                   __cardheight,
                   SRCCOPY);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);

        EndPaint(hwnd, &ps);

        break;
    }

    case WM_LBUTTONDOWN:
        pCardBack->bSelected = pCardBack->bSelected ? FALSE : TRUE;
        break;
    }

    return CallWindowProc(hOldProc,
                          hwnd,
                          msg,
                          wParam,
                          lParam);
}


BOOL CALLBACK CardBackDlgProc(HWND hDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    static PCARDBACK pCardBacks = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        INT i, c;
        SIZE_T size = sizeof(CARDBACK) * NUM_CARDBACKS;

        pCardBacks = (PCARDBACK)HeapAlloc(GetProcessHeap(),
                                          0,
                                          size);

        for (i = 0, c = CARDBACK_START; c <= CARDBACK_END; i++, c++)
        {
            pCardBacks[i].hSelf = GetDlgItem(hDlg, c);
            pCardBacks[i].bSelected = FALSE;
            pCardBacks[i].hdcNum = CARDBACK_RES_START + i;
            pCardBacks[i].imgNum = i + 1;
            pCardBacks[i].hOldProc = (WNDPROC)SetWindowLongPtr(pCardBacks[i].hSelf,
                                                               GWLP_WNDPROC,
                                                               (LONG_PTR)CardImageWndProc);

            SetWindowLongPtr(pCardBacks[i].hSelf,
                             GWL_USERDATA,
                             (LONG_PTR)&pCardBacks[i]);
        }

        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            INT i, num = 0;
            for (i = 0; i < NUM_CARDBACKS; i++)
            {
                if (pCardBacks[i].bSelected)
                {
                    num = pCardBacks[i].imgNum;
                }
            }

            EndDialog(hDlg, LOWORD(wParam) == IDOK ? num : FALSE);
            HeapFree(GetProcessHeap(), 0, pCardBacks);
            return TRUE;
        }

        if (HIWORD(wParam) == STN_CLICKED)
        {
            INT i;
            RECT rc;
            for (i = 0; i < NUM_CARDBACKS; i++)
            {
                if (pCardBacks[i].hSelf == (HWND)lParam)
                {
                    pCardBacks[i].bSelected = TRUE;
                }
                else
                    pCardBacks[i].bSelected = FALSE;

                GetClientRect(pCardBacks[i].hSelf, &rc);
                InvalidateRect(pCardBacks[i].hSelf, &rc, TRUE);
            }

            break;
        }
    }

    return FALSE;
}


VOID ShowDeckOptionsDlg(HWND hwnd)
{
    INT cardBack;

    if ((cardBack = DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_CARDBACK),
                              hwnd,
                              CardBackDlgProc)))
    {
        SolWnd.SetBackCardIdx(CARDBACK_RES_START + (cardBack - 1));
        SolWnd.Redraw();
    }
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static int nWidth, nHeight, nStatusHeight;

    switch(iMsg)
    {
        case WM_CREATE:
        {
            int parts[] = { 100, -1 };
            RECT rcStatus;

            hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, _T("Ready"), hwnd, 0);

            //SendMessage(hwndStatus, SB_SIMPLE, (WPARAM)TRUE, 0);

            SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts);
            SendMessage(hwndStatus, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)"");

            SolWnd.Create(hwnd, WS_EX_CLIENTEDGE, WS_CHILD|WS_VISIBLE, 0, 0, 0, 0);

            CreateSol();

            // The status bar height is fixed and needed later in WM_SIZE and WM_GETMINMAXINFO
            // Force the window to process WM_GETMINMAXINFO again
            GetWindowRect(hwndStatus, &rcStatus);
            nStatusHeight = rcStatus.bottom - rcStatus.top;
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER);

            NewGame();

            dwAppStartTime = GetTickCount();

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            nWidth  = LOWORD(lParam);
            nHeight = HIWORD(lParam);

            if (dwOptions & OPTION_SHOW_STATUS)
            {
                MoveWindow(SolWnd, 0, 0, nWidth, nHeight - nStatusHeight, TRUE);
                MoveWindow(hwndStatus, 0, nHeight - nStatusHeight, nWidth, nHeight, TRUE);
                SendMessage(hwndStatus, WM_SIZE, wParam, lParam);
            }
            else
            {
                MoveWindow(SolWnd, 0, 0, nWidth, nHeight, TRUE);
            }
            //parts[0] = nWidth - 256;
            //SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts);
            return 0;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi;

            mmi = (MINMAXINFO *)lParam;
            mmi->ptMinTrackSize.x = X_BORDER + NUM_ROW_STACKS * (__cardwidth + X_ROWSTACK_BORDER) + X_BORDER;
            mmi->ptMinTrackSize.y = GetSystemMetrics(SM_CYCAPTION) +
                                    GetSystemMetrics(SM_CYMENU) +
                                    Y_BORDER +
                                    __cardheight +
                                    Y_ROWSTACK_BORDER +
                                    6 * yRowStackCardOffset +
                                    __cardheight +
                                    Y_BORDER +
                                    (dwOptions & OPTION_SHOW_STATUS ? nStatusHeight : 0);
            return 0;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDM_GAME_NEW:
                //simulate a button click on the new button..
                NewGame();
                return 0;

            case IDM_GAME_DECK:
                ShowDeckOptionsDlg(hwnd);
                return 0;

            case IDM_GAME_OPTIONS:
                ShowGameOptionsDlg(hwnd);
                return 0;

            case IDM_HELP_CONTENTS:
                WinHelp(hwnd, szHelpPath, HELP_CONTENTS, 0);//HELP_KEY, (DWORD)"How to play");
                return 0;

            case IDM_HELP_ABOUT:
                MessageBox(hwnd, MsgAbout, szAppName, MB_OK|MB_ICONINFORMATION);
                return 0;

            case IDM_GAME_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }

            return 0;

        case WM_CLOSE:
            if (fGameStarted == false)
            {
                DestroyWindow(hwnd);
                return 0;
            }
            else
            {
                int ret;

                ret = MessageBox(hwnd, MsgQuit, szAppName, MB_OKCANCEL|MB_ICONQUESTION);
                if (ret == IDOK)
                {
                    WinHelp(hwnd, szHelpPath, HELP_QUIT, 0);
                    DestroyWindow(hwnd);
                }
            }
            return 0;
    }

    return DefWindowProc (hwnd, iMsg, wParam, lParam);
}


