/* Copyright 1998 DJ Delorie <dj@delorie.com>
   Distributed under the terms of the GNU GPL
   http://www.delorie.com/store/hcalc/
*/
#include <stdio.h>
#include <stdarg.h>

#define  STRICT
#define  WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "hcalc.h"

#define WIDTH 125
#define HEIGHT 147

/*
** local vars
*/
static char		szAppName[16];
static char		szTitle[80];
static HINSTANCE	hInst;
static HBITMAP		face, chars, bits;

static int r=0, g=0, b=0;
static HWND window;

static char shown_offsets[15];
static int shown_bitmask;
static int show_bits;

#define CHARS_LEFT	6
#define CHARS_TOP	6

#define BITS_RIGHT	92
#define BITS_TOP	6

char charmap[] = " 0123456789ABCDEF-x,.ro+";
int char_to_x[256];

void
paint_bits(HDC dc, HDC bdc)
{
  int i;
  SelectObject(bdc, bits);
  for (i=0; i<32; i++)
  {
    int b = (shown_bitmask >> i) & 1;
    BitBlt(dc, BITS_RIGHT-2*i-3*(i/4), BITS_TOP, 1, 7,
	   bdc, b, 0, SRCCOPY);
  }
  
}

void
paint_chars(HDC dc, HDC bdc)
{
  int i;
  SelectObject(bdc, chars);
  for (i=0; i<15; i++)
  {
    BitBlt(dc, CHARS_LEFT+6*i, CHARS_TOP, 5, 7,
	   bdc, shown_offsets[i], 0, SRCCOPY);
  }
}

int
paint()
{
  PAINTSTRUCT paintstruct;
  HDC dc = BeginPaint(window, &paintstruct);
  HDC bdc = CreateCompatibleDC(dc);
  SelectObject(bdc, face);
  BitBlt(dc, 0, 0, WIDTH, HEIGHT, bdc, 0, 0, SRCCOPY);

  if (show_bits)
    paint_bits(dc, bdc);
  else
    paint_chars(dc, bdc);

  DeleteDC(bdc);
  EndPaint(window, &paintstruct);
  return 0;
}

void
redraw()
{
  RECT r;
  r.left = 0;
  r.right = WIDTH-1;
  r.top = 0;
  r.bottom = HEIGHT-1;
  InvalidateRect(window, &r, FALSE);
}

void
set_bits(int b)
{
  shown_bitmask = b;
  show_bits = 1;
  redraw();
}

void
set_string(char *s)
{
  char tmp[16];
  int i;
  sprintf(tmp, "%15.15s", s);
  for (i=0; i<15; i++)
    shown_offsets[i] = char_to_x[tmp[i]];
  show_bits = 0;
  redraw();
}

static int count=0;
static char tmp[100];
static char ctmp[20] = "                ";

void
do_exit(int ec)
{
  PostQuitMessage(ec);
}

/*
** Main Windows Proc
*/
LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//  int i;
//  int lw = LOWORD(wParam);
//  int hw = HIWORD(wParam);
//  HWND w = (HWND)lParam;

  window = hWnd;

  switch (message)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_PAINT:
    return paint();

  case WM_LBUTTONDOWN:
#if 0
    count++;
    wsprintf(tmp, "%3d %3d", LOWORD(lParam), HIWORD(lParam));
    set_string(tmp);
#else
    button(1, LOWORD(lParam), HIWORD(lParam));
#endif
    break;

  case WM_RBUTTONDOWN:
#if 0
    count++;
    set_bits(count);
#else
    button(2, LOWORD(lParam), HIWORD(lParam));
#endif
    break;

  case WM_CHAR:
#if 0
    for (i=0; i<20; i++)
      ctmp[i] = ctmp[i+1];
    ctmp[14] = wParam;
    ctmp[15] = 0;
    set_string(ctmp);
#else
    key(wParam);
#endif
    break;

  default:
    break;

  } /* switch message */

  return DefWindowProc (hWnd, message, wParam, lParam);
}

/*
** register class
*/
static BOOL
InitApplication(HINSTANCE hInstance, int nCmdShow)
{
  int i, style;
  WNDCLASS wc;
  HWND hWnd;
  RECT size;

  LoadString(hInstance, IDS_APPNAME, szAppName, sizeof(szAppName));
  LoadString(hInstance, IDS_DESCRIPTION, szTitle, sizeof(szTitle));

  hInst = hInstance;

  wc.style	     = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon	     = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor	     = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szAppName;

  if (RegisterClass(&wc) == 0)
    return FALSE;

  style = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;

  size.left = 0;
  size.top = 0;
  size.right = WIDTH-3;
  size.bottom = HEIGHT-3;
  AdjustWindowRect(&size, style, 0);

  hWnd = CreateWindowEx(WS_EX_TOPMOST,
  		      szAppName,
		      szTitle,
		      style,
		      CW_USEDEFAULT, 0,
		      size.right-size.left, size.bottom-size.top,
		      NULL,
		      NULL,
		      hInstance,
		      NULL
		      );

  if (hWnd == NULL)
    return FALSE;

  face  = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FACE));
  chars = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_CHARS));
  bits  = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITS));

  for (i=0; i<256; i++)
    char_to_x[i] = 0;
  for (i=0; charmap[i]; i++)
    char_to_x[charmap[i]] = i*6;

  window = hWnd;

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

/*
** Main entry
*/
int WINAPI
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
    MSG msg;
    HANDLE hAccelTable;

    if (!InitApplication(hInstance, nCmdShow))
	return FALSE;

    hAccelTable = LoadAccelerators(hInstance, szAppName);

    while( GetMessage(&msg, NULL, 0, 0))
	if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg))
	{
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    return msg.wParam;
}
