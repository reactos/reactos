/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Simple framebuffer driver for NVIDIA NV2A XGPU
 * COPYRIGHT:   Copyright 2004 Ge van Geldorp
 *              Copyright 2004 Filip Navara
 *              Copyright 2019-2020 Stanislav Motylkov (x86corez@gmail.com)
 */

#pragma once

/* INCLUDES *******************************************************************/

/*
 * FIXME: specify headers properly in the triangle brackets and rearrange them
 * in a way so it would be simpler to add NDK and other headers for debugging.
 */
#include "ntdef.h"
#define PAGE_SIZE 4096
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ioaccess.h"
#include "video.h"

#include <drivers/xbox/xgpu.h>

/* Number of palette entries we keep in software for 8 bpp modes */
#define XBOXVMP_PALETTE_ENTRIES 256

#define _XBOXVMP_DEVICE_EXTENSION_DEFINED

typedef struct _XBOXVMP_DEVICE_EXTENSION
{
    /* Hardware resources */
    PHYSICAL_ADDRESS PhysControlStart;
    ULONG ControlLength;
    PVOID VirtControlStart;
    PHYSICAL_ADDRESS PhysFrameBufferStart;

    /* Cached mode state */
    ULONG ScreenWidth;
    ULONG ScreenHeight;
    ULONG ScreenStride;
    UCHAR BytesPerPixel;
    ULONG FrameBufferGpuOffset; /* offset inside VRAM (low 28 bits of CRTC) */
    ULONG FrameBufferLength;

    /* User-mapped framebuffer (also used by the CPU fallback) */
    PVOID FrameBufferUserAddress;

    /* NV2A 2D engine accelerator state.  Allocated lazily by Nv2aAccelInitialize. */
    BOOLEAN AccelInitAttempted;
    BOOLEAN AccelEnabled;        /* TRUE iff hardware acceleration is active */
    ULONG   PushBufferGpuOffset; /* GPU-relative byte offset of our pushbuffer */
    ULONG   PushBufferSize;
    PULONG  PushBufferVirt;      /* CPU-side mapping of the pushbuffer */
    ULONG   PushBufferPut;       /* write offset (DWORDs from start) */
    ULONG   SurfacePitch;
    ULONG   SurfaceFormat;       /* NV04_SURFACES_2D_FORMAT_* */

    /* NV2A 3D (Kelvin) state.  Set up lazily on the first 3D draw. */
    BOOLEAN Nv3dInitAttempted;
    BOOLEAN Nv3dReady;
    ULONG   DepthBufferGpuOffset; /* offscreen colour surface (FB+0x200000) */
    PVOID   OffscreenVirt;        /* CPU/kernel mapping of the offscreen surface (readback/writeback); lazily mapped */

    /* Texture VRAM heap (linear A8R8G8B8 textures uploaded by the ICD).  A simple
     * bump allocator: the game uploads its sprites once at startup. */
    ULONG   TextureHeapGpuOffset; /* GPU offset (== guest-physical addr) of the texture heap base */
    ULONG   TextureHeapSize;      /* bytes */
    ULONG   TextureHeapNext;      /* bump cursor (GPU offset of next free byte) */
    PVOID   TextureHeapVirt;      /* CPU-side mapping for pixel uploads */
    /* When non-NULL, TextureHeapVirt is a large physically-contiguous system-RAM
     * pool (VideoPortAllocateContiguousMemory) the NV2A textures from directly via
     * the base-0 DMA_3D object (UMA).  Freed with MmFreeContiguousMemory.  When
     * NULL, the heap is the small VideoPortMapMemory'd carve-out of the 4 MB FB. */
    PVOID   TextureHeapContig;
    /* Z24S8 depth/stencil buffer in a contiguous system-RAM allocation (UMA: the
     * base-0 DMA_3D object addresses it).  Z24S8 (4 B/px) doesn't fit the 4 MB FB
     * gap, so it lives in system RAM like the texture pool — this gives 24-bit
     * depth precision (vs the old in-gap Z16, which z-fought badly with games'
     * 0.1/100 frustums).  NULL = fell back to the in-gap Z16 surface. */
    PVOID   DepthZetaContig;
    ULONG   DepthZetaGpuOffset;

    /* Saved palette for the 8 bpp path (rebroadcast on mode change). */
    UCHAR  Palette[XBOXVMP_PALETTE_ENTRIES * 3];
} XBOXVMP_DEVICE_EXTENSION, *PXBOXVMP_DEVICE_EXTENSION;

VP_STATUS
NTAPI
XboxVmpFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PVOID HwContext,
    IN PWSTR ArgumentString,
    IN OUT PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    OUT PUCHAR Again);

BOOLEAN
NTAPI
XboxVmpInitialize(
    PVOID HwDeviceExtension);

BOOLEAN
NTAPI
XboxVmpStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket);

BOOLEAN
NTAPI
XboxVmpResetHw(
    PVOID DeviceExtension,
    ULONG Columns,
    ULONG Rows);

VP_STATUS
NTAPI
XboxVmpGetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl);

VP_STATUS
NTAPI
XboxVmpSetPowerState(
    PVOID HwDeviceExtension,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl);

BOOLEAN
FASTCALL
XboxVmpSetCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE RequestedMode,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpResetDevice(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpMapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY RequestedAddress,
    PVIDEO_MEMORY_INFORMATION MapInformation,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpUnmapVideoMemory(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MEMORY VideoMemory,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpQueryNumAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_NUM_MODES Modes,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpQueryAvailModes(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION ReturnedModes,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpQueryCurrentMode(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_MODE_INFORMATION VideoModeInfo,
    PSTATUS_BLOCK StatusBlock);

BOOLEAN
FASTCALL
XboxVmpSetColorRegisters(
    PXBOXVMP_DEVICE_EXTENSION DeviceExtension,
    PVIDEO_CLUT ColorLookUpTable,
    PSTATUS_BLOCK StatusBlock);

/* EOF */
