/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            keyboard.h
 * PURPOSE:         Keyboard emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

/* Command responses */
#define KEYBOARD_ACK    0xFA
#define KEYBOARD_RESEND 0xFE

/* FUNCTIONS ******************************************************************/

VOID KeyboardEventHandler(PKEY_EVENT_RECORD KeyEvent);
BOOLEAN KeyboardInit(BYTE PS2Connector);

#endif // _KEYBOARD_H_
