/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Simple framebuffer driver for NVIDIA NV2A XGPU
 * COPYRIGHT:   Copyright 2004 Ge van Geldorp
 *              Copyright 2004 Filip Navara
 *              Copyright 2019 Stanislav Motylkov (x86corez@gmail.com)
 */

#pragma once

/* INCLUDES *******************************************************************/

/*
 * FIXME: specify headers properly in the triangle brackets and rearrange them
 * in a way so it would be simpler to add NDK and other headers for debugging.
 */
#include "ntdef.h"
#define PAGE_SIZE 4096
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "video.h"

#define NV2A_VIDEO_MEMORY_SIZE    (4 * 1024 * 1024)

#define NV2A_CONTROL_FRAMEBUFFER_ADDRESS_OFFSET 0x600800
#define NV2A_CRTC_REGISTER_INDEX                0x6013D4
#define NV2A_CRTC_REGISTER_VALUE                0x6013D5
#define NV2A_RAMDAC_FP_HVALID_END               0x680838
#define NV2A_RAMDAC_FP_VVALID_END               0x680818

typedef struct
{
    PHYSICAL_ADDRESS PhysControlStart;
    ULONG ControlLength;
    PVOID VirtControlStart;
    PHYSICAL_ADDRESS PhysFrameBufferStart;
} XBOXVMP_DEVICE_EXTENSION, *PXBOXVMP_DEVICE_EXTENSION;

VP_STATUS
NTAPI
XboxVmpFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PVOID HwContext,
    IN PWSTR ArgumentString,
    IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    OUT PUCHAR Again);

BOOLEAN
NTAPI
XboxVmpInitialize(
    PVOID HwDeviceExtension);

BOOLEAN
NTAPI
XboxVmpStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket);

BOOLEAN
NTAPI
XboxVmpResetHw(
    PVOID DeviceExtension,
    ULONG Columns,
    ULONG Rows);

VP_STATUS
NTAPI
XboxVmpGetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl);

VP_STATUS
NTAPI
XboxVmpSetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl);

BOOLEAN
FASTCALL
XboxVmpSetCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE RequestedMode,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpResetDevice(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpMapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY RequestedAddress,
    PVIDEO_MEMORY_INFORMATION MapInformation,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpUnmapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY VideoMemory,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpQueryNumAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_NUM_MODES Modes,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpQueryAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION ReturnedModes,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpQueryCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION VideoModeInfo,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpSetColorRegisters(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_CLUT ColorLookUpTable,
    PSTATUS_BLOCK StatusBlock);

/* EOF */
