#include "precomp.h"

/* GLOBALS *******************************************************************/

static UCHAR lMaskTable[8] =
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
static UCHAR rMaskTable[8] =
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
UCHAR PixelMask[8] =
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
static ULONG lookup[16] =
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
NTAPI
ReadWriteMode(
    _In_ UCHAR Mode)
{
    UCHAR Value;

    /* Switch to graphics mode register */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_GRAPH_MODE);

    /* Get the current register value, minus the current mode */
    Value = __inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) & 0xF4;

    /* Set the new mode */
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, Mode | Value);
}

VOID
PrepareForSetPixel(VOID)
{
    /* Switch to mode 10 */
    ReadWriteMode(10);

    /* Clear the 4 planes (we're already in unchained mode here) */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x0F02);

    /* Select the color don't care register */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, 7);
}

#define SET_PIXELS(_PixelPtr, _PixelMask, _TextColor)       \
do {                                                        \
    /* Select the bitmask register and write the mask */    \
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, ((_PixelMask) << 8) | IND_BIT_MASK); \
    /* Dummy read to load latch registers */                \
    (VOID)READ_REGISTER_UCHAR((_PixelPtr));                 \
    /* Set the new color */                                 \
    WRITE_REGISTER_UCHAR((_PixelPtr), (UCHAR)(_TextColor)); \
} while (0);

VOID
NTAPI
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    PUCHAR FontChar, PixelPtr;
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
NTAPI
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
NTAPI
InitPaletteWithTable(
    _In_ PULONG Table,
    _In_ ULONG Count)
{
    ULONG i;
    PULONG Entry = Table;

    for (i = 0; i < Count; i++, Entry++)
    {
        SetPaletteEntryRGB(i, *Entry);
    }
}

VOID
NTAPI
DoScroll(
    _In_ ULONG Scroll)
{
    ULONG Top, RowSize;
    PUCHAR OldPosition, NewPosition;

    /* Clear the 4 planes */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x0F02);

    /* Set the bitmask to 0xFF for all 4 planes */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, 0xFF08);

    /* Set Mode 1 */
    ReadWriteMode(1);

    RowSize = (VidpScrollRegion[2] - VidpScrollRegion[0] + 1) / 8;

    /* Calculate the position in memory for the row */
    OldPosition = (PUCHAR)(VgaBase + (VidpScrollRegion[1] + Scroll) * (SCREEN_WIDTH / 8) + VidpScrollRegion[0] / 8);
    NewPosition = (PUCHAR)(VgaBase + VidpScrollRegion[1] * (SCREEN_WIDTH / 8) + VidpScrollRegion[0] / 8);

    /* Start loop */
    for (Top = VidpScrollRegion[1]; Top <= VidpScrollRegion[3]; ++Top)
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
NTAPI
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore)
{
    PUCHAR Position1, Position2;
    ULONG Count;

    /* Clear the 4 planes */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x0F02);

    /* Set the bitmask to 0xFF for all 4 planes */
    __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, 0xFF08);

    /* Set Mode 1 */
    ReadWriteMode(1);

    /* Calculate the position in memory for the row */
    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        Position1 = (PUCHAR)(VgaBase + CurrentTop * (SCREEN_WIDTH / 8));
        Position2 = (PUCHAR)(VgaBase + SCREEN_HEIGHT * (SCREEN_WIDTH / 8));
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        Position1 = (PUCHAR)(VgaBase + SCREEN_HEIGHT * (SCREEN_WIDTH / 8));
        Position2 = (PUCHAR)(VgaBase + CurrentTop * (SCREEN_WIDTH / 8));
    }

    /* Set the count and loop every pixel */
    Count = TopDelta * (SCREEN_WIDTH / 8);
#if defined(_M_IX86) || defined(_M_AMD64)
    __movsb(Position1, Position2, Count);
#else
    while (Count--)
    {
        /* Write the data back on the other position */
        WRITE_REGISTER_UCHAR(Position1, READ_REGISTER_UCHAR(Position2));

        /* Increase both positions */
        Position1++;
        Position2++;
    }
#endif
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
VidCleanUp(VOID)
{
    /* Select bit mask register and clear it */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
}

/*
 * @implemented
 */
VOID
NTAPI
VidScreenToBufferBlt(
    _Out_writes_bytes_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    ULONG Plane;
    ULONG XDistance;
    ULONG LeftDelta, RightDelta;
    ULONG PixelOffset;
    PUCHAR PixelPosition;
    PUCHAR k, i;
    PULONG m;
    UCHAR Value, Value2;
    UCHAR a;
    ULONG b;
    ULONG x, y;

    /* Calculate total distance to copy on X */
    XDistance = Left + Width - 1;

    /* Calculate the 8-byte left and right deltas */
    LeftDelta = Left & 7;
    RightDelta = 8 - LeftDelta;

    /* Clear the destination buffer */
    RtlZeroMemory(Buffer, Delta * Height);

    /* Calculate the pixel offset and convert the X distance into byte form */
    PixelOffset = Top * (SCREEN_WIDTH / 8) + (Left >> 3);
    XDistance >>= 3;

    /* Loop the 4 planes */
    for (Plane = 0; Plane < 4; ++Plane)
    {
        /* Set the current pixel position and reset buffer loop variable */
        PixelPosition = (PUCHAR)(VgaBase + PixelOffset);
        i = Buffer;

        /* Set Mode 0 */
        ReadWriteMode(0);

        /* Set the current plane */
        __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, (Plane << 8) | IND_READ_MAP);

        /* Start the outer Y height loop */
        for (y = Height; y > 0; --y)
        {
            /* Read the current value */
            m = (PULONG)i;
            Value = READ_REGISTER_UCHAR(PixelPosition);

            /* Set Pixel Position loop variable */
            k = PixelPosition + 1;

            /* Check if we're still within bounds */
            if (Left <= XDistance)
            {
                /* Start the X inner loop */
                for (x = (XDistance - Left) + 1; x > 0; --x)
                {
                    /* Read the current value */
                    Value2 = READ_REGISTER_UCHAR(k);

                    /* Increase pixel position */
                    k++;

                    /* Do the blt */
                    a = Value2 >> (UCHAR)RightDelta;
                    a |= Value << (UCHAR)LeftDelta;
                    b = lookup[a & 0xF];
                    a >>= 4;
                    b <<= 16;
                    b |= lookup[a];

                    /* Save new value to buffer */
                    *m |= (b << Plane);

                    /* Move to next destination location */
                    m++;

                    /* Write new value */
                    Value = Value2;
                }
            }

            /* Update pixel position */
            PixelPosition += (SCREEN_WIDTH / 8);
            i += Delta;
        }
    }
}

/*
 * @implemented
 */
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

            /* Set the bitmask to 0xFF for all 4 planes */
            __outpw(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, 0xFF08);

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
