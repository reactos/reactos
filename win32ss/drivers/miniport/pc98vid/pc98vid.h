/*
 * PROJECT:     ReactOS framebuffer driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport driver header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#ifndef _PC98VID_PCH_
#define _PC98VID_PCH_

#include <ntdef.h>
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

VP_STATUS
NTAPI
Pc98VidFindAdapter(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID HwContext,
    _In_opt_ PWSTR ArgumentString,
    _Inout_ PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    _Out_ PUCHAR Again);

BOOLEAN
NTAPI
HasPegcController(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension);

BOOLEAN
NTAPI
Pc98VidInitialize(
    _In_ PVOID HwDeviceExtension);

VP_STATUS
NTAPI
Pc98VidGetVideoChildDescriptor(
    _In_ PVOID HwDeviceExtension,
    _In_ PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    _Out_ PVIDEO_CHILD_TYPE VideoChildType,
    _Out_ PUCHAR pChildDescriptor,
    _Out_ PULONG UId,
    _Out_ PULONG pUnused);

BOOLEAN
NTAPI
Pc98VidStartIO(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVIDEO_REQUEST_PACKET RequestPacket);

VOID
FASTCALL
Pc98VidQueryMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG ModeNumber,
    _Out_ PVIDEO_MODE_INFORMATION VideoMode);

VP_STATUS
FASTCALL
Pc98VidQueryAvailModes(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_MODE_INFORMATION ModeInformation,
    _Out_ PSTATUS_BLOCK StatusBlock);

VP_STATUS
FASTCALL
Pc98VidQueryNumAvailModes(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_NUM_MODES Modes,
    _Out_ PSTATUS_BLOCK StatusBlock);

VP_STATUS
FASTCALL
Pc98VidSetCurrentMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MODE RequestedMode);

VP_STATUS
FASTCALL
Pc98VidQueryCurrentMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_MODE_INFORMATION VideoMode,
    _Out_ PSTATUS_BLOCK StatusBlock);

VP_STATUS
FASTCALL
Pc98VidMapVideoMemory(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MEMORY RequestedAddress,
    _Out_ PVIDEO_MEMORY_INFORMATION MapInformation,
    _Out_ PSTATUS_BLOCK StatusBlock);

VP_STATUS
FASTCALL
Pc98VidUnmapVideoMemory(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MEMORY VideoMemory);

VP_STATUS
FASTCALL
Pc98VidResetDevice(VOID);

VP_STATUS
FASTCALL
Pc98VidSetColorRegisters(
    _In_ PVIDEO_CLUT ColorLookUpTable);

VP_STATUS
FASTCALL
Pc98VidGetChildState(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PULONG ChildIndex,
    _Out_ PULONG ChildState,
    _Out_ PSTATUS_BLOCK StatusBlock);

VP_STATUS
NTAPI
Pc98VidGetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl);

VP_STATUS
NTAPI
Pc98VidSetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl);

extern const VIDEOMODE VideoModes[];

#endif /* _PC98VID_PCH_ */
