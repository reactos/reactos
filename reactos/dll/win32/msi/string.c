/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004, Mike McCormack for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define HASH_SIZE 0x101

typedef struct _msistring
{
    int hash_next;
    UINT refcount;
    LPWSTR str;
} msistring;

struct string_table
{
    UINT maxcount;         /* the number of strings */
    UINT freeslot;
    UINT codepage;
    int hash[HASH_SIZE];
    msistring *strings; /* an array of strings (in the tree) */
};

static UINT msistring_makehash( const WCHAR *str )
{
    UINT hash = 0;

    if (str==NULL)
        return hash;

    while( *str )
    {
        hash ^= *str++;
        hash *= 53;
        hash = (hash<<5) | (hash>>27);
    }
    return hash % HASH_SIZE;
}

string_table *msi_init_stringtable( int entries, UINT codepage )
{
    string_table *st;
    int i;

    st = msi_alloc( sizeof (string_table) );
    if( !st )
        return NULL;    
    if( entries < 1 )
        entries = 1;
    st->strings = msi_alloc_zero( sizeof (msistring) * entries );
    if( !st->strings )
    {
        msi_free( st );
        return NULL;    
    }
    st->maxcount = entries;
    st->freeslot = 1;
    st->codepage = codepage;

    for( i=0; i<HASH_SIZE; i++ )
        st->hash[i] = -1;

    return st;
}

VOID msi_destroy_stringtable( string_table *st )
{
    UINT i;

    for( i=0; i<st->maxcount; i++ )
    {
        if( st->strings[i].refcount )
            msi_free( st->strings[i].str );
    }
    msi_free( st->strings );
    msi_free( st );
}

static int st_find_free_entry( string_table *st )
{
    UINT i, sz;
    msistring *p;

    TRACE("%p\n", st);

    if( st->freeslot )
    {
        for( i = st->freeslot; i < st->maxcount; i++ )
            if( !st->strings[i].refcount )
                return i;
    }
    for( i = 1; i < st->maxcount; i++ )
        if( !st->strings[i].refcount )
            return i;

    /* dynamically resize */
    sz = st->maxcount + 1 + st->maxcount/2;
    p = msi_realloc_zero( st->strings, sz*sizeof(msistring) );
    if( !p )
        return -1;
    st->strings = p;
    st->freeslot = st->maxcount;
    st->maxcount = sz;
    if( st->strings[st->freeslot].refcount )
        ERR("oops. expected freeslot to be free...\n");
    return st->freeslot;
}

static void set_st_entry( string_table *st, UINT n, LPWSTR str )
{
    UINT hash = msistring_makehash( str );

    st->strings[n].refcount = 1;
    st->strings[n].str = str;

    st->strings[n].hash_next = st->hash[hash];
    st->hash[hash] = n;

    if( n < st->maxcount )
        st->freeslot = n + 1;
}

int msi_addstring( string_table *st, UINT n, const CHAR *data, int len, UINT refcount )
{
    LPWSTR str;
    int sz;

    if( !data )
        return 0;
    if( !data[0] )
        return 0;
    if( n > 0 )
    {
        if( st->strings[n].refcount )
            return -1;
    }
    else
    {
        if( ERROR_SUCCESS == msi_string2idA( st, data, &n ) )
        {
            st->strings[n].refcount++;
            return n;
        }
        n = st_find_free_entry( st );
        if( n < 0 )
            return -1;
    }

    if( n < 1 )
    {
        ERR("invalid index adding %s (%d)\n", debugstr_a( data ), n );
        return -1;
    }

    /* allocate a new string */
    if( len < 0 )
        len = strlen(data);
    sz = MultiByteToWideChar( st->codepage, 0, data, len, NULL, 0 );
    str = msi_alloc( (sz+1)*sizeof(WCHAR) );
    if( !str )
        return -1;
    MultiByteToWideChar( st->codepage, 0, data, len, str, sz );
    str[sz] = 0;

    set_st_entry( st, n, str );

    return n;
}

int msi_addstringW( string_table *st, UINT n, const WCHAR *data, int len, UINT refcount )
{
    LPWSTR str;

    /* TRACE("[%2d] = %s\n", string_no, debugstr_an(data,len) ); */

    if( !data )
        return 0;
    if( !data[0] )
        return 0;
    if( n > 0 )
    {
        if( st->strings[n].refcount )
            return -1;
    }
    else
    {
        if( ERROR_SUCCESS == msi_string2idW( st, data, &n ) )
        {
            st->strings[n].refcount++;
            return n;
        }
        n = st_find_free_entry( st );
        if( n < 0 )
            return -1;
    }

    if( n < 1 )
    {
        ERR("invalid index adding %s (%d)\n", debugstr_w( data ), n );
        return -1;
    }

    /* allocate a new string */
    if(len<0)
        len = strlenW(data);
    TRACE("%s, n = %d len = %d\n", debugstr_w(data), n, len );

    str = msi_alloc( (len+1)*sizeof(WCHAR) );
    if( !str )
        return -1;
    TRACE("%d\n",__LINE__);
    memcpy( str, data, len*sizeof(WCHAR) );
    str[len] = 0;

    set_st_entry( st, n, str );

    return n;
}

/* find the string identified by an id - return null if there's none */
const WCHAR *msi_string_lookup_id( string_table *st, UINT id )
{
    static const WCHAR zero[] = { 0 };
    if( id == 0 )
        return zero;

    if( id >= st->maxcount )
        return NULL;

    if( id && !st->strings[id].refcount )
        return NULL;

    return st->strings[id].str;
}

/*
 *  msi_id2stringW
 *
 *  [in] st         - pointer to the string table
 *  [in] id  - id of the string to retrieve
 *  [out] buffer    - destination of the string
 *  [in/out] sz     - number of bytes available in the buffer on input
 *                    number of bytes used on output
 *
 *   The size includes the terminating nul character.  Short buffers
 *  will be filled, but not nul terminated.
 */
UINT msi_id2stringW( string_table *st, UINT id, LPWSTR buffer, UINT *sz )
{
    UINT len;
    const WCHAR *str;

    TRACE("Finding string %d of %d\n", id, st->maxcount);

    str = msi_string_lookup_id( st, id );
    if( !str )
        return ERROR_FUNCTION_FAILED;

    len = strlenW( str ) + 1;

    if( !buffer )
    {
        *sz = len;
        return ERROR_SUCCESS;
    }

    if( *sz < len )
        *sz = len;
    memcpy( buffer, str, (*sz)*sizeof(WCHAR) ); 
    *sz = len;

    return ERROR_SUCCESS;
}

/*
 *  msi_id2stringA
 *
 *  [in] st         - pointer to the string table
 *  [in] id         - id of the string to retrieve
 *  [out] buffer    - destination of the UTF8 string
 *  [in/out] sz     - number of bytes available in the buffer on input
 *                    number of bytes used on output
 *
 *   The size includes the terminating nul character.  Short buffers
 *  will be filled, but not nul terminated.
 */
UINT msi_id2stringA( string_table *st, UINT id, LPSTR buffer, UINT *sz )
{
    UINT len;
    const WCHAR *str;
    int n;

    TRACE("Finding string %d of %d\n", id, st->maxcount);

    str = msi_string_lookup_id( st, id );
    if( !str )
        return ERROR_FUNCTION_FAILED;

    len = WideCharToMultiByte( st->codepage, 0, str, -1, NULL, 0, NULL, NULL );

    if( !buffer )
    {
        *sz = len;
        return ERROR_SUCCESS;
    }

    if( len > *sz )
    {
        n = strlenW( str ) + 1;
        while( n && (len > *sz) )
            len = WideCharToMultiByte( st->codepage, 0, 
                           str, --n, NULL, 0, NULL, NULL );
    }
    else
        n = -1;

    *sz = WideCharToMultiByte( st->codepage, 0, str, n, buffer, len, NULL, NULL );

    return ERROR_SUCCESS;
}

/*
 *  msi_string2idW
 *
 *  [in] st         - pointer to the string table
 *  [in] str        - string to find in the string table
 *  [out] id        - id of the string, if found
 */
UINT msi_string2idW( string_table *st, LPCWSTR str, UINT *id )
{
    UINT n, hash = msistring_makehash( str );
    msistring *se = st->strings;

    for (n = st->hash[hash]; n != -1; n = st->strings[n].hash_next )
    {
        if ((str == se[n].str) || !lstrcmpW(str, se[n].str))
        {
            *id = n;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_INVALID_PARAMETER;
}

UINT msi_string2idA( string_table *st, LPCSTR buffer, UINT *id )
{
    DWORD sz;
    UINT r = ERROR_INVALID_PARAMETER;
    LPWSTR str;

    TRACE("Finding string %s in string table\n", debugstr_a(buffer) );

    if( buffer[0] == 0 )
    {
        *id = 0;
        return ERROR_SUCCESS;
    }

    sz = MultiByteToWideChar( st->codepage, 0, buffer, -1, NULL, 0 );
    if( sz <= 0 )
        return r;
    str = msi_alloc( sz*sizeof(WCHAR) );
    if( !str )
        return ERROR_NOT_ENOUGH_MEMORY;
    MultiByteToWideChar( st->codepage, 0, buffer, -1, str, sz );

    r = msi_string2idW( st, str, id );
    msi_free( str );

    return r;
}

UINT msi_strcmp( string_table *st, UINT lval, UINT rval, UINT *res )
{
    const WCHAR *l_str, *r_str;

    l_str = msi_string_lookup_id( st, lval );
    if( !l_str )
        return ERROR_INVALID_PARAMETER;
    
    r_str = msi_string_lookup_id( st, rval );
    if( !r_str )
        return ERROR_INVALID_PARAMETER;

    /* does this do the right thing for all UTF-8 strings? */
    *res = strcmpW( l_str, r_str );

    return ERROR_SUCCESS;
}

UINT msi_string_count( string_table *st )
{
    return st->maxcount;
}

UINT msi_id_refcount( string_table *st, UINT i )
{
    if( i >= st->maxcount )
        return 0;
    return st->strings[i].refcount;
}

UINT msi_string_totalsize( string_table *st, UINT *datasize, UINT *poolsize )
{
    UINT i, len, max, holesize;

    if( st->strings[0].str || st->strings[0].refcount )
        ERR("oops. element 0 has a string\n");

    *poolsize = 4;
    *datasize = 0;
    max = 1;
    holesize = 0;
    for( i=1; i<st->maxcount; i++ )
    {
        if( st->strings[i].str )
        {
            TRACE("[%u] = %s\n", i, debugstr_w(st->strings[i].str));
            len = WideCharToMultiByte( st->codepage, 0,
                     st->strings[i].str, -1, NULL, 0, NULL, NULL);
            if( len )
                len--;
            (*datasize) += len;
            if (len>0xffff)
                (*poolsize) += 4;
            max = i + 1;
            (*poolsize) += holesize + 4;
            holesize = 0;
        }
        else
            holesize += 4;
    }
    TRACE("data %u pool %u codepage %x\n", *datasize, *poolsize, st->codepage );
    return max;
}

UINT msi_string_get_codepage( string_table *st )
{
    return st->codepage;
}
