/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

/* GENERIC TUI UTILS *********************************************************/

INT
TuiPrintf(
    _In_ PCSTR Format, ...);

VOID
TuiTruncateStringEllipsis(
    _Inout_z_ PSTR StringText,
    _In_ ULONG MaxChars);

#define TUI_TITLE_BOX_CHAR_HEIGHT    5

/* Textual User Interface Functions ******************************************/

BOOLEAN    TuiInitialize(VOID);                                    // Initialize User-Interface
VOID    TuiUnInitialize(VOID);                                    // Un-initialize User-Interface

VOID    TuiDrawBackdrop(VOID);                                    // Fills the entire screen with a backdrop
VOID    TuiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr /* Color Attributes */);    // Fills the area specified with FillChar and Attr
VOID    TuiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);    // Draws a shadow on the bottom and right sides of the area specified

/* Draws a box around the area specified */
VOID
TuiDrawBox(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ UCHAR VertStyle,
    _In_ UCHAR HorzStyle,
    _In_ BOOLEAN Fill,
    _In_ BOOLEAN Shadow,
    _In_ UCHAR Attr);

VOID
TuiDrawBoxTopLine(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ UCHAR VertStyle,
    _In_ UCHAR HorzStyle,
    _In_ UCHAR Attr);

VOID
TuiDrawBoxBottomLine(
    _In_ ULONG Left,
    _In_ ULONG Bottom,
    _In_ ULONG Right,
    _In_ UCHAR VertStyle,
    _In_ UCHAR HorzStyle,
    _In_ UCHAR Attr);

/* Draws text at coordinates specified */
VOID
TuiDrawText(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ PCSTR Text,
    _In_ UCHAR Attr);

/* Draws text at coordinates specified */
VOID
TuiDrawText2(
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_opt_ ULONG MaxNumChars,
    _In_reads_or_z_(MaxNumChars) PCSTR Text,
    _In_ UCHAR Attr);

/* Draws centered text at the coordinates specified and clips the edges */
VOID
TuiDrawCenteredText(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ PCSTR TextString,
    _In_ UCHAR Attr);

VOID    TuiDrawStatusText(PCSTR StatusText);                    // Draws text at the very bottom line on the screen
VOID    TuiUpdateDateTime(VOID);                                // Updates the date and time

/* Saves the screen so that it can be restored later */
_Ret_maybenull_
__drv_allocatesMem(Mem)
PUCHAR
TuiSaveScreen(VOID);

/* Restores the screen from a previous save */
VOID
TuiRestoreScreen(
    _In_opt_ __drv_freesMem(Mem) PUCHAR Buffer);

/* Displays a message box on the screen with an ok button */
VOID
TuiMessageBox(
    _In_ PCSTR MessageText);

/* Displays a message box on the screen with an ok button using no system resources */
VOID
TuiMessageBoxCritical(
    _In_ PCSTR MessageText);

BOOLEAN    TuiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);
UCHAR    TuiTextToColor(PCSTR ColorText);                        // Converts the text color into it's equivalent color value
UCHAR    TuiTextToFillStyle(PCSTR FillStyleText);                // Converts the text fill into it's equivalent fill value

VOID    TuiFadeInBackdrop(VOID);                                // Draws the backdrop and fades the screen in
VOID    TuiFadeOut(VOID);                                        // Fades the screen out

/* Menu Functions ************************************************************/

VOID
TuiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo);

VOID
TuiDrawMenuBox(
    _In_ PUI_MENU_INFO MenuInfo);

VOID
TuiDrawMenuItem(
    _In_ PUI_MENU_INFO MenuInfo,
    _In_ ULONG MenuItemNumber);

BOOLEAN
TuiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL);

/*
 * Definitions for corners, depending on HORZ and VERT
 */
#define UL      0xDA    /* HORZ and VERT */
#define UR      0xBF
#define LL      0xC0
#define LR      0xD9

#define D_UL    0xC9    /* D_HORZ and D_VERT */
#define D_UR    0xBB
#define D_LL    0xC8
#define D_LR    0xBC

#define HD_UL   0xD5    /* D_HORZ and VERT */
#define HD_UR   0xB8
#define HD_LL   0xD4
#define HD_LR   0xBE

#define VD_UL   0xD6    /* HORZ and D_VERT */
#define VD_UR   0xB7
#define VD_LL   0xD3
#define VD_LR   0xBD

extern const UIVTBL TuiVtbl;
