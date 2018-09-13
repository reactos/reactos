/*      File: \wacker\tdll\banner.h (created 16-Mar-94)
 *
 *      Copyright 1996 by Hilgraeve, Inc -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 2 $
 *      $Date: 2/05/99 3:20p $
 */
#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <term\res.h>

#include "globals.h"
#include "tdll.h"
#include "stdtyp.h"
#include "assert.h"
#include "file_msc.h"
#include "errorbox.h"
#include "banner.h"
#include "misc.h"
#include "upgrddlg.h"

LRESULT CALLBACK BannerProc(HWND, UINT, WPARAM, LPARAM);
STATIC_FUNC void banner_WM_PAINT(HWND hwnd);
STATIC_FUNC void banner_WM_CREATE(HWND hwnd, LPCREATESTRUCT lpstCreate);

// Also used in aboutdlg.c
//
const TCHAR *achVersion = {"Version 890270 "};  // trailing space
                                     // necessary - mrw
#define IDC_PB_UPGRADEINFO      101

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:    bannerRegisterClass
 *
 * DESCRIPTION:
 *      This function registers the window class for the banner window.
 *
 * ARGUEMENTS:
 *      The task instance handle.
 *
 * RETURNS:
 * The usual TRUE/FALSE from a registration function.
 *
 */
BOOL bannerRegisterClass(HANDLE hInstance)
   {
   ATOM bRet = FALSE;
   WNDCLASS wnd;

   wnd.style               = CS_HREDRAW | CS_VREDRAW;
   wnd.lpfnWndProc         = BannerProc;
   wnd.cbClsExtra          = 0;
   wnd.cbWndExtra          = sizeof(HANDLE);
   wnd.hInstance           = hInstance;
   wnd.hIcon               = extLoadIcon(MAKEINTRESOURCE(IDI_PROG));
   wnd.hCursor             = LoadCursor(NULL, IDC_ARROW);
   wnd.hbrBackground       = (HBRUSH)(COLOR_WINDOW+1);
   wnd.lpszMenuName        = NULL;
   wnd.lpszClassName       = BANNER_DISPLAY_CLASS;

   bRet = RegisterClass(&wnd);

   return bRet;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:    bannerCreateBanner
 *
 * DESCRIPTION:
 *      This function is called to creat the banner window.  The banner window is
 *      a short lived window that the program can run without.
 *
 * ARGUEMENTS:
 *      The task instance handle.
 *
 * RETURNS:
 *      The handle of the banner window.
 *
 */
HWND bannerCreateBanner(HANDLE hInstance, LPTSTR pszTitle)
   {
   HWND hwndBanner = NULL;
   hwndBanner = CreateWindow(BANNER_DISPLAY_CLASS,
                     pszTitle,
                     BANNER_WINDOW_STYLE,
                     0,
                     0,
                     100,
                     100,
                     NULL,
                     NULL,
                     hInstance,
                     NULL);
   return hwndBanner;
   }

#define BANNER_FILE     1

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:    BannerProc
 *
 * DESCRIPTION:
 *      This is the window procedure for the initial banner window.
 *
 * ARGUEMENTS:
 *      The usual stuff that a window proc gets.
 *
 * RETURNS:
 *      All sorts of different stuff.
 *
 */
LRESULT CALLBACK BannerProc(HWND hwnd, UINT wMsg, WPARAM wPar, LPARAM lPar)
   {
   HBITMAP        hBitmap = (HBITMAP)0;
   HWND           hwndParent;
#ifdef USE_PRIVATE_EDITION_3_BANNER
    HWND           hwndButton = 0;
#endif
   LPCREATESTRUCT lpstCreate = (LPCREATESTRUCT)lPar;

   hwndParent = 0;

   switch (wMsg)
      {
   case WM_CREATE:
      banner_WM_CREATE(hwnd, lpstCreate);
      break;

   case WM_PAINT:
      banner_WM_PAINT(hwnd);
      break;

#ifdef USE_PRIVATE_EDITION_3_BANNER
    case WM_SETFOCUS:
   // When we are displaying the "Upgrade" button, it is the only
   // control in the banner. So we always want to have the focus
   // on it. - cab:12/02/96
   //
   hwndButton = GetDlgItem(hwnd, IDC_PB_UPGRADEINFO);
   assert(hwndButton);
   SetFocus(hwndButton);
   break;

    case WM_COMMAND:
   switch(wPar)
       {
   case IDC_PB_UPGRADEINFO:
       DoUpgradeDialog(hwnd);
       break;

   default:
       break;
       }
   break;
#endif

   case WM_CHAR:
   case WM_KEYDOWN:
   case WM_KILLFOCUS:
   case WM_LBUTTONDOWN:
      hwndParent = (HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
      if (hwndParent)
         SendMessage(hwnd, WM_CLOSE, 0, 0);
      break;

   case WM_DESTROY:
      hBitmap = (HBITMAP)GetWindowLongPtr(hwnd, 0);
      hwndParent = (HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA);

      if (hBitmap != (HBITMAP)0)
         DeleteObject(hBitmap);

      if (hwndParent)
         SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);

      break;

   default:
      return DefWindowProc(hwnd, wMsg, wPar, lPar);
      }

   return 0L;
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: utilDrawBitmap
 *
 * DESCRIPTION:
 *      This function draws a bitmap in a window.
 *
 * ARGUMENTS:
 *      hWnd    -- handle of the window to draw in
 *      hBitmap -- bitmap to be drawn
 *      xStart  -- starting coordinate
 *      yStart  -- starting coordinate
 *
 * RETURNS:
 *
 */
VOID FAR PASCAL utilDrawBitmap(HWND hWnd, HDC hDC, HBITMAP hBitmap,
                        SHORT xStart, SHORT yStart)
  {
  BITMAP        bm;
  HDC           hdcMem;
  POINT         ptSize, ptOrg;

  if (hWnd && !hDC)
     hDC = GetDC(hWnd);

  hdcMem = CreateCompatibleDC(hDC);
  SelectObject(hdcMem, hBitmap);
  SetMapMode(hdcMem, GetMapMode(hDC));

  GetObject(hBitmap, sizeof(BITMAP), (LPTSTR)&bm);

  // Convert device coordintes into logical coordinates.
  //
  ptSize.x = bm.bmWidth;
  ptSize.y = bm.bmHeight;
  DPtoLP(hDC, &ptSize, 1);

  ptOrg.x = 0;
  ptOrg.y = 0;
  DPtoLP(hdcMem, &ptOrg, 1);

  BitBlt(hDC, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y,
   SRCCOPY);

  DeleteDC(hdcMem);

  if (hWnd && !hDC)
     ReleaseDC(hWnd, hDC);

  return;
  }

// TODO:cab,11/29/96 put this where it belongs
//
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  fontSetWindowFont
 *
 * DESCRIPTION:
 *  Changes the font for the given window to the one specified.
 *
 * ARGUMENTS:
 *  hwnd        - Handle of the window.
 *  pszFontName - Name of the new font.
 *  iPointSize  - The new font's point size.
 *
 * RETURNS:
 *  0 if successful, -1 if error
 *
 * AUTHOR:  C. Baumgartner, 11/29/96
 */
int fontSetWindowFont(HWND hwnd, LPCTSTR pszFontName, int iPointSize)
    {
    int     iPixPerLogicalInch = 0;
    HDC     hDC = 0;
    HFONT   hFont = 0;
    LOGFONT lf; memset(&lf, 0, sizeof(LOGFONT));

    assert(hwnd);
    assert(pszFontName);

    // Get the pixels per logical inch in the y direction.
    //
    hDC = GetDC(hwnd);
    iPixPerLogicalInch = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(hwnd, hDC);

    // Compute the height of the font in logical units.
    // This is simply: (iPointSize * iPixPerLogicalInch) / 72,
    // don't ask me to derive that equation, I just got it from
    // Charles Petzold's book.
    //
    lf.lfHeight = -MulDiv(iPointSize, iPixPerLogicalInch, 72);

    // Set the font name.
    //
   //mpt:1-28-98 changed from strcpy so that it will handle dbcs font names
    lstrcpy(lf.lfFaceName, pszFontName);

    // Create the desired font.
    //
    hFont = CreateFontIndirect(&lf);
    if ( !hFont )
      {
      assert(hFont);
      return -1;
      }

    // Tell the window what it's new font is.
    //
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));

    return 0;
    }


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC void banner_WM_CREATE(HWND hwnd, LPCREATESTRUCT lpstCreate)
   {
   RECT    rc;
   HBITMAP hBitmap = (HBITMAP)0;
   BITMAP  bm;
   INT     x, y, cx, cy;
#ifdef USE_PRIVATE_EDITION_3_BANNER
   TCHAR   achUpgradeInfo[100];
#ifndef NT_EDITION
   TCHAR  ach[80];
    INT     nSize1;
#endif
#endif
   LONG_PTR ExStyle;

   ExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
   //
   // [mhamid]: if it is mirrored then turn off mirroing.
   //
   if (ExStyle & WS_EX_LAYOUTRTL) {
       SetWindowLongPtr(hwnd, GWL_EXSTYLE, ExStyle & ~WS_EX_LAYOUTRTL);
   }

   if (lpstCreate->hwndParent)
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpstCreate->hwndParent);

   //mpt:03-12-98 Changed the bitmap and avi to use system colors
   //hBitmap = LoadBitmap(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_BM_BANNER));
   hBitmap = (HBITMAP)LoadImage(glblQueryDllHinst(),
            MAKEINTRESOURCE(IDD_BM_BANNER),
            IMAGE_BITMAP,
            0,
            0,
            LR_CREATEDIBSECTION | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
   SetWindowLongPtr(hwnd, 0, (LONG_PTR)hBitmap);

   GetObject(hBitmap, sizeof(BITMAP), (LPTSTR)&bm);

   SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
   AdjustWindowRect(&rc, BANNER_WINDOW_STYLE, FALSE);

   cx = rc.right - rc.left;
   cy = rc.bottom - rc.top;

   x = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;
   y = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;

   MoveWindow(hwnd, x, y, cx, cy, TRUE);

   if (lpstCreate->hwndParent)
      mscCenterWindowOnWindow(hwnd, lpstCreate->hwndParent);

    // Create an "Upgrade Information" button. - cab:11/29/96
    //
#ifdef USE_PRIVATE_EDITION_3_BANNER
    {
    HWND hwndButton = 0;

    // Create the button, but don't put any text in it yet. We'll
    // do that after we change the font.
    //
    hwndButton = CreateWindow("button",
               "",
               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
               IDN_UPGRADE_BUTTON_X,
               IDN_UPGRADE_BUTTON_Y,
               IDN_UPGRADE_BUTTON_W,
               IDN_UPGRADE_BUTTON_H,
               hwnd,
               (HMENU)IDC_PB_UPGRADEINFO,
               glblQueryDllHinst(),
               NULL);

   assert(hwndButton);

#ifndef NT_EDITION
    // Set the text font in the button to 8 point MS Sans Serif.
    // mpt:1-21-98 moved font and size to resources

    if (LoadString(glblQueryDllHinst(), IDS_UPGRADE_FONT_SIZE,
       ach, sizeof(ach)/sizeof(TCHAR)))
      {
      nSize1 = atoi(ach);
      }
   else
      {
      nSize1 = -8;
      }

    LoadString(glblQueryDllHinst(), IDS_UPGRADE_FONT, ach, sizeof(ach)/sizeof(TCHAR));

    if ( fontSetWindowFont(hwndButton, ach, nSize1) != 0 )
      {
      assert(0);
      }
#endif

    // Set the button text.
   LoadString(glblQueryDllHinst(), IDS_UPGRADE_INFO,
      achUpgradeInfo, sizeof(achUpgradeInfo)/sizeof(TCHAR));

    SetWindowText(hwndButton, achUpgradeInfo);
    }
#endif

#if defined(INCL_SPINNING_GLOBE)
    // Create an animation control and play spinning globe.
    //
   {
   HWND    hwndAnimate;
   //mpt:03-12-98 Changed the bitmap and avi to use system colors
   hwndAnimate = Animate_Create(hwnd, 100,
       WS_VISIBLE | WS_CHILD | ACS_TRANSPARENT,
       glblQueryDllHinst());

   MoveWindow(hwndAnimate, 177, 37, 118, 101, TRUE);
   Animate_Open(hwndAnimate, MAKEINTRESOURCE(IDR_GLOBE_AVI));
   Animate_Play(hwndAnimate, 0, -1, 1);
   }
#endif
   }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC void banner_WM_PAINT(HWND hwnd)
   {
   HDC         hDC;
   HBITMAP     hBitmap;
   PAINTSTRUCT ps;
#if !defined(USE_PRIVATE_EDITION_3_BANNER)
   LOGFONT     lf;
   HFONT       hFont;
#endif

   hDC = BeginPaint(hwnd, &ps);
   hBitmap = (HBITMAP)GetWindowLongPtr(hwnd, 0);

#ifdef USE_PRIVATE_EDITION_3_BANNER
    // Don't draw over the button. - cab:11/29/96
    //
    ExcludeClipRect(hDC, IDN_UPGRADE_BUTTON_X, IDN_UPGRADE_BUTTON_Y, IDN_UPGRADE_BUTTON_X + IDN_UPGRADE_BUTTON_W,
   IDN_UPGRADE_BUTTON_Y + IDN_UPGRADE_BUTTON_H);
#endif

   if (hBitmap)
      utilDrawBitmap((HWND)0, hDC, hBitmap, 0, 0);

    // In the HTPE 3 banner, the version # and lot # are now in the
    // lower left corner of the bitmap. - cab:11/29/96
    //
#if !defined(USE_PRIVATE_EDITION_3_BANNER)
   // Here's a mean trick.  The HwndFrame guy doesn't get set until
   // long after the banner goes up.  Since we don't want the version
   // number on the opening banner but do want it in the about portion
   // this works. - mrw:3/17/95
   //
   if (glblQueryHwndFrame())
      {
      // Draw in the version number
      //
      memset(&lf, 0, sizeof(LOGFONT));

      lf.lfHeight = 14;
      lf.lfCharSet = ANSI_CHARSET;
      strcpy(lf.lfFaceName, "Arial");

      hFont = CreateFontIndirect(&lf);

      if (hFont)
         {
         hFont = SelectObject(hDC, hFont);
         SetBkColor(hDC, RGB(192,192,192));
         TextOut(hDC, 15, 12, achVersion, strlen(achVersion));
         DeleteObject(SelectObject(hDC, hFont));
         }
      }
#endif

   EndPaint(hwnd, &ps);
   }
