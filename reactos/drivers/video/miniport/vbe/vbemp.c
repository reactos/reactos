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
 *
 * FIXMEs:
 * - Check input parameters everywhere.
 * - Add some comments.
 * - Implement support power management.
 */

#include "vbemp.h"

/******************************************************************************/

PVOID FASTCALL
MapPM(ULONG Address, ULONG Size)
{
   LARGE_INTEGER Offset;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PhysMemName;
   HANDLE PhysMemHandle;
   NTSTATUS Status;
   PVOID BaseAddress;
   ULONG ViewSize;

   /*
    * Open the physical memory section
    */
   RtlInitUnicodeString(&PhysMemName, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PhysMemName,
			      0,
			      NULL,
			      NULL);
   Status = ZwOpenSection(&PhysMemHandle, SECTION_ALL_ACCESS, 
			  &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: Couldn't open \\Device\\PhysicalMemory\n"));
      return NULL;
   }

   /*
    * Map the BIOS and device registers into the address space
    */
   Offset.QuadPart = Address;
   ViewSize = Size;
   BaseAddress = (PVOID)Address;
   Status = NtMapViewOfSection(PhysMemHandle,
			       NtCurrentProcess(),
			       &BaseAddress,
			       0,
			       8192,
			       &Offset,
			       &ViewSize,
			       ViewUnmap,
			       0,
			       PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: Couldn't map physical memory (%x)\n", Status));
      NtClose(PhysMemHandle);
      return NULL;
   }
   NtClose(PhysMemHandle);

   if (BaseAddress != (PVOID)Address)
   {
      DPRINT(("VBEMP: Couldn't map physical memory at the right address "
              "(was %x)(%x)\n", BaseAddress, Address));
      return NULL;
   }

   return BaseAddress;
}

ULONG FASTCALL
InitializeVideoAddressSpace(VOID)
{
   NTSTATUS Status;
   PVOID BaseAddress;
   PVOID NullAddress;
   ULONG ViewSize;
   CHAR IVT[1024];
   CHAR BDA[256];

   if (MapPM(0xa0000, 0x30000) == NULL)
   {
      return FALSE;
   }

   /*
    * Map some memory to use for the non-BIOS parts of the v86 mode address
    * space
    */
   BaseAddress = (PVOID)0x1;
   ViewSize = 0x20000;
   Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
				    &BaseAddress,
				    0,
				    &ViewSize,
				    MEM_COMMIT,
				    PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: Failed to allocate virtual memory (Status %x)\n", Status));
      return 0;
   }
   if (BaseAddress != (PVOID)0x0)
   {
      DPRINT(("VBEMP: Failed to allocate virtual memory at right address "
               "(was %x)\n", BaseAddress));
      return 0;
   }

   /*
    * Get the real mode IVT from the kernel
    */
   Status = NtVdmControl(0, IVT);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: NtVdmControl failed (status %x)\n", Status));
      return 0;
   }
   
   /*
    * Copy the real mode IVT into the right place
    */
   NullAddress = (PVOID)0x0; /* Workaround for GCC 3.4 */
   memcpy(NullAddress, IVT, 1024);
   
   /*
    * Get the BDA from the kernel
    */
   Status = NtVdmControl(1, BDA);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: NtVdmControl failed (status %x)\n", Status));
      return 0;
   }
   
   /*
    * Copy the BDA into the right place
    */
   memcpy((PVOID)0x400, BDA, 256);

   return 1;
}

VP_STATUS STDCALL
DriverEntry(IN PVOID Context1, IN PVOID Context2)
{
   VIDEO_HW_INITIALIZATION_DATA InitData;

   VideoPortZeroMemory(&InitData, sizeof(InitData));
   InitData.HwFindAdapter = VBEFindAdapter;
   InitData.HwInitialize = VBEInitialize;
   InitData.HwStartIO = VBEStartIO;
   InitData.HwDeviceExtensionSize = sizeof(VBE_DEVICE_EXTENSION);
  
   return VideoPortInitialize(Context1, Context2, &InitData, NULL);
}

VP_STATUS STDCALL
VBEFindAdapter(IN PVOID HwDeviceExtension, IN PVOID HwContext,
   IN PWSTR ArgumentString, IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again)
{
   KV86M_REGISTERS BiosRegisters;
   DWORD ViewSize;
   NTSTATUS Status;
   PVBE_INFO VbeInfo;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension = 
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

   /*
    * We support only one adapter.
    */

   *Again = FALSE;

   /*
    * Map the BIOS parts of memory into our memory space and intitalize
    * the real mode interrupt table.
    */

   InitializeVideoAddressSpace();

   /*
    * Allocate a bit of memory that will be later used for VBE transport
    * buffer. This memory must be accessible from V86 mode so it must fit
    * in the first megabyte of physical memory.
    */

   VBEDeviceExtension->TrampolineMemory = (PVOID)0x20000;
   ViewSize = 0x400;
   Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
      (PVOID*)&VBEDeviceExtension->TrampolineMemory, 0, &ViewSize, MEM_COMMIT,
      PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("Failed to allocate virtual memory (Status %x)\n", Status));
      return 0;
   }
   if (VBEDeviceExtension->TrampolineMemory > (PVOID)(0x100000 - 0x400))
   {
      DPRINT(("Failed to allocate virtual memory at right address "
              "(was %x)\n", VBEDeviceExtension->TrampolineMemory));
      return 0;
   }
   VBEDeviceExtension->PhysicalAddress.QuadPart = 
      (UINT_PTR)VBEDeviceExtension->TrampolineMemory;

   /*
    * Get the VBE general information.
    */
   
   VbeInfo = (PVBE_INFO)VBEDeviceExtension->TrampolineMemory;
   strncpy(VbeInfo->Signature, "VBE2", 4);
   memset(&BiosRegisters, 0, sizeof(BiosRegisters));
   BiosRegisters.Eax = 0x4F00;
   BiosRegisters.Edi = VBEDeviceExtension->PhysicalAddress.QuadPart & 0xFF;
   BiosRegisters.Es = VBEDeviceExtension->PhysicalAddress.QuadPart >> 4;
   Ke386CallBios(0x10, &BiosRegisters);
   if (BiosRegisters.Eax == 0x4F)
   {
      if (VbeInfo->Version >= 0x200)
      {
         DPRINT(("VBE BIOS Present (%d.%d, %8ld Kb)\n",
            VbeInfo->Version / 0x100, VbeInfo->Version & 0xFF,
            VbeInfo->TotalMemory * 16));

         return NO_ERROR;
      }
      else
      {
         DPRINT(("VBE BIOS present, but incompatible version.\n"));

         return ERROR_DEV_NOT_EXIST;
      }
   }
   else
   {
      DPRINT(("No VBE BIOS found.\n"));

      return ERROR_DEV_NOT_EXIST;
   }
}

BOOLEAN STDCALL
VBEInitialize(PVOID HwDeviceExtension)
{
   /*
    * Build a mode list here that can be later used by
    * IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES and IOCTL_VIDEO_QUERY_AVAIL_MODES
    * calls.
    */

   ULONG ModeCount;
   ULONG CurrentMode;
   KV86M_REGISTERS BiosRegisters;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension = 
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;
   PVBE_INFO VbeInfo;
   PVBE_MODEINFO VbeModeInfo;
   VBE_MODEINFO TempVbeModeInfo;
   WORD TempVbeModeNumber;
   WORD *ModeList;
   WORD DefaultMode;

   InitializeVideoAddressSpace();

   /*
    * Get the VBE general information.
    */
   
   VbeInfo = (PVBE_INFO)VBEDeviceExtension->TrampolineMemory;
   strncpy(VbeInfo->Signature, "VBE2", 4);
   memset(&BiosRegisters, 0, sizeof(BiosRegisters));
   BiosRegisters.Eax = 0x4F00;
   BiosRegisters.Edi = VBEDeviceExtension->PhysicalAddress.QuadPart & 0xFF;
   BiosRegisters.Es = VBEDeviceExtension->PhysicalAddress.QuadPart >> 4;
   Ke386CallBios(0x10, &BiosRegisters);

   VBEDeviceExtension->VBEVersion = VbeInfo->Version;
   VBEDeviceExtension->VGACompatible = !(VbeInfo->Capabilities & 2);
   /*
    * Get the number of supported video modes.
    */

   /*
    * No need to be mapped, it's either in BIOS memory or in our trampoline
    * memory. Both of them are already mapped.
    */
   ModeList = (WORD *)((HIWORD(VbeInfo->VideoModePtr) << 4) + LOWORD(VbeInfo->VideoModePtr));
   for (CurrentMode = 0, ModeCount = 0;
        ModeList[CurrentMode] != 0xFFFF && ModeList[CurrentMode] != 0;
        CurrentMode++)
   {
      ModeCount++;
   }

   /*
    * Allocate space for video modes information.
    */

   VBEDeviceExtension->ModeInfo =
      ExAllocatePool(PagedPool, ModeCount * sizeof(VBE_MODEINFO));
   VBEDeviceExtension->ModeNumbers =
      ExAllocatePool(PagedPool, ModeCount * sizeof(WORD));

   /*
    * Get the actual mode infos.
    */
   
   for (CurrentMode = 0, ModeCount = 0, DefaultMode = 0;
        ModeList[CurrentMode] != 0xFFFF && CurrentMode < 0x400;
        CurrentMode++)
   {
      BiosRegisters.Eax = 0x4F01;
      BiosRegisters.Ecx = ModeList[CurrentMode];
      BiosRegisters.Edi = VBEDeviceExtension->PhysicalAddress.QuadPart & 0xF;
      BiosRegisters.Es = VBEDeviceExtension->PhysicalAddress.QuadPart >> 4;
      Ke386CallBios(0x10, &BiosRegisters);
      VbeModeInfo = (PVBE_MODEINFO)VBEDeviceExtension->TrampolineMemory;
      if (BiosRegisters.Eax == 0x4F &&
          VbeModeInfo->XResolution >= 640 &&
          VbeModeInfo->YResolution >= 480 &&
/*          (VbeModeInfo->MemoryModel == 5 || VbeModeInfo->MemoryModel == 6) &&*/
          (VbeModeInfo->ModeAttributes & VBE_MODEATTR_LINEAR))
      {
         memcpy(VBEDeviceExtension->ModeInfo + ModeCount, 
                VBEDeviceExtension->TrampolineMemory,
                sizeof(VBE_MODEINFO));
         VBEDeviceExtension->ModeNumbers[ModeCount] = ModeList[CurrentMode] | 0x4000;
         if (VbeModeInfo->XResolution == 640 &&
             VbeModeInfo->YResolution == 480 &&
             VbeModeInfo->BitsPerPixel == 8)
         {
            DefaultMode = ModeCount;
         }
         ModeCount++;
      }
   }

   /*
    * Exchange the default mode so it's at the first place in list.
    */

   memcpy(&TempVbeModeInfo,
          VBEDeviceExtension->ModeInfo,
          sizeof(VBE_MODEINFO));
   memcpy(VBEDeviceExtension->ModeInfo,
          VBEDeviceExtension->ModeInfo + DefaultMode,
          sizeof(VBE_MODEINFO));
   memcpy(VBEDeviceExtension->ModeInfo + DefaultMode,
          &TempVbeModeInfo,
          sizeof(VBE_MODEINFO));
   TempVbeModeNumber = VBEDeviceExtension->ModeNumbers[0];
   VBEDeviceExtension->ModeNumbers[0] = VBEDeviceExtension->ModeNumbers[DefaultMode];
   VBEDeviceExtension->ModeNumbers[DefaultMode] = TempVbeModeNumber;

   if (ModeCount == 0)
   {
      DPRINT(("VBEMP: No video modes supported\n"));
      return FALSE;
   }
   
   VBEDeviceExtension->ModeCount = ModeCount;

#ifdef DBG
   for (CurrentMode = 0;
        CurrentMode < ModeCount;
        CurrentMode++)
   {
      DPRINT(("%dx%dx%d\n",
         VBEDeviceExtension->ModeInfo[CurrentMode].XResolution,
         VBEDeviceExtension->ModeInfo[CurrentMode].YResolution,
         VBEDeviceExtension->ModeInfo[CurrentMode].BitsPerPixel));
   }
#endif

   return TRUE;
}

BOOLEAN STDCALL
VBEStartIO(PVOID HwDeviceExtension, PVIDEO_REQUEST_PACKET RequestPacket)
{
   BOOL Result;

   RequestPacket->StatusBlock->Status = STATUS_UNSUCCESSFUL;

   switch (RequestPacket->IoControlCode)
   {
      case IOCTL_VIDEO_SET_CURRENT_MODE:
         if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE)) 
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBESetCurrentMode((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MODE)RequestPacket->InputBuffer, RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_RESET_DEVICE:
         Result = VBEResetDevice((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
         if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION) ||
             RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) 
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEMapVideoMemory((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MEMORY)RequestPacket->InputBuffer,
            (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
         Result = VBEUnmapVideoMemory((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
         if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES)) 
         {
            RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
            return TRUE;
         }
         Result = VBEQueryNumAvailModes((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
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
         Result = VBEQueryAvailModes((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
            RequestPacket->StatusBlock);
         break;

      case IOCTL_VIDEO_QUERY_CURRENT_MODE:
         UNIMPLEMENTED;
         break;

      case IOCTL_VIDEO_SET_COLOR_REGISTERS:
         /* FIXME: Check buffer size! */
         Result = VBESetColorRegisters((PVBE_DEVICE_EXTENSION)HwDeviceExtension,
            (PVIDEO_CLUT)RequestPacket->InputBuffer, RequestPacket->StatusBlock);
         break;

      default:
         RequestPacket->StatusBlock->Status = STATUS_NOT_IMPLEMENTED;
         return FALSE;
   }
  
   if (Result)
      RequestPacket->StatusBlock->Status = STATUS_SUCCESS;

   return TRUE;
}

BOOL FASTCALL
VBESetCurrentMode(PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode, PSTATUS_BLOCK StatusBlock)
{
   KV86M_REGISTERS BiosRegisters;

   memset(&BiosRegisters, 0, sizeof(BiosRegisters));
   BiosRegisters.Eax = 0x4F02;
   BiosRegisters.Ebx = DeviceExtension->ModeNumbers[RequestedMode->RequestedMode];
   Ke386CallBios(0x10, &BiosRegisters);
   if (BiosRegisters.Eax == 0x4F)
   {
      DeviceExtension->CurrentMode = RequestedMode->RequestedMode;
   }
   else
   {
      DPRINT(("VBEMP: VBESetCurrentMode failed (%x)\n", BiosRegisters.Eax));
      DeviceExtension->CurrentMode = -1;
   }
   return (BiosRegisters.Eax == 0x4F);
}

BOOL FASTCALL
VBEResetDevice(PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock)
{
   VIDEO_X86_BIOS_ARGUMENTS BiosRegisters;

   memset(&BiosRegisters, 0, sizeof(BiosRegisters));
   BiosRegisters.Eax = 0x4F02;
   BiosRegisters.Ebx = 0x3;
   VideoPortInt10(NULL, &BiosRegisters);
   return TRUE;
}

BOOL FASTCALL
VBEMapVideoMemory(PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress, PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock)
{
   KV86M_REGISTERS BiosRegisters;
   PVBE_MODEINFO VbeModeInfo;
   PHYSICAL_ADDRESS FrameBuffer;
   ULONG inIoSpace = 0;

   StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);

   BiosRegisters.Eax = 0x4F01;
   BiosRegisters.Ecx = DeviceExtension->ModeNumbers[DeviceExtension->CurrentMode];
   BiosRegisters.Edi = DeviceExtension->PhysicalAddress.QuadPart & 0xF;
   BiosRegisters.Es = DeviceExtension->PhysicalAddress.QuadPart >> 4;
   Ke386CallBios(0x10, &BiosRegisters);
   VbeModeInfo = (PVBE_MODEINFO)DeviceExtension->TrampolineMemory;
   if (BiosRegisters.Eax == 0x4F && 
       (VbeModeInfo->ModeAttributes & VBE_MODEATTR_LINEAR))
   {     
      FrameBuffer.QuadPart = VbeModeInfo->PhysBasePtr;
      MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
      MapInformation->VideoRamLength = (
         DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].XResolution *
         DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].YResolution *
         DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].BitsPerPixel
         ) >> 3;

      VideoPortMapMemory(DeviceExtension, FrameBuffer,
         &MapInformation->VideoRamLength, &inIoSpace,
         &MapInformation->VideoRamBase);

      MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
      MapInformation->FrameBufferLength = MapInformation->VideoRamLength;

      DeviceExtension->FrameBufferMemory = MapInformation->VideoRamBase;

      return TRUE;
   }
   else
   {
      DPRINT(("VBEMP: VBEMapVideoMemory Failed (%lx)\n", BiosRegisters.Eax));

      return FALSE;
   }
}

BOOL FASTCALL
VBEUnmapVideoMemory(PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock)
{
   VideoPortUnmapMemory(DeviceExtension, DeviceExtension->FrameBufferMemory,
      NULL);
   return TRUE;
}   

BOOL FASTCALL
VBEQueryNumAvailModes(PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_NUM_MODES Modes, PSTATUS_BLOCK StatusBlock)
{
   Modes->NumModes = DeviceExtension->ModeCount;
   Modes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
   StatusBlock->Information = sizeof(VIDEO_NUM_MODES);
   return TRUE;
}

BOOL FASTCALL
VBEQueryAvailModes(PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE_INFORMATION ReturnedModes, PSTATUS_BLOCK StatusBlock)
{
   ULONG CurrentModeId;
   PVIDEO_MODE_INFORMATION CurrentMode;
   PVBE_MODEINFO CurrentVBEMode;

   for (CurrentModeId = 0, CurrentMode = ReturnedModes,
        CurrentVBEMode = DeviceExtension->ModeInfo;
        CurrentModeId < DeviceExtension->ModeCount;
        CurrentModeId++, CurrentMode++, CurrentVBEMode++)
   {
      CurrentMode->Length = sizeof(VIDEO_MODE_INFORMATION);
      CurrentMode->ModeIndex = CurrentModeId;
      CurrentMode->VisScreenWidth = CurrentVBEMode->XResolution;
      CurrentMode->VisScreenHeight = CurrentVBEMode->YResolution;
      CurrentMode->ScreenStride = CurrentVBEMode->BytesPerScanLine;
      CurrentMode->NumberOfPlanes = CurrentVBEMode->NumberOfPlanes;
      CurrentMode->BitsPerPlane = CurrentVBEMode->BitsPerPixel /
         CurrentVBEMode->NumberOfPlanes;
      CurrentMode->Frequency = 0; /* FIXME */
      CurrentMode->XMillimeter = 0; /* FIXME */
      CurrentMode->YMillimeter = 0; /* FIXME */
      if (CurrentVBEMode->BitsPerPixel > 8)
      {
         if (DeviceExtension->VBEVersion < 0x300)
         {
            CurrentMode->NumberRedBits = CurrentVBEMode->RedMaskSize;
            CurrentMode->NumberGreenBits = CurrentVBEMode->GreenMaskSize;
            CurrentMode->NumberBlueBits = CurrentVBEMode->BlueMaskSize;
            CurrentMode->RedMask = ((1 << CurrentVBEMode->RedMaskSize) - 1) << CurrentVBEMode->RedFieldPosition;
            CurrentMode->GreenMask = ((1 << CurrentVBEMode->GreenMaskSize) - 1) << CurrentVBEMode->GreenFieldPosition;
            CurrentMode->BlueMask = ((1 << CurrentVBEMode->BlueMaskSize) - 1) << CurrentVBEMode->BlueFieldPosition;
         }
         else
         {
            CurrentMode->NumberRedBits = CurrentVBEMode->LinRedMaskSize;
            CurrentMode->NumberGreenBits = CurrentVBEMode->LinGreenMaskSize;
            CurrentMode->NumberBlueBits = CurrentVBEMode->LinBlueMaskSize;
            CurrentMode->RedMask = ((1 << CurrentVBEMode->LinRedMaskSize) - 1) << CurrentVBEMode->LinRedFieldPosition;
            CurrentMode->GreenMask = ((1 << CurrentVBEMode->LinGreenMaskSize) - 1) << CurrentVBEMode->LinGreenFieldPosition;
            CurrentMode->BlueMask = ((1 << CurrentVBEMode->LinBlueMaskSize) - 1) << CurrentVBEMode->LinBlueFieldPosition;
         }
      }
      else
      {
         CurrentMode->NumberRedBits = 
         CurrentMode->NumberGreenBits = 
         CurrentMode->NumberBlueBits = 6;
         CurrentMode->RedMask = 
         CurrentMode->GreenMask = 
         CurrentMode->BlueMask = 0;
      }
      CurrentMode->VideoMemoryBitmapWidth = CurrentVBEMode->XResolution;
      CurrentMode->VideoMemoryBitmapHeight = CurrentVBEMode->YResolution;
      CurrentMode->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
         VIDEO_MODE_NO_OFF_SCREEN;
      if (CurrentMode->BitsPerPlane <= 8)
         CurrentMode->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN;
      CurrentMode->DriverSpecificAttributeFlags = 0;
   }

   StatusBlock->Information =
      sizeof(VIDEO_MODE_INFORMATION) * DeviceExtension->ModeCount;

   return TRUE;
}

BOOL FASTCALL
VBESetColorRegisters(PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable, PSTATUS_BLOCK StatusBlock)
{
   KV86M_REGISTERS BiosRegisters;

   if (DeviceExtension->VGACompatible)
   {
      ULONG Entry;

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
       * FIXME:
       * This is untested code path, it's possible that it will
       * not work at all or that Red and Blue colors will be swapped.
       */

      memcpy(DeviceExtension->TrampolineMemory,
         &ColorLookUpTable->LookupTable[0].RgbArray,
         sizeof(DWORD) * ColorLookUpTable->NumEntries);
      BiosRegisters.Eax = 0x4F09;
      BiosRegisters.Ebx = 0;
      BiosRegisters.Ecx = ColorLookUpTable->NumEntries;
      BiosRegisters.Edx = ColorLookUpTable->FirstEntry;
      BiosRegisters.Edi = DeviceExtension->PhysicalAddress.QuadPart & 0xF;
      BiosRegisters.Es = DeviceExtension->PhysicalAddress.QuadPart >> 4;
      Ke386CallBios(0x10, &BiosRegisters);
      return (BiosRegisters.Eax == 0x4F);
   }
}
