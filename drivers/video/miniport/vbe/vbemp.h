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

#ifdef _MSC_VER
#include "dderror.h"
#include "devioctl.h"
#else
#include <ntddk.h>
#endif

#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_VBE TAG('V', 'B', 'E', ' ')

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
#define VBE_DDC                              0x4F15

/*
 * VBE DDC Sub-Functions
 */

#define VBE_DDC_READ_EDID                      0x01
#define VBE_DDC_REPORT_CAPABILITIES            0x10
#define VBE_DDC_BEGIN_SCL_SDA_CONTROL          0x11
#define VBE_DDC_END_SCL_SDA_CONTROL            0x12
#define VBE_DDC_WRITE_SCL_CLOCK_LINE           0x13
#define VBE_DDC_WRITE_SDA_DATA_LINE            0x14
#define VBE_DDC_READ_SCL_CLOCK_LINE            0x15
#define VBE_DDC_READ_SDA_DATA_LINE             0x16

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

#define VBE_GETRETURNCODE(x) (x & 0xFFFF)

/*
 * VBE specification defined structure for general adapter info
 * returned by function VBE_GET_CONTROLLER_INFORMATION command.
 */

typedef struct
{
   CHAR Signature[4];
   USHORT Version;
   ULONG OemStringPtr;
   LONG Capabilities;
   ULONG VideoModePtr;
   USHORT TotalMemory;
   USHORT OemSoftwareRevision;
   ULONG OemVendorNamePtr;
   ULONG OemProductNamePtr;
   ULONG OemProductRevPtr;
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
   USHORT ModeAttributes;
   UCHAR WinAAttributes;
   UCHAR WinBAttributes;
   USHORT WinGranularity;
   USHORT WinSize;
   USHORT WinASegment;
   USHORT WinBSegment;
   ULONG WinFuncPtr;
   USHORT BytesPerScanLine;

   /* Mandatory information for VBE 1.2 and above */
   USHORT XResolution;
   USHORT YResolution;
   UCHAR XCharSize;
   UCHAR YCharSize;
   UCHAR NumberOfPlanes;
   UCHAR BitsPerPixel;
   UCHAR NumberOfBanks;
   UCHAR MemoryModel;
   UCHAR BankSize;
   UCHAR NumberOfImagePages;
   UCHAR Reserved1;

   /* Direct Color fields (required for Direct/6 and YUV/7 memory models) */
   UCHAR RedMaskSize;
   UCHAR RedFieldPosition;
   UCHAR GreenMaskSize;
   UCHAR GreenFieldPosition;
   UCHAR BlueMaskSize;
   UCHAR BlueFieldPosition;
   UCHAR ReservedMaskSize;
   UCHAR ReservedFieldPosition;
   UCHAR DirectColorModeInfo;

   /* Mandatory information for VBE 2.0 and above */
   ULONG PhysBasePtr;
   ULONG Reserved2;
   USHORT Reserved3;

   /* Mandatory information for VBE 3.0 and above */
   USHORT LinBytesPerScanLine;
   UCHAR BnkNumberOfImagePages;
   UCHAR LinNumberOfImagePages;
   UCHAR LinRedMaskSize;
   UCHAR LinRedFieldPosition;
   UCHAR LinGreenMaskSize;
   UCHAR LinGreenFieldPosition;
   UCHAR LinBlueMaskSize;
   UCHAR LinBlueFieldPosition;
   UCHAR LinReservedMaskSize;
   UCHAR LinReservedFieldPosition;
   ULONG MaxPixelClock;

   CHAR Reserved4[189];
} VBE_MODEINFO, *PVBE_MODEINFO;

#define MAX_SIZE_OF_EDID 256

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
   USHORT *ModeNumbers;
   PVBE_MODEINFO ModeInfo;
   USHORT CurrentMode;

   /* Current child been enumerated */
   ULONG CurrentChildIndex;
} VBE_DEVICE_EXTENSION, *PVBE_DEVICE_EXTENSION;

/* edid.c */

VP_STATUS NTAPI
VBEGetVideoChildDescriptor(
   IN PVOID HwDeviceExtension,
   IN PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
   OUT PVIDEO_CHILD_TYPE VideoChildType,
   OUT PUCHAR pChildDescriptor,
   OUT PULONG UId,
   OUT PULONG pUnused);

/* vbemp.c */
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

BOOLEAN FASTCALL
VBESetCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBEResetDevice(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBEMapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress,
   PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBEUnmapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY VideoMemory,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBEQueryNumAvailModes(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_NUM_MODES Modes,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBEQueryAvailModes(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION ReturnedModes,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBEQueryCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION VideoModeInfo,
   PSTATUS_BLOCK StatusBlock);

BOOLEAN FASTCALL
VBESetColorRegisters(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock);

#endif /* VBEMP_H */
