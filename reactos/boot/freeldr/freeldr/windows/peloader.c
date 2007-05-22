/*
 * PROJECT:         WinLoader
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

/* FUNCTIONS **************************************************************/

BOOLEAN
WinLdrCheckForLoadedDll(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                        IN PCH DllName,
                        OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry)
{
	return FALSE;
}


BOOLEAN
WinLdrScanImportDescriptorTable(IN OUT PLOADER_PARAMETER_BLOCK WinLdrBlock,
                                IN PCCH DirectoryPath,
                                IN PLDR_DATA_TABLE_ENTRY ScanDTE)
{
	return FALSE;
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
	DataTableEntry = (PLDR_DATA_TABLE_ENTRY)MmAllocateMemory(sizeof(LDR_DATA_TABLE_ENTRY));
	if (DataTableEntry == NULL)
		return FALSE;
	RtlZeroMemory(DataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY));

	/* Get NT headers from the image */
	NtHeaders = RtlImageNtHeader(BasePA);

	/* Initialize corresponding fields of DTE based on NT headers value */
	DataTableEntry->DllBase = BaseVA;
	DataTableEntry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
	DataTableEntry->EntryPoint = (PVOID)((ULONG)BaseVA +
		NtHeaders->OptionalHeader.AddressOfEntryPoint);
	DataTableEntry->SectionPointer = 0;
	DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;

	/* Initialize BaseDllName field (UNICODE_STRING) from the Ansi BaseDllName
	   by simple conversion - copying each character */
	Length = (USHORT)(strlen(BaseDllName) * sizeof(WCHAR));
	Buffer = (PWSTR)MmAllocateMemory(Length);
	if (Buffer == NULL)
		return FALSE;
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
	Buffer = (PWSTR)MmAllocateMemory(Length);
	if (Buffer == NULL)
		return FALSE;
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
                OUT PVOID *ImageBasePA)
{
	PFILE FileHandle;
	PVOID PhysicalBase;
	PVOID VirtualBase = NULL;
	UCHAR HeadersBuffer[SECTOR_SIZE * 2];
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG VirtualSize, SizeOfRawData, NumberOfSections;
	BOOLEAN Status;
	ULONG i, BytesRead;

	//Print(L"Loading %s...  ", FileName);

	/* Open the image file */
	FileHandle = FsOpenFile(FileName);

	if (FileHandle == NULL)
	{
		//Print(L"Can not open the file %s\n",FileName);
		UiMessageBox("Can not open the file");
		return FALSE;
	}

	/* Load the first 2 sectors of the image so we can read the PE header */
	Status = FsReadFile(FileHandle, SECTOR_SIZE * 2, NULL, HeadersBuffer);
	if (!Status)
	{
		//Print(L"Error reading from file %s\n", FileName);
		UiMessageBox("Error reading from file");
		FsCloseFile(FileHandle);
		return FALSE;
	}

	/* Now read the MZ header to get the offset to the PE Header */
	NtHeaders = RtlImageNtHeader(HeadersBuffer);

	if (!NtHeaders)
	{
		//Print(L"Error - no NT header found in %s\n", FileName);
		UiMessageBox("Error - no NT header found");
		FsCloseFile(FileHandle);
		return FALSE;
	}

	/* Ensure this is executable image */
	if (((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0))
	{
		//Print(L"Not an executable image %s\n", FileName);
		UiMessageBox("Not an executable image");
		FsCloseFile(FileHandle);
		return FALSE;
	}

	/* Store number of sections to read and a pointer to the first section */
	NumberOfSections = NtHeaders->FileHeader.NumberOfSections;
	SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);

	/* Try to allocate this memory, if fails - allocate somewhere else */
	PhysicalBase = MmAllocateMemoryAtAddress(NtHeaders->OptionalHeader.SizeOfImage,
	                                       (PVOID)NtHeaders->OptionalHeader.ImageBase);

	if (PhysicalBase == NULL)
	{
		/* It's ok, we don't panic - let's allocate again at any other "low" place */
		MmChangeAllocationPolicy(FALSE);
		PhysicalBase = MmAllocateMemory(NtHeaders->OptionalHeader.SizeOfImage);
		MmChangeAllocationPolicy(TRUE);

		if (PhysicalBase == NULL)
		{
			//Print(L"Failed to alloc pages for image %s\n", FileName);
			UiMessageBox("Failed to alloc pages for image");
			FsCloseFile(FileHandle);
			return FALSE;
		}
	}

	/* This is the real image base - in form of a virtual address */
	VirtualBase = PaToVa(PhysicalBase);

	DbgPrint((DPRINT_WINDOWS, "Base PA: 0x%X, VA: 0x%X\n", PhysicalBase, VirtualBase));

	/* Set to 0 position and fully load the file image */
	FsSetFilePointer(FileHandle, 0);

	Status = FsReadFile(FileHandle, NtHeaders->OptionalHeader.SizeOfHeaders, NULL, PhysicalBase);

	if (!Status)
	{
		//Print(L"Error reading headers %s\n", FileName);
		UiMessageBox("Error reading headers");
		FsCloseFile(FileHandle);
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
			FsSetFilePointer(FileHandle, SectionHeader->PointerToRawData);

			DbgPrint((DPRINT_WINDOWS, "SH->VA: 0x%X\n", SectionHeader->VirtualAddress));

			/* Read this section from the file, size = SizeOfRawData */
			Status = FsReadFile(FileHandle, SizeOfRawData, &BytesRead, (PUCHAR)PhysicalBase + SectionHeader->VirtualAddress);

			if (!Status && (BytesRead == 0))
				break;
		}

		/* Size of data is less than the virtual size - fill up the remainder with zeroes */
		if (SizeOfRawData < VirtualSize)
			RtlZeroMemory((PVOID)(SectionHeader->VirtualAddress + (ULONG)PhysicalBase + SizeOfRawData), VirtualSize - SizeOfRawData);

		SectionHeader++;
	}

	/* We are done with the file - close it */
	FsCloseFile(FileHandle);

	/* If loading failed - return right now */
	if (!Status)
		return FALSE;


	/* Relocate the image, if it needs it */
	if (NtHeaders->OptionalHeader.ImageBase != (ULONG)VirtualBase)
	{
		DbgPrint((DPRINT_WINDOWS, "Relocating %p -> %p\n",
			NtHeaders->OptionalHeader.ImageBase, VirtualBase));
		Status = (BOOLEAN)LdrRelocateImageWithBias(PhysicalBase,
			0,
			"FLx86",
			TRUE,
			3,
			FALSE);
	}

	return Status;
}

/* PRIVATE FUNCTIONS *******************************************************/
