/* $Id$
 *
 *  FreeLoader
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

BOOLEAN
NTAPI
HalQueryRealTimeClock(OUT PTIME_FIELDS Time);

TIMEINFO*
PcGetTime(VOID)
{
    static TIMEINFO TimeInfo;
    TIME_FIELDS Time;

    if (!HalQueryRealTimeClock(&Time))
        return NULL;

    TimeInfo.Year = Time.Year;
    TimeInfo.Month = Time.Month;
    TimeInfo.Day = Time.Day;
    TimeInfo.Hour = Time.Hour;
    TimeInfo.Minute = Time.Minute;
    TimeInfo.Second = Time.Second;

    return &TimeInfo;
}

/* EOF */
