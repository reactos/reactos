/*
 *  ReactOS RosPerf - ReactOS GUI performance test program (gradient.c)
 *
 *  Copyright 2006 Timo Kreuzer <timo.kreuzer@web.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <windows.h>
#include "rosperf.h"

void
GradientProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  HDC Dc;
  TRIVERTEX vert[6];
  static GRADIENT_TRIANGLE gcap[4];

  Dc = PerfInfo->ForegroundDc;
  vert[0].x = 0;
  vert[0].y = 0;
  vert[0].Red = 0xff00;
  vert[0].Green = 0;
  vert[0].Blue = 0;
  vert[0].Alpha = 0;
  vert[1].x = PerfInfo->WndWidth;;
  vert[1].y = 0;
  vert[1].Red = 0;
  vert[1].Green = 0xff00;
  vert[1].Blue = 0;
  vert[1].Alpha = 0;
  vert[2].x = PerfInfo->WndWidth;
  vert[2].y = PerfInfo->WndHeight;
  vert[2].Red = 0;
  vert[2].Green = 0;
  vert[2].Blue = 0xff00;
  vert[2].Alpha = 0;
  vert[3].x = 0;
  vert[3].y = PerfInfo->WndHeight;
  vert[3].Red = 0;
  vert[3].Green = 0xff00;
  vert[3].Blue = 0;
  vert[3].Alpha = 0;
  vert[4].Red = 0;
  vert[4].Green = 0;
  vert[4].Blue = 0;
  vert[4].Alpha = 0;
  vert[5].Red = 0;
  vert[5].Green = 0;
  vert[5].Blue = 0;
  vert[5].Alpha = 0;

  for (Rep = 0; Rep < Reps; Rep++)
    {
      switch (2 * Rep / Reps)
      {
        case 0:
             vert[4].x = PerfInfo->WndWidth * 2 * Rep / Reps;
             vert[4].y = 0;
             vert[5].x = PerfInfo->WndWidth - vert[4].x;
             vert[5].y = PerfInfo->WndHeight;
             gcap[0].Vertex1 = 5; gcap[0].Vertex2 = 0; gcap[0].Vertex3 = 4;
             gcap[1].Vertex1 = 5; gcap[1].Vertex2 = 4; gcap[1].Vertex3 = 1;
             gcap[2].Vertex1 = 5; gcap[2].Vertex2 = 1; gcap[2].Vertex3 = 2;
             gcap[3].Vertex1 = 5; gcap[3].Vertex2 = 3; gcap[3].Vertex3 = 0;
             break;
        case 1:
             vert[4].x = PerfInfo->WndWidth;
             vert[4].y = PerfInfo->WndHeight * 2 * Rep / Reps - PerfInfo->WndHeight;
             vert[5].x = 0;
             vert[5].y = PerfInfo->WndHeight - vert[4].y;
             gcap[0].Vertex1 = 5; gcap[0].Vertex2 = 1; gcap[0].Vertex3 = 4;
             gcap[1].Vertex1 = 5; gcap[1].Vertex2 = 4; gcap[1].Vertex3 = 2;
             gcap[2].Vertex1 = 5; gcap[2].Vertex2 = 2; gcap[2].Vertex3 = 3;
             gcap[3].Vertex1 = 5; gcap[3].Vertex2 = 0; gcap[3].Vertex3 = 1;
             break;
      }

      GradientFill(Dc, vert, 6, &gcap, 4, GRADIENT_FILL_TRIANGLE);
   }
}

void
GradientHorizontalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  HDC Dc;
  TRIVERTEX vert[2];
  static GRADIENT_RECT gcap = {0, 1};

  Dc = PerfInfo->ForegroundDc;
  for (Rep = 0; Rep < Reps; Rep++)
    {
      vert[0].x = 0;
      vert[0].y = 0;
      vert[0].Red = 0xff00;
      vert[0].Green = 0xff00 - 0xff00 * Rep / Reps;
      vert[0].Blue = 0xff00 * Rep / Reps;
      vert[0].Alpha = 0;

      vert[1].x = PerfInfo->WndWidth;
      vert[1].y = PerfInfo->WndHeight;
      vert[1].Red = 0xff00 - 0xff00 * Rep / Reps;
      vert[1].Green = 0xff00 * Rep / Reps;
      vert[1].Blue = 0xff00;
      vert[1].Alpha = 0;

      GradientFill(Dc, vert, 2, &gcap, 1, GRADIENT_FILL_RECT_H);
    }
}

void
GradientVerticalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  HDC Dc;
  TRIVERTEX vert[2];
  static GRADIENT_RECT gcap = {0, 1};

  Dc = PerfInfo->ForegroundDc;
  for (Rep = 0; Rep < Reps; Rep++)
    {
      vert[0].x = 0;
      vert[0].y = 0;
      vert[0].Red = 0xff00 * Rep / Reps;
      vert[0].Green = 0xff00 - 0xff00 * Rep / Reps;
      vert[0].Blue = 0xff00;
      vert[0].Alpha = 0;

      vert[1].x = PerfInfo->WndWidth;
      vert[1].y = PerfInfo->WndHeight;
      vert[1].Red = 0xff00 - 0xff00 * Rep / Reps;
      vert[1].Green = 0xff00;
      vert[1].Blue = 0xff00 * Rep / Reps;
      vert[1].Alpha = 0;

      GradientFill(Dc, vert, 2, &gcap, 1, GRADIENT_FILL_RECT_V);
    }
}

/* EOF */
