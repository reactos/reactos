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

#ifndef __TUI_H
#define __TUI_H

#define	TUI_SCREEN_MEM				0xB8000
#define TUI_TITLE_BOX_CHAR_HEIGHT	5

///////////////////////////////////////////////////////////////////////////////////////
//
// Textual User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL	TuiInitialize(VOID);									// Initialize User-Interface
VOID	TuiUnInitialize(VOID);									// Un-initialize User-Interface

VOID	TuiDrawBackdrop(VOID);									// Fills the entire screen with a backdrop
VOID	TuiFillArea(U32 Left, U32 Top, U32 Right, U32 Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */);	// Fills the area specified with FillChar and Attr
VOID	TuiDrawShadow(U32 Left, U32 Top, U32 Right, U32 Bottom);	// Draws a shadow on the bottom and right sides of the area specified
VOID	TuiDrawBox(U32 Left, U32 Top, U32 Right, U32 Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOL Fill, BOOL Shadow, UCHAR Attr);	// Draws a box around the area specified
VOID	TuiDrawText(U32 X, U32 Y, PUCHAR Text, UCHAR Attr);	// Draws text at coordinates specified
VOID	TuiDrawCenteredText(U32 Left, U32 Top, U32 Right, U32 Bottom, PUCHAR TextString, UCHAR Attr);	// Draws centered text at the coordinates specified and clips the edges
VOID	TuiDrawStatusText(PUCHAR StatusText);					// Draws text at the very bottom line on the screen
VOID	TuiUpdateDateTime(VOID);								// Updates the date and time
VOID	TuiSaveScreen(PUCHAR Buffer);							// Saves the screen so that it can be restored later
VOID	TuiRestoreScreen(PUCHAR Buffer);						// Restores the screen from a previous save
VOID	TuiMessageBox(PUCHAR MessageText);						// Displays a message box on the screen with an ok button
VOID	TuiMessageBoxCritical(PUCHAR MessageText);				// Displays a message box on the screen with an ok button using no system resources
VOID	TuiDrawProgressBarCenter(U32 Position, U32 Range, PUCHAR ProgressText);			// Draws the progress bar showing nPos percent filled
VOID	TuiDrawProgressBar(U32 Left, U32 Top, U32 Right, U32 Bottom, U32 Position, U32 Range, PUCHAR ProgressText);			// Draws the progress bar showing nPos percent filled

UCHAR	TuiTextToColor(PUCHAR ColorText);						// Converts the text color into it's equivalent color value
UCHAR	TuiTextToFillStyle(PUCHAR FillStyleText);				// Converts the text fill into it's equivalent fill value

VOID	TuiFadeInBackdrop(VOID);								// Draws the backdrop and fades the screen in
VOID	TuiFadeOut(VOID);										// Fades the screen out

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	PUCHAR *MenuItemList;
	U32		MenuItemCount;
	S32		MenuTimeRemaining;
	U32		SelectedMenuItem;

	U32		Left;
	U32		Top;
	U32		Right;
	U32		Bottom;

} TUI_MENU_INFO, *PTUI_MENU_INFO;

VOID	TuiCalcMenuBoxSize(PTUI_MENU_INFO MenuInfo);
VOID	TuiDrawMenu(PTUI_MENU_INFO MenuInfo);
VOID	TuiDrawMenuBox(PTUI_MENU_INFO MenuInfo);
VOID	TuiDrawMenuItem(PTUI_MENU_INFO MenuInfo, U32 MenuItemNumber);
U32		TuiProcessMenuKeyboardEvent(PTUI_MENU_INFO MenuInfo);
BOOL	TuiDisplayMenu(PUCHAR MenuItemList[], U32 MenuItemCount, U32 DefaultMenuItem, S32 MenuTimeOut, U32* SelectedMenuItem);


/*
 * Combines the foreground and background colors into a single attribute byte
 */
#define	ATTR(cFore, cBack)	((cBack << 4)|cFore)

/*
 * Fill styles for DrawBackdrop()
 */
#define LIGHT_FILL			0xB0
#define MEDIUM_FILL			0xB1
#define DARK_FILL			0xB2

/*
 * Screen colors
 */
#define COLOR_BLACK			0
#define COLOR_BLUE			1
#define COLOR_GREEN			2
#define COLOR_CYAN			3
#define COLOR_RED			4
#define COLOR_MAGENTA		5
#define COLOR_BROWN			6
#define COLOR_GRAY			7

#define COLOR_DARKGRAY		8
#define COLOR_LIGHTBLUE		9
#define COLOR_LIGHTGREEN	10
#define COLOR_LIGHTCYAN		11
#define COLOR_LIGHTRED		12
#define COLOR_LIGHTMAGENTA	13
#define COLOR_YELLOW		14
#define COLOR_WHITE			15

/* Add COLOR_BLINK to a background to cause blinking */
#define COLOR_BLINK			8

/*
 * Defines for IBM box drawing characters
 */
#define HORZ	(0xc4)  /* Single horizontal line */
#define D_HORZ	(0xcd)  /* Double horizontal line.*/
#define VERT    (0xb3)  /* Single vertical line   */
#define D_VERT  (0xba)  /* Double vertical line.  */

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


#endif // #defined __TUI_H
