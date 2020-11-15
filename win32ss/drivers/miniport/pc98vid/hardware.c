/*
 * PROJECT:     ReactOS framebuffer driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware support code
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "pc98vid.h"

/* GLOBALS ********************************************************************/

#define PEGC_MAX_COLORS    256

/* FUNCTIONS ******************************************************************/

static BOOLEAN
GraphGetStatus(
    _In_ UCHAR Status)
{
    UCHAR Result;

    VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_STATUS_SELECT, Status);
    Result = VideoPortReadPortUchar((PUCHAR)GRAPH_IO_i_STATUS);

    return (Result & GRAPH_STATUS_SET) && (Result != 0xFF);
}

static BOOLEAN
TestMmio(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension)
{
    USHORT OldValue, NewValue;

    OldValue = VideoPortReadRegisterUshort((PUSHORT)(DeviceExtension->PegcControlVa +
                                                     PEGC_MMIO_MODE));

    /* Bits [15:1] are not writable */
    VideoPortWriteRegisterUshort((PUSHORT)(DeviceExtension->PegcControlVa +
                                           PEGC_MMIO_MODE), 0x80);
    NewValue = VideoPortReadRegisterUshort((PUSHORT)(DeviceExtension->PegcControlVa +
                                                     PEGC_MMIO_MODE));

    VideoPortWriteRegisterUshort((PUSHORT)(DeviceExtension->PegcControlVa +
                                           PEGC_MMIO_MODE), OldValue);

    return !(NewValue & 0x80);
}

static VOID
TextSync(VOID)
{
    while (VideoPortReadPortUchar((PUCHAR)GDC1_IO_i_STATUS) & GDC_STATUS_VSYNC)
        NOTHING;

    while (!(VideoPortReadPortUchar((PUCHAR)GDC1_IO_i_STATUS) & GDC_STATUS_VSYNC))
        NOTHING;
}

BOOLEAN
NTAPI
HasPegcController(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension)
{
    BOOLEAN Success;

    if (GraphGetStatus(GRAPH_STATUS_PEGC))
        return TestMmio(DeviceExtension);

    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_UNPROTECT);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_PEGC_ENABLE);
    Success = GraphGetStatus(GRAPH_STATUS_PEGC) ? TestMmio(DeviceExtension) : FALSE;
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_PEGC_DISABLE);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_PROTECT);

    return Success;
}

CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidSetCurrentMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MODE RequestedMode)
{
    SYNCPARAM SyncParameters;
    CSRFORMPARAM CursorParameters;
    CSRWPARAM CursorPosition;
    PITCHPARAM PitchParameters;
    PRAMPARAM RamParameters;
    ZOOMPARAM ZoomParameters;
    UCHAR RelayState;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Mode %d\n",
                     __FUNCTION__, RequestedMode->RequestedMode));

    if (RequestedMode->RequestedMode > DeviceExtension->ModeCount)
        return ERROR_INVALID_PARAMETER;

    /* Blank screen */
    VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_DISPLAY_DISABLE);

    /* RESET, without FIFO check */
    VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_COMMAND, GDC_COMMAND_RESET1);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_COMMAND, GDC_COMMAND_RESET1);

    /* Configure chipset */
    VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_COLORED);
    VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GDC2_MODE_ODD_RLINE_SHOW);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_COLORS_16);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_GRCG);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_LCD);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_LINES_400);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2,
                            VideoModes[RequestedMode->RequestedMode].Clock1);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2,
                            VideoModes[RequestedMode->RequestedMode].Clock2);
    VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_HORIZONTAL_SCAN_RATE,
                            VideoModes[RequestedMode->RequestedMode].HorizontalScanRate);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_VIDEO_PAGE, 0);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_VIDEO_PAGE_ACCESS, 0);

    /* =========================== MASTER ============================ */

    /* MASTER */
    WRITE_GDC1_COMMAND(GDC_COMMAND_MASTER);

    /* SYNC */
    SyncParameters = VideoModes[RequestedMode->RequestedMode].TextSyncParameters;
    SyncParameters.Flags = SYNC_DISPLAY_MODE_GRAPHICS_AND_CHARACTERS | SYNC_VIDEO_FRAMING_NONINTERLACED |
                           SYNC_DRAW_ONLY_DURING_RETRACE_BLANKING | SYNC_STATIC_RAM_NO_REFRESH;
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
    PitchParameters.WordsPerScanline = 80;
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
    SyncParameters = VideoModes[RequestedMode->RequestedMode].VideoSyncParameters;
    SyncParameters.Flags = SYNC_DISPLAY_MODE_GRAPHICS | SYNC_VIDEO_FRAMING_NONINTERLACED |
                           SYNC_DRAW_DURING_ACTIVE_DISPLAY_TIME_AND_RETRACE_BLANKING |
                           SYNC_STATIC_RAM_NO_REFRESH;
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
    PitchParameters.WordsPerScanline = 80;
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

    /* 256 colors, packed pixel */
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_UNPROTECT);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_MODE_PEGC_ENABLE);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2,
                            VideoModes[RequestedMode->RequestedMode].Mem);
    VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_MODE_FLIPFLOP2, GDC2_EGC_FF_PROTECT);
    VideoPortWriteRegisterUshort((PUSHORT)(DeviceExtension->PegcControlVa +
                                           PEGC_MMIO_MODE), PEGC_MODE_PACKED);
    VideoPortWriteRegisterUshort((PUSHORT)(DeviceExtension->PegcControlVa +
                                           PEGC_MMIO_FRAMEBUFFER), PEGC_FB_MAP);

    /* Select the video source */
    RelayState = VideoPortReadPortUchar((PUCHAR)GRAPH_IO_i_RELAY) &
                 ~(GRAPH_RELAY_0 | GRAPH_RELAY_1);
    RelayState |= GRAPH_VID_SRC_INTERNAL | GRAPH_SRC_GDC;
    VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_RELAY, RelayState);

    /* Unblank screen */
    VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1, GRAPH_MODE_DISPLAY_ENABLE);

    DeviceExtension->CurrentMode = RequestedMode->RequestedMode;

    return NO_ERROR;
}

CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidSetColorRegisters(
    _In_ PVIDEO_CLUT ColorLookUpTable)
{
    USHORT Entry;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    if (ColorLookUpTable->NumEntries > PEGC_MAX_COLORS)
        return ERROR_INVALID_PARAMETER;

    for (Entry = ColorLookUpTable->FirstEntry;
         Entry < ColorLookUpTable->FirstEntry + ColorLookUpTable->NumEntries;
         ++Entry)
    {
        VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_PALETTE_INDEX, Entry);
        VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_RED,
                                ColorLookUpTable->LookupTable[Entry].RgbArray.Red);
        VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_GREEN,
                                ColorLookUpTable->LookupTable[Entry].RgbArray.Green);
        VideoPortWritePortUchar((PUCHAR)GDC2_IO_o_BLUE,
                                ColorLookUpTable->LookupTable[Entry].RgbArray.Blue);
    }

    return NO_ERROR;
}

CODE_SEG("PAGE")
VP_STATUS
NTAPI
Pc98VidGetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Id %lX, State %x\n",
                     __FUNCTION__, HwId, VideoPowerControl->PowerState));

    if (HwId == MONITOR_HW_ID || HwId == DISPLAY_ADAPTER_HW_ID)
    {
        switch (VideoPowerControl->PowerState)
        {
            case VideoPowerOn:
            case VideoPowerStandBy:
            case VideoPowerSuspend:
            case VideoPowerOff:
            case VideoPowerShutdown:
                return NO_ERROR;
        }
    }

    return ERROR_DEVICE_REINITIALIZATION_NEEDED;
}

CODE_SEG("PAGE")
VP_STATUS
NTAPI
Pc98VidSetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    UCHAR Dpms;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Id %lX, State %x\n",
                     __FUNCTION__, HwId, VideoPowerControl->PowerState));

    if (HwId == MONITOR_HW_ID)
    {
        Dpms = VideoPortReadPortUchar((PUCHAR)GRAPH_IO_i_DPMS);

        switch (VideoPowerControl->PowerState)
        {
            case VideoPowerOn:
                /* Turn on HS/VS signals */
                Dpms &= ~(GRAPH_DPMS_HSYNC_MASK | GRAPH_DPMS_VSYNC_MASK);
                VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_DPMS, Dpms);

                /* Unblank screen */
                VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1,
                                        GRAPH_MODE_DISPLAY_ENABLE);
                break;

            case VideoPowerStandBy:
                /* Disable HS signal */
                Dpms = (Dpms | GRAPH_DPMS_HSYNC_MASK) & ~GRAPH_DPMS_VSYNC_MASK;
                VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_DPMS, Dpms);
                break;

            case VideoPowerSuspend:
                /* Disable VS signal */
                Dpms = (Dpms | GRAPH_DPMS_VSYNC_MASK) & ~GRAPH_DPMS_HSYNC_MASK;
                VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_DPMS, Dpms);
                break;

            case VideoPowerOff:
            case VideoPowerShutdown:
                /* Turn off HS/VS signals */
                Dpms |= GRAPH_DPMS_HSYNC_MASK | GRAPH_DPMS_VSYNC_MASK;
                VideoPortWritePortUchar((PUCHAR)GRAPH_IO_o_DPMS, Dpms);

                /* Blank screen */
                VideoPortWritePortUchar((PUCHAR)GDC1_IO_o_MODE_FLIPFLOP1,
                                        GRAPH_MODE_DISPLAY_DISABLE);
                break;
        }
    }

    return NO_ERROR;
}
