/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002, 2005 Mike McCormack for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msidefs.h"
#include "msipriv.h"
#include "objidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#include "pshpack1.h"

typedef struct { 
    WORD wByteOrder;
    WORD wFormat;
    DWORD dwOSVer;
    CLSID clsID;
    DWORD reserved;
} PROPERTYSETHEADER;

typedef struct { 
    FMTID fmtid;
    DWORD dwOffset;
} FORMATIDOFFSET;

typedef struct { 
    DWORD cbSection;
    DWORD cProperties;
} PROPERTYSECTIONHEADER; 
 
typedef struct { 
    DWORD propid;
    DWORD dwOffset;
} PROPERTYIDOFFSET; 

typedef struct {
    DWORD type;
    union {
        INT i4;
        SHORT i2;
        FILETIME ft;
        struct {
            DWORD len;
            BYTE str[1];
        } str;
    } u;
} PROPERTY_DATA;
 
#include "poppack.h"

#define SECT_HDR_SIZE (sizeof(PROPERTYSECTIONHEADER))

static const WCHAR szSumInfo[] = { 5 ,'S','u','m','m','a','r','y',
                       'I','n','f','o','r','m','a','t','i','o','n',0 };

static void free_prop( PROPVARIANT *prop )
{
    if (prop->vt == VT_LPSTR )
        msi_free( prop->u.pszVal );
    prop->vt = VT_EMPTY;
}

static void MSI_CloseSummaryInfo( MSIOBJECTHDR *arg )
{
    MSISUMMARYINFO *si = (MSISUMMARYINFO *) arg;
    DWORD i;

    for( i = 0; i < MSI_MAX_PROPS; i++ )
        free_prop( &si->property[i] );
    IStorage_Release( si->storage );
}

static UINT get_type( UINT uiProperty )
{
    switch( uiProperty )
    {
    case PID_CODEPAGE:
         return VT_I2;

    case PID_SUBJECT:
    case PID_AUTHOR:
    case PID_KEYWORDS:
    case PID_COMMENTS:
    case PID_TEMPLATE:
    case PID_LASTAUTHOR:
    case PID_REVNUMBER:
    case PID_APPNAME:
    case PID_TITLE:
         return VT_LPSTR;

    case PID_LASTPRINTED:
    case PID_CREATE_DTM:
    case PID_LASTSAVE_DTM:
         return VT_FILETIME;

    case PID_WORDCOUNT:
    case PID_CHARCOUNT:
    case PID_SECURITY:
    case PID_PAGECOUNT:
         return VT_I4;
    }
    return VT_EMPTY;
}

static UINT get_property_count( PROPVARIANT *property )
{
    UINT i, n = 0;

    if( !property )
        return n;
    for( i = 0; i < MSI_MAX_PROPS; i++ )
        if( property[i].vt != VT_EMPTY )
            n++;
    return n;
}

/* FIXME: doesn't deal with endian conversion */
static void read_properties_from_data( PROPVARIANT *prop, LPBYTE data, DWORD sz )
{
    UINT type;
    DWORD i;
    int size;
    PROPERTY_DATA *propdata;
    PROPVARIANT *property;
    PROPERTYIDOFFSET *idofs;
    PROPERTYSECTIONHEADER *section_hdr;

    section_hdr = (PROPERTYSECTIONHEADER*) &data[0];
    idofs = (PROPERTYIDOFFSET*) &data[SECT_HDR_SIZE];

    /* now set all the properties */
    for( i = 0; i < section_hdr->cProperties; i++ )
    {
        type = get_type( idofs[i].propid );
        if( type == VT_EMPTY )
        {
            ERR("propid %d has unknown type\n", idofs[i].propid);
            break;
        }

        propdata = (PROPERTY_DATA*) &data[ idofs[i].dwOffset ];

        /* check the type is the same as we expect */
        if( type != propdata->type )
        {
            ERR("wrong type %d != %d\n", type, propdata->type);
            break;
        }

        /* check we don't run off the end of the data */
        size = sz - idofs[i].dwOffset - sizeof(DWORD);
        if( sizeof(DWORD) > size ||
            ( type == VT_FILETIME && sizeof(FILETIME) > size ) ||
            ( type == VT_LPSTR && (propdata->u.str.len + sizeof(DWORD)) > size ) )
        {
            ERR("not enough data\n");
            break;
        }

        if( idofs[i].propid >= MSI_MAX_PROPS )
        {
            ERR("Unknown property ID %d\n", idofs[i].propid );
            break;
        }

        property = &prop[ idofs[i].propid ];
        property->vt = type;

        if( type == VT_LPSTR )
        {
            LPSTR str = msi_alloc( propdata->u.str.len );
            memcpy( str, propdata->u.str.str, propdata->u.str.len );
            str[ propdata->u.str.len - 1 ] = 0;
            property->u.pszVal = str;
        }
        else if( type == VT_FILETIME )
            property->u.filetime = propdata->u.ft;
        else if( type == VT_I2 )
            property->u.iVal = propdata->u.i2;
        else if( type == VT_I4 )
            property->u.lVal = propdata->u.i4;
    }
}

static UINT load_summary_info( MSISUMMARYINFO *si, IStream *stm )
{
    UINT ret = ERROR_FUNCTION_FAILED;
    PROPERTYSETHEADER set_hdr;
    FORMATIDOFFSET format_hdr;
    PROPERTYSECTIONHEADER section_hdr;
    LPBYTE data = NULL;
    LARGE_INTEGER ofs;
    ULONG count, sz;
    HRESULT r;

    TRACE("%p %p\n", si, stm);

    /* read the header */
    sz = sizeof set_hdr;
    r = IStream_Read( stm, &set_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ret;

    if( set_hdr.wByteOrder != 0xfffe )
    {
        ERR("property set not big-endian %04X\n", set_hdr.wByteOrder);
        return ret;
    }

    sz = sizeof format_hdr;
    r = IStream_Read( stm, &format_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ret;

    /* check the format id is correct */
    if( !IsEqualGUID( &FMTID_SummaryInformation, &format_hdr.fmtid ) )
        return ret;

    /* seek to the location of the section */
    ofs.QuadPart = format_hdr.dwOffset;
    r = IStream_Seek( stm, ofs, STREAM_SEEK_SET, NULL );
    if( FAILED(r) )
        return ret;

    /* read the section itself */
    sz = SECT_HDR_SIZE;
    r = IStream_Read( stm, &section_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ret;

    if( section_hdr.cProperties > MSI_MAX_PROPS )
    {
        ERR("too many properties %d\n", section_hdr.cProperties);
        return ret;
    }

    data = msi_alloc( section_hdr.cbSection);
    if( !data )
        return ret;

    memcpy( data, &section_hdr, SECT_HDR_SIZE );

    /* read all the data in one go */
    sz = section_hdr.cbSection - SECT_HDR_SIZE;
    r = IStream_Read( stm, &data[ SECT_HDR_SIZE ], sz, &count );
    if( SUCCEEDED(r) && count == sz )
        read_properties_from_data( si->property, data, sz + SECT_HDR_SIZE );
    else
        ERR("failed to read properties %d %d\n", count, sz);

    msi_free( data );
    return ret;
}

static DWORD write_dword( LPBYTE data, DWORD ofs, DWORD val )
{
    if( data )
    {
        data[ofs++] = val&0xff;
        data[ofs++] = (val>>8)&0xff;
        data[ofs++] = (val>>16)&0xff;
        data[ofs++] = (val>>24)&0xff;
    }
    return 4;
}

static DWORD write_filetime( LPBYTE data, DWORD ofs, LPFILETIME ft )
{
    write_dword( data, ofs, ft->dwLowDateTime );
    write_dword( data, ofs + 4, ft->dwHighDateTime );
    return 8;
}

static DWORD write_string( LPBYTE data, DWORD ofs, LPCSTR str )
{
    DWORD len = lstrlenA( str ) + 1;
    write_dword( data, ofs, len );
    if( data )
        memcpy( &data[ofs + 4], str, len );
    return (7 + len) & ~3;
}

static UINT write_property_to_data( PROPVARIANT *prop, LPBYTE data )
{
    DWORD sz = 0;

    if( prop->vt == VT_EMPTY )
        return sz;

    /* add the type */
    sz += write_dword( data, sz, prop->vt );
    switch( prop->vt )
    {
    case VT_I2:
        sz += write_dword( data, sz, prop->u.iVal );
        break;
    case VT_I4:
        sz += write_dword( data, sz, prop->u.lVal );
        break;
    case VT_FILETIME:
        sz += write_filetime( data, sz, &prop->u.filetime );
        break;
    case VT_LPSTR:
        sz += write_string( data, sz, prop->u.pszVal );
        break;
    }
    return sz;
}

static UINT save_summary_info( MSISUMMARYINFO * si, IStream *stm )
{
    UINT ret = ERROR_FUNCTION_FAILED;
    PROPERTYSETHEADER set_hdr;
    FORMATIDOFFSET format_hdr;
    PROPERTYSECTIONHEADER section_hdr;
    PROPERTYIDOFFSET idofs[MSI_MAX_PROPS];
    LPBYTE data = NULL;
    ULONG count, sz;
    HRESULT r;
    int i, n;

    /* write the header */
    sz = sizeof set_hdr;
    memset( &set_hdr, 0, sz );
    set_hdr.wByteOrder = 0xfffe;
    set_hdr.wFormat = 0;
    set_hdr.dwOSVer = 0x00020005; /* build 5, platform id 2 */
    /* set_hdr.clsID is {00000000-0000-0000-0000-000000000000} */
    set_hdr.reserved = 1;
    r = IStream_Write( stm, &set_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ret;

    /* write the format header */
    sz = sizeof format_hdr;
    memcpy( &format_hdr.fmtid, &FMTID_SummaryInformation, sizeof (FMTID) );
    format_hdr.dwOffset = sizeof format_hdr + sizeof set_hdr;
    r = IStream_Write( stm, &format_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ret;

    /* add up how much space the data will take and calculate the offsets */
    section_hdr.cbSection = sizeof section_hdr;
    section_hdr.cbSection += (get_property_count( si->property ) * sizeof idofs[0]);
    section_hdr.cProperties = 0;
    n = 0;
    for( i = 0; i < MSI_MAX_PROPS; i++ )
    {
        sz = write_property_to_data( &si->property[i], NULL );
        if( !sz )
            continue;
        idofs[ section_hdr.cProperties ].propid = i;
        idofs[ section_hdr.cProperties ].dwOffset = section_hdr.cbSection;
        section_hdr.cProperties++;
        section_hdr.cbSection += sz;
    }

    data = msi_alloc_zero( section_hdr.cbSection );

    sz = 0;
    memcpy( &data[sz], &section_hdr, sizeof section_hdr );
    sz += sizeof section_hdr;

    memcpy( &data[sz], idofs, section_hdr.cProperties * sizeof idofs[0] );
    sz += section_hdr.cProperties * sizeof idofs[0];

    /* write out the data */
    for( i = 0; i < MSI_MAX_PROPS; i++ )
        sz += write_property_to_data( &si->property[i], &data[sz] );

    r = IStream_Write( stm, data, sz, &count );
    msi_free( data );
    if( FAILED(r) || count != sz )
        return ret;

    return ERROR_SUCCESS;
}

MSISUMMARYINFO *MSI_GetSummaryInformationW( IStorage *stg, UINT uiUpdateCount )
{
    IStream *stm = NULL;
    MSISUMMARYINFO *si;
    DWORD grfMode;
    HRESULT r;

    TRACE("%p %d\n", stg, uiUpdateCount );

    si = alloc_msiobject( MSIHANDLETYPE_SUMMARYINFO, 
                  sizeof (MSISUMMARYINFO), MSI_CloseSummaryInfo );
    if( !si )
        return si;

    memset( &si->property, 0, sizeof si->property );
    si->update_count = uiUpdateCount;
    IStorage_AddRef( stg );
    si->storage = stg;

    /* read the stream... if we fail, we'll start with an empty property set */
    grfMode = STGM_READ | STGM_SHARE_EXCLUSIVE;
    r = IStorage_OpenStream( si->storage, szSumInfo, 0, grfMode, 0, &stm );
    if( SUCCEEDED(r) )
    {
        load_summary_info( si, stm );
        IStream_Release( stm );
    }

    return si;
}

UINT WINAPI MsiGetSummaryInformationW( MSIHANDLE hDatabase, 
              LPCWSTR szDatabase, UINT uiUpdateCount, MSIHANDLE *pHandle )
{
    MSISUMMARYINFO *si;
    MSIDATABASE *db;
    UINT ret = ERROR_FUNCTION_FAILED;

    TRACE("%ld %s %d %p\n", hDatabase, debugstr_w(szDatabase),
           uiUpdateCount, pHandle);

    if( !pHandle )
        return ERROR_INVALID_PARAMETER;

    if( szDatabase )
    {
        ret = MSI_OpenDatabaseW( szDatabase, NULL, &db );
        if( ret != ERROR_SUCCESS )
            return ret;
    }
    else
    {
        db = msihandle2msiinfo( hDatabase, MSIHANDLETYPE_DATABASE );
        if( !db )
            return ERROR_INVALID_PARAMETER;
    }

    si = MSI_GetSummaryInformationW( db->storage, uiUpdateCount );
    if (si)
    {
        *pHandle = alloc_msihandle( &si->hdr );
        if( *pHandle )
            ret = ERROR_SUCCESS;
        else
            ret = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &si->hdr );
    }

    if( db )
        msiobj_release( &db->hdr );

    return ret;
}

UINT WINAPI MsiGetSummaryInformationA(MSIHANDLE hDatabase, 
              LPCSTR szDatabase, UINT uiUpdateCount, MSIHANDLE *pHandle)
{
    LPWSTR szwDatabase = NULL;
    UINT ret;

    TRACE("%ld %s %d %p\n", hDatabase, debugstr_a(szDatabase), 
          uiUpdateCount, pHandle);

    if( szDatabase )
    {
        szwDatabase = strdupAtoW( szDatabase );
        if( !szwDatabase )
            return ERROR_FUNCTION_FAILED;
    }

    ret = MsiGetSummaryInformationW(hDatabase, szwDatabase, uiUpdateCount, pHandle);

    msi_free( szwDatabase );

    return ret;
}

UINT WINAPI MsiSummaryInfoGetPropertyCount(MSIHANDLE hSummaryInfo, UINT *pCount)
{
    MSISUMMARYINFO *si;

    TRACE("%ld %p\n", hSummaryInfo, pCount);

    si = msihandle2msiinfo( hSummaryInfo, MSIHANDLETYPE_SUMMARYINFO );
    if( !si )
        return ERROR_INVALID_HANDLE;

    if( pCount )
        *pCount = get_property_count( si->property );
    msiobj_release( &si->hdr );

    return ERROR_SUCCESS;
}

static UINT get_prop( MSIHANDLE handle, UINT uiProperty, UINT *puiDataType,
          INT *piValue, FILETIME *pftValue, awstring *str, DWORD *pcchValueBuf)
{
    MSISUMMARYINFO *si;
    PROPVARIANT *prop;
    UINT ret = ERROR_SUCCESS;

    TRACE("%ld %d %p %p %p %p %p\n", handle, uiProperty, puiDataType,
          piValue, pftValue, str, pcchValueBuf);

    si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO );
    if( !si )
        return ERROR_INVALID_HANDLE;

    if ( uiProperty >= MSI_MAX_PROPS )
    {
        *puiDataType = VT_EMPTY;
        return ret;
    }

    prop = &si->property[uiProperty];

    if( puiDataType )
        *puiDataType = prop->vt;

    switch( prop->vt )
    {
    case VT_I2:
        if( piValue )
            *piValue = prop->u.iVal;
        break;
    case VT_I4:
        if( piValue )
            *piValue = prop->u.lVal;
        break;
    case VT_LPSTR:
        if( pcchValueBuf )
        {
            DWORD len = 0;

            if( str->unicode )
            {
                len = MultiByteToWideChar( CP_ACP, 0, prop->u.pszVal, -1,
                                           str->str.w, *pcchValueBuf );
                len--;
            }
            else
            {
                len = lstrlenA( prop->u.pszVal );
                if( str->str.a )
                    lstrcpynA(str->str.a, prop->u.pszVal, *pcchValueBuf );
            }
            if (len >= *pcchValueBuf)
                ret = ERROR_MORE_DATA;
            *pcchValueBuf = len;
        }
        break;
    case VT_FILETIME:
        if( pftValue )
            memcpy(pftValue, &prop->u.filetime, sizeof (FILETIME) );
        break;
    case VT_EMPTY:
        break;
    default:
        FIXME("Unknown property variant type\n");
        break;
    }
    msiobj_release( &si->hdr );
    return ret;
}

LPWSTR msi_suminfo_dup_string( MSISUMMARYINFO *si, UINT uiProperty )
{
    PROPVARIANT *prop;

    if ( uiProperty >= MSI_MAX_PROPS )
        return NULL;
    prop = &si->property[uiProperty];
    if( prop->vt != VT_LPSTR )
        return NULL;
    return strdupAtoW( prop->u.pszVal );
}

LPWSTR msi_get_suminfo_product( IStorage *stg )
{
    MSISUMMARYINFO *si;
    LPWSTR prod;

    si = MSI_GetSummaryInformationW( stg, 0 );
    if (!si)
    {
        ERR("no summary information!\n");
        return NULL;
    }
    prod = msi_suminfo_dup_string( si, PID_REVNUMBER );
    msiobj_release( &si->hdr );
    return prod;
}

UINT WINAPI MsiSummaryInfoGetPropertyA(
      MSIHANDLE handle, UINT uiProperty, UINT *puiDataType, INT *piValue,
      FILETIME *pftValue, LPSTR szValueBuf, DWORD *pcchValueBuf)
{
    awstring str;

    TRACE("%ld %d %p %p %p %p %p\n", handle, uiProperty, puiDataType,
          piValue, pftValue, szValueBuf, pcchValueBuf );

    str.unicode = FALSE;
    str.str.a = szValueBuf;

    return get_prop( handle, uiProperty, puiDataType, piValue,
                     pftValue, &str, pcchValueBuf );
}

UINT WINAPI MsiSummaryInfoGetPropertyW(
      MSIHANDLE handle, UINT uiProperty, UINT *puiDataType, INT *piValue,
      FILETIME *pftValue, LPWSTR szValueBuf, DWORD *pcchValueBuf)
{
    awstring str;

    TRACE("%ld %d %p %p %p %p %p\n", handle, uiProperty, puiDataType,
          piValue, pftValue, szValueBuf, pcchValueBuf );

    str.unicode = TRUE;
    str.str.w = szValueBuf;

    return get_prop( handle, uiProperty, puiDataType, piValue,
                     pftValue, &str, pcchValueBuf );
}

static UINT set_prop( MSIHANDLE handle, UINT uiProperty, UINT uiDataType,
               INT iValue, FILETIME* pftValue, awcstring *str )
{
    MSISUMMARYINFO *si;
    PROPVARIANT *prop;
    UINT type, len, ret = ERROR_SUCCESS;

    TRACE("%ld %u %u %i %p %p\n", handle, uiProperty, uiDataType,
          iValue, pftValue, str );

    type = get_type( uiProperty );
    if( type == VT_EMPTY || type != uiDataType )
        return ERROR_DATATYPE_MISMATCH;

    if( uiDataType == VT_LPSTR && !str->str.w )
        return ERROR_INVALID_PARAMETER;

    if( uiDataType == VT_FILETIME && !pftValue )
        return ERROR_INVALID_PARAMETER;

    si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO );
    if( !si )
        return ERROR_INVALID_HANDLE;

    prop = &si->property[uiProperty];

    if( prop->vt == VT_EMPTY )
    {
        if( !si->update_count )
        {
            ret = ERROR_FUNCTION_FAILED;
            goto end;
        }
        si->update_count--;
    }
    else if( prop->vt != type )
        goto end;

    free_prop( prop );
    prop->vt = type;
    switch( type )
    {
    case VT_I4:
        prop->u.lVal = iValue;
        break;
    case VT_I2:
        prop->u.iVal = iValue;
        break;
    case VT_FILETIME:
        memcpy( &prop->u.filetime, pftValue, sizeof prop->u.filetime );
        break;
    case VT_LPSTR:
        if( str->unicode )
        {
            len = WideCharToMultiByte( CP_ACP, 0, str->str.w, -1,
                                       NULL, 0, NULL, NULL );
            prop->u.pszVal = msi_alloc( len );
            WideCharToMultiByte( CP_ACP, 0, str->str.w, -1,
                                 prop->u.pszVal, len, NULL, NULL );
        }
        else
        {
            len = lstrlenA( str->str.a ) + 1;
            prop->u.pszVal = msi_alloc( len );
            lstrcpyA( prop->u.pszVal, str->str.a );
        }
        break;
    }

end:
    msiobj_release( &si->hdr );
    return ret;
}

UINT WINAPI MsiSummaryInfoSetPropertyW( MSIHANDLE handle, UINT uiProperty,
               UINT uiDataType, INT iValue, FILETIME* pftValue, LPCWSTR szValue )
{
    awcstring str;

    TRACE("%ld %u %u %i %p %s\n", handle, uiProperty, uiDataType,
          iValue, pftValue, debugstr_w(szValue) );

    str.unicode = TRUE;
    str.str.w = szValue;
    return set_prop( handle, uiProperty, uiDataType, iValue, pftValue, &str );
}

UINT WINAPI MsiSummaryInfoSetPropertyA( MSIHANDLE handle, UINT uiProperty,
               UINT uiDataType, INT iValue, FILETIME* pftValue, LPCSTR szValue )
{
    awcstring str;

    TRACE("%ld %u %u %i %p %s\n", handle, uiProperty, uiDataType,
          iValue, pftValue, debugstr_a(szValue) );

    str.unicode = FALSE;
    str.str.a = szValue;
    return set_prop( handle, uiProperty, uiDataType, iValue, pftValue, &str );
}

UINT WINAPI MsiSummaryInfoPersist( MSIHANDLE handle )
{
    IStream *stm = NULL;
    MSISUMMARYINFO *si;
    DWORD grfMode;
    HRESULT r;
    UINT ret = ERROR_FUNCTION_FAILED;

    TRACE("%ld\n", handle );

    si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO );
    if( !si )
        return ERROR_INVALID_HANDLE;

    grfMode = STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
    r = IStorage_CreateStream( si->storage, szSumInfo, grfMode, 0, 0, &stm );
    if( SUCCEEDED(r) )
    {
        ret = save_summary_info( si, stm );
        IStream_Release( stm );
    }
    msiobj_release( &si->hdr );

    return ret;
}
