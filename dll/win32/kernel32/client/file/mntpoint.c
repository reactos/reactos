/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/mntpoint.c
 * PURPOSE:         File volume mount point functions
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

/**
 * @name GetVolumeNameForVolumeMountPointW
 * @implemented
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
BOOL
WINAPI
GetVolumeNameForVolumeMountPointW(IN LPCWSTR VolumeMountPoint,
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

   if (!VolumeMountPoint || !VolumeMountPoint[0])
   {
       SetLastError(ERROR_PATH_NOT_FOUND);
       return FALSE;
   }

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
      BaseSetLastNTError(Status);
      return FALSE;
   }

   BufferLength = sizeof(MOUNTDEV_NAME) + 50 * sizeof(WCHAR);
   do
   {
      MountDevName = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
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
         if (Status == STATUS_BUFFER_OVERFLOW)
         {
            BufferLength = sizeof(MOUNTDEV_NAME) + MountDevName->NameLength;
            RtlFreeHeap(GetProcessHeap(), 0, MountDevName);
            continue;
         }
         else
         {
            RtlFreeHeap(GetProcessHeap(), 0, MountDevName);
            NtClose(FileHandle);
            BaseSetLastNTError(Status);
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
   RtlFreeHeap(RtlGetProcessHeap(), 0, MountDevName);

   RtlInitUnicodeString(&NtFileName, L"\\??\\MountPointManager");
   InitializeObjectAttributes(&ObjectAttributes, &NtFileName, 0, NULL, NULL);
   Status = NtOpenFile(&FileHandle, FILE_GENERIC_READ, &ObjectAttributes,
                       &Iosb, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       FILE_SYNCHRONOUS_IO_NONALERT);
   if (!NT_SUCCESS(Status))
   {
      BaseSetLastNTError(Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
      return FALSE;
   }

   BufferLength = sizeof(MOUNTMGR_MOUNT_POINTS);
   do
   {
      MountPoints = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
      if (MountPoints == NULL)
      {
         RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
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
         if (Status == STATUS_BUFFER_OVERFLOW)
         {
            BufferLength = MountPoints->Size;
            RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
            continue;
         }
         else if (!NT_SUCCESS(Status))
         {
            RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
            RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
            NtClose(FileHandle);
            BaseSetLastNTError(Status);
            return FALSE;
         }
      }
   }
   while (!NT_SUCCESS(Status));

   RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoint);
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

            RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);

            return Result;
         }
      }
   }

   RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
   SetLastError(ERROR_INVALID_PARAMETER);

   return FALSE;
}

/*
 * @implemented (Wine 13 sep 2008)
 */
BOOL
WINAPI
GetVolumeNameForVolumeMountPointA(IN LPCSTR lpszVolumeMountPoint,
                                  IN LPSTR lpszVolumeName,
                                  IN DWORD cchBufferLength)
{
    BOOL ret;
    WCHAR volumeW[50], *pathW = NULL;
    DWORD len = min( sizeof(volumeW) / sizeof(WCHAR), cchBufferLength );

    TRACE("(%s, %p, %x)\n", debugstr_a(lpszVolumeMountPoint), lpszVolumeName, cchBufferLength);

    if (!lpszVolumeMountPoint || !(pathW = FilenameA2W( lpszVolumeMountPoint, TRUE )))
        return FALSE;

    if ((ret = GetVolumeNameForVolumeMountPointW( pathW, volumeW, len )))
        FilenameW2A_N( lpszVolumeName, len, volumeW, -1 );

    RtlFreeHeap( RtlGetProcessHeap(), 0, pathW );
    return ret;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetVolumeMountPointW(IN LPCWSTR lpszVolumeMountPoint,
                     IN LPCWSTR lpszVolumeName)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetVolumeMountPointA(IN LPCSTR lpszVolumeMountPoint,
                     IN LPCSTR lpszVolumeName)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteVolumeMountPointA(IN LPCSTR lpszVolumeMountPoint)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteVolumeMountPointW(IN LPCWSTR lpszVolumeMountPoint)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstVolumeMountPointW(IN LPCWSTR lpszRootPathName,
                           IN LPWSTR lpszVolumeMountPoint,
                           IN DWORD cchBufferLength)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstVolumeMountPointA(IN LPCSTR lpszRootPathName,
                           IN LPSTR lpszVolumeMountPoint,
                           IN DWORD cchBufferLength)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindNextVolumeMountPointA(IN HANDLE hFindVolumeMountPoint,
                          IN LPSTR lpszVolumeMountPoint,
                          DWORD cchBufferLength)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindNextVolumeMountPointW(IN HANDLE hFindVolumeMountPoint,
                          IN LPWSTR lpszVolumeMountPoint,
                          DWORD cchBufferLength)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindVolumeMountPointClose(IN HANDLE hFindVolumeMountPoint)
{
    STUB;
    return 0;
}

/* EOF */
