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
 * TODO:
 * - Check input parameters everywhere.
 * - Implement power management support.
 */

/* INCLUDES *******************************************************************/

#include "vbemp.h"

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

VP_STATUS STDCALL
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
   InitData.HwDeviceExtensionSize = sizeof(VBE_DEVICE_EXTENSION);
  
   return VideoPortInitialize(Context1, Context2, &InitData, NULL);
}

/*
 * InitializeVideoAddressSpace
 *
 * This function maps the BIOS memory into out virtual address space and
 * setups real-mode interrupt table.
 */

BOOL FASTCALL
InitializeVideoAddressSpace(VOID)
{
   NTSTATUS Status;
   PVOID BaseAddress;
   PVOID NullAddress;
   ULONG ViewSize;
   CHAR IVT[1024];
   CHAR BDA[256];
   LARGE_INTEGER Offset;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PhysMemName;
   HANDLE PhysMemHandle;

   /*
    * Open the physical memory section
    */

   RtlInitUnicodeString(&PhysMemName, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjectAttributes, &PhysMemName, 0, NULL, NULL);
   Status = ZwOpenSection(&PhysMemHandle, SECTION_ALL_ACCESS, &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: Couldn't open \\Device\\PhysicalMemory\n"));
      return FALSE;
   }

   /*
    * Map the BIOS and device registers into the address space
    */

   Offset.QuadPart = 0xa0000;
   ViewSize = 0x30000;
   BaseAddress = (PVOID)0xa0000;
   Status = NtMapViewOfSection(PhysMemHandle, NtCurrentProcess(), &BaseAddress,
      0, 8192, &Offset, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: Couldn't map physical memory (%x)\n", Status));
      NtClose(PhysMemHandle);
      return FALSE;
   }
   NtClose(PhysMemHandle);
   if (BaseAddress != (PVOID)0xa0000)
   {
      DPRINT(("VBEMP: Couldn't map physical memory at the right address "
              "(was %x)\n", BaseAddress));
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
      return FALSE;
   }
   if (BaseAddress != (PVOID)0x0)
   {
      DPRINT(("VBEMP: Failed to allocate virtual memory at right address "
               "(was %x)\n", BaseAddress));
      return FALSE;
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
   VideoPortMoveMemory(NullAddress, IVT, 1024);
   
   /*
    * Get the BDA from the kernel
    */

   Status = NtVdmControl(1, BDA);
   if (!NT_SUCCESS(Status))
   {
      DPRINT(("VBEMP: NtVdmControl failed (status %x)\n", Status));
      return FALSE;
   }
   
   /*
    * Copy the BDA into the right place
    */

   VideoPortMoveMemory((PVOID)0x400, BDA, 256);

   return TRUE;
}

VP_STATUS STDCALL
VBEFindAdapter(
   IN PVOID HwDeviceExtension,
   IN PVOID HwContext,
   IN PWSTR ArgumentString,
   IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
   OUT PUCHAR Again)
{
   KV86M_REGISTERS BiosRegisters;
   DWORD ViewSize;
   NTSTATUS Status;
   PVBE_INFO VbeInfo;
   PVBE_DEVICE_EXTENSION VBEDeviceExtension = 
     (PVBE_DEVICE_EXTENSION)HwDeviceExtension;

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
   VideoPortMoveMemory(VbeInfo->Signature, "VBE2", 4);
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
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

/*
 * VBEInitialize
 *
 * Performs the first initialization of the adapter, after the HAL has given
 * up control of the video hardware to the video port driver.
 */

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
   VideoPortMoveMemory(VbeInfo->Signature, "VBE2", 4);
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
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
      BiosRegisters.Edi = (VBEDeviceExtension->PhysicalAddress.QuadPart + 0x200) & 0xF;
      BiosRegisters.Es = (VBEDeviceExtension->PhysicalAddress.QuadPart + 0x200) >> 4;
      Ke386CallBios(0x10, &BiosRegisters);
      VbeModeInfo = (PVBE_MODEINFO)(VBEDeviceExtension->TrampolineMemory + 0x200);
      if (BiosRegisters.Eax == 0x4F &&
          VbeModeInfo->XResolution >= 640 &&
          VbeModeInfo->YResolution >= 480 &&
          (VbeModeInfo->ModeAttributes & VBE_MODEATTR_LINEAR))
      {
         VideoPortMoveMemory(VBEDeviceExtension->ModeInfo + ModeCount, 
                             VBEDeviceExtension->TrampolineMemory + 0x200,
                             sizeof(VBE_MODEINFO));
         VBEDeviceExtension->ModeNumbers[ModeCount] = ModeList[CurrentMode] | 0x4000;
         if (VbeModeInfo->XResolution == 640 &&
             VbeModeInfo->YResolution == 480 &&
             VbeModeInfo->BitsPerPixel == 32)
         {
            DefaultMode = ModeCount;
         }
         ModeCount++;
      }
   }

   /*
    * Exchange the default mode so it's at the first place in list.
    */

   VideoPortMoveMemory(&TempVbeModeInfo, VBEDeviceExtension->ModeInfo,
      sizeof(VBE_MODEINFO));
   VideoPortMoveMemory(VBEDeviceExtension->ModeInfo, VBEDeviceExtension->ModeInfo + DefaultMode,
      sizeof(VBE_MODEINFO));
   VideoPortMoveMemory(VBEDeviceExtension->ModeInfo + DefaultMode, &TempVbeModeInfo,
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

   /*
    * Print the supported video modes when DBG is set.
    */

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

/*
 * VBEStartIO
 *
 * Processes the specified Video Request Packet.
 */

BOOLEAN STDCALL
VBEStartIO(
   PVOID HwDeviceExtension,
   PVIDEO_REQUEST_PACKET RequestPacket)
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
         Result = VBEUnmapVideoMemory(
            (PVBE_DEVICE_EXTENSION)HwDeviceExtension,
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
             sizeof(VIDEO_CLUT))
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
         RequestPacket->StatusBlock->Status = STATUS_NOT_IMPLEMENTED;
         return FALSE;
   }
  
   if (Result)
      RequestPacket->StatusBlock->Status = STATUS_SUCCESS;

   return TRUE;
}

/*
 * VBEResetHw
 *
 * This function is called to reset the hardware to a known state.
 */

BOOLEAN STDCALL
VBEResetHw(
   PVOID DeviceExtension,
   ULONG Columns,
   ULONG Rows)
{
   return VBEResetDevice(DeviceExtension, NULL);
}

/*
 * VBEGetPowerState
 *
 * Queries whether the device can support the requested power state.
 */

VP_STATUS STDCALL
VBEGetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
   return ERROR_INVALID_FUNCTION;
}

/*
 * VBESetPowerState
 *
 * Sets the power state of the specified device
 */

VP_STATUS STDCALL
VBESetPowerState(
   PVOID HwDeviceExtension,
   ULONG HwId,
   PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
   return ERROR_INVALID_FUNCTION;
}

/*
 * VBESetCurrentMode
 *
 * Sets the adapter to the specified operating mode.
 */

BOOL FASTCALL
VBESetCurrentMode(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MODE RequestedMode,
   PSTATUS_BLOCK StatusBlock)
{
   KV86M_REGISTERS BiosRegisters;

   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
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

/*
 * VBEResetDevice
 *
 * Resets the video hardware to the default mode, to which it was initialized
 * at system boot. 
 */

BOOL FASTCALL
VBEResetDevice(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock)
{
   KV86M_REGISTERS BiosRegisters;

   InitializeVideoAddressSpace();
   VideoPortZeroMemory(&BiosRegisters, sizeof(BiosRegisters));
   BiosRegisters.Eax = 0x4F02;
   BiosRegisters.Ebx = 0x3;
   Ke386CallBios(0x10, &BiosRegisters);
   return TRUE;
}

/*
 * VBEMapVideoMemory
 *
 * Maps the video hardware frame buffer and video RAM into the virtual address
 * space of the requestor. 
 */

BOOL FASTCALL
VBEMapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_MEMORY RequestedAddress,
   PVIDEO_MEMORY_INFORMATION MapInformation,
   PSTATUS_BLOCK StatusBlock)
{
   PHYSICAL_ADDRESS FrameBuffer;
   ULONG inIoSpace = 0;

   StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);

   FrameBuffer.QuadPart =
      DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].PhysBasePtr;
   MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
   MapInformation->VideoRamLength = 
      DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].BytesPerScanLine *
      DeviceExtension->ModeInfo[DeviceExtension->CurrentMode].YResolution;

   VideoPortMapMemory(DeviceExtension, FrameBuffer,
      &MapInformation->VideoRamLength, &inIoSpace,
      &MapInformation->VideoRamBase);

   MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
   MapInformation->FrameBufferLength = MapInformation->VideoRamLength;

   DeviceExtension->FrameBufferMemory = MapInformation->VideoRamBase;

   return TRUE;
}

/*
 * VBEUnmapVideoMemory
 *
 * Releases a mapping between the virtual address space and the adapter's
 * frame buffer and video RAM.
 */

BOOL FASTCALL
VBEUnmapVideoMemory(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PSTATUS_BLOCK StatusBlock)
{
   VideoPortUnmapMemory(DeviceExtension, DeviceExtension->FrameBufferMemory,
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

BOOL FASTCALL
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

   VideoMode->Length = sizeof(VIDEO_MODE_INFORMATION);
   VideoMode->ModeIndex = VideoModeId;
   VideoMode->VisScreenWidth = VBEMode->XResolution;
   VideoMode->VisScreenHeight = VBEMode->YResolution;
   VideoMode->ScreenStride = VBEMode->BytesPerScanLine;
   VideoMode->NumberOfPlanes = VBEMode->NumberOfPlanes;
   VideoMode->BitsPerPlane = VBEMode->BitsPerPixel / VBEMode->NumberOfPlanes;
   VideoMode->Frequency = 0; /* FIXME */
   VideoMode->XMillimeter = 0; /* FIXME */
   VideoMode->YMillimeter = 0; /* FIXME */
   if (VBEMode->BitsPerPixel > 8)
   {
      if (DeviceExtension->VBEVersion < 0x300)
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

BOOL FASTCALL
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

BOOL FASTCALL  
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

BOOL FASTCALL
VBESetColorRegisters(
   PVBE_DEVICE_EXTENSION DeviceExtension,
   PVIDEO_CLUT ColorLookUpTable,
   PSTATUS_BLOCK StatusBlock)
{
   KV86M_REGISTERS BiosRegisters;
   ULONG Entry;
   PULONG OutputEntry;

   if (ColorLookUpTable->NumEntries + ColorLookUpTable->FirstEntry > 256)
      return FALSE;

   if (DeviceExtension->VGACompatible)
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
           OutputEntry = DeviceExtension->TrampolineMemory;
           Entry < ColorLookUpTable->NumEntries + ColorLookUpTable->FirstEntry;
           Entry++, OutputEntry++)
      {
         *OutputEntry =
            (ColorLookUpTable->LookupTable[Entry].RgbArray.Red << 16) |
            (ColorLookUpTable->LookupTable[Entry].RgbArray.Green << 8) |
            (ColorLookUpTable->LookupTable[Entry].RgbArray.Blue);
      }

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
