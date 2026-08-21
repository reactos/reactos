/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            win32ss/drivers/miniport/vmx_svga/vmx_svga_ioctl.h
 * PURPOSE:         VMware SVGA-II private display/miniport interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

/* Private function codes use the vendor range beginning at 0x800. */
#define IOCTL_VIDEO_VMX_SVGA_UPDATE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VMX_SVGA_UPDATE_RECT
{
    LONG Left;
    LONG Top;
    LONG Right;
    LONG Bottom;
} VMX_SVGA_UPDATE_RECT, *PVMX_SVGA_UPDATE_RECT;
