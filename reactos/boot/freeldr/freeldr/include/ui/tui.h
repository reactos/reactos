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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __TUI_H
#define __TUI_H

#define	TUI_SCREEN_MEM				0xB8000
#define TUI_TITLE_BOX_CHAR_HEIGHT	5

///////////////////////////////////////////////////////////////////////////////////////
//
// Textual User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOLEAN	TuiInitialize(VOID);									// Initialize User-Interface
VOID	TuiUnInitialize(VOID);									// Un-initialize User-Interface

VOID	TuiDrawBackdrop(VOID);									// Fills the entire screen with a backdrop
VOID	TuiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr /* Color Attributes */);	// Fills the area specified with FillChar and Attr
VOID	TuiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);	// Draws a shadow on the bottom and right sides of the area specified
VOID	TuiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);	// Draws a box around the area specified
VOID	TuiDrawText(ULONG X, ULONG Y, PCSTR Text, UCHAR Attr);	// Draws text at coordinates specified
VOID	TuiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PCSTR TextString, UCHAR Attr);	// Draws centered text at the coordinates specified and clips the edges
VOID	TuiDrawStatusText(PCSTR StatusText);					// Draws text at the very bottom line on the screen
VOID	TuiUpdateDateTime(VOID);								// Updates the date and time
VOID	TuiSaveScreen(PUCHAR Buffer);							// Saves the screen so that it can be restored later
VOID	TuiRestoreScreen(PUCHAR Buffer);						// Restores the screen from a previous save
VOID	TuiMessageBox(PCSTR MessageText);						// Displays a message box on the screen with an ok button
VOID	TuiMessageBoxCritical(PCSTR MessageText);				// Displays a message box on the screen with an ok button using no system resources
VOID	TuiDrawProgressBarCenter(ULONG Position, ULONG Range, PCHAR ProgressText);			// Draws the progress bar showing nPos percent filled
VOID	TuiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PCHAR ProgressText);			// Draws the progress bar showing nPos percent filled
BOOLEAN	TuiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);

UCHAR	TuiTextToColor(PCSTR ColorText);						// Converts the text color into it's equivalent color value
UCHAR	TuiTextToFillStyle(PCSTR FillStyleText);				// Converts the text fill into it's equivalent fill value

VOID	TuiFadeInBackdrop(VOID);								// Draws the backdrop and fades the screen in
VOID	TuiFadeOut(VOID);										// Fades the screen out

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////

struct tagUI_MENU_INFO
{
	PCSTR		*MenuItemList;
	ULONG		MenuItemCount;
	LONG		MenuTimeRemaining;
	ULONG		SelectedMenuItem;

	ULONG		Left;
	ULONG		Top;
	ULONG		Right;
	ULONG		Bottom;

};

VOID	NTAPI TuiCalcMenuBoxSize(PUI_MENU_INFO MenuInfo);
VOID	TuiDrawMenu(PUI_MENU_INFO MenuInfo);
VOID	NTAPI TuiDrawMenuBox(PUI_MENU_INFO MenuInfo);
VOID	NTAPI TuiDrawMenuItem(PUI_MENU_INFO MenuInfo, ULONG MenuItemNumber);
ULONG	NTAPI TuiProcessMenuKeyboardEvent(PUI_MENU_INFO MenuInfo, UiMenuKeyPressFilterCallback KeyPressFilter);
BOOLEAN TuiDisplayMenu(PCSTR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, ULONG* SelectedMenuItem, BOOLEAN CanEscape, UiMenuKeyPressFilterCallback KeyPressFilter);







/* Definitions for corners, depending on HORIZ and VERT */
#define UL		(0xda)
#define UR		(0xbf)  /* HORZ and VERT */
#define LL		(0xc0)
#define LR		(0xd9)

#define D_UL	(0xc9)
#define D_UR	(0xbb)  /* D_HORZ and D_VERT */
#define D_LL	(0xc8)
#define D_LR	(0xbc)

#define HD_UL	(0xd5)
#define HD_UR	(0xb8)  /* D_HORZ and VERT */
#define HD_LL	(0xd4)
#define HD_LR	(0xbe)

#define VD_UL	(0xd6)
#define VD_UR	(0xb7)  /* HORZ and D_VERT */
#define VD_LL	(0xd3)
#define VD_LR	(0xbd)

extern const UIVTBL TuiVtbl;

#endif // #defined __TUI_H
