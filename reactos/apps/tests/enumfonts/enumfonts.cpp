
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 4 - Listing 4.2 (Font Enumeration Demo)
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


#ifndef SNDMSG
#define SNDMSG ::SendMessage
#endif /* ifndef SNDMSG */


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


HWND hListBox = NULL;
const int ID_LISTBOX = 101;

HINSTANCE hInst;
const char* WndClassName = "GMainWnd";
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, 
   LPARAM lParam);


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, 
   int nCmdShow)
{
   hInst = hInstance;

   WNDCLASS wc;
   memset(&wc, 0, sizeof(WNDCLASS));
    
   wc.style = CS_VREDRAW | CS_HREDRAW;
   wc.lpszClassName = WndClassName;
   wc.lpfnWndProc = MainWndProc;
   wc.hInstance = hInstance;
   wc.hCursor = NULL; //LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = reinterpret_cast<HBRUSH>(
      COLOR_BTNFACE + 1
      );

   if (RegisterClass(&wc))
   {
      HWND hWnd = 
         CreateWindow(
            WndClassName, TEXT("Font Enumeration Demo"),
            WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | 
            WS_VISIBLE | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, 295, 285,
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
//-------------------------------------------------------------------------


int CALLBACK MyEnumFontFamExProc(ENUMLOGFONTEX *lpelfe,
   NEWTEXTMETRICEX* lpntme, int FontType, LPARAM lParam)
{
   if (FontType == TRUETYPE_FONTTYPE)
   {
      // if the typeface name is not already in the list
      LPARAM name = reinterpret_cast<LPARAM>(lpelfe->elfFullName);
      if (SNDMSG(hListBox, LB_FINDSTRINGEXACT, (ULONG)-1, name) == LB_ERR)
      {                 
         // add each font to the list box    
         int index = SNDMSG(hListBox, LB_ADDSTRING, 0, name);
   
         // fix the size of the font
         lpelfe->elfLogFont.lfHeight = -20;
         lpelfe->elfLogFont.lfWidth = 0;
   
         // create a new font and store its handle
         // as the data of the newly added item
         HFONT hFont = CreateFontIndirect(&lpelfe->elfLogFont);
         SNDMSG(hListBox, LB_SETITEMDATA, index, 
                reinterpret_cast<LPARAM>(hFont));
      }
   }
   return TRUE;   
}
//-------------------------------------------------------------------------


void AddScreenFonts()
{
   // grab a handle to the screen's DC    
   HDC hScreenDC = GetDC(NULL);    
   try    
   {  
      // 
      // NOTE: Windows 95, 98 and Me requires that the lpLogfont
      // (second) parameter of the EnumFontFamiliesEx function is 
      // non-NULL.  This parameter can be NULL on Windows NT/2000.
      //      
      LOGFONT lf = {0};
      lf.lfCharSet = DEFAULT_CHARSET;

      // enumerate the current screen fonts        
      EnumFontFamiliesEx(
         hScreenDC, &lf, 
         MyEnumFontFamExProc,
         0, 0
         );                         
   }    
   catch (...)    
   {        
      // release the screen's DC        
      ReleaseDC(NULL, hScreenDC);    
   }    
   // release the screen's DC        
   ReleaseDC(NULL, hScreenDC);
}
//-------------------------------------------------------------------------


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
               WS_CHILD | WS_VISIBLE | LBS_STANDARD | 
               LBS_HASSTRINGS | LBS_OWNERDRAWFIXED, 
               20, 10, 250, 250, 
               hWnd, reinterpret_cast<HMENU>(ID_LISTBOX),
               hInst, NULL
               );    
         if (hListBox)
         {
            AddScreenFonts();
         }         
      }
      case WM_MEASUREITEM:
      {
         // grab a pointer to the MEASUREITEMSTRUCT structure
         LPMEASUREITEMSTRUCT lpmis =
            reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

         // test the identifier of the control that
         // the message is meant for
         if (lpmis->CtlID == ID_LISTBOX)
         {
            // adjust the height
            lpmis->itemHeight = 25;

            return TRUE;
         }
         break;
      }
      case WM_DRAWITEM:
      {
         // grab a pointer to the DRAWITEMSTRUCT structure
         LPDRAWITEMSTRUCT lpdis =
            reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

         // test the identifier of the control that
         // the message is meant for
         if (lpdis->CtlID == ID_LISTBOX)
         {
            COLORREF OldColor = GetTextColor(lpdis->hDC);
            int stock = WHITE_BRUSH;

            // if the item is currently selected
            if (lpdis->itemState & ODS_SELECTED)
            {
               // set the text color to white and 
               // the background color to black
               COLORREF clrWhite = PALETTERGB(255, 255, 255);
               OldColor = SetTextColor(lpdis->hDC, clrWhite);
               stock = BLACK_BRUSH;
            }
            FillRect(
               lpdis->hDC, &lpdis->rcItem, 
               static_cast<HBRUSH>(GetStockObject(stock))
               );

            if (lpdis->itemID != -1)
            {
               // extract the item's text
               char text[MAX_PATH];
               SNDMSG(hListBox, LB_GETTEXT, lpdis->itemID,
                      reinterpret_cast<LPARAM>(text)); 

               // extract the corresponding font handle
               DWORD value =
                   SNDMSG(hListBox, LB_GETITEMDATA, lpdis->itemID, 0);
               HFONT hFont = reinterpret_cast<HFONT>(value);

               // select the corresponding font
               // into the device context
               HFONT HOldFont = static_cast<HFONT>(
                  SelectObject(lpdis->hDC, hFont)
                  );

               // render the text transparently
               SetBkMode(lpdis->hDC, TRANSPARENT);

               // render the text
               RECT RText = lpdis->rcItem;
               InflateRect(&RText, -2, -2);
               DrawText(lpdis->hDC, text, strlen(text), &RText, 
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

               // restore the previous font
               SelectObject(lpdis->hDC, HOldFont);            
            }

            // render the focus rectangle
            if (lpdis->itemState & ODS_FOCUS)
            {
               DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
            }

            // restore the previous color/mode
            SetTextColor(lpdis->hDC, OldColor);
            SetBkMode(lpdis->hDC, OPAQUE);

            return TRUE;
         }
         break;
      }
      case WM_DESTROY:
      {
         // delete each created font object    
         int count = SNDMSG(hListBox, LB_GETCOUNT, 0, 0);
         for (int index = 0; index < count; ++index)
         {
            DWORD value = SNDMSG(hListBox, LB_GETITEMDATA, index, 0);
            DeleteObject(reinterpret_cast<HFONT>(value));
         }         

         PostQuitMessage(0);
         break;
      }
   }
   return DefWindowProc(hWnd, msg, wParam, lParam);
}
//-------------------------------------------------------------------------



