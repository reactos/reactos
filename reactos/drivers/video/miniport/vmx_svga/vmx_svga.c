/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/video/miniport/vmx_svga/vmx_svga.c
 * PURPOSE:         VMWARE SVGA-II Card Main Driver File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* GLOBALS ********************************************************************/

PHW_DEVICE_EXTENSION VmxDeviceExtensionArray[SVGA_MAX_DISPLAYS];

/* FUNCTIONS ******************************************************************/

VP_STATUS
NTAPI
VmxFindAdapter(IN PVOID HwDeviceExtension,
               IN PVOID HwContext,
               IN PWSTR ArgumentString,
               IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
               OUT PUCHAR Again)
{
    return NO_ERROR;
}

BOOLEAN
NTAPI
VmxInitialize(IN PVOID HwDeviceExtension)
{
    return TRUE;
}

BOOLEAN
NTAPI
VmxStartIO(IN PVOID HwDeviceExtension,
           IN PVIDEO_REQUEST_PACKET RequestPacket)
{
    return TRUE;
}

BOOLEAN
NTAPI
VmxResetHw(IN PVOID DeviceExtension,
           IN ULONG Columns,
           IN ULONG Rows)
{
    return FALSE;
}

VP_STATUS
NTAPI
VmxGetPowerState(IN PVOID HwDeviceExtension,
                 IN ULONG HwId,
                 IN PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    return NO_ERROR;
}

VP_STATUS
NTAPI
VmxSetPowerState(IN PVOID HwDeviceExtension,
                 IN ULONG HwId,
                 IN PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{

   return NO_ERROR;
}

BOOLEAN
NTAPI
VmxInterrupt(IN PVOID HwDeviceExtension)
{
    return TRUE;
}

VP_STATUS
NTAPI
VmxGetVideoChildDescriptor(IN PVOID HwDeviceExtension,
                           IN PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
                           OUT PVIDEO_CHILD_TYPE VideoChildType,
                           OUT PUCHAR pChildDescriptor,
                           OUT PULONG UId,
                           OUT PULONG pUnused)
{
    return NO_ERROR;
}

VP_STATUS
NTAPI
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA InitData;

    /* Zero initialization structure and array of extensions, one per screen */
    VideoPortZeroMemory(VmxDeviceExtensionArray, sizeof(VmxDeviceExtensionArray));
    VideoPortZeroMemory(&InitData, sizeof(InitData));
    
    /* Setup the initialization structure with VideoPort */
    InitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    InitData.HwFindAdapter = VmxFindAdapter;
    InitData.HwInitialize = VmxInitialize;
    InitData.HwInterrupt = VmxInterrupt;
    InitData.HwStartIO = VmxStartIO;
    InitData.HwResetHw = VmxResetHw;
    InitData.HwGetPowerState = VmxGetPowerState;
    InitData.HwSetPowerState = VmxSetPowerState;
    InitData.HwGetVideoChildDescriptor = VmxGetVideoChildDescriptor;
    InitData.AdapterInterfaceType = PCIBus;
    InitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    InitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    return VideoPortInitialize(Context1, Context2, &InitData, NULL);
}
