#include "solitaire.h"

#include <winreg.h>
#include <commctrl.h>
#include <tchar.h>

#include "resource.h"

TCHAR szHelpPath[MAX_PATH];

DWORD        dwAppStartTime;
HWND        hwndMain;
HWND        hwndStatus;
HINSTANCE    hInstance;
HMENU        hGameMenu;

TCHAR szAppName[128];
TCHAR szScore[64];
TCHAR szTime[64];
TCHAR MsgQuit[128];
TCHAR MsgAbout[128];
TCHAR MsgWin[128];
TCHAR MsgDeal[128];
DWORD dwOptions = OPTION_THREE_CARDS;

DWORD dwTime;
DWORD dwWasteCount;
DWORD dwWasteTreshold;
DWORD dwPrevMode;
long lScore;
UINT_PTR PlayTimer = 0;

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

// Returns 0 for no points, 1 for Standard and 2 for Vegas
int GetScoreMode(void)
{
    if ((dwOptions & OPTION_SCORE_STD) && (dwOptions & OPTION_SCORE_VEGAS))
    {
        return SCORE_NONE;
    }

    if (dwOptions & OPTION_SCORE_STD)
    {
        return SCORE_STD;
    }

    if (dwOptions & OPTION_SCORE_VEGAS)
    {
        return SCORE_VEGAS;
    }

    return 0;
}

void UpdateStatusBar(void)
{
    TCHAR szStatusText[128];
    TCHAR szTempText[64];

    ZeroMemory(szStatusText, sizeof(szStatusText));

    if (GetScoreMode() != SCORE_NONE)
    {
        _stprintf(szStatusText, szScore, lScore);
        _tcscat(szStatusText, _T("   "));
    }

    if (dwOptions & OPTION_SHOW_TIME)
    {
        _stprintf(szTempText, szTime, dwTime);
        _tcscat(szStatusText, szTempText);
    }

    SendMessage(hwndStatus, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)(LPTSTR)szStatusText);
}

void SetPlayTimer(void)
{
    if (dwOptions & OPTION_SHOW_TIME)
    {
        if (!PlayTimer)
        {
            PlayTimer = SetTimer(hwndMain, IDT_PLAYTIMER, 1000, NULL);
        }
    }
}

void SetUndoMenuState(bool enable)
{
    if (enable)
    {
        EnableMenuItem(hGameMenu, IDM_GAME_UNDO, MF_BYCOMMAND | MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hGameMenu, IDM_GAME_UNDO, MF_BYCOMMAND | MF_GRAYED);
    }
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
    LoadString(hInst, IDS_SOL_DEAL, MsgDeal, sizeof(MsgDeal) / sizeof(MsgDeal[0]));

    LoadString(hInst, IDS_SOL_SCORE, szScore, sizeof(szScore) / sizeof(TCHAR));
    LoadString(hInst, IDS_SOL_TIME, szTime, sizeof(szTime) / sizeof(TCHAR));

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

    dwPrevMode = GetScoreMode();

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
    if (hwnd == NULL)
        return 1;

    hwndMain = hwnd;

    hGameMenu = GetSubMenu(GetMenu(hwndMain), 0);

    UpdateStatusBar();

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


INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hCtrl;

    switch (uMsg)
    {
    case WM_INITDIALOG:
         // For now, the Help dialog item is disabled because of lacking of HTML Help support
        EnableMenuItem(GetMenu(hDlg), IDM_HELP_CONTENTS, MF_BYCOMMAND | MF_GRAYED);

        CheckRadioButton(hDlg, IDC_OPT_DRAWONE, IDC_OPT_DRAWTHREE,
                         (dwOptions & OPTION_THREE_CARDS) ? IDC_OPT_DRAWTHREE : IDC_OPT_DRAWONE);

        CheckDlgButton(hDlg,
                       IDC_OPT_STATUSBAR,
                       (dwOptions & OPTION_SHOW_STATUS) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hDlg,
                       IDC_OPT_SHOWTIME,
                       (dwOptions & OPTION_SHOW_TIME) ? BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hDlg,
                       IDC_OPT_KEEPSCORE,
                       (dwOptions & OPTION_KEEP_SCORE) ? BST_CHECKED : BST_UNCHECKED);

        hCtrl = GetDlgItem(hDlg, IDC_OPT_KEEPSCORE);

        if (GetScoreMode() == SCORE_NONE)
        {
            CheckRadioButton(hDlg, IDC_OPT_STANDARD, IDC_OPT_NOSCORE, IDC_OPT_NOSCORE);
            EnableWindow(hCtrl, FALSE);
        }
        else if (GetScoreMode() == SCORE_STD)
        {
            CheckRadioButton(hDlg, IDC_OPT_STANDARD, IDC_OPT_NOSCORE, IDC_OPT_STANDARD);
            EnableWindow(hCtrl, FALSE);
        }
        else if (GetScoreMode() == SCORE_VEGAS)
        {
            CheckRadioButton(hDlg, IDC_OPT_STANDARD, IDC_OPT_NOSCORE, IDC_OPT_VEGAS);
            EnableWindow(hCtrl, TRUE);
        }
        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_OPT_NOSCORE:
        case IDC_OPT_STANDARD:
        case IDC_OPT_VEGAS:
            hCtrl = GetDlgItem(hDlg, IDC_OPT_KEEPSCORE);
            if (wParam == IDC_OPT_VEGAS)
                EnableWindow(hCtrl, TRUE);
            else
                EnableWindow(hCtrl, FALSE);
            return TRUE;

        case IDOK:
            dwOptions &= ~OPTION_THREE_CARDS;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_DRAWTHREE) == BST_CHECKED)
                dwOptions |= OPTION_THREE_CARDS;

            if (IsDlgButtonChecked(hDlg, IDC_OPT_STATUSBAR) == BST_CHECKED)
                dwOptions |= OPTION_SHOW_STATUS;
            else
                dwOptions &= ~OPTION_SHOW_STATUS;

            if (IsDlgButtonChecked(hDlg, IDC_OPT_SHOWTIME) == BST_CHECKED)
                dwOptions |= OPTION_SHOW_TIME;
            else
                dwOptions &= ~OPTION_SHOW_TIME;

            if (IsDlgButtonChecked(hDlg, IDC_OPT_KEEPSCORE) == BST_CHECKED)
                dwOptions |= OPTION_KEEP_SCORE;
            else
                dwOptions &= ~OPTION_KEEP_SCORE;

            if (IsDlgButtonChecked(hDlg, IDC_OPT_STANDARD) == BST_CHECKED)
            {
                dwOptions |= OPTION_SCORE_STD;
                dwOptions &= ~OPTION_SCORE_VEGAS;
            }
            else if (IsDlgButtonChecked(hDlg, IDC_OPT_VEGAS) == BST_CHECKED)
            {
                dwOptions |= OPTION_SCORE_VEGAS;
                dwOptions &= ~OPTION_SCORE_STD;
            }
            else if (IsDlgButtonChecked(hDlg, IDC_OPT_NOSCORE) == BST_CHECKED)
            {
                dwOptions |= OPTION_SCORE_VEGAS;
                dwOptions |= OPTION_SCORE_STD;
            }

            UpdateStatusBar();

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

    int iOldScoreMode = GetScoreMode();

    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPTIONS), hwnd, OptionsDlgProc))
    {
        if (((dwOldOptions & OPTION_THREE_CARDS) != (dwOptions & OPTION_THREE_CARDS)) ||
            ((dwOldOptions & OPTION_SHOW_TIME) != (dwOptions & OPTION_SHOW_TIME)) ||
            (iOldScoreMode != GetScoreMode()))
            NewGame();

        if ((dwOldOptions & OPTION_SHOW_STATUS) != (dwOptions & OPTION_SHOW_STATUS))
        {
            int nWidth, nHeight, nStatusHeight;

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
                                                      GWLP_USERDATA);
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


INT_PTR CALLBACK CardBackDlgProc(HWND hDlg,
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
                             GWLP_USERDATA,
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
            int parts[] = { 150, -1 };
            RECT rcStatus;

            // For now, the Help dialog item is disabled because of lacking of HTML Help support
            EnableMenuItem(GetMenu(hwnd), IDM_HELP_CONTENTS, MF_BYCOMMAND | MF_GRAYED);

            hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, _T("Ready"), hwnd, 0);

            //SendMessage(hwndStatus, SB_SIMPLE, (WPARAM)TRUE, 0);

            SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts);
            SendMessage(hwndStatus, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)"");

            SolWnd.Create(hwnd, 0, WS_CHILD | WS_VISIBLE, 0, 0, 100, 100);

            CreateSol();

            // The status bar height is fixed and needed later in WM_SIZE and WM_GETMINMAXINFO
            // Force the window to process WM_GETMINMAXINFO again
            GetWindowRect(hwndStatus, &rcStatus);
            nStatusHeight = rcStatus.bottom - rcStatus.top;

            // Hide status bar if options say so
            if (!(dwOptions & OPTION_SHOW_STATUS))
            {
                ShowWindow(hwndStatus, SW_HIDE);
            }

            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER);

            NewGame();

            dwAppStartTime = GetTickCount();

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_TIMER:
            if (!fGameStarted)
            {
                KillTimer(hwndMain, IDT_PLAYTIMER);
                PlayTimer = 0;
            }
            else if (dwOptions & OPTION_SHOW_TIME)
            {
                if (((dwTime + 1) % 10 == 0) && (GetScoreMode() == SCORE_STD))
                {
                    lScore = lScore >= 2 ? lScore - 2 : 0;
                }

                dwTime++;
            }

            UpdateStatusBar();
            return 0;

        case WM_SIZE:
            nWidth  = LOWORD(lParam);
            nHeight = HIWORD(lParam);

            if (dwOptions & OPTION_SHOW_STATUS)
            {
                MoveWindow(SolWnd, 0, 0, nWidth, nHeight - nStatusHeight, TRUE);
                MoveWindow(hwndStatus, 0, nHeight - nStatusHeight, nWidth, nStatusHeight, TRUE);
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

            case IDM_GAME_UNDO:
                Undo();
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

                ret = MessageBox(hwnd, MsgQuit, szAppName, MB_YESNO|MB_ICONQUESTION);
                if (ret == IDYES)
                {
                    WinHelp(hwnd, szHelpPath, HELP_QUIT, 0);
                    DestroyWindow(hwnd);
                }
            }
            return 0;
    }

    return DefWindowProc (hwnd, iMsg, wParam, lParam);
}


