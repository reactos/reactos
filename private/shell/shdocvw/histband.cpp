// histBand.cpp (based on favband.cpp)
//
// history band implementation
//
// Marc Miller(t-marcmi) - 1998
//

/*BUGBUG:

  A Note About The Search Feature:
  -------------------------------
  Making the ssearch results part of the history code should be temporary.
  There are currently plans to intergrate this into the search assistant.

  This code should be removed by the time IE5.0 ships.

  Contact:  t-shawnm or t-marcmi for details.
  */


#include "priv.h"
#include "sccls.h"
#include "nscband.h"
#include "nsc.h"
#include "resource.h"
#include "inpobj.h"
#include "dhuihand.h"

#include <mluisupp.h>

#define DM_HISTBAND     0x0000000
#define DM_GUIPAINS     0x40000000

#define REGKEY_HISTORY_VIEW TEXT("HistoryViewType")
#define REGKEY_DEFAULT_SIZE 0x10

#define VIEWTYPE_MAX        0x4  // A "guess" at how many viewtypes thare will be
#define VIEWTYPE_REALLOC    0x4  // How many to realloc at a time

// these are temporary
#define MENUID_SEARCH       0x4e4e

// Distance between history search go and stop buttons
#define HISTSRCH_BUTTONDIST 6 

extern HINSTANCE     g_hinst;

#define WM_SEARCH_STATE (WM_USER + 314)

class CHistBand : public CNSCBand,
                  public IShellFolderSearchableCallback
{
    friend HRESULT CHistBand_CreateInstance(IUnknown *punkOuter,
                                            IUnknown **ppunk, LPCOBJECTINFO poi);
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef (void) { return CNSCBand::AddRef();  };
    STDMETHODIMP_(ULONG) Release(void) { return CNSCBand::Release(); };
    
    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                  DWORD nCmdID,
                  DWORD nCmdexecopt,
                  VARIANTARG *pvarargIn,
                  VARIANTARG *pvarargOut);

    // *** IOleWindow methods ***
    //  (overriding CNSCBand implementation
    virtual STDMETHODIMP GetWindow(HWND *phwnd);

    // *** IInputObject methods ***
    //  (overriding CNSCBand/CToolBand's implementation)
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);

    // *** IShellFolderSearchableCallback methods ***
    virtual STDMETHODIMP RunBegin(DWORD dwReserved);
    virtual STDMETHODIMP RunEnd(DWORD dwReserved);
    
protected:
    virtual void    _AddButtons(BOOL fAdd);
    virtual HRESULT _OnRegisterBand(IOleCommandTarget *poctProxy);

    ~CHistBand();

    HRESULT       _InitViewPopup();
    HRESULT       _DoViewPopup(int x, int y);
    HRESULT       _ViewPopupSelect(UINT idCmd);

#ifdef SPLIT_HISTORY_VIEW_BUTTON
    UINT          _NextMenuItem();
#endif

    HRESULT       _ChangePidl(LPITEMIDLIST);
    HRESULT       _SelectPidl(LPCITEMIDLIST pidlSelect, BOOL fCreate,
                              LPCITEMIDLIST pidlViewType = NULL,
                              BOOL fReinsert = FALSE);

    LPITEMIDLIST  _GetCurrentSelectPidl(IOleCommandTarget *poctProxy = NULL);
    HRESULT       _SetRegistryPersistView(int iMenuID);
    int           _GetRegistryPersistView();
    LPCITEMIDLIST _MenuIDToPIDL(UINT uMenuID);
    int           _PIDLToMenuID(LPITEMIDLIST pidl);
    IShellFolderViewType*  _GetViewTypeInfo();
    HRESULT       _GetHistoryViews();
    HRESULT       _FreeViewInfo();

    void          _ResizeChildWindows(LONG width, LONG height, BOOL fRepaint);
    HRESULT       _DoSearchUIStuff();
    HRESULT       _ExecuteSearch(LPTSTR pszSearchString);
    HRESULT       _ClearSearch();
    IShellFolderSearchable *_EnsureSearch();
    static LRESULT CALLBACK s_EditWndSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR    CALLBACK s_HistSearchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    BOOL  _fStrsAdded;  // Strings from resource have been added as buttons on the toolbar
    LONG_PTR  _lStrOffset;

    HMENU _hViewMenu;          // an instance var so we can cache it
    UINT  _uViewCheckedItem;   // which menuitem in the View menu is checked?

    LPITEMIDLIST *_ppidlViewTypes;
    LPTSTR       *_ppszStrViewNames;
    UINT          _nViews;
    int           _iMaxMenuID;

    HWND          _hwndNSC;
    HWND          _hwndSearchDlg;
    LONG          _lSearchDlgHeight;
    LPITEMIDLIST  _pidlSearch;  // current search
    IShellFolderSearchable *_psfSearch;
    
    LPITEMIDLIST  _pidlHistory; // cache the history pidl from SHGetHistoryPIDL
    IShellFolder *_psfHistory;  // cache the history shell folder
    IShellFolderViewType  *_psfvtCache;  // view type information

    LPITEMIDLIST  _pidlLastSelect;
};

CHistBand::~CHistBand() {
    DestroyMenu(_hViewMenu);
    if (_pidlLastSelect)
        ILFree(_pidlLastSelect);
    if (_pidlHistory)
        ILFree(_pidlHistory);
    if (_psfHistory)
        _psfHistory->Release();
    if (_psfvtCache)
        _psfvtCache->Release();
   
    _ClearSearch(); // Frees _pidlSearch 
    if (_psfSearch)
        _psfSearch->Release();
    
    _FreeViewInfo();
}


// *** IUnknown methods ***
HRESULT CHistBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CHistBand, IShellFolderSearchableCallback),  // IID_IShellFolderSearchableCallback
        //QITABENT(CHistBand, IContextMenu),       // IID_IContextMenu
        //QITABENT(CHistBand, IWinEventHandler),   // IID_IWinEventHandler
        //QITABENT(CHistBand, IBandNavigate),      // IID_IBandNavigate
        { 0 },
    };
    HRESULT hres;

    hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CNSCBand::QueryInterface(riid, ppvObj);
    return hres;
}

// *** IOleCommandTarget methods ***
HRESULT CHistBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                        DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hRes = S_OK;
    if (pguidCmdGroup)
    {
        if (IsEqualGUID(CLSID_HistBand, *pguidCmdGroup))
        {
            switch(nCmdID)
            {
            case FCIDM_HISTBAND_VIEW:
                if (pvarargIn && (pvarargIn->vt == VT_I4))
                {
#ifdef SPLIT_HISTORY_VIEW_BUTTON
                    if (nCmdexecopt == OLECMDEXECOPT_PROMPTUSER)
                        hRes = _DoViewPopup(LOWORD(pvarargIn->lVal), HIWORD(pvarargIn->lVal));
                    else
                        hRes = _ViewPopupSelect(_NextMenuItem());
#else
                    ASSERT(nCmdexecopt == OLECMDEXECOPT_PROMPTUSER);
                    hRes = _DoViewPopup(LOWORD(pvarargIn->lVal), HIWORD(pvarargIn->lVal));
#endif
                }
                else
                    ASSERT(0);
                break;
                
            case FCIDM_HISTBAND_SEARCH:
                _ViewPopupSelect(MENUID_SEARCH);
                break;
            }
        }
        else if ((IsEqualGUID(CGID_Explorer, *pguidCmdGroup)))
        {
            switch (nCmdID)
            {
            case SBCMDID_SELECTHISTPIDL:
#ifdef ANNOYING_HISTORY_AUTOSELECT
                if (_uViewCheckedItem != MENUID_SEARCH)
                {
                    LPCITEMIDLIST pidlSelect = VariantToConstIDList(pvarargIn);

                    // Get the current view information
                    LPCITEMIDLIST pidlView = _MenuIDToPIDL(_uViewCheckedItem);
                    DWORD      dwViewFlags = SFVTFLAG_NOTIFY_CREATE;
                    IShellFolderViewType* psfvtInfo = _GetViewTypeInfo();

                    if (psfvtInfo)
                    {
                        // query for view type properties -- this will tell us how to
                        //   select the item...
                        hRes = psfvtInfo->GetViewTypeProperties(pidlView,
                                                                &dwViewFlags);
                        psfvtInfo->Release();
                    }
                    if (SUCCEEDED(hRes))
                    {
                        hRes = _SelectPidl(pidlSelect, dwViewFlags & SFVTFLAG_NOTIFY_CREATE,
                                           pidlView,   dwViewFlags & SFVTFLAG_NOTIFY_RESORT);
                    }
                }
                else //eat it, so that nsc doesn't get it
                    hRes = S_OK;
#endif //ANNOYING_HISTORY_AUTOSELECT
                hRes = S_OK;
                break;
                
            case SBCMDID_FILEDELETE:
                hRes = _InvokeCommandOnItem(TEXT("delete"));
                break;
            }
        }
        else
            hRes = CNSCBand::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    else
        hRes =  CNSCBand::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    return hRes;
}

// *** IInputObject methods ***
HRESULT CHistBand::TranslateAcceleratorIO(LPMSG pmsg)
{
#ifdef DEBUG
    if (pmsg->message == WM_KEYDOWN)
        TraceMsg(DM_GUIPAINS, "CHistBand -- TranslateAcceleratorIO called and _hwndSearchDlg is %x", _hwndSearchDlg);
#endif

    HWND hwndFocus = GetFocus();
    
    // Translate accelerator messages for dialog
    if ( (_hwndSearchDlg) && (hwndFocus != _hwndNSC) && (!hwndFocus || !IsChild(_hwndNSC, hwndFocus)) )
    {
        if (pmsg->message == WM_KEYDOWN)
        {
            if (IsVK_TABCycler(pmsg))
            {
                BOOL fBackwards = (GetAsyncKeyState(VK_SHIFT) < 0);
                HWND hwndCur = pmsg->hwnd;
                if (GetParent(pmsg->hwnd) != _hwndSearchDlg)
                    hwndCur = NULL;
                
                HWND hwndNext  = GetNextDlgTabItem(_hwndSearchDlg, hwndCur, fBackwards);
                
                // Get the First dialog item in this searching order
                HWND hwndFirst;
                if (!fBackwards) {
                    hwndFirst = GetNextDlgTabItem(_hwndSearchDlg, NULL, FALSE);
                }
                else
                    // passing NULL for the 2nd parameter returned NULL with ERROR_SUCCESS,
                    //  so this is a workaround
                    hwndFirst = GetNextDlgTabItem(_hwndSearchDlg,
                                                  GetNextDlgTabItem(_hwndSearchDlg,
                                                                    NULL, FALSE), TRUE);
                
                // If the next dialog tabstop is the first dialog tabstop, then
                //   let someone else get focus
                if ((!hwndCur) || (hwndNext != hwndFirst))
                {
                    SetFocus(hwndNext);
                    return S_OK;
                }
                else if (!fBackwards) {
                    SetFocus(_hwndNSC);
                    return S_OK;
                }
            }
            else if ( (pmsg->wParam == VK_RETURN) )
                SendMessage(_hwndSearchDlg, WM_COMMAND, MAKELONG(GetDlgCtrlID(pmsg->hwnd), 0), 0L);
        }
        // The History Search Edit Box is activated
        if (pmsg->hwnd == GetDlgItem(_hwndSearchDlg, IDC_EDITHISTSEARCH)) {
            // If the user pressed tab within the dialog
            return EditBox_TranslateAcceleratorST(pmsg);
        }
    }
    return CNSCBand::TranslateAcceleratorIO(pmsg);
}

// sends appropriate resize messages to our children windows
void CHistBand::_ResizeChildWindows(LONG width, LONG height, BOOL fRepaint)
{
    if (_hwndNSC)
    {
        int y1 = _hwndSearchDlg ? _lSearchDlgHeight : 0;
        int y2 = _hwndSearchDlg ? height - _lSearchDlgHeight : height;

        MoveWindow(_hwndNSC, 0, y1, width, y2, fRepaint);
    }

    if (_hwndSearchDlg)
    {
        MoveWindow(_hwndSearchDlg, 0, 0, width, _lSearchDlgHeight, fRepaint);
    }
}

HRESULT CHistBand::_DoSearchUIStuff()
{
    // host the search dialog inside my window:
    _hwndSearchDlg = CreateDialogParam(MLGetHinst(), MAKEINTRESOURCE(DLG_HISTSEARCH2),
                                       _hwnd, s_HistSearchDlgProc, reinterpret_cast<LPARAM>(this));

    RECT rcSelf;
    GetClientRect(_hwnd, &rcSelf);
    
    RECT rcDlg;
    GetClientRect(_hwndSearchDlg, &rcDlg);

    _lSearchDlgHeight = rcDlg.bottom;

    _ResizeChildWindows(rcSelf.right, rcSelf.bottom, TRUE);
    ShowWindow(_hwndSearchDlg, SW_SHOWDEFAULT);

    return S_OK;
}

// WndProc for main window to go in rebar
LRESULT CALLBACK CHistBand::s_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CHistBand* phb = reinterpret_cast<CHistBand *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_SETFOCUS:
        {
            TraceMsg(DM_GUIPAINS, "Histband Parent -- SETFOCUS");
            // The only way this should be called is via a RB_CYCLEFOCUS->...->UIActivateIO->SetFocus
            //  therefore, we can assume that we're being tabbed into or something with equally good.
            // If we tab into the outer dummy window, transfer the focus to
            //  our appropriate child:
            BOOL fBackwards = (GetAsyncKeyState(VK_SHIFT) < 0);
            if (phb->_hwndSearchDlg) {
                // Select either the first or the last item in the dialog depending on
                //  whether we're shifting in or shifting out
                SetFocus(GetNextDlgTabItem(phb->_hwndSearchDlg, (NULL), fBackwards));
            }
            else {
                TraceMsg(DM_GUIPAINS, "NSC is being given focus!");
                SetFocus(phb->_hwndNSC);
            }
        }
        return 0;
    case WM_CREATE:
        SetWindowLongPtr(hWnd, GWLP_USERDATA,
                      (reinterpret_cast<LONG_PTR>((reinterpret_cast<CREATESTRUCT *>(lParam))->lpCreateParams)));
        return 0;
    case WM_SIZE:
        if (phb)
            phb->_ResizeChildWindows(LOWORD(lParam), HIWORD(lParam), TRUE);
        return 0;
    case WM_NCDESTROY:
        //make sure the search object gets freed when the view/window is destroyed, because it holds a ref to us
        phb->_ClearSearch();
        break;
        
    case WM_NOTIFY:
        {
            if (phb) {
                // We proxy the notification messages to our own parent who thinks that we
                //  are the namespace control
                LPNMHDR pnmh = (LPNMHDR)lParam;
                
                // Notification message coming from NSC
                if (pnmh->hwndFrom == phb->_hwndNSC)
                    return SendMessage(phb->_hwndParent, msg, wParam, lParam);
            }
        } // INTENTIONAL FALLTHROUGH
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// *** IOleWindow methods ***
HRESULT CHistBand::GetWindow(HWND *phwnd)
{
    if (!_hwnd)
    {
        // we want to wrap a window around the namespace control so
        //  that we can add siblings later
        
        // Get our parent's dimensions
        RECT rcParent;
        GetClientRect(_hwndParent, &rcParent);

        static LPTSTR pszClassName = TEXT("History Pane");

        WNDCLASSEX wndclass    = { 0 };
        wndclass.cbSize        = sizeof(wndclass);
        wndclass.style         = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = ((WNDPROC)s_WndProc);
        wndclass.hInstance     = g_hinst;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.lpszClassName = pszClassName;

        RegisterClassEx(&wndclass);
    
        _hwnd = CreateWindow(pszClassName, TEXT("History Window"),
                             WS_CHILD | WS_TABSTOP,
                             0, 0, rcParent.right, rcParent.bottom,
                             _hwndParent, NULL, g_hinst, (LPVOID)this);
    }
    
    if (_hwnd)   // Host NSC
        _pns->CreateTree(_hwnd, 0, &_hwndNSC);

    return CToolBand::GetWindow(phwnd);
}

// *** IDockingWindow methods ***
HRESULT CHistBand::ShowDW(BOOL fShow)
{
    HRESULT hres = CNSCBand::ShowDW(fShow);
    _AddButtons(fShow);
    return hres;
}

static const TBBUTTON c_tbHistory[] =
{
    { I_IMAGENONE, FCIDM_HISTBAND_VIEW,   TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN | BTNS_SHOWTEXT,  {0,0}, 0, 0 },
    {           2, FCIDM_HISTBAND_SEARCH, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT,                       {0,0}, 0, 1 },
};

// Adds buttons from the above table to the Explorer
void CHistBand::_AddButtons(BOOL fAdd)
{
    // don't add button if we have no menu
    if (!_hViewMenu)
        return;

    IExplorerToolbar* piet;

    // BUGBUG: maybe QueryService would be better here ...
    if (SUCCEEDED(_punkSite->QueryInterface(IID_IExplorerToolbar, (void**)&piet)))
    {
        if (fAdd)
        {
            piet->SetCommandTarget((IUnknown*)SAFECAST(this, IOleCommandTarget*), &CLSID_HistBand, 0);

            if (!_fStrsAdded)
            {
                piet->AddString(&CLSID_HistBand, MLGetHinst(), IDS_HIST_BAR_LABELS, &_lStrOffset);
                _fStrsAdded = TRUE;
            }

            _EnsureImageListsLoaded();
            piet->SetImageList(&CLSID_HistBand, _himlNormal, _himlHot, NULL);

            TBBUTTON tbHistory[ARRAYSIZE(c_tbHistory)];
            memcpy(tbHistory, c_tbHistory, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbHistory));
            for (int i = 0; i < ARRAYSIZE(c_tbHistory); i++)
                tbHistory[i].iString += (long) _lStrOffset;

            piet->AddButtons(&CLSID_HistBand, ARRAYSIZE(tbHistory), tbHistory);
        }
        else
            piet->SetCommandTarget(NULL, NULL, 0);

        piet->Release();
    }
}

// *** IShellFolderSearchableCallback methods ***
// enable and disable cancel buttons 
HRESULT CHistBand::RunBegin(DWORD dwReserved)
{
    HRESULT hres = E_FAIL;
    if (_hwndSearchDlg)
    {
        SendMessage(_hwndSearchDlg, WM_SEARCH_STATE, (WPARAM)TRUE, NULL);
        hres = S_OK;
    }
    return hres;
}

HRESULT CHistBand::RunEnd(DWORD dwReserved)
{
    HRESULT hres = E_FAIL;
    if (_hwndSearchDlg)
    {
        SendMessage(_hwndSearchDlg, WM_SEARCH_STATE, (WPARAM)FALSE, NULL);
        hres = S_OK;
    }
    return hres;
}

// A utility function used in the WM_SIZE handling below...
inline HWND _GetHwndAndRect(HWND hwndDlg, int item, BOOL fClient, RECT &rc) {
    HWND hwnd = GetDlgItem(hwndDlg, item);
    if (fClient)
        GetClientRect(hwnd, &rc);
    else {
        GetWindowRect(hwnd, &rc);
        MapWindowPoints(NULL, hwndDlg, ((LPPOINT)&rc), 2);
    }
    return hwnd;
}

LRESULT CALLBACK CHistBand::s_EditWndSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_KEYDOWN:
        if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
            (wParam == TEXT('U'))) {
            uMsg   = WM_SETTEXT;
            wParam = 0;
            lParam = ((LPARAM)(LPCTSTR)TEXT(""));
        }
        break;

    case WM_CHAR:
        if (wParam == VK_RETURN) {
            PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDB_HISTSRCH_GO, 0), 0L);
            return 0L;
        }
        break;
    }
    return CallWindowProc((WNDPROC)(GetWindowLongPtr(hwnd, GWLP_USERDATA)), hwnd, uMsg, wParam, lParam);
}


//BUGBUG:  Please see note at top of file for explanation...
INT_PTR CALLBACK CHistBand::s_HistSearchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
    case WM_PAINT:
        {
            // paint a little separator bar on the bottom
            PAINTSTRUCT ps;
            RECT        rcSelf;
            HDC         hdc = BeginPaint(hwndDlg, &ps);
            GetClientRect(hwndDlg, &rcSelf);
            RECT        rcFill = { 0, rcSelf.bottom - 2, rcSelf.right, rcSelf.bottom };
            FillRect(hdc, &rcFill, GetSysColorBrush(COLOR_BTNFACE));
            EndPaint(hwndDlg, &ps);
            break;
        }

    // Supply child controls with correct bkgd color
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == GetDlgItem(hwndDlg, IDD_HISTSRCH_ANIMATION)) {
            SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
            return (INT_PTR) GetSysColorBrush(COLOR_WINDOW);
        }
        else {
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (INT_PTR) GetSysColorBrush(COLOR_WINDOW);
        }
    case WM_CTLCOLORDLG:
        //SetBkColor((HDC)HIWORD(lParam), GetSysColor(COLOR_WINDOW));
        return (INT_PTR) GetSysColorBrush(COLOR_WINDOW);
    case WM_INITDIALOG: {
        HWND    hwndEdit       = GetDlgItem(hwndDlg, IDC_EDITHISTSEARCH);
        WNDPROC pfnOldEditProc = (WNDPROC)(GetWindowLongPtr(hwndEdit, GWLP_WNDPROC));

        // subclass the editbox
        SetWindowLongPtr(hwndEdit, GWLP_USERDATA, (LPARAM)pfnOldEditProc);
        SetWindowLongPtr(hwndEdit, GWLP_WNDPROC,  (LPARAM)s_EditWndSubclassProc);
        
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        Animate_Open(GetDlgItem(hwndDlg, IDD_HISTSRCH_ANIMATION),
                     MAKEINTRESOURCE(IDA_HISTSEARCHAVI));

        // limit the edit control to MAX_PATH-1 characters
        Edit_LimitText(hwndEdit, MAX_PATH-1);

        break;
    }
    case WM_DESTROY:
        Animate_Close(GetDlgItem(hwndDlg, IDD_HISTSRCH_ANIMATION));
        break;
    case WM_SIZE: {
        if (wParam == SIZE_RESTORED) {
            UINT uWidth  = LOWORD(lParam);
            UINT uHeight = HIWORD(lParam);

            RECT rcAnimSize, rcCancel, rcSearch, rcEdit, rcStatic;
            HWND hwndAnim   = _GetHwndAndRect(hwndDlg, IDD_HISTSRCH_ANIMATION, TRUE,  rcAnimSize);
            HWND hwndCancel = _GetHwndAndRect(hwndDlg, IDCANCEL,               FALSE, rcCancel);
            HWND hwndSearch = _GetHwndAndRect(hwndDlg, IDB_HISTSRCH_GO,        FALSE, rcSearch);
            HWND hwndEdit   = _GetHwndAndRect(hwndDlg, IDC_EDITHISTSEARCH,     FALSE, rcEdit);
            
            // calculate the minimum tolerable width
            UINT uMinWidth  = ((rcCancel.right - rcCancel.left) +
                               (rcSearch.right - rcSearch.left) + HISTSRCH_BUTTONDIST +
                               rcEdit.left +
                               rcAnimSize.right + 1);

            if (uWidth < uMinWidth)
                uWidth = uMinWidth;

            HDWP hdwp = BeginDeferWindowPos(5);

            if (hdwp)
            {
                // align the animation box with the upper-right corner
                DeferWindowPos(hdwp, hwndAnim, HWND_TOP, uWidth - rcAnimSize.right, 0,
                               rcAnimSize.right, rcAnimSize.bottom, SWP_NOZORDER);
                
                // stretch the textbox as wide as possible
                UINT uNewTextWidth = uWidth - rcAnimSize.right - 1 - rcEdit.left;
                DeferWindowPos(hdwp, hwndEdit, HWND_TOP, rcEdit.left, rcEdit.top, uNewTextWidth,
                               rcEdit.bottom - rcEdit.top, SWP_NOZORDER);
                
                // static text should not be longer than edit textbox
                HWND hwndStatic = _GetHwndAndRect(hwndDlg, IDC_HISTSRCH_STATIC, FALSE, rcStatic);
                DeferWindowPos(hdwp, hwndStatic, HWND_TOP, rcEdit.left, rcStatic.top, uNewTextWidth,
                               rcStatic.bottom - rcStatic.top, SWP_NOZORDER);
                
                // align the cancel button with the right of the edit box
                UINT uCancelLeft = uWidth - rcAnimSize.right - 1 - (rcCancel.right - rcCancel.left);
                DeferWindowPos(hdwp, hwndCancel, HWND_TOP, uCancelLeft, rcCancel.top,
                               rcCancel.right - rcCancel.left, rcCancel.bottom - rcCancel.top, SWP_NOZORDER);
                
                // align the search button so that it ends six pixels (HISTSRCH_BUTTONDIST)
                //   to the left of the cancel button
                DeferWindowPos(hdwp, hwndSearch, HWND_TOP,
                               uCancelLeft - HISTSRCH_BUTTONDIST - (rcSearch.right - rcSearch.left),
                               rcSearch.top, rcSearch.right - rcSearch.left, rcSearch.bottom - rcSearch.top, SWP_NOZORDER);
            }
            EndDeferWindowPos(hdwp);
        }
        else
            return FALSE;
        break;
    }
    case WM_COMMAND: {
        CHistBand *phb = reinterpret_cast<CHistBand *>(GetWindowLongPtr(hwndDlg, DWLP_USER));

        switch (LOWORD(wParam)) {
        case IDC_EDITHISTSEARCH:
            switch (HIWORD(wParam)) {
            case EN_SETFOCUS:
                // This guy allows us to intercept TranslateAccelerator messages
                //  like backspace.  This is the same as calling UIActivateIO(TRUE), but
                //  doesn't cause an infinite setfocus loop in Win95
                UnkOnFocusChangeIS(phb->_punkSite, SAFECAST(phb, IInputObject*), TRUE);
                SetFocus((HWND)lParam);
                break;
            case EN_CHANGE:
                // Enable 'Go Fish' button iff there is text in the edit box
                EnableWindow(GetDlgItem(hwndDlg, IDB_HISTSRCH_GO),
                             (bool) SendDlgItemMessage(hwndDlg, IDC_EDITHISTSEARCH, EM_LINELENGTH, 0, 0));
                break;
            }
            break;
        case IDB_HISTSRCH_GO:
            {
                TCHAR szSearchString[MAX_PATH];
                if (GetDlgItemText(hwndDlg, IDC_EDITHISTSEARCH, szSearchString, ARRAYSIZE(szSearchString)))
                {
                    IServiceProvider *pServiceProvider;
                    
                    HRESULT hr = IUnknown_QueryService(phb->_punkSite, 
                                                       SID_SProxyBrowser, 
                                                       IID_IServiceProvider, 
                                                       (void **)&pServiceProvider);

                    if (SUCCEEDED(hr))
                    {
                        IWebBrowser2 *pWebBrowser2;
                        hr = pServiceProvider->QueryService(SID_SWebBrowserApp, 
                                                            IID_IWebBrowser2, 
                                                            (void **)&pWebBrowser2);
                        if (SUCCEEDED(hr))
                        {
                            ::PutFindText(pWebBrowser2, szSearchString);
                            pWebBrowser2->Release();
                        }

                        pServiceProvider->Release();
                    }

                    phb->_ExecuteSearch(szSearchString);
                }
            }
            break;
        case IDCANCEL:
            {
                if (phb->_EnsureSearch())
                {
                    phb->_psfSearch->CancelAsyncSearch(phb->_pidlSearch, NULL);
                }
                break;
            }
        default:
            return FALSE;
        }
        return FALSE;
    }
    case WM_SEARCH_STATE:
        {
            BOOL fStart = (BOOL)wParam;
            if (fStart)
                Animate_Play(GetDlgItem(hwndDlg, IDD_HISTSRCH_ANIMATION), 0, -1, -1);
            else {
                HWND hwndAnim = GetDlgItem(hwndDlg, IDD_HISTSRCH_ANIMATION);
                Animate_Stop(hwndAnim);
                Animate_Seek(hwndAnim, 0); // reset the animation

                //HACK for IE5 ship
                //if there's only one item found in history search, the item doesn't display
                //because someone (comctl32?) set redraw to false.
                //so, manually force it to true when the search stops
                CHistBand *phb = reinterpret_cast<CHistBand *>(GetWindowLongPtr(hwndDlg, DWLP_USER));
                if (phb)
                    SendMessage(phb->_hwndNSC, WM_SETREDRAW, TRUE, 0);
            }
            HWND hwndFocus = GetFocus();

            EnableWindow(GetDlgItem(hwndDlg, IDC_EDITHISTSEARCH), !fStart);
            EnableWindow(GetDlgItem(hwndDlg, IDB_HISTSRCH_GO), !fStart);            
            EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), fStart);

            //make sure the focus goes to the right place
            if ((NULL != hwndFocus) && (hwndFocus == GetDlgItem(hwndDlg, IDC_EDITHISTSEARCH) ||
                                       (hwndFocus == GetDlgItem(hwndDlg, IDCANCEL))))
                SetFocus(GetDlgItem(hwndDlg, fStart ? IDCANCEL : IDC_EDITHISTSEARCH));
            break;
        }

    default:
        return FALSE;
    }
    return TRUE;
}

IShellFolderSearchable *CHistBand::_EnsureSearch() {
    ASSERT(_psfHistory);
    if (!_pidlSearch) {
        _psfHistory->QueryInterface(IID_IShellFolderSearchable,
                                    (LPVOID *)&_psfSearch);
    }
    return _psfSearch;
}

HRESULT CHistBand::_ClearSearch() {
    HRESULT hres = S_FALSE;

    if (_pidlSearch) {
        if (_EnsureSearch())
        {
            hres = _psfSearch->InvalidateSearch(_pidlSearch, NULL);
        }
        ILFree(_pidlSearch);
        _pidlSearch = NULL;
    }
    return hres;
}
    
HRESULT CHistBand::_ExecuteSearch(LPTSTR pszSearchString)
{
    HRESULT hres = E_FAIL;
    
    if (_EnsureSearch())
    {
        _ClearSearch();
        hres = _psfSearch->FindString(pszSearchString,
                                                     NULL,
                                                     reinterpret_cast<IUnknown *>
                                                     (static_cast<IShellFolderSearchableCallback *>
                                                      (this)),
                                                     &_pidlSearch);
        if (SUCCEEDED(hres))
        {
            _ChangePidl(ILCombine(_pidlHistory, _pidlSearch));
        }
    }
    return hres;
}

#ifdef SPLIT_HISTORY_VIEW_BUTTON
UINT CHistBand::_NextMenuItem() {
    if (_uViewCheckedItem + 1 > _nViews)
        return 1;
    else
        return _uViewCheckedItem + 1;
}
#endif

HRESULT CHistBand::_ViewPopupSelect(UINT idCmd) 
{
    HRESULT hres = E_FAIL;

    if (idCmd == MENUID_SEARCH)
    {
        if (_uViewCheckedItem != MENUID_SEARCH)
        {
            // display the dialog box
            if (SUCCEEDED(hres = _DoSearchUIStuff()))
            {
                _ChangePidl((LPITEMIDLIST)INVALID_HANDLE_VALUE); // blank out NSC
                _uViewCheckedItem = MENUID_SEARCH;
                CheckMenuRadioItem(_hViewMenu, 1, _iMaxMenuID, _uViewCheckedItem, MF_BYCOMMAND);
            }
        }
        SetFocus(GetDlgItem(_hwndSearchDlg, IDC_EDITHISTSEARCH));
    }
    else
    {
        LPCITEMIDLIST pidlNewSelect = _MenuIDToPIDL(idCmd);
        if (pidlNewSelect) {
            if (ILIsEmpty(pidlNewSelect))
                hres = _ChangePidl(ILClone(_pidlHistory));
            else
                hres = _ChangePidl(ILCombine(_pidlHistory, pidlNewSelect));

            if (SUCCEEDED(hres))
                hres = _SelectPidl(NULL, TRUE, pidlNewSelect);

            if ((SUCCEEDED(hres)) && (_uViewCheckedItem >= 0))
            {
                // get rid of search dialog -- its no longer needed
                if (_hwndSearchDlg) {
                    EndDialog(_hwndSearchDlg, 0);
                    DestroyWindow(_hwndSearchDlg);
                    _hwndSearchDlg = NULL;
                    // invalidate the previous search and prepare for the next
                    _ClearSearch();
                    RECT rcSelf;
                    GetClientRect(_hwnd, &rcSelf);
                    _ResizeChildWindows(rcSelf.right, rcSelf.bottom, TRUE);
                }
                
                _uViewCheckedItem = idCmd;
                CheckMenuRadioItem(_hViewMenu, 1, _iMaxMenuID,
                                   _uViewCheckedItem, MF_BYCOMMAND);
                // write out the new selection to registry
                EVAL(SUCCEEDED(_SetRegistryPersistView(_uViewCheckedItem)));
                hres = S_OK;
            }
        }
    }
    return hres;
}

HRESULT CHistBand::_DoViewPopup(int x, int y)
{
    if (!_hViewMenu) return E_FAIL;

    HRESULT hres = E_FAIL;

    UINT idCmd = TrackPopupMenu(_hViewMenu, TPM_RETURNCMD, x, y, 0, _hwnd, NULL);
    // Currently, re-selecting the menu item will cause the item to be refreshed
    //  This makes sense to me, but it can be prevented by
    //  testing idCmd != _uViewCheckedItem
    if ( (idCmd > 0) )
    {
        return _ViewPopupSelect(idCmd);
    }
    else
        hres = S_FALSE;

    return hres;
}

// Change the current select NSC pidl
// WARNING: The pidl passed in will be assimilated by us...
//          We will deallocate it.
HRESULT CHistBand::_ChangePidl(LPITEMIDLIST pidl) {
    if (_pidl)
        ILFree(_pidl);

    _pidl = pidl;
    if ((LPITEMIDLIST)INVALID_HANDLE_VALUE == pidl)
        _pidl = NULL;
    _pns->Initialize(pidl, (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS), (NSS_DROPTARGET | NSS_BROWSERSELECT));
    return S_OK;
}

// _SelectPidl - Have NSC change the current selected pidl
//
// passing NULL for pidlSelect will select the current select pidl
HRESULT CHistBand::_SelectPidl(LPCITEMIDLIST pidlSelect,        // <-Standard Hist-type pidl to select
                               BOOL fCreate,                    // <-create NSC item if not there?
                               LPCITEMIDLIST pidlView,/*=NULL*/ // <-special history view type or NULL
                               BOOL fReinsert /*=0*/)           // <-reinsert pidl into NSC and re-sort
{
    HRESULT hRes = S_OK;
    BOOL    fFreePidlSelect = FALSE;

    if ( (!pidlSelect) &&
         ((pidlSelect = _GetCurrentSelectPidl())) )
        fFreePidlSelect = TRUE;

    if (pidlSelect) {
        LPITEMIDLIST pidlNewSelect = NULL;

        // cache the last selected pidl
        if (_pidlLastSelect != pidlSelect) {
            if (_pidlLastSelect)
                ILFree(_pidlLastSelect);
            _pidlLastSelect = ILClone(pidlSelect);
        }

        if (pidlView && !ILIsEmpty(pidlView)) {
            IShellFolderViewType *psfvtInfo = _GetViewTypeInfo();

            if (psfvtInfo) {
                LPITEMIDLIST pidlFromRoot = ILFindChild(_pidlHistory,
                                                        pidlSelect);
                if (pidlFromRoot && !ILIsEmpty(pidlFromRoot))
                {
                    LPITEMIDLIST pidlNewFromRoot;
                    if (SUCCEEDED(psfvtInfo->TranslateViewPidl(pidlFromRoot, pidlView,
                                                               &pidlNewFromRoot)))
                    {
                        if (pidlNewFromRoot) {
                            pidlNewSelect = ILCombine(_pidlHistory, pidlNewFromRoot);
                            if (pidlNewSelect) {
                                _pns->SetSelectedItem(pidlNewSelect, fCreate, fReinsert, 0);
                                ILFree(pidlNewSelect);
                            }
                            ILFree(pidlNewFromRoot);
                        }
                    }
                }
                psfvtInfo->Release();
            }
        }
        else
            _pns->SetSelectedItem(pidlSelect, fCreate, fReinsert, 0);

        if (fFreePidlSelect)
            ILFree(const_cast<LPITEMIDLIST>(pidlSelect));
    }
    return hRes;
}

HRESULT CHistBand::_SetRegistryPersistView(int iMenuID) {
    LPCITEMIDLIST pidlReg = _MenuIDToPIDL(iMenuID);

    if (!pidlReg)
        return E_FAIL;

    return HRESULT_FROM_WIN32
        (SHRegSetUSValue(REGSTR_PATH_MAIN, REGKEY_HISTORY_VIEW,
                         REG_BINARY, (LPVOID)pidlReg, ILGetSize(pidlReg),
                         SHREGSET_HKCU | SHREGSET_FORCE_HKCU));
}

// Get the default view from the registry as a menu item
int CHistBand::_GetRegistryPersistView() {
    int          iRegMenu = -1;
    DWORD        dwType = REG_BINARY;

    ITEMIDLIST   pidlDefault = { 0 };

    // make a preliminary call to find out the size of the data
    DWORD cbData = 0;
    LONG error   = SHRegGetUSValue(REGSTR_PATH_MAIN, REGKEY_HISTORY_VIEW, &dwType,
                                   NULL, &cbData, FALSE, &pidlDefault,
                                   sizeof(pidlDefault));
    if (cbData)
    {
        LPITEMIDLIST pidlReg = ((LPITEMIDLIST)SHAlloc(cbData));

        if (pidlReg)
        {
            error = SHRegGetUSValue(REGSTR_PATH_MAIN, REGKEY_HISTORY_VIEW, &dwType,
                                    (LPVOID)pidlReg, &cbData, FALSE, &pidlDefault,
                                    sizeof(pidlDefault));

            if (error == ERROR_SUCCESS)
                iRegMenu = _PIDLToMenuID(pidlReg);

            SHFree(pidlReg);
        }
    }

    return iRegMenu;
}

LPCITEMIDLIST CHistBand::_MenuIDToPIDL(UINT uMenuID) {
    ASSERT(_ppidlViewTypes);
    if ((uMenuID > 0) && (uMenuID <= _nViews))
        return _ppidlViewTypes[uMenuID - 1];
    return NULL;
}

int CHistBand::_PIDLToMenuID(LPITEMIDLIST pidl) {
    ASSERT(_psfHistory && _ppidlViewTypes);

    int iMenuID = -1;

    // handle the empty pidl, which designates the
    //  default view, separately
    if (ILIsEmpty(pidl))
        iMenuID = 1;
    else {
        for (UINT u = 0; u < _nViews; ++u) {
            if (_psfHistory->CompareIDs(0, pidl, _ppidlViewTypes[u]) == 0)
                iMenuID = u + 1;
        }
    }
    return iMenuID;
}

// remember to release return value
IShellFolderViewType* CHistBand::_GetViewTypeInfo() {
    IShellFolderViewType* psfvRet = NULL;

    if (_psfvtCache)
    {
        _psfvtCache->AddRef();
        psfvRet = _psfvtCache;
    }
    else if (_psfHistory)
    {
        // QI For the views
        // We set the pointer because of a bad QI somewhere...
        if (SUCCEEDED(_psfHistory->QueryInterface(IID_IShellFolderViewType,
                                                  ((void**)&psfvRet))))
        {
            _psfvtCache = psfvRet;
            psfvRet->AddRef(); // one released in destructor, another by caller
        }
        else
            psfvRet = NULL;
    }
    return psfvRet;
}

HRESULT CHistBand::_FreeViewInfo() {
    if (_ppidlViewTypes) {
        // the first pidl in this list is NULL, the default view
        for (UINT u = 0; u < _nViews; ++u)
            if (EVAL(_ppidlViewTypes[u]))
                ILFree(_ppidlViewTypes[u]);
        LocalFree(_ppidlViewTypes);
        _ppidlViewTypes = NULL;
    }
    if (_ppszStrViewNames) {
        for (UINT u = 0; u < _nViews; ++u)
            if (EVAL(_ppszStrViewNames[u]))
                CoTaskMemFree(_ppszStrViewNames[u]);
        LocalFree(_ppszStrViewNames);
        _ppszStrViewNames = NULL;
    }
    return S_OK;
}

// Load the popup menu (if there are views to be had)
HRESULT CHistBand::_InitViewPopup() {
    HRESULT hRes = E_FAIL;

    _iMaxMenuID = 0;

    if (SUCCEEDED((hRes = _GetHistoryViews()))) {
        if ((_hViewMenu = CreatePopupMenu()))
        {
            // the IDCMD for the view menu will always be
            //   one more than the index into the view tables
            for (UINT u = 0; u < _nViews; ++u) {
                int iMenuID = _PIDLToMenuID(_ppidlViewTypes[u]);
                if (iMenuID >= 0)
                    AppendMenu(_hViewMenu, MF_STRING, iMenuID,
                               _ppszStrViewNames[u]);
                if (iMenuID > _iMaxMenuID)
                    _iMaxMenuID = iMenuID;
            }

            // retrieve the persisted view information
            //  and check the corresponding menu item
            int iSelectMenuID = _GetRegistryPersistView();
            if (iSelectMenuID < 0 || ((UINT)iSelectMenuID) > _nViews)
                iSelectMenuID = 1; //bogus menuid
            _uViewCheckedItem = iSelectMenuID;
            CheckMenuRadioItem(_hViewMenu, 1, _nViews, _uViewCheckedItem, MF_BYCOMMAND);
        }
    }

#ifdef HISTORY_VIEWSEARCHMENU
    // if this is a searchable shell folder, then add the search menu item
    if (_EnsureSearch())
    {
        hRes = S_OK;

        // only add separator if there is a menu already!
        if (!_hViewMenu)
            _hViewMenu = CreatePopupMenu();
        else
            AppendMenu(_hViewMenu, MF_SEPARATOR, 0, NULL);

        if (_hViewMenu)
        {
            TCHAR szSearchMenuText[MAX_PATH];
            LoadString(MLGetHinst(), IDS_SEARCH_MENUOPT,
                       szSearchMenuText, ARRAYSIZE(szSearchMenuText));
            AppendMenu(_hViewMenu, MF_STRING, MENUID_SEARCH, szSearchMenuText);
            _iMaxMenuID = MENUID_SEARCH;
        }
        else
            hRes = E_FAIL;
    }
#endif
    return hRes;
}

// This guy calls the enumerator
HRESULT CHistBand::_GetHistoryViews() {
    ASSERT(_psfHistory);
    HRESULT hRes = E_FAIL;

    UINT cbViews; // how many views are allocated

    ASSERT(VIEWTYPE_MAX > 0);

    EVAL(SUCCEEDED(_FreeViewInfo()));

    IShellFolderViewType *psfViewType = _GetViewTypeInfo();

    if (psfViewType)
    {
        // allocate buffers to store the view information
        _ppidlViewTypes = ((LPITEMIDLIST *)LocalAlloc(LPTR, VIEWTYPE_MAX * sizeof(LPITEMIDLIST)));
        if (_ppidlViewTypes) {
            _ppszStrViewNames = ((LPTSTR *)LocalAlloc(LPTR, VIEWTYPE_MAX * sizeof(LPTSTR)));
            if (_ppszStrViewNames) {
                IEnumIDList *penum = NULL;
                cbViews  = VIEWTYPE_MAX;
                _nViews  = 1;
                // get the default view information
                _ppidlViewTypes[0]   = IEILCreate(sizeof(ITEMIDLIST));
                if (_ppidlViewTypes[0] &&
                    SUCCEEDED((hRes = psfViewType->GetDefaultViewName(0, &(_ppszStrViewNames[0])))))
                {
                    // empty pidl will be the default
                    ASSERT(ILIsEmpty(_ppidlViewTypes[0]));
                    // get the iterator for the other views
                    if (SUCCEEDED((hRes = psfViewType->EnumViews(0, &penum)))) {
                        ULONG cFetched = 0;
                        // iterate to get other view information
                        while(SUCCEEDED(hRes)                                                   &&
                              SUCCEEDED(penum->Next(1, &(_ppidlViewTypes[_nViews]), &cFetched)) &&
                              cFetched)
                        {
                            STRRET strret;
                            // get the name of this view
                            if (SUCCEEDED((hRes = _psfHistory->GetDisplayNameOf(_ppidlViewTypes[_nViews],
                                                                                0, &strret))) &&
                                SUCCEEDED((hRes = StrRetToStr(&strret, NULL,
                                                              &(_ppszStrViewNames[_nViews])))))
                            {
                                // prepare for next iteration by reallocating the buffer if necessary
                                if (_nViews > cbViews - 1)
                                {
                                    LPITEMIDLIST *ppidlViewTypes = ((LPITEMIDLIST *)LocalReAlloc(_ppidlViewTypes,
                                                                                       (cbViews + VIEWTYPE_REALLOC) * sizeof(LPITEMIDLIST),
                                                                                       LMEM_MOVEABLE | LMEM_ZEROINIT));
                                    if (ppidlViewTypes)
                                    {
                                        _ppidlViewTypes = ppidlViewTypes;
                                        LPTSTR * ppszStrViewNames = ((LPTSTR *)LocalReAlloc(_ppszStrViewNames,
                                                                                   (cbViews + VIEWTYPE_REALLOC) * sizeof(LPTSTR),
                                                                                   LMEM_MOVEABLE | LMEM_ZEROINIT));
                                        if (ppszStrViewNames)
                                        {
                                            _ppszStrViewNames = ppszStrViewNames;
                                            cbViews += VIEWTYPE_REALLOC;
                                        }
                                        else
                                        {
                                            hRes = E_OUTOFMEMORY;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        hRes = E_OUTOFMEMORY;
                                        break;
                                    }
                                }
                                ++_nViews;
                            }
                        }
                        penum->Release();
                    }
                }
            }
        }
        psfViewType->Release();
    }
    return hRes;
}

HRESULT CHistBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    CHistBand * phb = new CHistBand();

    if (!phb)
        return E_OUTOFMEMORY;

    ASSERT(phb->_pidlHistory    == NULL &&
           phb->_pidlLastSelect == NULL &&
           phb->_pidl           == NULL &&
           phb->_psfvtCache     == NULL);


    if (SUCCEEDED(SHGetHistoryPIDL(&(phb->_pidlHistory))) &&
        SUCCEEDED(IEBindToObject(phb->_pidlHistory,
                                 &(phb->_psfHistory))))
    {
        HRESULT hResLocal = E_FAIL;

        // if we can get different views, then init with the persisted
        //   view type, otherwise, init with the top-level history type
        if (SUCCEEDED(phb->_InitViewPopup())) {
            LPCITEMIDLIST pidlInit = phb->_MenuIDToPIDL(phb->_uViewCheckedItem);
            if (pidlInit) {
                LPITEMIDLIST pidlFullInit = ILCombine(phb->_pidlHistory, pidlInit);
                if (pidlFullInit) {
                    hResLocal = phb->_Init(pidlFullInit);
                    ILFree(pidlFullInit);
                }
            }
        }
        else
            hResLocal = phb->_Init(phb->_pidlHistory);

        // From old favband code: // if (SUCCEEDED(phb->_Init((LPCITEMIDLIST)CSIDL_FAVORITES)))
        if (SUCCEEDED(hResLocal))
        {
            phb->_pns = CNscTree_CreateInstance();
            if (phb->_pns)
            {
                ASSERT(poi);
                phb->_poi = poi;
                // if you change this cast, fix up CChannelBand_CreateInstance
                *ppunk = SAFECAST(phb, IDeskBand *);

                IUnknown_SetSite(phb->_pns, *ppunk);
                phb->SetNscMode(MODE_HISTORY);
                return S_OK;
            }
        }
    }

    phb->Release();

    return E_FAIL;
}

// Ask the powers that be which pidl is selected...
LPITEMIDLIST CHistBand::_GetCurrentSelectPidl(IOleCommandTarget *poctProxy/* = NULL*/) {
    LPITEMIDLIST pidlRet = NULL;
    VARIANT var;
    BOOL    fReleaseProxy = FALSE;
    VariantInit(&var);
    var.vt = VT_EMPTY;

    if (poctProxy == NULL) {
        IBrowserService *pswProxy;
        if (SUCCEEDED(QueryService(SID_SProxyBrowser, IID_IBrowserService,
                                   (void **)&pswProxy)))
        {
            ASSERT(pswProxy);
            if (FAILED(pswProxy->QueryInterface(IID_IOleCommandTarget,
                                                (void **)&poctProxy)))
            {
                pswProxy->Release();
                return NULL;
            }
            else
                fReleaseProxy = TRUE;

            pswProxy->Release();
        }
    }

    //  Inquire the current select pidl
    if ((SUCCEEDED(poctProxy->Exec(&CGID_Explorer, SBCMDID_GETHISTPIDL,
                                   OLECMDEXECOPT_PROMPTUSER, NULL, &var))) &&
        (var.vt != VT_EMPTY))
    {
        pidlRet = ILClone(VariantToConstIDList(&var));
        VariantClearLazy(&var);
    }
    if (fReleaseProxy)
        poctProxy->Release();
    return pidlRet;
}

// gets called by CNSCBand::ShowDW every time history band is shown
HRESULT CHistBand::_OnRegisterBand(IOleCommandTarget *poctProxy) 
{
    HRESULT hRes = E_FAIL;
    if (_uViewCheckedItem != MENUID_SEARCH)
    {
        LPITEMIDLIST pidlSelect = _GetCurrentSelectPidl(poctProxy);
        if (pidlSelect)
        {
            _SelectPidl(pidlSelect, TRUE);
            ILFree(pidlSelect);
            hRes = S_OK;
        }
    }
    return hRes;
}

