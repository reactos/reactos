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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

ARC_DISK_SIGNATURE reactos_arc_disk_info[32]; // ARC Disk Information
unsigned long reactos_disk_count = 0;
char reactos_arc_hardware_data[HW_MAX_ARC_HEAP_SIZE] = {0};
char reactos_arc_strings[32][256];

ULONG	 GetDefaultOperatingSystem(OperatingSystemItem* OperatingSystemList, ULONG	 OperatingSystemCount)
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
			if (_stricmp(DefaultOSName, OperatingSystemList[Idx].SystemPartition) == 0)
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

VOID RunLoader(VOID)
{
	CHAR	SettingValue[80];
	CHAR BootType[80];
	ULONG_PTR	SectionId;
	ULONG		OperatingSystemCount;
	OperatingSystemItem*	OperatingSystemList;
	PCSTR	*OperatingSystemDisplayNames;
	PCSTR SectionName;
	ULONG	i;
	ULONG		DefaultOperatingSystem;
	LONG		TimeOut;
	ULONG		SelectedOperatingSystem;

	// FIXME: if possible, only detect and register ARC devices...
	if (!MachHwDetect())
	{
		UiMessageBoxCritical("Error when detecting hardware");
		return;
	}

#ifdef _M_IX86
	// Load additional SCSI driver (if any)
	if (LoadBootDeviceDriver() != ESUCCESS)
	{
		UiMessageBoxCritical("Unable to load additional boot device driver");
	}
#endif

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

	if (!UiInitialize(TRUE))
	{
		UiMessageBoxCritical("Unable to initialize UI.");
		return;
	}

	OperatingSystemList = InitOperatingSystemList(&OperatingSystemCount);
	if (!OperatingSystemList)
	{
		UiMessageBox("Unable to read operating systems section in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	if (OperatingSystemCount == 0)
	{
		UiMessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemList, OperatingSystemCount);

	//
	// Create list of display names
	//
	OperatingSystemDisplayNames = MmHeapAlloc(sizeof(PCSTR) * OperatingSystemCount);
	if (!OperatingSystemDisplayNames)
	{
		goto reboot;
	}
	for (i = 0; i < OperatingSystemCount; i++)
	{
		OperatingSystemDisplayNames[i] = OperatingSystemList[i].LoadIdentifier;
	}

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
		SettingValue[0] = ANSI_NULL;
		SectionName = OperatingSystemList[SelectedOperatingSystem].SystemPartition;
		if (IniOpenSection(SectionName, &SectionId))
		{
			// Try to read the boot type
			IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));
		}
		else
			BootType[0] = ANSI_NULL;

		if (BootType[0] == ANSI_NULL && SectionName[0] != ANSI_NULL)
		{
			// Try to infere boot type value
#ifdef _M_IX86
			ULONG FileId;
			if (ArcOpen((CHAR*)SectionName, OpenReadOnly, &FileId) == ESUCCESS)
			{
				ArcClose(FileId);
				strcpy(BootType, "BootSector");
			}
			else
#endif
			{
				strcpy(BootType, "Windows");
			}
		}

		// Get OS setting value
		IniOpenSection("Operating Systems", &SectionId);
		IniReadSettingByName(SectionId, SectionName, SettingValue, sizeof(SettingValue));

		// Install the drive mapper according to this sections drive mappings
#if defined(_M_IX86) && !defined(_MSC_VER)
		DriveMapMapDrivesInSection(SectionName);
#endif

#ifdef FREELDR_REACTOS_SETUP
        // WinLdr-style boot
        LoadReactOSSetup();
#elif defined(_M_IX86)
		if (_stricmp(BootType, "Windows") == 0)
		{
			LoadAndBootWindows(SectionName, SettingValue, 0);
		}
		else if (_stricmp(BootType, "WindowsNT40") == 0)
		{
			LoadAndBootWindows(SectionName, SettingValue, _WIN32_WINNT_NT4);
		}
		else if (_stricmp(BootType, "Windows2003") == 0)
		{
			LoadAndBootWindows(SectionName, SettingValue, _WIN32_WINNT_WS03);
		}
		else if (_stricmp(BootType, "Linux") == 0)
		{
			LoadAndBootLinux(SectionName, OperatingSystemDisplayNames[SelectedOperatingSystem]);
		}
		else if (_stricmp(BootType, "BootSector") == 0)
		{
			LoadAndBootBootSector(SectionName);
		}
		else if (_stricmp(BootType, "Partition") == 0)
		{
			LoadAndBootPartition(SectionName);
		}
		else if (_stricmp(BootType, "Drive") == 0)
		{
			LoadAndBootDrive(SectionName);
		}
#else
		LoadAndBootWindows(SectionName, SettingValue, _WIN32_WINNT_WS03);
#endif
	}

reboot:
	UiUnInitialize("Rebooting...");
	return;
}
