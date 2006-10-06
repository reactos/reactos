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

BOOLEAN
WinLdrLoadNLSData(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                  IN LPCSTR DirectoryPath,
                  IN LPCSTR AnsiFileName,
                  IN LPCSTR OemFileName,
                  IN LPCSTR LanguageFileName);


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

// Queries registry for those three file names
BOOLEAN WinLdrGetNLSNames(LPSTR AnsiName,
                          LPSTR OemName,
                          LPSTR LangName)
{
	LONG rc = ERROR_SUCCESS;
	FRLDRHKEY hKey;
	WCHAR szIdBuffer[80];
	WCHAR NameBuffer[80];
	ULONG BufferSize;

	/* open the codepage key */
	rc = RegOpenKey(NULL,
		L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage",
		&hKey);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "Couldn't open CodePage registry key");
		return FALSE;
	}

	/* get ANSI codepage */
	BufferSize = sizeof(szIdBuffer);
	rc = RegQueryValue(hKey, L"ACP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "Couldn't get ACP NLS setting");
		return FALSE;
	}

	BufferSize = sizeof(NameBuffer);
	rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)NameBuffer, &BufferSize);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "ACP NLS Setting exists, but isn't readable");
		return FALSE;
	}
	sprintf(AnsiName, "%S", NameBuffer);

	/* get OEM codepage */
	BufferSize = sizeof(szIdBuffer);
	rc = RegQueryValue(hKey, L"OEMCP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "Couldn't get OEMCP NLS setting");
		return FALSE;
	}

	BufferSize = sizeof(NameBuffer);
	rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)NameBuffer, &BufferSize);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "OEMCP NLS setting exists, but isn't readable");
		return FALSE;
	}
	sprintf(OemName, "%S", NameBuffer);

	/* open the language key */
	rc = RegOpenKey(NULL,
		L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language",
		&hKey);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "Couldn't open Language registry key");
		return FALSE;
	}

	/* get the Unicode case table */
	BufferSize = sizeof(szIdBuffer);
	rc = RegQueryValue(hKey, L"Default", NULL, (PUCHAR)szIdBuffer, &BufferSize);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "Couldn't get Language Default setting");
		return FALSE;
	}

	BufferSize = sizeof(NameBuffer);
	rc = RegQueryValue(hKey, szIdBuffer, NULL, (PUCHAR)NameBuffer, &BufferSize);
	if (rc != ERROR_SUCCESS)
	{
		//strcpy(szErrorOut, "Language Default setting exists, but isn't readable");
		return FALSE;
	}
	sprintf(LangName, "%S", NameBuffer);

	return TRUE;
}

BOOLEAN WinLdrLoadAndScanSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                                    IN LPCSTR DirectoryPath)
{
	CHAR SearchPath[1024];
	CHAR AnsiName[256], OemName[256], LangName[256];
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

	// Initialize in-memory registry
	RegInitializeRegistry();

	// Import what was loaded
	Status = RegImportBinaryHive((PCHAR)VaToPa(LoaderBlock->RegistryBase), LoaderBlock->RegistryLength);
	if (!Status)
	{
		UiMessageBox("Importing binary hive failed!");
		return FALSE;
	}

	// Initialize the 'CurrentControlSet' link
	if (RegInitCurrentControlSet(FALSE) != ERROR_SUCCESS)
	{
		UiMessageBox("Initializing CurrentControlSet link failed!");
		return FALSE;
	}

	Status = WinLdrGetNLSNames(AnsiName, OemName, LangName);
	if (!Status)
	{
		UiMessageBox("Getting NLS names from registry failed!");
		return FALSE;
	}

	DbgPrint((DPRINT_WINDOWS, "NLS data %s %s %s\n", AnsiName, OemName, LangName));

	/* Load NLS data, should be moved to WinLdrLoadAndScanSystemHive() */
	strcpy(SearchPath, DirectoryPath);
	strcat(SearchPath, "SYSTEM32\\");
	Status = WinLdrLoadNLSData(LoaderBlock, SearchPath, AnsiName, OemName, LangName);
	DbgPrint((DPRINT_WINDOWS, "NLS data loaded with status %d\n", Status));

	return TRUE;
}


/* PRIVATE FUNCTIONS ******************************************************/

BOOLEAN
WinLdrLoadNLSData(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                  IN LPCSTR DirectoryPath,
                  IN LPCSTR AnsiFileName,
                  IN LPCSTR OemFileName,
                  IN LPCSTR LanguageFileName)
{
	CHAR FileName[255];
	PFILE AnsiFileHandle;
	PFILE OemFileHandle;
	PFILE LanguageFileHandle;
	ULONG AnsiFileSize, OemFileSize, LanguageFileSize;
	ULONG TotalSize;
	ULONG_PTR NlsDataBase;
	PVOID NlsVirtual;
	BOOLEAN Status, AnsiEqualsOem = FALSE;

	/* There may be a case, when OEM and ANSI page coincide */
	if (!strcmp(AnsiFileName, OemFileName))
		AnsiEqualsOem = TRUE;

	/* Open file with ANSI and store its size */
	//Print(L"Loading %s...\n", Filename);
	strcpy(FileName, DirectoryPath);
	strcat(FileName, AnsiFileName);
	AnsiFileHandle = FsOpenFile(FileName);

	if (AnsiFileHandle == NULL)
		goto Failure;

	AnsiFileSize = FsGetFileSize(AnsiFileHandle);
	DbgPrint((DPRINT_WINDOWS, "AnsiFileSize: %d\n", AnsiFileSize));
	FsCloseFile(AnsiFileHandle);

	/* Open OEM file and store its length */
	if (AnsiEqualsOem)
	{
		OemFileSize = 0;
	}
	else
	{
		//Print(L"Loading %s...\n", Filename);
		strcpy(FileName, DirectoryPath);
		strcat(FileName, OemFileName);
		OemFileHandle = FsOpenFile(FileName);

		if (OemFileHandle == NULL)
			goto Failure;

		OemFileSize = FsGetFileSize(OemFileHandle);
		FsCloseFile(OemFileHandle);
	}
	DbgPrint((DPRINT_WINDOWS, "OemFileSize: %d\n", OemFileSize));

	/* And finally open the language codepage file and store its length */
	//Print(L"Loading %s...\n", Filename);
	strcpy(FileName, DirectoryPath);
	strcat(FileName, LanguageFileName);
	LanguageFileHandle = FsOpenFile(FileName);

	if (LanguageFileHandle == NULL)
		goto Failure;

	LanguageFileSize = FsGetFileSize(LanguageFileHandle);
	FsCloseFile(LanguageFileHandle);
	DbgPrint((DPRINT_WINDOWS, "LanguageFileSize: %d\n", LanguageFileSize));

	/* Sum up all three length, having in mind that every one of them
	   must start at a page boundary => thus round up each file to a page */
	TotalSize = MM_SIZE_TO_PAGES(AnsiFileSize) +
		MM_SIZE_TO_PAGES(OemFileSize)  +
		MM_SIZE_TO_PAGES(LanguageFileSize);

	NlsDataBase = (ULONG_PTR)MmAllocateMemory(TotalSize*MM_PAGE_SIZE);

	if (NlsDataBase == 0)
		goto Failure;

	NlsVirtual = (PVOID)(KSEG0_BASE | NlsDataBase);
	LoaderBlock->NlsData->AnsiCodePageData = NlsVirtual;
	LoaderBlock->NlsData->OemCodePageData = (PVOID)((PUCHAR)NlsVirtual +
		(MM_SIZE_TO_PAGES(AnsiFileSize) << MM_PAGE_SHIFT));
	LoaderBlock->NlsData->UnicodeCodePageData = (PVOID)((PUCHAR)NlsVirtual +
		(MM_SIZE_TO_PAGES(AnsiFileSize) << MM_PAGE_SHIFT) +
		(MM_SIZE_TO_PAGES(OemFileSize) << MM_PAGE_SHIFT));

	/* Ansi and OEM data are the same - just set pointers to the same area */
	if (AnsiEqualsOem)
		LoaderBlock->NlsData->OemCodePageData = LoaderBlock->NlsData->AnsiCodePageData;


	/* Now actually read the data into memory, starting with Ansi file */
	strcpy(FileName, DirectoryPath);
	strcat(FileName, AnsiFileName);
	AnsiFileHandle = FsOpenFile(FileName);

	if (AnsiFileHandle == NULL)
		goto Failure;

	Status = FsReadFile(AnsiFileHandle, AnsiFileSize, NULL, VaToPa(LoaderBlock->NlsData->AnsiCodePageData));

	if (!Status)
		goto Failure;

	FsCloseFile(AnsiFileHandle);

	/* OEM now, if it doesn't equal Ansi of course */
	if (!AnsiEqualsOem)
	{
		strcpy(FileName, DirectoryPath);
		strcat(FileName, OemFileName);
		OemFileHandle = FsOpenFile(FileName);

		if (OemFileHandle == NULL)
			goto Failure;

		Status = FsReadFile(OemFileHandle, OemFileSize, NULL, VaToPa(LoaderBlock->NlsData->OemCodePageData));

		if (!Status)
			goto Failure;

		FsCloseFile(AnsiFileHandle);
	}

	/* finally the language file */
	strcpy(FileName, DirectoryPath);
	strcat(FileName, LanguageFileName);
	LanguageFileHandle = FsOpenFile(FileName);

	if (LanguageFileHandle == NULL)
		goto Failure;

	Status = FsReadFile(LanguageFileHandle, LanguageFileSize, NULL, VaToPa(LoaderBlock->NlsData->UnicodeCodePageData));

	if (!Status)
		goto Failure;

	FsCloseFile(LanguageFileHandle);

	//
	// THIS IS HAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACK
	// Should go to WinLdrLoadOemHalFont(), when it will be implemented
	//
	LoaderBlock->OemFontFile = VaToPa(LoaderBlock->NlsData->UnicodeCodePageData);

	/* Convert NlsTables address to VA */
	LoaderBlock->NlsData = PaToVa(LoaderBlock->NlsData);

	return TRUE;

Failure:
	//UiMessageBox("Error reading NLS file %s\n", Filename);
	UiMessageBox("Error reading NLS file!");
	return FALSE;
}
