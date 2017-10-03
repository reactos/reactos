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
LinesProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  int Dest;
  HDC Dc;

  for (Rep = 0; Rep < Reps; )
    {
      Dc = (Rep & 0x1000) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc;

      for (Dest = 2; Dest < PerfInfo->WndHeight && Rep < Reps; Rep++, Dest += 2)
        {
          MoveToEx(Dc, 0, 0, NULL);
          LineTo(Dc, PerfInfo->WndWidth, Dest);
        }

      for (Dest = PerfInfo->WndWidth - 2; 0 <= Dest && Rep < Reps; Rep++, Dest -= 2)
        {
          MoveToEx(Dc, PerfInfo->WndWidth, 0, NULL);
          LineTo(Dc, Dest, PerfInfo->WndHeight);
        }

      for (Dest = PerfInfo->WndHeight - 2; 0 <= Dest && Rep < Reps; Rep++, Dest -= 2)
        {
          MoveToEx(Dc, PerfInfo->WndWidth, PerfInfo->WndHeight, NULL);
          LineTo(Dc, 0, Dest);
        }

      for (Dest = 2; Dest < PerfInfo->WndWidth && Rep < Reps; Rep++, Dest += 2)
        {
          MoveToEx(Dc, 0, PerfInfo->WndHeight, NULL);
          LineTo(Dc, Dest, 0);
        }
    }
}

void
LinesHorizontalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  int y;
  HDC Dc;

  for (Rep = 0; Rep < Reps; )
    {
      Dc = (Rep & 0x10000) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc;

      for (y = 0; y < PerfInfo->WndHeight && Rep < Reps; Rep++, y += 3)
        {
          MoveToEx(Dc, 0, y, NULL);
          LineTo(Dc, PerfInfo->WndWidth, y);
        }
    }
}

void
LinesVerticalProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  int x;
  HDC Dc;

  for (Rep = 0; Rep < Reps; )
    {
      Dc = (Rep & 0x1000) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc;

      for (x = 0; x < PerfInfo->WndWidth && Rep < Reps; Rep++, x += 3)
        {
          MoveToEx(Dc, x, 0, NULL);
          LineTo(Dc, x, PerfInfo->WndHeight);
        }
    }
}

/* EOF */
