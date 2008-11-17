/* The idea of this test app is inspired from tutorial       *
 * found at http://www.theparticle.com/pgraph.html           *
 *                                                           *
 * Developed by: Aleksey Bragin <aleksey@studiocerebral.com> *
 * Version: 1.0                                              */

#include <windows.h>
#include <stdlib.h>

#define W_WIDTH 320
#define W_HEIGHT 240

// special version of BITMAPINFO and LOGPALETTE for max of 256 palette entries
typedef struct
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
} BITMAPINFO256;

typedef struct {
	WORD	palVersion;
	WORD	palNumEntries;
	PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

// The only global variable --- contents of the DIBitmap
BYTE* dibits;

void GeneratePalette(RGBQUAD* p)
{
	int i;
	for(i=0;i<256;i++)
	{
		p[i].rgbRed = i;
		p[i].rgbGreen = i;
		p[i].rgbBlue = i;
		p[i].rgbReserved = 0;
	}
}

void DoBlt(HBITMAP hBM)
{
	HDC hDC,Context;
	HWND ActiveWindow;
	RECT dest;
	HBITMAP dflBmp;

	if((ActiveWindow = GetActiveWindow()) == NULL)
		return;

	hDC = GetDC(ActiveWindow);
	GetClientRect(ActiveWindow,&dest);

	Context = CreateCompatibleDC(0);
	dflBmp = SelectObject(Context, hBM);
	BitBlt(hDC, 0, 0, dest.right, dest.bottom, Context, 0, 0, SRCCOPY);
	SelectObject(Context, dflBmp);
	DeleteDC(Context);
	DeleteObject(dflBmp);
	ReleaseDC(ActiveWindow, hDC);
}

void UpdatePalette(HBITMAP hBM){
	int i,y;
	static unsigned int c=0;

	for(i=0;i<W_WIDTH;i++){
		for(y=0;y<=W_HEIGHT-1;y++)
			dibits[y*320+i] = c % 256;

		if (c > 512)
			c = 0;
		else
			c++; // It's operation of incrementing of c variable, not reference of a cool OO language :-)
	}

	DoBlt(hBM);
}

void InitBitmap(HANDLE *hBM){
	HPALETTE PalHan;
	HWND ActiveWindow;
	HDC hDC;
	RGBQUAD palette[256];
	int i;
	BITMAPINFO256 bmInf;
	LOGPALETTE256 palInf;

	ActiveWindow = GetActiveWindow();
	hDC = GetDC(ActiveWindow);

	bmInf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInf.bmiHeader.biWidth = W_WIDTH;
	bmInf.bmiHeader.biHeight = -abs(W_HEIGHT);
	bmInf.bmiHeader.biPlanes = 1;
	bmInf.bmiHeader.biBitCount = 8;
	bmInf.bmiHeader.biCompression = BI_RGB;
	bmInf.bmiHeader.biSizeImage = 0;
	bmInf.bmiHeader.biXPelsPerMeter = 0;
	bmInf.bmiHeader.biYPelsPerMeter = 0;
	bmInf.bmiHeader.biClrUsed = 256;
	bmInf.bmiHeader.biClrImportant = 256;

	GeneratePalette(palette);

	for(i=0;i<256;i++)
		bmInf.bmiColors[i] = palette[i];

	palInf.palVersion = 0x300;
	palInf.palNumEntries = 256;
	for(i=0;i<256;i++){
		palInf.palPalEntry[i].peRed = palette[i].rgbRed;
		palInf.palPalEntry[i].peGreen = palette[i].rgbGreen;
		palInf.palPalEntry[i].peBlue = palette[i].rgbBlue;
		palInf.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
	}

	// Create palette
	PalHan = CreatePalette((LOGPALETTE*)&palInf);

	// Select it into hDC
	SelectPalette(hDC,PalHan,FALSE);

	// Realize palette in hDC
	RealizePalette(hDC);

	// Delete handle to palette
	DeleteObject(PalHan);

	// Create dib section
	*hBM = CreateDIBSection(hDC,(BITMAPINFO*)&bmInf,
		DIB_RGB_COLORS,(void**)&dibits,0,0);

	// Release dc
	ReleaseDC(ActiveWindow,hDC);
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT msg, WPARAM wParam,LPARAM lParam)
{
	switch(msg){
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProc(hWnd,msg,wParam,lParam);
	}
}



int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance, LPSTR lpszCmdParam,int nCmdShow)
{
	WNDCLASS WndClass;
	HWND hWnd;
	MSG msg;
	char szName[] = "Palette BitBlt test";
	BOOL exit = FALSE;
	HBITMAP hBM;

	// Create and register window class (not modified!!!!!!!!!!!1)
	WndClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = NULL;//GetStockObject(BLACK_BRUSH);
	WndClass.hIcon = NULL;//LoadIcon(hInstance,NULL);
	WndClass.hCursor = NULL;//LoadCursor(NULL,IDC_ARROW);
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = szName;
	WndClass.lpszMenuName = 0;

	RegisterClass(&WndClass);

	// Create and show window (change styles !!!!!!!!)
	hWnd = CreateWindow(szName, "ReactOS palette bitblt test",
		WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU,
		CW_USEDEFAULT,CW_USEDEFAULT,W_WIDTH,W_HEIGHT,
		0,0,hInstance,0);
	ShowWindow(hWnd,nCmdShow);

	// Prepare bitmap to be bitblt
	InitBitmap(&hBM);

	// Main message loop
	while (!exit)
	{
		UpdatePalette(hBM);
		Sleep(200);

		if(PeekMessage(&msg,0,0,0,PM_NOREMOVE) == TRUE)
		{
			if (!GetMessage(&msg,0,0,0))
				exit = TRUE;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
