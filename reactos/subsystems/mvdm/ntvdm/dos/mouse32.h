/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            mouse32.h
 * PURPOSE:         VDM 32-bit compatible MOUSE.COM driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _MOUSE32_H_
#define _MOUSE32_H_

/* DEFINES ********************************************************************/

//
// We are ROS PS/2 Mouse Driver Version 6.26, compatible MS-MOUSE 6.26
//
#define MOUSE_VERSION   MAKEWORD(0x26, 0x06)

#define DOS_MOUSE_INTERRUPT 0x33
#define MOUSE_IRQ_INT       0x74
#define MOUSE_MAX_HORIZ     640
#define MOUSE_MAX_VERT      200

enum
{
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    NUM_MOUSE_BUTTONS
};

typedef struct _MOUSE_USER_HANDLER
{
    /*
     * CallMask format: see table: http://www.ctyme.com/intr/rb-5968.htm#Table3171
     * Alternatively, see table:   http://www.ctyme.com/intr/rb-5981.htm#Table3174
     */
    USHORT CallMask;
    ULONG  Callback; // Far pointer to the callback
} MOUSE_USER_HANDLER, *PMOUSE_USER_HANDLER;

typedef struct _MOUSE_DRIVER_STATE
{
    SHORT ShowCount;
    COORD Position;
    WORD Character;
    WORD ButtonState;
    WORD PressCount[NUM_MOUSE_BUTTONS];
    COORD LastPress[NUM_MOUSE_BUTTONS];
    WORD ReleaseCount[NUM_MOUSE_BUTTONS];
    COORD LastRelease[NUM_MOUSE_BUTTONS];
    SHORT HorizCount;
    SHORT VertCount;
    WORD MinX, MaxX, MinY, MaxY;

    /*
     * User Subroutine Handlers called on mouse events
     */
    MOUSE_USER_HANDLER Handler0;    // Handler  compatible MS MOUSE v1.0+
    MOUSE_USER_HANDLER Handlers[3]; // Handlers compatible MS MOUSE v6.0+

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

VOID DosMouseEnable(VOID);
VOID DosMouseDisable(VOID);
BOOLEAN DosMouseInitialize(VOID);
VOID DosMouseCleanup(VOID);

#endif // _MOUSE32_H_

/* EOF */
