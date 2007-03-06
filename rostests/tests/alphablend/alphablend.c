#include <windows.h>
#include <string.h>

BOOL WINAPI GdiAlphaBlend(HDC hdcDst, int xDst, int yDst, int widthDst, int heightDst,
                          HDC hdcSrc, int xSrc, int ySrc, int widthSrc, int heightSrc,
                           BLENDFUNCTION blendFunction);

#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA	(0x1)
#endif

HINSTANCE HInst;
const char* WndClassName = "GMainWnd";
LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam,
   LPARAM LParam);

int APIENTRY WinMain(HINSTANCE HInstance, HINSTANCE HPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
   WNDCLASS wc;
   MSG msg;

   HInst = HInstance;

   memset(&wc, 0, sizeof(WNDCLASS));

   wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc = MainWndProc;
   wc.hInstance = HInstance;
   wc.hCursor = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
  /* wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); */
   wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
   wc.lpszClassName = WndClassName;

   if (RegisterClass(&wc))
   {
      HWND HWnd =
         CreateWindow(
            WndClassName, TEXT("AlphaBlend Rendering Demo"),
            WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION |
            WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 320, 430,
            NULL, NULL, HInst, NULL
            );

      if (HWnd)
      {
         ShowWindow(HWnd, nCmdShow);
         UpdateWindow(HWnd);

         while (GetMessage(&msg, NULL, 0, 0))
         {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
         }
      }
    }
    return 0;
}

/* image related */
BITMAP bmp;
LPCSTR filename = TEXT("lena.bmp");
HDC HMemDC = NULL, HMemDC2 = NULL;
HBITMAP HOldBmp = NULL;
PVOID pBmpBits = NULL;
HBITMAP H32BppBitmap = NULL;
BITMAPINFO bmpi;

BOOL ConvertBitmapTo32Bpp(HDC hDC, BITMAP *bmp)
{
  ZeroMemory(&bmpi, sizeof(BITMAPINFO));
  bmpi.bmiHeader.biSize = sizeof(BITMAPINFO);
  bmpi.bmiHeader.biWidth = bmp->bmWidth;
  bmpi.bmiHeader.biHeight = bmp->bmHeight;
  bmpi.bmiHeader.biPlanes = 1;
  bmpi.bmiHeader.biBitCount = 32;
  bmpi.bmiHeader.biCompression = BI_RGB;
  bmpi.bmiHeader.biSizeImage = 4 * bmpi.bmiHeader.biWidth * bmpi.bmiHeader.biHeight;
  H32BppBitmap = CreateDIBSection(hDC, &bmpi, DIB_RGB_COLORS, &pBmpBits, 0, 0);
  if(H32BppBitmap)
  {
    HBITMAP bmpalpha;
    SelectObject(hDC, H32BppBitmap);
    BitBlt(hDC, 0, 0, bmp->bmWidth, bmp->bmHeight, HMemDC, 0, 0, SRCCOPY);

    /* load and apply alpha channel */
    bmpalpha = LoadImage(HInst, MAKEINTRESOURCE(2000), IMAGE_BITMAP,
                            0, 0, 0);
    if(bmpalpha)
    {
      COLORREF *col = pBmpBits;
      int x, y;
      HDC hdcTemp = CreateCompatibleDC(NULL);
      if(!hdcTemp)
      {
        DeleteObject(bmpalpha);
        return FALSE;
      }
      SelectObject(hdcTemp, bmpalpha);

      for(y = 0; y < bmp->bmHeight; y++)
      {
        for(x = 0; x < bmp->bmWidth; x++)
        {
          COLORREF Color = (COLORREF)GetRValue(GetPixel(hdcTemp, x, y)) << 24;
          *col++ |= Color;
        }
      }

      DeleteObject(bmpalpha);
      DeleteDC(hdcTemp);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}

LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam,
   LPARAM LParam)
{
   switch (Msg)
   {
      case WM_CREATE:
      {
         /* create a memory DC */
         HMemDC = CreateCompatibleDC(NULL);
         if (HMemDC)
         {
            /* load a bitmap from file */
            HBITMAP HBmp =
               /* static_cast<HBITMAP> */(
                  LoadImage(HInst, MAKEINTRESOURCE(1000), IMAGE_BITMAP,
                            0, 0, 0)
                            );
            if (HBmp)
            {
               /* extract dimensions of the bitmap */
               GetObject(HBmp, sizeof(BITMAP), &bmp);

               /* associate the bitmap with the memory DC */
               /* HOldBmp = static_cast<HBITMAP> */
		(SelectObject(HMemDC, HBmp)
                  );
                HMemDC2 = CreateCompatibleDC(NULL);
                if(!ConvertBitmapTo32Bpp(HMemDC2, &bmp))
                {
                  PostQuitMessage(0);
                  return 0;
                }
            }
         }
      }
      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         BLENDFUNCTION BlendFunc;
         HDC Hdc = BeginPaint(HWnd, &ps);
#if 0
         try
#endif
         {

            BlendFunc.BlendOp = AC_SRC_OVER;
            BlendFunc.BlendFlags = 0;
            BlendFunc.SourceConstantAlpha = 128;
            BlendFunc.AlphaFormat = 0;

            BitBlt(Hdc, 100, 90,
                   bmp.bmWidth, bmp.bmHeight,
                   HMemDC2, 0, 0,
                   SRCCOPY);
            GdiAlphaBlend(Hdc, 0, 0, bmp.bmWidth, bmp.bmHeight,
                          HMemDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
                          BlendFunc);
            GdiAlphaBlend(Hdc, bmp.bmWidth - 15, 10, bmp.bmWidth / 2, bmp.bmHeight / 2,
                          HMemDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
                          BlendFunc);

            BlendFunc.SourceConstantAlpha = 255;
            BlendFunc.AlphaFormat = AC_SRC_ALPHA;

            GdiAlphaBlend(Hdc, 140, 200, bmp.bmWidth, bmp.bmHeight,
                          HMemDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
                          BlendFunc);
            GdiAlphaBlend(Hdc, 20, 210, (bmp.bmWidth / 3) * 2, (bmp.bmHeight / 3) * 2,
                          HMemDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
                          BlendFunc);
         }
#if 0
         catch (...)
         {
            EndPaint(HWnd, &ps);
         }
#endif
         EndPaint(HWnd, &ps);
         break;
      }
      case WM_DESTROY:
      {
         /* clean up */
         DeleteObject(SelectObject(HMemDC, HOldBmp));
         DeleteDC(HMemDC);
         DeleteDC(HMemDC2);

         PostQuitMessage(0);
         return 0;
      }
   }
   return DefWindowProc(HWnd, Msg, WParam, LParam);
}
