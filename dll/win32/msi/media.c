/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2008 James Hawkins
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

#include <fcntl.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winerror.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msipriv.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "objidl.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static BOOL source_matches_volume(MSIMEDIAINFO *mi, LPCWSTR source_root)
{
    WCHAR volume_name[MAX_PATH + 1], root[MAX_PATH + 1];
    const WCHAR *p;
    int len, len2;

    lstrcpyW(root, source_root);
    PathStripToRootW(root);
    PathAddBackslashW(root);

    if (!GetVolumeInformationW(root, volume_name, MAX_PATH + 1, NULL, NULL, NULL, NULL, 0))
    {
        WARN( "failed to get volume information for %s (%lu)\n", debugstr_w(root), GetLastError() );
        return FALSE;
    }

    len = lstrlenW( volume_name );
    len2 = lstrlenW( mi->volume_label );
    if (len2 > len) return FALSE;
    p = volume_name + len - len2;

    return !wcsicmp( mi->volume_label, p );
}

static UINT change_media(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    MSIRECORD *record;
    LPWSTR source_dir;
    UINT r = IDRETRY;

    source_dir = msi_dup_property(package->db, L"SourceDir");
    record = MSI_CreateRecord(2);

    while (r == IDRETRY && !source_matches_volume(mi, source_dir))
    {
        MSI_RecordSetStringW(record, 0, NULL);
        MSI_RecordSetInteger(record, 1, MSIERR_CABNOTFOUND);
        MSI_RecordSetStringW(record, 2, mi->disk_prompt);
        r = MSI_ProcessMessage(package, INSTALLMESSAGE_ERROR | MB_RETRYCANCEL, record);
    }

    msiobj_release(&record->hdr);
    free(source_dir);

    return r == IDRETRY ? ERROR_SUCCESS : ERROR_INSTALL_SOURCE_ABSENT;
}

static MSICABINETSTREAM *get_cabinet_stream( MSIPACKAGE *package, UINT disk_id )
{
    MSICABINETSTREAM *cab;

    LIST_FOR_EACH_ENTRY( cab, &package->cabinet_streams, MSICABINETSTREAM, entry )
    {
        if (cab->disk_id == disk_id) return cab;
    }
    return NULL;
}

static void * CDECL cabinet_alloc(ULONG cb)
{
    return malloc(cb);
}

static void CDECL cabinet_free(void *pv)
{
    free(pv);
}

static INT_PTR CDECL cabinet_open(char *pszFile, int oflag, int pmode)
{
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    HANDLE handle;
    WCHAR *path;

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

    path = strdupUtoW(pszFile);
    handle = CreateFileW(path, dwAccess, dwShareMode, NULL, dwCreateDisposition, 0, NULL);
    free(path);
    return (INT_PTR)handle;
}

static UINT CDECL cabinet_read(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE)hf;
    DWORD read;

    if (ReadFile(handle, pv, cb, &read, NULL))
        return read;

    return 0;
}

static UINT CDECL cabinet_write(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE)hf;
    DWORD written;

    if (WriteFile(handle, pv, cb, &written, NULL))
        return written;

    return 0;
}

static int CDECL cabinet_close(INT_PTR hf)
{
    HANDLE handle = (HANDLE)hf;
    return CloseHandle(handle) ? 0 : -1;
}

static LONG CDECL cabinet_seek(INT_PTR hf, LONG dist, int seektype)
{
    HANDLE handle = (HANDLE)hf;
    /* flags are compatible and so are passed straight through */
    return SetFilePointer(handle, dist, NULL, seektype);
}

struct package_disk
{
    MSIPACKAGE *package;
    UINT        id;
};

static struct package_disk package_disk;

static INT_PTR CDECL cabinet_open_stream( char *pszFile, int oflag, int pmode )
{
    MSICABINETSTREAM *cab;
    IStream *stream;

    if (!(cab = get_cabinet_stream( package_disk.package, package_disk.id )))
    {
        WARN("failed to get cabinet stream\n");
        return -1;
    }
    if (cab->storage == package_disk.package->db->storage)
    {
        UINT r = msi_get_stream( package_disk.package->db, cab->stream + 1, &stream );
        if (r != ERROR_SUCCESS)
        {
            WARN("failed to get stream %u\n", r);
            return -1;
        }
    }
    else /* patch storage */
    {
        HRESULT hr;
        WCHAR *encoded;

        if (!(encoded = encode_streamname( FALSE, cab->stream + 1 )))
        {
            WARN("failed to encode stream name\n");
            return -1;
        }
        hr = IStorage_OpenStream( cab->storage, encoded, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &stream );
        free( encoded );
        if (FAILED(hr))
        {
            WARN( "failed to open stream %#lx\n", hr );
            return -1;
        }
    }
    return (INT_PTR)stream;
}

static UINT CDECL cabinet_read_stream( INT_PTR hf, void *pv, UINT cb )
{
    IStream *stm = (IStream *)hf;
    DWORD read;
    HRESULT hr;

    hr = IStream_Read( stm, pv, cb, &read );
    if (hr == S_OK || hr == S_FALSE)
        return read;

    return 0;
}

static int CDECL cabinet_close_stream( INT_PTR hf )
{
    IStream *stm = (IStream *)hf;
    IStream_Release( stm );
    return 0;
}

static LONG CDECL cabinet_seek_stream( INT_PTR hf, LONG dist, int seektype )
{
    IStream *stm = (IStream *)hf;
    LARGE_INTEGER move;
    ULARGE_INTEGER newpos;
    HRESULT hr;

    move.QuadPart = dist;
    hr = IStream_Seek( stm, move, seektype, &newpos );
    if (SUCCEEDED(hr))
    {
        if (newpos.QuadPart <= MAXLONG) return newpos.QuadPart;
        ERR("Too big!\n");
    }
    return -1;
}

static UINT media_get_disk_info(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    MSIRECORD *row;

    row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `Media` WHERE `DiskId` = %d", mi->disk_id);
    if (!row)
    {
        TRACE("Unable to query row\n");
        return ERROR_FUNCTION_FAILED;
    }

    mi->disk_prompt = wcsdup(MSI_RecordGetString(row, 3));
    mi->cabinet = wcsdup(MSI_RecordGetString(row, 4));
    mi->volume_label = wcsdup(MSI_RecordGetString(row, 5));

    msiobj_release(&row->hdr);
    return ERROR_SUCCESS;
}

static INT_PTR cabinet_partial_file(FDINOTIFICATIONTYPE fdint,
                                    PFDINOTIFICATION pfdin)
{
    MSICABDATA *data = pfdin->pv;
    data->mi->is_continuous = FALSE;
    return 0;
}

static WCHAR *get_cabinet_filename(MSIMEDIAINFO *mi)
{
    int len;
    WCHAR *ret;

    len = lstrlenW(mi->sourcedir) + lstrlenW(mi->cabinet) + 1;
    if (!(ret = malloc(len * sizeof(WCHAR)))) return NULL;
    lstrcpyW(ret, mi->sourcedir);
    lstrcatW(ret, mi->cabinet);
    return ret;
}

static INT_PTR cabinet_next_cabinet(FDINOTIFICATIONTYPE fdint,
                                    PFDINOTIFICATION pfdin)
{
    MSICABDATA *data = pfdin->pv;
    MSIMEDIAINFO *mi = data->mi;
    LPWSTR cabinet_file = NULL, cab = strdupAtoW(pfdin->psz1);
    INT_PTR res = -1;
    UINT rc;

    free(mi->disk_prompt);
    free(mi->cabinet);
    free(mi->volume_label);
    mi->disk_prompt = NULL;
    mi->cabinet = NULL;
    mi->volume_label = NULL;

    mi->disk_id++;
    mi->is_continuous = TRUE;

    rc = media_get_disk_info(data->package, mi);
    if (rc != ERROR_SUCCESS)
    {
        ERR("Failed to get next cabinet information: %d\n", rc);
        goto done;
    }

    if (wcsicmp( mi->cabinet, cab ))
    {
        char *next_cab;
        ULONG length;

        WARN("Continuous cabinet %s does not match the next cabinet %s in the media table => use latter one\n", debugstr_w(cab), debugstr_w(mi->cabinet));

        /* Use cabinet name from the media table */
        next_cab = strdupWtoA(mi->cabinet);
        /* Modify path to cabinet file with full filename (psz3 points to a 256 bytes buffer that can be modified contrary to psz1 and psz2) */
        length = strlen(pfdin->psz3) + 1 + strlen(next_cab) + 1;
        if (length > 256)
        {
            WARN( "cannot update next cabinet filename with a string size %lu > 256\n", length );
            free(next_cab);
            goto done;
        }
        else
        {
            strcat(pfdin->psz3, "\\");
            strcat(pfdin->psz3, next_cab);
        }
        /* Path psz3 and cabinet psz1 are concatenated by FDI so just reset psz1 */
        *pfdin->psz1 = 0;
        free(next_cab);
    }

    if (!(cabinet_file = get_cabinet_filename(mi)))
        goto done;

    TRACE("Searching for %s\n", debugstr_w(cabinet_file));

    res = 0;
    if (GetFileAttributesW(cabinet_file) == INVALID_FILE_ATTRIBUTES)
    {
        if (change_media(data->package, mi) != ERROR_SUCCESS)
            res = -1;
    }

done:
    free(cab);
    free(cabinet_file);
    return res;
}

static INT_PTR cabinet_next_cabinet_stream( FDINOTIFICATIONTYPE fdint,
                                            PFDINOTIFICATION pfdin )
{
    MSICABDATA *data = pfdin->pv;
    MSIMEDIAINFO *mi = data->mi;
    UINT rc;

    free( mi->disk_prompt );
    free( mi->cabinet );
    free( mi->volume_label );
    mi->disk_prompt = NULL;
    mi->cabinet = NULL;
    mi->volume_label = NULL;

    mi->disk_id++;
    mi->is_continuous = TRUE;

    rc = media_get_disk_info( data->package, mi );
    if (rc != ERROR_SUCCESS)
    {
        ERR("Failed to get next cabinet information: %u\n", rc);
        return -1;
    }
    package_disk.id = mi->disk_id;

    TRACE("next cabinet is %s disk id %u\n", debugstr_w(mi->cabinet), mi->disk_id);
    return 0;
}

static INT_PTR cabinet_copy_file(FDINOTIFICATIONTYPE fdint,
                                 PFDINOTIFICATION pfdin)
{
    MSICABDATA *data = pfdin->pv;
    HANDLE handle = 0;
    LPWSTR path = NULL;
    DWORD attrs;

    data->curfile = strdupAtoW(pfdin->psz1);
    if (!data->cb(data->package, data->curfile, MSICABEXTRACT_BEGINEXTRACT, &path,
                  &attrs, data->user))
    {
        /* We're not extracting this file, so free the filename. */
        free(data->curfile);
        data->curfile = NULL;
        goto done;
    }

    TRACE("extracting %s -> %s\n", debugstr_w(data->curfile), debugstr_w(path));

    attrs = attrs & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
    if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;

    handle = msi_create_file( data->package, path, GENERIC_READ | GENERIC_WRITE, 0, CREATE_ALWAYS, attrs );
    if (handle == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        DWORD attrs2 = msi_get_file_attributes( data->package, path );

        if (attrs2 == INVALID_FILE_ATTRIBUTES)
        {
            ERR( "failed to create %s (error %lu)\n", debugstr_w(path), err );
            goto done;
        }
        else if (err == ERROR_ACCESS_DENIED && (attrs2 & FILE_ATTRIBUTE_READONLY))
        {
            TRACE("removing read-only attribute on %s\n", debugstr_w(path));
            msi_set_file_attributes( data->package, path, attrs2 & ~FILE_ATTRIBUTE_READONLY );
            handle = msi_create_file( data->package, path, GENERIC_READ | GENERIC_WRITE, 0, CREATE_ALWAYS, attrs );

            if (handle != INVALID_HANDLE_VALUE) goto done;
            err = GetLastError();
        }
        if (err == ERROR_SHARING_VIOLATION || err == ERROR_USER_MAPPED_FILE)
        {
            WCHAR *tmpfileW, *tmppathW, *p;
            DWORD len;

            TRACE("file in use, scheduling rename operation\n");

            if (!(tmppathW = wcsdup(path))) return ERROR_OUTOFMEMORY;
            if ((p = wcsrchr(tmppathW, '\\'))) *p = 0;
            len = lstrlenW( tmppathW ) + 16;
            if (!(tmpfileW = malloc(len * sizeof(WCHAR))))
            {
                free( tmppathW );
                return ERROR_OUTOFMEMORY;
            }
            if (!msi_get_temp_file_name( data->package, tmppathW, L"msi", tmpfileW )) tmpfileW[0] = 0;
            free( tmppathW );

            handle = msi_create_file( data->package, tmpfileW, GENERIC_READ | GENERIC_WRITE, 0, CREATE_ALWAYS, attrs );

            if (handle != INVALID_HANDLE_VALUE &&
                msi_move_file( data->package, path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT ) &&
                msi_move_file( data->package, tmpfileW, path, MOVEFILE_DELAY_UNTIL_REBOOT ))
            {
                data->package->need_reboot_at_end = 1;
            }
            else
            {
                WARN( "failed to schedule rename operation %s (error %lu)\n", debugstr_w(path), GetLastError() );
                msi_delete_file( data->package, tmpfileW );
            }
            free(tmpfileW);
        }
        else WARN( "failed to create %s (error %lu)\n", debugstr_w(path), err );
    }

done:
    free(path);

    return (INT_PTR)handle;
}

static INT_PTR cabinet_close_file_info(FDINOTIFICATIONTYPE fdint,
                                       PFDINOTIFICATION pfdin)
{
    MSICABDATA *data = pfdin->pv;
    FILETIME ft;
    FILETIME ftLocal;
    HANDLE handle = (HANDLE)pfdin->hf;

    data->mi->is_continuous = FALSE;

    if (!DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
    {
        CloseHandle(handle);
        return -1;
    }
    if (!LocalFileTimeToFileTime(&ft, &ftLocal))
    {
        CloseHandle(handle);
        return -1;
    }
    if (!SetFileTime(handle, &ftLocal, 0, &ftLocal))
    {
        CloseHandle(handle);
        return -1;
    }

    CloseHandle(handle);
    data->cb(data->package, data->curfile, MSICABEXTRACT_FILEEXTRACTED, NULL, NULL, data->user);

    free(data->curfile);
    data->curfile = NULL;

    return 1;
}

static INT_PTR CDECL cabinet_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    switch (fdint)
    {
    case fdintPARTIAL_FILE:
        return cabinet_partial_file(fdint, pfdin);

    case fdintNEXT_CABINET:
        return cabinet_next_cabinet(fdint, pfdin);

    case fdintCOPY_FILE:
        return cabinet_copy_file(fdint, pfdin);

    case fdintCLOSE_FILE_INFO:
        return cabinet_close_file_info(fdint, pfdin);

    default:
        return 0;
    }
}

static INT_PTR CDECL cabinet_notify_stream( FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin )
{
    switch (fdint)
    {
    case fdintPARTIAL_FILE:
        return cabinet_partial_file( fdint, pfdin );

    case fdintNEXT_CABINET:
        return cabinet_next_cabinet_stream( fdint, pfdin );

    case fdintCOPY_FILE:
        return cabinet_copy_file( fdint, pfdin );

    case fdintCLOSE_FILE_INFO:
        return cabinet_close_file_info( fdint, pfdin );

    case fdintCABINET_INFO:
        return 0;

    default:
        ERR("Unexpected notification %d\n", fdint);
        return 0;
    }
}

static BOOL extract_cabinet( MSIPACKAGE* package, MSIMEDIAINFO *mi, LPVOID data )
{
    LPSTR cabinet, cab_path = NULL;
    HFDI hfdi;
    ERF erf;
    BOOL ret = FALSE;

    TRACE("extracting %s disk id %u\n", debugstr_w(mi->cabinet), mi->disk_id);

    hfdi = FDICreate( cabinet_alloc, cabinet_free, cabinet_open, cabinet_read,
                      cabinet_write, cabinet_close, cabinet_seek, 0, &erf );
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    cabinet = strdupWtoU( mi->cabinet );
    if (!cabinet)
        goto done;

    cab_path = strdupWtoU( mi->sourcedir );
    if (!cab_path)
        goto done;

    ret = FDICopy( hfdi, cabinet, cab_path, 0, cabinet_notify, NULL, data );
    if (!ret)
        ERR("FDICopy failed\n");

done:
    FDIDestroy( hfdi );
    free( cabinet );
    free( cab_path );

    if (ret)
        mi->is_extracted = TRUE;

    return ret;
}

static BOOL extract_cabinet_stream( MSIPACKAGE *package, MSIMEDIAINFO *mi, LPVOID data )
{
    static char filename[] = {'<','S','T','R','E','A','M','>',0};
    HFDI hfdi;
    ERF erf;
    BOOL ret = FALSE;

    TRACE("extracting %s disk id %u\n", debugstr_w(mi->cabinet), mi->disk_id);

    hfdi = FDICreate( cabinet_alloc, cabinet_free, cabinet_open_stream, cabinet_read_stream,
                      cabinet_write, cabinet_close_stream, cabinet_seek_stream, 0, &erf );
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    package_disk.package = package;
    package_disk.id      = mi->disk_id;

    ret = FDICopy( hfdi, filename, NULL, 0, cabinet_notify_stream, NULL, data );
    if (!ret) ERR("FDICopy failed\n");

    FDIDestroy( hfdi );
    if (ret) mi->is_extracted = TRUE;
    return ret;
}

/***********************************************************************
 *            msi_cabextract
 *
 * Extract files from a cabinet file or stream.
 */
BOOL msi_cabextract(MSIPACKAGE* package, MSIMEDIAINFO *mi, LPVOID data)
{
    if (mi->cabinet[0] == '#')
    {
        return extract_cabinet_stream( package, mi, data );
    }
    return extract_cabinet( package, mi, data );
}

void msi_free_media_info(MSIMEDIAINFO *mi)
{
    free(mi->disk_prompt);
    free(mi->cabinet);
    free(mi->volume_label);
    free(mi->last_volume);
    free(mi);
}

static UINT get_drive_type(const WCHAR *path)
{
    WCHAR root[MAX_PATH + 1];

    lstrcpyW(root, path);
    PathStripToRootW(root);
    PathAddBackslashW(root);

    return GetDriveTypeW(root);
}

static WCHAR *get_base_url( MSIDATABASE *db )
{
    WCHAR *p, *ret = NULL, *orig_db = msi_dup_property( db, L"OriginalDatabase" );
    if (UrlIsW( orig_db, URLIS_URL ) && (ret = wcsdup( orig_db )) && (p = wcsrchr( ret, '/' ))) p[1] = 0;
    free( orig_db );
    return ret;
}

UINT msi_load_media_info(MSIPACKAGE *package, UINT Sequence, MSIMEDIAINFO *mi)
{
    MSIRECORD *row;
    WCHAR *source_dir, *source, *base_url = NULL;
    DWORD options;

    if (Sequence <= mi->last_sequence) /* already loaded */
        return ERROR_SUCCESS;

    row = MSI_QueryGetRecord(package->db, L"SELECT * FROM `Media` WHERE `LastSequence` >= %d ORDER BY `DiskId`", Sequence);
    if (!row)
    {
        TRACE("Unable to query row\n");
        return ERROR_FUNCTION_FAILED;
    }

    mi->is_extracted = FALSE;
    mi->disk_id = MSI_RecordGetInteger(row, 1);
    mi->last_sequence = MSI_RecordGetInteger(row, 2);
    free(mi->disk_prompt);
    mi->disk_prompt = wcsdup(MSI_RecordGetString(row, 3));
    free(mi->cabinet);
    mi->cabinet = wcsdup(MSI_RecordGetString(row, 4));
    free(mi->volume_label);
    mi->volume_label = wcsdup(MSI_RecordGetString(row, 5));
    msiobj_release(&row->hdr);

    msi_set_sourcedir_props(package, FALSE);
    source_dir = msi_dup_property(package->db, L"SourceDir");
    lstrcpyW(mi->sourcedir, source_dir);
    PathAddBackslashW(mi->sourcedir);
    mi->type = get_drive_type(source_dir);

    options = MSICODE_PRODUCT;
    if (mi->type == DRIVE_CDROM || mi->type == DRIVE_REMOVABLE)
    {
        source = source_dir;
        options |= MSISOURCETYPE_MEDIA;
    }
    else if ((base_url = get_base_url(package->db)))
    {
        source = base_url;
        options |= MSISOURCETYPE_URL;
    }
    else
    {
        source = mi->sourcedir;
        options |= MSISOURCETYPE_NETWORK;
    }

    msi_package_add_media_disk(package, package->Context,
                               MSICODE_PRODUCT, mi->disk_id,
                               mi->volume_label, mi->disk_prompt);

    msi_package_add_info(package, package->Context,
                         options, INSTALLPROPERTY_LASTUSEDSOURCEW, source);

    TRACE("sequence %u -> cabinet %s disk id %u\n", Sequence, debugstr_w(mi->cabinet), mi->disk_id);

    free(base_url);
    free(source_dir);
    return ERROR_SUCCESS;
}

/* FIXME: search URL sources as well */
static UINT find_published_source(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    WCHAR source[MAX_PATH];
    WCHAR volume[MAX_PATH];
    WCHAR prompt[MAX_PATH];
    DWORD volumesz, promptsz;
    DWORD index, size, id;
    WCHAR last_type[2];
    UINT r;

    size = 2;
    r = MsiSourceListGetInfoW(package->ProductCode, NULL,
                              package->Context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDTYPEW, last_type, &size);
    if (r != ERROR_SUCCESS)
        return r;

    size = MAX_PATH;
    r = MsiSourceListGetInfoW(package->ProductCode, NULL,
                              package->Context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCEW, source, &size);
    if (r != ERROR_SUCCESS)
        return r;

    if (last_type[0] == 'n')
    {
        WCHAR cabinet_file[MAX_PATH];
        BOOL check_all = FALSE;

        while(TRUE)
        {
            index = 0;
            volumesz = MAX_PATH;
            while (MsiSourceListEnumSourcesW(package->ProductCode, NULL,
                                             package->Context,
                                             MSISOURCETYPE_NETWORK, index++,
                                             volume, &volumesz) == ERROR_SUCCESS)
            {
                if (check_all || !wcsnicmp(source, volume, lstrlenW(source)))
                {
                    lstrcpyW(cabinet_file, volume);
                    PathAddBackslashW(cabinet_file);
                    lstrcatW(cabinet_file, mi->cabinet);

                    if (GetFileAttributesW(cabinet_file) == INVALID_FILE_ATTRIBUTES)
                    {
                        volumesz = MAX_PATH;
                        if(!check_all)
                            break;
                        continue;
                    }

                    lstrcpyW(mi->sourcedir, volume);
                    PathAddBackslashW(mi->sourcedir);
                    TRACE("Found network source %s\n", debugstr_w(mi->sourcedir));
                    return ERROR_SUCCESS;
                }
            }

            if (!check_all)
                check_all = TRUE;
            else
                break;
        }
    }

    index = 0;
    volumesz = MAX_PATH;
    promptsz = MAX_PATH;
    while (MsiSourceListEnumMediaDisksW(package->ProductCode, NULL,
                                        package->Context,
                                        MSICODE_PRODUCT, index++, &id,
                                        volume, &volumesz, prompt, &promptsz) == ERROR_SUCCESS)
    {
        mi->disk_id = id;
        free( mi->volume_label );
        if (!(mi->volume_label = malloc( ++volumesz * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
        lstrcpyW( mi->volume_label, volume );

        free( mi->disk_prompt );
        if (!(mi->disk_prompt = malloc( ++promptsz * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
        lstrcpyW( mi->disk_prompt, prompt );

        if (source_matches_volume(mi, source))
        {
            /* FIXME: what about SourceDir */
            lstrcpyW(mi->sourcedir, source);
            PathAddBackslashW(mi->sourcedir);
            TRACE("Found disk source %s\n", debugstr_w(mi->sourcedir));
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

UINT ready_media( MSIPACKAGE *package, BOOL compressed, MSIMEDIAINFO *mi )
{
    UINT rc;
    WCHAR *cabinet_file = NULL;

    /* media info for continuous cabinet is already loaded */
    if (mi->is_continuous) return ERROR_SUCCESS;

    if (mi->cabinet)
    {
        WCHAR *base_url;

        /* cabinet is internal, no checks needed */
        if (mi->cabinet[0] == '#') return ERROR_SUCCESS;

        if (!(cabinet_file = get_cabinet_filename( mi ))) return ERROR_OUTOFMEMORY;

        /* package should be downloaded */
        if (compressed && GetFileAttributesW( cabinet_file ) == INVALID_FILE_ATTRIBUTES &&
            (base_url = get_base_url( package->db )))
        {
            WCHAR temppath[MAX_PATH], *p, *url;

            free( cabinet_file );
            if (!(url = realloc( base_url, (wcslen( base_url ) + wcslen( mi->cabinet ) + 1) * sizeof(WCHAR) )))
            {
                free( base_url );
                return ERROR_OUTOFMEMORY;
            }
            lstrcatW( url, mi->cabinet );
            if ((rc = msi_download_file( url, temppath )) != ERROR_SUCCESS)
            {
                ERR("failed to download %s (%u)\n", debugstr_w(url), rc);
                free( url );
                return rc;
            }
            if ((p = wcsrchr( temppath, '\\' ))) *p = 0;
            lstrcpyW( mi->sourcedir, temppath );
            PathAddBackslashW( mi->sourcedir );
            free( mi->cabinet );
            mi->cabinet = wcsdup( p + 1 );

            free( url );
            return ERROR_SUCCESS;
        }
    }
    /* check volume matches, change media if not */
    if (mi->volume_label)
    {
        /* assume first volume is in the drive */
        if (mi->last_volume && wcsicmp( mi->last_volume, mi->volume_label ))
        {
            WCHAR *source = msi_dup_property( package->db, L"SourceDir" );
            BOOL match = source_matches_volume( mi, source );
            free( source );

            if (!match && (mi->type == DRIVE_CDROM || mi->type == DRIVE_REMOVABLE))
            {
                if ((rc = change_media( package, mi )) != ERROR_SUCCESS)
                {
                    free( cabinet_file );
                    return rc;
                }
            }
        }

        free(mi->last_volume);
        mi->last_volume = wcsdup(mi->volume_label);
    }
    if (mi->cabinet)
    {
        if (compressed && GetFileAttributesW( cabinet_file ) == INVALID_FILE_ATTRIBUTES)
        {
            if ((rc = find_published_source( package, mi )) != ERROR_SUCCESS)
            {
                ERR("cabinet not found: %s\n", debugstr_w(cabinet_file));
                free( cabinet_file );
                return ERROR_INSTALL_FAILURE;
            }
        }
    }
    free( cabinet_file );
    return ERROR_SUCCESS;
}

UINT msi_add_cabinet_stream( MSIPACKAGE *package, UINT disk_id, IStorage *storage, const WCHAR *name )
{
    MSICABINETSTREAM *cab, *item;

    TRACE("%p, %u, %p, %s\n", package, disk_id, storage, debugstr_w(name));

    LIST_FOR_EACH_ENTRY( item, &package->cabinet_streams, MSICABINETSTREAM, entry )
    {
        if (item->disk_id == disk_id)
        {
            TRACE("duplicate disk id %u\n", disk_id);
            return ERROR_FUNCTION_FAILED;
        }
    }
    if (!(cab = malloc( sizeof(*cab) ))) return ERROR_OUTOFMEMORY;
    if (!(cab->stream = malloc( (wcslen( name ) + 1) * sizeof(WCHAR) )))
    {
        free( cab );
        return ERROR_OUTOFMEMORY;
    }
    lstrcpyW( cab->stream, name );
    cab->disk_id = disk_id;
    cab->storage = storage;
    IStorage_AddRef( storage );
    list_add_tail( &package->cabinet_streams, &cab->entry );

    return ERROR_SUCCESS;
}
