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
    public IDispatchImpl<FolderItem, &IID_FolderItem>
{
private:
    CComHeapPtr<ITEMIDLIST> m_idlist;
    CComPtr<Folder> m_Folder;

public:
    CFolderItem();
    ~CFolderItem();

    HRESULT Initialize(Folder* folder, LPITEMIDLIST idlist);


    // *** FolderItem methods ***
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR *pbs);
    virtual HRESULT STDMETHODCALLTYPE put_Name(BSTR bs);
    virtual HRESULT STDMETHODCALLTYPE get_Path(BSTR *pbs);
    virtual HRESULT STDMETHODCALLTYPE get_GetLink(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_GetFolder(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_IsLink(VARIANT_BOOL *pb);
    virtual HRESULT STDMETHODCALLTYPE get_IsFolder(VARIANT_BOOL *pb);
    virtual HRESULT STDMETHODCALLTYPE get_IsFileSystem(VARIANT_BOOL *pb);
    virtual HRESULT STDMETHODCALLTYPE get_IsBrowsable(VARIANT_BOOL *pb);
    virtual HRESULT STDMETHODCALLTYPE get_ModifyDate(DATE *pdt);
    virtual HRESULT STDMETHODCALLTYPE put_ModifyDate(DATE dt);
    virtual HRESULT STDMETHODCALLTYPE get_Size(LONG *pul);
    virtual HRESULT STDMETHODCALLTYPE get_Type(BSTR *pbs);
    virtual HRESULT STDMETHODCALLTYPE Verbs(FolderItemVerbs **ppfic);
    virtual HRESULT STDMETHODCALLTYPE InvokeVerb(VARIANT vVerb);


DECLARE_NOT_AGGREGATABLE(CFolderItem)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolderItem)
    COM_INTERFACE_ENTRY_IID(IID_FolderItem, FolderItem)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()
};

class CFolderItems:
    public CComCoClass<CFolderItems>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<FolderItems, &IID_FolderItems>
{
private:
    CComHeapPtr<ITEMIDLIST> m_idlist;
    CComPtr<IEnumIDList> m_EnumIDList;
    CComPtr<Folder> m_Folder;
    long m_Count;

public:
    CFolderItems();
    ~CFolderItems();

    // Please note: CFolderItems takes ownership of idlist.
    HRESULT Initialize(LPITEMIDLIST idlist, Folder* parent);

    // *** FolderItems methods ***
    virtual HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE Item(VARIANT index, FolderItem **ppid);
    virtual HRESULT STDMETHODCALLTYPE _NewEnum(IUnknown **ppunk);

DECLARE_NOT_AGGREGATABLE(CFolderItems)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolderItems)
    COM_INTERFACE_ENTRY_IID(IID_FolderItems, FolderItems)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()
};

#endif
