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
#include "asmcode.h"
#include "reactos.h"
#include "stdlib.h"
#include "fs.h"
#include "tui.h"
#include "multiboot.h"
#include "arcname.h"
#include "memory.h"
#include "parseini.h"

BOOL	LoadReactOSKernel(PUCHAR OperatingSystemName);
BOOL	LoadReactOSDrivers(PUCHAR OperatingSystemName);

void LoadAndBootReactOS(PUCHAR OperatingSystemName)
{
	FILE		file;
	char		name[1024];
	char		value[1024];
	char		szFileName[1024];
	char		szBootPath[256];
	int			i;
	int			nNumDriverFiles=0;
	int			nNumFilesLoaded=0;
	char		MsgBuffer[256];

	/*
	 * Setup multiboot information structure
	 */
	mb_info.flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE | MB_INFO_FLAG_COMMAND_LINE | MB_INFO_FLAG_MODULES;
	mb_info.mem_lower = GetConventionalMemorySize();
	mb_info.mem_upper = GetExtendedMemorySize();
	mb_info.boot_device = 0xffffffff;
	mb_info.cmdline = (unsigned long)multiboot_kernel_cmdline;
	mb_info.mods_count = 0;
	mb_info.mods_addr = (unsigned long)multiboot_modules;
	mb_info.mmap_length = GetBiosMemoryMap(&multiboot_memory_map);
	if (mb_info.mmap_length)
	{
		mb_info.mmap_addr = (unsigned long)&multiboot_memory_map;
		mb_info.flags |= MB_INFO_FLAG_MEMORY_MAP;
		//printf("memory map length: %d\n", mb_info.mmap_length);
		//printf("dumping memory map:\n");
		//for (i=0; i<(mb_info.mmap_length / 4); i++)
		//{
		//	printf("0x%x\n", ((unsigned long *)&multiboot_memory_map)[i]);
		//}
		//getch();
	}
	//printf("low_mem = %d\n", mb_info.mem_lower);
	//printf("high_mem = %d\n", mb_info.mem_upper);

	/*
	 * Make sure the system path is set in the .ini file
	 */
	if (!ReadSectionSettingByName(OperatingSystemName, "SystemPath", value))
	{
		MessageBox("System path not specified for selected operating system.");
		return;
	}

	/*
	 * Verify system path
	 */
	if (!DissectArcPath(value, szBootPath, &BootDrive, &BootPartition))
	{
		sprintf(MsgBuffer,"Invalid system path: '%s'", value);
		MessageBox(MsgBuffer);
		return;
	}

	/* set boot drive and partition */
	((char *)(&mb_info.boot_device))[0] = (char)BootDrive;
	((char *)(&mb_info.boot_device))[1] = (char)BootPartition;

	/* copy ARC path into kernel command line */
	strcpy(multiboot_kernel_cmdline, value);

	/*
	 * Read the optional kernel parameters (if any)
	 */
	if (ReadSectionSettingByName(OperatingSystemName, "Options", value))
	{
		strcat(multiboot_kernel_cmdline, " ");
		strcat(multiboot_kernel_cmdline, value);
	}

	/* append a backslash */
	if ((strlen(szBootPath)==0) ||
	    szBootPath[strlen(szBootPath)] != '\\')
		strcat(szBootPath, "\\");

	/*
	 * Find the kernel image name
	 */
	if(!ReadSectionSettingByName(OperatingSystemName, "Kernel", value))
	{
		MessageBox("Kernel image file not specified for selected operating system.");
		return;
	}

	DrawBackdrop();

	DrawStatusText(" Loading...");
	DrawProgressBar(0);

	/*
	 * Try to open boot drive
	 */
	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		MessageBox("Failed to open boot drive.");
		return;
	}

	/*
	 * Parse the ini file and count the kernel and drivers
	 */
	for (i=1; i<=GetNumSectionItems(OperatingSystemName); i++)
	{
		/*
		 * Read the setting and check if it's a driver
		 */
		ReadSectionSettingByNumber(OperatingSystemName, i, name, value);
		if ((stricmp(name, "Kernel") == 0) || (stricmp(name, "Driver") == 0))
			nNumDriverFiles++;
	}

	/*
	 * Find the kernel image name
	 * and try to load the kernel off the disk
	 */
	if(ReadSectionSettingByName(OperatingSystemName, "Kernel", value))
	{
		/*
		 * Set the name and try to open the PE image
		 */
		strcpy(szFileName, szBootPath);
		strcat(szFileName, value);
		if (!OpenFile(szFileName, &file))
		{
			strcat(value, " not found.");
			MessageBox(value);
			return;
		}

		/*
		 * Update the status bar with the current file
		 */
		strcpy(name, " Reading ");
		strcat(name, value);
		while (strlen(name) < 80)
			strcat(name, " ");
		DrawStatusText(name);

		/*
		 * Load the kernel image
		 */
		MultiBootLoadKernel(&file);
		
		nNumFilesLoaded++;
		DrawProgressBar((nNumFilesLoaded * 100) / nNumDriverFiles);
	}

	/*
	 * Parse the ini file and load the kernel and
	 * load all the drivers specified
	 */
	for (i=1; i<=GetNumSectionItems(OperatingSystemName); i++)
	{
		/*
		 * Read the setting and check if it's a driver
		 */
		ReadSectionSettingByNumber(OperatingSystemName, i, name, value);
		if (stricmp(name, "Driver") == 0)
		{
			/*
			 * Set the name and try to open the PE image
			 */
			strcpy(szFileName, szBootPath);
			strcat(szFileName, value);
			if (!OpenFile(szFileName, &file))
			{
				strcat(value, " not found.");
				MessageBox(value);
				return;
			}

			/*
			 * Update the status bar with the current file
			 */
			strcpy(name, " Reading ");
			strcat(name, value);
			while (strlen(name) < 80)
				strcat(name, " ");
			DrawStatusText(name);

			/*
			 * Load the driver
			 */
			MultiBootLoadModule(&file, szFileName);


			nNumFilesLoaded++;
			DrawProgressBar((nNumFilesLoaded * 100) / nNumDriverFiles);
		}
		else if (stricmp(name, "MessageBox") == 0)
		{
			DrawStatusText(" Press ENTER to continue");
			MessageBox(value);
		}
		else if (stricmp(name, "MessageLine") == 0)
			MessageLine(value);
		else if (stricmp(name, "ReOpenBootDrive") == 0)
		{
			if (!OpenDiskDrive(BootDrive, BootPartition))
			{
				MessageBox("Failed to open boot drive.");
				return;
			}
		}
	}

	/*
	 * Clear the screen and redraw the backdrop and status bar
	 */
	DrawBackdrop();
	DrawStatusText(" Press any key to boot");

	/*
	 * Wait for user
	 */
	strcpy(name, "Kernel and Drivers loaded.\nPress any key to boot ");
	strcat(name, OperatingSystemName);
	strcat(name, ".");
	//MessageBox(name);

	RestoreScreen(ScreenBuffer);

	/*
	 * Now boot the kernel
	 */
	stop_floppy();
	boot_reactos();
}

