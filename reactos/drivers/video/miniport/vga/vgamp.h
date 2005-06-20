/*
 * ReactOS VGA miniport video driver
 *
 * Copyright (C) 2004 Filip Navara, Herve Poussineau
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

#ifndef VGAMP_H
#define VGAMP_H

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/miniport.h>
#include <ddk/video.h>
#include <ddk/ntddvdeo.h>
#include <debug.h>

void
InitVGAMode();

static VP_STATUS STDCALL
VGAFindAdapter(
   PVOID DeviceExtension,
   PVOID Context,
   PWSTR ArgumentString,
   PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   PUCHAR Again);

static BOOLEAN STDCALL
VGAInitialize(
   PVOID DeviceExtension);

static BOOLEAN STDCALL
VGAStartIO(
   PVOID DeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket);

/*static BOOLEAN STDCALL
VGAInterrupt(PVOID DeviceExtension);*/

static BOOLEAN STDCALL
VGAResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows);

/*static VOID STDCALL
VGATimer(PVOID DeviceExtension);*/

/* Mandatory IoControl routines */
BOOL
VGAMapVideoMemory(
   IN PVOID DeviceExtension,
   IN PVIDEO_MEMORY RequestedAddress,
   OUT PVIDEO_MEMORY_INFORMATION MapInformation,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAQueryAvailModes(
   OUT PVIDEO_MODE_INFORMATION ReturnedModes,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAQueryCurrentMode(
   OUT PVIDEO_MODE_INFORMATION CurrentMode,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAQueryNumAvailModes(
   OUT PVIDEO_NUM_MODES NumberOfModes,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAResetDevice(OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGASetColorRegisters(
   IN PVIDEO_CLUT ColorLookUpTable,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGASetPaletteRegisters(
   IN PWORD PaletteRegisters,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGASetCurrentMode(
   IN PVIDEO_MODE RequestedMode,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAShareVideoMemory(
   IN PVIDEO_SHARE_MEMORY RequestedMemory,
   OUT PVIDEO_MEMORY_INFORMATION ReturnedMemory,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAUnmapVideoMemory(
   IN PVOID DeviceExtension,
   IN PVIDEO_MEMORY MemoryToUnmap,
   OUT PSTATUS_BLOCK StatusBlock);

BOOL
VGAUnshareVideoMemory(
   IN PVIDEO_MEMORY MemoryToUnshare,
   OUT PSTATUS_BLOCK StatusBlock);

/* Optional IoControl routines */
/* None actually */

#endif /* VGAMP_H */
