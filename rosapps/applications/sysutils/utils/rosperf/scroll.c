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
ScrollProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
    unsigned Rep;
    RECT rect;
    int x = 0, y = 0, dx = 1, dy = 1;

    rect.left = rect.top = 0;
    rect.right = PerfInfo->WndWidth;
    rect.bottom = PerfInfo->WndHeight;

    DrawTextW(PerfInfo->ForegroundDc, L"rosperf", -1, &rect, DT_LEFT | DT_TOP);

    for (Rep = 0; Rep < Reps; Rep++)
    {
        ScrollDC((Rep & 0x100) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc,
                     dx, dy, NULL, NULL, NULL, NULL);
        x += dx;
        y += dy;
        if ((x >= PerfInfo->WndWidth -50) || x == 0) dx = -dx;
        if ((y >= PerfInfo->WndHeight -10) || y == 0) dy = -dy;
    }

}

/* EOF */
