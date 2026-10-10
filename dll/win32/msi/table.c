/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2005 Mike McCormack for CodeWeavers
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
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "winnls.h"
#include "msipriv.h"
#include "query.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define MSITABLE_HASH_TABLE_SIZE 37

struct column_hash_entry
{
    struct column_hash_entry *next;
    UINT value;
    UINT row;
};

struct column_info
{
    LPCWSTR tablename;
    UINT    number;
    LPCWSTR colname;
    UINT    type;
    UINT    offset;
    struct column_hash_entry **hash_table;
};

struct tagMSITABLE
{
    BYTE **data;
    BOOL *data_persistent;
    UINT row_count;
    struct list entry;
    struct column_info *colinfo;
    UINT col_count;
    MSICONDITION persistent;
    LONG ref_count;
    WCHAR name[1];
};

/* information for default tables */
static const struct column_info _Columns_cols[4] =
{
    { L"_Columns", 1, L"Table",  MSITYPE_VALID | MSITYPE_STRING | MSITYPE_KEY | 64, 0, NULL },
    { L"_Columns", 2, L"Number", MSITYPE_VALID | MSITYPE_KEY | 2,     2, NULL },
    { L"_Columns", 3, L"Name",   MSITYPE_VALID | MSITYPE_STRING | 64, 4, NULL },
    { L"_Columns", 4, L"Type",   MSITYPE_VALID | 2,                   6, NULL },
};

static const struct column_info _Tables_cols[1] =
{
    { L"_Tables",  1, L"Name",   MSITYPE_VALID | MSITYPE_STRING | MSITYPE_KEY | 64, 0, NULL },
};

#define MAX_STREAM_NAME 0x1f

static inline UINT bytes_per_column( MSIDATABASE *db, const struct column_info *col, UINT bytes_per_strref )
{
    if( MSITYPE_IS_BINARY(col->type) )
        return 2;

    if( col->type & MSITYPE_STRING )
        return bytes_per_strref;

    if( (col->type & 0xff) <= 2)
        return 2;

    if( (col->type & 0xff) != 4 )
        ERR("Invalid column size %u\n", col->type & 0xff);

    return 4;
}

static int utf2mime(int x)
{
    if( (x>='0') && (x<='9') )
        return x-'0';
    if( (x>='A') && (x<='Z') )
        return x-'A'+10;
    if( (x>='a') && (x<='z') )
        return x-'a'+10+26;
    if( x=='.' )
        return 10+26+26;
    if( x=='_' )
        return 10+26+26+1;
    return -1;
}

LPWSTR encode_streamname(BOOL bTable, LPCWSTR in)
{
    DWORD count = MAX_STREAM_NAME;
    DWORD ch, next;
    LPWSTR out, p;

    if( !bTable )
        count = lstrlenW( in )+2;
    if (!(out = malloc( count * sizeof(WCHAR) ))) return NULL;
    p = out;

    if( bTable )
    {
         *p++ = 0x4840;
         count --;
    }
    while( count -- )
    {
        ch = *in++;
        if( !ch )
        {
            *p = ch;
            return out;
        }
        if( ( ch < 0x80 ) && ( utf2mime(ch) >= 0 ) )
        {
            ch = utf2mime(ch) + 0x4800;
            next = *in;
            if( next && (next<0x80) )
            {
                next = utf2mime(next);
                if( next != -1 )
                {
                     next += 0x3ffffc0;
                     ch += (next<<6);
                     in++;
                }
            }
        }
        *p++ = ch;
    }
    ERR("Failed to encode stream name (%s)\n",debugstr_w(in));
    free( out );
    return NULL;
}

static int mime2utf(int x)
{
    if( x<10 )
        return x + '0';
    if( x<(10+26))
        return x - 10 + 'A';
    if( x<(10+26+26))
        return x - 10 - 26 + 'a';
    if( x == (10+26+26) )
        return '.';
    return '_';
}

BOOL decode_streamname(LPCWSTR in, LPWSTR out)
{
    WCHAR ch;
    DWORD count = 0;

    while ( (ch = *in++) )
    {
        if( (ch >= 0x3800 ) && (ch < 0x4840 ) )
        {
            if( ch >= 0x4800 )
                ch = mime2utf(ch-0x4800);
            else
            {
                ch -= 0x3800;
                *out++ = mime2utf(ch&0x3f);
                count++;
                ch = mime2utf((ch>>6)&0x3f);
            }
        }
        *out++ = ch;
        count++;
    }
    *out = 0;
    return count;
}

void enum_stream_names( IStorage *stg )
{
    IEnumSTATSTG *stgenum = NULL;
    HRESULT r;
    STATSTG stat;
    ULONG n, count;
    WCHAR name[0x40];

    r = IStorage_EnumElements( stg, 0, NULL, 0, &stgenum );
    if( FAILED( r ) )
        return;

    n = 0;
    while( 1 )
    {
        count = 0;
        r = IEnumSTATSTG_Next( stgenum, 1, &stat, &count );
        if( FAILED( r ) || !count )
            break;
        decode_streamname( stat.pwcsName, name );
        TRACE( "stream %2lu -> %s %s\n", n, debugstr_w(stat.pwcsName), debugstr_w(name) );
        CoTaskMemFree( stat.pwcsName );
        n++;
    }

    IEnumSTATSTG_Release( stgenum );
}

UINT read_stream_data( IStorage *stg, LPCWSTR stname, BOOL table,
                       BYTE **pdata, UINT *psz )
{
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;
    VOID *data;
    ULONG sz, count;
    IStream *stm = NULL;
    STATSTG stat;
    LPWSTR encname;

    encname = encode_streamname(table, stname);

    TRACE("%s -> %s\n",debugstr_w(stname),debugstr_w(encname));

    r = IStorage_OpenStream(stg, encname, NULL,
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    free(encname);
    if( FAILED( r ) )
    {
        WARN( "open stream failed r = %#lx - empty table?\n", r );
        return ret;
    }

    r = IStream_Stat(stm, &stat, STATFLAG_NONAME );
    if( FAILED( r ) )
    {
        WARN( "open stream failed r = %#lx!\n", r );
        goto end;
    }

    if( stat.cbSize.QuadPart >> 32 )
    {
        WARN("Too big!\n");
        goto end;
    }

    sz = stat.cbSize.QuadPart;
    data = malloc(sz);
    if( !data )
    {
        WARN( "couldn't allocate memory r = %#lx!\n", r );
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    r = IStream_Read(stm, data, sz, &count );
    if( FAILED( r ) || ( count != sz ) )
    {
        free(data);
        WARN("read stream failed r = %#lx!\n", r);
        goto end;
    }

    *pdata = data;
    *psz = sz;
    ret = ERROR_SUCCESS;

end:
    IStream_Release( stm );

    return ret;
}

UINT write_stream_data( IStorage *stg, LPCWSTR stname,
                        LPCVOID data, UINT sz, BOOL bTable )
{
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;
    ULONG count;
    IStream *stm = NULL;
    ULARGE_INTEGER size;
    LARGE_INTEGER pos;
    LPWSTR encname;

    encname = encode_streamname(bTable, stname );
    r = IStorage_OpenStream( stg, encname, NULL,
            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, &stm);
    if( FAILED(r) )
    {
        r = IStorage_CreateStream( stg, encname,
                STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    }
    free( encname );
    if( FAILED( r ) )
    {
        WARN( "open stream failed r = %#lx\n", r );
        return ret;
    }

    size.QuadPart = sz;
    r = IStream_SetSize( stm, size );
    if( FAILED( r ) )
    {
        WARN("Failed to SetSize\n");
        goto end;
    }

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, STREAM_SEEK_SET, NULL );
    if( FAILED( r ) )
    {
        WARN("Failed to Seek\n");
        goto end;
    }

    if (sz)
    {
        r = IStream_Write(stm, data, sz, &count );
        if( FAILED( r ) || ( count != sz ) )
        {
            WARN("Failed to Write\n");
            goto end;
        }
    }

    ret = ERROR_SUCCESS;

end:
    IStream_Release( stm );

    return ret;
}

static void free_colinfo( struct column_info *colinfo, UINT count )
{
    UINT i;
    for (i = 0; i < count; i++) free( colinfo[i].hash_table );
}

static void free_table( MSITABLE *table )
{
    UINT i;
    for( i=0; i<table->row_count; i++ )
        free( table->data[i] );
    free( table->data );
    free( table->data_persistent );
    free_colinfo( table->colinfo, table->col_count );
    free( table->colinfo );
    free( table );
}

static UINT table_get_row_size( MSIDATABASE *db, const struct column_info *cols, UINT count, UINT bytes_per_strref )
{
    const struct column_info *last_col;

    if (!count)
        return 0;

    if (bytes_per_strref != LONG_STR_BYTES)
    {
        UINT i, size = 0;
        for (i = 0; i < count; i++) size += bytes_per_column( db, &cols[i], bytes_per_strref );
        return size;
    }
    last_col = &cols[count - 1];
    return last_col->offset + bytes_per_column( db, last_col, bytes_per_strref );
}

/* add this table to the list of cached tables in the database */
static UINT read_table_from_storage( MSIDATABASE *db, MSITABLE *t, IStorage *stg )
{
    BYTE *rawdata = NULL;
    UINT rawsize = 0, i, j, row_size, row_size_mem;

    TRACE("%s\n",debugstr_w(t->name));

    row_size = table_get_row_size( db, t->colinfo, t->col_count, db->bytes_per_strref );
    row_size_mem = table_get_row_size( db, t->colinfo, t->col_count, LONG_STR_BYTES );

    /* if we can't read the table, just assume that it's empty */
    read_stream_data( stg, t->name, TRUE, &rawdata, &rawsize );
    if( !rawdata )
        return ERROR_SUCCESS;

    TRACE("Read %d bytes\n", rawsize );

    if( rawsize % row_size )
    {
        WARN("Table size is invalid %d/%d\n", rawsize, row_size );
        goto err;
    }

    if ((t->row_count = rawsize / row_size))
    {
        if (!(t->data = calloc( t->row_count, sizeof(*t->data) ))) goto err;
        if (!(t->data_persistent = calloc( t->row_count, sizeof(BOOL) ))) goto err;
    }

    /* transpose all the data */
    TRACE("Transposing data from %d rows\n", t->row_count );
    for (i = 0; i < t->row_count; i++)
    {
        UINT ofs = 0, ofs_mem = 0;

        t->data[i] = malloc( row_size_mem );
        if( !t->data[i] )
            goto err;
        t->data_persistent[i] = TRUE;

        for (j = 0; j < t->col_count; j++)
        {
            UINT m = bytes_per_column( db, &t->colinfo[j], LONG_STR_BYTES );
            UINT n = bytes_per_column( db, &t->colinfo[j], db->bytes_per_strref );
            UINT k;

            if ( n != 2 && n != 3 && n != 4 )
            {
                ERR("oops - unknown column width %d\n", n);
                goto err;
            }
            if (t->colinfo[j].type & MSITYPE_STRING && n < m)
            {
                for (k = 0; k < m; k++)
                {
                    if (k < n)
                        t->data[i][ofs_mem + k] = rawdata[ofs * t->row_count + i * n + k];
                    else
                        t->data[i][ofs_mem + k] = 0;
                }
            }
            else
            {
                for (k = 0; k < n; k++)
                    t->data[i][ofs_mem + k] = rawdata[ofs * t->row_count + i * n + k];
            }
            ofs_mem += m;
            ofs += n;
        }
    }

    free( rawdata );
    return ERROR_SUCCESS;
err:
    free( rawdata );
    return ERROR_FUNCTION_FAILED;
}

void free_cached_tables( MSIDATABASE *db )
{
    while( !list_empty( &db->tables ) )
    {
        MSITABLE *t = LIST_ENTRY( list_head( &db->tables ), MSITABLE, entry );

        list_remove( &t->entry );
        free_table( t );
    }
}

static MSITABLE *find_cached_table( MSIDATABASE *db, LPCWSTR name )
{
    MSITABLE *t;

    LIST_FOR_EACH_ENTRY( t, &db->tables, MSITABLE, entry )
        if( !wcscmp( name, t->name ) )
            return t;

    return NULL;
}

static void table_calc_column_offsets( MSIDATABASE *db, struct column_info *colinfo, DWORD count )
{
    DWORD i;

    for (i = 0; colinfo && i < count; i++)
    {
         assert( i + 1 == colinfo[i].number );
         if (i) colinfo[i].offset = colinfo[i - 1].offset +
                                    bytes_per_column( db, &colinfo[i - 1], LONG_STR_BYTES );
         else colinfo[i].offset = 0;

         TRACE("column %d is [%s] with type %08x ofs %d\n",
               colinfo[i].number, debugstr_w(colinfo[i].colname),
               colinfo[i].type, colinfo[i].offset);
    }
}

static UINT get_defaulttablecolumns( MSIDATABASE *db, const WCHAR *name, struct column_info *colinfo, UINT *sz )
{
    const struct column_info *p;
    DWORD i, n;

    TRACE("%s\n", debugstr_w(name));

    if (!wcscmp( name, L"_Tables" ))
    {
        p = _Tables_cols;
        n = 1;
    }
    else if (!wcscmp( name, L"_Columns" ))
    {
        p = _Columns_cols;
        n = 4;
    }
    else return ERROR_FUNCTION_FAILED;

    for (i = 0; i < n; i++)
    {
        if (colinfo && i < *sz) colinfo[i] = p[i];
        if (colinfo && i >= *sz) break;
    }
    table_calc_column_offsets( db, colinfo, n );
    *sz = n;
    return ERROR_SUCCESS;
}

static UINT get_tablecolumns( MSIDATABASE *, const WCHAR *, struct column_info *, UINT * );

static UINT table_get_column_info( MSIDATABASE *db, const WCHAR *name, struct column_info **pcols, UINT *pcount )
{
    UINT r, column_count = 0;
    struct column_info *columns;

    /* get the number of columns in this table */
    column_count = 0;
    r = get_tablecolumns( db, name, NULL, &column_count );
    if (r != ERROR_SUCCESS)
        return r;

    *pcount = column_count;

    /* if there are no columns, there's no table */
    if (!column_count)
        return ERROR_INVALID_PARAMETER;

    TRACE("table %s found\n", debugstr_w(name));

    columns = malloc( column_count * sizeof(*columns) );
    if (!columns)
        return ERROR_FUNCTION_FAILED;

    r = get_tablecolumns( db, name, columns, &column_count );
    if (r != ERROR_SUCCESS)
    {
        free( columns );
        return ERROR_FUNCTION_FAILED;
    }
    *pcols = columns;
    return r;
}

static UINT get_table( MSIDATABASE *db, LPCWSTR name, MSITABLE **table_ret )
{
    MSITABLE *table;
    UINT r;

    /* first, see if the table is cached */
    table = find_cached_table( db, name );
    if (table)
    {
        *table_ret = table;
        return ERROR_SUCCESS;
    }

    /* nonexistent tables should be interpreted as empty tables */
    table = malloc( sizeof(MSITABLE) + wcslen( name ) * sizeof(WCHAR) );
    if (!table)
        return ERROR_FUNCTION_FAILED;

    table->row_count = 0;
    table->data = NULL;
    table->data_persistent = NULL;
    table->colinfo = NULL;
    table->col_count = 0;
    table->persistent = MSICONDITION_TRUE;
    lstrcpyW( table->name, name );

    if (!wcscmp( name, L"_Tables" ) || !wcscmp( name, L"_Columns" ))
        table->persistent = MSICONDITION_NONE;

    r = table_get_column_info( db, name, &table->colinfo, &table->col_count );
    if (r != ERROR_SUCCESS)
    {
        free_table( table );
        return r;
    }
    r = read_table_from_storage( db, table, db->storage );
    if (r != ERROR_SUCCESS)
    {
        free_table( table );
        return r;
    }
    list_add_head( &db->tables, &table->entry );
    *table_ret = table;
    return ERROR_SUCCESS;
}

static UINT read_table_int( BYTE *const *data, UINT row, UINT col, UINT bytes )
{
    UINT ret = 0, i;

    for (i = 0; i < bytes; i++)
        ret += data[row][col + i] << i * 8;

    return ret;
}

static UINT get_tablecolumns( MSIDATABASE *db, const WCHAR *szTableName, struct column_info *colinfo, UINT *sz )
{
    UINT r, i, n = 0, table_id, count, maxcount = *sz;
    MSITABLE *table = NULL;

    TRACE("%s\n", debugstr_w(szTableName));

    /* first check if there is a default table with that name */
    r = get_defaulttablecolumns( db, szTableName, colinfo, sz );
    if (r == ERROR_SUCCESS && *sz)
        return r;

    r = get_table( db, L"_Columns", &table );
    if (r != ERROR_SUCCESS)
    {
        ERR("couldn't load _Columns table\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* convert table and column names to IDs from the string table */
    r = msi_string2id( db->strings, szTableName, -1, &table_id );
    if (r != ERROR_SUCCESS)
    {
        WARN("Couldn't find id for %s\n", debugstr_w(szTableName));
        return r;
    }
    TRACE("Table id is %d, row count is %d\n", table_id, table->row_count);

    /* Note: _Columns table doesn't have non-persistent data */

    /* if maxcount is non-zero, assume it's exactly right for this table */
    if (colinfo) memset( colinfo, 0, maxcount * sizeof(*colinfo) );
    count = table->row_count;
    for (i = 0; i < count; i++)
    {
        if (read_table_int( table->data, i, 0, LONG_STR_BYTES) != table_id) continue;
        if (colinfo)
        {
            UINT id = read_table_int( table->data, i, table->colinfo[2].offset, LONG_STR_BYTES );
            UINT col = read_table_int( table->data, i, table->colinfo[1].offset, sizeof(USHORT) ) - (1 << 15);

            /* check the column number is in range */
            if (col < 1 || col > maxcount)
            {
                ERR("column %d out of range (maxcount: %d)\n", col, maxcount);
                continue;
            }
            /* check if this column was already set */
            if (colinfo[col - 1].number)
            {
                ERR("duplicate column %d\n", col);
                continue;
            }
            colinfo[col - 1].tablename = msi_string_lookup( db->strings, table_id, NULL );
            colinfo[col - 1].number = col;
            colinfo[col - 1].colname = msi_string_lookup( db->strings, id, NULL );
            colinfo[col - 1].type = read_table_int( table->data, i, table->colinfo[3].offset,
                                                    sizeof(USHORT) ) - (1 << 15);
            colinfo[col - 1].offset = 0;
            colinfo[col - 1].hash_table = NULL;
        }
        n++;
    }
    TRACE("%s has %d columns\n", debugstr_w(szTableName), n);

    if (colinfo && n != maxcount)
    {
        ERR("missing column in table %s\n", debugstr_w(szTableName));
        free_colinfo( colinfo, maxcount );
        return ERROR_FUNCTION_FAILED;
    }
    table_calc_column_offsets( db, colinfo, n );
    *sz = n;
    return ERROR_SUCCESS;
}

UINT msi_create_table( MSIDATABASE *db, LPCWSTR name, column_info *col_info,
                       MSICONDITION persistent, BOOL hold )
{
    UINT r, nField;
    MSIVIEW *tv = NULL;
    MSIRECORD *rec = NULL;
    column_info *col;
    MSITABLE *table;
    UINT i;

    /* only add tables that don't exist already */
    if( TABLE_Exists(db, name ) )
    {
        WARN("table %s exists\n", debugstr_w(name));
        return ERROR_BAD_QUERY_SYNTAX;
    }

    table = malloc( sizeof(MSITABLE) + wcslen(name) * sizeof(WCHAR) );
    if( !table )
        return ERROR_FUNCTION_FAILED;

    table->ref_count = 0;
    table->row_count = 0;
    table->data = NULL;
    table->data_persistent = NULL;
    table->colinfo = NULL;
    table->col_count = 0;
    table->persistent = persistent;
    lstrcpyW( table->name, name );

    if( hold )
        table->ref_count++;

    for( col = col_info; col; col = col->next )
        table->col_count++;

    table->colinfo = malloc( table->col_count * sizeof(*table->colinfo) );
    if (!table->colinfo)
    {
        free_table( table );
        return ERROR_FUNCTION_FAILED;
    }

    for( i = 0, col = col_info; col; i++, col = col->next )
    {
        UINT table_id = msi_add_string( db->strings, col->table, -1, persistent );
        UINT col_id = msi_add_string( db->strings, col->column, -1, persistent );

        table->colinfo[ i ].tablename = msi_string_lookup( db->strings, table_id, NULL );
        table->colinfo[ i ].number = i + 1;
        table->colinfo[ i ].colname = msi_string_lookup( db->strings, col_id, NULL );
        table->colinfo[ i ].type = col->type;
        table->colinfo[ i ].offset = 0;
        table->colinfo[ i ].hash_table = NULL;
    }
    table_calc_column_offsets( db, table->colinfo, table->col_count);

    r = TABLE_CreateView( db, L"_Tables", &tv );
    TRACE("CreateView returned %x\n", r);
    if( r )
    {
        free_table( table );
        return r;
    }

    r = tv->ops->execute( tv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        goto err;

    rec = MSI_CreateRecord( 1 );
    if( !rec )
        goto err;

    r = MSI_RecordSetStringW( rec, 1, name );
    if( r )
        goto err;

    r = tv->ops->insert_row( tv, rec, -1, persistent == MSICONDITION_FALSE );
    TRACE("insert_row returned %x\n", r);
    if( r )
        goto err;

    tv->ops->delete( tv );
    tv = NULL;

    msiobj_release( &rec->hdr );
    rec = NULL;

    if( persistent != MSICONDITION_FALSE )
    {
        /* add each column to the _Columns table */
        r = TABLE_CreateView( db, L"_Columns", &tv );
        if( r )
            goto err;

        r = tv->ops->execute( tv, 0 );
        TRACE("tv execute returned %x\n", r);
        if( r )
            goto err;

        rec = MSI_CreateRecord( 4 );
        if( !rec )
            goto err;

        r = MSI_RecordSetStringW( rec, 1, name );
        if( r )
            goto err;

        /*
         * need to set the table, column number, col name and type
         * for each column we enter in the table
         */
        nField = 1;
        for( col = col_info; col; col = col->next )
        {
            r = MSI_RecordSetInteger( rec, 2, nField );
            if( r )
                goto err;

            r = MSI_RecordSetStringW( rec, 3, col->column );
            if( r )
                goto err;

            r = MSI_RecordSetInteger( rec, 4, col->type );
            if( r )
                goto err;

            r = tv->ops->insert_row( tv, rec, -1, FALSE );
            if( r )
                goto err;

            nField++;
        }
        if( !col )
            r = ERROR_SUCCESS;
    }

err:
    if (rec)
        msiobj_release( &rec->hdr );
    /* FIXME: remove values from the string table on error */
    if( tv )
        tv->ops->delete( tv );

    if (r == ERROR_SUCCESS)
        list_add_head( &db->tables, &table->entry );
    else
        free_table( table );

    return r;
}

static UINT save_table( MSIDATABASE *db, const MSITABLE *t, UINT bytes_per_strref )
{
    BYTE *rawdata = NULL;
    UINT rawsize, i, j, row_size, row_count;
    UINT r = ERROR_FUNCTION_FAILED;

    /* Nothing to do for non-persistent tables */
    if( t->persistent == MSICONDITION_FALSE )
        return ERROR_SUCCESS;

    TRACE("Saving %s\n", debugstr_w( t->name ) );

    row_size = table_get_row_size( db, t->colinfo, t->col_count, bytes_per_strref );
    row_count = t->row_count;
    for (i = 0; i < t->row_count; i++)
    {
        if (!t->data_persistent[i])
        {
            row_count = 1; /* yes, this is bizarre */
            break;
        }
    }
    rawsize = row_count * row_size;
    rawdata = calloc( 1, rawsize );
    if( !rawdata )
    {
        r = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }

    rawsize = 0;
    for (i = 0; i < row_count; i++)
    {
        UINT ofs = 0, ofs_mem = 0;

        if (!t->data_persistent[i]) break;

        for (j = 0; j < t->col_count; j++)
        {
            UINT m = bytes_per_column( db, &t->colinfo[j], LONG_STR_BYTES );
            UINT n = bytes_per_column( db, &t->colinfo[j], bytes_per_strref );
            UINT k;

            if (n != 2 && n != 3 && n != 4)
            {
                ERR("oops - unknown column width %d\n", n);
                goto err;
            }
            if (t->colinfo[j].type & MSITYPE_STRING && n < m)
            {
                UINT id = read_table_int( t->data, i, ofs_mem, LONG_STR_BYTES );
                if (id > 1 << bytes_per_strref * 8)
                {
                    ERR("string id %u out of range\n", id);
                    goto err;
                }
            }
            for (k = 0; k < n; k++)
            {
                rawdata[ofs * row_count + i * n + k] = t->data[i][ofs_mem + k];
            }
            ofs_mem += m;
            ofs += n;
        }
        rawsize += row_size;
    }

    TRACE("writing %d bytes\n", rawsize);
    r = write_stream_data( db->storage, t->name, rawdata, rawsize, TRUE );

err:
    free( rawdata );
    return r;
}

static void update_table_columns( MSIDATABASE *db, const WCHAR *name )
{
    MSITABLE *table;
    UINT size, offset, old_count;
    UINT n;

    if (!(table = find_cached_table( db, name ))) return;
    old_count = table->col_count;
    free_colinfo( table->colinfo, table->col_count );
    free( table->colinfo );
    table->colinfo = NULL;

    table_get_column_info( db, name, &table->colinfo, &table->col_count );
    if (!table->col_count) return;

    size = table_get_row_size( db, table->colinfo, table->col_count, LONG_STR_BYTES );
    offset = table->colinfo[table->col_count - 1].offset;

    for ( n = 0; n < table->row_count; n++ )
    {
        table->data[n] = realloc( table->data[n], size );
        if (old_count < table->col_count)
            memset( &table->data[n][offset], 0, size - offset );
    }
}

/* try to find the table name in the _Tables table */
BOOL TABLE_Exists( MSIDATABASE *db, LPCWSTR name )
{
    UINT r, table_id, i;
    MSITABLE *table;

    if( !wcscmp( name, L"_Tables" ) || !wcscmp( name, L"_Columns" ) ||
        !wcscmp( name, L"_Streams" ) || !wcscmp( name, L"_Storages" ) )
        return TRUE;

    r = msi_string2id( db->strings, name, -1, &table_id );
    if( r != ERROR_SUCCESS )
    {
        TRACE("Couldn't find id for %s\n", debugstr_w(name));
        return FALSE;
    }

    r = get_table( db, L"_Tables", &table );
    if( r != ERROR_SUCCESS )
    {
        ERR("table _Tables not available\n");
        return FALSE;
    }

    for( i = 0; i < table->row_count; i++ )
    {
        if( read_table_int( table->data, i, 0, LONG_STR_BYTES ) == table_id )
            return TRUE;
    }

    return FALSE;
}

/* below is the query interface to a table */

struct table_view
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSITABLE      *table;
    struct column_info *columns;
    UINT           num_cols;
    UINT           row_size;
    WCHAR          name[1];
};

static UINT TABLE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    struct table_view *tv = (struct table_view *)view;
    UINT offset, n;

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col>tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    /* how many rows are there ? */
    if( row >= tv->table->row_count )
        return ERROR_NO_MORE_ITEMS;

    if( tv->columns[col-1].offset >= tv->row_size )
    {
        ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
        ERR("%p %p\n", tv, tv->columns );
        return ERROR_FUNCTION_FAILED;
    }

    n = bytes_per_column( tv->db, &tv->columns[col - 1], LONG_STR_BYTES );
    if (n != 2 && n != 3 && n != 4)
    {
        ERR("oops! what is %d bytes per column?\n", n );
        return ERROR_FUNCTION_FAILED;
    }

    offset = tv->columns[col-1].offset;
    *val = read_table_int(tv->table->data, row, offset, n);

    /* TRACE("Data [%d][%d] = %d\n", row, col, *val ); */

    return ERROR_SUCCESS;
}

static UINT get_stream_name( const struct table_view *tv, UINT row, WCHAR **pstname )
{
    LPWSTR p, stname = NULL;
    UINT i, r, type, ival;
    DWORD len;
    LPCWSTR sval;
    MSIVIEW *view = (MSIVIEW *) tv;

    TRACE("%p %d\n", tv, row);

    len = lstrlenW( tv->name ) + 1;
    stname = malloc( len * sizeof(WCHAR) );
    if ( !stname )
    {
       r = ERROR_OUTOFMEMORY;
       goto err;
    }

    lstrcpyW( stname, tv->name );

    for ( i = 0; i < tv->num_cols; i++ )
    {
        type = tv->columns[i].type;
        if ( type & MSITYPE_KEY )
        {
            WCHAR number[0x20];

            r = TABLE_fetch_int( view, row, i+1, &ival );
            if ( r != ERROR_SUCCESS )
                goto err;

            if ( tv->columns[i].type & MSITYPE_STRING )
            {
                sval = msi_string_lookup( tv->db->strings, ival, NULL );
                if ( !sval )
                {
                    r = ERROR_INVALID_PARAMETER;
                    goto err;
                }
            }
            else
            {
                UINT n = bytes_per_column( tv->db, &tv->columns[i], LONG_STR_BYTES );

                switch( n )
                {
                case 2:
                    swprintf( number, ARRAY_SIZE(number), L"%d", ival-0x8000 );
                    break;
                case 4:
                    swprintf( number, ARRAY_SIZE(number), L"%d", ival^0x80000000 );
                    break;
                default:
                    ERR( "oops - unknown column width %d\n", n );
                    r = ERROR_FUNCTION_FAILED;
                    goto err;
                }
                sval = number;
            }

            len += lstrlenW( L"." ) + lstrlenW( sval );
            p = realloc( stname, len * sizeof(WCHAR) );
            if ( !p )
            {
                r = ERROR_OUTOFMEMORY;
                goto err;
            }
            stname = p;

            lstrcatW( stname, L"." );
            lstrcatW( stname, sval );
        }
        else
           continue;
    }

    *pstname = stname;
    return ERROR_SUCCESS;

err:
    free( stname );
    *pstname = NULL;
    return r;
}

/*
 * We need a special case for streams, as we need to reference column with
 * the name of the stream in the same table, and the table name
 * which may not be available at higher levels of the query
 */
static UINT TABLE_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm )
{
    struct table_view *tv = (struct table_view *)view;
    UINT r;
    WCHAR *name;

    if( !view->ops->fetch_int )
        return ERROR_INVALID_PARAMETER;

    r = get_stream_name( tv, row, &name );
    if (r != ERROR_SUCCESS)
    {
        ERR("fetching stream, error = %u\n", r);
        return r;
    }

    r = msi_get_stream( tv->db, name, stm );
    if (r != ERROR_SUCCESS)
        ERR("fetching stream %s, error = %u\n", debugstr_w(name), r);

    free( name );
    return r;
}

/* Set a table value, i.e. preadjusted integer or string ID. */
static UINT table_set_bytes( struct table_view *tv, UINT row, UINT col, UINT val )
{
    UINT offset, n, i;

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col>tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    if( row >= tv->table->row_count )
        return ERROR_INVALID_PARAMETER;

    if( tv->columns[col-1].offset >= tv->row_size )
    {
        ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
        ERR("%p %p\n", tv, tv->columns );
        return ERROR_FUNCTION_FAILED;
    }

    free( tv->columns[col-1].hash_table );
    tv->columns[col-1].hash_table = NULL;

    n = bytes_per_column( tv->db, &tv->columns[col - 1], LONG_STR_BYTES );
    if ( n != 2 && n != 3 && n != 4 )
    {
        ERR("oops! what is %d bytes per column?\n", n );
        return ERROR_FUNCTION_FAILED;
    }

    offset = tv->columns[col-1].offset;
    for ( i = 0; i < n; i++ )
        tv->table->data[row][offset + i] = (val >> i * 8) & 0xff;

    return ERROR_SUCCESS;
}

static UINT int_to_table_storage( const struct table_view *tv, UINT col, int val, UINT *ret )
{
    if ((tv->columns[col-1].type & MSI_DATASIZEMASK) == 2)
    {
        if (val == MSI_NULL_INTEGER)
            *ret = 0;
        else if ((val + 0x8000) & 0xffff0000)
        {
            ERR("value %d out of range\n", val);
            return ERROR_FUNCTION_FAILED;
        }
        else
            *ret = val + 0x8000;
    }
    else
        *ret = val ^ 0x80000000;

    return ERROR_SUCCESS;
}

static UINT TABLE_set_int( MSIVIEW *view, UINT row, UINT col, int val )
{
    struct table_view *tv = (struct table_view *)view;
    UINT r, table_int;

    TRACE("row %u, col %u, val %d.\n", row, col, val);

    if ((r = int_to_table_storage( tv, col, val, &table_int )))
        return r;

    if (tv->columns[col-1].type & MSITYPE_KEY)
    {
        UINT key;

        if ((r = TABLE_fetch_int( view, row, col, &key )))
            return r;
        if (key != table_int)
        {
            ERR("Cannot modify primary key %s.%s.\n",
                debugstr_w(tv->table->name), debugstr_w(tv->columns[col-1].colname));
            return ERROR_FUNCTION_FAILED;
        }
    }

    return table_set_bytes( tv, row, col, table_int );
}

static UINT TABLE_set_string( MSIVIEW *view, UINT row, UINT col, const WCHAR *val, int len )
{
    struct table_view *tv = (struct table_view *)view;
    BOOL persistent;
    UINT id, r;

    TRACE("row %u, col %u, val %s.\n", row, col, debugstr_wn(val, len));

    persistent = (tv->table->persistent != MSICONDITION_FALSE)
                  && tv->table->data_persistent[row];

    if (val)
    {
        r = msi_string2id( tv->db->strings, val, len, &id );
        if (r != ERROR_SUCCESS)
            id = msi_add_string( tv->db->strings, val, len, persistent );
    }
    else
        id = 0;

    if (tv->columns[col-1].type & MSITYPE_KEY)
    {
        UINT key;

        if ((r = TABLE_fetch_int( view, row, col, &key )))
            return r;
        if (key != id)
        {
            ERR("Cannot modify primary key %s.%s.\n",
                debugstr_w(tv->table->name), debugstr_w(tv->columns[col-1].colname));
            return ERROR_FUNCTION_FAILED;
        }
    }

    return table_set_bytes( tv, row, col, id );
}

static UINT TABLE_get_row( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec )
{
    struct table_view *tv = (struct table_view *)view;

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    return msi_view_get_row(tv->db, view, row, rec);
}

static UINT add_stream( MSIDATABASE *db, const WCHAR *name, IStream *data )
{
    MSIQUERY *query;
    MSIRECORD *rec;
    UINT r;

    TRACE("%p %s %p\n", db, debugstr_w(name), data);

    if (!(rec = MSI_CreateRecord( 2 )))
        return ERROR_OUTOFMEMORY;

    r = MSI_RecordSetStringW( rec, 1, name );
    if (r != ERROR_SUCCESS)
       goto done;

    r = MSI_RecordSetIStream( rec, 2, data );
    if (r != ERROR_SUCCESS)
       goto done;

    r = MSI_DatabaseOpenViewW( db, L"INSERT INTO `_Streams` (`Name`,`Data`) VALUES (?,?)", &query );
    if (r != ERROR_SUCCESS)
       goto done;

    r = MSI_ViewExecute( query, rec );
    msiobj_release( &query->hdr );
    if (r == ERROR_SUCCESS)
        goto done;

    msiobj_release( &rec->hdr );
    if (!(rec = MSI_CreateRecord( 2 )))
        return ERROR_OUTOFMEMORY;

    r = MSI_RecordSetIStream( rec, 1, data );
    if (r != ERROR_SUCCESS)
       goto done;

    r = MSI_RecordSetStringW( rec, 2, name );
    if (r != ERROR_SUCCESS)
       goto done;

    r = MSI_DatabaseOpenViewW( db, L"UPDATE `_Streams` SET `Data` = ? WHERE `Name` = ?", &query );
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewExecute( query, rec );
    msiobj_release( &query->hdr );

done:
    msiobj_release( &rec->hdr );
    return r;
}

static UINT TABLE_set_stream( MSIVIEW *view, UINT row, UINT col, IStream *stream )
{
    struct table_view *tv = (struct table_view *)view;
    WCHAR *name;
    UINT r;

    TRACE("row %u, col %u, stream %p.\n", row, col, stream);

    if ((r = get_stream_name( tv, row - 1, &name )))
        return r;

    r = add_stream( tv->db, name, stream );
    free( name );
    return r;
}

static UINT get_table_value_from_record( struct table_view *tv, MSIRECORD *rec, UINT iField, UINT *pvalue )
{
    struct column_info columninfo;
    UINT r;

    if (!iField || iField > tv->num_cols || MSI_RecordIsNull( rec, iField ))
        return ERROR_FUNCTION_FAILED;

    columninfo = tv->columns[ iField - 1 ];

    if ( MSITYPE_IS_BINARY(columninfo.type) )
    {
        *pvalue = 1; /* refers to the first key column */
    }
    else if ( columninfo.type & MSITYPE_STRING )
    {
        int len;
        const WCHAR *sval = msi_record_get_string( rec, iField, &len );
        if (sval)
        {
            r = msi_string2id( tv->db->strings, sval, len, pvalue );
            if (r != ERROR_SUCCESS)
                return ERROR_NOT_FOUND;
        }
        else *pvalue = 0;
    }
    else
        return int_to_table_storage( tv, iField, MSI_RecordGetInteger( rec, iField ), pvalue );

    return ERROR_SUCCESS;
}

static UINT TABLE_set_row( struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask )
{
    struct table_view *tv = (struct table_view *)view;
    UINT i, val, r = ERROR_SUCCESS;

    if ( !tv->table )
        return ERROR_INVALID_PARAMETER;

    /* test if any of the mask bits are invalid */
    if ( mask >= (1<<tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    for ( i = 0; i < tv->num_cols; i++ )
    {
        BOOL persistent;

        /* only update the fields specified in the mask */
        if ( !(mask&(1<<i)) )
            continue;

        persistent = (tv->table->persistent != MSICONDITION_FALSE) &&
                     (tv->table->data_persistent[row]);
        /* FIXME: should we allow updating keys? */

        val = 0;
        if ( !MSI_RecordIsNull( rec, i + 1 ) )
        {
            r = get_table_value_from_record (tv, rec, i + 1, &val);
            if ( MSITYPE_IS_BINARY(tv->columns[ i ].type) )
            {
                IStream *stm;
                LPWSTR stname;

                if ( r != ERROR_SUCCESS )
                    return ERROR_FUNCTION_FAILED;

                r = MSI_RecordGetIStream( rec, i + 1, &stm );
                if ( r != ERROR_SUCCESS )
                    return r;

                r = get_stream_name( tv, row, &stname );
                if ( r != ERROR_SUCCESS )
                {
                    IStream_Release( stm );
                    return r;
                }

                r = add_stream( tv->db, stname, stm );
                IStream_Release( stm );
                free( stname );

                if ( r != ERROR_SUCCESS )
                    return r;
            }
            else if ( tv->columns[i].type & MSITYPE_STRING )
            {
                UINT x;

                if ( r != ERROR_SUCCESS )
                {
                    int len;
                    const WCHAR *sval = msi_record_get_string( rec, i + 1, &len );
                    val = msi_add_string( tv->db->strings, sval, len, persistent );
                }
                else
                {
                    TABLE_fetch_int(&tv->view, row, i + 1, &x);
                    if (val == x)
                        continue;
                }
            }
            else
            {
                if ( r != ERROR_SUCCESS )
                    return ERROR_FUNCTION_FAILED;
            }
        }

        r = table_set_bytes( tv, row, i+1, val );
        if ( r != ERROR_SUCCESS )
            break;
    }
    return r;
}

static UINT table_create_new_row( struct tagMSIVIEW *view, UINT *num, BOOL temporary )
{
    struct table_view *tv = (struct table_view *)view;
    BYTE **p, *row;
    BOOL *b;
    UINT sz;
    BYTE ***data_ptr;
    BOOL **data_persist_ptr;
    UINT *row_count;

    TRACE("%p %s\n", view, temporary ? "TRUE" : "FALSE");

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    row = calloc( 1, tv->row_size );
    if( !row )
        return ERROR_NOT_ENOUGH_MEMORY;

    row_count = &tv->table->row_count;
    data_ptr = &tv->table->data;
    data_persist_ptr = &tv->table->data_persistent;
    if (*num == -1)
        *num = tv->table->row_count;

    sz = (*row_count + 1) * sizeof (BYTE*);
    if( *data_ptr )
        p = realloc( *data_ptr, sz );
    else
        p = malloc( sz );
    if( !p )
    {
        free( row );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    sz = (*row_count + 1) * sizeof (BOOL);
    if( *data_persist_ptr )
        b = realloc( *data_persist_ptr, sz );
    else
        b = malloc( sz );
    if( !b )
    {
        free( row );
        free( p );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *data_ptr = p;
    (*data_ptr)[*row_count] = row;

    *data_persist_ptr = b;
    (*data_persist_ptr)[*row_count] = !temporary;

    (*row_count)++;

    return ERROR_SUCCESS;
}

static UINT TABLE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    struct table_view *tv = (struct table_view *)view;

    TRACE("%p %p\n", tv, record);

    TRACE("There are %d columns\n", tv->num_cols );

    return ERROR_SUCCESS;
}

static UINT TABLE_close( struct tagMSIVIEW *view )
{
    TRACE("%p\n", view );

    return ERROR_SUCCESS;
}

static UINT TABLE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols)
{
    struct table_view *tv = (struct table_view *)view;

    TRACE("%p %p %p\n", view, rows, cols );

    if( cols )
        *cols = tv->num_cols;
    if( rows )
    {
        if( !tv->table )
            return ERROR_INVALID_PARAMETER;
        *rows = tv->table->row_count;
    }

    return ERROR_SUCCESS;
}

static UINT TABLE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPCWSTR *name, UINT *type, BOOL *temporary,
                LPCWSTR *table_name )
{
    struct table_view *tv = (struct table_view *)view;

    TRACE("%p %d %p %p\n", tv, n, name, type );

    if( ( n == 0 ) || ( n > tv->num_cols ) )
        return ERROR_INVALID_PARAMETER;

    if( name )
    {
        *name = tv->columns[n-1].colname;
        if( !*name )
            return ERROR_FUNCTION_FAILED;
    }

    if( table_name )
    {
        *table_name = tv->columns[n-1].tablename;
        if( !*table_name )
            return ERROR_FUNCTION_FAILED;
    }

    if( type )
        *type = tv->columns[n-1].type;

    if( temporary )
        *temporary = (tv->columns[n-1].type & MSITYPE_TEMPORARY) != 0;

    return ERROR_SUCCESS;
}

static UINT table_find_row( struct table_view *, MSIRECORD *, UINT *, UINT * );

static UINT table_validate_new( struct table_view *tv, MSIRECORD *rec, UINT *column )
{
    UINT r, row, i;

    /* check there are no null values where they're not allowed */
    for( i = 0; i < tv->num_cols; i++ )
    {
        if ( tv->columns[i].type & MSITYPE_NULLABLE )
            continue;

        if ( MSITYPE_IS_BINARY(tv->columns[i].type) )
            TRACE("skipping binary column\n");
        else if ( tv->columns[i].type & MSITYPE_STRING )
        {
            int len;
            const WCHAR *str = msi_record_get_string( rec, i+1, &len );

            if (!str || (!str[0] && !len))
            {
                if (column) *column = i;
                return ERROR_INVALID_DATA;
            }
        }
        else
        {
            UINT n;

            n = MSI_RecordGetInteger( rec, i+1 );
            if (n == MSI_NULL_INTEGER)
            {
                if (column) *column = i;
                return ERROR_INVALID_DATA;
            }
        }
    }

    /* check there are no duplicate keys */
    r = table_find_row( tv, rec, &row, column );
    if (r == ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

static int compare_record( struct table_view *tv, UINT row, MSIRECORD *rec )
{
    UINT r, i, ivalue, x;

    for (i = 0; i < tv->num_cols; i++ )
    {
        if (!(tv->columns[i].type & MSITYPE_KEY)) continue;

        r = get_table_value_from_record( tv, rec, i + 1, &ivalue );
        if (r != ERROR_SUCCESS)
            return 1;

        r = TABLE_fetch_int( &tv->view, row, i + 1, &x );
        if (r != ERROR_SUCCESS)
        {
            WARN("TABLE_fetch_int should not fail here %u\n", r);
            return -1;
        }
        if (ivalue > x)
        {
            return 1;
        }
        else if (ivalue == x)
        {
            if (i < tv->num_cols - 1) continue;
            return 0;
        }
        else
            return -1;
    }
    return 1;
}

static int find_insert_index( struct table_view *tv, MSIRECORD *rec, BOOL temporary )
{
    int idx, c, low = 0, high = tv->table->row_count - 1;

    TRACE("%p %p\n", tv, rec);

    for (c = 0; c < tv->num_cols; c++)
    {
        UINT ival;

        if (!(tv->columns[c].type & MSITYPE_KEY)) continue;

        /* Add primary key string - its index affects row index. */
        if (tv->columns[c].type & MSITYPE_STRING &&
                (get_table_value_from_record( tv, rec, c + 1, &ival ) == ERROR_NOT_FOUND))
        {
            int len;
            const WCHAR *sval = msi_record_get_string( rec, c + 1, &len );
            msi_add_string( tv->db->strings, sval, len, !temporary );
        }
        break;
    }

    while (low <= high)
    {
        idx = (low + high) / 2;
        c = compare_record( tv, idx, rec );

        if (c < 0)
            high = idx - 1;
        else if (c > 0)
            low = idx + 1;
        else
        {
            TRACE("found %u\n", idx);
            return idx;
        }
    }
    TRACE("found %u\n", high + 1);
    return high + 1;
}

static UINT TABLE_insert_row( struct tagMSIVIEW *view, MSIRECORD *rec, UINT row, BOOL temporary )
{
    struct table_view *tv = (struct table_view *)view;
    UINT i, r;

    TRACE("%p %p %s\n", tv, rec, temporary ? "TRUE" : "FALSE" );

    /* check that the key is unique - can we find a matching row? */
    r = table_validate_new( tv, rec, NULL );
    if( r != ERROR_SUCCESS )
        return ERROR_FUNCTION_FAILED;

    if (row == -1)
        row = find_insert_index( tv, rec, temporary );

    r = table_create_new_row( view, &row, temporary );
    TRACE("insert_row returned %08x\n", r);
    if( r != ERROR_SUCCESS )
        return r;

    /* shift the rows to make room for the new row */
    for (i = tv->table->row_count - 1; i > row; i--)
    {
        memmove(&(tv->table->data[i][0]),
                &(tv->table->data[i - 1][0]), tv->row_size);
        tv->table->data_persistent[i] = tv->table->data_persistent[i - 1];
    }

    /* Re-set the persistence flag */
    tv->table->data_persistent[row] = !temporary;
    return TABLE_set_row( view, row, rec, (1<<tv->num_cols) - 1 );
}

static UINT TABLE_delete_row( struct tagMSIVIEW *view, UINT row )
{
    struct table_view *tv = (struct table_view *)view;
    UINT r, num_rows, num_cols, i;

    TRACE("%p %d\n", tv, row);

    if ( !tv->table )
        return ERROR_INVALID_PARAMETER;

    r = TABLE_get_dimensions( view, &num_rows, &num_cols );
    if ( r != ERROR_SUCCESS )
        return r;

    if ( row >= num_rows )
        return ERROR_FUNCTION_FAILED;

    num_rows = tv->table->row_count;
    tv->table->row_count--;

    /* reset the hash tables */
    for (i = 0; i < tv->num_cols; i++)
    {
        free(tv->columns[i].hash_table);
        tv->columns[i].hash_table = NULL;
    }

    for (i = row + 1; i < num_rows; i++)
    {
        memcpy(tv->table->data[i - 1], tv->table->data[i], tv->row_size);
        tv->table->data_persistent[i - 1] = tv->table->data_persistent[i];
    }

    free(tv->table->data[num_rows - 1]);

    return ERROR_SUCCESS;
}

static UINT table_update(struct tagMSIVIEW *view, MSIRECORD *rec, UINT row)
{
    struct table_view *tv = (struct table_view *)view;
    UINT r, new_row;

    /* FIXME: MsiViewFetch should set rec index 0 to some ID that
     * sets the fetched record apart from other records
     */

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    r = table_find_row(tv, rec, &new_row, NULL);
    if (r != ERROR_SUCCESS)
    {
        ERR("can't find row to modify\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* the row cannot be changed */
    if (row != new_row)
        return ERROR_FUNCTION_FAILED;

    return TABLE_set_row(view, new_row, rec, (1 << tv->num_cols) - 1);
}

static UINT table_assign(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    struct table_view *tv = (struct table_view *)view;
    UINT r, row;

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    r = table_find_row(tv, rec, &row, NULL);
    if (r == ERROR_SUCCESS)
        return TABLE_set_row(view, row, rec, (1 << tv->num_cols) - 1);
    else
        return TABLE_insert_row( view, rec, -1, FALSE );
}

static UINT refresh_record( struct tagMSIVIEW *view, MSIRECORD *rec, UINT row )
{
    MSIRECORD *curr;
    UINT r, i, count;

    r = TABLE_get_row(view, row, &curr);
    if (r != ERROR_SUCCESS)
        return r;

    /* Close the original record */
    MSI_CloseRecord(&rec->hdr);

    count = MSI_RecordGetFieldCount(rec);
    for (i = 0; i < count; i++)
        MSI_RecordCopyField(curr, i + 1, rec, i + 1);

    msiobj_release(&curr->hdr);
    return ERROR_SUCCESS;
}

static UINT TABLE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                          MSIRECORD *rec, UINT row)
{
    struct table_view *tv = (struct table_view *)view;
    UINT r, frow, column;

    TRACE("%p %d %p\n", view, eModifyMode, rec );

    switch (eModifyMode)
    {
    case MSIMODIFY_DELETE:
        r = TABLE_delete_row( view, row );
        break;
    case MSIMODIFY_VALIDATE_NEW:
        r = table_validate_new( tv, rec, &column );
        if (r != ERROR_SUCCESS)
        {
            tv->view.error = MSIDBERROR_DUPLICATEKEY;
            tv->view.error_column = tv->columns[column].colname;
            r = ERROR_INVALID_DATA;
        }
        break;

    case MSIMODIFY_INSERT:
        r = table_validate_new( tv, rec, NULL );
        if (r != ERROR_SUCCESS)
            break;
        r = TABLE_insert_row( view, rec, -1, FALSE );
        break;

    case MSIMODIFY_INSERT_TEMPORARY:
        r = table_validate_new( tv, rec, NULL );
        if (r != ERROR_SUCCESS)
            break;
        r = TABLE_insert_row( view, rec, -1, TRUE );
        break;

    case MSIMODIFY_REFRESH:
        r = refresh_record( view, rec, row );
        break;

    case MSIMODIFY_UPDATE:
        r = table_update( view, rec, row );
        break;

    case MSIMODIFY_ASSIGN:
        r = table_assign( view, rec );
        break;

    case MSIMODIFY_MERGE:
        /* check row that matches this record */
        r = table_find_row( tv, rec, &frow, &column );
        if (r != ERROR_SUCCESS)
        {
            r = table_validate_new( tv, rec, NULL );
            if (r == ERROR_SUCCESS)
                r = TABLE_insert_row( view, rec, -1, FALSE );
        }
        break;

    case MSIMODIFY_REPLACE:
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

static UINT TABLE_delete( struct tagMSIVIEW *view )
{
    struct table_view *tv = (struct table_view *)view;

    TRACE("%p\n", view );

    tv->table = NULL;
    tv->columns = NULL;

    free( tv );

    return ERROR_SUCCESS;
}

static UINT TABLE_add_ref(struct tagMSIVIEW *view)
{
    struct table_view *tv = (struct table_view *)view;

    TRACE( "%p, %ld\n", view, tv->table->ref_count );
    return InterlockedIncrement(&tv->table->ref_count);
}

static UINT TABLE_remove_column(struct tagMSIVIEW *view, UINT number)
{
    struct table_view *tv = (struct table_view *)view;
    MSIRECORD *rec = NULL;
    MSIVIEW *columns = NULL;
    UINT row, r;

    if (tv->table->col_count != number)
        return ERROR_BAD_QUERY_SYNTAX;

    if (tv->table->colinfo[number-1].type & MSITYPE_TEMPORARY)
    {
        UINT size = tv->table->colinfo[number-1].offset;
        tv->table->col_count--;
        tv->table->colinfo = realloc(tv->table->colinfo, sizeof(*tv->table->colinfo) * tv->table->col_count);

        for (row = 0; row < tv->table->row_count; row++)
            tv->table->data[row] = realloc(tv->table->data[row], size);
        return ERROR_SUCCESS;
    }

    rec = MSI_CreateRecord(2);
    if (!rec)
        return ERROR_OUTOFMEMORY;

    MSI_RecordSetStringW(rec, 1, tv->name);
    MSI_RecordSetInteger(rec, 2, number);

    r = TABLE_CreateView(tv->db, L"_Columns", &columns);
    if (r != ERROR_SUCCESS)
    {
        msiobj_release(&rec->hdr);
        return r;
    }

    r = table_find_row((struct table_view *)columns, rec, &row, NULL);
    if (r != ERROR_SUCCESS)
        goto done;

    r = TABLE_delete_row(columns, row);
    if (r != ERROR_SUCCESS)
        goto done;

    update_table_columns(tv->db, tv->name);

done:
    msiobj_release(&rec->hdr);
    columns->ops->delete(columns);
    return r;
}

static UINT TABLE_release(struct tagMSIVIEW *view)
{
    struct table_view *tv = (struct table_view *)view;
    INT ref = tv->table->ref_count;
    UINT r;
    INT i;

    TRACE("%p %d\n", view, ref);

    ref = InterlockedDecrement(&tv->table->ref_count);
    if (ref == 0)
    {
        for (i = tv->table->col_count - 1; i >= 0; i--)
        {
            if (tv->table->colinfo[i].type & MSITYPE_TEMPORARY)
            {
                r = TABLE_remove_column(view, tv->table->colinfo[i].number);
                if (r != ERROR_SUCCESS)
                    break;
            }
            else
            {
                break;
            }
        }

        if (!tv->table->col_count)
        {
            list_remove(&tv->table->entry);
            free_table(tv->table);
            TABLE_delete(view);
        }
    }

    return ref;
}

static UINT TABLE_add_column(struct tagMSIVIEW *view, LPCWSTR column,
                             INT type, BOOL hold)
{
    UINT i, r, table_id, col_id, size, offset;
    BOOL temporary = type & MSITYPE_TEMPORARY;
    struct table_view *tv = (struct table_view *)view;
    struct column_info *colinfo;

    if (temporary && !hold && !tv->table->ref_count)
        return ERROR_SUCCESS;

    if (!temporary && tv->table->col_count &&
            tv->table->colinfo[tv->table->col_count-1].type & MSITYPE_TEMPORARY)
        return ERROR_BAD_QUERY_SYNTAX;

    for (i = 0; i < tv->table->col_count; i++)
    {
        if (!wcscmp(tv->table->colinfo[i].colname, column))
            return ERROR_BAD_QUERY_SYNTAX;
    }

    colinfo = realloc(tv->table->colinfo, sizeof(*tv->table->colinfo) * (tv->table->col_count + 1));
    if (!colinfo)
        return ERROR_OUTOFMEMORY;
    tv->table->colinfo = colinfo;

    r = msi_string2id( tv->db->strings, tv->name, -1, &table_id );
    if (r != ERROR_SUCCESS)
        return r;
    col_id = msi_add_string( tv->db->strings, column, -1, !temporary );

    colinfo[tv->table->col_count].tablename = msi_string_lookup( tv->db->strings, table_id, NULL );
    colinfo[tv->table->col_count].number = tv->table->col_count + 1;
    colinfo[tv->table->col_count].colname = msi_string_lookup( tv->db->strings, col_id, NULL );
    colinfo[tv->table->col_count].type = type;
    colinfo[tv->table->col_count].offset = 0;
    colinfo[tv->table->col_count].hash_table = NULL;
    tv->table->col_count++;

    table_calc_column_offsets( tv->db, tv->table->colinfo, tv->table->col_count);

    size = table_get_row_size( tv->db, tv->table->colinfo, tv->table->col_count, LONG_STR_BYTES );
    offset = tv->table->colinfo[tv->table->col_count - 1].offset;
    for (i = 0; i < tv->table->row_count; i++)
    {
        BYTE *data = realloc(tv->table->data[i], size);
        if (!data)
        {
            tv->table->col_count--;
            return ERROR_OUTOFMEMORY;
        }

        tv->table->data[i] = data;
        memset(data + offset, 0, size - offset);
    }

    if (!temporary)
    {
        MSIVIEW *columns;
        MSIRECORD *rec;

        rec = MSI_CreateRecord(4);
        if (!rec)
        {
            tv->table->col_count--;
            return ERROR_OUTOFMEMORY;
        }

        MSI_RecordSetStringW(rec, 1, tv->name);
        MSI_RecordSetInteger(rec, 2, tv->table->col_count);
        MSI_RecordSetStringW(rec, 3, column);
        MSI_RecordSetInteger(rec, 4, type);

        r = TABLE_CreateView(tv->db, L"_Columns", &columns);
        if (r != ERROR_SUCCESS)
        {
            tv->table->col_count--;
            msiobj_release(&rec->hdr);
            return r;
        }

        r = TABLE_insert_row(columns, rec, -1, FALSE);
        columns->ops->delete(columns);
        msiobj_release(&rec->hdr);
        if (r != ERROR_SUCCESS)
        {
            tv->table->col_count--;
            return r;
        }
    }
    if (hold)
        TABLE_add_ref(view);
    return ERROR_SUCCESS;
}

static UINT TABLE_drop(struct tagMSIVIEW *view)
{
    struct table_view *tv = (struct table_view *)view;
    MSIVIEW *tables = NULL;
    MSIRECORD *rec = NULL;
    UINT r, row;
    INT i;

    TRACE("dropping table %s\n", debugstr_w(tv->name));

    for (i = tv->table->col_count - 1; i >= 0; i--)
    {
        r = TABLE_remove_column(view, tv->table->colinfo[i].number);
        if (r != ERROR_SUCCESS)
            return r;
    }

    rec = MSI_CreateRecord(1);
    if (!rec)
        return ERROR_OUTOFMEMORY;

    MSI_RecordSetStringW(rec, 1, tv->name);

    r = TABLE_CreateView(tv->db, L"_Tables", &tables);
    if (r != ERROR_SUCCESS)
    {
        msiobj_release(&rec->hdr);
        return r;
    }

    r = table_find_row((struct table_view *)tables, rec, &row, NULL);
    if (r != ERROR_SUCCESS)
        goto done;

    r = TABLE_delete_row(tables, row);
    if (r != ERROR_SUCCESS)
        goto done;

    list_remove(&tv->table->entry);
    free_table(tv->table);

done:
    msiobj_release(&rec->hdr);
    tables->ops->delete(tables);

    return r;
}

static const MSIVIEWOPS table_ops =
{
    TABLE_fetch_int,
    TABLE_fetch_stream,
    TABLE_set_int,
    TABLE_set_string,
    TABLE_set_stream,
    TABLE_set_row,
    TABLE_insert_row,
    TABLE_delete_row,
    TABLE_execute,
    TABLE_close,
    TABLE_get_dimensions,
    TABLE_get_column_info,
    TABLE_modify,
    TABLE_delete,
    TABLE_add_ref,
    TABLE_release,
    TABLE_add_column,
    NULL,
    TABLE_drop,
};

UINT TABLE_CreateView( MSIDATABASE *db, LPCWSTR name, MSIVIEW **view )
{
    struct table_view *tv ;
    UINT r, sz;

    TRACE("%p %s %p\n", db, debugstr_w(name), view );

    if ( !wcscmp( name, L"_Streams" ) )
        return STREAMS_CreateView( db, view );
    else if ( !wcscmp( name, L"_Storages" ) )
        return STORAGES_CreateView( db, view );

    sz = FIELD_OFFSET( struct table_view, name[lstrlenW( name ) + 1] );
    tv = calloc( 1, sz );
    if( !tv )
        return ERROR_FUNCTION_FAILED;

    r = get_table( db, name, &tv->table );
    if( r != ERROR_SUCCESS )
    {
        free( tv );
        WARN("table not found\n");
        return r;
    }

    TRACE("table %p found with %d columns\n", tv->table, tv->table->col_count);

    /* fill the structure */
    tv->view.ops = &table_ops;
    tv->db = db;
    tv->columns = tv->table->colinfo;
    tv->num_cols = tv->table->col_count;
    tv->row_size = table_get_row_size( db, tv->table->colinfo, tv->table->col_count, LONG_STR_BYTES );

    TRACE("%s one row is %d bytes\n", debugstr_w(name), tv->row_size );

    *view = (MSIVIEW*) tv;
    lstrcpyW( tv->name, name );

    return ERROR_SUCCESS;
}

static WCHAR *create_key_string(struct table_view *tv, MSIRECORD *rec)
{
    DWORD i, p, len, key_len = 0;
    WCHAR *key;

    for (i = 0; i < tv->num_cols; i++)
    {
        if (!(tv->columns[i].type & MSITYPE_KEY))
            continue;
        if (MSI_RecordGetStringW( rec, i+1, NULL, &len ) == ERROR_SUCCESS)
            key_len += len;
        key_len++;
    }

    key = malloc( key_len * sizeof(WCHAR) );
    if(!key)
        return NULL;

    p = 0;
    for (i = 0; i < tv->num_cols; i++)
    {
        if (!(tv->columns[i].type & MSITYPE_KEY))
            continue;
        if (p)
            key[p++] = '\t';
        len = key_len - p;
        if (MSI_RecordGetStringW( rec, i+1, key + p, &len ) == ERROR_SUCCESS)
            p += len;
    }
    return key;
}

static UINT record_stream_name( const struct table_view *tv, MSIRECORD *rec, WCHAR *name, DWORD *len )
{
    UINT p = 0, i, r;
    DWORD l;

    l = wcslen( tv->name );
    if (name && *len > l)
        memcpy(name, tv->name, l * sizeof(WCHAR));
    p += l;

    for ( i = 0; i < tv->num_cols; i++ )
    {
        if (!(tv->columns[i].type & MSITYPE_KEY))
            continue;

        if (name && *len > p + 1)
            name[p] = '.';
        p++;

        l = (*len > p ? *len - p : 0);
        r = MSI_RecordGetStringW( rec, i + 1, name ? name + p : NULL, &l );
        if (r != ERROR_SUCCESS)
            return r;
        p += l;
    }

    if (name && *len > p)
        name[p] = 0;

    *len = p;
    return ERROR_SUCCESS;
}

static UINT TransformView_fetch_int( MSIVIEW *view, UINT row, UINT col, UINT *val )
{
    struct table_view *tv = (struct table_view *)view;

    if (!tv->table || col > tv->table->col_count)
    {
        *val = 0;
        return ERROR_SUCCESS;
    }
    return TABLE_fetch_int( view, row, col, val );
}

static UINT TransformView_fetch_stream( MSIVIEW *view, UINT row, UINT col, IStream **stm )
{
    struct table_view *tv = (struct table_view *)view;

    if (!tv->table || col > tv->table->col_count)
    {
        *stm = NULL;
        return ERROR_SUCCESS;
    }
    return TABLE_fetch_stream( view, row, col, stm );
}

static UINT TransformView_set_row( MSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask )
{
    static const WCHAR query_pfx[] =
        L"INSERT INTO `_TransformView` (`new`, `Table`, `Column`, `Row`, `Data`, `Current`) VALUES (1, '";

    struct table_view *tv = (struct table_view *)view;
    WCHAR buf[256], *query;
    MSIRECORD *old_rec;
    MSIQUERY *q;
    WCHAR *key;
    UINT i, p, r, qlen;
    DWORD len;

    if (!wcscmp( tv->name, L"_Columns" ))
    {
        ERR( "trying to modify existing column\n" );
        return ERROR_INSTALL_TRANSFORM_FAILURE;
    }

    if (!wcscmp( tv->name, L"_Tables" ))
    {
        ERR( "trying to modify existing table\n" );
        return ERROR_INSTALL_TRANSFORM_FAILURE;
    }

    key = create_key_string( tv, rec );
    if (!key)
        return ERROR_OUTOFMEMORY;

    r = msi_view_get_row( tv->db, view, row, &old_rec );
    if (r != ERROR_SUCCESS)
        old_rec = NULL;

    for (i = 0; i < tv->num_cols; i++)
    {
        if (!(mask & (1 << i)))
            continue;
        if (tv->columns[i].type & MSITYPE_KEY)
            continue;

        qlen = p = ARRAY_SIZE( query_pfx ) - 1;
        qlen += wcslen( tv->name ) + 3; /* strlen("','") */
        qlen += wcslen( tv->columns[i].colname ) + 3;
        qlen += wcslen( key ) + 3;
        if (MSITYPE_IS_BINARY( tv->columns[i].type ))
            r = record_stream_name( tv, rec, NULL, &len );
        else
            r = MSI_RecordGetStringW( rec, i + 1, NULL, &len );
        if (r != ERROR_SUCCESS)
        {
            if (old_rec)
                msiobj_release( &old_rec->hdr );
            free( key );
            return r;
        }
        qlen += len + 3;
        if (old_rec && (r = MSI_RecordGetStringW( old_rec, i+1, NULL, &len )))
        {
            msiobj_release( &old_rec->hdr );
            free( key );
            return r;
        }
        qlen += len + 3; /* strlen("')") + 1 */

        if (qlen > ARRAY_SIZE(buf))
        {
            query = malloc( qlen * sizeof(WCHAR) );
            if (!query)
            {
                if (old_rec)
                    msiobj_release( &old_rec->hdr );
                free( key );
                return ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            query = buf;
        }

        memcpy( query, query_pfx, p * sizeof(WCHAR) );
        len = wcslen( tv->name );
        memcpy( query + p, tv->name, len * sizeof(WCHAR) );
        p += len;
        query[p++] = '\'';
        query[p++] = ',';
        query[p++] = '\'';
        len = wcslen( tv->columns[i].colname );
        memcpy( query + p, tv->columns[i].colname, len * sizeof(WCHAR) );
        p += len;
        query[p++] = '\'';
        query[p++] = ',';
        query[p++] = '\'';
        len = wcslen( key );
        memcpy( query + p, key, len * sizeof(WCHAR) );
        p += len;
        query[p++] = '\'';
        query[p++] = ',';
        query[p++] = '\'';
        len = qlen - p;
        if (MSITYPE_IS_BINARY( tv->columns[i].type ))
            record_stream_name( tv, rec, query + p, &len );
        else
            MSI_RecordGetStringW( rec, i + 1, query + p, &len );
        p += len;
        query[p++] = '\'';
        query[p++] = ',';
        query[p++] = '\'';
        if (old_rec)
        {
            len = qlen - p;
            MSI_RecordGetStringW( old_rec, i + 1, query + p, &len );
            p += len;
        }
        query[p++] = '\'';
        query[p++] = ')';
        query[p++] = 0;

        r = MSI_DatabaseOpenViewW( tv->db, query, &q );
        if (query != buf)
            free( query );
        if (r != ERROR_SUCCESS)
        {
            if (old_rec)
                msiobj_release( &old_rec->hdr );
            free( key );
            return r;
        }

        r = MSI_ViewExecute( q, NULL );
        msiobj_release( &q->hdr );
        if (r != ERROR_SUCCESS)
        {
            if (old_rec)
                msiobj_release( &old_rec->hdr );
            free( key );
            return r;
        }
    }

    if (old_rec)
        msiobj_release( &old_rec->hdr );
    free( key );
    return ERROR_SUCCESS;
}

static UINT TransformView_create_table( struct table_view *tv, MSIRECORD *rec )
{
    static const WCHAR query_fmt[] =
        L"INSERT INTO `_TransformView` (`Table`, `Column`, `new`) VALUES ('%s', 'CREATE', 1)";

    WCHAR buf[256], *query = buf;
    const WCHAR *name;
    MSIQUERY *q;
    int len;
    UINT r;

    name = msi_record_get_string( rec, 1, &len );
    if (!name)
        return ERROR_INSTALL_TRANSFORM_FAILURE;

    len = _snwprintf( NULL, 0, query_fmt, name ) + 1;
    if (len > ARRAY_SIZE(buf))
    {
        query = malloc( len * sizeof(WCHAR) );
        if (!query)
            return ERROR_OUTOFMEMORY;
    }
    swprintf( query, len, query_fmt, name );

    r = MSI_DatabaseOpenViewW( tv->db, query, &q );
    if (query != buf)
        free( query );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( q, NULL );
    msiobj_release( &q->hdr );
    return r;
}

static UINT TransformView_add_column( struct table_view *tv, MSIRECORD *rec )
{
    static const WCHAR query_pfx[] =
        L"INSERT INTO `_TransformView` (`new`, `Table`, `Current`, `Column`, `Data`) VALUES (1, '";

    WCHAR buf[256], *query = buf;
    UINT i, p, r, qlen;
    DWORD len;
    MSIQUERY *q;

    qlen = p = wcslen( query_pfx );
    for (i = 1; i <= 4; i++)
    {
        r = MSI_RecordGetStringW( rec, i, NULL, &len );
        if (r != ERROR_SUCCESS)
            return r;
        qlen += len + 3; /* strlen( "','" ) */
    }

    if (qlen > ARRAY_SIZE(buf))
    {
        query = malloc( len * sizeof(WCHAR) );
        qlen = len;
        if (!query)
            return ERROR_OUTOFMEMORY;
    }

    memcpy( query, query_pfx, p * sizeof(WCHAR) );
    for (i = 1; i <= 4; i++)
    {
        len = qlen - p;
        MSI_RecordGetStringW( rec, i, query + p, &len );
        p += len;
        query[p++] = '\'';
        if (i != 4)
        {
            query[p++] = ',';
            query[p++] = '\'';
        }
    }
    query[p++] = ')';
    query[p++] = 0;

    r = MSI_DatabaseOpenViewW( tv->db, query, &q );
    if (query != buf)
        free( query );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( q, NULL );
    msiobj_release( &q->hdr );
    return r;
}

static UINT TransformView_insert_row( MSIVIEW *view, MSIRECORD *rec, UINT row, BOOL temporary )
{
    static const WCHAR query_fmt[] =
        L"INSERT INTO `_TransformView` (`new`, `Table`, `Column`, `Row`) VALUES (1, '%s', 'INSERT', '%s')";

    struct table_view *tv = (struct table_view *)view;
    WCHAR buf[256], *query = buf;
    MSIQUERY *q;
    WCHAR *key;
    int len;
    UINT r;

    if (!wcscmp(tv->name, L"_Tables"))
        return TransformView_create_table( tv, rec );

    if (!wcscmp(tv->name, L"_Columns"))
        return TransformView_add_column( tv, rec );

    key = create_key_string( tv, rec );
    if (!key)
        return ERROR_OUTOFMEMORY;

    len = _snwprintf( NULL, 0, query_fmt, tv->name, key ) + 1;
    if (len > ARRAY_SIZE(buf))
    {
        query = malloc( len * sizeof(WCHAR) );
        if (!query)
        {
            free( key );
            return ERROR_OUTOFMEMORY;
        }
    }
    swprintf( query, len, query_fmt, tv->name, key );
    free( key );

    r = MSI_DatabaseOpenViewW( tv->db, query, &q );
    if (query != buf)
        free( query );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( q, NULL );
    msiobj_release( &q->hdr );
    if (r != ERROR_SUCCESS)
        return r;

    return TransformView_set_row( view, row, rec, ~0 );
}

static UINT TransformView_drop_table( struct table_view *tv, UINT row )
{
    static const WCHAR query_pfx[] = L"INSERT INTO `_TransformView` ( `new`, `Table`, `Column` ) VALUES ( 1, '";
    static const WCHAR query_sfx[] = L"', 'DROP' )";

    WCHAR buf[256], *query = buf;
    UINT r, table_id, len;
    const WCHAR *table;
    int table_len;
    MSIQUERY *q;

    r = TABLE_fetch_int( &tv->view, row, 1, &table_id );
    if (r != ERROR_SUCCESS)
        return r;

    table = msi_string_lookup( tv->db->strings, table_id, &table_len );
    if (!table)
        return ERROR_INSTALL_TRANSFORM_FAILURE;

    len = ARRAY_SIZE(query_pfx) - 1 + table_len + ARRAY_SIZE(query_sfx);
    if (len > ARRAY_SIZE(buf))
    {
        query = malloc( len * sizeof(WCHAR) );
        if (!query)
            return ERROR_OUTOFMEMORY;
    }

    memcpy( query, query_pfx, ARRAY_SIZE(query_pfx) * sizeof(WCHAR) );
    len = ARRAY_SIZE(query_pfx) - 1;
    memcpy( query + len, table, table_len * sizeof(WCHAR) );
    len += table_len;
    memcpy( query + len, query_sfx, ARRAY_SIZE(query_sfx) * sizeof(WCHAR) );

    r = MSI_DatabaseOpenViewW( tv->db, query, &q );
    if (query != buf)
        free( query );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( q, NULL );
    msiobj_release( &q->hdr );
    return r;
}

static UINT TransformView_delete_row( MSIVIEW *view, UINT row )
{
    static const WCHAR query_pfx[] = L"INSERT INTO `_TransformView` ( `new`, `Table`, `Column`, `Row`) VALUES ( 1, '";
    static const WCHAR query_column[] = L"', 'DELETE', '";
    static const WCHAR query_sfx[] = L"')";

    struct table_view *tv = (struct table_view *)view;
    WCHAR *key, buf[256], *query = buf;
    UINT r, len, name_len, key_len;
    MSIRECORD *rec;
    MSIQUERY *q;

    if (!wcscmp( tv->name, L"_Columns" ))
    {
        ERR("trying to remove column\n");
        return ERROR_INSTALL_TRANSFORM_FAILURE;
    }

    if (!wcscmp( tv->name, L"_Tables" ))
        return TransformView_drop_table( tv, row );

    r = msi_view_get_row( tv->db, view, row, &rec );
    if (r != ERROR_SUCCESS)
        return r;

    key = create_key_string( tv, rec );
    msiobj_release( &rec->hdr );
    if (!key)
        return ERROR_OUTOFMEMORY;

    name_len = wcslen( tv->name );
    key_len = wcslen( key );
    len = ARRAY_SIZE(query_pfx) + name_len + ARRAY_SIZE(query_column) + key_len + ARRAY_SIZE(query_sfx) - 2;
    if (len > ARRAY_SIZE(buf))
    {
        query = malloc( len * sizeof(WCHAR) );
        if (!query)
        {
            free( tv );
            free( key );
            return ERROR_OUTOFMEMORY;
        }
    }

    memcpy( query, query_pfx, ARRAY_SIZE(query_pfx) * sizeof(WCHAR) );
    len = ARRAY_SIZE(query_pfx) - 1;
    memcpy( query + len, tv->name, name_len * sizeof(WCHAR) );
    len += name_len;
    memcpy( query + len, query_column, ARRAY_SIZE(query_column) * sizeof(WCHAR) );
    len += ARRAY_SIZE(query_column) - 1;
    memcpy( query + len, key, key_len * sizeof(WCHAR) );
    len += key_len;
    memcpy( query + len, query_sfx, ARRAY_SIZE(query_sfx) * sizeof(WCHAR) );
    free( key );

    r = MSI_DatabaseOpenViewW( tv->db, query, &q );
    if (query != buf)
        free( query );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_ViewExecute( q, NULL );
    msiobj_release( &q->hdr );
    return r;
}

static UINT TransformView_execute( MSIVIEW *view, MSIRECORD *record )
{
    return ERROR_SUCCESS;
}

static UINT TransformView_close( MSIVIEW *view )
{
    return ERROR_SUCCESS;
}

static UINT TransformView_get_dimensions( MSIVIEW *view, UINT *rows, UINT *cols )
{
    return TABLE_get_dimensions( view, rows, cols );
}

static UINT TransformView_get_column_info( MSIVIEW *view, UINT n, LPCWSTR *name, UINT *type,
                             BOOL *temporary, LPCWSTR *table_name )
{
    return TABLE_get_column_info( view, n, name, type, temporary, table_name );
}

static UINT TransformView_delete( MSIVIEW *view )
{
    struct table_view *tv = (struct table_view *)view;
    if (!tv->table || tv->columns != tv->table->colinfo)
        free( tv->columns );
    return TABLE_delete( view );
}

static const MSIVIEWOPS transform_view_ops =
{
    TransformView_fetch_int,
    TransformView_fetch_stream,
    NULL,
    NULL,
    NULL,
    TransformView_set_row,
    TransformView_insert_row,
    TransformView_delete_row,
    TransformView_execute,
    TransformView_close,
    TransformView_get_dimensions,
    TransformView_get_column_info,
    NULL,
    TransformView_delete,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static UINT TransformView_Create( MSIDATABASE *db, string_table *st, LPCWSTR name, MSIVIEW **view )
{
    static const WCHAR query_pfx[] = L"SELECT `Column`, `Data`, `Current` FROM `_TransformView` WHERE `Table`='";
    static const WCHAR query_sfx[] = L"' AND `Row` IS NULL AND `Current` IS NOT NULL AND `new` = 1";

    WCHAR buf[256], *query = buf;
    UINT r, len, name_len, size, add_col;
    struct column_info *colinfo;
    struct table_view *tv;
    MSIRECORD *rec;
    MSIQUERY *q;

    name_len = wcslen( name );

    r = TABLE_CreateView( db, name, (MSIVIEW **)&tv );
    if (r == ERROR_INVALID_PARAMETER)
    {
        /* table does not exist */
        size = FIELD_OFFSET( struct table_view, name[name_len + 1] );
        tv = calloc( 1, size );
        if (!tv)
            return ERROR_OUTOFMEMORY;

        tv->db = db;
        memcpy( tv->name, name, name_len * sizeof(WCHAR) );
    }
    else if (r != ERROR_SUCCESS)
    {
        return r;
    }

    tv->view.ops = &transform_view_ops;

    len = ARRAY_SIZE(query_pfx) + name_len + ARRAY_SIZE(query_sfx) - 1;
    if (len > ARRAY_SIZE(buf))
    {
        query = malloc( len * sizeof(WCHAR) );
        if (!query)
        {
            free( tv );
            return ERROR_OUTOFMEMORY;
        }
    }
    memcpy( query, query_pfx, ARRAY_SIZE(query_pfx) * sizeof(WCHAR) );
    len = ARRAY_SIZE(query_pfx) - 1;
    memcpy( query + len, name, name_len * sizeof(WCHAR) );
    len += name_len;
    memcpy( query + len, query_sfx, ARRAY_SIZE(query_sfx) * sizeof(WCHAR) );

    r = MSI_DatabaseOpenViewW( tv->db, query, &q );
    if (query != buf)
        free( query );
    if (r != ERROR_SUCCESS)
    {
        free( tv );
        return r;
    }

    r = MSI_ViewExecute( q, NULL );
    if (r != ERROR_SUCCESS)
    {
        free( tv );
        return r;
    }

    r = q->view->ops->get_dimensions( q->view, &add_col, NULL );
    if (r != ERROR_SUCCESS)
    {
        MSI_ViewClose( q );
        msiobj_release( &q->hdr );
        free( tv );
        return r;
    }
    if (!add_col)
    {
        MSI_ViewClose( q );
        msiobj_release( &q->hdr );
        *view = (MSIVIEW *)tv;
        return ERROR_SUCCESS;
    }

    colinfo = calloc( add_col + tv->num_cols, sizeof(*colinfo) );
    if (!colinfo)
    {
        MSI_ViewClose( q );
        msiobj_release( &q->hdr );
        free( tv );
        return ERROR_OUTOFMEMORY;
    }

    while (MSI_ViewFetch( q, &rec ) == ERROR_SUCCESS)
    {
        int name_len;
        const WCHAR *name = msi_record_get_string( rec, 1, &name_len );
        const WCHAR *type = msi_record_get_string( rec, 2, NULL );
        UINT name_id, idx;

        idx = _wtoi( msi_record_get_string(rec, 3, NULL) );
        colinfo[idx - 1].number = idx;
        colinfo[idx - 1].type = _wtoi( type );

        r = msi_string2id( st, name, name_len, &name_id );
        if (r == ERROR_SUCCESS)
            colinfo[idx - 1].colname = msi_string_lookup( st, name_id, NULL );
        else
            ERR( "column name %s is not defined in strings table\n", wine_dbgstr_w(name) );
        msiobj_release( &rec->hdr );
    }
    MSI_ViewClose( q );
    msiobj_release( &q->hdr );

    memcpy( colinfo, tv->columns, tv->num_cols * sizeof(*colinfo) );
    tv->columns = colinfo;
    tv->num_cols += add_col;
    *view = (MSIVIEW *)tv;
    return ERROR_SUCCESS;
}

UINT MSI_CommitTables( MSIDATABASE *db )
{
    UINT r, bytes_per_strref;
    HRESULT hr;
    MSITABLE *table = NULL;

    TRACE("%p\n",db);

    r = msi_save_string_table( db->strings, db->storage, &bytes_per_strref );
    if( r != ERROR_SUCCESS )
    {
        WARN("failed to save string table r=%08x\n",r);
        return r;
    }

    LIST_FOR_EACH_ENTRY( table, &db->tables, MSITABLE, entry )
    {
        r = save_table( db, table, bytes_per_strref );
        if( r != ERROR_SUCCESS )
        {
            WARN("failed to save table %s (r=%08x)\n",
                  debugstr_w(table->name), r);
            return r;
        }
    }

    hr = IStorage_Commit( db->storage, 0 );
    if (FAILED( hr ))
    {
        WARN( "failed to commit changes %#lx\n", hr );
        r = ERROR_FUNCTION_FAILED;
    }
    return r;
}

MSICONDITION MSI_DatabaseIsTablePersistent( MSIDATABASE *db, LPCWSTR table )
{
    MSITABLE *t;
    UINT r;

    TRACE("%p %s\n", db, debugstr_w(table));

    if (!table)
        return MSICONDITION_ERROR;

    r = get_table( db, table, &t );
    if (r != ERROR_SUCCESS)
        return MSICONDITION_NONE;

    return t->persistent;
}

static UINT read_raw_int(const BYTE *data, UINT col, UINT bytes)
{
    UINT ret = 0, i;

    for (i = 0; i < bytes; i++)
        ret += (data[col + i] << i * 8);

    return ret;
}

static UINT record_encoded_stream_name( const struct table_view *tv, MSIRECORD *rec, WCHAR **pstname )
{
    UINT r;
    DWORD len;
    WCHAR *name;

    TRACE("%p %p\n", tv, rec);

    r = record_stream_name( tv, rec, NULL, &len );
    if (r != ERROR_SUCCESS)
        return r;
    len++;

    name = malloc( len * sizeof(WCHAR) );
    if (!name)
        return ERROR_OUTOFMEMORY;

    r = record_stream_name( tv, rec, name, &len );
    if (r != ERROR_SUCCESS)
    {
        free( name );
        return r;
    }

    *pstname = encode_streamname( FALSE, name );
    free( name );
    return ERROR_SUCCESS;
}

static MSIRECORD *get_transform_record( const struct table_view *tv, const string_table *st, IStorage *stg,
                                        const BYTE *rawdata, UINT bytes_per_strref )
{
    UINT i, val, ofs = 0;
    USHORT mask;
    struct column_info *columns = tv->columns;
    MSIRECORD *rec;

    mask = rawdata[0] | (rawdata[1] << 8);
    rawdata += 2;

    rec = MSI_CreateRecord( tv->num_cols );
    if( !rec )
        return rec;

    TRACE("row ->\n");
    for( i=0; i<tv->num_cols; i++ )
    {
        if ( (mask&1) && (i>=(mask>>8)) )
            break;
        /* all keys must be present */
        if ( (~mask&1) && (~columns[i].type & MSITYPE_KEY) && ((1<<i) & ~mask) )
            continue;

        if( MSITYPE_IS_BINARY(tv->columns[i].type) )
        {
            LPWSTR encname;
            IStream *stm = NULL;
            UINT r;

            ofs += bytes_per_column( tv->db, &columns[i], bytes_per_strref );

            r = record_encoded_stream_name( tv, rec, &encname );
            if ( r != ERROR_SUCCESS )
            {
                msiobj_release( &rec->hdr );
                return NULL;
            }
            r = IStorage_OpenStream( stg, encname, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm );
            if ( r != ERROR_SUCCESS )
            {
                msiobj_release( &rec->hdr );
                free( encname );
                return NULL;
            }

            MSI_RecordSetStream( rec, i+1, stm );
            TRACE(" field %d [%s]\n", i+1, debugstr_w(encname));
            free( encname );
        }
        else if( columns[i].type & MSITYPE_STRING )
        {
            int len;
            const WCHAR *sval;

            val = read_raw_int(rawdata, ofs, bytes_per_strref);
            sval = msi_string_lookup( st, val, &len );
            msi_record_set_string( rec, i+1, sval, len );
            TRACE(" field %d [%s]\n", i+1, debugstr_wn(sval, len));
            ofs += bytes_per_strref;
        }
        else
        {
            UINT n = bytes_per_column( tv->db, &columns[i], bytes_per_strref );
            switch( n )
            {
            case 2:
                val = read_raw_int(rawdata, ofs, n);
                if (val)
                    MSI_RecordSetInteger( rec, i+1, val-0x8000 );
                TRACE(" field %d [0x%04x]\n", i+1, val );
                break;
            case 4:
                val = read_raw_int(rawdata, ofs, n);
                if (val)
                    MSI_RecordSetInteger( rec, i+1, val^0x80000000 );
                TRACE(" field %d [0x%08x]\n", i+1, val );
                break;
            default:
                ERR("oops - unknown column width %d\n", n);
                break;
            }
            ofs += n;
        }
    }
    return rec;
}

static void dump_table( const string_table *st, const USHORT *rawdata, UINT rawsize )
{
    UINT i;
    for (i = 0; i < rawsize / 2; i++)
    {
        int len;
        const WCHAR *sval = msi_string_lookup( st, rawdata[i], &len );
        MESSAGE(" %04x %s\n", rawdata[i], debugstr_wn(sval, len) );
    }
}

static UINT *record_to_row( const struct table_view *tv, MSIRECORD *rec )
{
    UINT i, r, *data;

    data = malloc( tv->num_cols * sizeof (UINT) );
    for( i=0; i<tv->num_cols; i++ )
    {
        data[i] = 0;

        if ( ~tv->columns[i].type & MSITYPE_KEY )
            continue;

        /* turn the transform column value into a row value */
        if ( ( tv->columns[i].type & MSITYPE_STRING ) &&
             ! MSITYPE_IS_BINARY(tv->columns[i].type) )
        {
            int len;
            const WCHAR *str = msi_record_get_string( rec, i+1, &len );
            if (str)
            {
                r = msi_string2id( tv->db->strings, str, len, &data[i] );

                /* if there's no matching string in the string table,
                   these keys can't match any record, so fail now. */
                if (r != ERROR_SUCCESS)
                {
                    free( data );
                    return NULL;
                }
            }
            else data[i] = 0;
        }
        else
        {
            if (int_to_table_storage( tv, i + 1, MSI_RecordGetInteger( rec, i + 1 ), &data[i] ))
            {
                free( data );
                return NULL;
            }
        }
    }
    return data;
}

static UINT row_matches( struct table_view *tv, UINT row, const UINT *data, UINT *column )
{
    UINT i, r, x, ret = ERROR_FUNCTION_FAILED;

    for( i=0; i<tv->num_cols; i++ )
    {
        if ( ~tv->columns[i].type & MSITYPE_KEY )
            continue;

        /* turn the transform column value into a row value */
        r = TABLE_fetch_int( &tv->view, row, i+1, &x );
        if ( r != ERROR_SUCCESS )
        {
            ERR("TABLE_fetch_int shouldn't fail here\n");
            break;
        }

        /* if this key matches, move to the next column */
        if ( x != data[i] )
        {
            ret = ERROR_FUNCTION_FAILED;
            break;
        }
        if (column) *column = i;
        ret = ERROR_SUCCESS;
    }
    return ret;
}

static UINT table_find_row( struct table_view *tv, MSIRECORD *rec, UINT *row, UINT *column )
{
    UINT i, r = ERROR_FUNCTION_FAILED, *data;

    data = record_to_row( tv, rec );
    if( !data )
        return r;
    for( i = 0; i < tv->table->row_count; i++ )
    {
        r = row_matches( tv, i, data, column );
        if( r == ERROR_SUCCESS )
        {
            *row = i;
            break;
        }
    }
    free( data );
    return r;
}

struct transform_data
{
    struct list entry;
    LPWSTR name;
};

static UINT table_load_transform( MSIDATABASE *db, IStorage *stg, string_table *st, struct transform_data *transform,
                                  UINT bytes_per_strref, int err_cond )
{
    BYTE *rawdata = NULL;
    struct table_view *tv = NULL;
    UINT r, n, sz, i, mask, num_cols, colcol = 0, rawsize = 0;
    MSIRECORD *rec = NULL;
    WCHAR coltable[32];
    const WCHAR *name;

    if (!transform)
        return ERROR_SUCCESS;

    name = transform->name;

    coltable[0] = 0;
    TRACE("%p %p %p %s\n", db, stg, st, debugstr_w(name) );

    /* read the transform data */
    read_stream_data( stg, name, TRUE, &rawdata, &rawsize );
    if ( !rawdata )
    {
        TRACE("table %s empty\n", debugstr_w(name) );
        return ERROR_INVALID_TABLE;
    }

    /* create a table view */
    if ( err_cond & MSITRANSFORM_ERROR_VIEWTRANSFORM )
        r = TransformView_Create( db, st, name, (MSIVIEW**) &tv );
    else
        r = TABLE_CreateView( db, name, (MSIVIEW**) &tv );
    if( r != ERROR_SUCCESS )
        goto err;

    r = tv->view.ops->execute( &tv->view, NULL );
    if( r != ERROR_SUCCESS )
        goto err;

    TRACE("name = %s columns = %u row_size = %u raw size = %u\n",
          debugstr_w(name), tv->num_cols, tv->row_size, rawsize );

    /* interpret the data */
    for (n = 0; n < rawsize;)
    {
        mask = rawdata[n] | (rawdata[n + 1] << 8);
        if (mask & 1)
        {
            /*
             * if the low bit is set, columns are continuous and
             * the number of columns is specified in the high byte
             */
            sz = 2;
            num_cols = mask >> 8;
            if (num_cols > tv->num_cols)
            {
                ERR("excess columns in transform: %u > %u\n", num_cols, tv->num_cols);
                break;
            }

            for (i = 0; i < num_cols; i++)
            {
                if( (tv->columns[i].type & MSITYPE_STRING) &&
                    ! MSITYPE_IS_BINARY(tv->columns[i].type) )
                    sz += bytes_per_strref;
                else
                    sz += bytes_per_column( tv->db, &tv->columns[i], bytes_per_strref );
            }
        }
        else
        {
            /*
             * If the low bit is not set, mask is a bitmask.
             * Excepting for key fields, which are always present,
             *  each bit indicates that a field is present in the transform record.
             *
             * mask == 0 is a special case ... only the keys will be present
             * and it means that this row should be deleted.
             */
            sz = 2;
            num_cols = tv->num_cols;
            for (i = 0; i < num_cols; i++)
            {
                if ((tv->columns[i].type & MSITYPE_KEY) || ((1 << i) & mask))
                {
                    if ((tv->columns[i].type & MSITYPE_STRING) &&
                        !MSITYPE_IS_BINARY(tv->columns[i].type))
                        sz += bytes_per_strref;
                    else
                        sz += bytes_per_column( tv->db, &tv->columns[i], bytes_per_strref );
                }
            }
        }

        /* check we didn't run of the end of the table */
        if (n + sz > rawsize)
        {
            ERR("borked.\n");
            dump_table( st, (USHORT *)rawdata, rawsize );
            break;
        }

        rec = get_transform_record( tv, st, stg, &rawdata[n], bytes_per_strref );
        if (rec)
        {
            WCHAR table[32];
            DWORD sz = 32;
            UINT number = MSI_NULL_INTEGER;
            UINT row = 0;

            if (!wcscmp( name, L"_Columns" ))
            {
                MSI_RecordGetStringW( rec, 1, table, &sz );
                number = MSI_RecordGetInteger( rec, 2 );

                /*
                 * Native msi seems writes nul into the Number (2nd) column of
                 * the _Columns table when there are new columns
                 */
                if ( number == MSI_NULL_INTEGER )
                {
                    /* reset the column number on a new table */
                    if (wcscmp( coltable, table ))
                    {
                        colcol = 0;
                        lstrcpyW( coltable, table );
                    }

                    /* fix nul column numbers */
                    MSI_RecordSetInteger( rec, 2, ++colcol );
                }
            }

            if (TRACE_ON(msidb)) dump_record( rec );

            if (tv->table)
                r = table_find_row( tv, rec, &row, NULL );
            else
                r = ERROR_FUNCTION_FAILED;
            if (r == ERROR_SUCCESS)
            {
                if (!mask)
                {
                    TRACE("deleting row [%d]:\n", row);
                    r = tv->view.ops->delete_row( &tv->view, row );
                    if (r != ERROR_SUCCESS)
                        WARN("failed to delete row %u\n", r);
                }
                else if (mask & 1)
                {
                    TRACE("modifying full row [%d]:\n", row);
                    r = tv->view.ops->set_row( &tv->view, row, rec, (1 << tv->num_cols) - 1 );
                    if (r != ERROR_SUCCESS)
                        WARN("failed to modify row %u\n", r);
                }
                else
                {
                    TRACE("modifying masked row [%d]:\n", row);
                    r = tv->view.ops->set_row( &tv->view, row, rec, mask );
                    if (r != ERROR_SUCCESS)
                        WARN("failed to modify row %u\n", r);
                }
            }
            else
            {
                TRACE("inserting row\n");
                r = tv->view.ops->insert_row( &tv->view, rec, -1, FALSE );
                if (r != ERROR_SUCCESS)
                    WARN("failed to insert row %u\n", r);
            }

            if (!(err_cond & MSITRANSFORM_ERROR_VIEWTRANSFORM) && !wcscmp( name, L"_Columns" ))
                update_table_columns( db, table );

            msiobj_release( &rec->hdr );
        }

        n += sz;
    }

err:
    /* no need to free the table, it's associated with the database */
    free( rawdata );
    if( tv )
        tv->view.ops->delete( &tv->view );

    return ERROR_SUCCESS;
}

/*
 * msi_table_apply_transform
 *
 * Enumerate the table transforms in a transform storage and apply each one.
 */
UINT msi_table_apply_transform( MSIDATABASE *db, IStorage *stg, int err_cond )
{
    struct list transforms;
    IEnumSTATSTG *stgenum = NULL;
    struct transform_data *transform;
    struct transform_data *tables = NULL, *columns = NULL;
    HRESULT hr;
    STATSTG stat;
    string_table *strings;
    UINT ret = ERROR_FUNCTION_FAILED;
    UINT bytes_per_strref;
    BOOL property_update = FALSE;
    MSIVIEW *transform_view = NULL;

    TRACE("%p %p\n", db, stg );

    strings = msi_load_string_table( stg, &bytes_per_strref );
    if( !strings )
        goto end;

    hr = IStorage_EnumElements( stg, 0, NULL, 0, &stgenum );
    if (FAILED( hr ))
        goto end;

    list_init(&transforms);

    while ( TRUE )
    {
        struct table_view *tv = NULL;
        WCHAR name[0x40];
        ULONG count = 0;

        hr = IEnumSTATSTG_Next( stgenum, 1, &stat, &count );
        if (FAILED( hr ) || !count)
            break;

        decode_streamname( stat.pwcsName, name );
        CoTaskMemFree( stat.pwcsName );
        if ( name[0] != 0x4840 )
            continue;

        if ( !wcscmp( name+1, L"_StringPool" ) ||
             !wcscmp( name+1, L"_StringData" ) )
            continue;

        transform = calloc( 1, sizeof(*transform) );
        if ( !transform )
            break;

        list_add_tail( &transforms, &transform->entry );

        transform->name = wcsdup( name + 1 );

        if ( !wcscmp( transform->name, L"_Tables" ) )
            tables = transform;
        else if (!wcscmp( transform->name, L"_Columns" ) )
            columns = transform;
        else if (!wcscmp( transform->name, L"Property" ))
            property_update = TRUE;

        TRACE("transform contains stream %s\n", debugstr_w(name));

        /* load the table */
        if (TABLE_CreateView( db, transform->name, (MSIVIEW**) &tv ) != ERROR_SUCCESS)
            continue;

        if (tv->view.ops->execute( &tv->view, NULL ) != ERROR_SUCCESS)
        {
            tv->view.ops->delete( &tv->view );
            continue;
        }

        tv->view.ops->delete( &tv->view );
    }

    if (err_cond & MSITRANSFORM_ERROR_VIEWTRANSFORM)
    {
        static const WCHAR create_query[] = L"CREATE TABLE `_TransformView` ( "
            L"`Table` CHAR(0) NOT NULL TEMPORARY, `Column` CHAR(0) NOT NULL TEMPORARY, "
            L"`Row` CHAR(0) TEMPORARY, `Data` CHAR(0) TEMPORARY, `Current` CHAR(0) TEMPORARY "
            L"PRIMARY KEY `Table`, `Column`, `Row` ) HOLD";

        MSIQUERY *query;
        UINT r;

        r = MSI_DatabaseOpenViewW( db, create_query, &query );
        if (r != ERROR_SUCCESS)
            goto end;

        r = MSI_ViewExecute( query, NULL );
        if (r == ERROR_SUCCESS)
            MSI_ViewClose( query );
        msiobj_release( &query->hdr );
        if (r != ERROR_BAD_QUERY_SYNTAX && r != ERROR_SUCCESS)
            goto end;

        if (TABLE_CreateView(db, L"_TransformView", &transform_view) != ERROR_SUCCESS)
            goto end;

        if (r == ERROR_BAD_QUERY_SYNTAX)
            transform_view->ops->add_ref( transform_view );

        r = transform_view->ops->add_column( transform_view, L"new",
                MSITYPE_TEMPORARY | MSITYPE_NULLABLE | 0x402 /* INT */, FALSE );
        if (r != ERROR_SUCCESS)
            goto end;
    }

    /*
     * Apply _Tables and _Columns transforms first so that
     * the table metadata is correct, and empty tables exist.
     */
    ret = table_load_transform( db, stg, strings, tables, bytes_per_strref, err_cond );
    if (ret != ERROR_SUCCESS && ret != ERROR_INVALID_TABLE)
        goto end;

    ret = table_load_transform( db, stg, strings, columns, bytes_per_strref, err_cond );
    if (ret != ERROR_SUCCESS && ret != ERROR_INVALID_TABLE)
        goto end;

    ret = ERROR_SUCCESS;

    while ( !list_empty( &transforms ) )
    {
        transform = LIST_ENTRY( list_head( &transforms ), struct transform_data, entry );

        if ( wcscmp( transform->name, L"_Columns" ) &&
             wcscmp( transform->name, L"_Tables" ) &&
             ret == ERROR_SUCCESS )
        {
            ret = table_load_transform( db, stg, strings, transform, bytes_per_strref, err_cond );
        }

        list_remove( &transform->entry );
        free( transform->name );
        free( transform );
    }

    if ( ret == ERROR_SUCCESS )
    {
        append_storage_to_db( db, stg );
        if (property_update) msi_clone_properties( db );
    }

end:
    if ( stgenum )
        IEnumSTATSTG_Release( stgenum );
    if ( strings )
        msi_destroy_stringtable( strings );
    if (transform_view)
    {
        struct tagMSITABLE *table = ((struct table_view *)transform_view)->table;

        if (ret != ERROR_SUCCESS)
            transform_view->ops->release( transform_view );

        if (!wcscmp(table->colinfo[table->col_count - 1].colname, L"new"))
            TABLE_remove_column( transform_view, table->colinfo[table->col_count - 1].number );
        transform_view->ops->delete( transform_view );
    }

    return ret;
}
