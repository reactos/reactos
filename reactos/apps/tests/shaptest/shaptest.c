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

void __stdcall PolygonTest(HDC Desktop)
{
	HPEN BluePen;  
	HPEN OldPen;
	HBRUSH RedBrush;
	HBRUSH OldBrush;
	POINT lpPointsAlternate[5];
	POINT lpPointsWinding[5];
	DWORD Mode;

	//create a pen to draw the shape
	BluePen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0xff));
	RedBrush = CreateSolidBrush(RGB(0xff, 0, 0));

	//initialize a set of points for alternate.
	lpPointsAlternate[0].x = 20;
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

	OldPen = (HPEN)SelectObject(Desktop, BluePen);
	OldBrush = (HBRUSH)SelectObject(Desktop, RedBrush);

	Mode = GetPolyFillMode(Desktop);

	SetPolyFillMode(Desktop, ALTERNATE);
	Polygon(Desktop,lpPointsAlternate,5);

	SetPolyFillMode(Desktop, WINDING);
	Polygon(Desktop,lpPointsWinding,5);
	
	//cleanup
	SetPolyFillMode(Desktop, Mode);
	SelectObject(Desktop, OldPen);
	SelectObject(Desktop, OldBrush);
	DeleteObject(BluePen);
	DeleteObject(RedBrush);

}


void shaptest( HDC DevCtx ){
  HDC  Desktop, MyDC, DC24;
  HPEN  RedPen, GreenPen, BluePen, WhitePen;
  HBITMAP  MyBitmap, DIB24;
  HFONT  hf, tf;
  BITMAPINFOHEADER BitInf;
  BITMAPINFO BitPalInf;
  HRGN hRgn1, hRgn2, hRgn3;
  HBRUSH BlueBrush, DefBrush;
  int sec,Xmod,Ymod;
  char tbuf[5];


  BlueBrush = CreateSolidBrush( RGB(0, 0, 0xff) );
  DefBrush = SelectObject( DevCtx, BlueBrush );

  SelectObject( DevCtx, DefBrush );
  DeleteObject( BlueBrush );

  // Create a blue pen and select it into the DC
  BluePen = CreatePen(PS_SOLID, 8, RGB(0, 0, 0xff));
  SelectObject(DevCtx, BluePen);

  //Test the Polygon routine.
  PolygonTest(DevCtx);
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
	      GetLastError());
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
	RECT clr, wir;
        char spr[100], sir[100];

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

