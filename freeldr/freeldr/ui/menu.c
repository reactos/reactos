/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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
	
#include <freeldr.h>
#include <rtl.h>
#include <ui.h>
#include <options.h>
#include <mm.h>


typedef struct
{
	PUCHAR *MenuItemList;
	ULONG	MenuItemCount;
	LONG	MenuTimeRemaining;
	ULONG	SelectedMenuItem;

	ULONG	Left;
	ULONG	Top;
	ULONG	Right;
	ULONG	Bottom;

} MENU_INFO, *PMENU_INFO;

VOID	CalcMenuBoxSize(PMENU_INFO MenuInfo);
VOID	DrawMenu(PMENU_INFO MenuInfo);
VOID	DrawMenuBox(PMENU_INFO MenuInfo);
VOID	DrawMenuItem(PMENU_INFO MenuInfo, ULONG MenuItemNumber);
ULONG	ProcessMenuKeyboardEvent(PMENU_INFO MenuInfo);

extern	ULONG	nScreenWidth;		// Screen Width
extern	ULONG	nScreenHeight;		// Screen Height

extern	CHAR	cStatusBarFgColor;			// Status bar foreground color
extern	CHAR	cStatusBarBgColor;			// Status bar background color
extern	CHAR	cBackdropFgColor;			// Backdrop foreground color
extern	CHAR	cBackdropBgColor;			// Backdrop background color
extern	CHAR	cBackdropFillStyle;			// Backdrop fill style
extern	CHAR	cTitleBoxFgColor;			// Title box foreground color
extern	CHAR	cTitleBoxBgColor;			// Title box background color
extern	CHAR	cMessageBoxFgColor;			// Message box foreground color
extern	CHAR	cMessageBoxBgColor;			// Message box background color
extern	CHAR	cMenuFgColor;			// Menu foreground color
extern	CHAR	cMenuBgColor;			// Menu background color
extern	CHAR	cTextColor;			// Normal text color
extern	CHAR	cSelectedTextColor;			// Selected text color
extern	CHAR	cSelectedTextBgColor;			// Selected text background color

BOOL DisplayMenu(PUCHAR MenuItemList[], ULONG MenuItemCount, ULONG DefaultMenuItem, LONG MenuTimeOut, PULONG SelectedMenuItem)
{
	PUCHAR		ScreenBuffer;
	MENU_INFO	MenuInformation;
	ULONG		CurrentClockSecond;

	//
	// The first thing we need to check is the timeout
	// If it's zero then don't bother with anything,
	// just return the default item
	//
	if (MenuTimeOut == 0)
	{
		if (SelectedMenuItem != NULL)
		{
			*SelectedMenuItem = DefaultMenuItem;
		}

		return TRUE;
	}

	//
	// Allocate memory to hold screen contents before menu is drawn
	//
	ScreenBuffer = AllocateMemory(4000);
	if (ScreenBuffer == NULL)
	{
		return FALSE;
	}

	//
	// Save screen contents to our buffer
	//
	SaveScreen(ScreenBuffer);

	//
	// Setup the MENU_INFO structure
	//
	MenuInformation.MenuItemList = MenuItemList;
	MenuInformation.MenuItemCount = MenuItemCount;
	MenuInformation.MenuTimeRemaining = MenuTimeOut;
	MenuInformation.SelectedMenuItem = DefaultMenuItem;

	//
	// Calculate the size of the menu box
	//
	CalcMenuBoxSize(&MenuInformation);

	//
	// Draw the menu
	//
	DrawMenu(&MenuInformation);

	//
	// Get the current second of time
	//
	CurrentClockSecond = getsecond();

	//
	// Process keys
	//
	while (1)
	{
		//
		// Process key presses
		//
		if (ProcessMenuKeyboardEvent(&MenuInformation) == KEY_ENTER)
		{
			//
			// If they pressed enter then exit this loop
			//
			break;
		}

		//
		// Update the date & time
		//
		UpdateDateTime();

		if (MenuInformation.MenuTimeRemaining > 0)
		{
			if (getsecond() != CurrentClockSecond)
			{
				//
				// Update the time information
				//
				CurrentClockSecond = getsecond();
				MenuInformation.MenuTimeRemaining--;

				//
				// Update the menu
				//
				DrawMenuBox(&MenuInformation);
			}
		}
		else if (MenuInformation.MenuTimeRemaining == 0)
		{
			//
			// A time out occurred, exit this loop and return default OS
			//
			break;
		}
	}

	//
	// Update the selected menu item information
	//
	if (SelectedMenuItem != NULL)
	{
		*SelectedMenuItem = MenuInformation.SelectedMenuItem;
	}
	
	return TRUE;
}

VOID CalcMenuBoxSize(PMENU_INFO MenuInfo)
{
	ULONG	Idx;
	ULONG	Width;
	ULONG	Height;
	ULONG	Length;

	//
	// Height is the menu item count plus 2 (top border & bottom border)
	//
	Height = MenuInfo->MenuItemCount + 2;
	Height -= 1; // Height is zero-based

	//
	// Find the length of the longest string in the menu
	//
	Width = 0;
	for(Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		Length = strlen(MenuInfo->MenuItemList[Idx]);

		if (Length > Width)
		{
			Width = Length;
		}
	}

	//
	// Allow room for left & right borders, plus 8 spaces on each side
	//
	Width += 18;

	//
	// Calculate the menu box area
	//
	MenuInfo->Left = (nScreenWidth - Width) / 2;
	MenuInfo->Right = (MenuInfo->Left) + Width;
	MenuInfo->Top = (( (nScreenHeight - TITLE_BOX_HEIGHT) - Height) / 2 + 1) + (TITLE_BOX_HEIGHT / 2);
	MenuInfo->Bottom = (MenuInfo->Top) + Height;
}

VOID DrawMenu(PMENU_INFO MenuInfo)
{
	ULONG	Idx;

	//
	// Draw the menu box
	//
	DrawMenuBox(MenuInfo);

	//
	// Draw each line of the menu
	//
	for (Idx=0; Idx<MenuInfo->MenuItemCount; Idx++)
	{
		DrawMenuItem(MenuInfo, Idx);
	}
}

VOID DrawMenuBox(PMENU_INFO MenuInfo)
{
	UCHAR	MenuLineText[80];
	UCHAR	TempString[80];

	//
	// Update the status bar
	//
	DrawStatusText(" Use \x18\x19 to select, ENTER to boot.");

	//
	// Draw the menu box
	//
	DrawBox(MenuInfo->Left,
		MenuInfo->Top,
		MenuInfo->Right,
		MenuInfo->Bottom,
		D_VERT,
		D_HORZ,
		FALSE,		// Filled
		TRUE,		// Shadow
		ATTR(cMenuFgColor, cMenuBgColor));

	//
	// If there is a timeout draw the time remaining
	//
	if (MenuInfo->MenuTimeRemaining >= 0)
	{
		strcpy(MenuLineText, "[ Time Remaining: ");
		itoa(MenuInfo->MenuTimeRemaining, TempString, 10);
		strcat(MenuLineText, TempString);
		strcat(MenuLineText, " ]");

		DrawText(MenuInfo->Right - strlen(MenuLineText) - 1,
			MenuInfo->Bottom,
			MenuLineText,
			ATTR(cMenuFgColor, cMenuBgColor));
	}
}

VOID DrawMenuItem(PMENU_INFO MenuInfo, ULONG MenuItemNumber)
{
	ULONG	Idx;
	UCHAR	MenuLineText[80];
	ULONG	SpaceTotal;
	ULONG	SpaceLeft;
	ULONG	SpaceRight;

	//
	// We will want the string centered so calculate
	// how many spaces will be to the left and right
	//
	SpaceTotal = (MenuInfo->Right - MenuInfo->Left - 2) - strlen(MenuInfo->MenuItemList[MenuItemNumber]);
	SpaceLeft = (SpaceTotal / 2) + 1;
	SpaceRight = (SpaceTotal - SpaceLeft) + 1;

	//
	// Insert the spaces on the left
	//
	for (Idx=0; Idx<SpaceLeft; Idx++)
	{
		MenuLineText[Idx] = ' ';
	}
	MenuLineText[Idx] = '\0';

	//
	// Now append the text string
	//
	strcat(MenuLineText, MenuInfo->MenuItemList[MenuItemNumber]);

	//
	// Now append the spaces on the right
	//
	for (Idx=0; Idx<SpaceRight; Idx++)
	{
		strcat(MenuLineText, " ");
	}

	//
	// If this is the selected menu item then draw it as selected
	// otherwise just draw it using the normal colors
	//
	if (MenuItemNumber == MenuInfo->SelectedMenuItem)
	{
		DrawText(MenuInfo->Left + 1,
			MenuInfo->Top + 1 + MenuItemNumber,
			MenuLineText,
			ATTR(cSelectedTextColor, cSelectedTextBgColor));
	}
	else
	{
		DrawText(MenuInfo->Left + 1,
			MenuInfo->Top + 1 + MenuItemNumber,
			MenuLineText,
			ATTR(cTextColor, cMenuBgColor));
	}
}

ULONG ProcessMenuKeyboardEvent(PMENU_INFO MenuInfo)
{
	ULONG	KeyEvent = 0;

	//
	// Check for a keypress
	//
	if (kbhit())
	{
		//
		// Cancel the timeout
		//
		if (MenuInfo->MenuTimeRemaining != -1)
		{
			MenuInfo->MenuTimeRemaining = -1;
			DrawMenuBox(MenuInfo);
		}

		//
		// Get the key
		//
		KeyEvent = getch();

		//
		// Is it extended?
		//
		if (KeyEvent == 0)
			KeyEvent = getch(); // Yes - so get the extended key

		//
		// Process the key
		//
		switch (KeyEvent)
		{
		case KEY_UP:
			
			if (MenuInfo->SelectedMenuItem > 0)
			{
				MenuInfo->SelectedMenuItem--;

				//
				// Update the menu
				//
				DrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem + 1);	// Deselect previous item
				DrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);		// Select new item
			}

			break;

		case KEY_DOWN:

			if (MenuInfo->SelectedMenuItem < (MenuInfo->MenuItemCount - 1))
			{
				MenuInfo->SelectedMenuItem++;

				//
				// Update the menu
				//
				DrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem - 1);	// Deselect previous item
				DrawMenuItem(MenuInfo, MenuInfo->SelectedMenuItem);		// Select new item
			}

			break;
		}
	}

	return KeyEvent;
}
