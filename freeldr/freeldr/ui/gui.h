/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GUI_H
#define __GUI_H

#define	TUI_SCREEN_MEM				0xB8000
#define TITLE_BOX_CHAR_HEIGHT		5

///////////////////////////////////////////////////////////////////////////////////////
//
// Graphical User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID	GuiDrawBackdrop(VOID);									// Fills the entire screen with a backdrop
VOID	GuiFillArea(U32 Left, U32 Top, U32 Right, U32 Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */);	// Fills the area specified with FillChar and Attr
VOID	GuiDrawShadow(U32 Left, U32 Top, U32 Right, U32 Bottom);	// Draws a shadow on the bottom and right sides of the area specified
VOID	GuiDrawBox(U32 Left, U32 Top, U32 Right, U32 Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOL Fill, BOOL Shadow, UCHAR Attr);	// Draws a box around the area specified
VOID	GuiDrawText(U32 X, U32 Y, PUCHAR Text, UCHAR Attr);	// Draws text at coordinates specified
VOID	GuiDrawStatusText(PUCHAR StatusText);					// Draws text at the very bottom line on the screen
VOID	GuiUpdateDateTime(VOID);								// Updates the date and time
VOID	GuiSaveScreen(PUCHAR Buffer);							// Saves the screen so that it can be restored later
VOID	GuiRestoreScreen(PUCHAR Buffer);						// Restores the screen from a previous save
VOID	GuiMessageBox(PUCHAR MessageText);						// Displays a message box on the screen with an ok button
VOID	GuiMessageBoxCritical(PUCHAR MessageText);				// Displays a message box on the screen with an ok button using no system resources
VOID	GuiDrawProgressBar(U32 Position, U32 Range);		// Draws the progress bar showing nPos percent filled

UCHAR	GuiTextToColor(PUCHAR ColorText);						// Converts the text color into it's equivalent color value
UCHAR	GuiTextToFillStyle(PUCHAR FillStyleText);				// Converts the text fill into it's equivalent fill value

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL	GuiDisplayMenu(PUCHAR MenuItemList[], U32 MenuItemCount, U32 DefaultMenuItem, S32 MenuTimeOut, U32* SelectedMenuItem);



#endif // #defined __GUI_H
