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
#define VIDEO_BIOS_INTERRUPT 0x10
#define VIDEO_KBD_INTERRUPT 0x16
#define CONSOLE_FONT_HEIGHT 8
#define BIOS_KBD_BUFFER_SIZE 256

/* FUNCTIONS ******************************************************************/

BOOLEAN BiosInitialize();
VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress);
VOID BiosUpdateVideoMemory(ULONG StartAddress, ULONG EndAddress);
WORD BiosPeekCharacter();
WORD BiosGetCharacter();
VOID BiosVideoService();
VOID BiosKeyboardService();
VOID BiosHandleIrq(BYTE IrqNumber);

#endif
