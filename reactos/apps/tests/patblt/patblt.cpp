
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 2 - Listing 2.1 (PatBlt Tracking Rect Demo)
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


int APIENTRY WinMain(HINSTANCE HInstance, HINSTANCE, LPTSTR, 
   int nCmdShow)
{
   HInst = HInstance;

   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
    
   wc.style = CS_VREDRAW | CS_HREDRAW;
   wc.lpszClassName = WndClassName;
   wc.lpfnWndProc = MainWndProc;
   wc.hInstance = HInstance;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = static_cast<HBRUSH>(
      GetStockObject(BLACK_BRUSH)
      );

   if (RegisterClass(&wc))
   {
      HWND HWnd = 
         CreateWindow(WndClassName, 
                      TEXT("PatBlt Tracking Rect Demo"),
                      WS_OVERLAPPEDWINDOW | WS_CAPTION | 
                      WS_VISIBLE | WS_CLIPCHILDREN,
                      CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                      NULL, NULL, HInst, NULL);
                                 
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
HDC HMemDC = NULL;
HBITMAP HOldBmp = NULL;
const char* filename = "PENGUIN.BMP";
RECT RImage = {225, 110, 225, 110};

// tracking related
bool is_tracking = false;
HDC HScreenDC = NULL;
POINT PMouse = {0, 0};
RECT RTrack = {0, 0, 0, 0};
const int line_width = 5;


// utility function to map to/from window coordinates
void MapRect(IN HWND HWndFrom, IN HWND HWndTo, IN OUT RECT& RMap)
{
   MapWindowPoints(
      HWndFrom, HWndTo,
      reinterpret_cast<LPPOINT>(&RMap), 2
      );
}
//------------------------------------------------------------------


// utility function that uses the PatBlt function to
// render a tracking rectangle
void RenderTrackingRect(IN HDC HDestDC, IN const RECT& RRender)
{
   const int width = RRender.right - RRender.left;
   const int height = RRender.bottom - RRender.top;
   const DWORD dwROP3 = DSTINVERT; // experiment with others

   // render top bar
   PatBlt(HDestDC, 
          RRender.left, RRender.top, 
          width, line_width, 
          dwROP3);
   // render bottom bar
   PatBlt(HDestDC, 
          RRender.left, RRender.bottom - line_width, 
          width, line_width, 
          dwROP3);
   // render left bar
   PatBlt(HDestDC, 
          RRender.left, RRender.top + line_width, 
          line_width, height - (2 * line_width), 
          dwROP3);
   // render right bar
   PatBlt(HDestDC, 
          RRender.right - line_width, RRender.top + line_width, 
          line_width, height - (2 * line_width), 
          dwROP3);

}
//------------------------------------------------------------------


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
            // load the penguin bitmap
            HBITMAP HBmp = static_cast<HBITMAP>(
               LoadImage(HInst, filename, IMAGE_BITMAP, 0, 0, 
                         LR_LOADFROMFILE | LR_DEFAULTSIZE)
               );         
            if (HBmp)
            {
               // get the bitmap's dimensions
               BITMAP bmp;
               if (GetObject(HBmp, sizeof(BITMAP), &bmp))
               {  
                  RImage.right += bmp.bmWidth;
                  RImage.bottom += bmp.bmHeight;

                  // realize the bitmap
                  HOldBmp = static_cast<HBITMAP>(
                     SelectObject(HMemDC, HBmp)
                     );
               }
               else DeleteObject(HBmp);
            }
         }
         break;
      }
      case WM_LBUTTONDOWN:
      {
         PMouse.x = LOWORD(LParam);
         PMouse.y = HIWORD(LParam);

         RECT RClient;
         if (PtInRect(&RImage, PMouse) && 
             GetClientRect(HWnd, &RClient))
         {   
            MapRect(HWnd, HWND_DESKTOP, RClient);
            ClipCursor(&RClient);

            // grab a handle to the screen DC and clip
            // all output to the client area of our window
            HScreenDC = GetDC(NULL);
            HRGN HClipRgn = CreateRectRgnIndirect(&RClient);
            SelectClipRgn(HScreenDC, HClipRgn);
            DeleteObject(HClipRgn);
            
            CopyRect(&RTrack, &RImage);
            MapRect(HWnd, HWND_DESKTOP, RTrack);                  

            // render the first tracking rect
            RenderTrackingRect(HScreenDC, RTrack);
            is_tracking = true;
         }                                       
         break;
      }
      case WM_MOUSEMOVE:
      {
         if (HScreenDC && is_tracking)
         {  
            POINT PCurrent = {LOWORD(LParam), HIWORD(LParam)};       
            const int dX = PCurrent.x - PMouse.x;
            const int dY = PCurrent.y - PMouse.y; 

            // erase the previous rectangle
            RenderTrackingRect(HScreenDC, RTrack);
            // update the postion
            OffsetRect(&RTrack, dX, dY);
            // render the new tracking rectangle
            RenderTrackingRect(HScreenDC, RTrack);

            // update the mouse position
            memcpy(&PMouse, &PCurrent, sizeof(POINT));
         }
         break;
      }
      case WM_LBUTTONUP:
      {
         // clean up
         if (is_tracking)
         {
            is_tracking = false;
            SelectClipRgn(HScreenDC, NULL);
            ReleaseDC(NULL, HScreenDC);

            InvalidateRect(HWnd, &RImage, true);
            CopyRect(&RImage, &RTrack);
            MapRect(HWND_DESKTOP, HWnd, RImage);
            InvalidateRect(HWnd, &RImage, true);

            ClipCursor(NULL);
         }
         break;
      }
      case WM_PAINT:
      {  
         PAINTSTRUCT ps;
         HDC Hdc = BeginPaint(HWnd, &ps);
#if 0
         try
#endif
         {
            // 
            // TODO: Add palette support...
            //

            // render the penguin
            BitBlt(Hdc, RImage.left, RImage.top,
                   RImage.right - RImage.left,
                   RImage.bottom - RImage.top,
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
         if (HOldBmp) 
         {
            DeleteObject(SelectObject(HMemDC, HOldBmp));
         }
         if (HMemDC)
         {
            DeleteDC(HMemDC);
         }
         PostQuitMessage(0);
         return 0;
      }
   }
   return DefWindowProc(HWnd, Msg, WParam, LParam);
}
//------------------------------------------------------------------
