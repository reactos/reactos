/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vidbios.h
 * PURPOSE:         VDM Video BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

enum
{
    SCROLL_DIRECTION_UP,
    SCROLL_DIRECTION_DOWN,
    SCROLL_DIRECTION_LEFT,
    SCROLL_DIRECTION_RIGHT
};

/* FUNCTIONS ******************************************************************/

VOID VidBiosPrintCharacter(CHAR Character, BYTE Attribute, BYTE Page);

BOOLEAN VidBiosInitialize(HANDLE BiosConsoleOutput);
VOID VidBiosCleanup(VOID);

#endif // _VIDBIOS_H_

/* EOF */
