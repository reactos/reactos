/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vbe.h
 * PURPOSE:         VDM VESA BIOS Extensions (for the Cirrus CL-GD5434 emulated card)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _VBE_H_
#define _VBE_H_

#include "hardware/video/svga.h"

/* DEFINITIONS ****************************************************************/

#define OEM_NAME     "Cirrus Logic GD-5434 VGA"
#define OEM_NAME_PTR 0xF01D5091

#define VBE_MODE_COUNT 40

/* VBE Mode Attribute Flags */
#define VBE_MODE_SUPPORTED     (1 << 0)
#define VBE_MODE_OPTIONAL_INFO (1 << 1)
#define VBE_MODE_BIOS_SUPPORT  (1 << 2)
#define VBE_MODE_COLOR         (1 << 3)
#define VBE_MODE_GRAPHICS      (1 << 4)

/* VBE Window Attribute Flags */
#define VBE_WINDOW_EXISTS   (1 << 0)
#define VBE_WINDOW_READABLE (1 << 1)
#define VBE_WINDOW_WRITABLE (1 << 2)

/* VBE Memory Models */
#define VBE_MODEL_TEXT      0
#define VBE_MODEL_CGA       1
#define VBE_MODEL_HGC       2
#define VBE_MODEL_EGA       3
#define VBE_MODEL_PACKED    4
#define VBE_MODEL_UNCHAINED 5
#define VBE_MODEL_DIRECT    6
#define VBE_MODEL_YUV       7

#pragma pack(push, 1)

typedef struct _VBE_INFORMATION
{
    DWORD Signature;
    WORD Version;
    DWORD OemName;
    DWORD Capabilities;
    DWORD ModeList;
    WORD VideoMemory;
    WORD ModeListBuffer[118];
} VBE_INFORMATION, *PVBE_INFORMATION;

C_ASSERT(sizeof(VBE_INFORMATION) == 256);

typedef struct _VBE_MODE_INFO
{
    WORD Attributes;
    BYTE WindowAttrA;
    BYTE WindowAttrB;
    WORD WindowGranularity;
    WORD WindowSize;
    WORD WindowSegmentA;
    WORD WindowSegmentB;
    DWORD WindowPosFunc;
    WORD BytesPerScanline;
    WORD Width;
    WORD Height;
    BYTE CellWidth;
    BYTE CellHeight;
    BYTE NumPlanes;
    BYTE BitsPerPixel;
    BYTE NumBanks;
    BYTE MemoryModel;
    BYTE BankSize;
    BYTE ImagePages;
    BYTE Reserved;
    BYTE RedMaskSize;
    BYTE RedFieldPosition;
    BYTE GreenMaskSize;
    BYTE GreenFieldPosition;
    BYTE BlueMaskSize;
    BYTE BlueFieldPosition;
    BYTE ReservedMaskSize;
    BYTE ReservedFieldPosition;
    BYTE DirectColorInfo;
} VBE_MODE_INFO, *PVBE_MODE_INFO;

C_ASSERT(sizeof(VBE_MODE_INFO) % sizeof(WORD) == 0);

#pragma pack(pop)

typedef struct _VBE_MODE
{
    BYTE Number;
    WORD VesaNumber;
    PVBE_MODE_INFO Info;
    PSVGA_REGISTERS Registers; // NULL means "forward to VGABIOS"
} VBE_MODE, *PVBE_MODE;

/* FUNCTIONS ******************************************************************/

VOID WINAPI VbeService(LPWORD Stack);
BOOLEAN VbeInitialize(VOID);

#endif // _VBE_H_
