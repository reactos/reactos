/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Framebuffer + 2D-accel driver for NVIDIA NV2A XGPU
 * COPYRIGHT:   Copyright 2004 Gé van Geldorp
 *              Copyright 2004 Filip Navara
 *              Copyright 2019-2020 Stanislav Motylkov (x86corez@gmail.com)
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "xboxvmp.h"
#include "nv2a_accel.h"

#include <debug.h>
#include <dpfilter.h>

/* Local prototypes */
static UCHAR NvGetCrtc(PXBOXVMP_DEVICE_EXTENSION Dx, UCHAR Index);
static UCHAR NvGetBytesPerPixel(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG ScreenWidth);
static VOID  Nv2aProgramPalette(PXBOXVMP_DEVICE_EXTENSION Dx,
                                ULONG FirstEntry, ULONG NumEntries);
static BOOLEAN XboxVmpHandleAccelFill(PXBOXVMP_DEVICE_EXTENSION Dx,
                                      PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleAccelBlt(PXBOXVMP_DEVICE_EXTENSION Dx,
                                     PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleAccelCaps(PXBOXVMP_DEVICE_EXTENSION Dx,
                                      PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleAccelBltEx(PXBOXVMP_DEVICE_EXTENSION Dx,
                                       PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleDraw3d(PXBOXVMP_DEVICE_EXTENSION Dx,
                                   PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleTexUpload(PXBOXVMP_DEVICE_EXTENSION Dx,
                                      PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleReadback(PXBOXVMP_DEVICE_EXTENSION Dx,
                                     PVIDEO_REQUEST_PACKET Rp);
static BOOLEAN XboxVmpHandleWriteback(PXBOXVMP_DEVICE_EXTENSION Dx,
                                      PVIDEO_REQUEST_PACKET Rp);
static VOID CpuFillRect(PXBOXVMP_DEVICE_EXTENSION Dx,
                        ULONG X, ULONG Y, ULONG W, ULONG H, ULONG Color);

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
    PXBOXVMP_DEVICE_EXTENSION Dx;
    VP_STATUS Status;
    VIDEO_ACCESS_RANGE AccessRanges[3];
    USHORT VendorId = 0x10DE; /* NVIDIA Corporation */
    USHORT DeviceId = 0x02A0; /* NV2A XGPU */
    ULONG Slot = 0;

    TRACE_(IHVVIDEO, "XboxVmpFindAdapter\n");

    Dx = (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension;

    VideoPortZeroMemory(&AccessRanges, sizeof(AccessRanges));
    Status = VideoPortGetAccessRanges(HwDeviceExtension, 0, NULL,
                                      RTL_NUMBER_OF(AccessRanges), AccessRanges,
                                      &VendorId, &DeviceId, &Slot);
    if (Status != NO_ERROR)
        return Status;

    Dx->PhysControlStart = AccessRanges[0].RangeStart;
    Dx->ControlLength = AccessRanges[0].RangeLength;
    Dx->PhysFrameBufferStart = AccessRanges[1].RangeStart;

    /* Reserve the BARs so no other driver tries to map them.  Ignore the
     * return value: failures here are non-fatal — typically they happen when
     * a previous miniport instance already claimed the range. */
    (VOID)VideoPortVerifyAccessRanges(HwDeviceExtension,
                                      RTL_NUMBER_OF(AccessRanges),
                                      AccessRanges);

    return NO_ERROR;
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
    PXBOXVMP_DEVICE_EXTENSION Dx;
    ULONG inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;
    ULONG Length;

    TRACE_(IHVVIDEO, "XboxVmpInitialize\n");

    Dx = (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension;

    Length = Dx->ControlLength;
    Dx->VirtControlStart = NULL;

    if (VideoPortMapMemory(HwDeviceExtension,
                           Dx->PhysControlStart,
                           &Length,
                           &inIoSpace,
                           &Dx->VirtControlStart) != NO_ERROR)
    {
        ERR_(IHVVIDEO, "Failed to map control memory\n");
        return FALSE;
    }

    INFO_(IHVVIDEO, "Mapped 0x%x bytes of control mem at 0x%x to virt addr 0x%x\n",
        Dx->ControlLength,
        Dx->PhysControlStart.u.LowPart,
        Dx->VirtControlStart);

    return TRUE;
}

/*
 * XboxVmpStartIO — Process the specified Video Request Packet.
 */
BOOLEAN
NTAPI
XboxVmpStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket)
{
    PXBOXVMP_DEVICE_EXTENSION Dx = (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension;
    BOOLEAN Result;

    RequestPacket->StatusBlock->Status = ERROR_INVALID_PARAMETER;

    switch (RequestPacket->IoControlCode)
    {
        case IOCTL_VIDEO_SET_CURRENT_MODE:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_SET_CURRENT_MODE\n");
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MODE))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpSetCurrentMode(Dx,
                (PVIDEO_MODE)RequestPacket->InputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_RESET_DEVICE:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_RESET_DEVICE\n");
            Result = XboxVmpResetDevice(Dx, RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_MAP_VIDEO_MEMORY:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_MAP_VIDEO_MEMORY\n");
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION) ||
                RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpMapVideoMemory(Dx,
                (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                (PVIDEO_MEMORY_INFORMATION)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_UNMAP_VIDEO_MEMORY\n");
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpUnmapVideoMemory(Dx,
                (PVIDEO_MEMORY)RequestPacket->InputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES\n");
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_NUM_MODES))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpQueryNumAvailModes(Dx,
                (PVIDEO_NUM_MODES)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_QUERY_AVAIL_MODES:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_QUERY_AVAIL_MODES\n");
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpQueryAvailModes(Dx,
                (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_QUERY_CURRENT_MODE:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_QUERY_CURRENT_MODE\n");
            if (RequestPacket->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpQueryCurrentMode(Dx,
                (PVIDEO_MODE_INFORMATION)RequestPacket->OutputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_SET_COLOR_REGISTERS:
            TRACE_(IHVVIDEO, "IOCTL_VIDEO_SET_COLOR_REGISTERS\n");
            if (RequestPacket->InputBufferLength < sizeof(VIDEO_CLUT))
            {
                RequestPacket->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
                return TRUE;
            }
            Result = XboxVmpSetColorRegisters(Dx,
                (PVIDEO_CLUT)RequestPacket->InputBuffer,
                RequestPacket->StatusBlock);
            break;

        case IOCTL_VIDEO_NV2A_FILL_RECT:
            Result = XboxVmpHandleAccelFill(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_SCREEN_BLT:
            Result = XboxVmpHandleAccelBlt(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_QUERY_CAPS:
            Result = XboxVmpHandleAccelCaps(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_BLT_EX:
            Result = XboxVmpHandleAccelBltEx(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_DRAW_3D:
            Result = XboxVmpHandleDraw3d(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_TEX_UPLOAD:
            Result = XboxVmpHandleTexUpload(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_READBACK:
            Result = XboxVmpHandleReadback(Dx, RequestPacket);
            break;

        case IOCTL_VIDEO_NV2A_WRITEBACK:
            Result = XboxVmpHandleWriteback(Dx, RequestPacket);
            break;

        default:
            WARN_(IHVVIDEO, "XboxVmpStartIO 0x%x not implemented\n",
                  RequestPacket->IoControlCode);
            RequestPacket->StatusBlock->Status = ERROR_INVALID_FUNCTION;
            return FALSE;
    }

    if (Result)
        RequestPacket->StatusBlock->Status = NO_ERROR;
    return TRUE;
}

/*
 * XboxVmpResetHw — text-mode reset.  The miniport keeps the firmware-set mode
 * alive; the OS draws its bugcheck text by writing directly into the FB.
 */
BOOLEAN
NTAPI
XboxVmpResetHw(
    PVOID DeviceExtension,
    ULONG Columns,
    ULONG Rows)
{
    TRACE_(IHVVIDEO, "XboxVmpResetHw\n");
    return XboxVmpResetDevice((PXBOXVMP_DEVICE_EXTENSION)DeviceExtension, NULL);
}

/* ------------------------------------------------------------------------- */
/* Power state ------------------------------------------------------------- */

/*
 * XboxVmpGetPowerState
 *
 * NV2A only exposes a binary on/off via NV_PMC_ENABLE.  We translate the four
 * ACPI device-power states to that.
 */
VP_STATUS
NTAPI
XboxVmpGetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(HwId);

    if (VideoPowerControl == NULL)
        return ERROR_INVALID_PARAMETER;

    /* We can transition to any state on demand. */
    switch (VideoPowerControl->PowerState)
    {
        case VideoPowerOn:
        case VideoPowerStandBy:
        case VideoPowerSuspend:
        case VideoPowerOff:
        case VideoPowerHibernate:
            return NO_ERROR;
        default:
            return ERROR_INVALID_PARAMETER;
    }
}

VP_STATUS
NTAPI
XboxVmpSetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    PXBOXVMP_DEVICE_EXTENSION Dx = (PXBOXVMP_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG value;

    UNREFERENCED_PARAMETER(HwId);

    if (Dx == NULL || Dx->VirtControlStart == NULL ||
        VideoPowerControl == NULL)
        return ERROR_INVALID_PARAMETER;

    switch (VideoPowerControl->PowerState)
    {
        case VideoPowerOn:
            value = NV2A_PMC_ENABLE_PFIFO  |
                    NV2A_PMC_ENABLE_PGRAPH |
                    NV2A_PMC_ENABLE_PTIMER |
                    NV2A_PMC_ENABLE_PCRTC  |
                    NV2A_PMC_ENABLE_PRAMDAC;
            WRITE_REGISTER_ULONG(
                (PULONG)((ULONG_PTR)Dx->VirtControlStart + NV2A_PMC_ENABLE),
                value);
            break;

        case VideoPowerStandBy:
        case VideoPowerSuspend:
            /* Disable PFIFO + PGRAPH; keep CRTC + RAMDAC so the TV stays in
             * sync but no new GPU work is dispatched. */
            value = NV2A_PMC_ENABLE_PTIMER |
                    NV2A_PMC_ENABLE_PCRTC  |
                    NV2A_PMC_ENABLE_PRAMDAC;
            WRITE_REGISTER_ULONG(
                (PULONG)((ULONG_PTR)Dx->VirtControlStart + NV2A_PMC_ENABLE),
                value);
            break;

        case VideoPowerOff:
        case VideoPowerHibernate:
            WRITE_REGISTER_ULONG(
                (PULONG)((ULONG_PTR)Dx->VirtControlStart + NV2A_PMC_ENABLE),
                NV2A_PMC_ENABLE_ALL_DISABLE);
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

/* ------------------------------------------------------------------------- */
/* Mode operations --------------------------------------------------------- */

BOOLEAN
FASTCALL
XboxVmpSetCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE RequestedMode,
    PSTATUS_BLOCK StatusBlock)
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(StatusBlock);

    if (RequestedMode->RequestedMode != 0)
        return FALSE;

    /* Single mode (the one the firmware programmed); nothing to do. */
    return TRUE;
}

BOOLEAN
FASTCALL
XboxVmpResetDevice(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PSTATUS_BLOCK StatusBlock)
{
    UNREFERENCED_PARAMETER(StatusBlock);

    if (DeviceExtension != NULL)
        Nv2aAccelShutdown(DeviceExtension);

    return TRUE;
}

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
    FrameBuffer.QuadPart = READ_REGISTER_ULONG(
        (PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CRTC_FRAMEBUFFER_START));
    FrameBuffer.QuadPart &= 0x0FFFFFFF;
    if (FrameBuffer.QuadPart != 0x3C00000 && FrameBuffer.QuadPart != 0x7C00000)
        WARN_(IHVVIDEO, "Non-standard framebuffer address 0x%p\n", FrameBuffer.QuadPart);

    ASSERT(FrameBuffer.QuadPart % PAGE_SIZE == 0);

    DeviceExtension->FrameBufferGpuOffset = (ULONG)FrameBuffer.QuadPart;
    DeviceExtension->FrameBufferLength = NV2A_VIDEO_MEMORY_SIZE;

    FrameBuffer.QuadPart += DeviceExtension->PhysFrameBufferStart.QuadPart;
    MapInformation->VideoRamBase = RequestedAddress->RequestedVirtualAddress;
    MapInformation->VideoRamLength = NV2A_VIDEO_MEMORY_SIZE;

    if (VideoPortMapMemory(DeviceExtension, FrameBuffer,
                           &MapInformation->VideoRamLength,
                           &inIoSpace,
                           &MapInformation->VideoRamBase) != NO_ERROR)
    {
        ERR_(IHVVIDEO, "Failed to map framebuffer\n");
        return FALSE;
    }

    MapInformation->FrameBufferBase = MapInformation->VideoRamBase;
    MapInformation->FrameBufferLength = MapInformation->VideoRamLength;

    DeviceExtension->FrameBufferUserAddress = MapInformation->VideoRamBase;

    /* Tell the nVidia controller about the framebuffer */
    WRITE_REGISTER_ULONG(
        (PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CRTC_FRAMEBUFFER_START),
        (ULONG)FrameBuffer.QuadPart);

    INFO_(IHVVIDEO, "Mapped 0x%x bytes of phys mem at 0x%lx to virt addr 0x%p\n",
        MapInformation->VideoRamLength,
        (ULONG)FrameBuffer.QuadPart, MapInformation->VideoRamBase);

    /* Cache geometry so the accelerator and the IOCTL handlers don't have to
     * re-poll the RAMDAC every call. */
    {
        ULONG w, h;
        UCHAR bpp;

        w = READ_REGISTER_ULONG(
            (PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_RAMDAC_FP_HVALID_END)) + 1;
        h = READ_REGISTER_ULONG(
            (PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_RAMDAC_FP_VVALID_END)) + 1;
        if (w > 1 && h > 1)
        {
            bpp = NvGetBytesPerPixel(DeviceExtension, w);
            if (bpp >= 1 && bpp <= 4)
            {
                DeviceExtension->ScreenWidth  = w;
                DeviceExtension->ScreenHeight = h;
                DeviceExtension->BytesPerPixel = bpp;
                DeviceExtension->ScreenStride = w * bpp;
            }
        }
    }

    /* Best effort: bring up the 2D accelerator now that we have a destination
     * surface to point it at.  Failure is logged and silently demoted to the
     * CPU fallback. */
    (VOID)Nv2aAccelInitialize(DeviceExtension);

    /* The NV2A 3D (Kelvin) engine is brought up lazily on the first
     * IOCTL_VIDEO_NV2A_DRAW_3D from the xboxogl ICD (see XboxVmpHandleDraw3d).
     * The bare video-init self-test was removed: it rendered correctly but
     * couldn't be presented to the raw front buffer without the desktop's
     * surface composition, so let the system boot to the GUI where gltri drives
     * the real ICD -> ExtEscape -> Kelvin path and the desktop presents it. */

    return TRUE;
}

BOOLEAN
FASTCALL
XboxVmpUnmapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY VideoMemory,
    PSTATUS_BLOCK StatusBlock)
{
    UNREFERENCED_PARAMETER(StatusBlock);

    Nv2aAccelShutdown(DeviceExtension);

    VideoPortUnmapMemory(DeviceExtension,
                         VideoMemory->RequestedVirtualAddress, NULL);

    DeviceExtension->FrameBufferUserAddress = NULL;
    return TRUE;
}

BOOLEAN
FASTCALL
XboxVmpQueryNumAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_NUM_MODES Modes,
    PSTATUS_BLOCK StatusBlock)
{
    UNREFERENCED_PARAMETER(DeviceExtension);

    Modes->NumModes = 1;
    Modes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
    StatusBlock->Information = sizeof(VIDEO_NUM_MODES);
    return TRUE;
}

BOOLEAN
FASTCALL
XboxVmpQueryAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION VideoMode,
    PSTATUS_BLOCK StatusBlock)
{
    return XboxVmpQueryCurrentMode(DeviceExtension, VideoMode, StatusBlock);
}

static UCHAR
NvGetCrtc(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    UCHAR Index)
{
    WRITE_REGISTER_UCHAR(
        (PUCHAR)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CRTC_REGISTER_INDEX),
        Index);
    return READ_REGISTER_UCHAR(
        (PUCHAR)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_CRTC_REGISTER_VALUE));
}

static UCHAR
NvGetBytesPerPixel(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    ULONG ScreenWidth)
{
    UCHAR BytesPerPixel;

    /* Get BPP directly from NV2A CRTC (magic constants are from Cromwell) */
    BytesPerPixel = 8 * (((NvGetCrtc(DeviceExtension, 0x19) & 0xE0) << 3) |
                        (NvGetCrtc(DeviceExtension, 0x13) & 0xFF)) / ScreenWidth;

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

    VideoMode->VisScreenWidth = READ_REGISTER_ULONG(
        (PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_RAMDAC_FP_HVALID_END)) + 1;
    VideoMode->VisScreenHeight = READ_REGISTER_ULONG(
        (PULONG)((ULONG_PTR)DeviceExtension->VirtControlStart + NV2A_RAMDAC_FP_VVALID_END)) + 1;

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
    VideoMode->XMillimeter = 0;
    VideoMode->YMillimeter = 0;

    if (BytesPerPixel >= 3)
    {
        VideoMode->NumberRedBits = 8;
        VideoMode->NumberGreenBits = 8;
        VideoMode->NumberBlueBits = 8;
        VideoMode->RedMask   = 0xFF0000;
        VideoMode->GreenMask = 0x00FF00;
        VideoMode->BlueMask  = 0x0000FF;
    }
    else if (BytesPerPixel == 2)
    {
        /* Assume 5:6:5 — what Cromwell/NV2A use in 16 bpp modes. */
        VideoMode->NumberRedBits = 5;
        VideoMode->NumberGreenBits = 6;
        VideoMode->NumberBlueBits = 5;
        VideoMode->RedMask   = 0xF800;
        VideoMode->GreenMask = 0x07E0;
        VideoMode->BlueMask  = 0x001F;
    }
    else
    {
        /* 8 bpp palette mode.  GDI fills in the LUT via IOCTL_VIDEO_SET_COLOR_REGISTERS. */
        VideoMode->NumberRedBits = 8;
        VideoMode->NumberGreenBits = 8;
        VideoMode->NumberBlueBits = 8;
        VideoMode->RedMask = 0;
        VideoMode->GreenMask = 0;
        VideoMode->BlueMask = 0;
    }

    VideoMode->VideoMemoryBitmapWidth = VideoMode->VisScreenWidth;
    VideoMode->VideoMemoryBitmapHeight = VideoMode->VisScreenHeight;
    VideoMode->AttributeFlags = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR |
        VIDEO_MODE_NO_OFF_SCREEN;
    if (BytesPerPixel == 1)
        VideoMode->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
                                     VIDEO_MODE_MANAGED_PALETTE;
    VideoMode->DriverSpecificAttributeFlags = 0;

    StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION);

    if (VideoMode->VisScreenWidth * VideoMode->VisScreenHeight *
        (VideoMode->BitsPerPlane / 8) > NV2A_VIDEO_MEMORY_SIZE)
    {
        ERR_(IHVVIDEO, "Current screen resolution exceeds video memory bounds!\n");
        return FALSE;
    }

    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Palette / colour registers --------------------------------------------- */

static VOID
Nv2aProgramPalette(PXBOXVMP_DEVICE_EXTENSION Dx,
                   ULONG FirstEntry, ULONG NumEntries)
{
    ULONG i;
    PUCHAR base;

    base = (PUCHAR)Dx->VirtControlStart;

    /* The NV2A RAMDAC palette is a classic VGA-style indexed register pair.
     * Write the start index, then stream R, G, B triplets through the data
     * port.  Each component is 8 bits; we widen our 6-bit-style triplet to 8
     * because that's what the cached palette already stores. */
    WRITE_REGISTER_UCHAR(base + NV2A_USER_DAC_WRITE_MODE, (UCHAR)(FirstEntry & 0xFF));

    for (i = 0; i < NumEntries; i++)
    {
        WRITE_REGISTER_UCHAR(base + NV2A_USER_DAC_PALETTE_DATA,
                             Dx->Palette[(FirstEntry + i) * 3 + 0]);
        WRITE_REGISTER_UCHAR(base + NV2A_USER_DAC_PALETTE_DATA,
                             Dx->Palette[(FirstEntry + i) * 3 + 1]);
        WRITE_REGISTER_UCHAR(base + NV2A_USER_DAC_PALETTE_DATA,
                             Dx->Palette[(FirstEntry + i) * 3 + 2]);
    }
}

BOOLEAN
FASTCALL
XboxVmpSetColorRegisters(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_CLUT ColorLookUpTable,
    PSTATUS_BLOCK StatusBlock)
{
    ULONG i;
    ULONG first = ColorLookUpTable->FirstEntry;
    ULONG count = ColorLookUpTable->NumEntries;

    UNREFERENCED_PARAMETER(StatusBlock);

    if (first >= XBOXVMP_PALETTE_ENTRIES ||
        count == 0 ||
        first + count > XBOXVMP_PALETTE_ENTRIES)
    {
        return FALSE;
    }

    for (i = 0; i < count; i++)
    {
        DeviceExtension->Palette[(first + i) * 3 + 0] =
            ColorLookUpTable->LookupTable[i].RgbArray.Red;
        DeviceExtension->Palette[(first + i) * 3 + 1] =
            ColorLookUpTable->LookupTable[i].RgbArray.Green;
        DeviceExtension->Palette[(first + i) * 3 + 2] =
            ColorLookUpTable->LookupTable[i].RgbArray.Blue;
    }

    Nv2aProgramPalette(DeviceExtension, first, count);
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Private acceleration IOCTLs -------------------------------------------- */

static VOID
CpuFillRect(PXBOXVMP_DEVICE_EXTENSION Dx,
            ULONG X, ULONG Y, ULONG W, ULONG H, ULONG Color)
{
    ULONG row, col;
    PUCHAR fb = (PUCHAR)Dx->FrameBufferUserAddress;
    ULONG stride = Dx->ScreenStride;
    UCHAR bpp = Dx->BytesPerPixel;

    if (fb == NULL || bpp == 0)
        return;

    for (row = 0; row < H; row++)
    {
        PUCHAR dst = fb + (Y + row) * stride + X * bpp;
        switch (bpp)
        {
            case 1:
                for (col = 0; col < W; col++)
                    dst[col] = (UCHAR)Color;
                break;
            case 2: {
                PUSHORT p = (PUSHORT)dst;
                for (col = 0; col < W; col++)
                    p[col] = (USHORT)Color;
                break;
            }
            case 4: {
                PULONG p = (PULONG)dst;
                for (col = 0; col < W; col++)
                    p[col] = Color;
                break;
            }
            default:
                break;
        }
    }
}

static VOID
CpuScreenBlt(PXBOXVMP_DEVICE_EXTENSION Dx,
             ULONG SrcX, ULONG SrcY,
             ULONG DstX, ULONG DstY,
             ULONG W, ULONG H)
{
    PUCHAR fb = (PUCHAR)Dx->FrameBufferUserAddress;
    ULONG stride = Dx->ScreenStride;
    UCHAR bpp = Dx->BytesPerPixel;
    ULONG row;
    BOOLEAN reverse;
    ULONG rowBytes;

    if (fb == NULL || bpp == 0)
        return;

    rowBytes = W * bpp;
    /* Handle overlap: if dst is below src, copy bottom-up. */
    reverse = (DstY > SrcY) && (DstY < SrcY + H);

    for (row = 0; row < H; row++)
    {
        ULONG sr = reverse ? (H - 1 - row) : row;
        PUCHAR src = fb + (SrcY + sr) * stride + SrcX * bpp;
        PUCHAR dst = fb + (DstY + sr) * stride + DstX * bpp;
        VideoPortMoveMemory(dst, src, rowBytes);
    }
}

static BOOLEAN
XboxVmpHandleAccelFill(PXBOXVMP_DEVICE_EXTENSION Dx,
                       PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_FILL_RECT r;

    if (Rp->InputBufferLength < sizeof(NV2A_FILL_RECT))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    r = (PNV2A_FILL_RECT)Rp->InputBuffer;

    if (Dx->ScreenWidth == 0 || Dx->ScreenHeight == 0 ||
        r->X >= Dx->ScreenWidth || r->Y >= Dx->ScreenHeight ||
        r->Width == 0 || r->Height == 0 ||
        r->X + r->Width > Dx->ScreenWidth ||
        r->Y + r->Height > Dx->ScreenHeight)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (!Nv2aAccelFillRect(Dx, r->X, r->Y, r->Width, r->Height, r->Color))
        CpuFillRect(Dx, r->X, r->Y, r->Width, r->Height, r->Color);

    Rp->StatusBlock->Information = 0;
    return TRUE;
}

static BOOLEAN
XboxVmpHandleAccelBlt(PXBOXVMP_DEVICE_EXTENSION Dx,
                      PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_SCREEN_BLT b;

    if (Rp->InputBufferLength < sizeof(NV2A_SCREEN_BLT))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    b = (PNV2A_SCREEN_BLT)Rp->InputBuffer;

    if (Dx->ScreenWidth == 0 || Dx->ScreenHeight == 0 ||
        b->Width == 0 || b->Height == 0 ||
        b->SrcX + b->Width  > Dx->ScreenWidth  ||
        b->SrcY + b->Height > Dx->ScreenHeight ||
        b->DstX + b->Width  > Dx->ScreenWidth  ||
        b->DstY + b->Height > Dx->ScreenHeight)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (!Nv2aAccelScreenBlt(Dx, b->SrcX, b->SrcY, b->DstX, b->DstY,
                            b->Width, b->Height))
        CpuScreenBlt(Dx, b->SrcX, b->SrcY, b->DstX, b->DstY,
                     b->Width, b->Height);

    Rp->StatusBlock->Information = 0;
    return TRUE;
}

static BOOLEAN
XboxVmpHandleAccelCaps(PXBOXVMP_DEVICE_EXTENSION Dx,
                       PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_CAPS caps;

    if (Rp->OutputBufferLength < sizeof(NV2A_CAPS))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    caps = (PNV2A_CAPS)Rp->OutputBuffer;
    caps->StructSize = sizeof(NV2A_CAPS);
    caps->HardwareAccelEnabled = Dx->AccelEnabled ? 1 : 0;
    caps->PushBufferSize = Dx->PushBufferSize;
    caps->PushBufferGpuOffset = Dx->PushBufferGpuOffset;
    caps->SurfacePitch = Dx->SurfacePitch;
    caps->SurfaceFormat = Dx->SurfaceFormat;
    caps->FrameBufferGpuOffset = Dx->FrameBufferGpuOffset;

    /* Offscreen device-bitmap heap.  VRAM layout (all relative to FB base):
     *   FB+0x000000  visible framebuffer        (stride*height)
     *   FB+0x152000  Z16 zeta / depth buffer    (ScreenWidth*2*height)
     *   FB+0x200000  3D offscreen colour surface(stride*height)
     *   FB+0x352000  texture heap               (0x60000, NV2A textures)
     *   ...heap...                              (this region)
     *   top          push buffer
     * The device-bitmap heap is the gap between the texture heap and the push
     * buffer; it stays clear of the zeta buffer, 3D scratch, and textures. */
    {
        ULONG heapStart, heapLimit;
        heapStart = Dx->FrameBufferGpuOffset + 0x352000 + 0x60000; /* past texture heap */
        heapLimit = Dx->PushBufferGpuOffset;            /* up to (but not into) the push buffer */
        caps->OffscreenHeapStart = heapStart;
        caps->OffscreenHeapSize  = (heapLimit > heapStart) ? (heapLimit - heapStart) : 0;
    }

    Rp->StatusBlock->Information = sizeof(NV2A_CAPS);
    return TRUE;
}

static BOOLEAN
XboxVmpHandleAccelBltEx(PXBOXVMP_DEVICE_EXTENSION Dx,
                        PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_BLT_EX b;
    ULONG fbBase, fbEnd, bpp;

    if (Rp->InputBufferLength < sizeof(NV2A_BLT_EX))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    b = (PNV2A_BLT_EX)Rp->InputBuffer;
    bpp = Dx->BytesPerPixel;
    fbBase = Dx->FrameBufferGpuOffset;
    fbEnd  = Dx->FrameBufferGpuOffset + Dx->FrameBufferLength;

    /* Both surfaces must live inside the mapped VRAM and the blt must stay within
     * each surface's rows (bounded by FrameBufferLength). */
    if (b->Width == 0 || b->Height == 0 || bpp == 0 ||
        b->SrcOffset < fbBase || b->SrcOffset >= fbEnd ||
        b->DstOffset < fbBase || b->DstOffset >= fbEnd ||
        b->SrcOffset + (b->SrcY + b->Height) * b->SrcPitch > fbEnd ||
        b->DstOffset + (b->DstY + b->Height) * b->DstPitch > fbEnd)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (!Nv2aAccelBltEx(Dx, b->SrcOffset, b->SrcPitch, b->SrcX, b->SrcY,
                        b->DstOffset, b->DstPitch, b->DstX, b->DstY,
                        b->Width, b->Height))
    {
        /* CPU fallback: copy row by row through the mapped framebuffer. */
        PUCHAR fb = (PUCHAR)Dx->FrameBufferUserAddress;
        if (fb == NULL)
        {
            Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
        {
            ULONG row, rowBytes = b->Width * bpp;
            PUCHAR src = fb + (b->SrcOffset - fbBase);
            PUCHAR dst = fb + (b->DstOffset - fbBase);
            for (row = 0; row < b->Height; row++)
                VideoPortMoveMemory(dst + (b->DstY + row) * b->DstPitch + b->DstX * bpp,
                                    src + (b->SrcY + row) * b->SrcPitch + b->SrcX * bpp,
                                    rowBytes);
        }
    }

    Rp->StatusBlock->Information = 0;
    return TRUE;
}

static BOOLEAN
XboxVmpHandleDraw3d(PXBOXVMP_DEVICE_EXTENSION Dx,
                    PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_DRAW_3D draw;
    ULONG count, need;

    if (Rp->InputBufferLength < FIELD_OFFSET(NV2A_DRAW_3D, Verts))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    draw = (PNV2A_DRAW_3D)Rp->InputBuffer;
    count = draw->VertexCount;

    /* count == 0 is valid (clear / present only).  Any topology (points/lines/
     * triangles) otherwise; just bound it to the payload capacity. */
    if (count > NV2A_3D_MAX_VERTS)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    need = FIELD_OFFSET(NV2A_DRAW_3D, Verts) + count * sizeof(NV2A_3D_VERTEX);
    if (Rp->InputBufferLength < need)
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    /* Lazy 3D bring-up on first use (after the framebuffer/accel are mapped). */
    if (!Dx->Nv3dInitAttempted)
        Nv2a3dInitialize(Dx);

    if (!Nv2a3dDrawTriangles(Dx, draw))
    {
        /* 3D unavailable — signal the caller to use its CPU/GDI fallback. */
        Rp->StatusBlock->Status = ERROR_INVALID_FUNCTION;
        return FALSE;
    }

    Rp->StatusBlock->Information = 0;
    return TRUE;
}

/* IOCTL_VIDEO_NV2A_TEX_UPLOAD: copy a linear A8R8G8B8 texture into the VRAM
 * texture heap; return the assigned GPU offset (0 = failure) in the output. */
static BOOLEAN
XboxVmpHandleTexUpload(PXBOXVMP_DEVICE_EXTENSION Dx, PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_TEX_UPLOAD up;
    ULONG need, offset;

    if (Rp->InputBufferLength < sizeof(NV2A_TEX_UPLOAD) ||
        Rp->OutputBufferLength < sizeof(ULONG))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    up = (PNV2A_TEX_UPLOAD)Rp->InputBuffer;
    if (up->Width == 0 || up->Height == 0 || up->Width > 4096 || up->Height > 4096)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    need = sizeof(NV2A_TEX_UPLOAD) + up->Width * up->Height * 4;
    if (Rp->InputBufferLength < need)
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }

    if (!Dx->Nv3dInitAttempted)
        Nv2a3dInitialize(Dx);

    offset = Nv2aTexUpload(Dx, up->Width, up->Height, up->ExistingOffset,
                           (const ULONG *)((PUCHAR)up + sizeof(NV2A_TEX_UPLOAD)));

    *(ULONG *)Rp->OutputBuffer = offset;
    Rp->StatusBlock->Information = sizeof(ULONG);
    return (offset != 0);
}

/* IOCTL_VIDEO_NV2A_READBACK: copy an ARGB rect of the offscreen colour surface
 * into the output buffer (Width*Height DWORDs, top-down). */
static BOOLEAN
XboxVmpHandleReadback(PXBOXVMP_DEVICE_EXTENSION Dx, PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_READBACK rb;
    ULONG need;

    if (Rp->InputBufferLength < sizeof(NV2A_READBACK))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }
    rb = (PNV2A_READBACK)Rp->InputBuffer;
    if (rb->Width == 0 || rb->Height == 0 || rb->Width > 4096 || rb->Height > 4096)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }
    need = rb->Width * rb->Height * 4;
    if (Rp->OutputBufferLength < need)
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }
    if (!Dx->Nv3dInitAttempted)
        Nv2a3dInitialize(Dx);
    if (!Nv2aReadback(Dx, rb->X, rb->Y, rb->Width, rb->Height, (ULONG *)Rp->OutputBuffer))
    {
        Rp->StatusBlock->Status = ERROR_INVALID_FUNCTION;
        return FALSE;
    }
    Rp->StatusBlock->Information = need;
    return TRUE;
}

/* IOCTL_VIDEO_NV2A_WRITEBACK: write an ARGB rect (after the header) into the
 * offscreen colour surface (glDrawPixels / glBitmap / glCopyPixels). */
static BOOLEAN
XboxVmpHandleWriteback(PXBOXVMP_DEVICE_EXTENSION Dx, PVIDEO_REQUEST_PACKET Rp)
{
    PNV2A_WRITEBACK wb;
    ULONG need;

    if (Rp->InputBufferLength < sizeof(NV2A_WRITEBACK))
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }
    wb = (PNV2A_WRITEBACK)Rp->InputBuffer;
    if (wb->Width == 0 || wb->Height == 0 || wb->Width > 4096 || wb->Height > 4096)
    {
        Rp->StatusBlock->Status = ERROR_INVALID_PARAMETER;
        return FALSE;
    }
    need = sizeof(NV2A_WRITEBACK) + wb->Width * wb->Height * 4;
    if (Rp->InputBufferLength < need)
    {
        Rp->StatusBlock->Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }
    if (!Dx->Nv3dInitAttempted)
        Nv2a3dInitialize(Dx);
    if (!Nv2aWriteback(Dx, wb->X, wb->Y, wb->Width, wb->Height, wb->Flags,
                       (const ULONG *)((PUCHAR)wb + sizeof(NV2A_WRITEBACK))))
    {
        Rp->StatusBlock->Status = ERROR_INVALID_FUNCTION;
        return FALSE;
    }
    Rp->StatusBlock->Information = 0;
    return TRUE;
}

/* EOF */
