/****************************Module*Header******************************\
* Module Name: dotpoly.c                                                *
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


void DotPoly(HDC dc, POINT *polyPts, int numPts)
{
   HPEN dotPen,oldPen;
   int oldDC;

   oldDC = SaveDC(dc);

   dotPen = CreatePen(PS_DOT, 1, WHITErgb);
   if (dotPen)
       oldPen = SelectObject(dc, dotPen);

   SelectObject(dc, GetStockObject(NULL_BRUSH));
   SetROP2(dc, R2_XORPEN);
   SetBkMode(dc, TRANSPARENT);

   Polygon(dc, (LPPOINT) polyPts, numPts);

   if (oldPen)
      SelectObject(dc, oldPen);
   if (dotPen) 
      DeleteObject(dotPen);

   RestoreDC(dc, oldDC);
}
