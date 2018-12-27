/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/osk/main.h
 * PURPOSE:         On screen keyboard.
 * PROGRAMMERS:     Denis ROBERT
 */

#ifndef _OSKMAIN_H
#define _OSKMAIN_H

/* INCLUDES *******************************************************************/

#include "osk_res.h"

/* TYPES **********************************************************************/

typedef struct
{
    HINSTANCE  hInstance;
    HWND       hMainWnd;
    HBRUSH     hBrushGreenLed;
    UINT_PTR   iTimer;
    /* FIXME: To be deleted when ReactOS will support WS_EX_NOACTIVATE */
    HWND       hActiveWnd;
    BOOL       bShowWarning;
} OSK_GLOBALS;

/* DEFINES ********************************************************************/

extern OSK_GLOBALS Globals;

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#endif

/* EOF */
