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
	
#include "freeldr.h"
#include "rtl.h"
#include "fs.h"
#include "reactos.h"
#include "ui.h"
#include "asmcode.h"
#include "miscboot.h"
#include "linux.h"
#include "mm.h"
#include "parseini.h"
#include "debug.h"
#include "oslist.h"

// Variable BootDrive moved to asmcode.S
//ULONG			BootDrive = 0;							// BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
ULONG			BootPartition = 0;						// Boot Partition, 1-4

PUCHAR			ScreenBuffer = (PUCHAR)(SCREENBUFFER);	// Save buffer for screen contents
ULONG			CursorXPos = 0;							// Cursor's X Position
ULONG			CursorYPos = 0;							// Cursor's Y Position

ULONG	GetDefaultOperatingSystem(PUCHAR OperatingSystemList[], ULONG OperatingSystemCount);
LONG	GetTimeOut(VOID);

VOID BootMain(VOID)
{
	ULONG	Idx;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	ULONG	SectionId;
	ULONG	OperatingSystemCount;
	PUCHAR	*OperatingSystemSectionNames;
	PUCHAR	*OperatingSystemDisplayNames;
	ULONG	DefaultOperatingSystem;
	LONG	TimeOut;
	ULONG	SelectedOperatingSystem;

	enable_a20();

	CursorXPos = (ULONG) *((PUCHAR)(SCREENXCOORD));
	CursorYPos = (ULONG) *((PUCHAR)(SCREENYCOORD));

#ifdef DEBUG
	DebugInit();
#endif

	InitMemoryManager( (PVOID) 0x20000 /* BaseAddress */, 0x70000 /* Length */);

	if (!ParseIniFile())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	if (!OpenSection("FreeLoader", &SectionId))
	{
		printf("Section [FreeLoader] not found in freeldr.ini.\n");
		getch();
		return;
	}

	if (!InitUserInterface())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	if (!InitOperatingSystemList(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, &OperatingSystemCount))
	{
		MessageBox("Press ENTER to reboot.\n");
		goto reboot;
	}
	
	if (OperatingSystemCount == 0)
	{
		MessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemSectionNames, OperatingSystemCount);
	TimeOut = GetTimeOut();
	
	//
	// Find all the message box settings and run them
	//
	for (Idx=0; Idx<GetNumSectionItems(SectionId); Idx++)
	{
		ReadSectionSettingByNumber(SectionId, Idx, SettingName, 80, SettingValue, 80);
		
		if (stricmp(SettingName, "MessageBox") == 0)
		{
			MessageBox(SettingValue);
		}
		else if (stricmp(SettingName, "MessageLine") == 0)
		{
			MessageLine(SettingValue);
		}
	}

	for (;;)
	{
		if (!DisplayMenu(OperatingSystemDisplayNames, OperatingSystemCount, DefaultOperatingSystem, TimeOut, &SelectedOperatingSystem))
		{
			MessageBox("Press ENTER to reboot.\n");
			return;
		}

		LoadAndBootReactOS(OperatingSystemSectionNames[SelectedOperatingSystem]);

		/*switch (OSList[nOSToBoot].nOSType)
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
		}*/
	}

	
reboot:
	RestoreScreen(ScreenBuffer);
	showcursor();
	gotoxy(CursorXPos, CursorYPos);
	return;
}

ULONG GetDefaultOperatingSystem(PUCHAR OperatingSystemList[], ULONG OperatingSystemCount)
{
	UCHAR	DefaultOSText[80];
	ULONG	SectionId;
	ULONG	DefaultOS = 0;
	ULONG	Idx;

	if (!OpenSection("FreeLoader", &SectionId))
	{
		return 0;
	}

	if (ReadSectionSettingByName(SectionId, "DefaultOS", DefaultOSText, 80))
	{
		for (Idx=0; Idx<OperatingSystemCount; Idx++)
		{
			if (stricmp(DefaultOSText, OperatingSystemList[Idx]) == 0)
			{
				DefaultOS = Idx;
				break;
			}
		}
	}

	return DefaultOS;
}

LONG GetTimeOut(VOID)
{
	UCHAR	TimeOutText[20];
	ULONG	TimeOut;
	ULONG	SectionId;

	if (!OpenSection("FreeLoader", &SectionId))
	{
		return -1;
	}

	if (ReadSectionSettingByName(SectionId, "TimeOut", TimeOutText, 20))
	{
		TimeOut = atoi(TimeOutText);
	}
	else
	{
		TimeOut = -1;
	}

	return TimeOut;
}
