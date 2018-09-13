/****************************Module*Header******************************\
* Module Name: offspoly.c                                               *
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

void OffsPoly(POINT *polyPts, int numPts, int x, int y)
{
   int i;

   for (i = 0; i < numPts; ++i, ++polyPts) {
       polyPts->x += x;
       polyPts->y += y;
   }
}
