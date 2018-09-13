/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   GetAspct.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  calculate aspect ratio for given size       *
*   date:   04/01/87 @ 11:00                            *
*   date:   07/12/87 @ 11:00    MSZ                     *
*                                                       *
********************************************************/

#include "onlypbr.h"
#undef NOMINMAX

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"

void GetAspct(int size, int *h, int *v)
{
   register int i, asize;

   asize = abs(size);
   if(asize <= 1) {
      *v = *h = 1;
   } else if((long)size * (horzDotsMM - vertDotsMM) > 0) {
      *v = asize;
      i = (int) (((long) asize * horzDotsMM + (vertDotsMM - 1)) / vertDotsMM);
      *h = max(i, 1);
   } else {
      *h = asize;
      i = (int) (((long) asize * vertDotsMM + (horzDotsMM - 1)) / horzDotsMM);
      *v = max(i, 1);
   }
}
