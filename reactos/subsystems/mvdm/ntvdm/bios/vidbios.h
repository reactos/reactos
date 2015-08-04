/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vidbios.h
 * PURPOSE:         VDM Video BIOS Support Library
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _VIDBIOS_H_
#define _VIDBIOS_H_

/* DEFINES ********************************************************************/

#define BIOS_VIDEO_INTERRUPT    0x10

#define CONSOLE_FONT_HEIGHT     8
#define BIOS_DEFAULT_VIDEO_MODE 0x03
#define BIOS_MAX_PAGES          8
#define BIOS_MAX_VIDEO_MODE     0x13
#define DEFAULT_ATTRIBUTE       0x07

#define GRAPHICS_VIDEO_SEG      0xA000
#define TEXT_VIDEO_SEG          0xB800
#define CGA_EVEN_VIDEO_SEG      0xB800
#define CGA_ODD_VIDEO_SEG       0xBA00
#define VIDEO_BIOS_DATA_SEG     0xC000

#define FONT_8x8_OFFSET         0x0100
#define FONT_8x8_HIGH_OFFSET    0x0500
#define FONT_8x16_OFFSET        0x0900
#define FONT_8x14_OFFSET        0x1900

#define VIDEO_STATE_INFO_OFFSET 0x3000 // == 0x1900 + (sizeof(Font8x14) == 0x0E00) + 0x0900 for padding

#define VIDEO_BIOS_ROM_SIZE     0x4000

typedef enum
{
    SCROLL_UP,
    SCROLL_DOWN,
    SCROLL_LEFT,
    SCROLL_RIGHT
} SCROLL_DIRECTION;

#pragma pack(push, 1)

typedef struct _VGA_STATIC_FUNC_TABLE
{
    BYTE SupportedModes[3];                     // 0x00
    DWORD Reserved0;                            // 0x03
    BYTE SupportedScanlines;                    // 0x07
    BYTE TextCharBlocksNumber;                  // 0x08
    BYTE MaxActiveTextCharBlocksNumber;         // 0x09
    WORD VGAFuncSupportFlags;                   // 0x0a
    WORD Reserved1;                             // 0x0c
    BYTE VGASavePtrFuncFlags;                   // 0x0e
    BYTE Reserved2;                             // 0x0f
} VGA_STATIC_FUNC_TABLE, *PVGA_STATIC_FUNC_TABLE;

typedef struct _VGA_DYNAMIC_FUNC_TABLE
{
    DWORD StaticFuncTablePtr;                   // 0x00

    /*
     * The following fields follow the same order as in the BDA,
     * from offset 0x49 up to offset 0x66...
     */
    BYTE VideoMode;                             // 0x04
    WORD ScreenColumns;                         // 0x05
    WORD VideoPageSize;                         // 0x07
    WORD VideoPageOffset;                       // 0x09
    WORD CursorPosition[BIOS_MAX_PAGES];        // 0x0b
    BYTE CursorEndLine;                         // 0x1b
    BYTE CursorStartLine;                       // 0x1c
    BYTE VideoPage;                             // 0x1d
    WORD CrtBasePort;                           // 0x1e
    BYTE CrtModeControl;                        // 0x20
    BYTE CrtColorPaletteMask;                   // 0x21
    /* ... and offsets 0x84 and 0x85. */
    BYTE ScreenRows;                            // 0x22
    WORD CharacterHeight;                       // 0x23

    BYTE VGADccIDActive;                        // 0x25
    BYTE VGADccIDAlternate;                     // 0x26
    WORD CurrModeSupportedColorsNum;            // 0x27
    BYTE CurrModeSupportedPagesNum;             // 0x29
    BYTE Scanlines;                             // 0x2a
    BYTE PrimaryCharTable;                      // 0x2b
    BYTE SecondaryCharTable;                    // 0x2c

    /* Contains part of information from BDA::VGAFlags (offset 0x89) */
    BYTE VGAFlags;                              // 0x2d

    BYTE Reserved0[3];                          // 0x2e
    BYTE VGAAvailMemory;                        // 0x31
    BYTE VGASavePtrStateFlags;                  // 0x32
    BYTE VGADispInfo;                           // 0x33

    BYTE Reserved1[12];                         // 0x34 - 0x40
} VGA_DYNAMIC_FUNC_TABLE, *PVGA_DYNAMIC_FUNC_TABLE;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

VOID VidBiosSyncCursorPosition(VOID);

VOID WINAPI VidBiosVideoService(LPWORD Stack);

VOID VidBiosDetachFromConsole(VOID);
VOID VidBiosAttachToConsole(VOID);

VOID VidBiosPost(VOID);
BOOLEAN VidBiosInitialize(VOID);
VOID VidBiosCleanup(VOID);

#endif // _VIDBIOS_H_

/* EOF */
