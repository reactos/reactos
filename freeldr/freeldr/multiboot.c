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
#include "stdlib.h"
#include "fs.h"
#include "multiboot.h"
#include "tui.h"

unsigned long				next_module_load_base = 0;

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
	module_t*	pModule;
	char*		ModuleNameString;
	
	/*
	 * Get current module data structure and module name string array
	 */
	pModule = &multiboot_modules[mb_info.mods_count];
	ModuleNameString = multiboot_module_strings[mb_info.mods_count];
	
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

int GetBootPartition(char *OperatingSystemName)
{
	int		BootPartitionNumber = -1;
	char	name[1024];
	char	value[1024];

	if (ReadSectionSettingByName(OperatingSystemName, "BootPartition", name, value))
	{
		BootPartitionNumber = atoi(value);
	}

	return BootPartitionNumber;
}