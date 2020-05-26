/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Console routines for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

extern ULONG VramText;
extern UCHAR TextCols;
extern UCHAR TextLines;

/* GLOBALS ********************************************************************/

#define TEXT_CHAR_SIZE 2

static USHORT CursorPosition = 0;

/* FUNCTIONS ******************************************************************/

VOID
Pc98ConsPutChar(int Ch)
{
    /* If scrolling is needed */
    if (CursorPosition >= TextCols * TextLines)
    {
        RtlCopyMemory((PUSHORT)VramText,
                      (PUSHORT)(VramText + TextCols * TEXT_CHAR_SIZE),
                      TextCols * TextLines * TEXT_CHAR_SIZE);

        CursorPosition -= TextCols;
    }

    if (Ch == '\n')
    {
        if (CursorPosition % TextCols != 0)
            CursorPosition += TextCols - (CursorPosition % TextCols);

        return;
    }

    if (Ch == '\t')
    {
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');
        Pc98ConsPutChar(' ');

        return;
    }

    *(PUSHORT)(VramText + CursorPosition * TEXT_CHAR_SIZE) = Ch;
    ++CursorPosition;
}

BOOLEAN
Pc98ConsKbHit(VOID)
{
    REGS Regs;

    /* Int 18h AH=01h
     * KEYBOARD - CHECK FOR KEYSTROKE
     *
     * Return:
     * BH - status
     *    00h - if no keystroke available
     *    01h - if keystroke available
     * AH - BIOS scan code
     * AL - ASCII character
     */
    Regs.b.ah = 0x01;
    Int386(0x18, &Regs, &Regs);

    return Regs.b.bh == 1;
}

int
Pc98ConsGetCh(VOID)
{
    static BOOLEAN ExtendedKey = FALSE;
    static UCHAR ExtendedScanCode = 0;
    REGS Regs;

    /*
     * If the last time we were called an
     * extended key was pressed then return
     * that keys scan code.
     */
    if (ExtendedKey)
    {
        ExtendedKey = FALSE;

        return ExtendedScanCode;
    }

    /* Int 18h AH=00h
     * KEYBOARD - GET KEYSTROKE
     *
     * Return:
     * AH - BIOS scan code
     * AL - ASCII character
     */
    Regs.b.ah = 0x00;
    Int386(0x18, &Regs, &Regs);

    /* Check for an extended keystroke */
    if (Regs.b.al == 0)
    {
        ExtendedKey = TRUE;
        ExtendedScanCode = Regs.b.ah;
    }

    /* Return keystroke */
    return Regs.b.al;
}
