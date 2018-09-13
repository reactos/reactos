/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   GetTanPt.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  calculates point tangent to an ellipse      *
*   date:   04/08/87 @ 13:50                            *
*                                                       *
********************************************************/

#include "onlypbr.h"
#undef NORASTEROPS

#include <windows.h>
#include "port1632.h"
#define NOEXTERN
#include "pbrush.h"

void GetTanPt(int wid, int hgt, int delX, int delY, RECT *tan)
{
   int x, y;
   int xExt, yExt, theExt, xTemp;
   HDC monoDC, hDC;
   HBITMAP monoBitmap, hMonoBM;

   tan->left = tan->right = tan->top = tan->bottom = 0;

   if(!(hDC = GetDC(NULL)))
      goto Error1;
   if(!(monoDC = CreateCompatibleDC(hDC)))
      goto Error2;

   if(!(monoBitmap = CreateBitmap(wid, hgt, (BYTE) 1, (BYTE) 1, NULL)))
      goto Error3;
   hMonoBM = SelectObject(monoDC, monoBitmap);

   PatBlt(monoDC, 0, 0, wid, hgt, WHITENESS);
   SelectObject(monoDC, GetStockObject(BLACK_PEN));
   SelectObject(monoDC, GetStockObject(BLACK_BRUSH));
   Ellipse(monoDC, 0, 0, wid, hgt);

   yExt = 0;
   for(xExt=0; xExt<wid-1; ++xExt)
      if(!GetPixel(monoDC, xExt, 0))
         break;
   theExt = 10*xExt;

   if(delY == 0) {
      tan->left = xExt;
      tan->top = 0;
      tan->right = xExt;
      tan->bottom = hgt-1;
   } else {
      for(y=0; y<hgt; ++y) {
         for(x=0; x<wid-1; ++x)
            if(!GetPixel(monoDC, x, y))
               break;
         xTemp = 10*x - 10*y*delX/delY;
         if(theExt > xTemp) {
            xExt = x;
            yExt = y;
            theExt = xTemp;
         } else if(theExt < xTemp)
            break;
      }
      tan->left = xExt;
      tan->top = yExt;

      for(y=0; y<hgt; ++y) {
         for(x=wid-1; x>0; --x)
            if(!GetPixel(monoDC, x, y))
               break;
         xTemp = 10*x - 10*y*delX/delY;
         if(theExt < xTemp) {
            xExt = x;
            yExt = y;
            theExt = xTemp;
         } else if(theExt > xTemp)
            break;
      }
      tan->right = xExt;
      tan->bottom = yExt;
   }

   if (hMonoBM)
       SelectObject(monoDC, hMonoBM);
   DeleteObject(monoBitmap);
Error3:
   DeleteDC(monoDC);
Error2:
   ReleaseDC(NULL, hDC);
Error1:
   ;
}
