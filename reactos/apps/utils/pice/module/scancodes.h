/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    scancodes.h

Abstract:

    HEADER, scancodes of IBM keyboard

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
/*
** Scan Code Definitions . . .
*/
// System Keys
#define SCANCODE_ESC            0x01
#define SCANCODE_BACKSPACE      0x0E
#define SCANCODE_TAB            0x0F

#define SCANCODE_ENTER          0x1C
#define SCANCODE_L_CTRL         0x1D
#define SCANCODE_R_CTRL         0x5A
#define SCANCODE_L_SHIFT        0x2A
#define SCANCODE_R_SHIFT        0x36
#define SCANCODE_L_ALT          0x38
#define SCANCODE_R_ALT          0x5C

#define SCANCODE_SPACE          0x39
#define SCANCODE_CAPS_LOCK      0x3A
#define SCANCODE_NUM_LOCK       0x45
#define SCANCODE_PRNT_SCRN      0x47
#define SCANCODE_SCROLL_LOCK    0x57

// Function Keys
#define SCANCODE_F1             0x3b
#define SCANCODE_F2             0x3c
#define SCANCODE_F3             0x3d
#define SCANCODE_F4             0x3e
#define SCANCODE_F5             0x3f
#define SCANCODE_F6             0x40
#define SCANCODE_F7             0x41
#define SCANCODE_F8             0x42
#define SCANCODE_F9             0x43
#define SCANCODE_F10            0x44
#define SCANCODE_F11            0x57
#define SCANCODE_F12            0x58

// Directional Control Keys
#define SCANCODE_HOME           0x47
#define SCANCODE_UP             0x48
#define SCANCODE_PGUP           0x49
#define SCANCODE_LEFT           0x4b
#define SCANCODE_CENTER         0x4c
#define SCANCODE_RIGHT          0x4d
#define SCANCODE_END            0x4f
#define SCANCODE_DOWN           0x50
#define SCANCODE_PGDN           0x51
#define SCANCODE_INS            0x52
#define SCANCODE_DEL            0x53

// Cluster Directional Control Keys
#define SCANCODE_C_ENTER        0x59
#define SCANCODE_C_HOME         0x5d
#define SCANCODE_C_UP           0x5e
#define SCANCODE_C_PGUP         0x5f
#define SCANCODE_C_LEFT         0x60
#define SCANCODE_C_RIGHT        0x61
#define SCANCODE_C_END          0x62
#define SCANCODE_C_DOWN         0x63
#define SCANCODE_C_PGDN         0x64
#define SCANCODE_C_INS          0x65
#define SCANCODE_C_DEL          0x66


// Alphanumerics
#define SCANCODE_1              0x02
#define SCANCODE_2              0x03
#define SCANCODE_3              0x04
#define SCANCODE_4              0x05
#define SCANCODE_5              0x06
#define SCANCODE_6              0x07
#define SCANCODE_7              0x08
#define SCANCODE_8              0x09
#define SCANCODE_9              0x0A
#define SCANCODE_0              0x0B

#define SCANCODE_EXTENDED       0xE0