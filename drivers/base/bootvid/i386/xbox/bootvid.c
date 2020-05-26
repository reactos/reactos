/*
 * PROJECT:     ReactOS Boot Video Driver for Original Xbox
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp (gvg@reactos.org)
 *              Copyright 2005 Filip Navara (navaraf@reactos.org)
 *              Copyright 2020 Stanislav Motylkov (x86corez@gmail.com)
 */

#include "precomp.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

static ULONG_PTR FrameBufferStart = 0;
static ULONG FrameBufferWidth, FrameBufferHeight, PanH, PanV;
static UCHAR BytesPerPixel;
static RGBQUAD CachedPalette[BV_MAX_COLORS];
static PUCHAR BackBuffer = NULL;

/* PRIVATE FUNCTIONS *********************************************************/

static UCHAR
NvGetCrtc(
    ULONG Base,
    UCHAR Index)
{
    WRITE_REGISTER_UCHAR((PUCHAR)(Base + NV2A_CRTC_REGISTER_INDEX), Index);
    return READ_REGISTER_UCHAR((PUCHAR)(Base + NV2A_CRTC_REGISTER_VALUE));
}

static UCHAR
NvGetBytesPerPixel(
    ULONG Base,
    ULONG ScreenWidth)
{
    /* Get BPP directly from NV2A CRTC (magic constants are from Cromwell) */
    UCHAR BytesPerPixel = 8 * (((NvGetCrtc(Base, 0x19) & 0xE0) << 3) | (NvGetCrtc(Base, 0x13) & 0xFF)) / ScreenWidth;

    if (BytesPerPixel == 4)
    {
        ASSERT((NvGetCrtc(Base, 0x28) & 0xF) == BytesPerPixel - 1);
    }
    else
    {
        ASSERT((NvGetCrtc(Base, 0x28) & 0xF) == BytesPerPixel);
    }

    return BytesPerPixel;
}

static VOID
ApplyPalette(VOID)
{
    PULONG Frame = (PULONG)FrameBufferStart;
    ULONG x, y;

    /* Top panning */
    for (x = 0; x < PanV * FrameBufferWidth; x++)
    {
        *Frame++ = CachedPalette[0];
    }

    /* Left panning */
    for (y = 0; y < SCREEN_HEIGHT; y++)
    {
        Frame = (PULONG)(FrameBufferStart + FB_OFFSET(-PanH, y));

        for (x = 0; x < PanH; x++)
        {
            *Frame++ = CachedPalette[0];
        }
    }

    /* Screen redraw */
    PUCHAR Back = BackBuffer;
    for (y = 0; y < SCREEN_HEIGHT; y++)
    {
        Frame = (PULONG)(FrameBufferStart + FB_OFFSET(0, y));

        for (x = 0; x < SCREEN_WIDTH; x++)
        {
            *Frame++ = CachedPalette[*Back++];
        }
    }

    /* Right panning */
    for (y = 0; y < SCREEN_HEIGHT; y++)
    {
        Frame = (PULONG)(FrameBufferStart + FB_OFFSET(SCREEN_WIDTH, y));

        for (x = 0; x < PanH; x++)
        {
            *Frame++ = CachedPalette[0];
        }
    }

    /* Bottom panning */
    Frame = (PULONG)(FrameBufferStart + FB_OFFSET(-PanH, SCREEN_HEIGHT));
    for (x = 0; x < PanV * FrameBufferWidth; x++)
    {
        *Frame++ = CachedPalette[0];
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    BOOLEAN Result = FALSE;

    /* FIXME: Add platform check */
    /* 1. Access PCI device 1:0:0 */
    /* 2. Check if device ID is 10DE:02A0 */

    /* FIXME: Get device MMIO ranges from PCI */
    PHYSICAL_ADDRESS PhysControlStart = {.QuadPart = 0xFD000000};
    PHYSICAL_ADDRESS PhysFrameBufferStart = {.QuadPart = 0xF0000000};
    ULONG ControlLength = 16 * 1024 * 1024;

    ULONG_PTR ControlStart = (ULONG_PTR)MmMapIoSpace(PhysControlStart, ControlLength, MmNonCached);
    if (!ControlStart)
    {
        DPRINT1("Out of memory!\n");
        return FALSE;
    }

    ULONG_PTR FrameBuffer = READ_REGISTER_ULONG((PULONG)(ControlStart + NV2A_CONTROL_FRAMEBUFFER_ADDRESS_OFFSET));
    FrameBufferWidth = READ_REGISTER_ULONG((PULONG)(ControlStart + NV2A_RAMDAC_FP_HVALID_END)) + 1;
    FrameBufferHeight = READ_REGISTER_ULONG((PULONG)(ControlStart + NV2A_RAMDAC_FP_VVALID_END)) + 1;

    FrameBuffer &= 0x0FFFFFFF;
    if (FrameBuffer != 0x3C00000 && FrameBuffer != 0x7C00000)
    {
        /* Check framebuffer address (high 4 MB of either 64 or 128 MB RAM) */
        DPRINT1("Non-standard framebuffer address 0x%p\n", FrameBuffer);
    }
    /* Verify that framebuffer address is page-aligned */
    ASSERT(FrameBuffer % PAGE_SIZE == 0);

    if (FrameBufferWidth < SCREEN_WIDTH || FrameBufferHeight < SCREEN_HEIGHT)
    {
        DPRINT1("Unsupported screen resolution!\n");
        goto cleanup;
    }

    BytesPerPixel = NvGetBytesPerPixel(ControlStart, FrameBufferWidth);
    ASSERT(BytesPerPixel >= 1 && BytesPerPixel <= 4);

    if (BytesPerPixel != 4)
    {
        DPRINT1("Unsupported BytesPerPixel = %d\n", BytesPerPixel);
        goto cleanup;
    }

    /* Calculate panning values */
    PanH = (FrameBufferWidth - SCREEN_WIDTH) / 2;
    PanV = (FrameBufferHeight - SCREEN_HEIGHT) / 2;

    /* Verify that screen fits framebuffer size */
    ULONG FrameBufferSize = FrameBufferWidth * FrameBufferHeight * BytesPerPixel;

    /* FIXME: obtain fb size from firmware somehow (Cromwell reserves high 4 MB of RAM) */
    if (FrameBufferSize > NV2A_VIDEO_MEMORY_SIZE)
    {
        DPRINT1("Current screen resolution exceeds video memory bounds!\n");
        goto cleanup;
    }

    /*
     * Reserve off-screen area for the backbuffer that contains 8-bit indexed
     * color screen image, plus preserved row data.
     */
    ULONG BackBufferSize = SCREEN_WIDTH * (SCREEN_HEIGHT + BOOTCHAR_HEIGHT + 1);

    /* Make sure there is enough video memory for backbuffer */
    if (NV2A_VIDEO_MEMORY_SIZE - FrameBufferSize < BackBufferSize)
    {
        DPRINT1("Out of memory!\n");
        goto cleanup;
    }

    /* Return the address back to GPU memory mapped I/O */
    PhysFrameBufferStart.QuadPart += FrameBuffer;
    FrameBufferStart = (ULONG_PTR)MmMapIoSpace(PhysFrameBufferStart, NV2A_VIDEO_MEMORY_SIZE, MmNonCached);
    if (!FrameBufferStart)
    {
        DPRINT1("Out of memory!\n");
        goto cleanup;
    }

    Result = TRUE;

    /* Place backbuffer in the hidden part of framebuffer */
    BackBuffer = (PUCHAR)(FrameBufferStart + NV2A_VIDEO_MEMORY_SIZE - BackBufferSize);

    /* Now check if we have to set the mode */
    if (SetMode)
        VidResetDisplay(TRUE);

cleanup:
    if (ControlStart)
        MmUnmapIoSpace((PVOID)ControlStart, ControlLength);

    /* Video is ready */
    return Result;
}

VOID
NTAPI
VidCleanUp(VOID)
{
    /* Just fill the screen black */
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN HalReset)
{
    /* Clear the current position */
    VidpCurrentX = 0;
    VidpCurrentY = 0;

    /* Clear the screen with HAL if we were asked to */
    if (HalReset)
        HalResetDisplay();

    /* Re-initialize the palette and fill the screen black */
    RtlZeroMemory((PULONG)FrameBufferStart, NV2A_VIDEO_MEMORY_SIZE);
    InitializePalette();
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}

VOID
NTAPI
InitPaletteWithTable(
    _In_ PULONG Table,
    _In_ ULONG Count)
{
    PULONG Entry = Table;

    for (ULONG i = 0; i < Count; i++, Entry++)
    {
        CachedPalette[i] = *Entry | 0xFF000000;
    }
    ApplyPalette();
}

VOID
PrepareForSetPixel(VOID)
{
    /* Nothing to prepare */
    NOTHING;
}

VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color)
{
    PUCHAR Back = BackBuffer + BB_OFFSET(Left, Top);
    PULONG Frame = (PULONG)(FrameBufferStart + FB_OFFSET(Left, Top));

    *Back = Color;
    *Frame = CachedPalette[Color];
}

VOID
NTAPI
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore)
{
    PUCHAR NewPosition, OldPosition;

    /* Calculate the position in memory for the row */
    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        NewPosition = BackBuffer + BB_OFFSET(0, CurrentTop);
        OldPosition = BackBuffer + BB_OFFSET(0, SCREEN_HEIGHT);
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        NewPosition = BackBuffer + BB_OFFSET(0, SCREEN_HEIGHT);
        OldPosition = BackBuffer + BB_OFFSET(0, CurrentTop);
    }

    /* Set the count and loop every pixel of backbuffer */
    ULONG Count = TopDelta * SCREEN_WIDTH;

    RtlCopyMemory(NewPosition, OldPosition, Count);

    if (Restore)
    {
        NewPosition = BackBuffer + BB_OFFSET(0, CurrentTop);

        /* Set the count and loop every pixel of framebuffer */
        for (ULONG y = 0; y < TopDelta; y++)
        {
            PULONG Frame = (PULONG)(FrameBufferStart + FB_OFFSET(0, CurrentTop + y));

            Count = SCREEN_WIDTH;
            while (Count--)
            {
                *Frame++ = CachedPalette[*NewPosition++];
            }
        }
    }
}

VOID
NTAPI
DoScroll(
    _In_ ULONG Scroll)
{
    ULONG RowSize = VidpScrollRegion[2] - VidpScrollRegion[0] + 1;

    /* Calculate the position in memory for the row */
    PUCHAR OldPosition = BackBuffer + BB_OFFSET(VidpScrollRegion[0], VidpScrollRegion[1] + Scroll);
    PUCHAR NewPosition = BackBuffer + BB_OFFSET(VidpScrollRegion[0], VidpScrollRegion[1]);

    /* Start loop */
    for (ULONG Top = VidpScrollRegion[1]; Top <= VidpScrollRegion[3]; ++Top)
    {
        ULONG i;

        /* Scroll the row */
        RtlCopyMemory(NewPosition, OldPosition, RowSize);

        PULONG Frame = (PULONG)(FrameBufferStart + FB_OFFSET(VidpScrollRegion[0], Top));

        for (i = 0; i < RowSize; ++i)
            Frame[i] = CachedPalette[NewPosition[i]];

        OldPosition += SCREEN_WIDTH;
        NewPosition += SCREEN_WIDTH;
    }
}

VOID
NTAPI
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor)
{
    /* Get the font and pixel pointer */
    PUCHAR FontChar = GetFontPtr(Character);

    /* Loop each pixel height */
    for (ULONG y = Top; y < Top + BOOTCHAR_HEIGHT; y++, FontChar += FONT_PTR_DELTA)
    {
        /* Loop each pixel width */
        ULONG x = Left;

        for (UCHAR bit = 1 << (BOOTCHAR_WIDTH - 1); bit > 0; bit >>= 1, x++)
        {
            /* Check if we should draw this pixel */
            if (*FontChar & bit)
            {
                /* We do, use the given Text Color */
                SetPixel(x, y, (UCHAR)TextColor);
            }
            else if (BackColor < BV_COLOR_NONE)
            {
                /*
                 * This is a background pixel. We're drawing it
                 * unless it's transparent.
                 */
                SetPixel(x, y, (UCHAR)BackColor);
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
    while (Top <= Bottom)
    {
        PUCHAR Back = BackBuffer + BB_OFFSET(Left, Top);
        PULONG Frame = (PULONG)(FrameBufferStart + FB_OFFSET(Left, Top));
        ULONG L = Left;

        while (L++ <= Right)
        {
            *Back++ = Color;
            *Frame++ = CachedPalette[Color];
        }
        Top++;
    }
}

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
    /* Clear the destination buffer */
    RtlZeroMemory(Buffer, Delta * Height);

    /* Start the outer Y height loop */
    for (ULONG y = 0; y < Height; y++)
    {
        /* Set current scanline */
        PUCHAR Back = BackBuffer + BB_OFFSET(Left, Top + y);
        PUCHAR Buf = Buffer + y * Delta;

        /* Start the X inner loop */
        for (ULONG x = 0; x < Width; x += 2)
        {
            /* Read the current value */
            *Buf = (*Back++ & 0xF) << 4;
            *Buf |= *Back++ & 0xF;
            Buf++;
        }
    }
}
