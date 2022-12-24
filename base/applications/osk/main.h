/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL - See COPYING in the top level directory
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
} OSK_GLOBALS;

/* DEFINES ********************************************************************/

extern OSK_GLOBALS Globals;

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#endif

/* EOF */
