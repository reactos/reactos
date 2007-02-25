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

#include <freeldr.h>

#define RTC_REGISTER_A   0x0A
#define   RTC_REG_A_UIP  0x80  /* Update In Progress bit */

#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

static UCHAR
HalpQueryCMOS(UCHAR Reg)
{
  UCHAR Val;
  Reg |= 0x80;

  WRITE_PORT_UCHAR((PUCHAR)0x70, Reg);
  Val = READ_PORT_UCHAR((PUCHAR)0x71);
  WRITE_PORT_UCHAR((PUCHAR)0x70, 0);

  return(Val);
}

VOID
XboxRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second)
{
  while (HalpQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
    {
      ;
    }

  if (NULL != Second)
    {
      *Second = BCD_INT(HalpQueryCMOS(0));
    }
  if (NULL != Minute)
    {
      *Minute = BCD_INT(HalpQueryCMOS(2));
    }
  if (NULL != Hour)
    {
      *Hour = BCD_INT(HalpQueryCMOS(4));
    }
  if (NULL != Day)
    {
      *Day = BCD_INT(HalpQueryCMOS(7));
    }
  if (NULL != Month)
    {
      *Month = BCD_INT(HalpQueryCMOS(8));
    }
  if (NULL != Year)
    {
      *Year = BCD_INT(HalpQueryCMOS(9));
      if (*Year > 80)
        {
          *Year += 1900;
        }
      else
        {
          *Year += 2000;
        }
    }
}

/* EOF */
