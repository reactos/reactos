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
#include "rosboot.h"
#include "stdlib.h"
#include "fs.h"
#include "tui.h"
#include "multiboot.h"

unsigned long				next_module_load_base = 0;

void LoadAndBootReactOS(char *OperatingSystemName)
{
	FILE		file;
	char		name[1024];
	char		value[1024];
	char		szFileName[1024];
	int			i;
	int			nNumDriverFiles=0;
	int			nNumFilesLoaded=0;

	/*
	 * Setup multiboot information structure
	 */
	mb_info.flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE | MB_INFO_FLAG_COMMAND_LINE | MB_INFO_FLAG_MODULES;
	mb_info.mem_lower = 640;//(640 * 1024);
	mb_info.mem_upper = (8 * 1024);
	mb_info.boot_device = 0xffffffff;
	mb_info.cmdline = (unsigned long)multiboot_kernel_cmdline;
	mb_info.mods_count = 0;
	mb_info.mods_addr = (unsigned long)multiboot_modules;
	mb_info.mmap_length = 0;
	mb_info.mmap_addr = 0;

	/*
	 * Read the optional kernel parameters (if any)
	 */
	ReadSectionSettingByName(OperatingSystemName, "Options", name, multiboot_kernel_cmdline);

	/*
	 * Find the kernel image name
	 */
	if(!ReadSectionSettingByName(OperatingSystemName, "Kernel", name, value))
	{
		MessageBox("Kernel image file not specified for selected operating system.");
		return;
	}

	/*
	 * Get the boot partition
	 */
	BootPartition = 0;
	if (ReadSectionSettingByName(OperatingSystemName, "BootPartition", name, value))
	{
		BootPartition = atoi(value);
	}
	((char *)(&mb_info.boot_device))[1] = (char)BootPartition;
	
	/*
	 * Make sure the boot drive is set in the .ini file
	 */
	if(!ReadSectionSettingByName(OperatingSystemName, "BootDrive", name, value))
	{
		MessageBox("Boot drive not specified for selected operating system.");
		return;
	}

	DrawBackdrop();

	DrawStatusText(" Loading...");
	DrawProgressBar(0);

	/*
	 * Set the boot drive and try to open it
	 */
	BootDrive = atoi(value);
	((char *)(&mb_info.boot_device))[0] = (char)BootDrive;
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
	if(ReadSectionSettingByName(OperatingSystemName, "Kernel", name, value))
	{
		/*
		 * Set the name and try to open the PE image
		 */
		strcpy(szFileName, value);
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
			strcpy(szFileName, value);
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

	RestoreScreen(pScreenBuffer);

	/*
	 * Now boot the kernel
	 */
	stop_floppy();
	boot_ros();
}

BOOL MultiBootLoadKernel(FILE *KernelImage)
{
	DWORD				ImageHeaders[2048];
	int					Idx;
	DWORD				dwHeaderChecksum;
	DWORD				dwFileLoadOffset;
	DWORD				dwDataSize;
	DWORD				dwBssSize;

	/*
	 * Load the first 8192 bytes of the kernel image
	 * so we can search for the multiboot header
	 */
	ReadFile(KernelImage, 8192, ImageHeaders);

	/*
	 * Now find the multiboot header and copy it
	 */
	for (Idx=0; Idx<2048; Idx++)
	{
		// Did we find it?
		if (ImageHeaders[Idx] == MULTIBOOT_HEADER_MAGIC)
		{
			// Yes, copy it and break out of this loop
			memcpy(&mb_header, &ImageHeaders[Idx], sizeof(multiboot_header_t));

			break;
		}
	}

	/*
	 * If we reached the end of the 8192 bytes without
	 * finding the multiboot header then return error
	 */
	if (Idx == 2048)
	{
		MessageBox("No multiboot header found!");
		return FALSE;
	}

	/*printf("multiboot header:\n");
	printf("0x%x\n", mb_header.magic);
	printf("0x%x\n", mb_header.flags);
	printf("0x%x\n", mb_header.checksum);
	printf("0x%x\n", mb_header.header_addr);
	printf("0x%x\n", mb_header.load_addr);
	printf("0x%x\n", mb_header.load_end_addr);
	printf("0x%x\n", mb_header.bss_end_addr);
	printf("0x%x\n", mb_header.entry_addr);
	getch();*/

	/*
	 * Calculate the checksum and make sure it matches
	 */
	dwHeaderChecksum = mb_header.magic;
	dwHeaderChecksum += mb_header.flags;
	dwHeaderChecksum += mb_header.checksum;
	if (dwHeaderChecksum != 0)
	{
		MessageBox("Multiboot header checksum invalid!");
		return FALSE;
	}
	
	/*
	 * Get the file offset, this should be 0, and move the file pointer
	 */
	dwFileLoadOffset = (Idx * sizeof(DWORD)) - (mb_header.header_addr - mb_header.load_addr);
	fseek(KernelImage, dwFileLoadOffset);
	
	/*
	 * Load the file image
	 */
	dwDataSize = (mb_header.load_end_addr - mb_header.load_addr);
	ReadFile(KernelImage, dwDataSize, (void*)mb_header.load_addr);

	/*
	 * Initialize bss area
	 */
	dwBssSize = (mb_header.bss_end_addr - mb_header.load_end_addr);
	memset((void*)mb_header.load_end_addr, 0, dwBssSize);

	next_module_load_base = ROUND_UP(mb_header.bss_end_addr, /*PAGE_SIZE*/4096);

	return TRUE;
}

BOOL MultiBootLoadModule(FILE *ModuleImage, char *ModuleName)
{
	DWORD		dwModuleSize;
	module_t*	pModule = &multiboot_modules[mb_info.mods_count];
	char*		ModuleNameString = multiboot_module_strings[mb_info.mods_count];

	dwModuleSize = GetFileSize(ModuleImage);
	pModule->mod_start = next_module_load_base;
	pModule->mod_end = next_module_load_base + dwModuleSize;
	strcpy(ModuleNameString, ModuleName);
	pModule->string = (unsigned long)ModuleNameString;
	
	/*
	 * Load the file image
	 */
	ReadFile(ModuleImage, dwModuleSize, (void*)next_module_load_base);

	next_module_load_base = ROUND_UP(pModule->mod_end, /*PAGE_SIZE*/4096);
	mb_info.mods_count++;

	return TRUE;
}
