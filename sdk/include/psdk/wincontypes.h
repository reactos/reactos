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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WINCONTYPES_
#define _WINCONTYPES_

typedef struct tagCOORD
{
    SHORT X;
    SHORT Y;
} COORD, *LPCOORD;

typedef struct tagSMALL_RECT
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT,*LPSMALL_RECT;

typedef struct tagKEY_EVENT_RECORD
{
    BOOL  bKeyDown;              /* 04 */
    WORD  wRepeatCount;          /* 08 */
    WORD  wVirtualKeyCode;       /* 0A */
    WORD  wVirtualScanCode;      /* 0C */
    union                        /* 0E */
    {
        WCHAR UnicodeChar;       /* 0E */
        CHAR AsciiChar;          /* 0E */
    } uChar;
    DWORD dwControlKeyState;     /* 10 */
} KEY_EVENT_RECORD,*LPKEY_EVENT_RECORD;

/* dwControlKeyState bitmask */
#define RIGHT_ALT_PRESSED       0x0001
#define LEFT_ALT_PRESSED        0x0002
#define RIGHT_CTRL_PRESSED      0x0004
#define LEFT_CTRL_PRESSED       0x0008
#define SHIFT_PRESSED           0x0010
#define NUMLOCK_ON              0x0020
#define SCROLLLOCK_ON           0x0040
#define CAPSLOCK_ON             0x0080
#define ENHANCED_KEY            0x0100

typedef struct tagMOUSE_EVENT_RECORD
{
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
} MOUSE_EVENT_RECORD,*LPMOUSE_EVENT_RECORD;

/* MOUSE_EVENT_RECORD.dwButtonState */
#define FROM_LEFT_1ST_BUTTON_PRESSED    0x0001
#define RIGHTMOST_BUTTON_PRESSED        0x0002
#define FROM_LEFT_2ND_BUTTON_PRESSED    0x0004
#define FROM_LEFT_3RD_BUTTON_PRESSED    0x0008
#define FROM_LEFT_4TH_BUTTON_PRESSED    0x0010

/* MOUSE_EVENT_RECORD.dwEventFlags */
#define MOUSE_MOVED                     0x0001
#define DOUBLE_CLICK                    0x0002
#define MOUSE_WHEELED                   0x0004
#define MOUSE_HWHEELED                  0x0008

typedef struct tagWINDOW_BUFFER_SIZE_RECORD
{
    COORD dwSize;
} WINDOW_BUFFER_SIZE_RECORD,*LPWINDOW_BUFFER_SIZE_RECORD;

typedef struct tagMENU_EVENT_RECORD
{
    UINT dwCommandId;
} MENU_EVENT_RECORD,*LPMENU_EVENT_RECORD;

typedef struct tagFOCUS_EVENT_RECORD
{
    BOOL      bSetFocus;
} FOCUS_EVENT_RECORD,*LPFOCUS_EVENT_RECORD;

typedef struct tagINPUT_RECORD
{
    WORD EventType;
    union
    {
        KEY_EVENT_RECORD KeyEvent;
        MOUSE_EVENT_RECORD MouseEvent;
        WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent;
    } Event;
} INPUT_RECORD,*PINPUT_RECORD;

/* INPUT_RECORD.wEventType */
#define KEY_EVENT                 0x01
#define MOUSE_EVENT               0x02
#define WINDOW_BUFFER_SIZE_EVENT  0x04
#define MENU_EVENT                0x08
#define FOCUS_EVENT               0x10

typedef struct tagCHAR_INFO
{
    union
    {
        WCHAR UnicodeChar;
        CHAR AsciiChar;
    } Char;
    WORD Attributes;
} CHAR_INFO,*LPCHAR_INFO;

typedef struct _CONSOLE_FONT_INFO
{
    DWORD       nFont;
    COORD       dwFontSize;
} CONSOLE_FONT_INFO,*LPCONSOLE_FONT_INFO;

typedef void *HPCON;

#endif /* _WINCONTYPES_ */
