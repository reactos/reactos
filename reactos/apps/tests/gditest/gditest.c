/*
 * gditest
 */

#include <windows.h>

extern BOOL STDCALL GdiDllInitialize(HANDLE hInstance, DWORD Event, LPVOID Reserved);

int main (void)
{
  HDC  Desktop, MyDC, DC24;
  HPEN  RedPen, GreenPen, BluePen;
  HBITMAP  MyBitmap, DIB24;
  HFONT  hf;
  BITMAPINFOHEADER BitInf;
  BITMAPINFO BitPalInf;

  printf("Entering GDITest..\n");

//  GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);

  // Set up a DC called Desktop that accesses DISPLAY
  Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);
  if (Desktop == NULL)
    return 1;

  // Create a red pen and select it into the DC
  BluePen = CreatePen(PS_SOLID, 8, 0x0000ff);
  SelectObject(Desktop, BluePen);

  // Draw a shape on the DC
  MoveToEx(Desktop, 50, 50, NULL);
  LineTo(Desktop, 200, 60);
  LineTo(Desktop, 200, 300);
  LineTo(Desktop, 50, 50);
  MoveToEx(Desktop, 50, 50, NULL);
  LineTo(Desktop, 200, 50);

  // TEST 1: Copy from the VGA into a device compatible DC, draw on it, then blt it to the VGA again
  MyDC = CreateCompatibleDC(Desktop);
  MyBitmap = CreateCompatibleBitmap(Desktop, 151, 251);
  SelectObject(MyDC, MyBitmap);
  BitBlt(MyDC, 0, 0, 152, 251, Desktop, 50, 50, SRCCOPY);

  GreenPen = CreatePen(PS_SOLID, 3, 0x00ff00);
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
  RedPen = CreatePen(PS_SOLID, 3, 0xff0000);
  SelectObject(DC24, RedPen);
  Rectangle(DC24, 80, 90, 100, 110);
  MoveToEx(DC24, 80, 90, NULL);
  LineTo(DC24, 100, 110);
  BitBlt(Desktop, 200, 200, 110, 120, DC24, 0, 0, SRCCOPY);

  // Text tests
/*        hf = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "System"); */
//  TextOutA(Desktop, 200, 0, "ReactOS", 7);
//  TextOutA(Desktop, 200, 9, "This is a test of the ReactOS GDI functionality", 47);
//  TextOutA(Desktop, 200, 18, "The font being used is an internal static one", 45);

  Sleep( 10000 );
  // Free up everything
  DeleteDC(Desktop);
  DeleteDC(MyDC);
  return 0;
}
