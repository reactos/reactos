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

#include <ndk/ldrtypes.h>
#include <debug.h>

//FIXME: Do a better way to retrieve Arc disk information
extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];
extern char reactos_arc_strings[32][256];

ARC_DISK_SIGNATURE BldrDiskInfo[32];
CHAR BldrArcNames[32][256];

BOOLEAN
WinLdrCheckForLoadedDll(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                        IN PCH DllName,
                        OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry);

// debug stuff
VOID DumpMemoryAllocMap(VOID);
VOID WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID WinLdrpDumpBootDriver(PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID WinLdrpDumpArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock);


void InitializeHWConfig(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock)
{
	PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
	PCONFIGURATION_COMPONENT Component;
	PCONFIGURATION_COMPONENT_DATA /*CurrentEntry,*/ PreviousEntry, AdapterEntry;
	BOOLEAN IsNextEntryChild;

	DbgPrint((DPRINT_WINDOWS, "InitializeHWConfig()\n"));

	LoaderBlock->ConfigurationRoot = MmAllocateMemory(sizeof(CONFIGURATION_COMPONENT_DATA));
	RtlZeroMemory(LoaderBlock->ConfigurationRoot, sizeof(CONFIGURATION_COMPONENT_DATA));

	/* Fill root == SystemClass */
	ConfigurationRoot = LoaderBlock->ConfigurationRoot;
	Component = &LoaderBlock->ConfigurationRoot->ComponentEntry;

	Component->Class = SystemClass;
	Component->Type = MaximumType;
	Component->Version = 0; // FIXME: ?
	Component->Key = 0;
	Component->AffinityMask = 0;

	IsNextEntryChild = TRUE;
	PreviousEntry = ConfigurationRoot;

	/* Enumerate all PCI buses */
	AdapterEntry = ConfigurationRoot;

	/* TODO: Disk Geometry */
	/* TODO: Keyboard */

	/* TODO: Serial port */

	//Config->ConfigurationData = alloc(sizeof(CONFIGURATION_COMPONENT_DATA), EfiLoaderData);

	/* Convert everything to VA */
	ConvertConfigToVA(LoaderBlock->ConfigurationRoot);
	LoaderBlock->ConfigurationRoot = PaToVa(LoaderBlock->ConfigurationRoot);
}


// Init "phase 0"
VOID
AllocateAndInitLPB(PLOADER_PARAMETER_BLOCK *OutLoaderBlock)
{
	PLOADER_PARAMETER_BLOCK LoaderBlock;

	/* Allocate and zero-init the LPB */
	LoaderBlock = MmAllocateMemory(sizeof(LOADER_PARAMETER_BLOCK));
	RtlZeroMemory(LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));

	/* Init three critical lists, used right away */
	InitializeListHead(&LoaderBlock->LoadOrderListHead);
	InitializeListHead(&LoaderBlock->MemoryDescriptorListHead);
	InitializeListHead(&LoaderBlock->BootDriverListHead);

	/* Alloc space for NLS (it will be converted to VA in WinLdrLoadNLS) */
	LoaderBlock->NlsData = MmAllocateMemory(sizeof(NLS_DATA_BLOCK));
	if (LoaderBlock->NlsData == NULL)
	{
		UiMessageBox("Failed to allocate memory for NLS table data!");
		return;
	}
	RtlZeroMemory(LoaderBlock->NlsData, sizeof(NLS_DATA_BLOCK));

	*OutLoaderBlock = LoaderBlock;
}

// Init "phase 1"
VOID
WinLdrInitializePhase1(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
	//CHAR	Options[] = "/CRASHDEBUG /DEBUGPORT=COM1 /BAUDRATE=115200";
	CHAR	Options[] = "/NODEBUG";
	CHAR	SystemRoot[] = "\\WINNT\\";
	CHAR	HalPath[] = "\\";
	CHAR	ArcBoot[] = "multi(0)disk(0)rdisk(1)partition(1)";
	CHAR	ArcHal[] = "multi(0)disk(0)rdisk(1)partition(1)";

	ULONG i;
	PLOADER_PARAMETER_EXTENSION Extension;

	LoaderBlock->u.I386.CommonDataArea = NULL; // Force No ABIOS support
	
	/* Fill Arc BootDevice */
	LoaderBlock->ArcBootDeviceName = MmAllocateMemory(strlen(ArcBoot)+1);
	strcpy(LoaderBlock->ArcBootDeviceName, ArcBoot);
	LoaderBlock->ArcBootDeviceName = PaToVa(LoaderBlock->ArcBootDeviceName);

	/* Fill Arc HalDevice */
	LoaderBlock->ArcHalDeviceName = MmAllocateMemory(strlen(ArcHal)+1);
	strcpy(LoaderBlock->ArcHalDeviceName, ArcHal);
	LoaderBlock->ArcHalDeviceName = PaToVa(LoaderBlock->ArcHalDeviceName);

	/* Fill SystemRoot */
	LoaderBlock->NtBootPathName = MmAllocateMemory(strlen(SystemRoot)+1);
	strcpy(LoaderBlock->NtBootPathName, SystemRoot);
	LoaderBlock->NtBootPathName = PaToVa(LoaderBlock->NtBootPathName);

	/* Fill NtHalPathName */
	LoaderBlock->NtHalPathName = MmAllocateMemory(strlen(HalPath)+1);
	strcpy(LoaderBlock->NtHalPathName, HalPath);
	LoaderBlock->NtHalPathName = PaToVa(LoaderBlock->NtHalPathName);

	/* Fill load options */
	LoaderBlock->LoadOptions = MmAllocateMemory(strlen(Options)+1);
	strcpy(LoaderBlock->LoadOptions, Options);
	LoaderBlock->LoadOptions = PaToVa(LoaderBlock->LoadOptions);

	/* Arc devices */
	LoaderBlock->ArcDiskInformation = (PARC_DISK_INFORMATION)MmAllocateMemory(sizeof(ARC_DISK_INFORMATION));
	InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

	/* Convert ARC disk information from freeldr to a correct format */
	for (i = 0; i < reactos_disk_count; i++)
	{
		PARC_DISK_SIGNATURE ArcDiskInfo;

		/* Get the ARC structure */
		ArcDiskInfo = &BldrDiskInfo[i];

		/* Copy the data over */
		ArcDiskInfo->Signature = reactos_arc_disk_info[i].Signature;
		ArcDiskInfo->CheckSum = reactos_arc_disk_info[i].CheckSum;

		/* Copy the ARC Name */
		strcpy(BldrArcNames[i], reactos_arc_disk_info[i].ArcName);
		ArcDiskInfo->ArcName = BldrArcNames[i];

		/* Insert into the list */
		InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
			&ArcDiskInfo->ListEntry);
	}

	/* Convert the list to virtual address */
	List_PaToVa(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);
	LoaderBlock->ArcDiskInformation = PaToVa(LoaderBlock->ArcDiskInformation);

	/* Create configuration entries */
	InitializeHWConfig(LoaderBlock);

	/* Convert all DTE into virtual addresses */
	//TODO: !!!

	/* Convert all list's to Virtual address */
	List_PaToVa(&LoaderBlock->LoadOrderListHead);

	/* this one will be converted right before switching to
	   virtual paging mode */
	//List_PaToVa(&LoaderBlock->MemoryDescriptorListHead);

	List_PaToVa(&LoaderBlock->BootDriverListHead);

	/* Initialize Extension now */
	Extension = MmAllocateMemory(sizeof(LOADER_PARAMETER_EXTENSION));
	if (Extension == NULL)
	{
		UiMessageBox("Failed to allocate LPB Extension!");
		return;
	}
	RtlZeroMemory(Extension, sizeof(LOADER_PARAMETER_EXTENSION));

	Extension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
	Extension->MajorVersion = 4;
	Extension->MinorVersion = 0;


	LoaderBlock->Extension = PaToVa(Extension);
}

// Last step before going virtual
void WinLdrSetupForNt(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      PVOID *GdtIdt,
                      ULONG *PcrBasePage,
                      ULONG *TssBasePage)
{
	ULONG TssSize;
	ULONG TssPages;
	ULONG_PTR Pcr = 0;
	ULONG_PTR Tss = 0;
	ULONG BlockSize, NumPages;

	LoaderBlock->u.I386.CommonDataArea = NULL;//CommonDataArea;
	//LoaderBlock->u.I386.MachineType = MachineType; //FIXME: MachineType?

	/* Allocate 2 pages for PCR */
	Pcr = (ULONG_PTR)MmAllocateMemory(2 * MM_PAGE_SIZE);
	*PcrBasePage = Pcr >> MM_PAGE_SHIFT;

	if (Pcr == 0)
	{
		UiMessageBox("Can't allocate PCR\n");
		return;
	}

	/* Allocate TSS */
	TssSize = (sizeof(KTSS) + MM_PAGE_SIZE) & ~(MM_PAGE_SIZE - 1);
	TssPages = TssSize / MM_PAGE_SIZE;

	Tss = (ULONG_PTR)MmAllocateMemory(TssSize);

	*TssBasePage = Tss >> MM_PAGE_SHIFT;

	/* Allocate space for new GDT + IDT */
	BlockSize = NUM_GDT*sizeof(KGDTENTRY) + NUM_IDT*sizeof(KIDTENTRY);//FIXME: Use GDT/IDT limits here?
	NumPages = (BlockSize + MM_PAGE_SIZE - 1) >> MM_PAGE_SHIFT;
	*GdtIdt = (PKGDTENTRY)MmAllocateMemory(NumPages * MM_PAGE_SIZE);

	if (*GdtIdt == NULL)
	{
		UiMessageBox("Can't allocate pages for GDT+IDT!\n");
		return;
	}

	/* Zero newly prepared GDT+IDT */
	RtlZeroMemory(*GdtIdt, NumPages << MM_PAGE_SHIFT);
}

BOOLEAN
WinLdrLoadDeviceDriver(PLOADER_PARAMETER_BLOCK LoaderBlock,
                       LPSTR BootPath,
                       PUNICODE_STRING FilePath,
                       ULONG Flags,
                       PLDR_DATA_TABLE_ENTRY *DriverDTE)
{
	CHAR FullPath[1024];
	CHAR DriverPath[1024];
	CHAR DllName[1024];
	PCHAR DriverNamePos;
	BOOLEAN Status;
	PVOID DriverBase;

	// Separate the path to file name and directory path
	sprintf(DriverPath, "%wZ", FilePath);
	DriverNamePos = strrchr(DriverPath, '\\');
	if (DriverNamePos != NULL)
	{
		// Copy the name
		strcpy(DllName, DriverNamePos+1);

		// Cut out the name from the path
		*(DriverNamePos+1) = 0;
	}

	DbgPrint((DPRINT_WINDOWS, "DriverPath: %s, DllName: %s, LPB %p\n", DriverPath, DllName, LoaderBlock));


	// Check if driver is already loaded
	Status = WinLdrCheckForLoadedDll(LoaderBlock, DllName, DriverDTE);
	if (Status)
	{
		// We've got the pointer to its DTE, just return success
		return TRUE;
	}

	// It's not loaded, we have to load it
	sprintf(FullPath,"%s%wZ", BootPath, FilePath);
	Status = WinLdrLoadImage(FullPath, &DriverBase);
	if (!Status)
		return FALSE;

	// Allocate a DTE for it
	Status = WinLdrAllocateDataTableEntry(LoaderBlock, DllName, DllName, DriverBase, DriverDTE);
	if (!Status)
	{
		DbgPrint((DPRINT_WINDOWS, "WinLdrAllocateDataTableEntry() failed\n"));
		return FALSE;
	}

	// Modify any flags, if needed
	(*DriverDTE)->Flags |= Flags;

	// Look for any dependencies it may have, and load them too
	sprintf(FullPath,"%s%s", BootPath, DriverPath);
	Status = WinLdrScanImportDescriptorTable(LoaderBlock, FullPath, *DriverDTE);
	if (!Status)
	{
		DbgPrint((DPRINT_WINDOWS, "WinLdrScanImportDescriptorTable() failed for %s\n",
			FullPath));
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
WinLdrLoadBootDrivers(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      LPSTR BootPath)
{
	PLIST_ENTRY NextBd;
	PBOOT_DRIVER_LIST_ENTRY BootDriver;
	BOOLEAN Status;

	// Walk through the boot drivers list
	NextBd = LoaderBlock->BootDriverListHead.Flink;

	while (NextBd != &LoaderBlock->BootDriverListHead)
	{
		BootDriver = CONTAINING_RECORD(NextBd, BOOT_DRIVER_LIST_ENTRY, ListEntry);

		//DbgPrint((DPRINT_WINDOWS, "BootDriver %wZ DTE %08X RegPath: %wZ\n", &BootDriver->FilePath,
		//	BootDriver->DataTableEntry, &BootDriver->RegistryPath));

		// Paths are relative (FIXME: Are they always relative?)

		// Load it
		Status = WinLdrLoadDeviceDriver(LoaderBlock, BootPath, &BootDriver->FilePath,
			0, &BootDriver->DataTableEntry);

		// If loading failed - cry loudly
		//FIXME: Maybe remove it from the list and try to continue?
		if (!Status)
		{
			UiMessageBox("Can't load boot driver!");
			return FALSE;
		}

		NextBd = BootDriver->ListEntry.Flink;
	}

	return TRUE;
}

VOID
LoadAndBootWindows(PCSTR OperatingSystemName, WORD OperatingSystemVersion)
{
	CHAR  MsgBuffer[256];
	CHAR  SystemPath[1024], SearchPath[1024];
	CHAR  FileName[1024];
	CHAR  BootPath[256];
	PVOID NtosBase = NULL, HalBase = NULL, KdComBase = NULL;
	BOOLEAN Status;
	ULONG SectionId;
	ULONG BootDevice;
	PLOADER_PARAMETER_BLOCK LoaderBlock, LoaderBlockVA;
	KERNEL_ENTRY_POINT KiSystemStartup;
	PLDR_DATA_TABLE_ENTRY KernelDTE, HalDTE, KdComDTE = NULL;
	// Mm-related things
	PVOID GdtIdt;
	ULONG PcrBasePage=0;
	ULONG TssBasePage=0;

	//sprintf(MsgBuffer,"Booting Microsoft(R) Windows(R) OS version '%04x' is not implemented yet", OperatingSystemVersion);
	//UiMessageBox(MsgBuffer);

	// Open the operating system section
	// specified in the .ini file
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(MsgBuffer,"Operating System section '%s' not found in freeldr.ini", OperatingSystemName);
		UiMessageBox(MsgBuffer);
		return;
	}

	UiDrawBackdrop();
	UiDrawStatusText("Detecting Hardware...");
	UiDrawProgressBarCenter(1, 100, "Loading Windows...");

	//FIXME: This is needed only for MachHwDetect() which performs registry operations!
	RegInitializeRegistry();

	/* Make sure the system path is set in the .ini file */
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

	/* Detect hardware */
	MachHwDetect();

	UiDrawStatusText("Loading...");

	/* Try to open system drive */
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

	// Allocate and minimalistic-initialize LPB
	AllocateAndInitLPB(&LoaderBlock);

	// Load kernel
	strcpy(FileName, BootPath);
	strcat(FileName, "SYSTEM32\\NTOSKRNL.EXE");
	Status = WinLdrLoadImage(FileName, &NtosBase);
	DbgPrint((DPRINT_WINDOWS, "Ntos loaded with status %d at %p\n", Status, NtosBase));

	// Load HAL
	strcpy(FileName, BootPath);
	strcat(FileName, "SYSTEM32\\HAL.DLL");
	Status = WinLdrLoadImage(FileName, &HalBase);
	DbgPrint((DPRINT_WINDOWS, "HAL loaded with status %d at %p\n", Status, HalBase));

	// Load kernel-debugger support dll
	if (OperatingSystemVersion > _WIN32_WINNT_NT4)
	{
		strcpy(FileName, BootPath);
		strcat(FileName, "SYSTEM32\\KDCOM.DLL");
		Status = WinLdrLoadImage(FileName, &KdComBase);
		DbgPrint((DPRINT_WINDOWS, "KdCom loaded with status %d at %p\n", Status, KdComBase));
	}

	// Allocate data table entries for above-loaded modules
	WinLdrAllocateDataTableEntry(LoaderBlock, "ntoskrnl.exe",
		"WINNT\\SYSTEM32\\NTOSKRNL.EXE", NtosBase, &KernelDTE);
	WinLdrAllocateDataTableEntry(LoaderBlock, "hal.dll",
		"WINNT\\SYSTEM32\\HAL.DLL", HalBase, &HalDTE);
	if (OperatingSystemVersion > _WIN32_WINNT_NT4)
	{
		WinLdrAllocateDataTableEntry(LoaderBlock, "kdcom.dll",
			"WINNT\\SYSTEM32\\KDCOM.DLL", KdComBase, &KdComDTE);
	}

	/* Load all referenced DLLs for kernel, HAL and kdcom.dll */
	strcpy(SearchPath, BootPath);
	strcat(SearchPath, "SYSTEM32\\");
	WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, KernelDTE);
	WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, HalDTE);
	if (KdComDTE)
		WinLdrScanImportDescriptorTable(LoaderBlock, SearchPath, KdComDTE);

	/* Load Hive, and then NLS data, OEM font, and prepare boot drivers list */
	Status = WinLdrLoadAndScanSystemHive(LoaderBlock, BootPath);
	DbgPrint((DPRINT_WINDOWS, "SYSTEM hive loaded and scanned with status %d\n", Status));

	/* Load boot drivers */
	Status = WinLdrLoadBootDrivers(LoaderBlock, BootPath);
	DbgPrint((DPRINT_WINDOWS, "Boot drivers loaded with status %d\n", Status));

	/* Initialize Phase 1 - no drivers loading anymore */
	WinLdrInitializePhase1(LoaderBlock);

	/* Alloc PCR, TSS, do magic things with the GDT/IDT */
	WinLdrSetupForNt(LoaderBlock, &GdtIdt, &PcrBasePage, &TssBasePage);

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
	WinLdrpDumpBootDriver(LoaderBlockVA);
	WinLdrpDumpArcDisks(LoaderBlockVA);

	//FIXME: If I substitute this debugging checkpoint, GCC will "optimize away" the code below
	//while (1) {};
	/*asm(".intel_syntax noprefix\n");
		asm("test1:\n");
		asm("jmp test1\n");
	asm(".att_syntax\n");*/


	(*KiSystemStartup)(LoaderBlockVA);

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

VOID
WinLdrpDumpBootDriver(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
	PLIST_ENTRY NextBd;
	PBOOT_DRIVER_LIST_ENTRY BootDriver;

	NextBd = LoaderBlock->BootDriverListHead.Flink;

	while (NextBd != &LoaderBlock->BootDriverListHead)
	{
		BootDriver = CONTAINING_RECORD(NextBd, BOOT_DRIVER_LIST_ENTRY, ListEntry);

		DbgPrint((DPRINT_WINDOWS, "BootDriver %wZ DTE %08X RegPath: %wZ\n", &BootDriver->FilePath,
			BootDriver->DataTableEntry, &BootDriver->RegistryPath));

		NextBd = BootDriver->ListEntry.Flink;
	}
}

VOID
WinLdrpDumpArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
	PLIST_ENTRY NextBd;
	PARC_DISK_SIGNATURE ArcDisk;

	NextBd = LoaderBlock->ArcDiskInformation->DiskSignatureListHead.Flink;

	while (NextBd != &LoaderBlock->ArcDiskInformation->DiskSignatureListHead)
	{
		ArcDisk = CONTAINING_RECORD(NextBd, ARC_DISK_SIGNATURE, ListEntry);

		DbgPrint((DPRINT_WINDOWS, "ArcDisk %s checksum: 0x%X, signature: 0x%X\n",
			ArcDisk->ArcName, ArcDisk->CheckSum, ArcDisk->Signature));

		NextBd = ArcDisk->ListEntry.Flink;
	}
}

