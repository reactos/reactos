
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 1 - Listing 1.5 (StretchBlt Zooming Demo)
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


HWND HListBox = NULL;
const int ID_LISTBOX = 101;

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
         CreateWindow(WndClassName, TEXT("StretchBlt Zooming Demo"),
                      WS_OVERLAPPEDWINDOW | WS_CAPTION | 
                      WS_VISIBLE | WS_CLIPSIBLINGS,
                      0, 0, 675, 560,
                      NULL, NULL, HInst, NULL);
                                 
      if (HWnd)
      {
         HListBox =
            CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "", 
                           LBS_NOTIFY | WS_CHILD | WS_VISIBLE, 
                           530, 5, 130, 150, HWnd, 
                           reinterpret_cast<HMENU>(ID_LISTBOX),
                           HInst, NULL);
         if (HListBox)
         {
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 25%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 50%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 75%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 100%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 125%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 150%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 200%"));
            SNDMSG(HListBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>("Zoom 300%"));
         }

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
const char* filename = "LENA.BMP";
HDC HMemDC = NULL;
HBITMAP HOldBmp = NULL;

// zooming related
float zoom_factor = 0.5;
RECT RDest = {5, 5, 0, 0};
enum {ID_ZOOM25, ID_ZOOM50, ID_ZOOM75, ID_ZOOM100, 
      ID_ZOOM125, ID_ZOOM150, ID_ZOOM200, ID_ZOOM300};

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
      case WM_COMMAND:
      {
         if (WParam == MAKEWPARAM(ID_LISTBOX, LBN_SELCHANGE))
         {
            switch (SNDMSG(HListBox, LB_GETCURSEL, 0, 0))
            {
               case ID_ZOOM25: zoom_factor = 0.25; break;
               case ID_ZOOM50: zoom_factor = 0.50; break;
               case ID_ZOOM75: zoom_factor = 0.75; break;
               case ID_ZOOM100: zoom_factor = 1.00; break;
               case ID_ZOOM125: zoom_factor = 1.25; break;
               case ID_ZOOM150: zoom_factor = 1.50; break;
               case ID_ZOOM200: zoom_factor = 2.00; break;
               case ID_ZOOM300: zoom_factor = 3.00; break;
            }

            // calculate the new width and height
            const int new_width = 
               static_cast<int>(zoom_factor * bmp.bmWidth);
            const int new_height = 
               static_cast<int>(zoom_factor * bmp.bmHeight);

            // is zooming in?
            bool zoom_in = (new_width > RDest.right - RDest.left);

            // caculate the area that needs to be updated 
            RECT RUpdate = {
               RDest.left, RDest.top,
               RDest.left + max(new_width, RDest.right - RDest.left),
               RDest.top + max(new_height, RDest.bottom - RDest.top)
               };

            // adjust the dimenstions of the
            // destination rectangle
            RDest.right = RDest.left + new_width;               
            RDest.bottom = RDest.top + new_height;            

            // create an update region from the XOR combination
            // of the update and destination rectangles
            HRGN HUpdateRgn = CreateRectRgnIndirect(&RUpdate);
            HRGN HDestRgn = CreateRectRgnIndirect(&RDest);
            int result = 
               CombineRgn(HUpdateRgn, HUpdateRgn, HDestRgn, RGN_XOR);

            // incite a repaint
            if (result != NULLREGION && result != ERROR)
            {              
               InvalidateRgn(HWnd, HUpdateRgn, true);               
               RedrawWindow(HWnd, &RDest, NULL, RDW_NOERASE | RDW_INVALIDATE);
            }
            else if (result == NULLREGION)
            {
               InvalidateRect(HWnd, &RUpdate, zoom_in ? false : true);
            }

            // clean up
            DeleteObject(HUpdateRgn);
            DeleteObject(HDestRgn);
         }
         break;
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

            if (RDest.right - RDest.left > 0)
            {
               // use BitBlt when not zooming
               if (zoom_factor == 1.0)
               {
                  BitBlt(Hdc, RDest.left, RDest.top, 
                         RDest.right - RDest.left, 
                         RDest.bottom - RDest.top,
                         HMemDC, 0, 0, 
                         SRCCOPY);  
               }
               else
               {
                  if (zoom_factor < 1.0)
                  {
                     SetStretchBltMode(Hdc, COLORONCOLOR);
                  }

                  // render the zoomed image
                  StretchBlt(Hdc, RDest.left, RDest.top, 
                             RDest.right - RDest.left, 
                             RDest.bottom - RDest.top,
                             HMemDC, 0, 0, 
                             bmp.bmWidth, bmp.bmHeight,
                             SRCCOPY);  
               }
            }
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



