
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 5 - Listing 5.1 (Output Primitives Demo)
//
// Created by Damon Chandler <dmc27@ee.cornell.edu>
// Updates can be downloaded at: <www.coriolis.com>
//
// Please do not hesistate to e-mail me at dmc27@ee.cornell.edu 
// if you have any questions about this code.
// ------------------------------------------------------------------


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#include <windows.h>
#include <cassert>

// for the MakeFont() function...
#include "mk_font.h"
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


HINSTANCE hInst;
const char* WndClassName = "GMainWnd";
LRESULT CALLBACK MainWndProc(HWND HWnd, UINT Msg, WPARAM WParam, 
   LPARAM LParam);


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, 
   int nCmdShow)
{
   hInst = hInstance;

   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
    
   wc.style = CS_VREDRAW | CS_HREDRAW;
   wc.lpszClassName = WndClassName;
   wc.lpfnWndProc = MainWndProc;
   wc.hInstance = hInst;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = reinterpret_cast<HBRUSH>(
      COLOR_BTNFACE + 1
      );

   if (RegisterClass(&wc))
   {
      HWND hWnd = 
         CreateWindow(
            WndClassName, TEXT("Output Primitives Demo"),
            WS_OVERLAPPEDWINDOW | WS_CAPTION | 
            WS_VISIBLE | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
            NULL, NULL, hInst, NULL
            );
                                 
      if (hWnd)
      {
         ShowWindow(hWnd, nCmdShow);
         UpdateWindow(hWnd);

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


enum OutPrimitive {
   opLine, opBezier, opRectangle, opRoundRect,
   opEllipse, opArc, opPie, opChord, opCustom
   };

void DrawPrimitive(IN HDC hDC, IN const RECT& RPrimitive, 
   IN OutPrimitive PrimitiveID)
{
   RECT R = RPrimitive;
   InflateRect(&R, -10, -10);

   switch (PrimitiveID)
   {
      case opLine:
      {
         const POINT PLine[] = {{R.left, R.top}, {R.right, R.bottom}};
         Polyline(hDC, PLine, 2);
         break;
      }
      case opBezier:
      {
         const POINT PControlPoints[] = {
            {R.left, R.top},
            {(R.right - R.left) / 2, R.top},
            {(R.right - R.left) / 2, R.bottom},
            {R.right, R.bottom}
            };         
         PolyBezier(hDC, PControlPoints, 4);         
         break;
      }
      case opRectangle:
      {
         Rectangle(hDC, R.left, R.top, R.right, R.bottom);
         break;
      }
      case opRoundRect:
      {
         RoundRect(hDC, R.left, R.top, R.right, R.bottom, 20, 20);
         break;
      }
      case opEllipse:
      {
         Ellipse(hDC, R.left, R.top, R.right, R.bottom);
         break;
      }
      case opArc:
      {
         const POINT PRads[] = {
            {(R.right - R.left) / 3 + R.left, R.top},
            {(R.right - R.left) / 3 + R.left, R.bottom}
            };         
         Arc(hDC, R.left, R.top, R.right, R.bottom, 
             PRads[0].x, PRads[0].y, PRads[1].x, PRads[1].y);
         break;
      }
      case opPie:
      {
         const POINT PRads[] = {
            {(R.right - R.left) / 3 + R.left, R.top},
            {(R.right - R.left) / 3 + R.left, R.bottom}
            };         
         Pie(hDC, R.left, R.top, R.right, R.bottom, 
             PRads[0].x, PRads[0].y, PRads[1].x, PRads[1].y);
         break;
      }
      case opChord:
      {
         const POINT PRads[] = {
            {(R.right - R.left) / 3 + R.left, R.top},
            {(R.right - R.left) / 3 + R.left, R.bottom}
            };         
         Chord(hDC, R.left, R.top, R.right, R.bottom, 
             PRads[0].x, PRads[0].y, PRads[1].x, PRads[1].y);
         break;
      }
      case opCustom:
      {
         const POINT PVertices[] = {
            {R.left, (R.bottom - R.top) / 2 + R.top},
            {(R.right - R.left) / 2 + R.left, R.top},
            {R.right, (R.bottom - R.top) / 2 + R.top},
            {(R.right - R.left) / 2 + R.left, R.bottom}            
            };         
         Polygon(hDC, PVertices, 4);
         break;
      }
   }
}
//------------------------------------------------------------------


HWND hListBox = NULL;      // handle to the list box
HBRUSH hListBrush = NULL;  // handle to the list box brush
HPEN hListPen = NULL;      // handle to the list box pen
HFONT hListFont = NULL;    // handle to the list box font

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, 
   LPARAM lParam)
{
   switch (msg)
   {
      case WM_CREATE:
      {
         hListBox = 
            CreateWindowEx(
               WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""), 
               WS_CHILD | WS_VISIBLE | WS_VSCROLL |
               LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | 
               LBS_OWNERDRAWFIXED, 
               0, 0, 640, 480, 
               hWnd, NULL, hInst, NULL
               );
         assert(hListBox != NULL);

         SetWindowLong(
            hListBox, GWL_ID, reinterpret_cast<LONG>(hListBox)
            );

         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Line")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Bezier Curve")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Rectangle")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Rounded Rectangle")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Ellipse")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Arc")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Pie Slice")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Chord")));
         SNDMSG(hListBox, LB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(TEXT("Custom")));

         hListBrush = CreateSolidBrush(PALETTERGB(64, 192, 64));
         hListPen = CreatePen(PS_SOLID, 3, PALETTERGB(0, 0, 0));
         HDC hScreenDC = GetDC(NULL);
#if 0
         try
#endif
         {
            // MakeFont() from Chapter 4
            hListFont = font::MakeFont(
               hScreenDC, "Impact", 20, ANSI_CHARSET, 
               font::FS_BOLD | font::FS_UNDERLINE
               ); 
         }
#if 0
         catch (...)
#endif
         {
            ReleaseDC(NULL, hScreenDC);
         }
         ReleaseDC(NULL, hScreenDC);

         break;
      }
      case WM_MEASUREITEM:
      {
         LPMEASUREITEMSTRUCT lpmis = 
            reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
         assert(lpmis != NULL);

         if (lpmis->CtlID == reinterpret_cast<UINT>(hListBox))
         {
            lpmis->itemHeight = 150;            
            return TRUE;         
         }
         break;
      }
      case WM_DRAWITEM:
      {
         LPDRAWITEMSTRUCT lpdis = 
            reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
         assert(lpdis != NULL);

         if (lpdis->CtlID == reinterpret_cast<UINT>(hListBox))
         {            
            SaveDC(lpdis->hDC);
#if 0
            try
#endif
            {              
               SelectObject(lpdis->hDC, hListBrush);
               SelectObject(lpdis->hDC, hListPen);
               SelectObject(lpdis->hDC, hListFont);

               bool selected = (lpdis->itemState & ODS_SELECTED);
               COLORREF clrText = GetSysColor(
                  selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT
                  );
               HBRUSH hListBrush = GetSysColorBrush(
                  selected ? COLOR_HIGHLIGHT : COLOR_WINDOW
                  );

               // fill the background
               FillRect(lpdis->hDC, &lpdis->rcItem, hListBrush);

               // render the output primitive
               RECT RPrimitive = lpdis->rcItem;
               InflateRect(&RPrimitive, -5, -5);
               RPrimitive.right = static_cast<int>(
                  0.6 * lpdis->rcItem.right + 0.5
                  );
               FillRect(
                  lpdis->hDC, &RPrimitive, 
                  reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1)
                  );
               DrawPrimitive(
                  lpdis->hDC, RPrimitive, 
                  static_cast<OutPrimitive>(lpdis->itemID)
                  );
               if (selected) InvertRect(lpdis->hDC, &RPrimitive);
               DrawEdge(lpdis->hDC, &RPrimitive, EDGE_SUNKEN, BF_RECT);

               // render the text
               TCHAR text[13];
               if (SNDMSG(hListBox, LB_GETTEXT, lpdis->itemID, 
                          reinterpret_cast<LPARAM>(&text)) != LB_ERR)
               {           
                  RECT RText = RPrimitive;
                  RText.left =  RPrimitive.right;
                  RText.right = lpdis->rcItem.right; 

                  SelectObject(lpdis->hDC, hListFont);
                  SetBkMode(lpdis->hDC, TRANSPARENT);
                  SetTextColor(lpdis->hDC, clrText);
            
                  DrawText(lpdis->hDC, text, -1, &RText, 
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE);
               }

               // indicate keyboard focus
               if (lpdis->itemState & ODS_FOCUS)
               {  
                  RECT RFocus = lpdis->rcItem;
                  InflateRect(&RFocus, -1, -1);
                  DrawFocusRect(lpdis->hDC, &RFocus);
               }
            }
#if 0
            catch (...)
#endif
            {
               RestoreDC(lpdis->hDC, -1);
            }
            RestoreDC(lpdis->hDC, -1);
            return TRUE;
         }
         break;
      }
      case WM_SIZE:
      {
         MoveWindow(
            hListBox, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE
            ); 
         return 0;
      }
      case WM_DESTROY:
      {
         if (hListBrush) DeleteObject(hListBrush);
         if (hListPen) DeleteObject(hListPen);
         if (hListFont) DeleteObject(hListFont);

         PostQuitMessage(0);
         return 0;
      }
   }
   return DefWindowProc(hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------



