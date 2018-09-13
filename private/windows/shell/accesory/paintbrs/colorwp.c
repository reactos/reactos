/****************************Module*Header******************************\
* Module Name: colorwp.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation			*
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

extern int imagePixels, imagePlanes;
extern RECT pbrushRct[];
extern HWND pbrushWnd[];
extern int theForeg, theBackg;
extern DWORD *rgbColor;
extern HWND colorWnd;
extern BOOL inMagnify;
extern HPALETTE hPalette;

/* draws rectangle on hDC in color if editing color image, or monochrome
** on a b/w image.
*/
void DrawMonoRect(HDC hDC, int left, int top, int right, int bottom)
{
   HDC hMonoDC;
   HBITMAP hMonoBM;
   HBRUSH hBrush;
   HPEN hPen;

   int width = right - left;
   int height = bottom - top;

   /* if we are editing a color image, just output the rectangle */
   if (imagePixels != 1 || imagePlanes != 1) {
       Rectangle(hDC, left, top, right, bottom);
       return;
   }

   /* Allocate a monochrome bitmap to draw on */
   if (!(hMonoBM = CreateBitmap(width, height, 1, 1, NULL)))
       goto cleanup1;
   if (!(hMonoDC = CreateCompatibleDC(hDC)))
       goto cleanup2;
   SelectObject(hMonoDC, hMonoBM);

   SaveDC(hDC);

   /* Set foreground/background to black/white if we are in b/w */
   SetBkColor(hDC, RGB(255,255,255));
   SetTextColor(hDC, RGB(0,0,0));

   /* Get the brush and pen selected into hDC */
   hBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
   hPen = SelectObject(hDC, GetStockObject(NULL_PEN));

   /* Select the brush and pen into monoDC */
   if (hBrush)
       SelectObject(hMonoDC, hBrush);
   if (hPen)
       SelectObject(hMonoDC, hPen);

   /* draw the rectangle in the mono bitmap */
   Rectangle(hMonoDC, 0, 0, width, height);

   /* copy mono bitmap to screen */
   BitBlt(hDC, left, top, width, height,
          hMonoDC, 0, 0,
          SRCCOPY);

   RestoreDC(hDC, -1);
   DeleteDC(hMonoDC);

cleanup2:
   DeleteObject(hMonoBM);

cleanup1:
   return;
}

LONG FAR PASCAL ColorWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int i,wid,hgt,newColor;
   PAINTSTRUCT ps;
   POINT pt;
   HBRUSH brush,oldBrush;
   HDC dc;
   RECT r;
   HCURSOR oldcsr;
   HPEN hOldPen;

   /* calculate dimensions of color rectangles */
   wid = (pbrushRct[COLORid].right - pbrushRct[COLORid].left) / COLORdiv;
   hgt = (pbrushRct[COLORid].bottom - pbrushRct[COLORid].top) >> 1;
   pt.x = (short)LOWORD(lParam);
   pt.y = (short)HIWORD(lParam);

   switch (message) {
       case WM_ERASEBKGND:
           if (fInvisible)
               return TRUE;
           else 
               return DefWindowProc(hWnd, message, wParam, lParam);

       case WM_MOUSEMOVE:
           if(SetCursorOn())
               SendMessage(pbrushWnd[PAINTid], WM_HIDECURSOR, 0, 0L);
           break;

       /* paint the color window */
       case WM_PAINT:
	   if (fInvisible)
           {
               BeginPaint(hWnd, &ps);
               EndPaint(hWnd, &ps);
               break;
           }
               
	   oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
	   BeginPaint(hWnd, &ps);

           if (hPalette) {
               SelectPalette(ps.hdc, hPalette, 0);
               RealizePalette(ps.hdc);
           }

           /* paint current background box */
           hOldPen = SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
           r.left = r.top = 0;
           r.right = wid << 1;
           r.bottom = hgt << 1;
           brush = CreateSolidBrush(rgbColor[theBackg]);
           if (brush) {
               oldBrush = SelectObject(ps.hdc, brush);
               DrawMonoRect(ps.hdc, r.left, r.top, r.right, r.bottom);
               if (oldBrush)
                   SelectObject(ps.hdc, oldBrush);
               DeleteObject(brush);
           }

           /* paint current foreground box */
           r.left = wid >> 1;
           r.right = r.left + wid;
           r.top = hgt >> 1;
           r.bottom = r.top + hgt;
           brush = CreateSolidBrush(rgbColor[theForeg]);
           if (brush) {
               oldBrush = SelectObject(ps.hdc, brush);
               DrawMonoRect(ps.hdc, r.left, r.top, r.right, r.bottom);
               if (oldBrush)
                   SelectObject(ps.hdc, oldBrush);
               DeleteObject(brush);
           }

           /* paint color boxes */
           r.left = wid << 1;
           r.right = r.left + wid;
           oldBrush = SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));
           Rectangle(ps.hdc, r.left, 0, r.left + 14 * wid, 2 * hgt - 1);
           if (oldBrush)
               SelectObject(ps.hdc, oldBrush);
           SelectObject(ps.hdc, GetStockObject(BLACK_PEN));

           for (i = 0; i < MAXcolors; ++i) {
               r.top = ((i & 1) ? (hgt-1) : 0);
               r.top = max(0, r.top);
               r.bottom = r.top + hgt;
               brush = CreateSolidBrush(rgbColor[i]);
               if (brush) {
                   oldBrush = SelectObject(ps.hdc, brush);
                   DrawMonoRect(ps.hdc, r.left, r.top, r.right, r.bottom);
                   if (oldBrush)
                       SelectObject(ps.hdc, oldBrush);
                   DeleteObject(brush);
               }

               if (i & 1) {
                   r.left += wid;
                   r.right += wid;
               }
           }

           if (hOldPen)
               SelectObject(ps.hdc, hOldPen);
           EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
           SetCursor(oldcsr);
           break;

       /* select foreground color */
       case WM_LBUTTONDOWN:
           if ((pt.x -= wid << 1) >= 0) {
               newColor = (min(pt.x / wid, 13) << 1) + min(pt.y / hgt, 1);
               if (newColor != theForeg) {
                   if (dc = GetDisplayDC(hWnd)) {
                       theForeg = newColor;
                       r.left = wid >> 1;
                       r.right = r.left + wid;
                       r.top = hgt >> 1;
                       r.bottom = r.top + hgt;
                       brush = CreateSolidBrush(rgbColor[theForeg]);

                       SelectObject(dc, GetStockObject(WHITE_PEN));
                       if (brush) {
                           oldBrush = SelectObject(dc, brush);
                           DrawMonoRect(dc, r.left, r.top, r.right, r.bottom);
                           if (oldBrush)
                               SelectObject(dc, oldBrush);
                           DeleteObject(brush);
                       }

                       ReleaseDC(hWnd, dc);
                   }
                   SendMessage(pbrushWnd[PAINTid], WM_CHANGEFONT, 0, 0L);
                   if (colorWnd) {
                       SendMessage(colorWnd, WM_COMMAND, IDSETBARS,
                                   rgbColor[theForeg]);
                       SendMessage(colorWnd, WM_COMMAND, IDSETCOLOR, 0L);
                       SendMessage(colorWnd, WM_COMMAND, IDSETEDIT, 0L);
                   }
               }
           }
           break;

       /* select background color */
       case WM_RBUTTONDOWN:
           if ((pt.x -= wid << 1) >= 0) {
               newColor = (min(pt.x / wid, 13) << 1) + min(pt.y / hgt, 1);
               if (newColor != theBackg) {
                   if (dc = GetDisplayDC(hWnd)) {
                       theBackg = newColor;
                       r.left = r.top = 0;
                       r.right = wid << 1;
                       r.bottom = hgt << 1;
                       brush = CreateSolidBrush(rgbColor[theBackg]);

                       SelectObject(dc, GetStockObject(WHITE_PEN));
                       if (brush) {
                           oldBrush = SelectObject(dc, brush);
                           DrawMonoRect(dc, r.left, r.top, r.right, r.bottom);
                           if (oldBrush)
                               SelectObject(dc, oldBrush);
                           DeleteObject(brush);
                       }

                       r.left = wid >> 1;
                       r.right = r.left + wid;
                       r.top = hgt >> 1;
                       r.bottom = r.top + hgt;
                       brush = CreateSolidBrush(rgbColor[theForeg]);

                       if (brush) { 
                           oldBrush = SelectObject(dc, brush);
                           DrawMonoRect(dc, r.left, r.top, r.right, r.bottom);
                           if (oldBrush)
                               SelectObject(dc, oldBrush);
                           DeleteObject(brush);
                       }

                       ReleaseDC(hWnd, dc);
                   }
                   SendMessage(pbrushWnd[PAINTid], WM_CHANGEFONT, 0, 0L);
               }
           }
           break;

       /* edit the color */
       case WM_LBUTTONDBLCLK:
           if (!colorWnd) {
	       if (inMagnify)
		    SendMessage(pbrushWnd[PARENTid],WM_COMMAND,
			GET_WM_COMMAND_MPS(ZOOMaccept,0,0));
	       SendMessage(pbrushWnd[PARENTid],WM_COMMAND,
			GET_WM_COMMAND_MPS(MISCeditColor,0,0));
           }
           break;

       /* all other messages handled by the system */
       default:
           return DefWindowProc(hWnd, message, wParam, lParam);
           break;
   }

   return(0L);
}

LONG FAR PASCAL PalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LONG lParam)
{
   return 0L;
}
