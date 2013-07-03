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

#define CONSOLE_VIDEO_MEM_START 0xB8000
#define CONSOLE_VIDEO_MEM_END 0xBFFFF
#define ROM_AREA_START 0xC0000
#define ROM_AREA_END 0xFFFFF
#define BIOS_PIC_MASTER_INT 0x08
#define BIOS_PIC_SLAVE_INT 0x70
#define BIOS_SEGMENT 0xF000
#define BIOS_VIDEO_INTERRUPT 0x10
#define BIOS_EQUIPMENT_INTERRUPT 0x11
#define BIOS_KBD_INTERRUPT 0x16
#define BIOS_TIME_INTERRUPT 0x1A
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25
#define CONSOLE_FONT_HEIGHT 8
#define BIOS_KBD_BUFFER_SIZE 256
#define BIOS_EQUIPMENT_LIST 0x3C // HACK: Disable FPU for now

/* FUNCTIONS ******************************************************************/

BOOLEAN BiosInitialize();
VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress);
VOID BiosUpdateVideoMemory(ULONG StartAddress, ULONG EndAddress);
WORD BiosPeekCharacter();
WORD BiosGetCharacter();
VOID BiosVideoService();
VOID BiosEquipmentService();
VOID BiosKeyboardService();
VOID BiosTimeService();
VOID BiosHandleIrq(BYTE IrqNumber);

#endif
