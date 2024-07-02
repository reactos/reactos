/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002,2003,2004,2005 Mike McCormack for CodeWeavers
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
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "msi.h"
#include "msidefs.h"
#include "msiquery.h"
#include "wincrypt.h"
#include "winver.h"
#include "winuser.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "objidl.h"
#include "wintrust.h"
#include "softpub.h"

#include "msipriv.h"
#include "winemsi_s.h"

#include "initguid.h"
#include "msxml2.h"

#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

BOOL is_wow64;

UINT msi_locate_product(LPCWSTR szProduct, MSIINSTALLCONTEXT *context)
{
    HKEY hkey = NULL;

    *context = MSIINSTALLCONTEXT_NONE;
    if (!szProduct) return ERROR_UNKNOWN_PRODUCT;

    if (MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                              &hkey, FALSE) == ERROR_SUCCESS)
        *context = MSIINSTALLCONTEXT_USERMANAGED;
    else if (MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_MACHINE,
                                   &hkey, FALSE) == ERROR_SUCCESS)
        *context = MSIINSTALLCONTEXT_MACHINE;
    else if (MSIREG_OpenProductKey(szProduct, NULL,
                                   MSIINSTALLCONTEXT_USERUNMANAGED,
                                   &hkey, FALSE) == ERROR_SUCCESS)
        *context = MSIINSTALLCONTEXT_USERUNMANAGED;

    RegCloseKey(hkey);

    if (*context == MSIINSTALLCONTEXT_NONE)
        return ERROR_UNKNOWN_PRODUCT;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiOpenProductA(LPCSTR szProduct, MSIHANDLE *phProduct)
{
    UINT r;
    LPWSTR szwProd = NULL;

    TRACE("%s %p\n",debugstr_a(szProduct), phProduct);

    if( szProduct )
    {
        szwProd = strdupAtoW( szProduct );
        if( !szwProd )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiOpenProductW( szwProd, phProduct );

    free( szwProd );

    return r;
}

static UINT MSI_OpenProductW(LPCWSTR szProduct, MSIPACKAGE **package)
{
    UINT r;
    HKEY props;
    LPWSTR path;
    MSIINSTALLCONTEXT context;

    TRACE("%s %p\n", debugstr_w(szProduct), package);

    r = msi_locate_product(szProduct, &context);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSIREG_OpenInstallProps(szProduct, context, NULL, &props, FALSE);
    if (r != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;

    if (context == MSIINSTALLCONTEXT_USERMANAGED)
        path = msi_reg_get_val_str(props, L"ManagedLocalPackage");
    else
        path = msi_reg_get_val_str(props, L"LocalPackage");

    r = ERROR_UNKNOWN_PRODUCT;

    if (!path || GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
        goto done;

    if (PathIsRelativeW(path))
    {
        r = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        goto done;
    }

    r = MSI_OpenPackageW(path, 0, package);

done:
    RegCloseKey(props);
    free(path);
    return r;
}

UINT WINAPI MsiOpenProductW(LPCWSTR szProduct, MSIHANDLE *phProduct)
{
    MSIPACKAGE *package = NULL;
    WCHAR squashed_pc[SQUASHED_GUID_SIZE];
    UINT r;

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (!phProduct)
        return ERROR_INVALID_PARAMETER;

    r = MSI_OpenProductW(szProduct, &package);
    if (r != ERROR_SUCCESS)
        return r;

    *phProduct = alloc_msihandle(&package->hdr);
    if (!*phProduct)
        r = ERROR_NOT_ENOUGH_MEMORY;

    msiobj_release(&package->hdr);
    return r;
}

UINT WINAPI MsiAdvertiseProductA(LPCSTR szPackagePath, LPCSTR szScriptfilePath,
                LPCSTR szTransforms, LANGID lgidLanguage)
{
    FIXME("%s %s %s %08x\n",debugstr_a(szPackagePath),
          debugstr_a(szScriptfilePath), debugstr_a(szTransforms), lgidLanguage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductW(LPCWSTR szPackagePath, LPCWSTR szScriptfilePath,
                LPCWSTR szTransforms, LANGID lgidLanguage)
{
    FIXME("%s %s %s %08x\n",debugstr_w(szPackagePath),
          debugstr_w(szScriptfilePath), debugstr_w(szTransforms), lgidLanguage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExA( const char *szPackagePath, const char *szScriptfilePath,
                                    const char *szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions )
{
    FIXME( "%s, %s, %s, %#x, %#lx, %#lx\n", debugstr_a(szPackagePath), debugstr_a(szScriptfilePath),
           debugstr_a(szTransforms), lgidLanguage, dwPlatform, dwOptions );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExW( const WCHAR *szPackagePath, const WCHAR *szScriptfilePath,
                                    const WCHAR *szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions )
{
    FIXME( "%s, %s, %s, %#x %#lx %#lx\n", debugstr_w(szPackagePath), debugstr_w(szScriptfilePath),
           debugstr_w(szTransforms), lgidLanguage, dwPlatform, dwOptions );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiInstallProductA(LPCSTR szPackagePath, LPCSTR szCommandLine)
{
    LPWSTR szwPath = NULL, szwCommand = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s\n",debugstr_a(szPackagePath), debugstr_a(szCommandLine));

    if( szPackagePath )
    {
        szwPath = strdupAtoW( szPackagePath );
        if( !szwPath )
            goto end;
    }

    if( szCommandLine )
    {
        szwCommand = strdupAtoW( szCommandLine );
        if( !szwCommand )
            goto end;
    }

    r = MsiInstallProductW( szwPath, szwCommand );

end:
    free( szwPath );
    free( szwCommand );

    return r;
}

UINT WINAPI MsiInstallProductW(LPCWSTR szPackagePath, LPCWSTR szCommandLine)
{
    MSIPACKAGE *package = NULL;
    const WCHAR *reinstallmode;
    DWORD options = 0;
    UINT r, len;

    TRACE("%s %s\n",debugstr_w(szPackagePath), debugstr_w(szCommandLine));

    if (!szPackagePath)
        return ERROR_INVALID_PARAMETER;

    if (!*szPackagePath)
        return ERROR_PATH_NOT_FOUND;

    reinstallmode = msi_get_command_line_option(szCommandLine, L"REINSTALLMODE", &len);
    if (reinstallmode)
    {
        while (len > 0)
        {
            if (reinstallmode[--len] == 'v' || reinstallmode[len] == 'V')
            {
                options |= WINE_OPENPACKAGEFLAGS_RECACHE;
                break;
            }
        }
    }

    r = MSI_OpenPackageW( szPackagePath, options, &package );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_InstallPackage( package, szPackagePath, szCommandLine );
        msiobj_release( &package->hdr );
    }

    return r;
}

UINT WINAPI MsiReinstallProductA( const char *szProduct, DWORD dwReinstallMode )
{
    WCHAR *wszProduct;
    UINT rc;

    TRACE( "%s, %#lx\n", debugstr_a(szProduct), dwReinstallMode );

    wszProduct = strdupAtoW(szProduct);
    rc = MsiReinstallProductW(wszProduct, dwReinstallMode);
    free(wszProduct);
    return rc;
}

UINT WINAPI MsiReinstallProductW( const WCHAR *szProduct, DWORD dwReinstallMode )
{
    TRACE( "%s, %#lx\n", debugstr_w(szProduct), dwReinstallMode );

    return MsiReinstallFeatureW(szProduct, L"ALL", dwReinstallMode);
}

UINT WINAPI MsiApplyPatchA(LPCSTR szPatchPackage, LPCSTR szInstallPackage,
        INSTALLTYPE eInstallType, LPCSTR szCommandLine)
{
    LPWSTR patch_package = NULL;
    LPWSTR install_package = NULL;
    LPWSTR command_line = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s %d %s\n", debugstr_a(szPatchPackage), debugstr_a(szInstallPackage),
          eInstallType, debugstr_a(szCommandLine));

    if (szPatchPackage && !(patch_package = strdupAtoW(szPatchPackage)))
        goto done;

    if (szInstallPackage && !(install_package = strdupAtoW(szInstallPackage)))
        goto done;

    if (szCommandLine && !(command_line = strdupAtoW(szCommandLine)))
        goto done;

    r = MsiApplyPatchW(patch_package, install_package, eInstallType, command_line);

done:
    free(patch_package);
    free(install_package);
    free(command_line);

    return r;
}

static UINT get_patch_product_codes( LPCWSTR szPatchPackage, WCHAR ***product_codes )
{
    MSIHANDLE patch, info = 0;
    UINT r, type;
    DWORD size;
    static WCHAR empty[] = L"";
    WCHAR *codes = NULL;

    r = MsiOpenDatabaseW( szPatchPackage, MSIDBOPEN_READONLY, &patch );
    if (r != ERROR_SUCCESS)
        return r;

    r = MsiGetSummaryInformationW( patch, NULL, 0, &info );
    if (r != ERROR_SUCCESS)
        goto done;

    size = 0;
    r = MsiSummaryInfoGetPropertyW( info, PID_TEMPLATE, &type, NULL, NULL, empty, &size );
    if (r != ERROR_MORE_DATA || !size || type != VT_LPSTR)
    {
        ERR("Failed to read product codes from patch\n");
        r = ERROR_FUNCTION_FAILED;
        goto done;
    }

    codes = malloc( ++size * sizeof(WCHAR) );
    if (!codes)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiSummaryInfoGetPropertyW( info, PID_TEMPLATE, &type, NULL, NULL, codes, &size );
    if (r == ERROR_SUCCESS)
        *product_codes = msi_split_string( codes, ';' );

done:
    MsiCloseHandle( info );
    MsiCloseHandle( patch );
    free( codes );
    return r;
}

static UINT MSI_ApplyPatchW(LPCWSTR szPatchPackage, LPCWSTR szProductCode, LPCWSTR szCommandLine)
{
    UINT i, r = ERROR_FUNCTION_FAILED;
    DWORD size;
    LPCWSTR cmd_ptr = szCommandLine;
    LPWSTR cmd, *codes = NULL;
    BOOL succeeded = FALSE;

    if (!szPatchPackage || !szPatchPackage[0])
        return ERROR_INVALID_PARAMETER;

    if (!szProductCode && (r = get_patch_product_codes( szPatchPackage, &codes )))
        return r;

    if (!szCommandLine)
        cmd_ptr = L"";

    size = lstrlenW(cmd_ptr) + lstrlenW(L"%s PATCH=\"%s\"") + lstrlenW(szPatchPackage) + 1;
    cmd = malloc(size * sizeof(WCHAR));
    if (!cmd)
    {
        free(codes);
        return ERROR_OUTOFMEMORY;
    }
    swprintf(cmd, size, L"%s PATCH=\"%s\"", cmd_ptr, szPatchPackage);

    if (szProductCode)
        r = MsiConfigureProductExW(szProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT, cmd);
    else
    {
        for (i = 0; codes[i]; i++)
        {
            r = MsiConfigureProductExW(codes[i], INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT, cmd);
            if (r == ERROR_SUCCESS)
            {
                TRACE("patch applied\n");
                succeeded = TRUE;
            }
        }

        if (succeeded)
            r = ERROR_SUCCESS;
    }

    free(cmd);
    free(codes);
    return r;
}

UINT WINAPI MsiApplyPatchW(LPCWSTR szPatchPackage, LPCWSTR szInstallPackage,
         INSTALLTYPE eInstallType, LPCWSTR szCommandLine)
{
    TRACE("%s %s %d %s\n", debugstr_w(szPatchPackage), debugstr_w(szInstallPackage),
          eInstallType, debugstr_w(szCommandLine));

    if (szInstallPackage || eInstallType == INSTALLTYPE_NETWORK_IMAGE ||
        eInstallType == INSTALLTYPE_SINGLE_INSTANCE)
    {
        FIXME("Only reading target products from patch\n");
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return MSI_ApplyPatchW(szPatchPackage, NULL, szCommandLine);
}

UINT WINAPI MsiApplyMultiplePatchesA(LPCSTR szPatchPackages,
        LPCSTR szProductCode, LPCSTR szPropertiesList)
{
    LPWSTR patch_packages = NULL;
    LPWSTR product_code = NULL;
    LPWSTR properties_list = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s %s\n", debugstr_a(szPatchPackages), debugstr_a(szProductCode),
          debugstr_a(szPropertiesList));

    if (!szPatchPackages || !szPatchPackages[0])
        return ERROR_INVALID_PARAMETER;

    if (!(patch_packages = strdupAtoW(szPatchPackages)))
        return ERROR_OUTOFMEMORY;

    if (szProductCode && !(product_code = strdupAtoW(szProductCode)))
        goto done;

    if (szPropertiesList && !(properties_list = strdupAtoW(szPropertiesList)))
        goto done;

    r = MsiApplyMultiplePatchesW(patch_packages, product_code, properties_list);

done:
    free(patch_packages);
    free(product_code);
    free(properties_list);

    return r;
}

UINT WINAPI MsiApplyMultiplePatchesW(LPCWSTR szPatchPackages,
        LPCWSTR szProductCode, LPCWSTR szPropertiesList)
{
    UINT r = ERROR_SUCCESS;
    LPCWSTR beg, end;

    TRACE("%s %s %s\n", debugstr_w(szPatchPackages), debugstr_w(szProductCode),
          debugstr_w(szPropertiesList));

    if (!szPatchPackages || !szPatchPackages[0])
        return ERROR_INVALID_PARAMETER;

    beg = end = szPatchPackages;
    while (*beg)
    {
        DWORD len;
        LPWSTR patch;

        while (*beg == ' ') beg++;
        while (*end && *end != ';') end++;

        len = end - beg;
        while (len && beg[len - 1] == ' ') len--;

        if (!len) return ERROR_INVALID_NAME;

        patch = malloc((len + 1) * sizeof(WCHAR));
        if (!patch)
            return ERROR_OUTOFMEMORY;

        memcpy(patch, beg, len * sizeof(WCHAR));
        patch[len] = '\0';

        r = MSI_ApplyPatchW(patch, szProductCode, szPropertiesList);
        free(patch);

        if (r != ERROR_SUCCESS || !*end)
            break;

        beg = ++end;
    }
    return r;
}

static void free_patchinfo( DWORD count, MSIPATCHSEQUENCEINFOW *info )
{
    DWORD i;
    for (i = 0; i < count; i++) free( (WCHAR *)info[i].szPatchData );
    free( info );
}

static MSIPATCHSEQUENCEINFOW *patchinfoAtoW( DWORD count, const MSIPATCHSEQUENCEINFOA *info )
{
    DWORD i;
    MSIPATCHSEQUENCEINFOW *ret;

    if (!(ret = malloc( count * sizeof(MSIPATCHSEQUENCEINFOW) ))) return NULL;
    for (i = 0; i < count; i++)
    {
        if (info[i].szPatchData && !(ret[i].szPatchData = strdupAtoW( info[i].szPatchData )))
        {
            free_patchinfo( i, ret );
            return NULL;
        }
        ret[i].ePatchDataType = info[i].ePatchDataType;
        ret[i].dwOrder = info[i].dwOrder;
        ret[i].uStatus = info[i].uStatus;
    }
    return ret;
}

UINT WINAPI MsiDetermineApplicablePatchesA( const char *szProductPackagePath, DWORD cPatchInfo,
                                            MSIPATCHSEQUENCEINFOA *pPatchInfo )
{
    UINT i, r;
    WCHAR *package_path = NULL;
    MSIPATCHSEQUENCEINFOW *psi;

    TRACE( "%s, %lu, %p\n", debugstr_a(szProductPackagePath), cPatchInfo, pPatchInfo );

    if (szProductPackagePath && !(package_path = strdupAtoW( szProductPackagePath )))
        return ERROR_OUTOFMEMORY;

    if (!(psi = patchinfoAtoW( cPatchInfo, pPatchInfo )))
    {
        free( package_path );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiDetermineApplicablePatchesW( package_path, cPatchInfo, psi );
    if (r == ERROR_SUCCESS)
    {
        for (i = 0; i < cPatchInfo; i++)
        {
            pPatchInfo[i].dwOrder = psi[i].dwOrder;
            pPatchInfo[i].uStatus = psi[i].uStatus;
        }
    }
    free( package_path );
    free_patchinfo( cPatchInfo, psi );
    return r;
}

static UINT MSI_ApplicablePatchW( MSIPACKAGE *package, LPCWSTR patch )
{
    MSISUMMARYINFO *si;
    MSIDATABASE *patch_db;
    UINT r;

    r = MSI_OpenDatabaseW( patch, MSIDBOPEN_READONLY, &patch_db );
    if (r != ERROR_SUCCESS)
    {
        WARN("failed to open patch file %s\n", debugstr_w(patch));
        return r;
    }

    r = msi_get_suminfo( patch_db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &patch_db->hdr );
        return ERROR_FUNCTION_FAILED;
    }

    r = msi_check_patch_applicable( package, si );
    if (r != ERROR_SUCCESS)
        TRACE("patch not applicable\n");

    msiobj_release( &patch_db->hdr );
    msiobj_release( &si->hdr );
    return r;
}

/* IXMLDOMDocument should be set to XPath mode already */
static UINT MSI_ApplicablePatchXML( MSIPACKAGE *package, IXMLDOMDocument *desc )
{
    UINT r = ERROR_FUNCTION_FAILED;
    IXMLDOMNodeList *list;
    LPWSTR product_code;
    IXMLDOMNode *node;
    HRESULT hr;
    BSTR s;

    product_code = msi_dup_property( package->db, L"ProductCode" );
    if (!product_code)
    {
        /* FIXME: the property ProductCode should be written into the DB somewhere */
        ERR("no product code to check\n");
        return ERROR_SUCCESS;
    }

    s = SysAllocString( L"MsiPatch/TargetProduct/TargetProductCode" );
    hr = IXMLDOMDocument_selectNodes( desc, s, &list );
    SysFreeString(s);
    if (hr != S_OK)
        return ERROR_INVALID_PATCH_XML;

    while (IXMLDOMNodeList_nextNode( list, &node ) == S_OK && r != ERROR_SUCCESS)
    {
        hr = IXMLDOMNode_get_text( node, &s );
        IXMLDOMNode_Release( node );
        if (hr == S_OK)
        {
            if (!wcscmp( s, product_code )) r = ERROR_SUCCESS;
            SysFreeString( s );
        }
    }
    IXMLDOMNodeList_Release( list );

    if (r != ERROR_SUCCESS)
        TRACE("patch not applicable\n");

    free( product_code );
    return r;
}

static UINT determine_patch_sequence( MSIPACKAGE *package, DWORD count, MSIPATCHSEQUENCEINFOW *info )
{
    IXMLDOMDocument *desc = NULL;
    DWORD i;

    if (count > 1)
        FIXME("patch ordering not supported\n");

    for (i = 0; i < count; i++)
    {
        switch (info[i].ePatchDataType)
        {
        case MSIPATCH_DATATYPE_PATCHFILE:
        {
            if (MSI_ApplicablePatchW( package, info[i].szPatchData ) != ERROR_SUCCESS)
            {
                info[i].dwOrder = ~0u;
                info[i].uStatus = ERROR_PATCH_TARGET_NOT_FOUND;
            }
            else
            {
                info[i].dwOrder = i;
                info[i].uStatus = ERROR_SUCCESS;
            }
            break;
        }
        case MSIPATCH_DATATYPE_XMLPATH:
        case MSIPATCH_DATATYPE_XMLBLOB:
        {
            VARIANT_BOOL b;
            HRESULT hr;
            BSTR s;

            if (!desc)
            {
                hr = CoCreateInstance( &CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IXMLDOMDocument, (void**)&desc );
                if (hr != S_OK)
                {
                    ERR( "failed to create DOMDocument30 instance, %#lx\n", hr );
                    return ERROR_FUNCTION_FAILED;
                }
            }

            s = SysAllocString( info[i].szPatchData );
            if (info[i].ePatchDataType == MSIPATCH_DATATYPE_XMLPATH)
            {
                VARIANT src;

                V_VT(&src) = VT_BSTR;
                V_BSTR(&src) = s;
                hr = IXMLDOMDocument_load( desc, src, &b );
            }
            else
                hr = IXMLDOMDocument_loadXML( desc, s, &b );
            SysFreeString( s );
            if ( hr != S_OK )
            {
                ERR("failed to parse patch description\n");
                IXMLDOMDocument_Release( desc );
                break;
            }

            if (MSI_ApplicablePatchXML( package, desc ) != ERROR_SUCCESS)
            {
                info[i].dwOrder = ~0u;
                info[i].uStatus = ERROR_PATCH_TARGET_NOT_FOUND;
            }
            else
            {
                info[i].dwOrder = i;
                info[i].uStatus = ERROR_SUCCESS;
            }
            break;
        }
        default:
        {
            FIXME("unknown patch data type %u\n", info[i].ePatchDataType);
            info[i].dwOrder = i;
            info[i].uStatus = ERROR_SUCCESS;
            break;
        }
        }

        TRACE("szPatchData: %s\n", debugstr_w(info[i].szPatchData));
        TRACE("ePatchDataType: %u\n", info[i].ePatchDataType);
        TRACE("dwOrder: %lu\n", info[i].dwOrder);
        TRACE("uStatus: %u\n", info[i].uStatus);
    }

    if (desc) IXMLDOMDocument_Release( desc );

    return ERROR_SUCCESS;
}

UINT WINAPI MsiDetermineApplicablePatchesW( const WCHAR *szProductPackagePath, DWORD cPatchInfo,
                                            MSIPATCHSEQUENCEINFOW *pPatchInfo )
{
    UINT r;
    MSIPACKAGE *package;

    TRACE( "%s, %lu, %p\n", debugstr_w(szProductPackagePath), cPatchInfo, pPatchInfo );

    r = MSI_OpenPackageW( szProductPackagePath, 0, &package );
    if (r != ERROR_SUCCESS)
    {
        ERR("failed to open package %u\n", r);
        return r;
    }
    r = determine_patch_sequence( package, cPatchInfo, pPatchInfo );
    msiobj_release( &package->hdr );
    return r;
}

UINT WINAPI MsiDeterminePatchSequenceA( const char *product, const char *usersid, MSIINSTALLCONTEXT context,
                                        DWORD count, MSIPATCHSEQUENCEINFOA *patchinfo )
{
    UINT i, r;
    WCHAR *productW, *usersidW = NULL;
    MSIPATCHSEQUENCEINFOW *patchinfoW;

    TRACE( "%s, %s, %d, %lu, %p\n", debugstr_a(product), debugstr_a(usersid), context, count, patchinfo );

    if (!product) return ERROR_INVALID_PARAMETER;
    if (!(productW = strdupAtoW( product ))) return ERROR_OUTOFMEMORY;
    if (usersid && !(usersidW = strdupAtoW( usersid )))
    {
        free( productW );
        return ERROR_OUTOFMEMORY;
    }
    if (!(patchinfoW = patchinfoAtoW( count, patchinfo )))
    {
        free( productW );
        free( usersidW );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiDeterminePatchSequenceW( productW, usersidW, context, count, patchinfoW );
    if (r == ERROR_SUCCESS)
    {
        for (i = 0; i < count; i++)
        {
            patchinfo[i].dwOrder = patchinfoW[i].dwOrder;
            patchinfo[i].uStatus = patchinfoW[i].uStatus;
        }
    }
    free( productW );
    free( usersidW );
    free_patchinfo( count, patchinfoW );
    return r;
}

static UINT open_package( const WCHAR *product, const WCHAR *usersid,
                          MSIINSTALLCONTEXT context, MSIPACKAGE **package )
{
    UINT r;
    HKEY props;
    WCHAR *localpath, sourcepath[MAX_PATH], filename[MAX_PATH];

    r = MSIREG_OpenInstallProps( product, context, usersid, &props, FALSE );
    if (r != ERROR_SUCCESS) return ERROR_BAD_CONFIGURATION;

    if ((localpath = msi_reg_get_val_str( props, L"LocalPackage" )))
    {
        lstrcpyW( sourcepath, localpath );
        free( localpath );
    }
    RegCloseKey( props );
    if (!localpath || GetFileAttributesW( sourcepath ) == INVALID_FILE_ATTRIBUTES)
    {
        DWORD sz = sizeof(sourcepath);
        MsiSourceListGetInfoW( product, usersid, context, MSICODE_PRODUCT,
                               INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz );
        sz = sizeof(filename);
        MsiSourceListGetInfoW( product, usersid, context, MSICODE_PRODUCT,
                               INSTALLPROPERTY_PACKAGENAMEW, filename, &sz );
        lstrcatW( sourcepath, filename );
    }
    if (GetFileAttributesW( sourcepath ) == INVALID_FILE_ATTRIBUTES)
        return ERROR_INSTALL_SOURCE_ABSENT;

    return MSI_OpenPackageW( sourcepath, 0, package );
}

UINT WINAPI MsiDeterminePatchSequenceW( const WCHAR *product, const WCHAR *usersid, MSIINSTALLCONTEXT context,
                                        DWORD count, MSIPATCHSEQUENCEINFOW *patchinfo )
{
    UINT r;
    MSIPACKAGE *package;

    TRACE( "%s, %s, %d, %lu, %p\n", debugstr_w(product), debugstr_w(usersid), context, count, patchinfo );

    if (!product) return ERROR_INVALID_PARAMETER;
    r = open_package( product, usersid, context, &package );
    if (r != ERROR_SUCCESS) return r;

    r = determine_patch_sequence( package, count, patchinfo );
    msiobj_release( &package->hdr );
    return r;
}

UINT WINAPI MsiConfigureProductExW(LPCWSTR szProduct, int iInstallLevel,
                        INSTALLSTATE eInstallState, LPCWSTR szCommandLine)
{
    MSIPACKAGE* package = NULL;
    MSIINSTALLCONTEXT context;
    UINT r;
    DWORD sz;
    WCHAR sourcepath[MAX_PATH], filename[MAX_PATH];
    LPWSTR commandline;

    TRACE("%s %d %d %s\n",debugstr_w(szProduct), iInstallLevel, eInstallState,
          debugstr_w(szCommandLine));

    if (!szProduct || lstrlenW(szProduct) != GUID_SIZE - 1)
        return ERROR_INVALID_PARAMETER;

    if (eInstallState == INSTALLSTATE_ADVERTISED ||
        eInstallState == INSTALLSTATE_SOURCE)
    {
        FIXME("State %d not implemented\n", eInstallState);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    r = msi_locate_product(szProduct, &context);
    if (r != ERROR_SUCCESS)
        return r;

    r = open_package(szProduct, NULL, context, &package);
    if (r != ERROR_SUCCESS)
        return r;

    sz = lstrlenW(L" Installed=1") + 1;

    if (szCommandLine)
        sz += lstrlenW(szCommandLine);

    if (eInstallState != INSTALLSTATE_DEFAULT)
        sz += lstrlenW(L" INSTALLLEVEL=32767");

    if (eInstallState == INSTALLSTATE_ABSENT)
        sz += lstrlenW(L" REMOVE=ALL");

    if (context == MSIINSTALLCONTEXT_MACHINE)
        sz += lstrlenW(L" ALLUSERS=1");

    commandline = malloc(sz * sizeof(WCHAR));
    if (!commandline)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    commandline[0] = 0;
    if (szCommandLine)
        lstrcpyW(commandline, szCommandLine);

    if (eInstallState != INSTALLSTATE_DEFAULT)
        lstrcatW(commandline, L" INSTALLLEVEL=32767");

    if (eInstallState == INSTALLSTATE_ABSENT)
        lstrcatW(commandline, L" REMOVE=ALL");

    if (context == MSIINSTALLCONTEXT_MACHINE)
        lstrcatW(commandline, L" ALLUSERS=1");

    sz = sizeof(sourcepath);
    MsiSourceListGetInfoW(szProduct, NULL, context, MSICODE_PRODUCT,
                          INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz);

    sz = sizeof(filename);
    MsiSourceListGetInfoW(szProduct, NULL, context, MSICODE_PRODUCT,
                          INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

    lstrcatW(sourcepath, filename);

    r = MSI_InstallPackage( package, sourcepath, commandline );

    free(commandline);

end:
    msiobj_release( &package->hdr );

    return r;
}

UINT WINAPI MsiConfigureProductExA(LPCSTR szProduct, int iInstallLevel,
                        INSTALLSTATE eInstallState, LPCSTR szCommandLine)
{
    LPWSTR szwProduct = NULL;
    LPWSTR szwCommandLine = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct )
            goto end;
    }

    if( szCommandLine)
    {
        szwCommandLine = strdupAtoW( szCommandLine );
        if( !szwCommandLine)
            goto end;
    }

    r = MsiConfigureProductExW( szwProduct, iInstallLevel, eInstallState,
                                szwCommandLine );
end:
    free( szwProduct );
    free( szwCommandLine);

    return r;
}

UINT WINAPI MsiConfigureProductA(LPCSTR szProduct, int iInstallLevel,
                                 INSTALLSTATE eInstallState)
{
    LPWSTR szwProduct = NULL;
    UINT r;

    TRACE("%s %d %d\n",debugstr_a(szProduct), iInstallLevel, eInstallState);

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiConfigureProductW( szwProduct, iInstallLevel, eInstallState );
    free( szwProduct );

    return r;
}

UINT WINAPI MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel,
                                 INSTALLSTATE eInstallState)
{
    return MsiConfigureProductExW(szProduct, iInstallLevel, eInstallState, NULL);
}

UINT WINAPI MsiGetProductCodeA(LPCSTR szComponent, LPSTR szBuffer)
{
    LPWSTR szwComponent = NULL;
    UINT r;
    WCHAR szwBuffer[GUID_SIZE];

    TRACE("%s %p\n", debugstr_a(szComponent), szBuffer);

    if( szComponent )
    {
        szwComponent = strdupAtoW( szComponent );
        if( !szwComponent )
            return ERROR_OUTOFMEMORY;
    }

    *szwBuffer = '\0';
    r = MsiGetProductCodeW( szwComponent, szwBuffer );

    if(*szwBuffer)
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, GUID_SIZE, NULL, NULL);

    free( szwComponent );

    return r;
}

UINT WINAPI MsiGetProductCodeW(LPCWSTR szComponent, LPWSTR szBuffer)
{
    UINT rc, index;
    HKEY compkey, prodkey;
    WCHAR squashed_comp[SQUASHED_GUID_SIZE], squashed_prod[SQUASHED_GUID_SIZE];
    DWORD sz = ARRAY_SIZE(squashed_prod);

    TRACE("%s %p\n", debugstr_w(szComponent), szBuffer);

    if (!szComponent || !*szComponent)
        return ERROR_INVALID_PARAMETER;

    if (!squash_guid( szComponent, squashed_comp ))
        return ERROR_INVALID_PARAMETER;

    if (MSIREG_OpenUserDataComponentKey(szComponent, NULL, &compkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenUserDataComponentKey(szComponent, L"S-1-5-18", &compkey, FALSE) != ERROR_SUCCESS)
    {
        return ERROR_UNKNOWN_COMPONENT;
    }

    rc = RegEnumValueW( compkey, 0, squashed_prod, &sz, NULL, NULL, NULL, NULL );
    if (rc != ERROR_SUCCESS)
    {
        RegCloseKey(compkey);
        return ERROR_UNKNOWN_COMPONENT;
    }

    /* check simple case, only one product */
    rc = RegEnumValueW( compkey, 1, squashed_prod, &sz, NULL, NULL, NULL, NULL );
    if (rc == ERROR_NO_MORE_ITEMS)
    {
        rc = ERROR_SUCCESS;
        goto done;
    }

    index = 0;
    while ((rc = RegEnumValueW( compkey, index, squashed_prod, &sz, NULL, NULL, NULL, NULL )) !=
           ERROR_NO_MORE_ITEMS)
    {
        index++;
        sz = GUID_SIZE;
        unsquash_guid( squashed_prod, szBuffer );

        if (MSIREG_OpenProductKey(szBuffer, NULL,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  &prodkey, FALSE) == ERROR_SUCCESS ||
            MSIREG_OpenProductKey(szBuffer, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  &prodkey, FALSE) == ERROR_SUCCESS ||
            MSIREG_OpenProductKey(szBuffer, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  &prodkey, FALSE) == ERROR_SUCCESS)
        {
            RegCloseKey(prodkey);
            rc = ERROR_SUCCESS;
            goto done;
        }
    }

    rc = ERROR_INSTALL_FAILURE;

done:
    RegCloseKey(compkey);
    unsquash_guid( squashed_prod, szBuffer );
    return rc;
}

static WCHAR *reg_get_value( HKEY hkey, const WCHAR *name, DWORD *type )
{
    LONG res;

    if ((res = RegQueryValueExW( hkey, name, NULL, type, NULL, NULL )) != ERROR_SUCCESS) return NULL;

    if (*type == REG_SZ) return msi_reg_get_val_str( hkey, name );
    if (*type == REG_DWORD)
    {
        WCHAR temp[11];
        DWORD val;

        if (!msi_reg_get_val_dword( hkey, name, &val )) return NULL;
        swprintf( temp, ARRAY_SIZE(temp), L"%u", val );
        return wcsdup( temp );
    }

    ERR( "unhandled value type %lu\n", *type );
    return NULL;
}

static UINT MSI_GetProductInfo(LPCWSTR szProduct, LPCWSTR szAttribute,
                               awstring *szValue, LPDWORD pcchValueBuf)
{
    static WCHAR empty[] = L"";
    MSIINSTALLCONTEXT context = MSIINSTALLCONTEXT_USERUNMANAGED;
    UINT r = ERROR_UNKNOWN_PROPERTY;
    HKEY prodkey, userdata, source;
    WCHAR *val = NULL, squashed_pc[SQUASHED_GUID_SIZE], packagecode[GUID_SIZE];
    BOOL badconfig = FALSE;
    LONG res;
    DWORD type = REG_NONE;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
          debugstr_w(szAttribute), szValue, pcchValueBuf);

    if ((szValue->str.w && !pcchValueBuf) || !szProduct || !szAttribute)
        return ERROR_INVALID_PARAMETER;

    if (!squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if ((r = MSIREG_OpenProductKey(szProduct, NULL,
                                   MSIINSTALLCONTEXT_USERMANAGED,
                                   &prodkey, FALSE)) != ERROR_SUCCESS &&
        (r = MSIREG_OpenProductKey(szProduct, NULL,
                                   MSIINSTALLCONTEXT_USERUNMANAGED,
                                   &prodkey, FALSE)) != ERROR_SUCCESS &&
        (r = MSIREG_OpenProductKey(szProduct, NULL,
                                   MSIINSTALLCONTEXT_MACHINE,
                                    &prodkey, FALSE)) == ERROR_SUCCESS)
    {
        context = MSIINSTALLCONTEXT_MACHINE;
    }

    if (!wcscmp( szAttribute, INSTALLPROPERTY_HELPLINKW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_HELPTELEPHONEW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_INSTALLDATEW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_INSTALLLOCATIONW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_INSTALLSOURCEW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_LOCALPACKAGEW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_PUBLISHERW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_URLINFOABOUTW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_URLUPDATEINFOW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_VERSIONMINORW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_VERSIONMAJORW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_VERSIONSTRINGW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_PRODUCTIDW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_REGCOMPANYW ) ||
        !wcscmp( szAttribute, INSTALLPROPERTY_REGOWNERW ))
    {
        if (!prodkey)
        {
            r = ERROR_UNKNOWN_PRODUCT;
            goto done;
        }
        if (MSIREG_OpenInstallProps(szProduct, context, NULL, &userdata, FALSE))
        {
            r = ERROR_UNKNOWN_PROPERTY;
            goto done;
        }

        if (!wcscmp( szAttribute, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW ))
            szAttribute = L"DisplayName";
        else if (!wcscmp( szAttribute, INSTALLPROPERTY_VERSIONSTRINGW ))
            szAttribute = L"DisplayVersion";

        val = reg_get_value(userdata, szAttribute, &type);
        if (!val)
            val = empty;
        RegCloseKey(userdata);
    }
    else if (!wcscmp( szAttribute, INSTALLPROPERTY_INSTANCETYPEW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_TRANSFORMSW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_LANGUAGEW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_PRODUCTNAMEW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_ASSIGNMENTTYPEW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_PACKAGECODEW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_VERSIONW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_PRODUCTICONW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_PACKAGENAMEW ) ||
             !wcscmp( szAttribute, INSTALLPROPERTY_AUTHORIZED_LUA_APPW ))
    {
        if (!prodkey)
        {
            r = ERROR_UNKNOWN_PRODUCT;
            goto done;
        }

        if (!wcscmp( szAttribute, INSTALLPROPERTY_ASSIGNMENTTYPEW ))
            szAttribute = L"Assignment";

        if (!wcscmp( szAttribute, INSTALLPROPERTY_PACKAGENAMEW ))
        {
            res = RegOpenKeyW(prodkey, L"SourceList", &source);
            if (res != ERROR_SUCCESS)
            {
                r = ERROR_UNKNOWN_PRODUCT;
                goto done;
            }

            val = reg_get_value(source, szAttribute, &type);
            if (!val)
                val = empty;

            RegCloseKey(source);
        }
        else
        {
            val = reg_get_value(prodkey, szAttribute, &type);
            if (!val)
                val = empty;
        }

        if (val != empty && type != REG_DWORD &&
            !wcscmp( szAttribute, INSTALLPROPERTY_PACKAGECODEW ))
        {
            if (lstrlenW( val ) != SQUASHED_GUID_SIZE - 1)
                badconfig = TRUE;
            else
            {
                unsquash_guid(val, packagecode);
                free(val);
                val = wcsdup(packagecode);
            }
        }
    }

    if (!val)
    {
        r = ERROR_UNKNOWN_PROPERTY;
        goto done;
    }

    if (pcchValueBuf)
    {
        int len = lstrlenW( val );

        /* If szBuffer (szValue->str) is NULL, there's no need to copy the value
         * out.  Also, *pcchValueBuf may be uninitialized in this case, so we
         * can't rely on its value.
         */
        if (szValue->str.a || szValue->str.w)
        {
            DWORD size = *pcchValueBuf;
            if (len < size)
                r = msi_strcpy_to_awstring( val, len, szValue, &size );
            else
                r = ERROR_MORE_DATA;
        }

        if (!badconfig)
            *pcchValueBuf = len;
    }

    if (badconfig)
        r = ERROR_BAD_CONFIGURATION;

    if (val != empty)
        free(val);

done:
    RegCloseKey(prodkey);
    return r;
}

UINT WINAPI MsiGetProductInfoA(LPCSTR szProduct, LPCSTR szAttribute,
                               LPSTR szBuffer, LPDWORD pcchValueBuf)
{
    LPWSTR szwProduct, szwAttribute = NULL;
    UINT r = ERROR_OUTOFMEMORY;
    awstring buffer;

    TRACE("%s %s %p %p\n", debugstr_a(szProduct), debugstr_a(szAttribute),
          szBuffer, pcchValueBuf);

    szwProduct = strdupAtoW( szProduct );
    if( szProduct && !szwProduct )
        goto end;

    szwAttribute = strdupAtoW( szAttribute );
    if( szAttribute && !szwAttribute )
        goto end;

    buffer.unicode = FALSE;
    buffer.str.a = szBuffer;

    r = MSI_GetProductInfo( szwProduct, szwAttribute,
                            &buffer, pcchValueBuf );

end:
    free( szwProduct );
    free( szwAttribute );

    return r;
}

UINT WINAPI MsiGetProductInfoW(LPCWSTR szProduct, LPCWSTR szAttribute,
                               LPWSTR szBuffer, LPDWORD pcchValueBuf)
{
    awstring buffer;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct), debugstr_w(szAttribute),
          szBuffer, pcchValueBuf);

    buffer.unicode = TRUE;
    buffer.str.w = szBuffer;

    return MSI_GetProductInfo( szProduct, szAttribute,
                               &buffer, pcchValueBuf );
}

UINT WINAPI MsiGetProductInfoExA(LPCSTR szProductCode, LPCSTR szUserSid,
                                 MSIINSTALLCONTEXT dwContext, LPCSTR szProperty,
                                 LPSTR szValue, LPDWORD pcchValue)
{
    LPWSTR product = NULL;
    LPWSTR usersid = NULL;
    LPWSTR property = NULL;
    LPWSTR value = NULL;
    DWORD len = 0;
    UINT r;

    TRACE("(%s, %s, %d, %s, %p, %p)\n", debugstr_a(szProductCode),
          debugstr_a(szUserSid), dwContext, debugstr_a(szProperty),
           szValue, pcchValue);

    if (szValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (szProductCode) product = strdupAtoW(szProductCode);
    if (szUserSid) usersid = strdupAtoW(szUserSid);
    if (szProperty) property = strdupAtoW(szProperty);

    r = MsiGetProductInfoExW(product, usersid, dwContext, property,
                             NULL, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    value = malloc(++len * sizeof(WCHAR));
    if (!value)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiGetProductInfoExW(product, usersid, dwContext, property,
                             value, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    if (!pcchValue)
        goto done;

    len = WideCharToMultiByte(CP_ACP, 0, value, -1, NULL, 0, NULL, NULL);
    if (*pcchValue >= len)
        WideCharToMultiByte(CP_ACP, 0, value, -1, szValue, len, NULL, NULL);
    else if (szValue)
    {
        r = ERROR_MORE_DATA;
        if (*pcchValue > 0)
            *szValue = '\0';
    }

    if (*pcchValue <= len || !szValue)
        len = len * sizeof(WCHAR) - 1;

    *pcchValue = len - 1;

done:
    free(product);
    free(usersid);
    free(property);
    free(value);

    return r;
}

static UINT copy_outval(const WCHAR *val, WCHAR *out, DWORD *size)
{
    UINT r = ERROR_SUCCESS;

    if (!val)
        return ERROR_UNKNOWN_PROPERTY;

    if (out)
    {
        if (lstrlenW(val) >= *size)
        {
            r = ERROR_MORE_DATA;
            if (*size > 0)
                *out = '\0';
        }
        else
            lstrcpyW(out, val);
    }

    if (size)
        *size = lstrlenW(val);

    return r;
}

UINT WINAPI MsiGetProductInfoExW(LPCWSTR szProductCode, LPCWSTR szUserSid,
                                 MSIINSTALLCONTEXT dwContext, LPCWSTR szProperty,
                                 LPWSTR szValue, LPDWORD pcchValue)
{
    WCHAR *val = NULL, squashed_pc[SQUASHED_GUID_SIZE];
    LPCWSTR package = NULL;
    HKEY props = NULL, prod, classes = NULL, managed, hkey = NULL;
    DWORD type;
    UINT r = ERROR_UNKNOWN_PRODUCT;

    TRACE("(%s, %s, %d, %s, %p, %p)\n", debugstr_w(szProductCode),
          debugstr_w(szUserSid), dwContext, debugstr_w(szProperty),
           szValue, pcchValue);

    if (!szProductCode || !squash_guid( szProductCode, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (szValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (dwContext != MSIINSTALLCONTEXT_USERUNMANAGED &&
        dwContext != MSIINSTALLCONTEXT_USERMANAGED &&
        dwContext != MSIINSTALLCONTEXT_MACHINE)
        return ERROR_INVALID_PARAMETER;

    if (!szProperty || !*szProperty)
        return ERROR_INVALID_PARAMETER;

    if (dwContext == MSIINSTALLCONTEXT_MACHINE && szUserSid)
        return ERROR_INVALID_PARAMETER;

    /* FIXME: dwContext is provided, no need to search for it */
    MSIREG_OpenProductKey(szProductCode, NULL,MSIINSTALLCONTEXT_USERMANAGED,
                          &managed, FALSE);
    MSIREG_OpenProductKey(szProductCode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                          &prod, FALSE);

    MSIREG_OpenInstallProps(szProductCode, dwContext, NULL, &props, FALSE);

    if (dwContext == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        package = INSTALLPROPERTY_LOCALPACKAGEW;

        if (!props && !prod)
            goto done;
    }
    else if (dwContext == MSIINSTALLCONTEXT_USERMANAGED)
    {
        package = L"ManagedLocalPackage";

        if (!props && !managed)
            goto done;
    }
    else if (dwContext == MSIINSTALLCONTEXT_MACHINE)
    {
        package = INSTALLPROPERTY_LOCALPACKAGEW;
        MSIREG_OpenProductKey(szProductCode, NULL, dwContext, &classes, FALSE);

        if (!props && !classes)
            goto done;
    }

    if (!wcscmp( szProperty, INSTALLPROPERTY_HELPLINKW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_HELPTELEPHONEW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_INSTALLDATEW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_INSTALLLOCATIONW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_INSTALLSOURCEW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_LOCALPACKAGEW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_PUBLISHERW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_URLINFOABOUTW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_URLUPDATEINFOW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_VERSIONMINORW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_VERSIONMAJORW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_VERSIONSTRINGW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_PRODUCTIDW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_REGCOMPANYW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_REGOWNERW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_INSTANCETYPEW ))
    {
        val = reg_get_value(props, package, &type);
        if (!val)
        {
            if (prod || classes)
                r = ERROR_UNKNOWN_PROPERTY;

            goto done;
        }

        free(val);

        if (!wcscmp( szProperty, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW ))
            szProperty = L"DisplayName";
        else if (!wcscmp( szProperty, INSTALLPROPERTY_VERSIONSTRINGW ))
            szProperty = L"DisplayVersion";

        val = reg_get_value(props, szProperty, &type);
        if (!val)
            val = wcsdup(L"");

        r = copy_outval(val, szValue, pcchValue);
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_TRANSFORMSW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_LANGUAGEW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_PRODUCTNAMEW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_PACKAGECODEW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_VERSIONW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_PRODUCTICONW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_PACKAGENAMEW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_AUTHORIZED_LUA_APPW ))
    {
        if (!prod && !classes)
            goto done;

        if (dwContext == MSIINSTALLCONTEXT_USERUNMANAGED)
            hkey = prod;
        else if (dwContext == MSIINSTALLCONTEXT_USERMANAGED)
            hkey = managed;
        else if (dwContext == MSIINSTALLCONTEXT_MACHINE)
            hkey = classes;

        val = reg_get_value(hkey, szProperty, &type);
        if (!val)
            val = wcsdup(L"");

        r = copy_outval(val, szValue, pcchValue);
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_PRODUCTSTATEW ))
    {
        if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        {
            if (props)
            {
                val = reg_get_value(props, package, &type);
                if (!val)
                    goto done;

                free(val);
                val = wcsdup(L"5");
            }
            else
                val = wcsdup(L"1");

            r = copy_outval(val, szValue, pcchValue);
            goto done;
        }
        else if (props && (val = reg_get_value(props, package, &type)))
        {
            free(val);
            val = wcsdup(L"5");
            r = copy_outval(val, szValue, pcchValue);
            goto done;
        }

        if (prod || managed)
            val = wcsdup(L"1");
        else
            goto done;

        r = copy_outval(val, szValue, pcchValue);
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_ASSIGNMENTTYPEW ))
    {
        if (!prod && !classes)
            goto done;

        /* FIXME */
        val = wcsdup(L"");
        r = copy_outval(val, szValue, pcchValue);
    }
    else
        r = ERROR_UNKNOWN_PROPERTY;

done:
    RegCloseKey(props);
    RegCloseKey(prod);
    RegCloseKey(managed);
    RegCloseKey(classes);
    free(val);

    return r;
}

UINT WINAPI MsiGetPatchFileListA(LPCSTR szProductCode, LPCSTR szPatchList,
                                 LPDWORD pcFiles, MSIHANDLE **pphFileRecords)
{
    FIXME("(%s, %s, %p, %p) stub!\n", debugstr_a(szProductCode),
          debugstr_a(szPatchList), pcFiles, pphFileRecords);
    return ERROR_FUNCTION_FAILED;
}

UINT WINAPI MsiGetPatchFileListW(LPCWSTR szProductCode, LPCWSTR szPatchList,
                                 LPDWORD pcFiles, MSIHANDLE **pphFileRecords)
{
    FIXME("(%s, %s, %p, %p) stub!\n", debugstr_w(szProductCode),
          debugstr_w(szPatchList), pcFiles, pphFileRecords);
    return ERROR_FUNCTION_FAILED;
}

UINT WINAPI MsiGetPatchInfoExA(LPCSTR szPatchCode, LPCSTR szProductCode,
                               LPCSTR szUserSid, MSIINSTALLCONTEXT dwContext,
                               LPCSTR szProperty, LPSTR lpValue, DWORD *pcchValue)
{
    LPWSTR patch = NULL, product = NULL, usersid = NULL;
    LPWSTR property = NULL, val = NULL;
    DWORD len;
    UINT r;

    TRACE("(%s, %s, %s, %d, %s, %p, %p)\n", debugstr_a(szPatchCode),
          debugstr_a(szProductCode), debugstr_a(szUserSid), dwContext,
          debugstr_a(szProperty), lpValue, pcchValue);

    if (lpValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (szPatchCode) patch = strdupAtoW(szPatchCode);
    if (szProductCode) product = strdupAtoW(szProductCode);
    if (szUserSid) usersid = strdupAtoW(szUserSid);
    if (szProperty) property = strdupAtoW(szProperty);

    len = 0;
    r = MsiGetPatchInfoExW(patch, product, usersid, dwContext, property,
                           NULL, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    val = malloc(++len * sizeof(WCHAR));
    if (!val)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiGetPatchInfoExW(patch, product, usersid, dwContext, property,
                           val, &len);
    if (r != ERROR_SUCCESS || !pcchValue)
        goto done;

    if (lpValue)
        WideCharToMultiByte(CP_ACP, 0, val, -1, lpValue,
                            *pcchValue - 1, NULL, NULL);

    len = lstrlenW(val);
    if ((*val && *pcchValue < len + 1) || !lpValue)
    {
        if (lpValue)
        {
            r = ERROR_MORE_DATA;
            lpValue[*pcchValue - 1] = '\0';
        }

        *pcchValue = len * sizeof(WCHAR);
    }
    else
        *pcchValue = len;

done:
    free(val);
    free(patch);
    free(product);
    free(usersid);
    free(property);

    return r;
}

UINT WINAPI MsiGetPatchInfoExW(LPCWSTR szPatchCode, LPCWSTR szProductCode,
                               LPCWSTR szUserSid, MSIINSTALLCONTEXT dwContext,
                               LPCWSTR szProperty, LPWSTR lpValue, DWORD *pcchValue)
{
    WCHAR *val = NULL, squashed_pc[SQUASHED_GUID_SIZE], squashed_patch[SQUASHED_GUID_SIZE];
    HKEY udprod = 0, prod = 0, props = 0;
    HKEY patch = 0, patches = 0;
    HKEY udpatch = 0, datakey = 0;
    HKEY prodpatches = 0;
    UINT r = ERROR_UNKNOWN_PRODUCT;
    DWORD len, type;
    LONG res;

    TRACE("(%s, %s, %s, %d, %s, %p, %p)\n", debugstr_w(szPatchCode),
          debugstr_w(szProductCode), debugstr_w(szUserSid), dwContext,
          debugstr_w(szProperty), lpValue, pcchValue);

    if (!szProductCode || !squash_guid( szProductCode, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (!szPatchCode || !squash_guid( szPatchCode, squashed_patch ))
        return ERROR_INVALID_PARAMETER;

    if (!szProperty)
        return ERROR_INVALID_PARAMETER;

    if (lpValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (dwContext != MSIINSTALLCONTEXT_USERMANAGED &&
        dwContext != MSIINSTALLCONTEXT_USERUNMANAGED &&
        dwContext != MSIINSTALLCONTEXT_MACHINE)
        return ERROR_INVALID_PARAMETER;

    if (dwContext == MSIINSTALLCONTEXT_MACHINE && szUserSid)
        return ERROR_INVALID_PARAMETER;

    if (szUserSid && !wcscmp( szUserSid, L"S-1-5-18" ))
        return ERROR_INVALID_PARAMETER;

    if (MSIREG_OpenUserDataProductKey(szProductCode, dwContext, NULL,
                                      &udprod, FALSE) != ERROR_SUCCESS)
        goto done;

    if (MSIREG_OpenInstallProps(szProductCode, dwContext, NULL,
                                &props, FALSE) != ERROR_SUCCESS)
        goto done;

    r = ERROR_UNKNOWN_PATCH;

    res = RegOpenKeyExW(udprod, L"Patches", 0, KEY_READ, &patches);
    if (res != ERROR_SUCCESS)
        goto done;

    res = RegOpenKeyExW( patches, squashed_patch, 0, KEY_READ, &patch );
    if (res != ERROR_SUCCESS)
        goto done;

    if (!wcscmp( szProperty, INSTALLPROPERTY_TRANSFORMSW ))
    {
        if (MSIREG_OpenProductKey(szProductCode, NULL, dwContext,
                                  &prod, FALSE) != ERROR_SUCCESS)
            goto done;

        res = RegOpenKeyExW(prod, L"Patches", 0, KEY_ALL_ACCESS, &prodpatches);
        if (res != ERROR_SUCCESS)
            goto done;

        datakey = prodpatches;
        szProperty = squashed_patch;
    }
    else
    {
        if (MSIREG_OpenUserDataPatchKey(szPatchCode, dwContext,
                                        &udpatch, FALSE) != ERROR_SUCCESS)
            goto done;

        if (!wcscmp( szProperty, INSTALLPROPERTY_LOCALPACKAGEW ))
        {
            if (dwContext == MSIINSTALLCONTEXT_USERMANAGED)
                szProperty = L"ManagedLocalPackage";
            datakey = udpatch;
        }
        else if (!wcscmp( szProperty, INSTALLPROPERTY_INSTALLDATEW ))
        {
            datakey = patch;
            szProperty = L"Installed";
        }
        else if (!wcscmp( szProperty, INSTALLPROPERTY_UNINSTALLABLEW ) ||
                 !wcscmp( szProperty, INSTALLPROPERTY_PATCHSTATEW ) ||
                 !wcscmp( szProperty, INSTALLPROPERTY_DISPLAYNAMEW ) ||
                 !wcscmp( szProperty, INSTALLPROPERTY_MOREINFOURLW ))
        {
            datakey = patch;
        }
        else
        {
            r = ERROR_UNKNOWN_PROPERTY;
            goto done;
        }
    }

    val = reg_get_value(datakey, szProperty, &type);
    if (!val)
        val = wcsdup(L"");

    r = ERROR_SUCCESS;

    if (!pcchValue)
        goto done;

    if (lpValue)
        lstrcpynW(lpValue, val, *pcchValue);

    len = lstrlenW(val);
    if ((*val && *pcchValue < len + 1) || !lpValue)
    {
        if (lpValue)
            r = ERROR_MORE_DATA;

        *pcchValue = len * sizeof(WCHAR);
    }

    *pcchValue = len;

done:
    free(val);
    RegCloseKey(prodpatches);
    RegCloseKey(prod);
    RegCloseKey(patch);
    RegCloseKey(patches);
    RegCloseKey(udpatch);
    RegCloseKey(props);
    RegCloseKey(udprod);

    return r;
}

UINT WINAPI MsiGetPatchInfoA( LPCSTR patch, LPCSTR attr, LPSTR buffer, LPDWORD buflen )
{
    UINT r = ERROR_OUTOFMEMORY;
    DWORD size;
    LPWSTR patchW = NULL, attrW = NULL, bufferW = NULL;

    TRACE("%s %s %p %p\n", debugstr_a(patch), debugstr_a(attr), buffer, buflen);

    if (!patch || !attr)
        return ERROR_INVALID_PARAMETER;

    if (!(patchW = strdupAtoW( patch )))
        goto done;

    if (!(attrW = strdupAtoW( attr )))
        goto done;

    size = 0;
    r = MsiGetPatchInfoW( patchW, attrW, NULL, &size );
    if (r != ERROR_SUCCESS)
        goto done;

    size++;
    if (!(bufferW = malloc( size * sizeof(WCHAR) )))
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiGetPatchInfoW( patchW, attrW, bufferW, &size );
    if (r == ERROR_SUCCESS)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
        if (len > *buflen)
            r = ERROR_MORE_DATA;
        else if (buffer)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, *buflen, NULL, NULL );

        *buflen = len - 1;
    }

done:
    free( patchW );
    free( attrW );
    free( bufferW );
    return r;
}

UINT WINAPI MsiGetPatchInfoW( LPCWSTR patch, LPCWSTR attr, LPWSTR buffer, LPDWORD buflen )
{
    UINT r;
    WCHAR product[GUID_SIZE];
    DWORD index;

    TRACE("%s %s %p %p\n", debugstr_w(patch), debugstr_w(attr), buffer, buflen);

    if (!patch || !attr)
        return ERROR_INVALID_PARAMETER;

    if (wcscmp( INSTALLPROPERTY_LOCALPACKAGEW, attr ))
        return ERROR_UNKNOWN_PROPERTY;

    index = 0;
    while (1)
    {
        r = MsiEnumProductsW( index, product );
        if (r != ERROR_SUCCESS)
            break;

        r = MsiGetPatchInfoExW( patch, product, NULL, MSIINSTALLCONTEXT_USERMANAGED, attr, buffer, buflen );
        if (r == ERROR_SUCCESS || r == ERROR_MORE_DATA)
            return r;

        r = MsiGetPatchInfoExW( patch, product, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, attr, buffer, buflen );
        if (r == ERROR_SUCCESS || r == ERROR_MORE_DATA)
            return r;

        r = MsiGetPatchInfoExW( patch, product, NULL, MSIINSTALLCONTEXT_MACHINE, attr, buffer, buflen );
        if (r == ERROR_SUCCESS || r == ERROR_MORE_DATA)
            return r;

        index++;
    }

    return ERROR_UNKNOWN_PRODUCT;
}

UINT WINAPI MsiEnableLogA( DWORD dwLogMode, const char *szLogFile, DWORD attributes )
{
    LPWSTR szwLogFile = NULL;
    UINT r;

    TRACE( "%#lx, %s, %#lx\n", dwLogMode, debugstr_a(szLogFile), attributes );

    if( szLogFile )
    {
        szwLogFile = strdupAtoW( szLogFile );
        if( !szwLogFile )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiEnableLogW( dwLogMode, szwLogFile, attributes );
    free( szwLogFile );
    return r;
}

UINT WINAPI MsiEnableLogW( DWORD dwLogMode, const WCHAR *szLogFile, DWORD attributes )
{
    TRACE( "%#lx, %s, %#lx\n", dwLogMode, debugstr_w(szLogFile), attributes );

    free(gszLogFile);
    gszLogFile = NULL;
    if (szLogFile)
    {
        HANDLE file;

        if (!(attributes & INSTALLLOGATTRIBUTES_APPEND))
            DeleteFileW(szLogFile);
        file = CreateFileW(szLogFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE)
        {
            gszLogFile = wcsdup(szLogFile);
            CloseHandle(file);
        }
        else ERR( "unable to enable log %s (%lu)\n", debugstr_w(szLogFile), GetLastError() );
    }

    return ERROR_SUCCESS;
}

UINT WINAPI MsiEnumComponentCostsA( MSIHANDLE handle, const char *component, DWORD index, INSTALLSTATE state,
                                    char *drive, DWORD *buflen, int *cost, int *temp )
{
    UINT r;
    DWORD len;
    WCHAR *driveW, *componentW = NULL;

    TRACE( "%lu, %s, %lu, %d, %p, %p, %p, %p\n", handle, debugstr_a(component), index, state, drive, buflen, cost,
           temp );

    if (!drive || !buflen) return ERROR_INVALID_PARAMETER;
    if (component && !(componentW = strdupAtoW( component ))) return ERROR_OUTOFMEMORY;

    len = *buflen;
    if (!(driveW = malloc( len * sizeof(WCHAR) )))
    {
        free( componentW );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiEnumComponentCostsW( handle, componentW, index, state, driveW, buflen, cost, temp );
    if (!r)
    {
        WideCharToMultiByte( CP_ACP, 0, driveW, -1, drive, len, NULL, NULL );
    }
    free( componentW );
    free( driveW );
    return r;
}

static UINT set_drive( WCHAR *buffer, WCHAR letter )
{
    buffer[0] = letter;
    buffer[1] = ':';
    buffer[2] = 0;
    return 2;
}

UINT WINAPI MsiEnumComponentCostsW( MSIHANDLE handle, const WCHAR *component, DWORD index, INSTALLSTATE state,
                                    WCHAR *drive, DWORD *buflen, int *cost, int *temp )
{
    UINT r = ERROR_NO_MORE_ITEMS;
    MSICOMPONENT *comp = NULL;
    MSIPACKAGE *package;
    MSIFILE *file;
    STATSTG stat = {0};
    WCHAR path[MAX_PATH];

    TRACE( "%lu, %s, %lu, %d, %p, %p, %p, %p\n", handle, debugstr_w(component), index, state, drive, buflen, cost,
           temp );

    if (!drive || !buflen || !cost || !temp) return ERROR_INVALID_PARAMETER;
    if (!(package = msihandle2msiinfo( handle, MSIHANDLETYPE_PACKAGE )))
    {
        WCHAR buffer[3];
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(handle)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            r = remote_EnumComponentCosts(remote, component, index, state, buffer, cost, temp);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY

        if (r == ERROR_SUCCESS)
        {
            lstrcpynW(drive, buffer, *buflen);
            if (*buflen < 3)
                r = ERROR_MORE_DATA;
            *buflen = 2;
        }
        return r;
    }

    if (!msi_get_property_int( package->db, L"CostingComplete", 0 ))
    {
        msiobj_release( &package->hdr );
        return ERROR_FUNCTION_NOT_CALLED;
    }
    if (component && component[0] && !(comp = msi_get_loaded_component( package, component )))
    {
        msiobj_release( &package->hdr );
        return ERROR_UNKNOWN_COMPONENT;
    }
    if (*buflen < 3)
    {
        *buflen = 2;
        msiobj_release( &package->hdr );
        return ERROR_MORE_DATA;
    }
    if (index)
    {
        msiobj_release( &package->hdr );
        return ERROR_NO_MORE_ITEMS;
    }

    drive[0] = 0;
    *cost = *temp = 0;
    GetWindowsDirectoryW( path, MAX_PATH );
    if (component && component[0])
    {
        if (msi_is_global_assembly( comp )) *temp = comp->cost;
        if (!comp->Enabled || !comp->KeyPath)
        {
            *cost = 0;
            *buflen = set_drive( drive, path[0] );
            r = ERROR_SUCCESS;
        }
        else if ((file = msi_get_loaded_file( package, comp->KeyPath )))
        {
            *cost = comp->cost;
            *buflen = set_drive( drive, file->TargetPath[0] );
            r = ERROR_SUCCESS;
        }
    }
    else if (IStorage_Stat( package->db->storage, &stat, STATFLAG_NONAME ) == S_OK)
    {
        *temp = cost_from_size( stat.cbSize.QuadPart );
        *buflen = set_drive( drive, path[0] );
        r = ERROR_SUCCESS;
    }
    msiobj_release( &package->hdr );
    return r;
}

UINT WINAPI MsiQueryComponentStateA(LPCSTR szProductCode,
                                    LPCSTR szUserSid, MSIINSTALLCONTEXT dwContext,
                                    LPCSTR szComponent, INSTALLSTATE *pdwState)
{
    LPWSTR prodcode = NULL, usersid = NULL, comp = NULL;
    UINT r;

    TRACE("(%s, %s, %d, %s, %p)\n", debugstr_a(szProductCode),
          debugstr_a(szUserSid), dwContext, debugstr_a(szComponent), pdwState);

    if (szProductCode && !(prodcode = strdupAtoW(szProductCode)))
        return ERROR_OUTOFMEMORY;

    if (szUserSid && !(usersid = strdupAtoW(szUserSid)))
            return ERROR_OUTOFMEMORY;

    if (szComponent && !(comp = strdupAtoW(szComponent)))
            return ERROR_OUTOFMEMORY;

    r = MsiQueryComponentStateW(prodcode, usersid, dwContext, comp, pdwState);

    free(prodcode);
    free(usersid);
    free(comp);

    return r;
}

static BOOL comp_find_prod_key(const WCHAR *prodcode, MSIINSTALLCONTEXT context)
{
    UINT r;
    HKEY hkey = NULL;

    r = MSIREG_OpenProductKey(prodcode, NULL, context, &hkey, FALSE);
    RegCloseKey(hkey);
    return (r == ERROR_SUCCESS);
}

static BOOL comp_find_package(const WCHAR *prodcode, MSIINSTALLCONTEXT context)
{
    LPCWSTR package;
    HKEY hkey;
    DWORD sz;
    LONG res;
    UINT r;

    r = MSIREG_OpenInstallProps(prodcode, context, NULL, &hkey, FALSE);
    if (r != ERROR_SUCCESS)
        return FALSE;

    if (context == MSIINSTALLCONTEXT_USERMANAGED)
        package = L"ManagedLocalPackage";
    else
        package = L"LocalPackage";

    sz = 0;
    res = RegQueryValueExW(hkey, package, NULL, NULL, NULL, &sz);
    RegCloseKey(hkey);

    return (res == ERROR_SUCCESS);
}

static UINT comp_find_prodcode(const WCHAR *squashed_pc, MSIINSTALLCONTEXT context, const WCHAR *comp, WCHAR *val,
                               DWORD *sz)
{
    HKEY hkey;
    LONG res;
    UINT r;

    if (context == MSIINSTALLCONTEXT_MACHINE)
        r = MSIREG_OpenUserDataComponentKey(comp, L"S-1-5-18", &hkey, FALSE);
    else
        r = MSIREG_OpenUserDataComponentKey(comp, NULL, &hkey, FALSE);

    if (r != ERROR_SUCCESS)
        return r;

    res = RegQueryValueExW( hkey, squashed_pc, NULL, NULL, (BYTE *)val, sz );
    if (res != ERROR_SUCCESS)
        return res;

    RegCloseKey(hkey);
    return res;
}

UINT WINAPI MsiQueryComponentStateW(LPCWSTR szProductCode,
                                    LPCWSTR szUserSid, MSIINSTALLCONTEXT dwContext,
                                    LPCWSTR szComponent, INSTALLSTATE *pdwState)
{
    WCHAR squashed_pc[SQUASHED_GUID_SIZE];
    BOOL found;
    DWORD sz;

    TRACE("(%s, %s, %d, %s, %p)\n", debugstr_w(szProductCode),
          debugstr_w(szUserSid), dwContext, debugstr_w(szComponent), pdwState);

    if (!pdwState || !szComponent)
        return ERROR_INVALID_PARAMETER;

    if (!szProductCode || !*szProductCode || lstrlenW(szProductCode) != GUID_SIZE - 1)
        return ERROR_INVALID_PARAMETER;

    if (!squash_guid( szProductCode, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    found = comp_find_prod_key(szProductCode, dwContext);

    if (!comp_find_package(szProductCode, dwContext))
    {
        if (found)
        {
            *pdwState = INSTALLSTATE_UNKNOWN;
            return ERROR_UNKNOWN_COMPONENT;
        }

        return ERROR_UNKNOWN_PRODUCT;
    }

    *pdwState = INSTALLSTATE_UNKNOWN;

    sz = 0;
    if (comp_find_prodcode( squashed_pc, dwContext, szComponent, NULL, &sz ))
        return ERROR_UNKNOWN_COMPONENT;

    if (sz == 0)
        *pdwState = INSTALLSTATE_NOTUSED;
    else
    {
        WCHAR *val;
        UINT r;

        if (!(val = malloc( sz ))) return ERROR_OUTOFMEMORY;
        if ((r = comp_find_prodcode( squashed_pc, dwContext, szComponent, val, &sz )))
        {
            free(val);
            return r;
        }

        if (lstrlenW(val) > 2 &&
            val[0] >= '0' && val[0] <= '9' && val[1] >= '0' && val[1] <= '9' && val[2] != ':')
        {
            *pdwState = INSTALLSTATE_SOURCE;
        }
        else
            *pdwState = INSTALLSTATE_LOCAL;
        free( val );
    }

    TRACE("-> %d\n", *pdwState);
    return ERROR_SUCCESS;
}

INSTALLSTATE WINAPI MsiQueryProductStateA(LPCSTR szProduct)
{
    LPWSTR szwProduct = NULL;
    INSTALLSTATE r;

    if( szProduct )
    {
         szwProduct = strdupAtoW( szProduct );
         if( !szwProduct )
             return ERROR_OUTOFMEMORY;
    }
    r = MsiQueryProductStateW( szwProduct );
    free( szwProduct );
    return r;
}

INSTALLSTATE WINAPI MsiQueryProductStateW(LPCWSTR szProduct)
{
    MSIINSTALLCONTEXT context = MSIINSTALLCONTEXT_USERUNMANAGED;
    INSTALLSTATE state = INSTALLSTATE_ADVERTISED;
    HKEY prodkey = 0, userdata = 0;
    DWORD val;
    UINT r;

    TRACE("%s\n", debugstr_w(szProduct));

    if (!szProduct || !*szProduct)
        return INSTALLSTATE_INVALIDARG;

    if (lstrlenW(szProduct) != GUID_SIZE - 1)
        return INSTALLSTATE_INVALIDARG;

    if (szProduct[0] != '{' || szProduct[37] != '}')
        return INSTALLSTATE_UNKNOWN;

    SetLastError( ERROR_SUCCESS );

    if (MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                              &prodkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              &prodkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_MACHINE,
                              &prodkey, FALSE) == ERROR_SUCCESS)
    {
        context = MSIINSTALLCONTEXT_MACHINE;
    }

    r = MSIREG_OpenInstallProps(szProduct, context, NULL, &userdata, FALSE);
    if (r != ERROR_SUCCESS)
        goto done;

    if (!msi_reg_get_val_dword(userdata, L"WindowsInstaller", &val))
        goto done;

    if (val)
        state = INSTALLSTATE_DEFAULT;
    else
        state = INSTALLSTATE_UNKNOWN;

done:
    if (!prodkey)
    {
        state = INSTALLSTATE_UNKNOWN;

        if (userdata)
            state = INSTALLSTATE_ABSENT;
    }

    RegCloseKey(prodkey);
    RegCloseKey(userdata);
    TRACE("-> %d\n", state);
    return state;
}

INSTALLUILEVEL WINAPI MsiSetInternalUI(INSTALLUILEVEL dwUILevel, HWND *phWnd)
{
    INSTALLUILEVEL old = gUILevel;
    HWND oldwnd = gUIhwnd;

    TRACE("%08x %p\n", dwUILevel, phWnd);

    if (dwUILevel & ~(INSTALLUILEVEL_MASK|INSTALLUILEVEL_HIDECANCEL|INSTALLUILEVEL_PROGRESSONLY|
                      INSTALLUILEVEL_ENDDIALOG|INSTALLUILEVEL_SOURCERESONLY))
    {
        FIXME("Unrecognized flags %08x\n", dwUILevel);
        return INSTALLUILEVEL_NOCHANGE;
    }

    if (dwUILevel != INSTALLUILEVEL_NOCHANGE)
        gUILevel = dwUILevel;

    if (phWnd)
    {
        gUIhwnd = *phWnd;
        *phWnd = oldwnd;
    }
    return old;
}

INSTALLUI_HANDLERA WINAPI MsiSetExternalUIA( INSTALLUI_HANDLERA puiHandler, DWORD dwMessageFilter, void *pvContext )
{
    INSTALLUI_HANDLERA prev = gUIHandlerA;

    TRACE( "%p, %#lx, %p\n", puiHandler, dwMessageFilter, pvContext );

    gUIHandlerA = puiHandler;
    gUIHandlerW = NULL;
    gUIFilter   = dwMessageFilter;
    gUIContext  = pvContext;

    return prev;
}

INSTALLUI_HANDLERW WINAPI MsiSetExternalUIW( INSTALLUI_HANDLERW puiHandler, DWORD dwMessageFilter, void *pvContext )
{
    INSTALLUI_HANDLERW prev = gUIHandlerW;

    TRACE( "%p, %#lx, %p\n", puiHandler, dwMessageFilter, pvContext );

    gUIHandlerA = NULL;
    gUIHandlerW = puiHandler;
    gUIFilter   = dwMessageFilter;
    gUIContext  = pvContext;

    return prev;
}

/******************************************************************
 *  MsiLoadStringW            [MSI.@]
 *
 * Loads a string from MSI's string resources.
 *
 * PARAMS
 *
 *   handle        [I]  only -1 is handled currently
 *   id            [I]  id of the string to be loaded
 *   lpBuffer      [O]  buffer for the string to be written to
 *   nBufferMax    [I]  maximum size of the buffer in characters
 *   lang          [I]  the preferred language for the string
 *
 * RETURNS
 *
 *   If successful, this function returns the language id of the string loaded
 *   If the function fails, the function returns zero.
 *
 * NOTES
 *
 *   The type of the first parameter is unknown.  LoadString's prototype
 *  suggests that it might be a module handle.  I have made it an MSI handle
 *  for starters, as -1 is an invalid MSI handle, but not an invalid module
 *  handle.  Maybe strings can be stored in an MSI database somehow.
 */
LANGID WINAPI MsiLoadStringW( MSIHANDLE handle, UINT id, WCHAR *lpBuffer, int nBufferMax, LANGID lang )
{
    HRSRC hres;
    HGLOBAL hResData;
    LPWSTR p;
    DWORD i, len;

    TRACE( "%lu, %u, %p, %d, %#x\n", handle, id, lpBuffer, nBufferMax, lang );

    if( handle != -1 )
        FIXME( "don't know how to deal with handle = %lu\n", handle );

    if( !lang )
        lang = GetUserDefaultLangID();

    hres = FindResourceExW( msi_hInstance, (const WCHAR *)RT_STRING, (WCHAR *)1, lang );
    if( !hres )
        return 0;
    hResData = LoadResource( msi_hInstance, hres );
    if( !hResData )
        return 0;
    p = LockResource( hResData );
    if( !p )
        return 0;

    for (i = 0; i < (id & 0xf); i++) p += *p + 1;
    len = *p;

    if( nBufferMax <= len )
        return 0;

    memcpy( lpBuffer, p+1, len * sizeof(WCHAR));
    lpBuffer[ len ] = 0;

    TRACE("found -> %s\n", debugstr_w(lpBuffer));
    return lang;
}

LANGID WINAPI MsiLoadStringA( MSIHANDLE handle, UINT id, LPSTR lpBuffer,
                int nBufferMax, LANGID lang )
{
    LPWSTR bufW;
    LANGID r;
    INT len;

    bufW = malloc(nBufferMax * sizeof(WCHAR));
    r = MsiLoadStringW(handle, id, bufW, nBufferMax, lang);
    if( r )
    {
        len = WideCharToMultiByte(CP_ACP, 0, bufW, -1, NULL, 0, NULL, NULL );
        if( len <= nBufferMax )
            WideCharToMultiByte( CP_ACP, 0, bufW, -1,
                                 lpBuffer, nBufferMax, NULL, NULL );
        else
            r = 0;
    }
    free(bufW);
    return r;
}

INSTALLSTATE WINAPI MsiLocateComponentA(LPCSTR szComponent, LPSTR lpPathBuf,
                LPDWORD pcchBuf)
{
    char szProduct[GUID_SIZE];

    TRACE("%s %p %p\n", debugstr_a(szComponent), lpPathBuf, pcchBuf);

    if (!szComponent || !pcchBuf)
        return INSTALLSTATE_INVALIDARG;

    if (MsiGetProductCodeA( szComponent, szProduct ) != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    return MsiGetComponentPathA( szProduct, szComponent, lpPathBuf, pcchBuf );
}

INSTALLSTATE WINAPI MsiLocateComponentW(LPCWSTR szComponent, LPWSTR lpPathBuf,
                LPDWORD pcchBuf)
{
    WCHAR szProduct[GUID_SIZE];

    TRACE("%s %p %p\n", debugstr_w(szComponent), lpPathBuf, pcchBuf);

    if (!szComponent || !pcchBuf)
        return INSTALLSTATE_INVALIDARG;

    if (MsiGetProductCodeW( szComponent, szProduct ) != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    return MsiGetComponentPathW( szProduct, szComponent, lpPathBuf, pcchBuf );
}

UINT WINAPI MsiMessageBoxA( HWND hWnd, const char *lpText, const char *lpCaption, UINT uType, WORD wLanguageId,
                            DWORD f )
{
    FIXME( "%p, %s, %s, %u, %#x, %#lx\n", hWnd, debugstr_a(lpText), debugstr_a(lpCaption), uType, wLanguageId, f );
    return MessageBoxExA( hWnd, lpText, lpCaption, uType, wLanguageId );
}

UINT WINAPI MsiMessageBoxW( HWND hWnd, const WCHAR *lpText, const WCHAR *lpCaption, UINT uType, WORD wLanguageId,
                            DWORD f )
{
    FIXME( "%p, %s, %s, %u, %#x %#lx\n", hWnd, debugstr_w(lpText), debugstr_w(lpCaption), uType, wLanguageId, f );
    return MessageBoxExW( hWnd, lpText, lpCaption, uType, wLanguageId );
}

UINT WINAPI MsiMessageBoxExA( HWND hWnd, const char *lpText, const char *lpCaption, UINT uType, DWORD unknown,
                              WORD wLanguageId, DWORD f )
{
    FIXME( "%p, %s, %s, %u, %#lx, %#x, %#lx): semi-stub\n", hWnd, debugstr_a(lpText), debugstr_a(lpCaption), uType,
           unknown, wLanguageId, f );
    return MessageBoxExA( hWnd, lpText, lpCaption, uType, wLanguageId );
}

UINT WINAPI MsiMessageBoxExW( HWND hWnd, const WCHAR *lpText, const WCHAR *lpCaption, UINT uType, DWORD unknown,
                              WORD wLanguageId, DWORD f )
{
    FIXME( "%p, %s, %s, %u, %#lx, %#x, %#lx): semi-stub\n", hWnd, debugstr_w(lpText), debugstr_w(lpCaption), uType,
           unknown, wLanguageId, f );
    return MessageBoxExW( hWnd, lpText, lpCaption, uType, wLanguageId );
}

UINT WINAPI MsiProvideAssemblyA( const char *szAssemblyName, const char *szAppContext, DWORD dwInstallMode,
                                 DWORD dwAssemblyInfo, char *lpPathBuf, DWORD *pcchPathBuf )
{
    FIXME( "%s, %s, %#lx, %#lx, %p, %p\n", debugstr_a(szAssemblyName), debugstr_a(szAppContext), dwInstallMode,
           dwAssemblyInfo, lpPathBuf, pcchPathBuf );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyW( const WCHAR *szAssemblyName, const WCHAR *szAppContext, DWORD dwInstallMode,
                                 DWORD dwAssemblyInfo, WCHAR *lpPathBuf, DWORD *pcchPathBuf )
{
    FIXME( "%s, %s, %#lx, %#lx, %p, %p\n", debugstr_w(szAssemblyName), debugstr_w(szAppContext), dwInstallMode,
           dwAssemblyInfo, lpPathBuf, pcchPathBuf );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideComponentFromDescriptorA( LPCSTR szDescriptor,
                LPSTR szPath, LPDWORD pcchPath, LPDWORD pcchArgs )
{
    FIXME("%s %p %p %p\n", debugstr_a(szDescriptor), szPath, pcchPath, pcchArgs );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideComponentFromDescriptorW( LPCWSTR szDescriptor,
                LPWSTR szPath, LPDWORD pcchPath, LPDWORD pcchArgs )
{
    FIXME("%s %p %p %p\n", debugstr_w(szDescriptor), szPath, pcchPath, pcchArgs );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationA( const char *path, DWORD flags, PCCERT_CONTEXT *cert, BYTE *hash,
                                                DWORD *hashlen )
{
    UINT r;
    WCHAR *pathW = NULL;

    TRACE( "%s, %#lx, %p, %p, %p\n", debugstr_a(path), flags, cert, hash, hashlen );

    if (path && !(pathW = strdupAtoW( path ))) return E_OUTOFMEMORY;
    r = MsiGetFileSignatureInformationW( pathW, flags, cert, hash, hashlen );
    free( pathW );
    return r;
}

HRESULT WINAPI MsiGetFileSignatureInformationW( const WCHAR *path, DWORD flags, PCCERT_CONTEXT *cert, BYTE *hash,
                                                DWORD *hashlen )
{
    static GUID generic_verify_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    HRESULT hr;
    WINTRUST_DATA data;
    WINTRUST_FILE_INFO info;
    CRYPT_PROVIDER_SGNR *signer;
    CRYPT_PROVIDER_CERT *provider;

    TRACE( "%s, %#lx, %p, %p, %p\n", debugstr_w(path), flags, cert, hash, hashlen );

    if (!path || !cert) return E_INVALIDARG;

    info.cbStruct       = sizeof(info);
    info.pcwszFilePath  = path;
    info.hFile          = NULL;
    info.pgKnownSubject = NULL;

    memset( &data, 0, sizeof(data) );
    data.cbStruct            = sizeof(data);
    data.dwUIChoice          = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    data.dwUnionChoice       = WTD_CHOICE_FILE;
    data.pFile               = &info;
    data.dwStateAction       = WTD_STATEACTION_VERIFY;
    data.dwUIContext         = WTD_UICONTEXT_INSTALL;

    hr = WinVerifyTrustEx( INVALID_HANDLE_VALUE, &generic_verify_v2, &data );
    *cert = NULL;
    if (FAILED(hr)) goto done;

    if (!(signer = WTHelperGetProvSignerFromChain( data.hWVTStateData, 0, FALSE, 0 )))
    {
        hr = TRUST_E_NOSIGNATURE;
        goto done;
    }
    if (hash)
    {
        DWORD len = signer->psSigner->EncryptedHash.cbData;
        if (*hashlen < len)
        {
            *hashlen = len;
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            goto done;
        }
        memcpy( hash, signer->psSigner->EncryptedHash.pbData, len );
        *hashlen = len;
    }
    if (!(provider = WTHelperGetProvCertFromChain( signer, 0 )))
    {
        hr = TRUST_E_PROVIDER_UNKNOWN;
        goto done;
    }
    *cert = CertDuplicateCertificateContext( provider->pCert );

done:
    data.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrustEx( INVALID_HANDLE_VALUE, &generic_verify_v2, &data );
    return hr;
}

/******************************************************************
 * MsiGetProductPropertyA      [MSI.@]
 */
UINT WINAPI MsiGetProductPropertyA( MSIHANDLE hProduct, const char *szProperty, char *szValue, DWORD *pccbValue )
{
    LPWSTR prop = NULL, val = NULL;
    DWORD len;
    UINT r;

    TRACE( "%lu, %s, %p, %p\n", hProduct, debugstr_a(szProperty), szValue, pccbValue );

    if (szValue && !pccbValue)
        return ERROR_INVALID_PARAMETER;

    if (szProperty) prop = strdupAtoW(szProperty);

    len = 0;
    r = MsiGetProductPropertyW(hProduct, prop, NULL, &len);
    if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA)
        goto done;

    if (r == ERROR_SUCCESS)
    {
        if (szValue) *szValue = '\0';
        if (pccbValue) *pccbValue = 0;
        goto done;
    }

    val = malloc(++len * sizeof(WCHAR));
    if (!val)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiGetProductPropertyW(hProduct, prop, val, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    len = WideCharToMultiByte(CP_ACP, 0, val, -1, NULL, 0, NULL, NULL);

    if (szValue)
        WideCharToMultiByte(CP_ACP, 0, val, -1, szValue,
                            *pccbValue, NULL, NULL);

    if (pccbValue)
    {
        if (len > *pccbValue)
            r = ERROR_MORE_DATA;

        *pccbValue = len - 1;
    }

done:
    free(prop);
    free(val);

    return r;
}

/******************************************************************
 * MsiGetProductPropertyW      [MSI.@]
 */
UINT WINAPI MsiGetProductPropertyW( MSIHANDLE hProduct, const WCHAR *szProperty, WCHAR *szValue, DWORD *pccbValue )
{
    MSIPACKAGE *package;
    MSIQUERY *view = NULL;
    MSIRECORD *rec = NULL;
    LPCWSTR val;
    UINT r;

    TRACE( "%lu, %s, %p, %p)\n", hProduct, debugstr_w(szProperty), szValue, pccbValue );

    if (!szProperty)
        return ERROR_INVALID_PARAMETER;

    if (szValue && !pccbValue)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(hProduct, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    r = MSI_OpenQuery(package->db, &view, L"SELECT * FROM `Property` WHERE `Property` = '%s'", szProperty);
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewExecute(view, 0);
    if (r != ERROR_SUCCESS)
        goto done;

    r = MSI_ViewFetch(view, &rec);
    if (r != ERROR_SUCCESS)
        goto done;

    val = MSI_RecordGetString(rec, 2);
    if (!val)
        goto done;

    if (lstrlenW(val) >= *pccbValue)
    {
        if (szValue) lstrcpynW(szValue, val, *pccbValue);
        r = ERROR_MORE_DATA;
    }
    else
    {
        if (szValue) lstrcpyW(szValue, val);
        r = ERROR_SUCCESS;
    }

    *pccbValue = lstrlenW(val);

done:
    if (view)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        if (rec) msiobj_release(&rec->hdr);
    }

    if (!rec)
    {
        if (szValue) *szValue = '\0';
        if (pccbValue) *pccbValue = 0;
        r = ERROR_SUCCESS;
    }

    msiobj_release(&package->hdr);
    return r;
}

UINT WINAPI MsiVerifyPackageA( LPCSTR szPackage )
{
    UINT r;
    LPWSTR szPack = NULL;

    TRACE("%s\n", debugstr_a(szPackage) );

    if( szPackage )
    {
        szPack = strdupAtoW( szPackage );
        if( !szPack )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiVerifyPackageW( szPack );

    free( szPack );

    return r;
}

UINT WINAPI MsiVerifyPackageW( LPCWSTR szPackage )
{
    MSIHANDLE handle;
    UINT r;

    TRACE("%s\n", debugstr_w(szPackage) );

    r = MsiOpenDatabaseW( szPackage, MSIDBOPEN_READONLY, &handle );
    MsiCloseHandle( handle );

    return r;
}

static BOOL open_userdata_comp_key( const WCHAR *comp, const WCHAR *usersid, MSIINSTALLCONTEXT ctx,
                                    HKEY *hkey )
{
    if (ctx & MSIINSTALLCONTEXT_MACHINE)
    {
        if (!MSIREG_OpenUserDataComponentKey( comp, L"S-1-5-18", hkey, FALSE )) return TRUE;
    }
    if (ctx & (MSIINSTALLCONTEXT_USERMANAGED|MSIINSTALLCONTEXT_USERUNMANAGED))
    {
        if (usersid && !wcsicmp( usersid, L"S-1-1-0" ))
        {
            FIXME( "only looking at the current user\n" );
            usersid = NULL;
        }
        if (!MSIREG_OpenUserDataComponentKey( comp, usersid, hkey, FALSE )) return TRUE;
    }
    return FALSE;
}

static INSTALLSTATE MSI_GetComponentPath( const WCHAR *szProduct, const WCHAR *szComponent,
                                          const WCHAR *szUserSid, MSIINSTALLCONTEXT ctx,
                                          awstring *lpPathBuf, DWORD *pcchBuf )
{
    WCHAR *path = NULL, squashed_pc[SQUASHED_GUID_SIZE], squashed_comp[SQUASHED_GUID_SIZE];
    HKEY hkey;
    INSTALLSTATE state;
    DWORD version;

    if (!szProduct || !szComponent)
        return INSTALLSTATE_INVALIDARG;

    if (lpPathBuf->str.w && !pcchBuf)
        return INSTALLSTATE_INVALIDARG;

    if (!squash_guid( szProduct, squashed_pc ) || !squash_guid( szComponent, squashed_comp ))
        return INSTALLSTATE_INVALIDARG;

    if (szUserSid && ctx == MSIINSTALLCONTEXT_MACHINE)
        return INSTALLSTATE_INVALIDARG;

    state = INSTALLSTATE_UNKNOWN;

    if (open_userdata_comp_key( szComponent, szUserSid, ctx, &hkey ))
    {
        path = msi_reg_get_val_str( hkey, squashed_pc );
        RegCloseKey(hkey);

        state = INSTALLSTATE_ABSENT;

        if ((!MSIREG_OpenInstallProps(szProduct, MSIINSTALLCONTEXT_MACHINE, NULL, &hkey, FALSE) ||
             !MSIREG_OpenUserDataProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED, NULL, &hkey, FALSE)) &&
            msi_reg_get_val_dword(hkey, L"WindowsInstaller", &version) &&
            GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
        {
            RegCloseKey(hkey);
            state = INSTALLSTATE_LOCAL;
        }
    }

    if (state != INSTALLSTATE_LOCAL &&
        (!MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, &hkey, FALSE) ||
         !MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_MACHINE, &hkey, FALSE)))
    {
        RegCloseKey(hkey);

        if (open_userdata_comp_key( szComponent, szUserSid, ctx, &hkey ))
        {
            free(path);
            path = msi_reg_get_val_str( hkey, squashed_pc );
            RegCloseKey(hkey);

            state = INSTALLSTATE_ABSENT;

            if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
                state = INSTALLSTATE_LOCAL;
        }
    }

    if (!path)
        return INSTALLSTATE_UNKNOWN;

    if (state == INSTALLSTATE_LOCAL && !*path)
        state = INSTALLSTATE_NOTUSED;

    if (msi_strcpy_to_awstring(path, -1, lpPathBuf, pcchBuf) == ERROR_MORE_DATA)
        state = INSTALLSTATE_MOREDATA;

    free(path);
    return state;
}

/******************************************************************
 * MsiGetComponentPathExW      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathExW( LPCWSTR product, LPCWSTR comp, LPCWSTR usersid,
                                            MSIINSTALLCONTEXT ctx, LPWSTR buf, LPDWORD buflen )
{
    awstring path;

    TRACE( "%s %s %s 0x%x %p %p\n", debugstr_w(product), debugstr_w(comp), debugstr_w(usersid),
           ctx, buf, buflen );

    path.unicode = TRUE;
    path.str.w   = buf;

    return MSI_GetComponentPath( product, comp, usersid, ctx, &path, buflen );
}

INSTALLSTATE WINAPI MsiGetComponentPathExA( LPCSTR product, LPCSTR comp, LPCSTR usersid,
                                            MSIINSTALLCONTEXT ctx, LPSTR buf, LPDWORD buflen )
{
    WCHAR *productW = NULL, *compW = NULL, *usersidW =  NULL;
    INSTALLSTATE r = INSTALLSTATE_UNKNOWN;
    awstring path;

    TRACE( "%s %s %s 0x%x %p %p\n", debugstr_a(product), debugstr_a(comp), debugstr_a(usersid),
           ctx, buf, buflen );

    if (product && !(productW = strdupAtoW( product ))) return INSTALLSTATE_UNKNOWN;
    if (comp && !(compW = strdupAtoW( comp ))) goto end;
    if (usersid && !(usersidW = strdupAtoW( usersid ))) goto end;

    path.unicode = FALSE;
    path.str.a   = buf;

    r = MSI_GetComponentPath( productW, compW, usersidW, ctx, &path, buflen );

end:
    free( productW );
    free( compW );
    free( usersidW );

    return r;
}

/******************************************************************
 * MsiGetComponentPathW      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathW( LPCWSTR product, LPCWSTR comp, LPWSTR buf, LPDWORD buflen )
{
    return MsiGetComponentPathExW( product, comp, L"S-1-1-0", MSIINSTALLCONTEXT_ALL, buf, buflen );
}

/******************************************************************
 * MsiGetComponentPathA      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathA( LPCSTR product, LPCSTR comp, LPSTR buf, LPDWORD buflen )
{
    return MsiGetComponentPathExA( product, comp, "s-1-1-0", MSIINSTALLCONTEXT_ALL, buf, buflen );
}

static UINT query_feature_state( const WCHAR *product, const WCHAR *squashed, const WCHAR *usersid,
                                 MSIINSTALLCONTEXT ctx, const WCHAR *feature, INSTALLSTATE *state )
{
    UINT r;
    HKEY hkey;
    WCHAR *parent, *components, *path;
    const WCHAR *p;
    BOOL missing = FALSE, source = FALSE;
    WCHAR comp[GUID_SIZE];
    GUID guid;

    if (ctx != MSIINSTALLCONTEXT_MACHINE) SetLastError( ERROR_SUCCESS );

    if (MSIREG_OpenFeaturesKey( product, usersid, ctx, &hkey, FALSE )) return ERROR_UNKNOWN_PRODUCT;

    parent = msi_reg_get_val_str( hkey, feature );
    RegCloseKey( hkey );
    if (!parent) return ERROR_UNKNOWN_FEATURE;

    *state = (parent[0] == 6) ? INSTALLSTATE_ABSENT : INSTALLSTATE_LOCAL;
    free( parent );
    if (*state == INSTALLSTATE_ABSENT)
        return ERROR_SUCCESS;

    r = MSIREG_OpenUserDataFeaturesKey( product, usersid, ctx, &hkey, FALSE );
    if (r != ERROR_SUCCESS)
    {
        *state = INSTALLSTATE_ADVERTISED;
        return ERROR_SUCCESS;
    }
    components = msi_reg_get_val_str( hkey, feature );
    RegCloseKey( hkey );

    TRACE("buffer = %s\n", debugstr_w(components));

    if (!components)
    {
        *state = INSTALLSTATE_ADVERTISED;
        return ERROR_SUCCESS;
    }
    for (p = components; *p && *p != 2 ; p += 20)
    {
        if (!decode_base85_guid( p, &guid ))
        {
            if (p != components) break;
            free( components );
            *state = INSTALLSTATE_BADCONFIG;
            return ERROR_BAD_CONFIGURATION;
        }
        StringFromGUID2( &guid, comp, GUID_SIZE );
        if (ctx == MSIINSTALLCONTEXT_MACHINE)
            r = MSIREG_OpenUserDataComponentKey( comp, L"S-1-5-18", &hkey, FALSE );
        else
            r = MSIREG_OpenUserDataComponentKey( comp, usersid, &hkey, FALSE );

        if (r != ERROR_SUCCESS)
        {
            free( components );
            *state = INSTALLSTATE_ADVERTISED;
            return ERROR_SUCCESS;
        }
        path = msi_reg_get_val_str( hkey, squashed );
        if (!path) missing = TRUE;
        else if (lstrlenW( path ) > 2 &&
                 path[0] >= '0' && path[0] <= '9' &&
                 path[1] >= '0' && path[1] <= '9')
        {
            source = TRUE;
        }
        free( path );
    }
    free( components );

    if (missing)
        *state = INSTALLSTATE_ADVERTISED;
    else if (source)
        *state = INSTALLSTATE_SOURCE;
    else
        *state = INSTALLSTATE_LOCAL;

    TRACE("returning state %d\n", *state);
    return ERROR_SUCCESS;
}

UINT WINAPI MsiQueryFeatureStateExA( LPCSTR product, LPCSTR usersid, MSIINSTALLCONTEXT ctx,
                                     LPCSTR feature, INSTALLSTATE *state )
{
    UINT r;
    WCHAR *productW = NULL, *usersidW = NULL, *featureW = NULL;

    if (product && !(productW = strdupAtoW( product ))) return ERROR_OUTOFMEMORY;
    if (usersid && !(usersidW = strdupAtoW( usersid )))
    {
        free( productW );
        return ERROR_OUTOFMEMORY;
    }
    if (feature && !(featureW = strdupAtoW( feature )))
    {
        free( productW );
        free( usersidW );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiQueryFeatureStateExW( productW, usersidW, ctx, featureW, state );
    free( productW );
    free( usersidW );
    free( featureW );
    return r;
}

UINT WINAPI MsiQueryFeatureStateExW( LPCWSTR product, LPCWSTR usersid, MSIINSTALLCONTEXT ctx,
                                     LPCWSTR feature, INSTALLSTATE *state )
{
    WCHAR squashed[33];
    if (!squash_guid( product, squashed )) return ERROR_INVALID_PARAMETER;
    return query_feature_state( product, squashed, usersid, ctx, feature, state );
}

/******************************************************************
 * MsiQueryFeatureStateA      [MSI.@]
 */
INSTALLSTATE WINAPI MsiQueryFeatureStateA(LPCSTR szProduct, LPCSTR szFeature)
{
    LPWSTR szwProduct = NULL, szwFeature= NULL;
    INSTALLSTATE rc = INSTALLSTATE_UNKNOWN;

    szwProduct = strdupAtoW( szProduct );
    if ( szProduct && !szwProduct )
        goto end;

    szwFeature = strdupAtoW( szFeature );
    if ( szFeature && !szwFeature )
        goto end;

    rc = MsiQueryFeatureStateW(szwProduct, szwFeature);

end:
    free(szwProduct);
    free(szwFeature);

    return rc;
}

/******************************************************************
 * MsiQueryFeatureStateW      [MSI.@]
 *
 * Checks the state of a feature
 *
 * PARAMS
 *   szProduct     [I]  Product's GUID string
 *   szFeature     [I]  Feature's GUID string
 *
 * RETURNS
 *   INSTALLSTATE_LOCAL        Feature is installed and usable
 *   INSTALLSTATE_ABSENT       Feature is absent
 *   INSTALLSTATE_ADVERTISED   Feature should be installed on demand
 *   INSTALLSTATE_UNKNOWN      An error occurred
 *   INSTALLSTATE_INVALIDARG   One of the GUIDs was invalid
 *
 */
INSTALLSTATE WINAPI MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    UINT r;
    INSTALLSTATE state;
    WCHAR squashed[33];

    TRACE("%s %s\n", debugstr_w(szProduct), debugstr_w(szFeature));

    if (!szProduct || !szFeature || !squash_guid( szProduct, squashed ))
        return INSTALLSTATE_INVALIDARG;

    r = query_feature_state( szProduct, squashed, NULL, MSIINSTALLCONTEXT_USERMANAGED, szFeature, &state );
    if (r == ERROR_SUCCESS || r == ERROR_BAD_CONFIGURATION) return state;

    r = query_feature_state( szProduct, squashed, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, szFeature, &state );
    if (r == ERROR_SUCCESS || r == ERROR_BAD_CONFIGURATION) return state;

    r = query_feature_state( szProduct, squashed, NULL, MSIINSTALLCONTEXT_MACHINE, szFeature, &state );
    if (r == ERROR_SUCCESS || r == ERROR_BAD_CONFIGURATION) return state;

    return INSTALLSTATE_UNKNOWN;
}

/******************************************************************
 * MsiGetFileVersionA         [MSI.@]
 */
UINT WINAPI MsiGetFileVersionA(LPCSTR szFilePath, LPSTR lpVersionBuf,
                LPDWORD pcchVersionBuf, LPSTR lpLangBuf, LPDWORD pcchLangBuf)
{
    LPWSTR szwFilePath = NULL, lpwVersionBuff = NULL, lpwLangBuff = NULL;
    UINT ret = ERROR_OUTOFMEMORY;

    if ((lpVersionBuf && !pcchVersionBuf) ||
        (lpLangBuf && !pcchLangBuf))
        return ERROR_INVALID_PARAMETER;

    if( szFilePath )
    {
        szwFilePath = strdupAtoW( szFilePath );
        if( !szwFilePath )
            goto end;
    }

    if( lpVersionBuf && pcchVersionBuf && *pcchVersionBuf )
    {
        lpwVersionBuff = malloc(*pcchVersionBuf * sizeof(WCHAR));
        if( !lpwVersionBuff )
            goto end;
    }

    if( lpLangBuf && pcchLangBuf && *pcchLangBuf )
    {
        lpwLangBuff = malloc(*pcchLangBuf * sizeof(WCHAR));
        if( !lpwLangBuff )
            goto end;
    }

    ret = MsiGetFileVersionW(szwFilePath, lpwVersionBuff, pcchVersionBuf,
                             lpwLangBuff, pcchLangBuf);

    if( (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) && lpwVersionBuff )
        WideCharToMultiByte(CP_ACP, 0, lpwVersionBuff, -1,
                            lpVersionBuf, *pcchVersionBuf + 1, NULL, NULL);
    if( (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) && lpwLangBuff )
        WideCharToMultiByte(CP_ACP, 0, lpwLangBuff, -1,
                            lpLangBuf, *pcchLangBuf + 1, NULL, NULL);

end:
    free(szwFilePath);
    free(lpwVersionBuff);
    free(lpwLangBuff);

    return ret;
}

static UINT get_file_version( const WCHAR *path, WCHAR *verbuf, DWORD *verlen,
                              WCHAR *langbuf, DWORD *langlen )
{
    UINT ret = ERROR_MORE_DATA;
    UINT len;
    DWORD error;
    LPVOID version;
    VS_FIXEDFILEINFO *ffi;
    USHORT *lang;
    WCHAR tmp[32];

    if (!(len = GetFileVersionInfoSizeW( path, NULL )))
    {
        error = GetLastError();
        if (error == ERROR_BAD_PATHNAME) return ERROR_FILE_NOT_FOUND;
        if (error == ERROR_RESOURCE_DATA_NOT_FOUND) return ERROR_FILE_INVALID;
        return error;
    }
    if (!(version = malloc( len ))) return ERROR_OUTOFMEMORY;
    if (!GetFileVersionInfoW( path, 0, len, version ))
    {
        free( version );
        return GetLastError();
    }
    if (!verbuf && !verlen && !langbuf && !langlen)
    {
        free( version );
        return ERROR_SUCCESS;
    }
    if (verlen)
    {
        if (VerQueryValueW( version, L"\\", (LPVOID *)&ffi, &len ) && len > 0)
        {
            swprintf( tmp, ARRAY_SIZE(tmp), L"%d.%d.%d.%d",
                      HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
                      HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS) );
            if (verbuf) lstrcpynW( verbuf, tmp, *verlen );
            len = lstrlenW( tmp );
            if (*verlen > len) ret = ERROR_SUCCESS;
            *verlen = len;
        }
        else
        {
            if (verbuf) *verbuf = 0;
            *verlen = 0;
        }
    }
    if (langlen)
    {
        if (VerQueryValueW( version, L"\\VarFileInfo\\Translation", (LPVOID *)&lang, &len ) && len > 0)
        {
            swprintf( tmp, ARRAY_SIZE(tmp), L"%d", *lang );
            if (langbuf) lstrcpynW( langbuf, tmp, *langlen );
            len = lstrlenW( tmp );
            if (*langlen > len) ret = ERROR_SUCCESS;
            *langlen = len;
        }
        else
        {
            if (langbuf) *langbuf = 0;
            *langlen = 0;
        }
    }
    free( version );
    return ret;
}


/******************************************************************
 * MsiGetFileVersionW         [MSI.@]
 */
UINT WINAPI MsiGetFileVersionW( const WCHAR *path, WCHAR *verbuf, DWORD *verlen, WCHAR *langbuf, DWORD *langlen )
{
    UINT ret;

    TRACE( "%s, %p(%lu), %p(%lu)\n", debugstr_w(path), verbuf, verlen ? *verlen : 0, langbuf, langlen ? *langlen : 0 );

    if ((verbuf && !verlen) || (langbuf && !langlen))
        return ERROR_INVALID_PARAMETER;

    ret = get_file_version( path, verbuf, verlen, langbuf, langlen );
    if (ret == ERROR_RESOURCE_DATA_NOT_FOUND && verlen)
    {
        int len;
        WCHAR *version = msi_get_font_file_version( NULL, path );
        if (!version) return ERROR_FILE_INVALID;
        len = lstrlenW( version );
        if (len >= *verlen) ret = ERROR_MORE_DATA;
        else if (verbuf)
        {
            lstrcpyW( verbuf, version );
            ret = ERROR_SUCCESS;
        }
        *verlen = len;
        free( version );
    }
    return ret;
}

/***********************************************************************
 * MsiGetFeatureUsageW           [MSI.@]
 */
UINT WINAPI MsiGetFeatureUsageW( LPCWSTR szProduct, LPCWSTR szFeature,
                                 LPDWORD pdwUseCount, LPWORD pwDateUsed )
{
    FIXME("%s %s %p %p\n",debugstr_w(szProduct), debugstr_w(szFeature),
          pdwUseCount, pwDateUsed);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiGetFeatureUsageA           [MSI.@]
 */
UINT WINAPI MsiGetFeatureUsageA( LPCSTR szProduct, LPCSTR szFeature,
                                 LPDWORD pdwUseCount, LPWORD pwDateUsed )
{
    LPWSTR prod = NULL, feat = NULL;
    UINT ret = ERROR_OUTOFMEMORY;

    TRACE("%s %s %p %p\n", debugstr_a(szProduct), debugstr_a(szFeature),
          pdwUseCount, pwDateUsed);

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    ret = MsiGetFeatureUsageW( prod, feat, pdwUseCount, pwDateUsed );

end:
    free( prod );
    free( feat );

    return ret;
}

/***********************************************************************
 * MsiUseFeatureExW           [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureExW( const WCHAR *szProduct, const WCHAR *szFeature, DWORD dwInstallMode,
                                      DWORD dwReserved )
{
    INSTALLSTATE state;

    TRACE( "%s, %s, %lu %#lx\n", debugstr_w(szProduct), debugstr_w(szFeature), dwInstallMode, dwReserved );

    state = MsiQueryFeatureStateW( szProduct, szFeature );

    if (dwReserved)
        return INSTALLSTATE_INVALIDARG;

    if (state == INSTALLSTATE_LOCAL && dwInstallMode != INSTALLMODE_NODETECTION)
    {
        FIXME("mark product %s feature %s as used\n",
              debugstr_w(szProduct), debugstr_w(szFeature) );
    }

    return state;
}

/***********************************************************************
 * MsiUseFeatureExA           [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureExA( const char *szProduct, const char *szFeature, DWORD dwInstallMode,
                                      DWORD dwReserved )
{
    INSTALLSTATE ret = INSTALLSTATE_UNKNOWN;
    WCHAR *prod = NULL, *feat = NULL;

    TRACE( "%s, %s, %lu, %#lx\n", debugstr_a(szProduct), debugstr_a(szFeature), dwInstallMode, dwReserved );

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    ret = MsiUseFeatureExW( prod, feat, dwInstallMode, dwReserved );

end:
    free( prod );
    free( feat );

    return ret;
}

/***********************************************************************
 * MsiUseFeatureW             [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureW( LPCWSTR szProduct, LPCWSTR szFeature )
{
    return MsiUseFeatureExW(szProduct, szFeature, 0, 0);
}

/***********************************************************************
 * MsiUseFeatureA             [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureA( LPCSTR szProduct, LPCSTR szFeature )
{
    return MsiUseFeatureExA(szProduct, szFeature, 0, 0);
}

static WCHAR *reg_get_multisz( HKEY hkey, const WCHAR *name )
{
    WCHAR *ret;
    DWORD len, type;
    if (RegQueryValueExW( hkey, name, NULL, &type, NULL, &len ) || type != REG_MULTI_SZ) return NULL;
    if ((ret = malloc( len ))) RegQueryValueExW( hkey, name, NULL, NULL, (BYTE *)ret, &len );
    return ret;
}

static WCHAR *reg_get_sz( HKEY hkey, const WCHAR *name )
{
    WCHAR *ret;
    DWORD len, type;
    if (RegQueryValueExW( hkey, name, NULL, &type, NULL, &len ) || type != REG_SZ) return NULL;
    if ((ret = malloc( len ))) RegQueryValueExW( hkey, name, NULL, NULL, (BYTE *)ret, &len );
    return ret;
}

#define BASE85_SIZE 20

/***********************************************************************
 * MSI_ProvideQualifiedComponentEx [internal]
 */
static UINT MSI_ProvideQualifiedComponentEx(LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPCWSTR szProduct,
                DWORD Unused1, DWORD Unused2, awstring *lpPathBuf,
                LPDWORD pcchPathBuf)
{
    WCHAR product[MAX_FEATURE_CHARS+1], comp[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1];
    WCHAR *desc;
    HKEY hkey;
    DWORD size;
    UINT ret;
    INSTALLSTATE state;

    if (MSIREG_OpenUserComponentsKey( szComponent, &hkey, FALSE )) return ERROR_UNKNOWN_COMPONENT;

    desc = reg_get_multisz( hkey, szQualifier );
    RegCloseKey(hkey);
    if (!desc) return ERROR_INDEX_ABSENT;

    /* FIXME: handle multiple descriptors */
    ret = MsiDecomposeDescriptorW( desc, product, feature, comp, &size );
    free( desc );
    if (ret != ERROR_SUCCESS) return ret;

    if (!szProduct) szProduct = product;
    if (!comp[0])
    {
        MSIINSTALLCONTEXT ctx;
        WCHAR *components;
        GUID guid;

        /* use the first component of the feature if the descriptor component is empty */
        if ((ret = msi_locate_product( szProduct, &ctx ))) return ret;
        if ((ret = MSIREG_OpenUserDataFeaturesKey( szProduct, NULL, ctx, &hkey, FALSE )))
        {
            return ERROR_FILE_NOT_FOUND;
        }
        components = reg_get_sz( hkey, feature );
        RegCloseKey( hkey );
        if (!components) return ERROR_FILE_NOT_FOUND;

        if (lstrlenW( components ) < BASE85_SIZE || !decode_base85_guid( components, &guid ))
        {
            free( components );
            return ERROR_FILE_NOT_FOUND;
        }
        free( components );
        StringFromGUID2( &guid, comp, ARRAY_SIZE( comp ));
    }

    state = MSI_GetComponentPath( szProduct, comp, L"S-1-1-0", MSIINSTALLCONTEXT_ALL, lpPathBuf, pcchPathBuf );

    if (state == INSTALLSTATE_MOREDATA) return ERROR_MORE_DATA;
    if (state != INSTALLSTATE_LOCAL) return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiProvideQualifiedComponentExW [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentExW( const WCHAR *szComponent, const WCHAR *szQualifier, DWORD dwInstallMode,
                                             const WCHAR *szProduct, DWORD Unused1, DWORD Unused2, WCHAR *lpPathBuf,
                                             DWORD *pcchPathBuf )
{
    awstring path;

    TRACE( "%s, %s, %lu, %s, %#lx, %#lx, %p, %p\n", debugstr_w(szComponent), debugstr_w(szQualifier), dwInstallMode,
           debugstr_w(szProduct), Unused1, Unused2, lpPathBuf, pcchPathBuf );

    path.unicode = TRUE;
    path.str.w = lpPathBuf;

    return MSI_ProvideQualifiedComponentEx( szComponent, szQualifier, dwInstallMode, szProduct, Unused1, Unused2,
                                            &path, pcchPathBuf );
}

/***********************************************************************
 * MsiProvideQualifiedComponentExA [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentExA( const char *szComponent, const char *szQualifier, DWORD dwInstallMode,
                                             const char *szProduct, DWORD Unused1, DWORD Unused2, char *lpPathBuf,
                                             DWORD *pcchPathBuf )
{
    WCHAR *szwComponent, *szwQualifier = NULL, *szwProduct = NULL;
    UINT r = ERROR_OUTOFMEMORY;
    awstring path;

    TRACE( "%s, %s, %lu, %s, %#lx, %#lx, %p, %p\n", debugstr_a(szComponent), debugstr_a(szQualifier), dwInstallMode,
           debugstr_a(szProduct), Unused1, Unused2, lpPathBuf, pcchPathBuf );

    szwComponent = strdupAtoW( szComponent );
    if (szComponent && !szwComponent)
        goto end;

    szwQualifier = strdupAtoW( szQualifier );
    if (szQualifier && !szwQualifier)
        goto end;

    szwProduct = strdupAtoW( szProduct );
    if (szProduct && !szwProduct)
        goto end;

    path.unicode = FALSE;
    path.str.a = lpPathBuf;

    r = MSI_ProvideQualifiedComponentEx(szwComponent, szwQualifier,
                              dwInstallMode, szwProduct, Unused1,
                              Unused2, &path, pcchPathBuf);
end:
    free(szwProduct);
    free(szwComponent);
    free(szwQualifier);

    return r;
}

/***********************************************************************
 * MsiProvideQualifiedComponentW [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentW( LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPWSTR lpPathBuf,
                LPDWORD pcchPathBuf)
{
    return MsiProvideQualifiedComponentExW(szComponent, szQualifier,
                    dwInstallMode, NULL, 0, 0, lpPathBuf, pcchPathBuf);
}

/***********************************************************************
 * MsiProvideQualifiedComponentA [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentA( LPCSTR szComponent,
                LPCSTR szQualifier, DWORD dwInstallMode, LPSTR lpPathBuf,
                LPDWORD pcchPathBuf)
{
    return MsiProvideQualifiedComponentExA(szComponent, szQualifier,
                              dwInstallMode, NULL, 0, 0, lpPathBuf, pcchPathBuf);
}

/***********************************************************************
 * MSI_GetUserInfo [internal]
 */
static USERINFOSTATE MSI_GetUserInfo(LPCWSTR szProduct,
                awstring *lpUserNameBuf, LPDWORD pcchUserNameBuf,
                awstring *lpOrgNameBuf, LPDWORD pcchOrgNameBuf,
                awstring *lpSerialBuf, LPDWORD pcchSerialBuf)
{
    WCHAR *user, *org, *serial, squashed_pc[SQUASHED_GUID_SIZE];
    USERINFOSTATE state;
    HKEY hkey, props;
    LPCWSTR orgptr;
    UINT r;

    TRACE("%s %p %p %p %p %p %p\n", debugstr_w(szProduct), lpUserNameBuf,
          pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf,
          pcchSerialBuf);

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return USERINFOSTATE_INVALIDARG;

    if (MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                              &hkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              &hkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, NULL, MSIINSTALLCONTEXT_MACHINE,
                              &hkey, FALSE) != ERROR_SUCCESS)
    {
        return USERINFOSTATE_UNKNOWN;
    }

    if (MSIREG_OpenInstallProps(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                                NULL, &props, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenInstallProps(szProduct, MSIINSTALLCONTEXT_MACHINE,
                                NULL, &props, FALSE) != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return USERINFOSTATE_ABSENT;
    }

    user = msi_reg_get_val_str(props, INSTALLPROPERTY_REGOWNERW);
    org = msi_reg_get_val_str(props, INSTALLPROPERTY_REGCOMPANYW);
    serial = msi_reg_get_val_str(props, INSTALLPROPERTY_PRODUCTIDW);
    state = USERINFOSTATE_ABSENT;

    RegCloseKey(hkey);
    RegCloseKey(props);

    if (user && serial)
        state = USERINFOSTATE_PRESENT;

    if (pcchUserNameBuf)
    {
        if (lpUserNameBuf && !user)
        {
            (*pcchUserNameBuf)--;
            goto done;
        }

        r = msi_strcpy_to_awstring(user, -1, lpUserNameBuf, pcchUserNameBuf);
        if (r == ERROR_MORE_DATA)
        {
            state = USERINFOSTATE_MOREDATA;
            goto done;
        }
    }

    if (pcchOrgNameBuf)
    {
        orgptr = org;
        if (!orgptr) orgptr = L"";

        r = msi_strcpy_to_awstring(orgptr, -1, lpOrgNameBuf, pcchOrgNameBuf);
        if (r == ERROR_MORE_DATA)
        {
            state = USERINFOSTATE_MOREDATA;
            goto done;
        }
    }

    if (pcchSerialBuf)
    {
        if (!serial)
        {
            (*pcchSerialBuf)--;
            goto done;
        }

        r = msi_strcpy_to_awstring(serial, -1, lpSerialBuf, pcchSerialBuf);
        if (r == ERROR_MORE_DATA)
            state = USERINFOSTATE_MOREDATA;
    }

done:
    free(user);
    free(org);
    free(serial);

    return state;
}

/***********************************************************************
 * MsiGetUserInfoW [MSI.@]
 */
USERINFOSTATE WINAPI MsiGetUserInfoW(LPCWSTR szProduct,
                LPWSTR lpUserNameBuf, LPDWORD pcchUserNameBuf,
                LPWSTR lpOrgNameBuf, LPDWORD pcchOrgNameBuf,
                LPWSTR lpSerialBuf, LPDWORD pcchSerialBuf)
{
    awstring user, org, serial;

    if ((lpUserNameBuf && !pcchUserNameBuf) ||
        (lpOrgNameBuf && !pcchOrgNameBuf) ||
        (lpSerialBuf && !pcchSerialBuf))
        return USERINFOSTATE_INVALIDARG;

    user.unicode = TRUE;
    user.str.w = lpUserNameBuf;
    org.unicode = TRUE;
    org.str.w = lpOrgNameBuf;
    serial.unicode = TRUE;
    serial.str.w = lpSerialBuf;

    return MSI_GetUserInfo( szProduct, &user, pcchUserNameBuf,
                            &org, pcchOrgNameBuf,
                            &serial, pcchSerialBuf );
}

USERINFOSTATE WINAPI MsiGetUserInfoA(LPCSTR szProduct,
                LPSTR lpUserNameBuf, LPDWORD pcchUserNameBuf,
                LPSTR lpOrgNameBuf, LPDWORD pcchOrgNameBuf,
                LPSTR lpSerialBuf, LPDWORD pcchSerialBuf)
{
    awstring user, org, serial;
    LPWSTR prod;
    UINT r;

    if ((lpUserNameBuf && !pcchUserNameBuf) ||
        (lpOrgNameBuf && !pcchOrgNameBuf) ||
        (lpSerialBuf && !pcchSerialBuf))
        return USERINFOSTATE_INVALIDARG;

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        return ERROR_OUTOFMEMORY;

    user.unicode = FALSE;
    user.str.a = lpUserNameBuf;
    org.unicode = FALSE;
    org.str.a = lpOrgNameBuf;
    serial.unicode = FALSE;
    serial.str.a = lpSerialBuf;

    r = MSI_GetUserInfo( prod, &user, pcchUserNameBuf,
                         &org, pcchOrgNameBuf,
                         &serial, pcchSerialBuf );

    free( prod );

    return r;
}

UINT WINAPI MsiCollectUserInfoW(LPCWSTR szProduct)
{
    MSIHANDLE handle;
    UINT rc;
    MSIPACKAGE *package;

    TRACE("(%s)\n",debugstr_w(szProduct));

    rc = MsiOpenProductW(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        return ERROR_INVALID_PARAMETER;

    /* MsiCollectUserInfo cannot be called from a custom action. */
    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_CALL_NOT_IMPLEMENTED;

    rc = ACTION_PerformAction(package, L"FirstRun");
    msiobj_release( &package->hdr );

    MsiCloseHandle(handle);

    return rc;
}

UINT WINAPI MsiCollectUserInfoA(LPCSTR szProduct)
{
    MSIHANDLE handle;
    UINT rc;
    MSIPACKAGE *package;

    TRACE("(%s)\n",debugstr_a(szProduct));

    rc = MsiOpenProductA(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        return ERROR_INVALID_PARAMETER;

    /* MsiCollectUserInfo cannot be called from a custom action. */
    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_CALL_NOT_IMPLEMENTED;

    rc = ACTION_PerformAction(package, L"FirstRun");
    msiobj_release( &package->hdr );

    MsiCloseHandle(handle);

    return rc;
}

/***********************************************************************
 * MsiConfigureFeatureA            [MSI.@]
 */
UINT WINAPI MsiConfigureFeatureA(LPCSTR szProduct, LPCSTR szFeature, INSTALLSTATE eInstallState)
{
    LPWSTR prod, feat = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s %i\n", debugstr_a(szProduct), debugstr_a(szFeature), eInstallState);

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    r = MsiConfigureFeatureW(prod, feat, eInstallState);

end:
    free(feat);
    free(prod);

    return r;
}

/***********************************************************************
 * MsiConfigureFeatureW            [MSI.@]
 */
UINT WINAPI MsiConfigureFeatureW(LPCWSTR szProduct, LPCWSTR szFeature, INSTALLSTATE eInstallState)
{
    MSIPACKAGE *package = NULL;
    UINT r;
    WCHAR sourcepath[MAX_PATH], filename[MAX_PATH];
    DWORD sz;

    TRACE("%s %s %i\n", debugstr_w(szProduct), debugstr_w(szFeature), eInstallState);

    if (!szProduct || !szFeature)
        return ERROR_INVALID_PARAMETER;

    switch (eInstallState)
    {
    case INSTALLSTATE_DEFAULT:
        /* FIXME: how do we figure out the default location? */
        eInstallState = INSTALLSTATE_LOCAL;
        break;
    case INSTALLSTATE_LOCAL:
    case INSTALLSTATE_SOURCE:
    case INSTALLSTATE_ABSENT:
    case INSTALLSTATE_ADVERTISED:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_OpenProductW( szProduct, &package );
    if (r != ERROR_SUCCESS)
        return r;

    sz = sizeof(sourcepath);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz);

    sz = sizeof(filename);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

    lstrcatW( sourcepath, filename );

    MsiSetInternalUI( INSTALLUILEVEL_BASIC, NULL );

    r = ACTION_PerformAction(package, L"CostInitialize");
    if (r != ERROR_SUCCESS)
        goto end;

    r = MSI_SetFeatureStateW( package, szFeature, eInstallState);
    if (r != ERROR_SUCCESS)
        goto end;

    r = MSI_InstallPackage( package, sourcepath, NULL );

end:
    msiobj_release( &package->hdr );

    return r;
}

/***********************************************************************
 * MsiCreateAndVerifyInstallerDirectory [MSI.@]
 *
 * Notes: undocumented
 */
UINT WINAPI MsiCreateAndVerifyInstallerDirectory(DWORD dwReserved)
{
    WCHAR path[MAX_PATH];

    TRACE( "%#lx\n", dwReserved );

    if (dwReserved)
    {
        FIXME( "dwReserved = %#lx\n", dwReserved );
        return ERROR_INVALID_PARAMETER;
    }

    if (!GetWindowsDirectoryW(path, MAX_PATH))
        return ERROR_FUNCTION_FAILED;

    lstrcatW(path, L"\\Installer");

    if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiGetShortcutTargetA           [MSI.@]
 */
UINT WINAPI MsiGetShortcutTargetA( LPCSTR szShortcutTarget,
                                   LPSTR szProductCode, LPSTR szFeatureId,
                                   LPSTR szComponentCode )
{
    LPWSTR target;
    const int len = MAX_FEATURE_CHARS+1;
    WCHAR product[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1], component[MAX_FEATURE_CHARS+1];
    UINT r;

    target = strdupAtoW( szShortcutTarget );
    if (szShortcutTarget && !target )
        return ERROR_OUTOFMEMORY;
    product[0] = 0;
    feature[0] = 0;
    component[0] = 0;
    r = MsiGetShortcutTargetW( target, product, feature, component );
    free( target );
    if (r == ERROR_SUCCESS)
    {
        WideCharToMultiByte( CP_ACP, 0, product, -1, szProductCode, len, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, feature, -1, szFeatureId, len, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, component, -1, szComponentCode, len, NULL, NULL );
    }
    return r;
}

/***********************************************************************
 * MsiGetShortcutTargetW           [MSI.@]
 */
UINT WINAPI MsiGetShortcutTargetW( LPCWSTR szShortcutTarget,
                                   LPWSTR szProductCode, LPWSTR szFeatureId,
                                   LPWSTR szComponentCode )
{
    IShellLinkDataList *dl = NULL;
    IPersistFile *pf = NULL;
    LPEXP_DARWIN_LINK darwin = NULL;
    HRESULT r, init;

    TRACE("%s %p %p %p\n", debugstr_w(szShortcutTarget),
          szProductCode, szFeatureId, szComponentCode );

    init = CoInitialize(NULL);

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IPersistFile, (LPVOID*) &pf );
    if( SUCCEEDED( r ) )
    {
        r = IPersistFile_Load( pf, szShortcutTarget,
                               STGM_READ | STGM_SHARE_DENY_WRITE );
        if( SUCCEEDED( r ) )
        {
            r = IPersistFile_QueryInterface( pf, &IID_IShellLinkDataList,
                                             (LPVOID*) &dl );
            if( SUCCEEDED( r ) )
            {
                IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG,
                                                  (LPVOID) &darwin );
                IShellLinkDataList_Release( dl );
            }
        }
        IPersistFile_Release( pf );
    }

    if (SUCCEEDED(init))
        CoUninitialize();

    TRACE("darwin = %p\n", darwin);

    if (darwin)
    {
        DWORD sz;
        UINT ret;

        ret = MsiDecomposeDescriptorW( darwin->szwDarwinID,
                  szProductCode, szFeatureId, szComponentCode, &sz );
        LocalFree( darwin );
        return ret;
    }

    return ERROR_FUNCTION_FAILED;
}

UINT WINAPI MsiReinstallFeatureW( const WCHAR *szProduct, const WCHAR *szFeature, DWORD dwReinstallMode )
{
    MSIPACKAGE *package;
    MSIINSTALLCONTEXT context;
    UINT r;
    WCHAR sourcepath[MAX_PATH], filename[MAX_PATH], reinstallmode[11];
    WCHAR *ptr, *cmdline;
    DWORD sz;

    TRACE( "%s, %s, %#lx\n", debugstr_w(szProduct), debugstr_w(szFeature), dwReinstallMode );

    r = msi_locate_product( szProduct, &context );
    if (r != ERROR_SUCCESS)
        return r;

    ptr = reinstallmode;

    if (dwReinstallMode & REINSTALLMODE_FILEMISSING)
        *ptr++ = 'p';
    if (dwReinstallMode & REINSTALLMODE_FILEOLDERVERSION)
        *ptr++ = 'o';
    if (dwReinstallMode & REINSTALLMODE_FILEEQUALVERSION)
        *ptr++ = 'w';
    if (dwReinstallMode & REINSTALLMODE_FILEEXACT)
        *ptr++ = 'd';
    if (dwReinstallMode & REINSTALLMODE_FILEVERIFY)
        *ptr++ = 'c';
    if (dwReinstallMode & REINSTALLMODE_FILEREPLACE)
        *ptr++ = 'a';
    if (dwReinstallMode & REINSTALLMODE_USERDATA)
        *ptr++ = 'u';
    if (dwReinstallMode & REINSTALLMODE_MACHINEDATA)
        *ptr++ = 'm';
    if (dwReinstallMode & REINSTALLMODE_SHORTCUT)
        *ptr++ = 's';
    if (dwReinstallMode & REINSTALLMODE_PACKAGE)
        *ptr++ = 'v';
    *ptr = 0;

    sz = sizeof(sourcepath);
    MsiSourceListGetInfoW( szProduct, NULL, context, MSICODE_PRODUCT,
                           INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz );
    sz = sizeof(filename);
    MsiSourceListGetInfoW( szProduct, NULL, context, MSICODE_PRODUCT,
                           INSTALLPROPERTY_PACKAGENAMEW, filename, &sz );
    lstrcatW( sourcepath, filename );

    if (dwReinstallMode & REINSTALLMODE_PACKAGE)
        r = MSI_OpenPackageW( sourcepath, 0, &package );
    else
        r = MSI_OpenProductW( szProduct, &package );

    if (r != ERROR_SUCCESS)
        return r;

    sz = (lstrlenW( L"%s=%s %s=%s" ) + lstrlenW( L"REINSTALLMODE" ) + lstrlenW( reinstallmode )) * sizeof(WCHAR);
    sz += (lstrlenW( L"REINSTALL" ) + lstrlenW( szFeature )) * sizeof(WCHAR);
    if (!(cmdline = malloc( sz )))
    {
        msiobj_release( &package->hdr );
        return ERROR_OUTOFMEMORY;
    }
    swprintf( cmdline, sz / sizeof(WCHAR), L"%s=%s %s=%s", L"REINSTALLMODE", reinstallmode, L"REINSTALL", szFeature );

    r = MSI_InstallPackage( package, sourcepath, cmdline );
    msiobj_release( &package->hdr );
    free( cmdline );

    return r;
}

UINT WINAPI MsiReinstallFeatureA( const char *szProduct, const char *szFeature, DWORD dwReinstallMode )
{
    WCHAR *wszProduct, *wszFeature;
    UINT rc;

    TRACE( "%s, %s, %lu\n", debugstr_a(szProduct), debugstr_a(szFeature), dwReinstallMode );

    wszProduct = strdupAtoW(szProduct);
    wszFeature = strdupAtoW(szFeature);

    rc = MsiReinstallFeatureW(wszProduct, wszFeature, dwReinstallMode);

    free(wszProduct);
    free(wszFeature);
    return rc;
}

struct md5_ctx
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
};

extern void WINAPI MD5Init( struct md5_ctx * );
extern void WINAPI MD5Update( struct md5_ctx *, const unsigned char *, unsigned int );
extern void WINAPI MD5Final( struct md5_ctx * );

UINT msi_get_filehash( MSIPACKAGE *package, const WCHAR *path, MSIFILEHASHINFO *hash )
{
    HANDLE handle, mapping;
    void *p;
    DWORD length;
    UINT r = ERROR_FUNCTION_FAILED;

    if (package)
        handle = msi_create_file( package, path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE, OPEN_EXISTING, 0 );
    else
        handle = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
    {
        WARN( "can't open file %lu\n", GetLastError() );
        return ERROR_FILE_NOT_FOUND;
    }
    if ((length = GetFileSize( handle, NULL )))
    {
        if ((mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, 0, NULL )))
        {
            if ((p = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, length )))
            {
                struct md5_ctx ctx;

                MD5Init( &ctx );
                MD5Update( &ctx, p, length );
                MD5Final( &ctx );
                UnmapViewOfFile( p );

                memcpy( hash->dwData, ctx.digest, sizeof(hash->dwData) );
                r = ERROR_SUCCESS;
            }
            CloseHandle( mapping );
        }
    }
    else
    {
        /* Empty file -> set hash to 0 */
        memset( hash->dwData, 0, sizeof(hash->dwData) );
        r = ERROR_SUCCESS;
    }

    CloseHandle( handle );
    return r;
}

/***********************************************************************
 * MsiGetFileHashW            [MSI.@]
 */
UINT WINAPI MsiGetFileHashW( const WCHAR *szFilePath, DWORD dwOptions, MSIFILEHASHINFO *pHash )
{
    TRACE( "%s, %#lx, %p\n", debugstr_w(szFilePath), dwOptions, pHash );

    if (!szFilePath)
        return ERROR_INVALID_PARAMETER;

    if (!*szFilePath)
        return ERROR_PATH_NOT_FOUND;

    if (dwOptions)
        return ERROR_INVALID_PARAMETER;
    if (!pHash)
        return ERROR_INVALID_PARAMETER;
    if (pHash->dwFileHashInfoSize < sizeof *pHash)
        return ERROR_INVALID_PARAMETER;

    return msi_get_filehash( NULL, szFilePath, pHash );
}

/***********************************************************************
 * MsiGetFileHashA            [MSI.@]
 */
UINT WINAPI MsiGetFileHashA( const char *szFilePath, DWORD dwOptions, MSIFILEHASHINFO *pHash )
{
    LPWSTR file;
    UINT r;

    TRACE( "%s, %#lx, %p\n", debugstr_a(szFilePath), dwOptions, pHash );

    file = strdupAtoW( szFilePath );
    if (szFilePath && !file)
        return ERROR_OUTOFMEMORY;

    r = MsiGetFileHashW( file, dwOptions, pHash );
    free( file );
    return r;
}

/***********************************************************************
 * MsiAdvertiseScriptW        [MSI.@]
 */
UINT WINAPI MsiAdvertiseScriptW( const WCHAR *szScriptFile, DWORD dwFlags, HKEY *phRegData, BOOL fRemoveItems )
{
    FIXME( "%s, %#lx, %p, %d\n", debugstr_w(szScriptFile), dwFlags, phRegData, fRemoveItems );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiAdvertiseScriptA        [MSI.@]
 */
UINT WINAPI MsiAdvertiseScriptA( const char *szScriptFile, DWORD dwFlags, HKEY *phRegData, BOOL fRemoveItems )
{
    FIXME( "%s, %#lx, %p, %d\n", debugstr_a(szScriptFile), dwFlags, phRegData, fRemoveItems );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiIsProductElevatedW        [MSI.@]
 */
UINT WINAPI MsiIsProductElevatedW( LPCWSTR szProduct, BOOL *pfElevated )
{
    FIXME("%s %p - stub\n",
          debugstr_w( szProduct ), pfElevated );
    *pfElevated = TRUE;
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiIsProductElevatedA        [MSI.@]
 */
UINT WINAPI MsiIsProductElevatedA( LPCSTR szProduct, BOOL *pfElevated )
{
    FIXME("%s %p - stub\n",
          debugstr_a( szProduct ), pfElevated );
    *pfElevated = TRUE;
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiSetExternalUIRecord     [MSI.@]
 */
UINT WINAPI MsiSetExternalUIRecord( INSTALLUI_HANDLER_RECORD handler, DWORD filter, void *context,
                                    INSTALLUI_HANDLER_RECORD *prev )
{
    TRACE( "%p, %#lx, %p, %p\n", handler, filter, context, prev );

    if (prev)
        *prev = gUIHandlerRecord;

    gUIHandlerRecord = handler;
    gUIFilterRecord  = filter;
    gUIContextRecord = context;

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiInstallMissingComponentA     [MSI.@]
 */
UINT WINAPI MsiInstallMissingComponentA( LPCSTR product, LPCSTR component, INSTALLSTATE state )
{
    UINT r;
    WCHAR *productW = NULL, *componentW = NULL;

    TRACE("%s, %s, %d\n", debugstr_a(product), debugstr_a(component), state);

    if (product && !(productW = strdupAtoW( product )))
        return ERROR_OUTOFMEMORY;

    if (component && !(componentW = strdupAtoW( component )))
    {
        free( productW );
        return ERROR_OUTOFMEMORY;
    }

    r = MsiInstallMissingComponentW( productW, componentW, state );
    free( productW );
    free( componentW );
    return r;
}

/***********************************************************************
 * MsiInstallMissingComponentW     [MSI.@]
 */
UINT WINAPI MsiInstallMissingComponentW(LPCWSTR szProduct, LPCWSTR szComponent, INSTALLSTATE eInstallState)
{
    FIXME("(%s %s %d\n", debugstr_w(szProduct), debugstr_w(szComponent), eInstallState);
    return ERROR_SUCCESS;
}

UINT WINAPI MsiProvideComponentA( const char *product, const char *feature, const char *component, DWORD mode,
                                  char *buf, DWORD *buflen )
{
    WCHAR *productW = NULL, *componentW = NULL, *featureW = NULL, *bufW = NULL;
    UINT r = ERROR_OUTOFMEMORY;
    DWORD lenW = 0;
    int len;

    TRACE( "%s, %s, %s, %#lx, %p, %p\n", debugstr_a(product), debugstr_a(component), debugstr_a(feature), mode,
           buf, buflen );

    if (product && !(productW = strdupAtoW( product ))) goto done;
    if (feature && !(featureW = strdupAtoW( feature ))) goto done;
    if (component && !(componentW = strdupAtoW( component ))) goto done;

    r = MsiProvideComponentW( productW, featureW, componentW, mode, NULL, &lenW );
    if (r != ERROR_SUCCESS)
        goto done;

    if (!(bufW = malloc( ++lenW * sizeof(WCHAR) )))
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiProvideComponentW( productW, featureW, componentW, mode, bufW, &lenW );
    if (r != ERROR_SUCCESS)
        goto done;

    len = WideCharToMultiByte( CP_ACP, 0, bufW, -1, NULL, 0, NULL, NULL );
    if (buf)
    {
        if (len > *buflen)
            r = ERROR_MORE_DATA;
        else
            WideCharToMultiByte( CP_ACP, 0, bufW, -1, buf, *buflen, NULL, NULL );
    }

    *buflen = len - 1;

done:
    free( productW );
    free( featureW );
    free( componentW );
    free( bufW );
    return r;
}

UINT WINAPI MsiProvideComponentW( const WCHAR *product, const WCHAR *feature, const WCHAR *component, DWORD mode,
                                  WCHAR *buf, DWORD *buflen )
{
    INSTALLSTATE state;

    TRACE( "%s, %s, %s, %#lx, %p, %p\n", debugstr_w(product), debugstr_w(component), debugstr_w(feature), mode,
           buf, buflen);

    state = MsiQueryFeatureStateW( product, feature );
    TRACE("feature state: %d\n", state);
    switch (mode)
    {
    case INSTALLMODE_NODETECTION:
        break;

    default:
        FIXME( "mode %#lx not implemented\n", mode );
        return ERROR_INSTALL_FAILURE;
    }

    state = MsiGetComponentPathW( product, component, buf, buflen );
    TRACE("component state: %d\n", state);
    switch (state)
    {
    case INSTALLSTATE_INVALIDARG:
        return ERROR_INVALID_PARAMETER;

    case INSTALLSTATE_MOREDATA:
        return ERROR_MORE_DATA;

    case INSTALLSTATE_ADVERTISED:
    case INSTALLSTATE_LOCAL:
    case INSTALLSTATE_SOURCE:
        MsiUseFeatureW( product, feature );
        return ERROR_SUCCESS;

    default:
        TRACE("MsiGetComponentPathW returned %d\n", state);
        return ERROR_INSTALL_FAILURE;
    }
}

/***********************************************************************
 * MsiBeginTransactionA     [MSI.@]
 */
UINT WINAPI MsiBeginTransactionA( const char *name, DWORD attrs, MSIHANDLE *id, HANDLE *event )
{
    WCHAR *nameW;
    UINT r;

    FIXME( "%s, %#lx, %p, %p\n", debugstr_a(name), attrs, id, event );

    nameW = strdupAtoW( name );
    if (name && !nameW)
        return ERROR_OUTOFMEMORY;

    r = MsiBeginTransactionW( nameW, attrs, id, event );
    free( nameW );
    return r;
}

/***********************************************************************
 * MsiBeginTransactionW     [MSI.@]
 */
UINT WINAPI MsiBeginTransactionW( const WCHAR *name, DWORD attrs, MSIHANDLE *id, HANDLE *event )
{
    FIXME( "%s, %#lx, %p, %p\n", debugstr_w(name), attrs, id, event );

    *id = (MSIHANDLE)0xdeadbeef;
    *event = (HANDLE)0xdeadbeef;

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiJoinTransaction     [MSI.@]
 */
UINT WINAPI MsiJoinTransaction( MSIHANDLE handle, DWORD attrs, HANDLE *event )
{
    FIXME( "%lu, %#lx, %p\n", handle, attrs, event );

    *event = (HANDLE)0xdeadbeef;
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiEndTransaction     [MSI.@]
 */
UINT WINAPI MsiEndTransaction( DWORD state )
{
    FIXME( "%#lx\n", state );
    return ERROR_SUCCESS;
}

UINT WINAPI Migrate10CachedPackagesW( void *a, void *b, void *c, DWORD d )
{
    FIXME( "%p, %p, %p, %#lx\n", a, b, c, d );
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiRemovePatchesA     [MSI.@]
 */
UINT WINAPI MsiRemovePatchesA(LPCSTR patchlist, LPCSTR product, INSTALLTYPE type, LPCSTR propertylist)
{
    FIXME("(%s %s %d %s\n", debugstr_a(patchlist), debugstr_a(product), type, debugstr_a(propertylist));
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiRemovePatchesW    [MSI.@]
 */
UINT WINAPI MsiRemovePatchesW(LPCWSTR patchlist, LPCWSTR product, INSTALLTYPE type, LPCWSTR propertylist)
{
    FIXME("(%s %s %d %s\n", debugstr_w(patchlist), debugstr_w(product), type, debugstr_w(propertylist));
    return ERROR_SUCCESS;
}
