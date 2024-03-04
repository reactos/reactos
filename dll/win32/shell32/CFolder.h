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
    STDMETHOD(get_Title)(BSTR *pbs) override;
    STDMETHOD(get_Application)(IDispatch **ppid) override;
    STDMETHOD(get_Parent)(IDispatch **ppid) override;
    STDMETHOD(get_ParentFolder)(Folder **ppsf) override;
    STDMETHOD(Items)(FolderItems **ppid) override;
    STDMETHOD(ParseName)(BSTR bName, FolderItem **ppid) override;
    STDMETHOD(NewFolder)(BSTR bName, VARIANT vOptions) override;
    STDMETHOD(MoveHere)(VARIANT vItem, VARIANT vOptions) override;
    STDMETHOD(CopyHere)(VARIANT vItem, VARIANT vOptions) override;
    STDMETHOD(GetDetailsOf)(VARIANT vItem, int iColumn, BSTR *pbs) override;

    // *** Folder2 methods ***
    STDMETHOD(get_Self)(FolderItem **ppfi) override;
    STDMETHOD(get_OfflineStatus)(LONG *pul) override;
    STDMETHOD(Synchronize)() override;
    STDMETHOD(get_HaveToShowWebViewBarricade)(VARIANT_BOOL *pbHaveToShowWebViewBarricade) override;
    STDMETHOD(DismissedWebViewBarricade)() override;

DECLARE_NOT_AGGREGATABLE(CFolder)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFolder)
    COM_INTERFACE_ENTRY_IID(IID_Folder2, Folder2)
    COM_INTERFACE_ENTRY_IID(IID_Folder, Folder)
    COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
END_COM_MAP()

};


#endif
