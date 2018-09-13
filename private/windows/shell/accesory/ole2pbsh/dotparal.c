/****************************Module*Header******************************\
* Module Name: dotparal.c                                               *
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


void DotParal(HDC dc, PARAL *p)
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

   MMoveTo(dc, p->topLeft.x, p->topLeft.y);
   LineTo(dc, p->topRight.x, p->topRight.y);
   LineTo(dc, p->botRight.x, p->botRight.y);
   LineTo(dc, p->botLeft.x, p->botLeft.y);
   LineTo(dc, p->topLeft.x, p->topLeft.y);

   if (oldPen)
       SelectObject(dc, oldPen);
   if (dotPen) 
       DeleteObject(dotPen);
   RestoreDC(dc, oldDC);
}
