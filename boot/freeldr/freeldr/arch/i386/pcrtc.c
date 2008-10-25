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

#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

VOID
PcRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second)
{
  REGS Regs;

  if (NULL != Year || NULL != Month || NULL != Day)
    {
      /* Some BIOSes, such es the 1998/07/25 system ROM
       * in the Compaq Deskpro EP/SB, leave CF unchanged
       * if successful, so CF should be cleared before
       * calling this function. */
      __asm__ ("clc");

      /* Int 1Ah AH=04h
       * TIME - GET REAL-TIME CLOCK DATE (AT,XT286,PS)
       *
       * AH = 04h
       * CF clear to avoid bug
       * Return:
       * CF clear if successful
       * CH = century (BCD)
       * CL = year (BCD)
       * DH = month (BCD)
       * DL = day (BCD)
       * CF set on error
       */
      Regs.b.ah = 0x04;
      Int386(0x1A, &Regs, &Regs);

      if (NULL != Year)
        {
          *Year = 100 * BCD_INT(Regs.b.ch) + BCD_INT(Regs.b.cl);
        }
      if (NULL != Month)
        {
          *Month = BCD_INT(Regs.b.dh);
        }
      if (NULL != Day)
        {
          *Day = BCD_INT(Regs.b.dl);
        }
    }

  if (NULL != Hour || NULL != Minute || NULL != Second)
    {
      /* Some BIOSes leave CF unchanged if successful,
       * so CF should be cleared before calling this function. */
      __asm__ ("clc");

      /* Int 1Ah AH=02h
       * TIME - GET REAL-TIME CLOCK TIME (AT,XT286,PS)
       *
       * AH = 02h
       * CF clear to avoid bug
       * Return:
       * CF clear if successful
       * CH = hour (BCD)
       * CL = minutes (BCD)
       * DH = seconds (BCD)
       * DL = daylight savings flag (00h standard time, 01h daylight time)
       * CF set on error (i.e. clock not running or in middle of update)
       */
      Regs.b.ah = 0x02;
      Int386(0x1A, &Regs, &Regs);

      if (NULL != Hour)
        {
          *Hour = BCD_INT(Regs.b.ch);
        }
      if (NULL != Minute)
        {
          *Minute = BCD_INT(Regs.b.cl);
        }
      if (NULL != Second)
        {
          *Second = BCD_INT(Regs.b.dh);
        }
    }
}

/* EOF */
