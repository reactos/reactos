/* Recycle bin management
 * This file is under the GPLv2 licence
 * Copyright (C) 2006 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "recyclebin_private.h"

#include <pshpack1.h>

/* MS Windows 2000/XP/2003 */
typedef struct _DELETED_FILE_RECORD
{
	CHAR FileNameA[MAX_PATH];
	DWORD dwRecordUniqueId;
	DWORD dwDriveNumber;
	FILETIME DeletionTime;
	DWORD dwPhysicalFileSize;
	WCHAR FileNameW[MAX_PATH];
} DELETED_FILE_RECORD, *PDELETED_FILE_RECORD;

#include <poppack.h>

static BOOL
CloseHandle5(
	IN HANDLE hDeletedFile);

static BOOL
DeleteFile5(
	IN PRECYCLE_BIN bin,
	IN LPCWSTR FullPath,
	IN LPCWSTR FileName);

static BOOL
EmptyRecycleBin5(
	IN PRECYCLE_BIN* bin);

static BOOL
EnumerateFiles5(
	IN PRECYCLE_BIN bin,
	IN PINT_ENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL);

static BOOL
GetDetails5(
	IN PRECYCLE_BIN bin,
	IN HANDLE hDeletedFile,
	IN DWORD BufferSize,
	IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL,
	OUT LPDWORD RequiredSize OPTIONAL);

static BOOL
RestoreFile5(
	IN PRECYCLE_BIN bin,
	IN HANDLE hDeletedFile);

static BOOL
IntDeleteRecursive(
	IN LPCWSTR FullName);

static BOOL
IntEmptyRecycleBinCallback(
	IN PVOID Context,
	IN HANDLE hDeletedFile);

static BOOL
IntGetFullName(
	IN PRECYCLE_BIN bin,
	IN PDELETED_FILE_RECORD pDeletedFile,
	OUT LPWSTR* pFullName);

static BOOL
IntSearchRecord(
	IN PRECYCLE_BIN bin,
	IN HANDLE hDeletedFile,
	OUT PDELETED_FILE_RECORD DeletedFile,
	OUT PLARGE_INTEGER Position OPTIONAL);
