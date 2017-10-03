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

#include "msipriv.h"

#include <fdi.h>

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/* from msvcrt/fcntl.h */
#define _O_RDONLY      0
#define _O_WRONLY      1
#define _O_RDWR        2
#define _O_ACCMODE     (_O_RDONLY|_O_WRONLY|_O_RDWR)
#define _O_APPEND      0x0008
#define _O_RANDOM      0x0010
#define _O_SEQUENTIAL  0x0020
#define _O_TEMPORARY   0x0040
#define _O_NOINHERIT   0x0080
#define _O_CREAT       0x0100
#define _O_TRUNC       0x0200
#define _O_EXCL        0x0400
#define _O_SHORT_LIVED 0x1000
#define _O_TEXT        0x4000
#define _O_BINARY      0x8000

static BOOL source_matches_volume(MSIMEDIAINFO *mi, LPCWSTR source_root)
{
    WCHAR volume_name[MAX_PATH + 1], root[MAX_PATH + 1];
    const WCHAR *p;
    int len, len2;

    strcpyW(root, source_root);
    PathStripToRootW(root);
    PathAddBackslashW(root);

    if (!GetVolumeInformationW(root, volume_name, MAX_PATH + 1, NULL, NULL, NULL, NULL, 0))
    {
        WARN("failed to get volume information for %s (%u)\n", debugstr_w(root), GetLastError());
        return FALSE;
    }

    len = strlenW( volume_name );
    len2 = strlenW( mi->volume_label );
    if (len2 > len) return FALSE;
    p = volume_name + len - len2;

    return !strcmpiW( mi->volume_label, p );
}

static UINT msi_change_media(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    LPWSTR error, error_dialog;
    LPWSTR source_dir;
    UINT r = ERROR_SUCCESS;

    static const WCHAR error_prop[] = {'E','r','r','o','r','D','i','a','l','o','g',0};

    if ((package->ui_level & INSTALLUILEVEL_MASK) == INSTALLUILEVEL_NONE &&
        !gUIHandlerA && !gUIHandlerW && !gUIHandlerRecord) return ERROR_SUCCESS;

    error = msi_build_error_string(package, 1302, 1, mi->disk_prompt);
    error_dialog = msi_dup_property(package->db, error_prop);
    source_dir = msi_dup_property(package->db, szSourceDir);

    while (r == ERROR_SUCCESS && !source_matches_volume(mi, source_dir))
    {
        r = msi_spawn_error_dialog(package, error_dialog, error);

        if (gUIHandlerW)
        {
            gUIHandlerW(gUIContext, MB_RETRYCANCEL | INSTALLMESSAGE_ERROR, error);
        }
        else if (gUIHandlerA)
        {
            char *msg = strdupWtoA(error);
            gUIHandlerA(gUIContext, MB_RETRYCANCEL | INSTALLMESSAGE_ERROR, msg);
            msi_free(msg);
        }
        else if (gUIHandlerRecord)
        {
            MSIHANDLE rec = MsiCreateRecord(1);
            MsiRecordSetStringW(rec, 0, error);
            gUIHandlerRecord(gUIContext, MB_RETRYCANCEL | INSTALLMESSAGE_ERROR, rec);
            MsiCloseHandle(rec);
        }
    }

    msi_free(error);
    msi_free(error_dialog);
    msi_free(source_dir);

    return r;
}

static MSICABINETSTREAM *msi_get_cabinet_stream( MSIPACKAGE *package, UINT disk_id )
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
    return msi_alloc(cb);
}

static void CDECL cabinet_free(void *pv)
{
    msi_free(pv);
}

static INT_PTR CDECL cabinet_open(char *pszFile, int oflag, int pmode)
{
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

    return (INT_PTR)CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                                dwCreateDisposition, 0, NULL);
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

    if (!(cab = msi_get_cabinet_stream( package_disk.package, package_disk.id )))
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
        msi_free( encoded );
        if (FAILED(hr))
        {
            WARN("failed to open stream 0x%08x\n", hr);
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

static UINT CDECL msi_media_get_disk_info(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    MSIRECORD *row;

    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
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

    len = strlenW(mi->sourcedir) + strlenW(mi->cabinet) + 1;
    if (!(ret = msi_alloc(len * sizeof(WCHAR)))) return NULL;
    strcpyW(ret, mi->sourcedir);
    strcatW(ret, mi->cabinet);
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
        ERR("Failed to get next cabinet information: %d\n", rc);
        goto done;
    }

    if (strcmpiW( mi->cabinet, cab ))
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
            WARN("Cannot update next cabinet filename with a string size %u > 256\n", length);
            msi_free(next_cab);
            goto done;
        }
        else
        {
            strcat(pfdin->psz3, "\\");
            strcat(pfdin->psz3, next_cab);
        }
        /* Path psz3 and cabinet psz1 are concatenated by FDI so just reset psz1 */
        *pfdin->psz1 = 0;
        msi_free(next_cab);
    }

    if (!(cabinet_file = get_cabinet_filename(mi)))
        goto done;

    TRACE("Searching for %s\n", debugstr_w(cabinet_file));

    res = 0;
    if (GetFileAttributesW(cabinet_file) == INVALID_FILE_ATTRIBUTES)
    {
        if (msi_change_media(data->package, mi) != ERROR_SUCCESS)
            res = -1;
    }

done:
    msi_free(cab);
    msi_free(cabinet_file);
    return res;
}

static INT_PTR cabinet_next_cabinet_stream( FDINOTIFICATIONTYPE fdint,
                                            PFDINOTIFICATION pfdin )
{
    MSICABDATA *data = pfdin->pv;
    MSIMEDIAINFO *mi = data->mi;
    UINT rc;

    msi_free( mi->disk_prompt );
    msi_free( mi->cabinet );
    msi_free( mi->volume_label );
    mi->disk_prompt = NULL;
    mi->cabinet = NULL;
    mi->volume_label = NULL;

    mi->disk_id++;
    mi->is_continuous = TRUE;

    rc = msi_media_get_disk_info( data->package, mi );
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
        msi_free(data->curfile);
        data->curfile = NULL;
        goto done;
    }

    TRACE("extracting %s -> %s\n", debugstr_w(data->curfile), debugstr_w(path));

    attrs = attrs & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
    if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;

    handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0,
                         NULL, CREATE_ALWAYS, attrs, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        DWORD attrs2 = GetFileAttributesW(path);

        if (attrs2 == INVALID_FILE_ATTRIBUTES)
        {
            ERR("failed to create %s (error %d)\n", debugstr_w(path), err);
            goto done;
        }
        else if (err == ERROR_ACCESS_DENIED && (attrs2 & FILE_ATTRIBUTE_READONLY))
        {
            TRACE("removing read-only attribute on %s\n", debugstr_w(path));
            SetFileAttributesW( path, attrs2 & ~FILE_ATTRIBUTE_READONLY );
            handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, attrs, NULL);

            if (handle != INVALID_HANDLE_VALUE) goto done;
            err = GetLastError();
        }
        if (err == ERROR_SHARING_VIOLATION || err == ERROR_USER_MAPPED_FILE)
        {
            WCHAR *tmpfileW, *tmppathW, *p;
            DWORD len;

            TRACE("file in use, scheduling rename operation\n");

            if (!(tmppathW = strdupW( path ))) return ERROR_OUTOFMEMORY;
            if ((p = strrchrW(tmppathW, '\\'))) *p = 0;
            len = strlenW( tmppathW ) + 16;
            if (!(tmpfileW = msi_alloc(len * sizeof(WCHAR))))
            {
                msi_free( tmppathW );
                return ERROR_OUTOFMEMORY;
            }
            if (!GetTempFileNameW(tmppathW, szMsi, 0, tmpfileW)) tmpfileW[0] = 0;
            msi_free( tmppathW );

            handle = CreateFileW(tmpfileW, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, attrs, NULL);

            if (handle != INVALID_HANDLE_VALUE &&
                MoveFileExW(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT) &&
                MoveFileExW(tmpfileW, path, MOVEFILE_DELAY_UNTIL_REBOOT))
            {
                data->package->need_reboot_at_end = 1;
            }
            else
            {
                WARN("failed to schedule rename operation %s (error %d)\n", debugstr_w(path), GetLastError());
                DeleteFileW( tmpfileW );
            }
            msi_free(tmpfileW);
        }
        else
            WARN("failed to create %s (error %d)\n", debugstr_w(path), err);
    }

done:
    msi_free(path);

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
        return -1;
    if (!LocalFileTimeToFileTime(&ft, &ftLocal))
        return -1;
    if (!SetFileTime(handle, &ftLocal, 0, &ftLocal))
        return -1;

    CloseHandle(handle);

    data->cb(data->package, data->curfile, MSICABEXTRACT_FILEEXTRACTED, NULL, NULL,
             data->user);

    msi_free(data->curfile);
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

    cabinet = strdupWtoA( mi->cabinet );
    if (!cabinet)
        goto done;

    cab_path = strdupWtoA( mi->sourcedir );
    if (!cab_path)
        goto done;

    ret = FDICopy( hfdi, cabinet, cab_path, 0, cabinet_notify, NULL, data );
    if (!ret)
        ERR("FDICopy failed\n");

done:
    FDIDestroy( hfdi );
    msi_free(cabinet );
    msi_free( cab_path );

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
    msi_free(mi->disk_prompt);
    msi_free(mi->cabinet);
    msi_free(mi->volume_label);
    msi_free(mi);
}

static UINT get_drive_type(const WCHAR *path)
{
    WCHAR root[MAX_PATH + 1];

    strcpyW(root, path);
    PathStripToRootW(root);
    PathAddBackslashW(root);

    return GetDriveTypeW(root);
}

UINT msi_load_media_info(MSIPACKAGE *package, UINT Sequence, MSIMEDIAINFO *mi)
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','`','M','e','d','i','a','`',' ',
        'W','H','E','R','E',' ','`','L','a','s','t','S','e','q','u','e','n','c','e','`',' ',
        '>','=',' ','%','i',' ','O','R','D','E','R',' ','B','Y',' ','`','D','i','s','k','I','d','`',0};
    MSIRECORD *row;
    LPWSTR source_dir, source;
    DWORD options;

    if (Sequence <= mi->last_sequence) /* already loaded */
        return ERROR_SUCCESS;

    row = MSI_QueryGetRecord(package->db, query, Sequence);
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

    msi_set_sourcedir_props(package, FALSE);
    source_dir = msi_dup_property(package->db, szSourceDir);
    lstrcpyW(mi->sourcedir, source_dir);
    PathAddBackslashW(mi->sourcedir);
    mi->type = get_drive_type(source_dir);

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
        source = mi->sourcedir;
        options |= MSISOURCETYPE_NETWORK;
    }

    msi_package_add_media_disk(package, package->Context,
                               MSICODE_PRODUCT, mi->disk_id,
                               mi->volume_label, mi->disk_prompt);

    msi_package_add_info(package, package->Context,
                         options, INSTALLPROPERTY_LASTUSEDSOURCEW, source);

    msi_free(source_dir);
    TRACE("sequence %u -> cabinet %s disk id %u\n", Sequence, debugstr_w(mi->cabinet), mi->disk_id);
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
                if (check_all || !strncmpiW(source, volume, strlenW(source)))
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
        msi_free( mi->volume_label );
        if (!(mi->volume_label = msi_alloc( ++volumesz * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
        strcpyW( mi->volume_label, volume );

        msi_free( mi->disk_prompt );
        if (!(mi->disk_prompt = msi_alloc( ++promptsz * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
        strcpyW( mi->disk_prompt, prompt );

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
        /* cabinet is internal, no checks needed */
        if (mi->cabinet[0] == '#') return ERROR_SUCCESS;

        if (!(cabinet_file = get_cabinet_filename( mi ))) return ERROR_OUTOFMEMORY;

        /* package should be downloaded */
        if (compressed && GetFileAttributesW( cabinet_file ) == INVALID_FILE_ATTRIBUTES &&
            package->BaseURL && UrlIsW( package->BaseURL, URLIS_URL ))
        {
            WCHAR temppath[MAX_PATH], *p;

            if ((rc = msi_download_file( cabinet_file, temppath )) != ERROR_SUCCESS)
            {
                ERR("failed to download %s (%u)\n", debugstr_w(cabinet_file), rc);
                msi_free( cabinet_file );
                return rc;
            }
            if ((p = strrchrW( temppath, '\\' ))) *p = 0;
            strcpyW( mi->sourcedir, temppath );
            PathAddBackslashW( mi->sourcedir );
            msi_free( mi->cabinet );
            mi->cabinet = strdupW( p + 1 );
            msi_free( cabinet_file );
            return ERROR_SUCCESS;
        }
    }
    /* check volume matches, change media if not */
    if (mi->volume_label && mi->disk_id > 1)
    {
        WCHAR *source = msi_dup_property( package->db, szSourceDir );
        BOOL match = source_matches_volume( mi, source );
        msi_free( source );

        if (!match && (mi->type == DRIVE_CDROM || mi->type == DRIVE_REMOVABLE))
        {
            if ((rc = msi_change_media( package, mi )) != ERROR_SUCCESS)
            {
                msi_free( cabinet_file );
                return rc;
            }
        }
    }
    if (mi->cabinet)
    {
        if (compressed && GetFileAttributesW( cabinet_file ) == INVALID_FILE_ATTRIBUTES)
        {
            if ((rc = find_published_source( package, mi )) != ERROR_SUCCESS)
            {
                ERR("cabinet not found: %s\n", debugstr_w(cabinet_file));
                msi_free( cabinet_file );
                return ERROR_INSTALL_FAILURE;
            }
        }
    }
    msi_free( cabinet_file );
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
    if (!(cab = msi_alloc( sizeof(*cab) ))) return ERROR_OUTOFMEMORY;
    if (!(cab->stream = msi_alloc( (strlenW( name ) + 1) * sizeof(WCHAR ) )))
    {
        msi_free( cab );
        return ERROR_OUTOFMEMORY;
    }
    strcpyW( cab->stream, name );
    cab->disk_id = disk_id;
    cab->storage = storage;
    IStorage_AddRef( storage );
    list_add_tail( &package->cabinet_streams, &cab->entry );

    return ERROR_SUCCESS;
}
