/*
 *    Shell Folder stuff
 *
 *    Copyright 1997            Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
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
 *  SHELL32_GetCustomFolderAttributeFromPath (internal function)
 *
 * Gets a value from the folder's desktop.ini file, if one exists.
 *
 * PARAMETERS
 *  pwszFolderPath[I] Folder containing the desktop.ini file.
 *  pwszHeading   [I] Heading in .ini file.
 *  pwszAttribute [I] Attribute in .ini file.
 *  pwszValue     [O] Buffer to store value into.
 *  cchValue      [I] Size in characters including NULL of buffer pointed to
 *                    by pwszValue.
 *
 *  RETURNS
 *    TRUE if returned non-NULL value.
 *    FALSE otherwise.
 */
static BOOL __inline SHELL32_GetCustomFolderAttributeFromPath(
    LPWSTR pwszFolderPath, LPCWSTR pwszHeading, LPCWSTR pwszAttribute,
    LPWSTR pwszValue, DWORD cchValue)
{
    static const WCHAR wszDesktopIni[] =
            {'d','e','s','k','t','o','p','.','i','n','i',0};
    static const WCHAR wszDefault[] = {0};

    PathAddBackslashW(pwszFolderPath);
    PathAppendW(pwszFolderPath, wszDesktopIni);
    return GetPrivateProfileStringW(pwszHeading, pwszAttribute, wszDefault,
                                    pwszValue, cchValue, pwszFolderPath);
}

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
static HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, LPCWSTR pathRoot,
                LPCITEMIDLIST pidlChild, REFCLSID clsid, LPVOID * ppvOut)
{
    HRESULT hr;
    CComPtr<IShellFolder> pShellFolder;

    TRACE ("%p %s %p\n", pidlRoot, debugstr_w(pathRoot), pidlChild);

    hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IShellFolder, &pShellFolder));
    if (SUCCEEDED (hr))
    {
        LPITEMIDLIST pidlAbsolute = ILCombine (pidlRoot, pidlChild);
        CComPtr<IPersistFolder> ppf;
        CComPtr<IPersistFolder3> ppf3;

        if ((_ILIsFolder(pidlChild) || _ILIsDrive(pidlChild)) &&
            SUCCEEDED(pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3))))
        {
            PERSIST_FOLDER_TARGET_INFO ppfti;

            ZeroMemory (&ppfti, sizeof (ppfti));

            /* fill the PERSIST_FOLDER_TARGET_INFO */
            ppfti.dwAttributes = -1;
            ppfti.csidl = -1;

            /* build path */
            if (pathRoot)
            {
                lstrcpynW (ppfti.szTargetParsingName, pathRoot, MAX_PATH - 1);
                PathAddBackslashW(ppfti.szTargetParsingName); /* FIXME: why have drives a backslash here ? */
            }

            if (pidlChild)
            {
                int len = wcslen(ppfti.szTargetParsingName);

                if (!_ILSimpleGetTextW(pidlChild, ppfti.szTargetParsingName + len, MAX_PATH - len))
                    hr = E_INVALIDARG;
            }

            ppf3->InitializeEx(NULL, pidlAbsolute, &ppfti);
        }
        else if (SUCCEEDED((hr = pShellFolder->QueryInterface(IID_PPV_ARG(IPersistFolder, &ppf)))))
        {
            ppf->Initialize(pidlAbsolute);
        }
        ILFree (pidlAbsolute);
    }

    *ppvOut = pShellFolder.Detach();

    TRACE ("-- (%p) ret=0x%08x\n", *ppvOut, hr);

    return hr;
}

/***********************************************************************
 *    SHELL32_BindToChild [Internal]
 *
 * Common code for IShellFolder_BindToObject.
 *
 * PARAMS
 *  pidlRoot     [I] The parent shell folder's absolute pidl.
 *  pathRoot     [I] Absolute dos path of the parent shell folder.
 *  pidlComplete [I] PIDL of the child. Relative to pidlRoot.
 *  riid         [I] GUID of the interface, which ppvOut shall be bound to.
 *  ppvOut       [O] A reference to the child's interface (riid).
 *
 * NOTES
 *  pidlComplete has to contain at least one non empty SHITEMID.
 *  This function makes special assumptions on the shell namespace, which
 *  means you probably can't use it for your IShellFolder implementation.
 */
HRESULT SHELL32_BindToChild (LPCITEMIDLIST pidlRoot,
                             LPCWSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut)
{
    static const WCHAR wszDotShellClassInfo[] = {
        '.','S','h','e','l','l','C','l','a','s','s','I','n','f','o',0 };

    GUID const *clsid;
    CComPtr<IShellFolder> pSF;
    HRESULT hr;
    LPITEMIDLIST pidlChild;

    if (!pidlRoot || !ppvOut || !pidlComplete || !pidlComplete->mkid.cb)
        return E_INVALIDARG;

    *ppvOut = NULL;

    pidlChild = ILCloneFirst (pidlComplete);

    if ((clsid = _ILGetGUIDPointer (pidlChild))) {
        /* virtual folder */
        hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild, *clsid, (LPVOID *)&pSF);
    } else {
        /* file system folder */
        CLSID clsidFolder = CLSID_ShellFSFolder;
        static const WCHAR wszCLSID[] = {'C','L','S','I','D',0};
        WCHAR wszCLSIDValue[CHARS_IN_GUID], wszFolderPath[MAX_PATH], *pwszPathTail = wszFolderPath;

        /* see if folder CLSID should be overridden by desktop.ini file */
        if (pathRoot) {
            lstrcpynW(wszFolderPath, pathRoot, MAX_PATH);
            pwszPathTail = PathAddBackslashW(wszFolderPath);
        }

        _ILSimpleGetTextW(pidlChild,pwszPathTail,MAX_PATH - (int)(pwszPathTail - wszFolderPath));

        if (SHELL32_GetCustomFolderAttributeFromPath (wszFolderPath,
            wszDotShellClassInfo, wszCLSID, wszCLSIDValue, CHARS_IN_GUID))
            CLSIDFromString (wszCLSIDValue, &clsidFolder);

        hr = SHELL32_CoCreateInitSF (pidlRoot, pathRoot, pidlChild,
                                     clsidFolder, (LPVOID *)&pSF);
    }
    ILFree (pidlChild);

    if (SUCCEEDED (hr)) {
        if (_ILIsPidlSimple (pidlComplete)) {
            /* no sub folders */
            hr = pSF->QueryInterface(riid, ppvOut);
        } else {
            /* go deeper */
            hr = pSF->BindToObject(ILGetNext (pidlComplete), NULL, riid, ppvOut);
        }
    }

    TRACE ("-- returning (%p) %08x\n", *ppvOut, hr);

    return hr;
}

HRESULT SHELL32_BindToGuidItem(LPCITEMIDLIST pidlRoot,
                               PCUIDLIST_RELATIVE pidl,
                               LPBC pbcReserved,
                               REFIID riid,
                               LPVOID *ppvOut)
{
    CComPtr<IPersistFolder> pFolder;
    HRESULT hr;

    GUID *pGUID = _ILGetGUIDPointer(pidl);
    if (!pGUID)
    {
        ERR("SHELL32_BindToGuidItem called for non guid item!\n");
        return E_FAIL;
    }

    hr = SHCoCreateInstance(NULL, pGUID, NULL, IID_PPV_ARG(IPersistFolder, &pFolder));
    if (FAILED(hr))
        return hr;

    hr = pFolder->Initialize(ILCombine(pidlRoot, pidl));
    if (FAILED(hr))
        return hr;

    if (_ILIsPidlSimple (pidl))
    {
        return pFolder->QueryInterface(riid, ppvOut);
    }
    else
    {
        CComPtr<IShellFolder> psf;
        hr = pFolder->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED(hr))
            return hr;

        return psf->BindToObject(ILGetNext (pidl), pbcReserved, riid, ppvOut);
    }
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
                       LPCITEMIDLIST pidl, DWORD dwFlags, LPWSTR szOut, DWORD dwOutLen)
{
    LPITEMIDLIST pidlFirst;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(pidl=%p 0x%08x %p 0x%08x)\n", psf, pidl, dwFlags, szOut, dwOutLen);
    pdump (pidl);

    pidlFirst = ILCloneFirst(pidl);
    if (pidlFirst)
    {
        CComPtr<IShellFolder> psfChild;

        hr = psf->BindToObject(pidlFirst, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
        if (SUCCEEDED (hr))
        {
            STRRET strTemp;
            LPITEMIDLIST pidlNext = ILGetNext (pidl);

            hr = psfChild->GetDisplayNameOf(pidlNext, dwFlags, &strTemp);
            if (SUCCEEDED (hr))
            {
                if(!StrRetToStrNW (szOut, dwOutLen, &strTemp, pidlNext))
                    hr = E_FAIL;
            }
        }
        ILFree (pidlFirst);
    } else
        hr = E_OUTOFMEMORY;

    TRACE ("-- ret=0x%08x %s\n", hr, debugstr_w(szOut));

    return hr;
}

HRESULT SHELL32_GetDisplayNameOfGUIDItem(IShellFolder2* psf, LPCWSTR pszFolderPath, PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    HRESULT hr = S_OK;
    GUID const *clsid = _ILGetGUIDPointer (pidl);

    if (!strRet)
        return E_INVALIDARG;

    LPWSTR pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING)
    {
        int bWantsForParsing;

        /*
            * We can only get a filesystem path from a shellfolder if the
            *  value WantsFORPARSING in CLSID\\{...}\\shellfolder exists.
            *
            * Exception: The MyComputer folder doesn't have this key,
            *   but any other filesystem backed folder it needs it.
            */
        if (IsEqualIID (*clsid, CLSID_MyComputer))
        {
            bWantsForParsing = TRUE;
        }
        else
        {
            HKEY hkeyClass;
            if (HCR_RegOpenClassIDKey(*clsid, &hkeyClass))
            {
                LONG res = SHGetValueW(hkeyClass, L"Shellfolder", L"WantsForParsing", NULL, NULL, NULL);
                bWantsForParsing = (res == ERROR_SUCCESS);
                RegCloseKey(hkeyClass);
            }
        }

        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                bWantsForParsing)
        {
            /*
                * we need the filesystem path to the destination folder.
                * Only the folder itself can know it
                */
            hr = SHELL32_GetDisplayNameOfChild (psf, pidl, dwFlags,
                                                pszPath,
                                                MAX_PATH);
        }
        else
        {
            wcscpy(pszPath, pszFolderPath);
            PWCHAR pItemName = &pszPath[wcslen(pszPath)];

            /* parsing name like ::{...} */
            pItemName[0] = ':';
            pItemName[1] = ':';
            SHELL32_GUIDToStringW (*clsid, &pItemName[2]);
        }
    }
    else
    {
        /* user friendly name */
        HCR_GetClassNameW (*clsid, pszPath, MAX_PATH);
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
    }
    else
    {
        CoTaskMemFree(pszPath);
    }

    return hr;
}

/***********************************************************************
 *  SHELL32_GetItemAttributes
 *
 * NOTES
 * Observed values:
 *  folder:    0xE0000177    FILESYSTEM | HASSUBFOLDER | FOLDER
 *  file:    0x40000177    FILESYSTEM
 *  drive:    0xf0000144    FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  mycomputer:    0xb0000154    HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  (seems to be default for shell extensions if no registry entry exists)
 *
 * win2k:
 *  folder:    0xF0400177      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER
 *  file:      0x40400177      FILESYSTEM | CANMONIKER
 *  drive      0xF0400154      FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR | CANMONIKER | CANRENAME (LABEL)
 *
 * According to the MSDN documentation this function should not set flags. It claims only to reset flags when necessary.
 * However it turns out the native shell32.dll _sets_ flags in several cases - so do we.
 */

static const DWORD dwSupportedAttr=
                      SFGAO_CANCOPY |           /*0x00000001 */
                      SFGAO_CANMOVE |           /*0x00000002 */
                      SFGAO_CANLINK |           /*0x00000004 */
                      SFGAO_CANRENAME |         /*0x00000010 */
                      SFGAO_CANDELETE |         /*0x00000020 */
                      SFGAO_HASPROPSHEET |      /*0x00000040 */
                      SFGAO_DROPTARGET |        /*0x00000100 */
                      SFGAO_LINK |              /*0x00010000 */
                      SFGAO_READONLY |          /*0x00040000 */
                      SFGAO_HIDDEN |            /*0x00080000 */
                      SFGAO_FILESYSANCESTOR |   /*0x10000000 */
                      SFGAO_FOLDER |            /*0x20000000 */
                      SFGAO_FILESYSTEM |        /*0x40000000 */
                      SFGAO_HASSUBFOLDER;       /*0x80000000 */

HRESULT SHELL32_GetGuidItemAttributes (IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    if (!_ILIsSpecialFolder(pidl))
    {
        ERR("Got wrong type of pidl!\n");
        *pdwAttributes &= SFGAO_CANLINK;
        return S_OK;
    }

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08x not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    /* First try to get them from the registry */
    if (HCR_GetFolderAttributes(pidl, pdwAttributes) && *pdwAttributes)
    {
        return S_OK;
    }
    else
    {
        /* If we can't get it from the registry we have to query the child */
        CComPtr<IShellFolder> psf2;
        if (SUCCEEDED(psf->BindToObject(pidl, 0, IID_PPV_ARG(IShellFolder, &psf2))))
        {
            return psf2->GetAttributesOf(0, NULL, pdwAttributes);
        }
    }

    *pdwAttributes &= SFGAO_CANLINK;
    return S_OK;
}

HRESULT SHELL32_GetFSItemAttributes(IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    DWORD dwFileAttributes, dwShellAttributes;

    if (!_ILIsFolder(pidl) && !_ILIsValue(pidl))
    {
        ERR("Got wrong type of pidl!\n");
        *pdwAttributes &= SFGAO_CANLINK;
        return S_OK;
    }

    if (*pdwAttributes & ~dwSupportedAttr)
    {
        WARN ("attributes 0x%08x not implemented\n", (*pdwAttributes & ~dwSupportedAttr));
        *pdwAttributes &= dwSupportedAttr;
    }

    dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);

    /* Set common attributes */
    dwShellAttributes = *pdwAttributes;
    dwShellAttributes |= SFGAO_FILESYSTEM | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANDELETE |
                         SFGAO_CANRENAME | SFGAO_CANLINK | SFGAO_CANMOVE | SFGAO_CANCOPY;

    if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        dwShellAttributes |=  (SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR);
    }
    else
        dwShellAttributes &= ~(SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_FILESYSANCESTOR);

    if (dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        dwShellAttributes |=  SFGAO_HIDDEN;
    else
        dwShellAttributes &= ~SFGAO_HIDDEN;

    if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        dwShellAttributes |=  SFGAO_READONLY;
    else
        dwShellAttributes &= ~SFGAO_READONLY;

    if (SFGAO_LINK & *pdwAttributes)
    {
        char ext[MAX_PATH];

        if (!_ILGetExtension(pidl, ext, MAX_PATH) || lstrcmpiA(ext, "lnk"))
        dwShellAttributes &= ~SFGAO_LINK;
    }

    if (SFGAO_HASSUBFOLDER & *pdwAttributes)
    {
        CComPtr<IShellFolder> psf2;
        if (SUCCEEDED(psf->BindToObject(pidl, 0, IID_PPV_ARG(IShellFolder, &psf2))))
        {
            CComPtr<IEnumIDList> pEnumIL;
            if (SUCCEEDED(psf2->EnumObjects(0, SHCONTF_FOLDERS, &pEnumIL)))
            {
                if (pEnumIL->Skip(1) != S_OK)
                    dwShellAttributes &= ~SFGAO_HASSUBFOLDER;
            }
        }
    }

    *pdwAttributes &= dwShellAttributes;

    TRACE ("-- 0x%08x\n", *pdwAttributes);
    return S_OK;
}

/***********************************************************************
 *  SHELL32_CompareIDs
 */
HRESULT SHELL32_CompareIDs(IShellFolder * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int type1,
        type2;
    char szTemp1[MAX_PATH];
    char szTemp2[MAX_PATH];
    HRESULT nReturn;
    LPITEMIDLIST firstpidl;
    LPITEMIDLIST nextpidl1;
    LPITEMIDLIST nextpidl2;
    CComPtr<IShellFolder> psf;

    /* test for empty pidls */
    BOOL isEmpty1 = _ILIsDesktop(pidl1);
    BOOL isEmpty2 = _ILIsDesktop(pidl2);

    if (isEmpty1 && isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    if (isEmpty1)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD) -1);
    if (isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);

    /* test for different types. Sort order is the PT_* constant */
    type1 = _ILGetDataPointer(pidl1)->type;
    type2 = _ILGetDataPointer(pidl2)->type;
    if (type1 < type2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD) -1);
    else if (type1 > type2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);

    /* test for name of pidl */
    _ILSimpleGetText(pidl1, szTemp1, MAX_PATH);
    _ILSimpleGetText(pidl2, szTemp2, MAX_PATH);
    nReturn = lstrcmpiA(szTemp1, szTemp2);
    if (nReturn < 0)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD) -1);
    else if (nReturn > 0)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);

    /* test of complex pidls */
    firstpidl = ILCloneFirst(pidl1);
    nextpidl1 = ILGetNext(pidl1);
    nextpidl2 = ILGetNext(pidl2);

    /* optimizing: test special cases and bind not deeper */
    /* the deeper shellfolder would do the same */
    isEmpty1 = _ILIsDesktop(nextpidl1);
    isEmpty2 = _ILIsDesktop(nextpidl2);

    if (isEmpty1 && isEmpty2) 
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    }
    else if (isEmpty1) 
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD) -1);
    }
    else if (isEmpty2)
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
        /* optimizing end */
    }
    else if (SUCCEEDED(iface->BindToObject(firstpidl, NULL, IID_PPV_ARG(IShellFolder, &psf)))) {
        nReturn = psf->CompareIDs(lParam, nextpidl1, nextpidl2);
    }
    ILFree(firstpidl);
    return nReturn;
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
SHOpenFolderAndSelectItems(LPITEMIDLIST pidlFolder,
                           UINT cidl,
                           PCUITEMID_CHILD_ARRAY apidl,
                           DWORD dwFlags)
{
    FIXME("SHOpenFolderAndSelectItems() stub\n");
    return E_NOTIMPL;
}
