/*
 * ReactOS VBE miniport video driver
 * Copyright (C) 2004 Filip Navara
 *
 * Power Management and VBE 1.2 support
 * Copyright (C) 2004 Magnus Olsen
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
 *
 * TODO:
 * - Check input parameters everywhere.
 * - Call VideoPortVerifyAccessRanges to reserve the memory we're about
 *   to map.
 */

/* INCLUDES *******************************************************************/

#include "vbemp.h"

#include <devioctl.h>

#undef LOWORD
#undef HIWORD
#define LOWORD(l)	((USHORT)((ULONG_PTR)(l)))
#define HIWORD(l)	((USHORT)(((ULONG_PTR)(l)>>16)&0xFFFF))

VIDEO_ACCESS_RANGE VBEAccessRange[] =
{
    { {{0x3b0}}, 0x3bb - 0x3b0 + 1, 1, 1, 0 },
    { {{0x3c0}}, 0x3df - 0x3c0 + 1, 1, 1, 0 },
    { {{0xa0000}}, 0x20000, 0, 1, 0 },
};

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

ULONG NTAPI
DriverEntry(IN PVOID Context1, IN PVOID Context2)
{
   VIDEO_HW_INITIALIZATION_DATA InitData;

   VideoPortZeroMemory(&InitData, sizeof(InitData));
   InitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
   InitData.HwFindAdapter = VBEFindAdapter;
   InitData.HwInitialize = VBEInitialize;
   InitData.HwStartIO = VBEStartIO;
   InitData.HwResetHw = VBEResetHw;
   InitData.HwGetPowerState = VBEGetPowerState;
   InitData.HwSetPowerState = VBESetPowerState;
   InitData.HwGetVideoChildDescriptor = VBEGetVideoChildDescriptor;
   InitData.HwDeviceExtensionSize = sizeof(VBE_DEVICE_EXTENSION);
   InitData.HwLegacyResourceList = VBEAccessRange;
   InitData.HwLegacyResourceCount = ARRAYSIZE(VBEAccessRange);

   return VideoPortInitialize(Context1, Context2, &InitData, NULL);
}

/*
 * VBEFindAdapter
 *
 * Should detect a VBE compatible display adapter, but it's not possible
 * to use video port Int 10 services at this time during initialization,
 * so we always return NO_ERROR and do the real work in VBEInitialize.
 */

VP_STATUS NTAPI
VBEFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again)
{
   if (VideoPortIsNoVesa())
       return ERROR_DEV_NOT_EXIST;

   if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
       return ERROR_INVALID_PARAMETER;

   ConfigInfo->VdmPhysicalVideoMemoryAddress = VBEAccessRange[2].RangeStart;
   ConfigInfo->VdmPhysicalVideoMemoryLength = VBEAccessRange[2].RangeLength;
   return NO_ERROR;
}

/*
 * VBESortModesCallback
 *
 * Helper function for sorting video mode list.
 */

static int
VBESortModesCallback(PVBE_MODEINFO VbeModeInfoA, PVBE_MODEINFO VbeModeInfoB)
{
   /*
    * FIXME: Until some reasonable method for changing video modes will
    * be available we favor more bits per pixel. It should be changed
    * later.
    */
   if (VbeModeInfoA->BitsPerPixel < VbeModeInfoB->BitsPerPixel) return -1;
   if (VbeModeInfoA->BitsPerPixel > VbeModeInfoB->BitsPerPixel) return 1;
   if (VbeModeInfoA->XResolution < VbeModeInfoB->XResolution) return -1;
   if (VbeModeInfoA->XResolution > VbeModeInfoB->XResolution) return 1;
   if (VbeModeInfoA->YResolution < VbeModeInfoB->YResolution) return -1;
   if (VbeModeInfoA->YResolution > VbeModeInfoB->YResolution) return 1;
   return 0;
}

/*
 * VBESortModes
 *
 * Simple function for sorting the video mode list. Uses bubble sort.
 */

VOID FASTCALL
VBESortModes(PVBE_DEVICE_EXTENSION DeviceExtension)
{
   BOOLEAN Finished = FALSE;
   ULONG Pos;
   int Result;
   VBE_MODEINFO TempModeInfo;
   USHORT TempModeNumber;

   while (!Finished)
   {
      Finished = TRUE;
      for (Pos = 0; Pos < DeviceExtension->ModeCount - 1; Pos++)
      {
         Result = VBESortModesCallback(
            DeviceExtension->ModeInfo + Pos,
            DeviceExtension->ModeInfo + Pos + 1);
         if (Result > 0)
         {
            Finished = FALSE;

            VideoPortMoveMemory(
               &TempModeInfo,
               DeviceExtension->ModeInfo + Pos,
               sizeof(VBE_MODEINFO));
            TempModeNumber = DeviceExtension->ModeNumbers[Pos];

            VideoPortMoveMemory(
               DeviceExtension->ModeInfo + Pos,
               DeviceExtension->ModeInfo + Pos + 1,
               sizeof(VBE_MODEINFO));
            DeviceExtension->ModeNumbers[Pos] =
               DeviceExtension->ModeNumbers[Pos + 1];

            VideoPortMoveMemory(
               DeviceExtension->ModeInfo + Pos + 1,
               &TempModeInfo,
               sizeof(VBE_MODEINFO));
            DeviceExtension->ModeNumbers[Pos + 1] = TempModeNumber;
         }
      }
   }
}

/*
 * VBEInitialize
 *
 * Performs the first initialization of the adapter, after the HAL has given
 * up control of the video hardware to the video port driver.
 *
 * This function performs these steps:
 * - Gets global VBE information and finds if VBE BIOS is present.
 * - Builds the internal mode list using the list of modes provided by
 *   the VBE.
 */

BOOLEAN NTAPI
VBEInitialize(PVOID HwDeviceExtension)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   VP_STATUS Status;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;
   ULONG Length;
   ULONG ModeCount;
   ULONG SuitableModeCount;
   USHORT ModeTemp;
   ULONG CurrentMode;
   PVBE_MODEINFO VbeModeInfo;

   if (VideoPortIsNoVesa())
   {
      VBEDeviceExtension->Int10Interface.Version = 0;
      VBEDeviceExtension->Int10Interface.Size = 0;
      return FALSE;
   }

   /*
    * Get the Int 10 interface that we will use for allocating real
    * mode memory and calling the video BIOS.
    */

   VBEDeviceExtension->Int10Interface.Version = VIDEO_PORT_INT10_INTERFACE_VERSION_1;
   VBEDeviceExtension->Int10Interface.Size = sizeof(VIDEO_PORT_INT10_INTERFACE);
   Status = VideoPortQueryServices(
      HwDeviceExtension,
      VideoPortServicesInt10,
      (PINTERFACE)&VBEDeviceExtension->Int10Interface);

   if (Status != NO_ERROR)
   {
      VideoPortDebugPrint(Error, "Failed to get Int 10 service functions (Status %x)\n", Status);
      return FALSE;
   }

   /*
    * Allocate a bit of memory that will be later used for VBE transport
    * buffer. This memory must be accessible from V86 mode so it must fit
    * in the first megabyte of physical memory.
    */

   Length = 0x400;
   Status = VBEDeviceExtension->Int10Interface.Int10AllocateBuffer(
      VBEDeviceExtension->Int10Interface.Context,
      &VBEDeviceExtension->TrampolineMemorySegment,
      &VBEDeviceExtension->TrampolineMemoryOffset,
      &Length);

   if (Status != NO_ERROR)
   {
      VideoPortDebugPrint(Error, "Failed to allocate virtual memory (Status %x)\n", Status);
      return FALSE;
   }

   /*
    * Get the VBE general information.
    */

   VBEDeviceExtension->Int10Interface.Int10WriteMemory(
      VBEDeviceExtension->Int10Interface.Context,
      VBEDeviceExtension->TrampolineMemorySegment,
      VBEDeviceExtension->TrampolineMemoryOffset,
      "VBE2",
      4);

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_GET_CONTROLLER_INFORMATION;
   BiosRegisters.Edi = VBEDeviceExtension->TrampolineMemoryOffset;
   BiosRegisters.SegEs = VBEDeviceExtension->TrampolineMemorySegment;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   if (VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS)
   {
      VBEDeviceExtension->Int10Interface.Int10ReadMemory(
         VBEDeviceExtension->Int10Interface.Context,
         VBEDeviceExtension->TrampolineMemorySegment,
         VBEDeviceExtension->TrampolineMemoryOffset,
         &VBEDeviceExtension->VbeInfo,
         sizeof(VBEDeviceExtension->VbeInfo));

      /* Verify the VBE signature. */
      if (VideoPortCompareMemory(VBEDeviceExtension->VbeInfo.Signature, "VESA", 4) != 4)
      {
         VideoPortDebugPrint(Error, "No VBE BIOS present\n");
         return FALSE;
      }

      VideoPortDebugPrint(Trace, "VBE BIOS Present (%d.%d, %8ld Kb)\n",
         VBEDeviceExtension->VbeInfo.Version / 0x100,
         VBEDeviceExtension->VbeInfo.Version & 0xFF,
         VBEDeviceExtension->VbeInfo.TotalMemory * 64);

#ifdef VBE12_SUPPORT
      if (VBEDeviceExtension->VbeInfo.Version < 0x102)
#else
      if (VBEDeviceExtension->VbeInfo.Version < 0x200)
#endif
      {
         VideoPortDebugPrint(Error, "VBE BIOS present, but incompatible version %d.%d\n",
                             VBEDeviceExtension->VbeInfo.Version / 0x100,
                             VBEDeviceExtension->VbeInfo.Version & 0xFF);
         return FALSE;
      }
   }
   else
   {
      VideoPortDebugPrint(Error, "No VBE BIOS found.\n");
      return FALSE;
   }

   /*
    * Build a mode list here that can be later used by
    * IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES and IOCTL_VIDEO_QUERY_AVAIL_MODES
    * calls.
    */

   /*
    * Get the number of supported video modes.
    *
    * No need to be map the memory. It's either in the video BIOS memory or
    * in our trampoline memory. In either case the memory is already mapped.
    */

   for (ModeCount = 0; ; ModeCount++)
   {
      /* Read the VBE mode number. */
      VBEDeviceExtension->Int10Interface.Int10ReadMemory(
         VBEDeviceExtension->Int10Interface.Context,
         HIWORD(VBEDeviceExtension->VbeInfo.VideoModePtr),
         LOWORD(VBEDeviceExtension->VbeInfo.VideoModePtr) + (ModeCount << 1),
         &ModeTemp,
         sizeof(ModeTemp));

      /* End of list? */
      if (ModeTemp == 0xFFFF || ModeTemp == 0)
         break;
   }

   /*
    * Allocate space for video modes information.
    */

   VBEDeviceExtension->ModeInfo =
      VideoPortAllocatePool(HwDeviceExtension, VpPagedPool, ModeCount * sizeof(VBE_MODEINFO), TAG_VBE);
   VBEDeviceExtension->ModeNumbers =
      VideoPortAllocatePool(HwDeviceExtension, VpPagedPool, ModeCount * sizeof(USHORT), TAG_VBE);

   /*
    * Get the actual mode infos.
    */

   for (CurrentMode = 0, SuitableModeCount = 0;
        CurrentMode < ModeCount;
        CurrentMode++)
   {
      /* Read the VBE mode number. */
      VBEDeviceExtension->Int10Interface.Int10ReadMemory(
         VBEDeviceExtension->Int10Interface.Context,
         HIWORD(VBEDeviceExtension->VbeInfo.VideoModePtr),
         LOWORD(VBEDeviceExtension->VbeInfo.VideoModePtr) + (CurrentMode << 1),
         &ModeTemp,
         sizeof(ModeTemp));

      /* Call VBE BIOS to read the mode info. */
      VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
      BiosRegisters.Eax = VBE_GET_MODE_INFORMATION;
      BiosRegisters.Ecx = ModeTemp;
      BiosRegisters.Edi = VBEDeviceExtension->TrampolineMemoryOffset + 0x200;
      BiosRegisters.SegEs = VBEDeviceExtension->TrampolineMemorySegment;
      VBEDeviceExtension->Int10Interface.Int10CallBios(
         VBEDeviceExtension->Int10Interface.Context,
         &BiosRegisters);

      /* Read the VBE mode info. */
      VBEDeviceExtension->Int10Interface.Int10ReadMemory(
         VBEDeviceExtension->Int10Interface.Context,
         VBEDeviceExtension->TrampolineMemorySegment,
         VBEDeviceExtension->TrampolineMemoryOffset + 0x200,
         VBEDeviceExtension->ModeInfo + SuitableModeCount,
         sizeof(VBE_MODEINFO));

      VbeModeInfo = VBEDeviceExtension->ModeInfo + SuitableModeCount;

      /* Is this mode acceptable? */
      if (VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS &&
          VbeModeInfo->XResolution >= 640 &&
          VbeModeInfo->YResolution >= 480 &&
          (VbeModeInfo->MemoryModel == VBE_MEMORYMODEL_PACKEDPIXEL ||
           VbeModeInfo->MemoryModel == VBE_MEMORYMODEL_DIRECTCOLOR) &&
          VbeModeInfo->PhysBasePtr != 0)
      {
         if (VbeModeInfo->ModeAttributes & VBE_MODEATTR_LINEAR)
         {
            /* Bit 15 14 13 12 | 11 10 9 8 | 7 6 5 4 | 3 2 1 0 */
             // if (ModeTemp & 0x4000)
             //{
                VBEDeviceExtension->ModeNumbers[SuitableModeCount] = ModeTemp | 0x4000;
                SuitableModeCount++;
             //}
         }
#ifdef VBE12_SUPPORT
         else
         {
            VBEDeviceExtension->ModeNumbers[SuitableModeCount] = ModeTemp;
            SuitableModeCount++;
         }
#endif
      }
   }


   if (SuitableModeCount == 0)
   {

      VideoPortDebugPrint(Warn, "VBEMP: No video modes supported\n");
      return FALSE;
   }

   VBEDeviceExtension->ModeCount = SuitableModeCount;

   /*
    * Sort the video mode list according to resolution and bits per pixel.
    */

   VBESortModes(VBEDeviceExtension);

   /*
    * Print the supported video modes.
    */

   for (CurrentMode = 0;
        CurrentMode < SuitableModeCount;
        CurrentMode++)
   {
      VideoPortDebugPrint(Trace, "%dx%dx%d\n",
         VBEDeviceExtension->ModeInfo[CurrentMode].XResolution,
         VBEDeviceExtension->ModeInfo[CurrentMode].YResolution,
         VBEDeviceExtension->ModeInfo[CurrentMode].BitsPerPixel);
   }

   /*
    * Enumerate our children.
    */
   VideoPortEnumerateChildren(HwDeviceExtension, NULL);

   return TRUE;
}

/*
 * VBEStartIO
 *
 * Processes the specified Video Request Packet.
 */

BOOLEAN NTAPI
VBEStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket)
{
   BOOLEAN Result;

   RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;

   switch (RequestPacket->IoControlCode)
   {
      case IOCTL_VIDEO_SET_CURRENT_MODE:
         if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBESetCurrentMode(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MODE)RequestPacket->InputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_RESET_DEVICE:
         Result = VBEResetDevice(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
         if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION) ||
             RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEMapVideoMemory(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MEMORY)RequestPacket->InputBuffer,
            (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
         if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEUnmapVideoMemory(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MEMORY)RequestPacket->InputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
         if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEQueryNumAvailModes(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_QUERY_AVAIL_MODES:
         if (RequestPacket->OutputBufferLength <
             ((PVBE_DEVICE_EXTENSION)HwDeviceExtension)->ModeCount * sizeof(VIDEO_MODE_INFORMATION))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEQueryAvailModes(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_SET_COLOR_REGISTERS:
         if (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) ||
             RequestPacket->InputBufferLength <
             (((PVIDEO_CLUT)RequestPacket->InputBuffer)->NumEntries * sizeof(ULONG)) +
             FIELD_OFFSET(VIDEO_CLUT, LookupTable))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBESetColorRegisters(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_CLUT)RequestPacket->InputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_QUERY_CURRENT_MODE:
         if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEQueryCurrentMode(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
            RequestPacket->StatusBlock);
         break;

      default:
         RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
         return FALSE;
   }

   if (Result)
      RequestPacket->StatusBlock->Status = NO_ERROR;

   return TRUE;
}

/*
 * VBEResetHw
 *
 * This function is called to reset the hardware to a known state.
 */

BOOLEAN NTAPI
VBEResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows)
{
   /* Return FALSE to let HAL reset the display with INT10 */
   return FALSE;
}

/*
 * VBEGetPowerState
 *
 * Queries whether the device can support the requested power state.
 */

VP_STATUS NTAPI
VBEGetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   if (HwId != DISPLAY_ADAPTER_HW_ID ||
       VideoPowerControl->Length < sizeof(VIDEO_POWER_MANAGEMENT))
      return ERROR_INVALID_FUNCTION;

   /*
    * Get general power support information.
    */

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_POWER_MANAGEMENT_EXTENSIONS;
   BiosRegisters.Ebx = 0;
   BiosRegisters.Edi = 0;
   BiosRegisters.SegEs = 0;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   if ( VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_NOT_SUPPORTED)
      return ERROR_DEV_NOT_EXIST;
   if (VBE_GETRETURNCODE(BiosRegisters.Eax) != VBE_SUCCESS)
      return ERROR_INVALID_FUNCTION;

   /*
    * Get current power state.
    */

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_POWER_MANAGEMENT_EXTENSIONS;
   BiosRegisters.Ebx = 0x2;
   BiosRegisters.Edi = 0;
   BiosRegisters.SegEs = 0;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   if (VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS)
   {
      VideoPowerControl->DPMSVersion = BiosRegisters.Ebx & 0xFF;
      switch (BiosRegisters.Ebx >> 8)
      {
         case 0: VideoPowerControl->PowerState = VideoPowerOn; break;
         case 1: VideoPowerControl->PowerState = VideoPowerStandBy; break;
         case 2: VideoPowerControl->PowerState = VideoPowerSuspend; break;
         case 4: VideoPowerControl->PowerState = VideoPowerOff; break;
         case 5: VideoPowerControl->PowerState = VideoPowerOn; break;
         default: VideoPowerControl->PowerState = VideoPowerUnspecified;
      }

      return NO_ERROR;
   }

   return ERROR_DEV_NOT_EXIST;
}

/*
 * VBESetPowerState
 *
 * Sets the power state of the specified device
 */

VP_STATUS NTAPI
VBESetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   if (HwId != DISPLAY_ADAPTER_HW_ID ||
       VideoPowerControl->Length < sizeof(VIDEO_POWER_MANAGEMENT) ||
       VideoPowerControl->PowerState < VideoPowerOn ||
       VideoPowerControl->PowerState > VideoPowerHibernate)
      return ERROR_INVALID_FUNCTION;

   if (VideoPowerControl->PowerState == VideoPowerHibernate)
      return NO_ERROR;

   /*
    * Set current power state.
    */

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_POWER_MANAGEMENT_EXTENSIONS;
   BiosRegisters.Ebx = 1;
   BiosRegisters.Edi = 0;
   BiosRegisters.SegEs = 0;
   switch (VideoPowerControl->PowerState)
   {
      case VideoPowerStandBy: BiosRegisters.Ebx |= 0x100; break;
      case VideoPowerSuspend: BiosRegisters.Ebx |= 0x200; break;
      case VideoPowerOff: BiosRegisters.Ebx |= 0x400; break;
   }

   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   if (VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_NOT_SUPPORTED)
      return ERROR_DEV_NOT_EXIST;
   if (VBE_GETRETURNCODE(BiosRegisters.Eax) != VBE_SUCCESS)
      return ERROR_INVALID_FUNCTION;

   return VBE_SUCCESS;
}

/*
 * VBESetCurrentMode
 *
 * Sets the adapter to the specified operating mode.
 */

BOOLEAN FASTCALL
VBESetCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode,
   PSTATUS_BLOCK StatusBlock)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;

   if (RequestedMode->RequestedMode >= DeviceExtension->ModeCount)
   {
      return ERROR_INVALID_PARAMETER;
   }

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_SET_VBE_MODE;
   BiosRegisters.Ebx = DeviceExtension->ModeNumbers[RequestedMode->RequestedMode];
   DeviceExtension->Int10Interface.Int10CallBios(
      DeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   if (VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS)
   {
      DeviceExtension->CurrentMode = RequestedMode->RequestedMode;
   }
   else
   {
      VideoPortDebugPrint(Error, "VBEMP: VBESetCurrentMode failed (%x)\n", BiosRegisters.Eax);
      DeviceExtension->CurrentMode = -1;
   }

   return VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS;
}

/*
 * VBEResetDevice
 *
 * Resets the video hardware to the default mode, to which it was initialized
 * at system boot.
 */

BOOLEAN FASTCALL
VBEResetDevice(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_SET_VBE_MODE;
   BiosRegisters.Ebx = 0x3;
   DeviceExtension->Int10Interface.Int10CallBios(
      DeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   return VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS;
}

/*
 * VBEMapVideoMemory
 *
 * Maps the video hardware frame buffer and video RAM into the virtual address
 * space of the requestor.
 */

BOOLEAN FASTCALL
VBEMapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress,
   PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock)
{
   PHYSICAL_ADDRESS FrameBuffer;
   ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;

   StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);

   if (DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].ModeAttributes &
       VBE_MODEATTR_LINEAR)
   {
      FrameBuffer.QuadPart =
         DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].PhysBasePtr;
      MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
      if (DeviceExtension->VbeInfo.Version < 0x300)
      {
         MapInformation->VideoRamLength =
            DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].BytesPerScanLine *
            DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].YResolution;
      }
      else
      {
         MapInformation->VideoRamLength =
            DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].LinBytesPerScanLine *
            DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].YResolution;
      }
   }
#ifdef VBE12_SUPPORT
   else
   {
      FrameBuffer.QuadPart = 0xA0000;
      MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
      MapInformation->VideoRamLength = 0x10000;
   }
#endif

   VideoPortMapMemory(DeviceExtension, FrameBuffer,
      &MapInformation->VideoRamLength, &inIoSpace,
      &MapInformation->VideoRamBase);

   MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
   MapInformation->FrameBufferLength = MapInformation->VideoRamLength;

   return TRUE;
}

/*
 * VBEUnmapVideoMemory
 *
 * Releases a mapping between the virtual address space and the adapter's
 * frame buffer and video RAM.
 */

BOOLEAN FASTCALL
VBEUnmapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY VideoMemory,
   PSTATUS_BLOCK StatusBlock)
{
   VideoPortUnmapMemory(DeviceExtension, VideoMemory->RequestedVirtualAddress,
      NULL);
   return TRUE;
}

/*
 * VBEQueryNumAvailModes
 *
 * Returns the number of video modes supported by the adapter and the size
 * in bytes of the video mode information, which can be used to allocate a
 * buffer for an IOCTL_VIDEO_QUERY_AVAIL_MODES request.
 */

BOOLEAN FASTCALL
VBEQueryNumAvailModes(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_NUM_MODES Modes,
   PSTATUS_BLOCK StatusBlock)
{
   Modes->NumModes = DeviceExtension->ModeCount;
   Modes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
   StatusBlock->Information = sizeof(VIDEO_NUM_MODES);
   return TRUE;
}

/*
 * VBEQueryMode
 *
 * Returns information about one particular video mode.
 */

VOID FASTCALL
VBEQueryMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION VideoMode,
   ULONG VideoModeId)
{
   PVBE_MODEINFO VBEMode = &DeviceExtension->ModeInfo[VideoModeId];
   ULONG dpi;

   VideoMode->Length = sizeof(VIDEO_MODE_INFORMATION);
   VideoMode->ModeIndex = VideoModeId;
   VideoMode->VisScreenWidth = VBEMode->XResolution;
   VideoMode->VisScreenHeight = VBEMode->YResolution;
   if (DeviceExtension->VbeInfo.Version < 0x300)
      VideoMode->ScreenStride = VBEMode->BytesPerScanLine;
   else
      VideoMode->ScreenStride = VBEMode->LinBytesPerScanLine;
   VideoMode->NumberOfPlanes = VBEMode->NumberOfPlanes;
   VideoMode->BitsPerPlane = VBEMode->BitsPerPixel / VBEMode->NumberOfPlanes;
   VideoMode->Frequency = 1;

   /* Assume 96DPI and 25.4 millimeters per inch, round to nearest */
   dpi = 96;
   VideoMode->XMillimeter = ((ULONGLONG)VBEMode->XResolution * 254 + (dpi * 5)) / (dpi * 10);
   VideoMode->YMillimeter = ((ULONGLONG)VBEMode->YResolution * 254 + (dpi * 5)) / (dpi * 10);

   if (VBEMode->BitsPerPixel > 8)
   {
      /*
       * Always report 16bpp modes and not 15bpp mode...
       */
      if (VBEMode->BitsPerPixel == 15 && VBEMode->NumberOfPlanes == 1)
      {
         VideoMode->BitsPerPlane = 16;
      }

      if (DeviceExtension->VbeInfo.Version < 0x300)
      {
         VideoMode->NumberRedBits = VBEMode->RedMaskSize;
         VideoMode->NumberGreenBits = VBEMode->GreenMaskSize;
         VideoMode->NumberBlueBits = VBEMode->BlueMaskSize;
         VideoMode->RedMask = ((1 << VBEMode->RedMaskSize) - 1) << VBEMode->RedFieldPosition;
         VideoMode->GreenMask = ((1 << VBEMode->GreenMaskSize) - 1) << VBEMode->GreenFieldPosition;
         VideoMode->BlueMask = ((1 << VBEMode->BlueMaskSize) - 1) << VBEMode->BlueFieldPosition;
      }
      else
      {
         VideoMode->NumberRedBits = VBEMode->LinRedMaskSize;
         VideoMode->NumberGreenBits = VBEMode->LinGreenMaskSize;
         VideoMode->NumberBlueBits = VBEMode->LinBlueMaskSize;
         VideoMode->RedMask = ((1 << VBEMode->LinRedMaskSize) - 1) << VBEMode->LinRedFieldPosition;
         VideoMode->GreenMask = ((1 << VBEMode->LinGreenMaskSize) - 1) << VBEMode->LinGreenFieldPosition;
         VideoMode->BlueMask = ((1 << VBEMode->LinBlueMaskSize) - 1) << VBEMode->LinBlueFieldPosition;
      }
   }
   else
   {
      VideoMode->NumberRedBits =
      VideoMode->NumberGreenBits =
      VideoMode->NumberBlueBits = 6;
      VideoMode->RedMask =
      VideoMode->GreenMask =
      VideoMode->BlueMask = 0;
   }
   VideoMode->VideoMemoryBitmapWidth = VBEMode->XResolution;
   VideoMode->VideoMemoryBitmapHeight = VBEMode->YResolution;
   VideoMode->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
      VIDEO_MODE_NO_OFF_SCREEN;
   if (VideoMode->BitsPerPlane <= 8)
      VideoMode->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN;
   VideoMode->DriverSpecificAttributeFlags = 0;
}

/*
 * VBEQueryAvailModes
 *
 * Returns information about each video mode supported by the adapter.
 */

BOOLEAN FASTCALL
VBEQueryAvailModes(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION ReturnedModes,
   PSTATUS_BLOCK StatusBlock)
{
   ULONG CurrentModeId;
   PVIDEO_MODE_INFORMATION CurrentMode;
   PVBE_MODEINFO CurrentVBEMode;

   for (CurrentModeId = 0, CurrentMode = ReturnedModes,
        CurrentVBEMode = DeviceExtension->ModeInfo;
        CurrentModeId < DeviceExtension->ModeCount;
        CurrentModeId++, CurrentMode++, CurrentVBEMode++)
   {
      VBEQueryMode(DeviceExtension, CurrentMode, CurrentModeId);
   }

   StatusBlock->Information =
      sizeof(VIDEO_MODE_INFORMATION) * DeviceExtension->ModeCount;

   return TRUE;
}

/*
 * VBEQueryCurrentMode
 *
 * Returns information about current video mode.
 */

BOOLEAN FASTCALL
VBEQueryCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION VideoModeInfo,
   PSTATUS_BLOCK StatusBlock)
{
   StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION);

   VBEQueryMode(
      DeviceExtension,
      VideoModeInfo,
      DeviceExtension->CurrentMode);

   return TRUE;
}

/*
 * VBESetColorRegisters
 *
 * Sets the adapter's color registers to the specified RGB values. There
 * are code paths in this function, one generic and one for VGA compatible
 * controllers. The latter is needed for Bochs, where the generic one isn't
 * yet implemented.
 */

BOOLEAN FASTCALL
VBESetColorRegisters(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   ULONG Entry;
   PULONG OutputEntry;
   ULONG OutputBuffer[256];

   if (ColorLookUpTable->NumEntries + ColorLookUpTable->FirstEntry > 256)
      return FALSE;

   /*
    * For VGA compatible adapters program the color registers directly.
    */

   if (!(DeviceExtension->VbeInfo.Capabilities & 2))
   {
      for (Entry = ColorLookUpTable->FirstEntry;
           Entry < ColorLookUpTable->NumEntries + ColorLookUpTable->FirstEntry;
           Entry++)
      {
         VideoPortWritePortUchar((PUCHAR)0x03c8, Entry);
         VideoPortWritePortUchar((PUCHAR)0x03c9, ColorLookUpTable->LookupTable[Entry].RgbArray.Red);
         VideoPortWritePortUchar((PUCHAR)0x03c9, ColorLookUpTable->LookupTable[Entry].RgbArray.Green);
         VideoPortWritePortUchar((PUCHAR)0x03c9, ColorLookUpTable->LookupTable[Entry].RgbArray.Blue);
      }

      return TRUE;
   }
   else
   {
      /*
       * We can't just copy the values, because we need to swap the Red
       * and Blue values.
       */

      for (Entry = ColorLookUpTable->FirstEntry,
           OutputEntry = OutputBuffer;
           Entry < ColorLookUpTable->NumEntries + ColorLookUpTable->FirstEntry;
           Entry++, OutputEntry++)
      {
         *OutputEntry =
            (ColorLookUpTable->LookupTable[Entry].RgbArray.Red << 16) |
            (ColorLookUpTable->LookupTable[Entry].RgbArray.Green << 8) |
            (ColorLookUpTable->LookupTable[Entry].RgbArray.Blue);
      }

      DeviceExtension->Int10Interface.Int10WriteMemory(
         DeviceExtension->Int10Interface.Context,
         DeviceExtension->TrampolineMemorySegment,
         DeviceExtension->TrampolineMemoryOffset,
         OutputBuffer,
         (OutputEntry - OutputBuffer) * sizeof(ULONG));

      VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
      BiosRegisters.Eax = VBE_SET_GET_PALETTE_DATA;
      BiosRegisters.Ebx = 0;
      BiosRegisters.Ecx = ColorLookUpTable->NumEntries;
      BiosRegisters.Edx = ColorLookUpTable->FirstEntry;
      BiosRegisters.Edi = DeviceExtension->TrampolineMemoryOffset;
      BiosRegisters.SegEs = DeviceExtension->TrampolineMemorySegment;
      DeviceExtension->Int10Interface.Int10CallBios(
         DeviceExtension->Int10Interface.Context,
         &BiosRegisters);

      return VBE_GETRETURNCODE(BiosRegisters.Eax) == VBE_SUCCESS;
   }
}
