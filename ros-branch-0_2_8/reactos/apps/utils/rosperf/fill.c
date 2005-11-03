/*
 *  ReactOS RosPerf - ReactOS GUI performance test program
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
FillProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;

  for (Rep = 0; Rep < Reps; Rep++)
    {
      PatBlt((Rep & 0x100) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc, 0, 0,
             PerfInfo->WndWidth, PerfInfo->WndHeight, PATCOPY);
    }
}

void
FillSmallProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
#define SMALL_SIZE 16
  unsigned Rep;
  int x, y;

  x = 0;
  y = 0;

  for (Rep = 0; Rep < Reps; Rep++)
    {
      PatBlt((Rep & 0x10000) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc, x, y,
             SMALL_SIZE, SMALL_SIZE, PATCOPY);
      x += SMALL_SIZE + 1;
      if (PerfInfo->WndWidth < x + SMALL_SIZE)
        {
          x = 0;
          y += SMALL_SIZE + 1;
          if (PerfInfo->WndHeight < y + SMALL_SIZE)
            {
              y = 0;
            }
        }
    }
}
/* EOF */
