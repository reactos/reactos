/*
 * PROJECT:      Spider Solitaire
 * LICENSE:      See COPYING in top level directory
 * FILE:         base/applications/games/spider/spider.cpp
 * PURPOSE:      Window and message queue for Spider Solitaire
 * PROGRAMMER:   Gregor Schneider
 */

#include "spider.h"

#include <commctrl.h>
#include <tchar.h>

TCHAR szHelpPath[MAX_PATH];

DWORD        dwAppStartTime;
HWND         hwndMain;
HINSTANCE    hInstance;

TCHAR szAppName[128];
TCHAR MsgQuit[128];
TCHAR MsgAbout[128];
TCHAR MsgWin[128];
TCHAR MsgDeal[128];
DWORD dwDifficulty;

CardWindow SpiderWnd;

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

INT_PTR CALLBACK DifficultyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_DIF_ONECOLOR, IDC_DIF_FOURCOLORS, IDC_DIF_ONECOLOR);
        return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
            case IDOK:
                if (IsDlgButtonChecked(hDlg, IDC_DIF_ONECOLOR) == BST_CHECKED)
                    dwDifficulty = IDC_DIF_ONECOLOR;
                else if (IsDlgButtonChecked(hDlg, IDC_DIF_TWOCOLORS) == BST_CHECKED)
                    dwDifficulty = IDC_DIF_TWOCOLORS;
                else if (IsDlgButtonChecked(hDlg, IDC_DIF_FOURCOLORS) == BST_CHECKED)
                    dwDifficulty = IDC_DIF_FOURCOLORS;

                NewGame();
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

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR szCmdLine, int iCmdShow)
{
    HWND        hwnd;
    MSG            msg;
    WNDCLASS    wndclass;
    INITCOMMONCONTROLSEX ice;
    HACCEL        hAccelTable;

    hInstance = hInst;

    /* Load application title */
    LoadString(hInst, IDS_SPI_NAME, szAppName, sizeof(szAppName) / sizeof(szAppName[0]));
    /* Load MsgBox() texts here to avoid loading them many times later */
    LoadString(hInst, IDS_SPI_ABOUT, MsgAbout, sizeof(MsgAbout) / sizeof(MsgAbout[0]));
    LoadString(hInst, IDS_SPI_QUIT, MsgQuit, sizeof(MsgQuit) / sizeof(MsgQuit[0]));
    LoadString(hInst, IDS_SPI_WIN, MsgWin, sizeof(MsgWin) / sizeof(MsgWin[0]));
    LoadString(hInst, IDS_SPI_DEAL, MsgDeal, sizeof(MsgDeal) / sizeof(MsgDeal[0]));

    /* Window class for the main application parent window */
    wndclass.style             = 0;
    wndclass.lpfnWndProc       = WndProc;
    wndclass.cbClsExtra        = 0;
    wndclass.cbWndExtra        = 0;
    wndclass.hInstance         = hInst;
    wndclass.hIcon             = LoadIcon (hInst, MAKEINTRESOURCE(IDI_SPIDER));
    wndclass.hCursor           = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground     = (HBRUSH)NULL;
    wndclass.lpszMenuName      = MAKEINTRESOURCE(IDR_MENU1);
    wndclass.lpszClassName     = szAppName;

    RegisterClass(&wndclass);

    ice.dwSize = sizeof(ice);
    ice.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&ice);

    srand((unsigned)GetTickCount());

    /* InitCardLib(); */

    /* Construct the path to our help file */
    MakePath(szHelpPath, MAX_PATH, _T(".hlp"));

    hwnd = CreateWindow(szAppName,
                        szAppName,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        0,  /*The real size will be computed in WndProc through WM_GETMINMAXINFO */
                        0,  /* The real size will be computed in WndProc through WM_GETMINMAXINFO */
                        NULL,
                        NULL,
                        hInst,
                        NULL);

    hwndMain = hwnd;

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIFFICULTY), hwnd, DifficultyDlgProc);

    while (GetMessage(&msg, NULL,0,0))
    {
        if (!TranslateAccelerator(hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}

LRESULT CALLBACK CardImageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PCARDBACK pCardBack = (PCARDBACK)GetWindowLongPtr(hwnd, GWL_USERDATA);
    static WNDPROC hOldProc = NULL;

    if (!pCardBack)
        return FALSE;

    if (!hOldProc)
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

            Rectangle(hdc, rc.left+1, rc.top+1, rc.right, rc.bottom);

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

    return CallWindowProc(hOldProc, hwnd, msg, wParam, lParam);
}


INT_PTR CALLBACK CardBackDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PCARDBACK pCardBacks = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            INT i, c;
            SIZE_T size = sizeof(CARDBACK) * NUM_CARDBACKS;

            pCardBacks = (PCARDBACK)HeapAlloc(GetProcessHeap(), 0, size);

            if (!pCardBacks)
                return FALSE;

            for (i = 0, c = CARDBACK_START; c <= CARDBACK_END; i++, c++)
            {
                pCardBacks[i].hSelf = GetDlgItem(hDlg, c);
                pCardBacks[i].bSelected = FALSE;
                pCardBacks[i].hdcNum = CARDBACK_RES_START + i;
                pCardBacks[i].imgNum = i + 1;
                pCardBacks[i].hOldProc = (WNDPROC)SetWindowLongPtr(pCardBacks[i].hSelf,
                                                                   GWLP_WNDPROC,
                                                                   (LONG_PTR)CardImageWndProc);

                SetWindowLongPtr(pCardBacks[i].hSelf, GWL_USERDATA, (LONG_PTR)&pCardBacks[i]);
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
                    pCardBacks[i].bSelected = pCardBacks[i].hSelf == (HWND)lParam;

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

    if ((cardBack = DialogBox(hInstance, MAKEINTRESOURCE(IDD_CARDBACK), hwnd, CardBackDlgProc)))
    {
        SpiderWnd.SetBackCardIdx(CARDBACK_RES_START + (cardBack - 1));
        SpiderWnd.Redraw();
    }
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static int nWidth, nHeight;

    switch (iMsg)
    {
        case WM_CREATE:
        {
            SpiderWnd.Create(hwnd, 0, WS_CHILD | WS_VISIBLE, 0, 0, 100, 100);
            dwDifficulty = IDC_DIF_ONECOLOR;
            CreateSpider();

            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER);

            dwAppStartTime = GetTickCount();

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            nWidth  = LOWORD(lParam);
            nHeight = HIWORD(lParam);

            MoveWindow(SpiderWnd, 0, 0, nWidth, nHeight, TRUE);
            return 0;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi;

            mmi = (MINMAXINFO *)lParam;
            mmi->ptMinTrackSize.x = NUM_STACKS * __cardwidth + (NUM_STACKS + 3) * X_BORDER + 12; // Border left and right of 6px
            mmi->ptMinTrackSize.y = GetSystemMetrics(SM_CYCAPTION) +
                                    GetSystemMetrics(SM_CYMENU) +
                                    2 * Y_BORDER +
                                    3 * __cardheight +
                                    6 * yRowStackCardOffset;
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_GAME_NEW:
                    NewGame();
                    return 0;

                case IDM_GAME_DECK:
                    ShowDeckOptionsDlg(hwnd);
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

