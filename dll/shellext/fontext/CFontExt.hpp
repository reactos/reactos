/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontExt definition
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
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
    CStringW m_LastDetailsFontName;
    WIN32_FIND_DATAW m_LastDetailsFileData;

public:

    CFontExt();
    ~CFontExt();

    // *** IShellFolder2 methods ***
    virtual STDMETHODIMP GetDefaultSearchGUID(GUID *lpguid);
    virtual STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);
    virtual STDMETHODIMP GetDefaultColumn(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay);
    virtual STDMETHODIMP GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags);
    virtual STDMETHODIMP GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv);
    virtual STDMETHODIMP GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);
    virtual STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid);

    // *** IShellFolder methods ***
    virtual STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes);
    virtual STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList);
    virtual STDMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
    virtual STDMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
    virtual STDMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);
    virtual STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut);
    virtual STDMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut);
    virtual STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    virtual STDMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet);
    virtual STDMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut);


    // *** IPersistFolder2 methods ***
    virtual STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // *** IPersistFolder methods ***
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // *** IPersist methods ***
    virtual STDMETHODIMP GetClassID(CLSID *lpClassId);

    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    virtual STDMETHODIMP DragLeave();
    virtual STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

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

protected:

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

    HRESULT DoInstallFontFile(LPCWSTR pszFontPath, LPCWSTR pszFontsDir, HKEY hkeyFonts);
    HRESULT DoGetFontTitle(LPCWSTR pszFontPath, LPCWSTR pszFontName);
};
