#include <windows.h>

extern BOOL STDCALL GdiDllInitialize(HANDLE hInstance, DWORD Event, LPVOID Reserved);

void __stdcall Test1BPP (HDC Desktop)
{
  HDC TestDC;
  HPEN WhitePen;
  HBITMAP DIB1;
  BITMAPINFOHEADER BitInf;
  PBITMAPINFO BitPalInf;
  DWORD bmiSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2;

  // Create a 1BPP DIB
  BitPalInf = (PBITMAPINFO)malloc(bmiSize);
  BitInf.biSize = sizeof(BITMAPINFOHEADER);
  BitInf.biWidth = 50;
  BitInf.biHeight = -50; // it's top down (since BI_RGB is used, the sign is operative of direction)
  BitInf.biPlanes = 1;
  BitInf.biBitCount = 1;
  BitInf.biCompression = BI_RGB;
  BitInf.biSizeImage = 0;
  BitInf.biXPelsPerMeter = 0;
  BitInf.biYPelsPerMeter = 0;
  BitInf.biClrUsed = 0;
  BitInf.biClrImportant = 0;
  BitPalInf->bmiHeader = BitInf;
  BitPalInf->bmiColors[1].rgbBlue = 255;
  BitPalInf->bmiColors[1].rgbGreen = 255;
  BitPalInf->bmiColors[1].rgbRed = 255;
  BitPalInf->bmiColors[1].rgbReserved = 255;
  BitPalInf->bmiColors[0].rgbBlue = 0;
  BitPalInf->bmiColors[0].rgbGreen = 0;
  BitPalInf->bmiColors[0].rgbRed = 0;
  BitPalInf->bmiColors[0].rgbReserved = 0;

  DIB1 = (HBITMAP) CreateDIBSection(NULL, BitPalInf, DIB_RGB_COLORS, NULL, NULL, 0);
  TestDC = CreateCompatibleDC(NULL);
  SelectObject(TestDC, DIB1);

  // Draw a white rectangle on the 1BPP DIB
  WhitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
  SelectObject(TestDC, WhitePen);
  Rectangle(TestDC, 0, 0, 40, 40);

  // Blt the 1BPP DIB to the display
  BitBlt(Desktop, 0, 0, 50, 50, TestDC, 0, 0, SRCCOPY);

  free(BitPalInf);
// Rectangle(Desktop, 50, 50, 200, 200);
}

void DIBTest(void)
{
  HDC  Desktop;

  // Set up a DC called Desktop that accesses DISPLAY
  Desktop = CreateDCA("DISPLAY", NULL, NULL, NULL);
  if(Desktop == NULL) {
	printf("Can't create desktop\n");
    return;
  }

  // 1BPP Test
  Test1BPP(Desktop);

  Sleep(10000);

  // Free up everything
  DeleteDC(Desktop);
}

int main(int argc, char* argv[])
{
  printf("Entering DIBTest..\n");

  GdiDllInitialize (NULL, DLL_PROCESS_ATTACH, NULL);
  DIBTest();

  return 0;
}
