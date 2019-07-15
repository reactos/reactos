/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         On screen keyboard.
 * COPYRIGHT:       Denis ROBERT
 *                  Copyright 2019 Bișoc George (fraizeraust99 at gmail dot com)
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
    BOOL       bIsEnhancedKeyboard;
    BOOL       bSoundClick;
    BOOL       bAlwaysOnTop;
    INT        PosX;
    INT        PosY;
} OSK_GLOBALS;

/* DEFINES ********************************************************************/

extern OSK_GLOBALS Globals;

#define countof(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_BUFF 256

#endif

/* EOF */
