/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Key codes header file
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer (brianp@reactos.org)
 *              Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#if defined(SARCH_PC98)
#define KEY_EXTENDED    0x00
#define KEY_ENTER       0x0D
#define KEY_BACKSPACE   0x08
#define KEY_DELETE      0x39
#define KEY_SPACE       0x20
#define KEY_LEFTSHIFT   0x70
#define KEY_HOME        0x3E
#define KEY_UP          0x3A
#define KEY_DOWN        0x3D
#define KEY_LEFT        0x3B
#define KEY_RIGHT       0x3C
#define KEY_ESC         0x1B
#define KEY_CAPS_LOCK   0x71
#define KEY_F1          0x62
#define KEY_F2          0x63
#define KEY_F3          0x64
#define KEY_F4          0x65
#define KEY_F5          0x66
#define KEY_F6          0x67
#define KEY_F7          0x68
#define KEY_F8          0x69
#define KEY_F9          0x6A
#define KEY_F10         0x6B
#define KEY_KEYPAD_PLUS 0x2B
#define KEY_END         0x3F
#else /* SARCH_PC98 */
#define KEY_EXTENDED    0x00
#define KEY_ENTER       0x0D
#define KEY_BACKSPACE   0x08
#define KEY_DELETE      0x53
#define KEY_SPACE       0x20
#define KEY_LEFTSHIFT   0x2A
#define KEY_HOME        0x47
#define KEY_UP          0x48
#define KEY_DOWN        0x50
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_ESC         0x1B
#define KEY_CAPS_LOCK   0x3A
#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_F5          0x3F
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44
#define KEY_F11         0x57
#define KEY_F12         0x58
#define KEY_KEYPAD_PLUS 0x4E
#define KEY_END         0x4F
#define KEY_SEND        0xE7
#endif
