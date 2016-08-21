/*
 * FolderItem(s) implementation
 *
 * Copyright 2015 Mark Jansen
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

#ifndef _FOLDERITEM_H_
#define _FOLDERITEM_H_


class CFolderItem:
    public CComCoClass<CFolderItem>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<FolderItem, &IID_FolderItem>
{
private:
    CComHeapPtr<ITEMIDLIST> m_idlist;

public:
    CFolderItem();
    ~CFolderItem();

    // Please note: CFolderItem takes ownership of idlist.
    void Init(LPITEMIDLIST idlist);


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
    long m_Count;

public:
    CFolderItems();
    ~CFolderItems();

    // Please note: CFolderItems takes ownership of idlist.
    HRESULT Init(LPITEMIDLIST idlist);

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
