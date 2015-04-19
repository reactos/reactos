/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            mouse.h
 * PURPOSE:         Mouse emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _MOUSE_H_
#define _MOUSE_H_

/* DEFINES ********************************************************************/

/* Mouse packet constants */
#define MOUSE_MAX 255

/* Mouse packet flags */
#define MOUSE_LEFT_BUTTON   (1 << 0)
#define MOUSE_RIGHT_BUTTON  (1 << 1)
#define MOUSE_MIDDLE_BUTTON (1 << 2)
#define MOUSE_ALWAYS_SET    (1 << 3)
#define MOUSE_X_SIGN        (1 << 4)
#define MOUSE_Y_SIGN        (1 << 5)
#define MOUSE_X_OVERFLOW    (1 << 6)
#define MOUSE_Y_OVERFLOW    (1 << 7)

/* Mouse packet extra flags */
#define MOUSE_4TH_BUTTON    (1 << 4)
#define MOUSE_5TH_BUTTON    (1 << 5)

/* Command responses */
#define MOUSE_BAT_SUCCESS   0xAA
#define MOUSE_ACK           0xFA
#define MOUSE_ERROR         0xFC

/*
 * Scrolling directions
 *
 * It may seem odd that the directions are implemented this way, but
 * this is how it's done on real hardware. It works because the two
 * scroll wheels can't be used at the same time.
 */
#define MOUSE_SCROLL_UP     1
#define MOUSE_SCROLL_DOWN   -1
#define MOUSE_SCROLL_RIGHT  2
#define MOUSE_SCROLL_LEFT   -2

typedef enum _MOUSE_MODE
{
    MOUSE_STREAMING_MODE,
    MOUSE_REMOTE_MODE,
    MOUSE_WRAP_MODE,
} MOUSE_MODE, *PMOUSE_MODE;

typedef struct _MOUSE_PACKET
{
    BYTE Flags;
    BYTE HorzCounter;
    BYTE VertCounter;
    BYTE Extra;
} MOUSE_PACKET, *PMOUSE_PACKET;

/* FUNCTIONS ******************************************************************/

VOID MouseEventHandler(PMOUSE_EVENT_RECORD MouseEvent);
VOID MouseGetDataFast(PCOORD CurrentPosition, PBYTE CurrentButtonState);
BOOLEAN MouseInit(BYTE PS2Connector);

#endif // _MOUSE_H_
