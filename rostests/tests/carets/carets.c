#include <windows.h>
#include <stdio.h>
#include "resource.h"

static int CaretWidth = 2;
static int CaretHeight = 16;
static int CharWidth = 10;
static int CharHeight = 16;
static HBITMAP CaretBitmap;

ULONG __cdecl DbgPrint(IN PCH  Format, IN ...);

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

  CaretBitmap = LoadBitmap(hInstance, (LPCTSTR)IDB_CARET);

  wc.lpszClassName = "CaretTestClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow(wc.lpszClassName,
		      "Caret Test",
		      WS_OVERLAPPEDWINDOW,
		      0,
		      0,
		      200,
		      250,
		      NULL,
		      NULL,
		      hInstance,
		      NULL);
  if (hWnd == NULL)
    {
      fprintf(stderr, "CreateWindow failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  POINT pt;
	switch(msg)
	{
    case WM_ACTIVATE:
      switch(LOWORD(wParam))
      {
        case WA_ACTIVE:
        case WA_CLICKACTIVE:
          if(!ShowCaret(hWnd))
            DbgPrint("ShowCaret(0x%x)\n", hWnd);
          break;
        case WA_INACTIVE:
          if(!HideCaret(hWnd))
            DbgPrint("HideCaret(0x%x)\n", hWnd);
          break;
      }
      break;

    case WM_KEYDOWN:
      if(!GetCaretPos(&pt))
      {
        DbgPrint("GetCaretPos() failed!\n");
        break;
      }
      switch(wParam)
      {
        case VK_LEFT:
          pt.x -= CharWidth;
          break;
        case VK_UP:
          pt.y -= CharHeight;
          break;
        case VK_RIGHT:
          pt.x += CharWidth;
          break;
        case VK_DOWN:
          pt.y += CharHeight;
          break;
      }
      if(!SetCaretPos(pt.x, pt.y))
        DbgPrint("SetCaretPos() failed!\n");
      break;

    case WM_RBUTTONDOWN:
      if(!CreateCaret(hWnd, CaretBitmap, 0, 0))
        DbgPrint("CreateCaret() for window 0x%x failed!\n", hWnd);
      else
        if(!ShowCaret(hWnd))
          DbgPrint("ShowCaret(0x%x)\n", hWnd);
      break;

    case WM_LBUTTONDOWN:
      if(!CreateCaret(hWnd, (HBITMAP)0, CaretWidth, CaretHeight))
        DbgPrint("CreateCaret() for window 0x%x failed!\n", hWnd);
      else
        if(!ShowCaret(hWnd))
          DbgPrint("ShowCaret(0x%x)\n", hWnd);
      break;

    case WM_CREATE:
      if(!CreateCaret(hWnd, (HBITMAP)0, CaretWidth, CaretHeight))
        DbgPrint("CreateCaret() for window 0x%x failed!\n", hWnd);
      else
        if(!SetCaretPos(1, 1))
          DbgPrint("SetCaretPos(%i, %i) failed!\n", 1, 1);
      break;

    case WM_DESTROY:
      if(!DestroyCaret())
        DbgPrint("DestroyCaret() failed!\n");
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
