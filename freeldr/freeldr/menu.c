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
	
#include "freeldr.h"
#include "stdlib.h"
#include "tui.h"
#include "menu.h"
#include "options.h"

static int nOSListBoxLeft;
static int nOSListBoxRight;
static int nOSListBoxTop;
static int nOSListBoxBottom;

static int nOSSelected = 0; // Currently selected OS (zero based)

int RunMenu(void)
{
	int		key;
	int		second;
	BOOL	bDone = FALSE;

	if (nTimeOut > 0)
		nTimeOut++; // Increment the timeout since 0 doesn't count for a second

	// Initialise the menu
	InitMenu();

	// Update the menu
	DrawMenu();

	second = getsecond();

	// Loop
	do
	{
		// Check for a keypress
		if (kbhit())
		{
			// Cancel the timeout
			if (nTimeOut != -1)
			{
				nTimeOut = -1;
				DrawMenu();
			}

			// Get the key
			key = getch();

			// Is it extended?
			if (key == 0)
				key = getch(); // Yes - so get the extended key

			// Process the key
			switch (key)
			{
			case KEY_UP:
				if (nOSSelected)
				{
					nOSSelected--;

					// Update the menu
					DrawMenu();
				}
				break;
			case KEY_DOWN:
				if (nOSSelected < (nNumOS-1))
				{
					nOSSelected++;

					// Update the menu
					DrawMenu();
				}
				break;
			case KEY_ENTER:
				bDone = TRUE;
				break;
			case KEY_F8:
				DoOptionsMenu();
				DrawBackdrop();
				DrawMenu();
				break;
			}
		}

		// Update the date & time
		UpdateDateTime();

		if (nTimeOut > 0)
		{
			if (getsecond() != second)
			{
				second = getsecond();
				nTimeOut--;

				// Update the menu
				DrawMenu();
			}
		}

		if (nTimeOut == 0)
			bDone = TRUE;
	}
	while (!bDone);

	return nOSSelected;
}

void InitMenu(void)
{
	int		i;
	int		height = 1; // Allow room for top & bottom borders
	int		width = 0;

	for(i=0; i<nNumOS; i++)
	{
		height++;
		if(strlen(OSList[i].name) > width)
			width = strlen(OSList[i].name);
	}
	width += 18; // Allow room for left & right borders, plus 8 spaces on each side

	// Calculate the OS list box area
	nOSListBoxLeft = (nScreenWidth - width) / 2;
	nOSListBoxRight = nOSListBoxLeft + width;
	nOSListBoxTop = (nScreenHeight - height) / 2 + 1;
	nOSListBoxBottom = nOSListBoxTop + height;

	nOSSelected = 0;
}

void DrawMenu(void)
{
	int		i, j;
	char	text[260];
	char	temp[260];
	int		space, space_left, space_right;

	// Update the status bar
	DrawStatusText(" Use \x18\x19 to select, ENTER to boot. Press F8 for advanced options.");

	DrawBox(nOSListBoxLeft, nOSListBoxTop, nOSListBoxRight, nOSListBoxBottom, D_VERT, D_HORZ, TRUE, TRUE, ATTR(cMenuFgColor, cMenuBgColor));

	for(i=0; i<nNumOS; i++)
	{
		space = (nOSListBoxRight - nOSListBoxLeft - 2) - strlen(OSList[i].name);
		space_left = (space / 2) + 1;
		space_right = (space - space_left) + 1;

		text[0] = '\0';
		for(j=0; j<space_left; j++)
			strcat(text, " ");
		strcat(text, OSList[i].name);
		for(j=0; j<space_right; j++)
			strcat(text, " ");

		if(i == nOSSelected)
		{
			DrawText(nOSListBoxLeft+1, nOSListBoxTop+1+i, text, ATTR(cSelectedTextColor, cSelectedTextBgColor));
		}
		else
		{
			DrawText(nOSListBoxLeft+1, nOSListBoxTop+1+i, text, ATTR(cTextColor, cMenuBgColor));
		}
	}

	if (nTimeOut >= 0)
	{
		strcpy(text, "[ Time Remaining: ");
		itoa(nTimeOut, temp, 10);
		strcat(text, temp);
		strcat(text, " ]");

		DrawText(nOSListBoxRight - strlen(text) - 1, nOSListBoxBottom, text, ATTR(cMenuFgColor, cMenuBgColor));
	}
}