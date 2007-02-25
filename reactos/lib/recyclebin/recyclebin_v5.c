/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/openclose.c
 * PURPOSE:     Deals with recycle bins of Windows 2000/XP/2003
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_v5.h"

VOID
InitializeCallbacks5(
	IN OUT PRECYCLEBIN_CALLBACKS Callbacks)
{
	Callbacks->CloseHandle     = CloseHandle5;
	Callbacks->DeleteFile      = DeleteFile5;
	Callbacks->EmptyRecycleBin = EmptyRecycleBin5;
	Callbacks->EnumerateFiles  = EnumerateFiles5;
	Callbacks->GetDetails      = GetDetails5;
	Callbacks->RestoreFile     = RestoreFile5;
}

static BOOL
CloseHandle5(
	IN HANDLE hDeletedFile)
{
	UNREFERENCED_PARAMETER(hDeletedFile);
	/* Nothing to do, as hDeletedFile is simply a DWORD... */
	return TRUE;
}

static BOOL
DeleteFile5(
	IN PRECYCLE_BIN bin,
	IN LPCWSTR FullPath,
	IN LPCWSTR FileName)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	INFO2_HEADER Header;
	DELETED_FILE_RECORD DeletedFile;
	DWORD bytesRead, bytesWritten;
	ULARGE_INTEGER fileSize;
	SYSTEMTIME SystemTime;
	WCHAR RootDir[4];
	WCHAR DeletedFileName[2 * MAX_PATH];
	LPCWSTR Extension;
	DWORD ClusterSize, BytesPerSector, SectorsPerCluster;
	BOOL ret = FALSE;

	if (wcslen(FullPath) >= MAX_PATH)
	{
		/* Unable to store a too long path in recycle bin */
		SetLastError(ERROR_INVALID_NAME);
		goto cleanup;
	}

	hFile = CreateFileW(FullPath, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto cleanup;

	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!ReadFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesRead, NULL))
		goto cleanup;
	if (bytesRead != sizeof(INFO2_HEADER) || Header.dwRecordSize == 0)
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	if (Header.dwVersion != 5 || Header.dwRecordSize != sizeof(DELETED_FILE_RECORD))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	/* Get file size */
#if 0
	if (!GetFileSizeEx(hFile, &fileSize))
		goto cleanup;
#else
	fileSize.u.LowPart = GetFileSize(hFile, &fileSize.u.HighPart);
	if (fileSize.u.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
		goto cleanup;
#endif
	/* Check if file size is > 4Gb */
	if (fileSize.u.HighPart != 0)
	{
		/* FIXME: how to delete files >= 4Gb? */
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		goto cleanup;
	}
	Header.dwTotalLogicalSize += fileSize.u.LowPart;

	/* Generate new name */
	Header.dwHighestRecordUniqueId++;
	Extension = wcsrchr(FileName, '.');
	wsprintfW(DeletedFileName, L"%s\\D%c%lu%s", bin->Folder, FullPath[0] - 'A' + 'a', Header.dwHighestRecordUniqueId, Extension);

	/* Get cluster size */
	wsprintfW(RootDir, L"%c:\\", bin->Folder[0]);
	if (!GetDiskFreeSpaceW(RootDir, &SectorsPerCluster, &BytesPerSector, NULL, NULL))
		goto cleanup;
	ClusterSize = BytesPerSector * SectorsPerCluster;

	/* Get current time */
	GetSystemTime(&SystemTime);
	if (!SystemTimeToFileTime(&SystemTime, &DeletedFile.DeletionTime))
		goto cleanup;

	/* Update INFO2 */
	memset(&DeletedFile, 0, sizeof(DELETED_FILE_RECORD));
	if (WideCharToMultiByte(CP_ACP, 0, FullPath, -1, DeletedFile.FileNameA, MAX_PATH, NULL, NULL) == 0)
	{
		SetLastError(ERROR_INVALID_NAME);
		goto cleanup;
	}
	DeletedFile.dwRecordUniqueId = Header.dwHighestRecordUniqueId;
	DeletedFile.dwDriveNumber = tolower(bin->Folder[0]) - 'a';
	DeletedFile.dwPhysicalFileSize = ROUND_UP(fileSize.u.LowPart, ClusterSize);
	wcscpy(DeletedFile.FileNameW, FullPath);

	if (!SetFilePointer(bin->hInfo, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!WriteFile(bin->hInfo, &DeletedFile, sizeof(DELETED_FILE_RECORD), &bytesWritten, NULL))
		goto cleanup;
	if (bytesWritten != sizeof(DELETED_FILE_RECORD))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}
	Header.dwNumberOfEntries++;
	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		goto cleanup;
	}
	if (!WriteFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesWritten, NULL))
		goto cleanup;
	if (bytesWritten != sizeof(INFO2_HEADER))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	/* Move file */
	if (!MoveFileW(FullPath, DeletedFileName))
		goto cleanup;

	ret = TRUE;

cleanup:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return ret;
}

static BOOL
EmptyRecycleBin5(
	IN PRECYCLE_BIN* bin)
{
	LPWSTR InfoFile = NULL;
	BOOL ret = FALSE;

	InfoFile = HeapAlloc(GetProcessHeap(), 0, wcslen((*bin)->InfoFile) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!InfoFile)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}
	wcscpy(InfoFile, (*bin)->InfoFile);

	/* Delete all files in the recycle bin */
	if (!EnumerateFiles5(*bin, IntEmptyRecycleBinCallback, *bin))
		goto cleanup;

	/* Delete INFO2 */
	if (!DereferenceHandle(&(*bin)->refCount))
		goto cleanup;
	if (!DeleteFileW(InfoFile))
		goto cleanup;
	*bin = NULL;
	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, InfoFile);
	return ret;
}

static BOOL
EnumerateFiles5(
	IN PRECYCLE_BIN bin,
	IN PINT_ENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL)
{
	INFO2_HEADER Header;
	DELETED_FILE_RECORD DeletedFile;
	DWORD bytesRead, dwEntries;
	BOOL ret = FALSE;

	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!ReadFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesRead, NULL))
		goto cleanup;
	if (bytesRead != sizeof(INFO2_HEADER) || Header.dwRecordSize == 0)
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	if (Header.dwVersion != 5 || Header.dwRecordSize != sizeof(DELETED_FILE_RECORD))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	SetLastError(ERROR_SUCCESS);
	for (dwEntries = 0; dwEntries < Header.dwNumberOfEntries; dwEntries++)
	{
		if (!ReadFile(bin->hInfo, &DeletedFile, Header.dwRecordSize, &bytesRead, NULL))
			goto cleanup;
		if (bytesRead != Header.dwRecordSize)
		{
			SetLastError(ERROR_GEN_FAILURE);
			goto cleanup;
		}
		if (!pFnCallback(Context, (HANDLE)(ULONG_PTR)DeletedFile.dwRecordUniqueId))
			goto cleanup;
		if (SetFilePointer(bin->hInfo, sizeof(INFO2_HEADER) + Header.dwRecordSize * dwEntries, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			goto cleanup;
	}

	ret = TRUE;

cleanup:
	return ret;
}

static BOOL
GetDetails5(
	IN PRECYCLE_BIN bin,
	IN HANDLE hDeletedFile,
	IN DWORD BufferSize,
	IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL,
	OUT LPDWORD RequiredSize OPTIONAL)
{
	DELETED_FILE_RECORD DeletedFile;
	SIZE_T Needed;
	LPWSTR FullName = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL ret = FALSE;

	/* Check parameters */
	if (BufferSize > 0 && FileDetails == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		goto cleanup;
	}

	if (!IntSearchRecord(bin, hDeletedFile, &DeletedFile, NULL))
		goto cleanup;
	Needed = FIELD_OFFSET(DELETED_FILE_DETAILS_W, FileName) + (wcslen(DeletedFile.FileNameW) + 1) * sizeof(WCHAR);
	if (RequiredSize)
		*RequiredSize = (DWORD)Needed;
	if (Needed > BufferSize)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		goto cleanup;
	}

	if (!IntGetFullName(bin, &DeletedFile, &FullName))
		goto cleanup;

	/* Open file */
	FileDetails->Attributes = GetFileAttributesW(FullName);
	if (FileDetails->Attributes == INVALID_FILE_ATTRIBUTES)
		goto cleanup;
	if (FileDetails->Attributes & FILE_ATTRIBUTE_DIRECTORY)
		hFile = CreateFileW(FullName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	else
		hFile = CreateFileW(FullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto cleanup;

	/* Fill returned structure */
	if (!GetFileTime(hFile, NULL, NULL, &FileDetails->LastModification))
		goto cleanup;
	memcpy(&FileDetails->DeletionTime, &DeletedFile.DeletionTime, sizeof(FILETIME));
	FileDetails->FileSize.u.LowPart = GetFileSize(hFile, &FileDetails->FileSize.u.HighPart);
	if (FileDetails->FileSize.u.LowPart == INVALID_FILE_SIZE)
		goto cleanup;
	FileDetails->PhysicalFileSize.u.HighPart = 0;
	FileDetails->PhysicalFileSize.u.LowPart = DeletedFile.dwPhysicalFileSize;
	wcscpy(FileDetails->FileName, DeletedFile.FileNameW);

	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, FullName);
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return ret;
}

static BOOL
RestoreFile5(
	IN PRECYCLE_BIN bin,
	IN HANDLE hDeletedFile)
{
	INFO2_HEADER Header;
	DWORD bytesRead, bytesWritten;
	LARGE_INTEGER Position;
	DELETED_FILE_RECORD DeletedFile, LastFile;
	LPWSTR FullName = NULL;
	BOOL ret = FALSE;

	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!ReadFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesRead, NULL))
		goto cleanup;
	if (bytesRead != sizeof(INFO2_HEADER) || Header.dwRecordSize == 0)
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	if (Header.dwVersion != 5 || Header.dwRecordSize != sizeof(DELETED_FILE_RECORD))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	/* Search deleted entry */
	if (!IntSearchRecord(bin, hDeletedFile, &DeletedFile, &Position))
		goto cleanup;
	/* Get destination full name */
	if (!IntGetFullName(bin, &DeletedFile, &FullName))
		goto cleanup;
	/* Restore file */
	if (!MoveFileW(FullName, DeletedFile.FileNameW))
		goto cleanup;

	/* Update INFO2 */
	/* 1) If not last entry, copy last entry to the current one */
	if (SetFilePointer(bin->hInfo, -(LONG)sizeof(DELETED_FILE_RECORD), NULL, FILE_END) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!ReadFile(bin->hInfo, &LastFile, sizeof(DELETED_FILE_RECORD), &bytesRead, NULL))
		goto cleanup;
	if (bytesRead != sizeof(DELETED_FILE_RECORD))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}
	if (LastFile.dwRecordUniqueId != DeletedFile.dwRecordUniqueId)
	{
		/* Move the last entry to the current one */
		if (SetFilePointer(bin->hInfo, Position.u.LowPart, &Position.u.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			goto cleanup;
		if (!WriteFile(bin->hInfo, &LastFile, sizeof(DELETED_FILE_RECORD), &bytesWritten, NULL))
			goto cleanup;
		if (bytesWritten != sizeof(DELETED_FILE_RECORD))
		{
			SetLastError(ERROR_GEN_FAILURE);
			goto cleanup;
		}
	}
	/* 2) Update the header */
	Header.dwNumberOfEntries--;
	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!WriteFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesWritten, NULL))
		goto cleanup;
	if (bytesWritten != sizeof(INFO2_HEADER))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}
	/* 3) Truncate file */
	if (SetFilePointer(bin->hInfo, -(LONG)sizeof(DELETED_FILE_RECORD), NULL, FILE_END) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!SetEndOfFile(bin->hInfo))
		goto cleanup;
	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, FullName);
	return ret;
}

static BOOL
IntDeleteRecursive(
	IN LPCWSTR FullName)
{
	DWORD RemovableAttributes = FILE_ATTRIBUTE_READONLY;
	DWORD FileAttributes;
	BOOL ret = FALSE;

	FileAttributes = GetFileAttributesW(FullName);
	if (FileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			ret = TRUE;
		goto cleanup;
	}
	if (FileAttributes & RemovableAttributes)
	{
		if (!SetFileAttributesW(FullName, FileAttributes & ~RemovableAttributes))
			goto cleanup;
	}
	if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		/* Recursive deletion */
		/* FIXME: recursive deletion */
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		goto cleanup;

		if (!RemoveDirectoryW(FullName))
			goto cleanup;
	}
	else
	{
		if (!DeleteFileW(FullName))
			goto cleanup;
	}
	ret = TRUE;

cleanup:
	return ret;
}

static BOOL
IntEmptyRecycleBinCallback(
	IN PVOID Context,
	IN HANDLE hDeletedFile)
{
	PRECYCLE_BIN bin = (PRECYCLE_BIN)Context;
	DELETED_FILE_RECORD DeletedFile;
	LPWSTR FullName = NULL;
	BOOL ret = FALSE;

	if (!IntSearchRecord(bin, hDeletedFile, &DeletedFile, NULL))
		goto cleanup;

	if (!IntGetFullName(bin, &DeletedFile, &FullName))
		goto cleanup;

	if (!IntDeleteRecursive(FullName))
		goto cleanup;
	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, FullName);
	return ret;
}

static BOOL
IntGetFullName(
	IN PRECYCLE_BIN bin,
	IN PDELETED_FILE_RECORD pDeletedFile,
	OUT LPWSTR* pFullName)
{
	SIZE_T Needed;
	LPCWSTR Extension;
	LPWSTR FullName = NULL;
	BOOL ret = FALSE;

	*pFullName = NULL;
	Extension = wcsrchr(pDeletedFile->FileNameW, '.');
	if (Extension < wcsrchr(pDeletedFile->FileNameW, '\\'))
		Extension = NULL;
	Needed = wcslen(bin->Folder) + 13;
	if (Extension)
		Needed += wcslen(Extension);
	FullName = HeapAlloc(GetProcessHeap(), 0, Needed * sizeof(WCHAR));
	if (!FullName)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}
	wsprintfW(FullName, L"%s\\D%c%lu%s", bin->Folder, pDeletedFile->dwDriveNumber + 'a', pDeletedFile->dwRecordUniqueId, Extension);
	*pFullName = FullName;
	ret = TRUE;

cleanup:
	if (!ret)
		HeapFree(GetProcessHeap(), 0, FullName);
	return ret;
}

static BOOL
IntSearchRecord(
	IN PRECYCLE_BIN bin,
	IN HANDLE hDeletedFile,
	OUT PDELETED_FILE_RECORD pDeletedFile,
	OUT PLARGE_INTEGER Position OPTIONAL)
{
	INFO2_HEADER Header;
	DELETED_FILE_RECORD DeletedFile;
	DWORD bytesRead, dwEntries;
	BOOL ret = FALSE;

	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!ReadFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesRead, NULL))
		goto cleanup;
	if (bytesRead != sizeof(INFO2_HEADER) || Header.dwRecordSize == 0)
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	if (Header.dwVersion != 5 || Header.dwRecordSize != sizeof(DELETED_FILE_RECORD))
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	SetLastError(ERROR_SUCCESS);
	for (dwEntries = 0; dwEntries < Header.dwNumberOfEntries; dwEntries++)
	{
		if (Position)
		{
			LARGE_INTEGER Zero;
			Zero.QuadPart = 0;
			if (!SetFilePointerEx(bin->hInfo, Zero, Position, FILE_CURRENT))
				goto cleanup;
		}
		if (!ReadFile(bin->hInfo, &DeletedFile, Header.dwRecordSize, &bytesRead, NULL))
			goto cleanup;
		if (bytesRead != Header.dwRecordSize)
		{
			SetLastError(ERROR_GEN_FAILURE);
			goto cleanup;
		}
		if (DeletedFile.dwRecordUniqueId == (DWORD)(ULONG_PTR)hDeletedFile)
		{
			memcpy(pDeletedFile, &DeletedFile, Header.dwRecordSize);
			ret = TRUE;
			goto cleanup;
		}
	}

	/* Entry not found */
	SetLastError(ERROR_INVALID_HANDLE);

cleanup:
	return ret;
}
