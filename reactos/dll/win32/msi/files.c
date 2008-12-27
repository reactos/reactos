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
 * Actions dealing with files These are
 *
 * InstallFiles
 * DuplicateFiles
 * MoveFiles (TODO)
 * PatchFiles (TODO)
 * RemoveDuplicateFiles(TODO)
 * RemoveFiles(TODO)
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

extern const WCHAR szInstallFiles[];
extern const WCHAR szDuplicateFiles[];
extern const WCHAR szMoveFiles[];
extern const WCHAR szPatchFiles[];
extern const WCHAR szRemoveDuplicateFiles[];
extern const WCHAR szRemoveFiles[];

static void msi_file_update_ui( MSIPACKAGE *package, MSIFILE *f, const WCHAR *action )
{
    MSIRECORD *uirow;
    LPWSTR uipath, p;

    /* the UI chunk */
    uirow = MSI_CreateRecord( 9 );
    MSI_RecordSetStringW( uirow, 1, f->FileName );
    uipath = strdupW( f->TargetPath );
    p = strrchrW(uipath,'\\');
    if (p)
        p[1]=0;
    MSI_RecordSetStringW( uirow, 9, uipath);
    MSI_RecordSetInteger( uirow, 6, f->FileSize );
    ui_actiondata( package, action, uirow);
    msiobj_release( &uirow->hdr );
    msi_free( uipath );
    ui_progress( package, 2, f->FileSize, 0, 0);
}

/* compares the version of a file read from the filesystem and
 * the version specified in the File table
 */
static int msi_compare_file_version(MSIFILE *file)
{
    WCHAR version[MAX_PATH];
    DWORD size;
    UINT r;

    size = MAX_PATH;
    version[0] = '\0';
    r = MsiGetFileVersionW(file->TargetPath, version, &size, NULL, NULL);
    if (r != ERROR_SUCCESS)
        return 0;

    return lstrcmpW(version, file->Version);
}

static UINT get_file_target(MSIPACKAGE *package, LPCWSTR file_key, 
                            MSIFILE** file)
{
    LIST_FOR_EACH_ENTRY( *file, &package->files, MSIFILE, entry )
    {
        if (lstrcmpW( file_key, (*file)->File )==0)
        {
            if ((*file)->state >= msifs_overwrite)
                return ERROR_SUCCESS;
            else
                return ERROR_FILE_NOT_FOUND;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static void schedule_install_files(MSIPACKAGE *package)
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY(file, &package->files, MSIFILE, entry)
    {
        if (!ACTION_VerifyComponentForAction(file->Component, INSTALLSTATE_LOCAL))
        {
            TRACE("File %s is not scheduled for install\n", debugstr_w(file->File));

            ui_progress(package,2,file->FileSize,0,0);
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

static UINT copy_install_file(MSIFILE *file, LPWSTR source)
{
    UINT gle;

    TRACE("Copying %s to %s\n", debugstr_w(source),
          debugstr_w(file->TargetPath));

    gle = copy_file(file, source);
    if (gle == ERROR_SUCCESS)
        return gle;

    if (gle == ERROR_ALREADY_EXISTS && file->state == msifs_overwrite)
    {
        TRACE("overwriting existing file\n");
        gle = ERROR_SUCCESS;
    }
    else if (gle == ERROR_ACCESS_DENIED)
    {
        SetFileAttributesW(file->TargetPath, FILE_ATTRIBUTE_NORMAL);

        gle = copy_file(file, source);
        TRACE("Overwriting existing file: %d\n", gle);
    }

    return gle;
}

static BOOL check_dest_hash_matches(MSIFILE *file)
{
    MSIFILEHASHINFO hash;
    UINT r;

    if (!file->hash.dwFileHashInfoSize)
        return FALSE;

    hash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);
    r = MsiGetFileHashW(file->TargetPath, 0, &hash);
    if (r != ERROR_SUCCESS)
        return FALSE;

    return !memcmp(&hash, &file->hash, sizeof(MSIFILEHASHINFO));
}

static BOOL installfiles_cb(MSIPACKAGE *package, LPCWSTR file, DWORD action,
                            LPWSTR *path, DWORD *attrs, PVOID user)
{
    static MSIFILE *f = NULL;

    if (action == MSICABEXTRACT_BEGINEXTRACT)
    {
        f = get_loaded_file(package, file);
        if (!f)
        {
            WARN("unknown file in cabinet (%s)\n", debugstr_w(file));
            return FALSE;
        }

        if (f->state != msifs_missing && f->state != msifs_overwrite)
        {
            TRACE("Skipping extraction of %s\n", debugstr_w(file));
            return FALSE;
        }

        msi_file_update_ui(package, f, szInstallFiles);

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
    UINT rc = ERROR_SUCCESS;
    MSIFILE *file;

    /* increment progress bar each time action data is sent */
    ui_progress(package,1,1,0,0);

    schedule_install_files(package);

    /*
     * Despite MSDN specifying that the CreateFolders action
     * should be called before InstallFiles, some installers don't
     * do that, and they seem to work correctly.  We need to create
     * directories here to make sure that the files can be copied.
     */
    msi_create_component_directories( package );

    mi = msi_alloc_zero( sizeof(MSIMEDIAINFO) );

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (file->state != msifs_missing && !mi->is_continuous && file->state != msifs_overwrite)
            continue;

        if (check_dest_hash_matches(file))
        {
            TRACE("File hashes match, not overwriting\n");
            continue;
        }

        if (MsiGetFileVersionW(file->TargetPath, NULL, NULL, NULL, NULL) == ERROR_SUCCESS &&
            msi_compare_file_version(file) >= 0)
        {
            TRACE("Destination file version greater, not overwriting\n");
            continue;
        }

        if (file->Sequence > mi->last_sequence || mi->is_continuous ||
            (file->IsCompressed && !mi->is_extracted))
        {
            MSICABDATA data;

            rc = ready_media(package, file, mi);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to ready media\n");
                break;
            }

            data.mi = mi;
            data.package = package;
            data.cb = installfiles_cb;
            data.user = NULL;

            if (file->IsCompressed &&
                !msi_cabextract(package, mi, &data))
            {
                ERR("Failed to extract cabinet: %s\n", debugstr_w(mi->cabinet));
                rc = ERROR_FUNCTION_FAILED;
                break;
            }
        }

        if (!file->IsCompressed)
        {
            LPWSTR source = resolve_file_source(package, file);

            TRACE("file paths %s to %s\n", debugstr_w(source),
                  debugstr_w(file->TargetPath));

            msi_file_update_ui(package, file, szInstallFiles);
            rc = copy_install_file(file, source);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to copy %s to %s (%d)\n", debugstr_w(source),
                    debugstr_w(file->TargetPath), rc);
                rc = ERROR_INSTALL_FAILURE;
                msi_free(source);
                break;
            }

            msi_free(source);
        }
        else if (file->state != msifs_installed)
        {
            ERR("compressed file wasn't extracted (%s)\n",
                debugstr_w(file->TargetPath));
            rc = ERROR_INSTALL_FAILURE;
            break;
        }
    }

    msi_free_media_info(mi);
    return rc;
}

static UINT ITERATE_DuplicateFiles(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    WCHAR dest_name[0x100];
    LPWSTR dest_path, dest;
    LPCWSTR file_key, component;
    DWORD sz;
    DWORD rc;
    MSICOMPONENT *comp;
    MSIFILE *file;

    component = MSI_RecordGetString(row,2);
    comp = get_loaded_component(package,component);

    if (!ACTION_VerifyComponentForAction( comp, INSTALLSTATE_LOCAL ))
    {
        TRACE("Skipping copy due to disabled component %s\n",
                        debugstr_w(component));

        /* the action taken was the same as the current install state */        
        comp->Action = comp->Installed;

        return ERROR_SUCCESS;
    }

    comp->Action = INSTALLSTATE_LOCAL;

    file_key = MSI_RecordGetString(row,3);
    if (!file_key)
    {
        ERR("Unable to get file key\n");
        return ERROR_FUNCTION_FAILED;
    }

    rc = get_file_target(package,file_key,&file);

    if (rc != ERROR_SUCCESS)
    {
        ERR("Original file unknown %s\n",debugstr_w(file_key));
        return ERROR_SUCCESS;
    }

    if (MSI_RecordIsNull(row,4))
        strcpyW(dest_name,strrchrW(file->TargetPath,'\\')+1);
    else
    {
        sz=0x100;
        MSI_RecordGetStringW(row,4,dest_name,&sz);
        reduce_to_longfilename(dest_name);
    }

    if (MSI_RecordIsNull(row,5))
    {
        LPWSTR p;
        dest_path = strdupW(file->TargetPath);
        p = strrchrW(dest_path,'\\');
        if (p)
            *p=0;
    }
    else
    {
        LPCWSTR destkey;
        destkey = MSI_RecordGetString(row,5);
        dest_path = resolve_folder(package, destkey, FALSE, FALSE, TRUE, NULL);
        if (!dest_path)
        {
            /* try a Property */
            dest_path = msi_dup_property( package, destkey );
            if (!dest_path)
            {
                FIXME("Unable to get destination folder, try AppSearch properties\n");
                return ERROR_SUCCESS;
            }
        }
    }

    dest = build_directory_name(2, dest_path, dest_name);
    create_full_pathW(dest_path);

    TRACE("Duplicating file %s to %s\n",debugstr_w(file->TargetPath),
                    debugstr_w(dest)); 

    if (strcmpW(file->TargetPath,dest))
        rc = !CopyFileW(file->TargetPath,dest,TRUE);
    else
        rc = ERROR_SUCCESS;

    if (rc != ERROR_SUCCESS)
        ERR("Failed to copy file %s -> %s, last error %d\n",
            debugstr_w(file->TargetPath), debugstr_w(dest_path), GetLastError());

    FIXME("We should track these duplicate files as well\n");   

    msi_free(dest_path);
    msi_free(dest);

    msi_file_update_ui(package, file, szDuplicateFiles);

    return ERROR_SUCCESS;
}

UINT ACTION_DuplicateFiles(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    static const WCHAR ExecSeqQuery[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','D','u','p','l','i','c','a','t','e','F','i','l','e','`',0};

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_IterateRecords(view, NULL, ITERATE_DuplicateFiles, package);
    msiobj_release(&view->hdr);

    return rc;
}

static BOOL verify_comp_for_removal(MSICOMPONENT *comp, UINT install_mode)
{
    INSTALLSTATE request = comp->ActionRequest;

    if (request == INSTALLSTATE_UNKNOWN)
        return FALSE;

    if (install_mode == msidbRemoveFileInstallModeOnInstall &&
        (request == INSTALLSTATE_LOCAL || request == INSTALLSTATE_SOURCE))
        return TRUE;

    if (request == INSTALLSTATE_ABSENT)
    {
        if (!comp->ComponentId)
            return FALSE;

        if (install_mode == msidbRemoveFileInstallModeOnRemove)
            return TRUE;
    }

    if (install_mode == msidbRemoveFileInstallModeOnBoth)
        return TRUE;

    return FALSE;
}

static UINT ITERATE_RemoveFiles(MSIRECORD *row, LPVOID param)
{
    MSIPACKAGE *package = (MSIPACKAGE*)param;
    MSICOMPONENT *comp;
    LPCWSTR component, filename, dirprop;
    UINT install_mode;
    LPWSTR dir = NULL, path = NULL;
    DWORD size;
    UINT r;

    component = MSI_RecordGetString(row, 2);
    filename = MSI_RecordGetString(row, 3);
    dirprop = MSI_RecordGetString(row, 4);
    install_mode = MSI_RecordGetInteger(row, 5);

    comp = get_loaded_component(package, component);
    if (!comp)
    {
        ERR("Invalid component: %s\n", debugstr_w(component));
        return ERROR_FUNCTION_FAILED;
    }

    if (!verify_comp_for_removal(comp, install_mode))
    {
        TRACE("Skipping removal due to missing conditions\n");
        comp->Action = comp->Installed;
        return ERROR_SUCCESS;
    }

    dir = msi_dup_property(package, dirprop);
    if (!dir)
        return ERROR_OUTOFMEMORY;

    size = (filename != NULL) ? lstrlenW(filename) : 0;
    size += lstrlenW(dir) + 2;
    path = msi_alloc(size * sizeof(WCHAR));
    if (!path)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    lstrcpyW(path, dir);
    PathAddBackslashW(path);

    if (filename)
    {
        lstrcatW(path, filename);

        TRACE("Deleting misc file: %s\n", debugstr_w(path));
        DeleteFileW(path);
    }
    else
    {
        TRACE("Removing misc directory: %s\n", debugstr_w(path));
        RemoveDirectoryW(path);
    }

done:
    msi_free(path);
    msi_free(dir);
    return ERROR_SUCCESS;
}

UINT ACTION_RemoveFiles( MSIPACKAGE *package )
{
    MSIQUERY *view;
    MSIFILE *file;
    UINT r;

    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','R','e','m','o','v','e','F','i','l','e','`',0};

    r = MSI_DatabaseOpenViewW(package->db, query, &view);
    if (r == ERROR_SUCCESS)
    {
        MSI_IterateRecords(view, NULL, ITERATE_RemoveFiles, package);
        msiobj_release(&view->hdr);
    }

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSIRECORD *uirow;
        LPWSTR uipath, p;

        if ( file->state == msifs_installed )
            ERR("removing installed file %s\n", debugstr_w(file->TargetPath));

        if ( file->Component->ActionRequest != INSTALLSTATE_ABSENT ||
             file->Component->Installed == INSTALLSTATE_SOURCE )
            continue;

        /* don't remove a file if the old file
         * is strictly newer than the version to be installed
         */
        if ( msi_compare_file_version( file ) < 0 )
            continue;

        TRACE("removing %s\n", debugstr_w(file->File) );
        if ( !DeleteFileW( file->TargetPath ) )
            TRACE("failed to delete %s\n",  debugstr_w(file->TargetPath));
        file->state = msifs_missing;

        /* the UI chunk */
        uirow = MSI_CreateRecord( 9 );
        MSI_RecordSetStringW( uirow, 1, file->FileName );
        uipath = strdupW( file->TargetPath );
        p = strrchrW(uipath,'\\');
        if (p)
            p[1]=0;
        MSI_RecordSetStringW( uirow, 9, uipath);
        ui_actiondata( package, szRemoveFiles, uirow);
        msiobj_release( &uirow->hdr );
        msi_free( uipath );
        /* FIXME: call ui_progress here? */
    }

    return ERROR_SUCCESS;
}
