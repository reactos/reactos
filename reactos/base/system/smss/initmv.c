/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/initmv.c
 * PURPOSE:         Process the file rename list.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

/*++
* @name SmpDeleteFile
*
* The SmpDeleteFile function deletes a specify file.
*
* @param    lpFileName
*           the name of a file which should be deleted
*
* @return   STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
*           othwerwise.
*
* @remarks
* This function is called by SmpMoveFilesQueryRoutine().
*
*
*--*/
NTSTATUS
SmpDeleteFile( IN LPCWSTR lpFileName )
{
	FILE_DISPOSITION_INFORMATION FileDispInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING FileNameU;
	HANDLE FileHandle;
	NTSTATUS Status;

	DPRINT("SmpDeleteFile ( %S )\n", lpFileName);

	if( !lpFileName )
		return (STATUS_INVALID_PARAMETER);

	RtlInitUnicodeString(&FileNameU, lpFileName);

	InitializeObjectAttributes(&ObjectAttributes,
		&FileNameU,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = NtCreateFile (&FileHandle,
		DELETE,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if( !NT_SUCCESS(Status) ) {
		DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
		return (Status);
	}

	FileDispInfo.DeleteFile = TRUE;

	Status = NtSetInformationFile(
		FileHandle,
		&IoStatusBlock,
		&FileDispInfo,
		sizeof(FILE_DISPOSITION_INFORMATION),
		FileDispositionInformation );

	NtClose(FileHandle);

	return (Status);
}


/*++
* @name SmpMoveFile
*
* The SmpMoveFile function deletes a specify file.
*
* @param	lpExistingFileName
*			the name of an existing file which should be removed
*
* @param	lpNewFileName
*			a new name of an existing file.
*
* @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
*         othwerwise.
*
* @remarks
* This function called from the SmpMoveFilesQueryRoutine function.
*
*
*--*/
NTSTATUS
SmpMoveFile( IN LPCWSTR lpExistingFileName,
			 IN LPCWSTR	lpNewFileName
			 )
{
	PFILE_RENAME_INFORMATION FileRenameInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING ExistingFileNameU;
	HANDLE FileHandle;
	DWORD FileNameSize;
	BOOLEAN ReplaceIfExists;
	NTSTATUS Status;

	if( !lpExistingFileName || !lpNewFileName )
		return (STATUS_INVALID_PARAMETER);

	DPRINT("SmpMoveFile (%S, %S)\n", lpExistingFileName, lpNewFileName);

	RtlInitUnicodeString(&ExistingFileNameU, lpExistingFileName);

	InitializeObjectAttributes(&ObjectAttributes,
		&ExistingFileNameU,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = NtCreateFile (&FileHandle,
		FILE_ALL_ACCESS,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if( !NT_SUCCESS(Status) ) {
		DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
		return (Status);
	}

	FileNameSize = wcslen(lpNewFileName)*sizeof(*lpNewFileName);
	FileRenameInfo = RtlAllocateHeap(
		RtlGetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(FILE_RENAME_INFORMATION)+FileNameSize);
	if( !FileRenameInfo ) {
		DPRINT("RtlAllocateHeap failed\n");
		NtClose(FileHandle);
		return (STATUS_NO_MEMORY);
	}

	if( L'!' == *lpNewFileName ) {
		lpNewFileName++;
		FileNameSize -= sizeof(*lpNewFileName);
		ReplaceIfExists = TRUE;
	}
	else {
		ReplaceIfExists = FALSE;
	}

	FileRenameInfo->RootDirectory = NULL;
	FileRenameInfo->ReplaceIfExists = ReplaceIfExists;
	FileRenameInfo->FileNameLength = FileNameSize;
	RtlCopyMemory(FileRenameInfo->FileName, lpNewFileName, FileNameSize);

	Status = NtSetInformationFile(
		FileHandle,
		&IoStatusBlock,
		FileRenameInfo,
		sizeof(FILE_RENAME_INFORMATION)+FileNameSize,
		FileRenameInformation );

	RtlFreeHeap(RtlGetProcessHeap(), 0, FileRenameInfo);

	/* FIXME: After the FileRenameInformation parameter will be implemented into the fs driver
	the following code can be removed */
	if( STATUS_NOT_IMPLEMENTED == Status )
	{
		HANDLE FileHandleNew;
		UNICODE_STRING NewFileNameU;
		FILE_BASIC_INFORMATION FileBasicInfo;
		UCHAR *lpBuffer = NULL;
		SIZE_T RegionSize = 0x10000;
		LARGE_INTEGER BytesCopied;
		BOOL EndOfFileFound;

		Status = NtQueryInformationFile(
			FileHandle,
			&IoStatusBlock,
			&FileBasicInfo,
			sizeof(FILE_BASIC_INFORMATION),
			FileBasicInformation);
		if( !NT_SUCCESS(Status) ) {
			DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
			NtClose(FileHandle);
			return (Status);
		}

		RtlInitUnicodeString(&NewFileNameU, lpNewFileName);

		InitializeObjectAttributes(&ObjectAttributes,
			&NewFileNameU,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);

		Status = NtCreateFile (&FileHandleNew,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			ReplaceIfExists ? FILE_OVERWRITE_IF : FILE_CREATE,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);

		if( !NT_SUCCESS(Status) ) {
			DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
			NtClose(FileHandle);
			return (Status);
		}

		Status = NtAllocateVirtualMemory(
			NtCurrentProcess(),
			(PVOID *)&lpBuffer,
			2,
			&RegionSize,
			MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE);
		if( !NT_SUCCESS(Status) ) {
			DPRINT("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
			NtClose(FileHandle);
			SmpDeleteFile(lpNewFileName);
			return (Status);
		}
		BytesCopied.QuadPart = 0;
		EndOfFileFound = FALSE;
		while( !EndOfFileFound
			&& NT_SUCCESS(Status) )
		{
			Status = NtReadFile(FileHandle,
				NULL,
				NULL,
				NULL,
				&IoStatusBlock,
				lpBuffer,
				RegionSize,
				NULL,
				NULL);
			if( NT_SUCCESS(Status) ) {
				Status = NtWriteFile(FileHandleNew,
					NULL,
					NULL,
					NULL,
					&IoStatusBlock,
					lpBuffer,
					IoStatusBlock.Information,
					NULL,
					NULL);
				if( NT_SUCCESS(Status) ) {
					BytesCopied.QuadPart += IoStatusBlock.Information;
				}
				else {
					DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
				}
			}
			else {
				if( STATUS_END_OF_FILE == Status ) {
					EndOfFileFound = TRUE;
					Status = STATUS_SUCCESS;
				}
				else {
					DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
				}
			}
		}

		NtFreeVirtualMemory(NtCurrentProcess(),
			(PVOID *)&lpBuffer,
			&RegionSize,
			MEM_RELEASE);

		Status = NtQueryInformationFile(
			FileHandleNew,
			&IoStatusBlock,
			&FileBasicInfo,
			sizeof(FILE_BASIC_INFORMATION),
			FileBasicInformation);
		if( !NT_SUCCESS(Status) ) {
			DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
		}

		Status = NtSetInformationFile(FileHandleNew,
			&IoStatusBlock,
			&FileBasicInfo,
			sizeof(FILE_BASIC_INFORMATION),
			FileBasicInformation);
		if( !NT_SUCCESS(Status) ) {
			DPRINT("NtSetInformationFile() failed (Status %lx)\n", Status);
		}
		NtClose(FileHandleNew);
		NtClose(FileHandle);

		SmpDeleteFile(lpExistingFileName);

		return (Status);
	}

	NtClose(FileHandle);

	return (Status);
}


/*++
* @name SmpMoveFilesQueryRoutine
*
* The SmpMoveFilesQueryRoutine function processes registry entries.
*
* @param    ValueName
*           The name of the value.
*
* @param    ValueType
*           The type of the value.
*
* @param    ValueData
*           The null-terminated data for the value.
*
* @param    ValueLength
*           The length of ValueData.
*
* @param    Context
*           NULL
*
* @param    EntryContext
*           NULL
*
* @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
*         othwerwise.
*
* @remarks
*
*
*
*--*/
static NTSTATUS NTAPI
SmpMoveFilesQueryRoutine(IN PWSTR ValueName,
                         IN ULONG ValueType,
                         IN PVOID ValueData,
                         IN ULONG ValueLength,
                         IN PVOID Context,
                         IN PVOID EntryContext)
{
	NTSTATUS Status;
	static LPWSTR FistFileName = NULL;

	DPRINT("SmpMoveFilesQueryRoutine() called \n");
	DPRINT("ValueData = %S \n", (PWSTR) ValueData);

	if( !FistFileName )
	{
		/* save a first file name */
		FistFileName = ValueData;
		Status = STATUS_SUCCESS;
	}
	else
	{
		if( 0 == *((LPWSTR)ValueData) ) {
			/* delete if second file name is absent */
			Status = SmpDeleteFile( FistFileName );
		} else {
			/* remove a file */
			Status = SmpMoveFile( FistFileName, (LPCWSTR)ValueData );
		}
		FistFileName = NULL;
	}

	return Status;
}


/*++
* @name SmProcessFileRenameList
* @implemented
*
* The SmProcessFileRenameList function moves or deletes files thats have been added to the specify registry key for delayed moving.
*
* @param	VOID
*
* @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
*         othwerwise.
*
* @remarks
* This function reads the following registry value:
* HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\PendingFileRenameOperations
* This registry value is of type REG_MULTI_SZ. The each operation is specifed as two file names.
* A first name is a source file, a second name is a destination file.
* In the case of deleting operation a second file name must be the empty string.
* For exapmle:
* szxSrcFile\0szxDestFile\0\0		<-- the szxSrcFile file will be renamed to the szxDestFile file
* szxSomeFile\0\0\0					<-- the szxSomeFile file will be removed
* After it will be done, the registry value will be deleted.
*
*
*--*/
NTSTATUS
SmProcessFileRenameList( VOID )
{
	RTL_QUERY_REGISTRY_TABLE QueryTable[2];
	NTSTATUS Status;

	DPRINT("SmProcessFileRenameList() called\n");

	RtlZeroMemory( &QueryTable, sizeof(QueryTable) );
	QueryTable[0].Name = L"PendingFileRenameOperations";
	QueryTable[0].Flags = RTL_QUERY_REGISTRY_DELETE;
	QueryTable[0].DefaultType = REG_NONE;
	QueryTable[0].QueryRoutine = SmpMoveFilesQueryRoutine;

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_CONTROL,
		L"\\Session Manager",
		QueryTable,
		NULL,
		NULL);

	if( !NT_SUCCESS(Status) ) {
		DPRINT("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
	}

	/* FIXME: RtlQueryRegistryValues can return an error status if the PendingFileRenameOperations value
	does not exist, in this case smss hungs, therefore we always return STATUS_SUCCESS */
	return (STATUS_SUCCESS);
}

/* EOF */
