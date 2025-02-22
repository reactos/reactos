/*
 * PROJECT:     ReactOS framebuffer driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     I/O control handling
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "pc98vid.h"

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
FASTCALL
Pc98VidQueryMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG ModeNumber,
    _Out_ PVIDEO_MODE_INFORMATION VideoMode)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Mode %d\n", __FUNCTION__, ModeNumber));

    VideoMode->Length = sizeof(VIDEO_MODE_INFORMATION);
    VideoMode->ModeIndex = ModeNumber;
    VideoMode->VisScreenWidth = VideoModes[ModeNumber].HResolution;
    VideoMode->VisScreenHeight = VideoModes[ModeNumber].VResolution;
    VideoMode->ScreenStride = VideoModes[ModeNumber].HResolution;
    VideoMode->NumberOfPlanes = 1;
    VideoMode->BitsPerPlane = 8;
    VideoMode->Frequency = VideoModes[ModeNumber].RefreshRate;
    VideoMode->XMillimeter = 320;
    VideoMode->YMillimeter = 240;
    VideoMode->NumberRedBits =
    VideoMode->NumberGreenBits =
    VideoMode->NumberBlueBits = 8;
    VideoMode->RedMask =
    VideoMode->GreenMask =
    VideoMode->BlueMask = 0;
    VideoMode->AttributeFlags = VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS |
                                VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidQueryAvailModes(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_MODE_INFORMATION ModeInformation,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    UCHAR ModeNumber;
    PVIDEO_MODE_INFORMATION VideoMode;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    for (ModeNumber = 0, VideoMode = ModeInformation;
         ModeNumber < DeviceExtension->ModeCount;
         ++ModeNumber, ++VideoMode)
    {
        Pc98VidQueryMode(DeviceExtension, ModeNumber, VideoMode);
    }

    StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION) * DeviceExtension->ModeCount;

    return NO_ERROR;
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidQueryNumAvailModes(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_NUM_MODES Modes,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    Modes->NumModes = DeviceExtension->ModeCount;
    Modes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    StatusBlock->Information = sizeof(VIDEO_NUM_MODES);

    return NO_ERROR;
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidQueryCurrentMode(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _Out_ PVIDEO_MODE_INFORMATION VideoMode,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Mode %d\n",
                     __FUNCTION__, DeviceExtension->CurrentMode));

    Pc98VidQueryMode(DeviceExtension, DeviceExtension->CurrentMode, VideoMode);

    StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION);

    return NO_ERROR;
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidMapVideoMemory(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MEMORY RequestedAddress,
    _Out_ PVIDEO_MEMORY_INFORMATION MapInformation,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    VP_STATUS Status;
    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
    MapInformation->VideoRamLength = DeviceExtension->FrameBufferLength;

    Status = VideoPortMapMemory(DeviceExtension,
                                DeviceExtension->FrameBuffer,
                                &MapInformation->VideoRamLength,
                                &inIoSpace,
                                &MapInformation->VideoRamBase);
    if (Status != NO_ERROR)
    {
        VideoDebugPrint((Error, "%s() Failed to map framebuffer memory\n",
                         __FUNCTION__));
    }
    else
    {
        MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
        MapInformation->FrameBufferLength = MapInformation->VideoRamLength;

        StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);
    }

    return Status;
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidUnmapVideoMemory(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PVIDEO_MEMORY VideoMemory)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    return VideoPortUnmapMemory(DeviceExtension,
                                VideoMemory->RequestedVirtualAddress,
                                NULL);
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidResetDevice(VOID)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s()\n", __FUNCTION__));

    return NO_ERROR;
}

static
CODE_SEG("PAGE")
VP_STATUS
FASTCALL
Pc98VidGetChildState(
    _In_ PHW_DEVICE_EXTENSION DeviceExtension,
    _In_ PULONG ChildIndex,
    _Out_ PULONG ChildState,
    _Out_ PSTATUS_BLOCK StatusBlock)
{
    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() Child %d\n", __FUNCTION__, *ChildIndex));

    *ChildState = VIDEO_CHILD_ACTIVE;

    StatusBlock->Information = sizeof(ULONG);

    return NO_ERROR;
}

CODE_SEG("PAGE")
BOOLEAN
NTAPI
Pc98VidStartIO(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVIDEO_REQUEST_PACKET RequestPacket)
{
    VP_STATUS Status;

    PAGED_CODE();

    VideoDebugPrint((Trace, "%s() IOCTL 0x%lX\n",
                    __FUNCTION__, RequestPacket->IoControlCode));

    switch (RequestPacket->IoControlCode)
    {
        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        {
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidQueryNumAvailModes((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                               (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer,
                                               RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
        {
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION) *
                ((PHW_DEVICE_EXTENSION)HwDeviceExtension)->ModeCount)
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidQueryAvailModes((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                            (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                                            RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_SET_CURRENT_MODE:
        {
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidSetCurrentMode((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                           (PVIDEO_MODE)RequestPacket->InputBuffer);
            break;
        }

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
        {
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidQueryCurrentMode((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                             (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                                             RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
        {
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY) ||
                RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidMapVideoMemory((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                           (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                                           (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
                                           RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
        {
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidUnmapVideoMemory((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                             (PVIDEO_MEMORY)RequestPacket->InputBuffer);
            break;
        }

        case IOCTL_VIDEO_RESET_DEVICE:
        {
            Status = Pc98VidResetDevice();
            break;
        }

        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
        {
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidSetColorRegisters((PVIDEO_CLUT)RequestPacket->InputBuffer);
            break;
        }

        case IOCTL_VIDEO_GET_CHILD_STATE:
        {
            if (RequestPacket->InputBufferLength < sizeof(ULONG) ||
                RequestPacket->OutputBufferLength < sizeof(ULONG))
            {
                Status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            Status = Pc98VidGetChildState((PHW_DEVICE_EXTENSION)HwDeviceExtension,
                                          (PULONG)RequestPacket->InputBuffer,
                                          (PULONG)RequestPacket->OutputBuffer,
                                          RequestPacket->StatusBlock);
            break;
        }

        default:
            Status = ERROR_INVALID_FUNCTION;
    }

    if (Status != NO_ERROR)
        VideoDebugPrint((Trace, "%s() Failed 0x%lX\n", __FUNCTION__, Status));

    RequestPacket->StatusBlock->Status = Status;

    return TRUE;
}
