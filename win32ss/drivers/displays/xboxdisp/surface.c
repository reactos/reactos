/*
 * PROJECT:     Xbox NV2A accelerated GDI display driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Surface enable / disable, mode assertion
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "xboxdisp.h"

HSURF APIENTRY
DrvEnableSurface(IN DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV)dhpdev;
    HSURF hSurface;
    ULONG BitmapType;
    SIZEL ScreenSize;
    VIDEO_MEMORY VideoMemory;
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    ULONG ulTemp;

    if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_CURRENT_MODE,
                           &(ppdev->ModeIndex), sizeof(ULONG), NULL, 0, &ulTemp))
        return NULL;

    VideoMemory.RequestedVirtualAddress = NULL;
    if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                           &VideoMemory, sizeof(VIDEO_MEMORY),
                           &VideoMemoryInfo, sizeof(VIDEO_MEMORY_INFORMATION),
                           &ulTemp))
        return NULL;

    ppdev->ScreenPtr = VideoMemoryInfo.FrameBufferBase;

    /* Base + length of the whole mapped video memory, for the offscreen
     * device-bitmap heap (a slice above the visible framebuffer). */
    ppdev->VramBase = VideoMemoryInfo.FrameBufferBase;
    ppdev->VramLen  = (ULONG)VideoMemoryInfo.VideoRamLength;

    switch (ppdev->BitsPerPixel)
    {
        case 8:  IntSetPalette(dhpdev, ppdev->PaletteEntries, 0, 256);
                 BitmapType = BMF_8BPP;  break;
        case 16: BitmapType = BMF_16BPP; break;
        case 24: BitmapType = BMF_24BPP; break;
        case 32: BitmapType = BMF_32BPP; break;
        default: return NULL;
    }

    ppdev->iDitherFormat = BitmapType;
    ScreenSize.cx = ppdev->ScreenWidth;
    ScreenSize.cy = ppdev->ScreenHeight;

    hSurface = (HSURF)EngCreateBitmap(ScreenSize, ppdev->ScreenDelta, BitmapType,
                                      (ppdev->ScreenDelta > 0) ? BMF_TOPDOWN : 0,
                                      ppdev->ScreenPtr);
    if (hSurface == NULL)
        return NULL;

    /* HOOK_BITBLT enables our DrvBitBlt; HOOK_COPYBITS routes screen-to-screen
     * blits through DrvCopyBits.  Anything we don't claim falls back to the
     * engine. */
    if (!EngAssociateSurface(hSurface, ppdev->hDevEng,
                             HOOK_BITBLT | HOOK_COPYBITS))
    {
        EngDeleteSurface(hSurface);
        return NULL;
    }

    ppdev->hSurfEng = hSurface;
    return hSurface;
}

VOID APIENTRY
DrvDisableSurface(IN DHPDEV dhpdev)
{
    DWORD ulTemp;
    VIDEO_MEMORY VideoMemory;
    PPDEV ppdev = (PPDEV)dhpdev;

    EngDeleteSurface(ppdev->hSurfEng);
    ppdev->hSurfEng = NULL;

    VideoMemory.RequestedVirtualAddress = ppdev->ScreenPtr;
    EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                       &VideoMemory, sizeof(VIDEO_MEMORY), NULL, 0, &ulTemp);
}

BOOL APIENTRY
DrvAssertMode(IN DHPDEV dhpdev, IN BOOL bEnable)
{
    PPDEV ppdev = (PPDEV)dhpdev;
    ULONG ulTemp;

    if (bEnable)
    {
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_CURRENT_MODE,
                               &(ppdev->ModeIndex), sizeof(ULONG), NULL, 0, &ulTemp))
            return FALSE;
        if (ppdev->BitsPerPixel == 8)
            IntSetPalette(dhpdev, ppdev->PaletteEntries, 0, 256);
        return TRUE;
    }
    return !EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_RESET_DEVICE,
                               NULL, 0, NULL, 0, &ulTemp);
}
