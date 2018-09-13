#ifndef __folder_h
#define __folder_h


/*-----------------------------------------------------------------------------
/ Resource ID's for our menu items
/----------------------------------------------------------------------------*/

#define MDVMID_ARRANGEFIRST     (FSIDM_SORT_FIRST)
#define MDVMID_ARRANGEBYNAME    (MDVMID_ARRANGEFIRST + 0)  // Arrange->by Name
#define MDVMID_ARRANGEBYSIZE    (MDVMID_ARRANGEFIRST + 1)  // Arrange->by Size
#define MDVMID_ARRANGEBYTYPE    (MDVMID_ARRANGEFIRST + 2)  // Arrange->by type
#define MDVMID_ARRANGEBYDATE    (MDVMID_ARRANGEFIRST + 3)  // Arrange->by date modified
#define MDVMID_ARRANGEBYATTRIB  (MDVMID_ARRANGEFIRST + 4)

#define FSSortIDToICol(x) ((x) - FSIDM_SORT_FIRST + FS_ICOL_NAME)

#define UIKEY_ALL       0
#define UIKEY_SPECIFIC  1
#define UIKEY_MAX       2


#define ITEM_OFFSET_SEPARATOR 0
#define ITEM_OFFSET_REMOVE    1
#define ITEM_OFFSET_RESTORE   1

enum folder_type {
    FOLDER_IS_UNKNOWN = 0,
    FOLDER_IS_ROOT,
    FOLDER_IS_ROOT_PATH,
    FOLDER_IS_SPECIAL_ITEM,
    FOLDER_IS_JUNCTION,
    FOLDER_IS_SENDTO,
    FOLDER_IS_UNBLESSED_ROOT_PATH,
    };

enum calling_app_type {
    APP_IS_UNKNOWN = 0,
    APP_IS_NORMAL,
    APP_IS_OFFICE95,
    APP_IS_OFFICE97,
    APP_IS_COREL7,
    APP_IS_SHELL
    };

enum extension_type {
    EXT_IS_UNKNOWN = 0,
    EXT_IS_ROOT,
    EXT_IS_ROOT_PATH
    };


HRESULT _MergeArrangeMenu( LPQCMINFO pInfo );


/*-----------------------------------------------------------------------------
/ CMyDocsFolder - our IShell folder implementation
/----------------------------------------------------------------------------*/

class CMyDocsFolder : public IPersistFolder, IPersistFile, IShellFolder,
                             IShellPropSheetExt,
                             IShellExtInit, IContextMenu, CUnknown
{
    private:
        LPITEMIDLIST         m_pidl;        // absolute IDLIST to our object
        LPITEMIDLIST         m_pidlReal;    // pidl that points to actual directory...
        LPTSTR               m_path;        // path to what this folder really points to
        IShellFolder *       m_psf;         // points to shell folder in use...
        IUnknown *           m_punk;        // points to IUnknown for shell folder in use...
        IShellFolderView *   m_psfv;        // points to shell folder view in use...
        IShellFolderViewCB * m_psfvcb;
        //IShellDetails *      m_psd;         // points to shell details to use
        //IShellDetails3 *     m_psd3;         // points to shell details 3, if its there
        folder_type          m_type;
        calling_app_type     m_host;
        UINT                 m_HideCmd;     // menu item id of hide cmd for context menu
        UINT                 m_RestoreCmd;  // menu item id of restore cmd for context menu
        extension_type       m_ext;
        IShellPropSheetExt * m_pseGeneral;  // General page prop sheet ext
        IShellPropSheetExt * m_pseSharing;  // Sharing page prop sheet ext

    private:
        STDMETHOD(RealInitialize)(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlBindTo, LPTSTR pRootPath );
        void WhoIsCalling();
#ifdef DO_OTHER_PAGES
        STDMETHOD(_AddGeneralAndSharingPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
#endif

    public:
        CMyDocsFolder();
        ~CMyDocsFolder();

        HRESULT _GetDetailsOf(PDETAILSINFO pDetails, UINT iColumn);

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IShellFolder
        STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbcReserved, LPOLESTR pDisplayName,
                                      ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

        STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppEnumIDList);
        STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut);
        STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvObj);
        STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
        STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
        STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
        STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
        STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET pName);
        STDMETHOD(SetNameOf)(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST* ppidlOut);

        // IPersist
        STDMETHOD(GetClassID)(LPCLSID pClassID);

        // IPersistFolder
        STDMETHOD(Initialize)(LPCITEMIDLIST pidlStart);

        // IPersistFile
        STDMETHOD(IsDirty)(void);
        STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
        STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
        STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
        STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName);

        // IShellExtInit
        STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

        // IShellPropSheetExt
        STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
        STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

        // IContextMenu
        STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
        STDMETHOD(GetCommandString)(UINT idCmd, UINT uType, UINT * pwReserved, LPSTR pszName, UINT cchMax);

};
#endif
