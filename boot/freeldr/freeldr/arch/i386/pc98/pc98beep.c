/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Beep routine for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#include <freeldr.h>

VOID Pc98Beep(VOID)
{
    REGS Regs;

    /* Int 18h AH=17h
     * CRT BIOS - Beep on
     */
    Regs.b.ah = 0x17;
    Int386(0x18, &Regs, &Regs);

    StallExecutionProcessor(100000);

    /* Int 18h AH=18h
     * CRT BIOS - Beep off
     */
    Regs.b.ah = 0x18;
    Int386(0x18, &Regs, &Regs);
}
