/*
 * String Table Functions
 *
 * Copyright 2002-2004, Mike McCormack for CodeWeavers
 * Copyright 2007 Robert Shearman for CodeWeavers
 * Copyright 2010 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

struct msistring
{
    USHORT persistent_refcount;
    USHORT nonpersistent_refcount;
    WCHAR *data;
    int    len;
};

struct string_table
{
    UINT maxcount;         /* the number of strings */
    UINT freeslot;
    UINT codepage;
    UINT sortcount;
    struct msistring *strings; /* an array of strings */
    UINT *sorted;              /* index */
};

static BOOL validate_codepage( UINT codepage )
{
    if (codepage != CP_ACP && !IsValidCodePage( codepage ))
    {
        WARN("invalid codepage %u\n", codepage);
        return FALSE;
    }
    return TRUE;
}

static string_table *init_stringtable( int entries, UINT codepage )
{
    string_table *st;

    if (!validate_codepage( codepage ))
        return NULL;

    st = malloc( sizeof(string_table) );
    if( !st )
        return NULL;
    if( entries < 1 )
        entries = 1;

    st->strings = calloc( entries, sizeof(struct msistring) );
    if( !st->strings )
    {
        free( st );
        return NULL;
    }

    st->sorted = malloc( sizeof(UINT) * entries );
    if( !st->sorted )
    {
        free( st->strings );
        free( st );
        return NULL;
    }

    st->maxcount = entries;
    st->freeslot = 1;
    st->codepage = codepage;
    st->sortcount = 0;

    return st;
}

VOID msi_destroy_stringtable( string_table *st )
{
    UINT i;

    for( i=0; i<st->maxcount; i++ )
    {
        if( st->strings[i].persistent_refcount ||
            st->strings[i].nonpersistent_refcount )
            free( st->strings[i].data );
    }
    free( st->strings );
    free( st->sorted );
    free( st );
}

static int st_find_free_entry( string_table *st )
{
    UINT i, sz, *s;
    struct msistring *p;

    TRACE("%p\n", st);

    if( st->freeslot )
    {
        for( i = st->freeslot; i < st->maxcount; i++ )
            if( !st->strings[i].persistent_refcount &&
                !st->strings[i].nonpersistent_refcount )
                return i;
    }
    for( i = 1; i < st->maxcount; i++ )
        if( !st->strings[i].persistent_refcount &&
            !st->strings[i].nonpersistent_refcount )
            return i;

    /* dynamically resize */
    sz = st->maxcount + 1 + st->maxcount / 2;
    if (!(p = realloc( st->strings, sz * sizeof(*p) ))) return -1;
    memset( p + st->maxcount, 0, (sz - st->maxcount) * sizeof(*p) );

    if (!(s = realloc( st->sorted, sz * sizeof(*s) )))
    {
        free( p );
        return -1;
    }

    st->strings = p;
    st->sorted = s;

    st->freeslot = st->maxcount;
    st->maxcount = sz;
    if( st->strings[st->freeslot].persistent_refcount ||
        st->strings[st->freeslot].nonpersistent_refcount )
        ERR("oops. expected freeslot to be free...\n");
    return st->freeslot;
}

static inline int cmp_string( const WCHAR *str1, int len1, const WCHAR *str2, int len2 )
{
    if (len1 < len2) return -1;
    else if (len1 > len2) return 1;
    while (len1)
    {
        if (*str1 == *str2) { str1++; str2++; }
        else return *str1 - *str2;
        len1--;
    }
    return 0;
}

static int find_insert_index( const string_table *st, UINT string_id )
{
    int i, c, low = 0, high = st->sortcount - 1;

    while (low <= high)
    {
        i = (low + high) / 2;
        c = cmp_string( st->strings[string_id].data, st->strings[string_id].len,
                        st->strings[st->sorted[i]].data, st->strings[st->sorted[i]].len );
        if (c < 0)
            high = i - 1;
        else if (c > 0)
            low = i + 1;
        else
            return -1; /* already exists */
    }
    return high + 1;
}

static void insert_string_sorted( string_table *st, UINT string_id )
{
    int i;

    i = find_insert_index( st, string_id );
    if (i == -1)
        return;

    memmove( &st->sorted[i] + 1, &st->sorted[i], (st->sortcount - i) * sizeof(UINT) );
    st->sorted[i] = string_id;
    st->sortcount++;
}

static void set_st_entry( string_table *st, UINT n, WCHAR *str, int len, USHORT refcount,
                          BOOL persistent )
{
    if (persistent)
    {
        st->strings[n].persistent_refcount = refcount;
        st->strings[n].nonpersistent_refcount = 0;
    }
    else
    {
        st->strings[n].persistent_refcount = 0;
        st->strings[n].nonpersistent_refcount = refcount;
    }

    st->strings[n].data = str;
    st->strings[n].len  = len;

    insert_string_sorted( st, n );

    if( n < st->maxcount )
        st->freeslot = n + 1;
}

static UINT string2id( const string_table *st, const char *buffer, UINT *id )
{
    int sz;
    UINT r = ERROR_INVALID_PARAMETER;
    LPWSTR str;

    TRACE("Finding string %s in string table\n", debugstr_a(buffer) );

    if( buffer[0] == 0 )
    {
        *id = 0;
        return ERROR_SUCCESS;
    }

    if (!(sz = MultiByteToWideChar( st->codepage, 0, buffer, -1, NULL, 0 )))
        return r;
    str = malloc( sz * sizeof(WCHAR) );
    if( !str )
        return ERROR_NOT_ENOUGH_MEMORY;
    MultiByteToWideChar( st->codepage, 0, buffer, -1, str, sz );

    r = msi_string2id( st, str, sz - 1, id );
    free( str );
    return r;
}

static int add_string( string_table *st, UINT n, const char *data, UINT len, USHORT refcount, BOOL persistent )
{
    LPWSTR str;
    int sz;

    if( !data || !len )
        return 0;
    if( n > 0 )
    {
        if( st->strings[n].persistent_refcount ||
            st->strings[n].nonpersistent_refcount )
            return -1;
    }
    else
    {
        if (string2id( st, data, &n ) == ERROR_SUCCESS)
        {
            if (persistent)
                st->strings[n].persistent_refcount += refcount;
            else
                st->strings[n].nonpersistent_refcount += refcount;
            return n;
        }
        n = st_find_free_entry( st );
        if( n == -1 )
            return -1;
    }

    if( n < 1 )
    {
        ERR("invalid index adding %s (%d)\n", debugstr_a( data ), n );
        return -1;
    }

    /* allocate a new string */
    sz = MultiByteToWideChar( st->codepage, 0, data, len, NULL, 0 );
    str = malloc( (sz + 1) * sizeof(WCHAR) );
    if( !str )
        return -1;
    MultiByteToWideChar( st->codepage, 0, data, len, str, sz );
    str[sz] = 0;

    set_st_entry( st, n, str, sz, refcount, persistent );
    return n;
}

int msi_add_string( string_table *st, const WCHAR *data, int len, BOOL persistent )
{
    UINT n;
    LPWSTR str;

    if( !data )
        return 0;

    if (len < 0) len = lstrlenW( data );

    if( !data[0] && !len )
        return 0;

    if (msi_string2id( st, data, len, &n) == ERROR_SUCCESS )
    {
        if (persistent)
            st->strings[n].persistent_refcount++;
        else
            st->strings[n].nonpersistent_refcount++;
        return n;
    }

    n = st_find_free_entry( st );
    if( n == -1 )
        return -1;

    /* allocate a new string */
    TRACE( "%s, n = %d len = %d\n", debugstr_wn(data, len), n, len );

    str = malloc( (len + 1) * sizeof(WCHAR) );
    if( !str )
        return -1;
    memcpy( str, data, len*sizeof(WCHAR) );
    str[len] = 0;

    set_st_entry( st, n, str, len, 1, persistent );
    return n;
}

/* find the string identified by an id - return null if there's none */
const WCHAR *msi_string_lookup( const string_table *st, UINT id, int *len )
{
    if( id == 0 )
    {
        if (len) *len = 0;
        return L"";
    }
    if( id >= st->maxcount )
        return NULL;

    if( id && !st->strings[id].persistent_refcount && !st->strings[id].nonpersistent_refcount)
        return NULL;

    if (len) *len = st->strings[id].len;

    return st->strings[id].data;
}

/*
 *  id2string
 *
 *  [in] st         - pointer to the string table
 *  [in] id         - id of the string to retrieve
 *  [out] buffer    - destination of the UTF8 string
 *  [in/out] sz     - number of bytes available in the buffer on input
 *                    number of bytes used on output
 *
 *  Returned string is not nul terminated.
 */
static UINT id2string( const string_table *st, UINT id, char *buffer, UINT *sz )
{
    int len, lenW;
    const WCHAR *str;

    TRACE("Finding string %d of %d\n", id, st->maxcount);

    str = msi_string_lookup( st, id, &lenW );
    if( !str )
        return ERROR_FUNCTION_FAILED;

    len = WideCharToMultiByte( st->codepage, 0, str, lenW, NULL, 0, NULL, NULL );
    if( *sz < len )
    {
        *sz = len;
        return ERROR_MORE_DATA;
    }
    *sz = WideCharToMultiByte( st->codepage, 0, str, lenW, buffer, *sz, NULL, NULL );
    return ERROR_SUCCESS;
}

/*
 *  msi_string2id
 *
 *  [in] st         - pointer to the string table
 *  [in] str        - string to find in the string table
 *  [out] id        - id of the string, if found
 */
UINT msi_string2id( const string_table *st, const WCHAR *str, int len, UINT *id )
{
    int i, c, low = 0, high = st->sortcount - 1;

    if (len < 0) len = lstrlenW( str );

    while (low <= high)
    {
        i = (low + high) / 2;
        c = cmp_string( str, len, st->strings[st->sorted[i]].data, st->strings[st->sorted[i]].len );

        if (c < 0)
            high = i - 1;
        else if (c > 0)
            low = i + 1;
        else
        {
            *id = st->sorted[i];
            return ERROR_SUCCESS;
        }
    }
    return ERROR_INVALID_PARAMETER;
}

static void string_totalsize( const string_table *st, UINT *datasize, UINT *poolsize )
{
    UINT i, len, holesize;

    if( st->strings[0].data || st->strings[0].persistent_refcount || st->strings[0].nonpersistent_refcount)
        ERR("oops. element 0 has a string\n");

    *poolsize = 4;
    *datasize = 0;
    holesize = 0;
    for( i=1; i<st->maxcount; i++ )
    {
        if( !st->strings[i].persistent_refcount )
        {
            TRACE("[%u] nonpersistent = %s\n", i, debugstr_wn(st->strings[i].data, st->strings[i].len));
            (*poolsize) += 4;
        }
        else if( st->strings[i].data )
        {
            TRACE("[%u] = %s\n", i, debugstr_wn(st->strings[i].data, st->strings[i].len));
            len = WideCharToMultiByte( st->codepage, 0, st->strings[i].data, st->strings[i].len + 1,
                                       NULL, 0, NULL, NULL);
            if( len )
                len--;
            (*datasize) += len;
            if (len>0xffff)
                (*poolsize) += 4;
            (*poolsize) += holesize + 4;
            holesize = 0;
        }
        else
            holesize += 4;
    }
    TRACE("data %u pool %u codepage %x\n", *datasize, *poolsize, st->codepage );
}

HRESULT msi_init_string_table( IStorage *stg )
{
    USHORT zero[2] = { 0, 0 };
    UINT ret;

    /* create the StringPool stream... add the zero string to it*/
    ret = write_stream_data(stg, L"_StringPool", zero, sizeof zero, TRUE);
    if (ret != ERROR_SUCCESS)
        return E_FAIL;

    /* create the StringData stream... make it zero length */
    ret = write_stream_data(stg, L"_StringData", NULL, 0, TRUE);
    if (ret != ERROR_SUCCESS)
        return E_FAIL;

    return S_OK;
}

string_table *msi_load_string_table( IStorage *stg, UINT *bytes_per_strref )
{
    string_table *st = NULL;
    CHAR *data = NULL;
    USHORT *pool = NULL;
    UINT r, datasize = 0, poolsize = 0, codepage;
    DWORD i, count, offset, len, n, refs;

    r = read_stream_data( stg, L"_StringPool", TRUE, (BYTE **)&pool, &poolsize );
    if( r != ERROR_SUCCESS)
        goto end;
    r = read_stream_data( stg, L"_StringData", TRUE, (BYTE **)&data, &datasize );
    if( r != ERROR_SUCCESS)
        goto end;

    if ( (poolsize > 4) && (pool[1] & 0x8000) )
        *bytes_per_strref = LONG_STR_BYTES;
    else
        *bytes_per_strref = sizeof(USHORT);

    count = poolsize/4;
    if( poolsize > 4 )
        codepage = pool[0] | ( (pool[1] & ~0x8000) << 16 );
    else
        codepage = CP_ACP;
    st = init_stringtable( count, codepage );
    if (!st)
        goto end;

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

        r = add_string( st, n, data+offset, len, refs, TRUE );
        if( r != n )
            ERR( "Failed to add string %lu\n", n );
        n++;
        offset += len;
    }

    if ( datasize != offset )
        ERR( "string table load failed! (%u != %lu), please report\n", datasize, offset );

    TRACE( "loaded %lu strings\n", count );

end:
    free( pool );
    free( data );

    return st;
}

UINT msi_save_string_table( const string_table *st, IStorage *storage, UINT *bytes_per_strref )
{
    UINT i, datasize = 0, poolsize = 0, sz, used, r, codepage, n;
    UINT ret = ERROR_FUNCTION_FAILED;
    CHAR *data = NULL;
    USHORT *pool = NULL;

    TRACE("\n");

    /* construct the new table in memory first */
    string_totalsize( st, &datasize, &poolsize );

    TRACE("%u %u %u\n", st->maxcount, datasize, poolsize );

    pool = malloc( poolsize );
    if( ! pool )
    {
        WARN("Failed to alloc pool %d bytes\n", poolsize );
        goto err;
    }
    data = malloc( datasize );
    if( ! data )
    {
        WARN("Failed to alloc data %d bytes\n", datasize );
        goto err;
    }

    used = 0;
    codepage = st->codepage;
    pool[0] = codepage & 0xffff;
    pool[1] = codepage >> 16;
    if (st->maxcount > 0xffff)
    {
        pool[1] |= 0x8000;
        *bytes_per_strref = LONG_STR_BYTES;
    }
    else
        *bytes_per_strref = sizeof(USHORT);

    n = 1;
    for( i=1; i<st->maxcount; i++ )
    {
        if( !st->strings[i].persistent_refcount )
        {
            pool[ n*2 ] = 0;
            pool[ n*2 + 1] = 0;
            n++;
            continue;
        }

        sz = datasize - used;
        r = id2string( st, i, data+used, &sz );
        if( r != ERROR_SUCCESS )
        {
            ERR("failed to fetch string\n");
            sz = 0;
        }

        if (sz)
            pool[ n*2 + 1 ] = st->strings[i].persistent_refcount;
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
    r = write_stream_data( storage, L"_StringData", data, datasize, TRUE );
    TRACE("Wrote StringData r=%08x\n", r);
    if( r )
        goto err;
    r = write_stream_data( storage, L"_StringPool", pool, poolsize, TRUE );
    TRACE("Wrote StringPool r=%08x\n", r);
    if( r )
        goto err;

    ret = ERROR_SUCCESS;

err:
    free( data );
    free( pool );

    return ret;
}

UINT msi_get_string_table_codepage( const string_table *st )
{
    return st->codepage;
}

UINT msi_set_string_table_codepage( string_table *st, UINT codepage )
{
    if (validate_codepage( codepage ))
    {
        st->codepage = codepage;
        return ERROR_SUCCESS;
    }
    return ERROR_FUNCTION_FAILED;
}
