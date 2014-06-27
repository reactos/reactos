/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

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
        BaseSetLastNTError(Status);

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

   /* destLen should never exceed MAX_PATH */
   if (destLen > MAX_PATH) destLen = MAX_PATH;

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
BOOL WINAPI
FlushFileBuffers(IN HANDLE hFile)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    hFile = TranslateStdHandle(hFile);

    if (IsConsoleHandle(hFile))
    {
        return FlushConsoleInputBuffer(hFile);
    }

    Status = NtFlushBuffersFile(hFile,
                                &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
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

   TRACE("SetFilePointer(hFile %p, lDistanceToMove %d, dwMoveMethod %lu)\n",
      hFile,lDistanceToMove,dwMoveMethod);

   if(IsConsoleHandle(hFile))
   {
     SetLastError(ERROR_INVALID_HANDLE);
     return INVALID_SET_FILE_POINTER;
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
    errCode = NtQueryInformationFile(hFile,
                   &IoStatusBlock,
                   &FilePosition,
                   sizeof(FILE_POSITION_INFORMATION),
                   FilePositionInformation);
    FilePosition.CurrentByteOffset.QuadPart += Distance.QuadPart;
    if (!NT_SUCCESS(errCode))
    {
      if (lpDistanceToMoveHigh != NULL)
          *lpDistanceToMoveHigh = -1;
      BaseSetLastNTError(errCode);
      return INVALID_SET_FILE_POINTER;
    }
    break;
     case FILE_END:
    errCode = NtQueryInformationFile(hFile,
                               &IoStatusBlock,
                               &FileStandard,
                               sizeof(FILE_STANDARD_INFORMATION),
                               FileStandardInformation);
    FilePosition.CurrentByteOffset.QuadPart =
                  FileStandard.EndOfFile.QuadPart + Distance.QuadPart;
    if (!NT_SUCCESS(errCode))
    {
      if (lpDistanceToMoveHigh != NULL)
          *lpDistanceToMoveHigh = -1;
      BaseSetLastNTError(errCode);
      return INVALID_SET_FILE_POINTER;
    }
    break;
     case FILE_BEGIN:
        FilePosition.CurrentByteOffset.QuadPart = Distance.QuadPart;
    break;
     default:
        SetLastError(ERROR_INVALID_PARAMETER);
    return INVALID_SET_FILE_POINTER;
   }

   if(FilePosition.CurrentByteOffset.QuadPart < 0)
   {
     SetLastError(ERROR_NEGATIVE_SEEK);
     return INVALID_SET_FILE_POINTER;
   }

   if (lpDistanceToMoveHigh == NULL && FilePosition.CurrentByteOffset.HighPart != 0)
   {
     /* If we're moving the pointer outside of the 32 bit boundaries but
        the application only passed a 32 bit value we need to bail out! */
     SetLastError(ERROR_INVALID_PARAMETER);
     return INVALID_SET_FILE_POINTER;
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

       BaseSetLastNTError(errCode);
       return INVALID_SET_FILE_POINTER;
     }

   if (lpDistanceToMoveHigh != NULL)
     {
        *lpDistanceToMoveHigh = FilePosition.CurrentByteOffset.u.HighPart;
     }

   if (FilePosition.CurrentByteOffset.u.LowPart == MAXDWORD)
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
WINAPI
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
	BaseSetLastNTError(errCode);
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
DWORD WINAPI
GetFileType(HANDLE hFile)
{
  FILE_FS_DEVICE_INFORMATION DeviceInfo;
  IO_STATUS_BLOCK StatusBlock;
  NTSTATUS Status;

  /* Get real handle */
  hFile = TranslateStdHandle(hFile);

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
      BaseSetLastNTError(Status);
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
DWORD WINAPI
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
	BaseSetLastNTError(errCode);
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
WINAPI
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
	BaseSetLastNTError(errCode);
	return FALSE;
     }
   if (lpFileSize)
     *lpFileSize = FileStandard.EndOfFile;

   return TRUE;
}


/*
 * @implemented
 */
DWORD WINAPI
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
DWORD WINAPI
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
	BaseSetLastNTError(errCode);
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
BOOL WINAPI
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
	BaseSetLastNTError(errCode);
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
	BaseSetLastNTError(errCode);
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
	BaseSetLastNTError(errCode);
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
	BaseSetLastNTError(errCode);
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
BOOL WINAPI
GetFileAttributesExW(LPCWSTR lpFileName,
		     GET_FILEEX_INFO_LEVELS fInfoLevelId,
		     LPVOID lpFileInformation)
{
  FILE_NETWORK_OPEN_INFORMATION FileInformation;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  NTSTATUS Status;
  WIN32_FILE_ATTRIBUTE_DATA* FileAttributeData;

  TRACE("GetFileAttributesExW(%S) called\n", lpFileName);


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
      WARN ("Invalid path '%S'\n", lpFileName);
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
      WARN ("NtQueryFullAttributesFile() failed (Status %lx)\n", Status);
      BaseSetLastNTError (Status);
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
BOOL WINAPI
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
DWORD WINAPI
GetFileAttributesA(LPCSTR lpFileName)
{
   WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
   PWSTR FileNameW;
   BOOL ret;

   if (!lpFileName || !(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return INVALID_FILE_ATTRIBUTES;

   ret = GetFileAttributesExW(FileNameW, GetFileExInfoStandard, &FileAttributeData);

   return ret ? FileAttributeData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
}


/*
 * @implemented
 */
DWORD WINAPI
GetFileAttributesW(LPCWSTR lpFileName)
{
  WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
  BOOL Result;

  TRACE ("GetFileAttributeW(%S) called\n", lpFileName);

  Result = GetFileAttributesExW(lpFileName, GetFileExInfoStandard, &FileAttributeData);

  return Result ? FileAttributeData.dwFileAttributes : INVALID_FILE_ATTRIBUTES;
}


/*
 * @implemented
 */
BOOL WINAPI
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

    BaseSetLastNTError(Status);
    return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
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
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
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
BOOL WINAPI
SetFileAttributesW(LPCWSTR lpFileName,
		   DWORD dwFileAttributes)
{
  FILE_BASIC_INFORMATION FileInformation;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING FileName;
  HANDLE FileHandle;
  NTSTATUS Status;

  TRACE ("SetFileAttributeW(%S, 0x%lx) called\n", lpFileName, dwFileAttributes);

  /* Validate and translate the filename */
  if (!RtlDosPathNameToNtPathName_U (lpFileName,
				     &FileName,
				     NULL,
				     NULL))
    {
      WARN ("Invalid path\n");
      SetLastError (ERROR_BAD_PATHNAME);
      return FALSE;
    }
  TRACE ("FileName: \'%wZ\'\n", &FileName);

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
      WARN ("NtOpenFile() failed (Status %lx)\n", Status);
      BaseSetLastNTError (Status);
      return FALSE;
    }

  Status = NtQueryInformationFile(FileHandle,
				  &IoStatusBlock,
				  &FileInformation,
				  sizeof(FILE_BASIC_INFORMATION),
				  FileBasicInformation);
  if (!NT_SUCCESS(Status))
    {
      WARN ("SetFileAttributes NtQueryInformationFile failed with status 0x%08x\n", Status);
      NtClose (FileHandle);
      BaseSetLastNTError (Status);
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
      WARN ("SetFileAttributes NtSetInformationFile failed with status 0x%08x\n", Status);
      BaseSetLastNTError (Status);
      return FALSE;
    }

  return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
GetFileTime(IN HANDLE hFile,
            OUT LPFILETIME lpCreationTime OPTIONAL,
            OUT LPFILETIME lpLastAccessTime OPTIONAL,
            OUT LPFILETIME lpLastWriteTime OPTIONAL)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileBasic;

    if(IsConsoleHandle(hFile))
    {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    Status = NtQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &FileBasic,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (lpCreationTime)
    {
        lpCreationTime->dwLowDateTime = FileBasic.CreationTime.LowPart;
        lpCreationTime->dwHighDateTime = FileBasic.CreationTime.HighPart;
    }

    if (lpLastAccessTime)
    {
        lpLastAccessTime->dwLowDateTime = FileBasic.LastAccessTime.LowPart;
        lpLastAccessTime->dwHighDateTime = FileBasic.LastAccessTime.HighPart;
    }

    if (lpLastWriteTime)
    {
        lpLastWriteTime->dwLowDateTime = FileBasic.LastWriteTime.LowPart;
        lpLastWriteTime->dwHighDateTime = FileBasic.LastWriteTime.HighPart;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
SetFileTime(IN HANDLE hFile,
            CONST FILETIME *lpCreationTime OPTIONAL,
            CONST FILETIME *lpLastAccessTime OPTIONAL,
            CONST FILETIME *lpLastWriteTime OPTIONAL)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileBasic;

    if(IsConsoleHandle(hFile))
    {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return FALSE;
    }

    memset(&FileBasic, 0, sizeof(FILE_BASIC_INFORMATION));

    if (lpCreationTime)
    {
        FileBasic.CreationTime.LowPart = lpCreationTime->dwLowDateTime;
        FileBasic.CreationTime.HighPart = lpCreationTime->dwHighDateTime;
    }

    if (lpLastAccessTime)
    {
        FileBasic.LastAccessTime.LowPart = lpLastAccessTime->dwLowDateTime;
        FileBasic.LastAccessTime.HighPart = lpLastAccessTime->dwHighDateTime;
    }

    if (lpLastWriteTime)
    {
        FileBasic.LastWriteTime.LowPart = lpLastWriteTime->dwLowDateTime;
        FileBasic.LastWriteTime.HighPart = lpLastWriteTime->dwHighDateTime;
    }

    Status = NtSetInformationFile(hFile,
                                  &IoStatusBlock,
                                  &FileBasic,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * The caller must have opened the file with the DesiredAccess FILE_WRITE_DATA flag set.
 *
 * @implemented
 */
BOOL WINAPI
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
		BaseSetLastNTError(Status);
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
		BaseSetLastNTError(Status);
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
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;

}


/*
 * @implemented
 */
BOOL
WINAPI
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
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;
}

/* EOF */
