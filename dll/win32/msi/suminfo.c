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

#include "stdio.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "msi.h"
#include "msiquery.h"
#include "msidefs.h"
#include "msipriv.h"
#include "objidl.h"
#include "propvarutil.h"
#include "msiserver.h"

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

static HRESULT (WINAPI *pPropVariantChangeType)
    (PROPVARIANT *ppropvarDest, REFPROPVARIANT propvarSrc,
     PROPVAR_CHANGE_FLAGS flags, VARTYPE vt);

#define SECT_HDR_SIZE (sizeof(PROPERTYSECTIONHEADER))

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

static UINT get_property_count( const PROPVARIANT *property )
{
    UINT i, n = 0;

    if( !property )
        return n;
    for( i = 0; i < MSI_MAX_PROPS; i++ )
        if( property[i].vt != VT_EMPTY )
            n++;
    return n;
}

static UINT propvar_changetype(PROPVARIANT *changed, PROPVARIANT *property, VARTYPE vt)
{
    HRESULT hr;
    HMODULE propsys = LoadLibraryA("propsys.dll");
    pPropVariantChangeType = (void *)GetProcAddress(propsys, "PropVariantChangeType");

    if (!pPropVariantChangeType)
    {
        ERR("PropVariantChangeType function missing!\n");
        return ERROR_FUNCTION_FAILED;
    }

    hr = pPropVariantChangeType(changed, property, 0, vt);
    return (hr == S_OK) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

/* FIXME: doesn't deal with endian conversion */
static void read_properties_from_data( PROPVARIANT *prop, LPBYTE data, DWORD sz )
{
    UINT type;
    DWORD i, size;
    PROPERTY_DATA *propdata;
    PROPVARIANT property, *ptr;
    PROPVARIANT changed;
    PROPERTYIDOFFSET *idofs;
    PROPERTYSECTIONHEADER *section_hdr;

    section_hdr = (PROPERTYSECTIONHEADER*) &data[0];
    idofs = (PROPERTYIDOFFSET*) &data[SECT_HDR_SIZE];

    /* now set all the properties */
    for( i = 0; i < section_hdr->cProperties; i++ )
    {
        if( idofs[i].propid >= MSI_MAX_PROPS )
        {
            ERR("Unknown property ID %d\n", idofs[i].propid );
            break;
        }

        type = get_type( idofs[i].propid );
        if( type == VT_EMPTY )
        {
            ERR("propid %d has unknown type\n", idofs[i].propid);
            break;
        }

        propdata = (PROPERTY_DATA*) &data[ idofs[i].dwOffset ];

        /* check we don't run off the end of the data */
        size = sz - idofs[i].dwOffset - sizeof(DWORD);
        if( sizeof(DWORD) > size ||
            ( propdata->type == VT_FILETIME && sizeof(FILETIME) > size ) ||
            ( propdata->type == VT_LPSTR && (propdata->u.str.len + sizeof(DWORD)) > size ) )
        {
            ERR("not enough data\n");
            break;
        }

        property.vt = propdata->type;
        if( propdata->type == VT_LPSTR )
        {
            LPSTR str = msi_alloc( propdata->u.str.len );
            memcpy( str, propdata->u.str.str, propdata->u.str.len );
            str[ propdata->u.str.len - 1 ] = 0;
            property.u.pszVal = str;
        }
        else if( propdata->type == VT_FILETIME )
            property.u.filetime = propdata->u.ft;
        else if( propdata->type == VT_I2 )
            property.u.iVal = propdata->u.i2;
        else if( propdata->type == VT_I4 )
            property.u.lVal = propdata->u.i4;

        /* check the type is the same as we expect */
        if( type != propdata->type )
        {
            propvar_changetype(&changed, &property, type);
            ptr = &changed;
        }
        else
            ptr = &property;

        prop[ idofs[i].propid ] = *ptr;
    }
}

static UINT load_summary_info( MSISUMMARYINFO *si, IStream *stm )
{
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
        return ERROR_FUNCTION_FAILED;

    if( set_hdr.wByteOrder != 0xfffe )
    {
        ERR("property set not big-endian %04X\n", set_hdr.wByteOrder);
        return ERROR_FUNCTION_FAILED;
    }

    sz = sizeof format_hdr;
    r = IStream_Read( stm, &format_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ERROR_FUNCTION_FAILED;

    /* check the format id is correct */
    if( !IsEqualGUID( &FMTID_SummaryInformation, &format_hdr.fmtid ) )
        return ERROR_FUNCTION_FAILED;

    /* seek to the location of the section */
    ofs.QuadPart = format_hdr.dwOffset;
    r = IStream_Seek( stm, ofs, STREAM_SEEK_SET, NULL );
    if( FAILED(r) )
        return ERROR_FUNCTION_FAILED;

    /* read the section itself */
    sz = SECT_HDR_SIZE;
    r = IStream_Read( stm, &section_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ERROR_FUNCTION_FAILED;

    if( section_hdr.cProperties > MSI_MAX_PROPS )
    {
        ERR("too many properties %d\n", section_hdr.cProperties);
        return ERROR_FUNCTION_FAILED;
    }

    data = msi_alloc( section_hdr.cbSection);
    if( !data )
        return ERROR_FUNCTION_FAILED;

    memcpy( data, &section_hdr, SECT_HDR_SIZE );

    /* read all the data in one go */
    sz = section_hdr.cbSection - SECT_HDR_SIZE;
    r = IStream_Read( stm, &data[ SECT_HDR_SIZE ], sz, &count );
    if( SUCCEEDED(r) && count == sz )
        read_properties_from_data( si->property, data, sz + SECT_HDR_SIZE );
    else
        ERR("failed to read properties %d %d\n", count, sz);

    msi_free( data );
    return ERROR_SUCCESS;
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

static DWORD write_filetime( LPBYTE data, DWORD ofs, const FILETIME *ft )
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

static UINT write_property_to_data( const PROPVARIANT *prop, LPBYTE data )
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

static UINT save_summary_info( const MSISUMMARYINFO * si, IStream *stm )
{
    UINT ret = ERROR_FUNCTION_FAILED;
    PROPERTYSETHEADER set_hdr;
    FORMATIDOFFSET format_hdr;
    PROPERTYSECTIONHEADER section_hdr;
    PROPERTYIDOFFSET idofs[MSI_MAX_PROPS];
    LPBYTE data = NULL;
    ULONG count, sz;
    HRESULT r;
    int i;

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
    format_hdr.fmtid = FMTID_SummaryInformation;
    format_hdr.dwOffset = sizeof format_hdr + sizeof set_hdr;
    r = IStream_Write( stm, &format_hdr, sz, &count );
    if( FAILED(r) || count != sz )
        return ret;

    /* add up how much space the data will take and calculate the offsets */
    section_hdr.cbSection = sizeof section_hdr;
    section_hdr.cbSection += (get_property_count( si->property ) * sizeof idofs[0]);
    section_hdr.cProperties = 0;
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

static MSISUMMARYINFO *create_suminfo( IStorage *stg, UINT update_count )
{
    MSISUMMARYINFO *si;

    if (!(si = alloc_msiobject( MSIHANDLETYPE_SUMMARYINFO, sizeof(MSISUMMARYINFO), MSI_CloseSummaryInfo )))
        return NULL;

    si->update_count = update_count;
    IStorage_AddRef( stg );
    si->storage = stg;

    return si;
}

UINT msi_get_suminfo( IStorage *stg, UINT uiUpdateCount, MSISUMMARYINFO **ret )
{
    IStream *stm;
    MSISUMMARYINFO *si;
    HRESULT hr;
    UINT r;

    TRACE("%p, %u\n", stg, uiUpdateCount);

    if (!(si = create_suminfo( stg, uiUpdateCount ))) return ERROR_OUTOFMEMORY;

    hr = IStorage_OpenStream( si->storage, szSumInfo, 0, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &stm );
    if (FAILED( hr ))
    {
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }

    r = load_summary_info( si, stm );
    IStream_Release( stm );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &si->hdr );
        return r;
    }

    *ret = si;
    return ERROR_SUCCESS;
}

UINT msi_get_db_suminfo( MSIDATABASE *db, UINT uiUpdateCount, MSISUMMARYINFO **ret )
{
    IStream *stm;
    MSISUMMARYINFO *si;
    UINT r;

    if (!(si = create_suminfo( db->storage, uiUpdateCount ))) return ERROR_OUTOFMEMORY;

    r = msi_get_stream( db, szSumInfo, &stm );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &si->hdr );
        return r;
    }

    r = load_summary_info( si, stm );
    IStream_Release( stm );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &si->hdr );
        return r;
    }

    *ret = si;
    return ERROR_SUCCESS;
}

UINT WINAPI MsiGetSummaryInformationW( MSIHANDLE hDatabase, 
              LPCWSTR szDatabase, UINT uiUpdateCount, MSIHANDLE *pHandle )
{
    MSISUMMARYINFO *si;
    MSIDATABASE *db;
    UINT ret;

    TRACE("%d %s %d %p\n", hDatabase, debugstr_w(szDatabase),
           uiUpdateCount, pHandle);

    if( !pHandle )
        return ERROR_INVALID_PARAMETER;

    if( szDatabase && szDatabase[0] )
    {
        LPCWSTR persist = uiUpdateCount ? MSIDBOPEN_TRANSACT : MSIDBOPEN_READONLY;

        ret = MSI_OpenDatabaseW( szDatabase, persist, &db );
        if( ret != ERROR_SUCCESS )
            return ret;
    }
    else
    {
        db = msihandle2msiinfo( hDatabase, MSIHANDLETYPE_DATABASE );
        if( !db )
        {
            HRESULT hr;
            IWineMsiRemoteDatabase *remote_database;

            remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( hDatabase );
            if ( !remote_database )
                return ERROR_INVALID_HANDLE;

            hr = IWineMsiRemoteDatabase_GetSummaryInformation( remote_database,
                                                               uiUpdateCount, pHandle );
            IWineMsiRemoteDatabase_Release( remote_database );

            if (FAILED(hr))
            {
                if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                    return HRESULT_CODE(hr);

                return ERROR_FUNCTION_FAILED;
            }

            return ERROR_SUCCESS;
        }
    }

    ret = msi_get_suminfo( db->storage, uiUpdateCount, &si );
    if (ret != ERROR_SUCCESS)
        ret = msi_get_db_suminfo( db, uiUpdateCount, &si );
    if (ret != ERROR_SUCCESS)
    {
        if ((si = create_suminfo( db->storage, uiUpdateCount )))
            ret = ERROR_SUCCESS;
    }

    if (ret == ERROR_SUCCESS)
    {
        *pHandle = alloc_msihandle( &si->hdr );
        if( *pHandle )
            ret = ERROR_SUCCESS;
        else
            ret = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &si->hdr );
    }

    msiobj_release( &db->hdr );
    return ret;
}

UINT WINAPI MsiGetSummaryInformationA(MSIHANDLE hDatabase, 
              LPCSTR szDatabase, UINT uiUpdateCount, MSIHANDLE *pHandle)
{
    LPWSTR szwDatabase = NULL;
    UINT ret;

    TRACE("%d %s %d %p\n", hDatabase, debugstr_a(szDatabase),
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

UINT WINAPI MsiSummaryInfoGetPropertyCount(MSIHANDLE hSummaryInfo, PUINT pCount)
{
    MSISUMMARYINFO *si;

    TRACE("%d %p\n", hSummaryInfo, pCount);

    si = msihandle2msiinfo( hSummaryInfo, MSIHANDLETYPE_SUMMARYINFO );
    if( !si )
        return ERROR_INVALID_HANDLE;

    if( pCount )
        *pCount = get_property_count( si->property );
    msiobj_release( &si->hdr );

    return ERROR_SUCCESS;
}

static UINT get_prop( MSISUMMARYINFO *si, UINT uiProperty, UINT *puiDataType, INT *piValue,
                      FILETIME *pftValue, awstring *str, DWORD *pcchValueBuf)
{
    PROPVARIANT *prop;
    UINT ret = ERROR_SUCCESS;

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
                len = MultiByteToWideChar( CP_ACP, 0, prop->u.pszVal, -1, NULL, 0 ) - 1;
                MultiByteToWideChar( CP_ACP, 0, prop->u.pszVal, -1, str->str.w, *pcchValueBuf );
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
            *pftValue = prop->u.filetime;
        break;
    case VT_EMPTY:
        break;
    default:
        FIXME("Unknown property variant type\n");
        break;
    }
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

INT msi_suminfo_get_int32( MSISUMMARYINFO *si, UINT uiProperty )
{
    PROPVARIANT *prop;

    if ( uiProperty >= MSI_MAX_PROPS )
        return -1;
    prop = &si->property[uiProperty];
    if( prop->vt != VT_I4 )
        return -1;
    return prop->u.lVal;
}

LPWSTR msi_get_suminfo_product( IStorage *stg )
{
    MSISUMMARYINFO *si;
    LPWSTR prod;
    UINT r;

    r = msi_get_suminfo( stg, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        ERR("no summary information!\n");
        return NULL;
    }
    prod = msi_suminfo_dup_string( si, PID_REVNUMBER );
    msiobj_release( &si->hdr );
    return prod;
}

UINT WINAPI MsiSummaryInfoGetPropertyA(
      MSIHANDLE handle, UINT uiProperty, PUINT puiDataType, LPINT piValue,
      FILETIME *pftValue, LPSTR szValueBuf, LPDWORD pcchValueBuf)
{
    MSISUMMARYINFO *si;
    awstring str;
    UINT r;

    TRACE("%u, %u, %p, %p, %p, %p, %p\n", handle, uiProperty, puiDataType,
          piValue, pftValue, szValueBuf, pcchValueBuf );

    if (uiProperty >= MSI_MAX_PROPS)
    {
        if (puiDataType) *puiDataType = VT_EMPTY;
        return ERROR_UNKNOWN_PROPERTY;
    }

    if (!(si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO )))
        return ERROR_INVALID_HANDLE;

    str.unicode = FALSE;
    str.str.a = szValueBuf;

    r = get_prop( si, uiProperty, puiDataType, piValue, pftValue, &str, pcchValueBuf );
    msiobj_release( &si->hdr );
    return r;
}

UINT WINAPI MsiSummaryInfoGetPropertyW(
      MSIHANDLE handle, UINT uiProperty, PUINT puiDataType, LPINT piValue,
      FILETIME *pftValue, LPWSTR szValueBuf, LPDWORD pcchValueBuf)
{
    MSISUMMARYINFO *si;
    awstring str;
    UINT r;

    TRACE("%u, %u, %p, %p, %p, %p, %p\n", handle, uiProperty, puiDataType,
          piValue, pftValue, szValueBuf, pcchValueBuf );

    if (uiProperty >= MSI_MAX_PROPS)
    {
        if (puiDataType) *puiDataType = VT_EMPTY;
        return ERROR_UNKNOWN_PROPERTY;
    }

    if (!(si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO )))
        return ERROR_INVALID_HANDLE;

    str.unicode = TRUE;
    str.str.w = szValueBuf;

    r = get_prop( si, uiProperty, puiDataType, piValue, pftValue, &str, pcchValueBuf );
    msiobj_release( &si->hdr );
    return r;
}

static UINT set_prop( MSISUMMARYINFO *si, UINT uiProperty, UINT type,
                      INT iValue, FILETIME *pftValue, awcstring *str )
{
    PROPVARIANT *prop;
    UINT len;

    TRACE("%p, %u, %u, %d, %p, %p\n", si, uiProperty, type, iValue, pftValue, str );

    prop = &si->property[uiProperty];

    if( prop->vt == VT_EMPTY )
    {
        if( !si->update_count )
            return ERROR_FUNCTION_FAILED;

        si->update_count--;
    }
    else if( prop->vt != type )
        return ERROR_SUCCESS;

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
        prop->u.filetime = *pftValue;
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

    return ERROR_SUCCESS;
}

UINT WINAPI MsiSummaryInfoSetPropertyW( MSIHANDLE handle, UINT uiProperty, UINT uiDataType,
                                        INT iValue, FILETIME *pftValue, LPCWSTR szValue )
{
    awcstring str;
    MSISUMMARYINFO *si;
    UINT type, ret;

    TRACE("%u, %u, %u, %d, %p, %s\n", handle, uiProperty, uiDataType, iValue, pftValue,
          debugstr_w(szValue) );

    type = get_type( uiProperty );
    if( type == VT_EMPTY || type != uiDataType )
        return ERROR_DATATYPE_MISMATCH;

    if( uiDataType == VT_LPSTR && !szValue )
        return ERROR_INVALID_PARAMETER;

    if( uiDataType == VT_FILETIME && !pftValue )
        return ERROR_INVALID_PARAMETER;

    if (!(si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO )))
        return ERROR_INVALID_HANDLE;

    str.unicode = TRUE;
    str.str.w = szValue;

    ret = set_prop( si, uiProperty, type, iValue, pftValue, &str );
    msiobj_release( &si->hdr );
    return ret;
}

UINT WINAPI MsiSummaryInfoSetPropertyA( MSIHANDLE handle, UINT uiProperty, UINT uiDataType,
                                        INT iValue, FILETIME *pftValue, LPCSTR szValue )
{
    awcstring str;
    MSISUMMARYINFO *si;
    UINT type, ret;

    TRACE("%u, %u, %u, %d, %p, %s\n", handle, uiProperty, uiDataType, iValue, pftValue,
          debugstr_a(szValue) );

    type = get_type( uiProperty );
    if( type == VT_EMPTY || type != uiDataType )
        return ERROR_DATATYPE_MISMATCH;

    if( uiDataType == VT_LPSTR && !szValue )
        return ERROR_INVALID_PARAMETER;

    if( uiDataType == VT_FILETIME && !pftValue )
        return ERROR_INVALID_PARAMETER;

    if (!(si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO )))
        return ERROR_INVALID_HANDLE;

    str.unicode = FALSE;
    str.str.a = szValue;

    ret = set_prop( si, uiProperty, uiDataType, iValue, pftValue, &str );
    msiobj_release( &si->hdr );
    return ret;
}

static UINT suminfo_persist( MSISUMMARYINFO *si )
{
    UINT ret = ERROR_FUNCTION_FAILED;
    IStream *stm = NULL;
    DWORD grfMode;
    HRESULT r;

    grfMode = STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
    r = IStorage_CreateStream( si->storage, szSumInfo, grfMode, 0, 0, &stm );
    if( SUCCEEDED(r) )
    {
        ret = save_summary_info( si, stm );
        IStream_Release( stm );
    }
    return ret;
}

static void parse_filetime( LPCWSTR str, FILETIME *ft )
{
    SYSTEMTIME lt, utc;
    const WCHAR *p = str;
    WCHAR *end;

    memset( &lt, 0, sizeof(lt) );

    /* YYYY/MM/DD hh:mm:ss */

    while (isspaceW( *p )) p++;

    lt.wYear = strtolW( p, &end, 10 );
    if (*end != '/') return;
    p = end + 1;

    lt.wMonth = strtolW( p, &end, 10 );
    if (*end != '/') return;
    p = end + 1;

    lt.wDay = strtolW( p, &end, 10 );
    if (*end != ' ') return;
    p = end + 1;

    while (isspaceW( *p )) p++;

    lt.wHour = strtolW( p, &end, 10 );
    if (*end != ':') return;
    p = end + 1;

    lt.wMinute = strtolW( p, &end, 10 );
    if (*end != ':') return;
    p = end + 1;

    lt.wSecond = strtolW( p, &end, 10 );

    TzSpecificLocalTimeToSystemTime( NULL, &lt, &utc );
    SystemTimeToFileTime( &utc, ft );
}

static UINT parse_prop( LPCWSTR prop, LPCWSTR value, UINT *pid, INT *int_value,
                        FILETIME *ft_value, awcstring *str_value )
{
    *pid = atoiW( prop );
    switch (*pid)
    {
    case PID_CODEPAGE:
    case PID_WORDCOUNT:
    case PID_CHARCOUNT:
    case PID_SECURITY:
    case PID_PAGECOUNT:
        *int_value = atoiW( value );
        break;

    case PID_LASTPRINTED:
    case PID_CREATE_DTM:
    case PID_LASTSAVE_DTM:
        parse_filetime( value, ft_value );
        break;

    case PID_SUBJECT:
    case PID_AUTHOR:
    case PID_KEYWORDS:
    case PID_COMMENTS:
    case PID_TEMPLATE:
    case PID_LASTAUTHOR:
    case PID_REVNUMBER:
    case PID_APPNAME:
    case PID_TITLE:
        str_value->str.w = value;
        str_value->unicode = TRUE;
        break;

    default:
        WARN("unhandled prop id %u\n", *pid);
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

UINT msi_add_suminfo( MSIDATABASE *db, LPWSTR **records, int num_records, int num_columns )
{
    UINT r;
    int i, j;
    MSISUMMARYINFO *si;

    r = msi_get_suminfo( db->storage, num_records * (num_columns / 2), &si );
    if (r != ERROR_SUCCESS)
    {
        if (!(si = create_suminfo( db->storage, num_records * (num_columns / 2) )))
            return ERROR_OUTOFMEMORY;
        r = ERROR_SUCCESS;
    }

    for (i = 0; i < num_records; i++)
    {
        for (j = 0; j < num_columns; j += 2)
        {
            UINT pid;
            INT int_value = 0;
            FILETIME ft_value;
            awcstring str_value;

            r = parse_prop( records[i][j], records[i][j + 1], &pid, &int_value, &ft_value, &str_value );
            if (r != ERROR_SUCCESS)
                goto end;

            r = set_prop( si, pid, get_type(pid), int_value, &ft_value, &str_value );
            if (r != ERROR_SUCCESS)
                goto end;
        }
    }

end:
    if (r == ERROR_SUCCESS)
        r = suminfo_persist( si );

    msiobj_release( &si->hdr );
    return r;
}

static UINT save_prop( MSISUMMARYINFO *si, HANDLE handle, UINT row )
{
    static const char fmt_systemtime[] = "%d/%02d/%02d %02d:%02d:%02d";
    char data[20]; /* largest string: YYYY/MM/DD hh:mm:ss */
    static const char fmt_begin[] = "%u\t";
    static const char data_end[] = "\r\n";
    static const char fmt_int[] = "%u";
    UINT r, data_type, len;
    SYSTEMTIME system_time;
    FILETIME file_time;
    INT int_value;
    awstring str;
    DWORD sz;

    str.unicode = FALSE;
    str.str.a = NULL;
    len = 0;
    r = get_prop( si, row, &data_type, &int_value, &file_time, &str, &len );
    if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA)
        return r;
    if (data_type == VT_EMPTY)
        return ERROR_SUCCESS; /* property not set */
    snprintf( data, sizeof(data), fmt_begin, row );
    sz = lstrlenA( data );
    if (!WriteFile( handle, data, sz, &sz, NULL ))
        return ERROR_WRITE_FAULT;

    switch (data_type)
    {
    case VT_I2:
    case VT_I4:
        snprintf( data, sizeof(data), fmt_int, int_value );
        sz = lstrlenA( data );
        if (!WriteFile( handle, data, sz, &sz, NULL ))
            return ERROR_WRITE_FAULT;
        break;
    case VT_LPSTR:
        len++;
        if (!(str.str.a = msi_alloc( len )))
            return ERROR_OUTOFMEMORY;
        r = get_prop( si, row, NULL, NULL, NULL, &str, &len );
        if (r != ERROR_SUCCESS)
        {
            msi_free( str.str.a );
            return r;
        }
        sz = lstrlenA( str.str.a );
        if (!WriteFile( handle, str.str.a, sz, &sz, NULL ))
        {
            msi_free( str.str.a );
            return ERROR_WRITE_FAULT;
        }
        msi_free( str.str.a );
        break;
    case VT_FILETIME:
        if (!FileTimeToSystemTime( &file_time, &system_time ))
            return ERROR_FUNCTION_FAILED;
        snprintf( data, sizeof(data), fmt_systemtime, system_time.wYear, system_time.wMonth,
                  system_time.wDay, system_time.wHour, system_time.wMinute,
                  system_time.wSecond );
        sz = lstrlenA( data );
        if (!WriteFile( handle, data, sz, &sz, NULL ))
            return ERROR_WRITE_FAULT;
        break;
    case VT_EMPTY:
        /* cannot reach here, property not set */
        break;
    default:
        FIXME( "Unknown property variant type\n" );
        return ERROR_FUNCTION_FAILED;
    }

    sz = lstrlenA( data_end );
    if (!WriteFile( handle, data_end, sz, &sz, NULL ))
        return ERROR_WRITE_FAULT;

    return ERROR_SUCCESS;
}

UINT msi_export_suminfo( MSIDATABASE *db, HANDLE handle )
{
    UINT i, r, num_rows;
    MSISUMMARYINFO *si;

    r = msi_get_suminfo( db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
        r = msi_get_db_suminfo( db, 0, &si );
    if (r != ERROR_SUCCESS)
        return r;

    num_rows = get_property_count( si->property );
    if (!num_rows)
    {
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }

    for (i = 0; i < num_rows; i++)
    {
        r = save_prop( si, handle, i );
        if (r != ERROR_SUCCESS)
        {
            msiobj_release( &si->hdr );
            return r;
        }
    }

    msiobj_release( &si->hdr );
    return ERROR_SUCCESS;
}

UINT WINAPI MsiSummaryInfoPersist( MSIHANDLE handle )
{
    MSISUMMARYINFO *si;
    UINT ret;

    TRACE("%d\n", handle );

    si = msihandle2msiinfo( handle, MSIHANDLETYPE_SUMMARYINFO );
    if( !si )
        return ERROR_INVALID_HANDLE;

    ret = suminfo_persist( si );

    msiobj_release( &si->hdr );
    return ret;
}

UINT WINAPI MsiCreateTransformSummaryInfoA( MSIHANDLE db, MSIHANDLE db_ref, LPCSTR transform, int error, int validation )
{
    UINT r;
    WCHAR *transformW = NULL;

    TRACE("%u, %u, %s, %d, %d\n", db, db_ref, debugstr_a(transform), error, validation);

    if (transform && !(transformW = strdupAtoW( transform )))
        return ERROR_OUTOFMEMORY;

    r = MsiCreateTransformSummaryInfoW( db, db_ref, transformW, error, validation );
    msi_free( transformW );
    return r;
}

UINT WINAPI MsiCreateTransformSummaryInfoW( MSIHANDLE db, MSIHANDLE db_ref, LPCWSTR transform, int error, int validation )
{
    FIXME("%u, %u, %s, %d, %d\n", db, db_ref, debugstr_w(transform), error, validation);
    return ERROR_FUNCTION_FAILED;
}

UINT msi_load_suminfo_properties( MSIPACKAGE *package )
{
    static const WCHAR packagecodeW[] = {'P','a','c','k','a','g','e','C','o','d','e',0};
    MSISUMMARYINFO *si;
    WCHAR *package_code;
    UINT r, len;
    awstring str;
    INT count;

    r = msi_get_suminfo( package->db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        r = msi_get_db_suminfo( package->db, 0, &si );
        if (r != ERROR_SUCCESS)
        {
            ERR("Unable to open summary information stream %u\n", r);
            return r;
        }
    }

    str.unicode = TRUE;
    str.str.w = NULL;
    len = 0;
    r = get_prop( si, PID_REVNUMBER, NULL, NULL, NULL, &str, &len );
    if (r != ERROR_MORE_DATA)
    {
        WARN("Unable to query revision number %u\n", r);
        msiobj_release( &si->hdr );
        return ERROR_FUNCTION_FAILED;
    }

    len++;
    if (!(package_code = msi_alloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    str.str.w = package_code;

    r = get_prop( si, PID_REVNUMBER, NULL, NULL, NULL, &str, &len );
    if (r != ERROR_SUCCESS)
    {
        msi_free( package_code );
        msiobj_release( &si->hdr );
        return r;
    }

    r = msi_set_property( package->db, packagecodeW, package_code, len );
    msi_free( package_code );

    count = 0;
    get_prop( si, PID_WORDCOUNT, NULL, &count, NULL, NULL, NULL );
    package->WordCount = count;

    msiobj_release( &si->hdr );
    return r;
}
