/*
 *                 Shell Library Functions
 *
 * Copyright 2005 Johannes Anderwald
 * Copyright 2012 Rafal Harabien
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

#define MAX_PROPERTY_SHEET_PAGE 32

WINE_DEFAULT_DEBUG_CHANNEL(shell);

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);

static BOOL CALLBACK
AddPropSheetPageCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADERW *pHeader = (PROPSHEETHEADERW *)lParam;

    if (pHeader->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        pHeader->phpage[pHeader->nPages++] = hPage;
        return TRUE;
    }

    return FALSE;
}

static UINT
LoadPropSheetHandlers(LPCWSTR pwszPath, PROPSHEETHEADERW *pHeader, UINT cMaxPages, HPSXA *phpsxa, IDataObject *pDataObj)
{
    WCHAR wszBuf[MAX_PATH];
    UINT cPages = 0, i = 0;

    LPWSTR pwszFilename = PathFindFileNameW(pwszPath);
    BOOL bDir = PathIsDirectoryW(pwszPath);

    if (bDir)
    {
        phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"Folder", cMaxPages - cPages, pDataObj);
        cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);

        phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"Directory", cMaxPages - cPages, pDataObj);
        cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);
    }
    else
    {
        /* Load property sheet handlers from ext key */
        LPWSTR pwszExt = PathFindExtensionW(pwszFilename);
        phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, pwszExt, cMaxPages - cPages, pDataObj);
        cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);

        /* Load property sheet handlers from prog id key */
        DWORD cbBuf = sizeof(wszBuf);
        if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, L"", RRF_RT_REG_SZ, NULL, wszBuf, &cbBuf) == ERROR_SUCCESS)
        {
            TRACE("EnumPropSheetExt wszBuf %s, pwszExt %s\n", debugstr_w(wszBuf), debugstr_w(pwszExt));
            phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, wszBuf, cMaxPages - cPages, pDataObj);
            cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);
        }

        /* Add property sheet handlers from "*" key */
        phpsxa[i] = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"*", cMaxPages - cPages, pDataObj);
        cPages += SHAddFromPropSheetExtArray(phpsxa[i++], AddPropSheetPageCallback, (LPARAM)pHeader);
    }

    return cPages;
}

/*************************************************************************
 *
 * SH_ShowPropertiesDialog
 *
 * called from ShellExecuteExW32
 *
 * pwszPath contains path of folder/file
 *
 * TODO: provide button change application type if file has registered type
 *       make filename field editable and apply changes to filename on close
 */

BOOL
SH_ShowPropertiesDialog(LPCWSTR pwszPath, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST *apidl)
{
    HPSXA hpsxa[3] = {NULL, NULL, NULL};
    CComObject<CFileDefExt> *pFileDefExt = NULL;

    TRACE("SH_ShowPropertiesDialog entered filename %s\n", debugstr_w(pwszPath));

    if (pwszPath == NULL || !wcslen(pwszPath))
        return FALSE;

    HPROPSHEETPAGE hppages[MAX_PROPERTY_SHEET_PAGE];
    memset(hppages, 0x0, sizeof(HPROPSHEETPAGE) * MAX_PROPERTY_SHEET_PAGE);

    /* Make a copy of path */
    WCHAR wszPath[MAX_PATH];
    StringCbCopyW(wszPath, sizeof(wszPath), pwszPath);

    /* remove trailing \\ at the end of path */
    PathRemoveBackslashW(wszPath);

    /* Handle drives */
    if (PathIsRootW(wszPath))
        return SH_ShowDriveProperties(wszPath, pidlFolder, apidl);

    /* Handle files and folders */
    PROPSHEETHEADERW Header;
    memset(&Header, 0x0, sizeof(PROPSHEETHEADERW));
    Header.dwSize = sizeof(PROPSHEETHEADERW);
    Header.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    Header.phpage = hppages;
    Header.pszCaption = PathFindFileNameW(wszPath);

    CComPtr<IDataObject> pDataObj;
    HRESULT hr = SHCreateDataObject(pidlFolder, 1, apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObj));

    if (SUCCEEDED(hr))
    {
        hr = CComObject<CFileDefExt>::CreateInstance(&pFileDefExt);
        if (SUCCEEDED(hr))
        {
            pFileDefExt->AddRef(); // CreateInstance returns object with 0 ref count
            hr = pFileDefExt->Initialize(pidlFolder, pDataObj, NULL);
            if (SUCCEEDED(hr))
            {
                hr = pFileDefExt->AddPages(AddPropSheetPageCallback, (LPARAM)&Header);
                if (FAILED(hr))
                    ERR("AddPages failed\n");
            } else
                ERR("Initialize failed\n");
        }

        LoadPropSheetHandlers(wszPath, &Header, MAX_PROPERTY_SHEET_PAGE - 1, hpsxa, pDataObj);
    }

    INT_PTR Result = PropertySheetW(&Header);

    for (UINT i = 0; i < 3; ++i)
        if (hpsxa[i])
            SHDestroyPropSheetExtArray(hpsxa[i]);
    if (pFileDefExt)
        pFileDefExt->Release();

    return (Result != -1);
}

/*EOF */
