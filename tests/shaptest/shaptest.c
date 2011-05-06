/*
 * COPYRIGHT:         See COPYING in the top level directory
 * AUTHOR:            See gditest-- (initial changes by Mark Tempel)
 * shaptest
 *
 * This is a windowed application that should draw two polygons. One
 * is drawn with ALTERNATE fill, the other is drawn with WINDING fill.
 * This is used to test out the Polygon() implementation.
 */

#include <windows.h>
#include <stdio.h>
#include <assert.h>

#ifndef ASSERT
#define ASSERT assert
#endif

#define nelem(x) (sizeof (x) / sizeof *(x))

HFONT tf;
LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

void PolygonTest ( HDC hdc )
{
  HPEN Pen, OldPen;
  HBRUSH RedBrush, OldBrush;
  DWORD Mode;
  POINT PointsAlternate[] =
  {
    { 20, 80 },
    { 60, 20 },
    { 90, 80 },
    { 20, 40 },
    { 100, 40 }
  };
  POINT PointsWinding[] =
  {
    { 130, 80 },
    { 170, 20 },
    { 200, 80 },
    { 130, 40 },
    { 210, 40 }
  };
  POINT Tri1[] =
  {
    { 3, 3 },
    { 5, 3 },
    { 3, 5 }
  };
  POINT Tri2[] =
  {
    { 7, 3 },
    { 7, 7 },
    { 3, 7 },
  };
  POINT Square1[] =
  {
    { 1, 15 },
    { 3, 15 },
    { 3, 17 },
    { 1, 17 }
  };
  POINT Square2[] =
  {
    { 5, 15 },
    { 7, 15 },
    { 7, 17 },
    { 5, 17 }
  };
  POINT Square3[] =
  {
    { 1, 23 },
    { 3, 23 },
    { 3, 25 },
    { 1, 25 }
  };
  POINT Square4[] =
  {
    { 5, 23 },
    { 7, 23 },
    { 7, 25 },
    { 5, 25 }
  };
  POINT Square5[] =
  {
    { 1, 31 },
    { 3, 31 },
    { 3, 33 },
    { 1, 33 }
  };
  POINT Square6[] =
  {
    { 5, 31 },
    { 7, 31 },
    { 7, 33 },
    { 5, 33 }
  };

  //create a pen to draw the shape
  Pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0xff));
  ASSERT(Pen);
  RedBrush = CreateSolidBrush(RGB(0xff, 0, 0));
  ASSERT(RedBrush);

  OldPen = (HPEN)SelectObject(hdc, Pen);
  OldBrush = (HBRUSH)SelectObject(hdc, RedBrush);

  Mode = GetPolyFillMode(hdc);

  RoundRect ( hdc, 32, 8, 48, 24, 8, 8 );

  SetPolyFillMode(hdc, ALTERNATE);
  Polygon(hdc,PointsAlternate,nelem(PointsAlternate));

  SetPolyFillMode(hdc, WINDING);
  Polygon(hdc,PointsWinding,nelem(PointsWinding));

  Rectangle ( hdc, 1, 1, 10, 10 );
  Polygon(hdc,Tri1,nelem(Tri1));
  Polygon(hdc,Tri2,nelem(Tri2));

  Rectangle ( hdc,  1, 11,  4, 14 );
  Rectangle ( hdc,  5, 11,  8, 14 );
  Rectangle ( hdc,  9, 11, 12, 14 );
  Rectangle ( hdc, 13, 11, 16, 14 );
  Polygon(hdc,Square1,nelem(Square1));
  Polygon(hdc,Square2,nelem(Square2));
  Rectangle ( hdc,  1, 19,  4, 22 );
  Rectangle ( hdc,  5, 19,  8, 22 );
  Rectangle ( hdc,  9, 19, 12, 22 );
  Rectangle ( hdc, 13, 19, 16, 22 );
  Polygon(hdc,Square3,nelem(Square3));
  Polygon(hdc,Square4,nelem(Square4));
  Rectangle ( hdc,  1, 27,  4, 30 );
  Rectangle ( hdc,  5, 27,  8, 30 );
  Rectangle ( hdc,  9, 27, 12, 30 );
  Rectangle ( hdc, 13, 27, 16, 30 );

  // switch to null pen to make surey they display correctly
  DeleteObject ( SelectObject(hdc, OldPen) );
  Pen = CreatePen ( PS_NULL, 0, 0 );
  ASSERT(Pen);
  OldPen = (HPEN)SelectObject(hdc, Pen);

  Polygon(hdc,Square5,nelem(Square5));
  Polygon(hdc,Square6,nelem(Square6));
  Rectangle ( hdc,  1, 35,  4, 38 );
  Rectangle ( hdc,  5, 35,  8, 38 );
  Rectangle ( hdc,  9, 35, 12, 38 );
  Rectangle ( hdc, 13, 35, 16, 38 );

  //cleanup
  SetPolyFillMode(hdc, Mode);
  DeleteObject ( SelectObject(hdc, OldPen) );
  DeleteObject ( SelectObject(hdc, OldBrush) );
}


void shaptest( HDC hdc )
{
  //Test the Polygon routine.
  PolygonTest(hdc);
}


int WINAPI
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = "ShapTestClass";
  wc.lpfnWndProc = MainWndProc;
  wc.style = CS_VREDRAW | CS_HREDRAW;
  wc.hInstance = hInstance;
  wc.hIcon = (HICON)LoadIcon(NULL, (LPCTSTR)IDI_APPLICATION);
  wc.hCursor = (HCURSOR)LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
  wc.lpszMenuName = NULL;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  if (RegisterClass(&wc) == 0)
    {
      fprintf(stderr, "RegisterClass failed (last error 0x%X)\n",
	      (unsigned int)GetLastError());
      return(1);
    }

  hWnd = CreateWindow("ShapTestClass",
		      "Shaptest",
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
	      (unsigned int)GetLastError());
      return(1);
    }

	tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DeleteObject(tf);

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;

  switch(msg)
  {

  case WM_PAINT:
    hDC = BeginPaint(hWnd, &ps);
    shaptest( hDC );
    EndPaint(hWnd, &ps);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

