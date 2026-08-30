/*
 * PROJECT:     ReactOS Generic Framebuffer Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2023-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* Include the Boot-time (POST) display discovery helper functions */
#include <drivers/bootvid/framebuf.c>

/* Scaling of the bootvid 640x480 default virtual screen to the larger video framebuffer */
#define SCALING_SUPPORT
#define SCALING_PROPORTIONAL

/* Keep borders black or controlled with palette */
// #define COLORED_BORDERS


/* GLOBALS ********************************************************************/

#define BB_PIXEL(x, y) \
    ((PUCHAR)BackBuffer + (y) * SCREEN_WIDTH + (x))

#define FB_PIXEL(x, y) \
    ((PUCHAR)FrameBufferStart + (PanV + VidpYScale * (y)) * BytesPerScanLine \
                              + (PanH + VidpXScale * (x)) * BytesPerPixel)

static ULONG_PTR FrameBufferStart = 0;
static ULONG FrameBufferSize;
static ULONG ScreenWidth, ScreenHeight, BytesPerScanLine;
static UCHAR BytesPerPixel;
static PUCHAR BackBuffer = NULL;
static SIZE_T BackBufferSize;

#ifdef SCALING_SUPPORT
static USHORT VidpXScale = 1;
static USHORT VidpYScale = 1;
#else
#define VidpXScale 1
#define VidpYScale 1
#endif
static ULONG PanH, PanV;

static RGBQUAD CachedPalette[BV_MAX_COLORS];


/* PRIVATE FUNCTIONS *********************************************************/

static VOID
FlushBackBufferRect(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Width,
    _In_ ULONG Height)
{
    ULONG y;

    if (!Width || !Height)
        return;

    for (y = Top; y < Top + Height; ++y)
    {
        PUCHAR Back = BB_PIXEL(Left, y);

        for (ULONG i = 0; i < VidpYScale; ++i)
        {
            PULONG Pixel = (PULONG)((ULONG_PTR)FB_PIXEL(Left, y) +
                                     i * BytesPerScanLine);

            for (ULONG x = 0; x < Width; ++x)
            {
                for (ULONG j = VidpXScale; j > 0; --j)
                    *Pixel++ = CachedPalette[Back[x]];
            }
        }
    }
}

static VOID
ApplyPalette(VOID)
{
#ifdef COLORED_BORDERS
    PULONG Frame = (PULONG)FrameBufferStart;
    ULONG x, y;

    /* Top border */
    for (x = 0; x < PanV * ScreenWidth; ++x)
    {
        *Frame++ = CachedPalette[BV_COLOR_BLACK];
    }

    /* Left border */
    for (y = 0; y < VidpYScale * SCREEN_HEIGHT; ++y)
    {
        // Frame = (PULONG)(FrameBufferStart + FB_OFFSET(-(LONG)PanH, y));
        Frame = (PULONG)(FrameBufferStart + (PanV + y) * BytesPerScanLine);
        for (x = 0; x < PanH; ++x)
        {
            *Frame++ = CachedPalette[BV_COLOR_BLACK];
        }
    }
#endif // COLORED_BORDERS

    /* Screen redraw */
    FlushBackBufferRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

#ifdef COLORED_BORDERS
    /* Right border */
    for (y = 0; y < VidpYScale * SCREEN_HEIGHT; ++y)
    {
        // Frame = (PULONG)(FrameBufferStart + FB_OFFSET(SCREEN_WIDTH, y));
        Frame = (PULONG)(FrameBufferStart + (PanV + y) * BytesPerScanLine + (PanH + VidpXScale * SCREEN_WIDTH) * BytesPerPixel);
        for (x = 0; x < PanH; ++x)
        {
            *Frame++ = CachedPalette[BV_COLOR_BLACK];
        }
    }

    /* Bottom border */
    // Frame = (PULONG)(FrameBufferStart + FB_OFFSET(-(LONG)PanH, SCREEN_HEIGHT));
    Frame = (PULONG)(FrameBufferStart + (PanV + VidpYScale * SCREEN_HEIGHT) * BytesPerScanLine);
    for (x = 0; x < PanV * ScreenWidth; ++x)
    {
        *Frame++ = CachedPalette[BV_COLOR_BLACK];
    }
#endif // COLORED_BORDERS
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    PHYSICAL_ADDRESS FrameBuffer;
    PHYSICAL_ADDRESS VramAddress;
    ULONG VramSize;
    CM_FRAMEBUF_DEVICE_DATA VideoConfigData; /* Configuration data from hardware tree */
    INTERFACE_TYPE Interface;
    ULONG BusNumber;
    NTSTATUS Status;

    /* Find boot-time framebuffer display information from the LoaderBlock */
    Status = FindBootDisplay(&VramAddress,
                             &VramSize,
                             &VideoConfigData,
                             NULL, // MonitorConfigData
                             &Interface,
                             &BusNumber);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Boot framebuffer does not exist!\n");
        return FALSE;
    }

    /* The VRAM address must be page-aligned */
    if (VramAddress.QuadPart % PAGE_SIZE != 0) // DPRINTed for diagnostics on some systems
        DPRINT1("** VramAddress 0x%I64X isn't PAGE_SIZE aligned\n", VramAddress.QuadPart);
    ASSERT(VramAddress.QuadPart % PAGE_SIZE == 0);
    if (VramSize % PAGE_SIZE != 0)
        DPRINT1("** VramSize %lu (0x%lx) isn't multiple of PAGE_SIZE\n", VramSize, VramSize);
    // ASSERT(VramSize % PAGE_SIZE == 0); // This assert may fail, e.g. 800x600@32bpp UEFI GOP display

    /* Retrieve the framebuffer address, its visible screen dimensions, and its attributes */
    FrameBuffer.QuadPart = VramAddress.QuadPart + VideoConfigData.FrameBufferOffset;
    ScreenWidth  = VideoConfigData.ScreenWidth;
    ScreenHeight = VideoConfigData.ScreenHeight;
    if (ScreenWidth < SCREEN_WIDTH || ScreenHeight < SCREEN_HEIGHT)
    {
        DPRINT1("Unsupported screen resolution!\n");
        return FALSE;
    }

    BytesPerPixel = (VideoConfigData.BitsPerPixel + 7) / 8; // Round up to nearest byte.
    ASSERT(BytesPerPixel >= 1 && BytesPerPixel <= 4);
    if (BytesPerPixel != 4)
    {
        UNIMPLEMENTED;
        DPRINT1("Unsupported BytesPerPixel = %u\n", BytesPerPixel);
        return FALSE;
    }

    ASSERT(ScreenWidth <= VideoConfigData.PixelsPerScanLine);
    BytesPerScanLine = VideoConfigData.PixelsPerScanLine * BytesPerPixel;
    if (BytesPerScanLine < 1)
    {
        DPRINT1("Invalid BytesPerScanLine = %lu\n", BytesPerScanLine);
        return FALSE;
    }

    /* Compute the visible framebuffer size */
    FrameBufferSize = ScreenHeight * BytesPerScanLine;

    /* Verify that the framebuffer actually fits inside the video RAM */
    if (FrameBuffer.QuadPart + FrameBufferSize > VramAddress.QuadPart + VramSize)
    {
        DPRINT1("The framebuffer exceeds video memory bounds!\n");
        return FALSE;
    }

    /* Translate the framebuffer from bus-relative to physical address */
    PHYSICAL_ADDRESS TranslatedAddress;
    ULONG AddressSpace = 0; /* MMIO space */
    if (!BootTranslateBusAddress(Interface,
                                 BusNumber,
                                 FrameBuffer,
                                 &AddressSpace,
                                 &TranslatedAddress))
    {
        DPRINT1("Could not translate framebuffer bus address 0x%I64X\n", FrameBuffer.QuadPart);
        return FALSE;
    }

    /* Map it into system space if necessary */
    ULONG MappedSize = 0;
    PVOID FrameBufferBase = NULL;
    if (AddressSpace == 0)
    {
        /* Calculate page-aligned address and size for MmMapIoSpace() */
        FrameBuffer.HighPart = TranslatedAddress.HighPart;
        FrameBuffer.LowPart  = ALIGN_DOWN_BY(TranslatedAddress.LowPart, PAGE_SIZE);
        MappedSize = FrameBufferSize;
        MappedSize += (ULONG)(TranslatedAddress.QuadPart - FrameBuffer.QuadPart); // BYTE_OFFSET()
        MappedSize = ROUND_TO_PAGES(MappedSize);
        /* Essentially MmMapVideoDisplay() */
        FrameBufferBase = MmMapIoSpace(FrameBuffer, MappedSize, MmFrameBufferCached);
        if (!FrameBufferBase)
            FrameBufferBase = MmMapIoSpace(FrameBuffer, MappedSize, MmNonCached);
        if (!FrameBufferBase)
        {
            DPRINT1("Could not map framebuffer 0x%I64X (%lu bytes)\n",
                    FrameBuffer.QuadPart, MappedSize);
            goto Failure;
        }
        FrameBufferStart = (ULONG_PTR)FrameBufferBase;
        FrameBufferStart += (TranslatedAddress.QuadPart - FrameBuffer.QuadPart); // BYTE_OFFSET()
    }
    else
    {
        /* The base is the translated address, no need to map */
        FrameBufferStart = (ULONG_PTR)TranslatedAddress.QuadPart;
    }


    /*
     * Reserve off-screen area for the backbuffer that contains
     * 8-bit indexed color screen image, plus preserved row data.
     */
    BackBufferSize = SCREEN_WIDTH * (SCREEN_HEIGHT + (BOOTCHAR_HEIGHT + 1));

    /*
     * Keep the backbuffer in cached system RAM. Reading from the GOP
     * framebuffer is often extremely slow, even when writes are combined.
     */
    {
        PHYSICAL_ADDRESS NullAddress = {{0, 0}};
        PHYSICAL_ADDRESS HighestAddress = {{-1, -1}};
        BackBuffer = MmAllocateContiguousMemorySpecifyCache(
                        BackBufferSize, NullAddress, HighestAddress,
                        NullAddress, MmCached);
    }

    if (!BackBuffer &&
        VideoConfigData.FrameBufferOffset + FrameBufferSize + BackBufferSize
            <= ((AddressSpace == 0) ? MappedSize : VramSize))
    {
        /* Backbuffer placed following the framebuffer in the hidden part */
        BackBuffer = (PUCHAR)(FrameBufferStart + FrameBufferSize);
        // BackBuffer = (PUCHAR)(VramAddress + VramSize - BackBufferSize); // Or at the end of VRAM.
    }

    if (!BackBuffer)
    {
        DPRINT1("Could not allocate backbuffer (size: %lu)\n", (ULONG)BackBufferSize);
        goto Failure;
    }

    RtlZeroMemory(BackBuffer, BackBufferSize);

#ifdef SCALING_SUPPORT
    /* Compute autoscaling; only integer (not fractional) scaling is supported */
    VidpXScale = ScreenWidth / SCREEN_WIDTH;
    VidpYScale = ScreenHeight / SCREEN_HEIGHT;
    ASSERT(VidpXScale >= 1);
    ASSERT(VidpYScale >= 1);
#ifdef SCALING_PROPORTIONAL
    VidpXScale = min(VidpXScale, VidpYScale);
    VidpYScale = VidpXScale;
#endif
    DPRINT1("Scaling X = %hu, Y = %hu\n", VidpXScale, VidpYScale);
#endif // SCALING_SUPPORT

    /* Calculate left/right and top/bottom border values
     * to keep the displayed area centered on the screen */
    PanH = (ScreenWidth - VidpXScale * SCREEN_WIDTH) / 2;
    PanV = (ScreenHeight - VidpYScale * SCREEN_HEIGHT) / 2;
    DPRINT1("Borders X = %lu, Y = %lu\n", PanH, PanV);

    /* Reset the video mode if requested */
    if (SetMode)
        VidResetDisplay(TRUE);

    return TRUE;

Failure:
    /* We failed somewhere; unmap the framebuffer if we mapped it */
    if (FrameBufferBase && (AddressSpace == 0))
        MmUnmapIoSpace(FrameBufferBase, MappedSize);

    return FALSE;
}

VOID
NTAPI
VidCleanUp(VOID)
{
    /* Just fill the screen black */
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

VOID
ResetDisplay(
    _In_ BOOLEAN SetMode)
{
    RtlZeroMemory(BackBuffer, BackBufferSize);
    RtlZeroMemory((PVOID)FrameBufferStart, FrameBufferSize);

    /* Re-initialize the palette and fill the screen black */
    InitializePalette();
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

VOID
InitPaletteWithTable(
    _In_reads_(Count) const ULONG* Table,
    _In_ ULONG Count)
{
    const ULONG* Entry = Table;
    ULONG i;
    BOOLEAN HasChanged = FALSE;

    for (i = 0; i < Count; i++, Entry++)
    {
        HasChanged |= !!((CachedPalette[i] ^ *Entry) & 0x00FFFFFF);
        CachedPalette[i] = *Entry | 0xFF000000;
    }

    /* Re-apply the palette if it has changed */
    if (HasChanged)
        ApplyPalette();
}

VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color)
{
    PUCHAR Back = BB_PIXEL(Left, Top);
    PULONG Frame = (PULONG)FB_PIXEL(Left, Top);

    *Back = Color;
    for (ULONG i = VidpYScale; i > 0; --i)
    {
        PULONG Pixel = Frame;
        for (ULONG j = VidpXScale; j > 0; --j)
            *Pixel++ = CachedPalette[Color];
        Frame = (PULONG)((ULONG_PTR)Frame + BytesPerScanLine);
    }
}

VOID
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG Height,
    _In_ BOOLEAN Restore)
{
    PUCHAR NewPosition, OldPosition;

    /* Calculate the position in memory for the row */
    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        NewPosition = BB_PIXEL(0, CurrentTop);
        OldPosition = BB_PIXEL(0, SCREEN_HEIGHT);
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        NewPosition = BB_PIXEL(0, SCREEN_HEIGHT);
        OldPosition = BB_PIXEL(0, CurrentTop);
    }

    /* Set the count and copy the pixel data back to the other position in the backbuffer */
    ULONG Count = Height * SCREEN_WIDTH;
    RtlCopyMemory(NewPosition, OldPosition, Count);

    /* On restore, mirror the backbuffer changes to the framebuffer */
    if (Restore)
    {
        FlushBackBufferRect(0, CurrentTop, SCREEN_WIDTH, Height);
    }
}

VOID
DoScroll(
    _In_ ULONG Scroll)
{
    ULONG RowSize = VidpScrollRegion.Right - VidpScrollRegion.Left + 1;
    ULONG Height = VidpScrollRegion.Bottom - VidpScrollRegion.Top + 1;

    if (!Scroll || Scroll >= Height)
        return;

    /* Calculate the position in memory for the row */
    PUCHAR OldPosition = BB_PIXEL(VidpScrollRegion.Left, VidpScrollRegion.Top + Scroll);
    PUCHAR NewPosition = BB_PIXEL(VidpScrollRegion.Left, VidpScrollRegion.Top);

    /* Start loop */
    for (ULONG Top = VidpScrollRegion.Top; Top <= VidpScrollRegion.Bottom; ++Top)
    {
        /* Scroll the row */
        RtlCopyMemory(NewPosition, OldPosition, RowSize);

        OldPosition += SCREEN_WIDTH;
        NewPosition += SCREEN_WIDTH;
    }

    FlushBackBufferRect(VidpScrollRegion.Left,
                        VidpScrollRegion.Top,
                        RowSize,
                        Height);
}

VOID
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    /* Get the font line for this character */
    const UCHAR* FontChar = GetFontPtr(Character);
    const BOOLEAN Opaque = (BackColor < BV_COLOR_NONE);

    /* Loop each pixel height */
    for (ULONG y = Top; y < Top + BOOTCHAR_HEIGHT; ++y, FontChar += FONT_PTR_DELTA)
    {
        PUCHAR Back = BB_PIXEL(Left, y);

        if (Opaque)
        {
            /* Loop each pixel width */
            for (UCHAR bit = 1 << (BOOTCHAR_WIDTH - 1); bit > 0; bit >>= 1)
            {
                UCHAR Color = (*FontChar & bit) ? (UCHAR)TextColor : (UCHAR)BackColor;

                *Back++ = Color;
            }

            for (ULONG i = 0; i < VidpYScale; ++i)
            {
                PULONG Pixel = (PULONG)((ULONG_PTR)FB_PIXEL(Left, y) +
                                         i * BytesPerScanLine);

                for (UCHAR bit = 1 << (BOOTCHAR_WIDTH - 1); bit > 0; bit >>= 1)
                {
                    UCHAR Color = (*FontChar & bit) ? (UCHAR)TextColor : (UCHAR)BackColor;

                    for (ULONG j = VidpXScale; j > 0; --j)
                        *Pixel++ = CachedPalette[Color];
                }
            }
        }
        else
        {
            ULONG x = Left;

            /* Loop each pixel width */
            for (UCHAR bit = 1 << (BOOTCHAR_WIDTH - 1); bit > 0; bit >>= 1, ++x)
            {
                if (!(*FontChar & bit))
                {
                    ++Back;
                    continue;
                }

                *Back++ = (UCHAR)TextColor;

                PULONG Frame = (PULONG)FB_PIXEL(x, y);
                for (ULONG i = VidpYScale; i > 0; --i)
                {
                    PULONG Pixel = Frame;
                    for (ULONG j = VidpXScale; j > 0; --j)
                        *Pixel++ = CachedPalette[(UCHAR)TextColor];
                    Frame = (PULONG)((ULONG_PTR)Frame + BytesPerScanLine);
                }
            }
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
    ULONG Width = Right - Left + 1;
    ULONG NativeWidth = Width * VidpXScale * BytesPerPixel;
    ULONG NativeColor = CachedPalette[Color];

    for (; Top <= Bottom; ++Top)
    {
        PUCHAR Back = BB_PIXEL(Left, Top);

        RtlFillMemory(Back, Width, Color);

        for (ULONG i = 0; i < VidpYScale; ++i)
        {
            PULONG Frame = (PULONG)((ULONG_PTR)FB_PIXEL(Left, Top) +
                                     i * BytesPerScanLine);
            RtlFillMemoryUlong(Frame, NativeWidth, NativeColor);
        }
    }
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
    ULONG x, y;

    /* Clear the destination buffer */
    RtlZeroMemory(Buffer, Height * Stride);

    /* Start the outer Y height loop */
    for (y = 0; y < Height; ++y)
    {
        /* Set current scanline */
        PUCHAR Back = BB_PIXEL(Left, Top + y);
        PUCHAR Buf = Buffer + y * Stride;

        /* Start the X inner loop */
        for (x = 0; x < Width; x += sizeof(USHORT))
        {
            /* Read the current value */
            *Buf = (*Back++ & 0xF) << 4;
            *Buf |= *Back++ & 0xF;
            Buf++;
        }
    }
}

/* EOF */
