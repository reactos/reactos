#include <windows.h>
#include <stdio.h>

static UINT WindowCount;
LRESULT WINAPI TopLevelWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI ChildWndProc(HWND, UINT, WPARAM, LPARAM);

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

  wc.lpszClassName = "TopLevelClass";
  wc.lpfnWndProc = TopLevelWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

  wc.lpszClassName = "ChildClass";
  wc.lpfnWndProc = ChildWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

  hWnd1 = CreateWindow("TopLevelClass",
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
  
  hWndChild = CreateWindow("ChildClass",
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
  
#ifdef TODO
  hWnd2 = CreateWindow("TopLevelClass",
		      "TopLevel2",
		      WS_OVERLAPPEDWINDOW,
		      400,
		      0,
		      160,
		      120,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);
#else
  hWnd2 = CreateWindow("TopLevelClass",
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
#endif

  if (! hWnd1 || ! hWnd2 || ! hWndChild)
    {
      fprintf(stderr, "CreateWindow failed (last error 0x%X)\n",
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

LRESULT CALLBACK TopLevelWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;

  switch(msg)
    {
      case WM_PAINT:
	hDC = BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);
	break;

      case WM_DESTROY:
	if (0 == --WindowCount)
	  {
	    PostQuitMessage(0);
	  }
	break;

      default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
    }

  return 0;
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;

  switch(msg)
    {
      case WM_PAINT:
	hDC = BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);
	break;

      default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
    }

  return 0;
}
