
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 1 - Listing 1.3 (BitBlt Bitmap Rendering Demo)
//
// Created by Damon Chandler <dmc27@ee.cornell.edu>
// Updates can be downloaded at: <www.coriolis.com>
//
// Please do not hesistate to e-mail me at dmc27@ee.cornell.edu 
// if you have any questions about this code.
// ------------------------------------------------------------------


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#include <windows.h>
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


HINSTANCE HInst;
const char* WndClassName = "GMainWnd";
LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam, 
   LPARAM LParam);


int APIENTRY WinMain(HINSTANCE HInstance, HINSTANCE HPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
   HInst = HInstance;

   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
    
   wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
   wc.lpfnWndProc = MainWndProc;
   wc.hInstance = HInstance;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
   wc.lpszClassName = WndClassName;

   if (RegisterClass(&wc))
   {
      HWND HWnd = 
         CreateWindow(
            WndClassName, TEXT("BitBlt Bitmap Rendering Demo"),
            WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | 
            WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 220, 230,
            NULL, NULL, HInst, NULL
            );
                                 
      if (HWnd)
      {
         ShowWindow(HWnd, nCmdShow);
         UpdateWindow(HWnd);

         MSG msg;
         while (GetMessage(&msg, NULL, 0, 0))
         {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
         }      
      }
    }
    return 0;
}
//------------------------------------------------------------------


// image related
BITMAP bmp;
LPCSTR filename = TEXT("lena.bmp");
HDC HMemDC = NULL;
HBITMAP HOldBmp = NULL;

LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam, 
   LPARAM LParam)
{
   switch (Msg)
   {
      case WM_CREATE:
      {         
         // create a memory DC
         HMemDC = CreateCompatibleDC(NULL);
         if (HMemDC)
         {
            // load a bitmap from file
            HBITMAP HBmp = 
               static_cast<HBITMAP>(
                  LoadImage(HInst, filename, IMAGE_BITMAP, 
                            0, 0, LR_LOADFROMFILE)
                            );  
            if (HBmp)
            { 
               // extract dimensions of the bitmap
               GetObject(HBmp, sizeof(BITMAP), &bmp);

               // associate the bitmap with the memory DC
               HOldBmp = static_cast<HBITMAP>(
                  SelectObject(HMemDC, HBmp)
                  );
            }
         }         
      }
      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         const HDC Hdc = BeginPaint(HWnd, &ps);
#if 0
         try
#endif
         {
            //
            // TODO: add palette support (see Chapter 9)...
            //

            BitBlt(Hdc, 20, 15, 
                   bmp.bmWidth, bmp.bmHeight,
                   HMemDC, 0, 0, 
                   SRCCOPY);  
         }           
#if 0
         catch (...)
#endif
         {
            EndPaint(HWnd, &ps);
         }
         EndPaint(HWnd, &ps);
         break;
      }
      case WM_DESTROY:
      {
         // clean up
         DeleteObject(SelectObject(HMemDC, HOldBmp));
         DeleteDC(HMemDC);

         PostQuitMessage(0);
         return 0;
      }
   }
   return DefWindowProc(HWnd, Msg, WParam, LParam);
}
//------------------------------------------------------------------
