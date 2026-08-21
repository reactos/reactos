/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            win32ss/drivers/displays/vmx_svga/surface.c
 * PURPOSE:         VMware SVGA-II device and backing surfaces
 */

#include "vmx_svga_disp.h"

static ULONG
VmxGetBitmapFormat(UCHAR BitsPerPixel)
{
    switch (BitsPerPixel)
    {
        case 8:  return BMF_8BPP;
        case 16: return BMF_16BPP;
        case 24: return BMF_24BPP;
        case 32: return BMF_32BPP;
        default: return 0;
    }
}

static VOID
VmxReleaseSurface(PPDEV ppdev)
{
    VIDEO_MEMORY VideoMemory;
    ULONG ReturnedLength;

    if (ppdev->hSurfEng != NULL)
    {
        EngDeleteSurface(ppdev->hSurfEng);
        ppdev->hSurfEng = NULL;
    }

    if (ppdev->psoBacking != NULL)
    {
        EngUnlockSurface(ppdev->psoBacking);
        ppdev->psoBacking = NULL;
    }

    if (ppdev->hBacking != NULL)
    {
        EngDeleteSurface(ppdev->hBacking);
        ppdev->hBacking = NULL;
    }

    if (ppdev->VideoRamPtr != NULL)
    {
        VideoMemory.RequestedVirtualAddress = ppdev->VideoRamPtr;
        EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                           &VideoMemory,
                           sizeof(VideoMemory),
                           NULL,
                           0,
                           &ReturnedLength);
        ppdev->VideoRamPtr = NULL;
        ppdev->ScreenPtr = NULL;
    }
}

HSURF APIENTRY
DrvEnableSurface(IN DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV)dhpdev;
    VIDEO_MODE VideoMode;
    VIDEO_MODE_INFORMATION ModeInfo;
    VIDEO_MEMORY VideoMemory;
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    ULONG ReturnedLength;
    ULONG BitmapFormat;
    SIZEL ScreenSize;

    VideoMode.RequestedMode = ppdev->ModeIndex;
    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_CURRENT_MODE,
                           &VideoMode,
                           sizeof(VideoMode),
                           NULL,
                           0,
                           &ReturnedLength))
    {
        return NULL;
    }

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_CURRENT_MODE,
                           NULL,
                           0,
                           &ModeInfo,
                           sizeof(ModeInfo),
                           &ReturnedLength))
    {
        return NULL;
    }

    ppdev->ScreenWidth = ModeInfo.VisScreenWidth;
    ppdev->ScreenHeight = ModeInfo.VisScreenHeight;
    ppdev->ScreenDelta = ModeInfo.ScreenStride;
    ppdev->BitsPerPixel = (UCHAR)(ModeInfo.BitsPerPlane * ModeInfo.NumberOfPlanes);
    ppdev->RedMask = ModeInfo.RedMask;
    ppdev->GreenMask = ModeInfo.GreenMask;
    ppdev->BlueMask = ModeInfo.BlueMask;

    BitmapFormat = VmxGetBitmapFormat(ppdev->BitsPerPixel);
    if (BitmapFormat == 0)
        return NULL;

    VideoMemory.RequestedVirtualAddress = NULL;
    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                           &VideoMemory,
                           sizeof(VideoMemory),
                           &VideoMemoryInfo,
                           sizeof(VideoMemoryInfo),
                           &ReturnedLength))
    {
        return NULL;
    }

    ppdev->VideoRamPtr = VideoMemoryInfo.VideoRamBase;
    ppdev->ScreenPtr = VideoMemoryInfo.FrameBufferBase;

    ScreenSize.cx = ppdev->ScreenWidth;
    ScreenSize.cy = ppdev->ScreenHeight;

    ppdev->hBacking = (HSURF)EngCreateBitmap(ScreenSize,
                                              ppdev->ScreenDelta,
                                              BitmapFormat,
                                              BMF_TOPDOWN,
                                              ppdev->ScreenPtr);
    if (ppdev->hBacking == NULL)
    {
        VmxReleaseSurface(ppdev);
        return NULL;
    }

    ppdev->psoBacking = EngLockSurface(ppdev->hBacking);
    if (ppdev->psoBacking == NULL)
    {
        VmxReleaseSurface(ppdev);
        return NULL;
    }

    ppdev->hSurfEng = EngCreateDeviceSurface((DHSURF)ppdev,
                                               ScreenSize,
                                               BitmapFormat);
    if (ppdev->hSurfEng == NULL ||
        !EngAssociateSurface(ppdev->hSurfEng, ppdev->hDevEng, VMX_SURFACE_HOOKS))
    {
        VmxReleaseSurface(ppdev);
        return NULL;
    }

    return ppdev->hSurfEng;
}

VOID APIENTRY
DrvDisableSurface(IN DHPDEV dhpdev)
{
    VmxReleaseSurface((PPDEV)dhpdev);
}

BOOL APIENTRY
DrvAssertMode(IN DHPDEV dhpdev, IN BOOL bEnable)
{
    PPDEV ppdev = (PPDEV)dhpdev;
    VIDEO_MODE VideoMode;
    ULONG ReturnedLength;

    if (bEnable)
    {
        VideoMode.RequestedMode = ppdev->ModeIndex;
        return EngDeviceIoControl(ppdev->hDriver,
                                  IOCTL_VIDEO_SET_CURRENT_MODE,
                                  &VideoMode,
                                  sizeof(VideoMode),
                                  NULL,
                                  0,
                                  &ReturnedLength) == 0;
    }

    return EngDeviceIoControl(ppdev->hDriver,
                              IOCTL_VIDEO_RESET_DEVICE,
                              NULL,
                              0,
                              NULL,
                              0,
                              &ReturnedLength) == 0;
}
