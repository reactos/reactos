/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
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
#define NONAMELESSUNION

#define PRSPEC_PROPID (1)

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "objidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static const WCHAR szSumInfo[] = { 5 ,'S','u','m','m','a','r','y',
                       'I','n','f','o','r','m','a','t','i','o','n',0 };

static void MSI_CloseSummaryInfo( MSIOBJECTHDR *arg )
{
    MSISUMMARYINFO *suminfo = (MSISUMMARYINFO *) arg;
    IPropertyStorage_Release( suminfo->propstg );
}

UINT WINAPI MsiGetSummaryInformationA(MSIHANDLE hDatabase, 
              LPCSTR szDatabase, UINT uiUpdateCount, MSIHANDLE *phSummaryInfo)
{
    LPWSTR szwDatabase = NULL;
    UINT ret;

    TRACE("%ld %s %d %p\n", hDatabase, debugstr_a(szDatabase), 
          uiUpdateCount, phSummaryInfo);

    if( szDatabase )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szDatabase, -1, NULL, 0 );
        szwDatabase = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwDatabase )
            return ERROR_FUNCTION_FAILED;
        MultiByteToWideChar( CP_ACP, 0, szDatabase, -1, szwDatabase, len );
    }

    ret = MsiGetSummaryInformationW(hDatabase, szwDatabase, uiUpdateCount, phSummaryInfo);

    if( szwDatabase )
        HeapFree( GetProcessHeap(), 0, szwDatabase );

    return ret;
}

UINT WINAPI MsiGetSummaryInformationW(MSIHANDLE hDatabase, 
              LPCWSTR szDatabase, UINT uiUpdateCount, MSIHANDLE *phSummaryInfo)
{
    HRESULT r;
    MSIHANDLE handle;
    MSISUMMARYINFO *suminfo;
    MSIDATABASE *db;
    UINT ret = ERROR_SUCCESS;
    IPropertySetStorage *psstg = NULL;
    IPropertyStorage *ps = NULL;
    DWORD grfMode;

    TRACE("%ld %s %d %p\n", hDatabase, debugstr_w(szDatabase),
           uiUpdateCount, phSummaryInfo);

    if( !phSummaryInfo )
        return ERROR_INVALID_PARAMETER;

    if( szDatabase )
    {
        UINT res;

        res = MSI_OpenDatabaseW(szDatabase, NULL, &db);
        if( res != ERROR_SUCCESS )
            return res;
    }
    else
    {
        db = msihandle2msiinfo(hDatabase, MSIHANDLETYPE_DATABASE);
        if( !db )
            return ERROR_INVALID_PARAMETER;
    }

    r = IStorage_QueryInterface( db->storage, 
             &IID_IPropertySetStorage, (LPVOID)&psstg);
    if( FAILED( r ) )
    {
        ERR("IStorage -> IPropertySetStorage failed\n");
        if (db)
            msiobj_release(&db->hdr);
        return ERROR_FUNCTION_FAILED;
    }
    ERR("storage = %p propertysetstorage = %p\n", db->storage, psstg);

    grfMode = STGM_READ | STGM_SHARE_EXCLUSIVE;

    r = IPropertySetStorage_Open( psstg, &FMTID_SummaryInformation, grfMode, &ps );
    if( FAILED( r ) )
    {
        ERR("failed to get IPropertyStorage r=%08lx\n",r);
        ret = ERROR_FUNCTION_FAILED;
        goto end;
    }

    suminfo = alloc_msiobject( MSIHANDLETYPE_SUMMARYINFO, 
                  sizeof (MSISUMMARYINFO), MSI_CloseSummaryInfo );
    if( !suminfo )
    {
        ret = ERROR_FUNCTION_FAILED;
        goto end;
    }

    IPropertyStorage_AddRef(ps);
    suminfo->propstg = ps;
    handle = alloc_msihandle( &suminfo->hdr );
    if( handle )
    *phSummaryInfo = handle;
    else
        ret = ERROR_FUNCTION_FAILED;
    msiobj_release( &suminfo->hdr );

end:
    if( ps )
        IPropertyStorage_Release(ps);
    if( psstg )
        IPropertySetStorage_Release(psstg);
    if (db)
        msiobj_release(&db->hdr);

    return ret;
}

UINT WINAPI MsiSummaryInfoGetPropertyCount(MSIHANDLE hSummaryInfo, UINT *pCount)
{
    MSISUMMARYINFO *suminfo;

    FIXME("%ld %p\n",hSummaryInfo, pCount);

    suminfo = msihandle2msiinfo( hSummaryInfo, MSIHANDLETYPE_SUMMARYINFO );
    if( !suminfo )
        return ERROR_INVALID_HANDLE;

    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiSummaryInfoGetPropertyA(
      MSIHANDLE hSummaryInfo, UINT uiProperty, UINT *puiDataType, INT *piValue,
      FILETIME *pftValue, LPSTR szValueBuf, DWORD *pcchValueBuf)
{
    MSISUMMARYINFO *suminfo;
    HRESULT r;
    PROPSPEC spec;
    PROPVARIANT var;

    TRACE("%ld %d %p %p %p %p %p\n",
        hSummaryInfo, uiProperty, puiDataType, piValue,
        pftValue, szValueBuf, pcchValueBuf);

    suminfo = msihandle2msiinfo( hSummaryInfo, MSIHANDLETYPE_SUMMARYINFO );
    if( !suminfo )
        return ERROR_INVALID_HANDLE;

    spec.ulKind = PRSPEC_PROPID;
    spec.u.propid = uiProperty;

    r = IPropertyStorage_ReadMultiple( suminfo->propstg, 1, &spec, &var);
    if( FAILED(r) )
        return ERROR_FUNCTION_FAILED;

    if( puiDataType )
        *puiDataType = var.vt;

    switch( var.vt )
    {
    case VT_I4:
        if( piValue )
            *piValue = var.u.lVal;
        break;
    case VT_LPSTR:
        if( pcchValueBuf && szValueBuf )
        {
            lstrcpynA(szValueBuf, var.u.pszVal, *pcchValueBuf );
            *pcchValueBuf = lstrlenA( var.u.pszVal );
        }
        break;
    case VT_FILETIME:
        if( pftValue )
            memcpy(pftValue, &var.u.filetime, sizeof (FILETIME) );
        break;
    case VT_EMPTY:
        break;
    default:
        FIXME("Unknown property variant type\n");
        break;
    }

    return ERROR_SUCCESS;
}

UINT WINAPI MsiSummaryInfoGetPropertyW(
      MSIHANDLE hSummaryInfo, UINT uiProperty, UINT *puiDataType, INT *piValue,
      FILETIME *pftValue, LPWSTR szValueBuf, DWORD *pcchValueBuf)
{
    FIXME("%ld %d %p %p %p %p %p\n",
        hSummaryInfo, uiProperty, puiDataType, piValue,
        pftValue, szValueBuf, pcchValueBuf);
    
    return ERROR_CALL_NOT_IMPLEMENTED;
}
