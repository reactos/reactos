/*
 * PROJECT:         ReactOS Tests
 * FILE:            rostests/win32/user32/biditext/biditext.c
 * PURPOSE:         Demonstrates how ExtTextOut and GetCharacterPlacement
 *                  handle bidirectional text strings via certain selection
 *                  of flags provided to them.
 *
 * PROGRAMMER:      Program skeleton: https://github.com/TransmissionZero/MinGW-Win32-Application
 *                  Test code by Baruch Rutman
 */

#include "biditext.h"

/* Prototypes */
DWORD WINAPI LpkGetCharacterPlacement(HDC hdc, LPCWSTR lpString, INT uCount, INT nMaxExtent,
                                      GCP_RESULTSW *lpResults, DWORD dwFlags, DWORD dwUnused);

BOOL WINAPI LpkExtTextOut(HDC hdc, int x, int y,
                          UINT fuOptions, const RECT *lprc, LPCWSTR lpString,
                          UINT uCount , const INT *lpDx, INT unknown);

/* Global instance handle */
HINSTANCE g_hInstance = NULL;

/* Our application entry point */
int WINAPI
wWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
  INITCOMMONCONTROLSEX icc;
  HWND hWnd;
  HACCEL hAccelerators;
  MSG msg;

  /* Assign global HINSTANCE */
  g_hInstance = hInstance;

  /* Initialise common controls */
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  /* Register our main window class, or error */
  if (!RegisterMainWindowClass())
  {
    MessageBox(NULL, TEXT("Error registering main window class."), TEXT("Error"), MB_ICONERROR | MB_OK);
    return 0;
  }

  /* Create our main window, or error */
  if (!(hWnd = CreateMainWindow()))
  {
    MessageBox(NULL, TEXT("Error creating main window."), TEXT("Error"), MB_ICONERROR | MB_OK);
    return 0;
  }

  /* Load accelerators */
  hAccelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

  /* Show main window and force a paint */
  ShowWindow(hWnd, nCmdShow | SW_MAXIMIZE);
  UpdateWindow(hWnd);

  /* Main message loop */
  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    if (!TranslateAccelerator(hWnd, hAccelerators, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

/* Dialog procedure for our "about" dialog */
INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_COMMAND:
    {
      WORD id = LOWORD(wParam);

      switch (id)
      {
        case IDOK:
        case IDCANCEL:
        {
          EndDialog(hwndDlg, (INT_PTR)id);
          return (INT_PTR)TRUE;
        }
      }
      break;
    }

    case WM_INITDIALOG:
    {
      return (INT_PTR)TRUE;
    }
  }

  return (INT_PTR)FALSE;
}

/* Show our "about" dialog */
void ShowAboutDialog(HWND owner)
{
  DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTDIALOG), owner, &AboutDialogProc);
}

/* Main window class and title */
static LPCTSTR MainWndClass = TEXT("BiDi Test");

/* Window procedure for our main window */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_COMMAND:
    {
      WORD id = LOWORD(wParam);

      switch (id)
      {
        case ID_HELP_ABOUT:
        {
          ShowAboutDialog(hWnd);
          return 0;
        }

        case ID_FILE_EXIT:
        {
          DestroyWindow(hWnd);
          return 0;
        }
      }
      break;
    }

    case WM_GETMINMAXINFO:
    {
      /* Prevent our window from being sized too small */
      MINMAXINFO *minMax = (MINMAXINFO*)lParam;
      minMax->ptMinTrackSize.x = 220;
      minMax->ptMinTrackSize.y = 110;

      return 0;
    }

    /* Item from system menu has been invoked */
    case WM_SYSCOMMAND:
    {
      WORD id = LOWORD(wParam);

      switch (id)
      {
        /* Show "about" dialog on about system menu item */
        case ID_HELP_ABOUT:
        {
          ShowAboutDialog(hWnd);
          return 0;
        }
      }
      break;
    }

    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            HDC hdc = BeginPaint(hWnd, &ps);

            enum
            {
                ALEF = 0x5D0,
                BET,
                GIMEL,
                DALET,
                HEY,
                VAV,
                ZAYIN,
                HET,
                TET,
                YUD
            };

            const WCHAR szString[] = {ALEF, BET, GIMEL, DALET, HEY, 'A', 'B', 'C', 'D', VAV, ZAYIN, HET, TET, YUD, 0};
            const WCHAR szReversedString[] = {HEY, DALET, GIMEL, BET, ALEF, 'A', 'B', 'C', 'D', YUD, TET, HET, ZAYIN, VAV, 0};
            int Len = lstrlenW(szString);
            int i, xpos, tempLength;
            WCHAR tempString[20] = { 0 };
            WCHAR Glyphs[100] = { 0 };
            WCHAR OutString[100] = { 0 };
            INT lpCaretPos[100] = { 0 };
            UINT lpOrder[100] = { 0 };
            GCP_RESULTSW Results = { 0 };

            Results.lStructSize = sizeof(Results);
            Results.lpOutString = OutString;
            Results.lpGlyphs = Glyphs;
            Results.nGlyphs = 100;
            Results.lpCaretPos = lpCaretPos;
            Results.lpOrder = lpOrder;

            SetBkMode(hdc, TRANSPARENT);

            TextOutW(hdc, 10, 10, L"Proper (string being used):", 27);
            TextOutW(hdc, 200, 10, szString, 14);
            TextOutW(hdc, 10, 30, L"Reversed (example):", 19);
            TextOutW(hdc, 200, 30, szReversedString, 14);

            TextOutW(hdc, 10, 50, L"String with NULL LpkETO call (not reversed):", 44);
            LpkExtTextOut(hdc, 10, 70, 0, NULL, szString, Len, NULL, 0);

            TextOutW(hdc, 10, 90, L"String with ETO_IGNORELANGUAGE LpkETO call (not reversed):", 58);
            LpkExtTextOut(hdc, 10, 110, ETO_IGNORELANGUAGE, NULL, szString, Len, NULL, 0);

            TextOutW(hdc, 10, 130, L"String with GCP_REORDER and ETO_GLYPH_INDEX LpkGCP call (not reversed):", 71);
            LpkGetCharacterPlacement(hdc, szString, Len, 0, &Results, GCP_REORDER, 0);
            LpkExtTextOut(hdc, 10, 150, ETO_GLYPH_INDEX, NULL, Glyphs, Results.nGlyphs, NULL, 0);
            TextOutW(hdc, 10, 170, L"String with GCP_REORDER and ETO_IGNORELANGUAGE LpkGCP call (not reversed, lpOutString):", 87);
            ExtTextOutW(hdc, 10, 190, ETO_IGNORELANGUAGE, NULL, OutString, Results.nGlyphs, NULL);

            TextOutW(hdc, 10, 210, L"String without GCP_REORDER and ETO_GLYPH_INDEX LpkGCP call (reversed):", 70);
            LpkGetCharacterPlacement(hdc, szString, Len, 0, &Results, 0, 0);
            LpkExtTextOut(hdc, 10, 230, ETO_GLYPH_INDEX, NULL, Glyphs, Results.nGlyphs, NULL, 0);
            TextOutW(hdc, 10, 250, L"String without GCP_REORDER and ETO_IGNORELANGUAGE LpkGCP call (reversed, lpOutString):", 86);
            ExtTextOutW(hdc, 10, 270, ETO_IGNORELANGUAGE, NULL, OutString, Len, NULL);

            TextOutW(hdc, 10, 290, L"String with ETO_IGNORELANGUAGE ETO call (reversed, not Lpk direct call!):", 73);
            ExtTextOutW(hdc, 10, 310, ETO_IGNORELANGUAGE, NULL, szString, Len, NULL);

            TextOutW(hdc, 10, 330, L"String with ETO_RTLREADING LpkETO call (slight order change)", 60);
            LpkExtTextOut(hdc, 10, 350, ETO_RTLREADING, NULL, szString, Len, NULL, 0);

            TextOutW(hdc, 10, 370, L"String with ETO_RTLREADING ETO call (slight order change)", 57);
            ExtTextOutW(hdc, 10, 390, ETO_RTLREADING, NULL, szString, Len, NULL);

            GetCharacterPlacementW(hdc, szString, Len, 0, &Results, GCP_REORDER);
            TextOutW(hdc, 10, 410, L"Glyph positions with GCP_REORDER flag", 37);

            /* Prints per column the location of the character in the string, reordered location, its position and the character itself */
            for (i = 0, xpos = 10; i < Len; i++, xpos += 30)
            {
                StringCchPrintfW(tempString, 20, L"%d", i);
                tempLength = lstrlenW(tempString);
                TextOutW(hdc, xpos, 430, tempString, tempLength);

                StringCchPrintfW(tempString, 20, L"%d", lpOrder[i]);
                tempLength = lstrlenW(tempString);
                TextOutW(hdc, xpos, 450, tempString, tempLength);

                StringCchPrintfW(tempString, 20, L"%d", lpCaretPos[i]);
                tempLength = lstrlenW(tempString);
                TextOutW(hdc, xpos, 470, tempString, tempLength);

                TextOutW(hdc, xpos, 490, &szString[i], 1);
            }
            TextOutW(hdc, xpos, 430, L"Character location", 18);
            TextOutW(hdc, xpos, 450, L"lpOrder[i]", 10);
            TextOutW(hdc, xpos, 470, L"lpCaretPos[i]", 13);
            TextOutW(hdc, xpos, 490, L"String[i]", 9);

            GetCharacterPlacementW(hdc, szString, Len, 0, &Results, 0);
            TextOutW(hdc, 10, 510, L"Glyph positions without GCP_REORDER flag", 40);

            for (i = 0, xpos = 10; i < Len; i++, xpos += 30)
            {
                StringCchPrintfW(tempString, 20, L"%d", i);
                tempLength = lstrlenW(tempString);
                TextOutW(hdc, xpos, 530, tempString, tempLength);

                StringCchPrintfW(tempString, 20, L"%d", lpOrder[i]);
                tempLength = lstrlenW(tempString);
                TextOutW(hdc, xpos, 550, tempString, tempLength);

                StringCchPrintfW(tempString, 20, L"%d", lpCaretPos[i]);
                tempLength = lstrlenW(tempString);
                TextOutW(hdc, xpos, 570, tempString, tempLength);

                TextOutW(hdc, xpos, 590, &szString[i], 1);
            }
            TextOutW(hdc, xpos, 530, L"Character location", 18);
            TextOutW(hdc, xpos, 550, L"lpOrder[i]", 10);
            TextOutW(hdc, xpos, 570, L"lpCaretPos[i]", 13);
            TextOutW(hdc, xpos, 590, L"String[i]", 9);

            EndPaint(hWnd, &ps);
            break;
        }

    case WM_DESTROY:
    {
      PostQuitMessage(0);
      return 0;
    }
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

/* Register a class for our main window */
BOOL RegisterMainWindowClass()
{
  WNDCLASSEX wc;

  /* Class for our main window */
  wc.cbSize        = sizeof(wc);
  wc.style         = 0;
  wc.lpfnWndProc   = &MainWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = g_hInstance;
  wc.hIcon         = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE |
                                      LR_DEFAULTCOLOR | LR_SHARED);
  wc.hCursor       = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
  wc.lpszClassName = MainWndClass;
  wc.hIconSm       = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

  return (RegisterClassEx(&wc)) ? TRUE : FALSE;
}

/* Create an instance of our main window */
HWND CreateMainWindow()
{
  /* Create instance of main window */
  HWND hWnd = CreateWindowEx(0, MainWndClass, MainWndClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                             NULL, NULL, g_hInstance, NULL);

  if (hWnd)
  {
    /* Add "about" to the system menu */
    HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
    InsertMenu(hSysMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenu(hSysMenu, 6, MF_BYPOSITION, ID_HELP_ABOUT, TEXT("About"));
  }

  return hWnd;
}
