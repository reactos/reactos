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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "freeldr.h"
#include "arch.h"
#include "machine.h"
#include "machxen.h"

#define SECSPERMIN      60
#define MINSPERHOUR     60
#define HOURSPERDAY     24
#define DAYSPERWEEK     7
#define DAYSPERNYEAR    365
#define DAYSPERLYEAR    366
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY      ((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR     12
#define EPOCH_YEAR      1970

#define IsLeap(Yr) ((0 == ((Yr) % 4) && 0 != ((Yr) % 100)) || 0 == ((Yr) % 400))

static const int MonLengths[2][MONSPERYEAR] = {
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};
static const int YearLengths[2] = {
DAYSPERNYEAR, DAYSPERLYEAR
};

static void
TimeSub(ULONG Time, LONG Offset,
        PULONG Year, PULONG Month, PULONG Day,
        PULONG Hour, PULONG Minute, PULONG Second)
{
  long Days;
  long Rem;
  int Yr;
  int Yleap;
  const int * ip;

  Days = Time / SECSPERDAY;
  Rem = Time % SECSPERDAY + Offset;
  while (Rem < 0)
    {
      Rem += SECSPERDAY;
      Days--;
    }
  while (SECSPERDAY <= Rem)
    {
      Rem -= SECSPERDAY;
      Days++;
    }
  *Hour = (ULONG) (Rem / SECSPERHOUR);
  Rem = Rem % SECSPERHOUR;
  *Minute = (ULONG) (Rem / SECSPERMIN);
  *Second = (ULONG) (Rem % SECSPERMIN);
  Yr = EPOCH_YEAR;
  if (0 <= Days)
    {
      while (TRUE)
        {
          Yleap = IsLeap(Yr);
          if (Days < (long) YearLengths[Yleap])
            {
              break;
            }
          ++Yr;
          Days = Days - (long) YearLengths[Yleap];
        }
    }
  else
    {
      do
        {
          --Yr;
          Yleap = IsLeap(Yr);
          Days = Days + (long) YearLengths[Yleap];
        }
      while (Days < 0);
    }
  *Year = Yr;
  ip = MonLengths[Yleap];
  for (*Month = 0; Days >= (long) ip[*Month]; ++(*Month))
    {
      Days = Days - (long) ip[*Month];
    }
  (*Month)++;
  *Day = (ULONG) (Days + 1);
}

VOID
XenRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day,
                         PULONG Hour, PULONG Minute, PULONG Second)
{
  ULONG Time;
  ULONG ShadowTimeVersion;

  do
    {
      ShadowTimeVersion = XenSharedInfo->time_version2;
      Time = XenSharedInfo->wc_sec;
    }
  while (ShadowTimeVersion != XenSharedInfo->time_version1);

  TimeSub(Time, 0, Year, Month, Day, Hour, Minute, Second);
}

/* EOF */
