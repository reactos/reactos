/*
 * enumwnd.c
 *
 * application to test the various Window Enumeration functions
 */

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

HBRUSH hbrBackground;
HFONT tf;
int test = 0;
const TCHAR* APP_NAME = "EnumWnd Test";
const TCHAR* CLASS_NAME = "EnumWndTestClass";

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = CLASS_NAME;
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      _ftprintf ( stderr, _T("RegisterClass failed (last error 0x%lX)\n"),
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow(CLASS_NAME,
		      APP_NAME,
		      WS_OVERLAPPEDWINDOW,
		      0,
		      0,
		      CW_USEDEFAULT,
		      CW_USEDEFAULT,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);
  if (hWnd == NULL)
    {
      _ftprintf ( stderr, _T("CreateWindow failed (last error 0x%lX)\n"),
	      GetLastError());
      return(1);
    }

  tf = CreateFont (14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
		    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		    DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, _T("Timmons"));

  hbrBackground = CreateSolidBrush ( RGB(192,192,192) );

  ShowWindow ( hWnd, nCmdShow );

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DeleteObject(hbrBackground);

  DeleteObject(tf);

  return msg.wParam;
}

void MyTextOut ( HDC hdc, int x, int y, const TCHAR* text )
{
  TextOut ( hdc, x, y, text, _tcslen(text) );
}

typedef struct _EnumData
{
  HDC hdc;
  int x;
  int y;
} EnumData;

BOOL CALLBACK MyWindowEnumProc ( HWND hwnd, LPARAM lParam )
{
  TCHAR wndcaption[1024], buf[1024];
  EnumData* ped = (EnumData*)lParam;
  GetWindowText ( hwnd, wndcaption, sizeof(wndcaption)/sizeof(*wndcaption) );
  _sntprintf ( buf, sizeof(buf)/sizeof(*buf), _T("%x - %s"), hwnd, wndcaption );
  MyTextOut ( ped->hdc, ped->x, ped->y, buf );
  ped->y += 13;
  return TRUE;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;
  RECT rect;
  TCHAR buf[100];
  EnumData ed;

  switch(msg)
  {
  case WM_PAINT:
    hDC = BeginPaint(hWnd, &ps);
    SelectObject(hDC, tf);

    GetClientRect ( hWnd, &rect );
    FillRect ( hDC, &rect, hbrBackground );

    MyTextOut ( hDC, 10, 10, "EnumWnd Test" );

    _sntprintf ( buf, sizeof(buf)/sizeof(*buf), _T("My HWND: %x"), hWnd );
    MyTextOut ( hDC, 10, 30, buf );

    ed.hdc = hDC;
    ed.x = 10;
    ed.y = 70;

    switch ( test )
    {
    case 1:
      MyTextOut ( hDC, 10, 50, _T("Test #1: EnumWindows()") );
      EnumWindows ( MyWindowEnumProc, (LPARAM)&ed );
      break;
    case 2:
      MyTextOut ( hDC, 10, 50, _T("Test #2: EnumChildWindows()") );
      EnumChildWindows ( hWnd, MyWindowEnumProc, (LPARAM)&ed );
      break;
    case 3:
      MyTextOut ( hDC, 10, 50, _T("Test #3: EnumDesktopWindows") );
      EnumDesktopWindows ( NULL, MyWindowEnumProc, (LPARAM)&ed );
      break;
    case 4:
      MyTextOut ( hDC, 10, 50, _T("Test #4: EnumThreadWindows") );
      EnumThreadWindows ( GetCurrentThreadId(), MyWindowEnumProc, (LPARAM)&ed );
      break;
    default:
      MyTextOut ( hDC, 10, 50, _T("Press any of the number keys from 1 to 4 to run a test") );
      MyTextOut ( hDC, 10, 70, _T("Press the left and right mouse buttons to cycle through the tests") );
      break;
    }

    EndPaint(hWnd, &ps);
    break;

  case WM_CHAR:
    test = (TCHAR)wParam - '1' + 1;
    RedrawWindow ( hWnd, NULL, NULL, RDW_INVALIDATE );
    break;

  case WM_LBUTTONDOWN:
    if ( ++test > 4 )
      test = 1;
    RedrawWindow ( hWnd, NULL, NULL, RDW_INVALIDATE );
    break;

  case WM_RBUTTONDOWN:
    if ( !--test )
      test = 4;
    RedrawWindow ( hWnd, NULL, NULL, RDW_INVALIDATE );
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}
