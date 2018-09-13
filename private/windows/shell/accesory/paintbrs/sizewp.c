/****************************Module*Header******************************\
* Module Name: sizewp.c                                                 *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
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
#include "pbserver.h"

static HBITMAP hArrow;

extern HWND pbrushWnd[];
extern int theSize;
extern POINT aspect;
extern struct csstat CursorStat;

int SizeTable[] = { 1,  2,  3,  4,  5,  7,  9, 11 };
int YPosTable[] = { 8, 13, 19, 26, 34, 43, 54, 67, 200 };

int PRIVATE Pos2Index(int pos)
{
   int i;

   if (SizeTable[7] < pos)
       return 7;

   for (i = 0; SizeTable[i] < pos; ++i)
      ;

   return i;
}

void PRIVATE InitDC(HDC hDC)
{
   RECT client;

   /* setup mapping mode to give consistent logical coordinates */
   SetMapMode(hDC, MM_ANISOTROPIC);
   MSetWindowExt(hDC, SIZE_EXTX, SIZE_EXTY);
   GetClientRect(pbrushWnd[SIZEid], &client);
   MSetViewportExt(hDC, 1 + client.right - client.left,
                       1 + client.bottom - client.top);
   MSetViewportOrg(hDC, 0, 0);
}

void PRIVATE PaintSize(HDC hDC)
{
   int       i, index;
   HDC       hDCMem;
   SHORTPARM oldBltMode;
   BITMAP    hbits;
   RECT      rect;
   HBRUSH    hBrush;

   InitDC(hDC);

   /* draw the line widths */
   rect.left = 19;
   rect.right = 51;
   hBrush = GetStockObject(BLACK_BRUSH);
   for (i = 0; i < NUM_SIZES; ++i) {
      rect.top = YPosTable[i];
      rect.bottom = rect.top + SizeTable[i];
      FillRect(hDC, &rect, hBrush);
   }

   /* draw the arrow */
   hDCMem = CreateCompatibleDC(hDC);
   if (hDCMem) {
        SetROP2(hDC, R2_COPYPEN);
        oldBltMode = SetStretchBltMode(hDC, COLORONCOLOR);
       GetObject(hArrow, sizeof(hbits), (LPVOID) &hbits);
       SelectObject(hDCMem, hArrow);
       index = Pos2Index(theSize);
       StretchBlt(hDC, 4, YPosTable[index] +
                          ((SizeTable[index] - (int)(hbits.bmHeight)) / 2),
                  hbits.bmWidth, hbits.bmHeight, hDCMem, 0, 0,
                  hbits.bmWidth, hbits.bmHeight,
                  SRCCOPY);
        SetStretchBltMode(hDC, oldBltMode);
       DeleteDC(hDCMem);
   }
}

int PRIVATE LocateSize(POINT pt)
{
   HDC hDC;
   int i;

   hDC = GetDisplayDC(pbrushWnd[SIZEid]);
   i = 0;
   if (hDC) {
       InitDC(hDC);
       DPtoLP(hDC, &pt, 1);

       for (; pt.y >= YPosTable[i] + SizeTable[i]; ++i)
           ;

       ReleaseDC(pbrushWnd[SIZEid], hDC);
   }

   return SizeTable[min(NUM_SIZES - 1, i)];
}

LONG FAR PASCAL SizeWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   int         newSize;
   PAINTSTRUCT ps;
   POINT       pt;

   LONG2POINT(lParam,pt);

   switch (message) {
        case WM_CREATE:
           hArrow = LoadBitmap(hInst, TEXT("pArrow"));
            /* if we couldn't load resource terminate the app */
            if (!hArrow)
                return -1L;
            break;

        case WM_DESTROY:
            if(hArrow)
               DeleteObject(hArrow);
            break;

       case WM_MOUSEMOVE:
           if (SetCursorOn())
               SendMessage(pbrushWnd[PAINTid], WM_HIDECURSOR, 0, 0L);
           return DefWindowProc(hWnd, message, wParam, lParam);
           break;

       case WM_PAINT:
            BeginPaint(hWnd, &ps);
           if (!fInvisible)
              PaintSize(ps.hdc);
            EndPaint(hWnd, &ps);
           break;

       /* change drawing size */
       case WM_LBUTTONDOWN:
           newSize = LocateSize(pt);
           if (newSize != theSize) {
               /* erase old and paint new size indicators */
               theSize = newSize;
               GetAspct(theSize, (PINT)&aspect.x, (PINT)&aspect.y);
               if (!CursorStat.captured)
                   CursorStat.noted = FALSE;
               InvalidateRect(pbrushWnd[SIZEid], NULL, TRUE);
               if (theTool != TEXTtool)
                   SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
           }
           break;

       default:
           return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0L;
}
