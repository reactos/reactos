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
#include "fs.h"
#include "reactos.h"
#include "tui.h"
#include "asmcode.h"
#include "menu.h"
#include "miscboot.h"
#include "linux.h"
#include "memory.h"
#include "parseini.h"
#include "debug.h"

// Variable BootDrive moved to asmcode.S
//ULONG			BootDrive = 0;		// BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
ULONG			BootPartition = 0;	// Boot Partition, 1-4
BOOL			UserInterfaceUp = FALSE; // Tells us if the user interface is displayed

PUCHAR			ScreenBuffer = (PUCHAR)(SCREENBUFFER); // Save buffer for screen contents
int				CursorXPos = 0;	// Cursor's X Position
int				CursorYPos = 0;	// Cursor's Y Position

OSTYPE			OSList[16];
int				nNumOS = 0;

int				nTimeOut = -1;		// Time to wait for the user before booting

void BootMain(void)
{
	int		i;
	char	name[1024];
	char	value[1024];
	int		nOSToBoot;

	enable_a20();

	SaveScreen(ScreenBuffer);
	CursorXPos = wherex();
	CursorYPos = wherey();

	printf("Loading FreeLoader...\n");

#ifdef DEBUG
	DebugInit();
#endif

	InitMemoryManager((PVOID)0x100000, 0x20000);

	if (!ParseIniFile())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	clrscr();
	hidecursor();
	// Draw the backdrop and title box
	DrawBackdrop();
	UserInterfaceUp = TRUE;

	if (nNumOS == 0)
	{
		DrawStatusText(" Press ENTER to reboot");
		MessageBox("Error: there were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		clrscr();
		showcursor();
		RestoreScreen(ScreenBuffer);
		return;
	}
	

	DrawStatusText(" Press ENTER to continue");
	// Find all the message box settings and run them
	for (i=1; i<=GetNumSectionItems("FREELOADER"); i++)
	{
		ReadSectionSettingByNumber("FREELOADER", i, name, value);
		if (stricmp(name, "MessageBox") == 0)
			MessageBox(value);
		if (stricmp(name, "MessageLine") == 0)
			MessageLine(value);
	}

	for (;;)
	{
		nOSToBoot = RunMenu();

		switch (OSList[nOSToBoot].nOSType)
		{
		case OSTYPE_REACTOS:
			LoadAndBootReactOS(OSList[nOSToBoot].name);
			break;
		case OSTYPE_LINUX:
			MessageBox("Cannot boot this OS type yet!");
			break;
		case OSTYPE_BOOTSECTOR:
			LoadAndBootBootSector(nOSToBoot);
			break;
		case OSTYPE_PARTITION:
			LoadAndBootPartition(nOSToBoot);
			break;
		case OSTYPE_DRIVE:
			LoadAndBootDrive(nOSToBoot);
			break;
		}

		DrawBackdrop();
	}

	MessageBox("Press any key to reboot.");
	RestoreScreen(ScreenBuffer);
	showcursor();
	gotoxy(CursorXPos, CursorYPos);
}
