/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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


/*
 * Actions dealing with files:
 *
 * InstallFiles
 * DuplicateFiles
 * MoveFiles
 * PatchFiles
 * RemoveDuplicateFiles
 * RemoveFiles
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msi.h"
#include "msidefs.h"
#include "msipriv.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static HMODULE hmspatcha;
static BOOL (WINAPI *ApplyPatchToFileW)(LPCWSTR, LPCWSTR, LPCWSTR, ULONG);

static void msi_file_update_ui( MSIPACKAGE *package, MSIFILE *f, const WCHAR *action )
{
    MSIRECORD *uirow;

    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, f->FileName );
    MSI_RecordSetStringW( uirow, 9, f->Component->Directory );
    MSI_RecordSetInteger( uirow, 6, f->FileSize );
    msi_ui_actiondata( package, action, uirow );
    msiobj_release( &uirow->hdr );
    msi_ui_progress( package, 2, f->FileSize, 0, 0 );
}

static msi_file_state calculate_install_state( MSIPACKAGE *package, MSIFILE *file )
{
    MSICOMPONENT *comp = file->Component;
    VS_FIXEDFILEINFO *file_version;
    WCHAR *font_version;
    msi_file_state state;
    DWORD file_size;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL || (comp->assembly && comp->assembly->installed))
    {
        TRACE("file %s is not scheduled for install\n", debugstr_w(file->File));
        return msifs_skipped;
    }
    if ((comp->assembly && !comp->assembly->application && !comp->assembly->installed) ||
        GetFileAttributesW( file->TargetPath ) == INVALID_FILE_ATTRIBUTES)
    {
        TRACE("file %s is missing\n", debugstr_w(file->File));
        return msifs_missing;
    }
    if (file->Version)
    {
        if ((file_version = msi_get_disk_file_version( file->TargetPath )))
        {
            TRACE("new %s old %u.%u.%u.%u\n", debugstr_w(file->Version),
                  HIWORD(file_version->dwFileVersionMS),
                  LOWORD(file_version->dwFileVersionMS),
                  HIWORD(file_version->dwFileVersionLS),
                  LOWORD(file_version->dwFileVersionLS));

            if (msi_compare_file_versions( file_version, file->Version ) < 0)
                state = msifs_overwrite;
            else
            {
                TRACE("destination file version equal or greater, not overwriting\n");
                state = msifs_present;
            }
            msi_free( file_version );
            return state;
        }
        else if ((font_version = msi_font_version_from_file( file->TargetPath )))
        {
            TRACE("new %s old %s\n", debugstr_w(file->Version), debugstr_w(font_version));

            if (msi_compare_font_versions( font_version, file->Version ) < 0)
                state = msifs_overwrite;
            else
            {
                TRACE("destination file version equal or greater, not overwriting\n");
                state = msifs_present;
            }
            msi_free( font_version );
            return state;
        }
    }
    if ((file_size = msi_get_disk_file_size( file->TargetPath )) != file->FileSize)
    {
        return msifs_overwrite;
    }
    if (file->hash.dwFileHashInfoSize)
    {
        if (msi_file_hash_matches( file ))
        {
            TRACE("file hashes match, not overwriting\n");
            return msifs_hashmatch;
        }
        else
        {
            TRACE("file hashes do not match, overwriting\n");
            return msifs_overwrite;
        }
    }
    /* assume present */
    return msifs_present;
}

static void schedule_install_files(MSIPACKAGE *package)
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY(file, &package->files, MSIFILE, entry)
    {
        MSICOMPONENT *comp = file->Component;

        file->state = calculate_install_state( package, file );
        if (file->state == msifs_overwrite && (comp->Attributes & msidbComponentAttributesNeverOverwrite))
        {
            TRACE("not overwriting %s\n", debugstr_w(file->TargetPath));
            file->state = msifs_skipped;
        }
    }
}

static UINT copy_file(MSIFILE *file, LPWSTR source)
{
    BOOL ret;

    ret = CopyFileW(source, file->TargetPath, FALSE);
    if (!ret)
        return GetLastError();

    SetFileAttributesW(file->TargetPath, FILE_ATTRIBUTE_NORMAL);

    file->state = msifs_installed;
    return ERROR_SUCCESS;
}

static UINT copy_install_file(MSIPACKAGE *package, MSIFILE *file, LPWSTR source)
{
    UINT gle;

    TRACE("Copying %s to %s\n", debugstr_w(source), debugstr_w(file->TargetPath));

    gle = copy_file(file, source);
    if (gle == ERROR_SUCCESS)
        return gle;

    if (gle == ERROR_ALREADY_EXISTS && file->state == msifs_overwrite)
    {
        TRACE("overwriting existing file\n");
        return ERROR_SUCCESS;
    }
    else if (gle == ERROR_ACCESS_DENIED)
    {
        SetFileAttributesW(file->TargetPath, FILE_ATTRIBUTE_NORMAL);

        gle = copy_file(file, source);
        TRACE("Overwriting existing file: %d\n", gle);
    }
    if (gle == ERROR_SHARING_VIOLATION || gle == ERROR_USER_MAPPED_FILE)
    {
        WCHAR *tmpfileW, *pathW, *p;
        DWORD len;

        TRACE("file in use, scheduling rename operation\n");

        if (!(pathW = strdupW( file->TargetPath ))) return ERROR_OUTOFMEMORY;
        if ((p = strrchrW(pathW, '\\'))) *p = 0;
        len = strlenW( pathW ) + 16;
        if (!(tmpfileW = msi_alloc(len * sizeof(WCHAR))))
        {
            msi_free( pathW );
            return ERROR_OUTOFMEMORY;
        }
        if (!GetTempFileNameW( pathW, szMsi, 0, tmpfileW )) tmpfileW[0] = 0;
        msi_free( pathW );

        if (CopyFileW(source, tmpfileW, FALSE) &&
            MoveFileExW(file->TargetPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT) &&
            MoveFileExW(tmpfileW, file->TargetPath, MOVEFILE_DELAY_UNTIL_REBOOT))
        {
            file->state = msifs_installed;
            package->need_reboot_at_end = 1;
            gle = ERROR_SUCCESS;
        }
        else
        {
            gle = GetLastError();
            WARN("failed to schedule rename operation: %d)\n", gle);
            DeleteFileW( tmpfileW );
        }
        msi_free(tmpfileW);
    }

    return gle;
}

static UINT msi_create_directory( MSIPACKAGE *package, const WCHAR *dir )
{
    MSIFOLDER *folder;
    const WCHAR *install_path;

    install_path = msi_get_target_folder( package, dir );
    if (!install_path) return ERROR_FUNCTION_FAILED;

    folder = msi_get_loaded_folder( package, dir );
    if (folder->State == FOLDER_STATE_UNINITIALIZED)
    {
        msi_create_full_path( install_path );
        folder->State = FOLDER_STATE_CREATED;
    }
    return ERROR_SUCCESS;
}

static MSIFILE *find_file( MSIPACKAGE *package, const WCHAR *filename )
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (file->state != msifs_installed && !strcmpiW( filename, file->File )) return file;
    }
    return NULL;
}

static BOOL installfiles_cb(MSIPACKAGE *package, LPCWSTR file, DWORD action,
                            LPWSTR *path, DWORD *attrs, PVOID user)
{
    static MSIFILE *f = NULL;
    UINT_PTR disk_id = (UINT_PTR)user;

    if (action == MSICABEXTRACT_BEGINEXTRACT)
    {
        if (!(f = find_file( package, file )))
        {
            TRACE("unknown file in cabinet (%s)\n", debugstr_w(file));
            return FALSE;
        }
        if (f->disk_id != disk_id || (f->state != msifs_missing && f->state != msifs_overwrite))
            return FALSE;

        if (!f->Component->assembly || f->Component->assembly->application)
        {
            msi_create_directory(package, f->Component->Directory);
        }
        *path = strdupW(f->TargetPath);
        *attrs = f->Attributes;
    }
    else if (action == MSICABEXTRACT_FILEEXTRACTED)
    {
        f->state = msifs_installed;
        f = NULL;
    }

    return TRUE;
}

WCHAR *msi_resolve_file_source( MSIPACKAGE *package, MSIFILE *file )
{
    WCHAR *p, *path;

    TRACE("Working to resolve source of file %s\n", debugstr_w(file->File));

    if (file->IsCompressed) return NULL;

    p = msi_resolve_source_folder( package, file->Component->Directory, NULL );
    path = msi_build_directory_name( 2, p, file->ShortName );

    if (file->LongName && GetFileAttributesW( path ) == INVALID_FILE_ATTRIBUTES)
    {
        msi_free( path );
        path = msi_build_directory_name( 2, p, file->LongName );
    }
    msi_free( p );
    TRACE("file %s source resolves to %s\n", debugstr_w(file->File), debugstr_w(path));
    return path;
}

/*
 * ACTION_InstallFiles()
 * 
 * For efficiency, this is done in two passes:
 * 1) Correct all the TargetPaths and determine what files are to be installed.
 * 2) Extract Cabinets and copy files.
 */
UINT ACTION_InstallFiles(MSIPACKAGE *package)
{
    MSIMEDIAINFO *mi;
    MSICOMPONENT *comp;
    UINT rc = ERROR_SUCCESS;
    MSIFILE *file;

    schedule_install_files(package);
    mi = msi_alloc_zero( sizeof(MSIMEDIAINFO) );

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        msi_file_update_ui( package, file, szInstallFiles );

        rc = msi_load_media_info( package, file->Sequence, mi );
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to load media info for %s (%u)\n", debugstr_w(file->File), rc);
            return ERROR_FUNCTION_FAILED;
        }
        if (!file->Component->Enabled) continue;

        if (file->state != msifs_hashmatch &&
            file->state != msifs_skipped &&
            (file->state != msifs_present || !msi_get_property_int( package->db, szInstalled, 0 )) &&
            (rc = ready_media( package, file->IsCompressed, mi )))
        {
            ERR("Failed to ready media for %s\n", debugstr_w(file->File));
            goto done;
        }

        if (file->state != msifs_missing && !mi->is_continuous && file->state != msifs_overwrite)
            continue;

        if (file->Sequence > mi->last_sequence || mi->is_continuous ||
            (file->IsCompressed && !mi->is_extracted))
        {
            MSICABDATA data;

            data.mi = mi;
            data.package = package;
            data.cb = installfiles_cb;
            data.user = (PVOID)(UINT_PTR)mi->disk_id;

            if (file->IsCompressed &&
                !msi_cabextract(package, mi, &data))
            {
                ERR("Failed to extract cabinet: %s\n", debugstr_w(mi->cabinet));
                rc = ERROR_INSTALL_FAILURE;
                goto done;
            }
        }

        if (!file->IsCompressed)
        {
            WCHAR *source = msi_resolve_file_source(package, file);

            TRACE("copying %s to %s\n", debugstr_w(source), debugstr_w(file->TargetPath));

            if (!file->Component->assembly || file->Component->assembly->application)
            {
                msi_create_directory(package, file->Component->Directory);
            }
            rc = copy_install_file(package, file, source);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to copy %s to %s (%d)\n", debugstr_w(source),
                    debugstr_w(file->TargetPath), rc);
                rc = ERROR_INSTALL_FAILURE;
                msi_free(source);
                goto done;
            }
            msi_free(source);
        }
        else if (file->state != msifs_installed && !(file->Attributes & msidbFileAttributesPatchAdded))
        {
            ERR("compressed file wasn't installed (%s)\n", debugstr_w(file->TargetPath));
            rc = ERROR_INSTALL_FAILURE;
            goto done;
        }
    }
    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        comp->Action = msi_get_component_action( package, comp );
        if (comp->Action == INSTALLSTATE_LOCAL && comp->assembly && !comp->assembly->installed)
        {
            rc = msi_install_assembly( package, comp );
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to install assembly\n");
                rc = ERROR_INSTALL_FAILURE;
                break;
            }
        }
    }

done:
    msi_free_media_info(mi);
    return rc;
}

static BOOL load_mspatcha(void)
{
    hmspatcha = LoadLibraryA("mspatcha.dll");
    if (!hmspatcha)
    {
        ERR("Failed to load mspatcha.dll: %d\n", GetLastError());
        return FALSE;
    }

    ApplyPatchToFileW = (void*)GetProcAddress(hmspatcha, "ApplyPatchToFileW");
    if(!ApplyPatchToFileW)
    {
        ERR("GetProcAddress(ApplyPatchToFileW) failed: %d.\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

static void unload_mspatch(void)
{
    FreeLibrary(hmspatcha);
}

static BOOL patchfiles_cb(MSIPACKAGE *package, LPCWSTR file, DWORD action,
                          LPWSTR *path, DWORD *attrs, PVOID user)
{
    static MSIFILEPATCH *p = NULL;
    static WCHAR patch_path[MAX_PATH] = {0};
    static WCHAR temp_folder[MAX_PATH] = {0};

    if (action == MSICABEXTRACT_BEGINEXTRACT)
    {
        if (temp_folder[0] == '\0')
            GetTempPathW(MAX_PATH, temp_folder);

        p = msi_get_loaded_filepatch(package, file);
        if (!p)
        {
            TRACE("unknown file in cabinet (%s)\n", debugstr_w(file));
            return FALSE;
        }
        GetTempFileNameW(temp_folder, NULL, 0, patch_path);

        *path = strdupW(patch_path);
        *attrs = p->File->Attributes;
    }
    else if (action == MSICABEXTRACT_FILEEXTRACTED)
    {
        WCHAR patched_file[MAX_PATH];
        BOOL br;

        GetTempFileNameW(temp_folder, NULL, 0, patched_file);

        br = ApplyPatchToFileW(patch_path, p->File->TargetPath, patched_file, 0);
        if (br)
        {
            /* FIXME: baseline cache */

            DeleteFileW( p->File->TargetPath );
            MoveFileW( patched_file, p->File->TargetPath );

            p->IsApplied = TRUE;
        }
        else
            ERR("Failed patch %s: %d.\n", debugstr_w(p->File->TargetPath), GetLastError());

        DeleteFileW(patch_path);
        p = NULL;
    }

    return TRUE;
}

UINT ACTION_PatchFiles( MSIPACKAGE *package )
{
    MSIFILEPATCH *patch;
    MSIMEDIAINFO *mi;
    UINT rc = ERROR_SUCCESS;
    BOOL mspatcha_loaded = FALSE;

    TRACE("%p\n", package);

    mi = msi_alloc_zero( sizeof(MSIMEDIAINFO) );

    LIST_FOR_EACH_ENTRY( patch, &package->filepatches, MSIFILEPATCH, entry )
    {
        MSIFILE *file = patch->File;
        MSICOMPONENT *comp = file->Component;

        rc = msi_load_media_info( package, patch->Sequence, mi );
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to load media info for %s (%u)\n", debugstr_w(file->File), rc);
            return ERROR_FUNCTION_FAILED;
        }
        comp->Action = msi_get_component_action( package, comp );
        if (!comp->Enabled || comp->Action != INSTALLSTATE_LOCAL) continue;

        if (!patch->IsApplied)
        {
            MSICABDATA data;

            rc = ready_media( package, TRUE, mi );
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to ready media for %s\n", debugstr_w(file->File));
                goto done;
            }

            if (!mspatcha_loaded && !load_mspatcha())
            {
                rc = ERROR_FUNCTION_FAILED;
                goto done;
            }
            mspatcha_loaded = TRUE;

            data.mi = mi;
            data.package = package;
            data.cb = patchfiles_cb;
            data.user = (PVOID)(UINT_PTR)mi->disk_id;

            if (!msi_cabextract(package, mi, &data))
            {
                ERR("Failed to extract cabinet: %s\n", debugstr_w(mi->cabinet));
                rc = ERROR_INSTALL_FAILURE;
                goto done;
            }
        }

        if (!patch->IsApplied && !(patch->Attributes & msidbPatchAttributesNonVital))
        {
            ERR("Failed to apply patch to file: %s\n", debugstr_w(file->File));
            rc = ERROR_INSTALL_FAILURE;
            goto done;
        }
    }

done:
    msi_free_media_info(mi);
    if (mspatcha_loaded)
        unload_mspatch();
    return rc;
}

#define is_dot_dir(x) ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

typedef struct
{
    struct list entry;
    LPWSTR sourcename;
    LPWSTR destname;
    LPWSTR source;
    LPWSTR dest;
} FILE_LIST;

static BOOL msi_move_file(LPCWSTR source, LPCWSTR dest, int options)
{
    BOOL ret;

    if (GetFileAttributesW(source) == FILE_ATTRIBUTE_DIRECTORY ||
        GetFileAttributesW(dest) == FILE_ATTRIBUTE_DIRECTORY)
    {
        WARN("Source or dest is directory, not moving\n");
        return FALSE;
    }

    if (options == msidbMoveFileOptionsMove)
    {
        TRACE("moving %s -> %s\n", debugstr_w(source), debugstr_w(dest));
        ret = MoveFileExW(source, dest, MOVEFILE_REPLACE_EXISTING);
        if (!ret)
        {
            WARN("MoveFile failed: %d\n", GetLastError());
            return FALSE;
        }
    }
    else
    {
        TRACE("copying %s -> %s\n", debugstr_w(source), debugstr_w(dest));
        ret = CopyFileW(source, dest, FALSE);
        if (!ret)
        {
            WARN("CopyFile failed: %d\n", GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

static LPWSTR wildcard_to_file(LPWSTR wildcard, LPWSTR filename)
{
    LPWSTR path, ptr;
    DWORD dirlen, pathlen;

    ptr = strrchrW(wildcard, '\\');
    dirlen = ptr - wildcard + 1;

    pathlen = dirlen + lstrlenW(filename) + 1;
    path = msi_alloc(pathlen * sizeof(WCHAR));

    lstrcpynW(path, wildcard, dirlen + 1);
    lstrcatW(path, filename);

    return path;
}

static void free_file_entry(FILE_LIST *file)
{
    msi_free(file->source);
    msi_free(file->dest);
    msi_free(file);
}

static void free_list(FILE_LIST *list)
{
    while (!list_empty(&list->entry))
    {
        FILE_LIST *file = LIST_ENTRY(list_head(&list->entry), FILE_LIST, entry);

        list_remove(&file->entry);
        free_file_entry(file);
    }
}

static BOOL add_wildcard(FILE_LIST *files, LPWSTR source, LPWSTR dest)
{
    FILE_LIST *new, *file;
    LPWSTR ptr, filename;
    DWORD size;

    new = msi_alloc_zero(sizeof(FILE_LIST));
    if (!new)
        return FALSE;

    new->source = strdupW(source);
    ptr = strrchrW(dest, '\\') + 1;
    filename = strrchrW(new->source, '\\') + 1;

    new->sourcename = filename;

    if (*ptr)
        new->destname = ptr;
    else
        new->destname = new->sourcename;

    size = (ptr - dest) + lstrlenW(filename) + 1;
    new->dest = msi_alloc(size * sizeof(WCHAR));
    if (!new->dest)
    {
        free_file_entry(new);
        return FALSE;
    }

    lstrcpynW(new->dest, dest, ptr - dest + 1);
    lstrcatW(new->dest, filename);

    if (list_empty(&files->entry))
    {
        list_add_head(&files->entry, &new->entry);
        return TRUE;
    }

    LIST_FOR_EACH_ENTRY(file, &files->entry, FILE_LIST, entry)
    {
        if (strcmpW( source, file->source ) < 0)
        {
            list_add_before(&file->entry, &new->entry);
            return TRUE;
        }
    }

    list_add_after(&file->entry, &new->entry);
    return TRUE;
}

static BOOL move_files_wildcard(LPWSTR source, LPWSTR dest, int options)
{
    WIN32_FIND_DATAW wfd;
    HANDLE hfile;
    LPWSTR path;
    BOOL res;
    FILE_LIST files, *file;
    DWORD size;

    hfile = FindFirstFileW(source, &wfd);
    if (hfile == INVALID_HANDLE_VALUE) return FALSE;

    list_init(&files.entry);

    for (res = TRUE; res; res = FindNextFileW(hfile, &wfd))
    {
        if (is_dot_dir(wfd.cFileName)) continue;

        path = wildcard_to_file(source, wfd.cFileName);
        if (!path)
        {
            res = FALSE;
            goto done;
        }

        add_wildcard(&files, path, dest);
        msi_free(path);
    }

    /* no files match the wildcard */
    if (list_empty(&files.entry))
        goto done;

    /* only the first wildcard match gets renamed to dest */
    file = LIST_ENTRY(list_head(&files.entry), FILE_LIST, entry);
    size = (strrchrW(file->dest, '\\') - file->dest) + lstrlenW(file->destname) + 2;
    file->dest = msi_realloc(file->dest, size * sizeof(WCHAR));
    if (!file->dest)
    {
        res = FALSE;
        goto done;
    }

    /* file->dest may be shorter after the reallocation, so add a NULL
     * terminator.  This is needed for the call to strrchrW, as there will no
     * longer be a NULL terminator within the bounds of the allocation in this case.
     */
    file->dest[size - 1] = '\0';
    lstrcpyW(strrchrW(file->dest, '\\') + 1, file->destname);

    while (!list_empty(&files.entry))
    {
        file = LIST_ENTRY(list_head(&files.entry), FILE_LIST, entry);

        msi_move_file(file->source, file->dest, options);

        list_remove(&file->entry);
        free_file_entry(file);
    }

    res = TRUE;

done:
    free_list(&files);
    FindClose(hfile);
    return res;
}

void msi_reduce_to_long_filename( WCHAR *filename )
{
    WCHAR *p = strchrW( filename, '|' );
    if (p) memmove( filename, p + 1, (strlenW( p + 1 ) + 1) * sizeof(WCHAR) );
}

static UINT ITERATE_MoveFiles( MSIRECORD *rec, LPVOID param )
{
    MSIPACKAGE *package = param;
    MSIRECORD *uirow;
    MSICOMPONENT *comp;
    LPCWSTR sourcename, component;
    LPWSTR sourcedir, destname = NULL, destdir = NULL, source = NULL, dest = NULL;
    int options;
    DWORD size;
    BOOL ret, wildcards;

    component = MSI_RecordGetString(rec, 2);
    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    sourcename = MSI_RecordGetString(rec, 3);
    options = MSI_RecordGetInteger(rec, 7);

    sourcedir = msi_dup_property(package->db, MSI_RecordGetString(rec, 5));
    if (!sourcedir)
        goto done;

    destdir = msi_dup_property(package->db, MSI_RecordGetString(rec, 6));
    if (!destdir)
        goto done;

    if (!sourcename)
    {
        if (GetFileAttributesW(sourcedir) == INVALID_FILE_ATTRIBUTES)
            goto done;

        source = strdupW(sourcedir);
        if (!source)
            goto done;
    }
    else
    {
        size = lstrlenW(sourcedir) + lstrlenW(sourcename) + 2;
        source = msi_alloc(size * sizeof(WCHAR));
        if (!source)
            goto done;

        lstrcpyW(source, sourcedir);
        if (source[lstrlenW(source) - 1] != '\\')
            lstrcatW(source, szBackSlash);
        lstrcatW(source, sourcename);
    }

    wildcards = strchrW(source, '*') || strchrW(source, '?');

    if (MSI_RecordIsNull(rec, 4))
    {
        if (!wildcards)
        {
            destname = strdupW(sourcename);
            if (!destname)
                goto done;
        }
    }
    else
    {
        destname = strdupW(MSI_RecordGetString(rec, 4));
        if (destname) msi_reduce_to_long_filename(destname);
    }

    size = 0;
    if (destname)
        size = lstrlenW(destname);

    size += lstrlenW(destdir) + 2;
    dest = msi_alloc(size * sizeof(WCHAR));
    if (!dest)
        goto done;

    lstrcpyW(dest, destdir);
    if (dest[lstrlenW(dest) - 1] != '\\')
        lstrcatW(dest, szBackSlash);

    if (destname)
        lstrcatW(dest, destname);

    if (GetFileAttributesW(destdir) == INVALID_FILE_ATTRIBUTES)
    {
        if (!(ret = msi_create_full_path(destdir)))
        {
            WARN("failed to create directory %u\n", GetLastError());
            goto done;
        }
    }

    if (!wildcards)
        msi_move_file(source, dest, options);
    else
        move_files_wildcard(source, dest, options);

done:
    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, MSI_RecordGetString(rec, 1) );
    MSI_RecordSetInteger( uirow, 6, 1 ); /* FIXME */
    MSI_RecordSetStringW( uirow, 9, destdir );
    msi_ui_actiondata( package, szMoveFiles, uirow );
    msiobj_release( &uirow->hdr );

    msi_free(sourcedir);
    msi_free(destdir);
    msi_free(destname);
    msi_free(source);
    msi_free(dest);

    return ERROR_SUCCESS;
}

UINT ACTION_MoveFiles( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','M','o','v','e','F','i','l','e','`',0};
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW(package->db, query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_MoveFiles, package);
    msiobj_release(&view->hdr);
    return rc;
}

static WCHAR *get_duplicate_filename( MSIPACKAGE *package, MSIRECORD *row, const WCHAR *file_key, const WCHAR *src )
{
    DWORD len;
    WCHAR *dst_name, *dst_path, *dst;

    if (MSI_RecordIsNull( row, 4 ))
    {
        len = strlenW( src ) + 1;
        if (!(dst_name = msi_alloc( len * sizeof(WCHAR)))) return NULL;
        strcpyW( dst_name, strrchrW( src, '\\' ) + 1 );
    }
    else
    {
        MSI_RecordGetStringW( row, 4, NULL, &len );
        if (!(dst_name = msi_alloc( ++len * sizeof(WCHAR) ))) return NULL;
        MSI_RecordGetStringW( row, 4, dst_name, &len );
        msi_reduce_to_long_filename( dst_name );
    }

    if (MSI_RecordIsNull( row, 5 ))
    {
        WCHAR *p;
        dst_path = strdupW( src );
        p = strrchrW( dst_path, '\\' );
        if (p) *p = 0;
    }
    else
    {
        const WCHAR *dst_key = MSI_RecordGetString( row, 5 );

        dst_path = strdupW( msi_get_target_folder( package, dst_key ) );
        if (!dst_path)
        {
            /* try a property */
            dst_path = msi_dup_property( package->db, dst_key );
            if (!dst_path)
            {
                FIXME("Unable to get destination folder, try AppSearch properties\n");
                msi_free( dst_name );
                return NULL;
            }
        }
    }

    dst = msi_build_directory_name( 2, dst_path, dst_name );
    msi_create_full_path( dst_path );

    msi_free( dst_name );
    msi_free( dst_path );
    return dst;
}

static UINT ITERATE_DuplicateFiles(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    LPWSTR dest;
    LPCWSTR file_key, component;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    MSIFILE *file;

    component = MSI_RecordGetString(row,2);
    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_LOCAL)
    {
        TRACE("component not scheduled for installation %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    file_key = MSI_RecordGetString(row,3);
    if (!file_key)
    {
        ERR("Unable to get file key\n");
        return ERROR_FUNCTION_FAILED;
    }

    file = msi_get_loaded_file( package, file_key );
    if (!file)
    {
        ERR("Original file unknown %s\n", debugstr_w(file_key));
        return ERROR_SUCCESS;
    }

    dest = get_duplicate_filename( package, row, file_key, file->TargetPath );
    if (!dest)
    {
        WARN("Unable to get duplicate filename\n");
        return ERROR_SUCCESS;
    }

    TRACE("Duplicating file %s to %s\n", debugstr_w(file->TargetPath), debugstr_w(dest));

    if (!CopyFileW( file->TargetPath, dest, TRUE ))
    {
        WARN("Failed to copy file %s -> %s (%u)\n",
             debugstr_w(file->TargetPath), debugstr_w(dest), GetLastError());
    }

    FIXME("We should track these duplicate files as well\n");   

    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, MSI_RecordGetString( row, 1 ) );
    MSI_RecordSetInteger( uirow, 6, file->FileSize );
    MSI_RecordSetStringW( uirow, 9, MSI_RecordGetString( row, 5 ) );
    msi_ui_actiondata( package, szDuplicateFiles, uirow );
    msiobj_release( &uirow->hdr );

    msi_free(dest);
    return ERROR_SUCCESS;
}

UINT ACTION_DuplicateFiles(MSIPACKAGE *package)
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','D','u','p','l','i','c','a','t','e','F','i','l','e','`',0};
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW(package->db, query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_DuplicateFiles, package);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT ITERATE_RemoveDuplicateFiles( MSIRECORD *row, LPVOID param )
{
    MSIPACKAGE *package = param;
    LPWSTR dest;
    LPCWSTR file_key, component;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    MSIFILE *file;

    component = MSI_RecordGetString( row, 2 );
    comp = msi_get_loaded_component( package, component );
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (comp->Action != INSTALLSTATE_ABSENT)
    {
        TRACE("component not scheduled for removal %s\n", debugstr_w(component));
        return ERROR_SUCCESS;
    }

    file_key = MSI_RecordGetString( row, 3 );
    if (!file_key)
    {
        ERR("Unable to get file key\n");
        return ERROR_FUNCTION_FAILED;
    }

    file = msi_get_loaded_file( package, file_key );
    if (!file)
    {
        ERR("Original file unknown %s\n", debugstr_w(file_key));
        return ERROR_SUCCESS;
    }

    dest = get_duplicate_filename( package, row, file_key, file->TargetPath );
    if (!dest)
    {
        WARN("Unable to get duplicate filename\n");
        return ERROR_SUCCESS;
    }

    TRACE("Removing duplicate %s of %s\n", debugstr_w(dest), debugstr_w(file->TargetPath));

    if (!DeleteFileW( dest ))
    {
        WARN("Failed to delete duplicate file %s (%u)\n", debugstr_w(dest), GetLastError());
    }

    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, MSI_RecordGetString( row, 1 ) );
    MSI_RecordSetStringW( uirow, 9, MSI_RecordGetString( row, 5 ) );
    msi_ui_actiondata( package, szRemoveDuplicateFiles, uirow );
    msiobj_release( &uirow->hdr );

    msi_free(dest);
    return ERROR_SUCCESS;
}

UINT ACTION_RemoveDuplicateFiles( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','D','u','p','l','i','c','a','t','e','F','i','l','e','`',0};
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( package->db, query, &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords( view, NULL, ITERATE_RemoveDuplicateFiles, package );
    msiobj_release( &view->hdr );
    return rc;
}

static BOOL verify_comp_for_removal(MSICOMPONENT *comp, UINT install_mode)
{
    /* special case */
    if (comp->Action != INSTALLSTATE_SOURCE &&
        comp->Attributes & msidbComponentAttributesSourceOnly &&
        (install_mode == msidbRemoveFileInstallModeOnRemove ||
         install_mode == msidbRemoveFileInstallModeOnBoth)) return TRUE;

    switch (comp->Action)
    {
    case INSTALLSTATE_LOCAL:
    case INSTALLSTATE_SOURCE:
        if (install_mode == msidbRemoveFileInstallModeOnInstall ||
            install_mode == msidbRemoveFileInstallModeOnBoth) return TRUE;
        break;
    case INSTALLSTATE_ABSENT:
        if (install_mode == msidbRemoveFileInstallModeOnRemove ||
            install_mode == msidbRemoveFileInstallModeOnBoth) return TRUE;
        break;
    default: break;
    }
    return FALSE;
}

static UINT ITERATE_RemoveFiles(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = param;
    MSICOMPONENT *comp;
    MSIRECORD *uirow;
    LPCWSTR component, dirprop;
    UINT install_mode;
    LPWSTR dir = NULL, path = NULL, filename = NULL;
    DWORD size;
    UINT ret = ERROR_SUCCESS;

    component = MSI_RecordGetString(row, 2);
    dirprop = MSI_RecordGetString(row, 4);
    install_mode = MSI_RecordGetInteger(row, 5);

    comp = msi_get_loaded_component(package, component);
    if (!comp)
        return ERROR_SUCCESS;

    comp->Action = msi_get_component_action( package, comp );
    if (!verify_comp_for_removal(comp, install_mode))
    {
        TRACE("Skipping removal due to install mode\n");
        return ERROR_SUCCESS;
    }
    if (comp->assembly && !comp->assembly->application)
    {
        return ERROR_SUCCESS;
    }
    if (comp->Attributes & msidbComponentAttributesPermanent)
    {
        TRACE("permanent component, not removing file\n");
        return ERROR_SUCCESS;
    }

    dir = msi_dup_property(package->db, dirprop);
    if (!dir)
    {
        WARN("directory property has no value\n");
        return ERROR_SUCCESS;
    }
    size = 0;
    if ((filename = strdupW( MSI_RecordGetString(row, 3) )))
    {
        msi_reduce_to_long_filename( filename );
        size = lstrlenW( filename );
    }
    size += lstrlenW(dir) + 2;
    path = msi_alloc(size * sizeof(WCHAR));
    if (!path)
    {
        ret = ERROR_OUTOFMEMORY;
        goto done;
    }

    if (filename)
    {
        lstrcpyW(path, dir);
        PathAddBackslashW(path);
        lstrcatW(path, filename);

        TRACE("Deleting misc file: %s\n", debugstr_w(path));
        DeleteFileW(path);
    }
    else
    {
        TRACE("Removing misc directory: %s\n", debugstr_w(dir));
        RemoveDirectoryW(dir);
    }

done:
    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, MSI_RecordGetString(row, 1) );
    MSI_RecordSetStringW( uirow, 9, dir );
    msi_ui_actiondata( package, szRemoveFiles, uirow );
    msiobj_release( &uirow->hdr );

    msi_free(filename);
    msi_free(path);
    msi_free(dir);
    return ret;
}

static void remove_folder( MSIFOLDER *folder )
{
    FolderList *fl;

    LIST_FOR_EACH_ENTRY( fl, &folder->children, FolderList, entry )
    {
        remove_folder( fl->folder );
    }
    if (!folder->persistent && folder->State != FOLDER_STATE_REMOVED)
    {
        if (RemoveDirectoryW( folder->ResolvedTarget )) folder->State = FOLDER_STATE_REMOVED;
    }
}

UINT ACTION_RemoveFiles( MSIPACKAGE *package )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','R','e','m','o','v','e','F','i','l','e','`',0};
    MSIQUERY *view;
    MSICOMPONENT *comp;
    MSIFILE *file;
    UINT r;

    r = MSI_DatabaseOpenViewW(package->db, query, &view);
    if (r == ERROR_SUCCESS)
    {
        r = MSI_IterateRecords(view, NULL, ITERATE_RemoveFiles, package);
        msiobj_release(&view->hdr);
        if (r != ERROR_SUCCESS)
            return r;
    }

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSIRECORD *uirow;
        VS_FIXEDFILEINFO *ver;

        comp = file->Component;
        msi_file_update_ui( package, file, szRemoveFiles );

        comp->Action = msi_get_component_action( package, comp );
        if (comp->Action != INSTALLSTATE_ABSENT || comp->Installed == INSTALLSTATE_SOURCE)
            continue;

        if (comp->assembly && !comp->assembly->application)
            continue;

        if (comp->Attributes & msidbComponentAttributesPermanent)
        {
            TRACE("permanent component, not removing file\n");
            continue;
        }

        if (file->Version)
        {
            ver = msi_get_disk_file_version( file->TargetPath );
            if (ver && msi_compare_file_versions( ver, file->Version ) > 0)
            {
                TRACE("newer version detected, not removing file\n");
                msi_free( ver );
                continue;
            }
            msi_free( ver );
        }

        if (file->state == msifs_installed)
            WARN("removing installed file %s\n", debugstr_w(file->TargetPath));

        TRACE("removing %s\n", debugstr_w(file->File) );

        SetFileAttributesW( file->TargetPath, FILE_ATTRIBUTE_NORMAL );
        if (!DeleteFileW( file->TargetPath ))
        {
            WARN("failed to delete %s (%u)\n",  debugstr_w(file->TargetPath), GetLastError());
        }
        file->state = msifs_missing;

        uirow = MSI_CreateRecord( 9 );
        MSI_RecordSetStringW( uirow, 1, file->FileName );
        MSI_RecordSetStringW( uirow, 9, comp->Directory );
        msi_ui_actiondata( package, szRemoveFiles, uirow );
        msiobj_release( &uirow->hdr );
    }

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        comp->Action = msi_get_component_action( package, comp );
        if (comp->Action != INSTALLSTATE_ABSENT) continue;

        if (comp->Attributes & msidbComponentAttributesPermanent)
        {
            TRACE("permanent component, not removing directory\n");
            continue;
        }
        if (comp->assembly && !comp->assembly->application)
            msi_uninstall_assembly( package, comp );
        else
        {
            MSIFOLDER *folder = msi_get_loaded_folder( package, comp->Directory );
            remove_folder( folder );
        }
    }
    return ERROR_SUCCESS;
}
