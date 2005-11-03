#include <windows.h>
#include <stdio.h>

static UINT WindowCount;
LRESULT WINAPI MultiWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd1;
  HWND hWnd2;
  HWND hWndChild;

  wc.lpszClassName = "MultiClass";
  wc.lpfnWndProc = MultiWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR) IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR) IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd1 = CreateWindow("MultiClass",
		      "TopLevel1",
		      WS_OVERLAPPEDWINDOW,
		      0,
		      0,
		      320,
		      240,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);

  hWndChild = CreateWindow("MultiClass",
		      "Child1 of TopLevel1",
		      WS_CHILD | WS_BORDER | WS_CAPTION | WS_VISIBLE | WS_SYSMENU,
		      20,
		      120,
		      200,
		      200,
		      hWnd1,
		      NULL,
		      hInstance,
		      NULL);

  hWnd2 = CreateWindow("MultiClass",
		      "TopLevel2",
		      WS_OVERLAPPEDWINDOW,
		      400,
		      0,
		      160,
		      490,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);

  if (! hWnd1 || ! hWnd2 || ! hWndChild)
    {
      fprintf(stderr, "CreateWindow failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }
  WindowCount = 2;

  ShowWindow(hWnd1, SW_NORMAL);
  ShowWindow(hWnd2, SW_NORMAL);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}

LRESULT CALLBACK MultiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;
  LONG Style;
  RECT Client;
  HBRUSH Brush;
  static COLORREF Colors[] =
    {
      RGB(0x00, 0x00, 0x00),
      RGB(0x80, 0x00, 0x00),
      RGB(0x00, 0x80, 0x00),
      RGB(0x00, 0x00, 0x80),
      RGB(0x80, 0x80, 0x00),
      RGB(0x80, 0x00, 0x80),
      RGB(0x00, 0x80, 0x80),
      RGB(0x80, 0x80, 0x80),
      RGB(0xff, 0x00, 0x00),
      RGB(0x00, 0xff, 0x00),
      RGB(0x00, 0x00, 0xff),
      RGB(0xff, 0xff, 0x00),
      RGB(0xff, 0x00, 0xff),
      RGB(0x00, 0xff, 0xff),
      RGB(0xff, 0xff, 0xff)
    };
  static unsigned CurrentColor = 0;

  switch(msg)
    {
      case WM_PAINT:
	hDC = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &Client);
	Brush = CreateSolidBrush(Colors[CurrentColor]);
	FillRect(hDC, &Client, Brush);
	DeleteObject(Brush);
	CurrentColor++;
	if (sizeof(Colors) / sizeof(Colors[0]) <= CurrentColor)
	  {
	    CurrentColor = 0;
	  }
	EndPaint(hWnd, &ps);
	break;

      case WM_DESTROY:
	Style = GetWindowLong(hWnd, GWL_STYLE);
	if (0 == (Style & WS_CHILD) && 0 == --WindowCount)
	  {
	    PostQuitMessage(0);
	  }
	break;

      default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
    }

  return 0;
}
