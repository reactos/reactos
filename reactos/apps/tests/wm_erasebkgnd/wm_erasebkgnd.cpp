
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 2 - CD-ROM (WM_ERASEBKGND Demo)
//
// Created by Damon Chandler <dmc27@ee.cornell.edu>
// Updates can be downloaded at: <www.coriolis.com>
//
// Please do not hesistate to e-mail me at dmc27@ee.cornell.edu 
// if you have any questions about this code.
// ------------------------------------------------------------------

//*********************************************************//
//                                                         // 
// SYNOPSIS:                                               //
//   This sample project demonstrates how to render        //
//   a background image in response to the WM_ERASEBKGND   //
//   message.  It also shows how to create a transparent   //
//   static (text) control by handling the                 //
//   WM_CTLCOLORSTATIC message.                            //
//                                                         // 
//*********************************************************//


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
   wc.hbrBackground = 
      reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
   wc.lpszClassName = WndClassName;

   if (RegisterClass(&wc))
   {
      HWND HWnd = 
         CreateWindow(WndClassName, 
                      TEXT("WM_ERASEBKGND Demo"),
                      WS_OVERLAPPEDWINDOW | WS_CAPTION | 
                      WS_VISIBLE | WS_CLIPSIBLINGS,
                      CW_USEDEFAULT, CW_USEDEFAULT, 205, 85,
                      NULL, NULL, HInstance, NULL);
                                 
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


// static text and bitmap-related variables
HWND HStatic;
HDC HMemDC;
HBITMAP HBmp, HOldBmp;
const char* filename = "BACKBITMAP.BMP";

LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam, 
   LPARAM LParam)
{
   switch (Msg)
   {
      case WM_CREATE:
      {
         HStatic = 
            CreateWindow(TEXT("STATIC"), TEXT("Static Text"), 
                         WS_CHILD | WS_VISIBLE | SS_CENTER, 
                         10, 20, 175, 30, 
                         HWnd, NULL, HInst, NULL);

         // create a memory DC compatible with the screen
         HMemDC = CreateCompatibleDC(NULL);
         if (HMemDC)
         {
            // load a DDB from file
            HBmp = static_cast<HBITMAP>(
               LoadImage(HInst, filename, IMAGE_BITMAP, 
                         0, 0, LR_LOADFROMFILE)
               );
            if (HBmp)
            {
               // associate the DDB with the memory DC
               HOldBmp = static_cast<HBITMAP>(
                  SelectObject(HMemDC, HBmp)
                  );
            }
         }
      }
      case WM_CTLCOLORSTATIC:
      {
         if (reinterpret_cast<HWND>(LParam) == HStatic)
         {
            HDC HStaticDC = reinterpret_cast<HDC>(WParam);
            SetBkMode(HStaticDC, TRANSPARENT);

            return reinterpret_cast<LRESULT>(
               GetStockObject(NULL_BRUSH)
               );
         }
         break;
      }
      case WM_ERASEBKGND:
      {
         BITMAP bmp;
         if (GetObject(HBmp, sizeof(BITMAP), &bmp))
         {
            RECT RClient;
            GetClientRect(HWnd, &RClient);

            HDC Hdc = reinterpret_cast<HDC>(WParam); 
            SetStretchBltMode(Hdc, COLORONCOLOR);
            
            //
            // TODO: add palette handling code for 
            // palettized displays (see Chapter 9)...
            //

            // render the background image            
            StretchBlt(Hdc, 0, 0, 
                       RClient.right - RClient.left,
                       RClient.bottom - RClient.top,
                       HMemDC, 0, 0, bmp.bmWidth, bmp.bmHeight, 
                       SRCCOPY);
            return TRUE;
         }
         break;
      }
      case WM_DESTROY:
      {
         if (HBmp)
         {
            // free the bitmap
            DeleteObject(SelectObject(HMemDC, HOldBmp));
         }
         // free the memory DC
         DeleteDC(HMemDC);

         PostQuitMessage(0);
         return 0;
      }
   }
   return DefWindowProc(HWnd, Msg, WParam, LParam);
}
//------------------------------------------------------------------



