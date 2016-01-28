/*
 * PROJECT:         VGA Miniport Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            win32ss/drivers/miniport/vga_new/vbe.h
 * PURPOSE:         VESA VBE Registers and Structures
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#pragma once

#define LOWORD(l)	((USHORT)((ULONG_PTR)(l)))
#define HIWORD(l)	((USHORT)(((ULONG_PTR)(l)>>16)&0xFFFF))


/*
 * VBE Command Definitions
 */

#define VBE_GET_CONTROLLER_INFORMATION       0x4F00
#define VBE_GET_MODE_INFORMATION             0x4F01
#define VBE_SET_VBE_MODE                     0x4F02
#define VBE_GET_CURRENT_VBE_MODE             0x4F03
#define VBE_SAVE_RESTORE_STATE               0x4F04
#define VBE_DISPLAY_WINDOW_CONTROL           0x4F05
#define VBE_SET_GET_LOGICAL_SCAN_LINE_LENGTH 0x4F06
#define VBE_SET_GET_DISPLAY_START            0x4F07
#define VBE_SET_GET_DAC_PALETTE_FORMAT       0x4F08
#define VBE_SET_GET_PALETTE_DATA             0x4F09

/* VBE 2.0+ */
#define VBE_RETURN_PROTECTED_MODE_INTERFACE  0x4F0A
#define VBE_GET_SET_PIXEL_CLOCK              0x4F0B

/* Extensions */
#define VBE_POWER_MANAGEMENT_EXTENSIONS      0x4F10
#define VBE_FLAT_PANEL_INTERFACE_EXTENSIONS  0x4F11
#define VBE_AUDIO_INTERFACE_EXTENSIONS       0x4F12
#define VBE_OEM_EXTENSIONS                   0x4F13
#define VBE_DISPLAY_DATA_CHANNEL             0x4F14
#define VBE_DDC                              0x4F15

/*
 * VBE DDC Sub-Functions
 */

#define VBE_DDC_READ_EDID                      0x01
#define VBE_DDC_REPORT_CAPABILITIES            0x10
#define VBE_DDC_BEGIN_SCL_SDA_CONTROL          0x11
#define VBE_DDC_END_SCL_SDA_CONTROL            0x12
#define VBE_DDC_WRITE_SCL_CLOCK_LINE           0x13
#define VBE_DDC_WRITE_SDA_DATA_LINE            0x14
#define VBE_DDC_READ_SCL_CLOCK_LINE            0x15
#define VBE_DDC_READ_SDA_DATA_LINE             0x16

/*
 * VBE Video Mode Information Definitions
 */
#define VBE_MODEATTR_VALID                      0x01
#define VBE_MODEATTR_COLOR                      0x08
#define VBE_MODEATTR_GRAPHICS                   0x10
#define VBE_MODEATTR_NON_VGA                    0x20
#define VBE_MODEATTR_NO_BANK_SWITCH             0x40
#define VBE_MODEATTR_LINEAR                     0x80

#define VBE_MODE_BITS                           8
#define VBE_MODE_RESERVED_1                     0x200
#define VBE_MODE_RESERVED_2                     0x400
#define VBE_MODE_REFRESH_CONTROL                0x800
#define VBE_MODE_ACCELERATED_1                  0x1000
#define VBE_MODE_ACCELERATED_2                  0x2000
#define VBE_MODE_LINEAR_FRAMEBUFFER             0x4000
#define VBE_MODE_PRESERVE_DISPLAY               0x8000
#define VBE_MODE_MASK                           ((1 << (VBE_MODE_BITS + 1)) - 1)

#define VBE_MEMORYMODEL_PACKEDPIXEL            0x04
#define VBE_MEMORYMODEL_DIRECTCOLOR            0x06

/*
 * VBE Return Codes
 */

#define VBE_SUCCESS                            0x4F
#define VBE_UNSUCCESSFUL                      0x14F
#define VBE_NOT_SUPPORTED                     0x24F
#define VBE_FUNCTION_INVALID                  0x34F

#define VBE_GETRETURNCODE(x) (x & 0xFFFF)

#include <pshpack1.h>

/*
 * VBE specification defined structure for general adapter info
 * returned by function VBE_GET_CONTROLLER_INFORMATION command.
 */

typedef struct _VBE_CONTROLLER_INFO
{
   ULONG Signature;
   USHORT Version;
   ULONG OemStringPtr;
   LONG Capabilities;
   ULONG VideoModePtr;
   USHORT TotalMemory;
   USHORT OemSoftwareRevision;
   ULONG OemVendorNamePtr;
   ULONG OemProductNamePtr;
   ULONG OemProductRevPtr;
   CHAR Reserved[222];
   CHAR OemData[256];
} VBE_CONTROLLER_INFO, *PVBE_CONTROLLER_INFO;

/*
 * VBE specification defined structure for specific video mode
 * info returned by function VBE_GET_MODE_INFORMATION command.
 */

typedef struct _VBE_MODE_INFO
{
   /* Mandatory information for all VBE revisions */
   USHORT ModeAttributes;
   UCHAR WinAAttributes;
   UCHAR WinBAttributes;
   USHORT WinGranularity;
   USHORT WinSize;
   USHORT WinASegment;
   USHORT WinBSegment;
   ULONG WinFuncPtr;
   USHORT BytesPerScanLine;

   /* Mandatory information for VBE 1.2 and above */
   USHORT XResolution;
   USHORT YResolution;
   UCHAR XCharSize;
   UCHAR YCharSize;
   UCHAR NumberOfPlanes;
   UCHAR BitsPerPixel;
   UCHAR NumberOfBanks;
   UCHAR MemoryModel;
   UCHAR BankSize;
   UCHAR NumberOfImagePages;
   UCHAR Reserved1;

   /* Direct Color fields (required for Direct/6 and YUV/7 memory models) */
   UCHAR RedMaskSize;
   UCHAR RedFieldPosition;
   UCHAR GreenMaskSize;
   UCHAR GreenFieldPosition;
   UCHAR BlueMaskSize;
   UCHAR BlueFieldPosition;
   UCHAR ReservedMaskSize;
   UCHAR ReservedFieldPosition;
   UCHAR DirectColorModeInfo;

   /* Mandatory information for VBE 2.0 and above */
   ULONG PhysBasePtr;
   ULONG Reserved2;
   USHORT Reserved3;

   /* Mandatory information for VBE 3.0 and above */
   USHORT LinBytesPerScanLine;
   UCHAR BnkNumberOfImagePages;
   UCHAR LinNumberOfImagePages;
   UCHAR LinRedMaskSize;
   UCHAR LinRedFieldPosition;
   UCHAR LinGreenMaskSize;
   UCHAR LinGreenFieldPosition;
   UCHAR LinBlueMaskSize;
   UCHAR LinBlueFieldPosition;
   UCHAR LinReservedMaskSize;
   UCHAR LinReservedFieldPosition;
   ULONG MaxPixelClock;

   CHAR Reserved4[190];
} VBE_MODE_INFO, *PVBE_MODE_INFO;

#include <poppack.h>

typedef struct _VBE_INFO
{
    VBE_CONTROLLER_INFO Info;
    VBE_MODE_INFO Modes;
    USHORT ModeArray[129];
} VBE_INFO, *PVBE_INFO;

C_ASSERT(sizeof(VBE_CONTROLLER_INFO) == 0x200);
C_ASSERT(sizeof(VBE_MODE_INFO) == 0x100);

typedef struct _VBE_COLOR_REGISTER
{
    UCHAR Blue;
    UCHAR Green;
    UCHAR Red;
    UCHAR Pad;
} VBE_COLOR_REGISTER, *PVBE_COLOR_REGISTER;

VOID
NTAPI
InitializeModeTable(IN PHW_DEVICE_EXTENSION VgaExtension);

VP_STATUS
NTAPI
VbeSetMode(IN PHW_DEVICE_EXTENSION VgaDeviceExtension,
         IN PVIDEOMODE VgaMode,
         OUT PULONG PhysPtrChange);

VP_STATUS
NTAPI
VbeSetColorLookup(IN PHW_DEVICE_EXTENSION VgaExtension,
            IN PVIDEO_CLUT ClutBuffer);

BOOLEAN
NTAPI
ValidateVbeInfo(IN PHW_DEVICE_EXTENSION VgaExtension,
                IN PVBE_INFO VbeInfo);

extern BOOLEAN g_bIntelBrookdaleBIOS;

/* VBE2 magic number */
#define VBE2_MAGIC ('V' + ('B' << 8) + ('E' << 16) + ('2' << 24))

/* EOF */
