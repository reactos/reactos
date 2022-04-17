/*
 * PROJECT:         ReactOS Tests
 * FILE:            rostests/win32/user32/psmtest/psmtest.c
 * PURPOSE:         Side by side comparison of the undocumented
 *                  LpkPSMTextOut and UserLpkPSMTextOut functions
 *
 * PROGRAMMER:      Program skeleton: https://github.com/TransmissionZero/MinGW-Win32-Application
 *                  Test code by Baruch Rutman
 */

#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>

#define IDI_APPICON     101
#define IDR_ACCELERATOR 103
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Prototypes */
INT WINAPI LpkPSMTextOut(HDC hdc, int x, int y, LPCWSTR lpString, int cString, DWORD dwFlags);
INT WINAPI UserLpkPSMTextOut(HDC hdc, int x, int y, LPCWSTR lpString, int cString, DWORD dwFlags);

/* Main window class and title */
static LPCWSTR MainWndClass = L"PSM Test";

#define LPK 1
#define USERLPK 2
static void DrawTest(HDC hdc, int ypos, LPCWSTR str, DWORD dwFlags, int testtype)
{
    WCHAR Result[100];
    INT ret = 0;

    if (testtype == LPK)
    {
        ret = LpkPSMTextOut(hdc, 0, ypos, str, (!str) ? 10 : lstrlenW(str), dwFlags);
        StringCchPrintfW(Result, 100, L"Return Value = %d", ret);
        TextOutW(hdc, 200, ypos, Result, lstrlenW(Result));
    }
    else if (testtype == USERLPK)
    {
        ret = UserLpkPSMTextOut(hdc, 400, ypos, str, lstrlenW(str), dwFlags);
        StringCchPrintfW(Result, 100, L"Return Value = %d", ret);
        TextOutW(hdc, 600, ypos, Result, lstrlenW(Result));
    }

}

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
  WNDCLASSEXW wc;

  /* Class for our main window */
  wc.cbSize        = sizeof(wc);
  wc.style         = 0;
  wc.lpfnWndProc   = &MainWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = (HICON)LoadImage(hInstance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE |
                                      LR_DEFAULTCOLOR | LR_SHARED);
  wc.hCursor       = (HCURSOR)LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = MainWndClass;
  wc.hIconSm       = (HICON)LoadImage(hInstance, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

  /* Initialise common controls */
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  /* Register our main window class, or error */
  if (!RegisterClassExW(&wc))
  {
    MessageBoxW(NULL, L"Error registering main window class.", L"Error", MB_ICONERROR | MB_OK);
    return 0;
  }

  hWnd = CreateWindowExW(0, MainWndClass, MainWndClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                             NULL, NULL, hInstance, NULL);

  /* Create our main window, or error */
  if (!hWnd)
  {
    MessageBoxW(NULL, L"Error creating main window.", L"Error", MB_ICONERROR | MB_OK);
    return 0;
  }

  /* Load accelerators */
  hAccelerators = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR));

  /* Show main window and force a paint */
  ShowWindow(hWnd, nCmdShow | SW_MAXIMIZE);
  UpdateWindow(hWnd);

  /* Main message loop */
  while (GetMessageW(&msg, NULL, 0, 0) > 0)
  {
    if (!TranslateAcceleratorW(hWnd, hAccelerators, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  return (int)msg.wParam;
}

/* Window procedure for our main window */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {

    case WM_GETMINMAXINFO:
    {
      /* Prevent our window from being sized too small */
      MINMAXINFO *minMax = (MINMAXINFO*)lParam;
      minMax->ptMinTrackSize.x = 220;
      minMax->ptMinTrackSize.y = 110;

      return 0;
    }

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            enum {
                ALEF = 0x5D0,
                BET,
                GIMEL,
                DALET,
                HEY,
                VAV,
                ZAYIN,
                HET,
                TET,
                YUD,
                KAF_SOFIT,
                KAF,
                LAMED,
                MEM_SOFIT,
                MEM,
                NUN_SOFIT,
                NUN,
                SAMEKH,
                AYIN,
                PEY_SOFIT,
                PEY,
                TSADI_SOFIT,
                TSADI,
                QOF,
                RESH,
                SHIN,
                TAV
            };

            LOGFONTW font;
            HFONT hfont;
            WCHAR Test[] = L"&Test Text";
            WCHAR Test2[] = L"&Test\nText";
            WCHAR Test3[] = L"&Test\tText";
            WCHAR Test4[] = L"&Test T&ex&t";
            WCHAR Test5[] = { TET, QOF, SAMEKH, TET, L' ', L'&', BET, DALET, YUD, QOF, HEY, 0 };
            WCHAR Test6[] = L"Test&&Text";
            WCHAR Test7[] = { BET, DALET, YUD, QOF, HEY, L'&', 'T','e','s','t', 0};
            WCHAR Test8[] = L"TestText&";
            WCHAR Test9[] = L"TestText&&";
            WCHAR Test10[] = L"TestText";

            ZeroMemory(&font, sizeof(LOGFONTW));
            StringCchCopyW(font.lfFaceName, 32, L"Microsoft Sans Serif");
            hfont = CreateFontIndirectW(&font);
            SelectObject(hdc, hfont);
            SetBkMode(hdc, TRANSPARENT);

            DrawTest(hdc, 0,   Test,  0, LPK);
            DrawTest(hdc, 20,  Test,  DT_NOPREFIX, LPK);
            DrawTest(hdc, 40,  Test,  DT_PREFIXONLY, LPK);
            DrawTest(hdc, 60,  Test2, DT_SINGLELINE, LPK);
            DrawTest(hdc, 80,  Test2, 0, LPK);
            DrawTest(hdc, 100, Test2, DT_CALCRECT, LPK);
            DrawTest(hdc, 120, Test3, 0, LPK);
            DrawTest(hdc, 140, Test3, DT_EXPANDTABS, LPK);
            DrawTest(hdc, 160, NULL,  0, LPK);
            DrawTest(hdc, 180, Test4, 0, LPK);
            DrawTest(hdc, 200, Test4, DT_NOPREFIX, LPK);
            DrawTest(hdc, 220, Test4, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 240, Test5, 0, LPK);
            DrawTest(hdc, 260, Test5, DT_NOPREFIX, LPK);
            DrawTest(hdc, 280, Test5, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 300, Test5, DT_PREFIXONLY, LPK);
            DrawTest(hdc, 320, Test6, 0, LPK);
            DrawTest(hdc, 340, Test6, DT_NOPREFIX, LPK);
            DrawTest(hdc, 360, Test6, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 380, Test6, DT_PREFIXONLY, LPK);
            DrawTest(hdc, 400, Test7, 0, LPK);
            DrawTest(hdc, 420, Test7, DT_NOPREFIX, LPK);
            DrawTest(hdc, 440, Test7, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 460, Test7, DT_PREFIXONLY, LPK);
            DrawTest(hdc, 480, Test7, DT_RTLREADING, LPK);
            SetTextAlign(hdc, GetTextAlign(hdc) | TA_RTLREADING);
            DrawTest(hdc, 500, Test7, 0, LPK);
            SetTextAlign(hdc, GetTextAlign(hdc) & ~TA_RTLREADING);
            DrawTest(hdc, 520, Test8, 0, LPK);
            DrawTest(hdc, 540, Test8, DT_NOPREFIX, LPK);
            DrawTest(hdc, 560, Test8, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 580, Test8, DT_PREFIXONLY, LPK);
            DrawTest(hdc, 600, Test9, 0, LPK);
            DrawTest(hdc, 620, Test9, DT_NOPREFIX, LPK);
            DrawTest(hdc, 640, Test9, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 660, Test9, DT_PREFIXONLY, LPK);
            DrawTest(hdc, 680, Test10, 0, LPK);
            DrawTest(hdc, 700, Test10, DT_NOPREFIX, LPK);
            DrawTest(hdc, 720, Test10, DT_HIDEPREFIX, LPK);
            DrawTest(hdc, 740, Test10, DT_PREFIXONLY, LPK);

            TextOutW(hdc, 100, 760, L"LpkPSMTextOut", 13);

            DrawTest(hdc, 0,   Test,  0, USERLPK);
            DrawTest(hdc, 20,  Test,  DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 40,  Test,  DT_PREFIXONLY, USERLPK);
            DrawTest(hdc, 60,  Test2, DT_SINGLELINE, USERLPK);
            DrawTest(hdc, 80,  Test2, 0, USERLPK);
            DrawTest(hdc, 100, Test2, DT_CALCRECT, USERLPK);
            DrawTest(hdc, 120, Test3, 0, USERLPK);
            DrawTest(hdc, 140, Test3, DT_EXPANDTABS, USERLPK);
            /* DrawTest(hdc, 160, NULL, 0, USERLPK); */ /* Crash on windows */
            DrawTest(hdc, 180, Test4, 0, USERLPK);
            DrawTest(hdc, 200, Test4, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 220, Test4, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 240, Test5, 0, USERLPK);
            DrawTest(hdc, 260, Test5, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 280, Test5, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 300, Test5, DT_PREFIXONLY, USERLPK);
            DrawTest(hdc, 320, Test6, 0, USERLPK);
            DrawTest(hdc, 340, Test6, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 360, Test6, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 380, Test6, DT_PREFIXONLY, USERLPK);
            DrawTest(hdc, 400, Test7, 0, USERLPK);
            DrawTest(hdc, 420, Test7, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 440, Test7, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 460, Test7, DT_PREFIXONLY, USERLPK);
            DrawTest(hdc, 480, Test7, DT_RTLREADING, USERLPK);
            SetTextAlign(hdc, GetTextAlign(hdc) | TA_RTLREADING);
            DrawTest(hdc, 500, Test7, 0, USERLPK);
            SetTextAlign(hdc, GetTextAlign(hdc) & ~TA_RTLREADING);
            DrawTest(hdc, 520, Test8, 0, USERLPK);
            DrawTest(hdc, 540, Test8, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 560, Test8, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 580, Test8, DT_PREFIXONLY, USERLPK);
            DrawTest(hdc, 600, Test9, 0, USERLPK);
            DrawTest(hdc, 620, Test9, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 640, Test9, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 660, Test9, DT_PREFIXONLY, USERLPK);
            DrawTest(hdc, 680, Test10, 0, USERLPK);
            DrawTest(hdc, 700, Test10, DT_NOPREFIX, USERLPK);
            DrawTest(hdc, 720, Test10, DT_HIDEPREFIX, USERLPK);
            DrawTest(hdc, 740, Test10, DT_PREFIXONLY, USERLPK);

            TextOutW(hdc, 500, 760, L"UserLpkPSMTextOut", 17);

            EndPaint(hWnd, &ps);
            break;
        }

    case WM_DESTROY:
    {
      PostQuitMessage(0);
      return 0;
    }
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}
