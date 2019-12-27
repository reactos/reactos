/*
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

#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

TIMEINFO*
PcGetTime(VOID)
{
    static TIMEINFO TimeInfo;
    REGS Regs;

    for (;;)
    {
        /* Some BIOSes, such as the 1998/07/25 system ROM
         * in the Compaq Deskpro EP/SB, leave CF unchanged
         * if successful, so CF should be cleared before
         * calling this function. */
        Regs.x.eflags = 0;
//        __writeeflags(__readeflags() & ~EFLAGS_CF);

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

        if (!INT386_SUCCESS(Regs)) continue;

        TimeInfo.Year = 100 * BCD_INT(Regs.b.ch) + BCD_INT(Regs.b.cl);
        TimeInfo.Month = BCD_INT(Regs.b.dh);
        TimeInfo.Day = BCD_INT(Regs.b.dl);

        /* Some BIOSes leave CF unchanged if successful,
         * so CF should be cleared before calling this function. */
        Regs.x.eflags = 0;
//        __writeeflags(__readeflags() & ~EFLAGS_CF);

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

        if (!INT386_SUCCESS(Regs)) continue;

        TimeInfo.Hour = BCD_INT(Regs.b.ch);
        TimeInfo.Minute = BCD_INT(Regs.b.cl);
        TimeInfo.Second = BCD_INT(Regs.b.dh);

        break;
    }
    return &TimeInfo;
}

/* EOF */
