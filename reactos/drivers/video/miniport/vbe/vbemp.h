/*
 * ReactOS VBE miniport video driver
 *
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

#ifndef VBEMP_H
#define VBEMP_H

/* INCLUDES *******************************************************************/

#include "stddef.h"
#include "windef.h"
#include "wingdi.h"
#include <ddk/miniport.h>
#include <ddk/video.h>
#include <ddk/ntddvdeo.h>
#include <ddk/ntapi.h>

#ifdef DBG
#define DPRINT(arg) DbgPrint arg;
#else
#define DPRINT(arg)
#endif

/*
 * Compile-time define to get VBE 1.2 support. The implementation
 * is far from complete now and so it's left undefined.
 */

/* #define VBE12_SUPPORT */

#include <pshpack1.h>

/*
 * VBE Command Definitions
 */

#define VBE_GET_CONTROLLER_INFORMATION       0x4F00
#define VBE_GET_MODE_INFORMATION             0x4F01
#define VBE_SET_VBE_MODE                     0x4F02
#define VBE_GET_CURRENT_VBE_MODE             0x4F03
#define VBE_SAVE_RESTORE_STATE               0x4F04
#define VBE_DISPLAY_WINDOW_CONTROL           0x4F05
#define VBE_SET_GET_LOGICAL_SCAN_LINE_LENGTH 0x4F06
#define VBE_SET_GET_DISPLAY_START            0x4F07 
#define VBE_SET_GET_DAC_PALETTE_FORMAT       0x4F08
#define VBE_SET_GET_PALETTE_DATA             0x4F09

/* VBE 2.0+ */
#define VBE_RETURN_PROTECTED_MODE_INTERFACE  0x4F0A 
#define VBE_GET_SET_PIXEL_CLOCK              0x4F0B

/* Extensions */
#define VBE_POWER_MANAGEMENT_EXTENSIONS      0x4F10
#define VBE_FLAT_PANEL_INTERFACE_EXTENSIONS  0x4F11
#define VBE_AUDIO_INTERFACE_EXTENSIONS       0x4F12 
#define VBE_OEM_EXTENSIONS                   0x4F13
#define VBE_DISPLAY_DATA_CHANNEL             0x4F14

/*
 * VBE Video Mode Information Definitions
 */

#define VBE_MODEATTR_LINEAR                    0x80

#define VBE_MEMORYMODEL_PACKEDPIXEL            0x04
#define VBE_MEMORYMODEL_DIRECTCOLOR            0x06

/*
 * VBE Return Codes
 */

#define VBE_SUCCESS                            0x4F 
#define VBE_UNSUCCESSFUL                      0x14F
#define VBE_NOT_SUPPORTED                     0x24F
#define VBE_FUNCTION_INVALID                  0x34F

/*
 * VBE specification defined structure for general adapter info
 * returned by function VBE_GET_CONTROLLER_INFORMATION command.
 */

typedef struct
{
   CHAR Signature[4];
   WORD Version;
   DWORD OemStringPtr;
   LONG Capabilities;
   DWORD VideoModePtr;
   WORD TotalMemory;
   WORD OemSoftwareRevision;
   DWORD OemVendorNamePtr;
   DWORD OemProductNamePtr;
   DWORD OemProductRevPtr;
   CHAR Reserved[222];
   CHAR OemData[256];
} VBE_INFO, *PVBE_INFO;

/*
 * VBE specification defined structure for specific video mode
 * info returned by function VBE_GET_MODE_INFORMATION command.
 */

typedef struct
{
   /* Mandatory information for all VBE revisions */
   WORD ModeAttributes;
   BYTE WinAAttributes;
   BYTE WinBAttributes;
   WORD WinGranularity;
   WORD WinSize;
   WORD WinASegment;
   WORD WinBSegment;
   DWORD WinFuncPtr;
   WORD BytesPerScanLine;

   /* Mandatory information for VBE 1.2 and above */
   WORD XResolution;
   WORD YResolution;
   BYTE XCharSize;
   BYTE YCharSize;
   BYTE NumberOfPlanes;
   BYTE BitsPerPixel;
   BYTE NumberOfBanks;
   BYTE MemoryModel;
   BYTE BankSize;
   BYTE NumberOfImagePages;
   BYTE Reserved1;

   /* Direct Color fields (required for Direct/6 and YUV/7 memory models) */
   BYTE RedMaskSize;
   BYTE RedFieldPosition;
   BYTE GreenMaskSize;
   BYTE GreenFieldPosition;
   BYTE BlueMaskSize;
   BYTE BlueFieldPosition;
   BYTE ReservedMaskSize;
   BYTE ReservedFieldPosition;
   BYTE DirectColorModeInfo;

   /* Mandatory information for VBE 2.0 and above */
   DWORD PhysBasePtr;
   DWORD Reserved2;
   WORD Reserved3;

   /* Mandatory information for VBE 3.0 and above */
   WORD LinBytesPerScanLine;
   BYTE BnkNumberOfImagePages;
   BYTE LinNumberOfImagePages;
   BYTE LinRedMaskSize;
   BYTE LinRedFieldPosition;
   BYTE LinGreenMaskSize;
   BYTE LinGreenFieldPosition;
   BYTE LinBlueMaskSize;
   BYTE LinBlueFieldPosition;
   BYTE LinReservedMaskSize;
   BYTE LinReservedFieldPosition;
   DWORD MaxPixelClock;

   CHAR Reserved4[189];
} VBE_MODEINFO, *PVBE_MODEINFO;

#include <poppack.h>

typedef struct
{
   /* Interface to Int10 calls */
   VIDEO_PORT_INT10_INTERFACE Int10Interface;

   /* Trampoline memory for communication with VBE real-mode interface */
   USHORT TrampolineMemorySegment;
   USHORT TrampolineMemoryOffset;

   /* General controller/BIOS information */
   VBE_INFO VbeInfo;

   /* Saved information about video modes */
   ULONG ModeCount;
   WORD *ModeNumbers;
   PVBE_MODEINFO ModeInfo;
   WORD CurrentMode;
} VBE_DEVICE_EXTENSION, *PVBE_DEVICE_EXTENSION;

VP_STATUS STDCALL
VBEFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again);

BOOLEAN STDCALL
VBEInitialize(PVOID HwDeviceExtension);

BOOLEAN STDCALL
VBEStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket);

BOOLEAN STDCALL
VBEResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows);

VP_STATUS STDCALL
VBEGetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl);

VP_STATUS STDCALL
VBESetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl);

BOOL FASTCALL
VBESetCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
VBEResetDevice(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
VBEMapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress,
   PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
VBEUnmapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY VideoMemory,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
VBEQueryNumAvailModes(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_NUM_MODES Modes,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
VBEQueryAvailModes(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION ReturnedModes,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL  
VBEQueryCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION VideoModeInfo,
   PSTATUS_BLOCK StatusBlock);

BOOL FASTCALL
VBESetColorRegisters(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock);

#endif /* VBEMP_H */
