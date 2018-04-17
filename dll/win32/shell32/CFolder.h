/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Folder implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
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
    CComPtr<IShellDispatch> m_Application;

public:
    CFolder();
    ~CFolder();

    HRESULT Initialize(LPITEMIDLIST idlist);

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
