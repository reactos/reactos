/*
 * ReactOS Xbox miniport video driver
 * Copyright (C) 2004 Gé van Geldorp
 *
 * Based on VBE miniport video driver
 * Copyright (C) 2004 Filip Navara
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef XBOXVMP_H
#define XBOXVMP_H

/* INCLUDES *******************************************************************/

#ifdef _MSC_VER
#include "dderror.h"
#include "devioctl.h"
#define PAGE_SIZE 4096
#else
#include <ntddk.h>
#endif

#include "dderror.h"
#include "miniport.h"
#include "ntddvdeo.h"
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

typedef struct
{
  PHYSICAL_ADDRESS PhysControlStart;
  ULONG ControlLength;
  PVOID VirtControlStart;
  PHYSICAL_ADDRESS PhysFrameBufferStart;
} XBOXVMP_DEVICE_EXTENSION, *PXBOXVMP_DEVICE_EXTENSION;

VP_STATUS NTAPI
XboxVmpFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again);

BOOLEAN NTAPI
XboxVmpInitialize(PVOID HwDeviceExtension);

BOOLEAN NTAPI
XboxVmpStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket);

BOOLEAN NTAPI
XboxVmpResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows);

VP_STATUS NTAPI
XboxVmpGetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl);

VP_STATUS NTAPI
XboxVmpSetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl);

BOOLEAN FASTCALL
XboxVmpSetCurrentMode(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpResetDevice(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpMapVideoMemory(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress,
   PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpUnmapVideoMemory(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY VideoMemory,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpQueryNumAvailModes(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_NUM_MODES Modes,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpQueryAvailModes(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION ReturnedModes,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpQueryCurrentMode(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION VideoModeInfo,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
XboxVmpSetColorRegisters(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock);

#endif /* XBOXVMP_H */

/* EOF */
