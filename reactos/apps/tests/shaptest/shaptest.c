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


HFONT tf;
LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

void PolygonTest ( HDC hdc )
{
  HPEN BluePen, OldPen;
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

  //create a pen to draw the shape
  BluePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0xff));
  RedBrush = CreateSolidBrush(RGB(0xff, 0, 0));

  //initialize a set of points for alternate.
  /*lpPointsAlternate[0].x = 20;
  lpPointsAlternate[0].y = 80; //{20,80};
  lpPointsAlternate[1].x = 60;
  lpPointsAlternate[1].y = 20; //{60,20};
  lpPointsAlternate[2].x = 90;
  lpPointsAlternate[2].y = 80; //{90,80};
  lpPointsAlternate[3].x = 20;
  lpPointsAlternate[3].y = 40; //{20,40};
  lpPointsAlternate[4].x = 100;
  lpPointsAlternate[4].y = 40; //{100,40};

  //initialize a set of points for winding.
  lpPointsWinding[0].x = 130;
  lpPointsWinding[0].y = 80; //{130,80};
  lpPointsWinding[1].x = 170;
  lpPointsWinding[1].y = 20; //{170,20};
  lpPointsWinding[2].x = 200;
  lpPointsWinding[2].y = 80; //{200,80};
  lpPointsWinding[3].x = 130;
  lpPointsWinding[3].y = 40; //{130,40};
  lpPointsWinding[4].x = 210;
  lpPointsWinding[4].y = 40; //{210,40};
*/
  OldPen = (HPEN)SelectObject(hdc, BluePen);
  OldBrush = (HBRUSH)SelectObject(hdc, RedBrush);

  Mode = GetPolyFillMode(hdc);

  SetPolyFillMode(hdc, ALTERNATE);
  Polygon(hdc,PointsAlternate,sizeof(PointsAlternate)/sizeof(PointsAlternate[0]));

  SetPolyFillMode(hdc, WINDING);
  Polygon(hdc,PointsWinding,sizeof(PointsWinding)/sizeof(PointsWinding[0]));

  Rectangle ( hdc, 1, 1, 10, 10 );
  Polygon(hdc,Tri1,sizeof(Tri1)/sizeof(Tri1[0]));
  Polygon(hdc,Tri2,sizeof(Tri2)/sizeof(Tri2[0]));

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

