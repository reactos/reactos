/****************************Module*Header******************************\
* Module Name: polyto.c                                                 *
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

void PolyTo(POINT fromPt, POINT toPt, int wid, int hgt)
{
   if (toPt.x > fromPt.x) {
       polyPts[0].x = polyPts[1].x = fromPt.x;
       polyPts[2].x = toPt.x;
       polyPts[3].x = polyPts[4].x = toPt.x + wid;
       polyPts[5].x = fromPt.x + wid;
   } else {
       polyPts[0].x = polyPts[1].x = fromPt.x + wid;
       polyPts[2].x = toPt.x + wid;
       polyPts[3].x = polyPts[4].x = toPt.x;
       polyPts[5].x = fromPt.x;
   }

   if (toPt.y > fromPt.y) {
       polyPts[0].y = polyPts[5].y = fromPt.y;
       polyPts[1].y = fromPt.y + hgt;
       polyPts[2].y = polyPts[3].y = toPt.y + hgt;
       polyPts[4].y = toPt.y;
   } else {
       polyPts[0].y = polyPts[5].y = fromPt.y + hgt;
       polyPts[1].y = fromPt.y;
       polyPts[2].y = polyPts[3].y = toPt.y;
       polyPts[4].y = toPt.y + hgt;
   }

}
