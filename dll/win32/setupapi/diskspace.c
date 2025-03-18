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

#include <stdlib.h>
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

static BOOL get_size_from_inf(HINF layoutinf, WCHAR *filename, LONGLONG *size)
{
    static const WCHAR SourceDisksFiles[]  = {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s',0};
    INFCONTEXT context;
    WCHAR buffer[20];

    if (!SetupFindFirstLineW(layoutinf, SourceDisksFiles, filename, &context))
        return FALSE;

    if (!SetupGetStringFieldW(&context, 3, buffer, sizeof(buffer), NULL))
        return FALSE;

    /* FIXME: is there a atollW ? */
    *size = wcstol(buffer, NULL, 10);
    return TRUE;
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
 *      SetupAddSectionToDiskSpaceListW  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddSectionToDiskSpaceListW(HDSKSPC diskspace, HINF hinf, HINF hlist,
                                            PCWSTR section, UINT operation, PVOID reserved1,
                                            UINT reserved2)
{
    static const WCHAR sepW[] = {'\\',0};
    WCHAR dest[MAX_PATH], src[MAX_PATH], *dest_dir, *full_path;
    INFCONTEXT context;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %p, %s, %u, %p, %u)\n", diskspace, hinf, hlist, debugstr_w(section),
                                            operation, reserved1, reserved2);

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!section)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!hlist) hlist = hinf;

    if (!SetupFindFirstLineW(hlist, section, NULL, &context))
    {
        SetLastError(ERROR_SECTION_NOT_FOUND);
        return FALSE;
    }

    dest_dir = get_destination_dir(hinf, section);
    if (!dest_dir)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    do
    {
        LONGLONG filesize;
        int path_size;
        BOOL tmp_ret;

        if (!SetupGetStringFieldW(&context, 1, dest, sizeof(dest) / sizeof(WCHAR), NULL))
            goto end;
        if (!SetupGetStringFieldW(&context, 2, src, sizeof(src) / sizeof(WCHAR), NULL))
            *src = 0;
        if (!get_size_from_inf(hinf, src[0] ? src : dest, &filesize))
            goto end;

        path_size = lstrlenW(dest_dir) + lstrlenW(dest) + 2;
        full_path = HeapAlloc(GetProcessHeap(), 0, path_size * sizeof(WCHAR));
        if (!full_path)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto end;
        }

        lstrcpyW(full_path, dest_dir);
        lstrcatW(full_path, sepW);
        lstrcatW(full_path, dest);

        tmp_ret = SetupAddToDiskSpaceListW(diskspace, full_path, filesize, operation, 0, 0);
        HeapFree(GetProcessHeap(), 0, full_path);
        if (!tmp_ret) goto end;
    }
    while (SetupFindNextLine(&context, &context));

    ret = TRUE;

end:
    HeapFree(GetProcessHeap(), 0, dest_dir);
    return ret;
}

/***********************************************************************
 *      SetupAddSectionToDiskSpaceListA  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddSectionToDiskSpaceListA(HDSKSPC diskspace, HINF hinf, HINF hlist,
                                            PCSTR section, UINT operation, PVOID reserved1,
                                            UINT reserved2)
{
    LPWSTR sectionW = NULL;
    DWORD len;
    BOOL ret;

    if (section)
    {
        len = MultiByteToWideChar(CP_ACP, 0, section, -1, NULL, 0);

        sectionW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!sectionW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, len);
    }

    ret = SetupAddSectionToDiskSpaceListW(diskspace, hinf, hlist, sectionW, operation,
                                          reserved1, reserved2);
    if (sectionW) HeapFree(GetProcessHeap(), 0, sectionW);
    return ret;
}

/***********************************************************************
 *      SetupAddInstallSectionToDiskSpaceListW  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddInstallSectionToDiskSpaceListW(HDSKSPC diskspace,
                        HINF inf, HINF layoutinf, LPCWSTR section,
                        PVOID reserved1, UINT reserved2)
{
    static const WCHAR CopyFiles[]  = {'C','o','p','y','F','i','l','e','s',0};
    static const WCHAR DelFiles[]   = {'D','e','l','F','i','l','e','s',0};
    WCHAR section_name[MAX_PATH];
    INFCONTEXT context;
    BOOL ret;
    int i;

    TRACE("(%p, %p, %p, %s, %p, %u)\n", diskspace, inf, layoutinf, debugstr_w(section),
                                        reserved1, reserved2);

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!section)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!inf) return TRUE;
    if (!layoutinf) layoutinf = inf;

    ret = SetupFindFirstLineW(inf, section, CopyFiles, &context);
    while (ret)
    {
        for (i = 1;; i++)
        {
            if (!SetupGetStringFieldW(&context, i, section_name, sizeof(section_name) / sizeof(WCHAR), NULL))
                break;
            SetupAddSectionToDiskSpaceListW(diskspace, layoutinf, inf, section_name, FILEOP_COPY, 0, 0);
        }
        ret = SetupFindNextLine(&context, &context);
    }

    ret = SetupFindFirstLineW(inf, section, DelFiles, &context);
    while (ret)
    {
        for (i = 1;; i++)
        {
            if (!SetupGetStringFieldW(&context, i, section_name, sizeof(section_name) / sizeof(WCHAR), NULL))
                break;
            SetupAddSectionToDiskSpaceListW(diskspace, layoutinf, inf, section_name, FILEOP_DELETE, 0, 0);
        }
        ret = SetupFindNextLine(&context, &context);
    }

    return TRUE;
}

/***********************************************************************
 *      SetupAddInstallSectionToDiskSpaceListA  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddInstallSectionToDiskSpaceListA(HDSKSPC diskspace,
                        HINF inf, HINF layoutinf, LPCSTR section,
                        PVOID reserved1, UINT reserved2)
{
    LPWSTR sectionW = NULL;
    DWORD len;
    BOOL ret;

    if (section)
    {
        len = MultiByteToWideChar(CP_ACP, 0, section, -1, NULL, 0);

        sectionW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!sectionW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, len);
    }

    ret = SetupAddInstallSectionToDiskSpaceListW(diskspace, inf, layoutinf,
                                                 sectionW, reserved1, reserved2);
    if (sectionW) HeapFree(GetProcessHeap(), 0, sectionW);
    return ret;
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

#ifdef __REACTOS__ // BUGFIX
    size++; // Add terminating NUL
    fullpathW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
#else
    size = (size+1) * sizeof(WCHAR);
    fullpathW = HeapAlloc(GetProcessHeap(), 0, size);
#endif

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
#ifdef __REACTOS__ // BUGFIX
        file = malloc(sizeof(*file));
#else
        file = HeapAlloc(GetProcessHeap(), 0, sizeof(*file));
#endif
        if (!file)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }

        file->path = wcsdup(fullpathW);
        if (!file->path)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
#ifdef __REACTOS__ // BUGFIX
            free(file);
#else
            HeapFree(GetProcessHeap(), 0, file);
#endif
            goto done;
        }

        list_add_tail(&list->files, &file->entry);
    }
    else if (operation == FILEOP_DELETE)
    {
        /* delete operations for added files are ignored */
        ret = TRUE;
        goto done;
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

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListW(HDSKSPC diskspace, PWSTR buffer, DWORD size, PDWORD required_size)
{
    struct space_list *list = diskspace;
    struct file_entry *file;
    DWORD cur_size = 1;
    BOOL used[26];

    TRACE("(%p, %p, %ld, %p)\n", diskspace, buffer, size, required_size);

    if (!diskspace)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    memset(&used, 0, sizeof(used));
    LIST_FOR_EACH_ENTRY(file, &list->files, struct file_entry, entry)
    {
        int device;

        /* UNC paths are not yet supported by this function */
        if (towlower(file->path[0]) < 'a' || towlower(file->path[0]) > 'z' || file->path[1] != ':')
            continue;

        device = towlower(file->path[0]) - 'a';
        if (used[device]) continue;

        cur_size += 3;

        if (buffer)
        {
            if (cur_size > size)
            {
                if (required_size) *required_size = cur_size;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            *buffer++ = towlower(file->path[0]);
            *buffer++ = ':';
            *buffer++ = 0;
        }

        used[device] = TRUE;
    }

    if (buffer && size) *buffer = 0;
    if (required_size)  *required_size = cur_size;
    return TRUE;
}

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListA(HDSKSPC diskspace, PSTR buffer, DWORD size, PDWORD required_size)
{
    WCHAR *bufferW = NULL;
    BOOL ret;
    int i;

    if (buffer && size)
    {
        bufferW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        if (!bufferW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    ret = SetupQueryDrivesInDiskSpaceListW(diskspace, bufferW ? bufferW : (WCHAR *)buffer,
                                           size, required_size);

    if (bufferW)
    {
        for (i = 0; i < size; i++)
            buffer[i] = bufferW[i];
        HeapFree(GetProcessHeap(), 0, bufferW);
    }

    return ret;
}
