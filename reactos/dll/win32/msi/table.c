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

#include "msipriv.h"

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
    UINT    number;
    LPCWSTR colname;
    UINT    type;
    UINT    offset;
    INT     ref_count;
    BOOL    temporary;
    MSICOLUMNHASHENTRY **hash_table;
} MSICOLUMNINFO;

struct tagMSITABLE
{
    BYTE **data;
    BOOL *data_persistent;
    UINT row_count;
    struct list entry;
    MSICOLUMNINFO *colinfo;
    UINT col_count;
    MSICONDITION persistent;
    INT ref_count;
    WCHAR name[1];
};

/* information for default tables */
static const WCHAR szTables[]  = {'_','T','a','b','l','e','s',0};
static const WCHAR szTable[]   = {'T','a','b','l','e',0};
static const WCHAR szColumns[] = {'_','C','o','l','u','m','n','s',0};
static const WCHAR szNumber[]  = {'N','u','m','b','e','r',0};
static const WCHAR szType[]    = {'T','y','p','e',0};

static const MSICOLUMNINFO _Columns_cols[4] = {
    { szColumns, 1, szTable,  MSITYPE_VALID | MSITYPE_STRING | MSITYPE_KEY | 64, 0, 0, 0, NULL },
    { szColumns, 2, szNumber, MSITYPE_VALID | MSITYPE_KEY | 2,     2, 0, 0, NULL },
    { szColumns, 3, szName,   MSITYPE_VALID | MSITYPE_STRING | 64, 4, 0, 0, NULL },
    { szColumns, 4, szType,   MSITYPE_VALID | 2,                   6, 0, 0, NULL },
};

static const MSICOLUMNINFO _Tables_cols[1] = {
    { szTables,  1, szName,   MSITYPE_VALID | MSITYPE_STRING | MSITYPE_KEY | 64, 0, 0, 0, NULL },
};

#define MAX_STREAM_NAME 0x1f

static inline UINT bytes_per_column( MSIDATABASE *db, const MSICOLUMNINFO *col, UINT bytes_per_strref )
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
    if (!(out = msi_alloc( count*sizeof(WCHAR) ))) return NULL;
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

static void msi_free_colinfo( MSICOLUMNINFO *colinfo, UINT count )
{
    UINT i;
    for (i = 0; i < count; i++) msi_free( colinfo[i].hash_table );
}

static void free_table( MSITABLE *table )
{
    UINT i;
    for( i=0; i<table->row_count; i++ )
        msi_free( table->data[i] );
    msi_free( table->data );
    msi_free( table->data_persistent );
    msi_free_colinfo( table->colinfo, table->col_count );
    msi_free( table->colinfo );
    msi_free( table );
}

static UINT msi_table_get_row_size( MSIDATABASE *db, const MSICOLUMNINFO *cols, UINT count, UINT bytes_per_strref )
{
    const MSICOLUMNINFO *last_col;

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

    row_size = msi_table_get_row_size( db, t->colinfo, t->col_count, db->bytes_per_strref );
    row_size_mem = msi_table_get_row_size( db, t->colinfo, t->col_count, LONG_STR_BYTES );

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
    t->data_persistent = msi_alloc_zero( t->row_count * sizeof(BOOL));
    if ( !t->data_persistent )
        goto err;

    /* transpose all the data */
    TRACE("Transposing data from %d rows\n", t->row_count );
    for (i = 0; i < t->row_count; i++)
    {
        UINT ofs = 0, ofs_mem = 0;

        t->data[i] = msi_alloc( row_size_mem );
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
        if( !strcmpW( name, t->name ) )
            return t;

    return NULL;
}

static void table_calc_column_offsets( MSIDATABASE *db, MSICOLUMNINFO *colinfo, DWORD count )
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

static UINT get_defaulttablecolumns( MSIDATABASE *db, LPCWSTR name, MSICOLUMNINFO *colinfo, UINT *sz )
{
    const MSICOLUMNINFO *p;
    DWORD i, n;

    TRACE("%s\n", debugstr_w(name));

    if (!strcmpW( name, szTables ))
    {
        p = _Tables_cols;
        n = 1;
    }
    else if (!strcmpW( name, szColumns ))
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

static UINT get_tablecolumns( MSIDATABASE *db, LPCWSTR szTableName, MSICOLUMNINFO *colinfo, UINT *sz );

static UINT table_get_column_info( MSIDATABASE *db, LPCWSTR name, MSICOLUMNINFO **pcols, UINT *pcount )
{
    UINT r, column_count = 0;
    MSICOLUMNINFO *columns;

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

    columns = msi_alloc( column_count * sizeof(MSICOLUMNINFO) );
    if (!columns)
        return ERROR_FUNCTION_FAILED;

    r = get_tablecolumns( db, name, columns, &column_count );
    if (r != ERROR_SUCCESS)
    {
        msi_free( columns );
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
    table = msi_alloc( sizeof(MSITABLE) + lstrlenW( name ) * sizeof(WCHAR) );
    if (!table)
        return ERROR_FUNCTION_FAILED;

    table->row_count = 0;
    table->data = NULL;
    table->data_persistent = NULL;
    table->colinfo = NULL;
    table->col_count = 0;
    table->persistent = MSICONDITION_TRUE;
    lstrcpyW( table->name, name );

    if (!strcmpW( name, szTables ) || !strcmpW( name, szColumns ))
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

static UINT get_tablecolumns( MSIDATABASE *db, LPCWSTR szTableName, MSICOLUMNINFO *colinfo, UINT *sz )
{
    UINT r, i, n = 0, table_id, count, maxcount = *sz;
    MSITABLE *table = NULL;

    TRACE("%s\n", debugstr_w(szTableName));

    /* first check if there is a default table with that name */
    r = get_defaulttablecolumns( db, szTableName, colinfo, sz );
    if (r == ERROR_SUCCESS && *sz)
        return r;

    r = get_table( db, szColumns, &table );
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
            colinfo[col - 1].ref_count = 0;
            colinfo[col - 1].hash_table = NULL;
        }
        n++;
    }
    TRACE("%s has %d columns\n", debugstr_w(szTableName), n);

    if (colinfo && n != maxcount)
    {
        ERR("missing column in table %s\n", debugstr_w(szTableName));
        msi_free_colinfo( colinfo, maxcount );
        return ERROR_FUNCTION_FAILED;
    }
    table_calc_column_offsets( db, colinfo, n );
    *sz = n;
    return ERROR_SUCCESS;
}

UINT msi_create_table( MSIDATABASE *db, LPCWSTR name, column_info *col_info,
                       MSICONDITION persistent )
{
    enum StringPersistence string_persistence = (persistent) ? StringPersistent : StringNonPersistent;
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
    table->data_persistent = NULL;
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
        UINT table_id = msi_add_string( db->strings, col->table, -1, string_persistence );
        UINT col_id = msi_add_string( db->strings, col->column, -1, string_persistence );

        table->colinfo[ i ].tablename = msi_string_lookup( db->strings, table_id, NULL );
        table->colinfo[ i ].number = i + 1;
        table->colinfo[ i ].colname = msi_string_lookup( db->strings, col_id, NULL );
        table->colinfo[ i ].type = col->type;
        table->colinfo[ i ].offset = 0;
        table->colinfo[ i ].ref_count = 0;
        table->colinfo[ i ].hash_table = NULL;
        table->colinfo[ i ].temporary = col->temporary;
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

    row_size = msi_table_get_row_size( db, t->colinfo, t->col_count, bytes_per_strref );
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
    rawdata = msi_alloc_zero( rawsize );
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
    msi_free( rawdata );
    return r;
}

static void msi_update_table_columns( MSIDATABASE *db, LPCWSTR name )
{
    MSITABLE *table;
    UINT size, offset, old_count;
    UINT n;

    if (!(table = find_cached_table( db, name ))) return;
    old_count = table->col_count;
    msi_free_colinfo( table->colinfo, table->col_count );
    msi_free( table->colinfo );
    table->colinfo = NULL;

    table_get_column_info( db, name, &table->colinfo, &table->col_count );
    if (!table->col_count) return;

    size = msi_table_get_row_size( db, table->colinfo, table->col_count, LONG_STR_BYTES );
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
    UINT r, table_id, i;
    MSITABLE *table;

    if( !strcmpW( name, szTables ) || !strcmpW( name, szColumns ) ||
        !strcmpW( name, szStreams ) || !strcmpW( name, szStorages ) )
        return TRUE;

    r = msi_string2id( db->strings, name, -1, &table_id );
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

    for( i = 0; i < table->row_count; i++ )
    {
        if( read_table_int( table->data, i, 0, LONG_STR_BYTES ) == table_id )
            return TRUE;
    }

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

static UINT get_stream_name( const MSITABLEVIEW *tv, UINT row, WCHAR **pstname )
{
    LPWSTR p, stname = NULL;
    UINT i, r, type, ival;
    DWORD len;
    LPCWSTR sval;
    MSIVIEW *view = (MSIVIEW *) tv;

    TRACE("%p %d\n", tv, row);

    len = lstrlenW( tv->name ) + 1;
    stname = msi_alloc( len*sizeof(WCHAR) );
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
                static const WCHAR fmt[] = { '%','d',0 };
                UINT n = bytes_per_column( tv->db, &tv->columns[i], LONG_STR_BYTES );

                switch( n )
                {
                case 2:
                    sprintfW( number, fmt, ival-0x8000 );
                    break;
                case 4:
                    sprintfW( number, fmt, ival^0x80000000 );
                    break;
                default:
                    ERR( "oops - unknown column width %d\n", n );
                    r = ERROR_FUNCTION_FAILED;
                    goto err;
                }
                sval = number;
            }

            len += lstrlenW( szDot ) + lstrlenW( sval );
            p = msi_realloc ( stname, len*sizeof(WCHAR) );
            if ( !p )
            {
                r = ERROR_OUTOFMEMORY;
                goto err;
            }
            stname = p;

            lstrcatW( stname, szDot );
            lstrcatW( stname, sval );
        }
        else
           continue;
    }

    *pstname = stname;
    return ERROR_SUCCESS;

err:
    msi_free( stname );
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
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
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

    msi_free( name );
    return r;
}

static UINT TABLE_set_int( MSITABLEVIEW *tv, UINT row, UINT col, UINT val )
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

    msi_free( tv->columns[col-1].hash_table );
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

static UINT TABLE_get_row( struct tagMSIVIEW *view, UINT row, MSIRECORD **rec )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    return msi_view_get_row(tv->db, view, row, rec);
}

static UINT add_stream( MSIDATABASE *db, const WCHAR *name, IStream *data )
{
    static const WCHAR insert[] = {
        'I','N','S','E','R','T',' ','I','N','T','O',' ',
        '`','_','S','t','r','e','a','m','s','`',' ',
        '(','`','N','a','m','e','`',',','`','D','a','t','a','`',')',' ',
        'V','A','L','U','E','S',' ','(','?',',','?',')',0};
    static const WCHAR update[] = {
        'U','P','D','A','T','E',' ','`','_','S','t','r','e','a','m','s','`',' ',
        'S','E','T',' ','`','D','a','t','a','`',' ','=',' ','?',' ',
        'W','H','E','R','E',' ','`','N','a','m','e','`',' ','=',' ','?',0};
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

    r = MSI_DatabaseOpenViewW( db, insert, &query );
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

    r = MSI_DatabaseOpenViewW( db, update, &query );
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewExecute( query, rec );
    msiobj_release( &query->hdr );

done:
    msiobj_release( &rec->hdr );
    return r;
}

static UINT get_table_value_from_record( MSITABLEVIEW *tv, MSIRECORD *rec, UINT iField, UINT *pvalue )
{
    MSICOLUMNINFO columninfo;
    UINT r;
    int ival;

    if ( (iField <= 0) ||
         (iField > tv->num_cols) ||
          MSI_RecordIsNull( rec, iField ) )
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
    else if ( bytes_per_column( tv->db, &columninfo, LONG_STR_BYTES ) == 2 )
    {
        ival = MSI_RecordGetInteger( rec, iField );
        if (ival == 0x80000000) *pvalue = 0x8000;
        else
        {
            *pvalue = 0x8000 + MSI_RecordGetInteger( rec, iField );
            if (*pvalue & 0xffff0000)
            {
                ERR("field %u value %d out of range\n", iField, *pvalue - 0x8000);
                return ERROR_FUNCTION_FAILED;
            }
        }
    }
    else
    {
        ival = MSI_RecordGetInteger( rec, iField );
        *pvalue = ival ^ 0x80000000;
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
                msi_free ( stname );

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
                    val = msi_add_string( tv->db->strings, sval, len,
                                          persistent ? StringPersistent : StringNonPersistent );
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
    BOOL *b;
    UINT sz;
    BYTE ***data_ptr;
    BOOL **data_persist_ptr;
    UINT *row_count;

    TRACE("%p %s\n", view, temporary ? "TRUE" : "FALSE");

    if( !tv->table )
        return ERROR_INVALID_PARAMETER;

    row = msi_alloc_zero( tv->row_size );
    if( !row )
        return ERROR_NOT_ENOUGH_MEMORY;

    row_count = &tv->table->row_count;
    data_ptr = &tv->table->data;
    data_persist_ptr = &tv->table->data_persistent;
    if (*num == -1)
        *num = tv->table->row_count;

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

    sz = (*row_count + 1) * sizeof (BOOL);
    if( *data_persist_ptr )
        b = msi_realloc( *data_persist_ptr, sz );
    else
        b = msi_alloc( sz );
    if( !b )
    {
        msi_free( row );
        msi_free( p );
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
        *rows = tv->table->row_count;
    }

    return ERROR_SUCCESS;
}

static UINT TABLE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPCWSTR *name, UINT *type, BOOL *temporary,
                LPCWSTR *table_name )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

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
        *temporary = tv->columns[n-1].temporary;

    return ERROR_SUCCESS;
}

static UINT msi_table_find_row( MSITABLEVIEW *tv, MSIRECORD *rec, UINT *row, UINT *column );

static UINT table_validate_new( MSITABLEVIEW *tv, MSIRECORD *rec, UINT *column )
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
    r = msi_table_find_row( tv, rec, &row, column );
    if (r == ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

static int compare_record( MSITABLEVIEW *tv, UINT row, MSIRECORD *rec )
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

static int find_insert_index( MSITABLEVIEW *tv, MSIRECORD *rec )
{
    int idx, c, low = 0, high = tv->table->row_count - 1;

    TRACE("%p %p\n", tv, rec);

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
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT i, r;

    TRACE("%p %p %s\n", tv, rec, temporary ? "TRUE" : "FALSE" );

    /* check that the key is unique - can we find a matching row? */
    r = table_validate_new( tv, rec, NULL );
    if( r != ERROR_SUCCESS )
        return ERROR_FUNCTION_FAILED;

    if (row == -1)
        row = find_insert_index( tv, rec );

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
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
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
        msi_free( tv->columns[i].hash_table );
        tv->columns[i].hash_table = NULL;
    }

    for (i = row + 1; i < num_rows; i++)
    {
        memcpy(tv->table->data[i - 1], tv->table->data[i], tv->row_size);
        tv->table->data_persistent[i - 1] = tv->table->data_persistent[i];
    }

    msi_free(tv->table->data[num_rows - 1]);

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

    r = msi_table_find_row(tv, rec, &new_row, NULL);
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

static UINT msi_table_assign(struct tagMSIVIEW *view, MSIRECORD *rec)
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;
    UINT r, row;

    if (!tv->table)
        return ERROR_INVALID_PARAMETER;

    r = msi_table_find_row(tv, rec, &row, NULL);
    if (r == ERROR_SUCCESS)
        return TABLE_set_row(view, row, rec, (1 << tv->num_cols) - 1);
    else
        return TABLE_insert_row( view, rec, -1, FALSE );
}

static UINT modify_delete_row( struct tagMSIVIEW *view, MSIRECORD *rec )
{
    MSITABLEVIEW *tv = (MSITABLEVIEW *)view;
    UINT row, r;

    r = msi_table_find_row(tv, rec, &row, NULL);
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
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;
    UINT r, frow, column;

    TRACE("%p %d %p\n", view, eModifyMode, rec );

    switch (eModifyMode)
    {
    case MSIMODIFY_DELETE:
        r = modify_delete_row( view, rec );
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
        r = msi_refresh_record( view, rec, row );
        break;

    case MSIMODIFY_UPDATE:
        r = msi_table_update( view, rec, row );
        break;

    case MSIMODIFY_ASSIGN:
        r = msi_table_assign( view, rec );
        break;

    case MSIMODIFY_MERGE:
        /* check row that matches this record */
        r = msi_table_find_row( tv, rec, &frow, &column );
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
    MSITABLEVIEW *tv = (MSITABLEVIEW*)view;

    TRACE("%p\n", view );

    tv->table = NULL;
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
    {
        msiobj_release(&rec->hdr);
        return r;
    }

    r = msi_table_find_row((MSITABLEVIEW *)columns, rec, &row, NULL);
    if (r != ERROR_SUCCESS)
        goto done;

    r = TABLE_delete_row(columns, row);
    if (r != ERROR_SUCCESS)
        goto done;

    msi_update_table_columns(tv->db, table);

done:
    msiobj_release(&rec->hdr);
    columns->ops->delete(columns);
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

    r = TABLE_insert_row(&tv->view, rec, -1, FALSE);
    if (r != ERROR_SUCCESS)
        goto done;

    msi_update_table_columns(tv->db, table);

    if (!hold)
        goto done;

    msitable = find_cached_table(tv->db, table);
    for (i = 0; i < msitable->col_count; i++)
    {
        if (!strcmpW( msitable->colinfo[i].colname, column ))
        {
            InterlockedIncrement(&msitable->colinfo[i].ref_count);
            break;
        }
    }

done:
    msiobj_release(&rec->hdr);
    return r;
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
    {
        msiobj_release(&rec->hdr);
        return r;
    }

    r = msi_table_find_row((MSITABLEVIEW *)tables, rec, &row, NULL);
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
    NULL,
    TABLE_drop,
};

UINT TABLE_CreateView( MSIDATABASE *db, LPCWSTR name, MSIVIEW **view )
{
    MSITABLEVIEW *tv ;
    UINT r, sz;

    TRACE("%p %s %p\n", db, debugstr_w(name), view );

    if ( !strcmpW( name, szStreams ) )
        return STREAMS_CreateView( db, view );
    else if ( !strcmpW( name, szStorages ) )
        return STORAGES_CreateView( db, view );

    sz = FIELD_OFFSET( MSITABLEVIEW, name[lstrlenW( name ) + 1] );
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
    tv->row_size = msi_table_get_row_size( db, tv->table->colinfo, tv->table->col_count, LONG_STR_BYTES );

    TRACE("%s one row is %d bytes\n", debugstr_w(name), tv->row_size );

    *view = (MSIVIEW*) tv;
    lstrcpyW( tv->name, name );

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
        WARN("failed to commit changes 0x%08x\n", hr);
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

static UINT msi_record_encoded_stream_name( const MSITABLEVIEW *tv, MSIRECORD *rec, LPWSTR *pstname )
{
    LPWSTR stname = NULL, sval, p;
    DWORD len;
    UINT i, r;

    TRACE("%p %p\n", tv, rec);

    len = lstrlenW( tv->name ) + 1;
    stname = msi_alloc( len*sizeof(WCHAR) );
    if ( !stname )
    {
       r = ERROR_OUTOFMEMORY;
       goto err;
    }

    lstrcpyW( stname, tv->name );

    for ( i = 0; i < tv->num_cols; i++ )
    {
        if ( tv->columns[i].type & MSITYPE_KEY )
        {
            sval = msi_dup_record_field( rec, i + 1 );
            if ( !sval )
            {
                r = ERROR_OUTOFMEMORY;
                goto err;
            }

            len += lstrlenW( szDot ) + lstrlenW ( sval );
            p = msi_realloc ( stname, len*sizeof(WCHAR) );
            if ( !p )
            {
                r = ERROR_OUTOFMEMORY;
                msi_free(sval);
                goto err;
            }
            stname = p;

            lstrcatW( stname, szDot );
            lstrcatW( stname, sval );

            msi_free( sval );
        }
        else
            continue;
    }

    *pstname = encode_streamname( FALSE, stname );
    msi_free( stname );

    return ERROR_SUCCESS;

err:
    msi_free ( stname );
    *pstname = NULL;
    return r;
}

static MSIRECORD *msi_get_transform_record( const MSITABLEVIEW *tv, const string_table *st,
                                            IStorage *stg,
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

        if( MSITYPE_IS_BINARY(tv->columns[i].type) )
        {
            LPWSTR encname;
            IStream *stm = NULL;
            UINT r;

            ofs += bytes_per_column( tv->db, &columns[i], bytes_per_strref );

            r = msi_record_encoded_stream_name( tv, rec, &encname );
            if ( r != ERROR_SUCCESS )
                return NULL;

            r = IStorage_OpenStream( stg, encname, NULL,
                     STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm );
            if ( r != ERROR_SUCCESS )
            {
                msi_free( encname );
                return NULL;
            }

            MSI_RecordSetStream( rec, i+1, stm );
            TRACE(" field %d [%s]\n", i+1, debugstr_w(encname));
            msi_free( encname );
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

static void dump_record( MSIRECORD *rec )
{
    UINT i, n;

    n = MSI_RecordGetFieldCount( rec );
    for( i=1; i<=n; i++ )
    {
        int len;
        const WCHAR *sval;

        if( MSI_RecordIsNull( rec, i ) )
            TRACE("row -> []\n");
        else if( (sval = msi_record_get_string( rec, i, &len )) )
            TRACE("row -> [%s]\n", debugstr_wn(sval, len));
        else
            TRACE("row -> [0x%08x]\n", MSI_RecordGetInteger( rec, i ) );
    }
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

static UINT* msi_record_to_row( const MSITABLEVIEW *tv, MSIRECORD *rec )
{
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
            int len;
            const WCHAR *str = msi_record_get_string( rec, i+1, &len );
            if (str)
            {
                r = msi_string2id( tv->db->strings, str, len, &data[i] );

                /* if there's no matching string in the string table,
                   these keys can't match any record, so fail now. */
                if (r != ERROR_SUCCESS)
                {
                    msi_free( data );
                    return NULL;
                }
            }
            else data[i] = 0;
        }
        else
        {
            data[i] = MSI_RecordGetInteger( rec, i+1 );

            if (data[i] == MSI_NULL_INTEGER)
                data[i] = 0;
            else if ((tv->columns[i].type&0xff) == 2)
                data[i] += 0x8000;
            else
                data[i] += 0x80000000;
        }
    }
    return data;
}

static UINT msi_row_matches( MSITABLEVIEW *tv, UINT row, const UINT *data, UINT *column )
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

static UINT msi_table_find_row( MSITABLEVIEW *tv, MSIRECORD *rec, UINT *row, UINT *column )
{
    UINT i, r = ERROR_FUNCTION_FAILED, *data;

    data = msi_record_to_row( tv, rec );
    if( !data )
        return r;
    for( i = 0; i < tv->table->row_count; i++ )
    {
        r = msi_row_matches( tv, i, data, column );
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
    BYTE *rawdata = NULL;
    MSITABLEVIEW *tv = NULL;
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

        rec = msi_get_transform_record( tv, st, stg, &rawdata[n], bytes_per_strref );
        if (rec)
        {
            WCHAR table[32];
            DWORD sz = 32;
            UINT number = MSI_NULL_INTEGER;
            UINT row = 0;

            if (!strcmpW( name, szColumns ))
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
                    if (strcmpW( coltable, table ))
                    {
                        colcol = 0;
                        lstrcpyW( coltable, table );
                    }

                    /* fix nul column numbers */
                    MSI_RecordSetInteger( rec, 2, ++colcol );
                }
            }

            if (TRACE_ON(msidb)) dump_record( rec );

            r = msi_table_find_row( tv, rec, &row, NULL );
            if (r == ERROR_SUCCESS)
            {
                if (!mask)
                {
                    TRACE("deleting row [%d]:\n", row);
                    r = TABLE_delete_row( &tv->view, row );
                    if (r != ERROR_SUCCESS)
                        WARN("failed to delete row %u\n", r);
                }
                else if (mask & 1)
                {
                    TRACE("modifying full row [%d]:\n", row);
                    r = TABLE_set_row( &tv->view, row, rec, (1 << tv->num_cols) - 1 );
                    if (r != ERROR_SUCCESS)
                        WARN("failed to modify row %u\n", r);
                }
                else
                {
                    TRACE("modifying masked row [%d]:\n", row);
                    r = TABLE_set_row( &tv->view, row, rec, mask );
                    if (r != ERROR_SUCCESS)
                        WARN("failed to modify row %u\n", r);
                }
            }
            else
            {
                TRACE("inserting row\n");
                r = TABLE_insert_row( &tv->view, rec, -1, FALSE );
                if (r != ERROR_SUCCESS)
                    WARN("failed to insert row %u\n", r);
            }

            if (!strcmpW( name, szColumns ))
                msi_update_table_columns( db, table );

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
    BOOL property_update = FALSE;

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

        if ( !strcmpW( name+1, szStringPool ) ||
             !strcmpW( name+1, szStringData ) )
            continue;

        transform = msi_alloc_zero( sizeof(TRANSFORMDATA) );
        if ( !transform )
            break;

        list_add_tail( &transforms, &transform->entry );

        transform->name = strdupW( name + 1 );

        if ( !strcmpW( transform->name, szTables ) )
            tables = transform;
        else if (!strcmpW( transform->name, szColumns ) )
            columns = transform;
        else if (!strcmpW( transform->name, szProperty ))
            property_update = TRUE;

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

        if ( strcmpW( transform->name, szColumns ) &&
             strcmpW( transform->name, szTables ) &&
             ret == ERROR_SUCCESS )
        {
            ret = msi_table_load_transform( db, stg, strings, transform, bytes_per_strref );
        }

        list_remove( &transform->entry );
        msi_free( transform->name );
        msi_free( transform );
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

    return ret;
}
