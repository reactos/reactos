/*
 * PROJECT:     ReactOS Boot Video Driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* GLOBALS ********************************************************************/

static ULONG_PTR VideoMemoryI;
ULONG_PTR FrameBuffer;

#define PEGC_MAX_COLORS    256

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN
GraphGetStatus(
    _In_ UCHAR Status)
{
    UCHAR Result;

    WRITE_PORT_UCHAR((PUCHAR)GRAPH_IO_o_STATUS_SELECT, Status);
    KeStallExecutionProcessor(1);
    Result = READ_PORT_UCHAR((PUCHAR)GRAPH_IO_i_STATUS);

    return (Result & GRAPH_STATUS_SET) && (Result != 0xFF);
}

static BOOLEAN
HasPegcController(VOID)
{
    BOOLEAN Success;

    if (GraphGetStatus(GRAPH_STATUS_PEGC))
        return TRUE;

    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_UNPROTECT);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_PEGC_ENABLE);
    Success = GraphGetStatus(GRAPH_STATUS_PEGC);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_PEGC_DISABLE);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_PROTECT);

    return Success;
}

static VOID
TextSync(VOID)
{
    while (READ_PORT_UCHAR((PUCHAR)GDC1_IO_i_STATUS) & GDC_STATUS_VSYNC)
        NOTHING;

    while (!(READ_PORT_UCHAR((PUCHAR)GDC1_IO_i_STATUS) & GDC_STATUS_VSYNC))
        NOTHING;
}

static VOID
InitializeDisplay(VOID)
{
    SYNCPARAM SyncParameters;
    CSRFORMPARAM CursorParameters;
    CSRWPARAM CursorPosition;
    PITCHPARAM PitchParameters;
    PRAMPARAM RamParameters;
    ZOOMPARAM ZoomParameters;
    UCHAR RelayState;

    /* RESET, without FIFO check */
    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_COMMAND, GDC_COMMAND_RESET1);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_COMMAND, GDC_COMMAND_RESET1);

    /* Configure chipset */
    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_COLORED);
    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GDC2_MODE_ODD_RLINE_SHOW);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_COLORS_16);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_GRCG);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_LCD);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_LINES_400);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_CLOCK1_5MHZ);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_CLOCK2_5MHZ);
    WRITE_PORT_UCHAR((PUCHAR)GRAPH_IO_o_HORIZONTAL_SCAN_RATE, GRAPH_HF_31KHZ);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_VIDEO_PAGE, 0);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_VIDEO_PAGE_ACCESS, 0);

    /* =========================== MASTER ============================ */

    /* MASTER */
    WRITE_GDC1_COMMAND(GDC_COMMAND_MASTER);

    /* SYNC */
    SyncParameters.Flags = SYNC_DISPLAY_MODE_GRAPHICS_AND_CHARACTERS | SYNC_VIDEO_FRAMING_NONINTERLACED |
                           SYNC_DRAW_ONLY_DURING_RETRACE_BLANKING | SYNC_STATIC_RAM_NO_REFRESH;
    SyncParameters.ScreenWidthChars = 80;
    SyncParameters.HorizontalSyncWidth = 12;
    SyncParameters.VerticalSyncWidth = 2;
    SyncParameters.HorizontalFrontPorchWidth = 4;
    SyncParameters.HorizontalBackPorchWidth = 4;
    SyncParameters.VerticalFrontPorchWidth = 6;
    SyncParameters.ScreenWidthLines = 480;
    SyncParameters.VerticalBackPorchWidth = 37;
    WRITE_GDC1_COMMAND(GDC_COMMAND_SYNC_ON);
    WRITE_GDC_SYNC((PUCHAR)GDC1_IO_o_PARAM, &SyncParameters);

    /* CSRFORM */
    CursorParameters.Show = FALSE;
    CursorParameters.Blink = FALSE;
    CursorParameters.BlinkRate = 12;
    CursorParameters.LinesPerRow = 16;
    CursorParameters.StartScanLine = 0;
    CursorParameters.EndScanLine = 15;
    WRITE_GDC1_COMMAND(GDC_COMMAND_CSRFORM);
    WRITE_GDC_CSRFORM((PUCHAR)GDC1_IO_o_PARAM, &CursorParameters);

    /* PITCH */
    PitchParameters.WordsPerScanline = BYTES_PER_SCANLINE;
    WRITE_GDC1_COMMAND(GDC_COMMAND_PITCH);
    WRITE_GDC_PITCH((PUCHAR)GDC1_IO_o_PARAM, &PitchParameters);

    /* PRAM */
    RamParameters.StartingAddress = 0;
    RamParameters.Length = 1023;
    RamParameters.ImageBit = FALSE;
    RamParameters.WideDisplay = FALSE;
    WRITE_GDC1_COMMAND(GDC_COMMAND_PRAM);
    WRITE_GDC_PRAM((PUCHAR)GDC1_IO_o_PARAM, &RamParameters);

    /* ZOOM */
    ZoomParameters.DisplayZoomFactor = 0;
    ZoomParameters.WritingZoomFactor = 0;
    WRITE_GDC1_COMMAND(GDC_COMMAND_ZOOM);
    WRITE_GDC_ZOOM((PUCHAR)GDC1_IO_o_PARAM, &ZoomParameters);

    /* CSRW */
    CursorPosition.CursorAddress = 0;
    CursorPosition.DotAddress = 0;
    WRITE_GDC1_COMMAND(GDC_COMMAND_CSRW);
    WRITE_GDC_CSRW((PUCHAR)GDC1_IO_o_PARAM, &CursorPosition);

    /* START */
    WRITE_GDC1_COMMAND(GDC_COMMAND_BCTRL_START);

    /* ============================ SLAVE ============================ */

    /* SLAVE */
    WRITE_GDC2_COMMAND(GDC_COMMAND_SLAVE);

    /* SYNC */
    SyncParameters.Flags = SYNC_DISPLAY_MODE_GRAPHICS | SYNC_VIDEO_FRAMING_NONINTERLACED |
                           SYNC_DRAW_DURING_ACTIVE_DISPLAY_TIME_AND_RETRACE_BLANKING |
                           SYNC_STATIC_RAM_NO_REFRESH;
    SyncParameters.ScreenWidthChars = 80;
    SyncParameters.HorizontalSyncWidth = 12;
    SyncParameters.VerticalSyncWidth = 2;
    SyncParameters.HorizontalFrontPorchWidth = 4;
    SyncParameters.HorizontalBackPorchWidth = 132;
    SyncParameters.VerticalFrontPorchWidth = 6;
    SyncParameters.ScreenWidthLines = 480;
    SyncParameters.VerticalBackPorchWidth = 37;
    WRITE_GDC2_COMMAND(GDC_COMMAND_SYNC_ON);
    WRITE_GDC_SYNC((PUCHAR)GDC2_IO_o_PARAM, &SyncParameters);

    /* CSRFORM */
    CursorParameters.Show = FALSE;
    CursorParameters.Blink = FALSE;
    CursorParameters.BlinkRate = 0;
    CursorParameters.LinesPerRow = 1;
    CursorParameters.StartScanLine = 0;
    CursorParameters.EndScanLine = 0;
    WRITE_GDC2_COMMAND(GDC_COMMAND_CSRFORM);
    WRITE_GDC_CSRFORM((PUCHAR)GDC2_IO_o_PARAM, &CursorParameters);

    /* PITCH */
    PitchParameters.WordsPerScanline = BYTES_PER_SCANLINE;
    WRITE_GDC2_COMMAND(GDC_COMMAND_PITCH);
    WRITE_GDC_PITCH((PUCHAR)GDC2_IO_o_PARAM, &PitchParameters);

    /* PRAM */
    RamParameters.StartingAddress = 0;
    RamParameters.Length = 1023;
    RamParameters.ImageBit = TRUE;
    RamParameters.WideDisplay = FALSE;
    WRITE_GDC2_COMMAND(GDC_COMMAND_PRAM);
    WRITE_GDC_PRAM((PUCHAR)GDC2_IO_o_PARAM, &RamParameters);

    /* ZOOM */
    ZoomParameters.DisplayZoomFactor = 0;
    ZoomParameters.WritingZoomFactor = 0;
    WRITE_GDC2_COMMAND(GDC_COMMAND_ZOOM);
    WRITE_GDC_ZOOM((PUCHAR)GDC2_IO_o_PARAM, &ZoomParameters);

    /* CSRW */
    CursorPosition.CursorAddress = 0;
    CursorPosition.DotAddress = 0;
    WRITE_GDC2_COMMAND(GDC_COMMAND_CSRW);
    WRITE_GDC_CSRW((PUCHAR)GDC2_IO_o_PARAM, &CursorPosition);

    /* Synchronize the master sync source */
    TextSync();
    TextSync();
    TextSync();
    TextSync();

    /* START */
    WRITE_GDC2_COMMAND(GDC_COMMAND_BCTRL_START);

    /* 256 colors */
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_UNPROTECT);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_PEGC_ENABLE);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_LINES_800);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_PROTECT);
    WRITE_REGISTER_USHORT((PUSHORT)(VideoMemoryI + PEGC_MMIO_MODE), PEGC_MODE_PACKED);
    WRITE_REGISTER_USHORT((PUSHORT)(VideoMemoryI + PEGC_MMIO_FRAMEBUFFER), PEGC_FB_MAP);

    /* Select the video source */
    RelayState = READ_PORT_UCHAR((PUCHAR)GRAPH_IO_i_RELAY) & ~(GRAPH_RELAY_0 | GRAPH_RELAY_1);
    RelayState |= GRAPH_VID_SRC_INTERNAL | GRAPH_SRC_GDC;
    WRITE_PORT_UCHAR((PUCHAR)GRAPH_IO_o_RELAY, RelayState);
}

static VOID
SetPaletteEntryRGB(
    _In_ ULONG Id,
    _In_ RGBQUAD Rgb)
{
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_PALETTE_INDEX, Id);
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_RED, GetRValue(Rgb));
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_GREEN, GetGValue(Rgb));
    WRITE_PORT_UCHAR((PUCHAR)GDC2_IO_o_BLUE, GetBValue(Rgb));
}

VOID
NTAPI
InitPaletteWithTable(
    _In_ PULONG Table,
    _In_ ULONG Count)
{
    ULONG i;
    PULONG Entry = Table;

    for (i = 0; i < Count; i++)
        SetPaletteEntryRGB(i, *Entry++);

    for (i = Count; i < PEGC_MAX_COLORS; i++)
        SetPaletteEntryRGB(i, VidpDefaultPalette[BV_COLOR_BLACK]);
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
    ULONG X, Y, PixelMask;
    PUCHAR FontChar = GetFontPtr(Character);

    for (Y = Top;
         Y < Top + BOOTCHAR_HEIGHT;
         ++Y, FontChar += FONT_PTR_DELTA)
    {
        for (X = Left, PixelMask = 1 << (BOOTCHAR_WIDTH - 1);
             X < Left + BOOTCHAR_WIDTH;
             ++X, PixelMask >>= 1)
        {
            if (*FontChar & PixelMask)
                SetPixel(X, Y, (UCHAR)TextColor);
            else if (BackColor < BV_COLOR_NONE)
                SetPixel(X, Y, (UCHAR)BackColor);
        }
    }
}

VOID
NTAPI
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore)
{
    PUCHAR OldPosition, NewPosition;
    ULONG PixelCount = TopDelta * SCREEN_WIDTH;

    if (Restore)
    {
        /* Restore the row by copying back the contents saved off-screen */
        OldPosition = (PUCHAR)(FrameBuffer + FB_OFFSET(0, SCREEN_HEIGHT));
        NewPosition = (PUCHAR)(FrameBuffer + FB_OFFSET(0, CurrentTop));
    }
    else
    {
        /* Preserve the row by saving its contents off-screen */
        OldPosition = (PUCHAR)(FrameBuffer + FB_OFFSET(0, CurrentTop));
        NewPosition = (PUCHAR)(FrameBuffer + FB_OFFSET(0, SCREEN_HEIGHT));
    }

    while (PixelCount--)
        WRITE_REGISTER_UCHAR(NewPosition++, READ_REGISTER_UCHAR(OldPosition++));
}

VOID
PrepareForSetPixel(VOID)
{
    NOTHING;
}

VOID
NTAPI
DoScroll(
    _In_ ULONG Scroll)
{
    USHORT i, Line;
    PUCHAR Src, Dst;
    PULONG SrcWide, DstWide;
    USHORT PixelCount = (VidpScrollRegion[2] - VidpScrollRegion[0]) + 1;
    ULONG_PTR SourceOffset = FrameBuffer + FB_OFFSET(VidpScrollRegion[0], VidpScrollRegion[1] + Scroll);
    ULONG_PTR DestinationOffset = FrameBuffer + FB_OFFSET(VidpScrollRegion[0], VidpScrollRegion[1]);

    for (Line = VidpScrollRegion[1]; Line <= VidpScrollRegion[3]; Line++)
    {
        SrcWide = (PULONG)SourceOffset;
        DstWide = (PULONG)DestinationOffset;
        for (i = 0; i < PixelCount / sizeof(ULONG); i++)
            WRITE_REGISTER_ULONG(DstWide++, READ_REGISTER_ULONG(SrcWide++));

        Src = (PUCHAR)SrcWide;
        Dst = (PUCHAR)DstWide;
        for (i = 0; i < PixelCount % sizeof(ULONG); i++)
            WRITE_REGISTER_UCHAR(Dst++, READ_REGISTER_UCHAR(Src++));

        SourceOffset += SCREEN_WIDTH;
        DestinationOffset += SCREEN_WIDTH;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    PHYSICAL_ADDRESS BaseAddress;

    BaseAddress.QuadPart = VRAM_NORMAL_PLANE_I;
    VideoMemoryI = (ULONG_PTR)MmMapIoSpace(BaseAddress, VRAM_PLANE_SIZE, MmNonCached);
    if (!VideoMemoryI)
        goto Failure;

    if (!HasPegcController())
        goto Failure;

    BaseAddress.QuadPart = PEGC_FRAMEBUFFER_PACKED;
    FrameBuffer = (ULONG_PTR)MmMapIoSpace(BaseAddress, PEGC_FRAMEBUFFER_SIZE, MmNonCached);
    if (!FrameBuffer)
        goto Failure;

    if (SetMode)
        VidResetDisplay(TRUE);

    return TRUE;

Failure:
    if (!VideoMemoryI) MmUnmapIoSpace((PVOID)VideoMemoryI, VRAM_PLANE_SIZE);
    if (!FrameBuffer) MmUnmapIoSpace((PVOID)FrameBuffer, PEGC_FRAMEBUFFER_SIZE);

    return FALSE;
}

VOID
NTAPI
VidCleanUp(VOID)
{
    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_DISPLAY_DISABLE);
}

VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN HalReset)
{
    PULONG PixelsPosition = (PULONG)(FrameBuffer + FB_OFFSET(0, 0));
    ULONG PixelCount = ((SCREEN_WIDTH * SCREEN_HEIGHT) / sizeof(ULONG)) + 1;

    /* Clear the current position */
    VidpCurrentX = 0;
    VidpCurrentY = 0;

    /* Clear the screen with HAL if we were asked to */
    if (HalReset)
        HalResetDisplay();

    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_DISPLAY_DISABLE);

    /* 640x480 256-color 31 kHz mode */
    InitializeDisplay();

    /* Re-initialize the palette and fill the screen black */
    InitializePalette();
    while (PixelCount--)
        WRITE_REGISTER_ULONG(PixelsPosition++, 0);

    WRITE_PORT_UCHAR((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_DISPLAY_ENABLE);
}

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
    ULONG X, Y;
    PUCHAR OutputBuffer;
    USHORT Px;
    PUSHORT PixelsPosition = (PUSHORT)(FrameBuffer + FB_OFFSET(Left, Top));

    /* Clear the destination buffer */
    RtlZeroMemory(Buffer, Delta * Height);

    for (Y = 0; Y < Height; Y++)
    {
        OutputBuffer = Buffer + Y * Delta;

        for (X = 0; X < Width; X += sizeof(USHORT))
        {
            Px = READ_REGISTER_USHORT(PixelsPosition++);
            *OutputBuffer++ = (FIRSTBYTE(Px) << 4) | (SECONDBYTE(Px) & 0x0F);
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
    USHORT i, Line;
    PUCHAR PixelPtr;
    PULONG PixelsPtr;
    ULONG WideColor = (Color << 24) | (Color << 16) | (Color << 8) | Color;
    USHORT PixelCount = (Right - Left) + 1;
    ULONG_PTR StartOffset = FrameBuffer + FB_OFFSET(Left, Top);

    for (Line = Top; Line <= Bottom; Line++)
    {
        PixelsPtr = (PULONG)StartOffset;
        for (i = 0; i < PixelCount / sizeof(ULONG); i++)
            WRITE_REGISTER_ULONG(PixelsPtr++, WideColor);

        PixelPtr = (PUCHAR)PixelsPtr;
        for (i = 0; i < PixelCount % sizeof(ULONG); i++)
            WRITE_REGISTER_UCHAR(PixelPtr++, Color);

        StartOffset += SCREEN_WIDTH;
    }
}
