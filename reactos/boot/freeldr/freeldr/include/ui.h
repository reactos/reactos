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

#ifndef __UI_H
#define __UI_H


extern	ULONG		UiScreenWidth;									// Screen Width
extern	ULONG		UiScreenHeight;									// Screen Height

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
extern	UCHAR	UiEditBoxTextColor;								// Edit box text color
extern	UCHAR	UiEditBoxBgColor;								// Edit box text background color

extern	CHAR	UiTitleBoxTitleText[260];						// Title box's title text

extern	BOOLEAN	UserInterfaceUp;								// Tells us if the user interface is displayed

extern	BOOLEAN	UiUseSpecialEffects;							// Tells us if we should use fade effects
extern BOOLEAN UiCenterMenu;
extern BOOLEAN UiMenuBox;
extern BOOLEAN UiMinimal;
extern CHAR UiTimeText[];

extern	const CHAR	UiMonthNames[12][15];

///////////////////////////////////////////////////////////////////////////////////////
//
// User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOLEAN	UiInitialize(BOOLEAN ShowGui);								// Initialize User-Interface
BOOLEAN	SetupUiInitialize(VOID);						// Initialize User-Interface
VOID	UiUnInitialize(PCSTR BootText);						// Un-initialize User-Interface
VOID	UiDrawBackdrop(VOID);									// Fills the entire screen with a backdrop
VOID	UiFillArea(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, CHAR FillChar, UCHAR Attr /* Color Attributes */);	// Fills the area specified with FillChar and Attr
VOID	UiDrawShadow(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom);	// Draws a shadow on the bottom and right sides of the area specified
VOID	UiDrawBox(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, UCHAR VertStyle, UCHAR HorzStyle, BOOLEAN Fill, BOOLEAN Shadow, UCHAR Attr);	// Draws a box around the area specified
VOID	UiDrawText(ULONG X, ULONG Y, PCSTR Text, UCHAR Attr);	// Draws text at coordinates specified
VOID	UiDrawCenteredText(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, PCSTR TextString, UCHAR Attr);	// Draws centered text at the coordinates specified and clips the edges
VOID	UiDrawStatusText(PCSTR StatusText);					// Draws text at the very bottom line on the screen
VOID	UiUpdateDateTime(VOID);									// Updates the date and time
VOID	UiInfoBox(PCSTR MessageText);							// Displays a info box on the screen
VOID	UiMessageBox(PCSTR MessageText);						// Displays a message box on the screen with an ok button
VOID	UiMessageBoxCritical(PCSTR MessageText);				// Displays a message box on the screen with an ok button using no system resources
VOID	UiDrawProgressBarCenter(ULONG Position, ULONG Range, PCHAR ProgressText);			// Draws the progress bar showing nPos percent filled
VOID	UiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PCHAR ProgressText);			// Draws the progress bar showing nPos percent filled
VOID	UiShowMessageBoxesInSection(PCSTR SectionName);		// Displays all the message boxes in a given section
VOID	UiEscapeString(PCHAR String);							// Processes a string and changes all occurances of "\n" to '\n'
BOOLEAN	UiEditBox(PCSTR MessageText, PCHAR EditTextBuffer, ULONG Length);

UCHAR	UiTextToColor(PCSTR ColorText);						// Converts the text color into it's equivalent color value
UCHAR	UiTextToFillStyle(PCSTR FillStyleText);				// Converts the text fill into it's equivalent fill value

VOID	UiTruncateStringEllipsis(PCHAR StringText, ULONG MaxChars);	// Truncates a string to MaxChars by adding an ellipsis on the end '...'

VOID	UiFadeInBackdrop(VOID);									// Draws the backdrop and fades the screen in
VOID	UiFadeOut(VOID);										// Fades the screen out

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////
typedef BOOLEAN	(*UiMenuKeyPressFilterCallback)(ULONG KeyPress);

BOOLEAN	UiDisplayMenu(PCSTR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, ULONG* SelectedMenuItem, BOOLEAN CanEscape, UiMenuKeyPressFilterCallback KeyPressFilter);



#endif // #defined __UI_H
