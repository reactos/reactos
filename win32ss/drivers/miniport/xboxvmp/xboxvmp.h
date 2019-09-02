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

#include "ntdef.h"
#define PAGE_SIZE 4096
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "video.h"

/* FIXME: NDK not compatible with miniport drivers */
#define SystemBasicInformation 0
typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG MinimumUserModeAddress;
    ULONG MaximumUserModeAddress;
    KAFFINITY ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

LONG
__stdcall
ZwQuerySystemInformation(
    IN ULONG SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

#define I2C_IO_BASE 0xC000
#define NV2A_CONTROL_FRAMEBUFFER_ADDRESS_OFFSET 0x600800

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
