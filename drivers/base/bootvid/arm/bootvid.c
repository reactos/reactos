#include "precomp.h"

#define NDEBUG
#include <debug.h>

PUSHORT VgaArmBase;
PHYSICAL_ADDRESS VgaPhysical;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    PUCHAR FontChar;
    ULONG i, j, XOffset;

    /* Get the font line for this character */
    FontChar = &VidpFontData[Character * BOOTCHAR_HEIGHT - Top];

    /* Loop each pixel height */
    for (i = BOOTCHAR_HEIGHT; i > 0; --i)
    {
        /* Loop each pixel width */
        XOffset = Left;
        for (j = (1 << 7); j > 0; j >>= 1)
        {
            /* Check if we should draw this pixel */
            if (FontChar[Top] & (UCHAR)j)
            {
                /* We do, use the given Text Color */
                SetPixel(XOffset, Top, (UCHAR)TextColor);
            }
            else if (BackColor < BV_COLOR_NONE)
            {
                /*
                 * This is a background pixel. We're drawing it
                 * unless it's transparent.
                 */
                SetPixel(XOffset, Top, (UCHAR)BackColor);
            }

            /* Increase X Offset */
            XOffset++;
        }

        /* Move to the next Y ordinate */
        Top++;
    }
}

VOID
NTAPI
DoScroll(
    _In_ ULONG Scroll)
{
    ULONG Top, Offset;
    PUSHORT SourceOffset, DestOffset;
    PUSHORT i, j;

    /* Set memory positions of the scroll */
    SourceOffset = &VgaArmBase[(VidpScrollRegion[1] * (SCREEN_WIDTH / 8)) + (VidpScrollRegion[0] >> 3)];
    DestOffset = &SourceOffset[Scroll * (SCREEN_WIDTH / 8)];

    /* Start loop */
    for (Top = VidpScrollRegion[1]; Top <= VidpScrollRegion[3]; ++Top)
    {
        /* Set number of bytes to loop and start offset */
        Offset = VidpScrollRegion[0] >> 3;
        j = SourceOffset;

        /* Check if this is part of the scroll region */
        if (Offset <= (VidpScrollRegion[2] >> 3))
        {
            /* Update position */
            i = (PUSHORT)(DestOffset - SourceOffset);

            /* Loop the X axis */
            do
            {
                /* Write value in the new position so that we can do the scroll */
                WRITE_REGISTER_USHORT(j, READ_REGISTER_USHORT(j + (ULONG_PTR)i));

                /* Move to the next memory location to write to */
                j++;

                /* Move to the next byte in the region */
                Offset++;

                /* Make sure we don't go past the scroll region */
            } while (Offset <= (VidpScrollRegion[2] >> 3));
        }

        /* Move to the next line */
        SourceOffset += (SCREEN_WIDTH / 8);
        DestOffset += (SCREEN_WIDTH / 8);
    }
}

VOID
NTAPI
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore)
{
    PUSHORT Position1, Position2;
    ULONG Count;

    /* Calculate the position in memory for the row */
    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        Position1 = &VgaArmBase[CurrentTop * (SCREEN_WIDTH / 8)];
        Position2 = &VgaArmBase[SCREEN_HEIGHT * (SCREEN_WIDTH / 8)];
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        Position1 = &VgaArmBase[SCREEN_HEIGHT * (SCREEN_WIDTH / 8)];
        Position2 = &VgaArmBase[CurrentTop * (SCREEN_WIDTH / 8)];
    }

    /* Set the count and loop every pixel */
    Count = TopDelta * (SCREEN_WIDTH / 8);
    while (Count--)
    {
        /* Write the data back on the other position */
        WRITE_REGISTER_USHORT(Position1, READ_REGISTER_USHORT(Position2));

        /* Increase both positions */
        Position1++;
        Position2++;
    }
}

VOID
NTAPI
VidpInitializeDisplay(VOID)
{
    //
    // Set framebuffer address
    //
    WRITE_REGISTER_ULONG(PL110_LCDUPBASE, VgaPhysical.LowPart);
    WRITE_REGISTER_ULONG(PL110_LCDLPBASE, VgaPhysical.LowPart);

    //
    // Initialize timings to 640x480
    //
    WRITE_REGISTER_ULONG(PL110_LCDTIMING0, LCDTIMING0_PPL(SCREEN_WIDTH));
    WRITE_REGISTER_ULONG(PL110_LCDTIMING1, LCDTIMING1_LPP(SCREEN_HEIGHT));

    //
    // Enable the LCD Display
    //
    WRITE_REGISTER_ULONG(PL110_LCDCONTROL,
                         LCDCONTROL_LCDEN |
                         LCDCONTROL_LCDTFT |
                         LCDCONTROL_LCDPWR |
                         LCDCONTROL_LCDBPP(4));
}

VOID
NTAPI
InitPaletteWithTable(
    _In_ PULONG Table,
    _In_ ULONG Count)
{
    UNIMPLEMENTED;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    DPRINT1("bv-arm v0.1\n");

    //
    // Allocate framebuffer
    // 600kb works out to 640x480@16bpp
    //
    VgaPhysical.QuadPart = -1;
    VgaArmBase = MmAllocateContiguousMemory(600 * 1024, VgaPhysical);
    if (!VgaArmBase) return FALSE;

    //
    // Get physical address
    //
    VgaPhysical = MmGetPhysicalAddress(VgaArmBase);
    if (!VgaPhysical.QuadPart) return FALSE;
    DPRINT1("[BV-ARM] Frame Buffer @ 0x%p 0p%p\n", VgaArmBase, VgaPhysical.LowPart);

    //
    // Setup the display
    //
    VidpInitializeDisplay();

    //
    // We are done!
    //
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN HalReset)
{
    //
    // Clear the current position
    //
    VidpCurrentX = 0;
    VidpCurrentY = 0;

    //
    // Re-initialize the VGA Display
    //
    VidpInitializeDisplay();

    //
    // Re-initialize the palette and fill the screen black
    //
    InitializePalette();
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

/*
 * @implemented
 */
VOID
NTAPI
VidCleanUp(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
VidScreenToBufferBlt(
    _Out_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    UNIMPLEMENTED;
    while (TRUE);
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
    int y, x;

    //
    // Loop along the Y-axis
    //
    for (y = Top; y <= Bottom; y++)
    {
        //
        // Loop along the X-axis
        //
        for (x = Left; x <= Right; x++)
        {
            //
            // Draw the pixel
            //
            SetPixel(x, y, Color);
        }
    }
}
