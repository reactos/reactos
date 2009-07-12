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

#include <freeldr.h>

VOID RunLoader(VOID)
{
	CHAR	SettingName[80];
	CHAR	SettingValue[80];
	ULONG_PTR	SectionId;
	ULONG		OperatingSystemCount;
	PCSTR	*OperatingSystemSectionNames;
	PCSTR	*OperatingSystemDisplayNames;
	ULONG		DefaultOperatingSystem;
	LONG		TimeOut;
	ULONG		SelectedOperatingSystem;

	if (!FsOpenBootVolume())
	{
		UiMessageBoxCritical("Error opening boot partition for file access.");
		return;
	}

	if (!IniFileInitialize())
	{
		UiMessageBoxCritical("Error initializing .ini file");
		return;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		UiMessageBoxCritical("Section [FreeLoader] not found in freeldr.ini.");
		return;
	}
	TimeOut = GetTimeOut();

	if (!UiInitialize(TimeOut))
	{
		UiMessageBoxCritical("Unable to initialize UI.");
		return;
	}


	if (!InitOperatingSystemList(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, &OperatingSystemCount))
	{
		UiMessageBox("Press ENTER to reboot.");
		goto reboot;
	}

	if (OperatingSystemCount == 0)
	{
		UiMessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemSectionNames, OperatingSystemCount);

	//
	// Find all the message box settings and run them
	//
	UiShowMessageBoxesInSection("FreeLoader");

	for (;;)
	{

		// Redraw the backdrop
		UiDrawBackdrop();

		// Show the operating system list menu
		if (!UiDisplayMenu(OperatingSystemDisplayNames, OperatingSystemCount, DefaultOperatingSystem, TimeOut, &SelectedOperatingSystem, FALSE, MainBootMenuKeyPressFilter))
		{
			UiMessageBox("Press ENTER to reboot.");
			goto reboot;
		}

		TimeOut = -1;

		// Try to open the operating system section in the .ini file
		if (!IniOpenSection(OperatingSystemSectionNames[SelectedOperatingSystem], &SectionId))
		{
			sprintf(SettingName, "Section [%s] not found in freeldr.ini.", OperatingSystemSectionNames[SelectedOperatingSystem]);
			UiMessageBox(SettingName);
			continue;
		}

		// Try to read the boot type
		if (!IniReadSettingByName(SectionId, "BootType", SettingValue, sizeof(SettingValue)))
		{
			sprintf(SettingName, "BootType= line not found in section [%s] in freeldr.ini.", OperatingSystemSectionNames[SelectedOperatingSystem]);
			UiMessageBox(SettingName);
			continue;
		}

		// Install the drive mapper according to this sections drive mappings
#ifdef __i386__
		DriveMapMapDrivesInSection(OperatingSystemSectionNames[SelectedOperatingSystem]);
#endif
		if (_stricmp(SettingValue, "ReactOS") == 0)
		{
			LoadAndBootReactOS(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
#ifdef FREELDR_REACTOS_SETUP
		else if (_stricmp(SettingValue, "ReactOSSetup") == 0)
		{
			// In future we could pass the selected OS details through this
			// to have different install methods, etc.
			LoadReactOSSetup();
		}
#ifdef __i386__
		else if (_stricmp(SettingValue, "ReactOSSetup2") == 0)
		{
			// WinLdr-style boot
			LoadReactOSSetup2();
		}
#endif
#endif
#ifdef __i386__
		else if (_stricmp(SettingValue, "WindowsNT40") == 0)
		{
			LoadAndBootWindows(OperatingSystemSectionNames[SelectedOperatingSystem], _WIN32_WINNT_NT4);
		}
		else if (_stricmp(SettingValue, "Windows2003") == 0)
		{
			LoadAndBootWindows(OperatingSystemSectionNames[SelectedOperatingSystem], _WIN32_WINNT_WS03);
		}
		else if (_stricmp(SettingValue, "Linux") == 0)
		{
			LoadAndBootLinux(OperatingSystemSectionNames[SelectedOperatingSystem], OperatingSystemDisplayNames[SelectedOperatingSystem]);
		}
		else if (_stricmp(SettingValue, "BootSector") == 0)
		{
			LoadAndBootBootSector(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (_stricmp(SettingValue, "Partition") == 0)
		{
			LoadAndBootPartition(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
		else if (_stricmp(SettingValue, "Drive") == 0)
		{
			LoadAndBootDrive(OperatingSystemSectionNames[SelectedOperatingSystem]);
		}
#endif
	}


reboot:
	UiUnInitialize("Rebooting...");
	return;
}

ULONG	 GetDefaultOperatingSystem(PCSTR OperatingSystemList[], ULONG	 OperatingSystemCount)
{
	CHAR	DefaultOSText[80];
	PCSTR	DefaultOSName;
	ULONG_PTR	SectionId;
	ULONG	DefaultOS = 0;
	ULONG	Idx;

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return 0;
	}

	DefaultOSName = CmdLineGetDefaultOS();
	if (NULL == DefaultOSName)
	{
		if (IniReadSettingByName(SectionId, "DefaultOS", DefaultOSText, sizeof(DefaultOSText)))
		{
			DefaultOSName = DefaultOSText;
		}
	}

	if (NULL != DefaultOSName)
	{
		for (Idx=0; Idx<OperatingSystemCount; Idx++)
		{
			if (_stricmp(DefaultOSName, OperatingSystemList[Idx]) == 0)
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
	CHAR	TimeOutText[20];
	LONG		TimeOut;
	ULONG_PTR	SectionId;

	TimeOut = CmdLineGetTimeOut();
	if (0 <= TimeOut)
	{
		return TimeOut;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return -1;
	}

	if (IniReadSettingByName(SectionId, "TimeOut", TimeOutText, sizeof(TimeOutText)))
	{
		TimeOut = atoi(TimeOutText);
	}
	else
	{
		TimeOut = -1;
	}

	return TimeOut;
}

BOOLEAN MainBootMenuKeyPressFilter(ULONG KeyPress)
{
	if (KeyPress == KEY_F8)
	{
		DoOptionsMenu();

		return TRUE;
	}

	// We didn't handle the key
	return FALSE;
}
