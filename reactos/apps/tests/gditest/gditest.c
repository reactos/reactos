/*
 * gditest
 */

#include <windows.h>


int main (void)
{
	HDC	Desktop, MyDC;
        HPEN	RedPen, GreenPen;
	HBITMAP	MyBitmap;

	GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);

	// Set up a DC called Desktop that accesses DISPLAY
	Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);

	// Create a red pen and select it into the DC
	RedPen = CreatePen(PS_SOLID, 1, 0xff0000);
	SelectObject(Desktop, RedPen);

	// Draw a shap on the DC
	MoveToEx(Desktop, 50, 50, NULL);
	LineTo(Desktop, 200, 60);
	LineTo(Desktop, 200, 300);
	LineTo(Desktop, 50, 50);

	// Create a DC that is compatible with the DISPLAY DC along with a bitmap that is selected into it
	MyDC = CreateCompatibleDC(Desktop);
	MyBitmap = CreateCompatibleBitmap(Desktop, 151, 251);
	SelectObject(MyDC, MyBitmap);

	// Bitblt from the DISPLAY DC to MyDC and then back again in a different location
	BitBlt(MyDC, 0, 0, 151, 251, Desktop, 50, 50, 0);

        // Draw a rectangle on the memory DC before bltting back with a green pen
	GreenPen = CreatePen(PS_SOLID, 1, 0x00ff00);
	SelectObject(MyDC, GreenPen);
        Rectangle(MyDC, 10, 10, 50, 50);

	BitBlt(Desktop, 400, 100, 151, 251, MyDC, 0, 0, 0);

	TextOutA(Desktop, 200, 0, "ReactOS", 7);
	TextOutA(Desktop, 200, 9, "This is a test of the ReactOS GDI functionality", 47);
	TextOutA(Desktop, 200, 18, "The font being used is an internal static one", 45);

	// Free up everything
/*	ReleaseDC(Desktop);
	ReleaseDC(MyDC); */

	return 0;
}
