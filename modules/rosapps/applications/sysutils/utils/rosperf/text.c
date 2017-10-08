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
TextProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  unsigned Rep;
  int y;
  HDC Dc = NULL;
  HFONT hfFont = GetStockObject(DEFAULT_GUI_FONT);

  for (Rep = 0; Rep < Reps; )
    {
      Dc = (Rep & 0x10000) ? PerfInfo->BackgroundDc : PerfInfo->ForegroundDc;
      SelectObject(Dc, hfFont);

      for (y = 0; y < PerfInfo->WndHeight && Rep < Reps; Rep++, y += 15)
        {
		TextOut(Dc, 0, y, L"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz:?<>0123456789", 66);
        }
    InvalidateRect(PerfInfo->Wnd, NULL, TRUE);
    UpdateWindow(PerfInfo->Wnd);
    }
}

/* EOF */
