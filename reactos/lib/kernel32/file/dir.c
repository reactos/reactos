/* $Id: dir.c,v 1.36 2003/05/06 06:49:57 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/*
 * NOTES: Changed to using ZwCreateFile
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS *****************************************************************/

WINBOOL
STDCALL
CreateDirectoryA (
	LPCSTR			lpPathName,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	return CreateDirectoryExA (NULL,
	                           lpPathName,
	                           lpSecurityAttributes);
}

WINBOOL
STDCALL
CreateDirectoryExA (
	LPCSTR			lpTemplateDirectory,
	LPCSTR			lpNewDirectory,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes)
{
	UNICODE_STRING TmplDirU;
	UNICODE_STRING NewDirU;
	ANSI_STRING TmplDir;
	ANSI_STRING NewDir;
	WINBOOL Result;

	RtlInitUnicodeString (&TmplDirU,
	                      NULL);

	RtlInitUnicodeString (&NewDirU,
	                      NULL);

	if (lpTemplateDirectory != NULL)
	{
		RtlInitAnsiString (&TmplDir,
		                   (LPSTR)lpTemplateDirectory);

		/* convert ansi (or oem) string to unicode */
		if (bIsFileApiAnsi)
			RtlAnsiStringToUnicodeString (&TmplDirU,
			                              &TmplDir,
			                              TRUE);
		else
			RtlOemStringToUnicodeString (&TmplDirU,
			                             &TmplDir,
			                             TRUE);
	}

	if (lpNewDirectory != NULL)
	{
		RtlInitAnsiString (&NewDir,
		                   (LPSTR)lpNewDirectory);

		/* convert ansi (or oem) string to unicode */
		if (bIsFileApiAnsi)
			RtlAnsiStringToUnicodeString (&NewDirU,
			                              &NewDir,
			                              TRUE);
		else
			RtlOemStringToUnicodeString (&NewDirU,
			                             &NewDir,
			                             TRUE);
	}

	Result = CreateDirectoryExW (TmplDirU.Buffer,
	                             NewDirU.Buffer,
	                             lpSecurityAttributes);

	if (lpTemplateDirectory != NULL)
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             TmplDirU.Buffer);

	if (lpNewDirectory != NULL)
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             NewDirU.Buffer);

	return Result;
}


WINBOOL
STDCALL
CreateDirectoryW (
	LPCWSTR			lpPathName,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	return CreateDirectoryExW (NULL,
	                           lpPathName,
	                           lpSecurityAttributes);
}


WINBOOL
STDCALL
CreateDirectoryExW (
	LPCWSTR			lpTemplateDirectory,
	LPCWSTR			lpNewDirectory,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	HANDLE DirectoryHandle;
	NTSTATUS Status;

	DPRINT ("lpTemplateDirectory %S lpNewDirectory %S lpSecurityAttributes %p\n",
	        lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);
  
  // Can't create empty directory
  if(lpNewDirectory == NULL || *lpNewDirectory == 0)
  {
    SetLastError(ERROR_PATH_NOT_FOUND);
    return FALSE;
  }

	if (lpTemplateDirectory != NULL && *lpTemplateDirectory != 0)
	{
		// get object attributes from template directory
		DPRINT("KERNEL32:FIXME:%s:%d\n",__FILE__,__LINE__);
		return FALSE;
	}

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewDirectory,
	                                   &NtPathU,
	                                   NULL,
	                                   NULL))
		return FALSE;

	DPRINT1 ("NtPathU \'%wZ\'\n", &NtPathU);

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &NtPathU;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	Status = NtCreateFile (&DirectoryHandle,
	                       DIRECTORY_ALL_ACCESS,
	                       &ObjectAttributes,
	                       &IoStatusBlock,
	                       NULL,
	                       FILE_ATTRIBUTE_DIRECTORY,
	                       0,
	                       FILE_CREATE,
	                       FILE_DIRECTORY_FILE,
	                       NULL,
	                       0);
	DPRINT("Status: %lx\n", Status);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             NtPathU.Buffer);

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus(Status);
		return FALSE;
	}

	NtClose (DirectoryHandle);

	return TRUE;
}


WINBOOL
STDCALL
RemoveDirectoryA (
	LPCSTR	lpPathName
	)
{
	UNICODE_STRING PathNameU;
	ANSI_STRING PathName;
	WINBOOL Result;

	RtlInitAnsiString (&PathName,
	                   (LPSTR)lpPathName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&PathNameU,
		                              &PathName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&PathNameU,
		                             &PathName,
		                             TRUE);

	Result = RemoveDirectoryW (PathNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             PathNameU.Buffer);

	return Result;
}


WINBOOL
STDCALL
RemoveDirectoryW (
	LPCWSTR	lpPathName
	)
{
	FILE_DISPOSITION_INFORMATION FileDispInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING NtPathU;
	HANDLE DirectoryHandle;
	NTSTATUS Status;

	DPRINT("lpPathName %S\n", lpPathName);

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpPathName,
	                                   &NtPathU,
	                                   NULL,
	                                   NULL))
		return FALSE;

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &NtPathU;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	DPRINT("NtPathU '%S'\n", NtPathU.Buffer);

	Status = NtCreateFile (&DirectoryHandle,
	                       FILE_WRITE_ATTRIBUTES,    /* 0x110080 */
	                       &ObjectAttributes,
	                       &IoStatusBlock,
	                       NULL,
	                       FILE_ATTRIBUTE_DIRECTORY, /* 0x7 */
	                       0,
	                       FILE_OPEN,
	                       FILE_DIRECTORY_FILE,      /* 0x204021 */
	                       NULL,
	                       0);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             NtPathU.Buffer);

	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	FileDispInfo.DoDeleteFile = TRUE;

	Status = NtSetInformationFile (DirectoryHandle,
	                               &IoStatusBlock,
	                               &FileDispInfo,
	                               sizeof(FILE_DISPOSITION_INFORMATION),
	                               FileDispositionInformation);
	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		NtClose(DirectoryHandle);
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	Status = NtClose (DirectoryHandle);
	if (!NT_SUCCESS(Status))
	{
		CHECKPOINT;
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


DWORD
STDCALL
GetFullPathNameA (
	LPCSTR	lpFileName,
	DWORD	nBufferLength,
	LPSTR	lpBuffer,
	LPSTR	*lpFilePart
	)
{
	UNICODE_STRING FileNameU;
	UNICODE_STRING FullNameU;
	ANSI_STRING FileName;
	ANSI_STRING FullName;
	PWSTR FilePartU;
	ULONG BufferLength;
	ULONG Offset;
	DWORD FullNameLen;

	DPRINT("GetFullPathNameA(lpFileName %s, nBufferLength %d, lpBuffer %p, "
	       "lpFilePart %p)\n",lpFileName,nBufferLength,lpBuffer,lpFilePart);

	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);

	RtlAnsiStringToUnicodeString (&FileNameU,
	                              &FileName,
	                              TRUE);

	BufferLength = nBufferLength * sizeof(WCHAR);

	FullNameU.MaximumLength = BufferLength;
	FullNameU.Length = 0;
	FullNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                    0,
	                                    BufferLength);

	FullNameU.Length = RtlGetFullPathName_U (FileNameU.Buffer,
	                                         BufferLength,
	                                         FullNameU.Buffer,
	                                         &FilePartU);

	RtlFreeUnicodeString (&FileNameU);

	FullName.MaximumLength = nBufferLength;
	FullName.Length = 0;
	FullName.Buffer = lpBuffer;

        if (lpBuffer != NULL )
	{
		RtlUnicodeStringToAnsiString (&FullName,
					&FullNameU,
					FALSE);

		if (lpFilePart != NULL)
		{
			Offset = (ULONG)(FilePartU - FullNameU.Buffer);
			*lpFilePart = FullName.Buffer + Offset;
		}
	}

	FullNameLen = FullNameU.Length / sizeof(WCHAR);
	
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             FullNameU.Buffer);

	DPRINT("lpBuffer %s lpFilePart %s Length %ld\n",
	       lpBuffer, lpFilePart, FullName.Length);

	return FullNameLen;
}


DWORD
STDCALL
GetFullPathNameW (
	LPCWSTR	lpFileName,
	DWORD	nBufferLength,
	LPWSTR	lpBuffer,
	LPWSTR	*lpFilePart
	)
{
	ULONG Length;

	DPRINT("GetFullPathNameW(lpFileName %S, nBufferLength %d, lpBuffer %p, "
	       "lpFilePart %p)\n",lpFileName,nBufferLength,lpBuffer,lpFilePart);

	Length = RtlGetFullPathName_U ((LPWSTR)lpFileName,
	                               nBufferLength * sizeof(WCHAR),
	                               lpBuffer,
	                               lpFilePart);

	DPRINT("lpBuffer %S lpFilePart %S Length %ld\n",
	       lpBuffer, lpFilePart, Length / sizeof(WCHAR));

	return (Length / sizeof(WCHAR));
}


DWORD
STDCALL
GetShortPathNameA (
	LPCSTR	lpszLongPath,
	LPSTR	lpszShortPath,
	DWORD	cchBuffer
	)
{
	//1 remove unicode chars and spaces
	//2 remove preceding and trailing periods.
	//3 remove embedded periods except the last one
	
	//4 Split the string in two parts before and after the period
	//	truncate the part before the period to 6 chars and add ~1
	//	truncate the part after the period to 3 chars
	//3 Put the new name in uppercase
	
	//4 Increment the ~1 string if the resulting name allready exists

	return 0;
}


DWORD
STDCALL
GetShortPathNameW (
	LPCWSTR	lpszLongPath,
	LPWSTR	lpszShortPath,
	DWORD	cchBuffer
	)
{
	return 0;
}


DWORD
STDCALL
SearchPathA (
	LPCSTR	lpPath,
	LPCSTR	lpFileName,
	LPCSTR	lpExtension,
	DWORD	nBufferLength,
	LPSTR	lpBuffer,
	LPSTR	*lpFilePart
	)
{
	UNICODE_STRING PathU;
	UNICODE_STRING FileNameU;
	UNICODE_STRING ExtensionU;
	UNICODE_STRING BufferU;
	ANSI_STRING Path;
	ANSI_STRING FileName;
	ANSI_STRING Extension;
	ANSI_STRING Buffer;
	PWCHAR FilePartW;
	DWORD RetValue;

	RtlInitAnsiString (&Path,
	                   (LPSTR)lpPath);
	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);
	RtlInitAnsiString (&Extension,
	                   (LPSTR)lpExtension);

	/* convert ansi (or oem) strings to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&PathU,
		                              &Path,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&FileNameU,
		                              &FileName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&ExtensionU,
		                              &Extension,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&PathU,
		                             &Path,
		                             TRUE);
		RtlOemStringToUnicodeString (&FileNameU,
		                             &FileName,
		                             TRUE);
		RtlOemStringToUnicodeString (&ExtensionU,
		                             &Extension,
		                             TRUE);
	}

	BufferU.Length = 0;
	BufferU.MaximumLength = nBufferLength * sizeof(WCHAR);
	BufferU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                  0,
	                                  BufferU.MaximumLength);

	Buffer.Length = 0;
	Buffer.MaximumLength = nBufferLength;
	Buffer.Buffer = lpBuffer;

	RetValue = SearchPathW (PathU.Buffer,
	                        FileNameU.Buffer,
	                        ExtensionU.Buffer,
	                        nBufferLength,
	                        BufferU.Buffer,
	                        &FilePartW);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             PathU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             FileNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             ExtensionU.Buffer);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&Buffer,
		                              &BufferU,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&Buffer,
		                             &BufferU,
		                             FALSE);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             BufferU.Buffer);

	*lpFilePart = strrchr (lpBuffer, '\\') + 1;

	return RetValue;
}


DWORD
STDCALL
SearchPathW (
	LPCWSTR	lpPath,
	LPCWSTR	lpFileName,
	LPCWSTR	lpExtension,
	DWORD	nBufferLength,
	LPWSTR	lpBuffer,
	LPWSTR	*lpFilePart
	)
/*
 * FUNCTION: Searches for the specified file
 * ARGUMENTS:
 *       lpPath = Points to a null-terminated string that specified the
 *                path to be searched. If this parameters is NULL then
 *                the following directories are searched
 *                          The directory from which the application loaded
 *                          The current directory
 *                          The system directory
 *                          The 16-bit system directory
 *                          The windows directory
 *                          The directories listed in the PATH environment
 *                          variable
 *        lpFileName = Specifies the filename to search for
 *        lpExtension = Points to the null-terminated string that specifies
 *                      an extension to be added to the filename when
 *                      searching for the file. The first character of the
 *                      filename extension must be a period (.). The
 *                      extension is only added if the specified filename
 *                      doesn't end with an extension
 *                      
 *                      If the filename extension is not required or if the
 *                      filename contains an extension, this parameters can be
 *                      NULL
 *        nBufferLength = The length in characters of the buffer for output
 *        lpBuffer = Points to the buffer for the valid path and filename of
 *                   file found
 *        lpFilePart = Points to the last component of the valid path and
 *                     filename 
 * RETURNS: On success, the length, in characters, of the string copied to the
 *          buffer
 *          On failure, zero.
 */
{
	DWORD retCode = 0;
	ULONG pos, len;
	PWCHAR EnvironmentBufferW = NULL;
	WCHAR Buffer;

	DPRINT("SearchPath\n");

	if (lpPath == NULL)
	{
		len = GetEnvironmentVariableW(L"PATH", &Buffer, 0);
		len += 1 + GetCurrentDirectoryW(0, &Buffer);
		len += 1 + GetSystemDirectoryW(&Buffer, 0);
		len += 1 + GetWindowsDirectoryW(&Buffer, 0);

		EnvironmentBufferW = (PWCHAR) RtlAllocateHeap(GetProcessHeap(), 
			                        HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, 
						len * sizeof(WCHAR));
		if (EnvironmentBufferW == NULL)
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return 0;
		}

		pos = GetCurrentDirectoryW(len, EnvironmentBufferW);
		EnvironmentBufferW[pos++] = L';';
		EnvironmentBufferW[pos] = 0;
		pos += GetSystemDirectoryW(&EnvironmentBufferW[pos], len - pos);
		EnvironmentBufferW[pos++] = L';';
		EnvironmentBufferW[pos] = 0;
		pos += GetWindowsDirectoryW(&EnvironmentBufferW[pos], len - pos);
		EnvironmentBufferW[pos++] = L';';
		EnvironmentBufferW[pos] = 0;
		pos += GetEnvironmentVariableW(L"PATH", &EnvironmentBufferW[pos], len - pos);
		lpPath = EnvironmentBufferW;
	}

	retCode = RtlDosSearchPath_U ((PWCHAR)lpPath, (PWCHAR)lpFileName, (PWCHAR)lpExtension, 
	                              nBufferLength * sizeof(WCHAR), lpBuffer, lpFilePart);

	if (EnvironmentBufferW != NULL)
	{
		RtlFreeHeap(GetProcessHeap(), 0, EnvironmentBufferW);
	}
	if (retCode == 0)
	{
		SetLastError(ERROR_FILE_NOT_FOUND);
	}
	return retCode / sizeof(WCHAR);
}

/* EOF */
