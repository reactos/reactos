/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/volume.c
 * PURPOSE:         File volume functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Erik Bos, Alexandre Julliard :
 *                      GetLogicalDriveStringsA,
 *                      GetLogicalDriveStringsW, GetLogicalDrives
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
//WINE copyright notice:
/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <k32.h>
#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

HANDLE
WINAPI
InternalOpenDirW(IN LPCWSTR DirName,
                 IN BOOLEAN Write)
{
    UNICODE_STRING NtPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS errCode;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile;

    if (!RtlDosPathNameToNtPathName_U(DirName, &NtPathU, NULL, NULL))
    {
        WARN("Invalid path\n");
        SetLastError(ERROR_BAD_PATHNAME);
        return INVALID_HANDLE_VALUE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    errCode = NtCreateFile(&hFile,
                           Write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           0,
                           NULL,
                           0);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathU.Buffer);

    if (!NT_SUCCESS(errCode))
    {
        BaseSetLastNTError(errCode);
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumeInformationA(IN LPCSTR lpRootPathName,
                      IN LPSTR lpVolumeNameBuffer,
                      IN DWORD nVolumeNameSize,
                      OUT LPDWORD lpVolumeSerialNumber OPTIONAL,
                      OUT LPDWORD lpMaximumComponentLength OPTIONAL,
                      OUT LPDWORD lpFileSystemFlags OPTIONAL,
                      OUT LPSTR lpFileSystemNameBuffer OPTIONAL,
                      IN DWORD nFileSystemNameSize)
{
  UNICODE_STRING FileSystemNameU;
  UNICODE_STRING VolumeNameU = { 0, 0, NULL };
  ANSI_STRING VolumeName;
  ANSI_STRING FileSystemName;
  PWCHAR RootPathNameW;
  BOOL Result;

  if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
     return FALSE;

  if (lpVolumeNameBuffer)
    {
      VolumeNameU.MaximumLength = (USHORT)nVolumeNameSize * sizeof(WCHAR);
      VolumeNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                    0,
	                                    VolumeNameU.MaximumLength);
      if (VolumeNameU.Buffer == NULL)
      {
          goto FailNoMem;
      }
    }

  if (lpFileSystemNameBuffer)
    {
      FileSystemNameU.Length = 0;
      FileSystemNameU.MaximumLength = (USHORT)nFileSystemNameSize * sizeof(WCHAR);
      FileSystemNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                        0,
	                                        FileSystemNameU.MaximumLength);
      if (FileSystemNameU.Buffer == NULL)
      {
          if (VolumeNameU.Buffer != NULL)
          {
              RtlFreeHeap(RtlGetProcessHeap(),
                          0,
                          VolumeNameU.Buffer);
          }

FailNoMem:
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          return FALSE;
      }
    }

  Result = GetVolumeInformationW (RootPathNameW,
	                          lpVolumeNameBuffer ? VolumeNameU.Buffer : NULL,
	                          nVolumeNameSize,
	                          lpVolumeSerialNumber,
	                          lpMaximumComponentLength,
	                          lpFileSystemFlags,
				  lpFileSystemNameBuffer ? FileSystemNameU.Buffer : NULL,
	                          nFileSystemNameSize);

  if (Result)
    {
      if (lpVolumeNameBuffer)
        {
          VolumeNameU.Length = wcslen(VolumeNameU.Buffer) * sizeof(WCHAR);
	  VolumeName.Length = 0;
	  VolumeName.MaximumLength = (USHORT)nVolumeNameSize;
	  VolumeName.Buffer = lpVolumeNameBuffer;
	}

      if (lpFileSystemNameBuffer)
	{
	  FileSystemNameU.Length = wcslen(FileSystemNameU.Buffer) * sizeof(WCHAR);
	  FileSystemName.Length = 0;
	  FileSystemName.MaximumLength = (USHORT)nFileSystemNameSize;
	  FileSystemName.Buffer = lpFileSystemNameBuffer;
	}

      /* convert unicode strings to ansi (or oem) */
      if (bIsFileApiAnsi)
        {
	  if (lpVolumeNameBuffer)
	    {
	      RtlUnicodeStringToAnsiString (&VolumeName,
			                    &VolumeNameU,
			                    FALSE);
	    }
	  if (lpFileSystemNameBuffer)
	    {
	      RtlUnicodeStringToAnsiString (&FileSystemName,
			                    &FileSystemNameU,
			                    FALSE);
	    }
	}
      else
        {
	  if (lpVolumeNameBuffer)
	    {
	      RtlUnicodeStringToOemString (&VolumeName,
			                   &VolumeNameU,
			                   FALSE);
	    }
          if (lpFileSystemNameBuffer)
	    {
	      RtlUnicodeStringToOemString (&FileSystemName,
			                   &FileSystemNameU,
			                   FALSE);
	    }
	}
    }

  if (lpVolumeNameBuffer)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
	           0,
	           VolumeNameU.Buffer);
    }
  if (lpFileSystemNameBuffer)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
	           0,
	           FileSystemNameU.Buffer);
    }

  return Result;
}

#define FS_VOLUME_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_VOLUME_INFORMATION))

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumeInformationW(IN LPCWSTR lpRootPathName,
                      IN LPWSTR lpVolumeNameBuffer,
                      IN DWORD nVolumeNameSize,
                      OUT LPDWORD lpVolumeSerialNumber OPTIONAL,
                      OUT LPDWORD lpMaximumComponentLength OPTIONAL,
                      OUT LPDWORD lpFileSystemFlags OPTIONAL,
                      OUT LPWSTR lpFileSystemNameBuffer OPTIONAL,
                      IN DWORD nFileSystemNameSize)
{
  PFILE_FS_VOLUME_INFORMATION FileFsVolume;
  PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
  IO_STATUS_BLOCK IoStatusBlock;
  WCHAR RootPathName[MAX_PATH];
  UCHAR Buffer[max(FS_VOLUME_BUFFER_SIZE, FS_ATTRIBUTE_BUFFER_SIZE)];

  HANDLE hFile;
  NTSTATUS errCode;

  FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;
  FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

  TRACE("FileFsVolume %p\n", FileFsVolume);
  TRACE("FileFsAttribute %p\n", FileFsAttribute);

  if (!lpRootPathName || !wcscmp(lpRootPathName, L""))
  {
      GetCurrentDirectoryW (MAX_PATH, RootPathName);
  }
  else
  {
      wcsncpy (RootPathName, lpRootPathName, 3);
  }
  RootPathName[3] = 0;

  hFile = InternalOpenDirW(RootPathName, FALSE);
  if (hFile == INVALID_HANDLE_VALUE)
    {
      return FALSE;
    }

  TRACE("hFile: %p\n", hFile);
  errCode = NtQueryVolumeInformationFile(hFile,
                                         &IoStatusBlock,
                                         FileFsVolume,
                                         FS_VOLUME_BUFFER_SIZE,
                                         FileFsVolumeInformation);
  if ( !NT_SUCCESS(errCode) )
    {
      WARN("Status: %x\n", errCode);
      CloseHandle(hFile);
      BaseSetLastNTError (errCode);
      return FALSE;
    }

  if (lpVolumeSerialNumber)
    *lpVolumeSerialNumber = FileFsVolume->VolumeSerialNumber;

  if (lpVolumeNameBuffer)
    {
      if (nVolumeNameSize * sizeof(WCHAR) >= FileFsVolume->VolumeLabelLength + sizeof(WCHAR))
        {
	  memcpy(lpVolumeNameBuffer,
		 FileFsVolume->VolumeLabel,
		 FileFsVolume->VolumeLabelLength);
	  lpVolumeNameBuffer[FileFsVolume->VolumeLabelLength / sizeof(WCHAR)] = 0;
	}
      else
        {
	  CloseHandle(hFile);
	  SetLastError(ERROR_MORE_DATA);
	  return FALSE;
	}
    }

  errCode = NtQueryVolumeInformationFile (hFile,
	                                  &IoStatusBlock,
	                                  FileFsAttribute,
	                                  FS_ATTRIBUTE_BUFFER_SIZE,
	                                  FileFsAttributeInformation);
  CloseHandle(hFile);
  if (!NT_SUCCESS(errCode))
    {
      WARN("Status: %x\n", errCode);
      BaseSetLastNTError (errCode);
      return FALSE;
    }

  if (lpFileSystemFlags)
    *lpFileSystemFlags = FileFsAttribute->FileSystemAttributes;
  if (lpMaximumComponentLength)
    *lpMaximumComponentLength = FileFsAttribute->MaximumComponentNameLength;
  if (lpFileSystemNameBuffer)
    {
      if (nFileSystemNameSize * sizeof(WCHAR) >= FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        {
	  memcpy(lpFileSystemNameBuffer,
		 FileFsAttribute->FileSystemName,
		 FileFsAttribute->FileSystemNameLength);
	  lpFileSystemNameBuffer[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = 0;
	}
      else
        {
	  SetLastError(ERROR_MORE_DATA);
	  return FALSE;
	}
    }
  return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetVolumeLabelA(IN LPCSTR lpRootPathName,
                IN LPCSTR lpVolumeName OPTIONAL) /* NULL if deleting label */
{
	PWCHAR RootPathNameW;
   PWCHAR VolumeNameW = NULL;
	BOOL Result;

   if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
      return FALSE;

   if (lpVolumeName)
   {
      if (!(VolumeNameW = FilenameA2W(lpVolumeName, TRUE)))
         return FALSE;
   }

   Result = SetVolumeLabelW (RootPathNameW,
                             VolumeNameW);

   if (VolumeNameW)
   {
	   RtlFreeHeap (RtlGetProcessHeap (),
	                0,
                   VolumeNameW );
   }

	return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetVolumeLabelW(IN LPCWSTR lpRootPathName,
                IN LPCWSTR lpVolumeName OPTIONAL) /* NULL if deleting label */
{
   PFILE_FS_LABEL_INFORMATION LabelInfo;
   IO_STATUS_BLOCK IoStatusBlock;
   ULONG LabelLength;
   HANDLE hFile;
   NTSTATUS Status;

   LabelLength = wcslen(lpVolumeName) * sizeof(WCHAR);
   LabelInfo = RtlAllocateHeap(RtlGetProcessHeap(),
			       0,
			       sizeof(FILE_FS_LABEL_INFORMATION) +
			       LabelLength);
   if (LabelInfo == NULL)
   {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return FALSE;
   }
   LabelInfo->VolumeLabelLength = LabelLength;
   memcpy(LabelInfo->VolumeLabel,
	  lpVolumeName,
	  LabelLength);

   hFile = InternalOpenDirW(lpRootPathName, TRUE);
   if (INVALID_HANDLE_VALUE == hFile)
   {
        RtlFreeHeap(RtlGetProcessHeap(),
	            0,
	            LabelInfo);
        return FALSE;
   }

   Status = NtSetVolumeInformationFile(hFile,
				       &IoStatusBlock,
				       LabelInfo,
				       sizeof(FILE_FS_LABEL_INFORMATION) +
				       LabelLength,
				       FileFsLabelInformation);

   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       LabelInfo);

   if (!NT_SUCCESS(Status))
     {
	WARN("Status: %x\n", Status);
	CloseHandle(hFile);
	BaseSetLastNTError(Status);
	return FALSE;
     }

   CloseHandle(hFile);
   return TRUE;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
HANDLE
WINAPI
FindFirstVolumeW(IN LPWSTR volume,
                 IN DWORD len)
{
    DWORD size = 1024;
    DWORD br;
    HANDLE mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE );
    if (mgr == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    for (;;)
    {
        MOUNTMGR_MOUNT_POINT input;
        MOUNTMGR_MOUNT_POINTS *output;

        if (!(output = RtlAllocateHeap( RtlGetProcessHeap(), 0, size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            break;
        }
        memset( &input, 0, sizeof(input) );

        if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_POINTS, &input, sizeof(input),
                              output, size, &br, NULL ))
        {
            if (GetLastError() != ERROR_MORE_DATA) break;
            size = output->Size;
            RtlFreeHeap( RtlGetProcessHeap(), 0, output );
            continue;
        }
        CloseHandle( mgr );
        /* abuse the Size field to store the current index */
        output->Size = 0;
        if (!FindNextVolumeW( output, volume, len ))
        {
            RtlFreeHeap( RtlGetProcessHeap(), 0, output );
            return INVALID_HANDLE_VALUE;
        }
        return (HANDLE)output;
    }
    CloseHandle( mgr );
    return INVALID_HANDLE_VALUE;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
HANDLE
WINAPI
FindFirstVolumeA(IN LPSTR volume,
                 IN DWORD len)
{
    WCHAR *buffer = NULL;
    HANDLE handle;

    buffer = RtlAllocateHeap( RtlGetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (!buffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    handle = FindFirstVolumeW( buffer, len );

    if (handle != INVALID_HANDLE_VALUE)
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL ))
        {
            FindVolumeClose( handle );
            handle = INVALID_HANDLE_VALUE;
        }
    }
    RtlFreeHeap( RtlGetProcessHeap(), 0, buffer );
    return handle;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
BOOL
WINAPI
FindVolumeClose(IN HANDLE hFindVolume)
{
    return RtlFreeHeap(RtlGetProcessHeap(), 0, hFindVolume);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNameA(IN LPCSTR lpszFileName,
                   IN LPSTR lpszVolumePathName,
                   IN DWORD cchBufferLength)
{
    PWCHAR FileNameW = NULL;
    WCHAR VolumePathName[MAX_PATH];
    BOOL Result;

    if (lpszFileName)
    {
        if (!(FileNameW = FilenameA2W(lpszFileName, FALSE)))
            return FALSE;
    }

    Result = GetVolumePathNameW(FileNameW, VolumePathName, cchBufferLength);

    if (Result)
        FilenameW2A_N(lpszVolumePathName, MAX_PATH, VolumePathName, -1);

    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNameW(IN LPCWSTR lpszFileName,
                   IN LPWSTR lpszVolumePathName,
                   IN DWORD cchBufferLength)
{
    DWORD PathLength;
    UNICODE_STRING UnicodeFilePath;
    LPWSTR FilePart;
    PWSTR FullFilePath, FilePathName;
    ULONG PathSize;
    WCHAR VolumeName[MAX_PATH];
    DWORD ErrorCode;
    BOOL Result = FALSE;

    if (!lpszFileName || !lpszVolumePathName || !cchBufferLength)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (!(PathLength = GetFullPathNameW(lpszFileName, 0, NULL, NULL)))
    {
        return Result;
    }
    else
    {
        PathLength = PathLength + 10;
        PathSize = PathLength * sizeof(WCHAR);

        if (!(FullFilePath = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathSize)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return Result;
        }

        if (!GetFullPathNameW(lpszFileName, PathLength, FullFilePath, &FilePart))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FullFilePath);
            return Result;
        }

        RtlInitUnicodeString(&UnicodeFilePath, FullFilePath);

        if (UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR) - 1] != '\\')
        {
            UnicodeFilePath.Length += sizeof(WCHAR);
            UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR) - 1] = '\\';
            UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';
        }

        if (!(FilePathName = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathSize)))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, FullFilePath);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return Result;
        }

        while (!GetVolumeNameForVolumeMountPointW(UnicodeFilePath.Buffer,
                                                  VolumeName,
                                                  MAX_PATH))
        {
            if (((UnicodeFilePath.Length == 4) && (UnicodeFilePath.Buffer[0] == '\\') &&
                (UnicodeFilePath.Buffer[1] == '\\')) || ((UnicodeFilePath.Length == 6) &&
                (UnicodeFilePath.Buffer[1] == ':')))
            {
                break;
            }

            UnicodeFilePath.Length -= sizeof(WCHAR);
            UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';

            memcpy(FilePathName, UnicodeFilePath.Buffer, UnicodeFilePath.Length);
            FilePathName[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';

            if (!GetFullPathNameW(FilePathName, PathLength, FullFilePath, &FilePart))
            {
                goto Cleanup2;
            }

            if (!FilePart)
            {
                RtlInitUnicodeString(&UnicodeFilePath, FullFilePath);
                UnicodeFilePath.Length += sizeof(WCHAR);
                UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR) - 1] = '\\';
                UnicodeFilePath.Buffer[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';
                break;
            }

            FilePart[0] = '\0';
            RtlInitUnicodeString(&UnicodeFilePath, FullFilePath);
        }
    }

    if (UnicodeFilePath.Length > (cchBufferLength * sizeof(WCHAR)) - sizeof(WCHAR))
    {
        ErrorCode = ERROR_FILENAME_EXCED_RANGE;
        goto Cleanup1;
    }

    memcpy(lpszVolumePathName, UnicodeFilePath.Buffer, UnicodeFilePath.Length);
    lpszVolumePathName[UnicodeFilePath.Length / sizeof(WCHAR)] = '\0';

    Result = TRUE;
    goto Cleanup2;

Cleanup1:
    SetLastError(ErrorCode);
Cleanup2:
    RtlFreeHeap(RtlGetProcessHeap(), 0, FullFilePath);
    RtlFreeHeap(RtlGetProcessHeap(), 0, FilePathName);
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextVolumeA(IN HANDLE handle,
                IN LPSTR volume,
                IN DWORD len)
{
    WCHAR *buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, len * sizeof(WCHAR));
    BOOL ret;

    if (!buffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ((ret = FindNextVolumeW( handle, buffer, len )))
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL )) ret = FALSE;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, buffer);
    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextVolumeW(IN HANDLE handle,
                IN LPWSTR volume,
                IN DWORD len)
{
    MOUNTMGR_MOUNT_POINTS *data = handle;

    while (data->Size < data->NumberOfMountPoints)
    {
        static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{',};
        WCHAR *link = (WCHAR *)((char *)data + data->MountPoints[data->Size].SymbolicLinkNameOffset);
        DWORD size = data->MountPoints[data->Size].SymbolicLinkNameLength;
        data->Size++;
        /* skip non-volumes */
        if (size < sizeof(volumeW) || memcmp( link, volumeW, sizeof(volumeW) )) continue;
        if (size + sizeof(WCHAR) >= len * sizeof(WCHAR))
        {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            return FALSE;
        }
        memcpy( volume, link, size );
        volume[1] = '\\';  /* map \??\ to \\?\ */
        volume[size / sizeof(WCHAR)] = '\\';  /* Windows appends a backslash */
        volume[size / sizeof(WCHAR) + 1] = 0;
        DPRINT( "returning entry %u %s\n", data->Size - 1, volume );
        return TRUE;
    }
    SetLastError( ERROR_NO_MORE_FILES );
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameA(IN LPCSTR lpszVolumeName,
                                 IN LPSTR lpszVolumePathNames,
                                 IN DWORD cchBufferLength,
                                 OUT PDWORD lpcchReturnLength)
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameW(IN LPCWSTR lpszVolumeName,
                                 IN LPWSTR lpszVolumePathNames,
                                 IN DWORD cchBufferLength,
                                 OUT PDWORD lpcchReturnLength)
{
    STUB;
    return 0;
}

/* EOF */
