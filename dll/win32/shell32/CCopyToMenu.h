#pragma once

class CCopyToMenu :
    public CComCoClass<CCopyToMenu, &CLSID_CopyToMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2,
    public IShellExtInit
{
protected:
    UINT m_idCmdFirst, m_idCmdLast, m_idCmdCopyTo;

    HRESULT DoCopyToFolder(LPCMINVOKECOMMANDINFO lpici);

public:
    CComHeapPtr<ITEMIDLIST> m_pidlFolder;
    CComPtr<IDataObject> m_pDataObject;

    CCopyToMenu();
    ~CCopyToMenu();

    // IContextMenu
    virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
    virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

    // IContextMenu2
    virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IShellExtInit
    virtual HRESULT STDMETHODCALLTYPE Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    DECLARE_REGISTRY_RESOURCEID(IDR_COPYTOMENU)
    DECLARE_NOT_AGGREGATABLE(CCopyToMenu)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCopyToMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
    END_COM_MAP()
};
