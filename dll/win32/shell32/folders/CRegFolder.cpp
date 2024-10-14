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
 *
 * The required-regitem design is based on the research by Geoff Chappell
 * https://www.geoffchappell.com/studies/windows/shell/shell32/classes/regfolder.htm
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

#define DEFAULTSORTORDERINDEX 0x80 // The default for registry items according to Geoff Chappell

static HRESULT CRegItemContextMenu_CreateInstance(PCIDLIST_ABSOLUTE pidlFolder, HWND hwnd, UINT cidl,
                                                  PCUITEMID_CHILD_ARRAY apidl, IShellFolder *psf, IContextMenu **ppcm);

static inline UINT GetRegItemCLSIDOffset(PIDLTYPE type)
{
    return type == PT_CONTROLS_NEWREGITEM ? 14 : 4;
}

static LPITEMIDLIST CreateRegItem(PIDLTYPE type, REFCLSID clsid, BYTE order = 0)
{
#if 1 // FIXME: CControlPanelFolder is not ready for this yet
    if (type == PT_CONTROLS_NEWREGITEM)
        type = PT_CONTROLS_OLDREGITEM;
#endif
    const UINT offset = GetRegItemCLSIDOffset(type);
    const UINT cb = offset + sizeof(CLSID), cbTotal = cb + sizeof(WORD);
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbTotal);
    if (pidl)
    {
        ZeroMemory(pidl, cbTotal); // Note: This also initializes the terminator WORD
        pidl->mkid.cb = cb;
        pidl->mkid.abID[0] = type;
        pidl->mkid.abID[1] = order;
        *(CLSID*)(SIZE_T(pidl) + offset) = clsid;
    }
    return pidl;
}

static LPITEMIDLIST CreateRegItem(PIDLTYPE type, LPCWSTR clsidstr)
{
    CLSID clsid;
    return SUCCEEDED(CLSIDFromString(clsidstr, &clsid)) ? CreateRegItem(type, clsid) : NULL;
}

HRESULT FormatGUIDKey(LPWSTR KeyName, SIZE_T KeySize, LPCWSTR RegPath, const GUID* riid)
{
    WCHAR xriid[CHARS_IN_GUID];
    StringFromGUID2(*riid, xriid, _countof(xriid));
    return StringCchPrintfW(KeyName, KeySize, RegPath, xriid);
}

static DWORD SHELL_QueryCLSIDValue(_In_ REFCLSID clsid, _In_opt_ LPCWSTR SubKey, _In_opt_ LPCWSTR Value, _In_opt_ PVOID pData, _In_opt_ PDWORD pSize)
{
    WCHAR Path[MAX_PATH];
    wcscpy(Path, L"CLSID\\");
    StringFromGUID2(clsid, Path + 6, 39);
    if (SubKey)
    {
        wcscpy(Path + 6 + 38, L"\\");
        wcscpy(Path + 6 + 39, SubKey);
    }
    return RegGetValueW(HKEY_CLASSES_ROOT, Path, Value, RRF_RT_ANY, NULL, pData, pSize);
}

static bool HasCLSIDShellFolderValue(REFCLSID clsid, LPCWSTR Value)
{
    return SHELL_QueryCLSIDValue(clsid, L"ShellFolder", Value, NULL, NULL) == ERROR_SUCCESS;
}

struct CRegFolderInfo
{
    const REGFOLDERINFO *m_pInfo;

    void InitializeFolderInfo(const REGFOLDERINFO *pInfo)
    {
        m_pInfo = pInfo;
    }

    const CLSID* IsRegItem(LPCITEMIDLIST pidl) const
    {
        if (pidl && pidl->mkid.cb >= sizeof(WORD) + 1 + 1 + sizeof(GUID))
        {
            if (pidl->mkid.abID[0] == m_pInfo->PidlType)
                return (CLSID*)(SIZE_T(pidl) + GetCLSIDOffset());
            if (pidl->mkid.abID[0] == PT_CONTROLS_OLDREGITEM)
                return (CLSID*)(SIZE_T(pidl) + GetRegItemCLSIDOffset(PT_CONTROLS_OLDREGITEM));
        }
        if (const IID* pIID = _ILGetGUIDPointer(pidl))
        {
            FIXME("Unexpected GUID PIDL type %#x\n", pidl->mkid.abID[0]);
            return pIID; // FIXME: Remove this when all folders have been fixed
        }
        return NULL;
    }

    LPITEMIDLIST CreateItem(size_t i) const
    {
        const REQUIREDREGITEM &item = GetAt(i);
        return CreateRegItem(GetPidlType(), item.clsid, item.Order);
    }

    LPCWSTR GetParsingPath() const { return m_pInfo->pszParsingPath; }
    UINT GetCLSIDOffset() const { return GetRegItemCLSIDOffset(m_pInfo->PidlType); }
    PIDLTYPE GetPidlType() const { return m_pInfo->PidlType; }
    UINT GetRequiredItemsCount() const { return m_pInfo->Count; }
    const REQUIREDREGITEM& GetAt(size_t i) const { return m_pInfo->Items[i]; }
};

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
    public CEnumIDListBase,
    public CRegFolderInfo
{
    SHCONTF m_SHCTF;
    public:
        HRESULT Initialize(const REGFOLDERINFO *pInfo, IShellFolder *pSF, DWORD dwFlags);
        HRESULT AddItemsFromKey(IShellFolder *pSF, HKEY hkey_root, LPCWSTR szRepPath);

        const CLSID* GetPidlClsid(PCUITEMID_CHILD pidl) { return IsRegItem(pidl); }
        BOOL HasItemWithCLSID(LPCITEMIDLIST pidl) { return HasItemWithCLSIDImpl<CRegFolderEnum>(pidl); }

        BEGIN_COM_MAP(CRegFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

HRESULT CRegFolderEnum::Initialize(const REGFOLDERINFO *pInfo, IShellFolder *pSF, DWORD dwFlags)
{
    InitializeFolderInfo(pInfo);
    m_SHCTF = (SHCONTF)dwFlags;
    if (!(dwFlags & SHCONTF_FOLDERS))
        return S_OK;

    WCHAR KeyName[MAX_PATH];
    HRESULT hr = StringCchPrintfW(KeyName, _countof(KeyName),
                                  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\%s\\Namespace",
                                  pInfo->pszEnumKeyName);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // First add the required items and then the items from the registry
    SFGAOF query = SHELL_CreateFolderEnumItemAttributeQuery(m_SHCTF, TRUE);
    for (size_t i = 0; i < GetRequiredItemsCount(); ++i)
    {
        LPITEMIDLIST pidl = CreateItem(i);
        if (pidl && SHELL_IncludeItemInFolderEnum(pSF, pidl, query, m_SHCTF))
            AddToEnumList(pidl);
        else
            ILFree(pidl);
    }
    AddItemsFromKey(pSF, HKEY_LOCAL_MACHINE, KeyName);
    AddItemsFromKey(pSF, HKEY_CURRENT_USER, KeyName);
    return S_OK;
}

HRESULT CRegFolderEnum::AddItemsFromKey(IShellFolder *pSF, HKEY hkey_root, LPCWSTR szRepPath)
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
            if (LPITEMIDLIST pidl = CreateRegItem(GetPidlType(), name))
            {
                SFGAOF query = SHELL_CreateFolderEnumItemAttributeQuery(m_SHCTF, TRUE);
                if (SHELL_IncludeItemInFolderEnum(pSF, pidl, query, m_SHCTF) && !HasItemWithCLSID(pidl))
                    AddToEnumList(pidl);
                else
                    ILFree(pidl);
            }
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
    public IShellFolder2,
    public CRegFolderInfo
{
    private:
        IShellFolder *m_pOuterFolder; // Not ref-counted
        CComHeapPtr<ITEMIDLIST> m_pidlRoot;

        HRESULT GetGuidItemAttributes (LPCITEMIDLIST pidl, LPDWORD pdwAttributes);
        BOOL _IsInNameSpace(_In_ LPCITEMIDLIST pidl);

    public:
        CRegFolder();
        ~CRegFolder();
        HRESULT WINAPI Initialize(PREGFOLDERINITDATA pInit, LPCITEMIDLIST pidlRoot);

        const REQUIREDREGITEM* IsRequiredItem(LPCITEMIDLIST pidl) const
        {
            const CLSID* const pCLSID = IsRegItem(pidl);
            for (size_t i = 0; pCLSID && i < GetRequiredItemsCount(); ++i)
            {
                const REQUIREDREGITEM &item = GetAt(i);
                if (item.clsid == *pCLSID)
                    return &item;
            }
            return NULL;
        }

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

HRESULT WINAPI CRegFolder::Initialize(PREGFOLDERINITDATA pInit, LPCITEMIDLIST pidlRoot)
{
    InitializeFolderInfo(pInit->pInfo);
    m_pOuterFolder = pInit->psfOuter;

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

    CComHeapPtr<ITEMIDLIST> pidlTemp(CreateRegItem(GetPidlType(), clsid));
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
    return ShellObjectCreatorInit<CRegFolderEnum>(m_pInfo, m_pOuterFolder, dwFlags,
                                                  IID_PPV_ARG(IEnumIDList, ppEnumIDList));
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
    if (GetPidlType() == PT_COMPUTER_REGITEM)
        return MAKE_COMPARE_HRESULT(clsid1 ? 1 : -1);
    else
        return MAKE_COMPARE_HRESULT(clsid1 ? -1 : 1);
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

        hr = CRegItemContextMenu_CreateInstance(m_pidlRoot, hwndOwner, cidl, apidl, static_cast<IShellFolder*>(this), (IContextMenu**)&pObj);
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
        if (IS_SHGDN_FOR_PARSING(dwFlags))
        {
            if (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER)
            {
                TRACE("GDNO returning INFOLDER instead of %#x\n", GET_SHGDN_RELATION(dwFlags));
            }
            LPWSTR pszPath = (LPWSTR)CoTaskMemAlloc((2 + 38 + 1) * sizeof(WCHAR));
            if (!pszPath)
                return E_OUTOFMEMORY;
            /* parsing name like ::{...} */
            pszPath[0] = ':';
            pszPath[1] = ':';
            SHELL32_GUIDToStringW(m_pInfo->clsid, &pszPath[2]);
            strRet->uType = STRRET_WSTR;
            strRet->pOleStr = pszPath;
            return S_OK;
        }
        else
        {
            BOOL bRet;
            WCHAR wstrName[MAX_PATH+1];
            bRet = HCR_GetClassNameW(m_pInfo->clsid, wstrName, MAX_PATH);
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
            hr = StringCchCopyW(pszPath, cchPath, GetParsingPath());
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
        return pPidlOut ? SHILClone(pidl, pPidlOut) : S_OK;
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

static HRESULT CALLBACK RegFolderContextMenuCallback(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
                                                     UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != DFM_INVOKECOMMAND || wParam != DFM_CMD_PROPERTIES)
        return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);

    PIDLIST_ABSOLUTE pidlFolder;
    PUITEMID_CHILD *apidl;
    UINT cidl;
    HRESULT hr = SH_GetApidlFromDataObject(pdtobj, &pidlFolder, &apidl, &cidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CRegFolder *pRegFolder = static_cast<CRegFolder*>(psf);
    const REQUIREDREGITEM* pRequired = pRegFolder->IsRequiredItem(apidl[0]);
    if (pRequired && pRequired->pszCpl)
    {
        WCHAR buf[MAX_PATH];
        wsprintfW(buf, L"%hs", const_cast<LPCSTR>(pRequired->pszCpl));
        hr = SHELL_ExecuteControlPanelCPL(hwnd, buf) ? S_OK : E_FAIL;
    }
#if 0 // Should never happen, CDesktopFolder.cpp handles this
    else if (_ILIsDesktop(pidlFolder) && _ILIsDesktop(apidl[0]))
    {
        hr = SHELL_ExecuteControlPanelCPL(hwnd, L"desk.cpl") ? S_OK : E_FAIL;
    }
#endif
    else if (_ILIsDesktop(pidlFolder) && _ILIsBitBucket(apidl[0]))
    {
        FIXME("Use SHOpenPropSheet on Recyclers PropertySheetHandlers from the registry\n");
        hr = SH_ShowRecycleBinProperties(L'C') ? S_OK : E_FAIL;
    }
    else
    {
        hr = S_FALSE; // Tell the caller to run the default action
    }

    SHFree(pidlFolder);
    _ILFreeaPidl(apidl, cidl);
    return hr;
}

static HRESULT CRegItemContextMenu_CreateInstance(PCIDLIST_ABSOLUTE pidlFolder, HWND hwnd, UINT cidl,
                                                  PCUITEMID_CHILD_ARRAY apidl, IShellFolder *psf, IContextMenu **ppcm)
{
    HKEY hKeys[3];
    UINT cKeys = 0;

    const GUID *pGuid = _ILGetGUIDPointer(apidl[0]);
    if (pGuid)
    {
        WCHAR key[sizeof("CLSID\\") + 38];
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
    if ((att & SFGAO_FOLDER) && (!pGuid || !HasCLSIDShellFolderValue(*pGuid, L"HideFolderVerbs")))
        AddClassKeyToArray(L"Folder", hKeys, &cKeys);

    return CDefFolderMenu_Create2(pidlFolder, hwnd, cidl, apidl, psf, RegFolderContextMenuCallback, cKeys, hKeys, ppcm);
}

/* In latest windows version this is exported but it takes different arguments! */
HRESULT CRegFolder_CreateInstance(PREGFOLDERINITDATA pInit, LPCITEMIDLIST pidlRoot, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CRegFolder>(pInit, pidlRoot, riid, ppv);
}
