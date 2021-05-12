/*
 * PROJECT:     ReactOS framebuffer driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport driver header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#ifndef _PC98VID_PCH_
#define _PC98VID_PCH_

#include <ntdef.h>
#include <section_attribs.h>
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>
#include <video.h>
#include <debug.h>

#undef WRITE_PORT_UCHAR
#undef READ_PORT_UCHAR
#define WRITE_PORT_UCHAR(p, d) VideoPortWritePortUchar(p, d)
#define READ_PORT_UCHAR(p) VideoPortReadPortUchar(p)
#include <drivers/pc98/video.h>

#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

#define MONITOR_HW_ID 0x1033FACE /* Dummy */

typedef struct _VIDEOMODE
{
    USHORT HResolution;
    USHORT VResolution;
    UCHAR HorizontalScanRate;
    UCHAR Clock1;
    UCHAR Clock2;
    UCHAR Mem;
    UCHAR RefreshRate;
    SYNCPARAM TextSyncParameters;
    SYNCPARAM VideoSyncParameters;
} VIDEOMODE, *PVIDEOMODE;

typedef struct _HW_DEVICE_EXTENSION
{
    UCHAR MonitorCount;
    UCHAR ModeCount;
    UCHAR CurrentMode;
    PHYSICAL_ADDRESS PegcControl;
    ULONG PegcControlLength;
    ULONG_PTR PegcControlVa;
    PHYSICAL_ADDRESS FrameBuffer;
    ULONG FrameBufferLength;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

BOOLEAN
NTAPI
HasPegcController(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension);

CODE_SEG("PAGE")
BOOLEAN
NTAPI
Pc98VidStartIO(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVIDEO_REQUEST_PACKET RequestPacket);

CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidSetCurrentMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MODE RequestedMode);

CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidSetColorRegisters(
    _In_ PVIDEO_CLUT ColorLookUpTable);

CODE_SEG("PAGE")
VP_STATUS
NTAPI
Pc98VidGetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl);

CODE_SEG("PAGE")
VP_STATUS
NTAPI
Pc98VidSetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl);

DATA_SEG("PAGECONS")
extern const VIDEOMODE VideoModes[];

#endif /* _PC98VID_PCH_ */
