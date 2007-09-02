/* $Id$
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
#include <debug.h>


/* GLOBALS ******************************************************************/

BOOL bIsFileApiAnsi = TRUE; // set the file api to ansi or oem

/* FUNCTIONS ****************************************************************/



PWCHAR
FilenameA2W(LPCSTR NameA, BOOL alloc)
{
   ANSI_STRING str;
   UNICODE_STRING strW;
   PUNICODE_STRING pstrW;
   NTSTATUS Status;

   //ASSERT(NtCurrentTeb()->StaticUnicodeString.Buffer == NtCurrentTeb()->StaticUnicodeBuffer);
   ASSERT(NtCurrentTeb()->StaticUnicodeString.MaximumLength == sizeof(NtCurrentTeb()->StaticUnicodeBuffer));

   RtlInitAnsiString(&str, NameA);
   pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;

   if (bIsFileApiAnsi)
        Status= RtlAnsiStringToUnicodeString( pstrW, &str, (BOOLEAN)alloc );
   else
        Status= RtlOemStringToUnicodeString( pstrW, &str, (BOOLEAN)alloc );

    if (NT_SUCCESS(Status))
       return pstrW->Buffer;

    if (Status== STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        SetLastErrorByStatus(Status);

    return NULL;
}


/*
No copy/conversion is done if the dest. buffer is too small.

Returns:
   Success: number of TCHARS copied into dest. buffer NOT including nullterm
   Fail: size of buffer in TCHARS required to hold the converted filename, including nullterm
*/
DWORD
FilenameU2A_FitOrFail(
   LPSTR  DestA,
   INT destLen, /* buffer size in TCHARS incl. nullchar */
   PUNICODE_STRING SourceU
   )
{
   DWORD ret;

   ret = bIsFileApiAnsi? RtlUnicodeStringToAnsiSize(SourceU) : RtlUnicodeStringToOemSize(SourceU);
   /* ret incl. nullchar */

   if (DestA && (INT)ret <= destLen)
   {
      ANSI_STRING str;

      str.Buffer = DestA;
      str.MaximumLength = (USHORT)destLen;


      if (bIsFileApiAnsi)
         RtlUnicodeStringToAnsiString(&str, SourceU, FALSE );
      else
         RtlUnicodeStringToOemString(&str, SourceU, FALSE );

      ret = str.Length;  /* SUCCESS: length without terminating 0 */
   }

   return ret;
}


/*
No copy/conversion is done if the dest. buffer is too small.

Returns:
   Success: number of TCHARS copied into dest. buffer NOT including nullterm
   Fail: size of buffer in TCHARS required to hold the converted filename, including nullterm
*/
DWORD
FilenameW2A_FitOrFail(
   LPSTR  DestA,
   INT destLen, /* buffer size in TCHARS incl. nullchar */
   LPCWSTR SourceW,
   INT sourceLen /* buffer size in TCHARS incl. nullchar */
   )
{
   UNICODE_STRING strW;

   if (sourceLen < 0) sourceLen = wcslen(SourceW) + 1;

   strW.Buffer = (PWCHAR)SourceW;
   strW.MaximumLength = sourceLen * sizeof(WCHAR);
   strW.Length = strW.MaximumLength - sizeof(WCHAR);

   return FilenameU2A_FitOrFail(DestA, destLen, &strW);
}


/*
Return: num. TCHARS copied into dest including nullterm
*/
DWORD
FilenameA2W_N(
   LPWSTR dest,
   INT destlen, /* buffer size in TCHARS incl. nullchar */
   LPCSTR src,
   INT srclen /* buffer size in TCHARS incl. nullchar */
   )
{
    DWORD ret;

    if (srclen < 0) srclen = strlen( src ) + 1;

    if (bIsFileApiAnsi)
        RtlMultiByteToUnicodeN( dest, destlen* sizeof(WCHAR), &ret, (LPSTR)src, srclen  );
    else
        RtlOemToUnicodeN( dest, destlen* sizeof(WCHAR), &ret, (LPSTR)src, srclen );

    if (ret) dest[(ret/sizeof(WCHAR))-1]=0;

    return ret/sizeof(WCHAR);
}

/*
Return: num. TCHARS copied into dest including nullterm
*/
DWORD
FilenameW2A_N(
   LPSTR dest,
   INT destlen, /* buffer size in TCHARS incl. nullchar */
   LPCWSTR src,
   INT srclen /* buffer size in TCHARS incl. nullchar */
   )
{
    DWORD ret;

    if (srclen < 0) srclen = wcslen( src ) + 1;

    if (bIsFileApiAnsi)
        RtlUnicodeToMultiByteN( dest, destlen, &ret, (LPWSTR) src, srclen * sizeof(WCHAR));
    else
        RtlUnicodeToOemN( dest, destlen, &ret, (LPWSTR) src, srclen * sizeof(WCHAR) );

    if (ret) dest[ret-1]=0;

    return ret;
}


/*
 * @implemented
 */
VOID
STDCALL
SetFileApisToOEM(VOID)
{
    /* Set the correct Base Api */
    Basep8BitStringToUnicodeString = (PRTL_CONVERT_STRING)RtlOemStringToUnicodeString;

    /* FIXME: Old, deprecated way */
    bIsFileApiAnsi = FALSE;
}


/*
 * @implemented
 */
VOID
STDCALL
SetFileApisToANSI(VOID)
{
    /* Set the correct Base Api */
    Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;

    /* FIXME: Old, deprecated way */
    bIsFileApiAnsi = TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
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

	if ((uStyle & OF_CREATE) == OF_CREATE)
	{
		DWORD Sharing;
		switch (uStyle & 0x70)
		{
			case OF_SHARE_EXCLUSIVE: Sharing = 0; break;
			case OF_SHARE_DENY_WRITE: Sharing = FILE_SHARE_READ; break;
			case OF_SHARE_DENY_READ: Sharing = FILE_SHARE_WRITE; break;
			case OF_SHARE_DENY_NONE:
			case OF_SHARE_COMPAT:
			default:
				Sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}
		return (HFILE) CreateFileA (lpFileName,
		                            GENERIC_READ | GENERIC_WRITE,
		                            Sharing,
		                            NULL,
		                            CREATE_ALWAYS,
		                            FILE_ATTRIBUTE_NORMAL,
		                            0);
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

	if (!RtlDosPathNameToNtPathName_U (PathNameW,
					   &FileNameString,
					   NULL,
					   NULL))
	{
		return (HFILE)INVALID_HANDLE_VALUE;
	}

	// FILE_SHARE_READ
	// FILE_NO_INTERMEDIATE_BUFFERING

	if ((uStyle & OF_PARSE) == OF_PARSE)
	{
		RtlFreeHeap(RtlGetProcessHeap(),
                            0,
                            FileNameString.Buffer);
		return (HFILE)NULL;
	}

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &FileNameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	errCode = NtOpenFile (&FileHandle,
	                      GENERIC_READ|SYNCHRONIZE,
	                      &ObjectAttributes,
	                      &IoStatusBlock,
	                      FILE_SHARE_READ,
	                      FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

	RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    FileNameString.Buffer);

	lpReOpenBuff->nErrCode = (WORD)RtlNtStatusToDosError(errCode);

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
BOOL STDCALL
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
   FILE_STANDARD_INFORMATION FileStandard;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;
   LARGE_INTEGER Distance;

   DPRINT("SetFilePointer(hFile %x, lDistanceToMove %d, dwMoveMethod %d)\n",
	  hFile,lDistanceToMove,dwMoveMethod);

   if(IsConsoleHandle(hFile))
   {
     SetLastError(ERROR_INVALID_HANDLE);
     return -1;
   }

   if (lpDistanceToMoveHigh)
   {
      Distance.u.HighPart = *lpDistanceToMoveHigh;
      Distance.u.LowPart = lDistanceToMove;
   }
   else
   {
      Distance.QuadPart = lDistanceToMove;
   }

   switch(dwMoveMethod)
   {
     case FILE_CURRENT:
	NtQueryInformationFile(hFile,
			       &IoStatusBlock,
			       &FilePosition,
			       sizeof(FILE_POSITION_INFORMATION),
			       FilePositionInformation);
	FilePosition.CurrentByteOffset.QuadPart += Distance.QuadPart;
	break;
     case FILE_END:
	NtQueryInformationFile(hFile,
                               &IoStatusBlock,
                               &FileStandard,
                               sizeof(FILE_STANDARD_INFORMATION),
                               FileStandardInformation);
        FilePosition.CurrentByteOffset.QuadPart =
                  FileStandard.EndOfFile.QuadPart + Distance.QuadPart;
	break;
     case FILE_BEGIN:
        FilePosition.CurrentByteOffset.QuadPart = Distance.QuadPart;
	break;
     default:
        SetLastError(ERROR_INVALID_PARAMETER);
	return -1;
   }

   if(FilePosition.CurrentByteOffset.QuadPart < 0)
   {
     SetLastError(ERROR_NEGATIVE_SEEK);
     return -1;
   }

   if (lpDistanceToMoveHigh == NULL && FilePosition.CurrentByteOffset.HighPart != 0)
   {
     /* If we're moving the pointer outside of the 32 bit boundaries but
        the application only passed a 32 bit value we need to bail out! */
     SetLastError(ERROR_INVALID_PARAMETER);
     return -1;
   }

   errCode = NtSetInformationFile(hFile,
				  &IoStatusBlock,
				  &FilePosition,
				  sizeof(FILE_POSITION_INFORMATION),
				  FilePositionInformation);
   if (!NT_SUCCESS(errCode))
     {
       if (lpDistanceToMoveHigh != NULL)
           *lpDistanceToMoveHigh = -1;

       SetLastErrorByStatus(errCode);
       return -1;
     }

   if (lpDistanceToMoveHigh != NULL)
     {
        *lpDistanceToMoveHigh = FilePosition.CurrentByteOffset.u.HighPart;
     }

   if (FilePosition.CurrentByteOffset.u.LowPart == -1)
     {
       /* The value of -1 is valid here, especially when the new
          file position is greater than 4 GB. Since NtSetInformationFile
          succeeded we never set an error code and we explicitly need
          to clear a previously set error code in this case, which
          an application will check if INVALID_SET_FILE_POINTER is returned! */
       SetLastError(ERROR_SUCCESS);
     }

   return FilePosition.CurrentByteOffset.u.LowPart;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetFilePointerEx(HANDLE hFile,
		 LARGE_INTEGER liDistanceToMove,
		 PLARGE_INTEGER lpNewFilePointer,
		 DWORD dwMoveMethod)
{
   FILE_POSITION_INFORMATION FilePosition;
   FILE_STANDARD_INFORMATION FileStandard;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatusBlock;

   if(IsConsoleHandle(hFile))
   {
     SetLastError(ERROR_INVALID_HANDLE);
     return FALSE;
   }

   switch(dwMoveMethod)
   {
     case FILE_CURRENT:
	NtQueryInformationFile(hFile,
			       &IoStatusBlock,
			       &FilePosition,
			       sizeof(FILE_POSITION_INFORMATION),
			       FilePositionInformation);
	FilePosition.CurrentByteOffset.QuadPart += liDistanceToMove.QuadPart;
	break;
     case FILE_END:
	NtQueryInformationFile(hFile,
                               &IoStatusBlock,
                               &FileStandard,
                               sizeof(FILE_STANDARD_INFORMATION),
                               FileStandardInformation);
        FilePosition.CurrentByteOffset.QuadPart =
                  FileStandard.EndOfFile.QuadPart + liDistanceToMove.QuadPart;
	break;
     case FILE_BEGIN:
        FilePosition.CurrentByteOffset.QuadPart = liDistanceToMove.QuadPart;
	break;
     default:
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
   }

   if(FilePosition.CurrentByteOffset.QuadPart < 0)
   {
     SetLastError(ERROR_NEGATIVE_SEEK);
     return FALSE;
   }

   errCode = NtSetInformationFile(hFile,
				  &IoStatusBlock,
				  &FilePosition,
				  sizeof(FILE_POSITION_INFORMATION),
				  FilePositionInformation);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return FALSE;
     }

   if (lpNewFilePointer)
     {
       *lpNewFilePointer = FilePosition.CurrentByteOffset;
     }
   return TRUE;
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
	hFile = NtCurrentPeb()->ProcessParameters->StandardInput;
	break;

      case STD_OUTPUT_HANDLE:
	hFile = NtCurrentPeb()->ProcessParameters->StandardOutput;
	break;

      case STD_ERROR_HANDLE:
	hFile = NtCurrentPeb()->ProcessParameters->StandardError;
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
BOOL
STDCALL
GetFileSizeEx(
    HANDLE hFile,
    PLARGE_INTEGER lpFileSize
    )
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
	return FALSE;
     }
   if (lpFileSize)
     *lpFileSize = FileStandard.EndOfFile;

   return TRUE;
}


/*
 * @implemented
 */
DWORD STDCALL
GetCompressedFileSizeA(LPCSTR lpFileName,
		       LPDWORD lpFileSizeHigh)
{
   PWCHAR FileNameW;

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return INVALID_FILE_SIZE;

   return GetCompressedFileSizeW(FileNameW, lpFileSizeHigh);
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

   if (hFile == INVALID_HANDLE_VALUE)
      return INVALID_FILE_SIZE;

   errCode = NtQueryInformationFile(hFile,
				    &IoStatusBlock,
				    &FileCompression,
				    sizeof(FILE_COMPRESSION_INFORMATION),
				    FileCompressionInformation);

   CloseHandle(hFile);

   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus(errCode);
	return INVALID_FILE_SIZE;
     }

   if(lpFileSizeHigh)
    *lpFileSizeHigh = FileCompression.CompressedFileSize.u.HighPart;

   SetLastError(NO_ERROR);
   return FileCompression.CompressedFileSize.u.LowPart;
}


/*
 * @implemented
 */
BOOL STDCALL
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

   if(IsConsoleHandle(hFile))
   {
     SetLastError(ERROR_INVALID_HANDLE);
     return FALSE;
   }

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

   lpFileInformation->ftCreationTime.dwHighDateTime = FileBasic.CreationTime.u.HighPart;
   lpFileInformation->ftCreationTime.dwLowDateTime = FileBasic.CreationTime.u.LowPart;

   lpFileInformation->ftLastAccessTime.dwHighDateTime = FileBasic.LastAccessTime.u.HighPart;
   lpFileInformation->ftLastAccessTime.dwLowDateTime = FileBasic.LastAccessTime.u.LowPart;

   lpFileInformation->ftLastWriteTime.dwHighDateTime = FileBasic.LastWriteTime.u.HighPart;
   lpFileInformation->ftLastWriteTime.dwLowDateTime = FileBasic.LastWriteTime.u.LowPart;

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
  UNICODE_STRING FileName;
  NTSTATUS Status;
  WIN32_FILE_ATTRIBUTE_DATA* FileAttributeData;

  DPRINT("GetFileAttributesExW(%S) called\n", lpFileName);


  if (fInfoLevelId != GetFileExInfoStandard || lpFileInformation == NULL)
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return FALSE;
  }

  /* Validate and translate the filename */
  if (!RtlDosPathNameToNtPathName_U (lpFileName,
				     &FileName,
				     NULL,
				     NULL))
    {
      DPRINT1 ("Invalid path\n");
      SetLastError (ERROR_BAD_PATHNAME);
      return FALSE;
    }

  /* build the object attributes */
  InitializeObjectAttributes (&ObjectAttributes,
			      &FileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  /* Get file attributes */
  Status = NtQueryFullAttributesFile(&ObjectAttributes,
                                     &FileInformation);
                                     
  RtlFreeUnicodeString (&FileName);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("NtQueryFullAttributesFile() failed (Status %lx)\n", Status);
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
   PWCHAR FileNameW;

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return FALSE;

   return GetFileAttributesExW(FileNameW, fInfoLevelId, lpFileInformation);
}


/*
 * @implemented
 */
DWORD STDCALL
GetFileAttributesA(LPCSTR lpFileName)
{
   WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
   PWSTR FileNameW;
	BOOL ret;

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return INVALID_FILE_ATTRIBUTES;

   ret = GetFileAttributesExW(FileNameW, GetFileExInfoStandard, &FileAttributeData);

   return ret ? FileAttributeData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
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

  return Result ? FileAttributeData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
}


/*
 * @implemented
 */
BOOL STDCALL
GetFileAttributesByHandle(IN HANDLE hFile,
                          OUT LPDWORD dwFileAttributes,
                          IN DWORD dwFlags)
{
    FILE_BASIC_INFORMATION FileBasic;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    
    UNREFERENCED_PARAMETER(dwFlags);
    
    if (IsConsoleHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    Status = NtQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &FileBasic,
                                    sizeof(FileBasic),
                                    FileBasicInformation);
    if (NT_SUCCESS(Status))
    {
        *dwFileAttributes = FileBasic.FileAttributes;
        return TRUE;
    }
    
    SetLastErrorByStatus(Status);
    return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetFileAttributesByHandle(IN HANDLE hFile,
                          IN DWORD dwFileAttributes,
                          IN DWORD dwFlags)
{
    FILE_BASIC_INFORMATION FileBasic;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(dwFlags);
    
    if (IsConsoleHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Status = NtQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &FileBasic,
                                    sizeof(FileBasic),
                                    FileBasicInformation);
    if (NT_SUCCESS(Status))
    {
        FileBasic.FileAttributes = dwFileAttributes;
        
        Status = NtSetInformationFile(hFile,
                                      &IoStatusBlock,
                                      &FileBasic,
                                      sizeof(FileBasic),
                                      FileBasicInformation);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    
    return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetFileAttributesA(
   LPCSTR lpFileName,
	DWORD dwFileAttributes)
{
   PWCHAR FileNameW;

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return FALSE;

   return SetFileAttributesW(FileNameW, dwFileAttributes);
}


/*
 * @implemented
 */
BOOL STDCALL
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
  if (!RtlDosPathNameToNtPathName_U (lpFileName,
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




/***********************************************************************
 *           GetTempFileNameA   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique, LPSTR buffer)
{
   WCHAR BufferW[MAX_PATH];
   PWCHAR PathW;
   WCHAR PrefixW[3+1];
   UINT ret;

   if (!(PathW = FilenameA2W(path, FALSE)))
      return 0;

   if (prefix)
      FilenameA2W_N(PrefixW, 3+1, prefix, -1);

   ret = GetTempFileNameW(PathW, prefix ? PrefixW : NULL, unique, BufferW);

   if (ret)
      FilenameW2A_N(buffer, MAX_PATH, BufferW, -1);

   return ret;
}

/***********************************************************************
 *           GetTempFileNameW   (KERNEL32.@)
 */
UINT WINAPI GetTempFileNameW( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer )
{
    static const WCHAR formatW[] = L"%x.tmp";

    int i;
    LPWSTR p;

    if ( !path || !buffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    wcscpy( buffer, path );
    p = buffer + wcslen(buffer);

    /* add a \, if there isn't one  */
    if ((p == buffer) || (p[-1] != '\\')) *p++ = '\\';

    if ( prefix )
        for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

    unique &= 0xffff;

    if (unique) swprintf( p, formatW, unique );
    else
    {
        /* get a "random" unique number and try to create the file */
        HANDLE handle;
        UINT num = GetTickCount() & 0xffff;

        if (!num) num = 1;
        unique = num;
        do
        {
            swprintf( p, formatW, unique );
            handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {  /* We created it */
                DPRINT("created %S\n", buffer);
                CloseHandle( handle );
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS &&
                GetLastError() != ERROR_SHARING_VIOLATION)
                break;  /* No need to go on */
            if (!(++unique & 0xffff)) unique = 1;
        } while (unique != num);
    }

    DPRINT("returning %S\n", buffer);
    return unique;
}





/*
 * @implemented
 */
BOOL STDCALL
GetFileTime(HANDLE hFile,
	    LPFILETIME lpCreationTime,
	    LPFILETIME lpLastAccessTime,
	    LPFILETIME lpLastWriteTime)
{
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_BASIC_INFORMATION FileBasic;
   NTSTATUS Status;

   if(IsConsoleHandle(hFile))
   {
     SetLastError(ERROR_INVALID_HANDLE);
     return FALSE;
   }

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
BOOL STDCALL
SetFileTime(HANDLE hFile,
	    CONST FILETIME *lpCreationTime,
	    CONST FILETIME *lpLastAccessTime,
	    CONST FILETIME *lpLastWriteTime)
{
   FILE_BASIC_INFORMATION FileBasic;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   if(IsConsoleHandle(hFile))
   {
     SetLastError(ERROR_INVALID_HANDLE);
     return FALSE;
   }

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
BOOL STDCALL
SetEndOfFile(HANDLE hFile)
{
	IO_STATUS_BLOCK  IoStatusBlock;
	FILE_END_OF_FILE_INFORMATION	EndOfFileInfo;
	FILE_ALLOCATION_INFORMATION		FileAllocationInfo;
	FILE_POSITION_INFORMATION		 FilePosInfo;
	NTSTATUS Status;

	if(IsConsoleHandle(hFile))
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

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


/*
 * @implemented
 */
BOOL
STDCALL
SetFileValidData(
    HANDLE hFile,
    LONGLONG ValidDataLength
    )
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_VALID_DATA_LENGTH_INFORMATION ValidDataLengthInformation;
	NTSTATUS Status;

	ValidDataLengthInformation.ValidDataLength.QuadPart = ValidDataLength;

	Status = NtSetInformationFile(
						hFile,
						&IoStatusBlock,	 //out
						&ValidDataLengthInformation,
						sizeof(FILE_VALID_DATA_LENGTH_INFORMATION),
						FileValidDataLengthInformation
						);

	if (!NT_SUCCESS(Status)){
		SetLastErrorByStatus(Status);
		return FALSE;
	}

	return TRUE;
}



/*
 * @implemented
 */
BOOL
STDCALL
SetFileShortNameW(
  HANDLE hFile,
  LPCWSTR lpShortName)
{
  NTSTATUS Status;
  ULONG NeededSize;
  UNICODE_STRING ShortName;
  IO_STATUS_BLOCK IoStatusBlock;
  PFILE_NAME_INFORMATION FileNameInfo;

  if(IsConsoleHandle(hFile))
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if(!lpShortName)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  RtlInitUnicodeString(&ShortName, lpShortName);

  NeededSize = sizeof(FILE_NAME_INFORMATION) + ShortName.Length + sizeof(WCHAR);
  if(!(FileNameInfo = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize)))
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  FileNameInfo->FileNameLength = ShortName.Length;
  RtlCopyMemory(FileNameInfo->FileName, ShortName.Buffer, ShortName.Length);

  Status = NtSetInformationFile(hFile,
                                &IoStatusBlock,	 //out
                                FileNameInfo,
                                NeededSize,
                                FileShortNameInformation);

  RtlFreeHeap(RtlGetProcessHeap(), 0, FileNameInfo);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetFileShortNameA(
    HANDLE hFile,
    LPCSTR lpShortName
    )
{
  PWCHAR ShortNameW;

  if(IsConsoleHandle(hFile))
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if(!lpShortName)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (!(ShortNameW = FilenameA2W(lpShortName, FALSE)))
     return FALSE;

  return SetFileShortNameW(hFile, ShortNameW);
}


/*
 * @implemented
 */
BOOL
STDCALL
CheckNameLegalDOS8Dot3W(
    LPCWSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    UNICODE_STRING Name;
    ANSI_STRING AnsiName;

    if(lpName == NULL ||
       (lpOemName == NULL && OemNameSize != 0) ||
       pbNameLegal == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    if(lpOemName != NULL)
    {
      AnsiName.Buffer = lpOemName;
      AnsiName.MaximumLength = (USHORT)OemNameSize * sizeof(CHAR);
      AnsiName.Length = 0;
    }

    RtlInitUnicodeString(&Name, lpName);

    *pbNameLegal = RtlIsNameLegalDOS8Dot3(&Name,
                                          (lpOemName ? &AnsiName : NULL),
                                          (BOOLEAN*)pbNameContainsSpaces);

    return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
CheckNameLegalDOS8Dot3A(
    LPCSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    UNICODE_STRING Name;
    ANSI_STRING AnsiName, AnsiInputName;
    NTSTATUS Status;

    if(lpName == NULL ||
       (lpOemName == NULL && OemNameSize != 0) ||
       pbNameLegal == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    if(lpOemName != NULL)
    {
      AnsiName.Buffer = lpOemName;
      AnsiName.MaximumLength = (USHORT)OemNameSize * sizeof(CHAR);
      AnsiName.Length = 0;
    }

    RtlInitAnsiString(&AnsiInputName, (LPSTR)lpName);
    if(bIsFileApiAnsi)
      Status = RtlAnsiStringToUnicodeString(&Name, &AnsiInputName, TRUE);
    else
      Status = RtlOemStringToUnicodeString(&Name, &AnsiInputName, TRUE);

    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }

    *pbNameLegal = RtlIsNameLegalDOS8Dot3(&Name,
                                          (lpOemName ? &AnsiName : NULL),
                                          (BOOLEAN*)pbNameContainsSpaces);

    RtlFreeUnicodeString(&Name);

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetFinalPathNameByHandleA(IN HANDLE hFile,
                          OUT LPSTR lpszFilePath,
                          IN DWORD cchFilePath,
                          IN DWORD dwFlags)
{
    WCHAR FilePathW[MAX_PATH];
    UNICODE_STRING FilePathU;
    DWORD PrevLastError;
    DWORD Ret = 0;

    if (cchFilePath != 0 &&
        cchFilePath > sizeof(FilePathW) / sizeof(FilePathW[0]))
    {
        FilePathU.Length = 0;
        FilePathU.MaximumLength = (USHORT)cchFilePath * sizeof(WCHAR);
        FilePathU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           FilePathU.MaximumLength);
        if (FilePathU.Buffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }
    else
    {
        FilePathU.Length = 0;
        FilePathU.MaximumLength = sizeof(FilePathW);
        FilePathU.Buffer = FilePathW;
    }

    /* save the last error code */
    PrevLastError = GetLastError();
    SetLastError(ERROR_SUCCESS);

    /* call the unicode version that does all the work */
    Ret = GetFinalPathNameByHandleW(hFile,
                                    FilePathU.Buffer,
                                    cchFilePath,
                                    dwFlags);

    if (GetLastError() == ERROR_SUCCESS)
    {
        /* no error, restore the last error code and convert the string */
        SetLastError(PrevLastError);

        Ret = FilenameU2A_FitOrFail(lpszFilePath,
                                    cchFilePath,
                                    &FilePathU);
    }

    /* free allocated memory if necessary */
    if (FilePathU.Buffer != FilePathW)
    {
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    FilePathU.Buffer);
    }

    return Ret;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetFinalPathNameByHandleW(IN HANDLE hFile,
                          OUT LPWSTR lpszFilePath,
                          IN DWORD cchFilePath,
                          IN DWORD dwFlags)
{
    if (dwFlags & ~(VOLUME_NAME_DOS | VOLUME_NAME_GUID | VOLUME_NAME_NT |
                    VOLUME_NAME_NONE | FILE_NAME_NORMALIZED | FILE_NAME_OPENED))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    UNIMPLEMENTED;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetFileBandwidthReservation(IN HANDLE hFile,
                            IN DWORD nPeriodMilliseconds,
                            IN DWORD nBytesPerPeriod,
                            IN BOOL bDiscardable,
                            OUT LPDWORD lpTransferSize,
                            OUT LPDWORD lpNumOutstandingRequests)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetFileBandwidthReservation(IN HANDLE hFile,
                            OUT LPDWORD lpPeriodMilliseconds,
                            OUT LPDWORD lpBytesPerPeriod,
                            OUT LPBOOL pDiscardable,
                            OUT LPDWORD lpTransferSize,
                            OUT LPDWORD lpNumOutstandingRequests)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetFileCompletionNotificationModes(IN HANDLE FileHandle,
                                   IN UCHAR Flags)
{
    if (Flags & ~(FILE_SKIP_COMPLETION_PORT_ON_SUCCESS | FILE_SKIP_SET_EVENT_ON_HANDLE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
HANDLE
WINAPI
OpenFileById(IN HANDLE hFile,
             IN LPFILE_ID_DESCRIPTOR lpFileID,
             IN DWORD dwDesiredAccess,
             IN DWORD dwShareMode,
             IN LPSECURITY_ATTRIBUTES lpSecurityAttributes  OPTIONAL,
             IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return INVALID_HANDLE_VALUE;
}

/* EOF */
