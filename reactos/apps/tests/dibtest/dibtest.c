#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define CELL_SIZE 10

static RGBQUAD Colors[] =
{
  { 0x00, 0x00, 0x00, 0x00 }, // black
  { 0x00, 0x00, 0x80, 0x00 }, // red
  { 0x00, 0x80, 0x00, 0x00 }, // green
  { 0x00, 0x80, 0x80, 0x00 }, // brown
  { 0x80, 0x00, 0x00, 0x00 }, // blue
  { 0x80, 0x00, 0x80, 0x00 }, // magenta
  { 0x80, 0x80, 0x00, 0x00 }, // cyan
  { 0x80, 0x80, 0x80, 0x00 }, // dark gray
  { 0xc0, 0xc0, 0xc0, 0x00 }, // light gray
  { 0x00, 0x00, 0xFF, 0x00 }, // bright red
  { 0x00, 0xFF, 0x00, 0x00 }, // bright green
  { 0x00, 0xFF, 0xFF, 0x00 }, // bright yellow
  { 0xFF, 0x00, 0x00, 0x00 }, // bright blue
  { 0xFF, 0x00, 0xFF, 0x00 }, // bright magenta
  { 0xFF, 0xFF, 0x00, 0x00 }, // bright cyan
  { 0xFF, 0xFF, 0xFF, 0x00 }  // bright white
};

LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  WNDCLASS wc;
  MSG msg;
  HWND hWnd;

  wc.lpszClassName = "DibTestClass";
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

  hWnd = CreateWindow("DibTestClass",
		      "DIB Test",
		      WS_OVERLAPPEDWINDOW,
		      CW_USEDEFAULT,
		      CW_USEDEFAULT,
                      25 * CELL_SIZE + 5,
                      25 * CELL_SIZE + 20,
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

  ShowWindow(hWnd, nCmdShow);

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

static void PaintCells(HDC WindowDC, WORD BitCount1, WORD BitCount2,
                       int XDest, int YDest)
{
  HBRUSH Brush;
  RECT Rect;
  UINT row, col;
  BITMAPINFO *BitmapInfo;
  HBITMAP DIB1, DIB2;
  HDC DC1, DC2;

  BitmapInfo = malloc(sizeof(BITMAPINFO) + 15 * sizeof(RGBQUAD));
  BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BitmapInfo->bmiHeader.biWidth = 4 * CELL_SIZE + 9;
  BitmapInfo->bmiHeader.biHeight = -(4 * CELL_SIZE + 9); // it's top down (since BI_RGB is used, the sign is operative of direction)
  BitmapInfo->bmiHeader.biPlanes = 1;
  BitmapInfo->bmiHeader.biBitCount = BitCount1;
  BitmapInfo->bmiHeader.biCompression = BI_RGB;
  BitmapInfo->bmiHeader.biSizeImage = 0;
  BitmapInfo->bmiHeader.biXPelsPerMeter = 0;
  BitmapInfo->bmiHeader.biYPelsPerMeter = 0;
  BitmapInfo->bmiHeader.biClrUsed = 16;
  BitmapInfo->bmiHeader.biClrImportant = 16;
  for (col = 0; col < 16; col++) {
    BitmapInfo->bmiColors[col] = Colors[col];
  }
  DIB1 = CreateDIBSection(NULL, BitmapInfo, DIB_RGB_COLORS, NULL, NULL, 0);
  DC1 = CreateCompatibleDC(NULL);
  SelectObject(DC1, DIB1);

  BitmapInfo->bmiHeader.biBitCount = BitCount2;
  DIB2 = CreateDIBSection(NULL, BitmapInfo, DIB_RGB_COLORS, NULL, NULL, 0);
  DC2 = CreateCompatibleDC(NULL);
  SelectObject(DC2, DIB2);
  free(BitmapInfo);

  /* Now paint on the first bitmap */
  for (row = 0; row < 4; row++)
    {
    for (col = 0; col < 4; col++)
      {
      Brush = CreateSolidBrush(RGB(Colors[4 * row + col].rgbRed,
                                   Colors[4 * row + col].rgbGreen,
                                   Colors[4 * row + col].rgbBlue));
      Rect.left = CELL_SIZE * col + 5;
      Rect.top = CELL_SIZE * row + 5;
      Rect.right = Rect.left + CELL_SIZE;
      Rect.bottom = Rect.top + CELL_SIZE;
      FillRect(DC1, &Rect, Brush);
      DeleteObject(Brush);
      }
    }

  /* Copy the first bitmap to the second */
  BitBlt(DC2, 4, 4, 4 * CELL_SIZE, 4 * CELL_SIZE, DC1, 5, 5, SRCCOPY);

  /* Show results on screen */
  BitBlt(WindowDC, XDest, YDest, 4 * CELL_SIZE, 4 * CELL_SIZE, DC2, 4, 4, SRCCOPY);
}

LRESULT CALLBACK MainWndProc(HWND Wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC WindowDC;

  switch(msg)
    {
    case WM_PAINT:
      WindowDC = BeginPaint(Wnd, &ps);

      PaintCells(WindowDC, 4,  4,  0,              0);
      PaintCells(WindowDC, 4,  8,  5  * CELL_SIZE, 0);
      PaintCells(WindowDC, 4,  16, 10 * CELL_SIZE, 0);
      PaintCells(WindowDC, 4,  24, 15 * CELL_SIZE, 0);
      PaintCells(WindowDC, 4,  32, 20 * CELL_SIZE, 0);

      PaintCells(WindowDC, 8,  4,  0,              5  * CELL_SIZE);
      PaintCells(WindowDC, 8,  8,  5  * CELL_SIZE, 5  * CELL_SIZE);
      PaintCells(WindowDC, 8,  16, 10 * CELL_SIZE, 5  * CELL_SIZE);
      PaintCells(WindowDC, 8,  24, 15 * CELL_SIZE, 5  * CELL_SIZE);
      PaintCells(WindowDC, 8,  32, 20 * CELL_SIZE, 5  * CELL_SIZE);

      PaintCells(WindowDC, 16, 4,  0,              10 * CELL_SIZE);
      PaintCells(WindowDC, 16, 8,  5  * CELL_SIZE, 10 * CELL_SIZE);
      PaintCells(WindowDC, 16, 16, 10 * CELL_SIZE, 10 * CELL_SIZE);
      PaintCells(WindowDC, 16, 24, 15 * CELL_SIZE, 10 * CELL_SIZE);
      PaintCells(WindowDC, 16, 32, 20 * CELL_SIZE, 10 * CELL_SIZE);

      PaintCells(WindowDC, 24, 4,  0,              15 * CELL_SIZE);
      PaintCells(WindowDC, 24, 8,  5  * CELL_SIZE, 15 * CELL_SIZE);
      PaintCells(WindowDC, 24, 16, 10 * CELL_SIZE, 15 * CELL_SIZE);
      PaintCells(WindowDC, 24, 24, 15 * CELL_SIZE, 15 * CELL_SIZE);
      PaintCells(WindowDC, 24, 32, 20 * CELL_SIZE, 15 * CELL_SIZE);

      PaintCells(WindowDC, 32, 4,  0,              20 * CELL_SIZE);
      PaintCells(WindowDC, 32, 8,  5  * CELL_SIZE, 20 * CELL_SIZE);
      PaintCells(WindowDC, 32, 16, 10 * CELL_SIZE, 20 * CELL_SIZE);
      PaintCells(WindowDC, 32, 24, 15 * CELL_SIZE, 20 * CELL_SIZE);
      PaintCells(WindowDC, 32, 32, 20 * CELL_SIZE, 20 * CELL_SIZE);

      EndPaint(Wnd, &ps);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(Wnd, msg, wParam, lParam);
    }

  return 0;
}
