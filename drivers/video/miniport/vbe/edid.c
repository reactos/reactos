/*
 * ReactOS VBE EDID management
 *
 * Copyright (C) 2006 Hervé Poussineau
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
 *
 */

/* INCLUDES *******************************************************************/

#include "vbemp.h"

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

static VOID NTAPI
VBEWriteClockLine(
   PVOID HwDeviceExtension,
   UCHAR data)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_WRITE_SCL_CLOCK_LINE;
   BiosRegisters.Ecx = VBEDeviceExtension->CurrentChildIndex;
   BiosRegisters.Edx = data;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);
}

static VOID NTAPI
VBEWriteDataLine(
   PVOID HwDeviceExtension,
   UCHAR data)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_WRITE_SDA_DATA_LINE;
   BiosRegisters.Ecx = VBEDeviceExtension->CurrentChildIndex;
   BiosRegisters.Edx = data;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);
}

static BOOLEAN NTAPI
VBEReadClockLine(
   PVOID HwDeviceExtension)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_READ_SCL_CLOCK_LINE;
   BiosRegisters.Ecx = VBEDeviceExtension->CurrentChildIndex;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   return BiosRegisters.Edx;
}

static BOOLEAN NTAPI
VBEReadDataLine(
   PVOID HwDeviceExtension)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_READ_SDA_DATA_LINE;
   BiosRegisters.Ecx = VBEDeviceExtension->CurrentChildIndex;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   return BiosRegisters.Edx;
}

static BOOLEAN
VBEReadEdidUsingSCI(
   IN PVOID HwDeviceExtension,
   IN ULONG ChildIndex,
   OUT PVOID Edid)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;
   DDC_CONTROL DDCControl;
   BOOLEAN ret;

   VideoPortDebugPrint(Trace, "VBEMP: VBEReadEdidUsingSCI() called\n");

   /*
    * Check if graphic card support I²C interface
    */
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_REPORT_CAPABILITIES;
   BiosRegisters.Ecx = VBEDeviceExtension->CurrentChildIndex;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);
   if (BiosRegisters.Eax != VBE_SUCCESS)
      return FALSE;
   VideoPortDebugPrint(Info, "VBEMP: VBE/SCI version %x\n", BiosRegisters.Ecx);
   if ((BiosRegisters.Ebx & 0xF) != 0xF)
      return FALSE;

   /*
    * Enable I²C interface
    */
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_BEGIN_SCL_SDA_CONTROL;
   BiosRegisters.Ecx = VBEDeviceExtension->CurrentChildIndex;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);
   if (BiosRegisters.Eax != VBE_SUCCESS)
      return FALSE;

   /*
    * Read EDID information
    */
   VBEDeviceExtension->CurrentChildIndex = ChildIndex;
   DDCControl.Size = sizeof(DDC_CONTROL);
   DDCControl.I2CCallbacks.WriteClockLine = VBEWriteClockLine;
   DDCControl.I2CCallbacks.WriteDataLine = VBEWriteDataLine;
   DDCControl.I2CCallbacks.ReadClockLine = VBEReadClockLine;
   DDCControl.I2CCallbacks.ReadDataLine = VBEReadDataLine;
   DDCControl.EdidSegment = 0;
   ret = VideoPortDDCMonitorHelper(
      HwDeviceExtension,
      &DDCControl,
      (PUCHAR)&Edid,
      MAX_SIZE_OF_EDID);

   /*
    * Disable I²C interface
    */
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_END_SCL_SDA_CONTROL;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);
   /* Ignore the possible error, as we did our best to prevent problems */

   return ret;
}

static BOOLEAN
VBEReadEdid(
   IN PVBE_DEVICE_EXTENSION VBEDeviceExtension,
   IN ULONG ChildIndex,
   OUT PVOID Edid)
{
   INT10_BIOS_ARGUMENTS BiosRegisters;

   VideoPortDebugPrint(Trace, "VBEMP: VBEReadEdid() called\n");

   /*
    * Directly read EDID information
    */
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = VBE_DDC;
   BiosRegisters.Ebx = VBE_DDC_READ_EDID;
   BiosRegisters.Ecx = ChildIndex;
   BiosRegisters.Edx = 1;
   BiosRegisters.Edi = VBEDeviceExtension->TrampolineMemoryOffset;
   BiosRegisters.SegEs = VBEDeviceExtension->TrampolineMemorySegment;
   VBEDeviceExtension->Int10Interface.Int10CallBios(
      VBEDeviceExtension->Int10Interface.Context,
      &BiosRegisters);

   if (BiosRegisters.Eax != VBE_SUCCESS)
      return FALSE;

   /*
    * Copy the EDID information to our buffer
    */
   VBEDeviceExtension->Int10Interface.Int10ReadMemory(
      VBEDeviceExtension->Int10Interface.Context,
      VBEDeviceExtension->TrampolineMemorySegment,
      VBEDeviceExtension->TrampolineMemoryOffset,
      Edid,
      MAX_SIZE_OF_EDID);

   return TRUE;
}

VP_STATUS NTAPI
VBEGetVideoChildDescriptor(
   IN PVOID HwDeviceExtension,
   IN PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
   OUT PVIDEO_CHILD_TYPE VideoChildType,
   OUT PUCHAR pChildDescriptor,
   OUT PULONG UId,
   OUT PULONG pUnused)
{
   PVBE_DEVICE_EXTENSION VBEDeviceExtension =
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;
   ULONG ChildIndex;

   /*
    * We are called very early in device initialization, even before
    * VBEInitialize is called. So, our Int10 interface is not set.
    * Ignore this call, we will trigger another one later.
    */
   if (VBEDeviceExtension->Int10Interface.Size == 0)
      return VIDEO_ENUM_NO_MORE_DEVICES;

   if (ChildEnumInfo->Size != sizeof(VIDEO_CHILD_ENUM_INFO))
   {
      VideoPortDebugPrint(Error, "VBEMP: Wrong VIDEO_CHILD_ENUM_INFO structure size\n");
      return VIDEO_ENUM_NO_MORE_DEVICES;
   }
   else if (ChildEnumInfo->ChildDescriptorSize < MAX_SIZE_OF_EDID)
   {
      VideoPortDebugPrint(Warn, "VBEMP: Too small buffer for EDID\n");
      return VIDEO_ENUM_NO_MORE_DEVICES;
   }
   else if (ChildEnumInfo->ChildIndex == DISPLAY_ADAPTER_HW_ID)
   {
      *VideoChildType = VideoChip;
      *UId = 0;
      return VIDEO_ENUM_MORE_DEVICES; /* FIXME: not sure... */
   }

   /*
    * Get Child ID
    */
   if (ChildEnumInfo->ChildIndex != 0)
      ChildIndex = ChildEnumInfo->ChildIndex;
   else
      ChildIndex = ChildEnumInfo->ACPIHwId;
   VideoPortDebugPrint(Info, "VBEMP: ChildEnumInfo->ChildIndex %lu, ChildEnumInfo->ACPIHwId %lu => %lu\n",
      ChildEnumInfo->ChildIndex, ChildEnumInfo->ACPIHwId, ChildIndex);

   /*
    * Try to read EDID information using 2 different methods.
    */
   if (VBEReadEdid(HwDeviceExtension, ChildIndex, pChildDescriptor))
   {
      VideoPortDebugPrint(Info, "VBEMP: EDID information read directly\n");
   }
   else if (VBEReadEdidUsingSCI(HwDeviceExtension, ChildIndex, pChildDescriptor))
   {
      VideoPortDebugPrint(Info, "VBEMP: EDID information read using I²C\n");
   }
   else
   {
      VideoPortDebugPrint(Warn, "VBEMP: Unable to read EDID information\n");
      return VIDEO_ENUM_NO_MORE_DEVICES;
   }

   /*
    * Fill return data
    */
   *VideoChildType = Monitor;
   if (ChildIndex == 0)
   {
      /*
       * This is the actual display adapter
       */
      *UId = DISPLAY_ADAPTER_HW_ID;
   }
   else
      *UId = ChildIndex;
   *pUnused = 0;
   return VIDEO_ENUM_MORE_DEVICES;
}
