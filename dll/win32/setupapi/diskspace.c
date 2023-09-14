/*
 * SetupAPI DiskSpace functions
 *
 * Copyright 2004 CodeWeavers (Aric Stewart)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

typedef struct {
    WCHAR   lpzName[20];
    LONGLONG dwFreeSpace;
    LONGLONG dwWantedSpace;
} DRIVE_ENTRY, *LPDRIVE_ENTRY;

typedef struct {
    DWORD   dwDriveCount;
    DRIVE_ENTRY Drives[26];
} DISKSPACELIST, *LPDISKSPACELIST;


/***********************************************************************
 *      SetupCreateDiskSpaceListW  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListW(PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    WCHAR drives[255];
    DWORD rc;
    WCHAR *ptr;
    LPDISKSPACELIST list=NULL;

    TRACE("(%p, %lu, 0x%08x)\n", Reserved1, Reserved2, Flags);

    if (Reserved1 || Reserved2 || Flags & ~SPDSL_IGNORE_DISK)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    rc = GetLogicalDriveStringsW(255,drives);

    if (rc == 0)
        return NULL;

    list = malloc(sizeof(DISKSPACELIST));

    list->dwDriveCount = 0;

    ptr = drives;

    while (*ptr)
    {
        DWORD type = GetDriveTypeW(ptr);
        if (type == DRIVE_FIXED)
        {
            DWORD clusters;
            DWORD sectors;
            DWORD bytes;
            DWORD total;
            lstrcpyW(list->Drives[list->dwDriveCount].lpzName,ptr);
            GetDiskFreeSpaceW(ptr,&sectors,&bytes,&clusters,&total);
            list->Drives[list->dwDriveCount].dwFreeSpace = clusters * sectors *
                                                           bytes;
            list->Drives[list->dwDriveCount].dwWantedSpace = 0;
            list->dwDriveCount++;
        }
       ptr += lstrlenW(ptr) + 1;
    }
    return list;
}


/***********************************************************************
 *		SetupCreateDiskSpaceListA  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListA(PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    return SetupCreateDiskSpaceListW( Reserved1, Reserved2, Flags );
}

/***********************************************************************
 *		SetupDuplicateDiskSpaceListW  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupDuplicateDiskSpaceListW(HDSKSPC DiskSpace, PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    DISKSPACELIST *list_copy, *list_original = DiskSpace;

    if (Reserved1 || Reserved2 || Flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!DiskSpace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    list_copy = malloc(sizeof(DISKSPACELIST));
    if (!list_copy)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    *list_copy = *list_original;

    return list_copy;
}

/***********************************************************************
 *		SetupDuplicateDiskSpaceListA  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupDuplicateDiskSpaceListA(HDSKSPC DiskSpace, PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    return SetupDuplicateDiskSpaceListW(DiskSpace, Reserved1, Reserved2, Flags);
}

/***********************************************************************
 *		SetupAddInstallSectionToDiskSpaceListA  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddInstallSectionToDiskSpaceListA(HDSKSPC DiskSpace,
                        HINF InfHandle, HINF LayoutInfHandle,
                        LPCSTR SectionName, PVOID Reserved1, UINT Reserved2)
{
    FIXME ("Stub\n");
    return TRUE;
}

/***********************************************************************
*		SetupQuerySpaceRequiredOnDriveW  (SETUPAPI.@)
*/
BOOL WINAPI SetupQuerySpaceRequiredOnDriveW(HDSKSPC DiskSpace,
                        LPCWSTR DriveSpec, LONGLONG *SpaceRequired,
                        PVOID Reserved1, UINT Reserved2)
{
    WCHAR *driveW;
    unsigned int i;
    LPDISKSPACELIST list = DiskSpace;
    BOOL rc = FALSE;
    static const WCHAR bkslsh[]= {'\\',0};

    if (!DiskSpace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!DriveSpec)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    driveW = malloc((wcslen(DriveSpec) + 2) * sizeof(WCHAR));
    if (!driveW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    lstrcpyW(driveW,DriveSpec);
    lstrcatW(driveW,bkslsh);

    TRACE("Looking for drive %s\n",debugstr_w(driveW));
 
    for (i = 0; i < list->dwDriveCount; i++)
    {
        TRACE("checking drive %s\n",debugstr_w(list->Drives[i].lpzName));
        if (wcscmp(driveW,list->Drives[i].lpzName)==0)
        {
            rc = TRUE;
            *SpaceRequired = list->Drives[i].dwWantedSpace;
            break;
        }
    }

    free(driveW);

    if (!rc) SetLastError(ERROR_INVALID_DRIVE);
    return rc;
}

/***********************************************************************
*		SetupQuerySpaceRequiredOnDriveA  (SETUPAPI.@)
*/
BOOL WINAPI SetupQuerySpaceRequiredOnDriveA(HDSKSPC DiskSpace,
                        LPCSTR DriveSpec, LONGLONG *SpaceRequired,
                        PVOID Reserved1, UINT Reserved2)
{
    DWORD len;
    LPWSTR DriveSpecW;
    BOOL ret;

    /* The parameter validation checks are in a different order from the
     * Unicode variant of SetupQuerySpaceRequiredOnDrive. */
    if (!DriveSpec)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!DiskSpace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, DriveSpec, -1, NULL, 0);

    DriveSpecW = malloc(len * sizeof(WCHAR));
    if (!DriveSpecW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, DriveSpec, -1, DriveSpecW, len);

    ret = SetupQuerySpaceRequiredOnDriveW(DiskSpace, DriveSpecW, SpaceRequired,
                                          Reserved1, Reserved2);

    free(DriveSpecW);

    return ret;
}

/***********************************************************************
*		SetupDestroyDiskSpaceList  (SETUPAPI.@)
*/
BOOL WINAPI SetupDestroyDiskSpaceList(HDSKSPC DiskSpace)
{
    LPDISKSPACELIST list = (LPDISKSPACELIST)DiskSpace;
    free(list);
    return TRUE;
}

/***********************************************************************
*		SetupAddToDiskSpaceListA  (SETUPAPI.@)
*/
BOOL WINAPI SetupAddToDiskSpaceListA(HDSKSPC diskspace, PCSTR targetfile,
                                    LONGLONG filesize, UINT operation,
                                    PVOID reserved1, UINT reserved2)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
*		SetupAddToDiskSpaceListW  (SETUPAPI.@)
*/
BOOL WINAPI SetupAddToDiskSpaceListW(HDSKSPC diskspace, PCWSTR targetfile,
                                    LONGLONG filesize, UINT operation,
                                    PVOID reserved1, UINT reserved2)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
