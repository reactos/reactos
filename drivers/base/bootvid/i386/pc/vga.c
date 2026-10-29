/*
 * PROJECT:     ReactOS Boot Video Driver for VGA-compatible cards
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     VGA helper functions
 * COPYRIGHT:   Copyright 2007 Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright 2013 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2019 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *              Copyright 2020 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"

/* GLOBALS *******************************************************************/

static const UCHAR lMaskTable[8] =
{
    (1 << 8) - (1 << 0),
    (1 << 7) - (1 << 0),
    (1 << 6) - (1 << 0),
    (1 << 5) - (1 << 0),
    (1 << 4) - (1 << 0),
    (1 << 3) - (1 << 0),
    (1 << 2) - (1 << 0),
    (1 << 1) - (1 << 0)
};
static const UCHAR rMaskTable[8] =
{
    (1 << 7),
    (1 << 7) + (1 << 6),
    (1 << 7) + (1 << 6) + (1 << 5),
    (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4),
    (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3),
    (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2),
    (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1),
    (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0),
};
const UCHAR PixelMask[8] =
{
    (1 << 7),
    (1 << 6),
    (1 << 5),
    (1 << 4),
    (1 << 3),
    (1 << 2),
    (1 << 1),
    (1 << 0),
};
static const ULONG ReverseAndUnpackMonochromePixels[16] =
{
    0x0000,
    0x0100,
    0x1000,
    0x1100,
    0x0001,
    0x0101,
    0x1001,
    0x1101,
    0x0010,
    0x0110,
    0x1010,
    0x1110,
    0x0011,
    0x0111,
    0x1011,
    0x1111,
};

ULONG_PTR VgaRegisterBase = 0;
ULONG_PTR VgaBase = 0;

/* PRIVATE FUNCTIONS *********************************************************/

static VOID
ReadWriteMode(
    _In_ UCHAR Mode)
{
    UCHAR Value;

    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_GRAPH_MODE);

    Value = __inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT);
    Value = (Value & ~0x0B) | Mode;
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, Value);
}

static VOID
PrepareForCopyArea(VOID)
{
    /* Enable all memory planes */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (0x0F << 8) | IND_MAP_MASK);

    /* Set the bit mask to access all pixels in byte */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (0xFF << 8) | IND_BIT_MASK);

    /* Select read mode 0 and write mode 1 */
    ReadWriteMode(1);
}

VOID
PrepareForSetPixel(VOID)
{
    /* Select read mode 1 and write mode 2 */
    ReadWriteMode(0xA);

    /* Enable all memory planes */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (0x0F << 8) | IND_MAP_MASK);

    /* Clear the color don't care register, so read mode 1 reads always return 0xFF */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, 7);
}

#define SET_PIXELS(_PixelPtr, _PixelMask, _TextColor)       \
do {                                                        \
    /* Select the pixels to modify in VGA memory */         \
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, ((_PixelMask) << 8) | IND_BIT_MASK); \
    /* Dummy read to load latch registers */                \
    (VOID)READ_REGISTER_UCHAR((_PixelPtr));                 \
    /* Set the new color */                                 \
    WRITE_REGISTER_UCHAR((_PixelPtr), (UCHAR)(_TextColor)); \
} while (0);

VOID
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    const UCHAR* FontChar;
    PUCHAR PixelPtr;
    ULONG Height;
    UCHAR Shift;

    PrepareForSetPixel();

    /* Calculate shift */
    Shift = Left & 7;

    /* Get the font and pixel pointer */
    FontChar = GetFontPtr(Character);
    PixelPtr = (PUCHAR)(VgaBase + (Left >> 3) + (Top * (SCREEN_WIDTH / 8)));

    /* Loop all pixel rows */
    for (Height = BOOTCHAR_HEIGHT; Height > 0; --Height)
    {
        SET_PIXELS(PixelPtr, *FontChar >> Shift, TextColor);
        PixelPtr += (SCREEN_WIDTH / 8);
        FontChar += FONT_PTR_DELTA;
    }

    /* Check if we need to update neighbor bytes */
    if (Shift)
    {
        /* Calculate shift for 2nd byte */
        Shift = 8 - Shift;

        /* Get the font and pixel pointer (2nd byte) */
        FontChar = GetFontPtr(Character);
        PixelPtr = (PUCHAR)(VgaBase + (Left >> 3) + (Top * (SCREEN_WIDTH / 8)) + 1);

        /* Loop all pixel rows */
        for (Height = BOOTCHAR_HEIGHT; Height > 0; --Height)
        {
            SET_PIXELS(PixelPtr, *FontChar << Shift, TextColor);
            PixelPtr += (SCREEN_WIDTH / 8);
            FontChar += FONT_PTR_DELTA;
        }
    }

    /* Check if the background color is transparent */
    if (BackColor >= BV_COLOR_NONE)
    {
        /* We are done */
        return;
    }

    /* Calculate shift */
    Shift = Left & 7;

    /* Get the font and pixel pointer */
    FontChar = GetFontPtr(Character);
    PixelPtr = (PUCHAR)(VgaBase + (Left >> 3) + (Top * (SCREEN_WIDTH / 8)));

    /* Loop all pixel rows */
    for (Height = BOOTCHAR_HEIGHT; Height > 0; --Height)
    {
        SET_PIXELS(PixelPtr, ~*FontChar >> Shift, BackColor);
        PixelPtr += (SCREEN_WIDTH / 8);
        FontChar += FONT_PTR_DELTA;
    }

    /* Check if we need to update neighbor bytes */
    if (Shift)
    {
        /* Calculate shift for 2nd byte */
        Shift = 8 - Shift;

        /* Get the font and pixel pointer (2nd byte) */
        FontChar = GetFontPtr(Character);
        PixelPtr = (PUCHAR)(VgaBase + (Left >> 3) + (Top * (SCREEN_WIDTH / 8)) + 1);

        /* Loop all pixel rows */
        for (Height = BOOTCHAR_HEIGHT; Height > 0; --Height)
        {
            SET_PIXELS(PixelPtr, ~*FontChar << Shift, BackColor);
            PixelPtr += (SCREEN_WIDTH / 8);
            FontChar += FONT_PTR_DELTA;
        }
    }
}

static VOID
SetPaletteEntryRGB(
    _In_ ULONG Id,
    _In_ RGBQUAD Rgb)
{
    /* Set the palette index */
    __outpb(VGA_BASE_IO_PORT + DAC_ADDRESS_WRITE_PORT, (UCHAR)Id);

    /* Set RGB colors */
    __outpb(VGA_BASE_IO_PORT + DAC_DATA_REG_PORT, GetRValue(Rgb) >> 2);
    __outpb(VGA_BASE_IO_PORT + DAC_DATA_REG_PORT, GetGValue(Rgb) >> 2);
    __outpb(VGA_BASE_IO_PORT + DAC_DATA_REG_PORT, GetBValue(Rgb) >> 2);
}

VOID
InitPaletteWithTable(
    _In_reads_(Count) const ULONG* Table,
    _In_ ULONG Count)
{
    const ULONG* Entry = Table;
    ULONG i;

    for (i = 0; i < Count; i++, Entry++)
    {
        SetPaletteEntryRGB(i, *Entry);
    }
}

VOID
DoScroll(
    _In_ ULONG Scroll)
{
    ULONG Top, RowSize;
    PUCHAR OldPosition, NewPosition;

    PrepareForCopyArea();

    RowSize = (VidpScrollRegion.Right - VidpScrollRegion.Left + 1) / 8;

    OldPosition = (PUCHAR)(VgaBase + (VidpScrollRegion.Top + Scroll) * (SCREEN_WIDTH / 8) + VidpScrollRegion.Left / 8);
    NewPosition = (PUCHAR)(VgaBase + VidpScrollRegion.Top * (SCREEN_WIDTH / 8) + VidpScrollRegion.Left / 8);

    for (Top = VidpScrollRegion.Top; Top <= VidpScrollRegion.Bottom; ++Top)
    {
#if defined(_M_IX86) || defined(_M_AMD64)
        __movsb(NewPosition, OldPosition, RowSize);
#else
        ULONG i;

        /* Scroll the row */
        for (i = 0; i < RowSize; ++i)
            WRITE_REGISTER_UCHAR(NewPosition + i, READ_REGISTER_UCHAR(OldPosition + i));
#endif
        OldPosition += (SCREEN_WIDTH / 8);
        NewPosition += (SCREEN_WIDTH / 8);
    }
}

VOID
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG Height,
    _In_ BOOLEAN Restore)
{
    PUCHAR OldPosition, NewPosition;
    ULONG Count;

    PrepareForCopyArea();

    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        OldPosition = (PUCHAR)(VgaBase + SCREEN_HEIGHT * (SCREEN_WIDTH / 8));
        NewPosition = (PUCHAR)(VgaBase + CurrentTop * (SCREEN_WIDTH / 8));
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        OldPosition = (PUCHAR)(VgaBase + CurrentTop * (SCREEN_WIDTH / 8));
        NewPosition = (PUCHAR)(VgaBase + SCREEN_HEIGHT * (SCREEN_WIDTH / 8));
    }

    Count = Height * (SCREEN_WIDTH / 8);
#if defined(_M_IX86) || defined(_M_AMD64)
    __movsb(NewPosition, OldPosition, Count);
#else
    while (Count--)
    {
        WRITE_REGISTER_UCHAR(NewPosition++, READ_REGISTER_UCHAR(OldPosition++));
    }
#endif
}

/* PUBLIC FUNCTIONS **********************************************************/

VOID
NTAPI
VidCleanUp(VOID)
{
    /* Restore the bit mask to its original state */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
}

VOID
NTAPI
VidScreenToBufferBlt(
    _Out_writes_bytes_all_(Height * Stride) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Stride)
{
    ULONG LineStart, LineEnd, LineWidth;
    ULONG LeftShift, RightShift;
    PUCHAR PixelPtr;
    ULONG Plane, X, Y;

    if ((Height == 0) || (Stride == 0))
        return;

    /* Clear the destination buffer */
    RtlZeroMemory(Buffer, Height * Stride);

    if (Width == 0)
        return;

    /* Calculate the start/final and number of horizontal 8-pixel blocks to copy.
     * The VGA bootvid only copies whole 8-pixel blocks by design. */
    LineStart = Left / 8;
    LineEnd = (Left + Width - 1) / 8;
    if (LineStart > LineEnd)
        return;
    LineWidth = (LineEnd - LineStart) + 1;

    // if (LineWidth * 4 > Stride) return; // FIX #1 for "not-ideal case"
    LineWidth = min(LineWidth, Stride / 4); // FIX #2 for "not-ideal case", see https://github.com/reactos/reactos/pull/8968#discussion_r4417396304

    /* This check helps the compiler to compile the inner loop as a do-while loop */
    if (LineWidth == 0)
        return;

    /* Calculate the 8-bit left and right shifts */
    LeftShift = Left & 7;
    RightShift = 8 - LeftShift;

    /* Calculate the starting pixel block pointer */
    PixelPtr = (PUCHAR)(VgaBase + Top * (SCREEN_WIDTH / 8) + (Left / 8));

    /* Select read mode 0 */
    ReadWriteMode(0);

    /* Loop through the pixels for each plane */
    for (Plane = 0; Plane < 4; ++Plane)
    {
        PUCHAR PixelPosition = PixelPtr;
        PUCHAR OutputBuffer = Buffer;

        /* Select the current plane to read the pixels */
        __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (Plane << 8) | IND_READ_MAP);

        for (Y = 0; Y < Height; ++Y)
        {
            PULONG Destination = (PULONG)OutputBuffer;
            PUCHAR NextPosition;
            UCHAR CurrPixels;

            CurrPixels = READ_REGISTER_UCHAR(PixelPosition);
            NextPosition = PixelPosition;

            for (X = 0; X < LineWidth; ++X)
            {
                UCHAR NextPixels, Px;
                ULONG Result;

                NextPixels = READ_REGISTER_UCHAR(++NextPosition);

                /* Store 8 pixels at once */
                Px = CurrPixels << LeftShift;
                Px |= NextPixels >> RightShift;
                Result = ReverseAndUnpackMonochromePixels[Px >> 4];
                Result |= ReverseAndUnpackMonochromePixels[Px & 0x0F] << 16;

                *Destination++ |= Result << Plane;

                CurrPixels = NextPixels;
            }

            PixelPosition += (SCREEN_WIDTH / 8);
            OutputBuffer += Stride;
        }
    }
}

VOID
NTAPI
VidSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR Color)
{
    ULONG rMask, lMask;
    ULONG LeftOffset, RightOffset, Distance;
    PUCHAR Offset;
    ULONG i, j;

    /* Get the left and right masks, shifts, and delta */
    LeftOffset = Left >> 3;
    lMask = (lMaskTable[Left & 0x7] << 8) | IND_BIT_MASK;
    RightOffset = Right >> 3;
    rMask = (rMaskTable[Right & 0x7] << 8) | IND_BIT_MASK;
    Distance = RightOffset - LeftOffset;

    /* If there is no distance, then combine the right and left masks */
    if (!Distance) lMask &= rMask;

    PrepareForSetPixel();

    /* Calculate pixel position for the read */
    Offset = (PUCHAR)(VgaBase + (Top * (SCREEN_WIDTH / 8)) + LeftOffset);

    /* Select the bitmask register and write the mask */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (USHORT)lMask);

    /* Check if the top coord is below the bottom one */
    if (Top <= Bottom)
    {
        /* Start looping each line */
        for (i = (Bottom - Top) + 1; i > 0; --i)
        {
            /* Read the previous value and add our color */
            WRITE_REGISTER_UCHAR(Offset, READ_REGISTER_UCHAR(Offset) & Color);

            /* Move to the next line */
            Offset += (SCREEN_WIDTH / 8);
        }
    }

    /* Check if we have a delta */
    if (Distance > 0)
    {
        /* Calculate new pixel position */
        Offset = (PUCHAR)(VgaBase + (Top * (SCREEN_WIDTH / 8)) + RightOffset);
        Distance--;

        /* Select the bitmask register and write the mask */
        __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (USHORT)rMask);

        /* Check if the top coord is below the bottom one */
        if (Top <= Bottom)
        {
            /* Start looping each line */
            for (i = (Bottom - Top) + 1; i > 0; --i)
            {
                /* Read the previous value and add our color */
                WRITE_REGISTER_UCHAR(Offset, READ_REGISTER_UCHAR(Offset) & Color);

                /* Move to the next line */
                Offset += (SCREEN_WIDTH / 8);
            }
        }

        /* Check if we still have a delta */
        if (Distance > 0)
        {
            /* Calculate new pixel position */
            Offset = (PUCHAR)(VgaBase + (Top * (SCREEN_WIDTH / 8)) + LeftOffset + 1);

            /* Set the bit mask to access all pixels in byte */
            __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (0xFF << 8) | IND_BIT_MASK);

            /* Check if the top coord is below the bottom one */
            if (Top <= Bottom)
            {
                /* Start looping each line */
                for (i = (Bottom - Top) + 1; i > 0; --i)
                {
                    /* Loop the shift delta */
                    for (j = Distance; j > 0; Offset++, --j)
                    {
                        /* Write the color */
                        WRITE_REGISTER_UCHAR(Offset, Color);
                    }

                    /* Update position in memory */
                    Offset += ((SCREEN_WIDTH / 8) - Distance);
                }
            }
        }
    }
}
