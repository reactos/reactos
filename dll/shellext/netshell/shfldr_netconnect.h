

typedef struct {
    int colnameid;
    int pcsFlags;
    int fmt;
    int cxChar;
} shvheader;

class CNetworkConnections:
    public CComCoClass<CNetworkConnections, &CLSID_ConnectionFolder>,
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
        virtual HRESULT WINAPI GetClassID(CLSID *lpClassId);
        virtual HRESULT WINAPI Initialize(PCIDLIST_ABSOLUTE pidl);
        virtual HRESULT WINAPI GetCurFolder(PIDLIST_ABSOLUTE *pidl);

        // IShellFolder
        virtual HRESULT WINAPI ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes);
        virtual HRESULT WINAPI EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList);
        virtual HRESULT WINAPI BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);
        virtual HRESULT WINAPI CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut);
        virtual HRESULT WINAPI GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
        virtual HRESULT WINAPI GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet);
        virtual HRESULT WINAPI SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut);

        // IShellFolder2
        virtual HRESULT WINAPI GetDefaultSearchGUID(GUID *pguid);
        virtual HRESULT WINAPI EnumSearches(IEnumExtraSearch **ppenum);
        virtual HRESULT WINAPI GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
        virtual HRESULT WINAPI GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags);
        virtual HRESULT WINAPI GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv);
        virtual HRESULT WINAPI GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);
        virtual HRESULT WINAPI MapColumnToSCID(UINT column, SHCOLUMNID *pscid);

        // IShellExtInit
        virtual HRESULT WINAPI Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

        // IOleCommandTarget
        virtual HRESULT WINAPI Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);
        virtual HRESULT WINAPI QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);

        // IShellFolderViewCB
        virtual HRESULT WINAPI MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

        // IShellExecuteHookW
        virtual HRESULT WINAPI Execute(LPSHELLEXECUTEINFOW pei);

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
        virtual HRESULT WINAPI QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
        virtual HRESULT WINAPI GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);
        virtual HRESULT WINAPI HandleMenuMsg( UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual HRESULT WINAPI HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

        // IObjectWithSite
        virtual HRESULT WINAPI SetSite(IUnknown *punk);
        virtual HRESULT WINAPI GetSite(REFIID iid, void **ppvSite);

        // IQueryInfo
        virtual HRESULT WINAPI GetInfoFlags(DWORD *pdwFlags);
        virtual HRESULT WINAPI GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);

        // IExtractIconW
        virtual HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
        virtual HRESULT STDMETHODCALLTYPE Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

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
