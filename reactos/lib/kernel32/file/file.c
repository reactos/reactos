/* $Id: file.c,v 1.45 2003/07/10 18:50:51 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBALS ******************************************************************/

WINBOOL bIsFileApiAnsi = TRUE; // set the file api to ansi or oem


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
SetFileApisToOEM(VOID)
{
   bIsFileApiAnsi = FALSE;
}


/*
 * @implemented
 */
VOID STDCALL
SetFileApisToANSI(VOID)
{
   bIsFileApiAnsi = TRUE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
AreFileApisANSI(VOID)
{
   return bIsFileApiAnsi;
}


/*
 * @implemented
 */
HFILE STDCALL
OpenFile(LPCSTR lpFileName,
	 LPOFSTRUCT lpReOpenBuff,
	 UINT uStyle)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING FileNameString;
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	WCHAR PathNameW[MAX_PATH];
	HANDLE FileHandle = NULL;
	NTSTATUS errCode;
	PWCHAR FilePart;
	ULONG Len;

	DPRINT("OpenFile('%s', lpReOpenBuff %x, uStyle %x)\n", lpFileName, lpReOpenBuff, uStyle);

	if (lpReOpenBuff == NULL)
	{
		return FALSE;
	}

	RtlInitAnsiString (&FileName, (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU, &FileName, TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU, &FileName, TRUE);

	Len = SearchPathW (NULL,
	                   FileNameU.Buffer,
	                   NULL,
	                   OFS_MAXPATHNAME,
	                   PathNameW,
	                   &FilePart);

	RtlFreeUnicodeString(&FileNameU);

	if (Len == 0 || Len > OFS_MAXPATHNAME)
	{
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	FileName.Buffer = lpReOpenBuff->szPathName;
	FileName.Length = 0;
	FileName.MaximumLength = OFS_MAXPATHNAME;

	RtlInitUnicodeString(&FileNameU, PathNameW);

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&FileName, &FileNameU, FALSE);
	else
		RtlUnicodeStringToOemString (&FileName, &FileNameU, FALSE);

	if (!RtlDosPathNameToNtPathName_U ((LPWSTR)PathNameW,
					   &FileNameString,
					   NULL,
					   NULL))
	{
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &FileNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	// FILE_SHARE_READ
	// FILE_NO_INTERMEDIATE_BUFFERING

	if ((uStyle & OF_PARSE) == OF_PARSE)
	{
		RtlFreeUnicodeString(&FileNameString);
		return (HFILE)NULL;
	}

	errCode = NtOpenFile (&FileHandle,
	                      GENERIC_READ|SYNCHRONIZE,
	                      &ObjectAttributes,
	                      &IoStatusBlock,
	                      FILE_SHARE_READ,
	                      FILE_NON_DIRECTORY_FILE);

	RtlFreeUnicodeString(&FileNameString);

	lpReOpenBuff->nErrCode = RtlNtStatusToDosError(errCode);

	if (!NT_SUCCESS(errCode))
	{
		SetLastErrorByStatus (errCode);
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	return (HFILE)FileHandle;
}


/*
 * @implemented
 */
WINBOOL STDCALL
FlushFileBuffers(HANDLE hFile)
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;

   if (IsConsoleHandle(hFile))
   {
      return FALSE;
   }

   errCode = NtFlushBuffersFile(hFile,
				&IoStatusBlock);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return(FALSE);
     }
   return(TRUE);
}


/*
 * @implemented
 */
DWORD STDCALL
SetFilePointer(HANDLE hFile,
	       LONG lDistanceToMove,
	       PLONG lpDistanceToMoveHigh,
	       DWORD dwMoveMethod)
{
   FILE_POSITION_INFORMATION FilePosition;
   FILE_STANDARD_INFORMATION FileStandart;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   LARGE_INTEGER Distance;
   
   DPRINT("SetFilePointer(hFile %x, lDistanceToMove %d, dwMoveMethod %d)\n",
	  hFile,lDistanceToMove,dwMoveMethod);

   Distance.u.LowPart = lDistanceToMove;
   if (lpDistanceToMoveHigh)
   {
      Distance.u.HighPart = *lpDistanceToMoveHigh;
   }
   else if (lDistanceToMove >= 0)
   {
      Distance.u.HighPart = 0;
   }
   else
   {
      Distance.u.HighPart = -1;
   }

   if (dwMoveMethod == FILE_CURRENT)
     {
	NtQueryInformationFile(hFile,
			       &IoStatusBlock,
			       &FilePosition,
			       sizeof(FILE_POSITION_INFORMATION),
			       FilePositionInformation);
	FilePosition.CurrentByteOffset.QuadPart += Distance.QuadPart;
     }
   else if (dwMoveMethod == FILE_END)
     {
	NtQueryInformationFile(hFile,
                               &IoStatusBlock,
                               &FileStandart,
                               sizeof(FILE_STANDARD_INFORMATION),
                               FileStandardInformation);
        FilePosition.CurrentByteOffset.QuadPart =
                  FileStandart.EndOfFile.QuadPart + Distance.QuadPart;
     }
   else if ( dwMoveMethod == FILE_BEGIN )
     {
        FilePosition.CurrentByteOffset.QuadPart = Distance.QuadPart;
     }
   
   errCode = NtSetInformationFile(hFile,
				  &IoStatusBlock,
				  &FilePosition,
				  sizeof(FILE_POSITION_INFORMATION),
				  FilePositionInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return -1;
     }
   
   if (lpDistanceToMoveHigh != NULL)
     {
        *lpDistanceToMoveHigh = FilePosition.CurrentByteOffset.u.HighPart;
     }
   return FilePosition.CurrentByteOffset.u.LowPart;
}


/*
 * @implemented
 */
DWORD STDCALL
GetFileType(HANDLE hFile)
{
  FILE_FS_DEVICE_INFORMATION DeviceInfo;
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;

  /* Get real handle */
  switch ((ULONG)hFile)
    {
      case STD_INPUT_HANDLE:
	hFile = NtCurrentPeb()->ProcessParameters->hStdInput;
	break;

      case STD_OUTPUT_HANDLE:
	hFile = NtCurrentPeb()->ProcessParameters->hStdOutput;
	break;

      case STD_ERROR_HANDLE:
	hFile = NtCurrentPeb()->ProcessParameters->hStdError;
	break;
    }

  /* Check for console handle */
  if (IsConsoleHandle(hFile))
    {
      if (VerifyConsoleIoHandle(hFile))
	return FILE_TYPE_CHAR;
    }

  Status = NtQueryVolumeInformationFile(hFile,
					&StatusBlock,
					&DeviceInfo,
					sizeof(FILE_FS_DEVICE_INFORMATION),
					FileFsDeviceInformation);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FILE_TYPE_UNKNOWN;
    }

  switch (DeviceInfo.DeviceType)
    {
      case FILE_DEVICE_CD_ROM:
      case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
      case FILE_DEVICE_CONTROLLER:
      case FILE_DEVICE_DATALINK:
      case FILE_DEVICE_DFS:
      case FILE_DEVICE_DISK:
      case FILE_DEVICE_DISK_FILE_SYSTEM:
      case FILE_DEVICE_VIRTUAL_DISK:
	return FILE_TYPE_DISK;

      case FILE_DEVICE_KEYBOARD:
      case FILE_DEVICE_MOUSE:
      case FILE_DEVICE_NULL:
      case FILE_DEVICE_PARALLEL_PORT:
      case FILE_DEVICE_PRINTER:
      case FILE_DEVICE_SERIAL_PORT:
      case FILE_DEVICE_SCREEN:
      case FILE_DEVICE_SOUND:
      case FILE_DEVICE_MODEM:
	return FILE_TYPE_CHAR;

      case FILE_DEVICE_NAMED_PIPE:
	return FILE_TYPE_PIPE;
    }

  return FILE_TYPE_UNKNOWN;
}


/*
 * @implemented
 */
DWORD STDCALL
GetFileSize(HANDLE hFile,
	    LPDWORD lpFileSizeHigh)
{
   NTSTATUS errCode;
   FILE_STANDARD_INFORMATION FileStandard;
   IO_STATUS_BLOCK IoStatusBlock;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileStandard,
				    sizeof(FILE_STANDARD_INFORMATION),
				    FileStandardInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	if ( lpFileSizeHigh == NULL )
	  {
	     return -1;
	  }
	else
	  {
	     return 0;
	  }
     }
   if ( lpFileSizeHigh != NULL )
     *lpFileSizeHigh = FileStandard.EndOfFile.u.HighPart;

   return FileStandard.EndOfFile.u.LowPart;
}


/*
 * @implemented
 */
DWORD STDCALL
GetCompressedFileSizeA(LPCSTR lpFileName,
		       LPDWORD lpFileSizeHigh)
{
   UNICODE_STRING FileNameU;
   ANSI_STRING FileName;
   DWORD Size;

   RtlInitAnsiString(&FileName,
		     (LPSTR)lpFileName);

   /* convert ansi (or oem) string to unicode */
   if (bIsFileApiAnsi)
     RtlAnsiStringToUnicodeString(&FileNameU,
				  &FileName,
				  TRUE);
   else
     RtlOemStringToUnicodeString(&FileNameU,
				 &FileName,
				 TRUE);

   Size = GetCompressedFileSizeW(FileNameU.Buffer,
				 lpFileSizeHigh);

   RtlFreeUnicodeString (&FileNameU);

   return Size;
}


/*
 * @implemented
 */
DWORD STDCALL
GetCompressedFileSizeW(LPCWSTR lpFileName,
		       LPDWORD lpFileSizeHigh)
{
   FILE_COMPRESSION_INFORMATION FileCompression;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   HANDLE hFile;
   
   hFile = CreateFileW(lpFileName,
		       GENERIC_READ,
		       FILE_SHARE_READ,
		       NULL,
		       OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL,
		       NULL);

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileCompression,
				    sizeof(FILE_COMPRESSION_INFORMATION),
				    FileCompressionInformation);
   if (!NT_SUCCESS(errCode))
     {
	CloseHandle(hFile);
	SetLastErrorByStatus(errCode);
	return INVALID_FILE_SIZE;
     }
   CloseHandle(hFile);

   if(lpFileSizeHigh)
    *lpFileSizeHigh = FileCompression.CompressedFileSize.u.HighPart;

   return FileCompression.CompressedFileSize.u.LowPart;
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetFileInformationByHandle(HANDLE hFile,
			   LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   struct
   {
        FILE_FS_VOLUME_INFORMATION FileFsVolume;
        WCHAR Name[255];
   }
   FileFsVolume;

   FILE_BASIC_INFORMATION FileBasic;
   FILE_INTERNAL_INFORMATION FileInternal;
   FILE_STANDARD_INFORMATION FileStandard;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileBasic,
				    sizeof(FILE_BASIC_INFORMATION),
				    FileBasicInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->dwFileAttributes = (DWORD)FileBasic.FileAttributes;
   memcpy(&lpFileInformation->ftCreationTime,&FileBasic.CreationTime,sizeof(LARGE_INTEGER));
   memcpy(&lpFileInformation->ftLastAccessTime,&FileBasic.LastAccessTime,sizeof(LARGE_INTEGER));
   memcpy(&lpFileInformation->ftLastWriteTime, &FileBasic.LastWriteTime,sizeof(LARGE_INTEGER));

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileInternal,
				    sizeof(FILE_INTERNAL_INFORMATION),
				    FileInternalInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->nFileIndexHigh = FileInternal.IndexNumber.u.HighPart;
   lpFileInformation->nFileIndexLow = FileInternal.IndexNumber.u.LowPart;

   errCode = NtQueryVolumeInformationFile(hFile,
					  &IoStatusBlock,
					  &FileFsVolume,
					  sizeof(FileFsVolume),
					  FileFsVolumeInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->dwVolumeSerialNumber = FileFsVolume.FileFsVolume.VolumeSerialNumber;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileStandard,
				    sizeof(FILE_STANDARD_INFORMATION),
				    FileStandardInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   lpFileInformation->nNumberOfLinks = FileStandard.NumberOfLinks;
   lpFileInformation->nFileSizeHigh = FileStandard.EndOfFile.u.HighPart;
   lpFileInformation->nFileSizeLow = FileStandard.EndOfFile.u.LowPart;

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetFileAttributesExW(LPCWSTR lpFileName, 
		     GET_FILEEX_INFO_LEVELS fInfoLevelId, 
		     LPVOID lpFileInformation)
{
  FILE_NETWORK_OPEN_INFORMATION FileInformation;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING FileName;
  HANDLE FileHandle;
  NTSTATUS Status;
  WIN32_FILE_ATTRIBUTE_DATA* FileAttributeData;

  DPRINT ("GetFileAttributesExW(%S) called\n", lpFileName);


  if (fInfoLevelId != GetFileExInfoStandard || lpFileInformation == NULL)
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return FALSE;
  }

  /* Validate and translate the filename */
  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				     &FileName,
				     NULL,
				     NULL))
    {
      DPRINT ("Invalid path\n");
      SetLastError (ERROR_BAD_PATHNAME);
      return FALSE;
    }

  /* build the object attributes */
  InitializeObjectAttributes (&ObjectAttributes,
			      &FileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  /* Open the file */
  Status = NtOpenFile (&FileHandle,
		       SYNCHRONIZE | FILE_READ_ATTRIBUTES,
		       &ObjectAttributes,
		       &IoStatusBlock,
		       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		       FILE_SYNCHRONOUS_IO_NONALERT);
  RtlFreeUnicodeString (&FileName);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtOpenFile() failed (Status %lx)\n", Status);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  /* Get file attributes */
  Status = NtQueryInformationFile (FileHandle,
				   &IoStatusBlock,
				   &FileInformation,
				   sizeof(FILE_NETWORK_OPEN_INFORMATION),
				   FileNetworkOpenInformation);
  NtClose (FileHandle);

  if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtQueryInformationFile() failed (Status %lx)\n", Status);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  FileAttributeData = (WIN32_FILE_ATTRIBUTE_DATA*)lpFileInformation;
  FileAttributeData->dwFileAttributes = FileInformation.FileAttributes;
  FileAttributeData->ftCreationTime.dwLowDateTime = FileInformation.CreationTime.u.LowPart;
  FileAttributeData->ftCreationTime.dwHighDateTime = FileInformation.CreationTime.u.HighPart;
  FileAttributeData->ftLastAccessTime.dwLowDateTime = FileInformation.LastAccessTime.u.LowPart;
  FileAttributeData->ftLastAccessTime.dwHighDateTime = FileInformation.LastAccessTime.u.HighPart;
  FileAttributeData->ftLastWriteTime.dwLowDateTime = FileInformation.LastWriteTime.u.LowPart;
  FileAttributeData->ftLastWriteTime.dwHighDateTime = FileInformation.LastWriteTime.u.HighPart;
  FileAttributeData->nFileSizeLow = FileInformation.EndOfFile.u.LowPart;
  FileAttributeData->nFileSizeHigh = FileInformation.EndOfFile.u.HighPart;

  return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
GetFileAttributesExA(LPCSTR lpFileName,
		     GET_FILEEX_INFO_LEVELS fInfoLevelId, 
		     LPVOID lpFileInformation)
{
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	BOOL Result;
	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU,
		                              &FileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU,
		                             &FileName,
		                             TRUE);

        Result = GetFileAttributesExW(FileNameU.Buffer, fInfoLevelId, lpFileInformation);

	RtlFreeUnicodeString (&FileNameU);

	return Result;
}


/*
 * @implemented
 */
DWORD STDCALL
GetFileAttributesA(LPCSTR lpFileName)
{
        WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
	UNICODE_STRING FileNameU;
	ANSI_STRING FileName;
	BOOL Result;

	RtlInitAnsiString (&FileName,
	                   (LPSTR)lpFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&FileNameU,
		                              &FileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&FileNameU,
		                             &FileName,
		                             TRUE);

        Result = GetFileAttributesExW(FileNameU.Buffer, GetFileExInfoStandard, &FileAttributeData);

	RtlFreeUnicodeString (&FileNameU);

	return Result ? FileAttributeData.dwFileAttributes : 0xffffffff;
}


/*
 * @implemented
 */
DWORD STDCALL
GetFileAttributesW(LPCWSTR lpFileName)
{
  WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
  BOOL Result;

  DPRINT ("GetFileAttributeW(%S) called\n", lpFileName);

  Result = GetFileAttributesExW(lpFileName, GetFileExInfoStandard, &FileAttributeData);

  return Result ? FileAttributeData.dwFileAttributes : 0xffffffff;
}

WINBOOL STDCALL
SetFileAttributesA(LPCSTR lpFileName,
		   DWORD dwFileAttributes)
{
  UNICODE_STRING FileNameU;
  ANSI_STRING FileName;
  WINBOOL Result;

  RtlInitAnsiString (&FileName,
		     (LPSTR)lpFileName);

  /* convert ansi (or oem) string to unicode */
  if (bIsFileApiAnsi)
    RtlAnsiStringToUnicodeString (&FileNameU,
				  &FileName,
				  TRUE);
   else
    RtlOemStringToUnicodeString (&FileNameU,
				 &FileName,
				 TRUE);

  Result = SetFileAttributesW (FileNameU.Buffer,
			       dwFileAttributes);

  RtlFreeUnicodeString (&FileNameU);

  return Result;
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetFileAttributesW(LPCWSTR lpFileName,
		   DWORD dwFileAttributes)
{
  FILE_BASIC_INFORMATION FileInformation;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING FileName;
  HANDLE FileHandle;
  NTSTATUS Status;

  DPRINT ("SetFileAttributeW(%S, 0x%lx) called\n", lpFileName, dwFileAttributes);

  /* Validate and translate the filename */
  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				     &FileName,
				     NULL,
				     NULL))
    {
      DPRINT ("Invalid path\n");
      SetLastError (ERROR_BAD_PATHNAME);
      return FALSE;
    }
  DPRINT ("FileName: \'%wZ\'\n", &FileName);

  /* build the object attributes */
  InitializeObjectAttributes (&ObjectAttributes,
			      &FileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  /* Open the file */
  Status = NtOpenFile (&FileHandle,
		       SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
		       &ObjectAttributes,
		       &IoStatusBlock,
		       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		       FILE_SYNCHRONOUS_IO_NONALERT);
  RtlFreeUnicodeString (&FileName);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtOpenFile() failed (Status %lx)\n", Status);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  Status = NtQueryInformationFile(FileHandle,
				  &IoStatusBlock,
				  &FileInformation,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT ("SetFileAttributes NtQueryInformationFile failed with status 0x%08x\n", Status);
      NtClose (FileHandle);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  FileInformation.FileAttributes = dwFileAttributes;
  Status = NtSetInformationFile(FileHandle,
				&IoStatusBlock,
				&FileInformation,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
  NtClose (FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT ("SetFileAttributes NtSetInformationFile failed with status 0x%08x\n", Status);
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
UINT STDCALL
GetTempFileNameA(LPCSTR lpPathName,
		 LPCSTR lpPrefixString,
		 UINT uUnique,
		 LPSTR lpTempFileName)
{
   HANDLE hFile;
   UINT unique = uUnique;
   UINT len;
   const char *format = "%.*s\\~%.3s%4.4x.TMP";

   DPRINT("GetTempFileNameA(lpPathName %s, lpPrefixString %.*s, "
	  "uUnique %x, lpTempFileName %x)\n", lpPathName, 4, 
	  lpPrefixString, uUnique, lpTempFileName);
  
   if (lpPathName == NULL)
     return 0;

   len = strlen(lpPathName);
   if (len > 0 && (lpPathName[len-1] == '\\' || lpPathName[len-1] == '/'))
     len--;

   if (uUnique == 0)
     uUnique = GetCurrentTime();
   
   sprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,uUnique);
   
   if (unique)
     return uUnique;
   
   while ((hFile = CreateFileA(lpTempFileName, GENERIC_WRITE, 0, NULL,
			       CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			       0)) == INVALID_HANDLE_VALUE)
   {
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return 0;
      }
      sprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,++uUnique);
   }
   CloseHandle(hFile);
   return uUnique;
}


/*
 * @implemented
 */
UINT STDCALL
GetTempFileNameW(LPCWSTR lpPathName,
		 LPCWSTR lpPrefixString,
		 UINT uUnique,
		 LPWSTR lpTempFileName)
{
   HANDLE hFile;
   UINT unique = uUnique;
   UINT len;
   const WCHAR *format = L"%.*S\\~%.3S%4.4x.TMP";
   
   DPRINT("GetTempFileNameW(lpPathName %S, lpPrefixString %.*S, "
	  "uUnique %x, lpTempFileName %x)\n", lpPathName, 4, 
	  lpPrefixString, uUnique, lpTempFileName);

   if (lpPathName == NULL)
     return 0;

   len = wcslen(lpPathName);
   if (len > 0 && (lpPathName[len-1] == L'\\' || lpPathName[len-1] == L'/'))
     len--;
   
   if (uUnique == 0)
     uUnique = GetCurrentTime();
   
   swprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,uUnique);
   
   if (unique)
     return uUnique;
  
   while ((hFile = CreateFileW(lpTempFileName, GENERIC_WRITE, 0, NULL,
			       CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY,
			       0)) == INVALID_HANDLE_VALUE)
   {
      if (GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return 0;
      }
      swprintf(lpTempFileName,format,len,lpPathName,lpPrefixString,++uUnique);
   }
   CloseHandle(hFile);
   return uUnique;
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetFileTime(HANDLE hFile,
	    LPFILETIME lpCreationTime,
	    LPFILETIME lpLastAccessTime,
	    LPFILETIME lpLastWriteTime)
{
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;
   NTSTATUS Status;

   Status = NtQueryInformationFile(hFile,
				   &IoStatusBlock,
				   &FileBasic,
				   sizeof(FILE_BASIC_INFORMATION),
				   FileBasicInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   if (lpCreationTime)
     memcpy(lpCreationTime, &FileBasic.CreationTime, sizeof(FILETIME));
   if (lpLastAccessTime)
     memcpy(lpLastAccessTime, &FileBasic.LastAccessTime, sizeof(FILETIME));
   if (lpLastWriteTime)
     memcpy(lpLastWriteTime, &FileBasic.LastWriteTime, sizeof(FILETIME));

   return TRUE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetFileTime(HANDLE hFile,
	    CONST FILETIME *lpCreationTime,
	    CONST FILETIME *lpLastAccessTime,
	    CONST FILETIME *lpLastWriteTime)
{
   FILE_BASIC_INFORMATION FileBasic;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   Status = NtQueryInformationFile(hFile,
				   &IoStatusBlock,
				   &FileBasic,
				   sizeof(FILE_BASIC_INFORMATION),
				   FileBasicInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   if (lpCreationTime)
     memcpy(&FileBasic.CreationTime, lpCreationTime, sizeof(FILETIME));
   if (lpLastAccessTime)
     memcpy(&FileBasic.LastAccessTime, lpLastAccessTime, sizeof(FILETIME));
   if (lpLastWriteTime)
     memcpy(&FileBasic.LastWriteTime, lpLastWriteTime, sizeof(FILETIME));

   // should i initialize changetime ???

   Status = NtSetInformationFile(hFile,
				 &IoStatusBlock,
				 &FileBasic,
				 sizeof(FILE_BASIC_INFORMATION),
				 FileBasicInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   
   return TRUE;
}


/*
 * The caller must have opened the file with the DesiredAccess FILE_WRITE_DATA flag set.
 *
 * @implemented
 */
WINBOOL STDCALL
SetEndOfFile(HANDLE hFile)
{
	IO_STATUS_BLOCK  IoStatusBlock;
	FILE_END_OF_FILE_INFORMATION	EndOfFileInfo;
	FILE_ALLOCATION_INFORMATION		FileAllocationInfo;
	FILE_POSITION_INFORMATION		 FilePosInfo;
	NTSTATUS Status;

	//get current position
	Status = NtQueryInformationFile(
					hFile,
					&IoStatusBlock,
					&FilePosInfo,
					sizeof(FILE_POSITION_INFORMATION),
					FilePositionInformation
					);

	if (!NT_SUCCESS(Status)){
		SetLastErrorByStatus(Status);
		return FALSE;
	}

	EndOfFileInfo.EndOfFile.QuadPart = FilePosInfo.CurrentByteOffset.QuadPart;

	/*
	NOTE: 
	This call is not supposed to free up any space after the eof marker
	if the file gets truncated. We have to deallocate the space explicitly afterwards.
	But...most file systems dispatch both FileEndOfFileInformation 
	and FileAllocationInformation as they were the same	command.

	*/
	Status = NtSetInformationFile(
						hFile,
						&IoStatusBlock,	 //out
						&EndOfFileInfo,
						sizeof(FILE_END_OF_FILE_INFORMATION),
						FileEndOfFileInformation
						);

	if (!NT_SUCCESS(Status)){
		SetLastErrorByStatus(Status);
		return FALSE;
	}

	FileAllocationInfo.AllocationSize.QuadPart = FilePosInfo.CurrentByteOffset.QuadPart;


	Status = NtSetInformationFile(
						hFile,
						&IoStatusBlock,	 //out
						&FileAllocationInfo,
						sizeof(FILE_ALLOCATION_INFORMATION),
						FileAllocationInformation
						);

	if (!NT_SUCCESS(Status)){
		SetLastErrorByStatus(Status);
		return FALSE;
	}
	
	return TRUE;

}

/* EOF */
