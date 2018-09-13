/****************************Module*Header******************************\
* Module Name: zoominwp.c                                               *
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
#include "pbrush.h"

extern int paintWid, paintHgt;
extern RECT zoomView, imageView;
extern int zoomWid, zoomHgt, imageWid, imageHgt;
extern HWND pbrushWnd[];
extern POINT csrPt, viewExt, viewOrg;
extern HPALETTE hPalette;
extern int zoomAmount;
extern BOOL drawing;

/* uncomment next line to use PatBlt to generate gridlines */
//#define FASTGRIDLINES

LONG ZoomInWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int         i;
   PAINTSTRUCT ps;
   POINT       oldPt;
   HDC         paintDC, hMemDC, patDC;
   HBITMAP     hBitmap, patBitmap;
   HBRUSH      patBrush, oldBrush;
   int         ix, iy;
   HCURSOR     oldcsr;
   HPALETTE    hOldPalette = NULL;
   BOOL        bIsScroll;

   switch (message) {
       case WM_SIZE:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           paintWid = LOWORD(lParam);
           paintHgt = HIWORD(lParam);
           CalcView();

           if ((zoomView.right = (zoomView.left + zoomWid)) > imageWid) {
               zoomView.left = imageWid - zoomWid;
               zoomView.right = imageWid;
           }

           if ((zoomView.bottom = (zoomView.top + zoomHgt)) > imageHgt) {
               zoomView.top = imageHgt - zoomHgt;
               zoomView.bottom = imageHgt;
           }
	   ix = imageWid;
	   iy = imageHgt;

           if ((ix + imageView.left) < imageWid)
               ix = min(ix + GetSystemMetrics(SM_CXVSCROLL),
                        imageWid - imageView.left);

           if ((iy + imageView.top) < imageHgt)
               iy = min(iy + GetSystemMetrics(SM_CYHSCROLL),
                        imageHgt - imageView.top);

/* EDH */
	   SetScrollRange(hWnd, SB_HORZ, 0, imageView.right - imageView.left - zoomWid, TRUE);
	   SetScrollRange(hWnd, SB_VERT, 0, imageView.bottom - imageView.top - zoomHgt, TRUE);

	   if (ix == zoomWid)
                zoomView.left = 0;
           SetScrollPos(hWnd, SB_HORZ, zoomView.left, TRUE);
           if (iy == zoomHgt)
                zoomView.top = 0;
           SetScrollPos(hWnd, SB_VERT, zoomView.top, TRUE);

           SetCursor(oldcsr);
           break;

       case WM_VSCROLL:
       case WM_HSCROLL:
	   bIsScroll = (GET_WM_HSCROLL_CODE(wParam,lParam) == SB_BOTTOM ||
			GET_WM_HSCROLL_CODE(wParam,lParam) == SB_PAGEDOWN ||
			GET_WM_HSCROLL_CODE(wParam,lParam) == SB_PAGEUP ||
			GET_WM_HSCROLL_CODE(wParam,lParam) == SB_THUMBPOSITION ||
			GET_WM_HSCROLL_CODE(wParam,lParam) == SB_TOP);

           if (bIsScroll)
               oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

           ScrolMag(hWnd, message, wParam, lParam);

           if (bIsScroll)
               SetCursor(oldcsr);

	   if (GET_WM_HSCROLL_CODE(wParam,lParam) == SB_ENDSCROLL)
               PostMessage(pbrushWnd[PAINTid], WM_SCROLLDONE, 0, 0l);
           break;

       case WM_PAINT:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           oldPt = csrPt;
           SendMessage(hWnd, WM_HIDECURSOR, 0, 0L);
           BeginPaint(hWnd, &ps);
           paintDC = ps.hdc;

#ifndef FASTGRIDLINES
           SelectObject(paintDC, GetStockObject(WHITE_BRUSH));
           oldBrush = NULL;
           if (patDC = CreateCompatibleDC(paintDC)) {
               i = max(8, zoomAmount);
               if (i > zoomAmount)
                   if ((zoomAmount != 1) && (zoomAmount != 2) && 
                       (zoomAmount != 4))
                       i = zoomAmount * (1 + 8 / zoomAmount);
               if (patBitmap = CreateCompatibleBitmap(paintDC, i, i)) {
                   SelectObject(patDC, patBitmap);
                   PatBlt(patDC, 0, 0, i, i, BLACKNESS);
                   PatBlt(patDC, 0, 0, i-1, i-1, WHITENESS);
                   patBrush = CreatePatternBrush(patBitmap);
                   if (patBrush)
                       oldBrush = SelectObject(paintDC, patBrush);

                   DeleteDC(patDC);
                   DeleteObject(patBitmap);
               } else
                   DeleteDC(patDC);
           }
#endif

           hMemDC = CreateCompatibleDC(paintDC);
           hBitmap = CreateCompatibleBitmap(paintDC, viewExt.x, viewExt.y);
           if (hMemDC && hBitmap)
               SelectObject(hMemDC, hBitmap);

           if (hPalette) {
               hOldPalette = SelectPalette(paintDC, hPalette, 0);
               SelectPalette(hMemDC, hPalette, 0);
               RealizePalette(hMemDC);
               RealizePalette(paintDC);
           }

           if (hMemDC) {
               PatBlt(hMemDC, 0, 0, viewExt.x, viewExt.y, BLACKNESS);
	       BitBlt(hMemDC, viewOrg.x, viewOrg.y, zoomWid, zoomHgt,
		       hdcWork, imageView.left + zoomView.left,
				imageView.top + zoomView.top, SRCCOPY);

               BitBlt(paintDC, 0, 0, viewExt.x, viewExt.y,
                   hMemDC, 0, 0, SRCCOPY);

               ExcludeClipRect(paintDC, 0, 0, viewExt.x, viewExt.y);

               SetStretchBltMode(paintDC, COLORONCOLOR);
   #ifndef FASTGRIDLINES
               StretchBlt(paintDC, 0, 0, zoomWid * zoomAmount, 
                       zoomHgt * zoomAmount,
                       hMemDC, viewOrg.x, viewOrg.y, zoomWid, zoomHgt, 
                       ROP_SPa);

   #else
               StretchBlt(paintDC, 0, 0, zoomWid * zoomAmount, 
                       zoomHgt * zoomAmount,
                       hMemDC, viewOrg.x, viewOrg.y, zoomWid, zoomHgt, SRCCOPY);
   #endif
           }

#ifdef FASTGRIDLINES

           {
           for (i = 8; i < paintWid; i += 8)
               PatBlt(paintDC, i, 0, 1, paintHgt, BLACKNESS);

           for (i = 8; i < paintWid; i += 8)
               PatBlt(paintDC, 0, i, paintWid, 1, BLACKNESS);
           }
#endif

           if (hMemDC)
               DeleteDC(hMemDC);
           if (hBitmap)
               DeleteObject(hBitmap);

#ifndef FASTGRIDLINES
           DeleteObject(SelectObject(paintDC, oldBrush));
#endif

           if (hOldPalette)
               SelectPalette(paintDC, hOldPalette, 0);

           EndPaint(hWnd, &ps);
           SetCursor(oldcsr);
           break;

       case WM_MOUSEMOVE:
           if (!drawing)
	       LONG2POINT(lParam,oldPt);
	   ZoomedInDP(hWnd, message, wParam, lParam);
           break;

       case WM_LBUTTONDOWN:
       case WM_RBUTTONDOWN:
       case WM_MBUTTONDOWN:
       case WM_LBUTTONUP:
       case WM_RBUTTONUP:
       case WM_LBUTTONDBLCLK:
       case WM_RBUTTONDBLCLK:
       case WM_HIDECURSOR:
       case WM_TERMINATE:
       case WM_ZOOMUNDO:
       case WM_ZOOMACCEPT:
       case WM_SCROLLINIT:
       case WM_SCROLLDONE:
       case WM_SCROLLVIEW:
	   ZoomedInDP(hWnd, message, wParam, lParam);
           break;

       case WM_COMMAND:
	   if (GET_WM_COMMAND_HWND(wParam,lParam) == 0)
	       MenuCmd(hWnd, GET_WM_COMMAND_ID(wParam,lParam));
           break;

       default:
           return DefWindowProc(hWnd, message, wParam, lParam);
           break;
      }
   return 0L;
}
