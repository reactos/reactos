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
#include "asmcode.h"
#include "rosboot.h"
#include "stdlib.h"
#include "fs.h"
#include "tui.h"

extern boot_param	boot_parameters;			// Boot parameter structure passed to the kernel
extern int	boot_param_struct_base;		// Physical address of the boot parameters structure
extern int	start_mem;					// Start of the continuous range of physical kernel memory
extern int	kernel_page_directory_base;	// Physical address of the kernel page directory
extern int	system_page_table_base;		// Physical address of the system page table
extern int	lowmem_page_table_base;		// Physical address of the low mem page table
extern int	start_kernel;				// Physical address of the start of kernel code
extern int	load_base;					// Physical address of the loaded kernel modules 

static int	next_load_base;

void LoadAndBootReactOS(int nOSToBoot)
{
	FILE		file;
	char		name[1024];
	char		value[1024];
	char		szFileName[1024];
	int			i;
	int			nNumDriverFiles=0;
	int			nNumFilesLoaded=0;

	/*
	 * Enable a20 so we can load at 1mb and initialize the page tables
	 */
	//enable_a20(); // enabled in freeldr.c
	ReactOSMemInit();

	// Setup kernel parameters
	boot_param_struct_base = (int)&boot_parameters;
	boot_parameters.magic = 0xdeadbeef;
	boot_parameters.cursorx = 0;
	boot_parameters.cursory = 0;
	boot_parameters.nr_files = 0;
	boot_parameters.start_mem = start_mem;
	boot_parameters.end_mem = load_base;
	boot_parameters.kernel_parameters[0] = 0;

	next_load_base = load_base;

	/*
	 * Read the optional kernel parameters
	 */
	if(ReadSectionSettingByName(OSList[nOSToBoot].name, "Options", name, value))
	{
		strcpy(boot_parameters.kernel_parameters,value);
	}

	/*
	 * Find the kernel image name
	 */
	if(!ReadSectionSettingByName(OSList[nOSToBoot].name, "Kernel", name, value))
	{
		MessageBox("Kernel image file not specified for selected operating system.");
		return;
	}

	/*
	 * Get the boot partition
	 */
	BootPartition = 0;
	if (ReadSectionSettingByName(OSList[nOSToBoot].name, "BootPartition", name, value))
		BootPartition = atoi(value);

	/*
	 * Make sure the boot drive is set in the .ini file
	 */
	if(!ReadSectionSettingByName(OSList[nOSToBoot].name, "BootDrive", name, value))
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
	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		MessageBox("Failed to open boot drive.");
		return;
	}

	/*
	 * Parse the ini file and count the kernel and drivers
	 */
	for (i=1; i<=GetNumSectionItems(OSList[nOSToBoot].name); i++)
	{
		/*
		 * Read the setting and check if it's a driver
		 */
		ReadSectionSettingByNumber(OSList[nOSToBoot].name, i, name, value);
		if ((stricmp(name, "Kernel") == 0) || (stricmp(name, "Driver") == 0))
			nNumDriverFiles++;
	}

	/*
	 * Parse the ini file and load the kernel and
	 * load all the drivers specified
	 */
	for (i=1; i<=GetNumSectionItems(OSList[nOSToBoot].name); i++)
	{
		/*
		 * Read the setting and check if it's a driver
		 */
		ReadSectionSettingByNumber(OSList[nOSToBoot].name, i, name, value);
		if ((stricmp(name, "Kernel") == 0) || (stricmp(name, "Driver") == 0))
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
			ReactOSLoadPEImage(&file);

			nNumFilesLoaded++;
			DrawProgressBar((nNumFilesLoaded * 100) / nNumDriverFiles);

			// Increment the number of files we loaded
			boot_parameters.nr_files++;
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
	 * End the list of modules we load with a zero length entry
	 * and update the end of kernel mem
	 */
	boot_parameters.module_lengths[boot_parameters.nr_files] = 0;
	boot_parameters.end_mem = next_load_base;

	/*
	 * Clear the screen and redraw the backdrop and status bar
	 */
	DrawBackdrop();
	DrawStatusText(" Press any key to boot");

	/*
	 * Wait for user
	 */
	strcpy(name, "Kernel and Drivers loaded.\nPress any key to boot ");
	strcat(name, OSList[nOSToBoot].name);
	strcat(name, ".");
	//MessageBox(name);

	RestoreScreen(pScreenBuffer);

	/*
	 * Now boot the kernel
	 */
	stop_floppy();
	ReactOSBootKernel();
}

void ReactOSMemInit(void)
{
	int		base;

	/* Calculate the start of extended memory */
	base = 0x100000;

	/* Set the start of the page directory */
	kernel_page_directory_base = base;

	/*
	 * Set the start of the continuous range of physical memory
	 * occupied by the kernel
	 */
	start_mem = base;
	base += 0x1000;

	/* Calculate the start of the system page table (0xc0000000 upwards) */
	system_page_table_base = base;
	base += 0x1000;

	/* Calculate the start of the page table to map the first 4mb */
	lowmem_page_table_base = base;
	base += 0x1000;

	/* Set the position for the first module to be loaded */
	load_base = base;

	/* Set the address of the start of kernel code */
	start_kernel = base;
}

void ReactOSBootKernel(void)
{
	int		i;
	int		*pPageDirectory = (int *)kernel_page_directory_base;
	int		*pLowMemPageTable = (int *)lowmem_page_table_base;
	int		*pSystemPageTable = (int *)system_page_table_base;

	/* Zero out the kernel page directory */
	for(i=0; i<1024; i++)
		pPageDirectory[i] = 0;

	/* Map in the lowmem page table */
	pPageDirectory[(0x00/4)] = lowmem_page_table_base + 0x07;

	/* Map in the lowmem page table (and reuse it for the identity map) */
	pPageDirectory[(0xd00/4)] = lowmem_page_table_base + 0x07;
	
	/* Map the page tables from the page directory */
	pPageDirectory[(0xf00/4)] = kernel_page_directory_base + 0x07;

	/* Map in the kernel page table */
	pPageDirectory[(0xc00/4)] = system_page_table_base + 0x07;

	/* Setup the lowmem page table */
	for(i=0; i<1024; i++)
		pLowMemPageTable[i] = (i * 4096) + 0x07;

	/* Setup the system page table */
	for(i=0; i<1024; i++)
		pSystemPageTable[i] = ((i * 4096) + start_kernel) + 0x07;

	boot_ros();
}

BOOL ReactOSLoadPEImage(FILE *pImage)
{
	unsigned int			Idx, ImageBase;
	PULONG					PEMagic;
	PIMAGE_DOS_HEADER		PEDosHeader;
	PIMAGE_FILE_HEADER		PEFileHeader;
	PIMAGE_OPTIONAL_HEADER	PEOptionalHeader;
	PIMAGE_SECTION_HEADER	PESectionHeaders;

	ImageBase = next_load_base;
	boot_parameters.module_lengths[boot_parameters.nr_files] = 0;

	/*
	 * Load the headers
	 */
	ReadFile(pImage, 0x1000, (void *)next_load_base);

	/*
	 * Get header pointers
	 */
	PEDosHeader = (PIMAGE_DOS_HEADER) next_load_base;
	PEMagic = (PULONG) ((unsigned int) next_load_base + 
				PEDosHeader->e_lfanew);
	PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) next_load_base + 
					PEDosHeader->e_lfanew + sizeof(ULONG));
	PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) next_load_base + 
					PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
	PESectionHeaders = (PIMAGE_SECTION_HEADER) ((unsigned int) next_load_base + 
					PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
					sizeof(IMAGE_OPTIONAL_HEADER));

	/*
	 * Check file magic numbers
	 */
	if(PEDosHeader->e_magic != IMAGE_DOS_MAGIC)
	{
		MessageBox("Incorrect MZ magic");
		return FALSE;
	}
	if(PEDosHeader->e_lfanew == 0)
	{
		MessageBox("Invalid lfanew offset");
		return 0;
	}
	if(*PEMagic != IMAGE_PE_MAGIC)
	{
		MessageBox("Incorrect PE magic");
		return 0;
	}
	if(PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
	{
		MessageBox("Incorrect Architecture");
		return 0;
	}

	/*
	 * Get header size and bump next_load_base
	 */
	next_load_base += ROUND_UP(PEOptionalHeader->SizeOfHeaders, PEOptionalHeader->SectionAlignment);
	boot_parameters.module_lengths[boot_parameters.nr_files] += ROUND_UP(PEOptionalHeader->SizeOfHeaders, PEOptionalHeader->SectionAlignment);
	boot_parameters.end_mem += ROUND_UP(PEOptionalHeader->SizeOfHeaders, PEOptionalHeader->SectionAlignment);

	/*
	 * Copy image sections into virtual section
	 */
//  memcpy(DriverBase, ModuleLoadBase, PESectionHeaders[0].PointerToRawData);
//  CurrentBase = (PVOID) ((DWORD)DriverBase + PESectionHeaders[0].PointerToRawData);
//	CurrentSize = 0;
	for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
	{
		/*
		 * Copy current section into current offset of virtual section
		 */
		if (PESectionHeaders[Idx].Characteristics & 
			(IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
		{
			//memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
			//	(PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
			//	PESectionHeaders[Idx].Misc.VirtualSize /*SizeOfRawData*/);

			//MessageBox("loading a section");
			fseek(pImage, PESectionHeaders[Idx].PointerToRawData);
			ReadFile(pImage, PESectionHeaders[Idx].Misc.VirtualSize /*SizeOfRawData*/, (void *)next_load_base);
			//printf("PointerToRawData: %x\n", PESectionHeaders[Idx].PointerToRawData);
			//printf("bytes at next_load_base: %x\n", *((unsigned long *)next_load_base));
			//getch();
		}
		else
		{
			//memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
			//	'\0', PESectionHeaders[Idx].Misc.VirtualSize /*SizeOfRawData*/);

			//MessageBox("zeroing a section");
			memset((void *)next_load_base, '\0', PESectionHeaders[Idx].Misc.VirtualSize /*SizeOfRawData*/);
		}

		PESectionHeaders[Idx].PointerToRawData = next_load_base - ImageBase;

		next_load_base += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,	PEOptionalHeader->SectionAlignment);
		boot_parameters.module_lengths[boot_parameters.nr_files] += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,	PEOptionalHeader->SectionAlignment);
		boot_parameters.end_mem += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,	PEOptionalHeader->SectionAlignment);

		//DrawProgressBar((Idx * 100) / PEFileHeader->NumberOfSections);
	}

	return TRUE;
}