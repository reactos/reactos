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

#ifndef __UI_H
#define __UI_H


extern	ULONG	UiScreenWidth;									// Screen Width
extern	ULONG	UiScreenHeight;									// Screen Height

extern	UCHAR	UiStatusBarFgColor;								// Status bar foreground color
extern	UCHAR	UiStatusBarBgColor;								// Status bar background color
extern	UCHAR	UiBackdropFgColor;								// Backdrop foreground color
extern	UCHAR	UiBackdropBgColor;								// Backdrop background color
extern	UCHAR	UiBackdropFillStyle;							// Backdrop fill style
extern	UCHAR	UiTitleBoxFgColor;								// Title box foreground color
extern	UCHAR	UiTitleBoxBgColor;								// Title box background color
extern	UCHAR	UiMessageBoxFgColor;							// Message box foreground color
extern	UCHAR	UiMessageBoxBgColor;							// Message box background color
extern	UCHAR	UiMenuFgColor;									// Menu foreground color
extern	UCHAR	UiMenuBgColor;									// Menu background color
extern	UCHAR	UiTextColor;									// Normal text color
extern	UCHAR	UiSelectedTextColor;							// Selected text color
extern	UCHAR	UiSelectedTextBgColor;							// Selected text background color
extern	UCHAR	UiTitleBoxTitleText[260];						// Title box's title text

extern	PUCHAR	UiMessageBoxLineText;

extern	BOOL	UserInterfaceUp;								// Tells us if the user interface is displayed

extern	UCHAR	UiMonthNames[12][15];

///////////////////////////////////////////////////////////////////////////////////////
//
// User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL	UiInitialize(VOID);										// Initialize User-Interface
VOID	UiDrawBackdrop(VOID);									// Fills the entire screen with a backdrop
VOID	UiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR FillChar, UCHAR Attr /* Color Attributes */);	// Fills the area specified with FillChar and Attr
VOID	UiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);	// Draws a shadow on the bottom and right sides of the area specified
VOID	UiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOL Fill, BOOL Shadow, UCHAR Attr);	// Draws a box around the area specified
VOID	UiDrawText(ULONG X, ULONG Y, PUCHAR Text, UCHAR Attr);	// Draws text at coordinates specified
VOID	UiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PUCHAR TextString, UCHAR Attr);	// Draws centered text at the coordinates specified and clips the edges
VOID	UiDrawStatusText(PUCHAR StatusText);					// Draws text at the very bottom line on the screen
VOID	UiUpdateDateTime(VOID);									// Updates the date and time
VOID	UiMessageBox(PUCHAR MessageText);						// Displays a message box on the screen with an ok button
VOID	UiMessageBoxCritical(PUCHAR MessageText);				// Displays a message box on the screen with an ok button using no system resources
VOID	UiMessageLine(PUCHAR MessageText);						// Adds a line of text to the message box buffer
VOID	UiDrawProgressBarCenter(ULONG Position, ULONG Range);			// Draws the progress bar showing nPos percent filled
VOID	UiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range);			// Draws the progress bar showing nPos percent filled
VOID	UiShowMessageBoxesInSection(PUCHAR SectionName);		// Displays all the message boxes in a given section

UCHAR	UiTextToColor(PUCHAR ColorText);						// Converts the text color into it's equivalent color value
UCHAR	UiTextToFillStyle(PUCHAR FillStyleText);				// Converts the text fill into it's equivalent fill value

VOID	UiTruncateStringEllipsis(PUCHAR StringText, ULONG MaxChars);	// Truncates a string to MaxChars by adding an ellipsis on the end '...'

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL	UiDisplayMenu(PUCHAR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, PULONG SelectedMenuItem);



#endif // #defined __UI_H
