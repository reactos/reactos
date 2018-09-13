#ifndef _STARTMNU_H
#define _STARTMNU_H
//--------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------------
BOOL StartButton_Create(HINSTANCE hinst, HWND hwndTray, UINT id, HWND *phwnd);
BOOL ExecItemByPidls(HWND hwnd, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem);
BOOL IsSubfolder(IShellFolder *psf, LPITEMIDLIST pidl);
void DoMenuOnItemByPidl(HWND hwnd, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem, IContextMenu **ppcm);

void Tray_RunDlg();
void ShowFolder(HWND hwnd, UINT wparam, UINT uFlags);
void Tray_Suspend();
void DoEjectPC();
void DoExitWindows(HWND);
BOOL ExecItemByPidls(HWND hwnd, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem);
void DoTrayProperties(INT nStartPage);
void _ForceStartButtonUp();

WORD    _GetHotkeyFromFolderItem(LPSHELLFOLDER psf, LPCITEMIDLIST pidl);
int     HotkeyList_Add(WORD wHotkey, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, BOOL fClone);






//---------------------------------------------------------------------------
#define SBM_RESET_AND_RESIZE            (WM_USER + 1)
#define StartButton_ResetAndResize(hwnd) \
    SendMessage(hwnd, SBM_RESET_AND_RESIZE, 0, 0)

#define SBM_FORCE_STATE                 (WM_USER + 2)
#define StartButton_ForceState(hwnd, fDown) \
    SendMessage(hwnd, SBM_FORCE_STATE, (WPARAM)fDown, 0)

#define SBM_PRESS         	            (WM_USER + 4)
#define StartButton_Press(hwnd) \
    SendMessage(hwnd, SBM_PRESS, 0, 0)

#define SBM_SET_MENU_IMAGE_SIZE	        (WM_USER + 5)
#define StartButton_SetMenuImageSize(hwnd, fLarge) \
    SendMessage(hwnd, SBM_SET_MENU_IMAGE_SIZE, (WPARAM)fLarge, 0)

#define SBM_DESTROY_MENU                (WM_USER + 7)
#define StartButton_DestroyMenu(hwnd) \
    SendMessage(hwnd, SBM_DESTROY_MENU, 0, 0)

#define SBM_CREATE_MENU                 (WM_USER + 8)
#define StartButton_CreateMenu(hwnd) \
    SendMessage(hwnd, SBM_CREATE_MENU, 0, 0)

#define SBM_START_MENU_CHANGE 	        (WM_USER + 9)

#define SBM_REFRESH                     (WM_USER + 10)
#define StartButton_Refresh(hwnd) \
    SendMessage(hwnd, SBM_REFRESH, 0, 0)

#define SBM_RESET_MENU_HEIGHT           (WM_USER + 11)
#define StartButton_ResetMenuHeight(hwnd) \
    SendMessage(hwnd, SBM_RESET_MENU_HEIGHT, 0, 0)

#define SBM_CANCELMENU                (WM_USER + 12)
#define SBM_SETFOREGROUNDPREV         (WM_USER + 13)


HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb);
HRESULT IMenuPopup_SetIconSize(IMenuPopup* punk,DWORD iIcon);

STDAPI  CHotKey_Create(IShellHotKey ** ppshk);


#ifdef __cplusplus
}


class CStartMenuHost : public ITrayPriv,
                        public IServiceProvider,
                        public IShellService,
                        public IMenuPopup,
                        public IOleCommandTarget,
                        public IWinEventHandler

{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // *** ITrayPriv methods ***
    STDMETHODIMP ExecItem (IShellFolder* psf, LPCITEMIDLIST pidl);
    STDMETHODIMP GetFindCM(HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu** ppcmFind);
    STDMETHODIMP GetStaticStartMenu(HMENU* phmenu);


    // *** IServiceProvider ***
    STDMETHODIMP QueryService (REFGUID guidService, REFIID riid, void ** ppvObject);

    // *** IShellService ***
    STDMETHODIMP SetOwner (struct IUnknown* punkOwner);

    // *** IOleWindow methods ***
    STDMETHODIMP GetWindow         (HWND * lphwnd);
    STDMETHODIMP ContextSensitiveHelp  (THIS_ BOOL fEnterMode) { return E_NOTIMPL; }

    // *** IDeskBarClient methods ***
    STDMETHODIMP SetClient         (IUnknown* punkClient) { return E_NOTIMPL; }
    STDMETHODIMP GetClient         (IUnknown** ppunkClient) { return E_NOTIMPL; }
    STDMETHODIMP OnPosRectChangeDB (LPRECT prc) { return E_NOTIMPL; }

    // *** IMenuPopup methods ***
    STDMETHODIMP Popup             (POINTL *ppt, RECTL *prcExclude, DWORD dwFlags);
    STDMETHODIMP OnSelect          (DWORD dwSelectType);
    STDMETHODIMP SetSubMenu        (IMenuPopup* pmp, BOOL fSet);

    // *** IOleCommandTarget ***
    STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup,
                             ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID * pguidCmdGroup,
                             DWORD nCmdID, DWORD nCmdexecopt, 
                             VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IWinEventHandler ***
    STDMETHODIMP OnWinEvent(HWND h, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    STDMETHODIMP IsWindowOwner (HWND hwnd);

    // *** IBanneredBar ***

protected:
    CStartMenuHost();

    friend HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb);

    int    _cRef;
};


class CHotKey : public IShellHotKey
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // *** IShellHotKey methods ***
    STDMETHODIMP RegisterHotKey(IShellFolder * psf, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl);

protected:
    CHotKey();
    
    friend HRESULT CHotKey_Create(IShellHotKey ** ppshk);

    int    _cRef;
};


#endif //C++

#ifdef WINNT // hydra specific functions
void MuSecurity(void);
#endif

#endif //_START_H
