/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2007 James Hawkins
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
#include "winerror.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "msipriv.h"
#include "query.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define NUM_STREAMS_COLS    2
#define MAX_STREAM_NAME_LEN 62

typedef struct tabSTREAM
{
    UINT str_index;
    LPWSTR name;
    IStream *stream;
} STREAM;

typedef struct tagMSISTREAMSVIEW
{
    MSIVIEW view;
    MSIDATABASE *db;
    STREAM **streams;
    UINT max_streams;
    UINT num_rows;
    UINT row_size;
} MSISTREAMSVIEW;

static BOOL streams_set_table_size(MSISTREAMSVIEW *sv, UINT size)
{
    if (size >= sv->max_streams)
    {
        sv->max_streams *= 2;
        sv->streams = msi_realloc(sv->streams, sv->max_streams * sizeof(STREAM *));
        if (!sv->streams)
            return FALSE;
    }

    return TRUE;
}

static STREAM *create_stream(MSISTREAMSVIEW *sv, LPWSTR name, BOOL encoded, IStream *stm)
{
    STREAM *stream;
    WCHAR decoded[MAX_STREAM_NAME_LEN];
    LPWSTR ptr = name;

    stream = msi_alloc(sizeof(STREAM));
    if (!stream)
        return NULL;

    if (encoded)
    {
        decode_streamname(name, decoded);
        ptr = decoded;
        TRACE("stream -> %s %s\n", debugstr_w(name), debugstr_w(decoded));
    }

    stream->name = strdupW(ptr);
    if (!stream->name)
    {
        msi_free(stream);
        return NULL;
    }

    stream->str_index = msi_addstringW(sv->db->strings, 0, stream->name, -1, 1, StringNonPersistent);
    stream->stream = stm;
    return stream;
}

static UINT STREAMS_fetch_int(struct tagMSIVIEW *view, UINT row, UINT col, UINT *val)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, val);

    if (col != 1)
        return ERROR_INVALID_PARAMETER;

    if (row >= sv->num_rows)
        return ERROR_NO_MORE_ITEMS;

    *val = sv->streams[row]->str_index;

    return ERROR_SUCCESS;
}

static UINT STREAMS_fetch_stream(struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, stm);

    if (row >= sv->num_rows)
        return ERROR_FUNCTION_FAILED;

    IStream_AddRef(sv->streams[row]->stream);
    *stm = sv->streams[row]->stream;

    return ERROR_SUCCESS;
}

static UINT STREAMS_get_row( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec )
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;

    FIXME("%p %d %p\n", sv, row, rec);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

static UINT STREAMS_set_row(struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;
    STREAM *stream;
    IStream *stm;
    STATSTG stat;
    LPWSTR name = NULL;
    USHORT *data = NULL;
    HRESULT hr;
    ULONG count;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("(%p, %p)\n", view, rec);

    if (row > sv->num_rows)
        return ERROR_FUNCTION_FAILED;

    r = MSI_RecordGetIStream(rec, 2, &stm);
    if (r != ERROR_SUCCESS)
        return r;

    hr = IStream_Stat(stm, &stat, STATFLAG_NONAME);
    if (FAILED(hr))
    {
        WARN("failed to stat stream: %08x\n", hr);
        goto done;
    }

    if (stat.cbSize.QuadPart >> 32)
        goto done;

    data = msi_alloc(stat.cbSize.QuadPart);
    if (!data)
        goto done;

    hr = IStream_Read(stm, data, stat.cbSize.QuadPart, &count);
    if (FAILED(hr) || count != stat.cbSize.QuadPart)
    {
        WARN("failed to read stream: %08x\n", hr);
        goto done;
    }

    name = strdupW(MSI_RecordGetString(rec, 1));
    if (!name)
        goto done;

    r = write_stream_data(sv->db->storage, name, data, count, FALSE);
    if (r != ERROR_SUCCESS)
    {
        WARN("failed to write stream data: %d\n", r);
        goto done;
    }

    stream = create_stream(sv, name, FALSE, NULL);
    if (!stream)
        goto done;

    IStorage_OpenStream(sv->db->storage, name, 0,
                        STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream->stream);

    sv->streams[row] = stream;

done:
    msi_free(name);
    msi_free(data);

    IStream_Release(stm);

    return r;
}

static UINT STREAMS_insert_row(struct tagMSIVIEW *view, MSIRECORD *rec, BOOL temporary)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;

    if (!streams_set_table_size(sv, ++sv->num_rows))
        return ERROR_FUNCTION_FAILED;

    return STREAMS_set_row(view, sv->num_rows - 1, rec, 0);
}

static UINT STREAMS_delete_row(struct tagMSIVIEW *view, UINT row)
{
    FIXME("(%p %d): stub!\n", view, row);
    return ERROR_SUCCESS;
}

static UINT STREAMS_execute(struct tagMSIVIEW *view, MSIRECORD *record)
{
    TRACE("(%p, %p)\n", view, record);
    return ERROR_SUCCESS;
}

static UINT STREAMS_close(struct tagMSIVIEW *view)
{
    TRACE("(%p)\n", view);
    return ERROR_SUCCESS;
}

static UINT STREAMS_get_dimensions(struct tagMSIVIEW *view, UINT *rows, UINT *cols)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;

    TRACE("(%p, %p, %p)\n", view, rows, cols);

    if (cols) *cols = NUM_STREAMS_COLS;
    if (rows) *rows = sv->num_rows;

    return ERROR_SUCCESS;
}

static UINT STREAMS_get_column_info(struct tagMSIVIEW *view,
                                    UINT n, LPWSTR *name, UINT *type)
{
    LPCWSTR name_ptr = NULL;

    static const WCHAR Name[] = {'N','a','m','e',0};
    static const WCHAR Data[] = {'D','a','t','a',0};

    TRACE("(%p, %d, %p, %p)\n", view, n, name, type);

    if (n == 0 || n > NUM_STREAMS_COLS)
        return ERROR_INVALID_PARAMETER;

    switch (n)
    {
    case 1:
        name_ptr = Name;
        if (type) *type = MSITYPE_STRING | MAX_STREAM_NAME_LEN;
        break;

    case 2:
        name_ptr = Data;
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MSITYPE_NULLABLE;
        break;
    }

    if (name)
    {
        *name = strdupW(name_ptr);
        if (!*name) return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT streams_find_row(MSISTREAMSVIEW *sv, MSIRECORD *rec, UINT *row)
{
    LPCWSTR str;
    UINT i, id, data;

    str = MSI_RecordGetString(rec, 1);
    msi_string2idW(sv->db->strings, str, &id);

    for (i = 0; i < sv->num_rows; i++)
    {
        STREAMS_fetch_int(&sv->view, i, 1, &data);

        if (data == id)
        {
            *row = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT streams_modify_update(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;
    UINT r, row;

    r = streams_find_row(sv, rec, &row);
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    return STREAMS_set_row(view, row, rec, 0);
}

static UINT streams_modify_assign(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;
    UINT r, row;

    r = streams_find_row(sv, rec, &row);
    if (r == ERROR_SUCCESS)
        return streams_modify_update(view, rec);

    return STREAMS_insert_row(view, rec, FALSE);
}

static UINT STREAMS_modify(struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIRECORD *rec, UINT row)
{
    UINT r;

    TRACE("%p %d %p\n", view, eModifyMode, rec);

    switch (eModifyMode)
    {
    case MSIMODIFY_ASSIGN:
        r = streams_modify_assign(view, rec);
        break;

    case MSIMODIFY_INSERT:
        r = STREAMS_insert_row(view, rec, FALSE);
        break;

    case MSIMODIFY_UPDATE:
        r = streams_modify_update(view, rec);
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

static UINT STREAMS_delete(struct tagMSIVIEW *view)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;
    UINT i;

    TRACE("(%p)\n", view);

    for (i = 0; i < sv->num_rows; i++)
    {
        if (sv->streams[i]->stream)
            IStream_Release(sv->streams[i]->stream);
        msi_free(sv->streams[i]);
    }

    msi_free(sv->streams);

    return ERROR_SUCCESS;
}

static UINT STREAMS_find_matching_rows(struct tagMSIVIEW *view, UINT col,
                                       UINT val, UINT *row, MSIITERHANDLE *handle)
{
    MSISTREAMSVIEW *sv = (MSISTREAMSVIEW *)view;
    UINT index = (UINT)*handle;

    TRACE("(%d, %d): %d\n", *row, col, val);

    if (col == 0 || col > NUM_STREAMS_COLS)
        return ERROR_INVALID_PARAMETER;

    while (index < sv->num_rows)
    {
        if (sv->streams[index]->str_index == val)
        {
            *row = index;
            break;
        }

        index++;
    }

    *handle = (MSIITERHANDLE)++index;
    if (index >= sv->num_rows)
        return ERROR_NO_MORE_ITEMS;

    return ERROR_SUCCESS;
}

static const MSIVIEWOPS streams_ops =
{
    STREAMS_fetch_int,
    STREAMS_fetch_stream,
    STREAMS_get_row,
    STREAMS_set_row,
    STREAMS_insert_row,
    STREAMS_delete_row,
    STREAMS_execute,
    STREAMS_close,
    STREAMS_get_dimensions,
    STREAMS_get_column_info,
    STREAMS_modify,
    STREAMS_delete,
    STREAMS_find_matching_rows,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static INT add_streams_to_table(MSISTREAMSVIEW *sv)
{
    IEnumSTATSTG *stgenum = NULL;
    STATSTG stat;
    STREAM *stream = NULL;
    HRESULT hr;
    UINT count = 0, size;

    hr = IStorage_EnumElements(sv->db->storage, 0, NULL, 0, &stgenum);
    if (FAILED(hr))
        return -1;

    sv->max_streams = 1;
    sv->streams = msi_alloc(sizeof(STREAM *));
    if (!sv->streams)
        return -1;

    while (TRUE)
    {
        size = 0;
        hr = IEnumSTATSTG_Next(stgenum, 1, &stat, &size);
        if (FAILED(hr) || !size)
            break;

        /* table streams are not in the _Streams table */
        if (*stat.pwcsName == 0x4840)
        {
            CoTaskMemFree(stat.pwcsName);
            continue;
        }

        stream = create_stream(sv, stat.pwcsName, TRUE, NULL);
        if (!stream)
        {
            count = -1;
            CoTaskMemFree(stat.pwcsName);
            break;
        }

        IStorage_OpenStream(sv->db->storage, stat.pwcsName, 0,
                            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream->stream);
        CoTaskMemFree(stat.pwcsName);

        if (!streams_set_table_size(sv, ++count))
        {
            count = -1;
            break;
        }

        sv->streams[count - 1] = stream;
    }

    IEnumSTATSTG_Release(stgenum);
    return count;
}

UINT STREAMS_CreateView(MSIDATABASE *db, MSIVIEW **view)
{
    MSISTREAMSVIEW *sv;
    INT rows;

    TRACE("(%p, %p)\n", db, view);

    sv = msi_alloc(sizeof(MSISTREAMSVIEW));
    if (!sv)
        return ERROR_FUNCTION_FAILED;

    sv->view.ops = &streams_ops;
    sv->db = db;
    rows = add_streams_to_table(sv);
    if (rows < 0)
        return ERROR_FUNCTION_FAILED;
    sv->num_rows = rows;

    *view = (MSIVIEW *)sv;

    return ERROR_SUCCESS;
}
