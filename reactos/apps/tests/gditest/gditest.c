/*
 * gditest
 */

#include <windows.h>

extern BOOL STDCALL GdiDllInitialize(HANDLE hInstance, DWORD Event, LPVOID Reserved);

int main (void)
{
  HDC  Desktop, MyDC, DC24;
  HPEN  RedPen, GreenPen, BluePen, WhitePen;
  HBITMAP  MyBitmap, DIB24;
  HFONT  hf, tf;
  BITMAPINFOHEADER BitInf;
  BITMAPINFO BitPalInf;

  printf("Entering GDITest..\n");

  GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);

  // Set up a DC called Desktop that accesses DISPLAY
  Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);
  if (Desktop == NULL)
    return 1;

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
  SelectObject(Desktop, GreenPen);

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

  Sleep( 10000 );
  // Free up everything
  DeleteDC(Desktop);
  DeleteDC(MyDC);
  return 0;
}
