#include <windows.h>
#include <stdio.h>
#include <string.h>

static GUITHREADINFO gti;
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

  wc.lpszClassName = "GuiThreadInfoClass";
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
      fprintf(stderr, "RegisterClass failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  hWnd = CreateWindow("GuiThreadInfoClass",
		      "GetGUIThreadInfo",
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
      fprintf(stderr, "CreateWindow failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

	//tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
	//	ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	//	DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

  gti.cbSize = sizeof(GUITHREADINFO);
  GetGUIThreadInfo(0, &gti);

  SetTimer(hWnd, 1, 1000, NULL);
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
	char str[255];

	switch(msg)
	{

	case WM_PAINT:
	  hDC = BeginPaint(hWnd, &ps);
	  wsprintf(str, "flags: ");
	  if(gti.flags & GUI_16BITTASK) lstrcat(str, "GUI_16BITTASK ");
	  if(gti.flags & GUI_CARETBLINKING) lstrcat(str, "GUI_CARETBLINKING ");
	  if(gti.flags & GUI_INMENUMODE) lstrcat(str, "GUI_INMENUMODE ");
	  if(gti.flags & GUI_INMOVESIZE) lstrcat(str, "GUI_INMOVESIZE ");
	  if(gti.flags & GUI_POPUPMENUMODE) lstrcat(str, "GUI_POPUPMENUMODE ");
	  if(gti.flags & GUI_SYSTEMMENUMODE) lstrcat(str, "GUI_SYSTEMMENUMODE ");
	  TextOut(hDC, 10, 10, str, strlen(str));

	  wsprintf(str, "hwndActive == %08X", gti.hwndActive);
	  TextOut(hDC, 10, 30, str, strlen(str));
	  wsprintf(str, "hwndFocus == %08X", gti.hwndFocus);
	  TextOut(hDC, 10, 50, str, strlen(str));
	  wsprintf(str, "hwndCapture == %08X", gti.hwndCapture);
	  TextOut(hDC, 10, 70, str, strlen(str));
	  wsprintf(str, "hwndMenuOwner == %08X", gti.hwndMenuOwner);
	  TextOut(hDC, 10, 90, str, strlen(str));
	  wsprintf(str, "hwndMoveSize == %08X", gti.hwndMoveSize);
	  TextOut(hDC, 10, 110, str, strlen(str));
	  wsprintf(str, "hwndCaret == %08X", gti.hwndCaret);
	  TextOut(hDC, 10, 130, str, strlen(str));
	  wsprintf(str, "rcCaret == (%lu, %lu, %lu, %lu)", gti.rcCaret.left, gti.rcCaret.top, gti.rcCaret.right, gti.rcCaret.bottom);
	  TextOut(hDC, 10, 150, str, strlen(str));

	  wsprintf(str, "GetGuiResources for the current process: %08X", GetCurrentProcess());
	  TextOut(hDC, 10, 180, str, strlen(str));
	  wsprintf(str, "GetGuiResources: GR_GDIOBJECTS == %04X", GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS));
	  TextOut(hDC, 10, 200, str, strlen(str));
	  wsprintf(str, "GetGuiResources: GR_USEROBJECTS == %04x", GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS));
	  TextOut(hDC, 10, 220, str, strlen(str));
	  EndPaint(hWnd, &ps);
	  break;

    case WM_TIMER:
      GetGUIThreadInfo(0, &gti);
      InvalidateRect(hWnd, NULL, TRUE);
      break;

	case WM_DESTROY:
	  PostQuitMessage(0);
	  break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
