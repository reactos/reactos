/*
 * VGA.C - a generic VGA miniport driver
 * 
 */

#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#define UNIMPLEMENTED do {DbgPrint("%s:%d: Function not implemented", __FILE__, __LINE__); for(;;);} while (0)

#define VERSION "0.0.0"

//  ----------------------------------------------------  Forward Declarations
static VP_STATUS VGAFindAdapter(PVOID  DeviceExtension, 
                                PVOID  Context, 
                                PWSTR  ArgumentString, 
                                PVIDEO_PORT_CONFIG_INFO  ConfigInfo, 
                                PUCHAR  Again);
static BOOLEAN VGAInitialize(PVOID  DeviceExtension);
static BOOLEAN VGAStartIO(PVOID  DeviceExtension, 
                          PVIDEO_REQUEST_PACKET  RequestPacket); 
/*
static BOOLEAN VGAInterrupt(PVOID  DeviceExtension); 
static BOOLEAN VGAResetHw(PVOID  DeviceExtension, 
                          ULONG  Columns, 
                          ULONG  Rows); 
static VOID VGATimer(PVOID  DeviceExtension);
*/

/*  Mandatory IoControl routines  */
VOID  VGAMapVideoMemory(IN PVIDEO_MEMORY  RequestedAddress,
                        OUT PVIDEO_MEMORY_INFORMATION  MapInformation,
                        OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAQueryAvailModes(OUT PVIDEO_MODE_INFORMATION  ReturnedModes,
                         OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAQueryCurrentMode(OUT PVIDEO_MODE_INFORMATION  CurrentMode,
                          OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAQueryNumAvailModes(OUT PVIDEO_NUM_MODES  NumberOfModes,
                            OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAResetDevice(OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGASetColorRegisters(IN PVIDEO_CLUT  ColorLookUpTable,
                           OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGASetCurrentMode(IN PVIDEO_MODE  RequestedMode,
                        OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAShareVideoMemory(IN PVIDEO_SHARE_MEMORY  RequestedMemory,
                          OUT PVIDEO_MEMORY_INFORMATION  ReturnedMemory,
                          OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAUnmapVideoMemory(IN PVIDEO_MEMORY  MemoryToUnmap,
                          OUT PSTATUS_BLOCK  StatusBlock);
VOID  VGAUnshareVideoMemory(IN PVIDEO_MEMORY  MemoryToUnshare,
                            OUT PSTATUS_BLOCK  StatusBlock);

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
//    VP_STATUS

VP_STATUS STDCALL
DriverEntry(IN PVOID Context1, 
            IN PVOID Context2) 
{
  VIDEO_HW_INITIALIZATION_DATA  InitData;
  
  DbgPrint("VGA miniport Driver %s\n", VERSION);

  VideoPortZeroMemory(&InitData, sizeof InitData);
  
  /* FIXME: Fill in InitData members  */
  InitData.StartingDeviceNumber = 0;
  
  /*  Export driver entry points...  */
  InitData.HwFindAdapter = VGAFindAdapter;
  InitData.HwInitialize = VGAInitialize;
  InitData.HwStartIO = VGAStartIO;
  /* InitData.HwInterrupt = VGAInterrupt;  */
  /* InitData.HwResetHw = VGAResetHw;  */
  /* InitData.HwTimer = VGATimer;  */
  
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

static VP_STATUS 
VGAFindAdapter(PVOID  DeviceExtension, 
               PVOID  Context, 
               PWSTR  ArgumentString, 
               PVIDEO_PORT_CONFIG_INFO  ConfigInfo, 
               PUCHAR  Again)
{
  /* FIXME: Determine if the adapter is present  */
  *Again = FALSE;
  return  ERROR_DEV_NOT_EXIST;
  
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
static BOOLEAN 
VGAInitialize(PVOID  DeviceExtension)
{
  return  FALSE;
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

static BOOLEAN 
VGAStartIO(PVOID  DeviceExtension, 
           PVIDEO_REQUEST_PACKET  RequestPacket)
{
  switch (RequestPacket->IoControlCode)
    {
    case  IOCTL_VIDEO_MAP_VIDEO_MEMORY:
      VGAMapVideoMemory((PVIDEO_MEMORY) RequestPacket->InputBuffer,
                        (PVIDEO_MEMORY_INFORMATION) 
                          RequestPacket->OutputBuffer,
                        &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_QUERY_AVAIL_MODES:
      VGAQueryAvailModes((PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                         &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_QUERY_CURRENT_MODE:
      VGAQueryCurrentMode((PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                          &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
      VGAQueryNumAvailModes((PVIDEO_NUM_MODES) RequestPacket->OutputBuffer,
                            &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_RESET_DEVICE:
      VGAResetDevice(&RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_SET_COLOR_REGISTERS:
      VGASetColorRegisters((PVIDEO_CLUT) RequestPacket->InputBuffer,
                           &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_SET_CURRENT_MODE:
      VGASetCurrentMode((PVIDEO_MODE) RequestPacket->InputBuffer,
                        &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_SHARE_VIDEO_MEMORY:
      VGAShareVideoMemory((PVIDEO_SHARE_MEMORY) RequestPacket->InputBuffer,
                          (PVIDEO_MEMORY_INFORMATION) RequestPacket->OutputBuffer,
                          &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
      VGAUnmapVideoMemory((PVIDEO_MEMORY) RequestPacket->InputBuffer,
                          &RequestPacket->StatusBlock);
      break;
      
    case  IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:
      VGAUnshareVideoMemory((PVIDEO_MEMORY) RequestPacket->InputBuffer,
                            &RequestPacket->StatusBlock);
      break;

#if 0
    case  IOCTL_VIDEO_DISABLE_CURSOR:
    case  IOCTL_VIDEO_DISABLE_POINTER:
    case  IOCTL_VIDEO_ENABLE_CURSOR:
    case  IOCTL_VIDEO_ENABLE_POINTER:

    case  IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:
      VGAFreePublicAccessRanges((PVIDEO_PUBLIC_ACCESS_RANGES)
                                  RequestPacket->InputBuffer,
                                &RequestPacket->StatusBlock);
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
                                 &RequestPacket->StatusBlock);
      break;

    case  IOCTL_VIDEO_RESTORE_HARDWARE_STATE:
    case  IOCTL_VIDEO_SAVE_HARDWARE_STATE:
    case  IOCTL_VIDEO_SET_CURSOR_ATTR:
    case  IOCTL_VIDEO_SET_CURSOR_POSITION:
    case  IOCTL_VIDEO_SET_PALETTE_REGISTERS:
    case  IOCTL_VIDEO_SET_POINTER_ATTR:
    case  IOCTL_VIDEO_SET_POINTER_POSITION:
    case  IOCTL_VIDEO_SET_POWER_MANAGEMENT:

#endif    
      
    default:
      RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
      break;
    }
  
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

static BOOLEAN VGAInterrupt(PVOID  DeviceExtension); 

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
//             needs to still do a BOIS int 10 reset.

static BOOLEAN VGAResetHw(PVOID  DeviceExtension, 
                          ULONG  Columns, 
                          ULONG  Rows);

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

static VOID VGATimer(PVOID  DeviceExtension);

#endif

VOID  VGAMapVideoMemory(IN PVIDEO_MEMORY  RequestedAddress,
                        OUT PVIDEO_MEMORY_INFORMATION  MapInformation,
                        OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAQueryAvailModes(OUT PVIDEO_MODE_INFORMATION  ReturnedModes,
                         OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAQueryCurrentMode(OUT PVIDEO_MODE_INFORMATION  CurrentMode,
                          OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAQueryNumAvailModes(OUT PVIDEO_NUM_MODES  NumberOfModes,
                            OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAResetDevice(OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGASetColorRegisters(IN PVIDEO_CLUT  ColorLookUpTable,
                           OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGASetCurrentMode(IN PVIDEO_MODE  RequestedMode,
                        OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAShareVideoMemory(IN PVIDEO_SHARE_MEMORY  RequestedMemory,
                          OUT PVIDEO_MEMORY_INFORMATION  ReturnedMemory,
                          OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAUnmapVideoMemory(IN PVIDEO_MEMORY  MemoryToUnmap,
                          OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}

VOID  VGAUnshareVideoMemory(IN PVIDEO_MEMORY  MemoryToUnshare,
                            OUT PSTATUS_BLOCK  StatusBlock)
{
  UNIMPLEMENTED;
}



