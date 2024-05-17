/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2007 James Hawkins
 * Copyright 2015 Hans Leidekker for CodeWeavers
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

struct streams_view
{
    MSIVIEW view;
    MSIDATABASE *db;
    UINT num_cols;
};

static BOOL streams_resize_table( MSIDATABASE *db, UINT size )
{
    if (!db->num_streams_allocated)
    {
        if (!(db->streams = calloc( size, sizeof(MSISTREAM) ))) return FALSE;
        db->num_streams_allocated = size;
        return TRUE;
    }
    while (size >= db->num_streams_allocated)
    {
        MSISTREAM *tmp;
        UINT new_size = db->num_streams_allocated * 2;
        if (!(tmp = realloc( db->streams, new_size * sizeof(*tmp) ))) return FALSE;
        memset( tmp + db->num_streams_allocated, 0, (new_size - db->num_streams_allocated) * sizeof(*tmp) );
        db->streams = tmp;
        db->num_streams_allocated = new_size;
    }
    return TRUE;
}

static UINT STREAMS_fetch_int(struct tagMSIVIEW *view, UINT row, UINT col, UINT *val)
{
    struct streams_view *sv = (struct streams_view *)view;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, val);

    if (col != 1)
        return ERROR_INVALID_PARAMETER;

    if (row >= sv->db->num_streams)
        return ERROR_NO_MORE_ITEMS;

    *val = sv->db->streams[row].str_index;

    return ERROR_SUCCESS;
}

static UINT STREAMS_fetch_stream(struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    struct streams_view *sv = (struct streams_view *)view;
    LARGE_INTEGER pos;
    HRESULT hr;

    TRACE("(%p, %d, %d, %p)\n", view, row, col, stm);

    if (row >= sv->db->num_streams)
        return ERROR_FUNCTION_FAILED;

    pos.QuadPart = 0;
    hr = IStream_Seek( sv->db->streams[row].stream, pos, STREAM_SEEK_SET, NULL );
    if (FAILED( hr ))
        return ERROR_FUNCTION_FAILED;

    *stm = sv->db->streams[row].stream;
    IStream_AddRef( *stm );

    return ERROR_SUCCESS;
}

static UINT STREAMS_set_string( struct tagMSIVIEW *view, UINT row, UINT col, const WCHAR *val, int len )
{
    ERR("Cannot modify primary key.\n");
    return ERROR_FUNCTION_FAILED;
}

static UINT STREAMS_set_stream( MSIVIEW *view, UINT row, UINT col, IStream *stream )
{
    struct streams_view *sv = (struct streams_view *)view;
    IStream *prev;

    TRACE("view %p, row %u, col %u, stream %p.\n", view, row, col, stream);

    prev = sv->db->streams[row].stream;
    IStream_AddRef(sv->db->streams[row].stream = stream);
    if (prev) IStream_Release(prev);
    return ERROR_SUCCESS;
}

static UINT STREAMS_set_row(struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask)
{
    struct streams_view *sv = (struct streams_view *)view;

    TRACE("(%p, %d, %p, %08x)\n", view, row, rec, mask);

    if (row > sv->db->num_streams || mask >= (1 << sv->num_cols))
        return ERROR_INVALID_PARAMETER;

    if (mask & 1)
    {
        const WCHAR *name = MSI_RecordGetString( rec, 1 );

        if (!name) return ERROR_INVALID_PARAMETER;
        sv->db->streams[row].str_index = msi_add_string( sv->db->strings, name, -1, FALSE );
    }
    if (mask & 2)
    {
        IStream *old, *new;
        HRESULT hr;
        UINT r;

        r = MSI_RecordGetIStream( rec, 2, &new );
        if (r != ERROR_SUCCESS)
            return r;

        old = sv->db->streams[row].stream;
        hr = IStream_QueryInterface( new, &IID_IStream, (void **)&sv->db->streams[row].stream );
        IStream_Release( new );
        if (FAILED( hr ))
        {
            return ERROR_FUNCTION_FAILED;
        }
        if (old) IStream_Release( old );
    }

    return ERROR_SUCCESS;
}

static UINT streams_find_row( struct streams_view *sv, MSIRECORD *rec, UINT *row )
{
    const WCHAR *str;
    UINT r, i, id, val;

    str = MSI_RecordGetString( rec, 1 );
    r = msi_string2id( sv->db->strings, str, -1, &id );
    if (r != ERROR_SUCCESS)
        return r;

    for (i = 0; i < sv->db->num_streams; i++)
    {
        STREAMS_fetch_int( &sv->view, i, 1, &val );

        if (val == id)
        {
            if (row) *row = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT STREAMS_insert_row(struct tagMSIVIEW *view, MSIRECORD *rec, UINT row, BOOL temporary)
{
    struct streams_view *sv = (struct streams_view *)view;
    UINT i, r, num_rows = sv->db->num_streams + 1;

    TRACE("(%p, %p, %d, %d)\n", view, rec, row, temporary);

    r = streams_find_row( sv, rec, NULL );
    if (r == ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    if (!streams_resize_table( sv->db, num_rows ))
        return ERROR_FUNCTION_FAILED;

    if (row == -1)
        row = num_rows - 1;

    /* shift the rows to make room for the new row */
    for (i = num_rows - 1; i > row; i--)
    {
        sv->db->streams[i] = sv->db->streams[i - 1];
    }

    r = STREAMS_set_row( view, row, rec, (1 << sv->num_cols) - 1 );
    if (r == ERROR_SUCCESS)
        sv->db->num_streams = num_rows;

    return r;
}

static UINT STREAMS_delete_row(struct tagMSIVIEW *view, UINT row)
{
    MSIDATABASE *db = ((struct streams_view *)view)->db;
    UINT i, num_rows = db->num_streams - 1;
    const WCHAR *name;
    WCHAR *encname;
    HRESULT hr;

    TRACE("(%p %d)\n", view, row);

    if (!db->num_streams || row > num_rows)
        return ERROR_FUNCTION_FAILED;

    name = msi_string_lookup( db->strings, db->streams[row].str_index, NULL );
    if (!(encname = encode_streamname( FALSE, name ))) return ERROR_OUTOFMEMORY;
    IStream_Release( db->streams[row].stream );

    for (i = row; i < num_rows; i++)
        db->streams[i] = db->streams[i + 1];
    db->num_streams = num_rows;

    hr = IStorage_DestroyElement( db->storage, encname );
    free( encname );
    return FAILED( hr ) ? ERROR_FUNCTION_FAILED : ERROR_SUCCESS;
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
    struct streams_view *sv = (struct streams_view *)view;

    TRACE("(%p, %p, %p)\n", view, rows, cols);

    if (cols) *cols = sv->num_cols;
    if (rows) *rows = sv->db->num_streams;

    return ERROR_SUCCESS;
}

static UINT STREAMS_get_column_info( struct tagMSIVIEW *view, UINT n, LPCWSTR *name,
                                     UINT *type, BOOL *temporary, LPCWSTR *table_name )
{
    struct streams_view *sv = (struct streams_view *)view;

    TRACE("(%p, %d, %p, %p, %p, %p)\n", view, n, name, type, temporary, table_name);

    if (!n || n > sv->num_cols)
        return ERROR_INVALID_PARAMETER;

    switch (n)
    {
    case 1:
        if (name) *name = L"Name";
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MAX_STREAM_NAME_LEN;
        break;

    case 2:
        if (name) *name = L"Data";
        if (type) *type = MSITYPE_STRING | MSITYPE_VALID | MSITYPE_NULLABLE;
        break;
    }
    if (table_name) *table_name = L"_Streams";
    if (temporary) *temporary = FALSE;
    return ERROR_SUCCESS;
}

static UINT streams_modify_update(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    struct streams_view *sv = (struct streams_view *)view;
    UINT r, row;

    r = streams_find_row(sv, rec, &row);
    if (r != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    return STREAMS_set_row( view, row, rec, (1 << sv->num_cols) - 1 );
}

static UINT streams_modify_assign(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    struct streams_view *sv = (struct streams_view *)view;
    UINT r;

    r = streams_find_row( sv, rec, NULL );
    if (r == ERROR_SUCCESS)
        return streams_modify_update(view, rec);

    return STREAMS_insert_row(view, rec, -1, FALSE);
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
        r = STREAMS_insert_row(view, rec, -1, FALSE);
        break;

    case MSIMODIFY_UPDATE:
        r = streams_modify_update(view, rec);
        break;

    case MSIMODIFY_DELETE:
        r = STREAMS_delete_row(view, row - 1);
        break;

    case MSIMODIFY_VALIDATE_NEW:
    case MSIMODIFY_INSERT_TEMPORARY:
    case MSIMODIFY_REFRESH:
    case MSIMODIFY_REPLACE:
    case MSIMODIFY_MERGE:
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
    struct streams_view *sv = (struct streams_view *)view;

    TRACE("(%p)\n", view);

    free(sv);
    return ERROR_SUCCESS;
}

static const MSIVIEWOPS streams_ops =
{
    STREAMS_fetch_int,
    STREAMS_fetch_stream,
    NULL,
    STREAMS_set_string,
    STREAMS_set_stream,
    STREAMS_set_row,
    STREAMS_insert_row,
    STREAMS_delete_row,
    STREAMS_execute,
    STREAMS_close,
    STREAMS_get_dimensions,
    STREAMS_get_column_info,
    STREAMS_modify,
    STREAMS_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static HRESULT open_stream( MSIDATABASE *db, const WCHAR *name, IStream **stream )
{
    HRESULT hr;

    hr = IStorage_OpenStream( db->storage, name, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, stream );
    if (FAILED( hr ))
    {
        MSITRANSFORM *transform;

        LIST_FOR_EACH_ENTRY( transform, &db->transforms, MSITRANSFORM, entry )
        {
            hr = IStorage_OpenStream( transform->stg, name, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, stream );
            if (SUCCEEDED( hr ))
                break;
        }
    }
    return hr;
}

static MSISTREAM *find_stream( MSIDATABASE *db, const WCHAR *name )
{
    UINT r, id, i;

    r = msi_string2id( db->strings, name, -1, &id );
    if (r != ERROR_SUCCESS)
        return NULL;

    for (i = 0; i < db->num_streams; i++)
    {
        if (db->streams[i].str_index == id) return &db->streams[i];
    }
    return NULL;
}

static UINT append_stream( MSIDATABASE *db, const WCHAR *name, IStream *stream )
{
    UINT i = db->num_streams;

    if (!streams_resize_table( db, db->num_streams + 1 ))
        return ERROR_OUTOFMEMORY;

    db->streams[i].str_index = msi_add_string( db->strings, name, -1, FALSE );
    db->streams[i].stream = stream;
    db->num_streams++;

    TRACE("added %s\n", debugstr_w( name ));
    return ERROR_SUCCESS;
}

static UINT load_streams( MSIDATABASE *db )
{
    WCHAR decoded[MAX_STREAM_NAME_LEN + 1];
    IEnumSTATSTG *stgenum;
    STATSTG stat;
    HRESULT hr;
    ULONG count;
    UINT r = ERROR_SUCCESS;
    IStream *stream;

    hr = IStorage_EnumElements( db->storage, 0, NULL, 0, &stgenum );
    if (FAILED( hr ))
        return ERROR_FUNCTION_FAILED;

    for (;;)
    {
        count = 0;
        hr = IEnumSTATSTG_Next( stgenum, 1, &stat, &count );
        if (FAILED( hr ) || !count)
            break;

        /* table streams are not in the _Streams table */
        if (stat.type != STGTY_STREAM || *stat.pwcsName == 0x4840)
        {
            CoTaskMemFree( stat.pwcsName );
            continue;
        }
        decode_streamname( stat.pwcsName, decoded );
        if (find_stream( db, decoded ))
        {
            CoTaskMemFree( stat.pwcsName );
            continue;
        }
        TRACE("found new stream %s\n", debugstr_w( decoded ));

        hr = open_stream( db, stat.pwcsName, &stream );
        CoTaskMemFree( stat.pwcsName );
        if (FAILED( hr ))
        {
            ERR( "unable to open stream %#lx\n", hr );
            r = ERROR_FUNCTION_FAILED;
            break;
        }

        r = append_stream( db, decoded, stream );
        if (r != ERROR_SUCCESS)
            break;
    }

    TRACE("loaded %u streams\n", db->num_streams);
    IEnumSTATSTG_Release( stgenum );
    return r;
}

UINT msi_get_stream( MSIDATABASE *db, const WCHAR *name, IStream **ret )
{
    MSISTREAM *stream;
    WCHAR *encname;
    HRESULT hr;
    UINT r;

    if ((stream = find_stream( db, name )))
    {
        LARGE_INTEGER pos;

        pos.QuadPart = 0;
        hr = IStream_Seek( stream->stream, pos, STREAM_SEEK_SET, NULL );
        if (FAILED( hr ))
            return ERROR_FUNCTION_FAILED;

        *ret = stream->stream;
        IStream_AddRef( *ret );
        return ERROR_SUCCESS;
    }

    if (!(encname = encode_streamname( FALSE, name )))
        return ERROR_OUTOFMEMORY;

    hr = open_stream( db, encname, ret );
    free( encname );
    if (FAILED( hr ))
        return ERROR_FUNCTION_FAILED;

    r = append_stream( db, name, *ret );
    if (r != ERROR_SUCCESS)
    {
        IStream_Release( *ret );
        return r;
    }

    IStream_AddRef( *ret );
    return ERROR_SUCCESS;
}

UINT STREAMS_CreateView(MSIDATABASE *db, MSIVIEW **view)
{
    struct streams_view *sv;
    UINT r;

    TRACE("(%p, %p)\n", db, view);

    r = load_streams( db );
    if (r != ERROR_SUCCESS)
        return r;

    if (!(sv = calloc( 1, sizeof(*sv) )))
        return ERROR_OUTOFMEMORY;

    sv->view.ops = &streams_ops;
    sv->num_cols = NUM_STREAMS_COLS;
    sv->db = db;

    *view = (MSIVIEW *)sv;

    return ERROR_SUCCESS;
}

static HRESULT write_stream( IStream *dst, IStream *src )
{
    HRESULT hr;
    char buf[4096];
    STATSTG stat;
    LARGE_INTEGER pos;
    ULONG count;
    UINT size;

    hr = IStream_Stat( src, &stat, STATFLAG_NONAME );
    if (FAILED( hr )) return hr;

    hr = IStream_SetSize( dst, stat.cbSize );
    if (FAILED( hr )) return hr;

    pos.QuadPart = 0;
    hr = IStream_Seek( dst, pos, STREAM_SEEK_SET, NULL );
    if (FAILED( hr )) return hr;

    for (;;)
    {
        size = min( sizeof(buf), stat.cbSize.QuadPart );
        hr = IStream_Read( src, buf, size, &count );
        if (FAILED( hr ) || count != size)
        {
            WARN( "failed to read stream: %#lx\n", hr );
            return E_INVALIDARG;
        }
        stat.cbSize.QuadPart -= count;
        if (count)
        {
            size = count;
            hr = IStream_Write( dst, buf, size, &count );
            if (FAILED( hr ) || count != size)
            {
                WARN( "failed to write stream: %#lx\n", hr );
                return E_INVALIDARG;
            }
        }
        if (!stat.cbSize.QuadPart) break;
    }

    return S_OK;
}

UINT msi_commit_streams( MSIDATABASE *db )
{
    UINT i;
    const WCHAR *name;
    WCHAR *encname;
    IStream *stream;
    HRESULT hr;

    TRACE("got %u streams\n", db->num_streams);

    for (i = 0; i < db->num_streams; i++)
    {
        name = msi_string_lookup( db->strings, db->streams[i].str_index, NULL );
        if (!wcscmp( name, L"\5SummaryInformation" )) continue;

        if (!(encname = encode_streamname( FALSE, name ))) return ERROR_OUTOFMEMORY;
        TRACE("saving stream %s as %s\n", debugstr_w(name), debugstr_w(encname));

        hr = IStorage_CreateStream( db->storage, encname, STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &stream );
        if (SUCCEEDED( hr ))
        {
            hr = write_stream( stream, db->streams[i].stream );
            if (FAILED( hr ))
            {
                ERR( "failed to write stream %s (hr = %#lx)\n", debugstr_w(encname), hr );
                free( encname );
                IStream_Release( stream );
                return ERROR_FUNCTION_FAILED;
            }
            hr = IStream_Commit( stream, 0 );
            IStream_Release( stream );
            if (FAILED( hr ))
            {
                ERR( "failed to commit stream %s (hr = %#lx)\n", debugstr_w(encname), hr );
                free( encname );
                return ERROR_FUNCTION_FAILED;
            }
        }
        else if (hr != STG_E_FILEALREADYEXISTS)
        {
            ERR( "failed to create stream %s (hr = %#lx)\n", debugstr_w(encname), hr );
            free( encname );
            return ERROR_FUNCTION_FAILED;
        }
        free( encname );
    }

    return ERROR_SUCCESS;
}
