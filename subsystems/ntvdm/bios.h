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

#define CONSOLE_VIDEO_MEM_END 0xBFFFF
#define ROM_AREA_START 0xE0000
#define ROM_AREA_END 0xFFFFF
#define BDA_SEGMENT 0x40
#define BIOS_PIC_MASTER_INT 0x08
#define BIOS_PIC_SLAVE_INT 0x70
#define BIOS_SEGMENT 0xF000
#define BIOS_VIDEO_INTERRUPT 0x10
#define BIOS_EQUIPMENT_INTERRUPT 0x11
#define BIOS_KBD_INTERRUPT 0x16
#define BIOS_TIME_INTERRUPT 0x1A
#define BIOS_SYS_TIMER_INTERRUPT 0x1C
#define CONSOLE_FONT_HEIGHT 8
#define BIOS_KBD_BUFFER_SIZE 16
#define BIOS_EQUIPMENT_LIST 0x2C // HACK: Disable FPU for now
#define BIOS_DEFAULT_VIDEO_MODE 0x03
#define BIOS_MAX_PAGES 8
#define BIOS_MAX_VIDEO_MODE 0x13

typedef struct
{
    DWORD Width;
    DWORD Height;
    BOOLEAN Text;
    BYTE Bpp;
    BOOLEAN Gray;
    BYTE Pages;
    WORD Segment;
} VIDEO_MODE;

#pragma pack(push, 1)

typedef struct
{
    WORD SerialPorts[4];
    WORD ParallelPorts[3];
    WORD EbdaSegment;
    WORD EquipmentList;
    BYTE Reserved0;
    WORD MemorySize;
    WORD Reserved1;
    WORD KeyboardFlags;
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
} BIOS_DATA_AREA, *PBIOS_DATA_AREA;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

BOOLEAN BiosInitialize(VOID);
VOID BiosCleanup(VOID);
VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress);
VOID BiosUpdateVideoMemory(ULONG StartAddress, ULONG EndAddress);
inline DWORD BiosGetVideoMemoryStart(VOID);
inline VOID BiosVerticalRefresh(VOID);
WORD BiosPeekCharacter(VOID);
WORD BiosGetCharacter(VOID);
VOID BiosVideoService(LPWORD Stack);
VOID BiosEquipmentService(LPWORD Stack);
VOID BiosKeyboardService(LPWORD Stack);
VOID BiosTimeService(LPWORD Stack);
VOID BiosHandleIrq(BYTE IrqNumber, LPWORD Stack);
VOID BiosSystemTimerInterrupt(LPWORD Stack);

#endif
