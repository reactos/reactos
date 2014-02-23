/*
* Shell Menu Band
*
* Copyright 2014 David Quintana
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
#include "precomp.h"
#include <windowsx.h>
#include <CommonControls.h>
#include <shlwapi_undoc.h>

extern "C"
HRESULT WINAPI SHGetImageList(
    _In_   int iImageList,
    _In_   REFIID riid,
    _Out_  void **ppv
    );


#define TBSTYLE_EX_VERTICAL 4

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBand);

#define TIMERID_HOTTRACK 1
#define SUBCLASS_ID_MENUBAND 1

extern "C" BOOL WINAPI Shell_GetImageLists(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

class CMenuBand;
class CMenuFocusManager;

class CMenuToolbarBase
{
protected:
    CMenuBand * m_menuBand;
    HWND        m_hwnd;
    DWORD       m_dwMenuFlags;
    INT         m_hotItem;
    WNDPROC     m_SubclassOld;

private:
    static LRESULT CALLBACK s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CMenuToolbarBase(CMenuBand *menuBand);
    virtual ~CMenuToolbarBase() {}

    HRESULT IsWindowOwner(HWND hwnd);
    HRESULT CreateToolbar(HWND hwndParent, DWORD dwFlags);
    HRESULT GetWindow(HWND *phwnd);
    HRESULT ShowWindow(BOOL fShow);
    HRESULT Close();

    virtual HRESULT FillToolbar() = 0;
    virtual HRESULT PopupItem(UINT uItem) = 0;
    virtual HRESULT HasSubMenu(UINT uItem) = 0;
    virtual HRESULT OnContextMenu(NMMOUSE * rclick) = 0;
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    HRESULT PopupSubMenu(UINT index, IShellMenu* childShellMenu);
    HRESULT PopupSubMenu(UINT index, HMENU menu);
    HRESULT DoContextMenu(IContextMenu* contextMenu);

    HRESULT ChangeHotItem(DWORD changeType);
    HRESULT OnHotItemChange(const NMTBHOTITEM * hot);

protected:
    LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CMenuStaticToolbar :
    public CMenuToolbarBase
{
private:
    HMENU m_hmenu;

public:
    CMenuStaticToolbar(CMenuBand *menuBand);
    virtual ~CMenuStaticToolbar() {}

    HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    HRESULT GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

};

class CMenuSFToolbar :
    public CMenuToolbarBase
{
private:
    IShellFolder * m_shellFolder;
    LPCITEMIDLIST  m_idList;
    HKEY           m_hKey;

public:
    CMenuSFToolbar(CMenuBand *menuBand);
    virtual ~CMenuSFToolbar();

    HRESULT SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    HRESULT GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

private:
    LPITEMIDLIST GetPidlFromId(UINT uItem, INT* pIndex = NULL);
};

class CMenuBand :
    public CComCoClass<CMenuBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IMenuPopup,
    public IMenuBand,
    public IShellMenu2,
    public IWinEventHandler,
    public IShellMenuAcc
{
private:
    CMenuFocusManager  * m_focusManager;
    CMenuStaticToolbar * m_staticToolbar;
    CMenuSFToolbar     * m_SFToolbar;

    CComPtr<IOleWindow>         m_site;
    CComPtr<IShellMenuCallback> m_psmc;
    CComPtr<IMenuPopup>         m_subMenuChild;
    CComPtr<IMenuPopup>         m_subMenuParent;

    UINT  m_uId;
    UINT  m_uIdAncestor;
    DWORD m_dwFlags;
    PVOID m_UserData;
    HMENU m_hmenu;
    HWND  m_menuOwner;

    BOOL m_useBigIcons;
    HWND m_topLevelWindow;

    CMenuToolbarBase * m_hotBar;
    INT                m_hotItem;

public:
    CMenuBand();
    ~CMenuBand();

    DECLARE_NOT_AGGREGATABLE(CMenuBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu, IShellMenu)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IMenuBand, IMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu2, IShellMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenuAcc, IShellMenuAcc)
    END_COM_MAP()

    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc);

    // *** IMenuBand methods ***
    virtual HRESULT STDMETHODCALLTYPE IsMenuMessage(MSG *pmsg);
    virtual HRESULT STDMETHODCALLTYPE TranslateMenuMessage(MSG *pmsg, LRESULT *plRet);

    // *** IShellMenu methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE InvalidateItem(LPSMDATA psmd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetState(LPSMDATA psmd);
    virtual HRESULT STDMETHODCALLTYPE SetMenuToolbar(IUnknown *punk, DWORD dwFlags);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IShellMenu2 methods ***
    virtual HRESULT STDMETHODCALLTYPE GetSubMenu(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetToolbar(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetMinWidth(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetNoBorder(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetTheme(THIS);

    // *** IShellMenuAcc methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTop(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetBottom(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetTracked(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetParentSite(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetState(THIS);
    virtual HRESULT STDMETHODCALLTYPE DoDefaultAction(THIS);
    virtual HRESULT STDMETHODCALLTYPE IsEmpty(THIS);

    HRESULT _CallCBWithItemId(UINT Id, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _CallCBWithItemPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _TrackSubMenuUsingTrackPopupMenu(HMENU popup, INT x, INT y);
    HRESULT _GetTopLevelWindow(HWND*topLevel);
    HRESULT _OnHotItemChanged(CMenuToolbarBase * tb, INT id);
    HRESULT _MenuItemHotTrack(DWORD changeType);
    HRESULT _OnPopupSubMenu(IMenuPopup * popup, POINTL * pAt, RECTL * pExclude);

    BOOL UseBigIcons()
    {
        return m_useBigIcons;
    }

private:
    HRESULT _CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id = 0, LPITEMIDLIST pidl = NULL);
};

class CMenuFocusManager :
    public CComCoClass<CMenuFocusManager>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>
{
private:
    static DWORD TlsIndex;

    static CMenuFocusManager * GetManager()
    {
        return reinterpret_cast<CMenuFocusManager *>(TlsGetValue(TlsIndex));
    }

public:
    static CMenuFocusManager * AcquireManager()
    {
        CMenuFocusManager * obj = NULL;

        if (!TlsIndex)
        {
            if ((TlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
                return NULL;
        }

        obj = GetManager();

        if (!obj)
        {
            obj = new CComObject<CMenuFocusManager>();
            TlsSetValue(TlsIndex, obj);
        }

        obj->AddRef();

        return obj;
    }

    static void ReleaseManager(CMenuFocusManager * obj)
    {
        if (!obj->Release())
        {
            TlsSetValue(TlsIndex, NULL);
        }
    }

private:
    static LRESULT CALLBACK s_GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam)
    {
        return GetManager()->GetMsgHook(nCode, wParam, lParam);
    }

private:
    CMenuBand * m_currentBand;
    HWND m_currentFocus;
    HHOOK m_hHook;
    DWORD m_threadId;

    // TODO: make dynamic
#define MAX_RECURSE 20
    CMenuBand* m_bandStack[MAX_RECURSE];
    int m_bandCount;

    HRESULT PushToArray(CMenuBand * item)
    {
        if (m_bandCount >= MAX_RECURSE)
            return E_OUTOFMEMORY;

        m_bandStack[m_bandCount++] = item;
        return S_OK;
    }

    HRESULT PopFromArray(CMenuBand ** pItem)
    {
        if (pItem)
            *pItem = NULL;

        if (m_bandCount <= 0)
            return E_FAIL;

        m_bandCount--;

        if (pItem)
            *pItem = m_bandStack[m_bandCount];

        m_bandStack[m_bandCount] = NULL;

        return S_OK;
    }

    HRESULT PeekArray(CMenuBand ** pItem)
    {
        if (!pItem)
            return E_FAIL;

        *pItem = NULL;

        if (m_bandCount <= 0)
            return E_FAIL;

        *pItem = m_bandStack[m_bandCount - 1];

        return S_OK;
    }

protected:
    CMenuFocusManager() :
        m_currentBand(NULL),
        m_currentFocus(NULL),
        m_bandCount(0)
    {
        m_threadId = GetCurrentThreadId();
    }

    ~CMenuFocusManager()
    {
    }

public:

    DECLARE_NOT_AGGREGATABLE(CMenuFocusManager)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CMenuFocusManager)
    END_COM_MAP()

    LRESULT GetMsgHook(INT nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode < 0)
            return CallNextHookEx(m_hHook, nCode, wParam, lParam);

        if (nCode == HC_ACTION)
        {
            BOOL callNext = TRUE;
            MSG* msg = reinterpret_cast<MSG*>(lParam);

            // Do whatever is necessary here

            switch (msg->message)
            {
            case WM_CLOSE:
                break;
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
                switch (msg->wParam)
                {
                case VK_MENU:
                case VK_LMENU:
                case VK_RMENU:
                    m_currentBand->_MenuItemHotTrack(MPOS_FULLCANCEL);
                    break;
                case VK_LEFT:
                    m_currentBand->_MenuItemHotTrack(MPOS_SELECTLEFT);
                    break;
                case VK_RIGHT:
                    m_currentBand->_MenuItemHotTrack(MPOS_SELECTRIGHT);
                    break;
                case VK_UP:
                    m_currentBand->_MenuItemHotTrack(VK_UP);
                    break;
                case VK_DOWN:
                    m_currentBand->_MenuItemHotTrack(VK_DOWN);
                    break;
                }
                break;
            case WM_CHAR:
                //if (msg->wParam >= 'a' && msg->wParam <= 'z')
                //{
                //    callNext = FALSE;
                //    PostMessage(m_currentFocus, WM_SYSCHAR, wParam, lParam);
                //}
                break;
            }

            if (!callNext)
                return 0;
        }

        return CallNextHookEx(m_hHook, nCode, wParam, lParam);
    }

    HRESULT PlaceHooks(HWND window)
    {
        //SetCapture(window);
        m_hHook = SetWindowsHookEx(WH_GETMESSAGE, s_GetMsgHook, NULL, m_threadId);
        return S_OK;
    }

    HRESULT RemoveHooks(HWND window)
    {
        UnhookWindowsHookEx(m_hHook);
        //ReleaseCapture();
        return S_OK;
    }

    HRESULT UpdateFocus(CMenuBand * newBand)
    {
        HRESULT hr;
        HWND newFocus;

        if (newBand == NULL)
        {
            hr = RemoveHooks(m_currentFocus);
            m_currentFocus = NULL;
            m_currentBand = NULL;
            return S_OK;
        }

        hr = newBand->_GetTopLevelWindow(&newFocus);
        if (FAILED(hr))
            return hr;

        if (!m_currentBand)
        {
            hr = PlaceHooks(newFocus);
            if (FAILED(hr))
                return hr;
        }

        m_currentFocus = newFocus;
        m_currentBand = newBand;

        return S_OK;
    }

public:
    HRESULT PushMenu(CMenuBand * mb)
    {
        HRESULT hr;

        hr = PushToArray(mb);
        if (FAILED(hr))
            return hr;

        return UpdateFocus(mb);
    }

    HRESULT PopMenu(CMenuBand * mb)
    {
        CMenuBand * mbc;
        HRESULT hr;

        hr = PopFromArray(&mbc);
        if (FAILED(hr))
            return hr;

        if (mb != mbc)
            return E_FAIL;

        hr = PeekArray(&mbc);

        return UpdateFocus(mbc);
    }
};

DWORD CMenuFocusManager::TlsIndex = 0;

extern "C"
HRESULT CMenuBand_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuBand * site = new CComObject<CMenuBand>();

    if (!site)
        return E_OUTOFMEMORY;

    HRESULT hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

CMenuToolbarBase::CMenuToolbarBase(CMenuBand *menuBand) :
    m_menuBand(menuBand),
    m_hwnd(NULL),
    m_dwMenuFlags(0)
{
}

HRESULT CMenuToolbarBase::IsWindowOwner(HWND hwnd)
{
    return (m_hwnd && m_hwnd == hwnd) ? S_OK : S_FALSE;
}

HRESULT CMenuToolbarBase::ShowWindow(BOOL fShow)
{
    ::ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    HIMAGELIST ilBig, ilSmall;
    Shell_GetImageLists(&ilBig, &ilSmall);

    if (m_menuBand->UseBigIcons())
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ilBig));
    }
    else
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(ilSmall));
    }

    return S_OK;
}

HRESULT CMenuToolbarBase::Close()
{
    DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    return S_OK;
}

HRESULT CMenuToolbarBase::CreateToolbar(HWND hwndParent, DWORD dwFlags)
{
    LONG tbStyles = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
        TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_REGISTERDROP | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_CUSTOMERASE |
        CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP;
    LONG tbExStyles = TBSTYLE_EX_DOUBLEBUFFER;

    if (dwFlags & SMINIT_VERTICAL)
    {
        tbStyles |= CCS_VERT;
        tbExStyles |= TBSTYLE_EX_VERTICAL | WS_EX_TOOLWINDOW;
    }

    RECT rc;

    if (!::GetClientRect(hwndParent, &rc) || (rc.left == rc.right) || (rc.top == rc.bottom))
    {
        rc.left = 0;
        rc.top = 0;
        rc.right = 1;
        rc.bottom = 1;
    }

    HWND hwndToolbar = CreateWindowEx(
        tbExStyles, TOOLBARCLASSNAMEW, NULL,
        tbStyles, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hwndParent, NULL, _AtlBaseModule.GetModuleInstance(), 0);

    if (hwndToolbar == NULL)
        return E_FAIL;

    ::SetParent(hwndToolbar, hwndParent);

    m_hwnd = hwndToolbar;

    /* Identify the version of the used Common Controls DLL by sending the size of the TBBUTTON structure */
    SendMessageW(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    //if (dwFlags & SMINIT_TOPLEVEL)
    //{
    //    /* Hide the placeholders for the button images */
    //    SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, 0);
    //}
    //else
    if (m_menuBand->UseBigIcons())
    {
        IImageList * piml;
        HRESULT hr = SHGetImageList(SHIL_LARGE, IID_PPV_ARG(IImageList, &piml));

        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(piml));
        SendMessageW(m_hwnd, TB_SETPADDING, 0, MAKELPARAM(0, 0));
    }
    else
    {
        IImageList * piml;
        HRESULT hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARG(IImageList, &piml));

        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(piml));
    }

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_SubclassOld = (WNDPROC) SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CMenuToolbarBase::s_SubclassProc));

    return S_OK;
}

HRESULT CMenuToolbarBase::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_FAIL;

    *phwnd = m_hwnd;

    return S_OK;
}

LRESULT CALLBACK CMenuToolbarBase::s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMenuToolbarBase * pthis = reinterpret_cast<CMenuToolbarBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return pthis->SubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CMenuToolbarBase::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        if (wParam == TIMERID_HOTTRACK)
        {
            KillTimer(hWnd, TIMERID_HOTTRACK);

            m_menuBand->_OnPopupSubMenu(NULL, NULL, NULL);

            if (HasSubMenu(m_hotItem) == S_OK)
            {
                PopupItem(m_hotItem);
            }
        }
    }

    return m_SubclassOld(hWnd, uMsg, wParam, lParam);
}

HRESULT CMenuToolbarBase::OnHotItemChange(const NMTBHOTITEM * hot)
{
    if (hot->dwFlags & HICF_LEAVING)
    {
        KillTimer(m_hwnd, TIMERID_HOTTRACK);
        m_hotItem = -1;
        m_menuBand->_OnHotItemChanged(NULL, -1);
        m_menuBand->_MenuItemHotTrack(MPOS_CHILDTRACKING);
    }
    else if (m_hotItem != hot->idNew)
    {
        DWORD elapsed = 0;
        SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &elapsed, 0);
        SetTimer(m_hwnd, TIMERID_HOTTRACK, elapsed, NULL);

        m_hotItem = hot->idNew;
        m_menuBand->_OnHotItemChanged(this, m_hotItem);
        m_menuBand->_MenuItemHotTrack(MPOS_CHILDTRACKING);
    }
    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT index, IShellMenu* childShellMenu)
{
    IBandSite* pBandSite;
    IDeskBar* pDeskBar;

    HRESULT hr = 0;
    RECT rc = { 0 };

    if (!SendMessage(m_hwnd, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwnd, &a);
    ClientToScreen(m_hwnd, &b);

    POINTL pt = { b.x, a.y };
    RECTL rcl = { a.x, a.y, b.x, b.y }; // maybe-TODO: fetch client area of deskbar?


#if USE_SYSTEM_MENUSITE
    hr = CoCreateInstance(CLSID_MenuBandSite,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IBandSite, &pBandSite));
#else
    hr = CMenuSite_Constructor(IID_PPV_ARG(IBandSite, &pBandSite));
#endif
    if (FAILED(hr))
        return hr;
#if WRAP_MENUSITE
    hr = CMenuSite_Wrapper(pBandSite, IID_PPV_ARG(IBandSite, &pBandSite));
    if (FAILED(hr))
        return hr;
#endif

#if USE_SYSTEM_MENUDESKBAR
    hr = CoCreateInstance(CLSID_MenuDeskBar,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IDeskBar, &pDeskBar));
#else
    hr = CMenuDeskBar_Constructor(IID_PPV_ARG(IDeskBar, &pDeskBar));
#endif
    if (FAILED(hr))
        return hr;
#if WRAP_MENUDESKBAR
    hr = CMenuDeskBar_Wrapper(pDeskBar, IID_PPV_ARG(IDeskBar, &pDeskBar));
    if (FAILED(hr))
        return hr;
#endif

    hr = pDeskBar->SetClient(pBandSite);
    if (FAILED(hr))
        return hr;

    hr = pBandSite->AddBand(childShellMenu);
    if (FAILED(hr))
        return hr;

    CComPtr<IMenuPopup> popup;
    hr = pDeskBar->QueryInterface(IID_PPV_ARG(IMenuPopup, &popup));
    if (FAILED(hr))
        return hr;

    m_menuBand->_OnPopupSubMenu(popup, &pt, &rcl);

    return S_OK;
}

HRESULT CMenuToolbarBase::PopupSubMenu(UINT index, HMENU menu)
{
    RECT rc = { 0 };

    if (!SendMessage(m_hwnd, TB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rc)))
        return E_FAIL;

    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwnd, &b);

    HMENU popup = GetSubMenu(menu, index);

    m_menuBand->_TrackSubMenuUsingTrackPopupMenu(popup, b.x, b.y);

    return S_OK;
}

HRESULT CMenuToolbarBase::DoContextMenu(IContextMenu* contextMenu)
{
    HRESULT hr;
    HMENU hPopup = CreatePopupMenu();

    if (hPopup == NULL)
        return E_FAIL;

    hr = contextMenu->QueryContextMenu(hPopup, 0, 0, UINT_MAX, CMF_NORMAL);
    if (FAILED(hr))
    {
        DestroyMenu(hPopup);
        return hr;
    }

    DWORD dwPos = GetMessagePos();
    UINT uCommand = ::TrackPopupMenu(hPopup, TPM_RETURNCMD, GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), 0, m_hwnd, NULL);
    if (uCommand == 0)
        return S_FALSE;

    CMINVOKECOMMANDINFO cmi = { 0 };
    cmi.cbSize = sizeof(cmi);
    cmi.lpVerb = MAKEINTRESOURCEA(uCommand);
    cmi.hwnd = m_hwnd;
    hr = contextMenu->InvokeCommand(&cmi);

    DestroyMenu(hPopup);
    return hr;
}

HRESULT CMenuToolbarBase::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    theResult = 0;
    return m_menuBand->_MenuItemHotTrack(MPOS_EXECUTE);
}


HRESULT CMenuToolbarBase::ChangeHotItem(DWORD dwSelectType)
{
    int prev = m_hotItem;
    int index = -1;

    if (dwSelectType != 0xFFFFFFFF)
    {
        int count = SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0);

        if (m_hotItem >= 0)
        {
            TBBUTTONINFO info = { 0 };
            info.cbSize = sizeof(TBBUTTONINFO);
            info.dwMask = 0;
            index = SendMessage(m_hwnd, TB_GETBUTTONINFO, m_hotItem, reinterpret_cast<LPARAM>(&info));
        }

        if (dwSelectType == VK_HOME)
        {
            index = 0;
            dwSelectType = VK_DOWN;
        }
        else if (dwSelectType == VK_END)
        {
            index = count - 1;
            dwSelectType = VK_UP;
        }
        else if (index < 0)
        {
            if (dwSelectType == VK_UP)
            {
                index = count - 1;
            }
            else if (dwSelectType == VK_DOWN)
            {
                index = 0;
            }
        }
        else
        {
            if (dwSelectType == VK_UP)
            {
                index--;
            }
            else if (dwSelectType == VK_DOWN)
            {
                index++;
            }
        }

        TBBUTTON btn = { 0 };
        while (index >= 0 && index < count)
        {
            DWORD res = SendMessage(m_hwnd, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn));
            if (!res)
                return E_FAIL;

            if (btn.dwData)
            {
                m_hotItem = btn.idCommand;
                if (prev != m_hotItem)
                {
                    SendMessage(m_hwnd, TB_SETHOTITEM, index, 0);
                    return m_menuBand->_OnHotItemChanged(this, m_hotItem);
                }
                return S_OK;
            }

            if (dwSelectType == VK_UP)
            {
                index--;
            }
            else if (dwSelectType == VK_DOWN)
            {
                index++;
            }
        }
    }

    m_hotItem = -1;
    if (prev != m_hotItem)
    {
        SendMessage(m_hwnd, TB_SETHOTITEM, -1, 0);
        m_menuBand->_OnHotItemChanged(NULL, -1);
    }
    return S_FALSE;
}

BOOL
AllocAndGetMenuString(HMENU hMenu, UINT ItemIDByPosition, WCHAR** String)
{
    int Length;

    Length = GetMenuStringW(hMenu, ItemIDByPosition, NULL, 0, MF_BYPOSITION);

    if (!Length)
        return FALSE;

    /* Also allocate space for the terminating NULL character */
    ++Length;
    *String = (PWSTR) HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));

    GetMenuStringW(hMenu, ItemIDByPosition, *String, Length, MF_BYPOSITION);

    return TRUE;
}

CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand *menuBand) :
    CMenuToolbarBase(menuBand),
    m_hmenu(NULL)
{
}

HRESULT  CMenuStaticToolbar::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    *phmenu = m_hmenu;
    *phwnd = NULL;
    *pdwFlags = m_dwMenuFlags;

    return S_OK;
}

HRESULT  CMenuStaticToolbar::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    m_hmenu = hmenu;
    m_dwMenuFlags = dwFlags;

    return S_OK;
}

HRESULT CMenuStaticToolbar::FillToolbar()
{
    int i;
    int ic = GetMenuItemCount(m_hmenu);

    for (i = 0; i < ic; i++)
    {
        MENUITEMINFOW info;
        TBBUTTON tbb = { 0 };
        PWSTR MenuString = NULL;

        tbb.fsState = TBSTATE_ENABLED;
        tbb.fsStyle = 0;

        info.cbSize = sizeof(info);
        info.fMask = MIIM_FTYPE | MIIM_ID;

        GetMenuItemInfoW(m_hmenu, i, TRUE, &info);

        if (info.fType == MFT_STRING)
        {
            if (!AllocAndGetMenuString(m_hmenu, i, &MenuString))
                return E_OUTOFMEMORY;
            if (::GetSubMenu(m_hmenu, i) != NULL)
                tbb.fsStyle |= BTNS_DROPDOWN;
            tbb.iString = (INT_PTR) MenuString;
            tbb.idCommand = info.wID;

            SMINFO * sminfo = new SMINFO();
            sminfo->dwMask = SMIM_ICON | SMIM_FLAGS;
            if (SUCCEEDED(m_menuBand->_CallCBWithItemId(info.wID, SMC_GETINFO, 0, reinterpret_cast<LPARAM>(sminfo))))
            {
                tbb.iBitmap = sminfo->iIcon;
                tbb.dwData = reinterpret_cast<DWORD_PTR>(sminfo);
                // FIXME: remove before deleting the toolbar or it will leak
            }
        }
        else
        {
            tbb.fsStyle |= BTNS_SEP;
        }

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));

        if (MenuString)
            HeapFree(GetProcessHeap(), 0, MenuString);
    }

    return S_OK;
}

HRESULT CMenuStaticToolbar::OnContextMenu(NMMOUSE * rclick)
{
    CComPtr<IContextMenu> contextMenu;
    HRESULT hr = m_menuBand->_CallCBWithItemId(rclick->dwItemSpec, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IContextMenu), reinterpret_cast<LPARAM>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return DoContextMenu(contextMenu);
}

HRESULT CMenuStaticToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT hr;
    hr = CMenuToolbarBase::OnCommand(wParam, lParam, theResult);
    if (FAILED(hr))
        return hr;

    return m_menuBand->_CallCBWithItemId(wParam, SMC_EXEC, 0, 0);
}

HRESULT CMenuStaticToolbar::PopupItem(UINT uItem)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return E_FAIL;

    TBBUTTON btn = { 0 };
    SendMessage(m_hwnd, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn));

    SMINFO * nfo = reinterpret_cast<SMINFO*>(btn.dwData);
    if (!nfo)
        return E_FAIL;

    if (nfo->dwFlags&SMIF_TRACKPOPUP)
    {
        return PopupSubMenu(index, m_hmenu);
    }
    else
    {
        CComPtr<IShellMenu> shellMenu;
        HRESULT hr = m_menuBand->_CallCBWithItemId(uItem, SMC_GETOBJECT, reinterpret_cast<WPARAM>(&IID_IShellMenu), reinterpret_cast<LPARAM>(&shellMenu));
        if (FAILED(hr))
            return hr;

        return PopupSubMenu(index, shellMenu);
    }
}

HRESULT CMenuStaticToolbar::HasSubMenu(UINT uItem)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return E_FAIL;
    return ::GetSubMenu(m_hmenu, index) ? S_OK : S_FALSE;
}

CMenuSFToolbar::CMenuSFToolbar(CMenuBand * menuBand) :
    CMenuToolbarBase(menuBand),
    m_shellFolder(NULL)
{
}

CMenuSFToolbar::~CMenuSFToolbar()
{
}

HRESULT CMenuSFToolbar::FillToolbar()
{
    HRESULT hr;
    int i = 0;
    PWSTR MenuString;

    IEnumIDList * eidl;
    m_shellFolder->EnumObjects(m_hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &eidl);

    LPITEMIDLIST item = static_cast<LPITEMIDLIST>(CoTaskMemAlloc(sizeof(ITEMIDLIST)));
    ULONG fetched;
    while ((hr = eidl->Next(1, &item, &fetched)) == S_OK)
    {
        INT index = 0;
        INT indexOpen = 0;

        TBBUTTON tbb = { 0 };
        tbb.fsState = TBSTATE_ENABLED;
        tbb.fsStyle = 0;

        CComPtr<IShellItem> psi;
        hr = SHCreateShellItem(NULL, m_shellFolder, item, &psi);
        if (FAILED(hr))
            return hr;

        hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &MenuString);
        if (FAILED(hr))
            return hr;

        index = SHMapPIDLToSystemImageListIndex(m_shellFolder, item, &indexOpen);

        SFGAOF attrs;
        hr = psi->GetAttributes(SFGAO_FOLDER, &attrs);

        if (attrs != 0)
        {
            tbb.fsStyle |= BTNS_DROPDOWN;
        }

        tbb.idCommand = ++i;
        tbb.iString = (INT_PTR) MenuString;
        tbb.iBitmap = index;
        tbb.dwData = reinterpret_cast<DWORD_PTR>(ILClone(item));
        // FIXME: remove before deleting the toolbar or it will leak

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));
        HeapFree(GetProcessHeap(), 0, MenuString);

    }
    CoTaskMemFree(item);

    // If no items were added, show the "empty" placeholder
    if (i == 0)
    {
        TBBUTTON tbb = { 0 };
        PCWSTR MenuString = L"(Empty)";

        tbb.fsState = 0/*TBSTATE_DISABLED*/;
        tbb.fsStyle = 0;
        tbb.iString = (INT_PTR) MenuString;
        tbb.iBitmap = -1;

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));

        return S_OK;
    }

    return hr;
}

HRESULT CMenuSFToolbar::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    m_shellFolder = psf;
    m_idList = pidlFolder;
    m_hKey = hKey;
    m_dwMenuFlags = dwFlags;
    return S_OK;
}

HRESULT CMenuSFToolbar::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    HRESULT hr;

    hr = m_shellFolder->QueryInterface(riid, ppv);
    if (FAILED(hr))
        return hr;

    if (pdwFlags)
        *pdwFlags = m_dwMenuFlags;

    if (ppidl)
    {
        LPITEMIDLIST pidl = NULL;

        if (m_idList)
        {
            pidl = ILClone(m_idList);
            if (!pidl)
            {
                (*(IUnknown**) ppv)->Release();
                return E_FAIL;
            }
        }

        *ppidl = pidl;
    }

    return hr;
}

LPITEMIDLIST CMenuSFToolbar::GetPidlFromId(UINT uItem, INT* pIndex)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, reinterpret_cast<LPARAM>(&info));
    if (index < 0)
        return NULL;

    if (pIndex)
        *pIndex = index;

    TBBUTTON btn = { 0 };
    if (!SendMessage(m_hwnd, TB_GETBUTTON, index, reinterpret_cast<LPARAM>(&btn)))
        return NULL;

    return reinterpret_cast<LPITEMIDLIST>(btn.dwData);
}

HRESULT CMenuSFToolbar::OnContextMenu(NMMOUSE * rclick)
{
    HRESULT hr;
    CComPtr<IContextMenu> contextMenu;
    LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(rclick->dwItemData);

    hr = m_shellFolder->GetUIObjectOf(m_hwnd, 1, &pidl, IID_IContextMenu, NULL, reinterpret_cast<VOID **>(&contextMenu));
    if (hr != S_OK)
        return hr;

    return DoContextMenu(contextMenu);
}

HRESULT CMenuSFToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT hr;
    hr = CMenuToolbarBase::OnCommand(wParam, lParam, theResult);
    if (FAILED(hr))
        return hr;

    return m_menuBand->_CallCBWithItemPidl(GetPidlFromId(wParam), SMC_SFEXEC, 0, 0);
}

HRESULT CMenuSFToolbar::PopupItem(UINT uItem)
{
    HRESULT hr;
    UINT uId;
    UINT uIdAncestor;
    DWORD flags;
    int index;
    CComPtr<IShellMenuCallback> psmc;
    CComPtr<IShellMenu> shellMenu;

    LPITEMIDLIST pidl = GetPidlFromId(uItem, &index);

    if (!pidl)
        return E_FAIL;

#if USE_SYSTEM_MENUBAND
    hr = CoCreateInstance(CLSID_MenuBand,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(IShellMenu, &shellMenu));
#else
    hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &shellMenu));
#endif
    if (FAILED(hr))
        return hr;
#if WRAP_MENUBAND
    hr = CMenuBand_Wrapper(shellMenu, IID_PPV_ARG(IShellMenu, &shellMenu));
    if (FAILED(hr))
        return hr;
#endif

    m_menuBand->GetMenuInfo(&psmc, &uId, &uIdAncestor, &flags);

    // FIXME: not sure waht to use as uId/uIdAncestor here
    hr = shellMenu->Initialize(psmc, 0, uId, SMINIT_VERTICAL);
    if (FAILED(hr))
        return hr;

    CComPtr<IShellFolder> childFolder;
    hr = m_shellFolder->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &childFolder));
    if (FAILED(hr))
        return hr;

    hr = shellMenu->SetShellFolder(childFolder, NULL, NULL, 0);
    if (FAILED(hr))
        return hr;

    return PopupSubMenu(index, shellMenu);
}

HRESULT CMenuSFToolbar::HasSubMenu(UINT uItem)
{
    HRESULT hr;
    CComPtr<IShellItem> psi;
    hr = SHCreateShellItem(NULL, m_shellFolder, GetPidlFromId(uItem), &psi);
    if (FAILED(hr))
        return S_FALSE;

    SFGAOF attrs;
    hr = psi->GetAttributes(SFGAO_FOLDER, &attrs);
    if (FAILED(hr))
        return hr;

    return (attrs != 0) ? S_OK : S_FALSE;
}

CMenuBand::CMenuBand() :
    m_staticToolbar(NULL),
    m_SFToolbar(NULL),
    m_site(NULL),
    m_psmc(NULL),
    m_subMenuChild(NULL),
    m_useBigIcons(FALSE),
    m_hotBar(NULL),
    m_hotItem(-1)
{
    m_focusManager = CMenuFocusManager::AcquireManager();
}

CMenuBand::~CMenuBand()
{
    CMenuFocusManager::ReleaseManager(m_focusManager);

    if (m_staticToolbar)
        delete m_staticToolbar;

    if (m_SFToolbar)
        delete m_SFToolbar;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::Initialize(
    IShellMenuCallback *psmc,
    UINT uId,
    UINT uIdAncestor,
    DWORD dwFlags)
{
    if (m_psmc != psmc)
        m_psmc = psmc;
    m_uId = uId;
    m_uIdAncestor = uIdAncestor;
    m_dwFlags = dwFlags;

    if (m_psmc)
    {
        _CallCB(SMC_CREATE, 0, reinterpret_cast<LPARAM>(&m_UserData));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenuInfo(
    IShellMenuCallback **ppsmc,
    UINT *puId,
    UINT *puIdAncestor,
    DWORD *pdwFlags)
{
    if (!pdwFlags) // maybe?
        return E_INVALIDARG;

    if (ppsmc)
        *ppsmc = m_psmc;

    if (puId)
        *puId = m_uId;

    if (puIdAncestor)
        *puIdAncestor = m_uIdAncestor;

    *pdwFlags = m_dwFlags;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    if (m_staticToolbar == NULL)
    {
        m_staticToolbar = new CMenuStaticToolbar(this);
    }
    m_hmenu = hmenu;
    m_menuOwner;

    HRESULT hr = m_staticToolbar->SetMenu(hmenu, hwnd, dwFlags);
    if (FAILED(hr))
        return hr;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    if (m_staticToolbar == NULL)
        return E_FAIL;

    return m_staticToolbar->GetMenu(phmenu, phwnd, pdwFlags);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetSite(IUnknown *pUnkSite)
{
    HWND    hwndParent;
    HRESULT hr;

    m_site = NULL;

    if (pUnkSite == NULL)
        return S_OK;

    hwndParent = NULL;
    hr = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_site));
    if (FAILED(hr))
        return hr;

    hr = m_site->GetWindow(&hwndParent);
    if (FAILED(hr))
        return hr;

    if (!::IsWindow(hwndParent))
        return E_FAIL;

    if (m_staticToolbar != NULL)
    {
        hr = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_staticToolbar->FillToolbar();
        if (FAILED(hr))
            return hr;
    }

    if (m_SFToolbar != NULL)
    {
        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
        if (FAILED(hr))
            return hr;
    }

    hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &m_subMenuParent));
    if (FAILED(hr))
        return hr;

    CComPtr<IOleWindow> pTopLevelWindow;
    hr = IUnknown_QueryService(m_site, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
    if (FAILED(hr))
        return hr;

    return pTopLevelWindow->GetWindow(&m_topLevelWindow);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetSite(REFIID riid, PVOID *ppvSite)
{
    if (m_site == NULL)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetWindow(
    HWND *phwnd)
{
    if (m_SFToolbar != NULL)
        return m_SFToolbar->GetWindow(phwnd);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->GetWindow(phwnd);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    SIZE sizeStaticY = { 0 };
    SIZE sizeShlFldY = { 0 };
    HWND hwndStatic = NULL;
    HWND hwndShlFld = NULL;
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetWindow(&hwndStatic);
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetWindow(&hwndShlFld);
    if (FAILED(hr))
        return hr;

    if (hwndStatic == NULL && hwndShlFld == NULL)
        return E_FAIL;

    if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeStaticY));
    if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeShlFldY));

    int sy = min(prc->bottom - prc->top, sizeStaticY.cy + sizeShlFldY.cy);

    int syStatic = sizeStaticY.cy;
    int syShlFld = sy - syStatic;

    if (hwndShlFld)
    {
        SetWindowPos(hwndShlFld, NULL,
            prc->left,
            prc->top,
            prc->right - prc->left,
            syShlFld,
            0);
        DWORD btnSize = SendMessage(hwndShlFld, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(hwndShlFld, TB_SETBUTTONSIZE, 0, MAKELPARAM(prc->right - prc->left, HIWORD(btnSize)));
    }
    if (hwndStatic)
    {
        SetWindowPos(hwndStatic, hwndShlFld,
            prc->left,
            prc->top + syShlFld,
            prc->right - prc->left,
            syStatic,
            0);
        DWORD btnSize = SendMessage(hwndStatic, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(hwndStatic, TB_SETBUTTONSIZE, 0, MAKELPARAM(prc->right - prc->left, HIWORD(btnSize)));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetBandInfo(
    DWORD dwBandID,
    DWORD dwViewMode,
    DESKBANDINFO *pdbi)
{
    HWND hwndStatic = NULL;
    HWND hwndShlFld = NULL;
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->GetWindow(&hwndStatic);
    if (FAILED(hr))
        return hr;

    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->GetWindow(&hwndShlFld);
    if (FAILED(hr))
        return hr;

    if (hwndStatic == NULL && hwndShlFld == NULL)
        return E_FAIL;

    // HACK (?)
    if (pdbi->dwMask == 0)
    {
        pdbi->dwMask = DBIM_MINSIZE | DBIM_MAXSIZE | DBIM_INTEGRAL | DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS | DBIM_BKCOLOR;
    }

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeShlFld));

        pdbi->ptMinSize.x = 0;
        pdbi->ptMinSize.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeShlFld));

        pdbi->ptMaxSize.x = max(sizeStatic.cx, sizeShlFld.cx); // ignored
        pdbi->ptMaxSize.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 0;
        pdbi->ptIntegral.y = 0;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&sizeShlFld));
        pdbi->ptActual.x = max(sizeStatic.cx, sizeShlFld.cx);

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeStatic));
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, reinterpret_cast<LPARAM>(&sizeShlFld));
        pdbi->ptActual.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

/* IDockingWindow */
HRESULT STDMETHODCALLTYPE  CMenuBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;
    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;

    if (fShow)
    {
        hr = _CallCB(SMC_INITMENU, 0, 0);
        if (FAILED(hr))
            return hr;
    }

    if (fShow)
        hr = m_focusManager->PushMenu(this);
    else
        hr = m_focusManager->PopMenu(this);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->Close();

    if (m_SFToolbar != NULL)
        return m_SFToolbar->Close();

    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMenuBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hr;

    hr = m_subMenuParent->SetSubMenu(this, fActivate);
    if (FAILED(hr))
        return hr;

    if (fActivate)
    {
        CComPtr<IOleWindow> pTopLevelWindow;
        hr = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IOleWindow, &pTopLevelWindow));
        if (FAILED(hr))
            return hr;

        hr = pTopLevelWindow->GetWindow(&m_topLevelWindow);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        m_topLevelWindow = NULL;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::HasFocusIO()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsDirty()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClassID(CLSID *pClassID)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (!pguidCmdGroup)
        return E_FAIL;

    if (IsEqualGUID(*pguidCmdGroup, CLSID_MenuBand))
    {
        if (nCmdID == 16) // set (big) icon size
        {
            this->m_useBigIcons = nCmdexecopt == 2;
            return S_OK;
        }
        else if (nCmdID == 19) // popup-related
        {
            return S_FALSE;
        }
    }

    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(guidService, SID_SMenuBandChild) ||
        IsEqualIID(guidService, SID_SMenuBandBottom) ||
        IsEqualIID(guidService, SID_SMenuBandBottomSelected))
        return this->QueryInterface(riid, ppvObject);
    WARN("Unknown service requested %s\n", wine_dbgstr_guid(&guidService));
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnSelect(DWORD dwSelectType)
{
    switch (dwSelectType)
    {
    case MPOS_CHILDTRACKING:
        // TODO: Cancel timers?
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_SELECTLEFT:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_SELECTRIGHT:
        if (m_hotBar && m_hotItem >= 0)
        {
            // TODO: popup the current child if it has subitems, otherwise spread up.
        }
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_EXECUTE:
    case MPOS_FULLCANCEL:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(dwSelectType);
        return m_subMenuParent->OnSelect(dwSelectType);
    case MPOS_CANCELLEVEL:
        if (m_subMenuChild)
            m_subMenuChild->OnSelect(dwSelectType);
        break;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetClient(IUnknown *punkClient)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClient(IUnknown **ppunkClient)
{
    // HACK, so I can test for a submenu in the DeskBar
    //UNIMPLEMENTED;
    if (ppunkClient)
    {
        if (m_subMenuChild)
            *ppunkClient = m_subMenuChild;
        else
            *ppunkClient = NULL;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    //UNIMPLEMENTED;
    //return S_OK;
    return S_FALSE;
    //return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    //UNIMPLEMENTED;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    if (m_SFToolbar == NULL)
    {
        m_SFToolbar = new CMenuSFToolbar(this);
    }

    HRESULT hr = m_SFToolbar->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    if (FAILED(hr))
        return hr;

    if (m_site)
    {
        HWND hwndParent;

        hr = m_site->GetWindow(&hwndParent);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hr))
            return hr;

        hr = m_SFToolbar->FillToolbar();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    if (m_SFToolbar)
        return m_SFToolbar->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(LPSMDATA psmd)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:

        if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
        {
            return m_staticToolbar->OnCommand(wParam, lParam, theResult);
        }

        if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
        {
            return m_SFToolbar->OnCommand(wParam, lParam, theResult);
        }

        return S_OK;

    case WM_NOTIFY:
        NMHDR * hdr = reinterpret_cast<LPNMHDR>(lParam);
        NMTBCUSTOMDRAW * cdraw;
        NMTBHOTITEM * hot;
        NMMOUSE * rclick;
        switch (hdr->code)
        {
        case TBN_HOTITEMCHANGE:
            hot = reinterpret_cast<LPNMTBHOTITEM>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_staticToolbar->OnHotItemChange(hot);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_SFToolbar->OnHotItemChange(hot);
            }

            return S_OK;

        case NM_RCLICK:
            rclick = reinterpret_cast<LPNMMOUSE>(hdr);

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_staticToolbar->OnContextMenu(rclick);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
            {
                return m_SFToolbar->OnContextMenu(rclick);
            }

            return S_OK;
        case NM_CUSTOMDRAW:
            cdraw = reinterpret_cast<LPNMTBCUSTOMDRAW>(hdr);
            switch (cdraw->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                *theResult = CDRF_NOTIFYITEMDRAW;
                return S_OK;

            case CDDS_ITEMPREPAINT:

                cdraw->clrBtnFace = GetSysColor(COLOR_MENU);
                cdraw->clrBtnHighlight = GetSysColor(COLOR_MENUHILIGHT);

                cdraw->clrText = GetSysColor(COLOR_MENUTEXT);
                cdraw->clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
                cdraw->clrHighlightHotTrack = GetSysColor(COLOR_HIGHLIGHTTEXT);

                RECT rc = cdraw->nmcd.rc;
                HDC hdc = cdraw->nmcd.hdc;

                HBRUSH bgBrush = GetSysColorBrush(COLOR_MENU);
                HBRUSH hotBrush = GetSysColorBrush(COLOR_MENUHILIGHT);

                switch (cdraw->nmcd.uItemState)
                {
                case CDIS_HOT:
                case CDIS_FOCUS:
                    FillRect(hdc, &rc, hotBrush);
                    break;
                default:
                    FillRect(hdc, &rc, bgBrush);
                    break;
                }

                *theResult = TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOETCHEDEFFECT | TBCDRF_HILITEHOTTRACK | TBCDRF_NOOFFSET;
                return S_OK;
            }
            return S_OK;
        }
        return S_OK;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsWindowOwner(HWND hWnd)
{
    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd) == S_OK)
        return S_OK;

    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd) == S_OK)
        return S_OK;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSubMenu(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetToolbar(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMinWidth(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetNoBorder(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetTheme(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTop(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetBottom(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTracked(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetParentSite(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::DoDefaultAction(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsEmpty(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT CMenuBand::_CallCBWithItemId(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _CallCB(uMsg, wParam, lParam, id);
}

HRESULT CMenuBand::_CallCBWithItemPidl(LPITEMIDLIST pidl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _CallCB(uMsg, wParam, lParam, 0, pidl);
}

HRESULT CMenuBand::_CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT id, LPITEMIDLIST pidl)
{
    if (!m_psmc)
        return S_FALSE;

    HWND hwnd;
    GetWindow(&hwnd);

    SMDATA smData = { 0 };
    smData.punk = static_cast<IShellMenu2*>(this);
    smData.uId = id;
    smData.uIdParent = m_uId;
    smData.uIdAncestor = m_uIdAncestor;
    smData.hwnd = hwnd;
    smData.pidlItem = pidl;
    if (m_staticToolbar)
    {
        smData.hmenu = m_hmenu;
    }
    smData.pvUserData = NULL;
    if (m_SFToolbar)
        m_SFToolbar->GetShellFolder(NULL, &smData.pidlFolder, IID_PPV_ARG(IShellFolder, &smData.psf));
    HRESULT hr = m_psmc->CallbackSM(&smData, uMsg, wParam, lParam);
    ILFree(smData.pidlFolder);
    if (smData.psf)
        smData.psf->Release();
    return hr;
}

HRESULT CMenuBand::_TrackSubMenuUsingTrackPopupMenu(HMENU popup, INT x, INT y)
{
    ::TrackPopupMenu(popup, 0, x, y, 0, m_menuOwner, NULL);
    return S_OK;
}

HRESULT CMenuBand::_GetTopLevelWindow(HWND*topLevel)
{
    *topLevel = m_topLevelWindow;
    return S_OK;
}

HRESULT CMenuBand::_OnHotItemChanged(CMenuToolbarBase * tb, INT id)
{
    if (m_hotBar && m_hotBar != tb)
        m_hotBar->ChangeHotItem(-1);
    m_hotBar = tb;
    m_hotItem = id;
    return S_OK;
}

HRESULT CMenuBand::_MenuItemHotTrack(DWORD changeType)
{
    HRESULT hr;

    if (changeType == VK_DOWN)
    {
        if (m_SFToolbar && (m_hotBar == m_SFToolbar || m_hotBar == NULL))
        {
            hr = m_SFToolbar->ChangeHotItem(VK_DOWN);
            if (hr == S_FALSE)
            {
                if (m_staticToolbar)
                    return m_staticToolbar->ChangeHotItem(VK_HOME);
                else
                    return m_SFToolbar->ChangeHotItem(VK_HOME);
            }
            return hr;
        }
        else if (m_staticToolbar && m_hotBar == m_staticToolbar)
        {
            hr = m_staticToolbar->ChangeHotItem(VK_DOWN);
            if (hr == S_FALSE)
            {
                if (m_SFToolbar)
                    return m_SFToolbar->ChangeHotItem(VK_HOME);
                else
                    return m_staticToolbar->ChangeHotItem(VK_HOME);
            }
            return hr;
        }
    }
    else if (changeType == VK_UP)
    {
        if (m_staticToolbar && (m_hotBar == m_staticToolbar || m_hotBar == NULL))
        {
            hr = m_staticToolbar->ChangeHotItem(VK_DOWN);
            if (hr == S_FALSE)
            {
                if (m_SFToolbar)
                    return m_SFToolbar->ChangeHotItem(VK_END);
                else
                    return m_staticToolbar->ChangeHotItem(VK_END);
            }
            return hr;
        }
        else if (m_SFToolbar && m_hotBar == m_SFToolbar)
        {
            hr = m_SFToolbar->ChangeHotItem(VK_UP);
            if (hr == S_FALSE)
            {
                if (m_staticToolbar)
                    return m_staticToolbar->ChangeHotItem(VK_END);
                else
                    return m_SFToolbar->ChangeHotItem(VK_END);
            }
            return hr;
        }
    }
    else
    {
        m_subMenuParent->OnSelect(changeType);
    }
    return S_OK;
}

HRESULT CMenuBand::_OnPopupSubMenu(IMenuPopup * popup, POINTL * pAt, RECTL * pExclude)
{
    if (m_subMenuChild)
    {
        HRESULT hr = m_subMenuChild->OnSelect(MPOS_CANCELLEVEL);
        if (FAILED(hr))
            return hr;
    }
    m_subMenuChild = popup;
    if (popup)
    {
        IUnknown_SetSite(popup, m_subMenuParent);
        popup->Popup(pAt, pExclude, MPPF_RIGHT);
    }
    return S_OK;
}
