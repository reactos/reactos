/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/volume.c
 * PURPOSE:         File volume functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <wstring.h>
#include <string.h>
#include <ddk/li.h>

DWORD
STDCALL
GetLogicalDrives(VOID)
{
	return (DWORD)-1;
}

DWORD
STDCALL
GetLogicalDriveStringsA(
			DWORD nBufferLength,
			LPSTR lpBuffer
			)
{
}

DWORD
STDCALL
GetLogicalDriveStringsW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
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
	return GetDiskFreeSpaceW(RootPathNameW,lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters );
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
	HANDLE hFile;
	NTSTATUS errCode;
	
	hFile = CreateFileW(
  		lpRootPathName,	
    		GENERIC_READ,	
    		FILE_SHARE_READ,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);

	errCode = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,&FileFsSize, sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

	*lpBytesPerSector = FileFsSize.BytesPerSector;
	*lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
	*lpNumberOfFreeClusters = GET_LARGE_INTEGER_LOW_PART(FileFsSize.AvailableAllocationUnits);
   	*lpTotalNumberOfClusters = GET_LARGE_INTEGER_LOW_PART(FileFsSize.TotalAllocationUnits);
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
	FILE_FS_VOLUME_INFORMATION *FileFsVolume;
	FILE_FS_ATTRIBUTE_INFORMATION *FileFsAttribute;
	IO_STATUS_BLOCK IoStatusBlock;
	USHORT Buffer[FS_VOLUME_BUFFER_SIZE];
	USHORT Buffer2[FS_ATTRIBUTE_BUFFER_SIZE];

	HANDLE hFile;
	NTSTATUS errCode;

	FileFsVolume = (FILE_FS_VOLUME_INFORMATION *)Buffer;
	FileFsAttribute =  (FILE_FS_VOLUME_INFORMATION *)Buffer2;
	
	hFile = CreateFileW(
  		lpRootPathName,	
    		GENERIC_ALL,	
    		FILE_SHARE_READ|FILE_SHARE_WRITE,	
    		NULL,	
    		OPEN_EXISTING,	
    		FILE_ATTRIBUTE_NORMAL,	
    		NULL 
   	);

	errCode = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,FileFsVolume, FS_VOLUME_BUFFER_SIZE,FileFsVolumeInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}

	memcpy(lpVolumeSerialNumber, &FileFsVolume->VolumeSerialNumber, sizeof(DWORD));
	memcpy(lpVolumeNameBuffer, FileFsVolume->VolumeLabel,min(nVolumeNameSize,MAX_PATH));

	errCode = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,FileFsAttribute, FS_ATTRIBUTE_BUFFER_SIZE,FileFsAttributeInformation);
	if ( !NT_SUCCESS(errCode) ) {
		CloseHandle(hFile);
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	memcpy(lpFileSystemFlags,&FileFsAttribute->FileSystemAttributes,sizeof(DWORD));
	memcpy(lpMaximumComponentLength, &FileFsAttribute->MaximumComponentNameLength, sizeof(DWORD));
	memcpy(lpFileSystemNameBuffer, FileFsAttribute->FileSystemName,min(nFileSystemNameSize,MAX_PATH));
	CloseHandle(hFile);
	return TRUE;
	
}

