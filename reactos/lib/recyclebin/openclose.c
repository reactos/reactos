/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/openclose.c
 * PURPOSE:     Open/close recycle bins
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

BOOL
IntCheckDeletedFileHandle(
	IN HANDLE hDeletedFile)
{
	if (hDeletedFile == NULL || hDeletedFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (((PDELETED_FILE_HANDLE)hDeletedFile)->magic != DELETEDFILE_MAGIC)
		return FALSE;

	return TRUE;
}

static BOOL
IntCloseRecycleBinHandle(
	IN PREFCOUNT_DATA pData)
{
	PRECYCLE_BIN bin;

	bin = CONTAINING_RECORD(pData, RECYCLE_BIN, refCount);
	if (!CloseHandle(bin->hInfo))
		return FALSE;

	RemoveEntryList(&bin->ListEntry);
	HeapFree(GetProcessHeap(), 0, bin);
	return TRUE;
}

static BOOL
IntCreateEmptyRecycleBin(
	IN PRECYCLE_BIN bin,
	IN PSID OwnerSid OPTIONAL)
{
	LPWSTR BufferName = NULL;
	LPWSTR Separator; /* Pointer into BufferName buffer */
	LPWSTR FileName; /* Pointer into BufferName buffer */
	LPCSTR DesktopIniContents = "[.ShellClassInfo]\r\nCLSID={645FF040-5081-101B-9F08-00AA002F954E}\r\n";
	DWORD Info2Contents[] = { 5, 0, 0, 0x320, 0 };
	SIZE_T BytesToWrite, BytesWritten, Needed;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BOOL ret = FALSE;

	Needed = (wcslen(bin->Folder) + 1 + max(wcslen(RECYCLE_BIN_FILE_NAME), wcslen(L"desktop.ini")) + 1) * sizeof(WCHAR);
	BufferName = HeapAlloc(GetProcessHeap(), 0, Needed);
	if (!BufferName)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}

	wcscpy(BufferName, bin->Folder);
	Separator = wcsstr(&BufferName[3], L"\\");
	if (Separator)
		*Separator = UNICODE_NULL;
	ret = CreateDirectoryW(BufferName, NULL);
	if (!ret && GetLastError() != ERROR_ALREADY_EXISTS)
		goto cleanup;
	SetFileAttributesW(BufferName, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
	if (Separator)
	{
		*Separator = L'\\';
		ret = CreateDirectoryW(BufferName, NULL);
		if (!ret && GetLastError() != ERROR_ALREADY_EXISTS)
			goto cleanup;
	}

	if (OwnerSid)
	{
		//DWORD rc;

		/* Add ACL to allow only user/SYSTEM to open it */
		/* FIXME: rc = SetNamedSecurityInfo(
			BufferName,
			SE_FILE_OBJECT,
			???,
			OwnerSid,
			NULL,
			???,
			???);
		if (rc != ERROR_SUCCESS)
		{
			SetLastError(rc);
			goto cleanup;
		}
		*/
	}

	wcscat(BufferName, L"\\");
	FileName = &BufferName[wcslen(BufferName)];

	/* Create desktop.ini */
	wcscpy(FileName, L"desktop.ini");
	hFile = CreateFileW(BufferName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto cleanup;
	BytesToWrite = strlen(DesktopIniContents);
	ret = WriteFile(hFile, DesktopIniContents, (DWORD)BytesToWrite, &BytesWritten, NULL);
	if (!ret)
		goto cleanup;
	if (BytesWritten != BytesToWrite)
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	/* Create empty INFO2 file */
	wcscpy(FileName, RECYCLE_BIN_FILE_NAME);
	hFile = CreateFileW(BufferName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto cleanup;
	BytesToWrite = sizeof(Info2Contents);
	ret = WriteFile(hFile, Info2Contents, (DWORD)BytesToWrite, &BytesWritten, NULL);
	if (!ret)
		goto cleanup;
	if (BytesWritten != BytesToWrite)
	{
		SetLastError(ERROR_GEN_FAILURE);
		goto cleanup;
	}

	bin->hInfo = hFile;
	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, BufferName);
	if (!ret)
	{
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
	}
	return ret;
}

PRECYCLE_BIN
IntReferenceRecycleBin(
	IN WCHAR driveLetter)
{
	PLIST_ENTRY ListEntry;
	PRECYCLE_BIN bin = NULL, ret = NULL;
	WCHAR RootPath[4];
	DWORD FileSystemFlags;
	LPCWSTR RecycleBinDirectory;
	HANDLE tokenHandle = INVALID_HANDLE_VALUE;
	PTOKEN_USER TokenUserInfo = NULL;
	LPWSTR StringSid = NULL;
	SIZE_T Needed, DirectoryLength;
	INFO2_HEADER Header;
	BOOL AlreadyCreated = FALSE;
	DWORD bytesRead;

	static LIST_ENTRY ListHead;
	static BOOL ListInitialized = FALSE;

	if (!ListInitialized)
	{
		InitializeListHead(&ListHead);
		ListInitialized = TRUE;
	}

	/* Search if the recycle bin has already been opened */
	driveLetter = (WCHAR)toupper(driveLetter);
	ListEntry = ListHead.Flink;
	while (ListEntry != &ListHead)
	{
		bin = CONTAINING_RECORD(ListEntry, RECYCLE_BIN, ListEntry);
		if (bin->Folder[0] == driveLetter)
		{
			ReferenceHandle(&bin->refCount);
			return bin;
		}
		ListEntry = ListEntry->Flink;
	}
	bin = NULL;

	/* We need to create a new recycle bin */

	/* Get information about file system */
	wsprintfW(RootPath, L"%c:\\", driveLetter);
	if (!GetVolumeInformationW(
		RootPath,
		NULL,
		0,
		NULL,
		NULL,
		&FileSystemFlags,
		NULL,
		0))
	{
		goto cleanup;
	}
	if (!(FileSystemFlags & FILE_PERSISTENT_ACLS))
		RecycleBinDirectory = RECYCLE_BIN_DIRECTORY_WITHOUT_ACL;
	else
	{
		RecycleBinDirectory = RECYCLE_BIN_DIRECTORY_WITH_ACL;

		/* Get user SID */
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
			goto cleanup;
		if (GetTokenInformation(tokenHandle, TokenUser, NULL, 0, &Needed))
		{
			SetLastError(ERROR_GEN_FAILURE);
			goto cleanup;
		}
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto cleanup;
		TokenUserInfo = HeapAlloc(GetProcessHeap(), 0, Needed);
		if (!TokenUserInfo)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto cleanup;
		}
		if (!GetTokenInformation(tokenHandle, TokenUser, TokenUserInfo, (DWORD)Needed, &Needed))
			goto cleanup;
		if (!ConvertSidToStringSidW(TokenUserInfo->User.Sid, &StringSid))
			goto cleanup;
	}

	/* Create RECYCLEBIN structure */
openfile:
	DirectoryLength = 3 + wcslen(RecycleBinDirectory) + 1;
	if (StringSid)
		DirectoryLength += wcslen(StringSid) + 1;
	Needed = FIELD_OFFSET(RECYCLE_BIN, Folder) + (2 * DirectoryLength + 2 + wcslen(RECYCLE_BIN_FILE_NAME))* sizeof(WCHAR);
	bin = HeapAlloc(GetProcessHeap(), 0, Needed);
	if (!bin)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}
	memset(bin, 0, Needed);
	InitializeHandle(&bin->refCount, IntCloseRecycleBinHandle);
	bin->magic = RECYCLEBIN_MAGIC;
	bin->hInfo = INVALID_HANDLE_VALUE;
	bin->InfoFile = &bin->Folder[DirectoryLength];
	bin->Folder[0] = driveLetter;
	bin->Folder[1] = '\0';
	wcscat(bin->Folder, L":\\");
	wcscat(bin->Folder, RecycleBinDirectory);
	if (StringSid)
	{
		wcscat(bin->Folder, L"\\");
		wcscat(bin->Folder, StringSid);
	}
	wcscpy(bin->InfoFile, bin->Folder);
	wcscat(bin->InfoFile, L"\\");
	wcscat(bin->InfoFile, RECYCLE_BIN_FILE_NAME);
	InsertTailList(&ListHead, &bin->ListEntry);

	/* Open info file */
	bin->hInfo = CreateFileW(bin->InfoFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (bin->hInfo == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			if (!IntCreateEmptyRecycleBin(bin, TokenUserInfo ? TokenUserInfo->User.Sid : NULL))
				goto cleanup;
			AlreadyCreated = TRUE;
		}
	}
	if (bin->hInfo == INVALID_HANDLE_VALUE)
		goto cleanup;

	if (SetFilePointer(bin->hInfo, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto cleanup;
	if (!ReadFile(bin->hInfo, &Header, sizeof(INFO2_HEADER), &bytesRead, NULL))
		goto cleanup;
	if (bytesRead != sizeof(INFO2_HEADER) && !AlreadyCreated)
	{
		/* Create a new file */
		if (!DereferenceHandle(&bin->refCount))
			goto cleanup;
		if (!DeleteFileW(bin->InfoFile))
			goto cleanup;
		goto openfile;
	}
	switch (Header.dwVersion)
	{
		case 5:
			InitializeCallbacks5(&bin->Callbacks);
			break;
		default:
			/* Unknown recycle bin version */
			SetLastError(ERROR_NOT_SUPPORTED);
			goto cleanup;
	}

	ret = bin;

cleanup:
	if (tokenHandle != INVALID_HANDLE_VALUE)
		CloseHandle(tokenHandle);
	HeapFree(GetProcessHeap(), 0, TokenUserInfo);
	if (StringSid)
		LocalFree(StringSid);
	if (!ret)
	{
		if (bin && bin->hInfo != INVALID_HANDLE_VALUE)
			DereferenceHandle(&bin->refCount);
		HeapFree(GetProcessHeap(), 0, bin);
	}
	return ret;
}
