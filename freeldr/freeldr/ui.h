/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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

#define	SCREEN_MEM				0xB8000
#define TITLE_BOX_HEIGHT		5

// Initialize Textual-User-Interface
BOOL	InitUserInterface(VOID);
// Fills the entire screen with a backdrop
void	DrawBackdrop(void);
// Fills the area specified with cFillChar and cAttr
void	FillArea(int nLeft, int nTop, int nRight, int nBottom, char cFillChar, char cAttr /* Color Attributes */);
// Draws a shadow on the bottom and right sides of the area specified
void	DrawShadow(int nLeft, int nTop, int nRight, int nBottom);
// Draws a box around the area specified
void	DrawBox(int nLeft, int nTop, int nRight, int nBottom, int nVertStyle, int nHorzStyle, int bFill, int bShadow, char cAttr);
// Draws text at coordinates specified
void	DrawText(int nX, int nY, char *text, char cAttr);
// Draws text at the very bottom line on the screen
void	DrawStatusText(char *text);
// Updates the date and time
void	UpdateDateTime(void);
// Saves the screen so that it can be restored later
void	SaveScreen(char *buffer);
// Restores the screen from a previous save
void	RestoreScreen(char *buffer);
// Displays a message box on the screen with an ok button
void	MessageBox(char *text);
// Adds a line of text to the message box buffer
void	MessageLine(char *text);
// Returns true if color is valid
BOOL	IsValidColor(char *color);
// Converts the text color into it's equivalent color value
char	TextToColor(char *color);
// Returns true if fill is valid
BOOL	IsValidFillStyle(char *fill);
// Converts the text fill into it's equivalent fill value
char	TextToFillStyle(char *fill);
// Draws the progress bar showing nPos percent filled
void	DrawProgressBar(int nPos);

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

// Key codes
#define KEY_EXTENDED	0x00
#define	KEY_ENTER		0x0D
#define KEY_SPACE		0x20
#define KEY_UP			0x48
#define KEY_DOWN		0x50
#define KEY_LEFT		0x4B
#define KEY_RIGHT		0x4D
#define KEY_ESC			0x1B
#define KEY_F1			0x3B
#define KEY_F2			0x3C
#define KEY_F3			0x3D
#define KEY_F4			0x3E
#define KEY_F5			0x3F
#define KEY_F6			0x40
#define KEY_F7			0x41
#define KEY_F8			0x42
#define KEY_F9			0x43
#define KEY_F10			0x44


///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL	DisplayMenu(PUCHAR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, PULONG SelectedMenuItem);


#endif // #defined __TUI_H
