
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 1 - Listing 1.1 (WM_PAINT Demo)
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


const char* WndClassName = "GMainWnd";
LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam, 
   LPARAM LParam);


int APIENTRY WinMain(HINSTANCE HInstance, HINSTANCE HPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
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
         CreateWindow(WndClassName, TEXT("WM_PAINT Demo"),
                      WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_VISIBLE,
                      CW_USEDEFAULT, CW_USEDEFAULT, 200, 150,
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


LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam, 
   LPARAM LParam)
{
   switch (Msg)
   {
      case WM_PAINT:
      {
         // determine the invalidated area of the window            
         RECT RUpdate;            
         GetUpdateRect(HWnd, &RUpdate, false);

         // grab a handle to our window's
         // common display device context
         HDC Hdc = GetDC(HWnd);
#if 0
         try
#endif
         {
            RECT RClient;
            GetClientRect(HWnd, &RClient);

            // set the clipping region
            IntersectClipRect(Hdc, RUpdate.left, RUpdate.top, 
                              RUpdate.right, RUpdate.bottom);

            // fill the client area with the background brush
            HBRUSH HBrush = 
               reinterpret_cast<HBRUSH>(
                  GetClassLong(HWnd, GCL_HBRBACKGROUND)
                  );
            FillRect(Hdc, &RClient, HBrush);
            
            // render the persistent text
            const char* text = "Persistent Text"; 
            SetTextColor(Hdc, PALETTERGB(0, 0, 255));
            DrawText(Hdc, text, strlen(text), &RClient,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
         }
#if 0
         catch (...)
#endif
         {
            // release the device context
            ReleaseDC(HWnd, Hdc);

            // validate the update area            
            ValidateRect(HWnd, &RUpdate);
         }
         // release the device context
         ReleaseDC(HWnd, Hdc);

         // validate the update area            
         ValidateRect(HWnd, &RUpdate);

         break;
      }
      case WM_DESTROY:
      {
         PostQuitMessage(0);
         return 0;
      }
   }
   return DefWindowProc(HWnd, Msg, WParam, LParam);
}
//------------------------------------------------------------------

