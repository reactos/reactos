/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/volume.c
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
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

#define MAX_DOS_DRIVES 26


static HANDLE
InternalOpenDirW(LPCWSTR DirName,
		 BOOLEAN Write)
{
  UNICODE_STRING NtPathU;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS errCode;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE hFile;

  if (!RtlDosPathNameToNtPathName_U(DirName,
				    &NtPathU,
				    NULL,
				    NULL))
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

    errCode = NtCreateFile (&hFile,
	                    Write ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
			    &ObjectAttributes,
			    &IoStatusBlock,
			    NULL,
			    0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,
			    FILE_OPEN,
			    0,
			    NULL,
			    0);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                NtPathU.Buffer);

    if (!NT_SUCCESS(errCode))
    {
	SetLastErrorByStatus (errCode);
	return INVALID_HANDLE_VALUE;
    }
    return hFile;
}


/*
 * @implemented
 */
/* Synced to Wine-? */
DWORD WINAPI
GetLogicalDriveStringsA(DWORD nBufferLength,
			LPSTR lpBuffer)
{
   DWORD drive, count;
   DWORD dwDriveMap;

   dwDriveMap = GetLogicalDrives();

   for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
     {
	if (dwDriveMap & (1<<drive))
	   count++;
     }


   if (count * 4 <= nBufferLength)
     {
	LPSTR p = lpBuffer;

	for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
	  if (dwDriveMap & (1<<drive))
	  {
	     *p++ = 'A' + (UCHAR)drive;
	     *p++ = ':';
	     *p++ = '\\';
	     *p++ = '\0';
	  }
	*p = '\0';
     }
    return (count * 4);
}


/*
 * @implemented
 */
/* Synced to Wine-? */
DWORD WINAPI
GetLogicalDriveStringsW(DWORD nBufferLength,
			LPWSTR lpBuffer)
{
   DWORD drive, count;
   DWORD dwDriveMap;

   dwDriveMap = GetLogicalDrives();

   for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
     {
	if (dwDriveMap & (1<<drive))
	   count++;
     }

    if (count * 4 <=  nBufferLength)
    {
        LPWSTR p = lpBuffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (dwDriveMap & (1<<drive))
            {
                *p++ = (WCHAR)('A' + drive);
                *p++ = (WCHAR)':';
                *p++ = (WCHAR)'\\';
                *p++ = (WCHAR)'\0';
            }
        *p = (WCHAR)'\0';
    }
    return (count * 4);
}


/*
 * @implemented
 */
/* Synced to Wine-? */
DWORD WINAPI
GetLogicalDrives(VOID)
{
	NTSTATUS Status;
	PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

	/* Get the Device Map for this Process */
	Status = NtQueryInformationProcess(NtCurrentProcess(),
					   ProcessDeviceMap,
					   &ProcessDeviceMapInfo,
					   sizeof(ProcessDeviceMapInfo),
					   NULL);

	/* Return the Drive Map */
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus(Status);
		return 0;
	}

        return ProcessDeviceMapInfo.Query.DriveMap;
}


/*
 * @implemented
 */
BOOL WINAPI
GetDiskFreeSpaceA (
	LPCSTR	lpRootPathName,
	LPDWORD	lpSectorsPerCluster,
	LPDWORD	lpBytesPerSector,
	LPDWORD	lpNumberOfFreeClusters,
	LPDWORD	lpTotalNumberOfClusters
	)
{
   PWCHAR RootPathNameW=NULL;

   if (lpRootPathName)
   {
      if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
         return FALSE;
   }

	return GetDiskFreeSpaceW (RootPathNameW,
	                            lpSectorsPerCluster,
	                            lpBytesPerSector,
	                            lpNumberOfFreeClusters,
	                            lpTotalNumberOfClusters);
}


/*
 * @implemented
 */
BOOL WINAPI
GetDiskFreeSpaceW(
    LPCWSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    FILE_FS_SIZE_INFORMATION FileFsSize;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR RootPathName[MAX_PATH];
    HANDLE hFile;
    NTSTATUS errCode;

    if (lpRootPathName)
    {
        wcsncpy (RootPathName, lpRootPathName, 3);
    }
    else
    {
        GetCurrentDirectoryW (MAX_PATH, RootPathName);
    }
    RootPathName[3] = 0;

  hFile = InternalOpenDirW(RootPathName, FALSE);
  if (INVALID_HANDLE_VALUE == hFile)
    {
      SetLastError(ERROR_PATH_NOT_FOUND);
      return FALSE;
    }

    errCode = NtQueryVolumeInformationFile(hFile,
                                           &IoStatusBlock,
                                           &FileFsSize,
                                           sizeof(FILE_FS_SIZE_INFORMATION),
                                           FileFsSizeInformation);
    if (!NT_SUCCESS(errCode))
    {
        CloseHandle(hFile);
        SetLastErrorByStatus (errCode);
        return FALSE;
    }

    if (lpSectorsPerCluster)
        *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    if (lpBytesPerSector)
        *lpBytesPerSector = FileFsSize.BytesPerSector;
    if (lpNumberOfFreeClusters)
        *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.u.LowPart;
    if (lpTotalNumberOfClusters)
        *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.u.LowPart;
    CloseHandle(hFile);

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
GetDiskFreeSpaceExA (
	LPCSTR lpDirectoryName   OPTIONAL,
	PULARGE_INTEGER	lpFreeBytesAvailableToCaller,
	PULARGE_INTEGER	lpTotalNumberOfBytes,
	PULARGE_INTEGER	lpTotalNumberOfFreeBytes
	)
{
   PWCHAR DirectoryNameW=NULL;

	if (lpDirectoryName)
	{
      if (!(DirectoryNameW = FilenameA2W(lpDirectoryName, FALSE)))
         return FALSE;
	}

   return GetDiskFreeSpaceExW (DirectoryNameW ,
	                              lpFreeBytesAvailableToCaller,
	                              lpTotalNumberOfBytes,
	                              lpTotalNumberOfFreeBytes);
}


/*
 * @implemented
 */
BOOL WINAPI
GetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName OPTIONAL,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    union
    {
        FILE_FS_SIZE_INFORMATION FsSize;
        FILE_FS_FULL_SIZE_INFORMATION FsFullSize;
    } FsInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    ULARGE_INTEGER BytesPerCluster;
    HANDLE hFile;
    NTSTATUS Status;

    if (lpDirectoryName == NULL)
        lpDirectoryName = L"\\";

    hFile = InternalOpenDirW(lpDirectoryName, FALSE);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }

    if (lpFreeBytesAvailableToCaller != NULL || lpTotalNumberOfBytes != NULL)
    {
        /* To get the free space available to the user associated with the
           current thread, try FileFsFullSizeInformation. If this is not
           supported by the file system, fall back to FileFsSize */

        Status = NtQueryVolumeInformationFile(hFile,
                                              &IoStatusBlock,
                                              &FsInfo.FsFullSize,
                                              sizeof(FsInfo.FsFullSize),
                                              FileFsFullSizeInformation);

        if (NT_SUCCESS(Status))
        {
            /* Close the handle before returning data
               to avoid a handle leak in case of a fault! */
            CloseHandle(hFile);

            BytesPerCluster.QuadPart =
                FsInfo.FsFullSize.BytesPerSector * FsInfo.FsFullSize.SectorsPerAllocationUnit;

            if (lpFreeBytesAvailableToCaller != NULL)
            {
                lpFreeBytesAvailableToCaller->QuadPart =
                    BytesPerCluster.QuadPart * FsInfo.FsFullSize.CallerAvailableAllocationUnits.QuadPart;
            }

            if (lpTotalNumberOfBytes != NULL)
            {
                lpTotalNumberOfBytes->QuadPart =
                    BytesPerCluster.QuadPart * FsInfo.FsFullSize.TotalAllocationUnits.QuadPart;
            }

            if (lpTotalNumberOfFreeBytes != NULL)
            {
                lpTotalNumberOfFreeBytes->QuadPart =
                    BytesPerCluster.QuadPart * FsInfo.FsFullSize.ActualAvailableAllocationUnits.QuadPart;
            }

            return TRUE;
        }
    }

    Status = NtQueryVolumeInformationFile(hFile,
                                          &IoStatusBlock,
                                          &FsInfo.FsSize,
                                          sizeof(FsInfo.FsSize),
                                          FileFsSizeInformation);

    /* Close the handle before returning data
       to avoid a handle leak in case of a fault! */
    CloseHandle(hFile);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus (Status);
        return FALSE;
    }

    BytesPerCluster.QuadPart =
        FsInfo.FsSize.BytesPerSector * FsInfo.FsSize.SectorsPerAllocationUnit;

    if (lpFreeBytesAvailableToCaller)
    {
        lpFreeBytesAvailableToCaller->QuadPart =
            BytesPerCluster.QuadPart * FsInfo.FsSize.AvailableAllocationUnits.QuadPart;
    }

    if (lpTotalNumberOfBytes)
    {
        lpTotalNumberOfBytes->QuadPart =
            BytesPerCluster.QuadPart * FsInfo.FsSize.TotalAllocationUnits.QuadPart;
    }

    if (lpTotalNumberOfFreeBytes)
    {
        lpTotalNumberOfFreeBytes->QuadPart =
            BytesPerCluster.QuadPart * FsInfo.FsSize.AvailableAllocationUnits.QuadPart;
    }

    return TRUE;
}


/*
 * @implemented
 */
UINT WINAPI
GetDriveTypeA(LPCSTR lpRootPathName)
{
   PWCHAR RootPathNameW;

   if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
      return DRIVE_UNKNOWN;

   return GetDriveTypeW(RootPathNameW);
}


/*
 * @implemented
 */
UINT WINAPI
GetDriveTypeW(LPCWSTR lpRootPathName)
{
	FILE_FS_DEVICE_INFORMATION FileFsDevice;
	IO_STATUS_BLOCK IoStatusBlock;

	HANDLE hFile;
	NTSTATUS errCode;

	hFile = InternalOpenDirW(lpRootPathName, FALSE);
	if (hFile == INVALID_HANDLE_VALUE)
	{
	    return DRIVE_NO_ROOT_DIR;	/* According to WINE regression tests */
	}

	errCode = NtQueryVolumeInformationFile (hFile,
	                                        &IoStatusBlock,
	                                        &FileFsDevice,
	                                        sizeof(FILE_FS_DEVICE_INFORMATION),
	                                        FileFsDeviceInformation);
	if (!NT_SUCCESS(errCode))
	{
		CloseHandle(hFile);
		SetLastErrorByStatus (errCode);
		return 0;
	}
	CloseHandle(hFile);

        switch (FileFsDevice.DeviceType)
        {
		case FILE_DEVICE_CD_ROM:
		case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
			return DRIVE_CDROM;
	        case FILE_DEVICE_VIRTUAL_DISK:
	        	return DRIVE_RAMDISK;
	        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
	        	return DRIVE_REMOTE;
	        case FILE_DEVICE_DISK:
	        case FILE_DEVICE_DISK_FILE_SYSTEM:
			if (FileFsDevice.Characteristics & FILE_REMOTE_DEVICE)
				return DRIVE_REMOTE;
			if (FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA)
				return DRIVE_REMOVABLE;
			return DRIVE_FIXED;
        }

        ERR("Returning DRIVE_UNKNOWN for device type %d\n", FileFsDevice.DeviceType);

	return DRIVE_UNKNOWN;
}


/*
 * @implemented
 */
BOOL WINAPI
GetVolumeInformationA(
	LPCSTR	lpRootPathName,
	LPSTR	lpVolumeNameBuffer,
	DWORD	nVolumeNameSize,
	LPDWORD	lpVolumeSerialNumber,
	LPDWORD	lpMaximumComponentLength,
	LPDWORD	lpFileSystemFlags,
	LPSTR	lpFileSystemNameBuffer,
	DWORD	nFileSystemNameSize
	)
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
BOOL WINAPI
GetVolumeInformationW(
    LPCWSTR lpRootPathName,
    LPWSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPWSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )
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

  TRACE("hFile: %x\n", hFile);
  errCode = NtQueryVolumeInformationFile(hFile,
                                         &IoStatusBlock,
                                         FileFsVolume,
                                         FS_VOLUME_BUFFER_SIZE,
                                         FileFsVolumeInformation);
  if ( !NT_SUCCESS(errCode) )
    {
      WARN("Status: %x\n", errCode);
      CloseHandle(hFile);
      SetLastErrorByStatus (errCode);
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
      SetLastErrorByStatus (errCode);
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
SetVolumeLabelA (
	LPCSTR	lpRootPathName,
	LPCSTR	lpVolumeName /* NULL if deleting label */
	)
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
BOOL WINAPI
SetVolumeLabelW(
   LPCWSTR lpRootPathName,
   LPCWSTR lpVolumeName /* NULL if deleting label */
   )
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
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   CloseHandle(hFile);
   return TRUE;
}

/**
 * @name GetVolumeNameForVolumeMountPointW
 *
 * Return an unique volume name for a drive root or mount point.
 *
 * @param VolumeMountPoint
 *        Pointer to string that contains either root drive name or
 *        mount point name.
 * @param VolumeName
 *        Pointer to buffer that is filled with resulting unique
 *        volume name on success.
 * @param VolumeNameLength
 *        Size of VolumeName buffer in TCHARs.
 *
 * @return
 *     TRUE when the function succeeds and the VolumeName buffer is filled,
 *     FALSE otherwise.
 */

BOOL WINAPI
GetVolumeNameForVolumeMountPointW(
   IN LPCWSTR VolumeMountPoint,
   OUT LPWSTR VolumeName,
   IN DWORD VolumeNameLength)
{
   UNICODE_STRING NtFileName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE FileHandle;
   IO_STATUS_BLOCK Iosb;
   ULONG BufferLength;
   PMOUNTDEV_NAME MountDevName;
   PMOUNTMGR_MOUNT_POINT MountPoint;
   ULONG MountPointSize;
   PMOUNTMGR_MOUNT_POINTS MountPoints;
   ULONG Index;
   PUCHAR SymbolicLinkName;
   BOOL Result;
   NTSTATUS Status;

   /*
    * First step is to convert the passed volume mount point name to
    * an NT acceptable name.
    */

   if (!RtlDosPathNameToNtPathName_U(VolumeMountPoint, &NtFileName, NULL, NULL))
   {
      SetLastError(ERROR_PATH_NOT_FOUND);
      return FALSE;
   }

   if (NtFileName.Length > sizeof(WCHAR) &&
       NtFileName.Buffer[(NtFileName.Length / sizeof(WCHAR)) - 1] == '\\')
   {
      NtFileName.Length -= sizeof(WCHAR);
   }

   /*
    * Query mount point device name which we will later use for determining
    * the volume name.
    */

   InitializeObjectAttributes(&ObjectAttributes, &NtFileName, 0, NULL, NULL);
   Status = NtOpenFile(&FileHandle, FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                       &ObjectAttributes, &Iosb,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       FILE_SYNCHRONOUS_IO_NONALERT);
   RtlFreeUnicodeString(&NtFileName);
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }

   BufferLength = sizeof(MOUNTDEV_NAME) + 50 * sizeof(WCHAR);
   do
   {
      MountDevName = RtlAllocateHeap(GetProcessHeap(), 0, BufferLength);
      if (MountDevName == NULL)
      {
         NtClose(FileHandle);
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         return FALSE;
      }

      Status = NtDeviceIoControlFile(FileHandle, NULL, NULL, NULL, &Iosb,
                                     IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                     NULL, 0, MountDevName, BufferLength);
      if (!NT_SUCCESS(Status))
      {
         RtlFreeHeap(GetProcessHeap(), 0, MountDevName);
         if (Status == STATUS_BUFFER_OVERFLOW)
         {
            BufferLength = sizeof(MOUNTDEV_NAME) + MountDevName->NameLength;
            continue;
         }
         else 
         {
            NtClose(FileHandle);
            SetLastErrorByStatus(Status);
            return FALSE;
         }
      }
   }
   while (!NT_SUCCESS(Status));

   NtClose(FileHandle);

   /*
    * Get the mount point information from mount manager.
    */

   MountPointSize = MountDevName->NameLength + sizeof(MOUNTMGR_MOUNT_POINT);
   MountPoint = RtlAllocateHeap(GetProcessHeap(), 0, MountPointSize);
   if (MountPoint == NULL)
   {
      RtlFreeHeap(GetProcessHeap(), 0, MountDevName);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }
   RtlZeroMemory(MountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
   MountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
   MountPoint->DeviceNameLength = MountDevName->NameLength;
   RtlCopyMemory(MountPoint + 1, MountDevName->Name, MountDevName->NameLength);
   RtlFreeHeap(GetProcessHeap(), 0, MountDevName);

   RtlInitUnicodeString(&NtFileName, L"\\??\\MountPointManager");
   InitializeObjectAttributes(&ObjectAttributes, &NtFileName, 0, NULL, NULL);
   Status = NtOpenFile(&FileHandle, FILE_GENERIC_READ, &ObjectAttributes,
                       &Iosb, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       FILE_SYNCHRONOUS_IO_NONALERT);
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      RtlFreeHeap(GetProcessHeap(), 0, MountPoint);
      return FALSE;
   }

   BufferLength = sizeof(MOUNTMGR_MOUNT_POINTS);
   do
   {
      MountPoints = RtlAllocateHeap(GetProcessHeap(), 0, BufferLength);
      if (MountPoints == NULL)
      {
         RtlFreeHeap(GetProcessHeap(), 0, MountPoint);
         NtClose(FileHandle);
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         return FALSE;
      }

      Status = NtDeviceIoControlFile(FileHandle, NULL, NULL, NULL, &Iosb,
                                     IOCTL_MOUNTMGR_QUERY_POINTS,
                                     MountPoint, MountPointSize,
                                     MountPoints, BufferLength);
      if (!NT_SUCCESS(Status))
      {
         RtlFreeHeap(GetProcessHeap(), 0, MountPoints);
         if (Status == STATUS_BUFFER_OVERFLOW)
         {
            BufferLength = MountPoints->Size;
            continue;
         }
         else if (!NT_SUCCESS(Status))
         {
            RtlFreeHeap(GetProcessHeap(), 0, MountPoint);
            NtClose(FileHandle);
            SetLastErrorByStatus(Status);
            return FALSE;
         }
      }
   }
   while (!NT_SUCCESS(Status));

   RtlFreeHeap(GetProcessHeap(), 0, MountPoint);
   NtClose(FileHandle);

   /*
    * Now we've gathered info about all mount points mapped to our device, so
    * select the correct one and copy it into the output buffer.
    */

   for (Index = 0; Index < MountPoints->NumberOfMountPoints; Index++)
   {
      MountPoint = MountPoints->MountPoints + Index;
      SymbolicLinkName = (PUCHAR)MountPoints + MountPoint->SymbolicLinkNameOffset;
      
      /*
       * Check for "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\"
       * (with the last slash being optional) style symbolic links.
       */

      if (MountPoint->SymbolicLinkNameLength == 48 * sizeof(WCHAR) ||
          (MountPoint->SymbolicLinkNameLength == 49 * sizeof(WCHAR) &&
           SymbolicLinkName[48] == L'\\'))
      {
         if (RtlCompareMemory(SymbolicLinkName, L"\\??\\Volume{",
                              11 * sizeof(WCHAR)) == 11 * sizeof(WCHAR) &&
             SymbolicLinkName[19] == L'-' && SymbolicLinkName[24] == L'-' &&
             SymbolicLinkName[29] == L'-' && SymbolicLinkName[34] == L'-' &&
             SymbolicLinkName[47] == L'}')
         {
            if (VolumeNameLength >= MountPoint->SymbolicLinkNameLength / sizeof(WCHAR))
            {
               RtlCopyMemory(VolumeName,
                             (PUCHAR)MountPoints + MountPoint->SymbolicLinkNameOffset,
                             MountPoint->SymbolicLinkNameLength);
               VolumeName[1] = L'\\';
               Result = TRUE;
            }
            else
            {
               SetLastError(ERROR_FILENAME_EXCED_RANGE);
               Result = FALSE;
            }

            RtlFreeHeap(GetProcessHeap(), 0, MountPoints);

            return Result;
         }
      }
   }

   RtlFreeHeap(GetProcessHeap(), 0, MountPoints);
   SetLastError(ERROR_INVALID_PARAMETER);

   return FALSE;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
BOOL
WINAPI
GetVolumeNameForVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    BOOL ret;
    WCHAR volumeW[50], *pathW = NULL;
    DWORD len = min( sizeof(volumeW) / sizeof(WCHAR), cchBufferLength );

    TRACE("(%s, %p, %x)\n", debugstr_a(lpszVolumeMountPoint), lpszVolumeName, cchBufferLength);

    if (!lpszVolumeMountPoint || !(pathW = FilenameA2W( lpszVolumeMountPoint, TRUE )))
        return FALSE;

    if ((ret = GetVolumeNameForVolumeMountPointW( pathW, volumeW, len )))
        FilenameW2A_N( lpszVolumeName, len, volumeW, -1 );

    RtlFreeHeap( GetProcessHeap(), 0, pathW );
    return ret;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
HANDLE
WINAPI
FindFirstVolumeW(
	LPWSTR volume,
	DWORD len
    )
{
    DWORD size = 1024;
    HANDLE mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, 0 );
    if (mgr == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    for (;;)
    {
        MOUNTMGR_MOUNT_POINT input;
        MOUNTMGR_MOUNT_POINTS *output;

        if (!(output = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            break;
        }
        memset( &input, 0, sizeof(input) );

        if (!DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_POINTS, &input, sizeof(input),
                              output, size, NULL, NULL ))
        {
            if (GetLastError() != ERROR_MORE_DATA) break;
            size = output->Size;
            RtlFreeHeap( GetProcessHeap(), 0, output );
            continue;
        }
        CloseHandle( mgr );
        /* abuse the Size field to store the current index */
        output->Size = 0;
        if (!FindNextVolumeW( output, volume, len ))
        {
            RtlFreeHeap( GetProcessHeap(), 0, output );
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
FindFirstVolumeA(
	LPSTR volume,
	DWORD len
    )
{
    WCHAR *buffer = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    HANDLE handle = FindFirstVolumeW( buffer, len );

    if (handle != INVALID_HANDLE_VALUE)
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL ))
        {
            FindVolumeClose( handle );
            handle = INVALID_HANDLE_VALUE;
        }
    }
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return handle;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
BOOL
WINAPI
FindVolumeClose(
    HANDLE hFindVolume
    )
{
    return RtlFreeHeap(GetProcessHeap(), 0, hFindVolume);
}

/* EOF */
