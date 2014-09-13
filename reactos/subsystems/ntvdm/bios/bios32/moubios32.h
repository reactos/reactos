/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            moubios32.h
 * PURPOSE:         VDM Mouse 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _MOUBIOS32_H_
#define _MOUBIOS32_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define BIOS_MOUSE_INTERRUPT 0x33

typedef struct _MOUSE_DRIVER_STATE
{
    SHORT ShowCount;
    COORD Position;
    WORD Character;

    struct
    {
        WORD ScreenMask;
        WORD CursorMask;
    } TextCursor;

    struct
    {
        COORD HotSpot;
        WORD ScreenMask[16];
        WORD CursorMask[16];
    } GraphicsCursor;
} MOUSE_DRIVER_STATE, *PMOUSE_DRIVER_STATE;

/* FUNCTIONS ******************************************************************/

VOID MouseBiosUpdatePosition(PCOORD NewPosition);
BOOLEAN MouseBios32Initialize(VOID);
VOID MouseBios32Cleanup(VOID);

#endif // _MOUBIOS32_H_

/* EOF */
