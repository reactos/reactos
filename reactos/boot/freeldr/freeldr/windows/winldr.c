/*
 *  FreeLoader
 *
 *  Copyright (C) 1998-2003  Brian Palmer    <brianp@sginet.com>
 *  Copyright (C) 2006       Aleksey Bragin  <aleksey@reactos.org>
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

#define NDEBUG
#include <debug.h>

VOID DumpMemoryAllocMap(VOID);
VOID WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock);

VOID
LoadAndBootWindows(PCSTR OperatingSystemName, WORD OperatingSystemVersion)
{
	CHAR  MsgBuffer[256];
	CHAR  SystemPath[1024], SearchPath[1024];
	CHAR  FileName[1024];
	CHAR  BootPath[256];
	PVOID NtosBase = NULL, HalBase = NULL;
	BOOLEAN Status;
	ULONG SectionId;
	ULONG BootDevice;
	PLOADER_PARAMETER_BLOCK LoaderBlock=NULL, LoaderBlockVA;
	PLDR_DATA_TABLE_ENTRY KernelDTE, HalDTE;
	KERNEL_ENTRY_POINT KiSystemStartup;
	// Mm-related things
	PVOID GdtIdt=NULL;
	ULONG PcrBasePage=0;
	ULONG TssBasePage=0;

	//sprintf(MsgBuffer,"Booting Microsoft(R) Windows(R) OS version '%04x' is not implemented yet", OperatingSystemVersion);
	//UiMessageBox(MsgBuffer);

	//
	// Open the operating system section
	// specified in the .ini file
	//
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(MsgBuffer,"Operating System section '%s' not found in freeldr.ini", OperatingSystemName);
		UiMessageBox(MsgBuffer);
		return;
	}

	/*
	 * Make sure the system path is set in the .ini file
	 */
	if (!IniReadSettingByName(SectionId, "SystemPath", SystemPath, sizeof(SystemPath)))
	{
		UiMessageBox("System path not specified for selected operating system.");
		return;
	}

	if (!MachDiskNormalizeSystemPath(SystemPath,
	                                 sizeof(SystemPath)))
	{
		UiMessageBox("Invalid system path");
		return;
	}

	UiDrawStatusText("Loading...");

	/*
	 * Try to open system drive
	 */
	BootDevice = 0xffffffff;
	if (!FsOpenSystemVolume(SystemPath, BootPath, &BootDevice))
	{
		UiMessageBox("Failed to open boot drive.");
		return;
	}

	/* append a backslash */
	if ((strlen(BootPath)==0) ||
	    BootPath[strlen(BootPath)] != '\\')
		strcat(BootPath, "\\");

	DbgPrint((DPRINT_WINDOWS,"SystemRoot: '%s'\n", BootPath));

	/* Allocate and minimalistic-initialize LPB */
	//AllocateAndInitLPB(&LoaderBlock);

	// Load kernel
	strcpy(FileName, BootPath);
	strcat(FileName, "SYSTEM32\\NTOSKRNL.EXE");
	Status = WinLdrLoadImage(FileName, &NtosBase);
	DbgPrint((DPRINT_WINDOWS, "Ntos loaded with status %d\n", Status));

	// Load HAL
	strcpy(FileName, BootPath);
	strcat(FileName, "SYSTEM32\\HAL.DLL");
	Status = WinLdrLoadImage(FileName, &HalBase);
	DbgPrint((DPRINT_WINDOWS, "HAL loaded with status %d\n", Status));

	WinLdrAllocateDataTableEntry(LoaderBlock, "ntoskrnl.exe",
		"WINNT\\SYSTEM32\\NTOSKRNL.EXE", NtosBase, &KernelDTE);
	WinLdrAllocateDataTableEntry(LoaderBlock, "hal.dll",
		"WINNT\\SYSTEM32\\HAL.EXE", HalBase, &HalDTE);

	/* Load all referenced DLLs for kernel and HAL */
	strcpy(SearchPath, BootPath);
	strcat(SearchPath, "SYSTEM32\\");
	WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, KernelDTE);
	WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, HalDTE);

	/* Initialize Phase 1 - before NLS */
	//WinLdrInitializePhase1(LoaderBlock);

	/* Load SYSTEM hive and its LOG file */
	strcpy(SearchPath, BootPath);
	strcat(SearchPath, "SYSTEM32\\CONFIG\\");
	//Status = WinLdrLoadSystemHive(LoaderBlock, SearchPath, "SYSTEM");
	DbgPrint((DPRINT_WINDOWS, "SYSTEM hive loaded with status %d\n", Status));

	/* Load NLS data */
	strcpy(SearchPath, BootPath);
	strcat(SearchPath, "SYSTEM32\\");
	//Status = WinLdrLoadNLSData(LoaderBlock, SearchPath,
	//	"c_1252.nls", "c_437.nls", "l_intl.nls");
	DbgPrint((DPRINT_WINDOWS, "NLS data loaded with status %d\n", Status));

	/* Load OEM HAL font */

	/* Load boot drivers */

	/* Alloc PCR, TSS, do magic things with the GDT/IDT */
	//WinLdrSetupForNt(LoaderBlock, &GdtIdt, &PcrBasePage, &TssBasePage);

	/* Save entry-point pointer (VA) */
	KiSystemStartup = (KERNEL_ENTRY_POINT)KernelDTE->EntryPoint;
	LoaderBlockVA = PaToVa(LoaderBlock);

	/* Debugging... */
	//DumpMemoryAllocMap();

	/* Turn on paging mode of CPU*/
	WinLdrTurnOnPaging(LoaderBlock, PcrBasePage, TssBasePage, GdtIdt);

	DbgPrint((DPRINT_WINDOWS, "Hello from paged mode, KiSystemStartup %p, LoaderBlockVA %p!\n",
		KiSystemStartup, LoaderBlockVA));

	WinLdrpDumpMemoryDescriptors(LoaderBlockVA);

	/*__asm
	{
		or esp, KSEG0_BASE;
		or ebp, KSEG0_BASE;
	}*/

	/*
	{
		ULONG *trrr = (ULONG *)(512*1024*1024);

		*trrr = 0x13131414;
	}

	//FIXME!
	while (1) {};*/

	(KiSystemStartup)(LoaderBlockVA);

	return;
}

VOID
WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
	PLIST_ENTRY NextMd;
	PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

	NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

	while (NextMd != &LoaderBlock->MemoryDescriptorListHead)
	{
		MemoryDescriptor = CONTAINING_RECORD(NextMd, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);


		DbgPrint((DPRINT_WINDOWS, "BP %08X PC %04X MT %d\n", MemoryDescriptor->BasePage,
			MemoryDescriptor->PageCount, MemoryDescriptor->MemoryType));

		NextMd = MemoryDescriptor->ListEntry.Flink;
	}
}
