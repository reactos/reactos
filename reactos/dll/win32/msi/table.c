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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

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
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define MSITABLE_HASH_TABLE_SIZE 37
#define LONG_STR_BYTES 3

typedef struct tagMSICOLUMNHASHENTRY
{
    struct tagMSICOLUMNHASHENTRY *next;
    UINT value;
    UINT row;
} MSICOLUMNHASHENTRY;

typedef struct tagMSICOLUMNINFO
{
    LPWSTR tablename;
    UINT   number;
    LPWSTR colname;
    UINT   type;
    UINT   offset;
    INT    ref_count;
    MSICOLUMNHASHENTRY **hash_table;
} MSICOLUMNINFO;

typedef struct tagMSIORDERINFO
{
    UINT *reorder;
    UINT num_cols;
    UINT cols[1];
} MSIORDERINFO;

struct tagMSITABLE
{
    BYTE **data;
    UINT row_count;
    BYTE **nonpersistent_data;
    UINT nonpersistent_row_count;
    struct list entry;
    MSICOLUMNINFO *colinfo;
    UINT col_count;
    BOOL persistent;
    INT ref_count;
    WCHAR name[1];
};

typedef struct tagMSITRANSFORM {
    struct list entry;
    IStorage *stg;
} MSITRANSFORM;

static const WCHAR szStringData[] = {
    '_','S','t','r','i','n','g','D','a','t','a',0 };
static const WCHAR szStringPool[] = {
    '_','S','t','r','i','n','g','P','o','o','l',0 };

/* information for default tables */
static WCHAR szTables[]  = { '_','T','a','b','l','e','s',0 };
static WCHAR szTable[]  = { 'T','a','b','l','e',0 };
static WCHAR szName[]    = { 'N','a','m','e',0 };
static WCHAR szColumns[] = { '_','C','o','l','u','m','n','s',0 };
static WCHAR szNumber[]  = { 'N','u','m','b','e','r',0 };
static WCHAR szType[]    = { 'T','y','p','e',0 };

static const MSICOLUMNINFO _Columns_cols[4] = {
    { szColumns, 1, szTable,  MSITYPE_VALID | MSITYPE_STRING | MSITYPE_KEY | 64, 0, 0, NULL },
    { szColumns, 2, szNumber, MSITYPE_VALID | MSITYPE_KEY | 2,     2, 0, NULL },
    { szColumns, 3, szName,   MSITYPE_VALID | MSITYPE_STRING | 64, 4, 0, NULL },
    { szColumns, 4, szType,   MSITYPE_VALID | 2,                   6, 0, NULL },
};

static const MSICOLUMNINFO _Tables_cols[1] = {
    { szTables,  1, szName,   MSITYPE_VALID | MSITYPE_STRING | MSITYPE_KEY | 64, 0, 0, NULL },
};

#define MAX_STREAM_NAME 0x1f

static UINT table_get_column_info( MSIDATABASE *db, LPCWSTR name,
       MSICOLUMNINFO **pcols, UINT *pcount );
static void table_calc_column_offsets( MSIDATABASE *db, MSICOLUMNINFO *colinfo,
       DWORD count );
static UINT get_tablecolumns( MSIDATABASE *db,
       LPCWSTR szTableName, MSICOLUMNINFO *colinfo, UINT *sz);
static void msi_free_colinfo( MSICOLUMNINFO *colinfo, UINT count );

static inline UINT bytes_per_column( MSIDATABASE *db, const MSICOLUMNINFO *col )
{
    if( MSITYPE_IS_BINARY(col->type) )
        return 2;
    if( col->type & MSITYPE_STRING )
        return db->bytes_per_strref;
    if( (col->type & 0xff) > 4 )
        ERR("Invalid column size!\n");
    return col->type & 0xff;
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
    out = msi_alloc( count*sizeof(WCHAR) );
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
    msi_free( out );
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
        TRACE("stream %2d -> %s %s\n", n,
              debugstr_w(stat.pwcsName), debugstr_w(name) );
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
    msi_free( encname );
    if( FAILED( r ) )
    {
        WARN("open stream failed r = %08x - empty table?\n", r);
        return ret;
    }

    r = IStream_Stat(stm, &stat, STATFLAG_NONAME );
    if( FAILED( r ) )
    {
        WARN("open stream failed r = %08x!\n", r);
        goto end;
    }

    if( stat.cbSize.QuadPart >> 32 )
    {
        WARN("Too big!\n");
        goto end;
    }
        
    sz = stat.cbSize.QuadPart;
    data = msi_alloc( sz );
    if( !data )
    {
        WARN("couldn't allocate memory r=%08x!\n", r);
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }
        
    r = IStream_Read(stm, data, sz, &count );
    if( FAILED( r ) || ( count != sz ) )
    {
        msi_free( data );
        WARN("read stream failed r = %08x!\n", r);
        goto end;
    }

    *pdata = data;
    *psz = sz;
    ret = ERROR_SUCCESS;

end:
    IStream_Release( stm );

    return ret;
}

UINT db_get_raw_stream( MSIDATABASE *db, LPCWSTR stname, IStream **stm )
{
    LPWSTR encname;
    HRESULT r;

    encname = encode_streamname(FALSE, stname);

    TRACE("%s -> %s\n",debugstr_w(stname),debugstr_w(encname));

    r = IStorage_OpenStream(db->storage, encname, NULL, 
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, stm);
    if( FAILED( r ) )
    {
        MSITRANSFORM *transform;

        LIST_FOR_EACH_ENTRY( transform, &db->transforms, MSITRANSFORM, entry )
        {
            TRACE("looking for %s in transform storage\n", debugstr_w(stname) );
            r = IStorage_OpenStream( transform->stg, encname, NULL, 
                    STGM_READ | STGM_SHARE_EXCLUSIVE, 0, stm );
            if (SUCCEEDED(r))
                break;
        }
    }

    msi_free( encname );

    return SUCCEEDED(r) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

UINT read_raw_stream_data( MSIDATABASE *db, LPCWSTR stname,
                              USHORT **pdata, UINT *psz )
{
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;
    VOID *data;
    ULONG sz, count;
    IStream *stm = NULL;
    STATSTG stat;

    r = db_get_raw_stream( db, stname, &stm );
    if( r != ERROR_SUCCESS)
        return ret;
    r = IStream_Stat(stm, &stat, STATFLAG_NONAME );
    if( FAILED( r ) )
    {
        WARN("open stream failed r = %08x!\n", r);
        goto end;
    }

    if( stat.cbSize.QuadPart >> 32 )
    {
        WARN("Too big!\n");
        goto end;
    }
        
    sz = stat.cbSize.QuadPart;
    data = msi_alloc( sz );
    if( !data )
    {
        WARN("couldn't allocate memory r=%08x!\n", r);
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }
        
    r = IStream_Read(stm, data, sz, &count );
    if( FAILED( r ) || ( count != sz ) )
    {
        msi_free( data );
        WARN("read stream failed r = %08x!\n", r);
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
    msi_free( encname );
    if( FAILED( r ) )
    {
        WARN("open stream failed r = %08x\n", r);
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

static void free_table( MSITABLE *table )
{
    UINT i;
    for( i=0; i<table->row_count; i++ )
        msi_free( table->data[i] );
    msi_free( table->data );
    for( i=0; i<table->nonpersistent_row_count; i++ )
        msi_free( table->nonpersistent_data[i] );
    msi_free( table->nonpersistent_data );
    msi_free_colinfo( table->colinfo, table->col_count );
    msi_free( table->colinfo );
    msi_free( table );
}

static UINT msi_table_get_row_size( MSIDATABASE *db,const MSICOLUMNINFO *cols,
                                    UINT count )
{
    const MSICOLUMNINFO *last_col = &cols[count-1];
    if (!count)
        return 0;
    return last_col->offset + bytes_per_column( db, last_col );
}

/* add this table to the list of cached tables in the database */
static UINT read_table_from_storage( MSIDATABASE *db, MSITABLE *t, IStorage *stg )
{
    BYTE *rawdata = NULL;
    UINT rawsize = 0, i, j, row_size = 0;

    TRACE("%s\n",debugstr_w(t->name));

    row_size = msi_table_get_row_size( db, t->colinfo, t->col_count );

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

    t->row_count = rawsize / row_size;
    t->data = msi_alloc_zero( t->row_count * sizeof (USHORT*) );
    if( !t->data )
        goto err;

    /* transpose all the data */
    TRACE("Transposing data from %d rows\n", t->row_count );
    for( i=0; i<t->row_count; i++ )
    {
        t->data[i] = msi_alloc( row_size );
        if( !t->data[i] )
            goto err;

        for( j=0; j<t->col_count; j++ )
        {
            UINT ofs = t->colinfo[j].offset;
            UINT n = bytes_per_column( db, &t->colinfo[j] );
            UINT k;

            if ( n != 2 && n != 3 && n != 4 )
            {
                ERR("oops - unknown column width %d\n", n);
                goto err;
            }

            for ( k = 0; k < n; k++ )
                t->data[i][ofs + k] = rawdata[ofs*t->row_count + i * n + k];
        }
    }

    msi_free( rawdata );
    return ERROR_SUCCESS;
err:
    msi_free( rawdata );
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
        if( !lstrcmpW( name, t->name ) )
            return t;

    return NULL;
}

static UINT table_get_column_info( MSIDATABASE *db, LPCWSTR name, MSICOLUMNINFO **pcols, UINT *pcount )
{
    UINT r, column_count = 0;
    MSICOLUMNINFO *columns;

    /* get the number of columns in this table */
    column_count = 0;
    r = get_tablecolumns( db, name, NULL, &column_count );
    if( r != ERROR_SUCCESS )
        return r;

    *pcount = column_count;

    /* if there's no columns, there's no table */
    if( column_count == 0 )
        return ERROR_INVALID_PARAMETER;

    TRACE("Table %s found\n", debugstr_w(name) );

    columns = msi_alloc( column_count*sizeof (MSICOLUMNINFO) );
    if( !columns )
        return ERROR_FUNCTION_FAILED;

    r = get_tablecolumns( db, name, columns, &column_count );
    if( r != ERROR_SUCCESS )
    {
        msi_free( columns );
        return ERROR_FUNCTION_FAILED;
    }

    *pcols = columns;

    return r;
}

UINT msi_create_table( MSIDATABASE *db, LPCWSTR name, column_info *col_info,
                       BOOL persistent, MSITABLE **table_ret)
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

    table = msi_alloc( sizeof (MSITABLE) + lstrlenW(name)*sizeof (WCHAR) );
    if( !table )
        return ERROR_FUNCTION_FAILED;

    table->ref_count = 1;
    table->row_count = 0;
    table->data = NULL;
    table->nonpersistent_row_count = 0;
    table->nonpersistent_data = NULL;
    table->colinfo = NULL;
    table->col_count = 0;
    table->persistent = persistent;
    lstrcpyW( table->name, name );

    for( col = col_info; col; col = col->next )
        table->col_count++;

    table->colinfo = msi_alloc( table->col_count * sizeof(MSICOLUMNINFO) );
    if (!table->colinfo)
    {
        free_table( table );
        return ERROR_FUNCTION_FAILED;
    }

    for( i = 0, col = col_info; col; i++, col = col->next )
    {
        table->colinfo[ i ].tablename = strdupW( col->table );
        table->colinfo[ i ].number = i + 1;
        table->colinfo[ i ].colname = strdupW( col->column );
        table->colinfo[ i ].type = col->type;
        table->colinfo[ i ].offset = 0;
        table->colinfo[ i ].ref_count = 0;
        table->colinfo[ i ].hash_table = NULL;
    }
    table_calc_column_offsets( db, table->colinfo, table->col_count);

    r = TABLE_CreateView( db, szTables, &tv );
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

    r = tv->ops->insert_row( tv, rec, !persistent );
    TRACE("insert_row returned %x\n", r);
    if( r )
        goto err;

    tv->ops->delete( tv );
    tv = NULL;

    msiobj_release( &rec->hdr );
    rec = NULL;

    if( persistent )
    {
        /* add each column to the _Columns table */
        r = TABLE_CreateView( db, szColumns, &tv );
        if( r )
            return r;

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

            r = tv->ops->insert_row( tv, rec, FALSE );
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
    {
        list_add_head( &db->tables, &table->entry );
        *table_ret = table;
    }
    else
        free_table( table );

    return r;
}

static UINT get_table( MSIDATABASE *db, LPCWSTR name, MSITABLE **table_ret )
{
    MSITABLE *table;
    UINT r;

    /* first, see if the table is cached */
    table = find_cached_table( db, name );
    if( table )
    {
        *table_ret = table;
        return ERROR_SUCCESS;
    }

    /* nonexistent tables should be interpreted as empty tables */
    table = msi_alloc( sizeof (MSITABLE) + lstrlenW(name)*sizeof (WCHAR) );
    if( !table )
        return ERROR_FUNCTION_FAILED;

    table->row_count = 0;
    table->data = NULL;
    table->nonpersistent_row_count = 0;
    table->nonpersistent_data = NULL;
    table->colinfo = NULL;
    table->col_count = 0;
    table->persistent = TRUE;
    lstrcpyW( table->name, name );

    r = table_get_column_info( db, name, &table->colinfo, &table->col_count);
    if (r != ERROR_SUCCESS)
    {
        free_table ( table );
        return r;
    }

    r = read_table_from_storage( db, table, db->storage );
    if( r != ERROR_SUCCESS )
    {
        free_table( table );
        return r;
    }

    list_add_head( &db->tables, &table->entry );
    *table_ret = table;
    return ERROR_SUCCESS;
}

static UINT save_table( MSIDATABASE *db, const MSITABLE *t )
{
    BYTE *rawdata = NULL, *p;
    UINT rawsize, r, i, j, row_size;

    /* Nothing to do for non-persistent tables */
    if( !t->persistent )
        return ERROR_SUCCESS;

    TRACE("Saving %s\n", debugstr_w( t->name ) );

    row_size = msi_table_get_row_size( db, t->colinfo, t->col_count );

    rawsize = t->row_count * row_size;
    rawdata = msi_alloc_zero( rawsize );
    if( !rawdata )
    {
        r = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }

    p = rawdata;
    for( i=0; i<t->col_count; i++ )
    {
        for( j=0; j<t->row_count; j++ )
        {
            UINT offset = t->colinfo[i].offset;

            *p++ = t->data[j][offset];
            *p++ = t->data[j][offset + 1];
            if( 4 == bytes_per_column( db, &t->colinfo[i] ) )
            {
                *p++ = t->data[j][offset + 2];
                *p++ = t->data[j][offset + 3];
            }
        }
    }

    TRACE("writing %d bytes\n", rawsize);
    r = write_stream_data( db->storage, t->name, rawdata, rawsize, TRUE );

err:
    msi_free( rawdata );

    return r;
}

static void table_calc_column_offsets( MSIDATABASE *db, MSICOLUMNINFO *colinfo,
                                       DWORD count )
{
    DWORD i;

    for( i=0; colinfo && (i<count); i++ )
    {
         assert( (i+1) == colinfo[ i ].number );
         if (i)
             colinfo[i].offset = colinfo[ i - 1 ].offset
                               + bytes_per_column( db, &colinfo[ i - 1 ] );
         else
             colinfo[i].offset = 0;
         TRACE("column %d is [%s] with type %08x ofs %d\n",
               colinfo[i].number, debugstr_w(colinfo[i].colname),
               colinfo[i].type, colinfo[i].offset);
    }
}

static UINT get_defaulttablecolumns( MSIDATABASE *db, LPCWSTR name,
                                     MSICOLUMNINFO *colinfo, UINT *sz)
{
    const MSICOLUMNINFO *p;
    DWORD i, n;

    TRACE("%s\n", debugstr_w(name));

    if (!lstrcmpW( name, szTables ))
    {
        p = _Tables_cols;
        n = 1;
    }
    else if (!lstrcmpW( name, szColumns ))
    {
        p = _Columns_cols;
        n = 4;
    }
    else
        return ERROR_FUNCTION_FAILED;

    /* duplicate the string data so we can free it in msi_free_colinfo */
    for (i=0; i<n; i++)
    {
        if (colinfo && (i < *sz) )
        {
            colinfo[i] = p[i];
            colinfo[i].tablename = strdupW( p[i].tablename );
            colinfo[i].colname = strdupW( p[i].colname );
        }
        if( colinfo && (i >= *sz) )
            break;
    }
    table_calc_column_offsets( db, colinfo, n );
    *sz = n;
    return ERROR_SUCCESS;
}

static void msi_free_colinfo( MSICOLUMNINFO *colinfo, UINT count )
{
    UINT i;

    for( i=0; i<count; i++ )
    {
        msi_free( colinfo[i].tablename );
        msi_free( colinfo[i].colname );
        msi_free( colinfo[i].hash_table );
    }
}

static LPWSTR msi_makestring( const MSIDATABASE *db, UINT stringid)
{
    return strdupW(msi_string_lookup_id( db->strings, stringid ));
}

static UINT read_table_int(BYTE *const *data, UINT row, UINT col, UINT bytes)
{
    UINT ret = 0, i;

    for (i = 0; i < bytes; i++)
        ret += (data[row][col + i] << i * 8);

    return ret;
}

static UINT get_tablecolumns( MSIDATABASE *db,
       LPCWSTR szTableName, MSICOLUMNINFO *colinfo, UINT *sz)
{
    UINT r, i, n=0, table_id, count, maxcount = *sz;
    MSITABLE *table = NULL;

    TRACE("%s\n", debugstr_w(szTableName));

    /* first check if there is a default table with that name */
    r = get_defaulttablecolumns( db, szTableName, colinfo, sz );
    if( ( r == ERROR_SUCCESS ) && *sz )
        return r;

    r = get_table( db, szColumns, &table );
    if( r != ERROR_SUCCESS )
    {
        ERR("couldn't load _Columns table\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* convert table and column names to IDs from the string table */
    r = msi_string2idW( db->strings, szTableName, &table_id );
    if( r != ERROR_SUCCESS )
    {
        WARN("Couldn't find id for %s\n", debugstr_w(szTableName));
        return r;
    }

    TRACE("Table id is %d, row count is %d\n", table_id, table->row_count);

    /* Note: _Columns table doesn't have non-persistent data */

    /* if maxcount is non-zero, assume it's exactly right for this table */
    memset( colinfo, 0, maxcount*sizeof(*colinfo) );
    count = table->row_count;
    for( i=0; i<count; i++ )
    {
        if( read_table_int(table->data, i, 0, db->bytes_per_strref) != table_id )
            continue;
        if( colinfo )
        {
            UINT id = read_table_int(table->data, i, table->colinfo[2].offset, db->bytes_per_strref);
            UINT col = read_table_int(table->data, i, table->colinfo[1].offset, sizeof(USHORT)) - (1<<15);

            /* check the column number is in range */
            if (col<1 || col>maxcount)
            {
                ERR("column %d out of range\n", col);
                continue;
            }

            /* check if this column was already set */
            if (colinfo[ col - 1 ].number)
            {
                ERR("duplicate column %d\n", col);
                continue;
            }

            colinfo[ col - 1 ].tablename = msi_makestring( db, table_id );
            colinfo[ col - 1 ].number = col;
            colinfo[ col - 1 ].colname = msi_makestring( db, id );
            colinfo[ col - 1 ].type = read_table_int(table->data, i,
                                                     table->colinfo[3].offset,
                                                     sizeof(USHORT)) - (1<<15);
            colinfo[ col - 1 ].offset = 0;
            colinfo[ col - 1 ].ref_count = 0;
            colinfo[ col - 1 ].hash_table = NULL;
        }
        n++;
    }

    TRACE("%s has %d columns\n", debugstr_w(szTableName), n);

    if (colinfo && n != maxcount)
    {
        ERR("missing column in table %s\n", debugstr_w(szTableName));
        msi_free_colinfo(colinfo, maxcount );
        return ERROR_FUNCTION_FAILED;
    }

    table_calc_column_offsets( db, colinfo, n );
    *sz = n;

    return ERROR_SUCCESS;
}

static void msi_update_table_columns( MSIDATABASE *db, LPCWSTR name )
{
    MSITABLE *table;
    UINT size, offset, old_count;
    UINT n;

    table = find_cached_table( db, name );
    old_count = table->col_count;
    msi_free( table->colinfo );
    table_get_column_info( db, name, &table->colinfo, &table->col_count );

    if (!table->col_count)
        return;

    size = msi_table_get_row_size( db, table->colinfo, table->col_count );
    offset = table->colinfo[table->col_count - 1].offset;

    for ( n = 0; n < table->row_count; n++ )
    {
        table->data[n] = msi_realloc( table->data[n], size );
        if (old_count < table->col_count)
            memset( &table->data[n][offset], 0, size - offset );
    }
}

/* try to find the table name in the _Tables table */
BOOL TABLE_Exists( MSIDATABASE *db, LPCWSTR name )
{
    UINT r, table_id = 0, i, count;
    MSITABLE *table = NULL;

    if( !lstrcmpW( name, szTables ) )
        return TRUE;
    if( !lstrcmpW( name, szColumns ) )
        return TRUE;

    r = msi_string2idW( db->strings, name, &table_id );
    if( r != ERROR_SUCCESS )
    {
        TRACE("Couldn't find id for %s\n", debugstr_w(name));
        return FALSE;
    }

    r = get_table( db, szTables, &table );
    if( r != ERROR_SUCCESS )
    {
        ERR("table %s not available\n", debugstr_w(szTables));
        return FALSE;
    }

    count = table->row_count;
    for( i=0; i<count; i++ )
        if( table->data[ i ][ 0 ] == table_id )
            return TRUE;

    count = table->nonpersistent_row_count;
    for( i=0; i<count; i++ )
        if( table->nonpersistent_data[ i ][ 0 ] == table_id )
            return TRUE;

    return FALSE;
}

/* below is the query interface to a table */

typedef struct tagMSITABLEVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSITABLE      *table;
    MSICOLUMNINFO *columns;
    MSIORDERINFO  *order;
    UINT           num_cols;
    UINT           row_size;
    WCHAR          name[1];
} MSITABLEVIEW;

static UINT TABLE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT offset, n;
    BYTE **data;

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col>tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    /* how many rows are there ? */
    if( row >= tv->table->row_count + tv->table->nonpersistent_row_count )
        return ERROR_NO_MORE_ITEMS;

    if( tv->columns[col-1].offset >= tv->row_size )
    {
        ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
        ERR("%p %p\n", tv, tv->columns );
        return ERROR_FUNCTION_FAILED;
    }

    if (tv->order)
        row = tv->order->reorder[row];

    if (row >= tv->table->row_count)
    {
        row -= tv->table->row_count;
        data = tv->table->nonpersistent_data;
    }
    else
        data = tv->table->data;

    n = bytes_per_column( tv->db, &tv->columns[col-1] );
    if (n != 2 && n != 3 && n != 4)
    {
        ERR("oops! what is %d bytes per column?\n", n );
        return ERROR_FUNCTION_FAILED;
    }

    offset = tv->columns[col-1].offset;
    *val = read_table_int(data, row, offset, n);

    /* TRACE("Data [%d][%d] = %d\n", row, col, *val ); */

    return ERROR_SUCCESS;
}

/*
 * We need a special case for streams, as we need to reference column with
 * the name of the stream in the same table, and the table name
 * which may not be available at higher levels of the query
 */
static UINT TABLE_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT ival = 0, refcol = 0, r;
    LPCWSTR sval;
    LPWSTR full_name;
    DWORD len;
    static const WCHAR szDot[] = { '.', 0 };
    WCHAR number[0x20];

    if( !view->ops->fetch_int )
        return ERROR_INVALID_PARAMETER;

    /*
     * The column marked with the type stream data seems to have a single number
     * which references the column containing the name of the stream data
     *
     * Fetch the column to reference first.
     */
    r = view->ops->fetch_int( view, row, col, &ival );
    if( r != ERROR_SUCCESS )
        return r;

    /* check the column value is in range */
    if (ival > tv->num_cols || ival == col)
    {
        ERR("bad column ref (%u) for stream\n", ival);
        return ERROR_FUNCTION_FAILED;
    }

    if ( tv->columns[ival - 1].type & MSITYPE_STRING )
    {
        /* now get the column with the name of the stream */
        r = view->ops->fetch_int( view, row, ival, &refcol );
        if ( r != ERROR_SUCCESS )
            return r;

        /* lookup the string value from the string table */
        sval = msi_string_lookup_id( tv->db->strings, refcol );
        if ( !sval )
            return ERROR_INVALID_PARAMETER;
    }
    else
    {
        static const WCHAR fmt[] = { '%','d',0 };
        sprintfW( number, fmt, ival );
        sval = number;
    }

    len = lstrlenW( tv->name ) + 2 + lstrlenW( sval );
    full_name = msi_alloc( len*sizeof(WCHAR) );
    lstrcpyW( full_name, tv->name );
    lstrcatW( full_name, szDot );
    lstrcatW( full_name, sval );

    r = db_get_raw_stream( tv->db, full_name, stm );
    if( r )
        ERR("fetching stream %s, error = %d\n",debugstr_w(full_name), r);
    msi_free( full_name );

    return r;
}

static UINT TABLE_set_int( MSITABLEVIEW *tv, UINT row, UINT col, UINT val )
{
    UINT offset, n, i;
    BYTE **data;

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col>tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    if( row >= tv->table->row_count + tv->table->nonpersistent_row_count )
        return ERROR_INVALID_PARAMETER;

    if( tv->columns[col-1].offset >= tv->row_size )
    {
        ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
        ERR("%p %p\n", tv, tv->columns );
        return ERROR_FUNCTION_FAILED;
    }

    msi_free( tv->columns[col-1].hash_table );
    tv->columns[col-1].hash_table = NULL;

    if (row >= tv->table->row_count)
    {
        row -= tv->table->row_count;
        data = tv->table->nonpersistent_data;
    }
    else
        data = tv->table->data;

    n = bytes_per_column( tv->db, &tv->columns[col-1] );
    if ( n != 2 && n != 3 && n != 4 )
    {
        ERR("oops! what is %d bytes per column?\n", n );
        return ERROR_FUNCTION_FAILED;
    }

    offset = tv->columns[col-1].offset;
    for ( i = 0; i < n; i++ )
        data[row][offset + i] = (val >> i * 8) & 0xff;

    return ERROR_SUCCESS;
}

static UINT TABLE_get_row( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    if (tv->order)
        row = tv->order->reorder[row];

    return msi_view_get_row(tv->db, view, row, rec);
}

static UINT TABLE_set_row( struct tagMSIVIEW *view, UINT row, MSIRECORD *rec, UINT mask )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
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

        /* if row >= tv->table->row_count then it is a non-persistent row */
        persistent = tv->table->persistent && (row < tv->table->row_count);
        /* FIXME: should we allow updating keys? */

        val = 0;
        if ( !MSI_RecordIsNull( rec, i + 1 ) )
        {
            if ( MSITYPE_IS_BINARY(tv->columns[ i ].type) )
            {
                val = 1; /* refers to the first key column */
            }
            else if ( tv->columns[i].type & MSITYPE_STRING )
            {
                LPCWSTR sval = MSI_RecordGetString( rec, i + 1 );
                UINT ival, x;

                r = msi_string2idW(tv->db->strings, sval, &ival);
                if (r == ERROR_SUCCESS)
                {
                    TABLE_fetch_int(&tv->view, row, i + 1, &x);
                    if (ival == x)
                        continue;
                }

                val = msi_addstringW( tv->db->strings, 0, sval, -1, 1,
                                      persistent ? StringPersistent : StringNonPersistent );
            }
            else if ( 2 == bytes_per_column( tv->db, &tv->columns[ i ] ) )
            {
                val = 0x8000 + MSI_RecordGetInteger( rec, i + 1 );
                if ( val & 0xffff0000 )
                {
                    ERR("field %u value %d out of range\n", i+1, val - 0x8000 );
                    return ERROR_FUNCTION_FAILED;
                }
            }
            else
            {
                INT ival = MSI_RecordGetInteger( rec, i + 1 );
                val = ival ^ 0x80000000;
            }
        }

        r = TABLE_set_int( tv, row, i+1, val );
        if ( r != ERROR_SUCCESS )
            break;
    }
    return r;
}

static UINT table_create_new_row( struct tagMSIVIEW *view, UINT *num, BOOL temporary )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    BYTE **p, *row;
    UINT sz;
    BYTE ***data_ptr;
    UINT *row_count;

    TRACE("%p %s\n", view, temporary ? "TRUE" : "FALSE");

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    row = msi_alloc_zero( tv->row_size );
    if( !row )
        return ERROR_NOT_ENOUGH_MEMORY;

    if( temporary )
    {
        row_count = &tv->table->nonpersistent_row_count;
        data_ptr = &tv->table->nonpersistent_data;
        *num = tv->table->row_count + tv->table->nonpersistent_row_count;
    }
    else
    {
        row_count = &tv->table->row_count;
        data_ptr = &tv->table->data;
        *num = tv->table->row_count;
    }

    sz = (*row_count + 1) * sizeof (BYTE*);
    if( *data_ptr )
        p = msi_realloc( *data_ptr, sz );
    else
        p = msi_alloc( sz );
    if( !p )
    {
        msi_free( row );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *data_ptr = p;
    (*data_ptr)[*row_count] = row;
    (*row_count)++;

    return ERROR_SUCCESS;
}

static UINT TABLE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

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
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p %p %p\n", view, rows, cols );

    if( cols )
        *cols = tv->num_cols;
    if( rows )
    {
        if( !tv->table )
            return ERROR_INVALID_PARAMETER;
        *rows = tv->table->row_count + tv->table->nonpersistent_row_count;
    }

    return ERROR_SUCCESS;
}

static UINT TABLE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p %d %p %p\n", tv, n, name, type );

    if( ( n == 0 ) || ( n > tv->num_cols ) )
        return ERROR_INVALID_PARAMETER;

    if( name )
    {
        *name = strdupW( tv->columns[n-1].colname );
        if( !*name )
            return ERROR_FUNCTION_FAILED;
    }
    if( type )
        *type = tv->columns[n-1].type;

    return ERROR_SUCCESS;
}

static UINT msi_table_find_row( MSITABLEVIEW *tv, MSIRECORD *rec, UINT *row );

static UINT table_validate_new( MSITABLEVIEW *tv, MSIRECORD *rec )
{
    UINT r, row, i;

    /* check there's no null values where they're not allowed */
    for( i = 0; i < tv->num_cols; i++ )
    {
        if ( tv->columns[i].type & MSITYPE_NULLABLE )
            continue;

        if ( MSITYPE_IS_BINARY(tv->columns[i].type) )
            TRACE("skipping binary column\n");
        else if ( tv->columns[i].type & MSITYPE_STRING )
        {
            LPCWSTR str;

            str = MSI_RecordGetString( rec, i+1 );
            if (str == NULL || str[0] == 0)
                return ERROR_INVALID_DATA;
        }
        else
        {
            UINT n;

            n = MSI_RecordGetInteger( rec, i+1 );
            if (n == MSI_NULL_INTEGER)
                return ERROR_INVALID_DATA;
        }
    }

    /* check there's no duplicate keys */
    r = msi_table_find_row( tv, rec, &row );
    if (r == ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

static UINT TABLE_insert_row( struct tagMSIVIEW *view, MSIRECORD *rec, BOOL temporary )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT r, row = -1;

    TRACE("%p %p %s\n", tv, rec, temporary ? "TRUE" : "FALSE" );

    /* check that the key is unique - can we find a matching row? */
    r = table_validate_new( tv, rec );
    if( r != ERROR_SUCCESS )
        return ERROR_FUNCTION_FAILED;

    r = table_create_new_row( view, &row, temporary );
    TRACE("insert_row returned %08x\n", r);
    if( r != ERROR_SUCCESS )
        return r;

    return TABLE_set_row( view, row, rec, (1<<tv->num_cols) - 1 );
}

static UINT TABLE_delete_row( struct tagMSIVIEW *view, UINT row )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT r, num_rows, num_cols, i;
    BYTE **data;

    TRACE("%p %d\n", tv, row);

    if ( !tv->table )
        return ERROR_INVALID_PARAMETER;

    r = TABLE_get_dimensions( view, &num_rows, &num_cols );
    if ( r != ERROR_SUCCESS )
        return r;

    if ( row >= num_rows )
        return ERROR_FUNCTION_FAILED;

    if ( row < tv->table->row_count )
    {
        num_rows = tv->table->row_count;
        tv->table->row_count--;
        data = tv->table->data;
    }
    else
    {
        num_rows = tv->table->nonpersistent_row_count;
        row -= tv->table->row_count;
        tv->table->nonpersistent_row_count--;
        data = tv->table->nonpersistent_data;
    }

    /* reset the hash tables */
    for (i = 0; i < tv->num_cols; i++)
    {
        msi_free( tv->columns[i].hash_table );
        tv->columns[i].hash_table = NULL;
    }

    if ( row == num_rows - 1 )
        return ERROR_SUCCESS;

    for (i = row + 1; i < num_rows; i++)
        memcpy(data[i - 1], data[i], tv->row_size);

    return ERROR_SUCCESS;
}

static UINT msi_table_update(struct tagMSIVIEW *view, MSIRECORD *rec, UINT row)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;
    UINT r, new_row;

    /* FIXME: MsiViewFetch should set rec index 0 to some ID that
     * sets the fetched record apart from other records
     */

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    r = msi_table_find_row(tv, rec, &new_row);
    if (r != ERROR_SUCCESS)
    {
        ERR("can't find row to modify\n");
        return ERROR_FUNCTION_FAILED;
    }

    /* the row cannot be changed */
    if (row != new_row + 1)
        return ERROR_FUNCTION_FAILED;

    return TABLE_set_row(view, new_row, rec, (1 << tv->num_cols) - 1);
}

static UINT modify_delete_row( struct tagMSIVIEW *view, MSIRECORD *rec )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;
    UINT row, r;

    r = msi_table_find_row(tv, rec, &row);
    if (r != ERROR_SUCCESS)
        return r;

    return TABLE_delete_row(view, row);
}

static UINT msi_refresh_record( struct tagMSIVIEW *view, MSIRECORD *rec, UINT row )
{
    MSIRECORD *curr;
    UINT r, i, count;

    r = TABLE_get_row(view, row - 1, &curr);
    if (r != ERROR_SUCCESS)
        return r;

    count = MSI_RecordGetFieldCount(rec);
    for (i = 0; i < count; i++)
        MSI_RecordCopyField(curr, i + 1, rec, i + 1);

    msiobj_release(&curr->hdr);
    return ERROR_SUCCESS;
}

static UINT TABLE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                          MSIRECORD *rec, UINT row)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT r;

    TRACE("%p %d %p\n", view, eModifyMode, rec );

    switch (eModifyMode)
    {
    case MSIMODIFY_DELETE:
        r = modify_delete_row( view, rec );
        break;
    case MSIMODIFY_VALIDATE_NEW:
        r = table_validate_new( tv, rec );
        break;

    case MSIMODIFY_INSERT:
        r = table_validate_new( tv, rec );
        if (r != ERROR_SUCCESS)
            break;
        r = TABLE_insert_row( view, rec, FALSE );
        break;

    case MSIMODIFY_INSERT_TEMPORARY:
        r = table_validate_new( tv, rec );
        if (r != ERROR_SUCCESS)
            break;
        r = TABLE_insert_row( view, rec, TRUE );
        break;

    case MSIMODIFY_REFRESH:
        r = msi_refresh_record( view, rec, row );
        break;

    case MSIMODIFY_UPDATE:
        r = msi_table_update( view, rec, row );
        break;

    case MSIMODIFY_ASSIGN:
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

static UINT TABLE_delete( struct tagMSIVIEW *view )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p\n", view );

    tv->table = NULL;
    tv->columns = NULL;

    if (tv->order)
    {
        msi_free( tv->order->reorder );
        msi_free( tv->order );
        tv->order = NULL;
    }

    msi_free( tv );

    return ERROR_SUCCESS;
}

static UINT TABLE_find_matching_rows( struct tagMSIVIEW *view, UINT col,
    UINT val, UINT *row, MSIITERHANDLE *handle )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    const MSICOLUMNHASHENTRY *entry;

    TRACE("%p, %d, %u, %p\n", view, col, val, *handle);

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col > tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    if( !tv->columns[col-1].hash_table )
    {
        UINT i;
        UINT num_rows = tv->table->row_count + tv->table->nonpersistent_row_count;
        MSICOLUMNHASHENTRY **hash_table;
        MSICOLUMNHASHENTRY *new_entry;

        if( tv->columns[col-1].offset >= tv->row_size )
        {
            ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
            ERR("%p %p\n", tv, tv->columns );
            return ERROR_FUNCTION_FAILED;
        }

        /* allocate contiguous memory for the table and its entries so we
         * don't have to do an expensive cleanup */
        hash_table = msi_alloc(MSITABLE_HASH_TABLE_SIZE * sizeof(MSICOLUMNHASHENTRY*) +
            num_rows * sizeof(MSICOLUMNHASHENTRY));
        if (!hash_table)
            return ERROR_OUTOFMEMORY;

        memset(hash_table, 0, MSITABLE_HASH_TABLE_SIZE * sizeof(MSICOLUMNHASHENTRY*));
        tv->columns[col-1].hash_table = hash_table;

        new_entry = (MSICOLUMNHASHENTRY *)(hash_table + MSITABLE_HASH_TABLE_SIZE);

        for (i = 0; i < num_rows; i++, new_entry++)
        {
            UINT row_value;

            if (view->ops->fetch_int( view, i, col, &row_value ) != ERROR_SUCCESS)
                continue;

            new_entry->next = NULL;
            new_entry->value = row_value;
            new_entry->row = i;
            if (hash_table[row_value % MSITABLE_HASH_TABLE_SIZE])
            {
                MSICOLUMNHASHENTRY *prev_entry = hash_table[row_value % MSITABLE_HASH_TABLE_SIZE];
                while (prev_entry->next)
                    prev_entry = prev_entry->next;
                prev_entry->next = new_entry;
            }
            else
                hash_table[row_value % MSITABLE_HASH_TABLE_SIZE] = new_entry;
        }
    }

    if( !*handle )
        entry = tv->columns[col-1].hash_table[val % MSITABLE_HASH_TABLE_SIZE];
    else
        entry = (*handle)->next;

    while (entry && entry->value != val)
        entry = entry->next;

    *handle = entry;
    if (!entry)
        return ERROR_NO_MORE_ITEMS;

    *row = entry->row;

    return ERROR_SUCCESS;
}

static UINT TABLE_add_ref(struct tagMSIVIEW *view)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT i;

    TRACE("%p %d\n", view, tv->table->ref_count);

    for (i = 0; i < tv->table->col_count; i++)
    {
        if (tv->table->colinfo[i].type & MSITYPE_TEMPORARY)
            InterlockedIncrement(&tv->table->colinfo[i].ref_count);
    }

    return InterlockedIncrement(&tv->table->ref_count);
}

static UINT TABLE_remove_column(struct tagMSIVIEW *view, LPCWSTR table, UINT number)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    MSIRECORD *rec = NULL;
    MSIVIEW *columns = NULL;
    UINT row, r;

    rec = MSI_CreateRecord(2);
    if (!rec)
        return ERROR_OUTOFMEMORY;

    MSI_RecordSetStringW(rec, 1, table);
    MSI_RecordSetInteger(rec, 2, number);

    r = TABLE_CreateView(tv->db, szColumns, &columns);
    if (r != ERROR_SUCCESS)
        return r;

    r = msi_table_find_row((MSITABLEVIEW *)columns, rec, &row);
    if (r != ERROR_SUCCESS)
        goto done;

    r = TABLE_delete_row(columns, row);
    if (r != ERROR_SUCCESS)
        goto done;

    msi_update_table_columns(tv->db, table);

done:
    msiobj_release(&rec->hdr);
    if (columns) columns->ops->delete(columns);
    return r;
}

static UINT TABLE_release(struct tagMSIVIEW *view)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    INT ref = tv->table->ref_count;
    UINT i, r;

    TRACE("%p %d\n", view, ref);

    for (i = 0; i < tv->table->col_count; i++)
    {
        if (tv->table->colinfo[i].type & MSITYPE_TEMPORARY)
        {
            ref = InterlockedDecrement(&tv->table->colinfo[i].ref_count);
            if (ref == 0)
            {
                r = TABLE_remove_column(view, tv->table->colinfo[i].tablename,
                                        tv->table->colinfo[i].number);
                if (r != ERROR_SUCCESS)
                    break;
            }
        }
    }

    ref = InterlockedDecrement(&tv->table->ref_count);
    if (ref == 0)
    {
        if (!tv->table->row_count)
        {
            list_remove(&tv->table->entry);
            free_table(tv->table);
            TABLE_delete(view);
        }
    }

    return ref;
}

static UINT TABLE_add_column(struct tagMSIVIEW *view, LPCWSTR table, UINT number,
                             LPCWSTR column, UINT type, BOOL hold)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    MSITABLE *msitable;
    MSIRECORD *rec;
    UINT r, i;

    rec = MSI_CreateRecord(4);
    if (!rec)
        return ERROR_OUTOFMEMORY;

    MSI_RecordSetStringW(rec, 1, table);
    MSI_RecordSetInteger(rec, 2, number);
    MSI_RecordSetStringW(rec, 3, column);
    MSI_RecordSetInteger(rec, 4, type);

    r = TABLE_insert_row(&tv->view, rec, FALSE);
    if (r != ERROR_SUCCESS)
        goto done;

    msi_update_table_columns(tv->db, table);

    if (!hold)
        goto done;

    msitable = find_cached_table(tv->db, table);
    for (i = 0; i < msitable->col_count; i++)
    {
        if (!lstrcmpW(msitable->colinfo[i].colname, column))
        {
            InterlockedIncrement(&msitable->colinfo[i].ref_count);
            break;
        }
    }

done:
    msiobj_release(&rec->hdr);
    return r;
}

static UINT order_add_column(struct tagMSIVIEW *view, MSIORDERINFO *order, LPCWSTR name)
{
    UINT n, r, count;

    r = TABLE_get_dimensions(view, NULL, &count);
    if (r != ERROR_SUCCESS)
        return r;

    if (order->num_cols >= count)
        return ERROR_FUNCTION_FAILED;

    r = VIEW_find_column(view, name, &n);
    if (r != ERROR_SUCCESS)
        return r;

    order->cols[order->num_cols] = n;
    TRACE("Ordering by column %s (%d)\n", debugstr_w(name), n);

    order->num_cols++;

    return ERROR_SUCCESS;
}

static UINT order_compare(struct tagMSIVIEW *view, MSIORDERINFO *order,
                          UINT a, UINT b, UINT *swap)
{
    UINT r, i, a_val = 0, b_val = 0;

    *swap = 0;
    for (i = 0; i < order->num_cols; i++)
    {
        r = TABLE_fetch_int(view, a, order->cols[i], &a_val);
        if (r != ERROR_SUCCESS)
            return r;

        r = TABLE_fetch_int(view, b, order->cols[i], &b_val);
        if (r != ERROR_SUCCESS)
            return r;

        if (a_val != b_val)
        {
            if (a_val > b_val)
                *swap = 1;
            break;
        }
    }

    return ERROR_SUCCESS;
}

static UINT order_mergesort(struct tagMSIVIEW *view, MSIORDERINFO *order,
                            UINT left, UINT right)
{
    UINT r, i, j, temp;
    UINT swap = 0, center = (left + right) / 2;
    UINT *array = order->reorder;

    if (left == right)
        return ERROR_SUCCESS;

    /* sort the left half */
    r = order_mergesort(view, order, left, center);
    if (r != ERROR_SUCCESS)
        return r;

    /* sort the right half */
    r = order_mergesort(view, order, center + 1, right);
    if (r != ERROR_SUCCESS)
        return r;

    for (i = left, j = center + 1; (i <= center) && (j <= right); i++)
    {
        r = order_compare(view, order, array[i], array[j], &swap);
        if (r != ERROR_SUCCESS)
            return r;

        if (swap)
        {
            temp = array[j];
            memmove(&array[i + 1], &array[i], (j - i) * sizeof(UINT));
            array[i] = temp;
            j++;
            center++;
        }
    }

    return ERROR_SUCCESS;
}

static UINT order_verify(struct tagMSIVIEW *view, MSIORDERINFO *order, UINT num_rows)
{
    UINT i, swap, r;

    for (i = 1; i < num_rows; i++)
    {
        r = order_compare(view, order, order->reorder[i - 1],
                          order->reorder[i], &swap);
        if (r != ERROR_SUCCESS)
            return r;

        if (!swap)
            continue;

        ERR("Bad order! %d\n", i);
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static UINT TABLE_sort(struct tagMSIVIEW *view, column_info *columns)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;
    MSIORDERINFO *order;
    column_info *ptr;
    UINT r, i;
    UINT rows, cols;

    TRACE("sorting table %s\n", debugstr_w(tv->name));

    r = TABLE_get_dimensions(view, &rows, &cols);
    if (r != ERROR_SUCCESS)
        return r;

    if (rows == 0)
        return ERROR_SUCCESS;

    order = msi_alloc_zero(sizeof(MSIORDERINFO) + sizeof(UINT) * cols);
    if (!order)
        return ERROR_OUTOFMEMORY;

    for (ptr = columns; ptr; ptr = ptr->next)
        order_add_column(view, order, ptr->column);

    order->reorder = msi_alloc(rows * sizeof(UINT));
    if (!order->reorder)
        return ERROR_OUTOFMEMORY;

    for (i = 0; i < rows; i++)
        order->reorder[i] = i;

    r = order_mergesort(view, order, 0, rows - 1);
    if (r != ERROR_SUCCESS)
        return r;

    r = order_verify(view, order, rows);
    if (r != ERROR_SUCCESS)
        return r;

    tv->order = order;

    return ERROR_SUCCESS;
}

static UINT TABLE_drop(struct tagMSIVIEW *view)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    MSIVIEW *tables = NULL;
    MSIRECORD *rec = NULL;
    UINT r, row;
    INT i;

    TRACE("dropping table %s\n", debugstr_w(tv->name));

    for (i = tv->table->col_count - 1; i >= 0; i--)
    {
        r = TABLE_remove_column(view, tv->table->colinfo[i].tablename,
                                tv->table->colinfo[i].number);
        if (r != ERROR_SUCCESS)
            return r;
    }

    rec = MSI_CreateRecord(1);
    if (!rec)
        return ERROR_OUTOFMEMORY;

    MSI_RecordSetStringW(rec, 1, tv->name);

    r = TABLE_CreateView(tv->db, szTables, &tables);
    if (r != ERROR_SUCCESS)
        return r;

    r = msi_table_find_row((MSITABLEVIEW *)tables, rec, &row);
    if (r != ERROR_SUCCESS)
        goto done;

    r = TABLE_delete_row(tables, row);
    if (r != ERROR_SUCCESS)
        goto done;

    list_remove(&tv->table->entry);
    free_table(tv->table);
    TABLE_delete(view);

done:
    msiobj_release(&rec->hdr);
    tables->ops->delete(tables);

    return r;
}

static const MSIVIEWOPS table_ops =
{
    TABLE_fetch_int,
    TABLE_fetch_stream,
    TABLE_get_row,
    TABLE_set_row,
    TABLE_insert_row,
    TABLE_delete_row,
    TABLE_execute,
    TABLE_close,
    TABLE_get_dimensions,
    TABLE_get_column_info,
    TABLE_modify,
    TABLE_delete,
    TABLE_find_matching_rows,
    TABLE_add_ref,
    TABLE_release,
    TABLE_add_column,
    TABLE_remove_column,
    TABLE_sort,
    TABLE_drop,
};

UINT TABLE_CreateView( MSIDATABASE *db, LPCWSTR name, MSIVIEW **view )
{
    MSITABLEVIEW *tv ;
    UINT r, sz;

    static const WCHAR Streams[] = {'_','S','t','r','e','a','m','s',0};
    static const WCHAR Storages[] = {'_','S','t','o','r','a','g','e','s',0};

    TRACE("%p %s %p\n", db, debugstr_w(name), view );

    if ( !lstrcmpW( name, Streams ) )
        return STREAMS_CreateView( db, view );
    else if ( !lstrcmpW( name, Storages ) )
        return STORAGES_CreateView( db, view );

    sz = sizeof *tv + lstrlenW(name)*sizeof name[0] ;
    tv = msi_alloc_zero( sz );
    if( !tv )
        return ERROR_FUNCTION_FAILED;

    r = get_table( db, name, &tv->table );
    if( r != ERROR_SUCCESS )
    {
        msi_free( tv );
        WARN("table not found\n");
        return r;
    }

    TRACE("table %p found with %d columns\n", tv->table, tv->table->col_count);

    /* fill the structure */
    tv->view.ops = &table_ops;
    tv->db = db;
    tv->columns = tv->table->colinfo;
    tv->num_cols = tv->table->col_count;
    tv->row_size = msi_table_get_row_size( db, tv->table->colinfo, tv->table->col_count );

    TRACE("%s one row is %d bytes\n", debugstr_w(name), tv->row_size );

    *view = (MSIVIEW*) tv;
    lstrcpyW( tv->name, name );

    return ERROR_SUCCESS;
}

UINT MSI_CommitTables( MSIDATABASE *db )
{
    UINT r;
    MSITABLE *table = NULL;

    TRACE("%p\n",db);

    r = msi_save_string_table( db->strings, db->storage );
    if( r != ERROR_SUCCESS )
    {
        WARN("failed to save string table r=%08x\n",r);
        return r;
    }

    LIST_FOR_EACH_ENTRY( table, &db->tables, MSITABLE, entry )
    {
        r = save_table( db, table );
        if( r != ERROR_SUCCESS )
        {
            WARN("failed to save table %s (r=%08x)\n",
                  debugstr_w(table->name), r);
            return r;
        }
    }

    /* force everything to reload next time */
    free_cached_tables( db );

    return ERROR_SUCCESS;
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

    if (t->persistent)
        return MSICONDITION_TRUE;
    else
        return MSICONDITION_FALSE;
}

static UINT read_raw_int(const BYTE *data, UINT col, UINT bytes)
{
    UINT ret = 0, i;

    for (i = 0; i < bytes; i++)
        ret += (data[col + i] << i * 8);

    return ret;
}

static MSIRECORD *msi_get_transform_record( const MSITABLEVIEW *tv, const string_table *st,
                                            const BYTE *rawdata, UINT bytes_per_strref )
{
    UINT i, val, ofs = 0;
    USHORT mask;
    MSICOLUMNINFO *columns = tv->columns;
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

        if( (columns[i].type & MSITYPE_STRING) &&
            ! MSITYPE_IS_BINARY(tv->columns[i].type) )
        {
            LPCWSTR sval;

            val = read_raw_int(rawdata, ofs, bytes_per_strref);
            sval = msi_string_lookup_id( st, val );
            MSI_RecordSetStringW( rec, i+1, sval );
            TRACE(" field %d [%s]\n", i+1, debugstr_w(sval));
            ofs += bytes_per_strref;
        }
        else
        {
            UINT n = bytes_per_column( tv->db, &columns[i] );
            switch( n )
            {
            case 2:
                val = read_raw_int(rawdata, ofs, n);
                if (val)
                    MSI_RecordSetInteger( rec, i+1, val^0x8000 );
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

static void dump_record( MSIRECORD *rec )
{
    UINT i, n;

    n = MSI_RecordGetFieldCount( rec );
    for( i=1; i<=n; i++ )
    {
        LPCWSTR sval = MSI_RecordGetString( rec, i );

        if( MSI_RecordIsNull( rec, i ) )
            TRACE("row -> []\n");
        else if( (sval = MSI_RecordGetString( rec, i )) )
            TRACE("row -> [%s]\n", debugstr_w(sval));
        else
            TRACE("row -> [0x%08x]\n", MSI_RecordGetInteger( rec, i ) );
    }
}

static void dump_table( const string_table *st, const USHORT *rawdata, UINT rawsize )
{
    LPCWSTR sval;
    UINT i;

    for( i=0; i<(rawsize/2); i++ )
    {
        sval = msi_string_lookup_id( st, rawdata[i] );
        MESSAGE(" %04x %s\n", rawdata[i], debugstr_w(sval) );
    }
}

static UINT* msi_record_to_row( const MSITABLEVIEW *tv, MSIRECORD *rec )
{
    LPCWSTR str;
    UINT i, r, *data;

    data = msi_alloc( tv->num_cols *sizeof (UINT) );
    for( i=0; i<tv->num_cols; i++ )
    {
        data[i] = 0;

        if ( ~tv->columns[i].type & MSITYPE_KEY )
            continue;

        /* turn the transform column value into a row value */
        if ( ( tv->columns[i].type & MSITYPE_STRING ) &&
             ! MSITYPE_IS_BINARY(tv->columns[i].type) )
        {
            str = MSI_RecordGetString( rec, i+1 );
            r = msi_string2idW( tv->db->strings, str, &data[i] );

            /* if there's no matching string in the string table,
               these keys can't match any record, so fail now. */
            if( ERROR_SUCCESS != r )
            {
                msi_free( data );
                return NULL;
            }
        }
        else
        {
            data[i] = MSI_RecordGetInteger( rec, i+1 );
            if ((tv->columns[i].type&0xff) == 2)
                data[i] += 0x8000;
            else
                data[i] += 0x80000000;
        }
    }
    return data;
}

static UINT msi_row_matches( MSITABLEVIEW *tv, UINT row, const UINT *data )
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

        ret = ERROR_SUCCESS;
    }

    return ret;
}

static UINT msi_table_find_row( MSITABLEVIEW *tv, MSIRECORD *rec, UINT *row )
{
    UINT i, r = ERROR_FUNCTION_FAILED, *data;

    data = msi_record_to_row( tv, rec );
    if( !data )
        return r;
    for( i = 0; i < tv->table->row_count + tv->table->nonpersistent_row_count; i++ )
    {
        r = msi_row_matches( tv, i, data );
        if( r == ERROR_SUCCESS )
        {
            *row = i;
            break;
        }
    }
    msi_free( data );
    return r;
}

typedef struct
{
    struct list entry;
    LPWSTR name;
} TRANSFORMDATA;

static UINT msi_table_load_transform( MSIDATABASE *db, IStorage *stg,
                                      string_table *st, TRANSFORMDATA *transform,
                                      UINT bytes_per_strref )
{
    UINT rawsize = 0;
    BYTE *rawdata = NULL;
    MSITABLEVIEW *tv = NULL;
    UINT r, n, sz, i, mask;
    MSIRECORD *rec = NULL;
    UINT colcol = 0;
    WCHAR coltable[32];
    LPWSTR name;

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
    r = TABLE_CreateView( db, name, (MSIVIEW**) &tv );
    if( r != ERROR_SUCCESS )
        goto err;

    r = tv->view.ops->execute( &tv->view, NULL );
    if( r != ERROR_SUCCESS )
        goto err;

    TRACE("name = %s columns = %u row_size = %u raw size = %u\n",
          debugstr_w(name), tv->num_cols, tv->row_size, rawsize );

    /* interpret the data */
    r = ERROR_SUCCESS;
    for( n=0; n < rawsize;  )
    {
        mask = rawdata[n] | (rawdata[n+1] << 8);

        if (mask&1)
        {
            /*
             * if the low bit is set, columns are continuous and
             * the number of columns is specified in the high byte
             */
            sz = 2;
            for( i=0; i<tv->num_cols; i++ )
            {
                if( (tv->columns[i].type & MSITYPE_STRING) &&
                    ! MSITYPE_IS_BINARY(tv->columns[i].type) )
                    sz += bytes_per_strref;
                else
                    sz += bytes_per_column( tv->db, &tv->columns[i] );
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
            for( i=0; i<tv->num_cols; i++ )
            {
                if( (tv->columns[i].type & MSITYPE_KEY) || ((1<<i)&mask))
                {
                    if( (tv->columns[i].type & MSITYPE_STRING) &&
                        ! MSITYPE_IS_BINARY(tv->columns[i].type) )
                        sz += bytes_per_strref;
                    else
                        sz += bytes_per_column( tv->db, &tv->columns[i] );
                }
            }
        }

        /* check we didn't run of the end of the table */
        if ( (n+sz) > rawsize )
        {
            ERR("borked.\n");
            dump_table( st, (USHORT *)rawdata, rawsize );
            break;
        }

        rec = msi_get_transform_record( tv, st, &rawdata[n], bytes_per_strref );
        if (rec)
        {
            if ( mask & 1 )
            {
                WCHAR table[32];
                DWORD sz = 32;
                UINT number = MSI_NULL_INTEGER;

                TRACE("inserting record\n");

                if (!lstrcmpW(name, szColumns))
                {
                    MSI_RecordGetStringW( rec, 1, table, &sz );
                    number = MSI_RecordGetInteger( rec, 2 );

                    /*
                     * Native msi seems writes nul into the Number (2nd) column of
                     * the _Columns table, only when the columns are from a new table
                     */
                    if ( number == MSI_NULL_INTEGER )
                    {
                        /* reset the column number on a new table */
                        if ( lstrcmpW(coltable, table) )
                        {
                            colcol = 0;
                            lstrcpyW( coltable, table );
                        }

                        /* fix nul column numbers */
                        MSI_RecordSetInteger( rec, 2, ++colcol );
                    }
                }

                r = TABLE_insert_row( &tv->view, rec, FALSE );
                if (r != ERROR_SUCCESS)
                    ERR("insert row failed\n");

                if ( number != MSI_NULL_INTEGER && !lstrcmpW(name, szColumns) )
                    msi_update_table_columns( db, table );
            }
            else
            {
                UINT row = 0;

                r = msi_table_find_row( tv, rec, &row );
                if (r != ERROR_SUCCESS)
                    ERR("no matching row to transform\n");
                else if ( mask )
                {
                    TRACE("modifying row [%d]:\n", row);
                    TABLE_set_row( &tv->view, row, rec, mask );
                }
                else
                {
                    TRACE("deleting row [%d]:\n", row);
                    TABLE_delete_row( &tv->view, row );
                }
            }
            if( TRACE_ON(msidb) ) dump_record( rec );
            msiobj_release( &rec->hdr );
        }

        n += sz;
    }

err:
    /* no need to free the table, it's associated with the database */
    msi_free( rawdata );
    if( tv )
        tv->view.ops->delete( &tv->view );

    return ERROR_SUCCESS;
}

/*
 * msi_table_apply_transform
 *
 * Enumerate the table transforms in a transform storage and apply each one.
 */
UINT msi_table_apply_transform( MSIDATABASE *db, IStorage *stg )
{
    struct list transforms;
    IEnumSTATSTG *stgenum = NULL;
    TRANSFORMDATA *transform;
    TRANSFORMDATA *tables = NULL, *columns = NULL;
    HRESULT r;
    STATSTG stat;
    string_table *strings;
    UINT ret = ERROR_FUNCTION_FAILED;
    UINT bytes_per_strref;

    TRACE("%p %p\n", db, stg );

    strings = msi_load_string_table( stg, &bytes_per_strref );
    if( !strings )
        goto end;

    r = IStorage_EnumElements( stg, 0, NULL, 0, &stgenum );
    if( FAILED( r ) )
        goto end;

    list_init(&transforms);

    while ( TRUE )
    {
        MSITABLEVIEW *tv = NULL;
        WCHAR name[0x40];
        ULONG count = 0;

        r = IEnumSTATSTG_Next( stgenum, 1, &stat, &count );
        if ( FAILED( r ) || !count )
            break;

        decode_streamname( stat.pwcsName, name );
        CoTaskMemFree( stat.pwcsName );
        if ( name[0] != 0x4840 )
            continue;

        if ( !lstrcmpW( name+1, szStringPool ) ||
             !lstrcmpW( name+1, szStringData ) )
            continue;

        transform = msi_alloc_zero( sizeof(TRANSFORMDATA) );
        if ( !transform )
            break;

        list_add_tail( &transforms, &transform->entry );

        transform->name = strdupW( name + 1 );

        if ( !lstrcmpW( transform->name, szTables ) )
            tables = transform;
        else if (!lstrcmpW( transform->name, szColumns ) )
            columns = transform;

        TRACE("transform contains stream %s\n", debugstr_w(name));

        /* load the table */
        r = TABLE_CreateView( db, transform->name, (MSIVIEW**) &tv );
        if( r != ERROR_SUCCESS )
            continue;

        r = tv->view.ops->execute( &tv->view, NULL );
        if( r != ERROR_SUCCESS )
        {
            tv->view.ops->delete( &tv->view );
            continue;
        }

        tv->view.ops->delete( &tv->view );
    }

    /*
     * Apply _Tables and _Columns transforms first so that
     * the table metadata is correct, and empty tables exist.
     */
    ret = msi_table_load_transform( db, stg, strings, tables, bytes_per_strref );
    if (ret != ERROR_SUCCESS && ret != ERROR_INVALID_TABLE)
        goto end;

    ret = msi_table_load_transform( db, stg, strings, columns, bytes_per_strref );
    if (ret != ERROR_SUCCESS && ret != ERROR_INVALID_TABLE)
        goto end;

    ret = ERROR_SUCCESS;

    while ( !list_empty( &transforms ) )
    {
        transform = LIST_ENTRY( list_head( &transforms ), TRANSFORMDATA, entry );

        if ( lstrcmpW( transform->name, szColumns ) &&
             lstrcmpW( transform->name, szTables ) &&
             ret == ERROR_SUCCESS )
        {
            ret = msi_table_load_transform( db, stg, strings, transform, bytes_per_strref );
        }

        list_remove( &transform->entry );
        msi_free( transform->name );
        msi_free( transform );
    }

    if ( ret == ERROR_SUCCESS )
        append_storage_to_db( db, stg );

end:
    if ( stgenum )
        IEnumSTATSTG_Release( stgenum );
    if ( strings )
        msi_destroy_stringtable( strings );

    return ret;
}

void append_storage_to_db( MSIDATABASE *db, IStorage *stg )
{
    MSITRANSFORM *t;

    t = msi_alloc( sizeof *t );
    t->stg = stg;
    IStorage_AddRef( stg );
    list_add_tail( &db->transforms, &t->entry );
}

void msi_free_transforms( MSIDATABASE *db )
{
    while( !list_empty( &db->transforms ) )
    {
        MSITRANSFORM *t = LIST_ENTRY( list_head( &db->transforms ),
                                      MSITRANSFORM, entry );
        list_remove( &t->entry );
        IStorage_Release( t->stg );
        msi_free( t );
    }
}
