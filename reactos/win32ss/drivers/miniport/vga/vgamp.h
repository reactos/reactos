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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef VGAMP_H
#define VGAMP_H

/* INCLUDES *******************************************************************/

#include <ntdef.h>
#include <miniport.h>
#include <video.h>

#define UNIMPLEMENTED \
   VideoPortDebugPrint(Error, "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__);

VOID
InitVGAMode(VOID);

VP_STATUS NTAPI
VGAFindAdapter(
   PVOID DeviceExtension,
   PVOID Context,
   PWSTR ArgumentString,
   PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   PUCHAR Again);

BOOLEAN NTAPI
VGAInitialize(
   PVOID DeviceExtension);

BOOLEAN NTAPI
VGAStartIO(
   PVOID DeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket);

/*static BOOLEAN NTAPI
VGAInterrupt(PVOID DeviceExtension);*/

BOOLEAN NTAPI
VGAResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows);

/*static VOID NTAPI
VGATimer(PVOID DeviceExtension);*/

/* Mandatory IoControl routines */
BOOLEAN
VGAMapVideoMemory(
   IN PVOID DeviceExtension,
   IN PVIDEO_MEMORY RequestedAddress,
   OUT PVIDEO_MEMORY_INFORMATION MapInformation,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGAQueryAvailModes(
   OUT PVIDEO_MODE_INFORMATION ReturnedModes,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGAQueryCurrentMode(
   OUT PVIDEO_MODE_INFORMATION CurrentMode,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGAQueryNumAvailModes(
   OUT PVIDEO_NUM_MODES NumberOfModes,
   OUT PSTATUS_BLOCK StatusBlock);

VOID
VGAResetDevice(OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGASetColorRegisters(
   IN PVIDEO_CLUT ColorLookUpTable,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGASetPaletteRegisters(
   IN PUSHORT PaletteRegisters,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGASetCurrentMode(
   IN PVIDEO_MODE RequestedMode,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGAShareVideoMemory(
   IN PVIDEO_SHARE_MEMORY RequestedMemory,
   OUT PVIDEO_MEMORY_INFORMATION ReturnedMemory,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGAUnmapVideoMemory(
   IN PVOID DeviceExtension,
   IN PVIDEO_MEMORY MemoryToUnmap,
   OUT PSTATUS_BLOCK StatusBlock);

BOOLEAN
VGAUnshareVideoMemory(
   IN PVIDEO_MEMORY MemoryToUnshare,
   OUT PSTATUS_BLOCK StatusBlock);

/* Optional IoControl routines */
/* None actually */

#endif /* VGAMP_H */
