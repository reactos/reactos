#include <windows.h>
#include <stdio.h>
#include <string.h>

#define ID_ACCEL1 0x100
#define ID_ACCEL2 0x101
#define ID_ACCEL3 0x102
#define ID_ACCEL4 0x103

#ifndef VK_A
#define VK_A 0x41
#endif

/*
 * {fVirt, key, cmd}
 * fVirt |= FVIRTKEY | FCONTROL | FALT | FSHIFT
 */
//static HFONT tf;
static ACCEL Accelerators[4] = {
	{ FVIRTKEY, VK_A, ID_ACCEL1},
	{ FVIRTKEY | FSHIFT, VK_A, ID_ACCEL2},
	{ FVIRTKEY | FCONTROL, VK_A, ID_ACCEL3},
	{ FVIRTKEY | FALT, VK_A, ID_ACCEL4}};
static HACCEL hAcceleratorTable;
static char Event[200];

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

  wc.lpszClassName = "AcceleratorTest";
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

  hWnd = CreateWindow("AcceleratorTest",
		      "Accelerator Test",
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
      fprintf(stderr, "CreateWindow failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  /*tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
	ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");*/

  Event[0] = 0;

  ShowWindow(hWnd, nCmdShow);

  hAcceleratorTable = CreateAcceleratorTable(Accelerators,
	sizeof(Accelerators)/sizeof(Accelerators[1]));
  if (hAcceleratorTable == NULL)
    {
      fprintf(stderr, "CreateAcceleratorTable failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  while(GetMessage(&msg, NULL, 0, 0))
    {
	  if (!TranslateAccelerator(hWnd, hAcceleratorTable, &msg))
	    {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
	    }
    }

  if (!DestroyAcceleratorTable(hAcceleratorTable))
    {
      fprintf(stderr, "DestroyAcceleratorTable failed (last error 0x%lX)\n",
	      GetLastError());
      return(1);
    }

  //DeleteObject(tf);

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	char buf[200];

	switch(msg)
	{
	case WM_PAINT:
      hDC = BeginPaint(hWnd, &ps);
	  //SelectObject(hDC, tf);
	  sprintf(buf, "Event: '%s'", Event);
	  TextOut(hDC, 10, 10, buf, strlen(buf));
	  EndPaint(hWnd, &ps);
	  break;

	case WM_DESTROY:
	  PostQuitMessage(0);
	  break;

    case WM_COMMAND:

	  switch (LOWORD(wParam))
	  {
	  case ID_ACCEL1:
	    strcpy(Event, "A");
		break;

	  case ID_ACCEL2:
	    strcpy(Event, "SHIFT+A");
		break;

	  case ID_ACCEL3:
	    strcpy(Event, "CTRL+A");
		break;

	  case ID_ACCEL4:
	    strcpy(Event, "ALT+A");
		break;

	  default:
	    sprintf(Event, "%d", LOWORD(wParam));
		break;
	  }

	  InvalidateRect(hWnd, NULL, TRUE);
	  UpdateWindow(hWnd);
	  break;

	default:
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
