/****************************Module*Header******************************\
* Module Name: flippoly.c                                               *
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
#define    NOEXTERN
#include "pbrush.h"


void FlipPoly(POINT *polyPts, int numPts, int dir, RECT *r)
{
   int i;

   if (dir == FLIPh) {
       for (i = 0; i < numPts; ++i)
           polyPts[i].x = r->right - 1 - (polyPts[i].x - r->left);
   } else {
       for (i = 0; i < numPts; ++i)
           polyPts[i].y = r->bottom - 1 - (polyPts[i].y - r->top);
   }
}

void OffsetPoly(POINT *polyPts, int numPts, int xOffset, int yOffset)
{
   int i;

   for (i = 0; i < numPts; ++i) {
       polyPts[i].x += xOffset;
       polyPts[i].y += yOffset;
   }
}
