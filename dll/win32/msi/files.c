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
#include "msvcrt/fcntl.h"
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

static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};

static BOOL source_matches_volume(MSIMEDIAINFO *mi, LPWSTR source_root)
{
    WCHAR volume_name[MAX_PATH + 1];

    if (!GetVolumeInformationW(source_root, volume_name, MAX_PATH + 1,
                               NULL, NULL, NULL, NULL, 0))
    {
        ERR("Failed to get volume information\n");
        return FALSE;
    }

    return !lstrcmpW(mi->volume_label, volume_name);
}

static UINT msi_change_media( MSIPACKAGE *package, MSIMEDIAINFO *mi )
{
    LPSTR msg;
    LPWSTR error, error_dialog;
    LPWSTR source_dir;
    UINT r = ERROR_SUCCESS;

    static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};
    static const WCHAR error_prop[] = {'E','r','r','o','r','D','i','a','l','o','g',0};

    if ( (msi_get_property_int(package, szUILevel, 0) & INSTALLUILEVEL_MASK) == INSTALLUILEVEL_NONE && !gUIHandlerA )
        return ERROR_SUCCESS;

    error = generate_error_string( package, 1302, 1, mi->disk_prompt );
    error_dialog = msi_dup_property( package, error_prop );
    source_dir = msi_dup_property( package, cszSourceDir );
    PathStripToRootW(source_dir);

    while ( r == ERROR_SUCCESS &&
            !source_matches_volume(mi, source_dir) )
    {
        r = msi_spawn_error_dialog( package, error_dialog, error );

        if (gUIHandlerA)
        {
            msg = strdupWtoA( error );
            gUIHandlerA( gUIContext, MB_RETRYCANCEL | INSTALLMESSAGE_ERROR, msg );
            msi_free(msg);
        }
    }

    msi_free( error );
    msi_free( error_dialog );
    msi_free( source_dir );

    return r;
}

/*
 * This is a helper function for handling embedded cabinet media
 */
static UINT writeout_cabinet_stream(MSIPACKAGE *package, LPCWSTR stream_name,
                                    WCHAR* source)
{
    UINT rc;
    USHORT* data;
    UINT    size;
    DWORD   write;
    HANDLE  the_file;
    WCHAR tmp[MAX_PATH];

    rc = read_raw_stream_data(package->db,stream_name,&data,&size); 
    if (rc != ERROR_SUCCESS)
        return rc;

    write = MAX_PATH;
    if (MSI_GetPropertyW(package, cszTempFolder, tmp, &write))
        GetTempPathW(MAX_PATH,tmp);

    GetTempFileNameW(tmp,stream_name,0,source);

    track_tempfile(package, source);
    the_file = CreateFileW(source, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n",debugstr_w(source));
        rc = ERROR_FUNCTION_FAILED;
        goto end;
    }

    WriteFile(the_file,data,size,&write,NULL);
    CloseHandle(the_file);
    TRACE("wrote %i bytes to %s\n",write,debugstr_w(source));
end:
    msi_free(data);
    return rc;
}


/* Support functions for FDI functions */
typedef struct
{
    MSIPACKAGE* package;
    MSIMEDIAINFO *mi;
} CabData;

static void * cabinet_alloc(ULONG cb)
{
    return msi_alloc(cb);
}

static void cabinet_free(void *pv)
{
    msi_free(pv);
}

static INT_PTR cabinet_open(char *pszFile, int oflag, int pmode)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    switch (oflag & _O_ACCMODE)
    {
    case _O_RDONLY:
        dwAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
        break;
    case _O_WRONLY:
        dwAccess = GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case _O_RDWR:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    }
    if ((oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        dwCreateDisposition = CREATE_NEW;
    else if (oflag & _O_CREAT)
        dwCreateDisposition = CREATE_ALWAYS;
    handle = CreateFileA( pszFile, dwAccess, dwShareMode, NULL, 
                          dwCreateDisposition, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    return (INT_PTR) handle;
}

static UINT cabinet_read(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwRead;
    if (ReadFile(handle, pv, cb, &dwRead, NULL))
        return dwRead;
    return 0;
}

static UINT cabinet_write(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwWritten;
    if (WriteFile(handle, pv, cb, &dwWritten, NULL))
        return dwWritten;
    return 0;
}

static int cabinet_close(INT_PTR hf)
{
    HANDLE handle = (HANDLE) hf;
    return CloseHandle(handle) ? 0 : -1;
}

static long cabinet_seek(INT_PTR hf, long dist, int seektype)
{
    HANDLE handle = (HANDLE) hf;
    /* flags are compatible and so are passed straight through */
    return SetFilePointer(handle, dist, NULL, seektype);
}

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

static UINT msi_media_get_disk_info( MSIPACKAGE *package, MSIMEDIAINFO *mi )
{
    MSIRECORD *row;
    LPWSTR ptr;

    static const WCHAR query[] =
        {'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
         '`','M','e','d','i','a','`',' ','W','H','E','R','E',' ',
         '`','D','i','s','k','I','d','`',' ','=',' ','%','i',0};

    row = MSI_QueryGetRecord(package->db, query, mi->disk_id);
    if (!row)
    {
        TRACE("Unable to query row\n");
        return ERROR_FUNCTION_FAILED;
    }

    mi->disk_prompt = strdupW(MSI_RecordGetString(row, 3));
    mi->cabinet = strdupW(MSI_RecordGetString(row, 4));
    mi->volume_label = strdupW(MSI_RecordGetString(row, 5));

    if (!mi->first_volume)
        mi->first_volume = strdupW(mi->volume_label);

    ptr = strrchrW(mi->source, '\\') + 1;
    lstrcpyW(ptr, mi->cabinet);
    msiobj_release(&row->hdr);

    return ERROR_SUCCESS;
}

static INT_PTR cabinet_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    TRACE("(%d)\n", fdint);

    switch (fdint)
    {
    case fdintPARTIAL_FILE:
    {
        CabData *data = (CabData *)pfdin->pv;
        data->mi->is_continuous = FALSE;
        return 0;
    }
    case fdintNEXT_CABINET:
    {
        CabData *data = (CabData *)pfdin->pv;
        MSIMEDIAINFO *mi = data->mi;
        LPWSTR cab = strdupAtoW(pfdin->psz1);
        UINT rc;

        msi_free(mi->disk_prompt);
        msi_free(mi->cabinet);
        msi_free(mi->volume_label);
        mi->disk_prompt = NULL;
        mi->cabinet = NULL;
        mi->volume_label = NULL;

        mi->disk_id++;
        mi->is_continuous = TRUE;

        rc = msi_media_get_disk_info(data->package, mi);
        if (rc != ERROR_SUCCESS)
        {
            msi_free(cab);
            ERR("Failed to get next cabinet information: %d\n", rc);
            return -1;
        }

        if (lstrcmpiW(mi->cabinet, cab))
        {
            msi_free(cab);
            ERR("Continuous cabinet does not match the next cabinet in the Media table\n");
            return -1;
        }

        msi_free(cab);

        TRACE("Searching for %s\n", debugstr_w(mi->source));

        if (GetFileAttributesW(mi->source) == INVALID_FILE_ATTRIBUTES)
            rc = msi_change_media(data->package, mi);

        if (rc != ERROR_SUCCESS)
            return -1;

        return 0;
    }
    case fdintCOPY_FILE:
    {
        CabData *data = (CabData*) pfdin->pv;
        HANDLE handle;
        LPWSTR file;
        MSIFILE *f;
        DWORD attrs;

        file = strdupAtoW(pfdin->psz1);
        f = get_loaded_file(data->package, file);
        msi_free(file);

        if (!f)
        {
            WARN("unknown file in cabinet (%s)\n",debugstr_a(pfdin->psz1));
            return 0;
        }

        if (f->state != msifs_missing && f->state != msifs_overwrite)
        {
            TRACE("Skipping extraction of %s\n",debugstr_a(pfdin->psz1));
            return 0;
        }

        msi_file_update_ui( data->package, f, szInstallFiles );

        TRACE("extracting %s\n", debugstr_w(f->TargetPath) );

        attrs = f->Attributes & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
        if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;

        handle = CreateFileW( f->TargetPath, GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, CREATE_ALWAYS, attrs, NULL );
        if ( handle == INVALID_HANDLE_VALUE )
        {
            if ( GetFileAttributesW( f->TargetPath ) != INVALID_FILE_ATTRIBUTES )
                f->state = msifs_installed;
            else
                ERR("failed to create %s (error %d)\n",
                    debugstr_w( f->TargetPath ), GetLastError() );

            return 0;
        }

        f->state = msifs_installed;
        return (INT_PTR) handle;
    }
    case fdintCLOSE_FILE_INFO:
    {
        CabData *data = (CabData*) pfdin->pv;
        FILETIME ft;
        FILETIME ftLocal;
        HANDLE handle = (HANDLE) pfdin->hf;

        data->mi->is_continuous = FALSE;

        if (!DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
            return -1;
        if (!LocalFileTimeToFileTime(&ft, &ftLocal))
            return -1;
        if (!SetFileTime(handle, &ftLocal, 0, &ftLocal))
            return -1;
        CloseHandle(handle);
        return 1;
    }
    default:
        return 0;
    }
}

/***********************************************************************
 *            msi_cabextract
 *
 * Extract files from a cab file.
 */
BOOL msi_cabextract(MSIPACKAGE* package, MSIMEDIAINFO *mi,
                    PFNFDINOTIFY notify, LPVOID data)
{
    LPSTR cabinet, cab_path = NULL;
    LPWSTR ptr;
    HFDI hfdi;
    ERF erf;
    BOOL ret = FALSE;

    TRACE("Extracting %s\n", debugstr_w(mi->source));

    hfdi = FDICreate(cabinet_alloc, cabinet_free, cabinet_open, cabinet_read,
                     cabinet_write, cabinet_close, cabinet_seek, 0, &erf);
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    ptr = strrchrW(mi->source, '\\') + 1;
    cabinet = strdupWtoA(ptr);
    if (!cabinet)
        goto done;

    cab_path = strdupWtoA(mi->source);
    if (!cab_path)
        goto done;

    cab_path[ptr - mi->source] = '\0';

    ret = FDICopy(hfdi, cabinet, cab_path, 0, notify, NULL, data);
    if (!ret)
        ERR("FDICopy failed\n");

done:
    FDIDestroy(hfdi);
    msi_free(cabinet);
    msi_free(cab_path);

    if (ret)
        mi->is_extracted = TRUE;

    return ret;
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

void msi_free_media_info( MSIMEDIAINFO *mi )
{
    msi_free( mi->disk_prompt );
    msi_free( mi->cabinet );
    msi_free( mi->volume_label );
    msi_free( mi->first_volume );
    msi_free( mi );
}

UINT msi_load_media_info(MSIPACKAGE *package, MSIFILE *file, MSIMEDIAINFO *mi)
{
    MSIRECORD *row;
    LPWSTR source_dir;
    LPWSTR source;
    DWORD options;
    UINT r;

    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
        '`','M','e','d','i','a','`',' ','W','H','E','R','E',' ',
        '`','L','a','s','t','S','e','q','u','e','n','c','e','`',' ','>','=',
        ' ','%','i',' ','A','N','D',' ','`','D','i','s','k','I','d','`',' ','>','=',
        ' ','%','i',' ','O','R','D','E','R',' ','B','Y',' ',
        '`','D','i','s','k','I','d','`',0
    };

    row = MSI_QueryGetRecord(package->db, query, file->Sequence, mi->disk_id);
    if (!row)
    {
        TRACE("Unable to query row\n");
        return ERROR_FUNCTION_FAILED;
    }

    mi->is_extracted = FALSE;
    mi->disk_id = MSI_RecordGetInteger(row, 1);
    mi->last_sequence = MSI_RecordGetInteger(row, 2);
    msi_free(mi->disk_prompt);
    mi->disk_prompt = strdupW(MSI_RecordGetString(row, 3));
    msi_free(mi->cabinet);
    mi->cabinet = strdupW(MSI_RecordGetString(row, 4));
    msi_free(mi->volume_label);
    mi->volume_label = strdupW(MSI_RecordGetString(row, 5));
    msiobj_release(&row->hdr);

    if (!mi->first_volume)
        mi->first_volume = strdupW(mi->volume_label);

    source_dir = msi_dup_property(package, cszSourceDir);
    lstrcpyW(mi->source, source_dir);

    PathStripToRootW(source_dir);
    mi->type = GetDriveTypeW(source_dir);

    if (file->IsCompressed && mi->cabinet)
    {
        if (mi->cabinet[0] == '#')
        {
            r = writeout_cabinet_stream(package, &mi->cabinet[1], mi->source);
            if (r != ERROR_SUCCESS)
            {
                ERR("Failed to extract cabinet stream\n");
                return ERROR_FUNCTION_FAILED;
            }
        }
        else
            lstrcatW(mi->source, mi->cabinet);
    }

    options = MSICODE_PRODUCT;
    if (mi->type == DRIVE_CDROM || mi->type == DRIVE_REMOVABLE)
    {
        source = source_dir;
        options |= MSISOURCETYPE_MEDIA;
    }
    else if (package->BaseURL && UrlIsW(package->BaseURL, URLIS_URL))
    {
        source = package->BaseURL;
        options |= MSISOURCETYPE_URL;
    }
    else
    {
        source = mi->source;
        options |= MSISOURCETYPE_NETWORK;
    }

    msi_package_add_media_disk(package, package->Context,
                               MSICODE_PRODUCT, mi->disk_id,
                               mi->volume_label, mi->disk_prompt);

    msi_package_add_info(package, package->Context,
                         options, INSTALLPROPERTY_LASTUSEDSOURCEW, source);

    msi_free(source_dir);
    return ERROR_SUCCESS;
}

/* FIXME: search NETWORK and URL sources as well */
UINT find_published_source(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    WCHAR source[MAX_PATH];
    WCHAR volume[MAX_PATH];
    WCHAR prompt[MAX_PATH];
    DWORD volumesz, promptsz;
    DWORD index, size, id;
    UINT r;

    size = MAX_PATH;
    r = MsiSourceListGetInfoW(package->ProductCode, NULL,
                              package->Context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCEW, source, &size);
    if (r != ERROR_SUCCESS)
        return r;

    index = 0;
    volumesz = MAX_PATH;
    promptsz = MAX_PATH;
    while (MsiSourceListEnumMediaDisksW(package->ProductCode, NULL,
                                        package->Context,
                                        MSICODE_PRODUCT, index++, &id,
                                        volume, &volumesz, prompt, &promptsz) == ERROR_SUCCESS)
    {
        mi->disk_id = id;
        mi->volume_label = msi_realloc(mi->volume_label, ++volumesz * sizeof(WCHAR));
        lstrcpyW(mi->volume_label, volume);
        mi->disk_prompt = msi_realloc(mi->disk_prompt, ++promptsz * sizeof(WCHAR));
        lstrcpyW(mi->disk_prompt, prompt);

        if (source_matches_volume(mi, source))
        {
            /* FIXME: what about SourceDir */
            lstrcpyW(mi->source, source);
            lstrcatW(mi->source, mi->cabinet);
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT ready_media(MSIPACKAGE *package, MSIFILE *file, MSIMEDIAINFO *mi)
{
    UINT rc = ERROR_SUCCESS;

    /* media info for continuous cabinet is already loaded */
    if (mi->is_continuous)
        return ERROR_SUCCESS;

    rc = msi_load_media_info(package, file, mi);
    if (rc != ERROR_SUCCESS)
    {
        ERR("Unable to load media info\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* cabinet is internal, no checks needed */
    if (!mi->cabinet || mi->cabinet[0] == '#')
        return ERROR_SUCCESS;

    /* package should be downloaded */
    if (file->IsCompressed &&
        GetFileAttributesW(mi->source) == INVALID_FILE_ATTRIBUTES &&
        package->BaseURL && UrlIsW(package->BaseURL, URLIS_URL))
    {
        WCHAR temppath[MAX_PATH];

        msi_download_file(mi->source, temppath);
        lstrcpyW(mi->source, temppath);
        return ERROR_SUCCESS;
    }

    /* check volume matches, change media if not */
    if (mi->volume_label && mi->disk_id > 1 &&
        lstrcmpW(mi->first_volume, mi->volume_label))
    {
        LPWSTR source = msi_dup_property(package, cszSourceDir);
        BOOL matches;

        matches = source_matches_volume(mi, source);
        msi_free(source);

        if ((mi->type == DRIVE_CDROM || mi->type == DRIVE_REMOVABLE) && !matches)
        {
            rc = msi_change_media(package, mi);
            if (rc != ERROR_SUCCESS)
                return rc;
        }
    }

    if (file->IsCompressed &&
        GetFileAttributesW(mi->source) == INVALID_FILE_ATTRIBUTES)
    {
        /* FIXME: this might be done earlier in the install process */
        rc = find_published_source(package, mi);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Cabinet not found: %s\n", debugstr_w(mi->source));
            return ERROR_INSTALL_FAILURE;
        }
    }

    return ERROR_SUCCESS;
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

static UINT copy_file(MSIFILE *file)
{
    BOOL ret;

    ret = CopyFileW(file->SourcePath, file->TargetPath, FALSE);
    if (ret)
    {
        file->state = msifs_installed;
        return ERROR_SUCCESS;
    }

    return GetLastError();
}

static UINT copy_install_file(MSIFILE *file)
{
    UINT gle;

    TRACE("Copying %s to %s\n", debugstr_w(file->SourcePath),
          debugstr_w(file->TargetPath));

    gle = copy_file(file);
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

        gle = copy_file(file);
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
            CabData data;

            rc = ready_media(package, file, mi);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to ready media\n");
                break;
            }

            data.mi = mi;
            data.package = package;

            if (file->IsCompressed &&
                !msi_cabextract(package, mi, cabinet_notify, &data))
            {
                ERR("Failed to extract cabinet: %s\n", debugstr_w(mi->cabinet));
                rc = ERROR_FUNCTION_FAILED;
                break;
            }
        }

        if (!file->IsCompressed)
        {
            TRACE("file paths %s to %s\n", debugstr_w(file->SourcePath),
                  debugstr_w(file->TargetPath));

            msi_file_update_ui(package, file, szInstallFiles);
            rc = copy_install_file(file);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to copy %s to %s (%d)\n", debugstr_w(file->SourcePath),
                    debugstr_w(file->TargetPath), rc);
                rc = ERROR_INSTALL_FAILURE;
                break;
            }
        }
        else if (file->state != msifs_installed)
        {
            ERR("compressed file wasn't extracted (%s)\n", debugstr_w(file->TargetPath));
            rc = ERROR_INSTALL_FAILURE;
            break;
        }
    }

    msi_free_media_info( mi );
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

UINT ACTION_RemoveFiles( MSIPACKAGE *package )
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        MSIRECORD *uirow;
        LPWSTR uipath, p;

        if ( !file->Component )
            continue;
        if ( file->Component->Installed == INSTALLSTATE_LOCAL )
            continue;

        if ( file->state == msifs_installed )
            ERR("removing installed file %s\n", debugstr_w(file->TargetPath));

        if ( file->state != msifs_present )
            continue;

        /* only remove a file if the version to be installed
         * is strictly newer than the old file
         */
        if ( msi_compare_file_version( file ) >= 0 )
            continue;

        TRACE("removing %s\n", debugstr_w(file->File) );
        if ( !DeleteFileW( file->TargetPath ) )
            ERR("failed to delete %s\n",  debugstr_w(file->TargetPath) );
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
