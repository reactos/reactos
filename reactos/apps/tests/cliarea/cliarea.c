#include <windows.h>
#include <stdio.h>

//HFONT tf;
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

  wc.lpszClassName = "CliAreaClass";
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
      fprintf(stderr, "RegisterClass failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("CliAreaClass",
		      "ClientArea Test",
		      WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL,
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
      fprintf(stderr, "CreateWindow failed (last error 0x%X)\n",
	      GetLastError());
      return(1);
    }

	//tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
	//	ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  //DeleteObject(tf);

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	RECT clr, wir;
    char txt[100];

	switch(msg)
	{
	case WM_LBUTTONUP:
	  {
	    ULONG x, y;
	    RECT Rect;
	    GetWindowRect(hWnd, &Rect);
	    SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM)(&Rect));
	    hDC = GetWindowDC(0);
	    Rectangle(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom);
	    sprintf(txt, "Client coordinates: %d, %d, %d, %d\0", Rect.left, Rect.top, Rect.right, Rect.bottom);
	    TextOut(hDC, Rect.left + 1, Rect.top + 1, (LPCTSTR)txt, strlen(txt));
	    ReleaseDC(0, hDC);
	    break;
	  }

	case WM_DESTROY:
	  PostQuitMessage(0);
	  break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
