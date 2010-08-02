/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

/*
 * Actions focused on in this module
 *
 * FindRelatedProducts
 * MigrateFeatureStates (TODO)
 * RemoveExistingProducts (TODO)
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msidefs.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static BOOL check_language(DWORD lang1, LPCWSTR lang2, DWORD attributes)
{
    DWORD langdword;

    if (!lang2 || lang2[0]==0)
        return TRUE;

    langdword = atoiW(lang2);

    if (attributes & msidbUpgradeAttributesLanguagesExclusive)
        return (lang1 != langdword);
    else
        return (lang1 == langdword);
}

static void append_productcode(MSIPACKAGE* package, LPCWSTR action_property,
                               LPCWSTR productid)
{
    LPWSTR prop;
    LPWSTR newprop;
    DWORD len;
    UINT r;

    prop = msi_dup_property(package->db, action_property );
    if (prop)
        len = strlenW(prop);
    else
        len = 0;

    /*separator*/
    len ++;

    len += strlenW(productid);

    /*null*/
    len++;

    newprop = msi_alloc( len*sizeof(WCHAR) );

    if (prop)
    {
        strcpyW(newprop,prop);
        strcatW(newprop,szSemiColon);
    }
    else
        newprop[0] = 0;
    strcatW(newprop,productid);

    r = msi_set_property( package->db, action_property, newprop );
    if (r == ERROR_SUCCESS && !strcmpW( action_property, cszSourceDir ))
        msi_reset_folders( package, TRUE );

    TRACE("Found Related Product... %s now %s\n",
          debugstr_w(action_property), debugstr_w(newprop));

    msi_free( prop );
    msi_free( newprop );
}

static UINT ITERATE_FindRelatedProducts(MSIRECORD *rec, LPVOID param)
{
    MSIPACKAGE *package = param;
    WCHAR product[GUID_SIZE];
    DWORD index = 0;
    DWORD attributes = 0;
    DWORD sz = GUID_SIZE;
    LPCWSTR upgrade_code;
    HKEY hkey = 0;
    UINT rc = ERROR_SUCCESS;
    MSIRECORD *uirow;

    upgrade_code = MSI_RecordGetString(rec,1);

    rc = MSIREG_OpenUpgradeCodesKey(upgrade_code, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    uirow = MSI_CreateRecord(1);
    attributes = MSI_RecordGetInteger(rec,5);

    while (rc == ERROR_SUCCESS)
    {
        rc = RegEnumValueW(hkey, index, product, &sz, NULL, NULL, NULL, NULL);
        TRACE("Looking at (%i) %s\n",index,debugstr_w(product));
        if (rc == ERROR_SUCCESS)
        {
            WCHAR productid[GUID_SIZE];
            LPCWSTR ver;
            LPCWSTR language;
            LPCWSTR action_property;
            DWORD check = 0x00000000;
            DWORD comp_ver = 0x00000000;
            DWORD sz = 0x100;
            HKEY hukey;
            INT r;

            unsquash_guid(product, productid);
            rc = MSIREG_OpenProductKey(productid, NULL, package->Context,
                                       &hukey, FALSE);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                index ++;
                continue;
            }

            sz = sizeof(DWORD);
            RegQueryValueExW(hukey, INSTALLPROPERTY_VERSIONW, NULL, NULL,
                    (LPBYTE)&check, &sz);
            /* check min */
            ver = MSI_RecordGetString(rec,2);
            if (ver)
            {
                comp_ver = msi_version_str_to_dword(ver);
                r = check - comp_ver;
                if (r < 0 || (r == 0 && !(attributes & msidbUpgradeAttributesVersionMinInclusive)))
                {
                    RegCloseKey(hukey);
                    index ++;
                    continue;
                }
            }

            /* check max */
            ver = MSI_RecordGetString(rec,3);
            if (ver)
            {
                comp_ver = msi_version_str_to_dword(ver);
                r = check - comp_ver;
                if (r > 0 || (r == 0 && !(attributes & msidbUpgradeAttributesVersionMaxInclusive)))
                {
                    RegCloseKey(hukey);
                    index ++;
                    continue;
                }
            }

            /* check language*/
            sz = sizeof(DWORD);
            RegQueryValueExW(hukey, INSTALLPROPERTY_LANGUAGEW, NULL, NULL,
                    (LPBYTE)&check, &sz);
            RegCloseKey(hukey);
            language = MSI_RecordGetString(rec,4);
            TRACE("Checking languages %x and %s\n", check, 
                            debugstr_w(language));
            if (!check_language(check, language, attributes))
            {
                index ++;
                continue;
            }

            action_property = MSI_RecordGetString(rec, 7);
            append_productcode(package, action_property, productid);
            MSI_RecordSetStringW(uirow, 1, productid);
            ui_actiondata(package, szFindRelatedProducts, uirow);
        }
        index ++;
    }
    RegCloseKey(hkey);
    msiobj_release( &uirow->hdr);
    
    return ERROR_SUCCESS;
}

UINT ACTION_FindRelatedProducts(MSIPACKAGE *package)
{
    static const WCHAR Query[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',
         ' ','`','U','p','g','r','a','d','e','`',0};
    UINT rc = ERROR_SUCCESS;
    MSIQUERY *view;

    if (msi_get_property_int(package->db, szInstalled, 0))
    {
        TRACE("Skipping FindRelatedProducts action: product already installed\n");
        return ERROR_SUCCESS;
    }

    if (check_unique_action(package, szFindRelatedProducts))
    {
        TRACE("Skipping FindRelatedProducts action: already done in UI sequence\n");
        return ERROR_SUCCESS;
    }
    else
        register_unique_action(package, szFindRelatedProducts);

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;
    
    rc = MSI_IterateRecords(view, NULL, ITERATE_FindRelatedProducts, package);
    msiobj_release(&view->hdr);
    
    return rc;
}
