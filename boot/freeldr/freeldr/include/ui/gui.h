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

#define TITLE_BOX_CHAR_HEIGHT        5

///////////////////////////////////////////////////////////////////////////////////////
//
// Graphical User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID    GuiDrawBackdrop(VOID);                                    // Fills the entire screen with a backdrop
VOID    GuiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */);    // Fills the area specified with FillChar and Attr
VOID    GuiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);    // Draws a shadow on the bottom and right sides of the area specified
VOID    GuiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);    // Draws a box around the area specified
VOID    GuiDrawText(ULONG X, ULONG Y, PUCHAR Text, UCHAR Attr);    // Draws text at coordinates specified
VOID    GuiDrawText2(ULONG X, ULONG Y, ULONG MaxNumChars, PUCHAR Text, UCHAR Attr);    // Draws text at coordinates specified
VOID    GuiDrawStatusText(PCSTR StatusText);                    // Draws text at the very bottom line on the screen
VOID    GuiUpdateDateTime(VOID);                                // Updates the date and time
VOID    GuiSaveScreen(PUCHAR Buffer);                            // Saves the screen so that it can be restored later
VOID    GuiRestoreScreen(PUCHAR Buffer);                        // Restores the screen from a previous save
VOID    GuiMessageBox(PCSTR MessageText);                        // Displays a message box on the screen with an ok button
VOID    GuiMessageBoxCritical(PCSTR MessageText);                // Displays a message box on the screen with an ok button using no system resources
VOID    GuiDrawProgressBar(ULONG Position, ULONG Range);        // Draws the progress bar showing nPos percent filled

UCHAR    GuiTextToColor(PCSTR ColorText);                        // Converts the text color into it's equivalent color value
UCHAR    GuiTextToFillStyle(PCSTR FillStyleText);                // Converts the text fill into it's equivalent fill value

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////

BOOLEAN
GuiDisplayMenu(
    IN PCSTR MenuHeader,
    IN PCSTR MenuFooter OPTIONAL,
    IN BOOLEAN ShowBootOptions,
    IN PCSTR MenuItemList[],
    IN ULONG MenuItemCount,
    IN ULONG DefaultMenuItem,
    IN LONG MenuTimeOut,
    OUT PULONG SelectedMenuItem,
    IN BOOLEAN CanEscape,
    IN UiMenuKeyPressFilterCallback KeyPressFilter OPTIONAL,
    IN PVOID Context OPTIONAL);

extern const UIVTBL GuiVtbl;
