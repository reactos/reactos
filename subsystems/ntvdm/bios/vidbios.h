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

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define BIOS_VIDEO_INTERRUPT    0x10

#define CONSOLE_FONT_HEIGHT     8
#define BIOS_DEFAULT_VIDEO_MODE 0x03
#define BIOS_MAX_PAGES          8
#define BIOS_MAX_VIDEO_MODE     0x13
#define DEFAULT_ATTRIBUTE       0x07

#define GRAPHICS_VIDEO_SEG      0xA000
#define TEXT_VIDEO_SEG          0xB800
#define VIDEO_BIOS_DATA_SEG     0xC000

#define FONT_8x8_OFFSET         0x0000
#define FONT_8x8_HIGH_OFFSET    0x0400
#define FONT_8x16_OFFSET        0x0800
#define FONT_8x14_OFFSET        0x1800

typedef enum
{
    SCROLL_UP,
    SCROLL_DOWN,
    SCROLL_LEFT,
    SCROLL_RIGHT
} SCROLL_DIRECTION;

/* FUNCTIONS ******************************************************************/

VOID VidBiosSyncCursorPosition(VOID);

VOID WINAPI VidBiosVideoService(LPWORD Stack);

VOID VidBiosDetachFromConsole(VOID);
VOID VidBiosAttachToConsole(VOID);

BOOLEAN VidBiosInitialize(VOID);
VOID VidBiosCleanup(VOID);

#endif // _VIDBIOS_H_

/* EOF */
