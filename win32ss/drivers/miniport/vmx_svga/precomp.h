/*
 * PROJECT:         ReactOS
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            win32ss/drivers/miniport/vmx_svga/precomp.h
 * PURPOSE:         VMware SVGA-II video miniport definitions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

#include <ntdef.h>
#include <dderror.h>
#include <miniport.h>
#include <video.h>
#include <devioctl.h>

#include "vmx_regs.h"

#define VMX_MAX_MODES 23

typedef struct _VMX_ADDRESS_RANGE
{
    PUCHAR Mapped;
    PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    UCHAR RangeInIoSpace;
} VMX_ADDRESS_RANGE, *PVMX_ADDRESS_RANGE;

typedef struct _HW_DEVICE_EXTENSION
{
    ULONG Version;

    VMX_ADDRESS_RANGE IoPorts;
    VMX_ADDRESS_RANGE FrameBuffer;
    VMX_ADDRESS_RANGE FifoRange;

    PULONG IndexPort;
    PULONG ValuePort;
    PULONG InterruptPort;
    PULONG Fifo;

    ULONG VramSize;
    ULONG FrameBufferSize;
    ULONG MemSize;
    ULONG FrameBufferOffset;
    ULONG Capabilities;
    ULONG MaxWidth;
    ULONG MaxHeight;
    ULONG BitsPerPixel;
    ULONG Depth;
    ULONG RedMask;
    ULONG GreenMask;
    ULONG BlueMask;

    VIDEO_MODE_INFORMATION Modes[VMX_MAX_MODES];
    ULONG VideoModeCount;
    ULONG CurrentModeIndex;

    ULONG InterruptState;
    BOOLEAN FifoReady;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;
