/*
 * gditest
   dec 26, 2001 -- gditest bug fix by Richard Campbell
 */

#include <windows.h>

extern BOOL STDCALL GdiDllInitialize(HANDLE hInstance, DWORD Event, LPVOID Reserved);

void __stdcall Background (HDC Desktop)
{
	HPEN Pen;
	int x, y;

	Pen = CreatePen(PS_SOLID, 1, RGB(64, 64, 128));

	SelectObject (Desktop, Pen);

	MoveToEx (Desktop, 0, 0, NULL);
	LineTo (Desktop, 640, 480);
	for (y = 479, x = 0; x < 640; x+=2)
	{
		MoveToEx (Desktop, 0, 0, NULL);
		LineTo (Desktop, x, y);
	}
	for (y = 0, x = 639; y < 480; y+=2)
	{
		MoveToEx (Desktop, 0, 0, NULL);
		LineTo (Desktop, x, y);
	}
}

void gditest( void ){
  HDC  Desktop, MyDC, DC24;
  HPEN  RedPen, GreenPen, BluePen, WhitePen;
  HBITMAP  MyBitmap, DIB24;
  HFONT  hf, tf;
  BITMAPINFOHEADER BitInf;
  BITMAPINFO BitPalInf;
  HRGN hRgn1, hRgn2, hRgn3;
  HBRUSH BlueBrush, DefBrush;

  // Set up a DC called Desktop that accesses DISPLAY
  Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);
  if (Desktop == NULL){
	printf("Can't create desktop\n");
    return;
  }

  // Background
  Background (Desktop);


//ei
  BlueBrush = CreateSolidBrush( RGB(0, 0, 0xff) );
  DefBrush = SelectObject( Desktop, BlueBrush );

  hRgn1 = CreateRectRgn( 1, 2, 100, 101 );
  hRgn2 = CreateRectRgn( 10, 20, 150, 151 );
  hRgn3 = CreateRectRgn( 1, 1, 1, 1);
  CombineRgn( hRgn3, hRgn1, hRgn2, RGN_XOR );

  PaintRgn( Desktop, hRgn3 );
  SelectObject( Desktop, DefBrush );
  DeleteObject( BlueBrush );

  // Create a blue pen and select it into the DC
  BluePen = CreatePen(PS_SOLID, 8, RGB(0, 0, 0xff));
  SelectObject(Desktop, BluePen);

  // Draw a shape on the DC
  MoveToEx(Desktop, 50, 50, NULL);
  LineTo(Desktop, 200, 60);
  LineTo(Desktop, 200, 300);
  LineTo(Desktop, 50, 50);
  MoveToEx(Desktop, 50, 50, NULL);
  LineTo(Desktop, 200, 50);

  WhitePen = CreatePen(PS_SOLID, 3, RGB(0xff, 0xff, 0xff));
  SelectObject(Desktop, WhitePen);

  MoveToEx(Desktop, 20, 70, NULL);
  LineTo(Desktop, 500, 70);
  MoveToEx(Desktop, 70, 20, NULL);
  LineTo(Desktop, 70, 150);

  // Test font support
  GreenPen = CreatePen(PS_SOLID, 3, RGB(0, 0xff, 0));
  RedPen = CreatePen(PS_SOLID, 3, RGB(0xff, 0, 0));

  hf = CreateFontA(24, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
                   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                   DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Helmet");
  SelectObject(Desktop, hf);
  SetTextColor(Desktop, RGB(0xff, 0, 0));
  TextOutA(Desktop, 70, 70, "React", 5);
  SetTextColor(Desktop, RGB(0, 0xff, 0));
  TextOutA(Desktop, 140, 70, "OS", 2);

  tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
                   ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                   DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");
  SelectObject(Desktop, tf);
  SetTextColor(Desktop, RGB(0xff, 0xff, 0xff));
  TextOutA(Desktop, 70, 90, "This is a test of ReactOS text, using the FreeType 2 library!", 61);

  // TEST 1: Copy from the VGA into a device compatible DC, draw on it, then blt it to the VGA again
  MyDC = CreateCompatibleDC(Desktop);
  MyBitmap = CreateCompatibleBitmap(Desktop, 151, 251);
  SelectObject(MyDC, MyBitmap);
  BitBlt(MyDC, 0, 0, 151, 251, Desktop, 50, 50, SRCCOPY); // can we say 151, 251 since bottom corner is not inclusive?

  SelectObject(MyDC, GreenPen);
  Rectangle(MyDC, 10, 10, 50, 50);

  // TEST 2: Copy from the device compatible DC into a 24BPP bitmap, draw on it, then blt to the VGA again
  BitInf.biSize = sizeof(BITMAPINFOHEADER);
  BitInf.biWidth = 152;
  BitInf.biHeight = -252; // it's top down (since BI_RGB is used, the sign is operative of direction)
  BitInf.biPlanes = 1;
  BitInf.biBitCount = 24;
  BitInf.biCompression = BI_RGB;
  BitInf.biSizeImage = 0;
  BitInf.biXPelsPerMeter = 0;
  BitInf.biYPelsPerMeter = 0;
  BitInf.biClrUsed = 0;
  BitInf.biClrImportant = 0;
  BitPalInf.bmiHeader = BitInf;
  DIB24 = (HBITMAP) CreateDIBSection(NULL, &BitPalInf, DIB_RGB_COLORS, NULL, NULL, 0);
  DC24 = CreateCompatibleDC(NULL);
  SelectObject(DC24, DIB24);

  BitBlt(DC24, 0, 0, 101, 201, MyDC, 0, 0, SRCCOPY);
  SelectObject(DC24, RedPen);
  Rectangle(DC24, 80, 90, 100, 110);
  MoveToEx(DC24, 80, 90, NULL);
  LineTo(DC24, 100, 110);
  BitBlt(Desktop, 200, 200, 110, 120, DC24, 0, 0, SRCCOPY);

  Sleep( 10000 ); // fixme delay only 10000 (for 10 seconds)
  // Free up everything
  DeleteDC(Desktop);
  DeleteDC(MyDC);
}

void DumpRgnData( HRGN hRgn )
{
	int size, ret, i;
	LPRGNDATA rgnData;

	size = GetRegionData( hRgn, 0, NULL );
	if( size == 0 ){
		printf("GetRegionData returned 0\n");
		return;
	}
	rgnData = (LPRGNDATA) malloc( size );
	ret = GetRegionData( hRgn, size, rgnData );
	if( ret == 0 ){
		printf("GetRegionData( hRgn, size, rgnData ) returned 0\n");
		return;
	}
	printf("Bounds: left=%d top=%d right=%d bottom=%d, count: %d, type: %i\n\n",
		rgnData->rdh.rcBound.left, rgnData->rdh.rcBound.top, rgnData->rdh.rcBound.right, rgnData->rdh.rcBound.bottom,
		rgnData->rdh.nCount, rgnData->rdh.iType);
	printf("Rects:\t i \t left \t top \t right \t bottom\n");
	for ( i = 0; i < rgnData->rdh.nCount; i++ ) {
		PRECT pr = (PRECT) rgnData->Buffer + i;
		printf("\t %d \t %d \t %d \t %d \t %d\n", i, pr->left, pr->top, pr->right, pr->bottom );
	}
	printf("\n");
}

void rgntest( void )
{
	HRGN hRgn1, hRgn2, hRgn3;
	RECT Rect;
	int i;

	hRgn1 = CreateRectRgn( 1, 2, 100, 101 );
	if( hRgn1 == NULL ) {
		printf("Failed at hRgn1 = CreateRectRgn( 1, 2, 100, 101 )\n");
		return;
	}
	i = GetRgnBox( hRgn1, &Rect );
	if( i==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("GetRgnBox( hRgn1, &Rect ): i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );

	DumpRgnData( hRgn1 );

	hRgn2 = CreateRectRgn( 51, 53, 150, 152 );
	if( hRgn2 == NULL ) {
		printf("Failed at hRgn2 = CreateRectRgn( 51, 53, 150, 152 )\n");
		return;
	}
	i = GetRgnBox( hRgn2, &Rect );
	if( i==0 ){
		printf("Failed GetRgnBox( hRgn2, &Rect )\n");
		return;
	}
	printf("GetRgnBox( hRgn2, &Rect ): i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );

	DumpRgnData( hRgn2 );

	if( EqualRgn( hRgn1, hRgn2 ) == TRUE ){
		printf("\t hRgn1, hRgn2 are equal\n");
	}
	else{
		printf("\t hRgn1, hRgn2 are NOT equal\n\n");
	}

	i = OffsetRgn(hRgn1,50,51);
	if( i==ERROR ){
		printf("Failed OffsetRgn(hRgn1,50,51)\n");
		return;
	}

	i = GetRgnBox( hRgn1, &Rect );
	if( i==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("After offset\nGetRgnBox( hRgn1, &Rect ): i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );

	if( EqualRgn( hRgn1, hRgn2 ) == TRUE ){
		printf("\t hRgn1, hRgn2 are equal after offset\n");
	}
	else{
		printf("\t hRgn1, hRgn2 are NOT equal after offset!\n\n");
	}

	i = SetRectRgn(hRgn1, 10, 11, 110, 111 );
	if( i==0 ){
		printf("Failed SetRectRgn(hRgn1... )\n");
		return;
	}
	i = GetRgnBox( hRgn1, &Rect );
	if( i==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("after SetRectRgn(hRgn1, 10, 11, 110, 111 ):\n i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );

	hRgn3 = CreateRectRgn( 1, 1, 1, 1);
	i = CombineRgn( hRgn3, hRgn1, hRgn2, RGN_AND );
	if( i==ERROR ){
		printf("Fail: CombineRgn( hRgn3, hRgn1, hRgn2, RGN_AND ). LastError: %d\n", GetLastError);
		return;
	}

	if( GetRgnBox( hRgn3, &Rect )==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("After CombineRgn( hRgn3, hRgn1, hRgn2, RGN_AND ): \nGetRgnBox( hRgn3, &Rect ): CR_i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );
	DumpRgnData( hRgn3 );

	i = CombineRgn( hRgn3, hRgn1, hRgn2, RGN_OR );
	if( i==ERROR ){
		printf("Fail: CombineRgn( hRgn3, hRgn1, hRgn2, RGN_OR ). LastError: %d\n", GetLastError);
		return;
	}

	if( GetRgnBox( hRgn3, &Rect )==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("After CombineRgn( hRgn3, hRgn1, hRgn2, RGN_OR ): \nGetRgnBox( hRgn3, &Rect ): CR_i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );
	DumpRgnData( hRgn3 );

	i = CombineRgn( hRgn3, hRgn1, hRgn2, RGN_DIFF );
	if( i==ERROR ){
		printf("Fail: CombineRgn( hRgn3, hRgn1, hRgn2, RGN_DIFF ). LastError: %d\n", GetLastError);
		return;
	}

	if( GetRgnBox( hRgn3, &Rect )==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("After CombineRgn( hRgn3, hRgn1, hRgn2, RGN_DIFF ): \nGetRgnBox( hRgn3, &Rect ): CR_i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );
	DumpRgnData( hRgn3 );

	i = CombineRgn( hRgn3, hRgn1, hRgn2, RGN_XOR );
	if( i==ERROR ){
		printf("Fail: CombineRgn( hRgn3, hRgn1, hRgn2, RGN_XOR ). LastError: %d\n", GetLastError);
		return;
	}

	if( GetRgnBox( hRgn3, &Rect )==0 ){
		printf("Failed GetRgnBox( hRgn3, &Rect )\n");
		return;
	}
	printf("After CombineRgn( hRgn3, hRgn1, hRgn2, RGN_XOR ): \nGetRgnBox( hRgn3, &Rect ): CR_i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );
	DumpRgnData( hRgn3 );

	i = CombineRgn( hRgn1, hRgn3, hRgn2, RGN_COPY );
	if( i==ERROR ){
		printf("Fail: CombineRgn( hRgn1, hRgn3, hRgn2, RGN_COPY ). LastError: %d\n", GetLastError);
		return;
	}

	if( GetRgnBox( hRgn1, &Rect )==0 ){
		printf("Failed GetRgnBox( hRgn1, &Rect )\n");
		return;
	}
	printf("After CombineRgn( hRgn1, hRgn3, hRgn2, RGN_COPY ): \nGetRgnBox( hRgn1, &Rect ): CR_i=%d, left=%d top=%d right=%d bottom=%d\n\n",
		i, Rect.left, Rect.top, Rect.right, Rect.bottom );
	DumpRgnData( hRgn1 );


	DeleteObject( hRgn1 );
	DeleteObject( hRgn2 );
	DeleteObject( hRgn3 );
	printf("region test finished\n");
}

int main (int argc, char* argv[])
{
  printf("Entering GDITest..\n");
  printf("use gditest for older tests\n");
  printf("use gditest 1 for region test\n");

  GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);
  if( argc < 2 )
	gditest();
  else {
	if( !strncmp( argv[1], "1", 1 ) ) {
		rgntest();
	}
  }

  return 0;
}
