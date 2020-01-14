/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Simple framebuffer driver for NVIDIA NV2A XGPU
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp
 *              Copyright 2004 Filip Navara
 *              Copyright 2019 Stanislav Motylkov (x86corez@gmail.com)
 *
 * TODO:
 * - Check input parameters everywhere.
 * - Call VideoPortVerifyAccessRanges to reserve the memory we're about
 *   to map.
 */

/* INCLUDES *******************************************************************/

#include "xboxvmp.h"

#include <debug.h>
#include <dpfilter.h>

/* PUBLIC AND PRIVATE FUNCTIONS ***********************************************/

ULONG
NTAPI
DriverEntry(
    IN PVOID Context1,
    IN PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA InitData;

    VideoPortZeroMemory(&InitData, sizeof(InitData));
    InitData.AdapterInterfaceType = PCIBus;
    InitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    InitData.HwFindAdapter = XboxVmpFindAdapter;
    InitData.HwInitialize = XboxVmpInitialize;
    InitData.HwStartIO = XboxVmpStartIO;
    InitData.HwResetHw = XboxVmpResetHw;
    InitData.HwGetPowerState = XboxVmpGetPowerState;
    InitData.HwSetPowerState = XboxVmpSetPowerState;
    InitData.HwDeviceExtensionSize = sizeof(XBOXVMP_DEVICE_EXTENSION);

    return VideoPortInitialize(Context1, Context2, &InitData, NULL);
}

/*
 * XboxVmpFindAdapter
 *
 * Detects the Xbox Nvidia display adapter.
 */

VP_STATUS
NTAPI
XboxVmpFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PVOID HwContext,
    IN PWSTR ArgumentString,
    IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    OUT PUCHAR Again)
{
    PXBOXVMP_DEVICE_EXTENSION XboxVmpDeviceExtension;
    VIDEO_ACCESS_RANGE AccessRanges[3];
    VP_STATUS Status;
    USHORT VendorId = 0x10DE; /* NVIDIA Corporation */
    USHORT DeviceId = 0x02A0; /* NV2A XGPU */

    TRACE_(IHVVIDEO, "XboxVmpFindAdapter\n");

    XboxVmpDeviceExtension = (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension;

    Status = VideoPortGetAccessRanges(HwDeviceExtension, 0, NULL, 3, AccessRanges,
                                      &VendorId, &DeviceId, NULL);

    if (Status == NO_ERROR)
    {
        XboxVmpDeviceExtension->PhysControlStart = AccessRanges[0].RangeStart;
        XboxVmpDeviceExtension->ControlLength = AccessRanges[0].RangeLength;
        XboxVmpDeviceExtension->PhysFrameBufferStart = AccessRanges[1].RangeStart;
    }

    return Status;
}

/*
 * XboxVmpInitialize
 *
 * Performs the first initialization of the adapter, after the HAL has given
 * up control of the video hardware to the video port driver.
 */

BOOLEAN
NTAPI
XboxVmpInitialize(
    PVOID HwDeviceExtension)
{
    PXBOXVMP_DEVICE_EXTENSION XboxVmpDeviceExtension;
    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;
    ULONG Length;

    TRACE_(IHVVIDEO, "XboxVmpInitialize\n");

    XboxVmpDeviceExtension = (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension;

    Length = XboxVmpDeviceExtension->ControlLength;
    XboxVmpDeviceExtension->VirtControlStart = NULL;

    if (VideoPortMapMemory(HwDeviceExtension,
                           XboxVmpDeviceExtension->PhysControlStart,
                           &Length,
                           &inIoSpace,
                           &XboxVmpDeviceExtension->VirtControlStart) != NO_ERROR)
    {
        ERR_(IHVVIDEO, "Failed to map control memory\n");
        return FALSE;
    }

    INFO_(IHVVIDEO, "Mapped 0x%x bytes of control mem at 0x%x to virt addr 0x%x\n",
        XboxVmpDeviceExtension->ControlLength,
        XboxVmpDeviceExtension->PhysControlStart.u.LowPart,
        XboxVmpDeviceExtension->VirtControlStart);

    return TRUE;
}

/*
 * XboxVmpStartIO
 *
 * Processes the specified Video Request Packet.
 */

BOOLEAN
NTAPI
XboxVmpStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket)
{
    BOOLEAN Result;

    RequestPacket->StatusBlock->Status = ERROR_INVALID_PARAMETER;

    switch (RequestPacket->IoControlCode)
    {
        case IOCTL_VIDEO_SET_CURRENT_MODE:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_SET_CURRENT_MODE\n");

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = XboxVmpSetCurrentMode(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                (PVIDEO_MODE)RequestPacket->InputBuffer,
                RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_RESET_DEVICE:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_RESET_DEVICE\n");

            Result = XboxVmpResetDevice(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_MAP_VIDEO_MEMORY\n");

            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION) ||
                RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = XboxVmpMapVideoMemory(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_UNMAP_VIDEO_MEMORY\n");

            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = XboxVmpUnmapVideoMemory(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES\n");

            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = XboxVmpQueryNumAvailModes(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_QUERY_AVAIL_MODES\n");

            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = XboxVmpQueryAvailModes(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;
        }

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
        {
            TRACE_(IHVVIDEO, "XboxVmpStartIO IOCTL_VIDEO_QUERY_CURRENT_MODE\n");

            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }

            Result = XboxVmpQueryCurrentMode(
                (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension,
                (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;
        }

        default:
        {
            WARN_(IHVVIDEO, "XboxVmpStartIO 0x%x not implemented\n", RequestPacket->IoControlCode);

            RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
            return FALSE;
        }
    }

    if (Result)
    {
        RequestPacket->StatusBlock->Status = NO_ERROR;
    }

    return TRUE;
}

/*
 * XboxVmpResetHw
 *
 * This function is called to reset the hardware to a known state.
 */

BOOLEAN
NTAPI
XboxVmpResetHw(
    PVOID DeviceExtension,
    ULONG Columns,
    ULONG Rows)
{
    TRACE_(IHVVIDEO, "XboxVmpResetHw\n");

    if (!XboxVmpResetDevice((PXBOXVMP_DEVICE_EXTENSION)DeviceExtension, NULL))
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * XboxVmpGetPowerState
 *
 * Queries whether the device can support the requested power state.
 */

VP_STATUS
NTAPI
XboxVmpGetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    ERR_(IHVVIDEO, "XboxVmpGetPowerState is not supported\n");

    return ERROR_INVALID_FUNCTION;
}

/*
 * XboxVmpSetPowerState
 *
 * Sets the power state of the specified device
 */

VP_STATUS
NTAPI
XboxVmpSetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    ERR_(IHVVIDEO, "XboxVmpSetPowerState not supported\n");

    return ERROR_INVALID_FUNCTION;
}

/*
 * VBESetCurrentMode
 *
 * Sets the adapter to the specified operating mode.
 */

BOOLEAN
FASTCALL
XboxVmpSetCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE RequestedMode,
    PSTATUS_BLOCK StatusBlock)
{
    if (RequestedMode->RequestedMode != 0)
    {
        return FALSE;
    }

    /* Nothing to do, really. We only support a single mode and we're already
     * in that mode
     */
    return TRUE;
}

/*
 * XboxVmpResetDevice
 *
 * Resets the video hardware to the default mode, to which it was initialized
 * at system boot.
 */

BOOLEAN
FASTCALL
XboxVmpResetDevice(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PSTATUS_BLOCK StatusBlock)
{
    /* There is nothing to be done here */

    return TRUE;
}

/*
 * XboxVmpMapVideoMemory
 *
 * Maps the video hardware frame buffer and video RAM into the virtual address
 * space of the requestor.
 */

BOOLEAN
FASTCALL
XboxVmpMapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY RequestedAddress,
    PVIDEO_MEMORY_INFORMATION MapInformation,
    PSTATUS_BLOCK StatusBlock)
{
    PHYSICAL_ADDRESS FrameBuffer;
    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;

    StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);

    /* Reuse framebuffer that was set up by firmware */
    FrameBuffer.QuadPart = *((PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CONTROL_FRAMEBUFFER_ADDRESS_OFFSET));
    /* Framebuffer address offset value is coming from the GPU within
     * memory mapped I/O address space, so we're comparing only low
     * 28 bits of the address within actual RAM address space */
    FrameBuffer.QuadPart &= 0x0FFFFFFF;
    if (FrameBuffer.QuadPart != 0x3C00000 && FrameBuffer.QuadPart != 0x7C00000)
    {
        /* Check framebuffer address (high 4 MB of either 64 or 128 MB RAM) */
        WARN_(IHVVIDEO, "Non-standard framebuffer address 0x%p\n", FrameBuffer.QuadPart);
    }
    /* Verify that framebuffer address is page-aligned */
    ASSERT(FrameBuffer.QuadPart % PAGE_SIZE == 0);

    /* Return the address back to GPU memory mapped I/O */
    FrameBuffer.QuadPart += DeviceExtension->PhysFrameBufferStart.QuadPart;
    MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
    /* FIXME: obtain fb size from firmware somehow (Cromwell reserves high 4 MB of RAM) */
    MapInformation->VideoRamLength = NV2A_VIDEO_MEMORY_SIZE;

    VideoPortMapMemory(
        DeviceExtension,
        FrameBuffer,
        &MapInformation->VideoRamLength,
        &inIoSpace,
        &MapInformation->VideoRamBase);

    MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
    MapInformation->FrameBufferLength = MapInformation->VideoRamLength;

    /* Tell the nVidia controller about the framebuffer */
    *((PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CONTROL_FRAMEBUFFER_ADDRESS_OFFSET)) = FrameBuffer.u.LowPart;

    INFO_(IHVVIDEO, "Mapped 0x%x bytes of phys mem at 0x%lx to virt addr 0x%p\n",
        MapInformation->VideoRamLength, FrameBuffer.u.LowPart, MapInformation->VideoRamBase);

    return TRUE;
}

/*
 * VBEUnmapVideoMemory
 *
 * Releases a mapping between the virtual address space and the adapter's
 * frame buffer and video RAM.
 */

BOOLEAN
FASTCALL
XboxVmpUnmapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY VideoMemory,
    PSTATUS_BLOCK StatusBlock)
{
    VideoPortUnmapMemory(
        DeviceExtension,
        VideoMemory->RequestedVirtualAddress,
        NULL);

    return TRUE;
}

/*
 * XboxVmpQueryNumAvailModes
 *
 * Returns the number of video modes supported by the adapter and the size
 * in bytes of the video mode information, which can be used to allocate a
 * buffer for an IOCTL_VIDEO_QUERY_AVAIL_MODES request.
 */

BOOLEAN
FASTCALL
XboxVmpQueryNumAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_NUM_MODES Modes,
    PSTATUS_BLOCK StatusBlock)
{
    Modes->NumModes = 1;
    Modes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
    StatusBlock->Information = sizeof(VIDEO_NUM_MODES);
    return TRUE;
}

/*
 * XboxVmpQueryAvailModes
 *
 * Returns information about each video mode supported by the adapter.
 */

BOOLEAN
FASTCALL
XboxVmpQueryAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION VideoMode,
    PSTATUS_BLOCK StatusBlock)
{
    return XboxVmpQueryCurrentMode(DeviceExtension, VideoMode, StatusBlock);
}

UCHAR
NvGetCrtc(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    UCHAR Index)
{
    *((PUCHAR)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CRTC_REGISTER_INDEX)) = Index;
    return *((PUCHAR)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CRTC_REGISTER_VALUE));
}

UCHAR
NvGetBytesPerPixel(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    ULONG ScreenWidth)
{
    UCHAR BytesPerPixel;

    /* Get BPP directly from NV2A CRTC (magic constants are from Cromwell) */
    BytesPerPixel = 8 * (((NvGetCrtc(DeviceExtension, 0x19) & 0xE0) << 3) | (NvGetCrtc(DeviceExtension, 0x13) & 0xFF)) / ScreenWidth;

    if (BytesPerPixel == 4)
    {
        ASSERT((NvGetCrtc(DeviceExtension, 0x28) & 0xF) == BytesPerPixel - 1);
    }
    else
    {
        ASSERT((NvGetCrtc(DeviceExtension, 0x28) & 0xF) == BytesPerPixel);
    }

    return BytesPerPixel;
}

/*
 * VBEQueryCurrentMode
 *
 * Returns information about current video mode.
 */

BOOLEAN
FASTCALL
XboxVmpQueryCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION VideoMode,
    PSTATUS_BLOCK StatusBlock)
{
    UCHAR BytesPerPixel;

    VideoMode->Length = sizeof(VIDEO_MODE_INFORMATION);
    VideoMode->ModeIndex = 0;

    VideoMode->VisScreenWidth = *((PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_RAMDAC_FP_HVALID_END)) + 1;
    VideoMode->VisScreenHeight = *((PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_RAMDAC_FP_VVALID_END)) + 1;

    if (VideoMode->VisScreenWidth <= 1 || VideoMode->VisScreenHeight <= 1)
    {
        ERR_(IHVVIDEO, "Cannot obtain current screen resolution!\n");
        return FALSE;
    }

    BytesPerPixel = NvGetBytesPerPixel(DeviceExtension, VideoMode->VisScreenWidth);
    ASSERT(BytesPerPixel >= 1 && BytesPerPixel <= 4);

    VideoMode->ScreenStride = VideoMode->VisScreenWidth * BytesPerPixel;
    VideoMode->NumberOfPlanes = 1;
    VideoMode->BitsPerPlane = BytesPerPixel * 8;
    VideoMode->Frequency = 1;
    VideoMode->XMillimeter = 0; /* FIXME */
    VideoMode->YMillimeter = 0; /* FIXME */
    if (BytesPerPixel >= 3)
    {
        VideoMode->NumberRedBits = 8;
        VideoMode->NumberGreenBits = 8;
        VideoMode->NumberBlueBits = 8;
        VideoMode->RedMask = 0xFF0000;
        VideoMode->GreenMask = 0x00FF00;
        VideoMode->BlueMask = 0x0000FF;
    }
    else
    {
        /* FIXME: not implemented */
        WARN_(IHVVIDEO, "BytesPerPixel %d - not implemented\n", BytesPerPixel);
    }
    VideoMode->VideoMemoryBitmapWidth = VideoMode->VisScreenWidth;
    VideoMode->VideoMemoryBitmapHeight = VideoMode->VisScreenHeight;
    VideoMode->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
        VIDEO_MODE_NO_OFF_SCREEN;
    VideoMode->DriverSpecificAttributeFlags = 0;

    StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION);

    /* Verify that screen fits framebuffer size */
    if (VideoMode->VisScreenWidth * VideoMode->VisScreenHeight * (VideoMode->BitsPerPlane / 8) > NV2A_VIDEO_MEMORY_SIZE)
    {
        ERR_(IHVVIDEO, "Current screen resolution exceeds video memory bounds!\n");
        return FALSE;
    }

    return TRUE;
}

/* EOF */
