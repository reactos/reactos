/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.h
 * PURPOSE:         VDM BIOS (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _BIOS_H_
#define _BIOS_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define ROM_AREA_START 0xE0000
#define ROM_AREA_END 0xFFFFF
#define BDA_SEGMENT 0x40

#define BIOS_PIC_MASTER_INT 0x08
#define BIOS_PIC_SLAVE_INT  0x70

#define BIOS_SEGMENT 0xF000

#define BIOS_VIDEO_INTERRUPT 0x10
#define BIOS_EQUIPMENT_INTERRUPT 0x11
#define BIOS_MEMORY_SIZE 0x12
#define BIOS_KBD_INTERRUPT 0x16
#define BIOS_TIME_INTERRUPT 0x1A
#define BIOS_SYS_TIMER_INTERRUPT 0x1C

#define CONSOLE_FONT_HEIGHT 8
#define BIOS_KBD_BUFFER_SIZE 16
#define BIOS_EQUIPMENT_LIST 0x2C // HACK: Disable FPU for now
#define BIOS_DEFAULT_VIDEO_MODE 0x03
#define BIOS_MAX_PAGES 8
#define BIOS_PAGE_SIZE 0x1000
#define BIOS_MAX_VIDEO_MODE 0x13
#define DEFAULT_ATTRIBUTE 0x07
#define GRAPHICS_VIDEO_SEG 0xA000
#define TEXT_VIDEO_SEG 0xB800

#define BDA_KBDFLAG_RSHIFT      (1 << 0)
#define BDA_KBDFLAG_LSHIFT      (1 << 1)
#define BDA_KBDFLAG_CTRL        (1 << 2)
#define BDA_KBDFLAG_ALT         (1 << 3)
#define BDA_KBDFLAG_SCROLL_ON   (1 << 4)
#define BDA_KBDFLAG_NUMLOCK_ON  (1 << 5)
#define BDA_KBDFLAG_CAPSLOCK_ON (1 << 6)
#define BDA_KBDFLAG_INSERT_ON   (1 << 7)
#define BDA_KBDFLAG_RALT        (1 << 8)
#define BDA_KBDFLAG_LALT        (1 << 9)
#define BDA_KBDFLAG_SYSRQ       (1 << 10)
#define BDA_KBDFLAG_PAUSE       (1 << 11)
#define BDA_KBDFLAG_SCROLL      (1 << 12)
#define BDA_KBDFLAG_NUMLOCK     (1 << 13)
#define BDA_KBDFLAG_CAPSLOCK    (1 << 14)
#define BDA_KBDFLAG_INSERT      (1 << 15)

enum
{
    SCROLL_DIRECTION_UP,
    SCROLL_DIRECTION_DOWN,
    SCROLL_DIRECTION_LEFT,
    SCROLL_DIRECTION_RIGHT
};

#pragma pack(push, 1)

typedef struct
{
    WORD SerialPorts[4];
    WORD ParallelPorts[3];
    WORD EbdaSegment;       // Sometimes, ParallelPort
    WORD EquipmentList;
    BYTE Reserved0;         // Errors in PCjr infrared keyboard link
    WORD MemorySize;
    WORD Reserved1;         // Scratch pad for manufacturing error tests
    WORD KeybdShiftFlags;
    BYTE AlternateKeypad;
    WORD KeybdBufferHead;
    WORD KeybdBufferTail;
    WORD KeybdBuffer[BIOS_KBD_BUFFER_SIZE];
    BYTE DriveRecalibrate;
    BYTE DriveMotorStatus;
    BYTE MotorShutdownCounter;
    BYTE LastDisketteOperation;
    BYTE Reserved2[7];
    BYTE VideoMode;
    WORD ScreenColumns;
    WORD VideoPageSize;
    WORD VideoPageOffset;
    WORD CursorPosition[BIOS_MAX_PAGES];
    BYTE CursorEndLine;
    BYTE CursorStartLine;
    BYTE VideoPage;
    WORD CrtBasePort;
    BYTE CrtModeControl;
    BYTE CrtColorPaletteMask;
    DWORD Uptime;
    BYTE Reserved3;
    DWORD TickCounter;
    BYTE MidnightPassed;
    BYTE BreakFlag;
    WORD SoftReset;
    BYTE LastDiskOperation;
    BYTE NumDisks;
    BYTE DriveControlByte;
    BYTE DiskPortOffset;
    BYTE LptTimeOut[4];
    BYTE ComTimeOut[4];
    WORD KeybdBufferStart;
    WORD KeybdBufferEnd;
    BYTE ScreenRows;
    WORD CharacterHeight;
} BIOS_DATA_AREA, *PBIOS_DATA_AREA;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

extern PBIOS_DATA_AREA Bda;

BOOLEAN BiosInitialize(VOID);
VOID BiosCleanup(VOID);
BYTE BiosGetVideoMode(VOID);
BOOLEAN BiosSetVideoMode(BYTE ModeNumber);
WORD BiosPeekCharacter(VOID);
WORD BiosGetCharacter(VOID);
VOID BiosSetCursorPosition(BYTE Row, BYTE Column, BYTE Page);
VOID BiosVideoService(LPWORD Stack);
VOID BiosEquipmentService(LPWORD Stack);
VOID BiosGetMemorySize(LPWORD Stack);
VOID BiosKeyboardService(LPWORD Stack);
VOID BiosTimeService(LPWORD Stack);
VOID BiosHandleIrq(BYTE IrqNumber, LPWORD Stack);
VOID BiosSystemTimerInterrupt(LPWORD Stack);
VOID BiosPrintCharacter(CHAR Character, BYTE Attribute, BYTE Page);
BOOLEAN BiosScrollWindow(
    INT Direction,
    DWORD Amount,
    SMALL_RECT Rectangle,
    BYTE Page,
    BYTE FillAttribute
);

#endif // _BIOS_H_

/* EOF */
