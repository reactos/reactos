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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msidefs.h"
#include "msiquery.h"
#include "msipriv.h"
#include "wincrypt.h"
#include "winver.h"
#include "winuser.h"
#include "shlobj.h"
#include "shobjidl.h"
#include "objidl.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static const WCHAR installerW[] = {'\\','I','n','s','t','a','l','l','e','r',0};

static UINT msi_locate_product(LPCWSTR szProduct, MSIINSTALLCONTEXT *context)
{
    HKEY hkey = NULL;

    *context = MSIINSTALLCONTEXT_NONE;

    if (MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERMANAGED,
                              &hkey, FALSE) == ERROR_SUCCESS)
        *context = MSIINSTALLCONTEXT_USERMANAGED;
    else if (MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_MACHINE,
                                   &hkey, FALSE) == ERROR_SUCCESS)
        *context = MSIINSTALLCONTEXT_MACHINE;
    else if (MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
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

    msi_free( szwProd );

    return r;
}

static UINT MSI_OpenProductW(LPCWSTR szProduct, MSIPACKAGE **package)
{
    UINT r;
    HKEY props;
    LPWSTR path;
    MSIINSTALLCONTEXT context;

    static const WCHAR managed[] = {
        'M','a','n','a','g','e','d','L','o','c','a','l','P','a','c','k','a','g','e',0};
    static const WCHAR local[] = {'L','o','c','a','l','P','a','c','k','a','g','e',0};

    TRACE("%s %p\n", debugstr_w(szProduct), package);

    r = msi_locate_product(szProduct, &context);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSIREG_OpenInstallProps(szProduct, context, NULL, &props, FALSE);
    if (r != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;

    if (context == MSIINSTALLCONTEXT_USERMANAGED)
        path = msi_reg_get_val_str(props, managed);
    else
        path = msi_reg_get_val_str(props, local);

    r = ERROR_UNKNOWN_PRODUCT;

    if (!path || GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
        goto done;

    if (PathIsRelativeW(path))
    {
        r = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        goto done;
    }

    r = MSI_OpenPackageW(path, package);

done:
    RegCloseKey(props);
    msi_free(path);
    return r;
}

UINT WINAPI MsiOpenProductW(LPCWSTR szProduct, MSIHANDLE *phProduct)
{
    MSIPACKAGE *package = NULL;
    WCHAR squished_pc[GUID_SIZE];
    UINT r;

    if (!szProduct || !squash_guid(szProduct, squished_pc))
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

UINT WINAPI MsiAdvertiseProductExA(LPCSTR szPackagePath, LPCSTR szScriptfilePath,
      LPCSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s %08x %08x %08x\n", debugstr_a(szPackagePath),
          debugstr_a(szScriptfilePath), debugstr_a(szTransforms),
          lgidLanguage, dwPlatform, dwOptions);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExW( LPCWSTR szPackagePath, LPCWSTR szScriptfilePath,
      LPCWSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s %08x %08x %08x\n", debugstr_w(szPackagePath),
          debugstr_w(szScriptfilePath), debugstr_w(szTransforms),
          lgidLanguage, dwPlatform, dwOptions);
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
    msi_free( szwPath );
    msi_free( szwCommand );

    return r;
}

UINT WINAPI MsiInstallProductW(LPCWSTR szPackagePath, LPCWSTR szCommandLine)
{
    MSIPACKAGE *package = NULL;
    UINT r;

    TRACE("%s %s\n",debugstr_w(szPackagePath), debugstr_w(szCommandLine));

    r = MSI_OpenPackageW( szPackagePath, &package );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_InstallPackage( package, szPackagePath, szCommandLine );
        msiobj_release( &package->hdr );
    }

    return r;
}

UINT WINAPI MsiReinstallProductA(LPCSTR szProduct, DWORD dwReinstallMode)
{
    FIXME("%s %08x\n", debugstr_a(szProduct), dwReinstallMode);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiReinstallProductW(LPCWSTR szProduct, DWORD dwReinstallMode)
{
    FIXME("%s %08x\n", debugstr_w(szProduct), dwReinstallMode);
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    msi_free(patch_package);
    msi_free(install_package);
    msi_free(command_line);

    return r;
}

UINT WINAPI MsiApplyPatchW(LPCWSTR szPatchPackage, LPCWSTR szInstallPackage,
         INSTALLTYPE eInstallType, LPCWSTR szCommandLine)
{
    MSIHANDLE patch, info;
    UINT r, type;
    DWORD size = 0;
    LPCWSTR cmd_ptr = szCommandLine;
    LPWSTR beg, end;
    LPWSTR cmd = NULL, codes = NULL;

    static const WCHAR space[] = {' ',0};
    static const WCHAR patcheq[] = {'P','A','T','C','H','=',0};
    static WCHAR empty[] = {0};

    TRACE("%s %s %d %s\n", debugstr_w(szPatchPackage), debugstr_w(szInstallPackage),
          eInstallType, debugstr_w(szCommandLine));

    if (szInstallPackage || eInstallType == INSTALLTYPE_NETWORK_IMAGE ||
        eInstallType == INSTALLTYPE_SINGLE_INSTANCE)
    {
        FIXME("Only reading target products from patch\n");
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    r = MsiOpenDatabaseW(szPatchPackage, MSIDBOPEN_READONLY, &patch);
    if (r != ERROR_SUCCESS)
        return r;

    r = MsiGetSummaryInformationW(patch, NULL, 0, &info);
    if (r != ERROR_SUCCESS)
        goto done;

    r = MsiSummaryInfoGetPropertyW(info, PID_TEMPLATE, &type, NULL, NULL, empty, &size);
    if (r != ERROR_MORE_DATA || !size || type != VT_LPSTR)
    {
        ERR("Failed to read product codes from patch\n");
        goto done;
    }

    codes = msi_alloc(++size * sizeof(WCHAR));
    if (!codes)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = MsiSummaryInfoGetPropertyW(info, PID_TEMPLATE, &type, NULL, NULL, codes, &size);
    if (r != ERROR_SUCCESS)
        goto done;

    if (!szCommandLine)
        cmd_ptr = empty;

    size = lstrlenW(cmd_ptr) + lstrlenW(patcheq) + lstrlenW(szPatchPackage) + 1;
    cmd = msi_alloc(size * sizeof(WCHAR));
    if (!cmd)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    lstrcpyW(cmd, cmd_ptr);
    if (szCommandLine) lstrcatW(cmd, space);
    lstrcatW(cmd, patcheq);
    lstrcatW(cmd, szPatchPackage);

    beg = codes;
    while ((end = strchrW(beg, '}')))
    {
        *(end + 1) = '\0';

        r = MsiConfigureProductExW(beg, INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT, cmd);
        if (r != ERROR_SUCCESS)
            goto done;

        beg = end + 2;
    }

done:
    msi_free(cmd);
    msi_free(codes);

    MsiCloseHandle(info);
    MsiCloseHandle(patch);

    return r;
}

UINT WINAPI MsiDetermineApplicablePatchesA(LPCSTR szProductPackagePath,
        DWORD cPatchInfo, PMSIPATCHSEQUENCEINFOA pPatchInfo)
{
    FIXME("(%s, %d, %p): stub!\n", debugstr_a(szProductPackagePath),
          cPatchInfo, pPatchInfo);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDetermineApplicablePatchesW(LPCWSTR szProductPackagePath,
        DWORD cPatchInfo, PMSIPATCHSEQUENCEINFOW pPatchInfo)
{
    FIXME("(%s, %d, %p): stub!\n", debugstr_w(szProductPackagePath),
          cPatchInfo, pPatchInfo);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

static UINT msi_open_package(LPCWSTR product, MSIINSTALLCONTEXT context,
                             MSIPACKAGE **package)
{
    UINT r;
    DWORD sz;
    HKEY props;
    LPWSTR localpack;
    WCHAR sourcepath[MAX_PATH];
    WCHAR filename[MAX_PATH];

    static const WCHAR szLocalPackage[] = {
        'L','o','c','a','l','P','a','c','k','a','g','e',0};


    r = MSIREG_OpenInstallProps(product, context, NULL, &props, FALSE);
    if (r != ERROR_SUCCESS)
        return ERROR_BAD_CONFIGURATION;

    localpack = msi_reg_get_val_str(props, szLocalPackage);
    if (localpack)
    {
        lstrcpyW(sourcepath, localpack);
        msi_free(localpack);
    }

    if (!localpack || GetFileAttributesW(sourcepath) == INVALID_FILE_ATTRIBUTES)
    {
        sz = sizeof(sourcepath);
        MsiSourceListGetInfoW(product, NULL, context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz);

        sz = sizeof(filename);
        MsiSourceListGetInfoW(product, NULL, context, MSICODE_PRODUCT,
                              INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

        lstrcatW(sourcepath, filename);
    }

    if (GetFileAttributesW(sourcepath) == INVALID_FILE_ATTRIBUTES)
        return ERROR_INSTALL_SOURCE_ABSENT;

    return MSI_OpenPackageW(sourcepath, package);
}

UINT WINAPI MsiConfigureProductExW(LPCWSTR szProduct, int iInstallLevel,
                        INSTALLSTATE eInstallState, LPCWSTR szCommandLine)
{
    MSIPACKAGE* package = NULL;
    MSIINSTALLCONTEXT context;
    UINT r;
    DWORD sz;
    WCHAR sourcepath[MAX_PATH];
    LPWSTR commandline;

    static const WCHAR szInstalled[] = {
        ' ','I','n','s','t','a','l','l','e','d','=','1',0};
    static const WCHAR szRemoveAll[] = {
        ' ','R','E','M','O','V','E','=','A','L','L',0};
    static const WCHAR szMachine[] = {
        ' ','A','L','L','U','S','E','R','S','=','1',0};

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

    r = msi_open_package(szProduct, context, &package);
    if (r != ERROR_SUCCESS)
        return r;

    sz = lstrlenW(szInstalled) + 1;

    if (szCommandLine)
        sz += lstrlenW(szCommandLine);

    if (eInstallState == INSTALLSTATE_ABSENT)
        sz += lstrlenW(szRemoveAll);

    if (context == MSIINSTALLCONTEXT_MACHINE)
        sz += lstrlenW(szMachine);

    commandline = msi_alloc(sz * sizeof(WCHAR));
    if (!commandline)
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    commandline[0] = 0;
    if (szCommandLine)
        lstrcpyW(commandline,szCommandLine);

    if (MsiQueryProductStateW(szProduct) != INSTALLSTATE_UNKNOWN)
        lstrcatW(commandline,szInstalled);

    if (eInstallState == INSTALLSTATE_ABSENT)
        lstrcatW(commandline, szRemoveAll);

    if (context == MSIINSTALLCONTEXT_MACHINE)
        lstrcatW(commandline, szMachine);

    r = MSI_InstallPackage( package, sourcepath, commandline );

    msi_free(commandline);

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
    msi_free( szwProduct );
    msi_free( szwCommandLine);

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
    msi_free( szwProduct );

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

    TRACE("%s %s\n",debugstr_a(szComponent), debugstr_a(szBuffer));

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

    msi_free( szwComponent );

    return r;
}

UINT WINAPI MsiGetProductCodeW(LPCWSTR szComponent, LPWSTR szBuffer)
{
    UINT rc, index;
    HKEY compkey, prodkey;
    WCHAR squished_comp[GUID_SIZE];
    WCHAR squished_prod[GUID_SIZE];
    DWORD sz = GUID_SIZE;

    TRACE("%s %p\n", debugstr_w(szComponent), szBuffer);

    if (!szComponent || !*szComponent)
        return ERROR_INVALID_PARAMETER;

    if (!squash_guid(szComponent, squished_comp))
        return ERROR_INVALID_PARAMETER;

    if (MSIREG_OpenUserDataComponentKey(szComponent, NULL, &compkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenUserDataComponentKey(szComponent, szLocalSid, &compkey, FALSE) != ERROR_SUCCESS)
    {
        return ERROR_UNKNOWN_COMPONENT;
    }

    rc = RegEnumValueW(compkey, 0, squished_prod, &sz, NULL, NULL, NULL, NULL);
    if (rc != ERROR_SUCCESS)
    {
        RegCloseKey(compkey);
        return ERROR_UNKNOWN_COMPONENT;
    }

    /* check simple case, only one product */
    rc = RegEnumValueW(compkey, 1, squished_prod, &sz, NULL, NULL, NULL, NULL);
    if (rc == ERROR_NO_MORE_ITEMS)
    {
        rc = ERROR_SUCCESS;
        goto done;
    }

    index = 0;
    while ((rc = RegEnumValueW(compkey, index, squished_prod, &sz,
           NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS)
    {
        index++;
        sz = GUID_SIZE;
        unsquash_guid(squished_prod, szBuffer);

        if (MSIREG_OpenProductKey(szBuffer, MSIINSTALLCONTEXT_USERMANAGED,
                                  &prodkey, FALSE) == ERROR_SUCCESS ||
            MSIREG_OpenProductKey(szBuffer, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  &prodkey, FALSE) == ERROR_SUCCESS ||
            MSIREG_OpenProductKey(szBuffer, MSIINSTALLCONTEXT_MACHINE,
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
    unsquash_guid(squished_prod, szBuffer);
    return rc;
}

static LPWSTR msi_reg_get_value(HKEY hkey, LPCWSTR name, DWORD *type)
{
    DWORD dval;
    LONG res;
    WCHAR temp[20];

    static const WCHAR format[] = {'%','d',0};

    res = RegQueryValueExW(hkey, name, NULL, type, NULL, NULL);
    if (res != ERROR_SUCCESS)
        return NULL;

    if (*type == REG_SZ)
        return msi_reg_get_val_str(hkey, name);

    if (!msi_reg_get_val_dword(hkey, name, &dval))
        return NULL;

    sprintfW(temp, format, dval);
    return strdupW(temp);
}

static UINT MSI_GetProductInfo(LPCWSTR szProduct, LPCWSTR szAttribute,
                               awstring *szValue, LPDWORD pcchValueBuf)
{
    MSIINSTALLCONTEXT context = MSIINSTALLCONTEXT_USERUNMANAGED;
    UINT r = ERROR_UNKNOWN_PROPERTY;
    HKEY prodkey, userdata, source;
    LPWSTR val = NULL;
    WCHAR squished_pc[GUID_SIZE];
    WCHAR packagecode[GUID_SIZE];
    BOOL badconfig = FALSE;
    LONG res;
    DWORD save, type = REG_NONE;

    static WCHAR empty[] = {0};
    static const WCHAR sourcelist[] = {
        'S','o','u','r','c','e','L','i','s','t',0};
    static const WCHAR display_name[] = {
        'D','i','s','p','l','a','y','N','a','m','e',0};
    static const WCHAR display_version[] = {
        'D','i','s','p','l','a','y','V','e','r','s','i','o','n',0};
    static const WCHAR assignment[] = {
        'A','s','s','i','g','n','m','e','n','t',0};

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
          debugstr_w(szAttribute), szValue, pcchValueBuf);

    if ((szValue->str.w && !pcchValueBuf) || !szProduct || !szAttribute)
        return ERROR_INVALID_PARAMETER;

    if (!squash_guid(szProduct, squished_pc))
        return ERROR_INVALID_PARAMETER;

    if ((r = MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERMANAGED,
                                   &prodkey, FALSE)) != ERROR_SUCCESS &&
        (r = MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   &prodkey, FALSE)) != ERROR_SUCCESS &&
        (r = MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_MACHINE,
                                      &prodkey, FALSE)) == ERROR_SUCCESS)
    {
        context = MSIINSTALLCONTEXT_MACHINE;
    }

    MSIREG_OpenInstallProps(szProduct, context, NULL, &userdata, FALSE);

    if (!lstrcmpW(szAttribute, INSTALLPROPERTY_HELPLINKW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_HELPTELEPHONEW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_INSTALLDATEW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_INSTALLLOCATIONW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_INSTALLSOURCEW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_LOCALPACKAGEW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_PUBLISHERW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_URLINFOABOUTW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_URLUPDATEINFOW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_VERSIONMINORW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_VERSIONMAJORW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_VERSIONSTRINGW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_PRODUCTIDW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_REGCOMPANYW) ||
        !lstrcmpW(szAttribute, INSTALLPROPERTY_REGOWNERW))
    {
        if (!prodkey)
        {
            r = ERROR_UNKNOWN_PRODUCT;
            goto done;
        }

        if (!userdata)
            return ERROR_UNKNOWN_PROPERTY;

        if (!lstrcmpW(szAttribute, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW))
            szAttribute = display_name;
        else if (!lstrcmpW(szAttribute, INSTALLPROPERTY_VERSIONSTRINGW))
            szAttribute = display_version;

        val = msi_reg_get_value(userdata, szAttribute, &type);
        if (!val)
            val = empty;
    }
    else if (!lstrcmpW(szAttribute, INSTALLPROPERTY_INSTANCETYPEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_TRANSFORMSW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_LANGUAGEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_PRODUCTNAMEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_ASSIGNMENTTYPEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_PACKAGECODEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_VERSIONW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_PRODUCTICONW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_PACKAGENAMEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_AUTHORIZED_LUA_APPW))
    {
        if (!prodkey)
        {
            r = ERROR_UNKNOWN_PRODUCT;
            goto done;
        }

        if (!lstrcmpW(szAttribute, INSTALLPROPERTY_ASSIGNMENTTYPEW))
            szAttribute = assignment;

        if (!lstrcmpW(szAttribute, INSTALLPROPERTY_PACKAGENAMEW))
        {
            res = RegOpenKeyW(prodkey, sourcelist, &source);
            if (res == ERROR_SUCCESS)
                val = msi_reg_get_value(source, szAttribute, &type);

            RegCloseKey(source);
        }
        else
        {
            val = msi_reg_get_value(prodkey, szAttribute, &type);
            if (!val)
                val = empty;
        }

        if (val != empty && type != REG_DWORD &&
            !lstrcmpW(szAttribute, INSTALLPROPERTY_PACKAGECODEW))
        {
            if (lstrlenW(val) != SQUISH_GUID_SIZE - 1)
                badconfig = TRUE;
            else
            {
                unsquash_guid(val, packagecode);
                msi_free(val);
                val = strdupW(packagecode);
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
        save = *pcchValueBuf;

        if (strlenW(val) < *pcchValueBuf)
            r = msi_strcpy_to_awstring(val, szValue, pcchValueBuf);
        else if (szValue->str.a || szValue->str.w)
            r = ERROR_MORE_DATA;

        if (!badconfig)
            *pcchValueBuf = lstrlenW(val);
        else if (r == ERROR_SUCCESS)
        {
            *pcchValueBuf = save;
            r = ERROR_BAD_CONFIGURATION;
        }
    }
    else if (badconfig)
        r = ERROR_BAD_CONFIGURATION;

    if (val != empty)
        msi_free(val);

done:
    RegCloseKey(prodkey);
    RegCloseKey(userdata);
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
    msi_free( szwProduct );
    msi_free( szwAttribute );

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

    value = msi_alloc(++len * sizeof(WCHAR));
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
    msi_free(product);
    msi_free(usersid);
    msi_free(property);
    msi_free(value);

    return r;
}

static UINT msi_copy_outval(LPWSTR val, LPWSTR out, LPDWORD size)
{
    UINT r;

    if (!val)
        return ERROR_UNKNOWN_PROPERTY;

    if (out)
    {
        if (strlenW(val) >= *size)
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

    return ERROR_SUCCESS;
}

UINT WINAPI MsiGetProductInfoExW(LPCWSTR szProductCode, LPCWSTR szUserSid,
                                 MSIINSTALLCONTEXT dwContext, LPCWSTR szProperty,
                                 LPWSTR szValue, LPDWORD pcchValue)
{
    WCHAR squished_pc[GUID_SIZE];
    LPWSTR val = NULL;
    LPCWSTR package = NULL;
    HKEY props = NULL, prod;
    HKEY classes = NULL, managed;
    HKEY hkey = NULL;
    DWORD type;
    UINT r = ERROR_UNKNOWN_PRODUCT;

    static const WCHAR one[] = {'1',0};
    static const WCHAR five[] = {'5',0};
    static const WCHAR empty[] = {0};
    static const WCHAR displayname[] = {
        'D','i','s','p','l','a','y','N','a','m','e',0};
    static const WCHAR displayversion[] = {
        'D','i','s','p','l','a','y','V','e','r','s','i','o','n',0};
    static const WCHAR managed_local_package[] = {
        'M','a','n','a','g','e','d','L','o','c','a','l',
        'P','a','c','k','a','g','e',0};

    TRACE("(%s, %s, %d, %s, %p, %p)\n", debugstr_w(szProductCode),
          debugstr_w(szUserSid), dwContext, debugstr_w(szProperty),
           szValue, pcchValue);

    if (!szProductCode || !squash_guid(szProductCode, squished_pc))
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
    MSIREG_OpenProductKey(szProductCode, MSIINSTALLCONTEXT_USERMANAGED,
                          &managed, FALSE);
    MSIREG_OpenProductKey(szProductCode, MSIINSTALLCONTEXT_USERUNMANAGED,
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
        package = managed_local_package;

        if (!props && !managed)
            goto done;
    }
    else if (dwContext == MSIINSTALLCONTEXT_MACHINE)
    {
        package = INSTALLPROPERTY_LOCALPACKAGEW;
        MSIREG_OpenProductKey(szProductCode, dwContext, &classes, FALSE);

        if (!props && !classes)
            goto done;
    }

    if (!lstrcmpW(szProperty, INSTALLPROPERTY_HELPLINKW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_HELPTELEPHONEW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_INSTALLDATEW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_INSTALLLOCATIONW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_INSTALLSOURCEW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_LOCALPACKAGEW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_PUBLISHERW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_URLINFOABOUTW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_URLUPDATEINFOW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_VERSIONMINORW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_VERSIONMAJORW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_VERSIONSTRINGW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_PRODUCTIDW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_REGCOMPANYW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_REGOWNERW) ||
        !lstrcmpW(szProperty, INSTALLPROPERTY_INSTANCETYPEW))
    {
        val = msi_reg_get_value(props, package, &type);
        if (!val)
        {
            if (prod || classes)
                r = ERROR_UNKNOWN_PROPERTY;

            goto done;
        }

        msi_free(val);

        if (!lstrcmpW(szProperty, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW))
            szProperty = displayname;
        else if (!lstrcmpW(szProperty, INSTALLPROPERTY_VERSIONSTRINGW))
            szProperty = displayversion;

        val = msi_reg_get_value(props, szProperty, &type);
        if (!val)
            val = strdupW(empty);

        r = msi_copy_outval(val, szValue, pcchValue);
    }
    else if (!lstrcmpW(szProperty, INSTALLPROPERTY_TRANSFORMSW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_LANGUAGEW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_PRODUCTNAMEW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_PACKAGECODEW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_VERSIONW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_PRODUCTICONW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_PACKAGENAMEW) ||
             !lstrcmpW(szProperty, INSTALLPROPERTY_AUTHORIZED_LUA_APPW))
    {
        if (!prod && !classes)
            goto done;

        if (dwContext == MSIINSTALLCONTEXT_USERUNMANAGED)
            hkey = prod;
        else if (dwContext == MSIINSTALLCONTEXT_USERMANAGED)
            hkey = managed;
        else if (dwContext == MSIINSTALLCONTEXT_MACHINE)
            hkey = classes;

        val = msi_reg_get_value(hkey, szProperty, &type);
        if (!val)
            val = strdupW(empty);

        r = msi_copy_outval(val, szValue, pcchValue);
    }
    else if (!lstrcmpW(szProperty, INSTALLPROPERTY_PRODUCTSTATEW))
    {
        if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        {
            if (props)
            {
                val = msi_reg_get_value(props, package, &type);
                if (!val)
                    goto done;

                msi_free(val);
                val = strdupW(five);
            }
            else
                val = strdupW(one);

            r = msi_copy_outval(val, szValue, pcchValue);
            goto done;
        }
        else if (props && (val = msi_reg_get_value(props, package, &type)))
        {
            msi_free(val);
            val = strdupW(five);
            r = msi_copy_outval(val, szValue, pcchValue);
            goto done;
        }

        if (prod || managed)
            val = strdupW(one);
        else
            goto done;

        r = msi_copy_outval(val, szValue, pcchValue);
    }
    else if (!lstrcmpW(szProperty, INSTALLPROPERTY_ASSIGNMENTTYPEW))
    {
        if (!prod && !classes)
            goto done;

        /* FIXME */
        val = strdupW(empty);
        r = msi_copy_outval(val, szValue, pcchValue);
    }
    else
        r = ERROR_UNKNOWN_PROPERTY;

done:
    RegCloseKey(props);
    RegCloseKey(prod);
    RegCloseKey(managed);
    RegCloseKey(classes);
    msi_free(val);

    return r;
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

    val = msi_alloc(++len * sizeof(WCHAR));
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
    msi_free(val);
    msi_free(patch);
    msi_free(product);
    msi_free(usersid);
    msi_free(property);

    return r;
}

UINT WINAPI MsiGetPatchInfoExW(LPCWSTR szPatchCode, LPCWSTR szProductCode,
                               LPCWSTR szUserSid, MSIINSTALLCONTEXT dwContext,
                               LPCWSTR szProperty, LPWSTR lpValue, DWORD *pcchValue)
{
    WCHAR squished_pc[GUID_SIZE];
    WCHAR squished_patch[GUID_SIZE];
    HKEY udprod = 0, prod = 0, props = 0;
    HKEY patch = 0, patches = 0;
    HKEY udpatch = 0, datakey = 0;
    HKEY prodpatches = 0;
    LPWSTR val = NULL;
    UINT r = ERROR_UNKNOWN_PRODUCT;
    DWORD len;
    LONG res;

    static const WCHAR szEmpty[] = {0};
    static const WCHAR szPatches[] = {'P','a','t','c','h','e','s',0};
    static const WCHAR szInstalled[] = {'I','n','s','t','a','l','l','e','d',0};
    static const WCHAR szManagedPackage[] = {'M','a','n','a','g','e','d',
        'L','o','c','a','l','P','a','c','k','a','g','e',0};

    TRACE("(%s, %s, %s, %d, %s, %p, %p)\n", debugstr_w(szPatchCode),
          debugstr_w(szProductCode), debugstr_w(szUserSid), dwContext,
          debugstr_w(szProperty), lpValue, pcchValue);

    if (!szProductCode || !squash_guid(szProductCode, squished_pc))
        return ERROR_INVALID_PARAMETER;

    if (!szPatchCode || !squash_guid(szPatchCode, squished_patch))
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

    if (!lstrcmpW(szUserSid, szLocalSid))
        return ERROR_INVALID_PARAMETER;

    if (MSIREG_OpenUserDataProductKey(szProductCode, dwContext, NULL,
                                      &udprod, FALSE) != ERROR_SUCCESS)
        goto done;

    if (MSIREG_OpenInstallProps(szProductCode, dwContext, NULL,
                                &props, FALSE) != ERROR_SUCCESS)
        goto done;

    r = ERROR_UNKNOWN_PATCH;

    res = RegOpenKeyExW(udprod, szPatches, 0, KEY_READ, &patches);
    if (res != ERROR_SUCCESS)
        goto done;

    res = RegOpenKeyExW(patches, squished_patch, 0, KEY_READ, &patch);
    if (res != ERROR_SUCCESS)
        goto done;

    if (!lstrcmpW(szProperty, INSTALLPROPERTY_TRANSFORMSW))
    {
        if (MSIREG_OpenProductKey(szProductCode, dwContext,
                                  &prod, FALSE) != ERROR_SUCCESS)
            goto done;

        res = RegOpenKeyExW(prod, szPatches, 0, KEY_ALL_ACCESS, &prodpatches);
        if (res != ERROR_SUCCESS)
            goto done;

        datakey = prodpatches;
        szProperty = squished_patch;
    }
    else
    {
        if (MSIREG_OpenUserDataPatchKey(szPatchCode, dwContext,
                                        &udpatch, FALSE) != ERROR_SUCCESS)
            goto done;

        if (!lstrcmpW(szProperty, INSTALLPROPERTY_LOCALPACKAGEW))
        {
            if (dwContext == MSIINSTALLCONTEXT_USERMANAGED)
                szProperty = szManagedPackage;
            datakey = udpatch;
        }
        else if (!lstrcmpW(szProperty, INSTALLPROPERTY_INSTALLDATEW))
        {
            datakey = patch;
            szProperty = szInstalled;
        }
        else if (!lstrcmpW(szProperty, INSTALLPROPERTY_LOCALPACKAGEW))
        {
            datakey = udpatch;
        }
        else if (!lstrcmpW(szProperty, INSTALLPROPERTY_UNINSTALLABLEW) ||
                 !lstrcmpW(szProperty, INSTALLPROPERTY_PATCHSTATEW) ||
                 !lstrcmpW(szProperty, INSTALLPROPERTY_DISPLAYNAMEW) ||
                 !lstrcmpW(szProperty, INSTALLPROPERTY_MOREINFOURLW))
        {
            datakey = patch;
        }
        else
        {
            r = ERROR_UNKNOWN_PROPERTY;
            goto done;
        }
    }

    val = msi_reg_get_val_str(datakey, szProperty);
    if (!val)
        val = strdupW(szEmpty);

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
    msi_free(val);
    RegCloseKey(prodpatches);
    RegCloseKey(prod);
    RegCloseKey(patch);
    RegCloseKey(patches);
    RegCloseKey(udpatch);
    RegCloseKey(props);
    RegCloseKey(udprod);

    return r;
}

UINT WINAPI MsiEnableLogA(DWORD dwLogMode, LPCSTR szLogFile, DWORD attributes)
{
    LPWSTR szwLogFile = NULL;
    UINT r;

    TRACE("%08x %s %08x\n", dwLogMode, debugstr_a(szLogFile), attributes);

    if( szLogFile )
    {
        szwLogFile = strdupAtoW( szLogFile );
        if( !szwLogFile )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiEnableLogW( dwLogMode, szwLogFile, attributes );
    msi_free( szwLogFile );
    return r;
}

UINT WINAPI MsiEnableLogW(DWORD dwLogMode, LPCWSTR szLogFile, DWORD attributes)
{
    HANDLE file = INVALID_HANDLE_VALUE;

    TRACE("%08x %s %08x\n", dwLogMode, debugstr_w(szLogFile), attributes);

    if (szLogFile)
    {
        lstrcpyW(gszLogFile,szLogFile);
        if (!(attributes & INSTALLLOGATTRIBUTES_APPEND))
            DeleteFileW(szLogFile);
        file = CreateFileW(szLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
        else
            ERR("Unable to enable log %s\n",debugstr_w(szLogFile));
    }
    else
        gszLogFile[0] = '\0';

    return ERROR_SUCCESS;
}

UINT WINAPI MsiEnumComponentCostsW(MSIHANDLE hInstall, LPCWSTR szComponent,
                                   DWORD dwIndex, INSTALLSTATE iState,
                                   LPWSTR lpDriveBuf, DWORD *pcchDriveBuf,
                                   int *piCost, int *pTempCost)
{
    FIXME("(%d, %s, %d, %d, %p, %p, %p %p): stub!\n", hInstall,
          debugstr_w(szComponent), dwIndex, iState, lpDriveBuf,
          pcchDriveBuf, piCost, pTempCost);

    return ERROR_NO_MORE_ITEMS;
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

    msi_free(prodcode);
    msi_free(usersid);
    msi_free(comp);

    return r;
}

static BOOL msi_comp_find_prod_key(LPCWSTR prodcode, MSIINSTALLCONTEXT context)
{
    UINT r;
    HKEY hkey;

    r = MSIREG_OpenProductKey(prodcode, context, &hkey, FALSE);
    RegCloseKey(hkey);
    return (r == ERROR_SUCCESS);
}

static BOOL msi_comp_find_package(LPCWSTR prodcode, MSIINSTALLCONTEXT context)
{
    LPCWSTR package;
    HKEY hkey;
    DWORD sz;
    LONG res;
    UINT r;

    static const WCHAR local_package[] = {'L','o','c','a','l','P','a','c','k','a','g','e',0};
    static const WCHAR managed_local_package[] = {
        'M','a','n','a','g','e','d','L','o','c','a','l','P','a','c','k','a','g','e',0
    };

    r = MSIREG_OpenInstallProps(prodcode, context, NULL, &hkey, FALSE);
    if (r != ERROR_SUCCESS)
        return FALSE;

    if (context == MSIINSTALLCONTEXT_USERMANAGED)
        package = managed_local_package;
    else
        package = local_package;

    sz = 0;
    res = RegQueryValueExW(hkey, package, NULL, NULL, NULL, &sz);
    RegCloseKey(hkey);

    return (res == ERROR_SUCCESS);
}

static BOOL msi_comp_find_prodcode(LPWSTR squished_pc,
                                   MSIINSTALLCONTEXT context,
                                   LPCWSTR comp, LPWSTR val, DWORD *sz)
{
    HKEY hkey;
    LONG res;
    UINT r;

    if (context == MSIINSTALLCONTEXT_MACHINE)
        r = MSIREG_OpenUserDataComponentKey(comp, szLocalSid, &hkey, FALSE);
    else
        r = MSIREG_OpenUserDataComponentKey(comp, NULL, &hkey, FALSE);

    if (r != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, squished_pc, NULL, NULL, (BYTE *)val, sz);
    if (res != ERROR_SUCCESS)
        return FALSE;

    RegCloseKey(hkey);
    return TRUE;
}

UINT WINAPI MsiQueryComponentStateW(LPCWSTR szProductCode,
                                    LPCWSTR szUserSid, MSIINSTALLCONTEXT dwContext,
                                    LPCWSTR szComponent, INSTALLSTATE *pdwState)
{
    WCHAR squished_pc[GUID_SIZE];
    WCHAR val[MAX_PATH];
    BOOL found;
    DWORD sz;

    TRACE("(%s, %s, %d, %s, %p)\n", debugstr_w(szProductCode),
          debugstr_w(szUserSid), dwContext, debugstr_w(szComponent), pdwState);

    if (!pdwState)
        return ERROR_INVALID_PARAMETER;

    if (!szProductCode || !*szProductCode || lstrlenW(szProductCode) != GUID_SIZE - 1)
        return ERROR_INVALID_PARAMETER;

    if (!squash_guid(szProductCode, squished_pc))
        return ERROR_INVALID_PARAMETER;

    found = msi_comp_find_prod_key(szProductCode, dwContext);

    if (!msi_comp_find_package(szProductCode, dwContext))
    {
        if (found)
        {
            *pdwState = INSTALLSTATE_UNKNOWN;
            return ERROR_UNKNOWN_COMPONENT;
        }

        return ERROR_UNKNOWN_PRODUCT;
    }

    *pdwState = INSTALLSTATE_UNKNOWN;

    sz = MAX_PATH;
    if (!msi_comp_find_prodcode(squished_pc, dwContext, szComponent, val, &sz))
        return ERROR_UNKNOWN_COMPONENT;

    if (sz == 0)
        *pdwState = INSTALLSTATE_NOTUSED;
    else
    {
        if (lstrlenW(val) > 2 &&
            val[0] >= '0' && val[0] <= '9' && val[1] >= '0' && val[1] <= '9')
        {
            *pdwState = INSTALLSTATE_SOURCE;
        }
        else
            *pdwState = INSTALLSTATE_LOCAL;
    }

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
    msi_free( szwProduct );
    return r;
}

INSTALLSTATE WINAPI MsiQueryProductStateW(LPCWSTR szProduct)
{
    MSIINSTALLCONTEXT context = MSIINSTALLCONTEXT_USERUNMANAGED;
    INSTALLSTATE state = INSTALLSTATE_ADVERTISED;
    HKEY prodkey = 0, userdata = 0;
    DWORD val;
    UINT r;

    static const WCHAR szWindowsInstaller[] = {
        'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0};

    TRACE("%s\n", debugstr_w(szProduct));

    if (!szProduct || !*szProduct)
        return INSTALLSTATE_INVALIDARG;

    if (lstrlenW(szProduct) != GUID_SIZE - 1)
        return INSTALLSTATE_INVALIDARG;

    if (MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERMANAGED,
                              &prodkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                              &prodkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_MACHINE,
                              &prodkey, FALSE) == ERROR_SUCCESS)
    {
        context = MSIINSTALLCONTEXT_MACHINE;
    }

    r = MSIREG_OpenInstallProps(szProduct, context, NULL, &userdata, FALSE);
    if (r != ERROR_SUCCESS)
        goto done;

    if (!msi_reg_get_val_dword(userdata, szWindowsInstaller, &val))
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
    return state;
}

INSTALLUILEVEL WINAPI MsiSetInternalUI(INSTALLUILEVEL dwUILevel, HWND *phWnd)
{
    INSTALLUILEVEL old = gUILevel;
    HWND oldwnd = gUIhwnd;

    TRACE("%08x %p\n", dwUILevel, phWnd);

    gUILevel = dwUILevel;
    if (phWnd)
    {
        gUIhwnd = *phWnd;
        *phWnd = oldwnd;
    }
    return old;
}

INSTALLUI_HANDLERA WINAPI MsiSetExternalUIA(INSTALLUI_HANDLERA puiHandler,
                                  DWORD dwMessageFilter, LPVOID pvContext)
{
    INSTALLUI_HANDLERA prev = gUIHandlerA;

    TRACE("%p %x %p\n",puiHandler, dwMessageFilter,pvContext);
    gUIHandlerA = puiHandler;
    gUIFilter = dwMessageFilter;
    gUIContext = pvContext;

    return prev;
}

INSTALLUI_HANDLERW WINAPI MsiSetExternalUIW(INSTALLUI_HANDLERW puiHandler,
                                  DWORD dwMessageFilter, LPVOID pvContext)
{
    INSTALLUI_HANDLERW prev = gUIHandlerW;

    TRACE("%p %x %p\n",puiHandler,dwMessageFilter,pvContext);
    gUIHandlerW = puiHandler;
    gUIFilter = dwMessageFilter;
    gUIContext = pvContext;

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
LANGID WINAPI MsiLoadStringW( MSIHANDLE handle, UINT id, LPWSTR lpBuffer,
                int nBufferMax, LANGID lang )
{
    HRSRC hres;
    HGLOBAL hResData;
    LPWSTR p;
    DWORD i, len;

    TRACE("%d %u %p %d %d\n", handle, id, lpBuffer, nBufferMax, lang);

    if( handle != -1 )
        FIXME("don't know how to deal with handle = %08x\n", handle);

    if( !lang )
        lang = GetUserDefaultLangID();

    hres = FindResourceExW( msi_hInstance, (LPCWSTR) RT_STRING,
                            (LPWSTR)1, lang );
    if( !hres )
        return 0;
    hResData = LoadResource( msi_hInstance, hres );
    if( !hResData )
        return 0;
    p = LockResource( hResData );
    if( !p )
        return 0;

    for (i = 0; i < (id&0xf); i++)
	p += *p + 1;
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

    bufW = msi_alloc(nBufferMax*sizeof(WCHAR));
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
    msi_free(bufW);
    return r;
}

INSTALLSTATE WINAPI MsiLocateComponentA(LPCSTR szComponent, LPSTR lpPathBuf,
                LPDWORD pcchBuf)
{
    char szProduct[GUID_SIZE];

    TRACE("%s %p %p\n", debugstr_a(szComponent), lpPathBuf, pcchBuf);

    if (MsiGetProductCodeA( szComponent, szProduct ) != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    return MsiGetComponentPathA( szProduct, szComponent, lpPathBuf, pcchBuf );
}

INSTALLSTATE WINAPI MsiLocateComponentW(LPCWSTR szComponent, LPWSTR lpPathBuf,
                LPDWORD pcchBuf)
{
    WCHAR szProduct[GUID_SIZE];

    TRACE("%s %p %p\n", debugstr_w(szComponent), lpPathBuf, pcchBuf);

    if (MsiGetProductCodeW( szComponent, szProduct ) != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    return MsiGetComponentPathW( szProduct, szComponent, lpPathBuf, pcchBuf );
}

UINT WINAPI MsiMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType,
                WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08x\n", hWnd, debugstr_a(lpText), debugstr_a(lpCaption),
          uType, wLanguageId, f);
    return MessageBoxExA(hWnd,lpText,lpCaption,uType,wLanguageId); 
}

UINT WINAPI MsiMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType,
                WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08x\n", hWnd, debugstr_w(lpText), debugstr_w(lpCaption),
          uType, wLanguageId, f);
    return MessageBoxExW(hWnd,lpText,lpCaption,uType,wLanguageId); 
}

UINT WINAPI MsiProvideAssemblyA( LPCSTR szAssemblyName, LPCSTR szAppContext,
                DWORD dwInstallMode, DWORD dwAssemblyInfo, LPSTR lpPathBuf,
                LPDWORD pcchPathBuf )
{
    FIXME("%s %s %08x %08x %p %p\n", debugstr_a(szAssemblyName),
          debugstr_a(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf,
          pcchPathBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyW( LPCWSTR szAssemblyName, LPCWSTR szAppContext,
                DWORD dwInstallMode, DWORD dwAssemblyInfo, LPWSTR lpPathBuf,
                LPDWORD pcchPathBuf )
{
    FIXME("%s %s %08x %08x %p %p\n", debugstr_w(szAssemblyName),
          debugstr_w(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf,
          pcchPathBuf);
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

HRESULT WINAPI MsiGetFileSignatureInformationA( LPCSTR szSignedObjectPath,
                DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, LPBYTE pbHashData,
                LPDWORD pcbHashData)
{
    FIXME("%s %08x %p %p %p\n", debugstr_a(szSignedObjectPath), dwFlags,
          ppcCertContext, pbHashData, pcbHashData);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationW( LPCWSTR szSignedObjectPath,
                DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, LPBYTE pbHashData,
                LPDWORD pcbHashData)
{
    FIXME("%s %08x %p %p %p\n", debugstr_w(szSignedObjectPath), dwFlags,
          ppcCertContext, pbHashData, pcbHashData);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************
 * MsiGetProductPropertyA      [MSI.@]
 */
UINT WINAPI MsiGetProductPropertyA(MSIHANDLE hProduct, LPCSTR szProperty,
                                   LPSTR szValue, LPDWORD pccbValue)
{
    LPWSTR prop = NULL, val = NULL;
    DWORD len;
    UINT r;

    TRACE("(%d, %s, %p, %p)\n", hProduct, debugstr_a(szProperty),
          szValue, pccbValue);

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

    val = msi_alloc(++len * sizeof(WCHAR));
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
    msi_free(prop);
    msi_free(val);

    return r;
}

/******************************************************************
 * MsiGetProductPropertyW      [MSI.@]
 */
UINT WINAPI MsiGetProductPropertyW(MSIHANDLE hProduct, LPCWSTR szProperty,
                                   LPWSTR szValue, LPDWORD pccbValue)
{
    MSIPACKAGE *package;
    MSIQUERY *view = NULL;
    MSIRECORD *rec = NULL;
    LPCWSTR val;
    UINT r;

    static const WCHAR query[] = {
       'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
       '`','P','r','o','p','e','r','t','y','`',' ','W','H','E','R','E',' ',
       '`','P','r','o','p','e','r','t','y','`','=','\'','%','s','\'',0};

    TRACE("(%d, %s, %p, %p)\n", hProduct, debugstr_w(szProperty),
          szValue, pccbValue);

    if (!szProperty)
        return ERROR_INVALID_PARAMETER;

    if (szValue && !pccbValue)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(hProduct, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    r = MSI_OpenQuery(package->db, &view, query, szProperty);
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
        lstrcpynW(szValue, val, *pccbValue);
        *pccbValue = lstrlenW(val);
        r = ERROR_MORE_DATA;
    }
    else
    {
        lstrcpyW(szValue, val);
        *pccbValue = lstrlenW(val);
        r = ERROR_SUCCESS;
    }

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

    msi_free( szPack );

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

static INSTALLSTATE MSI_GetComponentPath(LPCWSTR szProduct, LPCWSTR szComponent,
                                         awstring* lpPathBuf, LPDWORD pcchBuf)
{
    WCHAR squished_pc[GUID_SIZE];
    WCHAR squished_comp[GUID_SIZE];
    HKEY hkey;
    LPWSTR path = NULL;
    INSTALLSTATE state;
    DWORD version;

    static const WCHAR wininstaller[] = {
        'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0};

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
           debugstr_w(szComponent), lpPathBuf->str.w, pcchBuf);

    if (!szProduct || !szComponent)
        return INSTALLSTATE_INVALIDARG;

    if (lpPathBuf->str.w && !pcchBuf)
        return INSTALLSTATE_INVALIDARG;

    if (!squash_guid(szProduct, squished_pc) ||
        !squash_guid(szComponent, squished_comp))
        return INSTALLSTATE_INVALIDARG;

    state = INSTALLSTATE_UNKNOWN;

    if (MSIREG_OpenUserDataComponentKey(szComponent, szLocalSid, &hkey, FALSE) == ERROR_SUCCESS ||
        MSIREG_OpenUserDataComponentKey(szComponent, NULL, &hkey, FALSE) == ERROR_SUCCESS)
    {
        path = msi_reg_get_val_str(hkey, squished_pc);
        RegCloseKey(hkey);

        state = INSTALLSTATE_ABSENT;

        if ((MSIREG_OpenInstallProps(szProduct, MSIINSTALLCONTEXT_MACHINE, NULL,
                                     &hkey, FALSE) == ERROR_SUCCESS ||
            MSIREG_OpenUserDataProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                                          NULL, &hkey, FALSE) == ERROR_SUCCESS) &&
            msi_reg_get_val_dword(hkey, wininstaller, &version) &&
            GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
        {
            RegCloseKey(hkey);
            state = INSTALLSTATE_LOCAL;
        }
    }

    if (state != INSTALLSTATE_LOCAL &&
        (MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                               &hkey, FALSE) == ERROR_SUCCESS ||
         MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_MACHINE,
                               &hkey, FALSE) == ERROR_SUCCESS))
    {
        RegCloseKey(hkey);

        if (MSIREG_OpenUserDataComponentKey(szComponent, szLocalSid, &hkey, FALSE) == ERROR_SUCCESS ||
            MSIREG_OpenUserDataComponentKey(szComponent, NULL, &hkey, FALSE) == ERROR_SUCCESS)
        {
            msi_free(path);
            path = msi_reg_get_val_str(hkey, squished_pc);
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

    msi_strcpy_to_awstring(path, lpPathBuf, pcchBuf);
    msi_free(path);
    return state;
}

/******************************************************************
 * MsiGetComponentPathW      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathW(LPCWSTR szProduct, LPCWSTR szComponent,
                                         LPWSTR lpPathBuf, LPDWORD pcchBuf)
{
    awstring path;

    path.unicode = TRUE;
    path.str.w = lpPathBuf;

    return MSI_GetComponentPath( szProduct, szComponent, &path, pcchBuf );
}

/******************************************************************
 * MsiGetComponentPathA      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathA(LPCSTR szProduct, LPCSTR szComponent,
                                         LPSTR lpPathBuf, LPDWORD pcchBuf)
{
    LPWSTR szwProduct, szwComponent = NULL;
    INSTALLSTATE r = INSTALLSTATE_UNKNOWN;
    awstring path;

    szwProduct = strdupAtoW( szProduct );
    if( szProduct && !szwProduct)
        goto end;

    szwComponent = strdupAtoW( szComponent );
    if( szComponent && !szwComponent )
        goto end;

    path.unicode = FALSE;
    path.str.a = lpPathBuf;

    r = MSI_GetComponentPath( szwProduct, szwComponent, &path, pcchBuf );

end:
    msi_free( szwProduct );
    msi_free( szwComponent );

    return r;
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
    msi_free( szwProduct);
    msi_free( szwFeature);

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
    WCHAR squishProduct[33], comp[GUID_SIZE];
    GUID guid;
    LPWSTR components, p, parent_feature, path;
    UINT rc;
    HKEY hkey;
    INSTALLSTATE r;
    BOOL missing = FALSE;
    BOOL machine = FALSE;
    BOOL source = FALSE;

    TRACE("%s %s\n", debugstr_w(szProduct), debugstr_w(szFeature));

    if (!szProduct || !szFeature)
        return INSTALLSTATE_INVALIDARG;

    if (!squash_guid( szProduct, squishProduct ))
        return INSTALLSTATE_INVALIDARG;

    if (MSIREG_OpenFeaturesKey(szProduct, MSIINSTALLCONTEXT_USERMANAGED,
                               &hkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenFeaturesKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                               &hkey, FALSE) != ERROR_SUCCESS)
    {
        rc = MSIREG_OpenFeaturesKey(szProduct, MSIINSTALLCONTEXT_MACHINE,
                                    &hkey, FALSE);
        if (rc != ERROR_SUCCESS)
            return INSTALLSTATE_UNKNOWN;

        machine = TRUE;
    }

    parent_feature = msi_reg_get_val_str( hkey, szFeature );
    RegCloseKey(hkey);

    if (!parent_feature)
        return INSTALLSTATE_UNKNOWN;

    r = (parent_feature[0] == 6) ? INSTALLSTATE_ABSENT : INSTALLSTATE_LOCAL;
    msi_free(parent_feature);
    if (r == INSTALLSTATE_ABSENT)
        return r;

    if (machine)
        rc = MSIREG_OpenUserDataFeaturesKey(szProduct,
                                            MSIINSTALLCONTEXT_MACHINE,
                                            &hkey, FALSE);
    else
        rc = MSIREG_OpenUserDataFeaturesKey(szProduct,
                                            MSIINSTALLCONTEXT_USERUNMANAGED,
                                            &hkey, FALSE);

    if (rc != ERROR_SUCCESS)
        return INSTALLSTATE_ADVERTISED;

    components = msi_reg_get_val_str( hkey, szFeature );
    RegCloseKey(hkey);

    TRACE("rc = %d buffer = %s\n", rc, debugstr_w(components));

    if (!components)
        return INSTALLSTATE_ADVERTISED;

    for( p = components; *p && *p != 2 ; p += 20)
    {
        if (!decode_base85_guid( p, &guid ))
        {
            if (p != components)
                break;

            msi_free(components);
            return INSTALLSTATE_BADCONFIG;
        }

        StringFromGUID2(&guid, comp, GUID_SIZE);

        if (machine)
            rc = MSIREG_OpenUserDataComponentKey(comp, szLocalSid, &hkey, FALSE);
        else
            rc = MSIREG_OpenUserDataComponentKey(comp, NULL, &hkey, FALSE);

        if (rc != ERROR_SUCCESS)
        {
            msi_free(components);
            return INSTALLSTATE_ADVERTISED;
        }

        path = msi_reg_get_val_str(hkey, squishProduct);
        if (!path)
            missing = TRUE;
        else if (lstrlenW(path) > 2 &&
                 path[0] >= '0' && path[0] <= '9' &&
                 path[1] >= '0' && path[1] <= '9')
        {
            source = TRUE;
        }

        msi_free(path);
    }

    TRACE("%s %s -> %d\n", debugstr_w(szProduct), debugstr_w(szFeature), r);
    msi_free(components);

    if (missing)
        return INSTALLSTATE_ADVERTISED;

    if (source)
        return INSTALLSTATE_SOURCE;

    return INSTALLSTATE_LOCAL;
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
        lpwVersionBuff = msi_alloc(*pcchVersionBuf*sizeof(WCHAR));
        if( !lpwVersionBuff )
            goto end;
    }

    if( lpLangBuf && pcchLangBuf && *pcchLangBuf )
    {
        lpwLangBuff = msi_alloc(*pcchLangBuf*sizeof(WCHAR));
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
    msi_free(szwFilePath);
    msi_free(lpwVersionBuff);
    msi_free(lpwLangBuff);

    return ret;
}

/******************************************************************
 * MsiGetFileVersionW         [MSI.@]
 */
UINT WINAPI MsiGetFileVersionW(LPCWSTR szFilePath, LPWSTR lpVersionBuf,
                LPDWORD pcchVersionBuf, LPWSTR lpLangBuf, LPDWORD pcchLangBuf)
{
    static const WCHAR szVersionResource[] = {'\\',0};
    static const WCHAR szVersionFormat[] = {
        '%','d','.','%','d','.','%','d','.','%','d',0};
    static const WCHAR szLangResource[] = {
        '\\','V','a','r','F','i','l','e','I','n','f','o','\\',
        'T','r','a','n','s','l','a','t','i','o','n',0};
    static const WCHAR szLangFormat[] = {'%','d',0};
    UINT ret = 0;
    DWORD dwVerLen, gle;
    LPVOID lpVer = NULL;
    VS_FIXEDFILEINFO *ffi;
    USHORT *lang;
    UINT puLen;
    WCHAR tmp[32];

    TRACE("%s %p %d %p %d\n", debugstr_w(szFilePath),
          lpVersionBuf, pcchVersionBuf?*pcchVersionBuf:0,
          lpLangBuf, pcchLangBuf?*pcchLangBuf:0);

    if ((lpVersionBuf && !pcchVersionBuf) ||
        (lpLangBuf && !pcchLangBuf))
        return ERROR_INVALID_PARAMETER;

    dwVerLen = GetFileVersionInfoSizeW(szFilePath, NULL);
    if( !dwVerLen )
    {
        gle = GetLastError();
        if (gle == ERROR_BAD_PATHNAME)
            return ERROR_FILE_NOT_FOUND;
        else if (gle == ERROR_RESOURCE_DATA_NOT_FOUND)
            return ERROR_FILE_INVALID;

        return gle;
    }

    lpVer = msi_alloc(dwVerLen);
    if( !lpVer )
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }

    if( !GetFileVersionInfoW(szFilePath, 0, dwVerLen, lpVer) )
    {
        ret = GetLastError();
        goto end;
    }

    if (pcchVersionBuf)
    {
        if( VerQueryValueW(lpVer, szVersionResource, (LPVOID*)&ffi, &puLen) &&
            (puLen > 0) )
        {
            wsprintfW(tmp, szVersionFormat,
                  HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
                  HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
            if (lpVersionBuf) lstrcpynW(lpVersionBuf, tmp, *pcchVersionBuf);

            if (strlenW(tmp) >= *pcchVersionBuf)
                ret = ERROR_MORE_DATA;

            *pcchVersionBuf = lstrlenW(tmp);
        }
        else
        {
            if (lpVersionBuf) *lpVersionBuf = 0;
            *pcchVersionBuf = 0;
        }
    }

    if (pcchLangBuf)
    {
        if (VerQueryValueW(lpVer, szLangResource, (LPVOID*)&lang, &puLen) &&
            (puLen > 0))
        {
            wsprintfW(tmp, szLangFormat, *lang);
            if (lpLangBuf) lstrcpynW(lpLangBuf, tmp, *pcchLangBuf);

            if (strlenW(tmp) >= *pcchLangBuf)
                ret = ERROR_MORE_DATA;

            *pcchLangBuf = lstrlenW(tmp);
        }
        else
        {
            if (lpLangBuf) *lpLangBuf = 0;
            *pcchLangBuf = 0;
        }
    }

end:
    msi_free(lpVer);
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
    msi_free( prod );
    msi_free( feat );

    return ret;
}

/***********************************************************************
 * MsiUseFeatureExW           [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureExW( LPCWSTR szProduct, LPCWSTR szFeature,
                                      DWORD dwInstallMode, DWORD dwReserved )
{
    INSTALLSTATE state;

    TRACE("%s %s %i %i\n", debugstr_w(szProduct), debugstr_w(szFeature),
          dwInstallMode, dwReserved);

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
INSTALLSTATE WINAPI MsiUseFeatureExA( LPCSTR szProduct, LPCSTR szFeature,
                                      DWORD dwInstallMode, DWORD dwReserved )
{
    INSTALLSTATE ret = INSTALLSTATE_UNKNOWN;
    LPWSTR prod = NULL, feat = NULL;

    TRACE("%s %s %i %i\n", debugstr_a(szProduct), debugstr_a(szFeature),
          dwInstallMode, dwReserved);

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    ret = MsiUseFeatureExW( prod, feat, dwInstallMode, dwReserved );

end:
    msi_free( prod );
    msi_free( feat );

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

/***********************************************************************
 * MSI_ProvideQualifiedComponentEx [internal]
 */
static UINT MSI_ProvideQualifiedComponentEx(LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPCWSTR szProduct,
                DWORD Unused1, DWORD Unused2, awstring *lpPathBuf,
                LPDWORD pcchPathBuf)
{
    WCHAR product[MAX_FEATURE_CHARS+1], component[MAX_FEATURE_CHARS+1],
          feature[MAX_FEATURE_CHARS+1];
    LPWSTR info;
    HKEY hkey;
    DWORD sz;
    UINT rc;

    TRACE("%s %s %i %s %i %i %p %p\n", debugstr_w(szComponent),
          debugstr_w(szQualifier), dwInstallMode, debugstr_w(szProduct),
          Unused1, Unused2, lpPathBuf, pcchPathBuf);

    rc = MSIREG_OpenUserComponentsKey(szComponent, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return ERROR_INDEX_ABSENT;

    info = msi_reg_get_val_str( hkey, szQualifier );
    RegCloseKey(hkey);

    if (!info)
        return ERROR_INDEX_ABSENT;

    MsiDecomposeDescriptorW(info, product, feature, component, &sz);

    if (!szProduct)
        rc = MSI_GetComponentPath(product, component, lpPathBuf, pcchPathBuf);
    else
        rc = MSI_GetComponentPath(szProduct, component, lpPathBuf, pcchPathBuf);

    msi_free( info );

    if (rc != INSTALLSTATE_LOCAL)
        return ERROR_FILE_NOT_FOUND;

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiProvideQualifiedComponentExW [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentExW(LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPCWSTR szProduct,
                DWORD Unused1, DWORD Unused2, LPWSTR lpPathBuf,
                LPDWORD pcchPathBuf)
{
    awstring path;

    path.unicode = TRUE;
    path.str.w = lpPathBuf;

    return MSI_ProvideQualifiedComponentEx(szComponent, szQualifier,
            dwInstallMode, szProduct, Unused1, Unused2, &path, pcchPathBuf);
}

/***********************************************************************
 * MsiProvideQualifiedComponentExA [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentExA(LPCSTR szComponent,
                LPCSTR szQualifier, DWORD dwInstallMode, LPCSTR szProduct,
                DWORD Unused1, DWORD Unused2, LPSTR lpPathBuf,
                LPDWORD pcchPathBuf)
{
    LPWSTR szwComponent, szwQualifier = NULL, szwProduct = NULL;
    UINT r = ERROR_OUTOFMEMORY;
    awstring path;

    TRACE("%s %s %u %s %u %u %p %p\n", debugstr_a(szComponent),
          debugstr_a(szQualifier), dwInstallMode, debugstr_a(szProduct),
          Unused1, Unused2, lpPathBuf, pcchPathBuf);

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
    msi_free(szwProduct);
    msi_free(szwComponent);
    msi_free(szwQualifier);

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
    WCHAR squished_pc[SQUISH_GUID_SIZE];
    LPWSTR user, org, serial;
    USERINFOSTATE state;
    HKEY hkey, props;
    LPCWSTR orgptr;
    UINT r;

    static const WCHAR szEmpty[] = {0};

    TRACE("%s %p %p %p %p %p %p\n", debugstr_w(szProduct), lpUserNameBuf,
          pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf,
          pcchSerialBuf);

    if (!szProduct || !squash_guid(szProduct, squished_pc))
        return USERINFOSTATE_INVALIDARG;

    if (MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERMANAGED,
                              &hkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_USERUNMANAGED,
                              &hkey, FALSE) != ERROR_SUCCESS &&
        MSIREG_OpenProductKey(szProduct, MSIINSTALLCONTEXT_MACHINE,
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

        r = msi_strcpy_to_awstring(user, lpUserNameBuf, pcchUserNameBuf);
        if (r == ERROR_MORE_DATA)
        {
            state = USERINFOSTATE_MOREDATA;
            goto done;
        }
    }

    if (pcchOrgNameBuf)
    {
        orgptr = org;
        if (!orgptr) orgptr = szEmpty;

        r = msi_strcpy_to_awstring(orgptr, lpOrgNameBuf, pcchOrgNameBuf);
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

        r = msi_strcpy_to_awstring(serial, lpSerialBuf, pcchSerialBuf);
        if (r == ERROR_MORE_DATA)
            state = USERINFOSTATE_MOREDATA;
    }

done:
    msi_free(user);
    msi_free(org);
    msi_free(serial);

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

    msi_free( prod );

    return r;
}

UINT WINAPI MsiCollectUserInfoW(LPCWSTR szProduct)
{
    MSIHANDLE handle;
    UINT rc;
    MSIPACKAGE *package;
    static const WCHAR szFirstRun[] = {'F','i','r','s','t','R','u','n',0};

    TRACE("(%s)\n",debugstr_w(szProduct));

    rc = MsiOpenProductW(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    rc = ACTION_PerformUIAction(package, szFirstRun, -1);
    msiobj_release( &package->hdr );

    MsiCloseHandle(handle);

    return rc;
}

UINT WINAPI MsiCollectUserInfoA(LPCSTR szProduct)
{
    MSIHANDLE handle;
    UINT rc;
    MSIPACKAGE *package;
    static const WCHAR szFirstRun[] = {'F','i','r','s','t','R','u','n',0};

    TRACE("(%s)\n",debugstr_a(szProduct));

    rc = MsiOpenProductA(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    rc = ACTION_PerformUIAction(package, szFirstRun, -1);
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
    msi_free(feat);
    msi_free(prod);

    return r;
}

/***********************************************************************
 * MsiConfigureFeatureW            [MSI.@]
 */
UINT WINAPI MsiConfigureFeatureW(LPCWSTR szProduct, LPCWSTR szFeature, INSTALLSTATE eInstallState)
{
    static const WCHAR szCostInit[] = { 'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0 };
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

    r = ACTION_PerformUIAction( package, szCostInit, -1 );
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

    TRACE("%d\n", dwReserved);

    if (dwReserved)
    {
        FIXME("dwReserved=%d\n", dwReserved);
        return ERROR_INVALID_PARAMETER;
    }

    if (!GetWindowsDirectoryW(path, MAX_PATH))
        return ERROR_FUNCTION_FAILED;

    lstrcatW(path, installerW);

    if (!CreateDirectoryW(path, NULL))
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
    msi_free( target );
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

UINT WINAPI MsiReinstallFeatureW( LPCWSTR szProduct, LPCWSTR szFeature,
                                  DWORD dwReinstallMode )
{
    MSIPACKAGE* package = NULL;
    UINT r;
    WCHAR sourcepath[MAX_PATH];
    WCHAR filename[MAX_PATH];
    static const WCHAR szLogVerbose[] = {
        ' ','L','O','G','V','E','R','B','O','S','E',0 };
    static const WCHAR szInstalled[] = { 'I','n','s','t','a','l','l','e','d',0};
    static const WCHAR szReinstall[] = {'R','E','I','N','S','T','A','L','L',0};
    static const WCHAR szReinstallMode[] = {'R','E','I','N','S','T','A','L','L','M','O','D','E',0};
    static const WCHAR szOne[] = {'1',0};
    WCHAR reinstallmode[11];
    LPWSTR ptr;
    DWORD sz;

    FIXME("%s %s %i\n", debugstr_w(szProduct), debugstr_w(szFeature),
                           dwReinstallMode);

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
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
            MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz);

    sz = sizeof(filename);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
            MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

    lstrcatW( sourcepath, filename );

    if (dwReinstallMode & REINSTALLMODE_PACKAGE)
        r = MSI_OpenPackageW( sourcepath, &package );
    else
        r = MSI_OpenProductW( szProduct, &package );

    if (r != ERROR_SUCCESS)
        return r;

    MSI_SetPropertyW( package, szReinstallMode, reinstallmode );
    MSI_SetPropertyW( package, szInstalled, szOne );
    MSI_SetPropertyW( package, szLogVerbose, szOne );
    MSI_SetPropertyW( package, szReinstall, szFeature );

    r = MSI_InstallPackage( package, sourcepath, NULL );

    msiobj_release( &package->hdr );

    return r;
}

UINT WINAPI MsiReinstallFeatureA( LPCSTR szProduct, LPCSTR szFeature,
                                  DWORD dwReinstallMode )
{
    LPWSTR wszProduct;
    LPWSTR wszFeature;
    UINT rc;

    TRACE("%s %s %i\n", debugstr_a(szProduct), debugstr_a(szFeature),
                           dwReinstallMode);

    wszProduct = strdupAtoW(szProduct);
    wszFeature = strdupAtoW(szFeature);

    rc = MsiReinstallFeatureW(wszProduct, wszFeature, dwReinstallMode);

    msi_free(wszProduct);
    msi_free(wszFeature);
    return rc;
}

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

extern VOID WINAPI MD5Init( MD5_CTX *);
extern VOID WINAPI MD5Update( MD5_CTX *, const unsigned char *, unsigned int );
extern VOID WINAPI MD5Final( MD5_CTX *);

/***********************************************************************
 * MsiGetFileHashW            [MSI.@]
 */
UINT WINAPI MsiGetFileHashW( LPCWSTR szFilePath, DWORD dwOptions,
                             PMSIFILEHASHINFO pHash )
{
    HANDLE handle, mapping;
    void *p;
    DWORD length;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("%s %08x %p\n", debugstr_w(szFilePath), dwOptions, pHash );

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

    handle = CreateFileW( szFilePath, GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
        return ERROR_FILE_NOT_FOUND;

    length = GetFileSize( handle, NULL );

    mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, 0, NULL );
    if (mapping)
    {
        p = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, length );
        if (p)
        {
            MD5_CTX ctx;

            MD5Init( &ctx );
            MD5Update( &ctx, p, length );
            MD5Final( &ctx );
            UnmapViewOfFile( p );

            memcpy( pHash->dwData, ctx.digest, sizeof pHash->dwData );
            r = ERROR_SUCCESS;
        }
        CloseHandle( mapping );
    }
    CloseHandle( handle );

    return r;
}

/***********************************************************************
 * MsiGetFileHashA            [MSI.@]
 */
UINT WINAPI MsiGetFileHashA( LPCSTR szFilePath, DWORD dwOptions,
                             PMSIFILEHASHINFO pHash )
{
    LPWSTR file;
    UINT r;

    TRACE("%s %08x %p\n", debugstr_a(szFilePath), dwOptions, pHash );

    file = strdupAtoW( szFilePath );
    if (szFilePath && !file)
        return ERROR_OUTOFMEMORY;

    r = MsiGetFileHashW( file, dwOptions, pHash );
    msi_free( file );
    return r;
}

/***********************************************************************
 * MsiAdvertiseScriptW        [MSI.@]
 */
UINT WINAPI MsiAdvertiseScriptW( LPCWSTR szScriptFile, DWORD dwFlags,
                                 PHKEY phRegData, BOOL fRemoveItems )
{
    FIXME("%s %08x %p %d\n",
          debugstr_w( szScriptFile ), dwFlags, phRegData, fRemoveItems );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiAdvertiseScriptA        [MSI.@]
 */
UINT WINAPI MsiAdvertiseScriptA( LPCSTR szScriptFile, DWORD dwFlags,
                                 PHKEY phRegData, BOOL fRemoveItems )
{
    FIXME("%s %08x %p %d\n",
          debugstr_a( szScriptFile ), dwFlags, phRegData, fRemoveItems );
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
UINT WINAPI MsiSetExternalUIRecord( INSTALLUI_HANDLER_RECORD puiHandler,
                                    DWORD dwMessageFilter, LPVOID pvContext,
                                    PINSTALLUI_HANDLER_RECORD ppuiPrevHandler)
{
    FIXME("%p %08x %p %p\n", puiHandler, dwMessageFilter ,pvContext,
                             ppuiPrevHandler);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiInstallMissingComponentW     [MSI.@]
 */
UINT WINAPI MsiInstallMissingComponentW(LPCWSTR szProduct, LPCWSTR szComponent, INSTALLSTATE eInstallState)
{
    FIXME("(%s %s %d\n", debugstr_w(szProduct), debugstr_w(szComponent), eInstallState);
    return ERROR_SUCCESS;
}
