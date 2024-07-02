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
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "ole2.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "msipriv.h"
#include "query.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define NUM_STORAGES_COLS    2
#define MAX_STORAGES_NAME_LEN 62

struct storage
{
    UINT str_index;
    IStorage *storage;
} STORAGE;

struct storages_view
{
    MSIVIEW view;
    MSIDATABASE *db;
    struct storage *storages;
    UINT max_storages;
    UINT num_rows;
    UINT row_size;
};

static BOOL storages_set_table_size(struct storages_view *sv, UINT size)
{
    if (size >= sv->max_storages)
    {
        sv->max_storages *= 2;
        sv->storages = realloc(sv->storages, sv->max_storages * sizeof(*sv->storages));
        if (!sv->storages)
            return FALSE;
    }

    return TRUE;
}

static UINT STORAGES_fetch_int(struct tagMSIVIEW *view, UINT row, UINT col, UINT *val)
{
    struct storages_view *sv = (struct storages_view *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, val);

    if (col != 1)
        return ERROR_INVALID_PARAMETER;

    if (row >= sv->num_rows)
        return ERROR_NO_MORE_ITEMS;

    *val = sv->storages[row].str_index;

    return ERROR_SUCCESS;
}

static UINT STORAGES_fetch_stream(struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    struct storages_view *sv = (struct storages_view *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, stm);

    if (row >= sv->num_rows)
        return ERROR_FUNCTION_FAILED;

    return ERROR_INVALID_DATA;
}

static UINT STORAGES_set_string( struct tagMSIVIEW *view, UINT row, UINT col, const WCHAR *val, int len )
{
    ERR("Cannot modify primary key.\n");
    return ERROR_FUNCTION_FAILED;
}

static HRESULT stream_to_storage(IStream *stm, IStorage **stg)
{
    ILockBytes *lockbytes = NULL;
    STATSTG stat;
    LPVOID data;
    HRESULT hr;
    DWORD size, read;
    ULARGE_INTEGER offset;

    hr = IStream_Stat(stm, &stat, STATFLAG_NONAME);
    if (FAILED(hr))
        return hr;

    if (stat.cbSize.QuadPart >> 32)
    {
        ERR("Storage is too large\n");
        return E_FAIL;
    }

    size = stat.cbSize.QuadPart;
    data = malloc(size);
    if (!data)
        return E_OUTOFMEMORY;

    hr = IStream_Read(stm, data, size, &read);
    if (FAILED(hr) || read != size)
        goto done;

    hr = CreateILockBytesOnHGlobal(NULL, TRUE, &lockbytes);
    if (FAILED(hr))
        goto done;

    ZeroMemory(&offset, sizeof(ULARGE_INTEGER));
    hr = ILockBytes_WriteAt(lockbytes, offset, data, size, &read);
    if (FAILED(hr) || read != size)
        goto done;

    hr = StgOpenStorageOnILockBytes(lockbytes, NULL,
                                    STGM_READWRITE | STGM_SHARE_DENY_NONE,
                                    NULL, 0, stg);
    if (FAILED(hr))
        goto done;

done:
    free(data);
    if (lockbytes) ILockBytes_Release(lockbytes);
    return hr;
}

static UINT STORAGES_set_stream( MSIVIEW *view, UINT row, UINT col, IStream *stream )
{
    struct storages_view *sv = (struct storages_view *)view;
    IStorage *stg, *substg, *prev;
    const WCHAR *name;
    HRESULT hr;
    UINT r;

    TRACE("view %p, row %u, col %u, stream %p.\n", view, row, col, stream);

    if ((r = stream_to_storage(stream, &stg)))
        return r;

    name = msi_string_lookup(sv->db->strings, sv->storages[row].str_index, NULL);

    hr = IStorage_CreateStorage(sv->db->storage, name,
                                STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                                0, 0, &substg);
    if (FAILED(hr))
    {
        IStorage_Release(stg);
        return ERROR_FUNCTION_FAILED;
    }

    hr = IStorage_CopyTo(stg, 0, NULL, NULL, substg);
    if (FAILED(hr))
    {
        IStorage_Release(substg);
        IStorage_Release(stg);
        return ERROR_FUNCTION_FAILED;
    }
    IStorage_Release(substg);

    prev = sv->storages[row].storage;
    sv->storages[row].storage = stg;
    if (prev) IStorage_Release(prev);

    return ERROR_SUCCESS;
}

static UINT STORAGES_set_row(struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask)
{
    struct storages_view *sv = (struct storages_view *)view;
    IStorage *stg, *substg = NULL, *prev;
    IStream *stm;
    LPWSTR name = NULL;
    HRESULT hr;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("(%p, %p)\n", view, rec);

    if (row >= sv->num_rows)
        return ERROR_FUNCTION_FAILED;

    r = MSI_RecordGetIStream(rec, 2, &stm);
    if (r != ERROR_SUCCESS)
        return r;

    r = stream_to_storage(stm, &stg);
    if (r != ERROR_SUCCESS)
    {
        IStream_Release(stm);
        return r;
    }

    name = wcsdup(MSI_RecordGetString(rec, 1));
    if (!name)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    hr = IStorage_CreateStorage(sv->db->storage, name,
                                STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                                0, 0, &substg);
    if (FAILED(hr))
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    hr = IStorage_CopyTo(stg, 0, NULL, NULL, substg);
    if (FAILED(hr))
    {
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    prev = sv->storages[row].storage;
    sv->storages[row].str_index = msi_add_string(sv->db->strings, name, -1, FALSE);
    IStorage_AddRef(stg);
    sv->storages[row].storage = stg;
    if (prev) IStorage_Release(prev);

done:
    free(name);

    if (substg) IStorage_Release(substg);
    IStorage_Release(stg);
    IStream_Release(stm);

    return r;
}

static UINT STORAGES_insert_row(struct tagMSIVIEW *view, MSIRECORD *rec, UINT row, BOOL temporary)
{
    struct storages_view *sv = (struct storages_view *)view;

    if (!storages_set_table_size(sv, ++sv->num_rows))
        return ERROR_FUNCTION_FAILED;

    if (row == -1)
        row = sv->num_rows - 1;

    memset(&sv->storages[row], 0, sizeof(sv->storages[row]));

    /* FIXME have to readjust rows */

    return STORAGES_set_row(view, row, rec, 0);
}

static UINT STORAGES_delete_row(struct tagMSIVIEW *view, UINT row)
{
    FIXME("(%p %d): stub!\n", view, row);
    return ERROR_SUCCESS;
}

static UINT STORAGES_execute(struct tagMSIVIEW *view, MSIRECORD *record)
{
    TRACE("(%p, %p)\n", view, record);
    return ERROR_SUCCESS;
}

static UINT STORAGES_close(struct tagMSIVIEW *view)
{
    TRACE("(%p)\n", view);
    return ERROR_SUCCESS;
}

static UINT STORAGES_get_dimensions(struct tagMSIVIEW *view, UINT *rows, UINT *cols)
{
    struct storages_view *sv = (struct storages_view *)view;

    TRACE("(%p, %p, %p)\n", view, rows, cols);

    if (cols) *cols = NUM_STORAGES_COLS;
    if (rows) *rows = sv->num_rows;

    return ERROR_SUCCESS;
}

static UINT STORAGES_get_column_info( struct tagMSIVIEW *view, UINT n, LPCWSTR *name,
                                      UINT *type, BOOL *temporary, LPCWSTR *table_name )
{
    TRACE("(%p, %d, %p, %p, %p, %p)\n", view, n, name, type, temporary,
          table_name);

    if (n == 0 || n > NUM_STORAGES_COLS)
        return ERROR_INVALID_PARAMETER;

    switch (n)
    {
    case 1:
        if (name) *name = L"Name";
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MAX_STORAGES_NAME_LEN;
        break;

    case 2:
        if (name) *name = L"Data";
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MSITYPE_NULLABLE;
        break;
    }
    if (table_name) *table_name = L"_Storages";
    if (temporary) *temporary = FALSE;
    return ERROR_SUCCESS;
}

static UINT storages_find_row(struct storages_view *sv, MSIRECORD *rec, UINT *row)
{
    LPCWSTR str;
    UINT r, i, id, data;

    str = MSI_RecordGetString(rec, 1);
    r = msi_string2id(sv->db->strings, str, -1, &id);
    if (r != ERROR_SUCCESS)
        return r;

    for (i = 0; i < sv->num_rows; i++)
    {
        STORAGES_fetch_int(&sv->view, i, 1, &data);

        if (data == id)
        {
            *row = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT storages_modify_update(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    struct storages_view *sv = (struct storages_view *)view;
    UINT r, row;

    r = storages_find_row(sv, rec, &row);
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    return STORAGES_set_row(view, row, rec, 0);
}

static UINT storages_modify_assign(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    struct storages_view *sv = (struct storages_view *)view;
    UINT r, row;

    r = storages_find_row(sv, rec, &row);
    if (r == ERROR_SUCCESS)
        return storages_modify_update(view, rec);

    return STORAGES_insert_row(view, rec, -1, FALSE);
}

static UINT STORAGES_modify(struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIRECORD *rec, UINT row)
{
    UINT r;

    TRACE("%p %d %p\n", view, eModifyMode, rec);

    switch (eModifyMode)
    {
    case MSIMODIFY_ASSIGN:
        r = storages_modify_assign(view, rec);
        break;

    case MSIMODIFY_INSERT:
        r = STORAGES_insert_row(view, rec, -1, FALSE);
        break;

    case MSIMODIFY_UPDATE:
        r = storages_modify_update(view, rec);
        break;

    case MSIMODIFY_VALIDATE_NEW:
    case MSIMODIFY_INSERT_TEMPORARY:
    case MSIMODIFY_REFRESH:
    case MSIMODIFY_REPLACE:
    case MSIMODIFY_MERGE:
    case MSIMODIFY_DELETE:
    case MSIMODIFY_VALIDATE:
    case MSIMODIFY_VALIDATE_FIELD:
    case MSIMODIFY_VALIDATE_DELETE:
        FIXME("%p %d %p - mode not implemented\n", view, eModifyMode, rec );
        r = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    default:
        r = ERROR_INVALID_DATA;
    }

    return r;
}

static UINT STORAGES_delete(struct tagMSIVIEW *view)
{
    struct storages_view *sv = (struct storages_view *)view;
    UINT i;

    TRACE("(%p)\n", view);

    for (i = 0; i < sv->num_rows; i++)
    {
        if (sv->storages[i].storage)
            IStorage_Release(sv->storages[i].storage);
    }

    free(sv->storages);
    sv->storages = NULL;
    free(sv);

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS storages_ops =
{
    STORAGES_fetch_int,
    STORAGES_fetch_stream,
    NULL,
    STORAGES_set_string,
    STORAGES_set_stream,
    STORAGES_set_row,
    STORAGES_insert_row,
    STORAGES_delete_row,
    STORAGES_execute,
    STORAGES_close,
    STORAGES_get_dimensions,
    STORAGES_get_column_info,
    STORAGES_modify,
    STORAGES_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static INT add_storages_to_table(struct storages_view *sv)
{
    IEnumSTATSTG *stgenum = NULL;
    STATSTG stat;
    HRESULT hr;
    UINT count = 0;
    ULONG size;

    hr = IStorage_EnumElements(sv->db->storage, 0, NULL, 0, &stgenum);
    if (FAILED(hr))
        return -1;

    sv->max_storages = 1;
    sv->storages = malloc(sizeof(*sv->storages));
    if (!sv->storages)
        return -1;

    while (TRUE)
    {
        size = 0;
        hr = IEnumSTATSTG_Next(stgenum, 1, &stat, &size);
        if (FAILED(hr) || !size)
            break;

        if (stat.type != STGTY_STORAGE)
        {
            CoTaskMemFree(stat.pwcsName);
            continue;
        }

        TRACE("enumerated storage %s\n", debugstr_w(stat.pwcsName));

        if (!storages_set_table_size(sv, ++count))
        {
            count = -1;
            break;
        }

        sv->storages[count - 1].str_index = msi_add_string(sv->db->strings, stat.pwcsName, -1, FALSE);
        sv->storages[count - 1].storage = NULL;

        IStorage_OpenStorage(sv->db->storage, stat.pwcsName, NULL,
                             STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0,
                             &sv->storages[count - 1].storage);
        CoTaskMemFree(stat.pwcsName);
    }

    IEnumSTATSTG_Release(stgenum);
    return count;
}

UINT STORAGES_CreateView(MSIDATABASE *db, MSIVIEW **view)
{
    struct storages_view *sv;
    INT rows;

    TRACE("(%p, %p)\n", db, view);

    sv = calloc(1, sizeof(*sv));
    if (!sv)
        return ERROR_FUNCTION_FAILED;

    sv->view.ops = &storages_ops;
    sv->db = db;

    rows = add_storages_to_table(sv);
    if (rows < 0)
    {
        free(sv);
        return ERROR_FUNCTION_FAILED;
    }
    sv->num_rows = rows;

    *view = (MSIVIEW *)sv;

    return ERROR_SUCCESS;
}
