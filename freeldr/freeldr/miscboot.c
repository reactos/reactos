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

	
#include "freeldr.h"
#include "arch.h"
#include "miscboot.h"
#include "rtl.h"
#include "fs.h"
#include "ui.h"
#include "parseini.h"
#include "disk.h"

VOID LoadAndBootBootSector(PUCHAR OperatingSystemName)
{
	PFILE	FilePointer;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	ULONG	SectionId;
	UCHAR	FileName[260];
	ULONG	BytesRead;

	// Find all the message box settings and run them
	ShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!OpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		MessageBox(SettingName);
		return;
	}

	if (!ReadSectionSettingByName(SectionId, "BootDrive", SettingValue, 80))
	{
		MessageBox("Boot drive not specified for selected OS!");
		return;
	}

	BootDrive = atoi(SettingValue);

	BootPartition = 0;
	if (ReadSectionSettingByName(SectionId, "BootPartition", SettingValue, 80))
	{
		BootPartition = atoi(SettingValue);
	}

	if (!ReadSectionSettingByName(SectionId, "BootSectorFile", FileName, 260))
	{
		MessageBox("Boot sector file not specified for selected OS!");
		return;
	}

	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		MessageBox("Failed to open boot drive.");
		return;
	}

	FilePointer = OpenFile(FileName);
	if (FilePointer == NULL)
	{
		strcat(FileName, " not found.");
		MessageBox(FileName);
		return;
	}

	// Read boot sector
	if (!ReadFile(FilePointer, 512, &BytesRead, (void*)0x7c00) || (BytesRead != 512))
	{
		DiskError("Disk read error.");
		return;
	}

	// Check for validity
	if (*((WORD*)(0x7c00 + 0x1fe)) != 0xaa55)
	{
		MessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}

	clrscr();
	showcursor();
	stop_floppy();
	JumpToBootCode();
}

VOID LoadAndBootPartition(PUCHAR OperatingSystemName)
{
	char	name[260];
	char	value[260];
	int		head, sector, cylinder;
	int		offset;
	int		i;

	// Find all the message box settings and run them
	/*for (i=1; i<=GetNumSectionItems(OSList[nOSToBoot].name); i++)
	{
		ReadSectionSettingByNumber(OSList[nOSToBoot].name, i, name, value);
		if (stricmp(name, "MessageBox") == 0)
			MessageBox(value);
		if (stricmp(name, "MessageLine") == 0)
			MessageLine(value);
	}

	if (!ReadSectionSettingByName(OSList[nOSToBoot].name, "BootDrive", value))
	{
		MessageBox("Boot drive not specified for selected OS!");
		return;
	}

	BootDrive = atoi(value);

	if (!ReadSectionSettingByName(OSList[nOSToBoot].name, "BootPartition", value))
	{
		MessageBox("Boot partition not specified for selected OS!");
		return;
	}

	BootPartition = atoi(value);

	if (!BiosInt13Read(BootDrive, 0, 0, 1, 1, DISKREADBUFFER))
	{
		MessageBox("Disk Read Error");
		return;
	}

	// Check for validity
	if (*((WORD*)(DISKREADBUFFER + 0x1fe)) != 0xaa55)
	{
		MessageBox("Invalid partition table magic (0xaa55)");
		return;
	}

	offset = 0x1BE + ((BootPartition-1) * 0x10);

	// Check for valid partition
	if (SectorBuffer[offset + 4] == 0)
	{
		MessageBox("Invalid boot partition");
		return;
	}

	head = SectorBuffer[offset + 1];
	sector = (SectorBuffer[offset + 2] & 0x3F);
	cylinder = SectorBuffer[offset + 3];
	if (SectorBuffer[offset + 2] & 0x80)
		cylinder += 0x200;
	if (SectorBuffer[offset + 2] & 0x40)
		cylinder += 0x100;

	// Read partition boot sector
	if (!biosdisk(_DISK_READ, BootDrive, head, cylinder, sector, 1, (void*)0x7c00))
	{
		MessageBox("Disk Read Error");
		return;
	}

	// Check for validity
	if (*((WORD*)(0x7c00 + 0x1fe)) != 0xaa55)
	{
		MessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}

	RestoreScreen(ScreenBuffer);
	showcursor();
	gotoxy(CursorXPos, CursorYPos);

	stop_floppy();
	JumpToBootCode();*/
}

VOID LoadAndBootDrive(PUCHAR OperatingSystemName)
{
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	ULONG	SectionId;

	// Find all the message box settings and run them
	ShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!OpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		MessageBox(SettingName);
		return;
	}

	if (!ReadSectionSettingByName(SectionId, "BootDrive", SettingValue, 80))
	{
		MessageBox("Boot drive not specified for selected OS!");
		return;
	}

	BootDrive = atoi(SettingValue);

	if (!BiosInt13Read(BootDrive, 0, 0, 1, 1, (PVOID)0x7C00))
	{
		DiskError("Disk read error.");
		return;
	}

	// Check for validity
	if (*((WORD*)(0x7c00 + 0x1fe)) != 0xaa55)
	{
		MessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}

	clrscr();
	showcursor();
	stop_floppy();
	JumpToBootCode();
}
