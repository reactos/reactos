/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
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
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "objidl.h"
#include "winnls.h"
#include "ole2.h"

#include "winreg.h"
#include "shlwapi.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msidb);

#define MSIFIELD_NULL   0
#define MSIFIELD_INT    1
#define MSIFIELD_WSTR   3
#define MSIFIELD_STREAM 4

static void MSI_FreeField( MSIFIELD *field )
{
    switch( field->type )
    {
    case MSIFIELD_NULL:
    case MSIFIELD_INT:
        break;
    case MSIFIELD_WSTR:
        free( field->u.szwVal);
        break;
    case MSIFIELD_STREAM:
        IStream_Release( field->u.stream );
        break;
    default:
        ERR("Invalid field type %d\n", field->type);
    }
}

void MSI_CloseRecord( MSIOBJECTHDR *arg )
{
    MSIRECORD *rec = (MSIRECORD *) arg;
    UINT i;

    for( i=0; i<=rec->count; i++ )
        MSI_FreeField( &rec->fields[i] );
}

MSIRECORD *MSI_CreateRecord( UINT cParams )
{
    MSIRECORD *rec;

    TRACE("%d\n", cParams);

    if( cParams>65535 )
        return NULL;

    rec = alloc_msiobject( MSIHANDLETYPE_RECORD, FIELD_OFFSET(MSIRECORD, fields[cParams + 1]),
            MSI_CloseRecord );
    if( rec )
        rec->count = cParams;
    return rec;
}

MSIHANDLE WINAPI MsiCreateRecord( UINT cParams )
{
    MSIRECORD *rec;
    MSIHANDLE ret = 0;

    TRACE("%d\n", cParams);

    rec = MSI_CreateRecord( cParams );
    if( rec )
    {
        ret = alloc_msihandle( &rec->hdr );
        msiobj_release( &rec->hdr );
    }
    return ret;
}

UINT MSI_RecordGetFieldCount( const MSIRECORD *rec )
{
    return rec->count;
}

UINT WINAPI MsiRecordGetFieldCount( MSIHANDLE handle )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu\n", handle );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return -1;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetFieldCount( rec );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );

    return ret;
}

static BOOL string2intW( LPCWSTR str, int *out )
{
    int x = 0;
    LPCWSTR p = str;

    if( *p == '-' ) /* skip the minus sign */
        p++;
    while ( *p )
    {
        if( (*p < '0') || (*p > '9') )
            return FALSE;
        x *= 10;
        x += (*p - '0');
        p++;
    }

    if( str[0] == '-' ) /* check if it's negative */
        x = -x;
    *out = x;

    return TRUE;
}

WCHAR *msi_strdupW( const WCHAR *value, int len )
{
    WCHAR *ret;

    if (!value) return NULL;
    if (!(ret = malloc( (len + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, value, len * sizeof(WCHAR) );
    ret[len] = 0;
    return ret;
}

UINT MSI_RecordCopyField( MSIRECORD *in_rec, UINT in_n,
                          MSIRECORD *out_rec, UINT out_n )
{
    UINT r = ERROR_SUCCESS;

    msiobj_lock( &in_rec->hdr );

    if ( in_n > in_rec->count || out_n > out_rec->count )
        r = ERROR_FUNCTION_FAILED;
    else if ( in_rec != out_rec || in_n != out_n )
    {
        LPWSTR str;
        MSIFIELD *in, *out;

        in = &in_rec->fields[in_n];
        out = &out_rec->fields[out_n];

        switch ( in->type )
        {
        case MSIFIELD_NULL:
            break;
        case MSIFIELD_INT:
            out->u.iVal = in->u.iVal;
            break;
        case MSIFIELD_WSTR:
            if ((str = msi_strdupW( in->u.szwVal, in->len )))
            {
                out->u.szwVal = str;
                out->len = in->len;
            }
            else r = ERROR_OUTOFMEMORY;
            break;
        case MSIFIELD_STREAM:
            IStream_AddRef( in->u.stream );
            out->u.stream = in->u.stream;
            break;
        default:
            ERR("invalid field type %d\n", in->type);
        }
        if (r == ERROR_SUCCESS)
            out->type = in->type;
    }

    msiobj_unlock( &in_rec->hdr );
    return r;
}

int MSI_RecordGetInteger( MSIRECORD *rec, UINT iField)
{
    int ret = 0;

    TRACE("%p %d\n", rec, iField );

    if( iField > rec->count )
        return MSI_NULL_INTEGER;

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        return rec->fields[iField].u.iVal;
    case MSIFIELD_WSTR:
        if( string2intW( rec->fields[iField].u.szwVal, &ret ) )
            return ret;
        return MSI_NULL_INTEGER;
    default:
        break;
    }

    return MSI_NULL_INTEGER;
}

int WINAPI MsiRecordGetInteger( MSIHANDLE handle, UINT iField)
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return MSI_NULL_INTEGER;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetInteger( rec, iField );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );

    return ret;
}

UINT WINAPI MsiRecordClearData( MSIHANDLE handle )
{
    MSIRECORD *rec;
    UINT i;

    TRACE( "%lu\n", handle );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    for( i=0; i<=rec->count; i++)
    {
        MSI_FreeField( &rec->fields[i] );
        rec->fields[i].type = MSIFIELD_NULL;
        rec->fields[i].u.iVal = 0;
    }
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );

    return ERROR_SUCCESS;
}

UINT MSI_RecordSetInteger( MSIRECORD *rec, UINT iField, int iVal )
{
    TRACE("%p %u %d\n", rec, iField, iVal);

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    MSI_FreeField( &rec->fields[iField] );

    if (iVal == MSI_NULL_INTEGER)
    {
        rec->fields[iField].type = MSIFIELD_NULL;
        rec->fields[iField].u.szwVal = NULL;
    }
    else
    {
        rec->fields[iField].type = MSIFIELD_INT;
        rec->fields[iField].u.iVal = iVal;
    }

    return ERROR_SUCCESS;
}

UINT WINAPI MsiRecordSetInteger( MSIHANDLE handle, UINT iField, int iVal )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u, %d\n", handle, iField, iVal );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetInteger( rec, iField, iVal );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

BOOL MSI_RecordIsNull( MSIRECORD *rec, UINT iField )
{
    BOOL r = TRUE;

    TRACE("%p %d\n", rec, iField );

    r = ( iField > rec->count ) ||
        ( rec->fields[iField].type == MSIFIELD_NULL );

    return r;
}

BOOL WINAPI MsiRecordIsNull( MSIHANDLE handle, UINT iField )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return FALSE;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordIsNull( rec, iField );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;

}

UINT MSI_RecordGetStringA(MSIRECORD *rec, UINT iField,
               LPSTR szValue, LPDWORD pcchValue)
{
    UINT len = 0, ret = ERROR_SUCCESS;
    CHAR buffer[16];

    TRACE("%p %d %p %p\n", rec, iField, szValue, pcchValue);

    if( iField > rec->count )
    {
        if ( szValue && *pcchValue > 0 )
            szValue[0] = 0;

        *pcchValue = 0;
        return ERROR_SUCCESS;
    }

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfA(buffer, "%d", rec->fields[iField].u.iVal);
        len = lstrlenA( buffer );
        if (szValue)
            lstrcpynA(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal,
                                   rec->fields[iField].len + 1, NULL, 0 , NULL, NULL );
        if (szValue)
            WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal,
                                 rec->fields[iField].len + 1, szValue, *pcchValue, NULL, NULL );
        if( szValue && *pcchValue && len>*pcchValue )
            szValue[*pcchValue-1] = 0;
        if( len )
            len--;
        break;
    case MSIFIELD_NULL:
        if( szValue && *pcchValue > 0 )
            szValue[0] = 0;
        break;
    default:
        ret = ERROR_INVALID_PARAMETER;
        break;
    }

    if( szValue && *pcchValue <= len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

UINT WINAPI MsiRecordGetStringA( MSIHANDLE handle, UINT iField, char *szValue, DWORD *pcchValue )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %d, %p, %p\n", handle, iField, szValue, pcchValue );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetStringA( rec, iField, szValue, pcchValue);
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

const WCHAR *msi_record_get_string( const MSIRECORD *rec, UINT field, int *len )
{
    if (field > rec->count)
        return NULL;

    if (rec->fields[field].type != MSIFIELD_WSTR)
        return NULL;

    if (len) *len = rec->fields[field].len;

    return rec->fields[field].u.szwVal;
}

const WCHAR *MSI_RecordGetString( const MSIRECORD *rec, UINT iField )
{
    return msi_record_get_string( rec, iField, NULL );
}

UINT MSI_RecordGetStringW( MSIRECORD *rec, UINT iField, WCHAR *szValue, DWORD *pcchValue )
{
    UINT len = 0, ret = ERROR_SUCCESS;
    WCHAR buffer[16];

    TRACE( "%p, %u, %p, %p\n", rec, iField, szValue, pcchValue );

    if( iField > rec->count )
    {
        if ( szValue && *pcchValue > 0 )
            szValue[0] = 0;

        *pcchValue = 0;
        return ERROR_SUCCESS;
    }

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfW(buffer, L"%d", rec->fields[iField].u.iVal);
        len = lstrlenW( buffer );
        if (szValue)
            lstrcpynW(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = rec->fields[iField].len;
        if (szValue)
            memcpy( szValue, rec->fields[iField].u.szwVal, min(len + 1, *pcchValue) * sizeof(WCHAR) );
        break;
    case MSIFIELD_NULL:
        if( szValue && *pcchValue > 0 )
            szValue[0] = 0;
        break;
    default:
        break;
    }

    if( szValue && *pcchValue <= len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

UINT WINAPI MsiRecordGetStringW( MSIHANDLE handle, UINT iField, WCHAR *szValue, DWORD *pcchValue )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u, %p, %p\n", handle, iField, szValue, pcchValue );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordGetStringW( rec, iField, szValue, pcchValue );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

static UINT get_stream_size( IStream *stm )
{
    STATSTG stat;
    HRESULT r;

    r = IStream_Stat( stm, &stat, STATFLAG_NONAME );
    if( FAILED(r) )
        return 0;
    return stat.cbSize.QuadPart;
}

static UINT MSI_RecordDataSize(MSIRECORD *rec, UINT iField)
{
    TRACE("%p %d\n", rec, iField);

    if( iField > rec->count )
        return 0;

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        return sizeof (INT);
    case MSIFIELD_WSTR:
        return rec->fields[iField].len;
    case MSIFIELD_NULL:
        break;
    case MSIFIELD_STREAM:
        return get_stream_size( rec->fields[iField].u.stream );
    }
    return 0;
}

UINT WINAPI MsiRecordDataSize( MSIHANDLE handle, UINT iField )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return 0;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordDataSize( rec, iField);
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT WINAPI MsiRecordSetStringA( MSIHANDLE handle, UINT iField, const char *szValue )
{
    WCHAR *valueW = NULL;
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u %s\n", handle, iField, debugstr_a(szValue) );

    if (szValue && !(valueW = strdupAtoW( szValue ))) return ERROR_OUTOFMEMORY;

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
    {
        free( valueW );
        return ERROR_INVALID_HANDLE;
    }
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetStringW( rec, iField, valueW );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    free( valueW );
    return ret;
}

UINT msi_record_set_string( MSIRECORD *rec, UINT field, const WCHAR *value, int len )
{
    if (field > rec->count)
        return ERROR_INVALID_FIELD;

    MSI_FreeField( &rec->fields[field] );

    if (value && len < 0) len = lstrlenW( value );

    if (value && len)
    {
        rec->fields[field].type = MSIFIELD_WSTR;
        rec->fields[field].u.szwVal = msi_strdupW( value, len );
        rec->fields[field].len = len;
    }
    else
    {
        rec->fields[field].type = MSIFIELD_NULL;
        rec->fields[field].u.szwVal = NULL;
        rec->fields[field].len = 0;
    }
    return 0;
}

UINT MSI_RecordSetStringW( MSIRECORD *rec, UINT iField, LPCWSTR szValue )
{
    TRACE("%p %d %s\n", rec, iField, debugstr_w(szValue));

    return msi_record_set_string( rec, iField, szValue, -1 );
}

UINT WINAPI MsiRecordSetStringW( MSIHANDLE handle, UINT iField, const WCHAR *szValue )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u, %s\n", handle, iField, debugstr_w(szValue) );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetStringW( rec, iField, szValue );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

/* read the data in a file into an IStream */
static UINT RECORD_StreamFromFile(LPCWSTR szFile, IStream **pstm)
{
    DWORD sz, szHighWord = 0, read;
    HANDLE handle;
    HGLOBAL hGlob = 0;
    HRESULT hr;
    ULARGE_INTEGER ulSize;

    TRACE("reading %s\n", debugstr_w(szFile));

    /* read the file into memory */
    handle = CreateFileW(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if( handle == INVALID_HANDLE_VALUE )
        return GetLastError();
    sz = GetFileSize(handle, &szHighWord);
    if( sz != INVALID_FILE_SIZE && szHighWord == 0 )
    {
        hGlob = GlobalAlloc(GMEM_FIXED, sz);
        if( hGlob )
        {
            BOOL r = ReadFile(handle, hGlob, sz, &read, NULL) && read == sz;
            if( !r )
            {
                GlobalFree(hGlob);
                hGlob = 0;
            }
        }
    }
    CloseHandle(handle);
    if( !hGlob )
        return ERROR_FUNCTION_FAILED;

    /* make a stream out of it, and set the correct file size */
    hr = CreateStreamOnHGlobal(hGlob, TRUE, pstm);
    if( FAILED( hr ) )
    {
        GlobalFree(hGlob);
        return ERROR_FUNCTION_FAILED;
    }

    /* set the correct size - CreateStreamOnHGlobal screws it up */
    ulSize.QuadPart = sz;
    IStream_SetSize(*pstm, ulSize);

    TRACE( "read %s, %lu bytes into IStream %p\n", debugstr_w(szFile), sz, *pstm );
    return ERROR_SUCCESS;
}

UINT MSI_RecordSetStream(MSIRECORD *rec, UINT iField, IStream *stream)
{
    if ( (iField == 0) || (iField > rec->count) )
        return ERROR_INVALID_PARAMETER;

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_STREAM;
    rec->fields[iField].u.stream = stream;

    return ERROR_SUCCESS;
}

UINT MSI_RecordSetStreamFromFileW(MSIRECORD *rec, UINT iField, LPCWSTR szFilename)
{
    IStream *stm = NULL;
    HRESULT hr;
    UINT ret;

    if( (iField == 0) || (iField > rec->count) )
        return ERROR_INVALID_PARAMETER;

    /* no filename means we should seek back to the start of the stream */
    if( !szFilename )
    {
        LARGE_INTEGER ofs;
        ULARGE_INTEGER cur;

        if( rec->fields[iField].type != MSIFIELD_STREAM )
            return ERROR_INVALID_FIELD;

        stm = rec->fields[iField].u.stream;
        if( !stm )
            return ERROR_INVALID_FIELD;

        ofs.QuadPart = 0;
        hr = IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
        if (FAILED( hr ))
            return ERROR_FUNCTION_FAILED;
    }
    else
    {
        /* read the file into a stream and save the stream in the record */
        ret = RECORD_StreamFromFile(szFilename, &stm);
        if (ret != ERROR_SUCCESS)
            return ret;

        /* if all's good, store it in the record */
        MSI_RecordSetStream(rec, iField, stm);
    }

    return ERROR_SUCCESS;
}

UINT WINAPI MsiRecordSetStreamA( MSIHANDLE hRecord, UINT iField, const char *szFilename )
{
    WCHAR *wstr = NULL;
    UINT ret;

    TRACE( "%lu, %u, %s\n", hRecord, iField, debugstr_a(szFilename) );

    if( szFilename )
    {
        wstr = strdupAtoW( szFilename );
        if( !wstr )
             return ERROR_OUTOFMEMORY;
    }
    ret = MsiRecordSetStreamW(hRecord, iField, wstr);
    free(wstr);

    return ret;
}

UINT WINAPI MsiRecordSetStreamW( MSIHANDLE handle, UINT iField, const WCHAR *szFilename )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u, %s\n", handle, iField, debugstr_w(szFilename) );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    msiobj_lock( &rec->hdr );
    ret = MSI_RecordSetStreamFromFileW( rec, iField, szFilename );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT MSI_RecordReadStream(MSIRECORD *rec, UINT iField, char *buf, LPDWORD sz)
{
    ULONG count;
    HRESULT r;
    IStream *stm;

    TRACE("%p %d %p %p\n", rec, iField, buf, sz);

    if( !sz )
        return ERROR_INVALID_PARAMETER;

    if( iField > rec->count)
        return ERROR_INVALID_PARAMETER;

    if ( rec->fields[iField].type == MSIFIELD_NULL )
    {
        *sz = 0;
        return ERROR_INVALID_DATA;
    }

    if( rec->fields[iField].type != MSIFIELD_STREAM )
        return ERROR_INVALID_DATATYPE;

    stm = rec->fields[iField].u.stream;
    if( !stm )
        return ERROR_INVALID_PARAMETER;

    /* if there's no buffer pointer, calculate the length to the end */
    if( !buf )
    {
        LARGE_INTEGER ofs;
        ULARGE_INTEGER end, cur;

        ofs.QuadPart = cur.QuadPart = 0;
        end.QuadPart = 0;
        IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
        IStream_Seek( stm, ofs, STREAM_SEEK_END, &end );
        ofs.QuadPart = cur.QuadPart;
        IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
        *sz = end.QuadPart - cur.QuadPart;

        return ERROR_SUCCESS;
    }

    /* read the data */
    count = 0;
    r = IStream_Read( stm, buf, *sz, &count );
    if( FAILED( r ) )
    {
        *sz = 0;
        return ERROR_FUNCTION_FAILED;
    }

    *sz = count;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiRecordReadStream( MSIHANDLE handle, UINT iField, char *buf, DWORD *sz )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE( "%lu, %u, %p, %p\n", handle, iField, buf, sz );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    msiobj_lock( &rec->hdr );
    ret = MSI_RecordReadStream( rec, iField, buf, sz );
    msiobj_unlock( &rec->hdr );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT MSI_RecordSetIStream( MSIRECORD *rec, UINT iField, IStream *stm )
{
    TRACE("%p %d %p\n", rec, iField, stm);

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    MSI_FreeField( &rec->fields[iField] );

    rec->fields[iField].type = MSIFIELD_STREAM;
    rec->fields[iField].u.stream = stm;
    IStream_AddRef( stm );

    return ERROR_SUCCESS;
}

UINT MSI_RecordGetIStream( MSIRECORD *rec, UINT iField, IStream **pstm)
{
    TRACE("%p %d %p\n", rec, iField, pstm);

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    if( rec->fields[iField].type != MSIFIELD_STREAM )
        return ERROR_INVALID_FIELD;

    *pstm = rec->fields[iField].u.stream;
    IStream_AddRef( *pstm );

    return ERROR_SUCCESS;
}

static UINT dump_stream_to_file( IStream *stm, const WCHAR *name )
{
    ULARGE_INTEGER size;
    LARGE_INTEGER pos;
    IStream *out;
    DWORD stgm;
    HRESULT r;

    stgm = STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE;
    r = SHCreateStreamOnFileW( name, stgm, &out );
    if( FAILED( r ) )
        return ERROR_FUNCTION_FAILED;

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, STREAM_SEEK_END, &size );
    if( FAILED( r ) )
        goto end;

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, STREAM_SEEK_SET, NULL );
    if( FAILED( r ) )
        goto end;

    r = IStream_CopyTo( stm, out, size, NULL, NULL );

end:
    IStream_Release( out );
    if( FAILED( r ) )
        return ERROR_FUNCTION_FAILED;
    return ERROR_SUCCESS;
}

UINT MSI_RecordStreamToFile( MSIRECORD *rec, UINT iField, LPCWSTR name )
{
    IStream *stm = NULL;
    UINT r;

    TRACE("%p %u %s\n", rec, iField, debugstr_w(name));

    msiobj_lock( &rec->hdr );

    r = MSI_RecordGetIStream( rec, iField, &stm );
    if( r == ERROR_SUCCESS )
    {
        r = dump_stream_to_file( stm, name );
        IStream_Release( stm );
    }

    msiobj_unlock( &rec->hdr );

    return r;
}

MSIRECORD *MSI_CloneRecord(MSIRECORD *rec)
{
    MSIRECORD *clone;
    UINT r, i, count;

    count = MSI_RecordGetFieldCount(rec);
    clone = MSI_CreateRecord(count);
    if (!clone)
        return NULL;

    for (i = 0; i <= count; i++)
    {
        if (rec->fields[i].type == MSIFIELD_STREAM)
        {
            if (FAILED(IStream_Clone(rec->fields[i].u.stream,
                                     &clone->fields[i].u.stream)))
            {
                msiobj_release(&clone->hdr);
                return NULL;
            }
            clone->fields[i].type = MSIFIELD_STREAM;
        }
        else
        {
            r = MSI_RecordCopyField(rec, i, clone, i);
            if (r != ERROR_SUCCESS)
            {
                msiobj_release(&clone->hdr);
                return NULL;
            }
        }
    }

    return clone;
}

BOOL MSI_RecordsAreFieldsEqual(MSIRECORD *a, MSIRECORD *b, UINT field)
{
    if (a->fields[field].type != b->fields[field].type)
        return FALSE;

    switch (a->fields[field].type)
    {
        case MSIFIELD_NULL:
            break;

        case MSIFIELD_INT:
            if (a->fields[field].u.iVal != b->fields[field].u.iVal)
                return FALSE;
            break;

        case MSIFIELD_WSTR:
            if (a->fields[field].len != b->fields[field].len) return FALSE;
            if (memcmp( a->fields[field].u.szwVal, b->fields[field].u.szwVal,
                        a->fields[field].len * sizeof(WCHAR) )) return FALSE;
            break;

        case MSIFIELD_STREAM:
        default:
            return FALSE;
    }
    return TRUE;
}


BOOL MSI_RecordsAreEqual(MSIRECORD *a, MSIRECORD *b)
{
    UINT i;

    if (a->count != b->count)
        return FALSE;

    for (i = 0; i <= a->count; i++)
    {
        if (!MSI_RecordsAreFieldsEqual( a, b, i ))
            return FALSE;
    }

    return TRUE;
}

WCHAR *msi_dup_record_field( MSIRECORD *rec, INT field )
{
    DWORD sz = 0;
    WCHAR *str;
    UINT r;

    if (MSI_RecordIsNull( rec, field )) return NULL;

    r = MSI_RecordGetStringW( rec, field, NULL, &sz );
    if (r != ERROR_SUCCESS)
        return NULL;

    sz++;
    str = malloc( sz * sizeof(WCHAR) );
    if (!str) return NULL;
    str[0] = 0;
    r = MSI_RecordGetStringW( rec, field, str, &sz );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to get string!\n");
        free( str );
        return NULL;
    }
    return str;
}

void dump_record(MSIRECORD *rec)
{
    int i;
    if (!rec)
    {
        TRACE("(null)\n");
        return;
    }

    TRACE("[");
    for (i = 0; i <= rec->count; i++)
    {
        switch(rec->fields[i].type)
        {
        case MSIFIELD_NULL: TRACE("(null)"); break;
        case MSIFIELD_INT: TRACE("%d", rec->fields[i].u.iVal); break;
        case MSIFIELD_WSTR: TRACE("%s", debugstr_w(rec->fields[i].u.szwVal)); break;
        case MSIFIELD_STREAM: TRACE("%p", rec->fields[i].u.stream); break;
        }
        if (i < rec->count) TRACE(", ");
    }
    TRACE("]\n");
}

UINT copy_remote_record(const struct wire_record *in, MSIHANDLE out)
{
    MSIRECORD *rec;
    unsigned int i;
    UINT r = ERROR_SUCCESS;

    if (!(rec = msihandle2msiinfo(out, MSIHANDLETYPE_RECORD)))
        return ERROR_INVALID_HANDLE;

    rec->cookie = in->cookie;
    for (i = 0; i <= in->count; i++)
    {
        switch (in->fields[i].type)
        {
        case MSIFIELD_NULL:
            MSI_FreeField(&rec->fields[i]);
            rec->fields[i].type = MSIFIELD_NULL;
            break;
        case MSIFIELD_INT:
            r = MSI_RecordSetInteger(rec, i, in->fields[i].u.iVal);
            break;
        case MSIFIELD_WSTR:
            r = MSI_RecordSetStringW(rec, i, in->fields[i].u.szwVal);
            break;
        case MSIFIELD_STREAM:
            r = MSI_RecordSetIStream(rec, i, in->fields[i].u.stream);
            break;
        default:
            ERR("invalid field type %d\n", in->fields[i].type);
            break;
        }

        if (r)
        {
            msiobj_release(&rec->hdr);
            return r;
        }
    }

    msiobj_release(&rec->hdr);
    return ERROR_SUCCESS;
}

UINT unmarshal_record(const struct wire_record *in, MSIHANDLE *out)
{
    if (!in)
    {
        *out = 0;
        return ERROR_SUCCESS;
    }

    *out = MsiCreateRecord(in->count);
    if (!*out) return ERROR_OUTOFMEMORY;

    return copy_remote_record(in, *out);
}

struct wire_record *marshal_record(MSIHANDLE handle)
{
    struct wire_record *ret;
    unsigned int i;
    MSIRECORD *rec;

    if (!(rec = msihandle2msiinfo(handle, MSIHANDLETYPE_RECORD)))
        return NULL;

    ret = midl_user_allocate(sizeof(*ret) + rec->count * sizeof(ret->fields[0]));
    ret->count = rec->count;
    ret->cookie = rec->cookie;

    for (i = 0; i <= rec->count; i++)
    {
        switch (rec->fields[i].type)
        {
        case MSIFIELD_NULL:
            break;
        case MSIFIELD_INT:
            ret->fields[i].u.iVal = rec->fields[i].u.iVal;
            break;
        case MSIFIELD_WSTR:
            ret->fields[i].u.szwVal = wcsdup(rec->fields[i].u.szwVal);
            break;
        case MSIFIELD_STREAM:
            IStream_AddRef(rec->fields[i].u.stream);
            ret->fields[i].u.stream = rec->fields[i].u.stream;
            break;
        default:
            ERR("invalid field type %d\n", rec->fields[i].type);
            break;
        }
        ret->fields[i].type = rec->fields[i].type;
    }

    msiobj_release(&rec->hdr);
    return ret;
}

void free_remote_record(struct wire_record *rec)
{
    int i;

    for (i = 0; i <= rec->count; i++)
    {
        if (rec->fields[i].type == MSIFIELD_WSTR)
            midl_user_free(rec->fields[i].u.szwVal);
        else if (rec->fields[i].type == MSIFIELD_STREAM)
            IStream_Release(rec->fields[i].u.stream);
    }

    midl_user_free(rec);
}
