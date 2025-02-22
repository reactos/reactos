/*
 * PROJECT:     ReactOS CabView Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main header file
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once
#include "precomp.h"
#include "resource.h"

#define FLATFOLDER TRUE

EXTERN_C const GUID CLSID_CabFolder;

enum EXTRACTCALLBACKMSG { ECM_BEGIN, ECM_FILE, ECM_PREPAREPATH, ECM_ERROR };
struct EXTRACTCALLBACKDATA
{
    LPCWSTR Path;
    const FDINOTIFICATION *pfdin;
    HRESULT hr;
};
typedef HRESULT (CALLBACK*EXTRACTCALLBACK)(EXTRACTCALLBACKMSG msg, const EXTRACTCALLBACKDATA &data, LPVOID cookie);
HRESULT ExtractCabinet(LPCWSTR cab, LPCWSTR destination, EXTRACTCALLBACK callback, LPVOID cookie);

class CEnumIDList :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
protected:
    HDPA m_Items;
    ULONG m_Pos;

public:
    static int CALLBACK DPADestroyCallback(void *pidl, void *pData)
    {
        SHFree(pidl);
        return TRUE;
    }

    CEnumIDList() : m_Pos(0)
    {
        m_Items = DPA_Create(0);
    }

    virtual ~CEnumIDList()
    {
        DPA_DestroyCallback(m_Items, DPADestroyCallback, NULL);
    }

    int FindNamedItem(PCUITEMID_CHILD pidl) const;
    HRESULT Fill(LPCWSTR path, HWND hwnd = NULL, SHCONTF contf = 0);
    HRESULT Fill(PCIDLIST_ABSOLUTE pidl, HWND hwnd = NULL, SHCONTF contf = 0);

    HRESULT Append(LPCITEMIDLIST pidl)
    {
        return DPA_AppendPtr(m_Items, (void*)pidl) != DPA_ERR ? S_OK : E_OUTOFMEMORY;
    }

    UINT GetCount() const { return m_Items ? DPA_GetPtrCount(m_Items) : 0; }

    // IEnumIDList
    IFACEMETHODIMP Next(ULONG celt, PITEMID_CHILD *rgelt, ULONG *pceltFetched)
    {
        if (!rgelt)
            return E_INVALIDARG;
        HRESULT hr = S_FALSE;
        UINT count = GetCount(), fetched = 0;
        if (m_Pos < count && fetched < celt)
        {
            if (SUCCEEDED(hr = SHILClone(DPA_FastGetPtr(m_Items, m_Pos), &rgelt[fetched])))
                fetched++;
        }
        if (pceltFetched)
            *pceltFetched = fetched;
        m_Pos += fetched;
        return FAILED(hr) ? hr : (celt == fetched && fetched) ? S_OK : S_FALSE;
    }

    IFACEMETHODIMP Reset()
    {
        m_Pos = 0;
        return S_OK;
    }

    IFACEMETHODIMP Skip(ULONG celt)
    {
        UINT count = GetCount(), newpos = m_Pos + celt;
        if (celt > count || newpos >= count)
            return E_INVALIDARG;
        m_Pos = newpos;
        return S_OK;
    }

    IFACEMETHODIMP Clone(IEnumIDList **ppenum)
    {
        UNIMPLEMENTED;
        *ppenum = NULL;
        return E_NOTIMPL;
    }

    static CEnumIDList* CreateInstance()
    {
        CComPtr<CEnumIDList> obj;
        return SUCCEEDED(ShellObjectCreator(obj)) ? obj.Detach() : NULL;
    }

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CEnumIDList)

    BEGIN_COM_MAP(CEnumIDList)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()
};

class CCabFolder :
    public CComCoClass<CCabFolder, &CLSID_CabFolder>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2,
    public IShellFolderViewCB,
    public IShellIcon
{
protected:
    CComHeapPtr<ITEMIDLIST> m_CurDir;
    HWND m_ShellViewWindow = NULL;

public:
    HRESULT ExtractFilesUI(HWND hWnd, IDataObject *pDO);
    HRESULT GetItemDetails(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd, VARIANT *pv);
    int MapSCIDToColumn(const SHCOLUMNID &scid);
    HRESULT CompareID(LPARAM lParam, PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2);

    HRESULT CreateEnum(CEnumIDList **List)
    {
        CEnumIDList *pEIDL = CEnumIDList::CreateInstance();
        *List = pEIDL;
        return pEIDL ? pEIDL->Fill(m_CurDir) : E_OUTOFMEMORY;
    }

    // IShellFolder2
    IFACEMETHODIMP GetDefaultSearchGUID(GUID *pguid) override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP EnumSearches(IEnumExtraSearch **ppenum) override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) override;

    IFACEMETHODIMP GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags) override;

    IFACEMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override;

    IFACEMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;

    IFACEMETHODIMP MapColumnToSCID(UINT column, SHCOLUMNID *pscid) override;

    IFACEMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override;

    IFACEMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
    
    IFACEMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override;
    
    IFACEMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override;
    
    IFACEMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut) override;
    
    IFACEMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT *prgfInOut, LPVOID *ppvOut) override;
    
    IFACEMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET pName) override;
    
    IFACEMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut) override
    {
        return E_NOTIMPL;
    }

    // IPersistFolder2
    IFACEMETHODIMP GetCurFolder(PIDLIST_ABSOLUTE *pidl) override
    {
        LPITEMIDLIST curdir = (LPITEMIDLIST)m_CurDir;
        return curdir ? SHILClone(curdir, pidl) : E_UNEXPECTED;
    }

    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl) override
    {
        WCHAR path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path))
        {
            PIDLIST_ABSOLUTE curdir = ILClone(pidl);
            if (curdir)
            {
                m_CurDir.Attach(curdir);
                return S_OK;
            }
            return E_OUTOFMEMORY;
        }
        return E_INVALIDARG;
    }

    IFACEMETHODIMP GetClassID(CLSID *lpClassId) override
    {
        *lpClassId = CLSID_CabFolder;
        return S_OK;
    }

    // IShellFolderViewCB
    IFACEMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // IShellIcon
    IFACEMETHODIMP GetIconOf(PCUITEMID_CHILD pidl, UINT flags, int *pIconIndex) override;

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CCabFolder)

    BEGIN_COM_MAP(CCabFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IShellIcon, IShellIcon)
    END_COM_MAP()
};
