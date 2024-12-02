

typedef struct {
    int colnameid;
    int pcsFlags;
    int fmt;
    int cxChar;
} shvheader;

class CNetworkConnections:
    public CComCoClass<CNetworkConnections, &CLSID_NetworkConnections>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IPersistFolder2,
    public IShellExtInit,
    public IShellFolder2,
    public IOleCommandTarget,
    public IShellFolderViewCB,
    public IShellExecuteHookW
{
    public:
        CNetworkConnections();
        ~CNetworkConnections();

        // IPersistFolder2
        STDMETHOD(GetClassID)(CLSID *lpClassId) override;
        STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl) override;
        STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE *pidl) override;

        // IShellFolder
        STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
        STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override;
        STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(CompareIDs)(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override;
        STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(GetAttributesOf)(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override;
        STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) override;
        STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override;
        STDMETHOD(SetNameOf)(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut) override;

        // IShellFolder2
        STDMETHOD(GetDefaultSearchGUID)(GUID *pguid) override;
        STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum) override;
        STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) override;
        STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pcsFlags) override;
        STDMETHOD(GetDetailsEx)(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override;
        STDMETHOD(GetDetailsOf)(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;
        STDMETHOD(MapColumnToSCID)(UINT column, SHCOLUMNID *pscid) override;

        // IShellExtInit
        STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) override;

        // IOleCommandTarget
        STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;
        STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText) override;

        // IShellFolderViewCB
        STDMETHOD(MessageSFVCB)(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

        // IShellExecuteHookW
        STDMETHOD(Execute)(LPSHELLEXECUTEINFOW pei) override;

    private:

        /* both paths are parsible from the desktop */
        PIDLIST_ABSOLUTE m_pidlRoot;
        CComPtr<IOleCommandTarget> m_lpOleCmd;

    public:

        BEGIN_COM_MAP(CNetworkConnections)
            COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersistFolder2)
            COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder2)
            COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
            COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder2)
            COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
            COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
            COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
            COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
            COM_INTERFACE_ENTRY_IID(IID_IShellExecuteHookW, IShellExecuteHookW)
        END_COM_MAP()

        DECLARE_NO_REGISTRY()
        DECLARE_NOT_AGGREGATABLE(CNetworkConnections)
        DECLARE_PROTECT_FINAL_CONSTRUCT()
};

class CNetConUiObject:
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3,
    public IObjectWithSite,
    public IQueryInfo,
    public IExtractIconW
{
    private:
        PCUITEMID_CHILD m_pidl;
        CComPtr<IUnknown> m_pUnknown;
        CComPtr<IOleCommandTarget> m_lpOleCmd;

    public:
        CNetConUiObject();
        ~CNetConUiObject();
        HRESULT WINAPI Initialize(PCUITEMID_CHILD pidl, IOleCommandTarget *lpOleCmd);

        // IContextMenu3
        STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
        STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici) override;
        STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax) override;
        STDMETHOD(HandleMenuMsg)( UINT uMsg, WPARAM wParam, LPARAM lParam) override;
        STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) override;

        // IObjectWithSite
        STDMETHOD(SetSite)(IUnknown *punk) override;
        STDMETHOD(GetSite)(REFIID iid, void **ppvSite) override;

        // IQueryInfo
        STDMETHOD(GetInfoFlags)(DWORD *pdwFlags) override;
        STDMETHOD(GetInfoTip)(DWORD dwFlags, WCHAR **ppwszTip) override;

        // IExtractIconW
        STDMETHOD(GetIconLocation)(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags) override;
        STDMETHOD(Extract)(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) override;

        BEGIN_COM_MAP(CNetConUiObject)
            COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu3)
            COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu3)
            COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
            COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
            COM_INTERFACE_ENTRY_IID(IID_IQueryInfo, IQueryInfo)
            COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
        END_COM_MAP()

        DECLARE_NOT_AGGREGATABLE(CNetConUiObject)
        DECLARE_PROTECT_FINAL_CONSTRUCT()
};

HRESULT ShowNetConnectionProperties(INetConnection * pNetConnect, HWND hwnd);
