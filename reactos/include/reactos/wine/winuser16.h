/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_WINUSER16_H
#define __WINE_WINE_WINUSER16_H

#include <wine/wingdi16.h> /* wingdi.h needed for COLORREF */
#include <winuser.h> /* winuser.h needed for MSGBOXCALLBACK */

#ifndef CB_SETITEMDATA16
#define CB_SETITEMDATA16           (WM_USER+17)
#endif

#include <pshpack1.h>

/* Cursors / Icons */

typedef struct tagCURSORICONINFO
{
    POINT16 ptHotSpot;
    WORD    nWidth;
    WORD    nHeight;
    WORD    nWidthBytes;
    BYTE    bPlanes;
    BYTE    bBitsPerPixel;
} CURSORICONINFO;

typedef struct
{
    BYTE   fVirt;
    WORD   key;
    WORD   cmd;
} ACCEL16, *LPACCEL16;

  /* WM_NCCALCSIZE parameter structure */
typedef struct
{
    RECT16  rgrc[3];
    SEGPTR  lppos;
} NCCALCSIZE_PARAMS16, *LPNCCALCSIZE_PARAMS16;

typedef struct {
	UINT16		cbSize;
	INT16		iBorderWidth;
	INT16		iScrollWidth;
	INT16		iScrollHeight;
	INT16		iCaptionWidth;
	INT16		iCaptionHeight;
	LOGFONT16	lfCaptionFont;
	INT16		iSmCaptionWidth;
	INT16		iSmCaptionHeight;
	LOGFONT16	lfSmCaptionFont;
	INT16		iMenuWidth;
	INT16		iMenuHeight;
	LOGFONT16	lfMenuFont;
	LOGFONT16	lfStatusFont;
	LOGFONT16	lfMessageFont;
} NONCLIENTMETRICS16,*LPNONCLIENTMETRICS16;

/* CreateWindow() coordinates */
#define CW_USEDEFAULT16 ((INT16)0x8000)

/* Edit control messages */
#define EM_GETTHUMB16              (WM_USER+14)

#define DRAG_FILE  0x454C4946

#include <poppack.h>

HGLOBAL16   WINAPI CreateCursorIconIndirect16(HINSTANCE16,CURSORICONINFO*,
                                            LPCVOID,LPCVOID);


#endif /* __WINE_WINE_WINUSER16_H */
