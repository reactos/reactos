/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/openclose.c
 * PURPOSE:     Public interface
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

typedef struct _ENUMERATE_RECYCLE_BIN_CONTEXT
{
	PRECYCLE_BIN bin;
	PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback;
	PVOID Context;
} ENUMERATE_RECYCLE_BIN_CONTEXT, *PENUMERATE_RECYCLE_BIN_CONTEXT;

BOOL WINAPI
CloseRecycleBinHandle(
	IN HANDLE hDeletedFile)
{
	BOOL ret = FALSE;

	if (!IntCheckDeletedFileHandle(hDeletedFile))
		SetLastError(ERROR_INVALID_HANDLE);
	else
	{
		PDELETED_FILE_HANDLE file = (PDELETED_FILE_HANDLE)hDeletedFile;
		ret = DereferenceHandle(&file->refCount);
	}

	return ret;
}

BOOL WINAPI
DeleteFileToRecycleBinA(
	IN LPCSTR FileName)
{
	int len;
	LPWSTR FileNameW = NULL;
	BOOL ret = FALSE;

	/* Check parameters */
	if (FileName == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		goto cleanup;
	}

	len = MultiByteToWideChar(CP_ACP, 0, FileName, -1, NULL, 0);
	if (len == 0)
		goto cleanup;
	FileNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	if (!FileNameW)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}
	if (MultiByteToWideChar(CP_ACP, 0, FileName, -1, FileNameW, len) == 0)
		goto cleanup;

	ret = DeleteFileToRecycleBinW(FileNameW);

cleanup:
	HeapFree(GetProcessHeap(), 0, FileNameW);
	return ret;
}

BOOL WINAPI
DeleteFileToRecycleBinW(
	IN LPCWSTR FileName)
{
	LPWSTR FullFileName = NULL;
	DWORD dwBufferLength = 0;
	LPWSTR lpFilePart;
	DWORD len;
	PRECYCLE_BIN bin = NULL;
	BOOL ret = FALSE;

	/* Check parameters */
	if (FileName == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		goto cleanup;
	}

	/* Get full file name */
	while (TRUE)
	{
		len = GetFullPathNameW(FileName, dwBufferLength, FullFileName, &lpFilePart);
		if (len == 0)
			goto cleanup;
		else if (len < dwBufferLength)
			break;
		HeapFree(GetProcessHeap(), 0, FullFileName);
		dwBufferLength = len;
		FullFileName = HeapAlloc(GetProcessHeap(), 0, dwBufferLength * sizeof(WCHAR));
		if (!FullFileName)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto cleanup;
		}
	}

	if (!lpFilePart || dwBufferLength < 2 || FullFileName[1] != ':')
	{
		/* Only a directory name, or not a local file */
		SetLastError(ERROR_INVALID_NAME);
	}

	/* Open recycle bin */
	bin = IntReferenceRecycleBin(FullFileName[0]);
	if (!bin)
		goto cleanup;

	if (bin->Callbacks.DeleteFile)
		ret = bin->Callbacks.DeleteFile(bin, FullFileName, lpFilePart);
	else
		SetLastError(ERROR_NOT_SUPPORTED);

cleanup:
	HeapFree(GetProcessHeap(), 0, FullFileName);
	if (bin)
		DereferenceHandle(&bin->refCount);
	return ret;
}

BOOL WINAPI
EmptyRecycleBinA(
	IN CHAR driveLetter)
{
	return EmptyRecycleBinW((WCHAR)driveLetter);
}

BOOL WINAPI
EmptyRecycleBinW(
	IN WCHAR driveLetter)
{
	PRECYCLE_BIN bin = NULL;
	BOOL ret = FALSE;

	/* Open recycle bin */
	bin = IntReferenceRecycleBin(driveLetter);
	if (!bin)
		goto cleanup;

	if (bin->Callbacks.EmptyRecycleBin)
		ret = bin->Callbacks.EmptyRecycleBin(&bin);
	else
		SetLastError(ERROR_NOT_SUPPORTED);

cleanup:
	if (bin)
		DereferenceHandle(&bin->refCount);
	return ret;
}

BOOL WINAPI
EnumerateRecycleBinA(
	IN CHAR driveLetter,
	IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL)
{
	return EnumerateRecycleBinW((WCHAR)driveLetter, pFnCallback, Context);
}

static BOOL
IntCloseDeletedFileHandle(
	IN PREFCOUNT_DATA pData)
{
	PDELETED_FILE_HANDLE file;

	file = CONTAINING_RECORD(pData, DELETED_FILE_HANDLE, refCount);
	if (!DereferenceHandle(&file->bin->refCount))
		return FALSE;

	file->magic = 0;
	HeapFree(GetProcessHeap(), 0, file);
	return TRUE;
}

static BOOL
IntEnumerateRecycleBinCallback(
	IN PVOID Context,
	IN HANDLE hDeletedFile)
{
	PENUMERATE_RECYCLE_BIN_CONTEXT CallbackContext = (PENUMERATE_RECYCLE_BIN_CONTEXT)Context;
	PDELETED_FILE_HANDLE DeletedFileHandle = NULL;
	BOOL ret = FALSE;

	DeletedFileHandle = HeapAlloc(GetProcessHeap(), 0, sizeof(DELETED_FILE_HANDLE));
	if (!DeletedFileHandle)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto cleanup;
	}

	ReferenceHandle(&CallbackContext->bin->refCount);
	InitializeHandle(&DeletedFileHandle->refCount, IntCloseDeletedFileHandle);
	DeletedFileHandle->magic = DELETEDFILE_MAGIC;
	DeletedFileHandle->bin = CallbackContext->bin;
	DeletedFileHandle->hDeletedFile = hDeletedFile;

	ret = CallbackContext->pFnCallback(CallbackContext->Context, DeletedFileHandle);

cleanup:
	if (!ret)
	{
		if (DeletedFileHandle)
			DereferenceHandle(&DeletedFileHandle->refCount);
	}
	return ret;
}

BOOL WINAPI
EnumerateRecycleBinW(
	IN WCHAR driveLetter,
	IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL)
{
	PRECYCLE_BIN bin = NULL;
	BOOL ret = FALSE;

	/* Check parameters */
	if (pFnCallback == NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		goto cleanup;
	}

	/* Open recycle bin */
	bin = IntReferenceRecycleBin(driveLetter);
	if (!bin)
		goto cleanup;

	if (bin->Callbacks.EnumerateFiles)
	{
		ENUMERATE_RECYCLE_BIN_CONTEXT CallbackContext;

		CallbackContext.bin = bin;
		CallbackContext.pFnCallback = pFnCallback;
		CallbackContext.Context = Context;

		ret = bin->Callbacks.EnumerateFiles(bin, IntEnumerateRecycleBinCallback, &CallbackContext);
	}
	else
		SetLastError(ERROR_NOT_SUPPORTED);

cleanup:
	if (bin)
		DereferenceHandle(&bin->refCount);
	return ret;
}

BOOL WINAPI
GetDeletedFileDetailsA(
	IN HANDLE hDeletedFile,
	IN DWORD BufferSize,
	IN OUT PDELETED_FILE_DETAILS_A FileDetails OPTIONAL,
	OUT LPDWORD RequiredSize OPTIONAL)
{
	PDELETED_FILE_DETAILS_W FileDetailsW = NULL;
	DWORD BufferSizeW = 0;
	BOOL ret = FALSE;

	if (BufferSize >= FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName))
	{
		BufferSizeW = FIELD_OFFSET(DELETED_FILE_DETAILS_W, FileName)
			+ (BufferSize - FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName)) * sizeof(WCHAR);
	}
	if (FileDetails && BufferSizeW)
	{
		FileDetailsW = HeapAlloc(GetProcessHeap(), 0, BufferSizeW);
		if (!FileDetailsW)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto cleanup;
		}
	}

	ret = GetDeletedFileDetailsW(hDeletedFile, BufferSizeW, FileDetailsW, RequiredSize);
	if (!ret)
		goto cleanup;

	if (FileDetails)
	{
		memcpy(FileDetails, FileDetailsW, FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName));
		if (0 == WideCharToMultiByte(CP_ACP, 0, FileDetailsW->FileName, -1, FileDetails->FileName, BufferSize - FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName), NULL, NULL))
			goto cleanup;
	}
	ret = TRUE;

cleanup:
	HeapFree(GetProcessHeap(), 0, FileDetailsW);
	return ret;
}

BOOL WINAPI
GetDeletedFileDetailsW(
	IN HANDLE hDeletedFile,
	IN DWORD BufferSize,
	IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL,
	OUT LPDWORD RequiredSize OPTIONAL)
{
	BOOL ret = FALSE;

	if (!IntCheckDeletedFileHandle(hDeletedFile))
		SetLastError(ERROR_INVALID_HANDLE);
	else
	{
		PDELETED_FILE_HANDLE DeletedFile = (PDELETED_FILE_HANDLE)hDeletedFile;
		if (DeletedFile->bin->Callbacks.GetDetails)
			ret = DeletedFile->bin->Callbacks.GetDetails(DeletedFile->bin, DeletedFile->hDeletedFile, BufferSize, FileDetails, RequiredSize);
		else
			SetLastError(ERROR_NOT_SUPPORTED);
	}

	return ret;
}

BOOL WINAPI
RestoreFile(
	IN HANDLE hDeletedFile)
{
	BOOL ret = FALSE;

	if (!IntCheckDeletedFileHandle(hDeletedFile))
		SetLastError(ERROR_INVALID_HANDLE);
	else
	{
		PDELETED_FILE_HANDLE DeletedFile = (PDELETED_FILE_HANDLE)hDeletedFile;
		if (DeletedFile->bin->Callbacks.RestoreFile)
			ret = DeletedFile->bin->Callbacks.RestoreFile(DeletedFile->bin, DeletedFile->hDeletedFile);
		else
			SetLastError(ERROR_NOT_SUPPORTED);
	}

	return ret;
}
