/*
 * PROJECT:         FreeLoader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/peloader.c
 * PURPOSE:         Provides routines for loading PE files. To be merged with
 *                  arch/i386/loader.c in future
 *                  This article was very handy during development:
 *                  http://msdn.microsoft.com/msdnmag/issues/02/03/PE2/
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  The source code in this file is based on the work of respective
 *                  authors of PE loading code in ReactOS and Brian Palmer and
 *                  Alex Ionescu's arch/i386/loader.c, and my research project
 *                  (creating a native EFI loader for Windows)
 */

/* INCLUDES ***************************************************************/
#include <freeldr.h>
#include <debug.h>


BOOLEAN
WinLdrpCompareDllName(IN PCH DllName,
                      IN PUNICODE_STRING UnicodeName);

BOOLEAN
WinLdrpBindImportName(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                      IN PVOID DllBase,
                      IN PVOID ImageBase,
                      IN PIMAGE_THUNK_DATA ThunkData,
                      IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
                      IN ULONG ExportSize,
                      IN BOOLEAN ProcessForwards);

BOOLEAN
WinLdrpLoadAndScanReferencedDll(PLOADER_PARAMETER_BLOCK WinLdrBlock,
                                PCCH DirectoryPath,
                                PCH ImportName,
                                PLDR_DATA_TABLE_ENTRY *DataTableEntry);

BOOLEAN
WinLdrpScanImportAddressTable(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                              IN PVOID DllBase,
                              IN PVOID ImageBase,
                              IN PIMAGE_THUNK_DATA ThunkData);



/* FUNCTIONS **************************************************************/

/* Returns TRUE if DLL has already been loaded - looks in LoadOrderList in LPB */
BOOLEAN
WinLdrCheckForLoadedDll(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                        IN PCH DllName,
                        OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry)
{
	PLDR_DATA_TABLE_ENTRY DataTableEntry;
	LIST_ENTRY *ModuleEntry;

	DPRINTM(DPRINT_PELOADER, "WinLdrCheckForLoadedDll: DllName %X, LoadedEntry: %X\n",
		DllName, LoadedEntry);

	/* Just go through each entry in the LoadOrderList and compare loaded module's
	   name with a given name */
	ModuleEntry = WinLdrBlock->LoadOrderListHead.Flink;
	while (ModuleEntry != &WinLdrBlock->LoadOrderListHead)
	{
		/* Get pointer to the current DTE */
		DataTableEntry = CONTAINING_RECORD(ModuleEntry,
			LDR_DATA_TABLE_ENTRY,
			InLoadOrderLinks);

		DPRINTM(DPRINT_PELOADER, "WinLdrCheckForLoadedDll: DTE %p, EP %p\n",
			DataTableEntry, DataTableEntry->EntryPoint);

		/* Compare names */
		if (WinLdrpCompareDllName(DllName, &DataTableEntry->BaseDllName))
		{
			/* Yes, found it, report pointer to the loaded module's DTE 
			   to the caller and increase load count for it */
			*LoadedEntry = DataTableEntry;
			DataTableEntry->LoadCount++;
			DPRINTM(DPRINT_PELOADER, "WinLdrCheckForLoadedDll: LoadedEntry %X\n", DataTableEntry);
			return TRUE;
		}

		/* Go to the next entry */
		ModuleEntry = ModuleEntry->Flink;
	}

	/* Nothing found */
	return FALSE;
}

BOOLEAN
WinLdrScanImportDescriptorTable(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                                IN PCCH DirectoryPath,
                                IN PLDR_DATA_TABLE_ENTRY ScanDTE)
{
	PLDR_DATA_TABLE_ENTRY DataTableEntry;
	PIMAGE_IMPORT_DESCRIPTOR ImportTable;
	ULONG ImportTableSize;
	PCH ImportName;
	BOOLEAN Status;

	/* Get a pointer to the import table of this image */
	ImportTable = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(VaToPa(ScanDTE->DllBase),
		TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportTableSize);

	{
		UNICODE_STRING BaseName;
		BaseName.Buffer = VaToPa(ScanDTE->BaseDllName.Buffer);
		BaseName.MaximumLength = ScanDTE->BaseDllName.MaximumLength;
		BaseName.Length = ScanDTE->BaseDllName.Length;
		DPRINTM(DPRINT_PELOADER, "WinLdrScanImportDescriptorTable(): %wZ ImportTable = 0x%X\n",
			&BaseName, ImportTable);
	}

	/* If image doesn't have any import directory - just return success */
	if (ImportTable == NULL)
		return TRUE;

	/* Loop through all entries */
	for (;(ImportTable->Name != 0) && (ImportTable->FirstThunk != 0);ImportTable++)
	{
		/* Get pointer to the name */
		ImportName = (PCH)VaToPa(RVA(ScanDTE->DllBase, ImportTable->Name));
		DPRINTM(DPRINT_PELOADER, "WinLdrScanImportDescriptorTable(): Looking at %s\n", ImportName);

		/* In case we get a reference to ourselves - just skip it */
		if (WinLdrpCompareDllName(ImportName, &ScanDTE->BaseDllName))
			continue;

		/* Load the DLL if it is not already loaded */
		if (!WinLdrCheckForLoadedDll(WinLdrBlock, ImportName, &DataTableEntry))
		{
			Status = WinLdrpLoadAndScanReferencedDll(WinLdrBlock,
				DirectoryPath,
				ImportName,
				&DataTableEntry);

			if (!Status)
			{
				DPRINTM(DPRINT_PELOADER, "WinLdrpLoadAndScanReferencedDll() failed\n");
				return Status;
			}
		}

		/* Scan its import address table */
		Status = WinLdrpScanImportAddressTable(
			WinLdrBlock,
			DataTableEntry->DllBase,
			ScanDTE->DllBase,
			(PIMAGE_THUNK_DATA)RVA(ScanDTE->DllBase, ImportTable->FirstThunk));

		if (!Status)
		{
			DPRINTM(DPRINT_PELOADER, "WinLdrpScanImportAddressTable() failed\n");
			return Status;
		}
	}

	return TRUE;
}

BOOLEAN
WinLdrAllocateDataTableEntry(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                             IN PCCH BaseDllName,
                             IN PCCH FullDllName,
                             IN PVOID BasePA,
                             OUT PLDR_DATA_TABLE_ENTRY *NewEntry)
{
	PVOID BaseVA = PaToVa(BasePA);
	PWSTR Buffer;
	PLDR_DATA_TABLE_ENTRY DataTableEntry;
	PIMAGE_NT_HEADERS NtHeaders;
	USHORT Length;

	/* Allocate memory for a data table entry, zero-initialize it */
	DataTableEntry = (PLDR_DATA_TABLE_ENTRY)MmHeapAlloc(sizeof(LDR_DATA_TABLE_ENTRY));
	if (DataTableEntry == NULL)
		return FALSE;
	RtlZeroMemory(DataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY));

	/* Get NT headers from the image */
	NtHeaders = RtlImageNtHeader(BasePA);

	/* Initialize corresponding fields of DTE based on NT headers value */
	DataTableEntry->DllBase = BaseVA;
	DataTableEntry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
	DataTableEntry->EntryPoint = RVA(BaseVA, NtHeaders->OptionalHeader.AddressOfEntryPoint);
	DataTableEntry->SectionPointer = 0;
	DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;

	/* Initialize BaseDllName field (UNICODE_STRING) from the Ansi BaseDllName
	   by simple conversion - copying each character */
	Length = (USHORT)(strlen(BaseDllName) * sizeof(WCHAR));
	Buffer = (PWSTR)MmHeapAlloc(Length);
	if (Buffer == NULL)
	{
		MmHeapFree(DataTableEntry);
		return FALSE;
	}
	RtlZeroMemory(Buffer, Length);

	DataTableEntry->BaseDllName.Length = Length;
	DataTableEntry->BaseDllName.MaximumLength = Length;
	DataTableEntry->BaseDllName.Buffer = PaToVa(Buffer);
	while (*BaseDllName != 0)
	{
		*Buffer++ = *BaseDllName++;
	}

	/* Initialize FullDllName field (UNICODE_STRING) from the Ansi FullDllName
	   using the same method */
	Length = (USHORT)(strlen(FullDllName) * sizeof(WCHAR));
	Buffer = (PWSTR)MmHeapAlloc(Length);
	if (Buffer == NULL)
	{
		MmHeapFree(DataTableEntry);
		return FALSE;
	}
	RtlZeroMemory(Buffer, Length);

	DataTableEntry->FullDllName.Length = Length;
	DataTableEntry->FullDllName.MaximumLength = Length;
	DataTableEntry->FullDllName.Buffer = PaToVa(Buffer);
	while (*FullDllName != 0)
	{
		*Buffer++ = *FullDllName++;
	}

	/* Initialize what's left - LoadCount which is 1, and set Flags so that
	   we know this entry is processed */
	DataTableEntry->Flags = LDRP_ENTRY_PROCESSED;
	DataTableEntry->LoadCount = 1;

	/* Insert this DTE to a list in the LPB */
	InsertTailList(&WinLdrBlock->LoadOrderListHead, &DataTableEntry->InLoadOrderLinks);

	/* Save pointer to a newly allocated and initialized entry */
	*NewEntry = DataTableEntry;

	/* Return success */
	return TRUE;
}

/* WinLdrLoadImage loads the specified image from the file (it doesn't
   perform any additional operations on the filename, just directly
   calls the file I/O routines), and relocates it so that it's ready
   to be used when paging is enabled.
   Addressing mode: physical
 */
BOOLEAN
WinLdrLoadImage(IN PCHAR FileName,
                TYPE_OF_MEMORY MemoryType,
                OUT PVOID *ImageBasePA)
{
	ULONG FileId;
	PVOID PhysicalBase;
	PVOID VirtualBase = NULL;
	UCHAR HeadersBuffer[SECTOR_SIZE * 2];
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG VirtualSize, SizeOfRawData, NumberOfSections;
	LONG Status;
	LARGE_INTEGER Position;
	ULONG i, BytesRead;

	CHAR ProgressString[256];

	/* Inform user we are loading files */
	sprintf(ProgressString, "Loading %s...", strchr(FileName, '\\') + 1);
	UiDrawProgressBarCenter(1, 100, ProgressString);

	/* Open the image file */
	Status = ArcOpen(FileName, OpenReadOnly, &FileId);
	if (Status != ESUCCESS)
	{
		UiMessageBox("Can not open the file");
		return FALSE;
	}

	/* Load the first 2 sectors of the image so we can read the PE header */
	Status = ArcRead(FileId, HeadersBuffer, SECTOR_SIZE * 2, &BytesRead);
	if (Status != ESUCCESS)
	{
		UiMessageBox("Error reading from file");
		ArcClose(FileId);
		return FALSE;
	}

	/* Now read the MZ header to get the offset to the PE Header */
	NtHeaders = RtlImageNtHeader(HeadersBuffer);

	if (!NtHeaders)
	{
		//Print(L"Error - no NT header found in %s\n", FileName);
		UiMessageBox("Error - no NT header found");
		ArcClose(FileId);
		return FALSE;
	}

	/* Ensure this is executable image */
	if (((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0))
	{
		//Print(L"Not an executable image %s\n", FileName);
		UiMessageBox("Not an executable image");
		ArcClose(FileId);
		return FALSE;
	}

	/* Store number of sections to read and a pointer to the first section */
	NumberOfSections = NtHeaders->FileHeader.NumberOfSections;
	SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

	/* Try to allocate this memory, if fails - allocate somewhere else */
	PhysicalBase = MmAllocateMemoryAtAddress(NtHeaders->OptionalHeader.SizeOfImage,
	                   (PVOID)((ULONG)NtHeaders->OptionalHeader.ImageBase & (KSEG0_BASE - 1)),
	                   MemoryType);

	if (PhysicalBase == NULL)
	{
		/* It's ok, we don't panic - let's allocate again at any other "low" place */
		PhysicalBase = MmAllocateMemoryWithType(NtHeaders->OptionalHeader.SizeOfImage, MemoryType);

		if (PhysicalBase == NULL)
		{
			//Print(L"Failed to alloc pages for image %s\n", FileName);
			UiMessageBox("Failed to alloc pages for image");
			ArcClose(FileId);
			return FALSE;
		}
	}

	/* This is the real image base - in form of a virtual address */
	VirtualBase = PaToVa(PhysicalBase);

	DPRINTM(DPRINT_PELOADER, "Base PA: 0x%X, VA: 0x%X\n", PhysicalBase, VirtualBase);

	/* Set to 0 position and fully load the file image */
	Position.HighPart = Position.LowPart = 0;
	Status = ArcSeek(FileId, &Position, SeekAbsolute);
	if (Status != ESUCCESS)
	{
		UiMessageBox("Error seeking to start of file");
		ArcClose(FileId);
		return FALSE;
	}

	Status = ArcRead(FileId, PhysicalBase, NtHeaders->OptionalHeader.SizeOfHeaders, &BytesRead);

	if (Status != ESUCCESS)
	{
		//Print(L"Error reading headers %s\n", FileName);
		UiMessageBox("Error reading headers");
		ArcClose(FileId);
		return FALSE;
	}

	/* Reload the NT Header */
	NtHeaders = RtlImageNtHeader(PhysicalBase);

	/* Load the first section */
	SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

	/* Fill output parameters */
	*ImageBasePA = PhysicalBase;

	/* Walk through each section and read it (check/fix any possible
	   bad situations, if they arise) */
	for (i = 0; i < NumberOfSections; i++)
	{
		VirtualSize = SectionHeader->Misc.VirtualSize;
		SizeOfRawData = SectionHeader->SizeOfRawData;

		/* Handle a case when VirtualSize equals 0 */
		if (VirtualSize == 0)
			VirtualSize = SizeOfRawData;

		/* If PointerToRawData is 0, then force its size to be also 0 */
		if (SectionHeader->PointerToRawData == 0)
		{
			SizeOfRawData = 0;
		}
		else
		{
			/* Cut the loaded size to the VirtualSize extents */
			if (SizeOfRawData > VirtualSize)
				SizeOfRawData = VirtualSize;
		}

		/* Actually read the section (if its size is not 0) */
		if (SizeOfRawData != 0)
		{
			/* Seek to the correct position */
			Position.LowPart = SectionHeader->PointerToRawData;
			Status = ArcSeek(FileId, &Position, SeekAbsolute);

			DPRINTM(DPRINT_PELOADER, "SH->VA: 0x%X\n", SectionHeader->VirtualAddress);

			/* Read this section from the file, size = SizeOfRawData */
			Status = ArcRead(FileId, (PUCHAR)PhysicalBase + SectionHeader->VirtualAddress, SizeOfRawData, &BytesRead);

			if (Status != ESUCCESS)
			{
				DPRINTM(DPRINT_PELOADER, "WinLdrLoadImage(): Error reading section from file!\n");
				break;
			}
		}

		/* Size of data is less than the virtual size - fill up the remainder with zeroes */
		if (SizeOfRawData < VirtualSize)
		{
			DPRINTM(DPRINT_PELOADER, "WinLdrLoadImage(): SORD %d < VS %d\n", SizeOfRawData, VirtualSize);
			RtlZeroMemory((PVOID)(SectionHeader->VirtualAddress + (ULONG_PTR)PhysicalBase + SizeOfRawData), VirtualSize - SizeOfRawData);
		}

		SectionHeader++;
	}

	/* We are done with the file - close it */
	ArcClose(FileId);

	/* If loading failed - return right now */
	if (Status != ESUCCESS)
		return FALSE;


	/* Relocate the image, if it needs it */
	if (NtHeaders->OptionalHeader.ImageBase != (ULONG_PTR)VirtualBase)
	{
		DPRINTM(DPRINT_PELOADER, "Relocating %p -> %p\n",
			NtHeaders->OptionalHeader.ImageBase, VirtualBase);
		return (BOOLEAN)LdrRelocateImageWithBias(PhysicalBase,
			(ULONG_PTR)VirtualBase - (ULONG_PTR)PhysicalBase,
			"FreeLdr",
			TRUE,
			TRUE, /* in case of conflict still return success */
			FALSE);
	}

	return TRUE;
}

/* PRIVATE FUNCTIONS *******************************************************/

/* DllName - physical, UnicodeString->Buffer - virtual */
BOOLEAN
WinLdrpCompareDllName(IN PCH DllName,
                      IN PUNICODE_STRING UnicodeName)
{
	PWSTR Buffer;
	UNICODE_STRING UnicodeNamePA;
	ULONG i, Length;
	
	/* First obvious check: for length of two names */
	Length = strlen(DllName);

	UnicodeNamePA.Length = UnicodeName->Length;
	UnicodeNamePA.MaximumLength = UnicodeName->MaximumLength;
	UnicodeNamePA.Buffer = VaToPa(UnicodeName->Buffer);
	DPRINTM(DPRINT_PELOADER, "WinLdrpCompareDllName: %s and %wZ, Length = %d "
		"UN->Length %d\n", DllName, &UnicodeNamePA, Length, UnicodeName->Length);

	if ((Length * sizeof(WCHAR)) > UnicodeName->Length)
		return FALSE;

	/* Store pointer to unicode string's buffer */
	Buffer = VaToPa(UnicodeName->Buffer);

	/* Loop character by character */
	for (i = 0; i < Length; i++)
	{
		/* Compare two characters, uppercasing them */
		if (toupper(*DllName) != toupper((CHAR)*Buffer))
			return FALSE;

		/* Move to the next character */
		DllName++;
		Buffer++;
	}

	/* Check, if strings either fully match, or match till the "." (w/o extension) */
	if ((UnicodeName->Length == Length * sizeof(WCHAR)) || (*Buffer == L'.'))
	{
		/* Yes they do */
		return TRUE;
	}

	/* Strings don't match, return FALSE */
	return FALSE;
}

BOOLEAN
WinLdrpBindImportName(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                      IN PVOID DllBase,
                      IN PVOID ImageBase,
                      IN PIMAGE_THUNK_DATA ThunkData,
                      IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
                      IN ULONG ExportSize,
                      IN BOOLEAN ProcessForwards)
{
	ULONG Ordinal;
	PULONG NameTable, FunctionTable;
	PUSHORT OrdinalTable;
	LONG High, Low, Middle, Result;
	ULONG Hint;

	//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName(): DllBase 0x%X, ImageBase 0x%X, ThunkData 0x%X, ExportDirectory 0x%X, ExportSize %d, ProcessForwards 0x%X\n",
	//	DllBase, ImageBase, ThunkData, ExportDirectory, ExportSize, ProcessForwards);

	/* Check passed DllBase param */
	if(DllBase == NULL)
	{
		DPRINTM(DPRINT_PELOADER, "WARNING: DllBase == NULL!\n");
		return FALSE;
	}

	/* Convert all non-critical pointers to PA from VA */
	ThunkData = VaToPa(ThunkData);

	/* Is the reference by ordinal? */
	if (IMAGE_SNAP_BY_ORDINAL(ThunkData->u1.Ordinal) && !ProcessForwards)
	{
		/* Yes, calculate the ordinal */
		Ordinal = (ULONG)(IMAGE_ORDINAL(ThunkData->u1.Ordinal) - (UINT32)ExportDirectory->Base);
		//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName(): Ordinal %d\n", Ordinal);
	}
	else
	{
		/* It's reference by name, we have to look it up in the export directory */
		if (!ProcessForwards)
		{
			/* AddressOfData in thunk entry will become a virtual address (from relative) */
			//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName(): ThunkData->u1.AOD was %p\n", ThunkData->u1.AddressOfData);
			ThunkData->u1.AddressOfData =
				(ULONG_PTR)RVA(ImageBase, ThunkData->u1.AddressOfData);
			//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName(): ThunkData->u1.AOD became %p\n", ThunkData->u1.AddressOfData);
		}

		/* Get pointers to Name and Ordinal tables (RVA -> VA) */
		NameTable = (PULONG)VaToPa(RVA(DllBase, ExportDirectory->AddressOfNames));
		OrdinalTable = (PUSHORT)VaToPa(RVA(DllBase, ExportDirectory->AddressOfNameOrdinals));

		//DPRINTM(DPRINT_PELOADER, "NameTable 0x%X, OrdinalTable 0x%X, ED->AddressOfNames 0x%X, ED->AOFO 0x%X\n",
		//	NameTable, OrdinalTable, ExportDirectory->AddressOfNames, ExportDirectory->AddressOfNameOrdinals);

		/* Get the hint, convert it to a physical pointer */
		Hint = ((PIMAGE_IMPORT_BY_NAME)VaToPa((PVOID)ThunkData->u1.AddressOfData))->Hint;
		//DPRINTM(DPRINT_PELOADER, "HintIndex %d\n", Hint);

		/* If Hint is less than total number of entries in the export directory,
		   and import name == export name, then we can just get it from the OrdinalTable */
		if (
			(Hint < ExportDirectory->NumberOfNames) &&
			(
			strcmp(VaToPa(&((PIMAGE_IMPORT_BY_NAME)VaToPa((PVOID)ThunkData->u1.AddressOfData))->Name[0]),
			       (PCHAR)VaToPa( RVA(DllBase, NameTable[Hint])) ) == 0
			)
			)
		{
			Ordinal = OrdinalTable[Hint];
			//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName(): Ordinal %d\n", Ordinal);
		}
		else
		{
			/* It's not the easy way, we have to lookup import name in the name table.
			   Let's use a binary search for this task. */

			//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName() looking up the import name using binary search...\n");

			/* Low boundary is set to 0, and high boundary to the maximum index */
			Low = 0;
			High = ExportDirectory->NumberOfNames - 1;

			/* Perform a binary-search loop */
			while (High >= Low)
			{
				/* Divide by 2 by shifting to the right once */
				Middle = (Low + High) >> 1;

				/* Compare the names */
				Result = strcmp(VaToPa(&((PIMAGE_IMPORT_BY_NAME)VaToPa((PVOID)ThunkData->u1.AddressOfData))->Name[0]),
					(PCHAR)VaToPa(RVA(DllBase, NameTable[Middle])));

				/*DPRINTM(DPRINT_PELOADER, "Binary search: comparing Import '__', Export '%s'\n",*/
					/*VaToPa(&((PIMAGE_IMPORT_BY_NAME)VaToPa(ThunkData->u1.AddressOfData))->Name[0]),*/
					/*(PCHAR)VaToPa(RVA(DllBase, NameTable[Middle])));*/

				/*DPRINTM(DPRINT_PELOADER, "TE->u1.AOD %p, fulladdr %p\n",
					ThunkData->u1.AddressOfData,
					((PIMAGE_IMPORT_BY_NAME)VaToPa(ThunkData->u1.AddressOfData))->Name );*/


				/* Depending on result of strcmp, perform different actions */
				if (Result < 0)
				{
					/* Adjust top boundary */
					High = Middle - 1;
				}
				else if (Result > 0)
				{
					/* Adjust bottom boundary */
					Low = Middle + 1;
				}
				else
				{
					/* Yay, found it! */
					break;
				}
			}

			/* If high boundary is less than low boundary, then no result found */
			if (High < Low)
			{
				//Print(L"Error in binary search\n");
				DPRINTM(DPRINT_PELOADER, "Error in binary search!\n");
				return FALSE;
			}

			/* Everything allright, get the ordinal */
			Ordinal = OrdinalTable[Middle];
			
			//DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName() found Ordinal %d\n", Ordinal);
		}
	}

	/* Check ordinal number for validity! */
	if (Ordinal >= ExportDirectory->NumberOfFunctions)
	{
		DPRINTM(DPRINT_PELOADER, "Ordinal number is invalid!\n");
		return FALSE;
	}

	/* Get a pointer to the function table */
	FunctionTable = (PULONG)VaToPa(RVA(DllBase, ExportDirectory->AddressOfFunctions));

	/* Save a pointer to the function */
	ThunkData->u1.Function = (ULONG_PTR)RVA(DllBase, FunctionTable[Ordinal]);

	/* Is it a forwarder? (function pointer isn't within the export directory) */
	if (((ULONG_PTR)VaToPa((PVOID)ThunkData->u1.Function) > (ULONG_PTR)ExportDirectory) &&
		((ULONG_PTR)VaToPa((PVOID)ThunkData->u1.Function) < ((ULONG_PTR)ExportDirectory + ExportSize)))
	{
		PLDR_DATA_TABLE_ENTRY DataTableEntry;
		CHAR ForwardDllName[255];
		PIMAGE_EXPORT_DIRECTORY RefExportDirectory;
		ULONG RefExportSize;

		/* Save the name of the forward dll */
		RtlCopyMemory(ForwardDllName, (PCHAR)VaToPa((PVOID)ThunkData->u1.Function), sizeof(ForwardDllName));

		/* Strip out its extension */
		*strchr(ForwardDllName,'.') = '\0';

		DPRINTM(DPRINT_PELOADER, "WinLdrpBindImportName(): ForwardDllName %s\n", ForwardDllName);
		if (!WinLdrCheckForLoadedDll(WinLdrBlock, ForwardDllName, &DataTableEntry))
		{
			/* We can't continue if DLL couldn't be loaded, so bomb out with an error */
			//Print(L"Error loading DLL!\n");
			DPRINTM(DPRINT_PELOADER, "Error loading DLL!\n");
			return FALSE;
		}

		/* Get pointer to the export directory of loaded DLL */
		RefExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
			RtlImageDirectoryEntryToData(VaToPa(DataTableEntry->DllBase),
			TRUE,
			IMAGE_DIRECTORY_ENTRY_EXPORT,
			&RefExportSize);

		/* Fail if it's NULL */
		if (RefExportDirectory)
		{
			UCHAR Buffer[128];
			IMAGE_THUNK_DATA RefThunkData;
			PIMAGE_IMPORT_BY_NAME ImportByName;
			PCHAR ImportName;
			BOOLEAN Status;

			/* Get pointer to the import name */
			ImportName = strchr((PCHAR)VaToPa((PVOID)ThunkData->u1.Function), '.') + 1;

			/* Create a IMAGE_IMPORT_BY_NAME structure, pointing to the local Buffer */
			ImportByName = (PIMAGE_IMPORT_BY_NAME)Buffer;

			/* Fill the name with the import name */
			RtlCopyMemory(ImportByName->Name, ImportName, strlen(ImportName)+1);

			/* Set Hint to 0 */
			ImportByName->Hint = 0;

			/* And finally point ThunkData's AddressOfData to that structure */
			RefThunkData.u1.AddressOfData = (ULONG_PTR)ImportByName;

			/* And recursively call ourselves */
			Status = WinLdrpBindImportName(
				WinLdrBlock,
				DataTableEntry->DllBase,
				ImageBase,
				&RefThunkData,
				RefExportDirectory,
				RefExportSize,
				TRUE);

			/* Fill out the ThunkData with data from RefThunkData */
			ThunkData->u1 = RefThunkData.u1;

			/* Return what we got from the recursive call */
			return Status;
		}
		else
		{
			/* Fail if ExportDirectory is NULL */
			return FALSE;
		}
	}

	/* Success! */
	return TRUE;
}

BOOLEAN
WinLdrpLoadAndScanReferencedDll(PLOADER_PARAMETER_BLOCK WinLdrBlock,
                                PCCH DirectoryPath,
                                PCH ImportName,
                                PLDR_DATA_TABLE_ENTRY *DataTableEntry)
{
	CHAR FullDllName[256];
	BOOLEAN Status;
	PVOID BasePA;

	/* Prepare the full path to the file to be loaded */
	strcpy(FullDllName, DirectoryPath);
	strcat(FullDllName, ImportName);

	DPRINTM(DPRINT_PELOADER, "Loading referenced DLL: %s\n", FullDllName);
	//Print(L"Loading referenced DLL: %s\n", FullDllName);

	/* Load the image */
	Status = WinLdrLoadImage(FullDllName, LoaderHalCode, &BasePA);

	if (!Status)
	{
		DPRINTM(DPRINT_PELOADER, "WinLdrLoadImage() failed\n");
		return Status;
	}

	/* Allocate DTE for newly loaded DLL */
	Status = WinLdrAllocateDataTableEntry(WinLdrBlock,
		ImportName,
		FullDllName,
		BasePA,
		DataTableEntry);

	if (!Status)
	{
		DPRINTM(DPRINT_PELOADER,
			"WinLdrAllocateDataTableEntry() failed with Status=0x%X\n", Status);
		return Status;
	}

	/* Scan its dependencies too */
	DPRINTM(DPRINT_PELOADER,
		"WinLdrScanImportDescriptorTable() calling ourselves for %S\n",
		VaToPa((*DataTableEntry)->BaseDllName.Buffer));
	Status = WinLdrScanImportDescriptorTable(WinLdrBlock, DirectoryPath, *DataTableEntry);

	if (!Status)
	{
		DPRINTM(DPRINT_PELOADER,
			"WinLdrScanImportDescriptorTable() failed with Status=0x%X\n", Status);
		return Status;
	}

	return TRUE;
}

BOOLEAN
WinLdrpScanImportAddressTable(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                              IN PVOID DllBase,
                              IN PVOID ImageBase,
                              IN PIMAGE_THUNK_DATA ThunkData)
{
	PIMAGE_EXPORT_DIRECTORY ExportDirectory = NULL;
	BOOLEAN Status;
	ULONG ExportSize;

	DPRINTM(DPRINT_PELOADER, "WinLdrpScanImportAddressTable(): DllBase 0x%X, "
		"ImageBase 0x%X, ThunkData 0x%X\n", DllBase, ImageBase, ThunkData);

	/* Obtain the export table from the DLL's base */
	if (DllBase == NULL)
	{
		//Print(L"Error, DllBase == NULL!\n");
		return FALSE;
	}
	else
	{
		ExportDirectory =
			(PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(VaToPa(DllBase),
				TRUE,
				IMAGE_DIRECTORY_ENTRY_EXPORT,
				&ExportSize);
	}

	DPRINTM(DPRINT_PELOADER, "WinLdrpScanImportAddressTable(): ExportDirectory 0x%X\n", ExportDirectory);

	/* If pointer to Export Directory is */
	if (ExportDirectory == NULL)
		return FALSE;

	/* Go through each entry in the thunk table and bind it */
	while (((PIMAGE_THUNK_DATA)VaToPa(ThunkData))->u1.AddressOfData != 0)
	{
		/* Bind it */
		Status = WinLdrpBindImportName(
			WinLdrBlock,
			DllBase,
			ImageBase,
			ThunkData,
			ExportDirectory,
			ExportSize,
			FALSE);

		/* Move to the next entry */
		ThunkData++;

		/* Return error if binding was unsuccessful */
		if (!Status)
			return Status;
	}

	/* Return success */
	return TRUE;
}
