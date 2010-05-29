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
#include "wine/unicode.h"

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
    WCHAR volume_name[MAX_PATH + 1];
    WCHAR root[MAX_PATH + 1];

    strcpyW(root, source_root);
    PathStripToRootW(root);
    PathAddBackslashW(root);

    if (!GetVolumeInformationW(root, volume_name, MAX_PATH + 1,
                               NULL, NULL, NULL, NULL, 0))
    {
        ERR("Failed to get volume information\n");
        return FALSE;
    }

    return !lstrcmpW(mi->volume_label, volume_name);
}

static UINT msi_change_media(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    LPWSTR error, error_dialog;
    LPWSTR source_dir;
    UINT r = ERROR_SUCCESS;

    static const WCHAR error_prop[] = {'E','r','r','o','r','D','i','a','l','o','g',0};

    if ((msi_get_property_int(package->db, szUILevel, 0) & INSTALLUILEVEL_MASK) ==
         INSTALLUILEVEL_NONE && !gUIHandlerA && !gUIHandlerW && !gUIHandlerRecord)
        return ERROR_SUCCESS;

    error = generate_error_string(package, 1302, 1, mi->disk_prompt);
    error_dialog = msi_dup_property(package->db, error_prop);
    source_dir = msi_dup_property(package->db, cszSourceDir);

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

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return 0;

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

struct cab_stream
{
    MSIDATABASE *db;
    WCHAR       *name;
};

static struct cab_stream cab_stream;

static INT_PTR CDECL cabinet_open_stream( char *pszFile, int oflag, int pmode )
{
    UINT r;
    IStream *stm;

    if (oflag)
        WARN("ignoring open flags 0x%08x\n", oflag);

    r = db_get_raw_stream( cab_stream.db, cab_stream.name, &stm );
    if (r != ERROR_SUCCESS)
    {
        WARN("Failed to get cabinet stream %u\n", r);
        return 0;
    }

    return (INT_PTR)stm;
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

    if (!mi->first_volume)
        mi->first_volume = strdupW(mi->volume_label);

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

    if (lstrcmpiW(mi->cabinet, cab))
    {
        ERR("Continuous cabinet does not match the next cabinet in the Media table\n");
        goto done;
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

    TRACE("extracting %s\n", debugstr_w(path));

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
            handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, attrs2, NULL);

            if (handle != INVALID_HANDLE_VALUE) goto done;
            err = GetLastError();
        }
        if (err == ERROR_SHARING_VIOLATION || err == ERROR_USER_MAPPED_FILE)
        {
            WCHAR tmpfileW[MAX_PATH], *tmppathW, *p;
            DWORD len;

            TRACE("file in use, scheduling rename operation\n");

            GetTempFileNameW(szBackSlash, szMsi, 0, tmpfileW);
            len = strlenW(path) + strlenW(tmpfileW) + 1;
            if (!(tmppathW = msi_alloc(len * sizeof(WCHAR))))
                return ERROR_OUTOFMEMORY;

            strcpyW(tmppathW, path);
            if ((p = strrchrW(tmppathW, '\\'))) *p = 0;
            strcatW(tmppathW, tmpfileW);

            handle = CreateFileW(tmppathW, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, attrs, NULL);

            if (handle != INVALID_HANDLE_VALUE &&
                MoveFileExW(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT) &&
                MoveFileExW(tmppathW, path, MOVEFILE_DELAY_UNTIL_REBOOT))
            {
                data->package->need_reboot = 1;
            }
            else
                WARN("failed to schedule rename operation %s (error %d)\n", debugstr_w(path), GetLastError());

            msi_free(tmppathW);
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

    TRACE("Extracting %s\n", debugstr_w(mi->cabinet));

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

    TRACE("Extracting %s\n", debugstr_w(mi->cabinet));

    hfdi = FDICreate( cabinet_alloc, cabinet_free, cabinet_open_stream, cabinet_read_stream,
                      cabinet_write, cabinet_close_stream, cabinet_seek_stream, 0, &erf );
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    cab_stream.db = package->db;
    cab_stream.name = encode_streamname( FALSE, mi->cabinet + 1 );
    if (!cab_stream.name)
        goto done;

    ret = FDICopy( hfdi, filename, NULL, 0, cabinet_notify_stream, NULL, data );
    if (!ret)
        ERR("FDICopy failed\n");

done:
    FDIDestroy( hfdi );
    msi_free( cab_stream.name );

    if (ret)
        mi->is_extracted = TRUE;

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
    msi_free(mi->first_volume);
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

static UINT msi_load_media_info(MSIPACKAGE *package, MSIFILE *file, MSIMEDIAINFO *mi)
{
    MSIRECORD *row;
    LPWSTR source_dir;
    LPWSTR source;
    DWORD options;

    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ', 'F','R','O','M',' ',
        '`','M','e','d','i','a','`',' ','W','H','E','R','E',' ',
        '`','L','a','s','t','S','e','q','u','e','n','c','e','`',' ','>','=',
        ' ','%','i',' ','A','N','D',' ','`','D','i','s','k','I','d','`',' ','>','=',
        ' ','%','i',' ','O','R','D','E','R',' ','B','Y',' ',
        '`','D','i','s','k','I','d','`',0};

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

    source_dir = msi_dup_property(package->db, cszSourceDir);
    lstrcpyW(mi->sourcedir, source_dir);
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

    index = 0;
    volumesz = MAX_PATH;
    promptsz = MAX_PATH;

    if (last_type[0] == 'n')
    {
        while (MsiSourceListEnumSourcesW(package->ProductCode, NULL,
                                        package->Context,
                                        MSISOURCETYPE_NETWORK, index++,
                                        volume, &volumesz) == ERROR_SUCCESS)
        {
            if (!strncmpiW(source, volume, strlenW(source)))
            {
                lstrcpyW(mi->sourcedir, source);
                TRACE("Found network source %s\n", debugstr_w(mi->sourcedir));
                return ERROR_SUCCESS;
            }
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
        mi->volume_label = msi_realloc(mi->volume_label, ++volumesz * sizeof(WCHAR));
        lstrcpyW(mi->volume_label, volume);
        mi->disk_prompt = msi_realloc(mi->disk_prompt, ++promptsz * sizeof(WCHAR));
        lstrcpyW(mi->disk_prompt, prompt);

        if (source_matches_volume(mi, source))
        {
            /* FIXME: what about SourceDir */
            lstrcpyW(mi->sourcedir, source);
            TRACE("Found disk source %s\n", debugstr_w(mi->sourcedir));
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

UINT ready_media(MSIPACKAGE *package, MSIFILE *file, MSIMEDIAINFO *mi)
{
    UINT rc = ERROR_SUCCESS;
    WCHAR *cabinet_file;

    /* media info for continuous cabinet is already loaded */
    if (mi->is_continuous)
        return ERROR_SUCCESS;

    rc = msi_load_media_info(package, file, mi);
    if (rc != ERROR_SUCCESS)
    {
        ERR("Unable to load media info %u\n", rc);
        return ERROR_FUNCTION_FAILED;
    }

    /* cabinet is internal, no checks needed */
    if (!mi->cabinet || mi->cabinet[0] == '#')
        return ERROR_SUCCESS;

    cabinet_file = get_cabinet_filename(mi);

    /* package should be downloaded */
    if (file->IsCompressed &&
        GetFileAttributesW(cabinet_file) == INVALID_FILE_ATTRIBUTES &&
        package->BaseURL && UrlIsW(package->BaseURL, URLIS_URL))
    {
        WCHAR temppath[MAX_PATH], *p;

        msi_download_file(cabinet_file, temppath);
        if ((p = strrchrW(temppath, '\\'))) *p = 0;

        msi_free(mi->sourcedir);
        strcpyW(mi->sourcedir, temppath);
        msi_free(mi->cabinet);
        strcpyW(mi->cabinet, p + 1);

        msi_free(cabinet_file);
        return ERROR_SUCCESS;
    }

    /* check volume matches, change media if not */
    if (mi->volume_label && mi->disk_id > 1 &&
        lstrcmpW(mi->first_volume, mi->volume_label))
    {
        LPWSTR source = msi_dup_property(package->db, cszSourceDir);
        BOOL matches;

        matches = source_matches_volume(mi, source);
        msi_free(source);

        if ((mi->type == DRIVE_CDROM || mi->type == DRIVE_REMOVABLE) && !matches)
        {
            rc = msi_change_media(package, mi);
            if (rc != ERROR_SUCCESS)
            {
                msi_free(cabinet_file);
                return rc;
            }
        }
    }

    if (file->IsCompressed &&
        GetFileAttributesW(cabinet_file) == INVALID_FILE_ATTRIBUTES)
    {
        rc = find_published_source(package, mi);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Cabinet not found: %s\n", debugstr_w(cabinet_file));
            msi_free(cabinet_file);
            return ERROR_INSTALL_FAILURE;
        }
    }

    msi_free(cabinet_file);
    return ERROR_SUCCESS;
}
