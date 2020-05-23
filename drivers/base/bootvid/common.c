#include "precomp.h"

/* GLOBALS ********************************************************************/

UCHAR VidpTextColor = BV_COLOR_WHITE;

ULONG VidpCurrentX = 0;
ULONG VidpCurrentY = 0;

ULONG VidpScrollRegion[4] =
{
    0,
    0,
    SCREEN_WIDTH  - 1,
    SCREEN_HEIGHT - 1
};

/*
 * Boot video driver default palette is similar to the standard 16-color
 * CGA palette, but it has Red and Blue channels swapped, and also dark
 * and light gray colors swapped.
 */
const RGBQUAD VidpDefaultPalette[BV_MAX_COLORS] =
{
    RGB(  0,   0,   0), /* Black */
    RGB(128,   0,   0), /* Red */
    RGB(  0, 128,   0), /* Green */
    RGB(128, 128,   0), /* Brown */
    RGB(  0,   0, 128), /* Blue */
    RGB(128,   0, 128), /* Magenta */
    RGB(  0, 128, 128), /* Cyan */
    RGB(128, 128, 128), /* Dark Gray */
    RGB(192, 192, 192), /* Light Gray */
    RGB(255,   0,   0), /* Light Red */
    RGB(  0, 255,   0), /* Light Green */
    RGB(255, 255,   0), /* Yellow */
    RGB(  0,   0, 255), /* Light Blue */
    RGB(255,   0, 255), /* Light Magenta */
    RGB(  0, 255, 255), /* Light Cyan */
    RGB(255, 255, 255), /* White */
};

static BOOLEAN ClearRow = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID
NTAPI
BitBlt(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ PUCHAR Buffer,
    _In_ ULONG BitsPerPixel,
    _In_ ULONG Delta)
{
    ULONG sx, dx, dy;
    UCHAR color;
    ULONG offset = 0;
    const ULONG Bottom = Top + Height;
    const ULONG Right = Left + Width;

    /* Check if the buffer isn't 4bpp */
    if (BitsPerPixel != 4)
    {
        /* FIXME: TODO */
        DbgPrint("Unhandled BitBlt\n"
                 "%lux%lu @ (%lu|%lu)\n"
                 "Bits Per Pixel %lu\n"
                 "Buffer: %p. Delta: %lu\n",
                 Width,
                 Height,
                 Left,
                 Top,
                 BitsPerPixel,
                 Buffer,
                 Delta);
        return;
    }

    PrepareForSetPixel();

    /* 4bpp blitting */
    for (dy = Top; dy < Bottom; ++dy)
    {
        sx = 0;
        do
        {
            /* Extract color */
            color = Buffer[offset + sx];

            /* Calc destination x */
            dx = Left + (sx << 1);

            /* Set two pixels */
            SetPixel(dx, dy, color >> 4);
            SetPixel(dx + 1, dy, color & 0x0F);

            sx++;
        } while (dx < Right);
        offset += Delta;
    }
}

static VOID
NTAPI
RleBitBlt(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ PUCHAR Buffer)
{
    ULONG YDelta;
    ULONG x;
    ULONG RleValue, NewRleValue;
    ULONG Color, Color2;
    ULONG i, j;
    ULONG Code;

    PrepareForSetPixel();

    /* Set Y height and current X value and start loop */
    YDelta = Top + Height - 1;
    x = Left;
    for (;;)
    {
        /* Get the current value and advance in the buffer */
        RleValue = *Buffer;
        Buffer++;
        if (RleValue)
        {
            /* Check if we've gone past the edge */
            if ((x + RleValue) > (Width + Left))
            {
                /* Fixup the pixel value */
                RleValue = Left - x + Width;
            }

            /* Get the new value */
            NewRleValue = *Buffer;

            /* Get the two colors */
            Color = NewRleValue >> 4;
            Color2 = NewRleValue & 0xF;

            /* Increase buffer position */
            Buffer++;

            /* Check if we need to do a fill */
            if (Color == Color2)
            {
                /* Do a fill and continue the loop */
                RleValue += x;
                VidSolidColorFill(x, YDelta, RleValue - 1, YDelta, (UCHAR)Color);
                x = RleValue;
                continue;
            }

            /* Check if the pixel value is 1 or below */
            if (RleValue > 1)
            {
                /* Set loop variables */
                for (i = (RleValue - 2) / 2 + 1; i > 0; --i)
                {
                    /* Set the pixels */
                    SetPixel(x, YDelta, (UCHAR)Color);
                    x++;
                    SetPixel(x, YDelta, (UCHAR)Color2);
                    x++;

                    /* Decrease pixel value */
                    RleValue -= 2;
                }
            }

            /* Check if there is any value at all */
            if (RleValue)
            {
                /* Set the pixel and increase position */
                SetPixel(x, YDelta, (UCHAR)Color);
                x++;
            }

            /* Start over */
            continue;
        }

        /* Get the current pixel value */
        RleValue = *Buffer;
        Code = RleValue;
        switch (Code)
        {
            /* Case 0 */
            case 0:
            {
                /* Set new x value, decrease distance and restart */
                x = Left;
                YDelta--;
                Buffer++;
                continue;
            }

            /* Case 1 */
            case 1:
            {
                /* Done */
                return;
            }

            /* Case 2 */
            case 2:
            {
                /* Set new x value, decrease distance and restart */
                Buffer++;
                x += *Buffer;
                Buffer++;
                YDelta -= *Buffer;
                Buffer++;
                continue;
            }

            /* Other values */
            default:
            {
                Buffer++;
                break;
            }
        }

        /* Check if we've gone past the edge */
        if ((x + RleValue) > (Width + Left))
        {
            /* Set fixed up loop count */
            i = RleValue - Left - Width + x;

            /* Fixup pixel value */
            RleValue -= i;
        }
        else
        {
            /* Clear loop count */
            i = 0;
        }

        /* Check the value now */
        if (RleValue > 1)
        {
            /* Set loop variables */
            for (j = (RleValue - 2) / 2 + 1; j > 0; --j)
            {
                /* Get the new value */
                NewRleValue = *Buffer;

                /* Get the two colors */
                Color = NewRleValue >> 4;
                Color2 = NewRleValue & 0xF;

                /* Increase buffer position */
                Buffer++;

                /* Set the pixels */
                SetPixel(x, YDelta, (UCHAR)Color);
                x++;
                SetPixel(x, YDelta, (UCHAR)Color2);
                x++;

                /* Decrease pixel value */
                RleValue -= 2;
            }
        }

        /* Check if there is any value at all */
        if (RleValue)
        {
            /* Set the pixel and increase position */
            Color = *Buffer >> 4;
            Buffer++;
            SetPixel(x, YDelta, (UCHAR)Color);
            x++;
            i--;
        }

        /* Check loop count now */
        if ((LONG)i > 0)
        {
            /* Decrease it */
            i--;

            /* Set new position */
            Buffer = Buffer + (i / 2) + 1;
        }

        /* Check if we need to increase the buffer */
        if ((ULONG_PTR)Buffer & 1) Buffer++;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
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
VidDisplayStringXY(
    _In_ PUCHAR String,
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
VidSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom)
{
    /* Assert alignment */
    ASSERT((Left % BOOTCHAR_WIDTH) == 0);
    ASSERT((Right % BOOTCHAR_WIDTH) == BOOTCHAR_WIDTH - 1);

    /* Set Scroll Region */
    VidpScrollRegion[0] = Left;
    VidpScrollRegion[1] = Top;
    VidpScrollRegion[2] = Right;
    VidpScrollRegion[3] = Bottom;

    /* Set current X and Y */
    VidpCurrentX = Left;
    VidpCurrentY = Top;
}

/*
 * @implemented
 */
VOID
NTAPI
VidDisplayString(
    _In_ PUCHAR String)
{
    /* Start looping the string */
    for (; *String; ++String)
    {
        /* Treat new-line separately */
        if (*String == '\n')
        {
            /* Modify Y position */
            VidpCurrentY += BOOTCHAR_HEIGHT + 1;
            if (VidpCurrentY + BOOTCHAR_HEIGHT > VidpScrollRegion[3])
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
            VidpCurrentX = VidpScrollRegion[0];

            /* No need to clear this row */
            ClearRow = FALSE;
        }
        else if (*String == '\r')
        {
            /* Update current X */
            VidpCurrentX = VidpScrollRegion[0];

            /* If a new-line does not follow we will clear the current row */
            if (String[1] != '\n') ClearRow = TRUE;
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
            if (VidpCurrentX + BOOTCHAR_WIDTH - 1 > VidpScrollRegion[2])
            {
                /* Update Y position and check if we should scroll it */
                VidpCurrentY += BOOTCHAR_HEIGHT + 1;
                if (VidpCurrentY + BOOTCHAR_HEIGHT > VidpScrollRegion[3])
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
                VidpCurrentX = VidpScrollRegion[0];
            }
        }
    }
}

VOID
NTAPI
VidBufferToScreenBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    /* Make sure we have a width and height */
    if (!Width || !Height)
        return;

    /* Call the helper function */
    BitBlt(Left, Top, Width, Height, Buffer, 4, Delta);
}

VOID
NTAPI
VidBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top)
{
    PBITMAPINFOHEADER BitmapInfoHeader;
    LONG Delta;
    PUCHAR BitmapOffset;
    ULONG PaletteCount;

    /* Get the Bitmap Header */
    BitmapInfoHeader = (PBITMAPINFOHEADER)Buffer;

    /* Initialize the palette */
    PaletteCount = BitmapInfoHeader->biClrUsed ?
                   BitmapInfoHeader->biClrUsed : BV_MAX_COLORS;
    InitPaletteWithTable((PULONG)(Buffer + BitmapInfoHeader->biSize),
                         PaletteCount);

    /* Make sure we can support this bitmap */
    ASSERT((BitmapInfoHeader->biBitCount * BitmapInfoHeader->biPlanes) <= 4);

    /*
     * Calculate the delta and align it on 32-bytes, then calculate
     * the actual start of the bitmap data.
     */
    Delta = (BitmapInfoHeader->biBitCount * BitmapInfoHeader->biWidth) + 31;
    Delta >>= 3;
    Delta &= ~3;
    BitmapOffset = Buffer + sizeof(BITMAPINFOHEADER) + PaletteCount * sizeof(ULONG);

    /* Check the compression of the bitmap */
    if (BitmapInfoHeader->biCompression == BI_RLE4)
    {
        /* Make sure we have a width and a height */
        if ((BitmapInfoHeader->biWidth) && (BitmapInfoHeader->biHeight))
        {
            /* We can use RLE Bit Blt */
            RleBitBlt(Left,
                      Top,
                      BitmapInfoHeader->biWidth,
                      BitmapInfoHeader->biHeight,
                      BitmapOffset);
        }
    }
    else
    {
        /* Check if the height is negative */
        if (BitmapInfoHeader->biHeight < 0)
        {
            /* Make it positive in the header */
            BitmapInfoHeader->biHeight *= -1;
        }
        else
        {
            /* Update buffer offset */
            BitmapOffset += ((BitmapInfoHeader->biHeight - 1) * Delta);
            Delta *= -1;
        }

        /* Make sure we have a width and a height */
        if ((BitmapInfoHeader->biWidth) && (BitmapInfoHeader->biHeight))
        {
            /* Do the BitBlt */
            BitBlt(Left,
                   Top,
                   BitmapInfoHeader->biWidth,
                   BitmapInfoHeader->biHeight,
                   BitmapOffset,
                   BitmapInfoHeader->biBitCount,
                   Delta);
        }
    }
}
