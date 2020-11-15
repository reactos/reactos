/*
 * PROJECT:     ReactOS framebuffer driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport driver entrypoint
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "pc98vid.h"

/* GLOBALS ********************************************************************/

const VIDEOMODE VideoModes[] =
{
    {640, 480, GRAPH_HF_31KHZ, GDC2_CLOCK1_5MHZ, GDC2_CLOCK2_5MHZ,
     GDC2_MODE_LINES_800, 60,
     {0, 80, 12, 2, 4, 4, 6, 480, 37}, {0, 80, 12, 2, 4, 132, 6, 480, 37}}
};

static VIDEO_ACCESS_RANGE LegacyRangeList[] =
{
    { {{0x60,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x62,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x68,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x6A,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x7C,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xA0,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xA2,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xA4,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xA6,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xA8,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xAA,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xAC,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xAE,  0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x9A0, 0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x9A2, 0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0x9A8, 0}}, 0x00000001, 1, 1, 1, 0 },
    { {{0xFAC, 0}}, 0x00000001, 1, 1, 1, 0 },
    { {{VRAM_NORMAL_PLANE_I,     0}}, PEGC_CONTROL_SIZE, 0, 0, 1, 0 },
    { {{PEGC_FRAMEBUFFER_PACKED, 0}}, PEGC_FRAMEBUFFER_SIZE, 0, 0, 1, 0 }
};
#define CONTROL_RANGE_INDEX     17
#define FRAMEBUFFER_RANGE_INDEX 18

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VP_STATUS
NTAPI
Pc98VidFindAdapter(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID HwContext,
    _In_opt_ PWSTR ArgumentString,
    _Inout_ PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    _Out_ PUCHAR Again)
{
    VP_STATUS Status;
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;
    static WCHAR AdapterChipType[] = L"Onboard";
    static WCHAR AdapterDacType[] = L"8 bit";
    static WCHAR AdapterString[] = L"PEGC";

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO))
        return ERROR_INVALID_PARAMETER;

    Status = VideoPortVerifyAccessRanges(DeviceExtension,
                                         RTL_NUMBER_OF(LegacyRangeList),
                                         LegacyRangeList);
    if (Status != NO_ERROR)
    {
        VideoDebugPrint((Error, "%s() Resource conflict was found\n", __FUNCTION__));

        return ERROR_INVALID_PARAMETER;
    }

    DeviceExtension->PegcControl = LegacyRangeList[CONTROL_RANGE_INDEX].RangeStart;
    DeviceExtension->PegcControlLength = LegacyRangeList[CONTROL_RANGE_INDEX].RangeLength;
    DeviceExtension->FrameBuffer = LegacyRangeList[FRAMEBUFFER_RANGE_INDEX].RangeStart;
    DeviceExtension->FrameBufferLength = LegacyRangeList[FRAMEBUFFER_RANGE_INDEX].RangeLength;

    Status = VideoPortMapMemory(DeviceExtension,
                                DeviceExtension->PegcControl,
                                &DeviceExtension->PegcControlLength,
                                &inIoSpace,
                                (PVOID)&DeviceExtension->PegcControlVa);
    if (Status != NO_ERROR)
    {
        VideoDebugPrint((Error, "%s() Failed to map control memory\n", __FUNCTION__));

        VideoPortVerifyAccessRanges(DeviceExtension, 0, NULL);

        return ERROR_DEV_NOT_EXIST;
    }

    if (!HasPegcController(DeviceExtension))
    {
        VideoDebugPrint((Error, "%s() Unsupported hardware\n", __FUNCTION__));

        VideoPortVerifyAccessRanges(DeviceExtension, 0, NULL);
        VideoPortUnmapMemory(DeviceExtension,
                             (PVOID)DeviceExtension->PegcControlVa,
                             NULL);

        return ERROR_DEV_NOT_EXIST;
    }

    /* Not VGA-compatible */
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;
    ConfigInfo->HardwareStateSize = 0;
    ConfigInfo->VdmPhysicalVideoMemoryAddress.QuadPart = 0;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0;

    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.ChipType",
                                   AdapterChipType,
                                   sizeof(AdapterChipType));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.DacType",
                                   AdapterDacType,
                                   sizeof(AdapterDacType));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.MemorySize",
                                   &DeviceExtension->FrameBufferLength,
                                   sizeof(ULONG));
    VideoPortSetRegistryParameters(DeviceExtension,
                                   L"HardwareInformation.AdapterString",
                                   AdapterString,
                                   sizeof(AdapterString));

    return NO_ERROR;
}

static
CODE_SEG("PAGE")
BOOLEAN
NTAPI
Pc98VidInitialize(
    _In_ PVOID HwDeviceExtension)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    DeviceExtension->ModeCount = RTL_NUMBER_OF(VideoModes);
    DeviceExtension->MonitorCount = 1;

    return TRUE;
}

static
CODE_SEG("PAGE")
VP_STATUS
NTAPI
Pc98VidGetVideoChildDescriptor(
    _In_ PVOID HwDeviceExtension,
    _In_ PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    _Out_ PVIDEO_CHILD_TYPE VideoChildType,
    _Out_ PUCHAR pChildDescriptor,
    _Out_ PULONG UId,
    _Out_ PULONG pUnused)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    UNREFERENCED_PARAMETER(pChildDescriptor);

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Index %d\n",
                     __FUNCTION__, ChildEnumInfo->ChildIndex));

    *pUnused = 0;

    if (ChildEnumInfo->ChildIndex > 0 &&
        ChildEnumInfo->ChildIndex <= DeviceExtension->MonitorCount)
    {
        *VideoChildType = Monitor;
        *UId = MONITOR_HW_ID;

        return VIDEO_ENUM_MORE_DEVICES;
    }

    return ERROR_NO_MORE_DEVICES;
}

CODE_SEG("INIT")
ULONG
NTAPI
DriverEntry(
    _In_ PVOID Context1,
    _In_ PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA InitData;
    ULONG Status;
    BOOLEAN IsLiveCd;

    VideoDebugPrint((Trace, "(%s:%d) %s()\n",
                     __FILE__, __LINE__, __FUNCTION__));

    // FIXME: Detect IsLiveCd
    IsLiveCd = TRUE;

    VideoPortZeroMemory(&InitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));
    InitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    InitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    InitData.HwFindAdapter = Pc98VidFindAdapter;
    InitData.HwInitialize = Pc98VidInitialize;
    InitData.HwStartIO = Pc98VidStartIO;
    /*
     * On LiveCD, we expect to see the initialized video
     * before starting the device enumeration,
     * so we should mark the driver as non-PnP miniport.
     */
    if (!IsLiveCd)
    {
        InitData.HwGetPowerState = Pc98VidGetPowerState;
        InitData.HwSetPowerState = Pc98VidSetPowerState;
        InitData.HwGetVideoChildDescriptor = Pc98VidGetVideoChildDescriptor;
    }

    InitData.HwLegacyResourceList = LegacyRangeList;
    InitData.HwLegacyResourceCount = RTL_NUMBER_OF(LegacyRangeList);

    InitData.AdapterInterfaceType = Isa;

    Status = VideoPortInitialize(Context1, Context2, &InitData, NULL);
    if (!NT_SUCCESS(Status))
    {
        VideoDebugPrint((Error, "(%s:%d) %s() Initialization failed 0x%lX\n",
                         __FILE__, __LINE__, __FUNCTION__, Status));
    }

    return Status;
}
