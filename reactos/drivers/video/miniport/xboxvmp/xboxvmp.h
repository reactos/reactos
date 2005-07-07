/*
 * ReactOS Xbox miniport video driver
 * Copyright (C) 2004 G‚ van Geldorp
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

#include <ddk/ntddk.h>
#include <ddk/miniport.h>
#include <ddk/video.h>
#include <ddk/ntddvdeo.h>
#include <ndk/ntndk.h>

#define NDEBUG
#include <debug.h>

typedef struct
{
  PHYSICAL_ADDRESS PhysControlStart;
  ULONG ControlLength;
  PVOID VirtControlStart;
  PHYSICAL_ADDRESS PhysFrameBufferStart;
} XBOXVMP_DEVICE_EXTENSION, *PXBOXVMP_DEVICE_EXTENSION;

VP_STATUS STDCALL
XboxVmpFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again);

BOOLEAN STDCALL
XboxVmpInitialize(PVOID HwDeviceExtension);

BOOLEAN STDCALL
XboxVmpStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket);

BOOLEAN STDCALL
XboxVmpResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows);

VP_STATUS STDCALL
XboxVmpGetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl);

VP_STATUS STDCALL
XboxVmpSetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl);

BOOL FASTCALL
XboxVmpSetCurrentMode(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpResetDevice(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpMapVideoMemory(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress,
   PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpUnmapVideoMemory(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY VideoMemory,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpQueryNumAvailModes(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_NUM_MODES Modes,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpQueryAvailModes(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION ReturnedModes,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpQueryCurrentMode(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION VideoModeInfo,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
XboxVmpSetColorRegisters(
   PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock);

#endif /* XBOXVMP_H */

/* EOF */
