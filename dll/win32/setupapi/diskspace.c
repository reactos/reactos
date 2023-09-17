/*
 * SetupAPI DiskSpace functions
 *
 * Copyright 2016 Michael Müller
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
#include "wine/list.h"

struct file_entry
{
    struct list entry;
    WCHAR *path;
    UINT operation;
    LONGLONG size;
};

struct space_list
{
    struct list files;
    UINT flags;
};

static LONGLONG get_file_size(WCHAR *path)
{
    HANDLE file;
    LARGE_INTEGER size;

    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;

    if (!GetFileSizeEx(file, &size))
        size.QuadPart = 0;

    CloseHandle(file);
    return size.QuadPart;
}

/***********************************************************************
 *      SetupCreateDiskSpaceListW  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListW(PVOID reserved1, DWORD reserved2, UINT flags)
{
    struct space_list *list;

    TRACE("(%p, %lu, 0x%08x)\n", reserved1, reserved2, flags);

    if (reserved1 || reserved2 || flags & ~SPDSL_IGNORE_DISK)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    list = malloc(sizeof(*list));
    if (list)
    {
        list->flags = flags;
        list_init(&list->files);
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
HDSKSPC WINAPI SetupDuplicateDiskSpaceListW(HDSKSPC diskspace, PVOID reserved1, DWORD reserved2, UINT flags)
{
    struct space_list *list_copy, *list = diskspace;
    struct file_entry *file, *file_copy;

    TRACE("(%p, %p, %lu, %u)\n", diskspace, reserved1, reserved2, flags);

    if (reserved1 || reserved2 || flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    list_copy = malloc(sizeof(*list_copy));
    if (!list_copy)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    list_copy->flags = list->flags;
    list_init(&list_copy->files);

    LIST_FOR_EACH_ENTRY(file, &list->files, struct file_entry, entry)
    {
        file_copy = malloc(sizeof(*file_copy));
        if (!file_copy) goto error;

        file_copy->path = wcsdup(file->path);
        if (!file_copy->path)
        {
            free(file_copy);
            goto error;
        }

        file_copy->operation = file->operation;
        file_copy->size = file->size;
        list_add_head(&list_copy->files, &file->entry);
    }

    return list_copy;

error:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    SetupDestroyDiskSpaceList(list_copy);
    return NULL;
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
BOOL WINAPI SetupQuerySpaceRequiredOnDriveW(HDSKSPC diskspace,
                        LPCWSTR drivespec, LONGLONG *required,
                        PVOID reserved1, UINT reserved2)
{
    struct space_list *list = diskspace;
    struct file_entry *file;
    LONGLONG sum = 0;

    TRACE("(%p, %s, %p, %p, %u)\n", diskspace, debugstr_w(drivespec), required, reserved1, reserved2);

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!drivespec || !drivespec[0])
    {
        SetLastError(drivespec ? ERROR_INVALID_DRIVE : ERROR_INVALID_DRIVE);
        return FALSE;
    }

    if (!required)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (towlower(drivespec[0]) < 'a' || towlower(drivespec[0]) > 'z' ||
        drivespec[1] != ':' || drivespec[2] != 0)
    {
        FIXME("UNC paths not yet supported (%s)\n", debugstr_w(drivespec));
        SetLastError((GetVersion() & 0x80000000) ? ERROR_INVALID_DRIVE : ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    LIST_FOR_EACH_ENTRY(file, &list->files, struct file_entry, entry)
    {
        if (towlower(file->path[0]) == towlower(drivespec[0]) &&
            file->path[1] == ':' && file->path[2] == '\\')
            sum += file->size;
    }

    *required = sum;
    return TRUE;
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
BOOL WINAPI SetupDestroyDiskSpaceList(HDSKSPC diskspace)
{
    struct space_list *list = diskspace;
    struct file_entry *file, *file2;

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    LIST_FOR_EACH_ENTRY_SAFE(file, file2, &list->files, struct file_entry, entry)
    {
        free(file->path);
        list_remove(&file->entry);
        free(file);
    }

    free(list);
    return TRUE;
}

/***********************************************************************
*		SetupAddToDiskSpaceListW  (SETUPAPI.@)
*/
BOOL WINAPI SetupAddToDiskSpaceListW(HDSKSPC diskspace, PCWSTR targetfile,
                                    LONGLONG filesize, UINT operation,
                                    PVOID reserved1, UINT reserved2)
{
    struct space_list *list = diskspace;
    struct file_entry *file;
    WCHAR *fullpathW;
    BOOL ret = FALSE;
    DWORD size;

    TRACE("(%p, %s, %s, %u, %p, %u)\n", diskspace, debugstr_w(targetfile),
          wine_dbgstr_longlong(filesize), operation, reserved1, reserved2);

    if (!targetfile)
        return TRUE;

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (operation != FILEOP_COPY && operation != FILEOP_DELETE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    size = GetFullPathNameW(targetfile, 0, NULL, NULL);
    if (!size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    size = (size+1) * sizeof(WCHAR);
    fullpathW = HeapAlloc(GetProcessHeap(), 0, size);

    if (!GetFullPathNameW(targetfile, size, fullpathW, NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (fullpathW[1] != ':' && fullpathW[2] != '\\')
    {
        FIXME("UNC paths not yet supported\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto done;
    }

    LIST_FOR_EACH_ENTRY(file, &list->files, struct file_entry, entry)
    {
        if (!lstrcmpiW(file->path, fullpathW))
            break;
    }

    if (&file->entry == &list->files)
    {
        file = HeapAlloc(GetProcessHeap(), 0, sizeof(*file));
        if (!file)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }

        file->path = wcsdup(fullpathW);
        if (!file->path)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            HeapFree(GetProcessHeap(), 0, file);
            goto done;
        }

        list_add_tail(&list->files, &file->entry);
    }

    file->operation = operation;
    if (operation == FILEOP_COPY)
        file->size = filesize;
    else
        file->size = 0;

    if (!(list->flags & SPDSL_IGNORE_DISK))
        file->size -= get_file_size(fullpathW);

    ret = TRUE;

done:
    HeapFree(GetProcessHeap(), 0, fullpathW);
    return ret;
}

/***********************************************************************
*       SetupAddToDiskSpaceListA  (SETUPAPI.@)
*/
BOOL WINAPI SetupAddToDiskSpaceListA(HDSKSPC diskspace, PCSTR targetfile,
                                    LONGLONG filesize, UINT operation,
                                    PVOID reserved1, UINT reserved2)
{
    LPWSTR targetfileW = NULL;
    DWORD len;
    BOOL ret;

    if (targetfile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, targetfile, -1, NULL, 0);

        targetfileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!targetfileW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP, 0, targetfile, -1, targetfileW, len);
    }

    ret = SetupAddToDiskSpaceListW(diskspace, targetfileW, filesize,
                                   operation, reserved1, reserved2);
    if (targetfileW) HeapFree(GetProcessHeap(), 0, targetfileW);
    return ret;
}
