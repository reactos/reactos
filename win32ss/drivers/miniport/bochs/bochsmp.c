/*
 * PROJECT:     ReactOS Bochs graphics card driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Bochs graphics card driver
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "bochsmp.h"

static const BOCHS_SIZE BochsAvailableResolutions[] = {
    { 640, 480 },   // VGA
    { 800, 600 },   // SVGA
    { 1024, 600 },  // WSVGA
    { 1024, 768 },  // XGA
    { 1152, 864 },  // XGA+
    { 1280, 720 },  // WXGA-H
    { 1280, 768 },  // WXGA
    { 1280, 960 },  // SXGA-
    { 1280, 1024 }, // SXGA
    { 1368, 768 },  // HD ready
    { 1400, 1050 }, // SXGA+
    { 1440, 900 },  // WSXGA
    { 1600, 900 },  // HD+
    { 1600, 1200 }, // UXGA
    { 1680, 1050 }, // WSXGA+
    { 1920, 1080 }, // FHD
    { 2048, 1536 }, // QXGA
    { 2560, 1440 }, // WQHD
    { 2560, 1600 }, // WQXGA
    { 2560, 2048 }, // QSXGA
    { 2800, 2100 }, // QSXGA+
    { 3200, 2400 }, // QUXGA
    { 3840, 2160 }, // 4K UHD-1
};

CODE_SEG("PAGE")
static VOID
BochsFreeResources(
    _Inout_ PBOCHS_DEVICE_EXTENSION DeviceExtension)
{
    if (DeviceExtension->AvailableModeInfo)
    {
        VideoPortFreePool(DeviceExtension, DeviceExtension->AvailableModeInfo);
        DeviceExtension->AvailableModeInfo = NULL;
    }
}

CODE_SEG("PAGE")
static VOID
BochsWriteDispI(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Index,
    _In_ USHORT Value)
{
    if (DeviceExtension->IoPorts.RangeInIoSpace)
    {
        VideoPortWritePortUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped - VBE_DISPI_IOPORT_INDEX + VBE_DISPI_IOPORT_INDEX), Index);
        VideoPortWritePortUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped - VBE_DISPI_IOPORT_INDEX + VBE_DISPI_IOPORT_DATA), Value);
    }
    else
    {
        VideoPortWriteRegisterUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped + 0x500 + Index * 2), Value);
    }
}

CODE_SEG("PAGE")
static USHORT
BochsReadDispI(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Index)
{
    if (DeviceExtension->IoPorts.RangeInIoSpace)
    {
        VideoPortWritePortUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped - VBE_DISPI_IOPORT_INDEX + VBE_DISPI_IOPORT_INDEX), Index);
        return VideoPortReadPortUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped - VBE_DISPI_IOPORT_INDEX + VBE_DISPI_IOPORT_DATA));
    }
    else
    {
        return VideoPortReadRegisterUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped + 0x500 + Index * 2));
    }
}

CODE_SEG("PAGE")
static BOOLEAN
BochsWriteDispIAndCheck(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG Index,
    _In_ USHORT Value)
{
    BochsWriteDispI(DeviceExtension, Index, Value);
    return BochsReadDispI(DeviceExtension, Index) == Value;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsInitializeSuitableModeInfo(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG PotentialModeCount)
{
    ULONG i, ModeCount = 0;

    for (i = 0; i < ARRAYSIZE(BochsAvailableResolutions) && ModeCount < PotentialModeCount; i++)
    {
        if (BochsAvailableResolutions[i].XResolution > DeviceExtension->MaxXResolution)
            continue;
        if (BochsAvailableResolutions[i].YResolution > DeviceExtension->MaxYResolution)
            continue;
        if (BochsAvailableResolutions[i].XResolution * BochsAvailableResolutions[i].YResolution * 4 > DeviceExtension->VramSize64K * 64 * 1024)
            continue;
        DeviceExtension->AvailableModeInfo[ModeCount++] = BochsAvailableResolutions[i];
    }

    if (ModeCount == 0)
    {
        VideoDebugPrint((Error, "Bochs: no suitable modes available!\n"));
        return FALSE;
    }

    DeviceExtension->AvailableModeCount = ModeCount;
    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsGetControllerInfo(
    _Inout_ PBOCHS_DEVICE_EXTENSION DeviceExtension)
{
    USHORT Version;
    WCHAR ChipType[5];
    ULONG SizeInBytes;

    /* Detect DISPI version */
    for (Version = VBE_DISPI_ID5; Version >= VBE_DISPI_ID0; Version--)
    {
        if (BochsWriteDispIAndCheck(DeviceExtension, VBE_DISPI_INDEX_ID, Version))
            break;
    }
    if (Version < VBE_DISPI_ID0)
    {
        VideoDebugPrint((Error, "Bochs: VBE extension signature incorrect\n"));
        return FALSE;
    }
    VideoDebugPrint((Error, "Bochs: detected version 0x%04x\n", Version));
    if (Version < VBE_DISPI_ID2)
    {
        /* Too old (no 32 bpp support, no linear frame buffer) */
        VideoDebugPrint((Error, "Bochs: VBE extension too old (0x%04x)\n", Version));
        return FALSE;
    }

    if (Version <= VBE_DISPI_ID2)
    {
        DeviceExtension->MaxXResolution = 1024;
        DeviceExtension->MaxYResolution = 768;
    }
    else
    {
        BochsWriteDispI(DeviceExtension, VBE_DISPI_INDEX_ENABLE, VBE_DISPI_GETCAPS);
        DeviceExtension->MaxXResolution = BochsReadDispI(DeviceExtension, VBE_DISPI_INDEX_XRES);
        DeviceExtension->MaxYResolution = BochsReadDispI(DeviceExtension, VBE_DISPI_INDEX_YRES);
        BochsWriteDispI(DeviceExtension, VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
        /* Workaround bug in QEMU bochs-display */
        if (DeviceExtension->MaxXResolution == 0 && DeviceExtension->MaxYResolution == 0)
        {
            DeviceExtension->MaxXResolution = 1024;
            DeviceExtension->MaxYResolution = 768;
        }
    }
    if (Version < VBE_DISPI_ID4)
    {
        DeviceExtension->VramSize64K = 4 * 1024 / 64; /* 4 MB */
    }
    else if (Version == VBE_DISPI_ID4)
    {
        DeviceExtension->VramSize64K = 8 * 1024 / 64; /* 8 MB */
    }
    else
    {
        DeviceExtension->VramSize64K = BochsReadDispI(DeviceExtension, VBE_DISPI_INDEX_VIDEO_MEMORY_64K);
    }
    VideoDebugPrint((Info, "Bochs: capabilities %dx%d (%d MB)\n",
                     DeviceExtension->MaxXResolution,
                     DeviceExtension->MaxYResolution,
                     DeviceExtension->VramSize64K * 64 / 1024));

    /* Store information in registry */
#define HEX(c) (((c) >= 0 && (c) <= 9) ? (c) + L'0' : (c) - 10 + L'A')
    ChipType[0] = HEX((Version >> 12) & 0xf);
    ChipType[1] = HEX((Version >> 8) & 0xf);
    ChipType[2] = HEX((Version >> 4) & 0xf);
    ChipType[3] = HEX(Version & 0xf);
    ChipType[4] = UNICODE_NULL;
    VideoPortSetRegistryParameters(DeviceExtension, L"HardwareInformation.ChipType", ChipType, sizeof(ChipType));
    SizeInBytes = DeviceExtension->VramSize64K * 64 * 1024;
    VideoPortSetRegistryParameters(DeviceExtension, L"HardwareInformation.MemorySize", &SizeInBytes, sizeof(SizeInBytes));
    return TRUE;
}

CODE_SEG("PAGE")
static VOID
BochsGetModeInfo(
    _In_ PBOCHS_SIZE AvailableModeInfo,
    _Out_ PVIDEO_MODE_INFORMATION ModeInfo,
    _In_ ULONG Index)
{
    VideoDebugPrint((Info, "Bochs: Filling details of mode #%d\n", Index));

    ModeInfo->Length = sizeof(*ModeInfo);
    ModeInfo->ModeIndex = Index;
    ModeInfo->VisScreenWidth = AvailableModeInfo->XResolution;
    ModeInfo->VisScreenHeight = AvailableModeInfo->YResolution;
    ModeInfo->ScreenStride = AvailableModeInfo->XResolution * 4;
    ModeInfo->NumberOfPlanes = 1;
    ModeInfo->BitsPerPlane = 32;
    ModeInfo->Frequency = 60;

    /* 960 DPI appears to be common */
    ModeInfo->XMillimeter = AvailableModeInfo->XResolution * 254 / 960;
    ModeInfo->YMillimeter = AvailableModeInfo->YResolution * 254 / 960;
    ModeInfo->NumberRedBits = 8;
    ModeInfo->NumberGreenBits = 8;
    ModeInfo->NumberBlueBits = 8;
    ModeInfo->RedMask = 0xff0000;
    ModeInfo->GreenMask = 0x00ff00;
    ModeInfo->BlueMask = 0x0000ff;

    ModeInfo->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR | VIDEO_MODE_NO_OFF_SCREEN;
    ModeInfo->VideoMemoryBitmapWidth = AvailableModeInfo->XResolution;
    ModeInfo->VideoMemoryBitmapHeight = AvailableModeInfo->YResolution;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsMapVideoMemory(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MEMORY RequestedAddress,
    _Out_ PVIDEO_MEMORY_INFORMATION MapInformation,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    VP_STATUS Status;
    PHYSICAL_ADDRESS VideoMemory;
    ULONG MemSpace = VIDEO_MEMORY_SPACE_MEMORY;

    VideoDebugPrint((Info, "Bochs: BochsMapVideoMemory Entry\n"));

    VideoMemory = DeviceExtension->FrameBuffer.RangeStart;
    MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
    MapInformation->VideoRamLength = 4 *
        DeviceExtension->AvailableModeInfo[DeviceExtension->CurrentMode].XResolution *
        DeviceExtension->AvailableModeInfo[DeviceExtension->CurrentMode].YResolution;

    Status = VideoPortMapMemory(DeviceExtension,
                                VideoMemory,
                                &MapInformation->VideoRamLength,
                                &MemSpace,
                                &MapInformation->VideoRamBase);
    if (Status != NO_ERROR)
    {
        VideoDebugPrint((Error, "BochsMapVideoMemory - VideoPortMapMemory failed status:%x\n", Status));
        StatusBlock->Status = Status;
        return FALSE;
    }

    MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
    MapInformation->FrameBufferLength = MapInformation->VideoRamLength;
    StatusBlock->Information = sizeof(*MapInformation);
    StatusBlock->Status = NO_ERROR;

    VideoDebugPrint((Info, "Bochs:BochsMapVideoMemory Exit VideoRamBase: %p VideoRamLength: 0x%x PhysBasePtr: 0x%x\n",
                     MapInformation->VideoRamBase, MapInformation->VideoRamLength, (ULONG)VideoMemory.QuadPart));
    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN NTAPI
BochsUnmapVideoMemory(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MEMORY VideoMemory,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    VP_STATUS Status;

    VideoDebugPrint((Info, "Bochs: BochsUnmapVideoMemory Entry VideoRamBase:%p\n", VideoMemory->RequestedVirtualAddress));

    Status = VideoPortUnmapMemory(DeviceExtension, VideoMemory->RequestedVirtualAddress, NULL);
    if (Status != NO_ERROR)
    {
        VideoDebugPrint((Error, "Bochs: BochsUnmapVideoMemory Failed to unmap memory:%p Status:%x\n",
                         VideoMemory->RequestedVirtualAddress,
                         Status));
    }

    StatusBlock->Status = Status;

    VideoDebugPrint((Info, "Bochs: BochsUnmapVideoMemory Exit status:%x\n", Status));
    return (Status == NO_ERROR);
}

CODE_SEG("PAGE")
static BOOLEAN
BochsQueryNumAvailableModes(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_NUM_MODES AvailableModes,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    AvailableModes->NumModes = DeviceExtension->AvailableModeCount;
    AvailableModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    StatusBlock->Information = sizeof(*AvailableModes);
    StatusBlock->Status = NO_ERROR;
    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsQueryAvailableModes(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_MODE_INFORMATION ReturnedModes,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    ULONG Count;
    PBOCHS_SIZE AvailableModeInfo;
    PVIDEO_MODE_INFORMATION ModeInfo;

    for (Count = 0, AvailableModeInfo = DeviceExtension->AvailableModeInfo, ModeInfo = ReturnedModes;
        Count < DeviceExtension->AvailableModeCount;
        Count++, AvailableModeInfo++, ModeInfo++)
    {
        VideoPortZeroMemory(ModeInfo, sizeof(*ModeInfo));
        BochsGetModeInfo(AvailableModeInfo, ModeInfo, Count);
    }

    StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION) * DeviceExtension->AvailableModeCount;
    StatusBlock->Status = NO_ERROR;

    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsSetCurrentMode(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MODE RequestedMode,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    PBOCHS_SIZE AvailableModeInfo;
    /* Mask the two high-order bits, which can be set to request special behavior */
    ULONG ModeRequested = RequestedMode->RequestedMode & 0x3fffffff;
    BOOLEAN Ret;

    VideoDebugPrint((Info, "Bochs:BochsSetCurrentMode Entry\n"));

    if (ModeRequested >= DeviceExtension->AvailableModeCount)
    {
        VideoDebugPrint((Error, "Bochs: set current mode - invalid parameter\n"));
        StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    AvailableModeInfo = &DeviceExtension->AvailableModeInfo[ModeRequested];

    /* Set the mode characteristics */
    BochsWriteDispI(DeviceExtension, VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    Ret = BochsWriteDispIAndCheck(DeviceExtension, VBE_DISPI_INDEX_XRES, AvailableModeInfo->XResolution) &&
          BochsWriteDispIAndCheck(DeviceExtension, VBE_DISPI_INDEX_YRES, AvailableModeInfo->YResolution) &&
          BochsWriteDispIAndCheck(DeviceExtension, VBE_DISPI_INDEX_BPP, 32);
    /* Always enable screen, even if display settings change failed */
    BochsWriteDispI(DeviceExtension, VBE_DISPI_INDEX_ENABLE, VBE_DISPI_LFB_ENABLED | VBE_DISPI_ENABLED);
    if (!Ret)
    {
        VideoDebugPrint((Error, "Bochs: failed to change mode\n"));
        return FALSE;
    }

    /* Enable VGA (QEMU secondary-vga disables it by default) */
    if (!DeviceExtension->IoPorts.RangeInIoSpace)
    {
        /* Discard AR flip-flip */
        (VOID)VideoPortReadRegisterUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped + 0x41A));
        /* Enable display */
        VideoPortWriteRegisterUshort((PUSHORT)(DeviceExtension->IoPorts.Mapped + 0x400), 0x20);
    }

    DeviceExtension->CurrentMode = (USHORT)ModeRequested;
    StatusBlock->Status = NO_ERROR;

    VideoDebugPrint((Info, "Bochs:BochsSetCurrentMode Exit Mode:%d\n", ModeRequested));
    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsQueryCurrentMode(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_MODE_INFORMATION VideoModeInfo,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    PBOCHS_SIZE AvailableModeInfo;

    if (DeviceExtension->CurrentMode > DeviceExtension->AvailableModeCount)
    {
        StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    AvailableModeInfo = &DeviceExtension->AvailableModeInfo[DeviceExtension->CurrentMode];
    VideoPortZeroMemory(VideoModeInfo, sizeof(*VideoModeInfo));
    BochsGetModeInfo(AvailableModeInfo, VideoModeInfo, DeviceExtension->CurrentMode);

    StatusBlock->Information = sizeof(*VideoModeInfo);
    StatusBlock->Status = NO_ERROR;
    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsResetDevice(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    VideoDebugPrint((Info, "Bochs:BochsResetDevice Entry\n"));

    StatusBlock->Status = NO_ERROR;

    VideoDebugPrint((Info, "Bochs:BochsResetDevice Exit\n"));
    return TRUE;
}

CODE_SEG("PAGE")
static BOOLEAN
BochsGetChildState(
    _In_ PBOCHS_DEVICE_EXTENSION DeviceExtension,
    _Out_ PULONG pChildState,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    *pChildState = VIDEO_CHILD_ACTIVE;

    StatusBlock->Information = sizeof(*pChildState);
    StatusBlock->Status = NO_ERROR;
    return TRUE;
}

CODE_SEG("PAGE")
VP_STATUS NTAPI
BochsFindAdapter(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID HwContext,
    _In_ PWSTR ArgumentString,
    _In_ PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    _In_ PUCHAR Again)
{
    PBOCHS_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    VIDEO_ACCESS_RANGE AccessRanges[2] = {0};

    if (ConfigInfo->Length < sizeof(*ConfigInfo))
        return ERROR_INVALID_PARAMETER;

    if (VideoPortGetAccessRanges(DeviceExtension, 0, NULL, ARRAYSIZE(AccessRanges), AccessRanges, NULL, NULL, NULL) != NO_ERROR)
    {
        VideoDebugPrint((Error, "Bochs: failed to get access ranges\n"));
        return ERROR_DEV_NOT_EXIST;
    }

    /* Framebuffer */
    DeviceExtension->FrameBuffer.RangeStart = AccessRanges[0].RangeStart;
    DeviceExtension->FrameBuffer.RangeLength = AccessRanges[0].RangeLength;
    DeviceExtension->FrameBuffer.RangeInIoSpace = AccessRanges[0].RangeInIoSpace;

    /* I/O ports */
    if (AccessRanges[1].RangeLength == 0)
    {
        /* Set default values */
        AccessRanges[1].RangeStart.LowPart = VBE_DISPI_IOPORT_INDEX;
        AccessRanges[1].RangeLength = 2;
        AccessRanges[1].RangeInIoSpace = TRUE;
        if (VideoPortVerifyAccessRanges(DeviceExtension, 1, &AccessRanges[1]) != NO_ERROR)
        {
            VideoDebugPrint((Error, "Bochs: failed to claim I/O range 0x%x-0x%x\n",
                            VBE_DISPI_IOPORT_INDEX,
                            VBE_DISPI_IOPORT_INDEX + 1));
            return ERROR_DEV_NOT_EXIST;
        }
    }
    else if (AccessRanges[1].RangeLength != 0x1000)
    {
        VideoDebugPrint((Error, "Bochs: invalid access ranges (size 0x%x)\n", AccessRanges[1].RangeLength));
        return ERROR_DEV_NOT_EXIST;
    }
    DeviceExtension->IoPorts.RangeStart = AccessRanges[1].RangeStart;
    DeviceExtension->IoPorts.RangeLength = AccessRanges[1].RangeLength;
    DeviceExtension->IoPorts.RangeInIoSpace = AccessRanges[1].RangeInIoSpace;

    DeviceExtension->IoPorts.Mapped = VideoPortGetDeviceBase(DeviceExtension,
                                                             DeviceExtension->IoPorts.RangeStart,
                                                             DeviceExtension->IoPorts.RangeLength,
                                                             DeviceExtension->IoPorts.RangeInIoSpace
                                                                 ? VIDEO_MEMORY_SPACE_IO
                                                                 : VIDEO_MEMORY_SPACE_MEMORY);
    if (!DeviceExtension->IoPorts.Mapped)
    {
        VideoDebugPrint((Error, "Bochs: failed to map dispi interface\n"));
        return ERROR_DEV_NOT_EXIST;
    }
    VideoDebugPrint((Info, "Bochs: address 0x%x mapped to 0x%p\n",
                     DeviceExtension->IoPorts.RangeStart.LowPart,
                     DeviceExtension->IoPorts.Mapped));

    return NO_ERROR;
}

CODE_SEG("PAGE")
BOOLEAN NTAPI
BochsInitialize(
    _In_ PVOID HwDeviceExtension)
{
    ULONG PotentialModeCount = 0;
    PBOCHS_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    VideoDebugPrint((Info, "Bochs: BochsInitialize\n"));

    if (!BochsGetControllerInfo(DeviceExtension))
    {
        BochsFreeResources(DeviceExtension);
        return FALSE;
    }

    PotentialModeCount = ARRAYSIZE(BochsAvailableResolutions);
    DeviceExtension->AvailableModeInfo = VideoPortAllocatePool(HwDeviceExtension,
                                                               VpPagedPool,
                                                               PotentialModeCount * sizeof(BOCHS_SIZE),
                                                               BOCHS_TAG);
    if (!DeviceExtension->AvailableModeInfo)
    {
        VideoDebugPrint((Error, "Bochs: insufficient resources\n"));
        BochsFreeResources(DeviceExtension);
        return FALSE;
    }

    if (!BochsInitializeSuitableModeInfo(DeviceExtension, PotentialModeCount))
    {
        BochsFreeResources(DeviceExtension);
        return FALSE;
    }

    return TRUE;
}

CODE_SEG("PAGE")
BOOLEAN NTAPI
BochsStartIO(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVIDEO_REQUEST_PACKET RequestPacket)
{
    PBOCHS_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    VideoDebugPrint((Info, "Bochs: BochsStartIO\n"));
    RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;

    switch (RequestPacket->IoControlCode)
    {
        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
        {
            VideoDebugPrint((Info, "BochsStartIO - Map video memory\n"));
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                VideoDebugPrint((Error, "BochsStartIO - invalid input parameter\n"));
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION))
            {
                VideoDebugPrint((Error, "BochsStartIO - Insufficent output buffer\n"));
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsMapVideoMemory(DeviceExtension,
                                       (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                                       (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
                                       RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
        {
            VideoDebugPrint((Info, "BochsStartIO - Unmap video memory\n"));
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsUnmapVideoMemory(DeviceExtension,
                                         (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                                         RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        {
            VideoDebugPrint((Info, "BochsStartIO - Query num available modes\n"));
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsQueryNumAvailableModes(DeviceExtension,
                                               (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer,
                                               RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
        {
            VideoDebugPrint((Info, "BochsStartIO - Query available modes\n"));
            if (RequestPacket->OutputBufferLength < DeviceExtension->AvailableModeCount * sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsQueryAvailableModes(DeviceExtension,
                                            (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                                            RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_SET_CURRENT_MODE:
        {
            VideoDebugPrint((Info, "BochsStartIO - Set current mode\n"));
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                VideoDebugPrint((Error, "Bochs: set current mode - invalid parameter\n"));
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsSetCurrentMode(DeviceExtension,
                                       (PVIDEO_MODE)RequestPacket->InputBuffer,
                                       RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
        {
            VideoDebugPrint((Info, "BochsStartIO - Query current mode\n"));
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsQueryCurrentMode(DeviceExtension,
                                         (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                                         RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_RESET_DEVICE:
        {
            VideoDebugPrint((Info, "BochsStartIO - Reset device\n"));
            return BochsResetDevice(DeviceExtension,
                                    RequestPacket->StatusBlock);
        }

        case IOCTL_VIDEO_GET_CHILD_STATE:
        {
            VideoDebugPrint((Info, "BochsStartIO - Get child state\n"));
            if (RequestPacket->OutputBufferLength < sizeof(ULONG))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return FALSE;
            }
            return BochsGetChildState(DeviceExtension,
                                      (PULONG)RequestPacket->OutputBuffer,
                                      RequestPacket->StatusBlock);
        }

        default:
        {
            VideoDebugPrint((Warn, "BochsStartIO - Unknown IOCTL - 0x%08x\n",
                             RequestPacket->IoControlCode));
            break;
        }
    }

    return FALSE;
}

CODE_SEG("PAGE")
VP_STATUS NTAPI
BochsSetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _In_ PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    return NO_ERROR;
}

CODE_SEG("PAGE")
VP_STATUS NTAPI
BochsGetPowerState(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG HwId,
    _Out_ PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    return ERROR_DEVICE_REINITIALIZATION_NEEDED;
}

CODE_SEG("PAGE")
VP_STATUS NTAPI
BochsGetVideoChildDescriptor(
    _In_ PVOID HwDeviceExtension,
    _In_ PVIDEO_CHILD_ENUM_INFO ChildEnumInfo,
    _Out_ PVIDEO_CHILD_TYPE VideoChildType,
    _Out_ PUCHAR pChildDescriptor,
    _Out_ PULONG UId,
    _Out_ PULONG pUnused)
{
    PBOCHS_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    VideoDebugPrint((Info, "Bochs: BochsGetVideoChildDescriptor Entry\n"));

    if (ChildEnumInfo->Size < sizeof(*VideoChildType))
        return VIDEO_ENUM_NO_MORE_DEVICES;

    if (ChildEnumInfo->ChildIndex == 0)
    {
        /* Ignore ACPI enumerations */
        return VIDEO_ENUM_INVALID_DEVICE;
    }

    *pUnused = 0;
    if (ChildEnumInfo->ChildIndex == DISPLAY_ADAPTER_HW_ID)
    {
        *VideoChildType = VideoChip;
        return VIDEO_ENUM_MORE_DEVICES;
    }

    if (ChildEnumInfo->ChildIndex != 1)
        return VIDEO_ENUM_NO_MORE_DEVICES;

    *UId = 0;
    *VideoChildType = Monitor;

    if (pChildDescriptor &&
        ChildEnumInfo->ChildDescriptorSize >= VBE_EDID_SIZE &&
        !DeviceExtension->IoPorts.RangeInIoSpace)
    {
        memcpy(pChildDescriptor,
               DeviceExtension->IoPorts.Mapped,
               VBE_EDID_SIZE);
    }

    VideoDebugPrint((Info, "Bochs: BochsGetVideoChildDescriptor Exit Uid:%d\n", ChildEnumInfo->ChildIndex));

    return VIDEO_ENUM_MORE_DEVICES;
}

ULONG NTAPI
DriverEntry(PVOID Context1, PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA VideoInitData;

    VideoDebugPrint((Info, "Bochs: DriverEntry\n"));
    VideoPortZeroMemory(&VideoInitData, sizeof(VideoInitData));
    VideoInitData.HwInitDataSize = sizeof(VideoInitData);
    VideoInitData.HwFindAdapter = BochsFindAdapter;
    VideoInitData.HwInitialize = BochsInitialize;
    VideoInitData.HwStartIO = BochsStartIO;
    VideoInitData.HwDeviceExtensionSize = sizeof(BOCHS_DEVICE_EXTENSION);
    VideoInitData.HwSetPowerState = BochsSetPowerState;
    VideoInitData.HwGetPowerState = BochsGetPowerState;
    VideoInitData.HwGetVideoChildDescriptor = BochsGetVideoChildDescriptor;

    return VideoPortInitialize(Context1, Context2, &VideoInitData, NULL);
}
