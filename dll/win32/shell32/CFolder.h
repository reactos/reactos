/*
 * Folder implementation
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

#ifndef _FOLDER_H_
#define _FOLDER_H_


class CFolder:
    public CComCoClass<CFolder>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<Folder2, &IID_Folder2>
{
private:
    HRESULT GetShellFolder(CComPtr<IShellFolder>& psfCurrent);

    CComHeapPtr<ITEMIDLIST> m_idlist;

public:
    CFolder();
    ~CFolder();

    void Init(LPITEMIDLIST idlist);

    // *** Folder methods ***
    virtual HRESULT STDMETHODCALLTYPE get_Title(BSTR *pbs);
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch **ppid);
    virtual HRESULT STDMETHODCALLTYPE get_ParentFolder(Folder **ppsf);
    virtual HRESULT STDMETHODCALLTYPE Items(FolderItems **ppid);
    virtual HRESULT STDMETHODCALLTYPE ParseName(BSTR bName, FolderItem **ppid);
    virtual HRESULT STDMETHODCALLTYPE NewFolder(BSTR bName, VARIANT vOptions);
    virtual HRESULT STDMETHODCALLTYPE MoveHere(VARIANT vItem, VARIANT vOptions);
    virtual HRESULT STDMETHODCALLTYPE CopyHere(VARIANT vItem, VARIANT vOptions);
    virtual HRESULT STDMETHODCALLTYPE GetDetailsOf(VARIANT vItem, int iColumn, BSTR *pbs);

    // *** Folder2 methods ***
    virtual HRESULT STDMETHODCALLTYPE get_Self(FolderItem **ppfi);
    virtual HRESULT STDMETHODCALLTYPE get_OfflineStatus(LONG *pul);
    virtual HRESULT STDMETHODCALLTYPE Synchronize();
    virtual HRESULT STDMETHODCALLTYPE get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade);
    virtual HRESULT STDMETHODCALLTYPE DismissedWebViewBarricade();

DECLARE_NOT_AGGREGATABLE(CFolder)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolder)
    COM_INTERFACE_ENTRY_IID(IID_Folder2, Folder2)
    COM_INTERFACE_ENTRY_IID(IID_Folder, Folder)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()

};


#endif
