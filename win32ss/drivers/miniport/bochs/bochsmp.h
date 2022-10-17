/*
 * PROJECT:     ReactOS Bochs graphics card driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Bochs graphics card driver
 * COPYRIGHT:   Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#ifndef BOCHS_H
#define BOCHS_H

#include <ntdef.h>
#include <dderror.h>
#include <miniport.h>
#include <video.h>
#include <devioctl.h>
#include <section_attribs.h>

#define VBE_EDID_SIZE                        0x80

#define VBE_DISPI_IOPORT_INDEX               0x01CE
#define VBE_DISPI_IOPORT_DATA                0x01CF
#define VBE_DISPI_INDEX_ID                   0x00
  #define VBE_DISPI_ID0                        0xB0C0
  #define VBE_DISPI_ID1                        0xB0C1
  #define VBE_DISPI_ID2                        0xB0C2
  #define VBE_DISPI_ID3                        0xB0C3
  #define VBE_DISPI_ID4                        0xB0C4
  #define VBE_DISPI_ID5                        0xB0C5
#define VBE_DISPI_INDEX_XRES                 0x01
#define VBE_DISPI_INDEX_YRES                 0x02
#define VBE_DISPI_INDEX_BPP                  0x03
#define VBE_DISPI_INDEX_ENABLE               0x04
  #define VBE_DISPI_DISABLED                   0x00
  #define VBE_DISPI_ENABLED                    0x01
  #define VBE_DISPI_GETCAPS                    0x02
  #define VBE_DISPI_LFB_ENABLED                0x40
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K     0x0A

#define BOCHS_TAG 'BCHS'

typedef struct
{
    USHORT XResolution;
    USHORT YResolution;
} BOCHS_SIZE, *PBOCHS_SIZE;

typedef struct
{
    PUCHAR Mapped;
    PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    UCHAR RangeInIoSpace;
} BOCHS_ADDRESS_RANGE;

typedef struct
{
    PBOCHS_SIZE AvailableModeInfo;
    ULONG AvailableModeCount;
    USHORT CurrentMode;

    BOCHS_ADDRESS_RANGE FrameBuffer;
    BOCHS_ADDRESS_RANGE IoPorts;

    ULONG MaxXResolution;
    ULONG MaxYResolution;
    ULONG VramSize64K;
} BOCHS_DEVICE_EXTENSION, *PBOCHS_DEVICE_EXTENSION;

#endif //BOCHS_H
