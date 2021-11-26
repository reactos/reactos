/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/matrix.c
 * PURPOSE:         LLB Matrix Keypad Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

/* SYNPATICS KEYPAD MATRIX ****************************************************/

UCHAR KeyMatrix[8][8] =
{
    {'e', 'r', 't', KEY_HOME, 0, 0, 'i', KEY_LEFTSHIFT},
    {'d', 'f', 'g', KEY_SEND, 0, 0, 'k', KEY_ENTER},
    {'x', 'c', 'v', KEY_END, 0, 0, '.', KEY_CAPS_LOCK},
    {'z', '+', 'b', KEY_F1, 0, 0, 'o', KEY_SPACE},
    {'w', 'y', 'u', KEY_F2, 0, 0, 'l', KEY_LEFT},
    {'s', 'h', 'j', KEY_F3, 0, 0, 'm', KEY_RIGHT},
    {'q', 'a', 'n', KEY_BACKSPACE, 0, 0, 'p', KEY_UP},
    {0, 0, 0, 0, 0, 0, KEY_ENTER, KEY_DOWN}
};

/* FUNCTIONS ******************************************************************/

CHAR
NTAPI
LlbKeypadGetChar(VOID)
{
    UCHAR ScanCode;
    UCHAR Col, Row;

    ScanCode = LlbHwKbdRead();
    Col = ScanCode >> 4;
    Row = ScanCode & 0xF;

    /* Return the ASCII character */
    return KeyMatrix[Col][Row];
}

/* EOF */
