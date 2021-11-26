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

/***************************************************************************
 *  GetNextElement (internal function)
 *
 * Gets a part of a string till the first backslash.
 *
 * PARAMETERS
 *  pszNext [IN] string to get the element from
 *  pszOut  [IN] pointer to buffer which receives string
 *  dwOut   [IN] length of pszOut
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut)
{
    LPCWSTR pszTail = pszNext;
    DWORD dwCopy;

    TRACE ("(%s %p 0x%08x)\n", debugstr_w (pszNext), pszOut, dwOut);

    *pszOut = 0x0000;

    if (!pszNext || !*pszNext)
        return NULL;

    while (*pszTail && (*pszTail != (WCHAR) '\\'))
        pszTail++;

    dwCopy = pszTail - pszNext + 1;
    lstrcpynW (pszOut, pszNext, (dwOut < dwCopy) ? dwOut : dwCopy);

    if (*pszTail)
        pszTail++;
    else
        pszTail = NULL;

    TRACE ("--(%s %s 0x%08x %p)\n", debugstr_w (pszNext), debugstr_w (pszOut), dwOut, pszTail);
    return pszTail;
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

    hres = isf->GetDetailsOf(pidl1, lParam, &sd);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    hres = StrRetToBufW(&sd.str, pidl1, wszItem1, MAX_PATH);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    hres = isf->GetDetailsOf(pidl2, lParam, &sd);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    hres = StrRetToBufW(&sd.str, pidl2, wszItem2, MAX_PATH);
    if (FAILED(hres))
        return MAKE_COMPARE_HRESULT(1);

    int ret = wcsicmp(wszItem1, wszItem2);
    if (ret == 0)
        return SHELL32_CompareChildren(isf, lParam, pidl1, pidl2);

    return MAKE_COMPARE_HRESULT(ret);
}

void AddClassKeyToArray(const WCHAR * szClass, HKEY* array, UINT* cKeys)
{
    if (*cKeys >= 16)
        return;

    HKEY hkey;
    LSTATUS result = RegOpenKeyExW(HKEY_CLASSES_ROOT, szClass, 0, KEY_READ | KEY_QUERY_VALUE, &hkey);
    if (result != ERROR_SUCCESS)
        return;

    array[*cKeys] = hkey;
    *cKeys += 1;
}

void AddFSClassKeysToArray(PCUITEMID_CHILD pidl, HKEY* array, UINT* cKeys)
{
    if (_ILIsValue(pidl))
    {
        FileStructW* pFileData = _ILGetFileStructW(pidl);
        LPWSTR extension = PathFindExtension(pFileData->wszName);

        if (extension)
        {
            AddClassKeyToArray(extension, array, cKeys);

            WCHAR wszClass[MAX_PATH], wszClass2[MAX_PATH];
            DWORD dwSize = sizeof(wszClass);
            if (RegGetValueW(HKEY_CLASSES_ROOT, extension, NULL, RRF_RT_REG_SZ, NULL, wszClass, &dwSize) == ERROR_SUCCESS)
            {
                swprintf(wszClass2, L"%s//%s", extension, wszClass);

                AddClassKeyToArray(wszClass, array, cKeys);
                AddClassKeyToArray(wszClass2, array, cKeys);
            }

            swprintf(wszClass2, L"SystemFileAssociations//%s", extension);
            AddClassKeyToArray(wszClass2, array, cKeys);

            if (RegGetValueW(HKEY_CLASSES_ROOT, extension, L"PerceivedType ", RRF_RT_REG_SZ, NULL, wszClass, &dwSize) == ERROR_SUCCESS)
            {
                swprintf(wszClass2, L"SystemFileAssociations//%s", wszClass);
                AddClassKeyToArray(wszClass2, array, cKeys);
            }
        }

        AddClassKeyToArray(L"AllFilesystemObjects", array, cKeys);
        AddClassKeyToArray(L"*", array, cKeys);
    }
    else if (_ILIsFolder(pidl))
    {
        AddClassKeyToArray(L"AllFilesystemObjects", array, cKeys);
        AddClassKeyToArray(L"Directory", array, cKeys);
        AddClassKeyToArray(L"Folder", array, cKeys);
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
    PCIDLIST_ABSOLUTE pidlItem;
    if (cidl)
    {
        /* Firefox sends a full pidl here dispite the fact it is a PCUITEMID_CHILD_ARRAY -_- */
        if (ILGetNext(apidl[0]) != NULL)
        {
            pidlItem = apidl[0];
        }
        else
        {
            pidlItem = ILCombine(pidlFolder, apidl[0]);
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
HRESULT WINAPI
Shell_DefaultContextMenuCallBack(IShellFolder *psf, IDataObject *pdtobj)
{
    PIDLIST_ABSOLUTE pidlFolder;
    PUITEMID_CHILD *apidl;
    UINT cidl;
    HRESULT hr = SH_GetApidlFromDataObject(pdtobj, &pidlFolder, &apidl, &cidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (cidl > 1)
    {
        ERR("SHMultiFileProperties is not yet implemented\n");
        SHFree(pidlFolder);
        _ILFreeaPidl(apidl, cidl);
        return E_FAIL;
    }

    STRRET strFile;
    hr = psf->GetDisplayNameOf(apidl[0], SHGDN_FORPARSING, &strFile);
    if (SUCCEEDED(hr))
    {
        hr = SH_ShowPropertiesDialog(strFile.pOleStr, pidlFolder, apidl);
        if (FAILED(hr))
            ERR("SH_ShowPropertiesDialog failed\n");
    }
    else
    {
        ERR("Failed to get display name\n");
    }

    SHFree(pidlFolder);
    _ILFreeaPidl(apidl, cidl);

    return hr;
}
