/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/volume.c
 * PURPOSE:         File volume functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Erik Bos, Alexandre Julliard :
 *                      DRIVE_IsValid, GetLogicalDriveStringsA,
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

#include <windows.h>
#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>


#define MAX_DOS_DRIVES 26


int DRIVE_IsValid( int drive )
{
    char Drives[4];
    Drives[0] = 'A';
    Drives[1] = ':';
    Drives[2] = '\\';
    Drives[3] = 0;

    Drives[0] = 'A' + drive -1;
    if ((drive < 0) || (drive >= MAX_DOS_DRIVES)) return 0;
    if ( CreateFileA(Drives,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_ATTRIBUTE_DIRECTORY,NULL) == INVALID_HANDLE_VALUE ) {
	return 0;
    }
    return drive;
    
}


DWORD
STDCALL
GetLogicalDriveStringsA(
			DWORD nBufferLength,
			LPSTR lpBuffer
			)
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if (count * 4 * sizeof(char) <= nBufferLength)
    {
        LPSTR p = lpBuffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (DRIVE_IsValid(drive))
            {
                *p++ = 'A' + drive;
                *p++ = ':';
                *p++ = '\\';
                *p++ = '\0';
            }
        *p = '\0';
    }
    return count * 4 * sizeof(char);
}


DWORD
STDCALL
GetLogicalDriveStringsW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    int drive, count;

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) count++;
    if (count * 4 * sizeof(WCHAR) <=  nBufferLength)
    {
        LPWSTR p = lpBuffer;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            if (DRIVE_IsValid(drive))
            {
                *p++ = (WCHAR)('A' + drive);
                *p++ = (WCHAR)':';
                *p++ = (WCHAR)'\\';
                *p++ = (WCHAR)'\0';
            }
        *p = (WCHAR)'\0';
    }
    return count * 4 * sizeof(WCHAR);
}


DWORD
STDCALL
GetLogicalDrives(VOID)
{
    DWORD ret = 0;
    int drive;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        if (DRIVE_IsValid(drive)) ret |= (1 << drive);
    return ret;
}



WINBOOL
STDCALL
GetDiskFreeSpaceA(
    LPCSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    WCHAR RootPathNameW[MAX_PATH];

    if (lpRootPathName)
    {
        ULONG i = 0;
   	while ((*lpRootPathName)!=0 && i < MAX_PATH)
     	{
		RootPathNameW[i] = *lpRootPathName;
		lpRootPathName++;
		i++;
     	}
   	RootPathNameW[i] = 0;
    }

    return GetDiskFreeSpaceW(lpRootPathName?RootPathNameW:NULL,
                             lpSectorsPerCluster,
                             lpBytesPerSector,
                             lpNumberOfFreeClusters,
                             lpTotalNumberOfClusters);
}

WINBOOL
STDCALL
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
        SetLastError(RtlNtStatusToDosError(errCode));
        return FALSE;
    }

    *lpBytesPerSector = FileFsSize.BytesPerSector;
    *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.LowPart;
    *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.LowPart;
    CloseHandle(hFile);
    return TRUE;
}

WINBOOL
STDCALL
GetDiskFreeSpaceExA(
    LPCSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    WCHAR DirectoryNameW[MAX_PATH];

    if (lpDirectoryName)
    {
        ULONG i = 0;
        while ((*lpDirectoryName)!=0 && i < MAX_PATH)
        {
            DirectoryNameW[i] = *lpDirectoryName;
            lpDirectoryName++;
            i++;
        }
        DirectoryNameW[i] = 0;
    }

    return GetDiskFreeSpaceExW(lpDirectoryName?DirectoryNameW:NULL,
                               lpFreeBytesAvailableToCaller,
                               lpTotalNumberOfBytes,
                               lpTotalNumberOfFreeBytes);
}


WINBOOL
STDCALL
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
        SetLastError(RtlNtStatusToDosError(errCode));
        return FALSE;
    }

    BytesPerCluster.QuadPart =
        FileFsSize.BytesPerSector * FileFsSize.SectorsPerAllocationUnit;

    // FIXME: Use quota information
    lpFreeBytesAvailableToCaller->QuadPart =
        BytesPerCluster.QuadPart * FileFsSize.AvailableAllocationUnits.QuadPart;

    lpTotalNumberOfBytes->QuadPart =
        BytesPerCluster.QuadPart * FileFsSize.TotalAllocationUnits.LowPart;
    lpTotalNumberOfFreeBytes->QuadPart =
        BytesPerCluster.QuadPart * FileFsSize.AvailableAllocationUnits.QuadPart;

    CloseHandle(hFile);
    return TRUE;
}


UINT
STDCALL
GetDriveTypeA(
    LPCSTR lpRootPathName
    )
{
	ULONG i;
	WCHAR RootPathNameW[MAX_PATH];
    	i = 0;
   	while ((*lpRootPathName)!=0 && i < MAX_PATH)
     	{
		RootPathNameW[i] = *lpRootPathName;
		lpRootPathName++;
		i++;
     	}
   	RootPathNameW[i] = 0;
	return GetDriveTypeW(RootPathNameW);
}

UINT
STDCALL
GetDriveTypeW(
    LPCWSTR lpRootPathName
    )
{
	FILE_FS_DEVICE_INFORMATION FileFsDevice;
	IO_STATUS_BLOCK IoStatusBlock;

	HANDLE hFile;
	NTSTATUS errCode;

	hFile = CreateFileW(
  		lpRootPathName,	
    		GENERIC_ALL,	
    		FILE_SHARE_READ|FILE_SHARE_WRITE,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);

	errCode = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,&FileFsDevice, sizeof(FILE_FS_DEVICE_INFORMATION),FileFsDeviceInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return 0;
	}
	CloseHandle(hFile);
	return (UINT)FileFsDevice.DeviceType;

	

}

WINBOOL
STDCALL
GetVolumeInformationA(
    LPCSTR lpRootPathName,
    LPSTR lpVolumeNameBuffer,
    DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPSTR lpFileSystemNameBuffer,
    DWORD nFileSystemNameSize
    )
{
	ULONG i;
	WCHAR RootPathNameW[MAX_PATH];
	WCHAR VolumeNameBufferW[MAX_PATH];
	WCHAR FileSystemNameBufferW[MAX_PATH];
	

    	i = 0;
   	while ((*lpRootPathName)!=0 && i < MAX_PATH)
     	{
		RootPathNameW[i] = *lpRootPathName;
		lpRootPathName++;
		i++;
     	}
   	RootPathNameW[i] = 0;

	if ( GetVolumeInformationW(RootPathNameW,
    		VolumeNameBufferW,
   		nVolumeNameSize,
    		lpVolumeSerialNumber,
    		lpMaximumComponentLength,
   		lpFileSystemFlags,
    		FileSystemNameBufferW,
 		nFileSystemNameSize ) ) {
		for(i=0;i<nVolumeNameSize;i++)
			lpVolumeNameBuffer[i] = (CHAR)VolumeNameBufferW[i];

		for(i=0;i<nFileSystemNameSize;i++)
			lpFileSystemNameBuffer[i] = (CHAR)FileSystemNameBufferW[i];
		
		return TRUE;
	}
	return FALSE;

}




#define FS_VOLUME_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_VOLUME_INFORMATION))

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))


WINBOOL
STDCALL
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
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

        if (lpVolumeSerialNumber)
                *lpVolumeSerialNumber = FileFsVolume->VolumeSerialNumber;

        if (lpVolumeNameBuffer)
                wcsncpy(lpVolumeNameBuffer, FileFsVolume->VolumeLabel,min(nVolumeNameSize,MAX_PATH));

	errCode = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,FileFsAttribute, FS_ATTRIBUTE_BUFFER_SIZE,FileFsAttributeInformation);
	if ( !NT_SUCCESS(errCode) ) {
                DPRINT("Status: %x\n", errCode);
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
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
SetVolumeLabelA(
    LPCSTR lpRootPathName,
    LPCSTR lpVolumeName
    )
{
	WCHAR RootPathNameW[MAX_PATH];
	WCHAR VolumeNameW[MAX_PATH];
	UINT i;

   	i = 0;
   	while ((*lpRootPathName)!=0 && i < MAX_PATH)
     	{
		RootPathNameW[i] = *lpRootPathName;
		lpRootPathName++;
		i++;
     	}
   	RootPathNameW[i] = 0;

	i = 0;
   	while ((*lpVolumeName)!=0 && i < MAX_PATH)
     	{
		VolumeNameW[i] = *lpVolumeName;
		lpVolumeName++;
		i++;
     	}
   	VolumeNameW[i] = 0;
	return SetVolumeLabelW(RootPathNameW,VolumeNameW);
}

WINBOOL
STDCALL
SetVolumeLabelW(
    LPCWSTR lpRootPathName,
    LPCWSTR lpVolumeName
    )
{
	return FALSE;
}
