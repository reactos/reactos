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
	
#include <freeldr.h>
#include <rtl.h>
#include <fs.h>
#include <reactos.h>
#include <ui.h>
#include <arch.h>
#include <miscboot.h>
#include <linux.h>
#include <mm.h>
#include <inifile.h>
#include <debug.h>
#include <oslist.h>
#include <video.h>
#include <bootmgr.h>
#include <drivemap.h>

VOID RunBootManager(VOID)
{
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	U32		SectionId;
	U32		OperatingSystemCount;
	PUCHAR	*OperatingSystemSectionNames;
	PUCHAR	*OperatingSystemDisplayNames;
	U32		DefaultOperatingSystem;
	S32		TimeOut;
	U32		SelectedOperatingSystem;

	if (!IniFileInitialize())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		printf("Section [FreeLoader] not found in freeldr.ini.\n");
		getch();
		return;
	}

	if (!UiInitialize())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	if (!InitOperatingSystemList(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, &OperatingSystemCount))
	{
		UiMessageBox("Press ENTER to reboot.\n");
		goto reboot;
	}
	
	if (OperatingSystemCount == 0)
	{
		UiMessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemSectionNames, OperatingSystemCount);
	TimeOut = GetTimeOut();
	
	//
	// Find all the message box settings and run them
	//
	UiShowMessageBoxesInSection("FreeLoader");

	for (;;)
	{
		// Redraw the backdrop
		UiDrawBackdrop();

		// Show the operating system list menu
		if (!UiDisplayMenu(OperatingSystemDisplayNames, OperatingSystemCount, DefaultOperatingSystem, TimeOut, &SelectedOperatingSystem))
		{
			UiMessageBox("Press ENTER to reboot.\n");
			goto reboot;
		}
		TimeOut = -1;
		DefaultOperatingSystem = SelectedOperatingSystem;

		// Try to open the operating system section in the .ini file
		if (!IniOpenSection(OperatingSystemSectionNames[SelectedOperatingSystem], &SectionId))
		{
			sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemSectionNames[SelectedOperatingSystem]);
			UiMessageBox(SettingName);
			continue;
		}

		// Try to read the boot type
		if (!IniReadSettingByName(SectionId, "BootType", SettingValue, 80))
		{
			sprintf(SettingName, "BootType= line not found in section [%s] in freeldr.ini.\n", OperatingSystemSectionNames[SelectedOperatingSystem]);
			UiMessageBox(SettingName);
			continue;
		}

		// Install the drive mapper according to this sections drive mappings
		DriveMapMapDrivesInSection(OperatingSystemSectionNames[SelectedOperatingSystem]);

		if (stricmp(SettingValue, "ReactOS") == 0)
		{
			LoadAndBootReactOS(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "Linux") == 0)
		{
			LoadAndBootLinux(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "BootSector") == 0)
		{
			LoadAndBootBootSector(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "Partition") == 0)
		{
			LoadAndBootPartition(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (stricmp(SettingValue, "Drive") == 0)
		{
			LoadAndBootDrive(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
	}

	
reboot:
	VideoClearScreen();
	VideoShowTextCursor();
	return;
}

U32	 GetDefaultOperatingSystem(PUCHAR OperatingSystemList[], U32	 OperatingSystemCount)
{
	UCHAR	DefaultOSText[80];
	U32		SectionId;
	U32		DefaultOS = 0;
	U32		Idx;

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return 0;
	}

	if (IniReadSettingByName(SectionId, "DefaultOS", DefaultOSText, 80))
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

S32 GetTimeOut(VOID)
{
	UCHAR	TimeOutText[20];
	U32		TimeOut;
	U32		SectionId;

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return -1;
	}

	if (IniReadSettingByName(SectionId, "TimeOut", TimeOutText, 20))
	{
		TimeOut = atoi(TimeOutText);
	}
	else
	{
		TimeOut = -1;
	}

	return TimeOut;
}
