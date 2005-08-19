/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Gerhard W. Gruber (sparhawk_at_gmx.at)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#include <malloc.h>

#define NDEBUG
#include "../include/debug.h"

/* GLOBALS *****************************************************************/

/* FUNCTIONS ****************************************************************/

static BOOL
AdjustFileAttributes (
	LPCWSTR ExistingFileName,
	LPCWSTR NewFileName
	)
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_BASIC_INFORMATION ExistingInfo,
		NewInfo;
	HANDLE hFile;
	DWORD Attributes;
	NTSTATUS errCode;
	BOOL Result = FALSE;

	hFile = CreateFileW (ExistingFileName,
	                     FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
	                     FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		errCode = NtQueryInformationFile (hFile,
		                                  &IoStatusBlock,
		                                  &ExistingInfo,
		                                  sizeof(FILE_BASIC_INFORMATION),
		                                  FileBasicInformation);
		if (NT_SUCCESS (errCode))
		{
			if (0 != (ExistingInfo.FileAttributes & FILE_ATTRIBUTE_READONLY))
			{
				Attributes = ExistingInfo.FileAttributes;
				ExistingInfo.FileAttributes &= ~ FILE_ATTRIBUTE_READONLY;
				if (0 == (ExistingInfo.FileAttributes &
				          (FILE_ATTRIBUTE_HIDDEN |
				           FILE_ATTRIBUTE_SYSTEM |
				           FILE_ATTRIBUTE_ARCHIVE)))
				{
					ExistingInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
				}
				errCode = NtSetInformationFile (hFile,
				                                &IoStatusBlock,
				                                &ExistingInfo,
				                                sizeof(FILE_BASIC_INFORMATION),
				                                FileBasicInformation);
				if (!NT_SUCCESS(errCode))
				{
					DPRINT("Removing READONLY attribute from source failed with status 0x%08x\n", errCode);
				}
				ExistingInfo.FileAttributes = Attributes;
			}
			CloseHandle(hFile);

			if (NT_SUCCESS(errCode))
			{
				hFile = CreateFileW (NewFileName,
				                     FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
				                     FILE_SHARE_READ,
				                     NULL,
				                     OPEN_EXISTING,
			        	             FILE_ATTRIBUTE_NORMAL,
				                     NULL);
				if (INVALID_HANDLE_VALUE != hFile)
				{
					errCode = NtQueryInformationFile(hFile,
					                                 &IoStatusBlock,
					                                 &NewInfo,
					                                 sizeof(FILE_BASIC_INFORMATION),
					                                 FileBasicInformation);
					if (NT_SUCCESS(errCode))
					{
						NewInfo.FileAttributes = (NewInfo.FileAttributes &
						                          ~ (FILE_ATTRIBUTE_HIDDEN |
						                             FILE_ATTRIBUTE_SYSTEM |
						                             FILE_ATTRIBUTE_READONLY |
						                             FILE_ATTRIBUTE_NORMAL)) |
					                                 (ExistingInfo.FileAttributes &
					                                  (FILE_ATTRIBUTE_HIDDEN |
						                           FILE_ATTRIBUTE_SYSTEM |
						                           FILE_ATTRIBUTE_READONLY |
						                           FILE_ATTRIBUTE_NORMAL)) |
						                         FILE_ATTRIBUTE_ARCHIVE;
						NewInfo.CreationTime = ExistingInfo.CreationTime;
						NewInfo.LastAccessTime = ExistingInfo.LastAccessTime;
						NewInfo.LastWriteTime = ExistingInfo.LastWriteTime;
						errCode = NtSetInformationFile (hFile,
						                                &IoStatusBlock,
						                                &NewInfo,
						                                sizeof(FILE_BASIC_INFORMATION),
						                                FileBasicInformation);
						if (NT_SUCCESS(errCode))
						{
							Result = TRUE;
						}
						else
						{
							DPRINT("Setting attributes on dest file failed with status 0x%08x\n", errCode);
						}
					}
					else
					{
						DPRINT("Obtaining attributes from dest file failed with status 0x%08x\n", errCode);
					}
					CloseHandle(hFile);
				}
				else
				{
					DPRINT("Opening dest file to set attributes failed with code %d\n", GetLastError());
				}
			}
		}
		else
		{
			DPRINT("Obtaining attributes from source file failed with status 0x%08x\n", errCode);
			CloseHandle(hFile);
		}
	}
	else
	{
		DPRINT("Opening source file to obtain attributes failed with code %d\n", GetLastError());
	}

	return Result;
}

/***********************************************************************
 *           add_boot_rename_entry
 *
 * Adds an entry to the registry that is loaded when windows boots and
 * checks if there are some files to be removed or renamed/moved.
 * <fn1> has to be valid and <fn2> may be NULL. If both pointers are
 * non-NULL then the file is moved, otherwise it is deleted.  The
 * entry of the registrykey is always appended with two zero
 * terminated strings. If <fn2> is NULL then the second entry is
 * simply a single 0-byte. Otherwise the second filename goes
 * there. The entries are prepended with \??\ before the path and the
 * second filename gets also a '!' as the first character if
 * MOVEFILE_REPLACE_EXISTING is set. After the final string another
 * 0-byte follows to indicate the end of the strings.
 * i.e.:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * \??\D:\test\file2[0]
 * !\??\D:\test\file2_renamed[0]
 * [0]                        <- indicates end of strings
 *
 * or:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * [0]                        <- indicates end of strings
 *
 */
static BOOL add_boot_rename_entry( LPCWSTR source, LPCWSTR dest, DWORD flags )
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, source_name, dest_name;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HANDLE Reboot = 0;
    DWORD len1, len2;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;

    DPRINT("Add support to smss for keys created by MOVEFILE_DELAY_UNTIL_REBOOT\n");

    if (!RtlDosPathNameToNtPathName_U( (LPWSTR)source, &source_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    dest_name.Buffer = NULL;
    if (dest && !RtlDosPathNameToNtPathName_U( (LPWSTR)dest, &dest_name, NULL, NULL ))
    {
        RtlFreeUnicodeString( &source_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, SessionW );

    if (NtCreateKey( &Reboot, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)
    {
        DPRINT1("Error creating key for reboot managment [%s]\n",
             "SYSTEM\\CurrentControlSet\\Control\\Session Manager");
        RtlFreeUnicodeString( &source_name );
        RtlFreeUnicodeString( &dest_name );
        return FALSE;
    }

    len1 = source_name.Length + sizeof(WCHAR);
    if (dest)
    {
        len2 = dest_name.Length + sizeof(WCHAR);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            len2 += sizeof(WCHAR); /* Plus 1 because of the leading '!' */
    }
    else len2 = sizeof(WCHAR); /* minimum is the 0 characters for the empty second string */

    RtlInitUnicodeString( &nameW, ValueName );

    /* First we check if the key exists and if so how many bytes it already contains. */
    if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                         NULL, 0, &DataSize ) == STATUS_BUFFER_OVERFLOW)
    {
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
        if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                             Buffer, DataSize, &DataSize )) goto Quit;
        info = (KEY_VALUE_PARTIAL_INFORMATION *)Buffer;
        if (info->Type != REG_MULTI_SZ) goto Quit;
        if (DataSize > sizeof(info)) DataSize -= sizeof(WCHAR);  /* remove terminating null (will be added back later) */
    }
    else
    {
        DataSize = info_size;
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
    }

    memcpy( Buffer + DataSize, source_name.Buffer, len1 );
    DataSize += len1;
    p = (WCHAR *)(Buffer + DataSize);
    if (dest)
    {
        if (flags & MOVEFILE_REPLACE_EXISTING)
            *p++ = '!';
        memcpy( p, dest_name.Buffer, len2 );
        DataSize += len2;
    }
    else
    {
        *p = 0;
        DataSize += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(Buffer + DataSize);
    *p = 0;
    DataSize += sizeof(WCHAR);

    rc = !NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size);

 Quit:
    RtlFreeUnicodeString( &source_name );
    RtlFreeUnicodeString( &dest_name );
    if (Reboot) NtClose(Reboot);
    HeapFree( GetProcessHeap(), 0, Buffer );
    return(rc);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressW (
	LPCWSTR			lpExistingFileName,
	LPCWSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	PFILE_RENAME_INFORMATION FileRename;
	NTSTATUS errCode;
	BOOL Result;
	UNICODE_STRING DstPathU;

	DPRINT("MoveFileWithProgressW()\n");

	if (dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT)
		return add_boot_rename_entry( lpExistingFileName, lpNewFileName, dwFlags );

	hFile = CreateFileW (lpExistingFileName,
	                     GENERIC_ALL,
	                     FILE_SHARE_WRITE|FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
           return FALSE;
	}

        /* validate & translate the filename */
        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewFileName,
				           &DstPathU,
				           NULL,
				           NULL))
        {
           DPRINT("Invalid destination path\n");
	   CloseHandle(hFile);
           SetLastError(ERROR_PATH_NOT_FOUND);
           return FALSE;
        }

	FileRename = alloca(sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length);
	if ((dwFlags & MOVEFILE_REPLACE_EXISTING) == MOVEFILE_REPLACE_EXISTING)
		FileRename->ReplaceIfExists = TRUE;
	else
		FileRename->ReplaceIfExists = FALSE;

	memcpy(FileRename->FileName, DstPathU.Buffer, DstPathU.Length);
        RtlFreeHeap (RtlGetProcessHeap (),
		     0,
		     DstPathU.Buffer);
	/*
	 * FIXME:
	 *   Is the length the count of characters or the length of the buffer?
	 */
	FileRename->FileNameLength = DstPathU.Length / sizeof(WCHAR);
	errCode = NtSetInformationFile (hFile,
	                                &IoStatusBlock,
	                                FileRename,
	                                sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length,
	                                FileRenameInformation);
	CloseHandle(hFile);
	if (NT_SUCCESS(errCode))
	{
		Result = TRUE;
	}
	else if (STATUS_NOT_SAME_DEVICE == errCode &&
		 MOVEFILE_COPY_ALLOWED == (dwFlags & MOVEFILE_COPY_ALLOWED))
	{
		Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
		if (Result)
		{
			/* Cleanup the source file */
			AdjustFileAttributes(lpExistingFileName, lpNewFileName);
	                Result = DeleteFileW (lpExistingFileName);
		}
	}
#if 1
	/* FIXME file rename not yet implemented in all FSDs so it will always
	 * fail, even when the move is to the same device
	 */
	else if (STATUS_NOT_IMPLEMENTED == errCode)
	{

		UNICODE_STRING SrcPathU;

		SrcPathU.Buffer = alloca(sizeof(WCHAR) * MAX_PATH);
		SrcPathU.MaximumLength = MAX_PATH * sizeof(WCHAR);
		SrcPathU.Length = GetFullPathNameW(lpExistingFileName, MAX_PATH, SrcPathU.Buffer, NULL);
		if (SrcPathU.Length >= MAX_PATH)
		{
		    SetLastError(ERROR_FILENAME_EXCED_RANGE);
		    return FALSE;
		}
		SrcPathU.Length *= sizeof(WCHAR);

		DstPathU.Buffer = alloca(sizeof(WCHAR) * MAX_PATH);
		DstPathU.MaximumLength = MAX_PATH * sizeof(WCHAR);
		DstPathU.Length = GetFullPathNameW(lpNewFileName, MAX_PATH, DstPathU.Buffer, NULL);
		if (DstPathU.Length >= MAX_PATH)
		{
		    SetLastError(ERROR_FILENAME_EXCED_RANGE);
		    return FALSE;
		}
		DstPathU.Length *= sizeof(WCHAR);

		if (0 == RtlCompareUnicodeString(&SrcPathU, &DstPathU, TRUE))
		{
		   /* Source and destination file are the same, nothing to do */
		   return TRUE;
		}

		Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
		if (Result)
		{
		    /* Cleanup the source file */
                    AdjustFileAttributes(lpExistingFileName, lpNewFileName);
		    Result = DeleteFileW (lpExistingFileName);
		}
	}
#endif
	else
	{
		SetLastErrorByStatus (errCode);
		Result = FALSE;
	}
	return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressA (
	LPCSTR			lpExistingFileName,
	LPCSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	PWCHAR ExistingFileNameW;
   PWCHAR NewFileNameW;
	BOOL ret;

   if (!(ExistingFileNameW = FilenameA2W(lpExistingFileName, FALSE)))
      return FALSE;

   if (!(NewFileNameW= FilenameA2W(lpNewFileName, TRUE)))
      return FALSE;

   ret = MoveFileWithProgressW (ExistingFileNameW ,
                                   NewFileNameW,
	                                lpProgressRoutine,
	                                lpData,
	                                dwFlags);

   RtlFreeHeap (RtlGetProcessHeap (), 0, NewFileNameW);

	return ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName
	)
{
	return MoveFileExW (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileExW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	return MoveFileWithProgressW (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName
	)
{
	return MoveFileExA (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileExA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	return MoveFileWithProgressA (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}

/* EOF */
