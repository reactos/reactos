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
#include "asmcode.h"
#include "miscboot.h"
#include "rtl.h"
#include "fs.h"
#include "ui.h"
#include "linux.h"

void LoadAndBootLinux(int DriveNum, int Partition, char *vmlinuz, char *cmd_line)
{
	/*FILE	file;
	char	temp[260];
	char	bootsector[512];
	char	setup[2048];
	int		len;

	BootDrive = DriveNum;
	BootPartition = Partition;

	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		MessageBox("Failed to open boot drive.");
		return;
	}

	if (!OpenFile(vmlinuz, &file))
	{
		strcpy(temp, vmlinuz);
		strcat(temp, " not found.");
		MessageBox(temp);
		return;
	}

	// Read boot sector
	if (ReadFile(&file, 512, bootsector) != 512)
	{
		MessageBox("Disk Read Error");
		return;
	}
	MessageBox("bootsector loaded");

	// Read setup code
	if (ReadFile(&file, 2048, setup) != 2048)
	{
		MessageBox("Disk Read Error");
		return;
	}
	MessageBox("setup loaded");

	// Read kernel code
	len = GetFileSize(&file) - (2048 + 512);
	//len = 0x200;
	if (ReadFile(&file, len, (void*)0x100000) != len)
	{
		MessageBox("Disk Read Error");
		return;
	}
	MessageBox("kernel loaded");

	// Check for validity
	if (*((WORD*)(bootsector + 0x1fe)) != 0xaa55)
	{
		MessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}
	if (*((DWORD*)(setup + 2)) != 0x53726448)
	{
		MessageBox("Invalid setup magic (\"HdrS\")");
		return;
	}

	memcpy((void*)0x90000, bootsector, 512);
	memcpy((void*)0x90200, setup, 2048);

	RestoreScreen(ScreenBuffer);
	showcursor();
	gotoxy(CursorXPos, CursorYPos);

	stop_floppy();
	JumpToLinuxBootCode();*/
}
