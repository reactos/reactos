/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific beep routine
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer (brianp@reactos.org)
 */

#include <freeldr.h>

#if defined(SARCH_XBOX)
#define CLOCK_TICK_RATE 1125000
#else
#define CLOCK_TICK_RATE 1193182
#endif

static VOID
Sound(USHORT Frequency)
{
    USHORT Scale;

    if (Frequency == 0)
    {
        WRITE_PORT_UCHAR((PUCHAR)0x61, READ_PORT_UCHAR((PUCHAR)0x61) & ~3);
        return;
    }

    Scale = CLOCK_TICK_RATE / Frequency;
    WRITE_PORT_UCHAR((PUCHAR)0x43, 0xB6);
    WRITE_PORT_UCHAR((PUCHAR)0x42, Scale & 0xFF);
    WRITE_PORT_UCHAR((PUCHAR)0x42, Scale >> 8);
    WRITE_PORT_UCHAR((PUCHAR)0x61, READ_PORT_UCHAR((PUCHAR)0x61) | 3);
}

VOID PcBeep(VOID)
{
    Sound(700);
    StallExecutionProcessor(100000);
    Sound(0);
}
