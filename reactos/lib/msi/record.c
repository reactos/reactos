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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define MSIFIELD_NULL   0
#define MSIFIELD_INT    1
#define MSIFIELD_STR    2
#define MSIFIELD_WSTR   3
#define MSIFIELD_STREAM 4

void MSI_FreeField( MSIFIELD *field )
{
    switch( field->type )
    {
    case MSIFIELD_NULL:
    case MSIFIELD_INT:
        break;
    case MSIFIELD_WSTR:
        HeapFree( GetProcessHeap(), 0, field->u.szwVal);
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

MSIRECORD *MSI_CreateRecord( unsigned int cParams )
{
    MSIRECORD *rec;
    UINT len;

    TRACE("%d\n", cParams);

    len = sizeof (MSIRECORD) + sizeof (MSIFIELD)*cParams;
    rec = alloc_msiobject( MSIHANDLETYPE_RECORD, len, MSI_CloseRecord );
    if( rec )
    rec->count = cParams;
    return rec;
}

MSIHANDLE WINAPI MsiCreateRecord( unsigned int cParams )
{
    MSIRECORD *rec;
    MSIHANDLE ret = 0;

    TRACE("%d\n", cParams);

    rec = MSI_CreateRecord( cParams );
    if( rec )
        ret = alloc_msihandle( &rec->hdr );
    return ret;
}

unsigned int MSI_RecordGetFieldCount( MSIRECORD *rec )
{
    return rec->count;
}

unsigned int WINAPI MsiRecordGetFieldCount( MSIHANDLE handle )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld\n", handle );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
    {
        ERR("Record not found!\n");
        return 0;
    }

    ret = MSI_RecordGetFieldCount( rec );
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

int MSI_RecordGetInteger( MSIRECORD *rec, unsigned int iField)
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

int WINAPI MsiRecordGetInteger( MSIHANDLE handle, unsigned int iField)
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return MSI_NULL_INTEGER;

    ret = MSI_RecordGetInteger( rec, iField );
    msiobj_release( &rec->hdr );

    return ret;
}

UINT WINAPI MsiRecordClearData( MSIHANDLE handle )
{
    MSIRECORD *rec;
    UINT i;

    TRACE("%ld\n", handle );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    for( i=0; i<=rec->count; i++)
    {
        MSI_FreeField( &rec->fields[i] );
        rec->fields[i].type = MSIFIELD_NULL;
        rec->fields[i].u.iVal = 0;
    }

    return ERROR_SUCCESS;
}

UINT MSI_RecordSetInteger( MSIRECORD *rec, unsigned int iField, int iVal )
{
    TRACE("%p %u %d\n", rec, iField, iVal);

    if( iField <= rec->count )
    {
        MSI_FreeField( &rec->fields[iField] );
        rec->fields[iField].type = MSIFIELD_INT;
        rec->fields[iField].u.iVal = iVal;
    }

    return ERROR_SUCCESS;
}

UINT WINAPI MsiRecordSetInteger( MSIHANDLE handle, unsigned int iField, int iVal )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %u %d\n", handle, iField, iVal);

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    ret = MSI_RecordSetInteger( rec, iField, iVal );
    msiobj_release( &rec->hdr );
    return ret;
}

BOOL MSI_RecordIsNull( MSIRECORD *rec, unsigned int iField )
{
    BOOL r = TRUE;

    TRACE("%p %d\n", rec, iField );

    r = ( iField > rec->count ) ||
        ( rec->fields[iField].type == MSIFIELD_NULL );

    return r;
}

BOOL WINAPI MsiRecordIsNull( MSIHANDLE handle, unsigned int iField )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    ret = MSI_RecordIsNull( rec, iField );
    msiobj_release( &rec->hdr );
    return ret;

}

UINT MSI_RecordGetStringA(MSIRECORD *rec, unsigned int iField, 
               LPSTR szValue, DWORD *pcchValue)
{
    UINT len=0, ret;
    CHAR buffer[16];

    TRACE("%p %d %p %p\n", rec, iField, szValue, pcchValue);

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    ret = ERROR_SUCCESS;
    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfA(buffer, "%d", rec->fields[iField].u.iVal);
        len = lstrlenA( buffer );
        lstrcpynA(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal, -1,
                             NULL, 0 , NULL, NULL);
        WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal, -1,
                             szValue, *pcchValue, NULL, NULL);
        break;
    case MSIFIELD_NULL:
        len = 1;
        if( *pcchValue > 0 )
            szValue[0] = 0;
        break;
    default:
        ret = ERROR_INVALID_PARAMETER;
        break;
    }

    if( *pcchValue < len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

UINT WINAPI MsiRecordGetStringA(MSIHANDLE handle, unsigned int iField, 
               LPSTR szValue, DWORD *pcchValue)
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d %p %p\n", handle, iField, szValue, pcchValue);

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    ret = MSI_RecordGetStringA( rec, iField, szValue, pcchValue);
    msiobj_release( &rec->hdr );
    return ret;
}

const WCHAR *MSI_RecordGetString( MSIRECORD *rec, unsigned int iField )
{
    if( iField > rec->count )
        return NULL;

    if( rec->fields[iField].type != MSIFIELD_WSTR )
        return NULL;

    return rec->fields[iField].u.szwVal;
}

UINT MSI_RecordGetStringW(MSIRECORD *rec, unsigned int iField,
               LPWSTR szValue, DWORD *pcchValue)
{
    UINT len=0, ret;
    WCHAR buffer[16];
    static const WCHAR szFormat[] = { '%','d',0 };

    TRACE("%p %d %p %p\n", rec, iField, szValue, pcchValue);

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    ret = ERROR_SUCCESS;
    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfW(buffer, szFormat, rec->fields[iField].u.iVal);
        len = lstrlenW( buffer );
        lstrcpynW(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = lstrlenW( rec->fields[iField].u.szwVal );
        lstrcpynW(szValue, rec->fields[iField].u.szwVal, *pcchValue);
        break;
    case MSIFIELD_NULL:
        len = 1;
        if( *pcchValue > 0 )
            szValue[0] = 0;
    default:
        break;
    }

    if( *pcchValue < len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

UINT WINAPI MsiRecordGetStringW(MSIHANDLE handle, unsigned int iField,
               LPWSTR szValue, DWORD *pcchValue)
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d %p %p\n", handle, iField, szValue, pcchValue);

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    ret = MSI_RecordGetStringW( rec, iField, szValue, pcchValue );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT WINAPI MsiRecordDataSize(MSIHANDLE hRecord, unsigned int iField)
{
    FIXME("%ld %d\n", hRecord, iField);
    return 0;
}

UINT MSI_RecordSetStringA( MSIRECORD *rec, unsigned int iField, LPCSTR szValue )
{
    LPWSTR str;
    UINT len;

    TRACE("%p %d %s\n", rec, iField, debugstr_a(szValue));

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    len = MultiByteToWideChar( CP_ACP, 0, szValue, -1, NULL, 0 );
    str = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, szValue, -1, str, len );
    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_WSTR;
    rec->fields[iField].u.szwVal = str;

    return 0;
}

UINT WINAPI MsiRecordSetStringA( MSIHANDLE handle, unsigned int iField, LPCSTR szValue )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d %s\n", handle, iField, debugstr_a(szValue));

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    ret = MSI_RecordSetStringA( rec, iField, szValue );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT MSI_RecordSetStringW( MSIRECORD *rec, unsigned int iField, LPCWSTR szValue )
{
    LPWSTR str;
    UINT len;

    TRACE("%p %d %s\n", rec, iField, debugstr_w(szValue));

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    len = lstrlenW(szValue) + 1;
    str = HeapAlloc( GetProcessHeap(), 0, len*sizeof (WCHAR));
    lstrcpyW( str, szValue );

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_WSTR;
    rec->fields[iField].u.szwVal = str;

    return 0;
}

UINT WINAPI MsiRecordSetStringW( MSIHANDLE handle, unsigned int iField, LPCWSTR szValue )
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d %s\n", handle, iField, debugstr_w(szValue));

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    ret = MSI_RecordSetStringW( rec, iField, szValue );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT WINAPI MsiFormatRecordA(MSIHANDLE hInstall, MSIHANDLE hRecord, LPSTR szResult, DWORD *sz)
{
    FIXME("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiFormatRecordW(MSIHANDLE hInstall, MSIHANDLE hRecord, LPWSTR szResult, DWORD *sz)
{
    FIXME("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiRecordSetStreamA(MSIHANDLE hRecord, unsigned int iField, LPCSTR szFilename)
{
    FIXME("%ld %d %s\n", hRecord, iField, debugstr_a(szFilename));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiRecordSetStreamW(MSIHANDLE hRecord, unsigned int iField, LPCWSTR szFilename)
{
    FIXME("%ld %d %s\n", hRecord, iField, debugstr_w(szFilename));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT MSI_RecordReadStream(MSIRECORD *rec, unsigned int iField, char *buf, DWORD *sz)
{
    ULONG count;
    HRESULT r;
    IStream *stm;

    TRACE("%p %d %p %p\n", rec, iField, buf, sz);

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    if( rec->fields[iField].type != MSIFIELD_STREAM )
    {
        *sz = 0;
        return ERROR_INVALID_FIELD;
    }

    stm = rec->fields[iField].u.stream;
    if( !stm )
        return ERROR_INVALID_FIELD;

    /* if there's no buffer pointer, calculate the length to the end */
    if( !buf )
    {
        LARGE_INTEGER ofs;
        ULARGE_INTEGER end, cur;

        ofs.QuadPart = cur.QuadPart = 0;
        end.QuadPart = 0;
        r = IStream_Seek( stm, ofs, STREAM_SEEK_SET, &cur );
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
        return ERROR_FUNCTION_FAILED;

    *sz = count;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiRecordReadStream(MSIHANDLE handle, unsigned int iField, char *buf, DWORD *sz)
{
    MSIRECORD *rec;
    UINT ret;

    TRACE("%ld %d %p %p\n", handle, iField, buf, sz);

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;
    ret = MSI_RecordReadStream( rec, iField, buf, sz );
    msiobj_release( &rec->hdr );
    return ret;
}

UINT MSI_RecordSetIStream( MSIRECORD *rec, unsigned int iField, IStream *stm )
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
