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
#include "assert.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define MSITABLE_HASH_TABLE_SIZE 37

typedef struct tagMSICOLUMNHASHENTRY
{
    struct tagMSICOLUMNHASHENTRY *next;
    UINT value;
    UINT row;
} MSICOLUMNHASHENTRY;

typedef struct tagMSICOLUMNINFO
{
    LPCWSTR tablename;
    UINT   number;
    LPCWSTR colname;
    UINT   type;
    UINT   offset;
    MSICOLUMNHASHENTRY **hash_table;
} MSICOLUMNINFO;

struct tagMSITABLE
{
    USHORT **data;
    UINT row_count;
    struct list entry;
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

#define MAX_STREAM_NAME 0x1f

static UINT table_get_column_info( MSIDATABASE *db, LPCWSTR name,
       MSICOLUMNINFO **pcols, UINT *pcount );
static UINT get_tablecolumns( MSIDATABASE *db,
       LPCWSTR szTableName, MSICOLUMNINFO *colinfo, UINT *sz);
static void msi_free_colinfo( MSICOLUMNINFO *colinfo, UINT count );

static inline UINT bytes_per_column( const MSICOLUMNINFO *col )
{
    if( col->type & MSITYPE_STRING )
        return 2;
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

static LPWSTR encode_streamname(BOOL bTable, LPCWSTR in)
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
                if( next >= 0  )
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

static BOOL decode_streamname(LPWSTR in, LPWSTR out)
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
        n++;
    }

    IEnumSTATSTG_Release( stgenum );
}

static UINT read_stream_data( IStorage *stg, LPCWSTR stname,
                              USHORT **pdata, UINT *psz )
{
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;
    VOID *data;
    ULONG sz, count;
    IStream *stm = NULL;
    STATSTG stat;
    LPWSTR encname;

    encname = encode_streamname(TRUE, stname);

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

static UINT write_stream_data( IStorage *stg, LPCWSTR stname,
                               LPVOID data, UINT sz )
{
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;
    ULONG count;
    IStream *stm = NULL;
    ULARGE_INTEGER size;
    LARGE_INTEGER pos;
    LPWSTR encname;

    encname = encode_streamname(TRUE, stname );
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

    r = IStream_Write(stm, data, sz, &count );
    if( FAILED( r ) || ( count != sz ) )
    {
        WARN("Failed to Write\n");
        goto end;
    }

    ret = ERROR_SUCCESS;

end:
    IStream_Release( stm );

    return ret;
}

static void free_table( MSITABLE *table )
{
    int i;
    for( i=0; i<table->row_count; i++ )
        msi_free( table->data[i] );
    msi_free( table->data );
    msi_free( table );
}

static UINT msi_table_get_row_size( const MSICOLUMNINFO *cols, UINT count )
{
    const MSICOLUMNINFO *last_col = &cols[count-1];
    if (!count)
        return 0;
    return last_col->offset + bytes_per_column( last_col );
}

/* add this table to the list of cached tables in the database */
static MSITABLE *read_table_from_storage( IStorage *stg, LPCWSTR name,
                                    const MSICOLUMNINFO *cols, UINT num_cols )
{
    MSITABLE *t;
    USHORT *rawdata = NULL;
    UINT rawsize = 0, i, j, row_size = 0;

    TRACE("%s\n",debugstr_w(name));

    /* nonexistent tables should be interpreted as empty tables */
    t = msi_alloc( sizeof (MSITABLE) + lstrlenW(name)*sizeof (WCHAR) );
    if( !t )
        return t;

    row_size = msi_table_get_row_size( cols, num_cols );

    t->row_count = 0;
    t->data = NULL;
    lstrcpyW( t->name, name );

    /* if we can't read the table, just assume that it's empty */
    read_stream_data( stg, name, &rawdata, &rawsize );
    if( !rawdata )
        return t;

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

        for( j=0; j<num_cols; j++ )
        {
            UINT ofs = cols[j].offset/2;
            UINT n = bytes_per_column( &cols[j] );

            switch( n )
            {
            case 2:
                t->data[i][ofs] = rawdata[ofs*t->row_count + i ];
                break;
            case 4:
                t->data[i][ofs] = rawdata[ofs*t->row_count + i*2 ];
                t->data[i][ofs+1] = rawdata[ofs*t->row_count + i*2 + 1];
                break;
            default:
                ERR("oops - unknown column width %d\n", n);
                goto err;
            }
        }
    }

    msi_free( rawdata );
    return t;
err:
    msi_free( rawdata );
    free_table( t );
    return NULL;
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
    *pcount = column_count;

    return r;
}

static MSITABLE *get_table( MSIDATABASE *db, LPCWSTR name,
                            const MSICOLUMNINFO *cols, UINT num_cols )
{
    MSITABLE *table;

    /* first, see if the table is cached */
    table = find_cached_table( db, name );
    if( table )
        return table;

    table = read_table_from_storage( db->storage, name, cols, num_cols );
    if( table )
        list_add_head( &db->tables, &table->entry );

    return table;
}

static UINT save_table( MSIDATABASE *db, MSITABLE *t )
{
    USHORT *rawdata = NULL, *p;
    UINT rawsize, r, i, j, row_size, num_cols = 0;
    MSICOLUMNINFO *cols = NULL;

    TRACE("Saving %s\n", debugstr_w( t->name ) );

    r = table_get_column_info( db, t->name, &cols, &num_cols );
    if( r != ERROR_SUCCESS )
        return r;
    
    row_size = msi_table_get_row_size( cols, num_cols );

    rawsize = t->row_count * row_size;
    rawdata = msi_alloc_zero( rawsize );
    if( !rawdata )
    {
        r = ERROR_NOT_ENOUGH_MEMORY;
        goto err;
    }

    p = rawdata;
    for( i=0; i<num_cols; i++ )
    {
        for( j=0; j<t->row_count; j++ )
        {
            UINT offset = cols[i].offset;

            *p++ = t->data[j][offset/2];
            if( 4 == bytes_per_column( &cols[i] ) )
                *p++ = t->data[j][offset/2+1];
        }
    }

    TRACE("writing %d bytes\n", rawsize);
    r = write_stream_data( db->storage, t->name, rawdata, rawsize );

err:
    msi_free_colinfo( cols, num_cols );
    msi_free( cols );
    msi_free( rawdata );

    return r;
}

HRESULT init_string_table( IStorage *stg )
{
    HRESULT r;
    USHORT zero[2] = { 0, 0 };
    ULONG count = 0;
    IStream *stm = NULL;
    LPWSTR encname;

    encname = encode_streamname(TRUE, szStringPool );

    /* create the StringPool stream... add the zero string to it*/
    r = IStorage_CreateStream( stg, encname,
            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    msi_free( encname );
    if( r ) 
    {
        TRACE("Failed\n");
        return r;
    }

    r = IStream_Write(stm, zero, sizeof zero, &count );
    IStream_Release( stm );

    if( FAILED( r ) || ( count != sizeof zero ) )
    {
        TRACE("Failed\n");
        return E_FAIL;
    }

    /* create the StringData stream... make it zero length */
    encname = encode_streamname(TRUE, szStringData );
    r = IStorage_CreateStream( stg, encname,
            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    msi_free( encname );
    if( r ) 
    {
        TRACE("Failed\n");
        return E_FAIL;
    }
    IStream_Release( stm );

    return r;
}

string_table *load_string_table( IStorage *stg )
{
    string_table *st = NULL;
    CHAR *data = NULL;
    USHORT *pool = NULL;
    UINT r, datasize = 0, poolsize = 0, codepage;
    DWORD i, count, offset, len, n, refs;

    r = read_stream_data( stg, szStringPool, &pool, &poolsize );
    if( r != ERROR_SUCCESS)
        goto end;
    r = read_stream_data( stg, szStringData, (USHORT**)&data, &datasize );
    if( r != ERROR_SUCCESS)
        goto end;

    count = poolsize/4;
    if( poolsize > 4 )
        codepage = pool[0] | ( pool[1] << 16 );
    else
        codepage = CP_ACP;
    st = msi_init_stringtable( count, codepage );

    offset = 0;
    n = 1;
    i = 1;
    while( i<count )
    {
        /* the string reference count is always the second word */
        refs = pool[i*2+1];

        /* empty entries have two zeros, still have a string id */
        if (pool[i*2] == 0 && refs == 0)
        {
            i++;
            n++;
            continue;
        }

        /*
         * If a string is over 64k, the previous string entry is made null
         * and its the high word of the length is inserted in the null string's
         * reference count field.
         */
        if( pool[i*2] == 0)
        {
            len = (pool[i*2+3] << 16) + pool[i*2+2];
            i += 2;
        }
        else
        {
            len = pool[i*2];
            i += 1;
        }

        if ( (offset + len) > datasize )
        {
            ERR("string table corrupt?\n");
            break;
        }

        r = msi_addstring( st, n, data+offset, len, refs );
        if( r != n )
            ERR("Failed to add string %d\n", n );
        n++;
        offset += len;
    }

    if ( datasize != offset )
        ERR("string table load failed! (%08x != %08x), please report\n", datasize, offset );

    TRACE("Loaded %d strings\n", count);

end:
    msi_free( pool );
    msi_free( data );

    return st;
}

static UINT save_string_table( MSIDATABASE *db )
{
    UINT i, count, datasize = 0, poolsize = 0, sz, used, r, codepage, n;
    UINT ret = ERROR_FUNCTION_FAILED;
    CHAR *data = NULL;
    USHORT *pool = NULL;

    TRACE("\n");

    /* construct the new table in memory first */
    count = msi_string_totalsize( db->strings, &datasize, &poolsize );

    TRACE("%u %u %u\n", count, datasize, poolsize );

    pool = msi_alloc( poolsize );
    if( ! pool )
    {
        WARN("Failed to alloc pool %d bytes\n", poolsize );
        goto err;
    }
    data = msi_alloc( datasize );
    if( ! data )
    {
        WARN("Failed to alloc data %d bytes\n", poolsize );
        goto err;
    }

    used = 0;
    codepage = msi_string_get_codepage( db->strings );
    pool[0]=codepage&0xffff;
    pool[1]=(codepage>>16);
    n = 1;
    for( i=1; i<count; i++ )
    {
        sz = datasize - used;
        r = msi_id2stringA( db->strings, i, data+used, &sz );
        if( r != ERROR_SUCCESS )
        {
            ERR("failed to fetch string\n");
            sz = 0;
        }
        if( sz && (sz < (datasize - used ) ) )
            sz--;

        if (sz)
            pool[ n*2 + 1 ] = msi_id_refcount( db->strings, i );
        else
            pool[ n*2 + 1 ] = 0;
        if (sz < 0x10000)
        {
            pool[ n*2 ] = sz;
            n++;
        }
        else
        {
            pool[ n*2 ] = 0;
            pool[ n*2 + 2 ] = sz&0xffff;
            pool[ n*2 + 3 ] = (sz>>16);
            n += 2;
        }
        used += sz;
        if( used > datasize  )
        {
            ERR("oops overran %d >= %d\n", used, datasize);
            goto err;
        }
    }

    if( used != datasize )
    {
        ERR("oops used %d != datasize %d\n", used, datasize);
        goto err;
    }

    /* write the streams */
    r = write_stream_data( db->storage, szStringData, data, datasize );
    TRACE("Wrote StringData r=%08x\n", r);
    if( r )
        goto err;
    r = write_stream_data( db->storage, szStringPool, pool, poolsize );
    TRACE("Wrote StringPool r=%08x\n", r);
    if( r )
        goto err;

    ret = ERROR_SUCCESS;

err:
    msi_free( data );
    msi_free( pool );

    return ret;
}

/* information for default tables */
static const WCHAR szTables[]  = { '_','T','a','b','l','e','s',0 };
static const WCHAR szTable[]  = { 'T','a','b','l','e',0 };
static const WCHAR szName[]    = { 'N','a','m','e',0 };
static const WCHAR szColumns[] = { '_','C','o','l','u','m','n','s',0 };
static const WCHAR szColumn[]  = { 'C','o','l','u','m','n',0 };
static const WCHAR szNumber[]  = { 'N','u','m','b','e','r',0 };
static const WCHAR szType[]    = { 'T','y','p','e',0 };

static const MSICOLUMNINFO _Columns_cols[4] = {
    { szColumns, 1, szTable,  MSITYPE_VALID | MSITYPE_STRING | 64, 0 },
    { szColumns, 2, szNumber, MSITYPE_VALID | 2,                   2 },
    { szColumns, 3, szName,   MSITYPE_VALID | MSITYPE_STRING | 64, 4 },
    { szColumns, 4, szType,   MSITYPE_VALID | 2,                   6 },
};
static const MSICOLUMNINFO _Tables_cols[1] = {
    { szTables,  1, szName,   MSITYPE_VALID | MSITYPE_STRING | 64, 0 },
};

static UINT get_defaulttablecolumns( LPCWSTR name, MSICOLUMNINFO *colinfo, UINT *sz)
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
            memcpy( &colinfo[i], &p[i], sizeof(MSICOLUMNINFO) );
            colinfo[i].tablename = strdupW( p[i].tablename );
            colinfo[i].colname = strdupW( p[i].colname );
        }
        if( colinfo && (i >= *sz) )
            break;
    }
    *sz = n;
    return ERROR_SUCCESS;
}

static void msi_free_colinfo( MSICOLUMNINFO *colinfo, UINT count )
{
    UINT i;

    for( i=0; i<count; i++ )
    {
        msi_free( (LPWSTR) colinfo[i].tablename );
        msi_free( (LPWSTR) colinfo[i].colname );
        msi_free( colinfo[i].hash_table );
    }
}

static LPWSTR msi_makestring( MSIDATABASE *db, UINT stringid)
{
    return strdupW(msi_string_lookup_id( db->strings, stringid ));
}

static UINT get_tablecolumns( MSIDATABASE *db,
       LPCWSTR szTableName, MSICOLUMNINFO *colinfo, UINT *sz)
{
    UINT r, i, n=0, table_id, count, maxcount = *sz;
    MSITABLE *table = NULL;

    TRACE("%s\n", debugstr_w(szTableName));

    /* first check if there is a default table with that name */
    r = get_defaulttablecolumns( szTableName, colinfo, sz );
    if( ( r == ERROR_SUCCESS ) && *sz )
        return r;

    table = get_table( db, szColumns, _Columns_cols, 4 );
    if( !table )
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

    /* if maxcount is non-zero, assume it's exactly right for this table */
    memset( colinfo, 0, maxcount*sizeof(*colinfo) );
    count = table->row_count;
    for( i=0; i<count; i++ )
    {
        if( table->data[ i ][ 0 ] != table_id )
            continue;
        if( colinfo )
        {
            UINT id = table->data[ i ] [ 2 ];
            UINT col = table->data[ i ][ 1 ] - (1<<15);

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
            colinfo[ col - 1 ].type = table->data[ i ] [ 3 ] - (1<<15);
            colinfo[ col - 1 ].offset = 0;
            colinfo[ col - 1 ].hash_table = NULL;
        }
        n++;
    }

    TRACE("%s has %d columns\n", debugstr_w(szTableName), n);

    if (maxcount && n != maxcount)
    {
        ERR("missing column in table %s\n", debugstr_w(szTableName));
        msi_free_colinfo(colinfo, maxcount );
        return ERROR_FUNCTION_FAILED;
    }

    /* calculate the offsets */
    for( i=0; maxcount && (i<maxcount); i++ )
    {
         assert( (i+1) == colinfo[ i ].number );
         if (i)
             colinfo[i].offset = colinfo[ i - 1 ].offset
                               + bytes_per_column( &colinfo[ i - 1 ] );
         else
             colinfo[i].offset = 0;
         TRACE("column %d is [%s] with type %08x ofs %d\n",
               colinfo[i].number, debugstr_w(colinfo[i].colname),
               colinfo[i].type, colinfo[i].offset);
    }
    *sz = n;

    return ERROR_SUCCESS;
}

/* try to find the table name in the _Tables table */
BOOL TABLE_Exists( MSIDATABASE *db, LPWSTR name )
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

    table = get_table( db, szTables, _Tables_cols, 1 );
    if( !table )
    {
        TRACE("table %s not available\n", debugstr_w(szTables));
        return FALSE;
    }

    /* count = table->size/2; */
    count = table->row_count;
    for( i=0; i<count; i++ )
        if( table->data[ i ][ 0 ] == table_id )
            break;

    if (i!=count)
        return TRUE;

    TRACE("Searched %d tables, but %d was not found\n", count, table_id );

    return FALSE;
}

/* below is the query interface to a table */

typedef struct tagMSITABLEVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSITABLE      *table;
    MSICOLUMNINFO *columns;
    UINT           num_cols;
    UINT           row_size;
    WCHAR          name[1];
} MSITABLEVIEW;

static UINT TABLE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT offset, num_rows, n;

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col>tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    /* how many rows are there ? */
    num_rows = tv->table->row_count;
    if( row >= num_rows )
        return ERROR_NO_MORE_ITEMS;

    if( tv->columns[col-1].offset >= tv->row_size )
    {
        ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
        ERR("%p %p\n", tv, tv->columns );
        return ERROR_FUNCTION_FAILED;
    }

    offset = row + (tv->columns[col-1].offset/2) * num_rows;
    n = bytes_per_column( &tv->columns[col-1] );
    switch( n )
    {
    case 4:
        offset = tv->columns[col-1].offset/2;
        *val = tv->table->data[row][offset] +
               (tv->table->data[row][offset + 1] << 16);
        break;
    case 2:
        offset = tv->columns[col-1].offset/2;
        *val = tv->table->data[row][offset];
        break;
    default:
        ERR("oops! what is %d bytes per column?\n", n );
        return ERROR_FUNCTION_FAILED;
    }

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
    if (ival < 0 || ival > tv->num_cols || ival == col)
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
    UINT offset, n;

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    if( (col==0) || (col>tv->num_cols) )
        return ERROR_INVALID_PARAMETER;

    if( tv->columns[col-1].offset >= tv->row_size )
    {
        ERR("Stuffed up %d >= %d\n", tv->columns[col-1].offset, tv->row_size );
        ERR("%p %p\n", tv, tv->columns );
        return ERROR_FUNCTION_FAILED;
    }

    n = bytes_per_column( &tv->columns[col-1] );
    switch( n )
    {
    case 4:
        offset = tv->columns[col-1].offset/2;
        tv->table->data[row][offset]     = val & 0xffff;
        tv->table->data[row][offset + 1] = (val>>16)&0xffff;
        break;
    case 2:
        offset = tv->columns[col-1].offset/2;
        tv->table->data[row][offset] = val;
        break;
    default:
        ERR("oops! what is %d bytes per column?\n", n );
        return ERROR_FUNCTION_FAILED;
    }
    return ERROR_SUCCESS;
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
        /* only update the fields specified in the mask */
        if ( !(mask&(1<<i)) )
            continue;

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
                val = msi_addstringW( tv->db->strings, 0, sval, -1, 1 );
            }
            else if ( 2 == bytes_per_column( &tv->columns[ i ] ) )
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

static UINT table_create_new_row( struct tagMSIVIEW *view, UINT *num )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    USHORT **p, *row;
    UINT sz;

    TRACE("%p\n", view);

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    row = msi_alloc_zero( tv->row_size );
    if( !row )
        return ERROR_NOT_ENOUGH_MEMORY;

    sz = (tv->table->row_count + 1) * sizeof (UINT*);
    if( tv->table->data )
        p = msi_realloc( tv->table->data, sz );
    else
        p = msi_alloc( sz );
    if( !p )
    {
        msi_free( row );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    tv->table->data = p;
    tv->table->data[tv->table->row_count] = row;
    *num = tv->table->row_count;
    tv->table->row_count++;

    return ERROR_SUCCESS;
}

static UINT TABLE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p %p\n", tv, record);

    TRACE("There are %d columns\n", tv->num_cols );
    tv->table = get_table( tv->db, tv->name, tv->columns, tv->num_cols );
    if( !tv->table )
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

static UINT TABLE_close( struct tagMSIVIEW *view )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p\n", view );

    if( !tv->table )
        return ERROR_FUNCTION_FAILED;

    tv->table = NULL;
    
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
        *rows = tv->table->row_count;
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
        return ERROR_INVALID_DATA;

    return ERROR_SUCCESS;
}

static UINT TABLE_insert_row( struct tagMSIVIEW *view, MSIRECORD *rec )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT r, row = -1;

    TRACE("%p %p\n", tv, rec );

    /* check that the key is unique - can we find a matching row? */
    r = table_validate_new( tv, rec );
    if( r != ERROR_SUCCESS )
        return ERROR_FUNCTION_FAILED;

    r = table_create_new_row( view, &row );
    TRACE("insert_row returned %08x\n", r);
    if( r != ERROR_SUCCESS )
        return r;

    return TABLE_set_row( view, row, rec, (1<<tv->num_cols) - 1 );
}

static UINT TABLE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                MSIRECORD *rec)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT r;

    TRACE("%p %d %p\n", view, eModifyMode, rec );

    if (!tv->table)
    {
        r = TABLE_execute( view, NULL );
        if( r )
            return r;
    }

    switch (eModifyMode)
    {
    case MSIMODIFY_VALIDATE_NEW:
        r = table_validate_new( tv, rec );
        break;

    case MSIMODIFY_INSERT_TEMPORARY:
        r = table_validate_new( tv, rec );
        if (r != ERROR_SUCCESS)
            break;
        r = TABLE_insert_row( view, rec );
        break;

    case MSIMODIFY_REFRESH:
    case MSIMODIFY_INSERT:
    case MSIMODIFY_UPDATE:
    case MSIMODIFY_ASSIGN:
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

static UINT TABLE_delete( struct tagMSIVIEW *view )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p\n", view );

    tv->table = NULL;

    if( tv->columns )
    {
        msi_free_colinfo( tv->columns, tv->num_cols );
        msi_free( tv->columns );
    }
    tv->columns = NULL;

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
        UINT num_rows = tv->table->row_count;
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
            UINT row_value, n;
            UINT offset = i + (tv->columns[col-1].offset/2) * num_rows;
            n = bytes_per_column( &tv->columns[col-1] );
            switch( n )
            {
            case 4:
                offset = tv->columns[col-1].offset/2;
                row_value = tv->table->data[i][offset] + 
                    (tv->table->data[i][offset + 1] << 16);
                break;
            case 2:
                offset = tv->columns[col-1].offset/2;
                row_value = tv->table->data[i][offset];
                break;
            default:
                ERR("oops! what is %d bytes per column?\n", n );
                continue;
            }

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


static const MSIVIEWOPS table_ops =
{
    TABLE_fetch_int,
    TABLE_fetch_stream,
    TABLE_set_row,
    TABLE_insert_row,
    TABLE_execute,
    TABLE_close,
    TABLE_get_dimensions,
    TABLE_get_column_info,
    TABLE_modify,
    TABLE_delete,
    TABLE_find_matching_rows
};

UINT TABLE_CreateView( MSIDATABASE *db, LPCWSTR name, MSIVIEW **view )
{
    MSITABLEVIEW *tv ;
    UINT r, sz, column_count;
    MSICOLUMNINFO *columns;

    TRACE("%p %s %p\n", db, debugstr_w(name), view );

    /* get the number of columns in this table */
    column_count = 0;
    r = get_tablecolumns( db, name, NULL, &column_count );
    if( r != ERROR_SUCCESS )
        return r;

    /* if there's no columns, there's no table */
    if( column_count == 0 )
        return ERROR_INVALID_PARAMETER;

    TRACE("Table found\n");

    sz = sizeof *tv + lstrlenW(name)*sizeof name[0] ;
    tv = msi_alloc_zero( sz );
    if( !tv )
        return ERROR_FUNCTION_FAILED;
    
    columns = msi_alloc( column_count*sizeof (MSICOLUMNINFO));
    if( !columns )
    {
        msi_free( tv );
        return ERROR_FUNCTION_FAILED;
    }

    r = get_tablecolumns( db, name, columns, &column_count );
    if( r != ERROR_SUCCESS )
    {
        msi_free( columns );
        msi_free( tv );
        return ERROR_FUNCTION_FAILED;
    }

    TRACE("Table has %d columns\n", column_count);

    /* fill the structure */
    tv->view.ops = &table_ops;
    tv->db = db;
    tv->columns = columns;
    tv->num_cols = column_count;
    tv->table = NULL;
    tv->row_size = msi_table_get_row_size( columns, column_count );

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

    r = save_string_table( db );
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
    if (!table)
        return MSICONDITION_ERROR;

    return MSICONDITION_FALSE;
}

static MSIRECORD *msi_get_transform_record( MSITABLEVIEW *tv, string_table *st, USHORT *rawdata )
{
    UINT i, val, ofs = 0;
    USHORT mask = *rawdata++;
    MSICOLUMNINFO *columns = tv->columns;
    MSIRECORD *rec;

    rec = MSI_CreateRecord( tv->num_cols );
    if( !rec )
        return rec;

    TRACE("row ->\n");
    for( i=0; i<tv->num_cols; i++ )
    {
        UINT n = bytes_per_column( &columns[i] );

        if ( (mask&1) && (i>=(mask>>8)) )
            break;
        /* all keys must be present */
        if ( (~mask&1) && (~columns[i].type & MSITYPE_KEY) && ((1<<i) & ~mask) )
            continue;

        switch( n )
        {
        case 2:
            val = rawdata[ofs];
            if( (columns[i].type & MSITYPE_STRING) &&
                ! MSITYPE_IS_BINARY(tv->columns[i].type) )
            {
                LPCWSTR sval = msi_string_lookup_id( st, val );
                MSI_RecordSetStringW( rec, i+1, sval );
                TRACE(" field %d [%s]\n", i+1, debugstr_w(sval));
            }
            else
            {
                if (val)
                    MSI_RecordSetInteger( rec, i+1, val^0x8000 );
                TRACE(" field %d [0x%04x]\n", i+1, val );
            }
            break;
        case 4:
            val = (rawdata[ofs] + (rawdata[ofs + 1]<<16));
            if (val)
                MSI_RecordSetInteger( rec, i+1, val^0x80000000 );
            TRACE(" field %d [0x%08x]\n", i+1, val );
            break;
        default:
            ERR("oops - unknown column width %d\n", n);
            break;
        }
        ofs += n/2;
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

static void dump_table( string_table *st, USHORT *rawdata, UINT rawsize )
{
    LPCWSTR sval;
    UINT i;

    for( i=0; i<(rawsize/2); i++ )
    {
        sval = msi_string_lookup_id( st, rawdata[i] );
        MESSAGE(" %04x %s\n", rawdata[i], debugstr_w(sval) );
    }
}

static UINT* msi_record_to_row( MSITABLEVIEW *tv, MSIRECORD *rec )
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

static UINT msi_row_matches( MSITABLEVIEW *tv, UINT row, UINT *data )
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
    for( i=0; i<tv->table->row_count; i++ )
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

static UINT msi_delete_row( MSITABLEVIEW *tv, UINT row )
{
    UINT i;

    for( i=1; i<=tv->num_cols; i++ )
        TABLE_set_int( tv, row, i, 0 );
    return ERROR_SUCCESS;
}

static UINT msi_table_load_transform( MSIDATABASE *db, IStorage *stg,
                                      string_table *st, LPCWSTR name )
{
    UINT rawsize = 0;
    USHORT *rawdata = NULL;
    MSITABLEVIEW *tv = NULL;
    UINT r, n, sz, i, mask;
    MSIRECORD *rec = NULL;
    UINT colcol = 0;
    WCHAR coltable[32];

    coltable[0] = 0;
    TRACE("%p %p %p %s\n", db, stg, st, debugstr_w(name) );

    /* read the transform data */
    read_stream_data( stg, name, &rawdata, &rawsize );
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
    for( n=0; n < (rawsize/2);  )
    {
        mask = rawdata[n];

        if (mask&1)
        {
            /*
             * if the low bit is set, columns are continuous and
             * the number of columns is specified in the high byte
             */
            sz = 2 + tv->row_size;
        }
        else
        {
            /*
             * If the low bit is not set, rowdata[n] is a bitmask.
             * Excepting for key fields, which are always present,
             *  each bit indicates that a field is present in the transform record.
             *
             * rawdata[n] == 0 is a special case ... only the keys will be present
             * and it means that this row should be deleted.
             */
            sz = 2;
            for( i=0; i<tv->num_cols; i++ )
            {
                if( (tv->columns[i].type & MSITYPE_KEY) || ((1<<i)&mask))
                    sz += bytes_per_column( &tv->columns[i] );
            }
        }

        /* check we didn't run of the end of the table */
        if ( (n+sz) > rawsize )
        {
            ERR("borked.\n");
            dump_table( st, rawdata, rawsize );
            break;
        }

        rec = msi_get_transform_record( tv, st, &rawdata[n] );
        if (rec)
        {
            if ( mask & 1 )
            {
                TRACE("inserting record\n");

                /*
                 * Native msi seems writes nul into the
                 * Number (2nd) column of the _Columns table.
                 * Not sure that it's deliberate...
                 */
                if (!lstrcmpW(name, szColumns))
                {
                    WCHAR table[32];
                    DWORD sz = 32;

                    MSI_RecordGetStringW( rec, 1, table, &sz );

                    /* reset the column number on a new table */
                    if ( lstrcmpW(coltable, table) )
                    {
                        colcol = 0;
                        lstrcpyW( coltable, table );
                    }

                    /* fix nul column numbers */
                    MSI_RecordSetInteger( rec, 2, ++colcol );
                }

                r = TABLE_insert_row( &tv->view, rec );
                if (r != ERROR_SUCCESS)
                    ERR("insert row failed\n");
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
                    msi_delete_row( tv, row );
                }
            }
            if( TRACE_ON(msidb) ) dump_record( rec );
            msiobj_release( &rec->hdr );
        }

        n += sz/2;
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
    IEnumSTATSTG *stgenum = NULL;
    HRESULT r;
    STATSTG stat;
    ULONG count;
    WCHAR name[0x40];
    string_table *strings;
    UINT ret = ERROR_FUNCTION_FAILED;

    TRACE("%p %p\n", db, stg );

    strings = load_string_table( stg );
    if( !strings )
        goto end;

    r = IStorage_EnumElements( stg, 0, NULL, 0, &stgenum );
    if( FAILED( r ) )
        goto end;

    /*
     * Apply _Tables and _Columns transforms first so that
     * the table metadata is correct, and empty tables exist.
     */
    ret = msi_table_load_transform( db, stg, strings, szTables );
    if (ret != ERROR_SUCCESS && ret != ERROR_INVALID_TABLE)
        goto end;

    ret = msi_table_load_transform( db, stg, strings, szColumns );
    if (ret != ERROR_SUCCESS && ret != ERROR_INVALID_TABLE)
        goto end;

    ret = ERROR_SUCCESS;

    while( r == ERROR_SUCCESS )
    {
        count = 0;
        r = IEnumSTATSTG_Next( stgenum, 1, &stat, &count );
        if( FAILED( r ) || !count )
            break;

        decode_streamname( stat.pwcsName, name );
        if ( name[0] != 0x4840 )
            continue;

        TRACE("transform contains stream %s\n", debugstr_w(name));

        if ( !lstrcmpW( name+1, szStringPool ) ||
             !lstrcmpW( name+1, szStringData ) ||
             !lstrcmpW( name+1, szColumns ) ||
             !lstrcmpW( name+1, szTables ) )
            continue;

        ret = msi_table_load_transform( db, stg, strings, name+1 );
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
