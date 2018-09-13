/****************************Module*Header******************************\
* Module Name: dotrect.c                                                *
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
#define NOEXTERN
#include "pbrush.h"

void DotRect(HDC dc, int x1, int y1, int x2, int y2)
{
   static HBRUSH patBrush = NULL;

   HDC patDC;
   HBITMAP patBitmap;
   HBRUSH oldBrush;
   int t, wid, hgt;

   /* null DC is flag to free patBrush */
   if (!dc) {
       if (patBrush)
           DeleteObject(patBrush);
       patBrush = NULL;
       return;
   }

   /* if patBrush is NULL we have to create it */
   if (!patBrush) {
       patDC = CreateCompatibleDC(dc);
       if (patDC) {
           patBitmap = CreateCompatibleBitmap(dc, 8, 8);
           if (patBitmap) {
               SelectObject(patDC, patBitmap);
               PatBlt(patDC, 0, 0, 8, 8, BLACKNESS);
               PatBlt(patDC, 4, 0, 4, 4, WHITENESS);
               PatBlt(patDC, 0, 4, 4, 4, WHITENESS);
               patBrush = CreatePatternBrush(patBitmap);
               DeleteDC(patDC);
               DeleteObject(patBitmap);
           } else
               DeleteDC(patDC);
       }
   }

   if (patBrush)
       oldBrush = SelectObject(dc, patBrush);
   else 
       oldBrush = SelectObject(dc, GetStockObject(WHITE_BRUSH));

   if (x2 < x1) { t = x1; x1 = x2; x2 = t; }
   if (y2 < y1) { t = y1; y1 = y2; y2 = t; }

   PatBlt(dc, x1, y1, wid = x2 - x1, 1, PATINVERT);
   PatBlt(dc, x1, y2, wid, 1, PATINVERT);
   PatBlt(dc, x1, y1, 1, hgt = y2 - y1, PATINVERT);
   PatBlt(dc, x2, y1, 1, hgt, PATINVERT);

   if (oldBrush)
      SelectObject(dc, oldBrush);
}
