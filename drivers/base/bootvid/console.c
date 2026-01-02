/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Platform-independent console functionality
 * COPYRIGHT:   Copyright 2010 Gregor Schneider <gregor.schneider@reactos.org>
 *              Copyright 2011 Rafal Harabien <rafalh@reactos.org>
 *              Copyright 2020 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"

/* GLOBALS ********************************************************************/

UCHAR VidpTextColor = BV_COLOR_WHITE;
ULONG VidpCurrentX = 0;
ULONG VidpCurrentY = 0;
URECT VidpScrollRegion = {0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};

static BOOLEAN ClearRow = FALSE;

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN SetMode)
{
    /* Clear the current position */
    VidpCurrentX = 0;
    VidpCurrentY = 0;

    /* Invoke the hardware-specific routine */
    ResetDisplay(SetMode);
}

ULONG
NTAPI
VidSetTextColor(
    _In_ ULONG Color)
{
    ULONG OldColor;

    /* Save the old color and set the new one */
    OldColor = VidpTextColor;
    VidpTextColor = Color;
    return OldColor;
}

VOID
NTAPI
VidSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom)
{
    /* Assert alignment */
    ASSERT((Left % BOOTCHAR_WIDTH) == 0);
    ASSERT((Right % BOOTCHAR_WIDTH) == BOOTCHAR_WIDTH - 1);

    /* Set the scroll region */
    VidpScrollRegion.Left = Left;
    VidpScrollRegion.Top  = Top;
    VidpScrollRegion.Right  = Right;
    VidpScrollRegion.Bottom = Bottom;

    /* Set the current X and Y */
    VidpCurrentX = Left;
    VidpCurrentY = Top;
}

VOID
NTAPI
VidDisplayStringXY(
    _In_ PCSTR String,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ BOOLEAN Transparent)
{
    ULONG BackColor;

    /*
     * If the caller wanted transparent, then send the special value (16),
     * else use our default and call the helper routine.
     */
    BackColor = Transparent ? BV_COLOR_NONE : BV_COLOR_LIGHT_CYAN;

    /* Loop every character and adjust the position */
    for (; *String; ++String, Left += BOOTCHAR_WIDTH)
    {
        /* Display a character */
        DisplayCharacter(*String, Left, Top, BV_COLOR_LIGHT_BLUE, BackColor);
    }
}

VOID
NTAPI
VidDisplayString(
    _In_ PCSTR String)
{
    /* Start looping the string */
    for (; *String; ++String)
    {
        /* Treat new-line separately */
        if (*String == '\n')
        {
            /* Modify Y position */
            VidpCurrentY += BOOTCHAR_HEIGHT + 1;
            if (VidpCurrentY + BOOTCHAR_HEIGHT > VidpScrollRegion.Bottom)
            {
                /* Scroll the view and clear the current row */
                DoScroll(BOOTCHAR_HEIGHT + 1);
                VidpCurrentY -= BOOTCHAR_HEIGHT + 1;
                PreserveRow(VidpCurrentY, BOOTCHAR_HEIGHT + 1, TRUE);
            }
            else
            {
                /* Preserve the current row */
                PreserveRow(VidpCurrentY, BOOTCHAR_HEIGHT + 1, FALSE);
            }

            /* Update current X */
            VidpCurrentX = VidpScrollRegion.Left;

            /* No need to clear this row */
            ClearRow = FALSE;
        }
        else if (*String == '\r')
        {
            /* Update current X */
            VidpCurrentX = VidpScrollRegion.Left;

            /* If a new-line does not follow we will clear the current row */
            if (String[1] != '\n')
                ClearRow = TRUE;
        }
        else
        {
            /* Clear the current row if we had a return-carriage without a new-line */
            if (ClearRow)
            {
                PreserveRow(VidpCurrentY, BOOTCHAR_HEIGHT + 1, TRUE);
                ClearRow = FALSE;
            }

            /* Display this character */
            DisplayCharacter(*String, VidpCurrentX, VidpCurrentY, VidpTextColor, BV_COLOR_NONE);
            VidpCurrentX += BOOTCHAR_WIDTH;

            /* Check if we should scroll */
            if (VidpCurrentX + BOOTCHAR_WIDTH - 1 > VidpScrollRegion.Right)
            {
                /* Update Y position and check if we should scroll it */
                VidpCurrentY += BOOTCHAR_HEIGHT + 1;
                if (VidpCurrentY + BOOTCHAR_HEIGHT > VidpScrollRegion.Bottom)
                {
                    /* Scroll the view and clear the current row */
                    DoScroll(BOOTCHAR_HEIGHT + 1);
                    VidpCurrentY -= BOOTCHAR_HEIGHT + 1;
                    PreserveRow(VidpCurrentY, BOOTCHAR_HEIGHT + 1, TRUE);
                }
                else
                {
                    /* Preserve the current row */
                    PreserveRow(VidpCurrentY, BOOTCHAR_HEIGHT + 1, FALSE);
                }

                /* Update current X */
                VidpCurrentX = VidpScrollRegion.Left;
            }
        }
    }
}
