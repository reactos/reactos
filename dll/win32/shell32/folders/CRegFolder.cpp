/*
 * ReactOS Shell
 *
 * Copyright 2016 Giannis Adamopoulos
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

HRESULT CALLBACK RegFolderContextMenuCallback(IShellFolder *psf,
                                              HWND         hwnd,
                                              IDataObject  *pdtobj,
                                              UINT         uMsg,
                                              WPARAM       wParam,
                                              LPARAM       lParam)
{
    if (uMsg != DFM_INVOKECOMMAND || wParam != DFM_CMD_PROPERTIES)
        return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);

    PIDLIST_ABSOLUTE pidlFolder;
    PUITEMID_CHILD *apidl;
    UINT cidl;
    HRESULT hr = SH_GetApidlFromDataObject(pdtobj, &pidlFolder, &apidl, &cidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (_ILIsMyComputer(apidl[0]))
    {
        if (!SHELL_ExecuteControlPanelCPL(hwnd, L"sysdm.cpl"))
        {
            hr = E_FAIL;
        }
    }
    else if (_ILIsDesktop(apidl[0]))
    {
        if (!SHELL_ExecuteControlPanelCPL(hwnd, L"desk.cpl"))
        {
            hr = E_FAIL;
        }
    }
    else if (_ILIsNetHood(apidl[0]))
    {
        // FIXME path!
        if (32 >= (UINT_PTR)ShellExecuteW(NULL,
                                          L"open",
                                          L"explorer.exe",
                                          L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}",
                                          NULL,
                                          SW_SHOWDEFAULT))
        {
            hr = E_FAIL;
        }
    }
    else if (_ILIsBitBucket(apidl[0]))
    {
        /* FIXME: detect the drive path of bitbucket if appropiate */
        if (!SH_ShowRecycleBinProperties(L'C'))
            hr = E_FAIL;
    }
    else
    {
        /* Tell the caller to run the default action */
        hr = S_FALSE;
    }

    SHFree(pidlFolder);
    _ILFreeaPidl(apidl, cidl);

    return hr;
}

HRESULT CGuidItemContextMenu_CreateInstance(PCIDLIST_ABSOLUTE pidlFolder,
                                            HWND hwnd,
                                            UINT cidl,
                                            PCUITEMID_CHILD_ARRAY apidl,
                                            IShellFolder *psf,
                                            IContextMenu **ppcm)
{
    HKEY hKeys[10];
    UINT cKeys = 0;

    GUID *pGuid = _ILGetGUIDPointer(apidl[0]);
    if (pGuid)
    {
        WCHAR key[60];
        wcscpy(key, L"CLSID\\");
        StringFromGUID2(*pGuid, &key[6], 39);
        AddClassKeyToArray(key, hKeys, &cKeys);
    }

    // FIXME: CRegFolder should be aggregated by its outer folder and should
    // provide the attributes for all required non-registry folders.
    // It currently does not so we have to ask the outer folder ourself so
    // that we get the correct attributes for My Computer etc.
    CComPtr<IShellFolder> pOuterSF;
    SHBindToObject(NULL, pidlFolder, IID_PPV_ARG(IShellFolder, &pOuterSF));

    SFGAOF att = (psf && cidl) ? SHGetAttributes(pOuterSF ? pOuterSF.p : psf, apidl[0], SFGAO_FOLDER) : 0;
    if (att & SFGAO_FOLDER)
        AddClassKeyToArray(L"Folder", hKeys, &cKeys);

    return CDefFolderMenu_Create2(pidlFolder, hwnd, cidl, apidl, psf, RegFolderContextMenuCallback, cKeys, hKeys, ppcm);
}

HRESULT FormatGUIDKey(LPWSTR KeyName, SIZE_T KeySize, LPCWSTR RegPath, const GUID* riid)
{
    WCHAR xriid[40];

    if (!StringFromGUID2(*riid, xriid, _countof(xriid) - 1))
        return E_FAIL;

    return StringCchPrintfW(KeyName, KeySize, RegPath, xriid);
}

HRESULT CGuidItemExtractIcon_CreateInstance(LPCITEMIDLIST pidl, REFIID iid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit>    initIcon;
    HRESULT hr;
    GUID const * riid;
    int icon_idx;
    WCHAR wTemp[MAX_PATH];

    hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit,&initIcon));
    if (FAILED(hr))
        return hr;

    if (_ILIsDesktop(pidl))
    {
        initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_DESKTOP);
        return initIcon->QueryInterface(iid, ppvOut);
    }

    riid = _ILGetGUIDPointer(pidl);
    if (!riid)
        return E_FAIL;

    /* Choose a correct icon for Recycle Bin (full or empty) */
    const WCHAR* iconname = NULL;
    if (_ILIsBitBucket(pidl))
    {
        CComPtr<IEnumIDList> EnumIDList;
        CoInitialize(NULL);

        CComPtr<IShellFolder2> psfRecycleBin;
        CComPtr<IShellFolder> psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);

        if (SUCCEEDED(hr))
            hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder2, &psfRecycleBin));
        if (SUCCEEDED(hr))
            hr = psfRecycleBin->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &EnumIDList);

        ULONG itemcount;
        LPITEMIDLIST pidl = NULL;
        if (SUCCEEDED(hr) && (hr = EnumIDList->Next(1, &pidl, &itemcount)) == S_OK)
        {
            CoTaskMemFree(pidl);
            iconname = L"Full";
        } else {
            iconname = L"Empty";
        }
    }

    /* Prepare registry path for loading icons of My Computer and other shell extensions */
    WCHAR KeyName[MAX_PATH];

    hr = FormatGUIDKey(KeyName, _countof(KeyName),
                       L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\%s",
                       riid);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Load icon for the current user */
    BOOL ret = HCU_GetIconW(KeyName, wTemp, iconname, _countof(wTemp), &icon_idx);
    if (!ret)
    {
        /* Failed, load default system-wide icon */
        hr = FormatGUIDKey(KeyName, _countof(KeyName), L"CLSID\\%s", riid);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        ret = HCR_GetIconW(KeyName, wTemp, iconname, _countof(wTemp), &icon_idx);
    }

    if (ret)
    {
        /* Success, set loaded icon */
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else
    {
        /* Everything has failed, set blank paper icon */
        WARN("Failed to load an icon for the item, setting blank icon\n");
        initIcon->SetNormalIcon(swShell32Name, IDI_SHELL_DOCUMENT - 1);
    }

    return initIcon->QueryInterface(iid, ppvOut);
}

class CRegFolderEnum :
    public CEnumIDListBase
{
    public:
        CRegFolderEnum();
        ~CRegFolderEnum();
        HRESULT Initialize(LPCWSTR lpszEnumKeyName, DWORD dwFlags);
        HRESULT AddItemsFromKey(HKEY hkey_root, LPCWSTR szRepPath);

        BEGIN_COM_MAP(CRegFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

CRegFolderEnum::CRegFolderEnum()
{
}

CRegFolderEnum::~CRegFolderEnum()
{
}

HRESULT CRegFolderEnum::Initialize(LPCWSTR lpszEnumKeyName, DWORD dwFlags)
{
    WCHAR KeyName[MAX_PATH];

    if (!(dwFlags & SHCONTF_FOLDERS))
        return S_OK;

    HRESULT hr = StringCchPrintfW(KeyName, MAX_PATH,
                                  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\%s\\Namespace",
                                  lpszEnumKeyName);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    AddItemsFromKey(HKEY_LOCAL_MACHINE, KeyName);
    AddItemsFromKey(HKEY_CURRENT_USER, KeyName);

    return S_OK;
}

HRESULT CRegFolderEnum::AddItemsFromKey(HKEY hkey_root, LPCWSTR szRepPath)
{
    WCHAR name[MAX_PATH];
    HKEY hkey;

    if (RegOpenKeyW(hkey_root, szRepPath, &hkey) != ERROR_SUCCESS)
        return S_FALSE;

    for (int idx = 0; ; idx++)
    {
        if (RegEnumKeyW(hkey, idx, name, MAX_PATH) != ERROR_SUCCESS)
            break;

        /* If the name of the key is not a guid try to get the default value of the key */
        if (name[0] != L'{')
        {
            DWORD dwSize = sizeof(name);
            RegGetValueW(hkey, name, NULL, RRF_RT_REG_SZ, NULL, name, &dwSize);
        }

        if (*name == '{')
        {
            LPITEMIDLIST pidl = _ILCreateGuidFromStrW(name);

            if (pidl)
                AddToEnumList(pidl);
        }
    }

    RegCloseKey(hkey);

    return S_OK;
}

/*
 * These columns try to map to CFSFolder's columns because the CDesktopFolder
 * displays CFSFolder and CRegFolder items in the same view.
 */
enum REGFOLDERCOLUMNINDEX
{
    COL_NAME = SHFSF_COL_NAME,
    COL_TYPE = SHFSF_COL_TYPE,
    COL_INFOTIP = SHFSF_COL_COMMENT,
    REGFOLDERCOLUMNCOUNT = max(COL_INFOTIP, COL_TYPE) + 1
};

class CRegFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2
{
    private:
        GUID m_guid;
        CAtlStringW m_rootPath;
        CAtlStringW m_enumKeyName;
        CComHeapPtr<ITEMIDLIST> m_pidlRoot;

        HRESULT GetGuidItemAttributes (LPCITEMIDLIST pidl, LPDWORD pdwAttributes);
        BOOL _IsInNameSpace(_In_ LPCITEMIDLIST pidl);

    public:
        CRegFolder();
        ~CRegFolder();
        HRESULT WINAPI Initialize(const GUID *pGuid, LPCITEMIDLIST pidlRoot, LPCWSTR lpszPath, LPCWSTR lpszEnumKeyName);

        // IShellFolder
        STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes) override;
        STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override;
        STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(CompareIDs)(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override;
        STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(GetAttributesOf)(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override;
        STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) override;
        STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override;
        STDMETHOD(SetNameOf)(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut) override;

        /* ShellFolder2 */
        STDMETHOD(GetDefaultSearchGUID)(GUID *pguid) override;
        STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum) override;
        STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) override;
        STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pcsFlags) override;
        STDMETHOD(GetDetailsEx)(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override;
        STDMETHOD(GetDetailsOf)(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;
        STDMETHOD(MapColumnToSCID)(UINT column, SHCOLUMNID *pscid) override;

        DECLARE_NOT_AGGREGATABLE(CRegFolder)

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CRegFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        END_COM_MAP()
};

CRegFolder::CRegFolder()
{
}

CRegFolder::~CRegFolder()
{
}

HRESULT WINAPI CRegFolder::Initialize(const GUID *pGuid, LPCITEMIDLIST pidlRoot, LPCWSTR lpszPath, LPCWSTR lpszEnumKeyName)
{
    memcpy(&m_guid, pGuid, sizeof(m_guid));

    m_rootPath = lpszPath;
    if (!m_rootPath)
        return E_OUTOFMEMORY;

    m_enumKeyName = lpszEnumKeyName;
    if (!m_enumKeyName)
        return E_OUTOFMEMORY;

    m_pidlRoot.Attach(ILClone(pidlRoot));
    if (!m_pidlRoot)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT CRegFolder::GetGuidItemAttributes (LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    DWORD dwAttributes = *pdwAttributes;

    /* First try to get them from the registry */
    if (!HCR_GetFolderAttributes(pidl, pdwAttributes))
    {
        /* We couldn't get anything */
        *pdwAttributes = 0;
    }

    /* Items have more attributes when on desktop */
    if (_ILIsDesktop(m_pidlRoot))
    {
        *pdwAttributes |= (dwAttributes & (SFGAO_CANLINK|SFGAO_CANDELETE|SFGAO_CANRENAME|SFGAO_HASPROPSHEET));
    }

    /* In any case, links can be created */
    *pdwAttributes |= (dwAttributes & SFGAO_CANLINK);
    return S_OK;
}

BOOL CRegFolder::_IsInNameSpace(_In_ LPCITEMIDLIST pidl)
{
    CLSID clsid = *_ILGetGUIDPointer(pidl);
    if (IsEqualGUID(clsid, CLSID_Printers))
        return TRUE;
    if (IsEqualGUID(clsid, CLSID_ConnectionFolder))
        return TRUE;
    FIXME("Check registry\n");
    return TRUE;
}

HRESULT WINAPI CRegFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

    if (!lpszDisplayName)
        return E_INVALIDARG;

    if (lpszDisplayName[0] != L':' || lpszDisplayName[1] != L':')
    {
        FIXME("What should we do here?\n");
        return E_FAIL;
    }

    LPWSTR pch, pchNextOfComma = NULL;
    for (pch = &lpszDisplayName[2]; *pch && *pch != L'\\'; ++pch)
    {
        if (*pch == L',' && !pchNextOfComma)
            pchNextOfComma = pch + 1;
    }

    CLSID clsid;
    if (!GUIDFromStringW(&lpszDisplayName[2], &clsid))
        return CO_E_CLASSSTRING;

    if (pchNextOfComma)
    {
        FIXME("Delegate folder\n");
        return E_FAIL;
    }

    CComHeapPtr<ITEMIDLIST> pidlTemp(_ILCreateGuid(PT_GUID, clsid));
    if (!pidlTemp)
        return E_OUTOFMEMORY;

    if (!_IsInNameSpace(pidlTemp) && !(BindCtx_GetMode(pbc, 0) & STGM_CREATE))
        return E_INVALIDARG;

    *ppidl = pidlTemp.Detach();

    if (!*pch)
    {
        if (pdwAttributes && *pdwAttributes)
            GetGuidItemAttributes(*ppidl, pdwAttributes);

        return S_OK;
    }

    HRESULT hr = SHELL32_ParseNextElement(this, hwndOwner, pbc, ppidl, pch + 1, pchEaten,
                                          pdwAttributes);
    if (FAILED(hr))
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }
    return hr;
}

HRESULT WINAPI CRegFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CRegFolderEnum>(m_enumKeyName, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

HRESULT WINAPI CRegFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    CComPtr<IPersistFolder> pFolder;
    HRESULT hr;

    if (!ppvOut || !pidl || !pidl->mkid.cb)
        return E_INVALIDARG;

    *ppvOut = NULL;

    GUID *pGUID = _ILGetGUIDPointer(pidl);
    if (!pGUID)
    {
        ERR("CRegFolder::BindToObject called for non guid item!\n");
        return E_INVALIDARG;
    }

    hr = SHELL32_BindToSF(m_pidlRoot, NULL, pidl, pGUID, riid, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT WINAPI CRegFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CRegFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    if (!pidl1 || !pidl2 || pidl1->mkid.cb == 0 || pidl2->mkid.cb == 0)
    {
        ERR("Got an empty pidl!\n");
        return E_INVALIDARG;
    }

    GUID const *clsid1 = _ILGetGUIDPointer (pidl1);
    GUID const *clsid2 = _ILGetGUIDPointer (pidl2);

    if (!clsid1 && !clsid2)
    {
        ERR("Got no guid pidl!\n");
        return E_INVALIDARG;
    }
    else if (clsid1 && clsid2)
    {
        if (memcmp(clsid1, clsid2, sizeof(GUID)) == 0)
            return SHELL32_CompareChildren(this, lParam, pidl1, pidl2);

        return SHELL32_CompareDetails(this, lParam, pidl1, pidl2);
    }

    /* Guid folders come first compared to everything else */
    /* And Drives come before folders in My Computer */
    if (_ILIsMyComputer(m_pidlRoot))
    {
        return MAKE_COMPARE_HRESULT(clsid1 ? 1 : -1);
    }
    else
    {
        return MAKE_COMPARE_HRESULT(clsid1 ? -1 : 1);
    }
}

HRESULT WINAPI CRegFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CRegFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    if (!rgfInOut || !cidl || !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    while(cidl > 0 && *apidl)
    {
        if (_ILIsSpecialFolder(*apidl))
            GetGuidItemAttributes(*apidl, rgfInOut);
        else
            ERR("Got unknown pidl\n");
        apidl++;
        cidl--;
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    return S_OK;
}

HRESULT WINAPI CRegFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if ((IsEqualIID (riid, IID_IExtractIconA) || IsEqualIID (riid, IID_IExtractIconW)) && (cidl == 1))
    {
        hr = CGuidItemExtractIcon_CreateInstance(apidl[0], riid, &pObj);
    }
    else if (IsEqualIID (riid, IID_IContextMenu) && (cidl >= 1))
    {
        if (!_ILGetGUIDPointer (apidl[0]))
        {
            ERR("Got non guid item!\n");
            return E_FAIL;
        }

        hr = CGuidItemContextMenu_CreateInstance(m_pidlRoot, hwndOwner, cidl, apidl, static_cast<IShellFolder*>(this), (IContextMenu**)&pObj);
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor (hwndOwner, m_pidlRoot, apidl, cidl, TRUE, (IDataObject **)&pObj);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    *ppvOut = pObj;
    return hr;

}

HRESULT WINAPI CRegFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!strRet || (!_ILIsSpecialFolder(pidl) && pidl != NULL))
        return E_INVALIDARG;

    if (!pidl || !pidl->mkid.cb)
    {
        if ((GET_SHGDN_RELATION(dwFlags) == SHGDN_NORMAL) && (GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING))
        {
            LPWSTR pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
            if (!pszPath)
                return E_OUTOFMEMORY;

            /* parsing name like ::{...} */
            pszPath[0] = ':';
            pszPath[1] = ':';
            SHELL32_GUIDToStringW(m_guid, &pszPath[2]);

            strRet->uType = STRRET_WSTR;
            strRet->pOleStr = pszPath;

            return S_OK;
        }
        else
        {
            BOOL bRet;
            WCHAR wstrName[MAX_PATH+1];
            bRet = HCR_GetClassNameW(m_guid, wstrName, MAX_PATH);
            if (!bRet)
                return E_FAIL;

            return SHSetStrRet(strRet, wstrName);

        }
    }

    HRESULT hr;
    GUID const *clsid = _ILGetGUIDPointer (pidl);

    /* First of all check if we need to query the name from the child item */
    if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING &&
        GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL)
    {
        int bWantsForParsing = FALSE;

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

        if (bWantsForParsing)
        {
            /*
             * we need the filesystem path to the destination folder.
             * Only the folder itself can know it
             */
            return SHELL32_GetDisplayNameOfChild (this, pidl, dwFlags, strRet);
        }
    }

    /* Allocate the buffer for the result */
    SIZE_T cchPath = MAX_PATH + 1;
    LPWSTR pszPath = (LPWSTR)CoTaskMemAlloc(cchPath * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    hr = S_OK;

    if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING)
    {
        SIZE_T pathlen = 0;
        PWCHAR pItemName = pszPath; // GET_SHGDN_RELATION(dwFlags) == SHGDN_INFOLDER
        if (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER)
        {
            hr = StringCchCopyW(pszPath, cchPath, m_rootPath);
            if (SUCCEEDED(hr))
            {
                pathlen = wcslen(pszPath);
                pItemName = &pszPath[pathlen];
                if (pathlen)
                {
                    if (++pathlen < cchPath)
                        *pItemName++ = L'\\';
                    else
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                }
            }
        }

        if (SUCCEEDED(hr) && pathlen + 2 + 38 + 1 < cchPath)
        {
            /* parsing name like ::{...} */
            pItemName[0] = L':';
            pItemName[1] = L':';
            SHELL32_GUIDToStringW(*clsid, &pItemName[2]);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    else
    {
        /* user friendly name */
        if (!HCR_GetClassNameW(*clsid, pszPath, cchPath))
            hr = E_FAIL;
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

HRESULT WINAPI CRegFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    GUID const *clsid = _ILGetGUIDPointer (pidl);
    LPOLESTR pStr;
    HRESULT hr;
    WCHAR szName[100];

    if (!clsid)
    {
        ERR("Pidl is not reg item!\n");
        return E_FAIL;
    }

    hr = StringFromCLSID(*clsid, &pStr);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    swprintf(szName, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\%s", pStr);

    DWORD cbData = (wcslen(lpName) + 1) * sizeof(WCHAR);
    LONG res = SHSetValueW(HKEY_CURRENT_USER, szName, NULL, RRF_RT_REG_SZ, lpName, cbData);

    CoTaskMemFree(pStr);

    if (res == ERROR_SUCCESS)
    {
        *pPidlOut = ILClone(pidl);
        return S_OK;
    }

    return E_FAIL;
}


HRESULT WINAPI CRegFolder::GetDefaultSearchGUID(GUID *pguid)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CRegFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CRegFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CRegFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    if (iColumn >= REGFOLDERCOLUMNCOUNT)
        return E_INVALIDARG;
    *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
    return S_OK;
}

HRESULT WINAPI CRegFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CRegFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    if (!psd)
        return E_INVALIDARG;

    if (!pidl)
    {
        TRACE("CRegFolder has no column info\n");
        return E_INVALIDARG;
    }

    GUID const *clsid = _ILGetGUIDPointer (pidl);

    if (!clsid)
    {
        ERR("Pidl is not reg item!\n");
        return E_INVALIDARG;
    }

    switch(iColumn)
    {
        case COL_NAME:
            return GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
        case COL_TYPE:
            return SHSetStrRet(&psd->str, IDS_SYSTEMFOLDER);
        case COL_INFOTIP:
            HKEY hKey;
            if (!HCR_RegOpenClassIDKey(*clsid, &hKey))
                return SHSetStrRet(&psd->str, "");

            psd->str.cStr[0] = 0x00;
            psd->str.uType = STRRET_CSTR;
            RegLoadMUIStringA(hKey, "InfoTip", psd->str.cStr, MAX_PATH, NULL, 0, NULL);
            RegCloseKey(hKey);
            return S_OK;
        default:
            /* Return an empty string when we area asked for a column we don't support.
               Only  the regfolder is supposed to do this as it supports less columns compared to other folder
               and its contents are supposed to be presented alongside items that support more columns. */
            return SHSetStrRet(&psd->str, "");
    }
    return E_FAIL;
}

HRESULT WINAPI CRegFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    return E_NOTIMPL;
}

/* In latest windows version this is exported but it takes different arguments! */
HRESULT CRegFolder_CreateInstance(const GUID *pGuid, LPCITEMIDLIST pidlRoot, LPCWSTR lpszPath, LPCWSTR lpszEnumKeyName, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CRegFolder>(pGuid, pidlRoot, lpszPath, lpszEnumKeyName, riid, ppv);
}
