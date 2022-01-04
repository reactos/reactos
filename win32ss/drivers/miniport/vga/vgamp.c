/*
 * VGA.C - a generic VGA miniport driver
 *
 */

//  -------------------------------------------------------  Includes

#include "vgamp.h"

#include <dderror.h>
#include <devioctl.h>

VIDEO_ACCESS_RANGE VGAAccessRange[] =
{
    { {{0x3b0}}, 0x3bb - 0x3b0 + 1, 1, 0, 0 },
    { {{0x3c0}}, 0x3df - 0x3c0 + 1, 1, 0, 0 },
    { {{0xa0000}}, 0x20000, 0, 0, 0 },
};

//  -------------------------------------------------------  Public Interface

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PVOID  Context1  Context parameter to pass to VidPortInitialize
//    IN  PVOID  Context2  Context parameter to pass to VidPortInitialize
//  RETURNS:
//    ULONG

ULONG NTAPI
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
  VIDEO_HW_INITIALIZATION_DATA  InitData;

  VideoPortZeroMemory(&InitData, sizeof InitData);

  InitData.HwInitDataSize = sizeof(InitData);
  /* FIXME: Fill in InitData members  */
  InitData.StartingDeviceNumber = 0;

  /*  Export driver entry points...  */
  InitData.HwFindAdapter = VGAFindAdapter;
  InitData.HwInitialize = VGAInitialize;
  InitData.HwStartIO = VGAStartIO;
  /* InitData.HwInterrupt = VGAInterrupt;  */
  InitData.HwResetHw = VGAResetHw;
  /* InitData.HwTimer = VGATimer;  */
  InitData.HwLegacyResourceList = VGAAccessRange;
  InitData.HwLegacyResourceCount = ARRAYSIZE(VGAAccessRange);

  return  VideoPortInitialize(Context1, Context2, &InitData, NULL);
}

//    VGAFindAdapter
//
//  DESCRIPTION:
//    This routine is called by the videoport driver to find and allocate
//    the adapter for a given bus.  The miniport driver needs to do the
//    following in this routine:
//      - Determine if the adapter is present
//      - Claim any necessary memory/IO resources for the adapter
//      - Map resources into system memory for the adapter
//      - fill in relevant information in the VIDEO_PORT_CONFIG_INFO buffer
//      - update registry settings for adapter specifics.
//      - Set 'Again' based on whether the function should be called again
//        another adapter on the same bus.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    PVOID                    DeviceExtension
//    PVOID                    Context
//    PWSTR                    ArgumentString
//    PVIDEO_PORT_CONFIG_INFO  ConfigInfo
//    PUCHAR                   Again
//  RETURNS:
//    VP_STATUS

VP_STATUS NTAPI
VGAFindAdapter(PVOID DeviceExtension,
               PVOID Context,
               PWSTR ArgumentString,
               PVIDEO_PORT_CONFIG_INFO ConfigInfo,
               PUCHAR Again)
{
    VP_STATUS Status;

    /* FIXME: Determine if the adapter is present  */
    *Again = FALSE;

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
        return ERROR_INVALID_PARAMETER;

    Status = VideoPortVerifyAccessRanges(DeviceExtension, ARRAYSIZE(VGAAccessRange), VGAAccessRange);
    if (Status != NO_ERROR)
        return Status;

    ConfigInfo->VdmPhysicalVideoMemoryAddress = VGAAccessRange[2].RangeStart;
    ConfigInfo->VdmPhysicalVideoMemoryLength = VGAAccessRange[2].RangeLength;
    return NO_ERROR;

  /* FIXME: Claim any necessary memory/IO resources for the adapter  */
  /* FIXME: Map resources into system memory for the adapter  */
  /* FIXME: Fill in relevant information in the VIDEO_PORT_CONFIG_INFO buffer  */
  /* FIXME: Update registry settings for adapter specifics.  */
//  return  NO_ERROR;
}

//    VGAInitialize
//
//  DESCRIPTION:
//    Perform initialization tasks, but leave the adapter in the same
//    user visible state
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    PVOID  DeviceExtension
//  RETURNS:
//    BOOLEAN  Success or failure
BOOLEAN NTAPI
VGAInitialize(PVOID DeviceExtension)
{
  return  TRUE;
}

//    VGAStartIO
//
//  DESCRIPTION:
//    This function gets called in responce to GDI EngDeviceIoControl
//    calls.  Device requests are passed in VRPs.
//      Required VRPs:
//        IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES
//        IOCTL_VIDEO_QUERY_AVAIL_MODES
//        IOCTL_VIDEO_QUERY_CURRENT_MODE
//        IOCTL_VIDEO_SET_CURRENT_MODE
//        IOCTL_VIDEO_RESET_DEVICE
//        IOCTL_VIDEO_MAP_VIDEO_MEMORY
//        IOCTL_VIDEO_UNMAP_VIDEO_MEMORY
//        IOCTL_VIDEO_SHARE_VIDEO_MEMORY
//        IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY
//      Optional VRPs:
//        IOCTL_VIDEO_GET_PUBLIC_ACCESS_RANGES
//        IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES
//        IOCTL_VIDEO_GET_POWER_MANAGEMENT
//        IOCTL_VIDEO_SET_POWER_MANAGEMENT
//        IOCTL_QUERY_COLOR_CAPABILITIES
//        IOCTL_VIDEO_SET_COLOR_REGISTERS (required if the device has a palette)
//        IOCTL_VIDEO_DISABLE_POINTER
//        IOCTL_VIDEO_ENABLE_POINTER
//        IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES
//        IOCTL_VIDEO_QUERY_POINTER_ATTR
//        IOCTL_VIDEO_SET_POINTER_ATTR
//        IOCTL_VIDEO_QUERY_POINTER_POSITION
//        IOCTL_VIDEO_SET_POINTER_POSITION
//        IOCTL_VIDEO_SAVE_HARDWARE_STATE
//        IOCTL_VIDEO_RESTORE_HARDWARE_STATE
//        IOCTL_VIDEO_DISABLE_CURSOR
//        IOCTL_VIDEO_ENABLE_CURSOR
//        IOCTL_VIDEO_QUERY_CURSOR_ATTR
//        IOCTL_VIDEO_SET_CURSOR_ATTR
//        IOCTL_VIDEO_QUERY_CURSOR_POSITION
//        IOCTL_VIDEO_SET_CURSOR_POSITION
//        IOCTL_VIDEO_GET_BANK_SELECT_CODE
//        IOCTL_VIDEO_SET_PALETTE_REGISTERS
//        IOCTL_VIDEO_LOAD_AND_SET_FONT
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    PVOID                  DeviceExtension
//    PVIDEO_REQUEST_PACKET  RequestPacket
//  RETURNS:
//    BOOLEAN  This function must return TRUE, and complete the work or
//             set an error status in the VRP.

BOOLEAN NTAPI
VGAStartIO(PVOID DeviceExtension,
           PVIDEO_REQUEST_PACKET RequestPacket)
{
  BOOLEAN Result;

  RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;

  switch (RequestPacket->IoControlCode)
    {
    case  IOCTL_VIDEO_MAP_VIDEO_MEMORY:
      if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION) ||
          RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAMapVideoMemory(DeviceExtension,
			(PVIDEO_MEMORY) RequestPacket->InputBuffer,
                         (PVIDEO_MEMORY_INFORMATION)
                         RequestPacket->OutputBuffer,
                         RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_QUERY_AVAIL_MODES:
      if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAQueryAvailModes((PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                         RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_QUERY_CURRENT_MODE:
      if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAQueryCurrentMode((PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                          RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
      if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAQueryNumAvailModes((PVIDEO_NUM_MODES) RequestPacket->OutputBuffer,
                            RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_RESET_DEVICE:
      VGAResetDevice(RequestPacket->StatusBlock);
      Result = TRUE;
      break;

    case  IOCTL_VIDEO_SET_COLOR_REGISTERS:
      if (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT) ||
          RequestPacket->InputBufferLength <
          (((PVIDEO_CLUT)RequestPacket->InputBuffer)->NumEntries * sizeof(ULONG)) +
          FIELD_OFFSET(VIDEO_CLUT, LookupTable))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGASetColorRegisters((PVIDEO_CLUT) RequestPacket->InputBuffer,
                           RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_SET_CURRENT_MODE:
      if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGASetCurrentMode((PVIDEO_MODE) RequestPacket->InputBuffer,
                        RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
      if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION) ||
          RequestPacket->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAShareVideoMemory((PVIDEO_SHARE_MEMORY) RequestPacket->InputBuffer,
                          (PVIDEO_MEMORY_INFORMATION) RequestPacket->OutputBuffer,
                          RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
      if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAUnmapVideoMemory(DeviceExtension,
			  (PVIDEO_MEMORY) RequestPacket->InputBuffer,
                          RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
      if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
      {
        RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return TRUE;
      }
      Result = VGAUnshareVideoMemory((PVIDEO_MEMORY) RequestPacket->InputBuffer,
                            RequestPacket->StatusBlock);
      break;
    case  IOCTL_VIDEO_SET_PALETTE_REGISTERS:
      Result = VGASetPaletteRegisters((PUSHORT) RequestPacket->InputBuffer,
                             RequestPacket->StatusBlock);
      break;

#if 0
    case  IOCTL_VIDEO_DISABLE_CURSOR:
    case  IOCTL_VIDEO_DISABLE_POINTER:
    case  IOCTL_VIDEO_ENABLE_CURSOR:
    case  IOCTL_VIDEO_ENABLE_POINTER:

    case  IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:
      VGAFreePublicAccessRanges((PVIDEO_PUBLIC_ACCESS_RANGES)
                                  RequestPacket->InputBuffer,
                                RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_GET_BANK_SELECT_CODE:
    case  IOCTL_VIDEO_GET_POWER_MANAGEMENT:
    case  IOCTL_VIDEO_LOAD_AND_SET_FONT:
    case  IOCTL_VIDEO_QUERY_CURSOR_POSITION:
    case  IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES:
    case  IOCTL_VIDEO_QUERY_CURSOR_ATTR:
    case  IOCTL_VIDEO_QUERY_POINTER_ATTR:
    case  IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES:
    case  IOCTL_VIDEO_QUERY_POINTER_POSITION:

    case  IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:
      VGAQueryPublicAccessRanges((PVIDEO_PUBLIC_ACCESS_RANGES)
                                   RequestPacket->OutputBuffer,
                                 RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_RESTORE_HARDWARE_STATE:
    case  IOCTL_VIDEO_SAVE_HARDWARE_STATE:
    case  IOCTL_VIDEO_SET_CURSOR_ATTR:
    case  IOCTL_VIDEO_SET_CURSOR_POSITION:
    case  IOCTL_VIDEO_SET_POINTER_ATTR:
    case  IOCTL_VIDEO_SET_POINTER_POSITION:
    case  IOCTL_VIDEO_SET_POWER_MANAGEMENT:

#endif

    default:
      RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
      return FALSE;
    }

  if (Result)
    RequestPacket->StatusBlock->Status = NO_ERROR;

  return TRUE;
}

#if 0
//    VGAInterrupt
//
//  DESCRIPTION:
//    This function will be called upon receipt of a adapter generated
//    interrupt when enabled.
//
//  RUN LEVEL:
//    IRQL
//
//  ARGUMENTS:
//    PVOID                  DeviceExtension
//  RETURNS:
//    BOOLEAN  TRUE if the interrupt was handled by the routine

static BOOLEAN NTAPI
VGAInterrupt(PVOID DeviceExtension)
{
  return(TRUE);
}
#endif

//    VGAResetHw
//
//  DESCRIPTION:
//    This function is called to reset the hardware to a known state
//    if calling a BIOS int 10 reset will not achieve this result.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    PVOID  DeviceExtension
//    ULONG  Columns          Columns and Rows specify the mode parameters
//    ULONG  Rows               to reset to.
//  RETURNS:
//    BOOLEAN  TRUE if no further action is necessary, FALSE if the system
//             needs to still do a BIOS int 10 reset.

BOOLEAN NTAPI
VGAResetHw(PVOID DeviceExtension,
	   ULONG Columns,
	   ULONG Rows)
{
  /* We don't anything to the vga that int10 can't cope with. */
  return(FALSE);
}

#if 0
//    VGATimer
//
//  DESCRIPTION:
//    This function will be called once a second when enabled
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    PVOID  DeviceExtension
//  RETURNS:
//    VOID

static VOID NTAPI
VGATimer(PVOID DeviceExtension)
{
}

#endif

BOOLEAN  VGAMapVideoMemory(IN PVOID DeviceExtension,
			IN PVIDEO_MEMORY  RequestedAddress,
                        OUT PVIDEO_MEMORY_INFORMATION  MapInformation,
                        OUT PSTATUS_BLOCK  StatusBlock)
{
  ULONG ReturnedLength;
  PVOID ReturnedAddress;
  ULONG IoSpace;
  PHYSICAL_ADDRESS FrameBufferBase;
  ReturnedAddress = RequestedAddress->RequestedVirtualAddress;
  ReturnedLength = 256 * 1024;
  FrameBufferBase.QuadPart = 0xA0000;
  IoSpace = VIDEO_MEMORY_SPACE_MEMORY;
  StatusBlock->Status = VideoPortMapMemory(DeviceExtension,
					   FrameBufferBase,
					   &ReturnedLength,
					   &IoSpace,
					   &ReturnedAddress);
  if (StatusBlock->Status != 0)
    {
      StatusBlock->Information = 0;
      return TRUE;
    }
  MapInformation->VideoRamBase = MapInformation->FrameBufferBase =
    ReturnedAddress;
  MapInformation->VideoRamLength = MapInformation->FrameBufferLength =
    ReturnedLength;
  StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);
  return TRUE;
}

BOOLEAN  VGAQueryAvailModes(OUT PVIDEO_MODE_INFORMATION  ReturnedModes,
                         OUT PSTATUS_BLOCK  StatusBlock)
{
  /* Only one mode exists in VGA (640x480), so use VGAQueryCurrentMode */
  return VGAQueryCurrentMode(ReturnedModes, StatusBlock);
}

BOOLEAN  VGAQueryCurrentMode(OUT PVIDEO_MODE_INFORMATION  CurrentMode,
                          OUT PSTATUS_BLOCK  StatusBlock)
{
  CurrentMode->Length = sizeof(VIDEO_MODE_INFORMATION);
  CurrentMode->ModeIndex = 2;
  CurrentMode->VisScreenWidth = 640;
  CurrentMode->VisScreenHeight = 480;
  CurrentMode->ScreenStride = 80;
  CurrentMode->NumberOfPlanes = 4;
  CurrentMode->BitsPerPlane = 1;
  CurrentMode->Frequency = 60;
  CurrentMode->XMillimeter = 320;
  CurrentMode->YMillimeter = 240;
  CurrentMode->NumberRedBits =
  CurrentMode->NumberGreenBits =
  CurrentMode->NumberBlueBits = 6;
  CurrentMode->RedMask =
  CurrentMode->GreenMask =
  CurrentMode->BlueMask = 0;
  CurrentMode->VideoMemoryBitmapWidth = 640;
  CurrentMode->VideoMemoryBitmapHeight = 480;
  CurrentMode->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
      VIDEO_MODE_NO_OFF_SCREEN;
  CurrentMode->DriverSpecificAttributeFlags = 0;

  StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION);
  return TRUE;
}

BOOLEAN  VGAQueryNumAvailModes(OUT PVIDEO_NUM_MODES  NumberOfModes,
                            OUT PSTATUS_BLOCK  StatusBlock)
{
  NumberOfModes->NumModes = 1;
  NumberOfModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
  StatusBlock->Information = sizeof(VIDEO_NUM_MODES);
  return TRUE;
}

BOOLEAN  VGASetPaletteRegisters(IN PUSHORT  PaletteRegisters,
                             OUT PSTATUS_BLOCK  StatusBlock)
{
  ;

/*
  We don't need the following code because the palette registers are set correctly on VGA initialization.
  Still, we may include\test this is in the future.

  int i, j = 2;
  char tmp, v;

  tmp = VideoPortReadPortUchar(0x03da);
  v = VideoPortReadPortUchar(0x03c0);

  // Set the first 16 palette registers to map to the first 16 palette colors
  for (i=PaletteRegisters[1]; i<PaletteRegisters[0]; i++)
  {
    tmp = VideoPortReadPortUchar(0x03da);
    VideoPortWritePortUchar(0x03c0, i);
    VideoPortWritePortUchar(0x03c0, PaletteRegisters[j++]);
  }

  tmp = VideoPortReadPortUchar(0x03da);
  VideoPortWritePortUchar(0x03d0, v | 0x20);
*/
  return TRUE;
}

BOOLEAN  VGASetColorRegisters(IN PVIDEO_CLUT  ColorLookUpTable,
                           OUT PSTATUS_BLOCK  StatusBlock)
{
  int i;

  for (i=ColorLookUpTable->FirstEntry; i<ColorLookUpTable->NumEntries; i++)
  {
    VideoPortWritePortUchar((PUCHAR)0x03c8, i);
    VideoPortWritePortUchar((PUCHAR)0x03c9, ColorLookUpTable->LookupTable[i].RgbArray.Red);
    VideoPortWritePortUchar((PUCHAR)0x03c9, ColorLookUpTable->LookupTable[i].RgbArray.Green);
    VideoPortWritePortUchar((PUCHAR)0x03c9, ColorLookUpTable->LookupTable[i].RgbArray.Blue);
  }

  return TRUE;
}

BOOLEAN  VGASetCurrentMode(IN PVIDEO_MODE  RequestedMode,
                        OUT PSTATUS_BLOCK  StatusBlock)
{
  if(RequestedMode->RequestedMode == 12)
  {
    InitVGAMode();
    return TRUE;
  } else {
    VideoPortDebugPrint(Warn, "Unrecognised mode for VGASetCurrentMode\n");
    return FALSE;
  }
}

BOOLEAN  VGAShareVideoMemory(IN PVIDEO_SHARE_MEMORY  RequestedMemory,
                          OUT PVIDEO_MEMORY_INFORMATION  ReturnedMemory,
                          OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;

  StatusBlock->Status = ERROR_INVALID_FUNCTION;
  return FALSE;
}

BOOLEAN  VGAUnmapVideoMemory(IN PVOID DeviceExtension,
			  IN PVIDEO_MEMORY  MemoryToUnmap,
                          OUT PSTATUS_BLOCK  StatusBlock)
{
  if (VideoPortUnmapMemory(DeviceExtension,
		       MemoryToUnmap->RequestedVirtualAddress,
		       0) == NO_ERROR)
    return TRUE;
  else
    return FALSE;
}

BOOLEAN  VGAUnshareVideoMemory(IN PVIDEO_MEMORY  MemoryToUnshare,
                            OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;

  StatusBlock->Status = ERROR_INVALID_FUNCTION;
  return FALSE;
}
