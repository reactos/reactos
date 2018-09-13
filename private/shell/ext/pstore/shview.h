#ifndef SHVIEW_H
#define SHVIEW_H

#define NS_CLASS_NAME   (TEXT("PStoreNSClass"))

//
// menu items
//

#define IDM_MESSAGE1    (FCIDM_SHVIEWFIRST + 0x500)
#define IDM_MESSAGE2    (FCIDM_SHVIEWFIRST + 0x501)
#define IDM_VIEW_ISTB   (FCIDM_SHVIEWFIRST + 0x502)
#define IDM_VIEW_IETB   (FCIDM_SHVIEWFIRST + 0x503)

//
// control IDs
//

#define ID_LISTVIEW     2000


class CShellView : public IShellView, public IOleCommandTarget
{
protected:
    LONG m_ObjRefCount;

public:
    CShellView(CShellFolder*, LPCITEMIDLIST);
    ~CShellView();

    //
    // IUnknown methods
    //

    STDMETHOD (QueryInterface)(REFIID, LPVOID FAR *);
    STDMETHOD_ (DWORD, AddRef)();
    STDMETHOD_ (DWORD, Release)();

    //
    // IOleWindow methods
    //

    STDMETHOD (GetWindow) (HWND*);
    STDMETHOD (ContextSensitiveHelp) (BOOL);

    //
    // IShellView methods
    //

    STDMETHOD (TranslateAccelerator) (LPMSG);
    STDMETHOD (EnableModeless) (BOOL);
    STDMETHOD (UIActivate) (UINT);
    STDMETHOD (Refresh) (void);
    STDMETHOD (CreateViewWindow) (LPSHELLVIEW, LPCFOLDERSETTINGS, LPSHELLBROWSER, LPRECT, HWND*);
    STDMETHOD (DestroyViewWindow) (void);
    STDMETHOD (GetCurrentInfo) (LPFOLDERSETTINGS);
    STDMETHOD (AddPropertySheetPages) (DWORD, LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHOD (SaveViewState) (void);
    STDMETHOD (SelectItem) (LPCITEMIDLIST, UINT);
    STDMETHOD (GetItemObject) (UINT, REFIID, LPVOID*);

    //
    // IOleCommandTarget methods
    //

    STDMETHOD (QueryStatus) (const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
    STDMETHOD (Exec) (const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

private:
    //
    // private member variables
    //

    UINT m_uState;
    BOOL m_bShowIETB;
    BOOL m_bShowISTB;
    LPITEMIDLIST m_pidl;
    OLEMENUGROUPWIDTHS m_MenuWidths;
    FOLDERSETTINGS m_FolderSettings;
    LPSHELLBROWSER m_pShellBrowser;
    HWND m_hwndParent;
    HWND m_hWnd;
    HWND m_hwndList;
    HMENU m_hMenu;
    int m_nColumn1;
    int m_nColumn2;
    CShellFolder *m_pSFParent;

    //
    // private member functions
    //

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    void UpdateToolbar();
    LRESULT UpdateMenu(HMENU hMenu);
    HRESULT GetSettings(void);
    HRESULT SaveSettings(void);
    HMENU BuildMenu(void);
    LRESULT OnCommand(DWORD, DWORD, HWND);
    LRESULT OnActivate(WPARAM wParam, LPARAM lParam);
    LRESULT OnSetFocus(void);
    LRESULT OnNotify(UINT, LPNMHDR);
    LRESULT OnSize(WORD, WORD);
    LRESULT OnCreate(void);
    BOOL CreateList(void);
    BOOL InitList(void);
    BOOL FillList(void);
};

#endif   // SHVIEW_H
