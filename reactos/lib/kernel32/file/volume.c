/* $Id: volume.c,v 1.20 2001/06/15 11:14:06 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <napi/shared_data.h>
#include <windows.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


#define MAX_DOS_DRIVES 26


DWORD STDCALL
GetLogicalDriveStringsA(DWORD nBufferLength,
			LPSTR lpBuffer)
{
   DWORD drive, count;
   DWORD dwDriveMap;

   dwDriveMap = ((PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE)->DosDeviceMap;

   for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
     {
	if (dwDriveMap & (1<<drive))
	   count++;
     }


   if (count * 4 * sizeof(char) <= nBufferLength)
     {
	LPSTR p = lpBuffer;

	for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
	  if (dwDriveMap & (1<<drive))
	  {
	     *p++ = 'A' + drive;
	     *p++ = ':';
	     *p++ = '\\';
	     *p++ = '\0';
	  }
	*p = '\0';
     }
    return (count * 4 * sizeof(char));
}


DWORD STDCALL
GetLogicalDriveStringsW(DWORD nBufferLength,
			LPWSTR lpBuffer)
{
   DWORD drive, count;
   DWORD dwDriveMap;

   dwDriveMap = ((PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE)->DosDeviceMap;

   for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
     {
	if (dwDriveMap & (1<<drive))
	   count++;
     }

    if (count * 4 * sizeof(WCHAR) <=  nBufferLength)
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
    return (count * 4 * sizeof(WCHAR));
}


DWORD STDCALL
GetLogicalDrives(VOID)
{
   return (((PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE)->DosDeviceMap);
}


WINBOOL STDCALL
GetDiskFreeSpaceA (
	LPCSTR	lpRootPathName,
	LPDWORD	lpSectorsPerCluster,
	LPDWORD	lpBytesPerSector,
	LPDWORD	lpNumberOfFreeClusters,
	LPDWORD	lpTotalNumberOfClusters
	)
{
	UNICODE_STRING RootPathNameU;
	ANSI_STRING RootPathName;
	WINBOOL Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);

	RtlInitUnicodeString (&RootPathNameU,
	                      NULL);

	if (lpRootPathName)
	{
		/* convert ansi (or oem) string to unicode */
		if (bIsFileApiAnsi)
			RtlAnsiStringToUnicodeString (&RootPathNameU,
			                              &RootPathName,
			                              TRUE);
		else
			RtlOemStringToUnicodeString (&RootPathNameU,
			                             &RootPathName,
			                             TRUE);
	}

	Result = GetDiskFreeSpaceW (RootPathNameU.Buffer,
	                            lpSectorsPerCluster,
	                            lpBytesPerSector,
	                            lpNumberOfFreeClusters,
	                            lpTotalNumberOfClusters);

	if (lpRootPathName)
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             RootPathNameU.Buffer);
	}

	return Result;
}


WINBOOL STDCALL
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
        RootPathName[3] = 0;
    }
	
    hFile = CreateFileW(RootPathName,
                        FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

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

    *lpBytesPerSector = FileFsSize.BytesPerSector;
    *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.u.LowPart;
    *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.u.LowPart;
    CloseHandle(hFile);
    return TRUE;
}


WINBOOL STDCALL
GetDiskFreeSpaceExA (
	LPCSTR		lpDirectoryName,
	PULARGE_INTEGER	lpFreeBytesAvailableToCaller,
	PULARGE_INTEGER	lpTotalNumberOfBytes,
	PULARGE_INTEGER	lpTotalNumberOfFreeBytes
	)
{
	UNICODE_STRING DirectoryNameU;
	ANSI_STRING DirectoryName;
	WINBOOL Result;

	RtlInitAnsiString (&DirectoryName,
	                   (LPSTR)lpDirectoryName);

	RtlInitUnicodeString (&DirectoryNameU,
	                      NULL);

	if (lpDirectoryName)
	{
		/* convert ansi (or oem) string to unicode */
		if (bIsFileApiAnsi)
			RtlAnsiStringToUnicodeString (&DirectoryNameU,
			                              &DirectoryName,
			                              TRUE);
		else
			RtlOemStringToUnicodeString (&DirectoryNameU,
			                             &DirectoryName,
			                             TRUE);
	}

	Result = GetDiskFreeSpaceExW (DirectoryNameU.Buffer,
	                              lpFreeBytesAvailableToCaller,
	                              lpTotalNumberOfBytes,
	                              lpTotalNumberOfFreeBytes);

	if (lpDirectoryName)
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             DirectoryNameU.Buffer);
	}

	return Result;
}


WINBOOL STDCALL
GetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    FILE_FS_SIZE_INFORMATION FileFsSize;
    IO_STATUS_BLOCK IoStatusBlock;
    ULARGE_INTEGER BytesPerCluster;
    WCHAR RootPathName[MAX_PATH];
    HANDLE hFile;
    NTSTATUS errCode;

    if (lpDirectoryName)
    {
        wcsncpy (RootPathName, lpDirectoryName, 3);
    }
    else
    {
        GetCurrentDirectoryW (MAX_PATH, RootPathName);
        RootPathName[3] = 0;
    }
	
    hFile = CreateFileW(RootPathName,
                        FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

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

    BytesPerCluster.QuadPart =
        FileFsSize.BytesPerSector * FileFsSize.SectorsPerAllocationUnit;

    // FIXME: Use quota information
    lpFreeBytesAvailableToCaller->QuadPart =
        BytesPerCluster.QuadPart * FileFsSize.AvailableAllocationUnits.QuadPart;

    lpTotalNumberOfBytes->QuadPart =
        BytesPerCluster.QuadPart * FileFsSize.TotalAllocationUnits.QuadPart;
    lpTotalNumberOfFreeBytes->QuadPart =
        BytesPerCluster.QuadPart * FileFsSize.AvailableAllocationUnits.QuadPart;

    CloseHandle(hFile);
    return TRUE;
}


UINT STDCALL
GetDriveTypeA(LPCSTR lpRootPathName)
{
	UNICODE_STRING RootPathNameU;
	ANSI_STRING RootPathName;
	UINT Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&RootPathNameU,
		                              &RootPathName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&RootPathNameU,
		                             &RootPathName,
		                             TRUE);

	Result = GetDriveTypeW (RootPathNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             RootPathNameU.Buffer);

	return Result;
}


UINT STDCALL
GetDriveTypeW(LPCWSTR lpRootPathName)
{
	FILE_FS_DEVICE_INFORMATION FileFsDevice;
	IO_STATUS_BLOCK IoStatusBlock;

	HANDLE hFile;
	NTSTATUS errCode;

	hFile = CreateFileW (lpRootPathName,
	                     GENERIC_ALL,
	                     FILE_SHARE_READ|FILE_SHARE_WRITE,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);

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
	return (UINT)FileFsDevice.DeviceType;
}


WINBOOL STDCALL
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
	UNICODE_STRING RootPathNameU;
	UNICODE_STRING FileSystemNameU;
	UNICODE_STRING VolumeNameU;
	ANSI_STRING RootPathName;
	ANSI_STRING VolumeName;
	ANSI_STRING FileSystemName;
	WINBOOL Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&RootPathNameU,
		                              &RootPathName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&RootPathNameU,
		                             &RootPathName,
		                             TRUE);

	VolumeNameU.Length = 0;
	VolumeNameU.MaximumLength = nVolumeNameSize * sizeof(WCHAR);
	VolumeNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                      0,
	                                      VolumeNameU.MaximumLength);

	FileSystemNameU.Length = 0;
	FileSystemNameU.MaximumLength = nFileSystemNameSize * sizeof(WCHAR);
	FileSystemNameU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                          0,
	                                          FileSystemNameU.MaximumLength);

	Result = GetVolumeInformationW (RootPathNameU.Buffer,
	                                VolumeNameU.Buffer,
	                                nVolumeNameSize,
	                                lpVolumeSerialNumber,
	                                lpMaximumComponentLength,
	                                lpFileSystemFlags,
	                                FileSystemNameU.Buffer,
	                                nFileSystemNameSize);

	if (Result)
	{
		VolumeName.Length = 0;
		VolumeName.MaximumLength = nVolumeNameSize;
		VolumeName.Buffer = lpVolumeNameBuffer;

		FileSystemName.Length = 0;
		FileSystemName.MaximumLength = nFileSystemNameSize;
		FileSystemName.Buffer = lpFileSystemNameBuffer;

		/* convert unicode strings to ansi (or oem) */
		if (bIsFileApiAnsi)
		{
			RtlUnicodeStringToAnsiString (&VolumeName,
			                              &VolumeNameU,
			                              FALSE);
			RtlUnicodeStringToAnsiString (&FileSystemName,
			                              &FileSystemNameU,
			                              FALSE);
		}
		else
		{
			RtlUnicodeStringToOemString (&VolumeName,
			                             &VolumeNameU,
			                             FALSE);
			RtlUnicodeStringToOemString (&FileSystemName,
			                             &FileSystemNameU,
			                             FALSE);
		}
	}

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             RootPathNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             VolumeNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             FileSystemNameU.Buffer);

	return Result;
}




#define FS_VOLUME_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_VOLUME_INFORMATION))

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))


WINBOOL STDCALL
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
	USHORT Buffer[FS_VOLUME_BUFFER_SIZE];
	USHORT Buffer2[FS_ATTRIBUTE_BUFFER_SIZE];

	HANDLE hFile;
	NTSTATUS errCode;

        FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;
        FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer2;

        DPRINT("FileFsVolume %p\n", FileFsVolume);
        DPRINT("FileFsAttribute %p\n", FileFsAttribute);

        hFile = CreateFileW(lpRootPathName,
                            FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        DPRINT("hFile: %x\n", hFile);
        errCode = NtQueryVolumeInformationFile(hFile,
                                               &IoStatusBlock,
                                               FileFsVolume,
                                               FS_VOLUME_BUFFER_SIZE,
                                               FileFsVolumeInformation);
	if ( !NT_SUCCESS(errCode) ) {
                DPRINT("Status: %x\n", errCode);
                CloseHandle(hFile);
		SetLastErrorByStatus (errCode);
		return FALSE;
	}

        if (lpVolumeSerialNumber)
                *lpVolumeSerialNumber = FileFsVolume->VolumeSerialNumber;

	if (lpVolumeNameBuffer)
	        wcsncpy (lpVolumeNameBuffer,
	                 FileFsVolume->VolumeLabel,
	                 min(nVolumeNameSize,MAX_PATH));

	errCode = NtQueryVolumeInformationFile (hFile,
	                                        &IoStatusBlock,
	                                        FileFsAttribute,
	                                        FS_ATTRIBUTE_BUFFER_SIZE,
	                                        FileFsAttributeInformation);
	if (!NT_SUCCESS(errCode))
	{
		DPRINT("Status: %x\n", errCode);
		CloseHandle(hFile);
		SetLastErrorByStatus (errCode);
		return FALSE;
	}

        if (lpFileSystemFlags)
                *lpFileSystemFlags = FileFsAttribute->FileSystemAttributes;
        if (lpMaximumComponentLength)
                *lpMaximumComponentLength = FileFsAttribute->MaximumComponentNameLength;
        if (lpFileSystemNameBuffer)
                wcsncpy(lpFileSystemNameBuffer, FileFsAttribute->FileSystemName,min(nFileSystemNameSize,MAX_PATH));

	CloseHandle(hFile);
	return TRUE;
}


WINBOOL
STDCALL
SetVolumeLabelA (
	LPCSTR	lpRootPathName,
	LPCSTR	lpVolumeName
	)
{
	UNICODE_STRING RootPathNameU;
	ANSI_STRING RootPathName;
	UNICODE_STRING VolumeNameU;
	ANSI_STRING VolumeName;
	WINBOOL Result;

	RtlInitAnsiString (&RootPathName,
	                   (LPSTR)lpRootPathName);
	RtlInitAnsiString (&VolumeName,
	                   (LPSTR)lpVolumeName);

	/* convert ansi (or oem) strings to unicode */
	if (bIsFileApiAnsi)
	{
		RtlAnsiStringToUnicodeString (&RootPathNameU,
		                              &RootPathName,
		                              TRUE);
		RtlAnsiStringToUnicodeString (&VolumeNameU,
		                              &VolumeName,
		                              TRUE);
	}
	else
	{
		RtlOemStringToUnicodeString (&RootPathNameU,
		                             &RootPathName,
		                             TRUE);
		RtlOemStringToUnicodeString (&VolumeNameU,
		                             &VolumeName,
		                             TRUE);
	}

	Result = SetVolumeLabelW (RootPathNameU.Buffer,
	                          VolumeNameU.Buffer);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             RootPathNameU.Buffer);
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             VolumeNameU.Buffer);

	return Result;
}


WINBOOL STDCALL
SetVolumeLabelW(LPCWSTR lpRootPathName,
		LPCWSTR lpVolumeName)
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
   LabelInfo->VolumeLabelLength = LabelLength;
   wcscpy(LabelInfo->VolumeLabel,
	  lpVolumeName);
   
   hFile = CreateFileW(lpRootPathName,
		       FILE_WRITE_ATTRIBUTES,
		       FILE_SHARE_READ|FILE_SHARE_WRITE,
		       NULL,
		       OPEN_EXISTING,
		       FILE_ATTRIBUTE_NORMAL,
		       NULL);
   DPRINT("hFile: %x\n", hFile);
   
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
	DPRINT("Status: %x\n", Status);
	CloseHandle(hFile);
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   CloseHandle(hFile);
   return TRUE;
}

/* EOF */
