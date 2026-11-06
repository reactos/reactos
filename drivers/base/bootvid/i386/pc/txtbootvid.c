/*
 * PROJECT:     ReactOS Text-Mode Boot Video Driver for VGA-compatible cards
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include "roslogo2_ANSI.inc"
#include <reactos/buildno.h>
#include <reactos/version.h>

/* Enable this define for disabling extended features (colored text, region scrolling) */
#define MINIMAL_UI

/* GLOBALS *******************************************************************/

#ifndef MINIMAL_UI

/* Bitmap Header */
typedef struct tagBITMAPINFOHEADER
{
    ULONG biSize;
    LONG biWidth;
    LONG biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG biCompression;
    ULONG biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    ULONG biClrUsed;
    ULONG biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#define RGB(r, g, b)    ((RGBQUAD)(((UCHAR)(b) | ((USHORT)((UCHAR)(g))<<8)) | (((ULONG)(UCHAR)(r))<<16)))

#define GetRValue(quad)     ((UCHAR)(((quad)>>16) & 0xFF))
#define GetGValue(quad)     ((UCHAR)(((quad)>>8) & 0xFF))
#define GetBValue(quad)     ((UCHAR)((quad) & 0xFF))

typedef ULONG RGBQUAD;

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

#define InitializePalette() InitPaletteWithTable((PULONG)VidpDefaultPalette, BV_MAX_COLORS)

#endif // !MINIMAL_UI

#define TEXT_WIDTH  (SCREEN_WIDTH / BOOTCHAR_WIDTH)   // 80
#define TEXT_HEIGHT (SCREEN_HEIGHT / BOOTCHAR_HEIGHT) // 25

#define MEM_TEXT_VGA        0xB8000
#define MEM_TEXT_VGA_SIZE   (0xC0000 - 0xB8000)

ULONG_PTR VgaRegisterBase = 0;
ULONG_PTR VgaBase = 0;

#define ATTR(cFore, cBack)  ((cBack << 4) | cFore)

/* PRIVATE FUNCTIONS *********************************************************/

static BOOLEAN
VgaIsPresent(VOID)
{
    UCHAR OrgGCAddr, OrgReadMap, OrgBitMask;
    UCHAR OrgSCAddr, OrgMemMode;
    UCHAR i;

#if (NTDDI_VERSION >= NTDDI_WIN7)
    /* Check whether the current platform supports IO port access
     * and fail if not, since we won't be able to use VGA IO ports */
    if ((HALPRIVATEDISPATCH->Version >= 13) &&
        HalQueryIoPortAccessSupported && !HalQueryIoPortAccessSupported())
    {
        return FALSE;
    }
#endif

    /* Remember the original state of the Graphics Controller Address register */
    OrgGCAddr = __inpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT);

    /*
     * Write the Read Map register with a known state so we can verify
     * that it isn't changed after we fool with the Bit Mask. This ensures
     * that we're dealing with indexed registers, since both the Read Map and
     * the Bit Mask are addressed at GRAPH_DATA_PORT.
     */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);

    /*
     * If we can't read back the Graphics Address register setting we just
     * performed, it's not readable and this isn't a VGA.
     */
    if ((__inpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_READ_MAP)
        return FALSE;

    /*
     * Set the Read Map register to a known state.
     */
    OrgReadMap = __inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_TEST_SETTING);

    /* Read it back... it should be the same */
    if (__inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING)
    {
        /*
         * The Read Map setting we just performed can't be read back; not a
         * VGA. Restore the default Read Map state and fail.
         */
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        return FALSE;
    }

    /* Remember the original setting of the Bit Mask register */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);

    /* Read it back... it should be the same */
    if ((__inpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_BIT_MASK)
    {
        /*
         * The Graphics Address register setting we just made can't be read
         * back; not a VGA. Restore the default Read Map state and fail.
         */
        __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        return FALSE;
    }

    /* Read the VGA Data Register */
    OrgBitMask = __inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT);

    /*
     * Set up the initial test mask we'll write to and read from the Bit Mask,
     * and loop on the bitmasks.
     */
    for (i = 0xBB; i; i >>= 1)
    {
        /* Write the test mask to the Bit Mask */
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, i);

        /* Read it back... it should be the same */
        if (__inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) != i)
        {
            /*
             * The Bit Mask is not properly writable and readable; not a VGA.
             * Restore the Bit Mask and Read Map to their default states and fail.
             */
            __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
            __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);
            __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
            return FALSE;
        }
    }

    /*
     * There's something readable at GRAPH_DATA_PORT; now switch back and
     * make sure that the Read Map register hasn't changed, to verify that
     * we're dealing with indexed registers.
     */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);

    /* Read it back */
    if (__inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING)
    {
        /*
         * The Read Map is not properly writable and readable; not a VGA.
         * Restore the Bit Mask and Read Map to their default states, in case
         * this is an EGA, so subsequent writes to the screen aren't garbled.
         * Then fail.
         */
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
        return FALSE;
    }

    /*
     * We've pretty surely verified the existence of the Bit Mask register.
     * Put the Graphics Controller back to the original state.
     */
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, OrgReadMap);
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, OrgBitMask);
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, OrgGCAddr);

    /*
     * Now, check for the existence of the Chain4 bit.
     */

    /*
     * Remember the original states of the Sequencer Address and Memory Mode
     * registers.
     */
    OrgSCAddr = __inpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT);
    __outpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, IND_MEMORY_MODE);

    /* Read it back... it should be the same */
    if ((__inpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT) & SEQ_ADDR_MASK) != IND_MEMORY_MODE)
    {
        /*
         * Couldn't read back the Sequencer Address register setting
         * we just performed, fail.
         */
        return FALSE;
    }

    /* Read sequencer Data */
    OrgMemMode = __inpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT);

    /*
     * Toggle the Chain4 bit and read back the result. This must be done during
     * sync reset, since we're changing the chaining state.
     */

    /* Begin sync reset */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    /* Toggle the Chain4 bit */
    __outpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    __outpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT, OrgMemMode ^ CHAIN4_MASK);

    /* Read it back... it should be the same */
    if (__inpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT) != (OrgMemMode ^ CHAIN4_MASK))
    {
        /*
         * Chain4 bit is not there, not a VGA.
         * Set text mode default for Memory Mode register.
         */
        __outpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT, MEMORY_MODE_TEXT_DEFAULT);

        /* End sync reset */
        __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        /* Fail */
        return FALSE;
    }

    /*
     * It's a VGA.
     */

    /* Restore the original Memory Mode setting */
    __outpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT, OrgMemMode);

    /* End sync reset */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

    /* Restore the original Sequencer Address setting */
    __outpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, OrgSCAddr);

    /* VGA is present! */
    return TRUE;
}

#ifndef MINIMAL_UI

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

    /* Ensure the PEL mask is set (we do this because we don't explicitly
     * set the video mode registers, as in the standard VGA bootvid) */
    __outpb(VGA_BASE_IO_PORT + DAC_PIXEL_MASK_PORT, 0xFF);

    for (i = 0; i < Count; i++, Entry++)
    {
        SetPaletteEntryRGB(i, *Entry);
    }
}

#endif // !MINIMAL_UI

VOID
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    PUCHAR CharPtr;

    /* Convert pixel to text coordinates */
    Left /= (SCREEN_WIDTH / TEXT_WIDTH);
    Top  /= (SCREEN_HEIGHT / TEXT_HEIGHT);

    CharPtr = (PUCHAR)((PUSHORT)VgaBase + Top * TEXT_WIDTH + Left);
    *CharPtr++ = Character;
#ifndef MINIMAL_UI
    BackColor = (BackColor >= BV_COLOR_NONE) ? ((*CharPtr >> 4) & 0x0F) : BackColor;
    *CharPtr++ = ATTR(TextColor, BackColor);
#else
    *CharPtr++ = ATTR(BV_COLOR_DARK_GRAY, BV_COLOR_BLACK);
#endif
}

VOID
DoScroll(
    _In_ ULONG Scroll)
{
#ifndef MINIMAL_UI
    ULONG Left, Top, Right, Bottom;
    ULONG Y, RowSize;
    PUSHORT OldPosition, NewPosition;

    /* Convert pixel to text coordinates */
    Left = VidpScrollRegion.Left / (SCREEN_WIDTH / TEXT_WIDTH);
    Top  = VidpScrollRegion.Top  / (SCREEN_HEIGHT / TEXT_HEIGHT);
    Right  = VidpScrollRegion.Right  / (SCREEN_WIDTH / TEXT_WIDTH);
    Bottom = VidpScrollRegion.Bottom / (SCREEN_HEIGHT / TEXT_HEIGHT);
    Scroll /= (SCREEN_HEIGHT / TEXT_HEIGHT);

    RowSize = (Right - Left + 1);

    /* Calculate the position in memory for the row */
    OldPosition = ((PUSHORT)VgaBase + (Top + Scroll) * TEXT_WIDTH + Left);
    NewPosition = ((PUSHORT)VgaBase + Top * TEXT_WIDTH + Left);

    /* Start loop */
    for (Y = Top; Y <= Bottom; ++Y)
    {
        /* Scroll the row */
#if defined(_M_IX86) || defined(_M_AMD64)
        __movsw(NewPosition, OldPosition, RowSize);
#else
        ULONG i;
        for (i = 0; i < RowSize; ++i)
            NewPosition[i] = OldPosition[i]; // WRITE_REGISTER_USHORT(...)
#endif
        OldPosition += TEXT_WIDTH;
        NewPosition += TEXT_WIDTH;
    }
#else
    PUSHORT OldPosition, NewPosition;

    /* Convert pixel to text coordinates */
    Scroll /= (SCREEN_HEIGHT / TEXT_HEIGHT);

    /* Calculate the position in memory for the row */
    OldPosition = ((PUSHORT)VgaBase + Scroll * TEXT_WIDTH);
    NewPosition = (PUSHORT)VgaBase;

#if defined(_M_IX86) || defined(_M_AMD64)
    __movsw(NewPosition, OldPosition, TEXT_WIDTH * (TEXT_HEIGHT - Scroll));
#else
    RtlMoveMemory(NewPosition, OldPosition, TEXT_WIDTH * (TEXT_HEIGHT - Scroll) * sizeof(USHORT));
#endif
#endif // !MINIMAL_UI
}

VOID
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore)
{
    PUSHORT NewPosition, OldPosition;
    ULONG Count;

    /* Convert pixel to text coordinates */
    CurrentTop /= (SCREEN_HEIGHT / TEXT_HEIGHT);

    /* Calculate the position in memory for the row */
    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        NewPosition = ((PUSHORT)VgaBase + CurrentTop * TEXT_WIDTH);
        OldPosition = ((PUSHORT)VgaBase + TEXT_HEIGHT * TEXT_WIDTH);
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        NewPosition = ((PUSHORT)VgaBase + TEXT_HEIGHT * TEXT_WIDTH);
        OldPosition = ((PUSHORT)VgaBase + CurrentTop * TEXT_WIDTH);
    }

    /* Set the count and loop every character cell */
    // TopDelta /= (SCREEN_HEIGHT / TEXT_HEIGHT);
    // Count = TopDelta * TEXT_WIDTH;
    Count = 1 * TEXT_WIDTH;
#if defined(_M_IX86) || defined(_M_AMD64)
    __movsw(NewPosition, OldPosition, Count);
#else
    /* Write the data back to the other position */
    for (; Count--; NewPosition++, OldPosition++)
        WRITE_REGISTER_USHORT(NewPosition, READ_REGISTER_USHORT(OldPosition));
#endif
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    ULONG_PTR Context = 0;
    PHYSICAL_ADDRESS TranslatedAddress;
    PHYSICAL_ADDRESS NullAddress = {{0, 0}}, VgaAddress;
    ULONG AddressSpace;
    BOOLEAN Result;
    ULONG_PTR Base;

    /* Make sure that we have a bus translation function */
    if (!HalFindBusAddressTranslation)
        return FALSE;

    /* Loop trying to find possible VGA base addresses */
    while (TRUE)
    {
        /* Get the VGA register address */
        AddressSpace = 1;
        Result = HalFindBusAddressTranslation(NullAddress,
                                              &AddressSpace,
                                              &TranslatedAddress,
                                              &Context,
                                              TRUE);
        if (!Result)
            return FALSE;

        /* See if this is I/O space, which we need to map */
        if (!AddressSpace)
        {
            /* Map it */
            Base = (ULONG_PTR)MmMapIoSpace(TranslatedAddress, 0x400, MmNonCached);
        }
        else
        {
            /* The base is the translated address, no need to map I/O space */
            Base = TranslatedAddress.LowPart;
        }

        /* Try to see if this is VGA */
        VgaRegisterBase = Base;
        if (VgaIsPresent())
        {
            /* Translate the VGA memory address */
            VgaAddress.LowPart = MEM_TEXT_VGA;
            VgaAddress.HighPart = 0;
            AddressSpace = 0;
            Result = HalFindBusAddressTranslation(VgaAddress,
                                                  &AddressSpace,
                                                  &TranslatedAddress,
                                                  &Context,
                                                  FALSE);
            if (Result)
                break;
        }
        else
        {
            /* It's not, so unmap the I/O space if we mapped it */
            if (!AddressSpace)
                MmUnmapIoSpace((PVOID)VgaRegisterBase, 0x400);
        }

        /* Continue trying to see if there is any other address */
    }

    /* Success! See if this is I/O space, which we need to map */
    if (!AddressSpace)
    {
        /* Map it */
        Base = (ULONG_PTR)MmMapIoSpace(TranslatedAddress,
                                       MEM_TEXT_VGA_SIZE,
                                       MmNonCached);
    }
    else
    {
        /* The base is the translated address, no need to map I/O space */
        Base = TranslatedAddress.LowPart;
    }

    /* Set the VGA memory base */
    VgaBase = Base;

    /* Check whether we have to set the video mode */
    if (SetMode)
    {
        /* Reset the display */
        VidResetDisplay(FALSE);

        /* Display the hardcoded ReactOS banner */
        VidDisplayString("\n\n"
                         REACTOS_LOGO_TXT
                         "\n\n"
                         "                                     ReactOS\n"
                         "                      Copyright 1996-" COPYRIGHT_YEAR " ReactOS Project\n\n");
        VidDisplayString("ReactOS " KERNEL_VERSION_STR " (Build " KERNEL_VERSION_BUILD_STR ")\n\n"); // " (Commit " KERNEL_VERSION_COMMIT_HASH ")\n\n");
        // CHAR NtBuildLab[] = KERNEL_VERSION_BUILD_STR "." REACTOS_COMPILER_NAME "_" REACTOS_COMPILER_VERSION;
        // Example: "                 7758.0.amd64chk.winmain  Jun  4 2010  15:43:21"
    }

    return TRUE;
}

VOID
ResetDisplay(
    _In_ BOOLEAN SetMode)
{
#ifndef MINIMAL_UI
    /* Re-initialize the palette and fill the screen black */
    InitializePalette();
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
#else
    // ClearTextBuffer();
    /* Fill with character ' ' and specified color */
    PUSHORT CharPtr = (PUSHORT)VgaBase;
    USHORT Char = (' ' | (ATTR(BV_COLOR_DARK_GRAY, BV_COLOR_BLACK) << 8));
#if defined(_M_IX86) || defined(_M_AMD64)
    __stosw(CharPtr, Char, TEXT_WIDTH * TEXT_HEIGHT);
#else
    for (ULONG Count = TEXT_WIDTH * TEXT_HEIGHT; Count > 0; --Count)
        *(CharPtr++) = Char;
#endif
#endif // !MINIMAL_UI
}

#if 1 //def MINIMAL_UI
#undef VidSetScrollRegion
VOID
NTAPI
VidSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom)
{
    /* Don't change the scroll region nor the current X and Y */
    UNREFERENCED_PARAMETER(Left);
    UNREFERENCED_PARAMETER(Top);
    UNREFERENCED_PARAMETER(Right);
    UNREFERENCED_PARAMETER(Bottom);
}
#endif // MINIMAL_UI

#if (NTDDI_VERSION >= NTDDI_WIN8)
// NOTE: This API would have been much more useful if it were exposing the current cursor position instead!
NTSTATUS
NTAPI
VidGetTextInformation(
    _Out_ PULONG DispSizeX,
    _Out_ PULONG DispSizeY,
    _Out_ PULONG FontSizeX,
    _Out_ PULONG FontSizeY)
{
    if (!(DispSizeX && DispSizeY && FontSizeX && FontSizeY))
        return STATUS_INVALID_PARAMETER;

    *DispSizeX = TEXT_WIDTH;
    *DispSizeY = TEXT_HEIGHT;
    *FontSizeX = BOOTCHAR_WIDTH;
    *FontSizeY = BOOTCHAR_HEIGHT;

    return STATUS_SUCCESS;
}
#endif

VOID
NTAPI
VidCleanUp(VOID)
{
    /* Select bit mask register and clear it */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
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
#ifndef MINIMAL_UI
    /* Convert pixel to text coordinates */
    Left   /= (SCREEN_WIDTH / TEXT_WIDTH); // == 8
    Right  /= (SCREEN_WIDTH / TEXT_WIDTH);
    Top    /= (SCREEN_HEIGHT / TEXT_HEIGHT); // == 19
    Bottom /= (SCREEN_HEIGHT / TEXT_HEIGHT);

    if ((Left > Right) || (Top > Bottom) || (Left > TEXT_WIDTH) || (Top > TEXT_HEIGHT))
        return;

    /* Fill with character ' ' and specified color */
    for (ULONG Y = Top; Y <= Bottom; ++Y)
    {
        PUSHORT CharPtr = ((PUSHORT)VgaBase + (Y * TEXT_WIDTH) + Left);
        USHORT Char = (' ' | (ATTR(BV_COLOR_DARK_GRAY, Color) << 8));
#if defined(_M_IX86) || defined(_M_AMD64)
        __stosw(CharPtr, Char, Right - Left + 1);
#else
        for (ULONG X = Left; X < Right; ++X)
            *(CharPtr++) = Char;
#endif
    }
#else
    UNREFERENCED_PARAMETER(Left);
    UNREFERENCED_PARAMETER(Top);
    UNREFERENCED_PARAMETER(Right);
    UNREFERENCED_PARAMETER(Bottom);
    UNREFERENCED_PARAMETER(Color);
#endif // !MINIMAL_UI
}

VOID
NTAPI
VidScreenToBufferBlt(
    _Out_writes_bytes_all_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Left);
    UNREFERENCED_PARAMETER(Top);
    UNREFERENCED_PARAMETER(Width);
    UNREFERENCED_PARAMETER(Height);
    UNREFERENCED_PARAMETER(Delta);
}

VOID
NTAPI
VidBufferToScreenBlt(
    _In_reads_bytes_(Delta * Height) PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Left);
    UNREFERENCED_PARAMETER(Top);
    UNREFERENCED_PARAMETER(Width);
    UNREFERENCED_PARAMETER(Height);
    UNREFERENCED_PARAMETER(Delta);
}

VOID
NTAPI
VidBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG Left,
    _In_ ULONG Top)
{
#ifndef MINIMAL_UI
    PBITMAPINFOHEADER BitmapInfoHeader;
    ULONG PaletteCount;

    /* Get the bitmap header */
    BitmapInfoHeader = (PBITMAPINFOHEADER)Buffer;

    /* Initialize the palette */
    PaletteCount = BitmapInfoHeader->biClrUsed ?
                   BitmapInfoHeader->biClrUsed : BV_MAX_COLORS;
    InitPaletteWithTable((PULONG)(Buffer + BitmapInfoHeader->biSize),
                         PaletteCount);

#if 0
    /* If the height is negative, make it positive in the header */
    if (BitmapInfoHeader->biHeight < 0)
        BitmapInfoHeader->biHeight *= -1;

    /* Make sure we have a width and a height */
    if (BitmapInfoHeader->biWidth && BitmapInfoHeader->biHeight)
    {
        VidSolidColorFill(Left, Top,
                          Left + BitmapInfoHeader->biWidth,
                          Top + BitmapInfoHeader->biHeight,
                          pixel_0_0/*Color*/);
    }
#endif
#else
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Left);
    UNREFERENCED_PARAMETER(Top);
#endif // !MINIMAL_UI
}
