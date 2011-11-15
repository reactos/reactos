/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/video/miniport/vmx_svga/vmx_svga.c
 * PURPOSE:         VMWARE SVGA-II Card Main Driver File
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"
#include "debug.h"

/* GLOBALS ********************************************************************/

PHW_DEVICE_EXTENSION VmxDeviceExtensionArray[SVGA_MAX_DISPLAYS];
static WCHAR AdapterString[] = L"VMware SVGA II";

/* FUNCTIONS ******************************************************************/

ULONG
NTAPI
VmxReadUlong(IN PHW_DEVICE_EXTENSION DeviceExtension,
             IN ULONG Index)
{
    /* Program the index first, then read the value */
    VideoPortWritePortUlong(DeviceExtension->IndexPort, Index);
    return VideoPortReadPortUlong(DeviceExtension->ValuePort);
}

VOID
NTAPI
VmxWriteUlong(IN PHW_DEVICE_EXTENSION DeviceExtension,
              IN ULONG Index,
              IN ULONG Value)
{
    /* Program the index first, then write the value */
    VideoPortWritePortUlong(DeviceExtension->IndexPort, Index);
    VideoPortWritePortUlong(DeviceExtension->ValuePort, Value);
}

ULONG
NTAPI
VmxInitModes(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    /* Not here yet */
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

VP_STATUS
NTAPI
VmxInitDevice(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    /* Not here yet */
    UNIMPLEMENTED;
    while (TRUE);
    return NO_ERROR;
}

BOOLEAN
NTAPI
VmxIsMultiMon(IN PHW_DEVICE_EXTENSION DeviceExtension)
{
    ULONG Capabilities;

    /* Get the caps */
    Capabilities = DeviceExtension->Capabilities;

    /* Check for multi-mon support */
    if ((Capabilities & SVGA_CAP_MULTIMON) && (Capabilities & SVGA_CAP_PITCHLOCK))
    {
        /* Query the monitor count */
        if (VmxReadUlong(DeviceExtension, SVGA_REG_NUM_DISPLAYS) > 1) return TRUE;
    }

    /* Either no support, or just one screen */
    return FALSE;
}

VP_STATUS
NTAPI
VmxFindAdapter(IN PVOID HwDeviceExtension,
               IN PVOID HwContext,
               IN PWSTR ArgumentString,
               IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
               OUT PUCHAR Again)
{
    VP_STATUS Status;
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    DPRINT1("VMX searching for adapter\n");

    /* Zero out the fields */
    VideoPortZeroMemory(DeviceExtension, sizeof(HW_DEVICE_EXTENSION));

    /* Validate the Config Info */
    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
    {
        /* Incorrect OS version? */
        DPRINT1("Invalid configuration info\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Initialize the device extension and find the adapter */
    Status = VmxInitDevice(DeviceExtension);
    DPRINT1("Init status: %lx\n", Status);
    if (Status != NO_ERROR) return ERROR_DEV_NOT_EXIST;

    /* Save this adapter extension */
    VmxDeviceExtensionArray[0] = DeviceExtension;

    /* Create the sync event */
    VideoPortCreateEvent(DeviceExtension,
                         NOTIFICATION_EVENT,
                         NULL,
                         &DeviceExtension->SyncEvent);

    /* Check for multi-monitor configuration */
    if (VmxIsMultiMon(DeviceExtension))
    {
        /* Let's not go so far */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Zero the frame buffer */
    VideoPortZeroMemory((PVOID)DeviceExtension->FrameBuffer.LowPart,
                        DeviceExtension->VramSize.LowPart);

    /* Initialize the video modes */
    VmxInitModes(DeviceExtension);

    /* Setup registry keys */
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   AdapterString,
                                   sizeof(AdapterString));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.DacType",
                                   AdapterString,
                                   sizeof(AdapterString));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &DeviceExtension->VramSize.LowPart,
                                   sizeof(ULONG));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   AdapterString,
                                   sizeof(AdapterString));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.BiosString",
                                   AdapterString,
                                   sizeof(AdapterString));

    /* No VDM support */
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntriesContext = 0;
    ConfigInfo->HardwareStateSize = 0;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.QuadPart = 0;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0;

    /* Write that this is Windows XP or higher */
    VmxWriteUlong(DeviceExtension, SVGA_REG_GUEST_ID, 0x5000 | 0x08);
    return NO_ERROR;
}

BOOLEAN
NTAPI
VmxInitialize(IN PVOID HwDeviceExtension)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
VmxStartIO(IN PVOID HwDeviceExtension,
           IN PVIDEO_REQUEST_PACKET RequestPacket)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
VmxResetHw(IN PVOID DeviceExtension,
           IN ULONG Columns,
           IN ULONG Rows)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VP_STATUS
NTAPI
VmxGetPowerState(IN PVOID HwDeviceExtension,
                 IN ULONG HwId,
                 IN PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NO_ERROR;
}

VP_STATUS
NTAPI
VmxSetPowerState(IN PVOID HwDeviceExtension,
                 IN ULONG HwId,
                 IN PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NO_ERROR;
}

BOOLEAN
NTAPI
VmxInterrupt(IN PVOID HwDeviceExtension)
{
    UNIMPLEMENTED;
    while (TRUE);
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
    UNIMPLEMENTED;
    while (TRUE);
    return NO_ERROR;
}

ULONG
NTAPI
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA InitData;

    /* Zero initialization structure and array of extensions, one per screen */
    DPRINT1("VMX-SVGAII Loading...\n");
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
