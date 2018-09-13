/****************************Module*Header******************************\
* Module Name: polyrect.c                                               *
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


void PolyRect(LPPOINT polyPts, int numPts, LPRECT dstRect)
{
   int     i;
   RECT    r;

   r.left = r.right = polyPts[0].x;
   r.top = r.bottom = polyPts[0].y;
   ++r.right;
   ++r.bottom;

   for (i = 1; i < numPts; ++i) {
       if (polyPts[i].x < r.left)
           r.left = polyPts[i].x;
       else if (polyPts[i].x >= r.right)
           r.right = polyPts[i].x + 1;

       if (polyPts[i].y < r.top)
           r.top = polyPts[i].y;
       else if (polyPts[i].y >= r.bottom)
           r.bottom = polyPts[i].y + 1;
   }

   CopyRect(dstRect, &r);
}
