/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    menu.h

Abstract:

    This module contains the definitions for console system menu

Author:

    Therese Stowell (thereses) Feb-3-1992 (swiped from Win3.1)

Revision History:

--*/

/*
 * IDs of various STRINGTABLE entries
 *
 */

#define msgBufferTooBig      0x1006
#define msgNoFullScreen      0x1007
#define msgCmdLineF2         0x1008
#define msgCmdLineF4         0x1009
#define msgCmdLineF9         0x100A
#define msgSelectMode        0x100B
#define msgMarkMode          0x100C
#define msgScrollMode        0x100D

/* Menu Item strings */
#define cmCopy               0xFFF0
#define cmPaste              0xFFF1
#define cmMark               0xFFF2
#define cmScroll             0xFFF3
#define cmFind               0xFFF4
#define cmSelectAll          0xFFF5
#define cmEdit               0xFFF6
#define cmControl            0xFFF7
#define cmDefaults           0xFFF8

/*
 * MENU IDs
 *
 */
#define ID_WOMENU           500

/*
 * DIALOG IDs
 */
#define ID_FINDDLG          600
#define ID_FINDSTR          601
#define ID_FINDCASE         602
#define ID_FINDUP           603
#define ID_FINDDOWN         604

