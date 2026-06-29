/*
 * PROJECT:     ReactOS Boot Video Driver for VGA-compatible cards
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     VGA definitions
 * COPYRIGHT:   Copyright 2007 Alex Ionescu <alex.ionescu@reactos.org>
 */

#pragma once

#include "vga.h"

extern ULONG_PTR VgaRegisterBase;
extern ULONG_PTR VgaBase;
extern const USHORT AT_Initialization[];
extern const USHORT VGA_640x480[];
extern const UCHAR PixelMask[8];

#define __inpb(Port) \
    READ_PORT_UCHAR((PUCHAR)(VgaRegisterBase + (Port)))

#define __inpw(Port) \
    READ_PORT_USHORT((PUSHORT)(VgaRegisterBase + (Port)))

#define __outpb(Port, Value) \
    WRITE_PORT_UCHAR((PUCHAR)(VgaRegisterBase + (Port)), (UCHAR)(Value))

#define __outpw(Port, Value) \
    WRITE_PORT_USHORT((PUSHORT)(VgaRegisterBase + (Port)), (USHORT)(Value))

VOID
InitPaletteWithTable(
    _In_reads_(Count) const ULONG* Table,
    _In_ ULONG Count);

VOID
PrepareForSetPixel(VOID);

FORCEINLINE
VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color)
{
    PUCHAR PixelPosition;

    /* Calculate the pixel position */
    PixelPosition = (PUCHAR)(VgaBase + (Left >> 3) + (Top * (SCREEN_WIDTH / 8)));

    /* Select the bitmask register and write the mask */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (PixelMask[Left & 7] << 8) | IND_BIT_MASK);

    /* Dummy read to load latch registers */
    (VOID)READ_REGISTER_UCHAR(PixelPosition);

    /* Set the new color */
    WRITE_REGISTER_UCHAR(PixelPosition, Color);
}
