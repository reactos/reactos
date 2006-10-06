/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/wlregistry.c
 * PURPOSE:         Registry support functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS **************************************************************/

BOOLEAN
WinLdrLoadSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                     IN LPCSTR DirectoryPath,
                     IN LPCSTR HiveName)
{
	PFILE FileHandle;
	CHAR FullHiveName[256];
	BOOLEAN Status;
	ULONG HiveFileSize;
	ULONG_PTR HiveDataPhysical;
	PVOID HiveDataVirtual;

	/* Concatenate path and filename to get the full name */
	strcpy(FullHiveName, DirectoryPath);
	strcat(FullHiveName, HiveName);
	//Print(L"Loading %s...\n", FullHiveName);
	FileHandle = FsOpenFile(FullHiveName);

	if (FileHandle == NULL)
	{
		UiMessageBox("Opening hive file failed!");
		return FALSE;
	}

	/* Get the file length */
	HiveFileSize = FsGetFileSize(FileHandle);

	if (HiveFileSize == 0)
	{
		FsCloseFile(FileHandle);
		UiMessageBox("Hive file has 0 size!");
		return FALSE;
	}

	/* Round up the size to page boundary and alloc memory */
	HiveDataPhysical = (ULONG_PTR)MmAllocateMemory(
		MM_SIZE_TO_PAGES(HiveFileSize + MM_PAGE_SIZE - 1) << MM_PAGE_SHIFT);

	if (HiveDataPhysical == 0)
	{
		FsCloseFile(FileHandle);
		UiMessageBox("Unable to alloc memory for a hive!");
		return FALSE;
	}

	/* Convert address to virtual */
	HiveDataVirtual = (PVOID)(KSEG0_BASE | HiveDataPhysical);

	/* Fill LoaderBlock's entries */
	LoaderBlock->RegistryLength = HiveFileSize;
	LoaderBlock->RegistryBase = HiveDataVirtual;

	/* Finally read from file to the memory */
	Status = FsReadFile(FileHandle, HiveFileSize, NULL, (PVOID)HiveDataPhysical);
	FsCloseFile(FileHandle);
	if (!Status)
	{
		UiMessageBox("Unable to read from hive file!");
		return FALSE;
	}

	return TRUE;
}


BOOLEAN WinLdrLoadAndScanSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                                    IN LPCSTR DirectoryPath)
{
	CHAR SearchPath[1024];
	BOOLEAN Status;

	// There is a simple logic here: try to load usual hive (system), if it
	// fails, then give system.alt a try, and finally try a system.sav

	// FIXME: For now we only try system
	strcpy(SearchPath, DirectoryPath);
	strcat(SearchPath, "SYSTEM32\\CONFIG\\");
	Status = WinLdrLoadSystemHive(LoaderBlock, SearchPath, "SYSTEM");

	// Fail if failed...
	if (!Status)
		return FALSE;

	

	return TRUE;
}
