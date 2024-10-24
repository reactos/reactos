/*
 *    Shell Folder stuff
 *
 *    Copyright 1997            Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2018 Katayama Hirofumi MZ
 *
 *    IShellFolder2 and related interfaces
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

SHCONTF SHELL_GetDefaultFolderEnumSHCONTF()
{
    SHCONTF Flags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS | SSF_SHOWSUPERHIDDEN, FALSE);
    if (ss.fShowAllObjects)
        Flags |= SHCONTF_INCLUDEHIDDEN;
    if (ss.fShowSuperHidden)
        Flags |= SHCONTF_INCLUDESUPERHIDDEN;
     return Flags;
}

BOOL SHELL_IncludeItemInFolderEnum(IShellFolder *pSF, PCUITEMID_CHILD pidl, SFGAOF Query, SHCONTF Flags)
{
    if (SUCCEEDED(pSF->GetAttributesOf(1, &pidl, &Query)))
    {
        if (Query & SFGAO_NONENUMERATED)
            return FALSE;
        if ((Query & SFGAO_HIDDEN) && !(Flags & SHCONTF_INCLUDEHIDDEN))
            return FALSE;
        if ((Query & (SFGAO_HIDDEN | SFGAO_SYSTEM)) == (SFGAO_HIDDEN | SFGAO_SYSTEM) && !(Flags & SHCONTF_INCLUDESUPERHIDDEN))
            return FALSE;
        if ((Flags & (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS)) != (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS))
            return (Flags & SHCONTF_FOLDERS) ? (Query & SFGAO_FOLDER) : !(Query & SFGAO_FOLDER);
    }
    return TRUE;
}

HRESULT
Shell_NextElement(
    _Inout_ LPWSTR *ppch,
    _Out_ LPWSTR pszOut,
    _In_ INT cchOut,
    _In_ BOOL bValidate)
{
    *pszOut = UNICODE_NULL;

    if (!*ppch)
        return S_FALSE;

    HRESULT hr;
    LPWSTR pchNext = wcschr(*ppch, L'\\');
    if (pchNext)
    {
        if (*ppch < pchNext)
        {
            /* Get an element */
            StringCchCopyNW(pszOut, cchOut, *ppch, pchNext - *ppch);
            ++pchNext;

            if (!*pchNext)
                pchNext = NULL; /* No next */

            hr = S_OK;
        }
        else /* Double backslashes found? */
        {
            pchNext = NULL;
            hr = E_INVALIDARG;
        }
    }
    else /* No more next */
    {
        StringCchCopyW(pszOut, cchOut, *ppch);
        hr = S_OK;
    }

    *ppch = pchNext; /* Go next */

    if (hr == S_OK && bValidate && !PathIsValidElement(pszOut))
    {
        *pszOut = UNICODE_NULL;
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc,
                  LPITEMIDLIST * pidlInOut, LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    LPITEMIDLIST pidlIn = pidlInOut ? *pidlInOut : NULL;
    LPITEMIDLIST pidlOut = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    CComPtr<IShellFolder> psfChild;

    TRACE ("(%p, %p, %p, %s)\n", psf, pbc, pidlIn, debugstr_w (szNext));

    /* get the shellfolder for the child pidl and let it analyse further */
    hr = psf->BindToObject(pidlIn, pbc, IID_PPV_ARG(IShellFolder, &psfChild));
    if (FAILED(hr))
        return hr;

    hr = psfChild->ParseDisplayName(hwndOwner, pbc, szNext, pEaten, &pidlOut, pdwAttributes);
    if (FAILED(hr))
        return hr;

    pidlTemp = ILCombine (pidlIn, pidlOut);
    if (!pidlTemp)
    {
        hr = E_OUTOFMEMORY;
        if (pidlOut)
            ILFree(pidlOut);
        return hr;
    }

    if (pidlOut)
        ILFree (pidlOut);

    if (pidlIn)
        ILFree (pidlIn);

    *pidlInOut = pidlTemp;

    TRACE ("-- pidl=%p ret=0x%08x\n", pidlInOut ? *pidlInOut : NULL, hr);
    return S_OK;
}

/***********************************************************************
 *    SHELL32_CoCreateInitSF
 *
 * Creates a shell folder and initializes it with a pidl and a root folder
 * via IPersistFolder3 or IPersistFolder.
 *
 * NOTES
 *   pathRoot can be NULL for Folders being a drive.
 *   In this case the absolute path is built from pidlChild (eg. C:)
 */
HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, PERSIST_FOLDER_TARGET_INFO* ppfti,
                LPCITEMIDLIST pidlChild, const GUID* clsid, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr;
    CComPtr<IShellFolder> pShellFolder;

    hr = SHCoCreateInstance(NULL, clsid, NULL, IID_PPV_ARG(IShellFolder, &pShellFolder));
    if (FAILED(hr))
        return hr;

    LPITEMIDLIST pidlAbsolute = ILCombine (pidlRoot, pidlChild);
    CComPtr<IPersistFolder> ppf;
    CComPtr<IPersistFolder3> ppf3;

    if (ppfti && SUCCEEDED(pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3))))
    {
        ppf3->InitializeEx(NULL, pidlAbsolute, ppfti);
    }
    else if (SUCCEEDED(pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf))))
    {
        ppf->Initialize(pidlAbsolute);
    }
    ILFree (pidlAbsolute);

    return pShellFolder->QueryInterface(riid, ppvOut);
}

HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, const GUID* clsid,
                                int csidl, REFIID riid, LPVOID *ppvOut)
{
    /* fill the PERSIST_FOLDER_TARGET_INFO */
    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    pfti.dwAttributes = -1;
    pfti.csidl = csidl;

    return SHELL32_CoCreateInitSF(pidlRoot, &pfti, NULL, clsid, riid, ppvOut);
}

HRESULT SHELL32_BindToSF (LPCITEMIDLIST pidlRoot, PERSIST_FOLDER_TARGET_INFO* ppfti,
                LPCITEMIDLIST pidl, const GUID* clsid, REFIID riid, LPVOID *ppvOut)
{
    PITEMID_CHILD pidlChild = ILCloneFirst (pidl);
    if (!pidlChild)
        return E_FAIL;

    CComPtr<IShellFolder> psf;
    HRESULT hr = SHELL32_CoCreateInitSF(pidlRoot,
                                        ppfti,
                                        pidlChild,
                                        clsid,
                                        IID_PPV_ARG(IShellFolder, &psf));
    ILFree(pidlChild);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (_ILIsPidlSimple (pidl))
        return psf->QueryInterface(riid, ppvOut);
    else
        return psf->BindToObject(ILGetNext (pidl), NULL, riid, ppvOut);
}

/***********************************************************************
 *    SHELL32_GetDisplayNameOfChild
 *
 * Retrieves the display name of a child object of a shellfolder.
 *
 * For a pidl eg. [subpidl1][subpidl2][subpidl3]:
 * - it binds to the child shellfolder [subpidl1]
 * - asks it for the displayname of [subpidl2][subpidl3]
 *
 * Is possible the pidl is a simple pidl. In this case it asks the
 * subfolder for the displayname of an empty pidl. The subfolder
 * returns the own displayname eg. "::{guid}". This is used for
 * virtual folders with the registry key WantsFORPARSING set.
 */
HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf,
                       LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
    if (!pidlFirst)
        return E_OUTOFMEMORY;

    CComPtr<IShellFolder> psfChild;
    HRESULT hr = psf->BindToObject(pidlFirst, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
    if (SUCCEEDED (hr))
    {
        hr = psfChild->GetDisplayNameOf(ILGetNext (pidl), dwFlags, strRet);
    }
    ILFree (pidlFirst);

    return hr;
}

HRESULT SHELL32_CompareChildren(IShellFolder2* psf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    PUIDLIST_RELATIVE nextpidl1 = ILGetNext (pidl1);
    PUIDLIST_RELATIVE nextpidl2 = ILGetNext (pidl2);

    bool isEmpty1 = _ILIsDesktop(nextpidl1);
    bool isEmpty2 = _ILIsDesktop(nextpidl2);
    if (isEmpty1 || isEmpty2)
        return MAKE_COMPARE_HRESULT(isEmpty2 - isEmpty1);

    PITEMID_CHILD firstpidl = ILCloneFirst (pidl1);
    if (!firstpidl)
        return E_OUTOFMEMORY;

    CComPtr<IShellFolder> psf2;
    HRESULT hr = psf->BindToObject(firstpidl, 0, IID_PPV_ARG(IShellFolder, &psf2));
    ILFree(firstpidl);
    if (FAILED(hr))
        return MAKE_COMPARE_HRESULT(0);

    return psf2->CompareIDs(lParam, nextpidl1, nextpidl2);
}

HRESULT SHELL32_CompareDetails(IShellFolder2* isf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    SHELLDETAILS sd;
    WCHAR wszItem1[MAX_PATH], wszItem2[MAX_PATH];
    HRESULT hres;
    UINT col = LOWORD(lParam); // Column index without SHCIDS_* flags

    hres = isf->GetDetailsOf(pidl1, col, &sd);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    hres = StrRetToBufW(&sd.str, pidl1, wszItem1, MAX_PATH);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    hres = isf->GetDetailsOf(pidl2, col, &sd);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    hres = StrRetToBufW(&sd.str, pidl2, wszItem2, MAX_PATH);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    int ret = _wcsicmp(wszItem1, wszItem2);
    if (ret == 0)
        return SHELL32_CompareChildren(isf, lParam, pidl1, pidl2);

    return MAKE_COMPARE_HRESULT(ret);
}

void CloseRegKeyArray(HKEY* array, UINT cKeys)
{
    for (UINT i = 0; i < cKeys; ++i)
        RegCloseKey(array[i]);
}

LSTATUS AddClassKeyToArray(const WCHAR* szClass, HKEY* array, UINT* cKeys)
{
    if (*cKeys >= 16)
        return ERROR_MORE_DATA;

    HKEY hkey;
    LSTATUS result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szClass, 0, KEY_READ | KEY_QUERY_VALUE, &hkey);
    if (result == ERROR_SUCCESS)
    {
        array[*cKeys] = hkey;
        *cKeys += 1;
    }
    return result;
}

LSTATUS AddClsidKeyToArray(REFCLSID clsid, HKEY* array, UINT* cKeys)
{
    WCHAR path[6 + 38 + 1] = L"CLSID\\";
    StringFromGUID2(clsid, path + 6, 38 + 1);
    return AddClassKeyToArray(path, array, cKeys);
}

void AddFSClassKeysToArray(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, HKEY* array, UINT* cKeys)
{
    // This function opens the association array keys in canonical order for filesystem items.
    // The order is documented: learn.microsoft.com/en-us/windows/win32/shell/fa-associationarray

    ASSERT(cidl >= 1 && apidl);
    PCUITEMID_CHILD pidl = apidl[0];
    if (_ILIsValue(pidl))
    {
        WCHAR buf[MAX_PATH];
        PWSTR name;
        FileStructW* pFileData = _ILGetFileStructW(pidl);
        if (pFileData)
        {
            name = pFileData->wszName;
        }
        else
        {
            _ILSimpleGetTextW(pidl, buf, _countof(buf));
            name = buf;
        }
        LPCWSTR extension = PathFindExtension(name);

        if (extension)
        {
            WCHAR wszClass[MAX_PATH], wszSFA[23 + _countof(wszClass)];
            DWORD dwSize = sizeof(wszClass);
            if (RegGetValueW(HKEY_CLASSES_ROOT, extension, NULL, RRF_RT_REG_SZ, NULL, wszClass, &dwSize) != ERROR_SUCCESS ||
                !*wszClass || AddClassKeyToArray(wszClass, array, cKeys) != ERROR_SUCCESS)
            {
                // Only add the extension key if the ProgId is not valid
                AddClassKeyToArray(extension, array, cKeys);

                // "Open With" becomes the default when there are no verbs in the above keys
                if (cidl == 1)
                    AddClassKeyToArray(L"Unknown", array, cKeys);
            }

            swprintf(wszSFA, L"SystemFileAssociations\\%s", extension);
            AddClassKeyToArray(wszSFA, array, cKeys);

            dwSize = sizeof(wszClass);
            if (RegGetValueW(HKEY_CLASSES_ROOT, extension, L"PerceivedType ", RRF_RT_REG_SZ, NULL, wszClass, &dwSize) == ERROR_SUCCESS)
            {
                swprintf(wszSFA, L"SystemFileAssociations\\%s", wszClass);
                AddClassKeyToArray(wszSFA, array, cKeys);
            }
        }

        AddClassKeyToArray(L"*", array, cKeys);
        AddClassKeyToArray(L"AllFilesystemObjects", array, cKeys);
    }
    else if (_ILIsFolder(pidl))
    {
        // FIXME: Directory > Folder > AFO is the correct order and it's
        // the order Windows reports in its undocumented association array
        // but it is somehow not the order Windows adds the items to its menu!
        // Until the correct algorithm in CDefaultContextMenu can be determined,
        // we add the folder keys in "menu order".
        AddClassKeyToArray(L"Folder", array, cKeys);
        AddClassKeyToArray(L"AllFilesystemObjects", array, cKeys);
        AddClassKeyToArray(L"Directory", array, cKeys);
    }
    else
    {
        ERR("Got non FS pidl\n");
    }
}

HRESULT SH_GetApidlFromDataObject(IDataObject *pDataObject, PIDLIST_ABSOLUTE* ppidlfolder, PUITEMID_CHILD **apidlItems, UINT *pcidl)
{
    CDataObjectHIDA cida(pDataObject);

    if (FAILED_UNEXPECTEDLY(cida.hr()))
        return cida.hr();

    /* convert the data into pidl */
    LPITEMIDLIST pidl;
    LPITEMIDLIST *apidl = _ILCopyCidaToaPidl(&pidl, cida);
    if (!apidl)
    {
        return E_OUTOFMEMORY;
    }

    *ppidlfolder = pidl;
    *apidlItems = apidl;
    *pcidl = cida->cidl;

    return S_OK;
}

/***********************************************************************
 *  SHCreateLinks
 *
 *   Undocumented.
 */
HRESULT WINAPI SHCreateLinks( HWND hWnd, LPCSTR lpszDir, IDataObject * lpDataObject,
                              UINT uFlags, LPITEMIDLIST *lppidlLinks)
{
    FIXME("%p %s %p %08x %p\n", hWnd, lpszDir, lpDataObject, uFlags, lppidlLinks);
    return E_NOTIMPL;
}

/***********************************************************************
 *  SHOpenFolderAndSelectItems
 *
 *   Unimplemented.
 */
EXTERN_C HRESULT
WINAPI
SHOpenFolderAndSelectItems(PCIDLIST_ABSOLUTE pidlFolder,
                           UINT cidl,
                           PCUITEMID_CHILD_ARRAY apidl,
                           DWORD dwFlags)
{
    ERR("SHOpenFolderAndSelectItems() is hackplemented\n");
    CComHeapPtr<ITEMIDLIST> freeItem;
    PCIDLIST_ABSOLUTE pidlItem;
    if (cidl)
    {
        /* Firefox sends a full pidl here dispite the fact it is a PCUITEMID_CHILD_ARRAY -_- */
        if (!ILIsSingle(apidl[0]))
        {
            pidlItem = apidl[0];
        }
        else
        {
            HRESULT hr = SHILCombine(pidlFolder, apidl[0], &pidlItem);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
            freeItem.Attach(const_cast<PIDLIST_ABSOLUTE>(pidlItem));
        }
    }
    else
    {
        pidlItem = pidlFolder;
    }

    CComPtr<IShellFolder> psfDesktop;

    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    STRRET strret;
    hr = psfDesktop->GetDisplayNameOf(pidlItem, SHGDN_FORPARSING, &strret);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WCHAR wszBuf[MAX_PATH];
    hr = StrRetToBufW(&strret, pidlItem, wszBuf, _countof(wszBuf));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WCHAR wszParams[MAX_PATH];
    wcscpy(wszParams, L"/select,");
    wcscat(wszParams, wszBuf);

    SHELLEXECUTEINFOW sei;
    memset(&sei, 0, sizeof sei);
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_WAITFORINPUTIDLE;
    sei.lpFile = L"explorer.exe";
    sei.lpParameters = wszParams;

    if (ShellExecuteExW(&sei))
        return S_OK;
    else
        return E_FAIL;
}

/*
 * for internal use
 */
HRESULT
SHELL32_ShowPropertiesDialog(IDataObject *pdtobj)
{
    if (!pdtobj)
        return E_INVALIDARG;

    CDataObjectHIDA cida(pdtobj);
    if (FAILED_UNEXPECTEDLY(cida.hr()))
        return cida.hr();
    if (cida->cidl > 1)
    {
        ERR("SHMultiFileProperties is not yet implemented\n");
        return E_FAIL;
    }
    return SHELL32_ShowFilesystemItemPropertiesDialogAsync(pdtobj);
}

HRESULT
SHELL32_DefaultContextMenuCallBack(IShellFolder *psf, IDataObject *pdo, UINT msg)
{
    switch (msg)
    {
        case DFM_MERGECONTEXTMENU:
            return S_OK; // Yes, I want verbs
        case DFM_INVOKECOMMAND:
            return S_FALSE; // Do it for me please
        case DFM_GETDEFSTATICID:
            return S_FALSE; // Supposedly "required for Windows 7 to pick a default"
    }
    return E_NOTIMPL;
}
