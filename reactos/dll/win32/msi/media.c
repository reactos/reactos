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

#include "windef.h"
#include "winerror.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msipriv.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
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

static UINT msi_change_media(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    LPSTR msg;
    LPWSTR error, error_dialog;
    LPWSTR source_dir;
    UINT r = ERROR_SUCCESS;

    static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};
    static const WCHAR error_prop[] = {'E','r','r','o','r','D','i','a','l','o','g',0};

    if ((msi_get_property_int(package, szUILevel, 0) & INSTALLUILEVEL_MASK) ==
         INSTALLUILEVEL_NONE && !gUIHandlerA)
        return ERROR_SUCCESS;

    error = generate_error_string(package, 1302, 1, mi->disk_prompt);
    error_dialog = msi_dup_property(package, error_prop);
    source_dir = msi_dup_property(package, cszSourceDir);
    PathStripToRootW(source_dir);

    while (r == ERROR_SUCCESS &&
           !source_matches_volume(mi, source_dir))
    {
        r = msi_spawn_error_dialog(package, error_dialog, error);

        if (gUIHandlerA)
        {
            msg = strdupWtoA(error);
            gUIHandlerA(gUIContext, MB_RETRYCANCEL | INSTALLMESSAGE_ERROR, msg);
            msi_free(msg);
        }
    }

    msi_free(error);
    msi_free(error_dialog);
    msi_free(source_dir);

    return r;
}

static UINT writeout_cabinet_stream(MSIPACKAGE *package, LPCWSTR stream,
                                    WCHAR* source)
{
    UINT rc;
    USHORT* data;
    UINT size;
    DWORD write;
    HANDLE hfile;
    WCHAR tmp[MAX_PATH];

    static const WCHAR cszTempFolder[]= {
	'T','e','m','p','F','o','l','d','e','r',0};

    rc = read_raw_stream_data(package->db, stream, &data, &size);
    if (rc != ERROR_SUCCESS)
        return rc;

    write = MAX_PATH;
    if (MSI_GetPropertyW(package, cszTempFolder, tmp, &write))
        GetTempPathW(MAX_PATH, tmp);

    GetTempFileNameW(tmp, stream, 0, source);

    track_tempfile(package, source);
    hfile = CreateFileW(source, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfile == INVALID_HANDLE_VALUE)
    {
        ERR("Unable to create file %s\n", debugstr_w(source));
        rc = ERROR_FUNCTION_FAILED;
        goto end;
    }

    WriteFile(hfile, data, size, &write, NULL);
    CloseHandle(hfile);
    TRACE("wrote %i bytes to %s\n", write, debugstr_w(source));

end:
    msi_free(data);
    return rc;
}

static void CDECL *cabinet_alloc(ULONG cb)
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

static UINT CDECL msi_media_get_disk_info(MSIPACKAGE *package, MSIMEDIAINFO *mi)
{
    MSIRECORD *row;
    LPWSTR ptr;

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

    ptr = strrchrW(mi->source, '\\') + 1;
    lstrcpyW(ptr, mi->cabinet);
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

static INT_PTR cabinet_next_cabinet(FDINOTIFICATIONTYPE fdint,
                                    PFDINOTIFICATION pfdin)
{
    MSICABDATA *data = pfdin->pv;
    MSIMEDIAINFO *mi = data->mi;
    LPWSTR cab = strdupAtoW(pfdin->psz1);
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

    TRACE("Searching for %s\n", debugstr_w(mi->source));

    res = 0;
    if (GetFileAttributesW(mi->source) == INVALID_FILE_ATTRIBUTES)
    {
        if (msi_change_media(data->package, mi) != ERROR_SUCCESS)
            res = -1;
    }

done:
    msi_free(cab);
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
        goto done;

    TRACE("extracting %s\n", debugstr_w(path));

    attrs = attrs & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
    if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;

    handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0,
                         NULL, CREATE_ALWAYS, attrs, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
            ERR("failed to create %s (error %d)\n",
                debugstr_w(path), GetLastError());

        goto done;
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
    TRACE("(%d)\n", fdint);

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

/***********************************************************************
 *            msi_cabextract
 *
 * Extract files from a cab file.
 */
BOOL msi_cabextract(MSIPACKAGE* package, MSIMEDIAINFO *mi, LPVOID data)
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

    ret = FDICopy(hfdi, cabinet, cab_path, 0, cabinet_notify, NULL, data);
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

void msi_free_media_info(MSIMEDIAINFO *mi)
{
    msi_free(mi->disk_prompt);
    msi_free(mi->cabinet);
    msi_free(mi->volume_label);
    msi_free(mi->first_volume);
    msi_free(mi);
}

static UINT msi_load_media_info(MSIPACKAGE *package, MSIFILE *file, MSIMEDIAINFO *mi)
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
static UINT find_published_source(MSIPACKAGE *package, MSIMEDIAINFO *mi)
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

UINT ready_media(MSIPACKAGE *package, MSIFILE *file, MSIMEDIAINFO *mi)
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
        rc = find_published_source(package, mi);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Cabinet not found: %s\n", debugstr_w(mi->source));
            return ERROR_INSTALL_FAILURE;
        }
    }

    return ERROR_SUCCESS;
}
