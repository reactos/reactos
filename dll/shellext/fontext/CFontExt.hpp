/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontExt definition
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2019-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

class CFontExt :
    public CComCoClass<CFontExt, &CLSID_CFontExt>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2,
    public IDropTarget
{
    CComHeapPtr<ITEMIDLIST> m_Folder;

public:
    CFontExt();
    ~CFontExt();

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(GUID *lpguid) override;
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum) override;
    STDMETHODIMP GetDefaultColumn(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay) override;
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags) override;
    STDMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override;
    STDMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid) override;

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten,
                                  PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override;
    STDMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid,
                              LPVOID *ppvOut) override;
    STDMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid,
                               LPVOID *ppvOut) override;
    STDMETHODIMP CompareIDs(LPARAM lParam,
                            PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override;
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override;
    STDMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override;
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
                               UINT * prgfInOut, LPVOID * ppvOut) override;
    STDMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override;
    STDMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags,
                           PITEMID_CHILD *pPidlOut) override;

    // *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl) override;

    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl) override;

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID *lpClassId) override;

    // *** IDropTarget methods ***
    STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHODIMP DragLeave() override;
    STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

#if 0
    static HRESULT WINAPI log_stuff(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
    {
        WCHAR* g2s(REFCLSID iid);

        WCHAR buffer[MAX_PATH];
        StringCchPrintfW(buffer, _countof(buffer), L"CFontExt::QueryInterface(%s)\n", g2s(riid));
        OutputDebugStringW(buffer);

        return E_NOINTERFACE;
    }
#endif

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_FONTEXT)
    DECLARE_NOT_AGGREGATABLE(CFontExt)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFontExt)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        //COM_INTERFACE_ENTRY_FUNC_BLIND(0, log_stuff)
    END_COM_MAP()
};
