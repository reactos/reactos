/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/ntobjshex/ntobjfolder.h
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */
#pragma once

extern const GUID CLSID_NtObjectFolder;

class CFolderViewCB :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolderViewCB
{
    IShellView* m_View;

public:

    CFolderViewCB() : m_View(NULL) {}
    virtual ~CFolderViewCB() {}

    virtual HRESULT STDMETHODCALLTYPE MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case SFVM_DEFVIEWMODE:
        {
            FOLDERVIEWMODE* pViewMode = (FOLDERVIEWMODE*)lParam;
            *pViewMode = FVM_DETAILS;
            return S_OK;
        }
        case SFVM_COLUMNCLICK:
            return S_FALSE;
        case SFVM_BACKGROUNDENUM:
            return S_OK;
        }

        DbgPrint("MessageSFVCB unimplemented %d %08x %08x\n", uMsg, wParam, lParam);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Initialize(IShellView* psv)
    {
        m_View = psv;
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CFolderViewCB)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
    END_COM_MAP()
};

template<class TSelf, typename TItemId, class TExtractIcon>
class CCommonFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2
{
protected:
    WCHAR m_NtPath[MAX_PATH];

    LPITEMIDLIST m_shellPidl;

public:

    CCommonFolder() :
        m_shellPidl(NULL)
    {
    }

    virtual ~CCommonFolder()
    {
        if (m_shellPidl)
            ILFree(m_shellPidl);
    }

    // IShellFolder
    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName(
        HWND hwndOwner,
        LPBC pbcReserved,
        LPOLESTR lpszDisplayName,
        ULONG *pchEaten,
        LPITEMIDLIST *ppidl,
        ULONG *pdwAttributes)
    {
        if (!ppidl)
            return E_POINTER;

        if (pchEaten)
            *pchEaten = 0;

        if (pdwAttributes)
            *pdwAttributes = 0;

        TRACE("CCommonFolder::ParseDisplayName name=%S (ntPath=%S)\n", lpszDisplayName, m_NtPath);

        const TItemId * info;
        IEnumIDList * it;
        HRESULT hr = EnumObjects(hwndOwner, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &it);
        if (FAILED(hr))
            return hr;

        PWSTR end = StrChrW(lpszDisplayName, '\\');
        int length = end ? end - lpszDisplayName : wcslen(lpszDisplayName);

        while (TRUE)
        {
            hr = it->Next(1, ppidl, NULL);

            if (FAILED(hr))
                return hr;

            if (hr != S_OK)
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

            hr = GetInfoFromPidl(*ppidl, &info);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (StrCmpNW(info->entryName, lpszDisplayName, length) == 0)
                break;
        }

        // if has remaining path to parse (and the path didn't just terminate in a backslash)
        if (end && wcslen(end) > 1)
        {
            CComPtr<IShellFolder> psfChild;
            hr = BindToObject(*ppidl, pbcReserved, IID_PPV_ARG(IShellFolder, &psfChild));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            LPITEMIDLIST child;
            hr = psfChild->ParseDisplayName(hwndOwner, pbcReserved, end + 1, pchEaten, &child, pdwAttributes);
            if (FAILED(hr))
                return hr;

            LPITEMIDLIST old = *ppidl;
            *ppidl = ILCombine(old, child);
            ILFree(old);

            // Count the path separator
            if (pchEaten)
                (*pchEaten) += 1;
        }
        else
        {
            if (pdwAttributes)
                *pdwAttributes = ConvertAttributes(info, pdwAttributes);
        }

        if (pchEaten)
            *pchEaten += wcslen(info->entryName);

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE EnumObjects(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList) PURE;

    virtual HRESULT STDMETHODCALLTYPE BindToObject(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        void **ppvOut)
    {
        const TItemId * info;

        if (IsEqualIID(riid, IID_IShellFolder))
        {
            HRESULT hr = GetInfoFromPidl(pidl, &info);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            WCHAR path[MAX_PATH];

            StringCbCopyW(path, sizeof(path), m_NtPath);
            PathAppendW(path, info->entryName);

            LPITEMIDLIST first = ILCloneFirst(pidl);
            LPCITEMIDLIST rest = ILGetNext(pidl);

            LPITEMIDLIST fullPidl = ILCombine(m_shellPidl, first);

            CComPtr<IShellFolder> psfChild;
            hr = InternalBindToObject(path, info, first, rest, fullPidl, pbcReserved, &psfChild);

            ILFree(fullPidl);
            ILFree(first);

            if (FAILED(hr))
                return hr;

            if (hr == S_FALSE)
                return S_OK;

            if (rest->mkid.cb > 0)
            {
                return psfChild->BindToObject(rest, pbcReserved, riid, ppvOut);
            }

            return psfChild->QueryInterface(riid, ppvOut);
        }

        return E_NOTIMPL;
    }

protected:
    virtual HRESULT STDMETHODCALLTYPE InternalBindToObject(
        PWSTR path,
        const TItemId * info,
        LPITEMIDLIST first,
        LPCITEMIDLIST rest,
        LPITEMIDLIST fullPidl,
        LPBC pbcReserved,
        IShellFolder** ppsfChild) PURE;

    virtual HRESULT STDMETHODCALLTYPE ResolveSymLink(
        const TItemId * info,
        LPITEMIDLIST * fullPidl)
    {
        return E_NOTIMPL;
    }

public:

    virtual HRESULT STDMETHODCALLTYPE BindToStorage(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        void **ppvObj)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CompareIDs(
        LPARAM lParam,
        LPCITEMIDLIST pidl1,
        LPCITEMIDLIST pidl2)
    {
        HRESULT hr;

        TRACE("CompareIDs %d\n", lParam);

        const TItemId * id1;
        hr = GetInfoFromPidl(pidl1, &id1);
        if (FAILED(hr))
            return E_INVALIDARG;

        const TItemId * id2;
        hr = GetInfoFromPidl(pidl2, &id2);
        if (FAILED(hr))
            return E_INVALIDARG;

        hr = CompareIDs(lParam, id1, id2);
        if (hr != S_EQUAL)
            return hr;

        // The wollowing snipped is basically SHELL32_CompareChildren

        PUIDLIST_RELATIVE rest1 = ILGetNext(pidl1);
        PUIDLIST_RELATIVE rest2 = ILGetNext(pidl2);

        bool isEmpty1 = (rest1->mkid.cb == 0);
        bool isEmpty2 = (rest2->mkid.cb == 0);

        if (isEmpty1 || isEmpty2)
            return MAKE_COMPARE_HRESULT(isEmpty2 - isEmpty1);

        LPCITEMIDLIST first1 = ILCloneFirst(pidl1);
        if (!first1)
            return E_OUTOFMEMORY;

        CComPtr<IShellFolder> psfNext;
        hr = BindToObject(first1, NULL, IID_PPV_ARG(IShellFolder, &psfNext));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psfNext->CompareIDs(lParam, rest1, rest2);
    }

protected:
    virtual HRESULT STDMETHODCALLTYPE CompareName(
        LPARAM lParam,
        const TItemId * first,
        const TItemId * second)
    {
        bool f1 = IsFolder(first);
        bool f2 = IsFolder(second);

        HRESULT hr = MAKE_COMPARE_HRESULT(f2 - f1);
        if (hr != S_EQUAL)
            return hr;

        bool canonical = (lParam & 0xFFFF0000) == SHCIDS_CANONICALONLY;
        if (canonical)
        {
            // Shortcut: avoid comparing contents if not necessary when the results are not for display.
            hr = MAKE_COMPARE_HRESULT(second->entryNameLength - first->entryNameLength);
            if (hr != S_EQUAL)
                return hr;

            int minlength = min(first->entryNameLength, second->entryNameLength);
            if (minlength > 0)
            {
                hr = MAKE_COMPARE_HRESULT(memcmp(first->entryName, second->entryName, minlength));
                if (hr != S_EQUAL)
                    return hr;
            }

            return S_EQUAL;
        }

        int minlength = min(first->entryNameLength, second->entryNameLength);
        if (minlength > 0)
        {
            hr = MAKE_COMPARE_HRESULT(StrCmpNW(first->entryName, second->entryName, minlength / sizeof(WCHAR)));
            if (hr != S_EQUAL)
                return hr;
        }

        return MAKE_COMPARE_HRESULT(second->entryNameLength - first->entryNameLength);
    }

public:
    virtual HRESULT STDMETHODCALLTYPE CreateViewObject(
        HWND hwndOwner,
        REFIID riid,
        void **ppvOut)
    {
        if (!IsEqualIID(riid, IID_IShellView))
            return E_NOINTERFACE;

        _CComObject<CFolderViewCB> *pcb;

        HRESULT hr = _CComObject<CFolderViewCB>::CreateInstance(&pcb);
        if (FAILED(hr))
            return hr;

        pcb->AddRef();

        SFV_CREATE sfv;
        sfv.cbSize = sizeof(sfv);
        sfv.pshf = this;
        sfv.psvOuter = NULL;
        sfv.psfvcb = pcb;

        IShellView* view;

        hr = SHCreateShellFolderView(&sfv, &view);
        if (FAILED(hr))
            return hr;

        pcb->Initialize(view);

        pcb->Release();

        *ppvOut = view;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetAttributesOf(
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        SFGAOF *rgfInOut)
    {
        const TItemId * info;

        TRACE("GetAttributesOf %d\n", cidl);

        if (cidl == 0)
        {
            *rgfInOut &= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;
            return S_OK;
        }

        for (int i = 0; i < (int)cidl; i++)
        {
            PCUITEMID_CHILD pidl = apidl[i];

            HRESULT hr = GetInfoFromPidl(pidl, &info);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            // Update attributes.
            *rgfInOut = ConvertAttributes(info, rgfInOut);
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetUIObjectOf(
        HWND hwndOwner,
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid,
        UINT *prgfInOut,
        void **ppvOut)
    {
        DWORD res;
        TRACE("GetUIObjectOf\n");

        if (IsEqualIID(riid, IID_IContextMenu) ||
            IsEqualIID(riid, IID_IContextMenu2) ||
            IsEqualIID(riid, IID_IContextMenu3))
        {
            CComPtr<IContextMenu> pcm;

            HKEY keys[1];

            LPITEMIDLIST parent = m_shellPidl;

            CComPtr<IShellFolder> psfParent = this;

            LPCITEMIDLIST child;

            int nkeys = _countof(keys);
            if (cidl == 1 && IsSymLink(apidl[0]))
            {
                const TItemId * info;
                HRESULT hr = GetInfoFromPidl(apidl[0], &info);
                if (FAILED(hr))
                    return hr;

                LPITEMIDLIST target;
                hr = ResolveSymLink(info, &target);
                if (FAILED(hr))
                    return hr;

                CComPtr<IShellFolder> psfTarget;
                hr = ::SHBindToParent(target, IID_PPV_ARG(IShellFolder, &psfTarget), &child);
                if (FAILED(hr))
                {
                    ILFree(target);
                    return hr;
                }

                parent = ILClone(target);
                ILRemoveLastID(parent);
                psfParent = psfTarget;

                apidl = &child;
            }

            if (cidl == 1 && IsFolder(apidl[0]))
            {
                res = RegOpenKey(HKEY_CLASSES_ROOT, L"Folder", keys + 0);
                if (!NT_SUCCESS(res))
                    return HRESULT_FROM_NT(res);
            }
            else
            {
                nkeys = 0;
            }

            HRESULT hr = CDefFolderMenu_Create2(parent, hwndOwner, cidl, apidl, psfParent, DefCtxMenuCallback, nkeys, keys, &pcm);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            return pcm->QueryInterface(riid, ppvOut);
        }

        if (IsEqualIID(riid, IID_IExtractIconW))
        {
            return ShellObjectCreatorInit<TExtractIcon>(m_NtPath, m_shellPidl, cidl, apidl, riid, ppvOut);
        }

        if (IsEqualIID(riid, IID_IDataObject))
        {
            return CIDLData_CreateFromIDArray(m_shellPidl, cidl, apidl, (IDataObject**)ppvOut);
        }

        if (IsEqualIID(riid, IID_IQueryAssociations))
        {
            if (cidl == 1 && IsFolder(apidl[0]))
            {
                CComPtr<IQueryAssociations> pqa;
                HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                hr = pqa->Init(ASSOCF_INIT_DEFAULTTOFOLDER, L"NTObjShEx.NTDirectory", NULL, hwndOwner);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                return pqa->QueryInterface(riid, ppvOut);
            }
        }

        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDisplayNameOf(
        LPCITEMIDLIST pidl,
        SHGDNF uFlags,
        STRRET *lpName)
    {
        const TItemId * info;

        TRACE("GetDisplayNameOf %p\n", pidl);

        HRESULT hr = GetInfoFromPidl(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (GET_SHGDN_FOR(uFlags) & SHGDN_FOREDITING)
        {
            hr = MakeStrRetFromString(info->entryName, info->entryNameLength, lpName);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }

        WCHAR path[MAX_PATH] = { 0 };

        if (GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING)
        {
            if (GET_SHGDN_RELATION(uFlags) != SHGDN_INFOLDER)
            {
                hr = GetFullName(m_shellPidl, uFlags, path, _countof(path));
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
            }
        }

        PathAppendW(path, info->entryName);

        LPCITEMIDLIST pidlNext = ILGetNext(pidl);
        if (pidlNext && pidlNext->mkid.cb > 0)
        {
            LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);

            CComPtr<IShellFolder> psfChild;
            hr = BindToObject(pidlFirst, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            WCHAR temp[MAX_PATH];
            STRRET childName;

            hr = psfChild->GetDisplayNameOf(pidlNext, uFlags | SHGDN_INFOLDER, &childName);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            hr = StrRetToBufW(&childName, pidlNext, temp, _countof(temp));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            PathAppendW(path, temp);

            ILFree(pidlFirst);
        }

        hr = MakeStrRetFromString(path, lpName);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE SetNameOf(
        HWND hwnd,
        LPCITEMIDLIST pidl,
        LPCOLESTR lpszName,
        SHGDNF uFlags,
        LPITEMIDLIST *ppidlOut)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    // IShellFolder2
    virtual HRESULT STDMETHODCALLTYPE GetDefaultSearchGUID(
        GUID *lpguid)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE EnumSearches(
        IEnumExtraSearch **ppenum)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDefaultColumn(
        DWORD dwReserved,
        ULONG *pSort,
        ULONG *pDisplay)
    {
        if (pSort)
            *pSort = 0;
        if (pDisplay)
            *pDisplay = 0;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDefaultColumnState(
        UINT iColumn,
        SHCOLSTATEF *pcsFlags) PURE;

    virtual HRESULT STDMETHODCALLTYPE GetDetailsEx(
        LPCITEMIDLIST pidl,
        const SHCOLUMNID *pscid,
        VARIANT *pv) PURE;

    virtual HRESULT STDMETHODCALLTYPE GetDetailsOf(
        LPCITEMIDLIST pidl,
        UINT iColumn,
        SHELLDETAILS *psd) PURE;

    virtual HRESULT STDMETHODCALLTYPE MapColumnToSCID(
        UINT iColumn,
        SHCOLUMNID *pscid) PURE;

    // IPersist
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *lpClassId)
    {
        if (!lpClassId)
            return E_POINTER;

        *lpClassId = CLSID_NtObjectFolder;
        return S_OK;
    }

    // IPersistFolder
    virtual HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidl)
    {
        m_shellPidl = ILClone(pidl);

        StringCbCopyW(m_NtPath, sizeof(m_NtPath), L"\\");

        return S_OK;
    }

    // IPersistFolder2
    virtual HRESULT STDMETHODCALLTYPE GetCurFolder(PIDLIST_ABSOLUTE * pidl)
    {
        if (pidl)
            *pidl = ILClone(m_shellPidl);
        if (!m_shellPidl)
            return S_FALSE;
        return S_OK;
    }

    // Internal
protected:
    virtual HRESULT STDMETHODCALLTYPE CompareIDs(
        LPARAM lParam,
        const TItemId * first,
        const TItemId * second) PURE;

    virtual ULONG STDMETHODCALLTYPE ConvertAttributes(
        const TItemId * entry,
        PULONG inMask) PURE;

    virtual BOOL STDMETHODCALLTYPE IsFolder(LPCITEMIDLIST pcidl)
    {
        const TItemId * info;

        HRESULT hr = GetInfoFromPidl(pcidl, &info);
        if (FAILED(hr))
            return hr;

        return IsFolder(info);
    }

    virtual BOOL STDMETHODCALLTYPE IsFolder(const TItemId * info) PURE;

    virtual BOOL STDMETHODCALLTYPE IsSymLink(LPCITEMIDLIST pcidl)
    {
        const TItemId * info;

        HRESULT hr = GetInfoFromPidl(pcidl, &info);
        if (FAILED(hr))
            return hr;

        return IsSymLink(info);
    }

    virtual BOOL STDMETHODCALLTYPE IsSymLink(const TItemId * info)
    {
        return FALSE;
    }

    virtual HRESULT GetInfoFromPidl(LPCITEMIDLIST pcidl, const TItemId ** pentry) PURE;

public:
    static HRESULT CALLBACK DefCtxMenuCallback(IShellFolder * /*psf*/, HWND /*hwnd*/, IDataObject * /*pdtobj*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
    {
        switch (uMsg)
        {
        case DFM_MERGECONTEXTMENU:
            return S_OK;
        case DFM_INVOKECOMMAND:
        case DFM_INVOKECOMMANDEX:
        case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
            return S_FALSE;
        }
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(TSelf)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(TSelf)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
    END_COM_MAP()

};