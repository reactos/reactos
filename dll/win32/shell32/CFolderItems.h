/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     FolderItem(s) implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _FOLDERITEM_H_
#define _FOLDERITEM_H_


class CFolderItem:
    public CComCoClass<CFolderItem>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<FolderItem2, &IID_FolderItem2>,
    public IParentAndItem
{
private:
    CComHeapPtr<ITEMIDLIST> m_idlist;
    CComPtr<Folder> m_Folder;

    inline HRESULT GetParentShellFolderAndItem(REFIID riid, void**ppv, PCUITEMID_CHILD &pidlLast);

public:
    CFolderItem();
    ~CFolderItem();

    HRESULT Initialize(Folder* folder, LPCITEMIDLIST idlist);
    LPCITEMIDLIST GetAbsoluteIDList() { return m_idlist; }
    HWND GetHwnd() { return NULL; }
    IUnknown* GetSite() { return NULL; }
    HRESULT GetFindDataFromIDList(WIN32_FIND_DATA &wfd);
    HRESULT HasAttribute(DWORD sfgaof, VARIANT_BOOL *pB);
    HRESULT GetExtendedProperty(REFPROPERTYKEY pkey, VARIANT *pv);

    static HRESULT GetParentAndItem(const VARIANT *pV, IParentAndItem **ppPAI);
    static PITEMID_CHILD CloneLeafPidl(const VARIANT *pV);

    // *** FolderItem methods ***
    STDMETHOD(get_Application)(IDispatch **ppid) override;
    STDMETHOD(get_Parent)(IDispatch **ppid) override;
    STDMETHOD(get_Name)(BSTR *pbs) override;
    STDMETHOD(put_Name)(BSTR bs) override;
    STDMETHOD(get_Path)(BSTR *pbs) override;
    STDMETHOD(get_GetLink)(IDispatch **ppid) override;
    STDMETHOD(get_GetFolder)(IDispatch **ppid) override;
    STDMETHOD(get_IsLink)(VARIANT_BOOL *pb) override;
    STDMETHOD(get_IsFolder)(VARIANT_BOOL *pb) override;
    STDMETHOD(get_IsFileSystem)(VARIANT_BOOL *pb) override;
    STDMETHOD(get_IsBrowsable)(VARIANT_BOOL *pb) override;
    STDMETHOD(get_ModifyDate)(DATE *pdt) override;
    STDMETHOD(put_ModifyDate)(DATE dt) override;
    STDMETHOD(get_Size)(LONG *pul) override;
    STDMETHOD(get_Type)(BSTR *pbs) override;
    STDMETHOD(Verbs)(FolderItemVerbs **ppfic) override;
    STDMETHOD(InvokeVerb)(VARIANT vVerb) override;

    // *** FolderItem2 methods ***
    STDMETHOD(InvokeVerbEx)(VARIANT vVerb, VARIANT vArgs) override;
    STDMETHOD(ExtendedProperty)(BSTR bsPropName, VARIANT *pv) override;

    // *** IParentAndItem ***
    STDMETHOD(SetParentAndItem)(PCIDLIST_ABSOLUTE, IShellFolder*, PCUITEMID_CHILD) override { return E_NOTIMPL; }
    STDMETHOD(GetParentAndItem)(PIDLIST_ABSOLUTE *ppidlParent, IShellFolder **ppsf, PITEMID_CHILD *ppidlChild) override;

DECLARE_NOT_AGGREGATABLE(CFolderItem)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolderItem)
    COM_INTERFACE_ENTRY_IID(IID_FolderItem, FolderItem)
    COM_INTERFACE_ENTRY_IID(IID_FolderItem2, FolderItem2)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
    COM_INTERFACE_ENTRY_IID(IID_IParentAndItem, IParentAndItem)
END_COM_MAP()
};

class CFolderItems:
    public CComCoClass<CFolderItems>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<FolderItems3, &IID_FolderItems3>
{
private:
    CComHeapPtr<ITEMIDLIST> m_idlist;
    CComPtr<IEnumIDList> m_EnumIDList;
    CComPtr<Folder> m_Folder;
    long m_Count;
    LONG m_Contf = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    BSTR m_bstrFilter = NULL;

    void ResetEnum();
    BOOL IncludeItem(LPCITEMIDLIST pidl);
    static HRESULT CALLBACK ItemsEnumFilter(void *Cookie, LPCITEMIDLIST pidl);
    HRESULT EnumObjects();
    HRESULT GetNext(LPITEMIDLIST *ppidl) { return m_EnumIDList->Next(1, ppidl, NULL); }

public:
    CFolderItems();
    ~CFolderItems();

    HRESULT Initialize(LPCITEMIDLIST idlist, Folder* parent);
    HWND GetHwnd() { return NULL; }
    IUnknown* GetSite() { return NULL; }
    static HRESULT InvokeVerbHelper(HWND hWnd, IContextMenu &cm, VARIANT &vVerb, VARIANT &vArgs, IUnknown *pSite);

    // *** FolderItems methods ***
    STDMETHOD(get_Count)(long *plCount) override;
    STDMETHOD(get_Application)(IDispatch **ppid) override;
    STDMETHOD(get_Parent)(IDispatch **ppid) override;
    STDMETHOD(Item)(VARIANT index, FolderItem **ppid) override;
    STDMETHOD(_NewEnum)(IUnknown **ppunk) override;
    // *** FolderItems2 methods ***
    STDMETHOD(InvokeVerbEx)(VARIANT vVerb, VARIANT vArgs) override;
    // *** FolderItems3 methods ***
    STDMETHOD(Filter)(LONG grfFlags, BSTR bstrFilter) override;
    STDMETHOD(get_Verbs)(FolderItemVerbs **ppfic) override;

DECLARE_NOT_AGGREGATABLE(CFolderItems)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolderItems)
    COM_INTERFACE_ENTRY_IID(IID_FolderItems, FolderItems)
    COM_INTERFACE_ENTRY_IID(IID_FolderItems, FolderItems2)
    COM_INTERFACE_ENTRY_IID(IID_FolderItems, FolderItems3)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()
};

#endif
