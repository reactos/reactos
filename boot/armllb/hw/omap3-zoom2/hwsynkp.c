/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-zoom2/hwsynkp.c
 * PURPOSE:         LLB Synpatics Keypad Support for OMAP3 ZOOM 2
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
LlbHwOmap3SynKpdInitialize(VOID)
{
    /* Set GPIO pin 8 on the TWL4030 as an output pin */
    LlbHwOmap3TwlWrite1(0x49, 0x9B, 0xC0);

    /* Set GPIO pin 8 signal on the TWL4030 ON. This powers the keypad backlight */
    LlbHwOmap3TwlWrite1(0x49, 0xA4, 0xC0);

    /* Set PENDDIS and COR on the the keypad interrupt controller */
    LlbHwOmap3TwlWrite1(0x4A, 0xE9, 0x06);

    /* Only falling edge detection for key pressed */
    LlbHwOmap3TwlWrite1(0x4A, 0xE8, 0x01);

    /* Unmask key-pressed events */
    LlbHwOmap3TwlWrite1(0x4A, 0xE4, 0x0E);

    /* Set the keypad control register to turn hardware sequencing and turn it on */
    LlbHwOmap3TwlWrite1(0x4A, 0xD2, 0x0);
    LlbHwOmap3TwlRead1(0x4A, 0xE3);
    LlbHwOmap3TwlWrite1(0x4A, 0xD2, 0x43);
}

UCHAR KeyboardMatrixStatus[8];
BOOLEAN LastState = FALSE;

BOOLEAN
NTAPI
LlbHwKbdReady(VOID)
{
    UCHAR Value;

    Value = LlbHwOmap3TwlRead1(0x4A, 0xE3);
    if (!Value) return FALSE;

    LastState ^= 1;
    if (!LastState) return FALSE;

    /* Return whether or not an interrupt is pending */
    return TRUE;
}

INT
NTAPI
LlbHwKbdRead(VOID)
{
    UCHAR ActiveCol = 0, ActiveRow = 0, col, coldata, row;

    for (col = 0; col < 8; col++)
    {
        coldata = LlbHwOmap3TwlRead1(0x4A, 0xDB + col);
        if (coldata)
        {
            for (row = 0; row < 8; row++)
            {
                if (coldata == (1 << row))
                {
                    ActiveRow = row;
                    ActiveCol = col;
                    break;
                }
            }
        }
    }

    return ((ActiveCol << 4) | ActiveRow);
}

/* EOF */
