/*
 * COPYRIGHT:         See COPYING in the top level directory
 * AUTHOR:            See gditest-- (initial changes by Mark Tempel)
 * shaptest
 * This test application is for testing shape drawing GDI routines.
 * Much code is taken from gditest (just look at the revision history).
 */

#include <windows.h>

extern BOOL STDCALL GdiDllInitialize(HANDLE hInstance, DWORD Event, LPVOID Reserved);

void __stdcall PolygonTest(HDC Desktop)
{
	HPEN BluePen;  
	HPEN OldPen;
	HBRUSH RedBrush;
	HBRUSH OldBrush;
	POINT lpPointsAlternate[5];
	POINT lpPointsWinding[5];
	DWORD Mode;
	printf("In PolygonTest()\n");
	
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

	printf("Selecting in a blue pen.\n");
	OldPen = (HPEN)SelectObject(Desktop, BluePen);
	printf("Drawing a 5 point star.\n");
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


void shaptest( void ){
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

  // Set up a DC called Desktop that accesses DISPLAY
  Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);
  if (Desktop == NULL){
	printf("Can't create desktop\n");
    return;
  }


//ei
  BlueBrush = CreateSolidBrush( RGB(0, 0, 0xff) );
  DefBrush = SelectObject( Desktop, BlueBrush );

  hRgn1 = CreateRectRgn( 1, 2, 100, 101 );
  hRgn2 = CreateRectRgn( 10, 20, 150, 151 );
  hRgn3 = CreateRectRgn( 1, 1, 1, 1);
  CombineRgn( hRgn3, hRgn1, hRgn2, RGN_XOR );

  //PaintRgn( Desktop, hRgn3 );
  SelectObject( Desktop, DefBrush );
  DeleteObject( BlueBrush );

  // Create a blue pen and select it into the DC
  BluePen = CreatePen(PS_SOLID, 8, RGB(0, 0, 0xff));
  SelectObject(Desktop, BluePen);

  
  // Test font support
  GreenPen = CreatePen(PS_SOLID, 3, RGB(0, 0xff, 0));
  RedPen = CreatePen(PS_SOLID, 3, RGB(0xff, 0, 0));

  tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
                   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                   DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");
  SelectObject(Desktop, tf);
  SetTextColor(Desktop, RGB(0xff, 0xff, 0xff));
  
  //Test 3 Test the Polygon routine.
  PolygonTest(Desktop);

  //Countdown our exit.
  TextOutA(Desktop, 70, 110, "Exiting app in: ", 16);
#define TIME_OUT 30 
  for (sec = 0; sec < TIME_OUT; ++sec)
  {
	sprintf(tbuf,"%d",TIME_OUT - sec);
	
	Xmod = (sec % 10) * 20;
	Ymod = (sec / 10) * 20;

	TextOutA(Desktop, 200 + Xmod , 110 + Ymod, tbuf, strlen(tbuf));
	Sleep( 1500 ); 
  }
  // Free up everything
  DeleteDC(Desktop);
  DeleteDC(MyDC);
}

int main (int argc, char* argv[])
{
  printf("Entering ShapTest (with polygon)..\n");
  
  Sleep( 1000 );

  GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);
  
  shaptest();
  

  return 0;
}
