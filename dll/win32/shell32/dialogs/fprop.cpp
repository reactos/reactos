/*
 *                 Shell Library Functions
 *
 * Copyright 2005 Johannes Anderwald
 * Copyright 2012 Rafal Harabien
 * Copyright 2017-2018 Katayama Hirofumi MZ
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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);

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
SH_ShowPropertiesDialog(LPCWSTR pwszPath, IDataObject *pDataObj)
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

    CDataObjectHIDA cida(pDataObj);
    if (FAILED_UNEXPECTEDLY(cida.hr()))
        return FALSE;

    if (cida->cidl == 0)
    {
        ERR("Empty HIDA\n");
        return FALSE;
    }

    /* Handle drives */
    if (_ILIsDrive(HIDA_GetPIDLItem(cida, 0)))
        return SH_ShowDriveProperties(wszPath, pDataObj);


    RECT rcPosition = {CW_USEDEFAULT, CW_USEDEFAULT, 0, 0};
    POINT pt;
    if (SUCCEEDED(DataObject_GetOffset(pDataObj, &pt)))
    {
        rcPosition.left = pt.x;
        rcPosition.top = pt.y;
    }

    DWORD style = WS_DISABLED | WS_CLIPSIBLINGS | WS_CAPTION;
    DWORD exstyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;
    CStubWindow32 stub;
    if (!stub.Create(NULL, rcPosition, NULL, style, exstyle))
    {
        ERR("StubWindow32 creation failed\n");
        return FALSE;
    }

    /* Handle files and folders */
    PROPSHEETHEADERW Header;
    memset(&Header, 0x0, sizeof(PROPSHEETHEADERW));
    Header.dwSize = sizeof(PROPSHEETHEADERW);
    Header.hwndParent = stub;
    Header.dwFlags = PSH_NOCONTEXTHELP | PSH_PROPTITLE;
    Header.phpage = hppages;
    Header.pszCaption = PathFindFileNameW(wszPath);

    HRESULT hr = CComObject<CFileDefExt>::CreateInstance(&pFileDefExt);
    if (SUCCEEDED(hr))
    {
        pFileDefExt->AddRef(); // CreateInstance returns object with 0 ref count
        hr = pFileDefExt->Initialize(HIDA_GetPIDLFolder(cida), pDataObj, NULL);
        if (!FAILED_UNEXPECTEDLY(hr))
        {
            hr = pFileDefExt->AddPages(AddPropSheetPageCallback, (LPARAM)&Header);
            if (FAILED_UNEXPECTEDLY(hr))
            {
                ERR("AddPages failed\n");
                return FALSE;
            }
        }
        else
        {
            ERR("Initialize failed\n");
            return FALSE;
        }
    }

    LoadPropSheetHandlers(wszPath, &Header, MAX_PROPERTY_SHEET_PAGE - 1, hpsxa, pDataObj);

    INT_PTR Result = PropertySheetW(&Header);

    for (UINT i = 0; i < 3; ++i)
        if (hpsxa[i])
            SHDestroyPropSheetExtArray(hpsxa[i]);
    if (pFileDefExt)
        pFileDefExt->Release();

    stub.DestroyWindow();

    return (Result != -1);
}

/*EOF */
