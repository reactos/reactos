/*  LOOKPREV.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**
**  History:
**
*/
#include "pch.hxx" // PCH
#pragma hdrstop

#include "AccWiz.h"

#include "desk.h"
//#include "deskid.h"
#include "resource.h"
#include "look.h"

#include "LookPrev.h"

#define RCZ(element)         g_elements[element].rc


//////////////////////////////////
// Support function
void MyDrawBorderBelow(HDC hdc, LPRECT prc);
void MyDrawFrame(HDC hdc, LPRECT prc, HBRUSH hbrColor, int cl);


HDC g_hdcMem;
TCHAR g_szABC[] = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
int cxSize;

//////////////////////////////////////////////////////////////
// Declarations of static variables declared in classes
BOOL CLookPreviewGlobals::sm_bOneInstanceCreated = FALSE; // This variable insures that only one instance of CLookPreviewGlobals is created
CLookPreviewGlobals CLookPrev::sm_Globals;

//////////////////////////////////////////////////////////////
// CLookPreviewGlobals member functions
//

BOOL CLookPreviewGlobals::Initialize()
{
   if(m_bInitialized)
      return TRUE;

   m_bInitialized = TRUE;

   // Make sure there is only one instance of this class created
   _ASSERT(!sm_bOneInstanceCreated);
   sm_bOneInstanceCreated = TRUE;

   //
   // Load our display strings.
   //
   VERIFY(LoadString(g_hInstDll, IDS_ACTIVE, m_szActive, ARRAYSIZE(m_szActive)));
   VERIFY(LoadString(g_hInstDll, IDS_INACTIVE, m_szInactive, ARRAYSIZE(m_szInactive)));
   VERIFY(LoadString(g_hInstDll, IDS_MINIMIZED, m_szMinimized, ARRAYSIZE(m_szMinimized)));
   VERIFY(LoadString(g_hInstDll, IDS_ICONTITLE, m_szIconTitle, ARRAYSIZE(m_szIconTitle)));
   VERIFY(LoadString(g_hInstDll, IDS_NORMAL, m_szNormal, ARRAYSIZE(m_szNormal)));
   VERIFY(LoadString(g_hInstDll, IDS_DISABLED, m_szDisabled, ARRAYSIZE(m_szDisabled)));
   VERIFY(LoadString(g_hInstDll, IDS_SELECTED, m_szSelected, ARRAYSIZE(m_szSelected)));
   VERIFY(LoadString(g_hInstDll, IDS_MSGBOX, m_szMsgBox, ARRAYSIZE(m_szMsgBox)));
   VERIFY(LoadString(g_hInstDll, IDS_BUTTONTEXT, m_szButton, ARRAYSIZE(m_szButton)));
//    VERIFY(LoadString(g_hInstDll, IDS_SMCAPTION, m_szSmallCaption, ARRAYSIZE(m_szSmallCaption)));
   VERIFY(LoadString(g_hInstDll, IDS_WINDOWTEXT, m_szWindowText, ARRAYSIZE(m_szWindowText)));
   VERIFY(LoadString(g_hInstDll, IDS_MSGBOXTEXT, m_szMsgBoxText, ARRAYSIZE(m_szMsgBoxText)));

   /////////////////////////////////////////////
   // Register Look Preview window class
   WNDCLASS wc;
   memset(&wc, 0, sizeof(wc));
   wc.style = 0;
   wc.lpfnWndProc = CLookPrev::LookPreviewWndProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = g_hInstDll;
   wc.hIcon = NULL;
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = LOOKPREV_CLASS;

   VERIFY(RegisterClass(&wc));

   /////////////////////////////////////////////
   // Global initialization of g_hdcMem
   HBITMAP hbmDefault;
   HDC hdc = GetDC(NULL);
   VERIFY(g_hdcMem = CreateCompatibleDC(hdc));
   ReleaseDC(NULL, hdc);

   HBITMAP hbm = CreateBitmap(1, 1, 1, 1, NULL);
   hbmDefault = (HBITMAP)SelectObject(g_hdcMem, hbm);
   SelectObject(g_hdcMem, hbmDefault);
   DeleteObject(hbm);


   /////////////////////////////////////////////
   // Old initialization form Look_InitSysStuff()
   int i;
   NONCLIENTMETRICS ncm;
   HKEY hkey;

   hdc = GetDC(NULL);
   g_LogDPI = GetDeviceCaps(hdc, LOGPIXELSY);
   g_bPalette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
   ReleaseDC(NULL, hdc);

   // always make a palette even on non-pal device
   if (g_bPalette || TRUE)
   {
      DWORD pal[21];
      HPALETTE hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

      pal[1]   = RGB(255, 255, 255);
      pal[2]   = RGB(0,   0,  0  );
      pal[3]   = RGB(192, 192, 192);
      pal[4]   = RGB(128, 128, 128);
      pal[5]   = RGB(255, 0,  0  );
      pal[6]   = RGB(128, 0,  0  );
      pal[7]   = RGB(255, 255, 0  );
      pal[8]   = RGB(128, 128, 0  );
      pal[9]   = RGB(0  , 255, 0  );
      pal[10] = RGB(0  , 128, 0  );
      pal[11] = RGB(0  , 255, 255);
      pal[12] = RGB(0 , 128, 128); // Needs to be changed to get Blue color
      pal[13] = RGB(0  , 0,   255);
      pal[14] = RGB(0  , 0,   128);
      pal[15] = RGB(255, 0,   255);
      pal[16] = RGB(128, 0,   128);

      GetPaletteEntries(hpal, 11, 1, (LPPALETTEENTRY)&pal[17]);
      pal[0]   = MAKELONG(0x300, 17);
      g_hpalVGA = CreatePalette((LPLOGPALETTE)pal);

      // get magic colors
      GetPaletteEntries(hpal, 8, 4, (LPPALETTEENTRY)&pal[17]);

      pal[0]   = MAKELONG(0x300, 20);
      g_hpal3D = CreatePalette((LPLOGPALETTE)pal);
   }

   // system colors
   for (i = 0; i < NT40_COLOR_MAX; i++)
   {
      g_Options.m_schemePreview.m_rgb[i] = GetSysColor(i);
      g_brushes[i] = NULL;
   }

   // sizes and fonts
   ncm.cbSize = sizeof(ncm);
   SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm),
                        (void far *)(LPNONCLIENTMETRICS)&ncm, FALSE);

   SetMyNonClientMetrics(&ncm);

   SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT),
            (void far *)(LPLOGFONT)&(g_fonts[FONT_ICONTITLE].lf), FALSE);

   SystemParametersInfo(SPI_SETGRADIENTCAPTIONS, 0, (PVOID)TRUE, 0);

   // default shell icon sizes
   g_sizes[ SIZE_ICON ].CurSize = GetSystemMetrics( SM_CXICON );
   g_sizes[ SIZE_SMICON ].CurSize = g_sizes[ SIZE_ICON ].CurSize / 2;

   if( RegOpenKey( HKEY_CURRENT_USER, c_szRegPathUserMetrics, &hkey )
      == ERROR_SUCCESS )
   {
      TCHAR val[ 8 ];
      LONG len = sizeof( val );

      if( RegQueryValueEx( hkey, c_szRegValIconSize, 0, NULL, (LPBYTE)&val,
         (LPDWORD)&len ) == ERROR_SUCCESS )
      {
         g_sizes[ SIZE_ICON ].CurSize = (int)MyStrToLong( val );
      }

      len = SIZEOF( val );
      if( RegQueryValueEx( hkey, c_szRegValSmallIconSize, 0, NULL, (LPBYTE)&val,
         (LPDWORD)&len ) == ERROR_SUCCESS )
      {
         g_sizes[ SIZE_SMICON ].CurSize = (int)MyStrToLong( val );
      }

      RegCloseKey( hkey );
   }

   g_sizes[ SIZE_DXICON ].CurSize =
      GetSystemMetrics( SM_CXICONSPACING ) - g_sizes[ SIZE_ICON ].CurSize;
   if( g_sizes[ SIZE_DXICON ].CurSize < 0 )
      g_sizes[ SIZE_DXICON ].CurSize = 0;

   g_sizes[ SIZE_DYICON ].CurSize =
      GetSystemMetrics( SM_CYICONSPACING ) - g_sizes[ SIZE_ICON ].CurSize;
   if( g_sizes[ SIZE_DYICON ].CurSize < 0 )
      g_sizes[ SIZE_DYICON ].CurSize = 0;

   // clean out the memory
   for (i = 0; i < NUM_FONTS; i++)
   {
      g_fonts[i].hfont = NULL;
   }

   // build all the brushes/fonts we need
   Look_RebuildSysStuff(TRUE);


   // From Look_InitDialog
   // initialize some globals
   cyBorder = GetSystemMetrics(SM_CYBORDER);
   cxBorder = GetSystemMetrics(SM_CXBORDER);
   cxEdge = GetSystemMetrics(SM_CXEDGE);
   cyEdge = GetSystemMetrics(SM_CYEDGE);

   return TRUE;
}


//////////////////////////////////////////////////////////////
// CLookPreviewGlobals member functions
//

// This is the static window proc function of CLookPrev
LRESULT CALLBACK CLookPrev::LookPreviewWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   PAINTSTRUCT ps;

   CLookPrev *pThis = (CLookPrev *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
   if(!pThis)
   {
      // Create the class to handle this object
      // Store the 'this' pointer in 
      pThis = new CLookPrev;
      pThis->m_hwnd = hWnd;
      SetWindowLongPtr (hWnd, GWLP_USERDATA, (INT_PTR)pThis);
   }

   switch(message)
   {
      case WM_NCCREATE:
         {
            DWORD dw;
            dw = GetWindowLong (hWnd,GWL_STYLE);
            SetWindowLong (hWnd, GWL_STYLE, dw | WS_BORDER);
            dw = GetWindowLong (hWnd,GWL_EXSTYLE);
            SetWindowLong (hWnd, GWL_EXSTYLE, dw | WS_EX_CLIENTEDGE);
         }
         return TRUE;

      case WM_CREATE:
         pThis->OnCreate();
         break;

      case WM_PALETTECHANGED:
         if ((HWND)wParam == hWnd)
            break;
         //fallthru
      case WM_QUERYNEWPALETTE:
         if (g_hpal3D)
            InvalidateRect(hWnd, NULL, FALSE);
         break;

      case WM_PAINT:
         BeginPaint(hWnd, &ps);
         pThis->OnPaint(ps.hdc);
         EndPaint(hWnd, &ps);
         return 0;

      case LPM_REPAINT:
         pThis->OnRepaint();
         return 0;

      case LPM_RECALC:
         pThis->OnRecalc();
         return 0;

   }
   return DefWindowProc(hWnd,message,wParam,lParam);
}

void CLookPrev::OnCreate()
{
   // Load menu for window
   m_hmenuSample = LoadMenu(g_hInstDll, MAKEINTRESOURCE(IDR_MENU));
   EnableMenuItem(m_hmenuSample, IDM_DISABLED, MF_GRAYED | MF_BYCOMMAND);
   HiliteMenuItem(m_hwnd, m_hmenuSample, IDM_SELECTED, MF_HILITE | MF_BYCOMMAND);

   // Create Bitmap for window
   RECT rc;
   HDC hdc;
   GetClientRect(m_hwnd, &rc);
   hdc = GetDC(NULL);
   m_hbmLook = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
   ReleaseDC(NULL, hdc);
   // Mirror the memory DC if the window is mirrored to keep the text readable.
   if (GetWindowLong(m_hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
       SetLayout(g_hdcMem, LAYOUT_RTL);
   }
}

void CLookPrev::OnDestroy()
{
   if (m_hbmLook)
      DeleteObject(m_hbmLook);
   if (m_hmenuSample)
      DestroyMenu(m_hmenuSample);

   // Un-allocate memory for this instance of the class
   delete this;
}


void CLookPrev::OnPaint(HDC hdc)
{
   if (m_hbmLook)
      ShowBitmap(hdc);
   else
      Draw(hdc);
}

void CLookPrev::ShowBitmap(HDC hdc)
{
   RECT rc;
   HBITMAP hbmOld;
   HPALETTE hpalOld = NULL;

   if (g_hpal3D)
   {
      hpalOld = SelectPalette(hdc, g_hpal3D, FALSE);
      RealizePalette(hdc);
   }

   GetClientRect(m_hwnd, &rc);
   hbmOld = (HBITMAP)SelectObject(g_hdcMem, m_hbmLook);
   BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, g_hdcMem, 0, 0, SRCCOPY);
   SelectObject(g_hdcMem, hbmOld);

   if (hpalOld)
   {
      SelectPalette(hdc, hpalOld, FALSE);
      RealizePalette(hdc);
   }
}



void CLookPrev::Draw(HDC hdc)
{
   RECT rcT;
   int nMode;
   DWORD rgbBk;
   int cxSize, cySize;
   HANDLE hOldColors;
   HPALETTE hpalOld = NULL;
   HICON hiconLogo;
// HFONT hfontOld;

   SaveDC(hdc);

   if (g_hpal3D)
   {
      hpalOld = SelectPalette(hdc, g_hpal3D, TRUE);
      RealizePalette(hdc);
   }

   hOldColors = SetSysColorsTemp(g_Options.m_schemePreview.m_rgb, g_brushes, COLOR_MAX_97_NT5/*COLOR_MAX_95_NT4*/);

   hiconLogo = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON,
                  g_sizes[SIZE_CAPTION].CurSize - 2*cxBorder,
                  g_sizes[SIZE_CAPTION].CurSize - 2*cyBorder, 0);

   //
   // Setup drawing stuff
   //
   nMode = SetBkMode(hdc, TRANSPARENT);
   rgbBk = GetTextColor(hdc);

   cxSize   = GetSystemMetrics(SM_CXSIZE);
   cySize   = GetSystemMetrics(SM_CYSIZE);

   //
   // Desktop
   //
   FillRect(hdc, &RCZ(ELEMENT_DESKTOP), g_brushes[COLOR_BACKGROUND]);

   //
   // Inactive window
   //

   // Border
   rcT = RCZ(ELEMENT_INACTIVEBORDER);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_INACTIVEBORDER], g_sizes[SIZE_FRAME].CurSize);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);

   // Caption
   rcT = RCZ(ELEMENT_INACTIVECAPTION);
   MyDrawBorderBelow(hdc, &rcT);

   // NOTE: because USER draws icon stuff using its own DC and subsequently
   // its own palette, we need to make sure to use the inactivecaption
   // brush before USER does so that it will be realized against our palette.
   // this might get fixed in USER by better be safe. 

   // "clip" the caption title under the buttons
   rcT.left = RCZ(ELEMENT_INACTIVESYSBUT2).left - cyEdge;
   FillRect(hdc, &rcT, g_brushes[COLOR_GRADIENTINACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = RCZ(ELEMENT_INACTIVECAPTION).left;
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, sm_Globals.m_szInactive, DC_ICON | DC_TEXT | DC_GRADIENT);

   DrawFrameControl(hdc, &RCZ(ELEMENT_INACTIVESYSBUT1), DFC_CAPTION, DFCS_CAPTIONCLOSE);
   rcT = RCZ(ELEMENT_INACTIVESYSBUT2);
   rcT.right -= (rcT.right - rcT.left)/2;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMIN);
   rcT.left = rcT.right;
   rcT.right = RCZ(ELEMENT_INACTIVESYSBUT2).right;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMAX);


#if 0
   //
   // small caption window
   // 

   {
   HICON hicon;
   int temp;


   rcT = RCZ(ELEMENT_SMCAPTION);
   hicon = (HICON)LoadImage(NULL, IDI_APPLICATION,
         IMAGE_ICON,
                  g_sizes[SIZE_SMCAPTION].CurSize - 2*cxBorder,
                  g_sizes[SIZE_SMCAPTION].CurSize - 2*cyBorder,
               0);

   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_TOP | BF_LEFT | BF_RIGHT | BF_ADJUST);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);
   // "clip" the caption title under the buttons
   temp = rcT.left;  // remember start of actual caption
   rcT.left = RCZ(ELEMENT_SMCAPSYSBUT).left - cxEdge;
   FillRect(hdc, &rcT, g_brushes[COLOR_ACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = temp;  // start of actual caption
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_SMCAPTION].hfont, hicon, sm_Globals.m_szSmallCaption, DC_SMALLCAP | DC_ICON | DC_TEXT);
   DestroyIcon(hicon);

   DrawFrameControl(hdc, &RCZ(ELEMENT_SMCAPSYSBUT), DFC_CAPTION, DFCS_CAPTIONCLOSE);
   }
#endif

   //
   // Active window
   //

   // Border
   rcT = RCZ(ELEMENT_ACTIVEBORDER);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_ACTIVEBORDER], g_sizes[SIZE_FRAME].CurSize);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);

   // Caption
   rcT = RCZ(ELEMENT_ACTIVECAPTION);
   MyDrawBorderBelow(hdc, &rcT);
   // "clip" the caption title under the buttons
   rcT.left = RCZ(ELEMENT_ACTIVESYSBUT2).left - cxEdge;
   FillRect(hdc, &rcT, g_brushes[COLOR_GRADIENTACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = RCZ(ELEMENT_ACTIVECAPTION).left;
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, sm_Globals.m_szActive, DC_ACTIVE | DC_ICON | DC_TEXT | DC_GRADIENT);

   DrawFrameControl(hdc, &RCZ(ELEMENT_ACTIVESYSBUT1), DFC_CAPTION, DFCS_CAPTIONCLOSE);
   rcT = RCZ(ELEMENT_ACTIVESYSBUT2);
   rcT.right -= (rcT.right - rcT.left)/2;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMIN);
   rcT.left = rcT.right;
   rcT.right = RCZ(ELEMENT_ACTIVESYSBUT2).right;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMAX);

   // Menu
   rcT = RCZ(ELEMENT_MENUNORMAL);
   
#if 0 // HACK TO SLIP USING DrawMenuBarTemp() which is not available on Memphis
   DrawMenuBarTemp(m_hwnd, hdc, &rcT, g_Options.m_hmenuSample, g_fonts[FONT_MENU].hfont);
#else
   {
      // JMC: HACK - HARD CODED TEXT
      HFONT hOldFont = (HFONT)SelectObject(hdc, g_fonts[FONT_MENU].hfont);
      COLORREF clrrefOldText = SetTextColor(hdc, g_Options.m_schemePreview.m_rgb[COLOR_MENUTEXT]);
      COLORREF clrrefOldBk = SetBkColor(hdc, g_Options.m_schemePreview.m_rgb[COLOR_MENU]);
      int nOldMode = SetBkMode(hdc, OPAQUE);
//    LPCTSTR lpszText = __TEXT("  File   Edit   Help");
      TCHAR szText[200];
      LoadString(g_hInstDll, IDS_PREVIEWMENUTEXT, szText, ARRAYSIZE(szText));
      ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcT, NULL, 0, NULL);
      DrawText(hdc, szText, lstrlen(szText), &rcT, DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS);
      SetTextColor(hdc, clrrefOldText);
      SetBkColor(hdc, clrrefOldBk);
      SetBkMode(hdc, nOldMode);
      SelectObject(hdc, hOldFont);
   }
#endif
   MyDrawBorderBelow(hdc, &rcT);

   //
   // Client area
   //

   rcT = RCZ(ELEMENT_WINDOW);
   DrawEdge(hdc, &rcT, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
   FillRect(hdc, &rcT, g_brushes[COLOR_WINDOW]);

   // window text
   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, g_Options.m_schemePreview.m_rgb[COLOR_WINDOWTEXT]);
   TextOut(hdc, RCZ(ELEMENT_WINDOW).left + 2*cxEdge, RCZ(ELEMENT_WINDOW).top + 2*cyEdge, sm_Globals.m_szWindowText, lstrlen(sm_Globals.m_szWindowText));

   //
   // scroll bar
   //
   rcT = RCZ(ELEMENT_SCROLLBAR);
   //MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DSHADOW], 1);
   //g_brushes[COLOR_SCROLLBAR]);
   //FillRect(hdc, &rcT, (HBRUSH)DefWindowProc(m_hwnd, WM_CTLCOLORSCROLLBAR, (WPARAM)hdc, (LPARAM)m_hwnd));
   FillRect(hdc, &rcT, g_brushes[COLOR_SCROLLBAR]);

   DrawFrameControl(hdc, &RCZ(ELEMENT_SCROLLUP), DFC_SCROLL, DFCS_SCROLLUP);
   DrawFrameControl(hdc, &RCZ(ELEMENT_SCROLLDOWN), DFC_SCROLL, DFCS_SCROLLDOWN);

#if 0 // Don't draw message box
   //
   // MessageBox
   //
   rcT = RCZ(ELEMENT_MSGBOX);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
   FillRect(hdc, &rcT, g_brushes[COLOR_3DFACE]);

   rcT = RCZ(ELEMENT_MSGBOXCAPTION);
   MyDrawBorderBelow(hdc, &rcT);
   // "clip" the caption title under the buttons
   rcT.left = RCZ(ELEMENT_MSGBOXSYSBUT).left - cxEdge;
   FillRect(hdc, &rcT, g_brushes[COLOR_GRADIENTACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = RCZ(ELEMENT_MSGBOXCAPTION).left;
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, sm_Globals.m_szMsgBox, DC_ACTIVE | DC_ICON | DC_TEXT | DC_GRADIENT);

   DrawFrameControl(hdc, &RCZ(ELEMENT_MSGBOXSYSBUT), DFC_CAPTION, DFCS_CAPTIONCLOSE);

   // message box text
   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, g_Options.m_schemePreview.m_rgb[COLOR_WINDOWTEXT]);
   hfontOld = (HFONT)SelectObject(hdc, g_fonts[FONT_MSGBOX].hfont);
   TextOut(hdc, RCZ(ELEMENT_MSGBOX).left + 3*cxEdge, RCZ(ELEMENT_MSGBOXCAPTION).bottom + cyEdge,
                  sm_Globals.m_szMsgBoxText, lstrlen(sm_Globals.m_szMsgBoxText));
   if (hfontOld)
      SelectObject(hdc, hfontOld);

   //
   // Button
   //
   rcT = RCZ(ELEMENT_BUTTON);
   DrawFrameControl(hdc, &rcT, DFC_BUTTON, DFCS_BUTTONPUSH);

// ?????? what font should this use ??????
   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, g_Options.m_schemePreview.m_rgb[COLOR_BTNTEXT]);
   DrawText(hdc, sm_Globals.m_szButton, -1, &rcT, DT_CENTER | DT_NOPREFIX |
      DT_SINGLELINE | DT_VCENTER);
#endif

   SetBkColor(hdc, rgbBk);
   SetBkMode(hdc, nMode);

   if (hiconLogo)
      DestroyIcon(hiconLogo);

   SetSysColorsTemp(NULL, NULL, (UINT_PTR)hOldColors);

   if (hpalOld)
   {
      hpalOld = SelectPalette(hdc, hpalOld, FALSE);
      RealizePalette(hdc);
   }

   RestoreDC(hdc, -1);
}

void CLookPrev::OnRepaint()
{
   HBITMAP hbmOld;

   if (m_hbmLook)
   {
      hbmOld = (HBITMAP)SelectObject(g_hdcMem, m_hbmLook);
      Draw(g_hdcMem);
      SelectObject(g_hdcMem, hbmOld);
   }
   InvalidateRect(m_hwnd, NULL, FALSE);
}


void CLookPrev::OnRecalc()
{
   DWORD cxNormal;
   int cxDisabled, cxSelected;
   int cxAvgCharx2;
   RECT rc;
   HFONT hfontT;
   int cxFrame, cyFrame;
   int cyCaption;
   int i;
   SIZE sizButton;

   GetClientRect(m_hwnd, &rc);

   //
   // Get our drawing data
   //
   cxSize = GetSystemMetrics(SM_CXSIZE);
   cxFrame = (g_sizes[SIZE_FRAME].CurSize + 1) * cxBorder + cxEdge;
   cyFrame = (g_sizes[SIZE_FRAME].CurSize + 1) * cyBorder + cyEdge;
   cyCaption = g_sizes[SIZE_CAPTION].CurSize;

   //
   // Get text dimensions, with proper font.
   //

   hfontT = (HFONT)SelectObject(g_hdcMem, g_fonts[FONT_MENU].hfont);

   GetTextExtentPoint32(g_hdcMem, sm_Globals.m_szNormal, lstrlen(sm_Globals.m_szNormal), &sizButton);
   cxNormal = sizButton.cx;

   GetTextExtentPoint32(g_hdcMem, sm_Globals.m_szDisabled, lstrlen(sm_Globals.m_szDisabled), &sizButton);
   cxDisabled = sizButton.cx;

   GetTextExtentPoint32(g_hdcMem, sm_Globals.m_szSelected, lstrlen(sm_Globals.m_szSelected), &sizButton);
   cxSelected = sizButton.cx;

   // get the average width (USER style) of menu font
   GetTextExtentPoint32(g_hdcMem, g_szABC, 52, &sizButton);
   cxAvgCharx2 = 2 * (sizButton.cx / 52);

   // actual menu-handling widths of strings is bigger
   cxDisabled += cxAvgCharx2;
   cxSelected += cxAvgCharx2;
   cxNormal += cxAvgCharx2;

   SelectObject(g_hdcMem, hfontT);

   GetTextExtentPoint32(g_hdcMem, sm_Globals.m_szButton, lstrlen(sm_Globals.m_szButton), &sizButton);

   //
   // Desktop
   //
   RCZ(ELEMENT_DESKTOP) = rc;

   InflateRect(&rc, -8*cxBorder, -8*cyBorder);

   //
   // Windows
   //
   rc.bottom -= cyFrame + cyCaption;
   RCZ(ELEMENT_ACTIVEBORDER) = rc;
   OffsetRect(&RCZ(ELEMENT_ACTIVEBORDER), cxFrame,
                  cyFrame + cyCaption + cyBorder);
   RCZ(ELEMENT_ACTIVEBORDER).bottom -= cyCaption;

   //
   // Inactive window
   //

   rc.right -= cyCaption;
   RCZ(ELEMENT_INACTIVEBORDER) = rc;

   // Caption
   InflateRect(&rc, -cxFrame, -cyFrame);
   rc.bottom = rc.top + cyCaption + cyBorder;
   RCZ(ELEMENT_INACTIVECAPTION) = rc;

   // close button
   InflateRect(&rc, -cxEdge, -cyEdge);
   rc.bottom -= cyBorder;     // compensate for magic line under caption
   RCZ(ELEMENT_INACTIVESYSBUT1) = rc;
   RCZ(ELEMENT_INACTIVESYSBUT1).left = rc.right - (cyCaption - cxEdge);

   // min/max buttons
   RCZ(ELEMENT_INACTIVESYSBUT2) = rc;
   RCZ(ELEMENT_INACTIVESYSBUT2).right = RCZ(ELEMENT_INACTIVESYSBUT1).left - cxEdge;
   RCZ(ELEMENT_INACTIVESYSBUT2).left = RCZ(ELEMENT_INACTIVESYSBUT2).right - 
                                    2 * (cyCaption - cxEdge);

#if 0
   //
   // small caption window
   //
   RCZ(ELEMENT_SMCAPTION) = RCZ(ELEMENT_ACTIVEBORDER);
   RCZ(ELEMENT_SMCAPTION).bottom = RCZ(ELEMENT_SMCAPTION).top;
   RCZ(ELEMENT_SMCAPTION).top -= g_sizes[SIZE_SMCAPTION].CurSize + cyEdge + 2 * cyBorder;
   RCZ(ELEMENT_SMCAPTION).right -= cxFrame;
   RCZ(ELEMENT_SMCAPTION).left = RCZ(ELEMENT_INACTIVECAPTION).right + 2 * cxFrame;

   RCZ(ELEMENT_SMCAPSYSBUT) = RCZ(ELEMENT_SMCAPTION);
   // deflate inside frame/border to caption and then another edge's worth
   RCZ(ELEMENT_SMCAPSYSBUT).right -= 2 * cxEdge + cxBorder;
   RCZ(ELEMENT_SMCAPSYSBUT).top += 2 * cxEdge + cxBorder;
   RCZ(ELEMENT_SMCAPSYSBUT).bottom -= cxEdge + cxBorder;
   RCZ(ELEMENT_SMCAPSYSBUT).left = RCZ(ELEMENT_SMCAPSYSBUT).right - 
                              (g_sizes[SIZE_SMCAPTION].CurSize - cxEdge);
#endif

   //
   // Active window
   //

   // Caption
   rc = RCZ(ELEMENT_ACTIVEBORDER);
   InflateRect(&rc, -cxFrame, -cyFrame);
   RCZ(ELEMENT_ACTIVECAPTION) = rc;
   RCZ(ELEMENT_ACTIVECAPTION).bottom = 
      RCZ(ELEMENT_ACTIVECAPTION).top + cyCaption + cyBorder;

   // close button
   RCZ(ELEMENT_ACTIVESYSBUT1) = RCZ(ELEMENT_ACTIVECAPTION);
   InflateRect(&RCZ(ELEMENT_ACTIVESYSBUT1), -cxEdge, -cyEdge);
   RCZ(ELEMENT_ACTIVESYSBUT1).bottom -= cyBorder;     // compensate for magic line under caption
   RCZ(ELEMENT_ACTIVESYSBUT1).left = RCZ(ELEMENT_ACTIVESYSBUT1).right - 
                              (cyCaption - cxEdge);

   // min/max buttons
   RCZ(ELEMENT_ACTIVESYSBUT2) = RCZ(ELEMENT_ACTIVESYSBUT1);
   RCZ(ELEMENT_ACTIVESYSBUT2).right = RCZ(ELEMENT_ACTIVESYSBUT1).left - cxEdge;
   RCZ(ELEMENT_ACTIVESYSBUT2).left = RCZ(ELEMENT_ACTIVESYSBUT2).right - 
                                    2 * (cyCaption - cxEdge);

   // Menu
   rc.top = RCZ(ELEMENT_ACTIVECAPTION).bottom;
   RCZ(ELEMENT_MENUNORMAL) = rc;
   rc.top = RCZ(ELEMENT_MENUNORMAL).bottom = RCZ(ELEMENT_MENUNORMAL).top + g_sizes[SIZE_MENU].CurSize;
   RCZ(ELEMENT_MENUDISABLED) = RCZ(ELEMENT_MENUSELECTED) = RCZ(ELEMENT_MENUNORMAL);

   RCZ(ELEMENT_MENUDISABLED).left = RCZ(ELEMENT_MENUNORMAL).left + cxNormal;
   RCZ(ELEMENT_MENUDISABLED).right = RCZ(ELEMENT_MENUSELECTED).left = 
                  RCZ(ELEMENT_MENUDISABLED).left + cxDisabled;
   RCZ(ELEMENT_MENUSELECTED).right = RCZ(ELEMENT_MENUSELECTED).left + cxSelected;
   
   //
   // Client
   //
   RCZ(ELEMENT_WINDOW) = rc;

   //
   // Scrollbar
   //
   InflateRect(&rc, -cxEdge, -cyEdge); // take off client edge
   RCZ(ELEMENT_SCROLLBAR) = rc;
   rc.right = RCZ(ELEMENT_SCROLLBAR).left = rc.right - g_sizes[SIZE_SCROLL].CurSize;
   RCZ(ELEMENT_SCROLLUP) = RCZ(ELEMENT_SCROLLBAR);
   RCZ(ELEMENT_SCROLLUP).bottom = RCZ(ELEMENT_SCROLLBAR).top + g_sizes[SIZE_SCROLL].CurSize; 

   RCZ(ELEMENT_SCROLLDOWN) = RCZ(ELEMENT_SCROLLBAR);
   RCZ(ELEMENT_SCROLLDOWN).top = RCZ(ELEMENT_SCROLLBAR).bottom - g_sizes[SIZE_SCROLL].CurSize; 

   //
   // Message Box
   //
   rc.top = RCZ(ELEMENT_WINDOW).top + (RCZ(ELEMENT_WINDOW).bottom - RCZ(ELEMENT_WINDOW).top) / 2;
   rc.bottom = RCZ(ELEMENT_DESKTOP).bottom - 2*cyEdge;
   rc.left = RCZ(ELEMENT_WINDOW).left + 2*cyEdge;
   rc.right = RCZ(ELEMENT_WINDOW).left + (RCZ(ELEMENT_WINDOW).right - RCZ(ELEMENT_WINDOW).left) / 2 + 3*cyCaption;
   RCZ(ELEMENT_MSGBOX) = rc;

   // Caption
   RCZ(ELEMENT_MSGBOXCAPTION) = rc;
   RCZ(ELEMENT_MSGBOXCAPTION).top += cyEdge + cyBorder;
   RCZ(ELEMENT_MSGBOXCAPTION).bottom = RCZ(ELEMENT_MSGBOXCAPTION).top + cyCaption + cyBorder;
   RCZ(ELEMENT_MSGBOXCAPTION).left += cxEdge + cxBorder;
   RCZ(ELEMENT_MSGBOXCAPTION).right -= cxEdge + cxBorder;

   RCZ(ELEMENT_MSGBOXSYSBUT) = RCZ(ELEMENT_MSGBOXCAPTION);
   InflateRect(&RCZ(ELEMENT_MSGBOXSYSBUT), -cxEdge, -cyEdge);
   RCZ(ELEMENT_MSGBOXSYSBUT).left = RCZ(ELEMENT_MSGBOXSYSBUT).right - 
                              (cyCaption - cxEdge);
   RCZ(ELEMENT_MSGBOXSYSBUT).bottom -= cyBorder;      // line under caption

   // Button
   RCZ(ELEMENT_BUTTON).bottom = RCZ(ELEMENT_MSGBOX).bottom - (4*cyBorder + cyEdge);
   RCZ(ELEMENT_BUTTON).top = RCZ(ELEMENT_BUTTON).bottom - (sizButton.cy + 8 * cyBorder);

   i = (RCZ(ELEMENT_BUTTON).bottom - RCZ(ELEMENT_BUTTON).top) * 3;
   RCZ(ELEMENT_BUTTON).left = (rc.left + (rc.right - rc.left)/2) - i/2;
   RCZ(ELEMENT_BUTTON).right = RCZ(ELEMENT_BUTTON).left + i;
}












/////////////////////////////////////////////////////////
// Support functions





// ----------------------------------------------------------------------------
//
//  MyDrawFrame() -
//
//  Draws bordered frame, border size cl, and adjusts passed in rect.
//
// ----------------------------------------------------------------------------
void MyDrawFrame(HDC hdc, LPRECT prc, HBRUSH hbrColor, int cl)
{
   HBRUSH hbr;
   int cx, cy;
   RECT rcT;

   rcT = *prc;
   cx = cl * cxBorder;
   cy = cl * cyBorder;

   hbr = (HBRUSH)SelectObject(hdc, hbrColor);

   PatBlt(hdc, rcT.left, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);
   rcT.left += cx;

   PatBlt(hdc, rcT.left, rcT.top, rcT.right - rcT.left, cy, PATCOPY);
   rcT.top += cy;

   rcT.right -= cx;
   PatBlt(hdc, rcT.right, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);

   rcT.bottom -= cy;
   PatBlt(hdc, rcT.left, rcT.bottom, rcT.right - rcT.left, cy, PATCOPY);

   hbr = (HBRUSH)SelectObject(hdc, hbr);

   *prc = rcT;
}

/*
** draw a cyBorder band of 3DFACE at the bottom of the given rectangle.
** also, adjust the rectangle accordingly.
*/
void MyDrawBorderBelow(HDC hdc, LPRECT prc)
{
   int i;

   i = prc->top;
   prc->top = prc->bottom - cyBorder;
   FillRect(hdc, prc, g_brushes[COLOR_3DFACE]);
   prc->top = i;
   prc->bottom -= cyBorder;
}

/*-------------------------------------------------------------------
** draw a full window caption with system menu, minimize button,
** maximize button, and text.
**-------------------------------------------------------------------*/
void DrawFullCaption(HDC hdc, LPRECT prc, LPTSTR lpszTitle, UINT flags)
{
   int iRight;
   int iFont;

   SaveDC(hdc);

   // special case gross for small caption that already drew on bottom
   if (!(flags & DC_SMALLCAP))
      MyDrawBorderBelow(hdc, prc);

   iRight = prc->right;
   prc->right = prc->left + cxSize;
   DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONCLOSE);

   prc->left = prc->right;
   prc->right = iRight - 2*cxSize;
   iFont = flags & DC_SMALLCAP ? FONT_SMCAPTION : FONT_CAPTION;
   DrawCaptionTemp(NULL, hdc, prc, g_fonts[iFont].hfont, NULL, lpszTitle, flags | DC_ICON | DC_TEXT);

   prc->left = prc->right;
   prc->right = prc->left + cxSize;
   DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONMIN);
   prc->left = prc->right;
   prc->right = prc->left + cxSize;
   DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONMAX);

   RestoreDC(hdc, -1);
}


