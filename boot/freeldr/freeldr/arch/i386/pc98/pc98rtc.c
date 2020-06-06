/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Real-time clock access routine for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#include <freeldr.h>

#define BCD_INT(bcd) (((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F))

TIMEINFO*
Pc98GetTime(VOID)
{
    static TIMEINFO TimeInfo;
    REGS Regs;
    UCHAR SysTime[6];

    /* Int 1Ch AH=00h
     * TIMER BIOS - Read system time
     *
     * Call with:
     * ES:BX -> data buffer
     */
    Regs.b.ah = 0x00;
    Regs.w.es = ((ULONG_PTR)SysTime) >> 4;
    Regs.w.bx = ((ULONG_PTR)SysTime) & 0x0F;
    Int386(0x1C, &Regs, &Regs);

    TimeInfo.Year = BCD_INT(SysTime[0]);
    TimeInfo.Month = BCD_INT(SysTime[1] >> 4);
    TimeInfo.Day = BCD_INT(SysTime[2]);
    TimeInfo.Hour = BCD_INT(SysTime[3]);
    TimeInfo.Minute = BCD_INT(SysTime[4]);
    TimeInfo.Second = BCD_INT(SysTime[5]);
    if (TimeInfo.Year >= 80)
        TimeInfo.Year += 1900;
    else
        TimeInfo.Year += 2000;

    return &TimeInfo;
}
