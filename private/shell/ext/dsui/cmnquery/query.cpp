#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Private data and helper functions
/----------------------------------------------------------------------------*/

//
// ICommonQuery stuff 
//

#ifdef UNICODE
class CCommonQuery : public ICommonQuery, IObjectWithSite, ICommonQueryA, CUnknown
#else
class CCommonQuery : public ICommonQuery, IObjectWithSite, CUnknown
#endif
{
    private:
        IUnknown* _punkSite;

    public:
        CCommonQuery();
        ~CCommonQuery();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // ICommonQuery
        STDMETHOD(OpenQueryWindow)(THIS_ HWND hwndParent, LPOPENQUERYWINDOW pOpenQueryWnd, IDataObject** ppDataObject);

#ifdef UNICODE        
        // ICommonQueryA
        STDMETHOD(OpenQueryWindow)(THIS_ HWND hwndParent, LPOPENQUERYWINDOW_A pOpenQueryWnd, IDataObject** ppDataObject);
#endif

        // IObjectWithSite
        STDMETHODIMP SetSite(IUnknown* punk);
        STDMETHODIMP GetSite(REFIID riid, void **ppv);
};

//
// View layout constants used by our dialogs
//

#define VIEWER_DEFAULT_CY   200

#define COMBOEX_IMAGE_CX    16
#define COMBOEX_IMAGE_CY    16

//
// Internal reference to a form and page
//

typedef struct
{
    HDSA   hdsaPages;                   // DSA containing page entries
    DWORD  dwFlags;                     // flags
    CLSID  clsidForm;                   // CLSID identifier for this form
    LPTSTR pTitle;                      // title used for drop down / title bar
    HICON  hIcon;                       // hIcon passed by caller
    INT    iImage;                      // image list index of icon
    INT    iForm;                       // visible index of form in control
    INT    iPage;                       // currently selected page on form
} QUERYFORM, * LPQUERYFORM;

typedef struct
{
    CLSID    clsidForm;                 // CLSID to associate this form with
    BOOL     fPageIsANSI : 1;           // page was declared ANSI
    union
    {
        LPCQPAGE pPage;                 // CQPAGE structures
#ifdef UNICODE
        LPCQPAGE_A pPageA;              
#endif
    };
    union
    {
        LPCQPAGEPROC pPageProc;         // PageProc's used by thunking layer
#ifdef UNICODE
        LPCQPAGEPROC_A pPageProcA;
#endif
    };
    LPARAM   lParam;                    // PAGEPROC lParam
    HWND     hwndPage;                  // hWnd of page dialog // = NULL if none
} QUERYFORMPAGE, * LPQUERYFORMPAGE;

//
// Internal reference of a scope
//

typedef struct
{
    LPCQSCOPE pScope;
    INT iImage;
} QUERYSCOPE, * LPQUERYSCOPE;

//
// CQueryFrame, our implementation of IQueryFrame
//

#define CALLFORMPAGES_ANSI    0x0001    // call only ANSI query form pages
#define CALLFORMPAGES_UNICODE 0x0002    // call only UNICODE query form pages
#define CALLFORMPAGES_ALL     0x0003

class CQueryFrame : public IQueryFrame, CUnknown
{
    friend INT QueryWnd_MessageProc(HWND hwnd, LPMSG pMsg);
    friend INT_PTR CALLBACK QueryWnd_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    public:
        CQueryFrame(IUnknown* punkSite, LPOPENQUERYWINDOW pOpenQueryWindow, IDataObject** ppDataObject);
        ~CQueryFrame();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // Internal helper functions
        STDMETHOD(DoModal)(HWND hwndParent);

        // IQueryFrame
        STDMETHOD(AddScope)(THIS_ LPCQSCOPE pScope, INT i, BOOL fSelect);
        STDMETHOD(GetWindow)(THIS_ HWND* phWnd);
        STDMETHOD(InsertMenus)(THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidth);
        STDMETHOD(RemoveMenus)(THIS_ HMENU hmenuShared);
        STDMETHOD(SetMenu)(THIS_ HMENU hmenuShared, HOLEMENU holereservedMenu);
        STDMETHOD(SetStatusText)(THIS_ LPCTSTR pszStatusText);
        STDMETHOD(StartQuery)(THIS_ BOOL fStarting);
        STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery);
        STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery);
        STDMETHOD(CallForm)(THIS_ LPCLSID pclsidForm, UINT uMsg, WPARAM wParam, LPARAM lParam);
        STDMETHOD(GetScope)(THIS_ LPCQSCOPE* ppScope);
        STDMETHOD(GetHandler)(THIS_ REFIID riid, void **ppv);

    protected:
        // Helper functions
        VOID CloseQueryFrame(HRESULT hres);
#if HIDE_SEARCH_PANE
        VOID HideSearchPane(BOOL fHide);        
#endif
        INT FrameMessageBox(LPCTSTR pPrompt, UINT uType);

        // Message handlers
        HRESULT OnInitDialog(HWND hwnd);
        VOID DoEnableControls(VOID);
        LRESULT OnNotify(INT idCtrl, LPNMHDR pNotify);
        VOID OnSize(INT cx, INT cy);
        VOID OnGetMinMaxInfo(LPMINMAXINFO lpmmi);    
        VOID OnCommand(WPARAM wParam, LPARAM lParam);
        VOID OnInitMenu(HMENU hMenu);
        VOID OnEnterMenuLoop(BOOL fEntering);
        VOID OnMenuSelect(HMENU hMenu, UINT uID);
        HRESULT OnFindNow(VOID);
        BOOL OnNewQuery(BOOL fAlwaysPrompt);
        HRESULT OnBrowse(VOID);    
        HRESULT OnHelp(LPHELPINFO pHelpInfo);
        
        // Form/Scope helper fucntions
        HRESULT InsertScopeIntoList(LPCQSCOPE pScope, INT i, BOOL fAddToControl);
        HRESULT AddScopeToControl(LPQUERYSCOPE pQueryScope, INT i);
        HRESULT PopulateScopeControl(VOID);
        HRESULT GetSelectedScope(LPQUERYSCOPE* ppQueryScope);
        HRESULT AddFromIQueryForm(IQueryForm* pQueryForm, HKEY hkeyForm);
#ifdef UNICODE
        HRESULT AddFromIQueryFormA(IQueryFormA* pQueryForm, HKEY hkeyForm);
#endif
        HRESULT GatherForms(VOID);
        HRESULT GetForms(HKEY hKeyForms, LPTSTR pName);
        HRESULT PopulateFormControl(BOOL fIncludeHidden);
        HRESULT SelectForm(REFCLSID clsidForm);
        VOID SelectFormPage(LPQUERYFORM pQueryForm, INT iPage);
        HRESULT CallFormPages(LPQUERYFORM pQueryForm, DWORD dwFlags, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LPQUERYFORM FindQueryForm(REFCLSID clsidForm);

    private:
        IUnknown* _punkSite;                    // site object we need to pass through
        IQueryHandler* _pQueryHandler;         // IQueryHandler object we need to interact with
        LPOPENQUERYWINDOW _pOpenQueryWnd;      // copy of initial parameters provided by caller
        IDataObject** _ppDataObject;           // receives the resulting data object from handler

        DWORD      _dwHandlerViewFlags;        // flags from the handler

        BOOL       _fQueryRunning:1;           // = 1 => query has been started, via IQueryFrame::StartQuery(TRUE)
        BOOL       _fExitModalLoop:1;          // = 1 => must leave modal loop
        BOOL       _fScopesPopulated:1;        // = 1 => scope control has been populated
        BOOL       _fTrackingMenuBar:1;        // = 1 => then we are tracking the menu bar, therefore send activates etc
#if HIDE_SEARCH_PANE
        BOOL       _fHideSearchPane:1;         // = 1 => form area is currently hidden
#endif
        BOOL       _fAddScopesNYI:1;           // = 1 => did AddScopes return E_NOTIMPL
        BOOL       _fScopesAddedAsync:1;       // = 1 => scopes added async by the handler
        BOOL       _fScopeImageListSet:1;      // = 1 => scope image list has been set

        HRESULT    _hResult;                   // result value stored by CloseQueryFrame
        HKEY       _hkHandler;                 // registry key for the handler

        HWND       _hwnd;                      // main window handle
        HWND       _hwndResults;               // result viewer
        HWND       _hwndStatus;                // status bar

        HWND       _hwndFrame;                 // Query Pages tab control
        HWND       _hwndLookForLabel;          // "Find:"
        HWND       _hwndLookFor;               // Form combo
        HWND       _hwndLookInLabel;           // "In:"
        HWND       _hwndLookIn;                // Scope combo
        HWND       _hwndBrowse;                // "Browse"
        HWND       _hwndFindNow;               // "Find now"
        HWND       _hwndStop;                  // "Stop"
        HWND       _hwndNewQuery;              // "New Query"
        HWND       _hwndOK;                    // "OK"
        HWND       _hwndCancel;                // "Cancel"
        HWND       _hwndFindAnimation;         // Query issued animation

        HICON      _hiconSmall;                // large/small app icons
        HICON      _hiconLarge;

        HMENU      _hmenuFile;                 // handle of the frames menu bar

        HIMAGELIST _himlForms;                 // image list for query form objects

        SIZE       _szMinTrack;                // minimum track size of the window

        INT        _dxFormAreaLeft;            // offset to left edge of form area (from window left)
        INT        _dxFormAreaRight;           // offset to right edge of form area (from window right)
        INT        _dxButtonsLeft;             // offset to left edge of buttons (from window right)
        INT        _dxAnimationLeft;           // offset to left edge of aniimation (from window right)
        INT        _dyResultsTop;              // offset to top of results (from top of window)
        INT        _dyOKTop;                   // offset to top of "OK" buttom (from results top)
        INT        _dxGap;                     // gap between OK + Cancel / LookIn + Browse
        INT        _dyGap;                     // gap between bottom of OK,Cancel and the frame.

        INT        _cyStatus;                  // height of the status bar

        HDSA       _hdsaForms;                 // forms DSA
        HDSA       _hdsaPages;                 // pages DSA
        SIZE       _szForm;                    // size of the (current form we are displaying)

        HDSA       _hdsaScopes;                // scopes DSA
        INT        _iDefaultScope;             // index of the defualt scope to select (into DSA)

        LPQUERYFORM _pCurrentForm;              // == NULL if none / else -> form structure
        LPQUERYFORMPAGE _pCurrentFormPage;      // == NULL if none / else -> page structure
};

//
// Helper functions
//

INT_PTR CALLBACK QueryWnd_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT QueryWnd_MessageProc(HWND hwnd, LPMSG pMsg);

HRESULT _CallScopeProc(LPQUERYSCOPE pQueryScope, UINT uMsg, LPVOID pVoid);
INT _FreeScope(LPQUERYSCOPE pQueryScope);
INT _FreeScopeCB(LPVOID pItem, LPVOID pData);

HRESULT _CallPageProc(LPQUERYFORMPAGE pQueryFormPage, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT _FreeQueryFormCB(LPVOID pItem, LPVOID pData);
INT _FreeQueryForm(LPQUERYFORM pQueryForm);
INT _FreeQueryFormPageCB(LPVOID pItem, LPVOID pData);
INT _FreeQueryFormPage(LPQUERYFORMPAGE pQueryFormPage);

#ifdef UNICODE
HRESULT _AddFormsProcA(LPARAM lParam, LPCQFORM_A pForm);
HRESULT _AddPagesProcA(LPARAM lParam, REFCLSID clsidForm, LPCQPAGE_A pPage);
#endif

HRESULT _AddFormsProc(LPARAM lParam, LPCQFORM_W pForm);
HRESULT _AddPagesProc(LPARAM lParam, REFCLSID clsidForm, LPCQPAGE_W pPage);

//
// Help stuff
//

#define HELP_FILE (NULL)

static DWORD const aHelpIDs[] =
{
    0, 0
};

//
// constant strings
//

TCHAR const c_szCLSID[]             = TEXT("CLSID");
TCHAR const c_szForms[]             = TEXT("Forms");
TCHAR const c_szFlags[]             = TEXT("Flags");

TCHAR const c_szCommonQuery[]       = TEXT("CommonQuery");
TCHAR const c_szHandlerIs[]         = TEXT("Handler");
TCHAR const c_szFormIs[]            = TEXT("Form");
TCHAR const c_szSearchPaneHidden[]  = TEXT("SearchPaneHidden");


/*-----------------------------------------------------------------------------
/ CCommonQuery
/----------------------------------------------------------------------------*/

CCommonQuery::CCommonQuery() :
    _punkSite(NULL)
{
}

CCommonQuery::~CCommonQuery()
{
    DoRelease(_punkSite);
}

// IUnknown bits

#undef  CLASS_NAME
#define CLASS_NAME CCommonQuery
#include "unknown.inc"

STDMETHODIMP CCommonQuery::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_ICommonQuery, (ICommonQuery*)this,
#ifdef UNICODE
        &IID_ICommonQueryA, (ICommonQueryA*)this,
#endif
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// handle creating a new instance of CLSID_CommonQuery
//

STDAPI CCommonQuery_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CCommonQuery *pcq = new CCommonQuery;
    if ( !pcq )
        return E_OUTOFMEMORY;

    HRESULT hres = pcq->QueryInterface(IID_IUnknown, (void **)ppunk);
    pcq->Release();

    return hres;
}


/*-----------------------------------------------------------------------------
/ ICommonQuery methods
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ OpenQueryWindow
/ ---------------
/   Display the query window for the given provider, including collecting
/   all the forms etc.
/
/ In:
/   hwndParent -> parent window for this dialog
/   pOpenQueryWnd -> structure that defines how the window should be opened
/   ppDataObject -> receives a pointer to the data object
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CCommonQuery::OpenQueryWindow(THIS_ HWND hwndParent, LPOPENQUERYWINDOW pOpenQueryWnd, IDataObject** ppDataObject)
{
    HRESULT hres;
    CQueryFrame* pQueryFrame = NULL;

    TraceEnter(TRACE_QUERY, "CCommonQuery::OpenQueryWindow");

    if ( !pOpenQueryWnd || (hwndParent && !IsWindow(hwndParent)) )
        ExitGracefully(hres, E_INVALIDARG, "Bad parameters");
   
    if ( ppDataObject )
        *(ppDataObject) = NULL;

    pQueryFrame = new CQueryFrame(_punkSite, pOpenQueryWnd, ppDataObject);
    TraceAssert(pQueryFrame);

    if ( !pQueryFrame )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to construct the query window object");

    hres = pQueryFrame->DoModal(hwndParent);                // don't bother fail gracefully etc
    FailGracefully(hres, "Failed on calling DoModal");

exit_gracefully:

    DoRelease(pQueryFrame);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ OpenQueryWindow - thunking from ANSI
/ ---------------
/   Display the query window for the given provider, including collecting
/   all the forms etc.
/
/ In:
/   hwndParent -> parent window for this dialog
/   pOpenQueryWnd -> structure that defines how the window should be opened
/   ppDataObject -> receives a pointer to the data object
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

#ifdef UNICODE

STDMETHODIMP CCommonQuery::OpenQueryWindow(THIS_ HWND hwndParent, LPOPENQUERYWINDOW_A pOpenQueryWnd, IDataObject** ppDataObject)
{
    HRESULT hres;
    LPOPENQUERYWINDOW_W pOpenQueryWndW = NULL;

    TraceEnter(TRACE_QUERY, "CCommonQueryA::OpenQueryWindow");

    if ( !pOpenQueryWnd )
        ExitGracefully(hres, E_INVALIDARG, "No pOpenQueryWnd structure");

    // allocate the UNICODE verison of the OPENQUERYWINDOW structure that 
    // we can then passed to the wide versions of this API.
    
    pOpenQueryWndW = (LPOPENQUERYWINDOW_W)LocalAlloc(LPTR, pOpenQueryWnd->cbStruct);
    
    if ( !pOpenQueryWndW )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate OPENQUERYWINDOW structure");

    Trace(TEXT("Copying OPENQUERYWINDOW structure to %08x (from %08x, size %08x)"), 
                                        pOpenQueryWndW, pOpenQueryWnd, pOpenQueryWnd->cbStruct);

    CopyMemory(pOpenQueryWndW, pOpenQueryWnd, pOpenQueryWnd->cbStruct);

    // Thunk the persistance interface, this will be an ANSI assuming of course that
    // the caller has called the ANSI ICommonQuery interface.

    if ( !pOpenQueryWndW->pPersistQuery )
    {
        pOpenQueryWndW->pPersistQuery = new CPersistQueryW2A(pOpenQueryWnd->pPersistQuery);
        Trace(TEXT("pPersistA2W %08x"), pOpenQueryWndW->pPersistQuery);

        if ( !pOpenQueryWndW->pPersistQuery )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate persistance thunk");
    }

    hres = OpenQueryWindow(hwndParent, pOpenQueryWndW, ppDataObject);
    FailGracefully(hres, "Failed when calling UNICODE OpenQueryWindow function");

exit_gracefully:
  
    if ( pOpenQueryWndW )
    {
        DoRelease(pOpenQueryWndW->pPersistQuery);
        LocalFree((HLOCAL)pOpenQueryWndW);
    }

    TraceLeaveResult(hres);
}

#endif


/*----------------------------------------------------------------------------
/ IObjectWithSite
/----------------------------------------------------------------------------*/

STDMETHODIMP CCommonQuery::SetSite(IUnknown* punk)
{
    HRESULT hres = S_OK;

    TraceEnter(TRACE_QUERY, "CCommonQuery::SetSite");

    DoRelease(_punkSite);

    if ( punk )
    {
        TraceMsg("QIing for IUnknown from the site object");

        hres = punk->QueryInterface(IID_IUnknown, (void **)&_punkSite);
        FailGracefully(hres, "Failed to get IUnknown from the site object");
    }

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CCommonQuery::GetSite(REFIID riid, void **ppv)
{
    HRESULT hres;
    
    TraceEnter(TRACE_QUERY, "CCommonQuery::GetSite");

    if ( !_punkSite )
        ExitGracefully(hres, E_NOINTERFACE, "No site to QI from");

    hres = _punkSite->QueryInterface(riid, ppv);
    FailGracefully(hres, "QI failed on the site unknown object");

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame
/----------------------------------------------------------------------------*/

CQueryFrame::CQueryFrame(IUnknown *punkSite, LPOPENQUERYWINDOW pOpenQueryWindow, IDataObject** ppDataObject) :
    _punkSite(punkSite),
    _pOpenQueryWnd(pOpenQueryWindow),
    _ppDataObject(ppDataObject),
    _hiconLarge(NULL),
    _hiconSmall(NULL)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::CQueryFrame");

    if ( _punkSite )
        _punkSite->AddRef();

    TraceLeave();
}

CQueryFrame::~CQueryFrame()
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::~CQueryFrame");
    
    DoRelease(_punkSite);

    if ( _hiconLarge )
        DestroyIcon(_hiconLarge);

    if ( _hiconSmall )
        DestroyIcon(_hiconSmall);

    if ( _hkHandler )
        RegCloseKey(_hkHandler);

    if ( _hmenuFile )
        DestroyMenu(_hmenuFile);

    if ( _himlForms )
        ImageList_Destroy(_himlForms);

    if ( _hdsaForms )
    {
        Trace(TEXT("Destroying QUERYFORM DSA (%d)"), DSA_GetItemCount(_hdsaForms));
        DSA_DestroyCallback(_hdsaForms, _FreeQueryFormCB, NULL);
        _hdsaForms = NULL;
    }

    if ( _hdsaPages )
    {
        Trace(TEXT("Destroying QUERYFORMPAGE DSA (%d)"), DSA_GetItemCount(_hdsaPages));
        DSA_DestroyCallback(_hdsaPages, _FreeQueryFormPageCB, NULL);
        _hdsaPages = NULL;
    }

    if ( _hdsaScopes )
    {
        Trace(TEXT("Destroying QUERYSCOPE DSA (%d)"), DSA_GetItemCount(_hdsaScopes));
        DSA_DestroyCallback(_hdsaScopes, _FreeScopeCB, NULL);
        _hdsaScopes = NULL;
    }

    _pCurrentForm = NULL;
    _pCurrentFormPage = NULL;

    // Now discard the handler and its window (if we have one), if
    // we don't do this they will never kill their objects

    if ( _hwndResults )
    {
        DestroyWindow(_hwndResults);
        _hwndResults = NULL;
    }

    TraceLeave();
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CQueryFrame
#include "unknown.inc"

STDMETHODIMP CQueryFrame::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IQueryFrame, (IQueryFrame*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IQueryFrame
/----------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::DoModal(HWND hwndParent)
{
    HRESULT hres;
    HWND hwndFrame = NULL;
    HWND hwndFocus = NULL;
    HWND hwndTopOwner = hwndParent;
    MSG msg;
    INITCOMMONCONTROLSEX iccex;

    TraceEnter(TRACE_FRAME, "CQueryFrame::DoModal");

    // initialize with the query handler we need

    hres = CoCreateInstance(_pOpenQueryWnd->clsidHandler, NULL, CLSCTX_INPROC_SERVER, IID_IQueryHandler, (LPVOID*)&_pQueryHandler);
    FailGracefully(hres, "Failed to get IQueryHandler for the given CLSID");

    hres = _pQueryHandler->Initialize(this, _pOpenQueryWnd->dwFlags, _pOpenQueryWnd->pHandlerParameters);
    FailGracefully(hres, "Failed to initialize the handler");

    // mimic the behaviour of DialogBox by working out which control previously
    // had focus, which window to disable and then running a message
    // pump for our dialog.  Having done this we can then restore the state
    // back to something sensible.

    _fExitModalLoop = FALSE;                   // can be changed from hear down

    iccex.dwSize = SIZEOF(iccex);
    iccex.dwICC = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&iccex);

    if ( _pOpenQueryWnd->dwFlags & OQWF_HIDESEARCHUI )
    {
        hwndFrame = CreateDialogParam(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDD_FILTER),
                                      hwndParent, 
                                      QueryWnd_DlgProc, (LPARAM)this);
    }
    else
    {
        hwndFrame = CreateDialogParam(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDD_FIND),
                                      hwndParent, 
                                      QueryWnd_DlgProc, (LPARAM)this);
    }

    if ( !hwndFrame )
        ExitGracefully(hres, E_FAIL, "Failed to create the dialog");

    hwndFocus = GetFocus();

    if ( hwndTopOwner )
    {
        // walk up the window stack looking for the window to be disabled, this must
        // be the top-most non-child window.  If the resulting window is either
        // the desktop or is already disabled then don't bother.

        while ( GetWindowLong(hwndTopOwner, GWL_STYLE) & WS_CHILD )
            hwndTopOwner = GetParent(hwndTopOwner);

        TraceAssert(hwndTopOwner);

        if ( (hwndTopOwner == GetDesktopWindow()) 
                                || EnableWindow(hwndTopOwner, FALSE) )
        { 
            TraceMsg("Parent is disabled or the desktop window, therefore setting to NULL");
            hwndTopOwner = NULL;
        }
    }

    ShowWindow(hwndFrame, SW_SHOW);                     // show the query window
    
    while ( !_fExitModalLoop && GetMessage(&msg, NULL, 0, 0) > 0 ) 
    {
        if ( !QueryWnd_MessageProc(hwndFrame, &msg) && !IsDialogMessage(hwndFrame, &msg) )
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Now tidy up, make the parent the active window, enable the top most
    // window if there is one and restore focus as required.

    if ( hwndTopOwner )
        EnableWindow(hwndTopOwner, TRUE);

    if ( hwndParent && (GetActiveWindow() == hwndFrame) )
    {
        TraceMsg("Passing activation to parent");
        SetActiveWindow(hwndParent);
    }
    
    if ( IsWindow(hwndFocus) )
        SetFocus(hwndFocus);

    DestroyWindow(hwndFrame);                   // discard the current frame window

exit_gracefully:

    DoRelease(_pQueryHandler);

    TraceLeaveResult(_hResult);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::AddScope(THIS_ LPCQSCOPE pScope, INT i, BOOL fSelect)
{
    HRESULT hres;

    TraceEnter(TRACE_FRAME, "CQueryFrame::AddScope");

    if ( !pScope )
        ExitGracefully(hres, E_INVALIDARG, "No scope to add to the list");

    // Add the scope to the control and then ensure that we either have
    // its index stored (for default selection) or we select the 
    // item.

    if ( !_hdsaScopes || !DSA_GetItemCount(_hdsaScopes) )
    {
        TraceMsg("First scope being added, thefore selecting");
        fSelect = TRUE;
    }

    hres = InsertScopeIntoList(pScope, i, _fScopesPopulated);
    FailGracefully(hres, "Failed to add scope to control");

    if ( fSelect ) 
    {
        if ( !_fScopesPopulated )
        {
            Trace(TEXT("Storing default scope index %d"), ShortFromResult(hres));
            _iDefaultScope = ShortFromResult(hres);
        }
        else
        {
            Trace(TEXT("Selecting scope index %d"), ShortFromResult(hres));
            ComboBox_SetCurSel(_hwndLookIn, ShortFromResult(hres));
        }   
    }

    // hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::GetWindow(THIS_ HWND* phWnd)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::GetWindow");

    TraceAssert(phWnd);
    *phWnd = _hwnd;

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

// Add a menu group to the given menu bar, updating the width index accordingly
// so that other people can merge in accordingly

VOID _DoInsertMenu(HMENU hMenu, INT iIndexTo, HMENU hMenuToInsert, INT iIndexFrom)
{
    TCHAR szBuffer[MAX_PATH];
    HMENU hPopupMenu = NULL;

    TraceEnter(TRACE_FRAME, "_DoInsertMenu");
    
    hPopupMenu = CreatePopupMenu();
    
    if ( hPopupMenu )
    {
        Shell_MergeMenus(hPopupMenu, GetSubMenu(hMenuToInsert, iIndexFrom), 0x0, 0x0, 0x7fff, 0);

        GetMenuString(hMenuToInsert, iIndexFrom, szBuffer, ARRAYSIZE(szBuffer), MF_BYPOSITION);
        InsertMenu(hMenu, iIndexTo, MF_BYPOSITION|MF_POPUP, (UINT_PTR)hPopupMenu, szBuffer);
    }

    TraceLeave();
}

VOID _AddMenuGroup(HMENU hMenuShared, HMENU hMenuGroup, LONG iInsertAt, LPLONG pWidth)
{
    HRESULT hres;
    TCHAR szBuffer[MAX_PATH];
    HMENU hMenu;
    INT i;

    TraceEnter(TRACE_FRAME, "_AddMenuGroup");

    TraceAssert(hMenuShared);
    TraceAssert(hMenuGroup);
    TraceAssert(pWidth);

    for ( i = 0 ; i < GetMenuItemCount(hMenuGroup) ; i++ )
    {
        _DoInsertMenu(hMenuShared, iInsertAt+i, hMenuGroup, i);
        *pWidth += 1;
    }

    TraceLeave();
}

STDMETHODIMP CQueryFrame::InsertMenus(THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidth)
{
    HRESULT hres;

    TraceEnter(TRACE_FRAME, "CQueryFrame::InsertMenus");

    if ( !hmenuShared || !lpMenuWidth )
        ExitGracefully(hres, E_INVALIDARG, "Unable to insert menus");

    // if we don't have the menu bar already loaded then lets load it,
    // having done that we can then add our menu to the bar (we only
    // provide entries for the file menu).
    
    if ( !_hmenuFile )
    {
        _hmenuFile = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_FILEMENUGROUP));

        if ( !_hmenuFile )
            ExitGracefully(hres, E_FAIL, "Failed to load base menu defn");
    }

    _AddMenuGroup(hmenuShared, _hmenuFile, 0, &lpMenuWidth->width[0]);

    hres = S_OK;              // success

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::RemoveMenus(THIS_ HMENU hmenuShared)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::RemoveMenus");

    // We don't need to implement this as we copy or menus into the
    // menu that the handler supplies - fix DSQUERY if this ever
    // changes.

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::SetMenu(THIS_ HMENU hmenuShared, HOLEMENU holereservedMenu)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::SetMenu");

    if ( !(_pOpenQueryWnd->dwFlags & OQWF_HIDEMENUS) )
    {
        HMENU hmenuOld = ::GetMenu(_hwnd);

        if ( !hmenuShared )
            hmenuShared = _hmenuFile;

        ::SetMenu(_hwnd, hmenuShared);
        DoEnableControls();             // ensure the menu state is valid    
        ::DrawMenuBar(_hwnd);

        if ( (hmenuOld != _hmenuFile) && (hmenuOld != hmenuShared) )
        {
            TraceMsg("Destroying old menu");
            DestroyMenu(hmenuOld);
        }
    }

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::SetStatusText(THIS_ LPCTSTR pszStatusText)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::SetStatusText");
    Trace(TEXT("Setting status text to: %s"), pszStatusText);

    if ( _hwndStatus )
        SendMessage(_hwndStatus, SB_SETTEXT, 0, (LPARAM)pszStatusText); 

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::StartQuery(THIS_ BOOL fStarting)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::StartQuery");

    if ( fStarting )
    {
        Animate_Play(_hwndFindAnimation, 0, -1, -1);
    }
    else
    {
        Animate_Stop(_hwndFindAnimation);
        Animate_Seek(_hwndFindAnimation, 0);        // go to start
    }

    if ( _pQueryHandler )
        _pQueryHandler->ActivateView(CQRVA_STARTQUERY, (WPARAM)fStarting, 0);
    
    // now set the controls into a sensble state

    _fQueryRunning = fStarting;
    DoEnableControls();

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::LoadQuery(THIS_ IPersistQuery* pPersistQuery)
{
    HRESULT hres;
    TCHAR szGUID[GUIDSTR_MAX+1];
    LPQUERYFORM pQueryForm = NULL;
#ifdef UNICODE
    CPersistQueryA2W* pPersistQueryA2W = NULL;
#endif
#if HIDE_SEARCH_PANE
    BOOL fHideSearchPanes = FALSE;
#endif
    GUID guid;

    TraceEnter(TRACE_FRAME, "CQueryFrame::LoadQuery");

    _pQueryHandler->StopQuery();                       // ensure that the handler stops its processing

    // Attempt to read the handler GUID from the query stream, first try reading it as
    // as string then parsing it into something that we can use, if that fails then
    // try again, but this time read it as a structure.
    //
    // having aquired the GUID for the handler make sure that we have the correct handler
    // selected.

    if ( FAILED(pPersistQuery->ReadString(c_szCommonQuery, c_szHandlerIs, szGUID, ARRAYSIZE(szGUID))) ||
         !GetGUIDFromString(szGUID, &guid) )
    {
        TraceMsg("Trying new style handler GUID as struct");

        hres = pPersistQuery->ReadStruct(c_szCommonQuery, c_szHandlerIs, &guid, SIZEOF(guid));
        FailGracefully(hres, "Failed to read handler GUID as struct");
    }    

    if ( guid != _pOpenQueryWnd->clsidHandler )
        ExitGracefully(hres, E_FAIL, "Persisted handler GUID and specified handler GUID don't match");

#if HIDE_SEARCH_PANE    
    if ( SUCCEEDED(pPersistQuery->ReadInt(c_szCommonQuery, c_szSearchPaneHidden, &fHideSearchPanes)) )
    {
        Trace(TEXT("Hide forms %d form file"), fHideSearchPanes);
        HideSearchPane(fHideSearchPanes);
    }
#endif

    hres = _pQueryHandler->LoadQuery(pPersistQuery);
    FailGracefully(hres, "Handler failed to load its query data");

    // Get the form ID, then look up the form to see if we have one that matches,
    // if not then we cannot load any thing else. If we do haved that form then
    // ensure that we clear it and then load away.

    if ( FAILED(pPersistQuery->ReadString(c_szCommonQuery, c_szFormIs, szGUID, ARRAYSIZE(szGUID))) ||
         !GetGUIDFromString(szGUID, &guid) )
    {
        TraceMsg("Trying new style form GUID as struct");

        hres = pPersistQuery->ReadStruct(c_szCommonQuery, c_szFormIs, &guid, SIZEOF(guid));
        FailGracefully(hres, "Failed to read handler GUID as struct");
    }    

    hres = SelectForm(guid);
    FailGracefully(hres, "Failed to select the query form");

    if ( hres == S_FALSE )
        ExitGracefully(hres, E_FAIL, "Failed to select the query form to read the query info");
    
    hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_ALL, CQPM_CLEARFORM, 0, 0);
    FailGracefully(hres, "Failed to clear form before loading");

    // Load the persisted query from the stream, coping correctly with the 
    // UNICODE / ANSI issue.  We will be passed an IPersistQuery object which
    // we must then thunk accordingly if we are UNICODE for the pages we
    // are going to talk to.

#ifndef UNICODE
    hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_ANSI, CQPM_PERSIST, TRUE, (LPARAM)pPersistQuery);
    FailGracefully(hres, "Failed to load form data");
#else
    hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_UNICODE, CQPM_PERSIST, TRUE, (LPARAM)pPersistQuery);
    FailGracefully(hres, "Failed to load page data (UNICODE)");

    if ( SUCCEEDED(hres) )
    {
        pPersistQueryA2W = new CPersistQueryA2W(pPersistQuery);
        TraceAssert(pPersistQueryA2W);

        if ( !pPersistQueryA2W )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to construct persistance thunk object");

        hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_ANSI, CQPM_PERSIST, TRUE, (LPARAM)pPersistQueryA2W);
        FailGracefully(hres, "Failed to load page data (ANSI)");
    }
#endif

    hres = S_OK;          //  success

exit_gracefully:

    if ( SUCCEEDED(hres) )
    {
        TraceMsg("Query loaded successfully, select form query");
        SelectForm(guid);
    }

#ifdef UNICODE
    DoRelease(pPersistQueryA2W);
#endif

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::SaveQuery(THIS_ IPersistQuery* pPersistQuery)
{
    HRESULT hres;
    LPQUERYSCOPE pQueryScope;
#ifdef UNICODE
    CPersistQueryA2W* pPersistQueryA2W = NULL;
#endif
    TCHAR szBuffer[MAX_PATH];
    
    TraceEnter(TRACE_FRAME, "CQueryFrame::SaveQuery");

    if ( !pPersistQuery )
        ExitGracefully(hres, E_INVALIDARG, "No pPersistQuery object to write into");

    pPersistQuery->Clear();             // flush the contents

    hres = pPersistQuery->WriteStruct(c_szCommonQuery, c_szHandlerIs, 
                                                        &_pOpenQueryWnd->clsidHandler, 
                                                        SIZEOF(_pOpenQueryWnd->clsidHandler));
    FailGracefully(hres, "Failed to write handler GUID");

    hres = pPersistQuery->WriteStruct(c_szCommonQuery, c_szFormIs, 
                                                        &_pCurrentForm->clsidForm, 
                                                        SIZEOF(_pCurrentForm->clsidForm));
    FailGracefully(hres, "Failed to write form GUID");

#if HIDE_SEARCH_PANE
    hres = pPersistQuery->WriteInt(c_szCommonQuery, c_szSearchPaneHidden, _fHideSearchPane);
    FailGracefully(hres, "Failed to write form hide state");
#endif

    // Allow the handler to persist itself into the the stream, this includes
    // giving it the current scope to store.

    hres = GetSelectedScope(&pQueryScope);
    FailGracefully(hres, "Failed to get the scope from the LookIn control");

    hres = _pQueryHandler->SaveQuery(pPersistQuery, pQueryScope->pScope);
    FailGracefully(hres, "Failed when calling handler to persist itself");

    // Save the query into the stream, coping correctly with the 
    // UNICODE / ANSI issue.  We will be passed an IPersistQuery object which
    // we must then thunk accordingly if we are UNICODE for the pages we
    // are going to talk to.

#ifndef UNICODE
    hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_ANSI, CQPM_PERSIST, FALSE, (LPARAM)pPersistQuery);
    FailGracefully(hres, "Failed to load form data");
#else
    hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_UNICODE, CQPM_PERSIST, FALSE, (LPARAM)pPersistQuery);
    FailGracefully(hres, "Failed to load page data (UNICODE)");

    if ( SUCCEEDED(hres) )
    {
        pPersistQueryA2W = new CPersistQueryA2W(pPersistQuery);
        TraceAssert(pPersistQueryA2W);

        if ( !pPersistQueryA2W )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to construct persistance thunk object");

        hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_ANSI, CQPM_PERSIST, FALSE, (LPARAM)pPersistQueryA2W);
        FailGracefully(hres, "Failed to load page data (ANSI)");
    }
#endif

    hres = S_OK;

exit_gracefully:

#ifdef UNICODE
    DoRelease(pPersistQueryA2W);
#endif

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::CallForm(THIS_ LPCLSID pclsidForm, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;
    LPQUERYFORM pQueryForm = _pCurrentForm;

    TraceEnter(TRACE_FRAME, "CQueryFrame::CallForm");
    
    if ( pclsidForm )
    {
        pQueryForm = FindQueryForm(*pclsidForm);
        TraceAssert(pQueryForm);
    }

    if ( !pQueryForm )
        ExitGracefully(hres, E_FAIL, "Failed to find query form for given CLSID");

    hres = CallFormPages(pQueryForm, CALLFORMPAGES_ALL, uMsg, wParam, lParam);
    FailGracefully(hres, "Failed when calling CallFormPages");

    // hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::GetScope(THIS_ LPCQSCOPE* ppScope)
{
    HRESULT hres;
    LPQUERYSCOPE pQueryScope;

    TraceEnter(TRACE_FRAME, "CQueryFrame::GetScope");

    if ( !ppScope )
        ExitGracefully(hres, E_INVALIDARG, "ppScope == NULL, thats bad");

    hres = GetSelectedScope(&pQueryScope);
    FailGracefully(hres, "Failed to get the current scope");

    *ppScope = (LPCQSCOPE)CoTaskMemAlloc(pQueryScope->pScope->cbStruct);
    TraceAssert(*ppScope);

    if ( !*ppScope )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate the scope block");
                        
    memcpy(*ppScope, pQueryScope->pScope, pQueryScope->pScope->cbStruct);

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueryFrame::GetHandler(THIS_ REFIID riid, void **ppv)
{
    HRESULT hres;

    TraceEnter(TRACE_FRAME, "CQueryFrame::GetHandler");

    if ( !_pQueryHandler )
        ExitGracefully(hres, E_UNEXPECTED, "_pQueryHandler is NULL");

    hres = _pQueryHandler->QueryInterface(riid, ppv);

exit_gracefully:

    TraceLeaveResult(hres);
}

/*-----------------------------------------------------------------------------
/ Dialog box handler functions (core guts)
/----------------------------------------------------------------------------*/

#define REAL_WINDOW(hwnd)                   \
        (hwnd &&                            \
            IsWindowVisible(hwnd) &&        \
                IsWindowEnabled(hwnd) &&    \
                    (GetWindowLong(hwnd, GWL_STYLE) & WS_TABSTOP))

HWND _NextTabStop(HWND hwndSearch, BOOL fShift)
{
    HWND hwnd;

    Trace(TEXT("hwndSearch %08x, fShift %d"), hwndSearch, fShift);

    // do we have a window to search into?
    
    while ( hwndSearch )
    {
        // if we have a window then lets check to see if it has any children?

        hwnd = GetWindow(hwndSearch, GW_CHILD);
        Trace(TEXT("Child of %08x is %08x"), hwndSearch, hwnd);

        if ( hwnd )
        {
            // it has a child therefore lets to go its first/last
            // and continue the search there for a window that
            // matches the criteria we are looking for.

            hwnd = GetWindow(hwnd, fShift ? GW_HWNDLAST:GW_HWNDFIRST);

            if ( !REAL_WINDOW(hwnd) )
            {
                Trace(TEXT("Trying to recurse into %08x"), hwnd);
                hwnd = _NextTabStop(hwnd, fShift);
            }

            Trace(TEXT("Tabstop child of %08x is %08x"), hwndSearch, hwnd);
        }

        // after all that is hwnd a valid window?  if so then pass
        // that back out to the caller.

        if ( REAL_WINDOW(hwnd) )
        {
            Trace(TEXT("Child tab stop was %08x"), hwnd);
            return hwnd;
        }

        // do we have a sibling?  if so then lets return that otherwise
        // lets just continue to search until we either run out of windows
        // or hit something interesting

        hwndSearch = GetWindow(hwndSearch, fShift ? GW_HWNDPREV:GW_HWNDNEXT);

        if ( REAL_WINDOW(hwndSearch) )
        {
            Trace(TEXT("Next tab stop was %08x"), hwndSearch);
            return hwndSearch;
        }
    }

    return hwndSearch;
}

INT QueryWnd_MessageProc(HWND hwnd, LPMSG pMsg)
{
    LRESULT lResult = 0;
    CQueryFrame* pQueryFrame = NULL;
    NMHDR nmhdr;

    pQueryFrame = (CQueryFrame*)GetWindowLongPtr(hwnd, DWLP_USER);

    if ( !pQueryFrame )
        return 0;

    if ( (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_TAB) )
    {
        BOOL fCtrl = GetAsyncKeyState(VK_CONTROL) < 0;
        BOOL fShift = GetAsyncKeyState(VK_SHIFT) < 0;

        // ensure that the focus rectangles are shown

#if (_WIN32_WINNT >= 0x0500)
        SendMessage(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);
#endif

        if ( fCtrl )
        {
            // if this is a key press within the parent then lets ensure that we
            // allow the tab control to change the page correctly.  otherwise lets
            // just hack around the problem of the result view not handling tabs
            // properly.

            INT iCur = TabCtrl_GetCurSel(pQueryFrame->_hwndFrame);
            INT nPages = TabCtrl_GetItemCount(pQueryFrame->_hwndFrame);

            if ( fShift )
                iCur += (nPages-1);
            else
                iCur++;

            pQueryFrame->SelectFormPage(pQueryFrame->_pCurrentForm, iCur % nPages);

            return 1;                   // we processed it
        }
        else
        {
            // is the window that has the focus a child of the result view, if
            // so then we must attempt to pass focus to its 1st child and hope
            // that is can do the rest.

            HWND hwndNext, hwnd = GetFocus();
            Trace(TEXT("Current focus window %08x"), hwnd);
           
            while ( hwnd && GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD )
            {      
                hwndNext = _NextTabStop(hwnd, fShift);
                Trace(TEXT("_NextTabStop yeilds %08x from %08x"), hwndNext, hwnd);
        
                if ( hwndNext )
                {
                    Trace(TEXT("SetFocus on child %08x"), hwndNext);
                    SetFocus(hwndNext);
                    return 1;
                }

                while ( TRUE )
                {
                    // look up the parent list trying to find a window that we can 
                    // tab back into.  We must watch that when we walk out of the
                    // child list we loop correctly at the top of the list.

                    hwndNext = GetParent(hwnd);
                    Trace(TEXT("Parent hwnd %08x"), hwndNext);

                    if ( GetWindowLong(hwndNext, GWL_STYLE) & WS_CHILD )
                    {
                        // the parent window is a child, therefore we can check
                        // to see if has any siblings.
                        
                        Trace(TEXT("hwndNext is a child, therefore hwndNext of it is %08x"), 
                                                        GetWindow(hwndNext, fShift ? GW_HWNDPREV:GW_HWNDNEXT));
                                                                                
                        if ( GetWindow(hwndNext, fShift ? GW_HWNDPREV:GW_HWNDNEXT) )
                        {
                            hwnd = GetWindow(hwndNext, fShift ? GW_HWNDPREV:GW_HWNDNEXT);
                            Trace(TEXT("Silbing window found %08x"), hwnd);
                            break;
                        }
                        else
                        {
                            TraceMsg("There was no sibling, therefore continuing parent loop");
                            hwnd = hwndNext;
                        }
                    }
                    else
                    {
                        // we have hit the parent window of it all (the overlapped one)
                        // therefore we must attempt to go to its first child.  Walk forward
                        // in the stack looking for a window that matches the
                        // "REAL_WINDOW" conditions.

                        hwnd = GetWindow(hwnd, fShift ? GW_HWNDLAST:GW_HWNDFIRST);
                        Trace(TEXT("First child is %08x"), hwnd);
                        break;                                  // continue the sibling search etc
                    }
                }

                if ( REAL_WINDOW(hwnd) )
                {
                    SetFocus(hwnd);
                    return 1;
                }
            }
        }
    }

    return 0;
}

//
// Main DLGPROC
//

INT_PTR CALLBACK QueryWnd_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CQueryFrame* pQueryFrame;

    if ( uMsg == WM_INITDIALOG )
    {
        HRESULT hres;
        pQueryFrame = (CQueryFrame*)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LRESULT)pQueryFrame);

        hres = pQueryFrame->OnInitDialog(hwnd);
        Trace(TEXT("OnInitDialog returns %08x"), hres);
        
        if ( FAILED(hres) )
        {
            TraceMsg("Failed to initialize the dialog, Destroying the window");    
            pQueryFrame->CloseQueryFrame(hres);
            DestroyWindow(hwnd);
        }
    }
    else
    {
        pQueryFrame = (CQueryFrame*)GetWindowLongPtr(hwnd, DWLP_USER);

        if ( !pQueryFrame )
            goto exit_gracefully;

        switch ( uMsg )
        {
            case WM_ERASEBKGND:
            {
                HDC hdc = (HDC)wParam;
                RECT rc;

                // if we have a DC then lets fill it, and if we have a 
                // query form then lets paint the divider between the menu bar and
                // this area.

                if ( hdc  )
                {
                    GetClientRect(hwnd, &rc);
                    FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE+1));

                    if ( !(pQueryFrame->_pOpenQueryWnd->dwFlags & OQWF_HIDEMENUS) )
                    {
#if HIDE_SEARCH_PANE
                        if ( !pQueryFrame->_fHideSearchPane )
                            DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP);
#else
                        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP);
#endif
                    }

                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 1L);
                }
                
                return 1;
            }

            case WM_NOTIFY:
                return pQueryFrame->OnNotify((int)wParam, (LPNMHDR)lParam);

            case WM_SIZE:
                pQueryFrame->OnSize(LOWORD(lParam), HIWORD(lParam));
                return(1);

            case WM_GETMINMAXINFO:
                pQueryFrame->OnGetMinMaxInfo((LPMINMAXINFO)lParam);
                return(1);

            case WM_COMMAND:
                pQueryFrame->OnCommand(wParam, lParam);
                return(1);

            case WM_ACTIVATE:
                pQueryFrame->_pQueryHandler->ActivateView(wParam ? CQRVA_ACTIVATE : CQRVA_DEACTIVATE, 0, 0);
                return(1);
            
            case WM_INITMENU:
                pQueryFrame->OnInitMenu((HMENU)wParam);
                return(1);

            case WM_SETCURSOR:
            {
                // do we have any scopes? if not then let us display the wait
                // cursor for the user.  if we have a query running then lets
                // display the app start cursor.

                if ( !pQueryFrame->_fAddScopesNYI &&
                            !ComboBox_GetCount(pQueryFrame->_hwndLookIn) )
                {
                    if ( LOWORD(lParam) == HTCLIENT )
                    {
                        SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 1L);
                        return 1;
                    }
                }

                break;
            }

            case WM_INITMENUPOPUP:
            {
                // only send sub-menu activates if the menu bar is being tracked, this is
                // handled within OnInitMenu, if we are not tracking the menu then we
                // assume that the client has already primed the menu and that they are
                // using some kind of popup menu.

                if ( pQueryFrame->_fTrackingMenuBar )
                    pQueryFrame->_pQueryHandler->ActivateView(CQRVA_INITMENUBARPOPUP, wParam, lParam);

                return(1);
            }
            
            case WM_ENTERMENULOOP:
                pQueryFrame->OnEnterMenuLoop(TRUE);
                return(1);

            case WM_EXITMENULOOP:
                pQueryFrame->OnEnterMenuLoop(FALSE);
                return(1);

            case WM_MENUSELECT:
            {
                UINT uID = LOWORD(wParam);
                UINT uFlags = HIWORD(wParam);
                HMENU hMenu = (HMENU)lParam;
                
                // the command opens a popup menu the the uID is actually
                // the index into the menu, so lets ensure that we pick
                // up the correct ID by calling GetMenuItemInfo, note that
                // GetMenuItemID returns -1 in this case which is totally
                // useless.

                if ( uFlags & MF_POPUP )    
                {
                    MENUITEMINFO mii;

                    ZeroMemory(&mii, SIZEOF(mii));
                    mii.cbSize = SIZEOF(mii);
                    mii.fMask = MIIM_ID;

                    if ( GetMenuItemInfo(hMenu, uID, TRUE, &mii) )
                        uID = mii.wID;
                }

                pQueryFrame->OnMenuSelect(hMenu, uID);
                return(1);
            }

            case WM_SYSCOMMAND:
                if ( wParam == SC_CLOSE )
                {
                    pQueryFrame->CloseQueryFrame(S_FALSE);
                    return(1);
                }
                break;

            case WM_CONTEXTMENU:
            {
                // there are a couple of controls we don't care about for the
                // frame, so lets ignore those when passing the CQRVA_CONTEXTMENU
                // through to the handler.

                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient((HWND)wParam, &pt);
                
                switch ( GetDlgCtrlID(ChildWindowFromPoint((HWND)wParam, pt)) )
                {
                    case IDC_FORMAREA:
                    case IDC_FINDANIMATION:
                    case IDC_STATUS:
                        return TRUE;                // handled

                    default:
                        pQueryFrame->_pQueryHandler->ActivateView(CQRVA_CONTEXTMENU, wParam, lParam);
                        return TRUE;
                }

                return FALSE;
            }
            
            case WM_HELP:
            {
                LPHELPINFO phi = (LPHELPINFO)lParam;

                // filter out those controls we are not interested in (they make no sense)
                // to bother the user with

                switch ( GetDlgCtrlID((HWND)phi->hItemHandle) )
                {
                    case IDC_FORMAREA:
                    case IDC_FINDANIMATION:
                    case IDC_STATUS:
                        return TRUE;

                    default:
                        pQueryFrame->OnHelp(phi);
                        return TRUE;
                }

                return FALSE;                   
            }

            case CQFWM_ADDSCOPE:
            {
                LPCQSCOPE pScope = (LPCQSCOPE)wParam;
                BOOL fSelect = LOWORD(lParam);
                INT iIndex = HIWORD(lParam);

                if ( SUCCEEDED(pQueryFrame->AddScope(pScope, iIndex, fSelect)) )
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 1L);

                return 1;
            }

            case CQFWM_GETFRAME:
            {
                IQueryFrame** ppQueryFrame = (IQueryFrame**)lParam;

                if ( ppQueryFrame )
                {
                    pQueryFrame->AddRef();
                    *ppQueryFrame = pQueryFrame;
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 1L);
                }

                return 1;
            }

            case CQFWM_ALLSCOPESADDED:
            {
                // there is an async scope collector, it has added all the scopes
                // so we must now attempt to issue the query if the we are in the
                // holding pattern waiting for the scopes to be collected.

                pQueryFrame->_fScopesAddedAsync = FALSE;            // all scopes have been added

                if ( pQueryFrame->_pOpenQueryWnd->dwFlags & OQWF_ISSUEONOPEN )
                    PostMessage(pQueryFrame->_hwnd, CQFWM_STARTQUERY, 0, 0);

                return 1;
            }

            case CQFWM_STARTQUERY:
                pQueryFrame->OnFindNow();
                return 1;

            default:
                break;
        }
    }

exit_gracefully:

    return(0);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::CloseQueryFrame
/ ----------------------------
/   Close the query window passing back the data object if required, and ensuring
/   that our result code indicates what is going on.
/
/ In:
/   hResult = result code to pass to the caller
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::CloseQueryFrame(HRESULT hres)
{
    TraceEnter(TRACE_FRAME, "CQueryFrame::CloseQueryFrame");
    Trace(TEXT("hResult %08x"), hres);

    // If we succeeded then attempt to collect the IDataObject and pass it
    // back to the caller.

    if ( hres == S_OK )
    {
        if ( _ppDataObject )
        {
            hres = _pQueryHandler->GetViewObject(CQRVS_SELECTION, IID_IDataObject, (LPVOID*)_ppDataObject);
            FailGracefully(hres, "Failed when collecting the data object");
        }

        if ( (_pOpenQueryWnd->dwFlags & OQWF_SAVEQUERYONOK) && _pOpenQueryWnd->pPersistQuery )
        {
            hres = SaveQuery(_pOpenQueryWnd->pPersistQuery);
            FailGracefully(hres, "Failed when persisting query to IPersistQuery blob");
        }

        hres = S_OK;           // success
    }

exit_gracefully:

    _hResult = hres;
    _fExitModalLoop = TRUE;                // bomb out of the modal loop

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::HideSearchPane
/ -------------------------
/   Hide the form area of the query window.  This involves hiding the controls
/   present and moving the contents of the windo to reflect the change we
/   are making.
/
/ In:
/   fHide = show/hide flag
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

#if HIDE_SEARCH_PANE

const UINT g_idControls[] =
{   
    CQID_BROWSE,
    CQID_FINDNOW,
    CQID_STOP,
    CQID_CLEARALL,
    IDC_FORMAREA,
    IDC_FINDANIMATION,
};

VOID CQueryFrame::HideSearchPane(BOOL fHide)
{
    RECT rect, rcClient;
    MINMAXINFO mmi;
    INT i, cx, cy, cxClient, cyClient;
    INT nCmdShow = fHide ? SW_HIDE:SW_SHOW;

    TraceEnter(TRACE_FRAME, "CQueryFrame::HideSearchPane");

    if ( _fHideSearchPane != fHide )
    {   
        _fHideSearchPane = fHide;

        LockWindowUpdate(_hwnd);

        // hide the window controls that we are not interested in,
        // taking special care for the optional controls on the window

        for ( i = 0 ; i < ARRAYSIZE(g_idControls); i++ )
            ShowWindow(GetDlgItem(_hwnd, g_idControls[i]), nCmdShow);
           
        if ( _pCurrentFormPage && _pCurrentFormPage->hwndPage )
            ShowWindow(_pCurrentFormPage->hwndPage, nCmdShow);

        if ( !(_pOpenQueryWnd->dwFlags & OQWF_REMOVEFORMS) )
        {
            ShowWindow(_hwndLookForLabel, nCmdShow);
            ShowWindow(_hwndLookFor, nCmdShow);
        }

        if ( !(_pOpenQueryWnd->dwFlags & OQWF_REMOVESCOPES) )
        {
            ShowWindow(_hwndLookInLabel, nCmdShow);
            ShowWindow(_hwndLookIn, nCmdShow);
        }

        if ( _pOpenQueryWnd->dwFlags & OQWF_OKCANCEL )
        {
            ShowWindow(_hwndOK, nCmdShow);

            if ( _hwndCancel )
                ShowWindow(_hwndCancel, nCmdShow);
        }

        // now adjust the main window area to cope with the
        // form being hidden or shown.

        GetClientRect(_hwnd, &rcClient);
        GetWindowRect(_hwnd, &rect);

        cxClient = rcClient.right - rcClient.left;
        cyClient = rcClient.bottom - rcClient.top;

        if ( _fHideSearchPane )
        {
            OnSize(cxClient, cyClient);
        }
        else
        {
            // when enabling the form area we must ensure that the window is
            // at least big enough to show it, therefore we need to get the
            // min track size, and apply that the current size.  If there is
            // no change then we don't bother calling SetWindowPos, we just
            // call OnSize and let that take care of verything.

            OnGetMinMaxInfo(&mmi);    

            cx = max(rect.right-rect.left, mmi.ptMinTrackSize.x);   // width
            cy = max(rect.bottom-rect.top, mmi.ptMinTrackSize.y);   // height

            if ( (cx == (rect.right - rect.left)) && (cy == (rect.bottom - rect.top)) )
            {
                TraceMsg("Calling OnSize, no physical size change to window");
                OnSize(cxClient, cyClient);
            }
            else
            {
                TraceMsg("Sizing window to make form visible");
                SetWindowPos(_hwnd, NULL, 0, 0, cx, cy, SWP_NOZORDER|SWP_NOMOVE);
            }
        }

        LockWindowUpdate(NULL);
        RedrawWindow(_hwnd, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_UPDATENOW|RDW_ALLCHILDREN);
    }

    DoEnableControls();             // ensure we fix the grey state of the controls

    TraceLeave();
}

#endif


/*-----------------------------------------------------------------------------
/ CQueryFrame::FrameMessageBox
/ ----------------------------
/   Our message box for putting up prompts that relate to the current
/   query.  We handle getting the view information and displaying
/   the prompt, returning the result from MessageBox.
/
/ In:
/   pPrompt = text displayed as a prompt
/   uType = message box type
/
/ Out:
/   INT
/----------------------------------------------------------------------------*/
INT CQueryFrame::FrameMessageBox(LPCTSTR pPrompt, UINT uType)
{
    TCHAR szTitle[MAX_PATH];        
    CQVIEWINFO vi;

    TraceEnter(TRACE_FRAME, "CQueryFrame::FrameMessageBox");

    ZeroMemory(&vi, SIZEOF(vi));
    //vi. dwFlags = 0;                // display attributes

    if ( SUCCEEDED(_pQueryHandler->GetViewInfo(&vi)) && vi.hInstance && vi.idTitle )
        LoadString(vi.hInstance, vi.idTitle, szTitle, ARRAYSIZE(szTitle));
    else
        GetWindowText(_hwnd, szTitle, ARRAYSIZE(szTitle));

    TraceLeaveValue(MessageBox(_hwnd, pPrompt, szTitle, uType));    
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnInitDlg
/ ----------------------
/   Handle a WM_INITDAILOG message, this is sent as the first thing the
/   dialog receives, therefore we must handle our initialization that
/   was not handled in the constructor.
/
/ In:
/   hwnd = handle of dialog we are initializing
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::OnInitDialog(HWND hwnd)
{
    HRESULT hres;
    HICON hIcon = NULL;
    TCHAR szGUID[GUIDSTR_MAX+1];
    TCHAR szBuffer[MAX_PATH];
    CQVIEWINFO vi;
    INT dyControls = 0;
    RECT rect, rect2;
    SIZE size;
    
    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnInitDialog");

    // get the HKEY for the handler we are using

    hres = GetKeyForCLSID(_pOpenQueryWnd->clsidHandler, NULL, &_hkHandler);
    FailGracefully(hres, "Failed to open handlers HKEY");

    // pick up the control handles and store them, saves picking them up later

    _hwnd              = hwnd;
    _hwndFrame         = GetDlgItem(hwnd, IDC_FORMAREA);
    _hwndLookForLabel  = GetDlgItem(hwnd, CQID_LOOKFORLABEL);
    _hwndLookFor       = GetDlgItem(hwnd, CQID_LOOKFOR); 
    _hwndLookInLabel   = GetDlgItem(hwnd, CQID_LOOKINLABEL);
    _hwndLookIn        = GetDlgItem(hwnd, CQID_LOOKIN);    
    _hwndBrowse        = GetDlgItem(hwnd, CQID_BROWSE);
    _hwndFindNow       = GetDlgItem(hwnd, CQID_FINDNOW);
    _hwndStop          = GetDlgItem(hwnd, CQID_STOP);
    _hwndNewQuery      = GetDlgItem(hwnd, CQID_CLEARALL);
    _hwndFindAnimation = GetDlgItem(hwnd, IDC_FINDANIMATION);
    _hwndOK            = GetDlgItem(hwnd, IDOK);
    _hwndCancel        = GetDlgItem(hwnd, IDCANCEL);

    // call the IQueryHandler interface and get its display attributes,
    // then reflect these into the dialog we are about to display to the
    // outside world.

    vi.dwFlags = 0;
    vi.hInstance = NULL;
    vi.idLargeIcon = 0;
    vi.idSmallIcon = 0;
    vi.idTitle = 0;
    vi.idAnimation = 0;

    hres = _pQueryHandler->GetViewInfo(&vi);
    FailGracefully(hres, "Failed when getting the view info from the handler");

    _dwHandlerViewFlags = vi.dwFlags;

    if ( vi.hInstance )
    {
        HICON hiTemp = NULL;

        if ( vi.idLargeIcon )
        {
            _hiconLarge = (HICON)LoadImage(vi.hInstance, 
                                           MAKEINTRESOURCE(vi.idLargeIcon), 
                                           IMAGE_ICON,
                                           0, 0, 
                                           LR_DEFAULTCOLOR|LR_DEFAULTSIZE);
            if ( _hiconLarge )
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)_hiconLarge);
        }

        if ( vi.idSmallIcon )
        {
            _hiconSmall = (HICON)LoadImage(vi.hInstance, 
                                           MAKEINTRESOURCE(vi.idLargeIcon), 
                                           IMAGE_ICON,
                                           GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 
                                           LR_DEFAULTCOLOR);
            if ( _hiconSmall )
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)_hiconSmall);
        }

        if ( vi.idTitle )
        {
            LoadString(vi.hInstance, vi.idTitle, szBuffer, ARRAYSIZE(szBuffer));
            SetWindowText(hwnd, szBuffer);
        }
    }

    if ( vi.hInstance && vi.idAnimation )
    {
        SetWindowLongPtr(_hwndFindAnimation, GWLP_HINSTANCE, (LRESULT)vi.hInstance);
        Animate_Open(_hwndFindAnimation, MAKEINTRESOURCE(vi.idAnimation));
    }
    else
    {
        Animate_Open(_hwndFindAnimation, MAKEINTRESOURCE(AVI_FIND));
    }

    // now adjust the positions and hide the controls we are not interested in

    if ( _pOpenQueryWnd->dwFlags & OQWF_REMOVEFORMS )
    {
        ShowWindow(_hwndLookForLabel, SW_HIDE);
        ShowWindow(_hwndLookFor, SW_HIDE);
    }

    if ( _pOpenQueryWnd->dwFlags & OQWF_REMOVESCOPES )
    {
        ShowWindow(_hwndLookInLabel, SW_HIDE);
        ShowWindow(_hwndLookIn, SW_HIDE);
        ShowWindow(_hwndBrowse, SW_HIDE);
    }

    // hiding both the scopes and the forms control causes us to
    // move all the controls up by so many units.  

    if ( (_pOpenQueryWnd->dwFlags & (OQWF_REMOVEFORMS|OQWF_REMOVESCOPES)) 
                                        == (OQWF_REMOVEFORMS|OQWF_REMOVESCOPES) )
    {
        GetRealWindowInfo(_hwndLookForLabel, &rect, NULL);
        GetRealWindowInfo(_hwndFrame, &rect2, NULL);

        dyControls += rect2.top - rect.top;         
        Trace(TEXT("Moving all controls up by %d units"), dyControls);

        OffsetWindow(_hwndFrame, 0, -dyControls);
        OffsetWindow(_hwndFindNow, 0, -dyControls);
        OffsetWindow(_hwndStop, 0, -dyControls);
        OffsetWindow(_hwndNewQuery, 0, -dyControls);
        OffsetWindow(_hwndFindAnimation, 0, -dyControls);
        OffsetWindow(_hwndOK, 0, -dyControls);

        if ( _hwndCancel )
            OffsetWindow(_hwndCancel, 0, -dyControls);
    }

    // hiding OK/Cancel so lets adjust the size here to include the
    // OK/Cancel buttons disappearing, note that we update dyControls
    // to include this delta

    if ( !(_pOpenQueryWnd->dwFlags & OQWF_OKCANCEL) )
    {
        ShowWindow(_hwndOK, SW_HIDE);        

        if ( _hwndCancel )
            ShowWindow(_hwndCancel, SW_HIDE);

        // if this is the filter dialog then lets ensure that 
        // we trim the OK/Cancel buttons from the size by adjusting the 
        // dyControls further.

        GetRealWindowInfo(_hwndOK, &rect, NULL);
        GetRealWindowInfo(_hwndFrame, &rect2, NULL);
        dyControls += rect.bottom - rect2.bottom;
    }

    // having performed that extra bit of initialization lets cache the
    // positions of the various controls, to make sizing more fun...

    GetClientRect(hwnd, &rect2);
    rect2.bottom -= dyControls;

    _dyResultsTop = rect2.bottom;    

    GetRealWindowInfo(hwnd, NULL, &size);
    GetRealWindowInfo(_hwndFrame, &rect, &_szForm);

    Trace(TEXT("dyControls %d"), dyControls);
    size.cy -= dyControls;

    _dxFormAreaLeft = rect.left;
    _dxFormAreaRight = rect2.right - rect.right;
    
    _szMinTrack.cx = size.cx - _szForm.cx;
    _szMinTrack.cy = size.cy - _szForm.cy;

    if ( !(_pOpenQueryWnd->dwFlags & OQWF_HIDEMENUS) )
    {
        TraceMsg("Adjusting _szMinTrack.cy to account for menu bar");
        _szMinTrack.cy += GetSystemMetrics(SM_CYMENU);
    }
    
    GetRealWindowInfo(_hwndBrowse, &rect, NULL);
    _dxButtonsLeft = rect2.right - rect.left;

    GetRealWindowInfo(_hwndLookIn, &rect, NULL);
    _dxGap = (rect2.right - rect.right) - _dxButtonsLeft;

    GetRealWindowInfo(_hwndFindAnimation, &rect, NULL);
    _dxAnimationLeft = rect2.right - rect.left;

    GetRealWindowInfo(_hwndOK, &rect, NULL);
    _dyOKTop = rect2.bottom - rect.top;
    _dyGap = size.cy - rect.bottom;

    // Now collect the forms and pages, then walk them building the size
    // information that we need.

    hres = GatherForms();
    FailGracefully(hres, "Failed to init form list");

    _szMinTrack.cx += _szForm.cx;
    _szMinTrack.cy += _szForm.cy;

    // Populate the scope control by querying the handler for them,
    // if there are none then we display a suitable message box and
    // let the user know that something went wrong.

    hres = PopulateScopeControl();
    FailGracefully(hres, "Failed to init scope list");

    _fScopesPopulated = TRUE;                              // scope control now populated

    // perform final fix up of the window, ensure that we size it so that
    // the entire form and buttons are visible.  Then set ourselves into the
    // no query state and reset the animation.

    SetWindowPos(hwnd, 
                 NULL,
                 0, 0,
                 _szMinTrack.cx, _szMinTrack.cy,
                 SWP_NOMOVE|SWP_NOZORDER);


    if ( _pOpenQueryWnd->dwFlags & OQWF_HIDEMENUS )
        ::SetMenu(hwnd, NULL);

    hres = PopulateFormControl(_pOpenQueryWnd->dwFlags & OQWF_SHOWOPTIONAL);
    FailGracefully(hres, "Failed to populate form control");

    // Now load the query which inturn selects the form that we should be using,
    // if there is no query to load then either use the default form or
    // the first in the list.

    if ( (_pOpenQueryWnd->dwFlags & OQWF_LOADQUERY) && _pOpenQueryWnd->pPersistQuery ) 
    {
        hres = LoadQuery(_pOpenQueryWnd->pPersistQuery);
        FailGracefully(hres, "Failed when to load query from supplied IPersistQuery");
    }
    else
    {
        if ( _pOpenQueryWnd->dwFlags & OQWF_DEFAULTFORM )
        {
            SelectForm(_pOpenQueryWnd->clsidDefaultForm);

            if ( !_pCurrentForm )
                ExitGracefully(hres, E_FAIL, "Failed to select the query form");                
        }
        else
        {
            INT iForm = (int)ComboBox_GetItemData(_hwndLookFor, 0);
            LPQUERYFORM pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, iForm);
            TraceAssert(pQueryForm);

            SelectForm(pQueryForm->clsidForm);
        }
    }

    StartQuery(FALSE);
    
    if ( _pCurrentFormPage )
    {
        Trace(TEXT("Setting focus to the form page (%08x)"), _pCurrentFormPage->hwndPage);
        SetFocus(_pCurrentFormPage->hwndPage);
    }

#if HIDE_SEARCH_PANE
    // If we need to be issuing the query on open then lets ensure that
    // we have some scopes, if we do then we can just post ourselves a
    // WM_COMMAND with the relevant ID (IDC_FINDNOW).

    if ( _pOpenQueryWnd->dwFlags & OQWF_ISSUEONOPEN ) 
    {
        if ( _pOpenQueryWnd->dwFlags & OQWF_HIDESEARCHPANE )
            HideSearchPane(TRUE);

        PostMessage(_hwnd, CQFWM_STARTQUERY, 0, 0);
    }
#else
    // issue on open, therefore lets get the query going, if there is async
    // scope collection then the query will be issued by the bg thread.

    if ( _pOpenQueryWnd->dwFlags & OQWF_ISSUEONOPEN )
        PostMessage(_hwnd, CQFWM_STARTQUERY, 0, 0);
#endif

    SetForegroundWindow(hwnd);

    hres = S_OK;                          // success

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::EnableControls
/ ---------------------------
/   Set the controls into their enabled/disabled state based on the
/   state of the dialog.
/
/ In:
/   -
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
VOID CQueryFrame::DoEnableControls(VOID)
{   
    BOOL fScopes = (_fAddScopesNYI || ComboBox_GetCount(_hwndLookIn));
#if HIDE_SEARCH_PANE
    BOOL fEnable = fScopes && !_fHideSearchPane;
#else
    BOOL fEnable = fScopes;
#endif
    UINT uEnable = fScopes ? MF_ENABLED:MF_GRAYED;
    HMENU hMenu = GetMenu(_hwnd);
    INT i;

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::DoEnableControls");

    EnableWindow(_hwndFindNow, !_fQueryRunning && fEnable);
    EnableWindow(_hwndStop, _fQueryRunning && fEnable);
    EnableWindow(_hwndNewQuery, fEnable);

    EnableWindow(_hwndLookFor, !_fQueryRunning && fEnable);
    EnableWindow(_hwndLookIn, !_fQueryRunning && fEnable);
    EnableWindow(_hwndBrowse, !_fQueryRunning && fEnable);

    if ( _pCurrentForm )
        CallFormPages(_pCurrentForm, CALLFORMPAGES_ALL, CQPM_ENABLE, (BOOL)(!_fQueryRunning && fEnable), 0);

    if ( _hwndOK )
        EnableWindow(_hwndOK, !_fQueryRunning && fEnable);
    if ( _hwndCancel )
        EnableWindow(_hwndCancel, !_fQueryRunning && fEnable);

    for ( i = 0 ; i < GetMenuItemCount(hMenu) ; i++ )
        EnableMenuItem(hMenu, i, MF_BYPOSITION|uEnable);

    DrawMenuBar(_hwnd);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnNotify
/ ---------------------
/   Notify event received, decode it and handle accordingly
/
/ In:
/   idCtrl = ID of control issuing notify
/   pNotify -> LPNMHDR structure
/
/ Out:
/   LRESULT
/----------------------------------------------------------------------------*/
LRESULT CQueryFrame::OnNotify(INT idCtrl, LPNMHDR pNotify)
{
    LRESULT lr = 0;

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnNotify");

    // TCN_SELCHANGE used to indicate that the currently active
    // tab has been changed

    if ( pNotify->code == TCN_SELCHANGE )
    {
        INT iPage = TabCtrl_GetCurSel(_hwndFrame);
        TraceAssert(iPage >= 0);

        if ( iPage >= 0 )
        {
            SelectFormPage(_pCurrentForm, iPage);
            lr = 0;
        }
    }

    TraceLeaveResult((HRESULT)lr);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnSize
/ -------------------
/   The window is being sized and we received a WM_SIZE, therefore move 
/   the content of the window about.
/
/ In:
/   cx = new width
/   cy = new height
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::OnSize(INT cx, INT cy)
{
    HDWP hdwp;
    RECT rect, rect2;
    SIZE sz, sz2;
    INT x, cxForm, cyForm;
    INT dyResultsTop = 0;

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnSize");

    // do as much as we can within a DefWindowPos to aVOID too
    // much flicker.

    hdwp = BeginDeferWindowPos(16);

    if ( hdwp )
    {
        {
            // adjust the look for controls, if there is no scope then 
            // stretch the look for control over the entire client area
            // of the window.

            if ( !(_pOpenQueryWnd->dwFlags & OQWF_REMOVEFORMS) )
            {
                if ( _pOpenQueryWnd->dwFlags & OQWF_REMOVESCOPES )
                {
                    GetRealWindowInfo(_hwndLookFor, &rect, &sz);
                    hdwp = DeferWindowPos(hdwp, _hwndLookFor, NULL,
                                       0, 0, 
                                       (cx - _dxFormAreaRight) - rect.left, sz.cy,
                                       SWP_NOZORDER|SWP_NOMOVE);
                }
            }

            // adjust the "look in" controls, if there is a form control
            // then stretch across the remaining space, otherwise move the
            // label and stretch the scope over the remaining space.
        
            if ( !(_pOpenQueryWnd->dwFlags & OQWF_REMOVESCOPES) )
            {
                INT xScopeRight;

                GetRealWindowInfo(_hwndLookIn, &rect, &sz);
                xScopeRight = cx - _dxFormAreaRight - _dxGap;
                             
                if ( _pOpenQueryWnd->dwFlags & OQWF_HIDESEARCHUI )
                {
                    // 
                    // when hiding the search UI, then adjust the button position to account for the
                    // right edge of the dialog not having buttons.
                    //

                    xScopeRight -= (_dxButtonsLeft - _dxFormAreaRight) + _dxGap;
                }
                
                if ( _pOpenQueryWnd->dwFlags & OQWF_REMOVEFORMS )
                {
                    GetRealWindowInfo(_hwndLookInLabel, &rect2, &sz2);
                    hdwp = DeferWindowPos(hdwp, _hwndLookInLabel, NULL,
                                          _dxFormAreaLeft, rect2.top, 
                                          0, 0,
                                          SWP_NOSIZE|SWP_NOZORDER);

                    hdwp = DeferWindowPos(hdwp, _hwndLookIn, NULL,
                                          _dxFormAreaLeft+sz2.cx, rect.top, 
                                          xScopeRight - (_dxFormAreaLeft + sz2.cx), sz.cy,
                                          SWP_NOZORDER);
                }
                else
                {
                    hdwp = DeferWindowPos(hdwp, _hwndLookIn, NULL,
                                          0, 0, 
                                          xScopeRight - rect.left, sz.cy,
                                          SWP_NOZORDER|SWP_NOMOVE);
                }

                // browse control is displayed always if we are showing the 
                // scopes.

                GetRealWindowInfo(_hwndBrowse, &rect, NULL);
                hdwp = DeferWindowPos(hdwp, _hwndBrowse, NULL,
                                      xScopeRight+_dxGap, rect.top,
                                      0, 0,
                                      SWP_NOZORDER|SWP_NOSIZE);
            }
                    
            // all the buttons have a fixed offset from the right edege
            // of the dialog, so just handle that as we can.
            
            if ( !(_pOpenQueryWnd->dwFlags & OQWF_HIDESEARCHUI) )
            {
                GetRealWindowInfo(_hwndFindNow, &rect, NULL);
                hdwp = DeferWindowPos(hdwp, _hwndFindNow, NULL,
                                     (cx - _dxButtonsLeft), rect.top, 
                                     0, 0,
                                     SWP_NOZORDER|SWP_NOSIZE);

                GetRealWindowInfo(_hwndStop, &rect, &sz);
                hdwp = DeferWindowPos(hdwp, _hwndStop, NULL,
                                      (cx - _dxButtonsLeft), rect.top, 
                                      0, 0,
                                      SWP_NOZORDER|SWP_NOSIZE);

                GetRealWindowInfo(_hwndNewQuery, &rect, NULL);
                hdwp = DeferWindowPos(hdwp, _hwndNewQuery, NULL,
                                      (cx - _dxButtonsLeft), rect.top, 
                                      0, 0,
                                      SWP_NOZORDER|SWP_NOSIZE);
 
                GetRealWindowInfo(_hwndFindAnimation, &rect2, &sz2);
                hdwp = DeferWindowPos(hdwp, _hwndFindAnimation, NULL,
                                     (cx - _dxAnimationLeft), rect2.top, 
                                     0, 0,
                                     SWP_NOZORDER|SWP_NOSIZE);
            }

            // position the form "frame" control
        
            GetRealWindowInfo(_hwndFrame, &rect, &sz);
            cxForm = (cx - _dxFormAreaRight) - rect.left;

            hdwp = DeferWindowPos(hdwp, _hwndFrame, NULL,
                                  0, 0, 
                                  cxForm, _szForm.cy,
                                  SWP_NOZORDER|SWP_NOMOVE);

            dyResultsTop = _dyResultsTop;
            
            // when we have a cancel button then ensure that it is to the right
            // of the OK button.

            if ( _hwndCancel )
            {
                GetRealWindowInfo(_hwndCancel, &rect, &sz);
                hdwp = DeferWindowPos(hdwp, _hwndCancel, NULL,
                                      (cx - _dxButtonsLeft), dyResultsTop - _dyOKTop,
                                      0, 0,    
                                      SWP_NOZORDER|SWP_NOSIZE);

                GetRealWindowInfo(_hwndOK, &rect, &sz);
                hdwp = DeferWindowPos(hdwp, _hwndOK, NULL,
                                      (cx - _dxButtonsLeft - _dxGap - sz.cx), dyResultsTop - _dyOKTop,
                                      0, 0,    
                                      SWP_NOZORDER|SWP_NOSIZE);
            }
            else
            {
                GetRealWindowInfo(_hwndOK, &rect, &sz);
                hdwp = DeferWindowPos(hdwp, _hwndOK, NULL,
                                      (cx - _dxButtonsLeft), dyResultsTop - _dyOKTop,
                                      0, 0,    
                                      SWP_NOZORDER|SWP_NOSIZE);
            }                                                                
        }

        // move the results and status bar as required

        if ( _hwndResults )
        {
            hdwp = DeferWindowPos(hdwp, _hwndStatus, NULL,
                                  0, cy - _cyStatus,
                                  cx, _cyStatus,
                                  SWP_SHOWWINDOW|SWP_NOZORDER);

            hdwp = DeferWindowPos(hdwp, _hwndResults, NULL,
                                  0, dyResultsTop, 
                                  cx, max(0, cy - (dyResultsTop + _cyStatus)),
                                  SWP_SHOWWINDOW|SWP_NOZORDER);
        }

        EndDeferWindowPos(hdwp);

        // here is the strange bit, by this point we have moved & sized all the
        // controls on the dialog except the current page, as this is a child window
        // and not a control which in turn has controls doing this would break
        // the DefWindowPos path, therefore having updated everybody, lets update
        // the page.

#if HIDE_SEARCH_PANE
        if ( !_fHideSearchPane && _pCurrentFormPage && _pCurrentFormPage->hwndPage )
#else
        if ( _pCurrentFormPage && _pCurrentFormPage->hwndPage )
#endif
        {
            GetRealWindowInfo(_hwndFrame, &rect, NULL);
            TabCtrl_AdjustRect(_hwndFrame, FALSE, &rect);

            cxForm = rect.right - rect.left;
            cyForm = rect.bottom - rect.top;

            SetWindowPos(_pCurrentFormPage->hwndPage, NULL,
                         rect.left, rect.top, cxForm, cyForm,
                         SWP_NOZORDER);
        }
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnGetMinMaxInfo
/ ----------------------------
/   The window is being sized and we received a WM_SIZE, therefore move 
/   the content of the window about.
/
/ In:
/   lpmmin -> MINMAXINFO structure
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::OnGetMinMaxInfo(LPMINMAXINFO lpmmi)
{
    RECT rect = {0, 0, 0, 0};

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnGetMinMaxInfo");

#if 0
    if ( !_fHideSearchPane )
#endif
    {
        lpmmi->ptMinTrackSize.x = _szMinTrack.cx;
        lpmmi->ptMinTrackSize.y = _szMinTrack.cy;

        if ( !_hwndResults )
        {
            lpmmi->ptMaxSize.y = lpmmi->ptMinTrackSize.y;
            lpmmi->ptMaxTrackSize.y = lpmmi->ptMinTrackSize.y;
        }
    }
#if 0
    else
    {
        AdjustWindowRect(&rect, GetWindowLong(_hwnd, GWL_STYLE), (NULL != GetMenu(_hwnd)));
        lpmmi->ptMinTrackSize.y = rect.bottom - rect.top;
    }
#endif

    if ( _hwndResults && _hwndStatus )
        lpmmi->ptMinTrackSize.y += _cyStatus;

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnCommand
/ ----------------------
/   We have recieved a WM_COMMAND so process it accordingly.
/
/ In:
/   wParam, lParam = parameters from the message    
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;
    UINT uID = LOWORD(wParam);
    UINT uNotify = HIWORD(wParam); 
    HWND hwndControl = (HWND)lParam;
    INT i;
    
    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnCommand");
    Trace(TEXT("uID %08x, uNotify %d, hwndControl %08x"), uID, uNotify, hwndControl);

    switch ( uID )
    {
        case IDOK:
            TraceMsg("IDOK received");
            CloseQueryFrame(S_OK);
            break;

        case IDCANCEL:
            TraceMsg("IDCANCEL received");
            CloseQueryFrame(S_FALSE);
            break;

        case CQID_LOOKFOR:
        {
            if ( uNotify == CBN_SELCHANGE )
            {
                INT iSel = ComboBox_GetCurSel(_hwndLookFor);
                INT iForm = (int)ComboBox_GetItemData(_hwndLookFor, iSel);
                LPQUERYFORM pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, iForm);
                TraceAssert(pQueryForm);

                if ( S_FALSE == SelectForm(pQueryForm->clsidForm) )
                {
                    TraceMsg("SelectForm return S_FALSE, so the user doesn't want the new form");
                    PostMessage(_hwndLookFor, CB_SETCURSEL, (WPARAM)_pCurrentForm->iForm, 0);
                }
                    
            }

            break;
        }

        case CQID_BROWSE:
            OnBrowse();
            break;

        case CQID_FINDNOW:
            OnFindNow();
            break;

        case CQID_STOP:
        {
            LONG style;

            _pQueryHandler->StopQuery();
            // For some reason, the standard method of getting the old
            // def button used in SetDefButton() below isn't working,
            // so we have to forcibly remove the BS_DEFPUSHBUTTON style
            // from the CQID_STOP button.
            style = GetWindowLong(_hwndStop, GWL_STYLE) & ~BS_DEFPUSHBUTTON;
            SendMessage(_hwndStop, 
                        BM_SETSTYLE, 
                        MAKEWPARAM(style, 0), 
                        MAKELPARAM(TRUE, 0));
            SetDefButton(_hwnd, CQID_FINDNOW);
            SetFocus(_hwndFindNow);
            break;
        }

        case CQID_CLEARALL:
            OnNewQuery(TRUE);                        // discard the current query
            break;

#if HIDE_SEARCH_PANE
        case CQID_VIEW_SEARCHPANE:
            HideSearchPane(!_fHideSearchPane);
            break;
#endif

        case CQID_FILE_CLOSE:
            TraceMsg("CQID_FILE_CLOSE received");
            CloseQueryFrame(S_FALSE);
            break;

        default:
            _pQueryHandler->InvokeCommand(_hwnd, uID);
            break;
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnInitMenu
/ -----------------------
/   Handle telling the handler that the menu is being initialised, however
/   this should only happen if the menu being activated is the
/   menu bar, otherwise we assume that the caller is tracking a popup
/   menu and has performed the required initalization.
/
/ In:
/   wParam, lParam = parameters from the WM_INITMENU
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::OnInitMenu(HMENU hMenu)
{
    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnInitMenu");

    _fTrackingMenuBar = (GetMenu(_hwnd) == hMenu);

    if ( _fTrackingMenuBar )
    {
        TraceMsg("Tracking the menu bar, sending activate");

        _pQueryHandler->ActivateView(CQRVA_INITMENUBAR, (WPARAM)hMenu, 0L);

        EnableMenuItem(hMenu, CQID_VIEW_SEARCHPANE, 
                                MF_BYCOMMAND|(_hwndResults != NULL) ? MF_ENABLED:MF_GRAYED);
#if HIDE_SEARCH_PANE
        CheckMenuItem(hMenu, CQID_VIEW_SEARCHPANE, 
                                MF_BYCOMMAND|(_fHideSearchPane) ? MF_UNCHECKED:MF_CHECKED);
#endif
    }
        
    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnEnterMenuLoop
/ ----------------------------
/   When the user displays a menu we must reflect this into the status bar
/   so that we can give the user help text relating to the commands they 
/   select.
/
/ In:
/   fEntering = entering the menu loop, or leaving.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::OnEnterMenuLoop(BOOL fEntering)
{
    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnEnterMenuLoop");

    if ( _hwndStatus )
    {
        if ( fEntering )
        {
            SendMessage(_hwndStatus, SB_SIMPLE, (WPARAM)TRUE, 0L);
            SendMessage(_hwndStatus, SB_SETTEXT, (WPARAM)SBT_NOBORDERS|255, 0L);
        }
        else
        {
            SendMessage(_hwndStatus, SB_SIMPLE, (WPARAM)FALSE, 0L);
        }
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnMenuSelect
/ -------------------------
/   Get the status text for this menu item and display it to the user,
/   if this doesn't map to any particular command then NULL out
/   the string.  At this point we also trap our commands.
/
/ In:
/   hMenu = menu the user is on
/   uID = command ID for that item
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::OnMenuSelect(HMENU hMenu, UINT uID)
{
    TCHAR szBuffer[MAX_PATH] = { TEXT('\0') };

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnMenuSelect");
    Trace(TEXT("hMenu %08x, uID %08x"), hMenu, uID);
        
    if ( _hwndStatus )
    {
        switch ( uID )
        {
            case CQID_FILE_CLOSE:
            case CQID_VIEW_SEARCHPANE:
                LoadString(GLOBAL_HINSTANCE, uID, szBuffer, ARRAYSIZE(szBuffer));
                break;

            default:
                _pQueryHandler->GetCommandString(uID, 0x0, szBuffer, ARRAYSIZE(szBuffer));
                break;
        }

        Trace(TEXT("Setting status bar to: %s"), szBuffer);
        SendMessage(_hwndStatus, SB_SETTEXT, (WPARAM)SBT_NOBORDERS|255, (LPARAM)szBuffer);
    }    

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnFindNow
/ ----------------------
//  Issue the query, resulting in a view window being created and then issuing
//  the parameter block to the query client.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::OnFindNow(VOID)
{
    HRESULT hres;
    CQPARAMS qp = { 0 };
    LPQUERYSCOPE pQueryScope = NULL;
    TCHAR szBuffer[MAX_PATH];
    BOOL fFixSize = TRUE;
    RECT rc;
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnFindNow");
    TraceAssert(_pCurrentForm != NULL);

    if ( _fQueryRunning )
        ExitGracefully(hres, E_FAIL, "Quyery is already running");

    SetWaitCursor();

    // If we have not created the viewer before now lets do so, also at the
    // same time we attempt to fix the window size to ensure that enough
    // of the view is visible.

    if ( !_hwndResults )
    {
        if ( !_hwndStatus )
        {
            _hwndStatus = CreateStatusWindow(WS_CHILD, NULL, _hwnd, IDC_STATUS);
            GetClientRect(_hwndStatus, &rc);
            _cyStatus = rc.bottom - rc.top;
        }

        // Now construct the result viewer for us to use
  
        hres = _pQueryHandler->CreateResultView(_hwnd, &_hwndResults);
        FailGracefully(hres, "Failed when creating the view object");
    
        GetWindowRect(_hwnd, &rc);
        SetWindowPos(_hwnd, NULL,
                     0, 0,
                     rc.right - rc.left, 
                     _szMinTrack.cy + VIEWER_DEFAULT_CY,
                     SWP_NOZORDER|SWP_NOMOVE);        
    }

    // are we still collecting the scopes async?  If so then lets wait until
    // they have all arrived before we set the UI running.

    if ( _hdsaScopes && DSA_GetItemCount(_hdsaScopes) )
    {         
        // Collect the parameters ready for starting the query, if this fails then
        // there is no point us continuing.

        ZeroMemory(&qp, SIZEOF(qp));
        qp.cbStruct = SIZEOF(qp);
        //qp.dwFlags = 0x0;
        qp.clsidForm = _pCurrentForm->clsidForm;           // new NT5 beta 2

        hres = GetSelectedScope(&pQueryScope);
        FailGracefully(hres, "Failed to get the scope from the LookIn control");

        if ( pQueryScope )
        {
            Trace(TEXT("pQueryScope %08x"), pQueryScope);
            qp.pQueryScope = pQueryScope->pScope;
        }

        hres = CallFormPages(_pCurrentForm, CALLFORMPAGES_ALL, CQPM_GETPARAMETERS, 0, (LPARAM)&qp.pQueryParameters);
        FailGracefully(hres, "Failed when collecting parameters from form");

        if ( !qp.pQueryParameters )
        {
            LoadString(GLOBAL_HINSTANCE, IDS_ERR_NOPARAMS, szBuffer, ARRAYSIZE(szBuffer));
            FrameMessageBox(szBuffer, MB_ICONERROR|MB_OK);
            ExitGracefully(hres, E_FAIL, "Failed to issue the query, no parameters");
        }

        // We either already had a view, or have just created one.  Either way
        // we must now prepare the query for sending.

        Trace(TEXT("qp.cbStruct %08x"), qp.cbStruct);
        Trace(TEXT("qp.dwFlags %08x"), qp.dwFlags);
        Trace(TEXT("qp.pQueryScope %08x"), qp.pQueryScope);
        Trace(TEXT("qp.pQueryParameters %08x"), qp.pQueryParameters);
        TraceGUID("qp.clsidForm: ", qp.clsidForm);

        hres = _pQueryHandler->IssueQuery(&qp);
        FailGracefully(hres, "Failed in IssueQuery");
    }
    else
    {
        // set the status text to reflect that we are initializng, otherwise it is
        // left empty and looks like we have crashed.

        if ( LoadString(GLOBAL_HINSTANCE, IDS_INITIALIZING, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            SetStatusText(szBuffer);
        }
    }

    hres = S_OK;               // success

exit_gracefully:

#if HIDE_SEARCH_PANE
    if ( FAILED(hres) && !_hwndResults && _fHideSearchPane )
    {
        TraceMsg("Form area hidden, no results viewer created so now showing it");
        HideSearchPane(FALSE);
    }
#endif

    if ( qp.pQueryParameters )
        CoTaskMemFree(qp.pQueryParameters);

    ResetWaitCursor();

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnNewQuery
/ -----------------------
/   Discard the current query, prompting the user as requierd.
/
/ In:
/   fAlwaysPrompt = TRUE if we force prompting of the user
/
/ Out:
/   BOOL
/----------------------------------------------------------------------------*/
BOOL CQueryFrame::OnNewQuery(BOOL fAlwaysPrompt)
{
    BOOL fQueryCleared = TRUE;
    TCHAR szBuffer[MAX_PATH];
    RECT rc;

    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnNewQuery");

    if ( _hwndResults || fAlwaysPrompt )
    {
        LoadString(GLOBAL_HINSTANCE, IDS_CLEARCURRENT, szBuffer, ARRAYSIZE(szBuffer));
        if ( IDOK != FrameMessageBox(szBuffer, MB_ICONINFORMATION|MB_OKCANCEL) )
            ExitGracefully(fQueryCleared, FALSE, "Used cancled new query");

        if ( _pQueryHandler )
            _pQueryHandler->StopQuery();

        CallFormPages(_pCurrentForm, CALLFORMPAGES_ALL, CQPM_CLEARFORM, 0, 0);

        if ( _hwndResults )
        {
            DestroyWindow(_hwndResults);           // no result view now
            _hwndResults = NULL;

            DestroyWindow(_hwndStatus);            // no status bar
            _hwndStatus = NULL;

            GetWindowRect(_hwnd, &rc);             // shrink the window
            SetWindowPos(_hwnd, NULL,
                         0, 0, rc.right - rc.left, _szMinTrack.cy,         
                         SWP_NOZORDER|SWP_NOMOVE);
        }
    }

exit_gracefully:

    TraceLeaveValue(fQueryCleared);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnBrowse
/ ---------------------
/   Browse for a new scope, adding it to the list if not already present,
/   or selecting the previous scope.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::OnBrowse(VOID)
{
    HRESULT hres;
    LPQUERYSCOPE pQueryScope = NULL;
    LPCQSCOPE pScope = NULL;
    
    TraceEnter(TRACE_FRAMEDLG, "CQueryFrame::OnBrowse");

    // Call the handler and get a scope allocation back, then add it to the list
    // of scopes to be displayed.

    hres = GetSelectedScope(&pQueryScope);
    FailGracefully(hres, "Failed to get the scope from the LookIn control");

    Trace(TEXT("Calling BrowseForScope _hwnd %08x, pQueryScope %08x (%08x)"), 
                                            _hwnd, pQueryScope, pQueryScope->pScope);

    hres = _pQueryHandler->BrowseForScope(_hwnd, pQueryScope ? pQueryScope->pScope:NULL, &pScope);
    FailGracefully(hres, "Failed when calling BrowseForScope");

    if ( (hres != S_FALSE) && pScope )
    {
        hres = InsertScopeIntoList(pScope, DA_LAST, TRUE);
        FailGracefully(hres, "Failed when adding the scope to the control");

        ComboBox_SetCurSel(_hwndLookIn, ShortFromResult(hres));
    }

    hres = S_OK;

exit_gracefully:

    if ( pScope )
        CoTaskMemFree(pScope);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::OnHelp
/ -------------------
/   Invoke the context sensitive help for the window, catch the 
/   handler specific and page specific stuff and pass those help
/   requests down to the relevant objects.
/
/ In:
/   pHelpInfo -> help info structure
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::OnHelp(LPHELPINFO pHelpInfo)
{
    HRESULT hres;
    RECT rc;
    HWND hwnd = (HWND)pHelpInfo->hItemHandle;

    TraceEnter(TRACE_FRAME, "CQueryFrame::OnHelp");

    // We are invoking help, theroefore we need ot check to see where element
    // of the window we are being invoked for.  If it is the 
    // result view then route the message to that, if its the form then
    // likewise.
    //
    // If we don't hit any of the extension controls then lets pass the
    // help onto WinHelp and get it to display the topics we have.

    if ( pHelpInfo->iContextType != HELPINFO_WINDOW )
        ExitGracefully(hres, E_FAIL, "WM_HELP handler only copes with WINDOW objects");

    if ( _pCurrentFormPage->hwndPage && IsChild(_pCurrentFormPage->hwndPage, hwnd) )
    {
        // it was on the query form page, therefore let it go there, that way
        // they can provide topics specific to them

        TraceMsg("Invoking help on the form pane");

        hres = _CallPageProc(_pCurrentFormPage, CQPM_HELP, 0, (LPARAM)pHelpInfo);
        FailGracefully(hres, "Failed when calling page proc to get help");
    }
    else
    {
        // pass the help information through to the handler as an activation,
        // this should really just be a new method, but this works.

        TraceMsg("Invoking help on the results pane");
        TraceAssert(_pQueryHandler);

        hres = _pQueryHandler->ActivateView(CQRVA_HELP, 0, (LPARAM)pHelpInfo);
        FailGracefully(hres, "Handler WndProc returned FALSE");
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Scope helper functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ _CallScopeProc
/ --------------
/   Releae the given scope object, freeing the object that is referenced
/   and passing a CQSM_RELEASE message to it.
/
/ In:
/   pQueryScope -> scope object to be called
/   uMsg, pVoid -> parameters for the scope
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _CallScopeProc(LPQUERYSCOPE pQueryScope, UINT uMsg, LPVOID pVoid)
{
    HRESULT hres;

    TraceEnter(TRACE_SCOPES, "_CallScopeProc");
    Trace(TEXT("pQueryScope %08x, uMsg %d, pVoid %08x"), pQueryScope, uMsg, pVoid);
    
    Trace(TEXT("(cbStruct %d, pScopeProc %08x, lParam %08x)"),
                    pQueryScope->pScope->cbStruct,
                    pQueryScope->pScope->pScopeProc,
                    pQueryScope->pScope->lParam);

    if ( !pQueryScope )
        ExitGracefully(hres, S_OK, "pQueryScope == NULL");

    hres = (pQueryScope->pScope->pScopeProc)(pQueryScope->pScope, uMsg, pVoid);
    FailGracefully(hres, "Failed calling ScopeProc");

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ _FreeScope
/ ----------
/   Releae the given scope object, freeing the object that is referenced
/   and passing a CQSM_RELEASE message to it.
/
/ In:
/   pQueryScope -> scope object to be released
/
/ Out:
/   INT == 1 always
/----------------------------------------------------------------------------*/

INT _FreeScopeCB(LPVOID pItem, LPVOID pData)
{
    return _FreeScope((LPQUERYSCOPE)pItem);
}

INT _FreeScope(LPQUERYSCOPE pQueryScope)
{   
    TraceEnter(TRACE_SCOPES, "_FreeScope");
    Trace(TEXT("pQueryScope %08x, pQueryScope->pScope %08x"), pQueryScope, pQueryScope->pScope);
 
    if ( pQueryScope )
    {
        _CallScopeProc(pQueryScope, CQSM_RELEASE, NULL);

        if ( pQueryScope->pScope )
        {
            LocalFree((HLOCAL)pQueryScope->pScope);
            pQueryScope->pScope = NULL;
        }
    }

    TraceLeaveValue(TRUE);
}   


/*-----------------------------------------------------------------------------
/ CQueryFrame::InsertScopeIntoList
/ --------------------------------
/   Adds the given scope to the scope picker.
/
/ In:
/   pQueryScope -> zcope object to be added to the view
/   i = index to insert the scope at
/   fAddToControl = add the scope the picker control
/   ppQueryScope -> recieves the new query scope object / = NULL
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::InsertScopeIntoList(LPCQSCOPE pScope, INT i, BOOL fAddToControl)
{
    HRESULT hres;
    QUERYSCOPE qs;
    INT iScope;

    TraceEnter(TRACE_SCOPES, "CQueryFrame::InsertScopeIntoList");
    Trace(TEXT("pScope %08x, i %d, fAddToControl %d"), pScope, i, fAddToControl);
    
    if ( !pScope )
        ExitGracefully(hres, E_INVALIDARG, "pScope == NULL, not allowed");

    // if we don't have any scopes then allocate the DSA

    if ( !_hdsaScopes )
    {
        _hdsaScopes = DSA_Create(SIZEOF(QUERYSCOPE), 4);
        TraceAssert(_hdsaScopes);

        if ( !_hdsaScopes )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate the scope DPA");
    }

    // Walk the list of scopes checking to see if this one is already in
    // there, if not then we can add it.

    for ( iScope = 0 ; iScope < DSA_GetItemCount(_hdsaScopes) ; iScope++ )
    {
        LPQUERYSCOPE pQueryScope = (LPQUERYSCOPE)DSA_GetItemPtr(_hdsaScopes, iScope);
        TraceAssert(pQueryScope);

        if ( S_OK == _CallScopeProc(pQueryScope, CQSM_SCOPEEQUAL, pScope) )
        {
            hres = ResultFromShort(iScope);
            goto exit_gracefully;
        }
    }

    // Take a copy of the scope blob passed by the caller.  We copy the entire
    // structure who's size is defined by cbStruct into a LocalAlloc block,
    // once we have this we can then build the QUERYSCOPE structure that references
    // it.

    Trace(TEXT("pScope->cbStruct == %d"), pScope->cbStruct);
    qs.pScope = (LPCQSCOPE)LocalAlloc(LPTR, pScope->cbStruct);
    
    if ( !qs.pScope )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate query scope");

    Trace(TEXT("Copying structure qs.pScope %08x, pScope %08x"), qs.pScope, pScope);
    CopyMemory(qs.pScope, pScope, pScope->cbStruct);

    //qs.pScope = NULL;
    qs.iImage = -1;         // no image

    // We have a QUERYSCOPE, so initialize it, if that works then append it to the
    // DSA before either setting the return value or appending it to the control.

    _CallScopeProc(&qs, CQSM_INITIALIZE, NULL);
    
    iScope = DSA_InsertItem(_hdsaScopes, i, &qs);
    Trace(TEXT("iScope = %d"), iScope);

    if ( iScope == -1 )
    {
        _FreeScope(&qs);
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to add scope to DSA");
    }

    if ( fAddToControl )
    {
        LPQUERYSCOPE pQueryScope = (LPQUERYSCOPE)DSA_GetItemPtr(_hdsaScopes, iScope);
        TraceAssert(pQueryScope);

        Trace(TEXT("Calling AddScopeToControl with %08x (%d)"), pQueryScope, iScope);
        hres = AddScopeToControl(pQueryScope, iScope);
    }
    else
    {
        hres = ResultFromShort(iScope);
    }

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::AddScopeToControl
/ ------------------------------
/   Adds the given scope to the scope picker.
/
/ In:
/   pQueryScope -> zcope object to be added to the view
/   i = index into view where to insert the scope
/
/ Out:
/   HRESULT ( == index of item added )
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::AddScopeToControl(LPQUERYSCOPE pQueryScope, INT i)
{
    HRESULT hres;
    CQSCOPEDISPLAYINFO cqsdi;
    COMBOBOXEXITEM cbi;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szIconLocation[MAX_PATH] = { 0 };
    INT item;

    TraceEnter(TRACE_SCOPES, "CQueryFrame::AddScopeToControl");

    if ( !pQueryScope )
        ExitGracefully(hres, E_INVALIDARG, "No scope specified");

    // Call the scope to get the display information about this
    // scope before we attempt to add it.

    cqsdi.cbStruct = SIZEOF(cqsdi);
    cqsdi.dwFlags = 0;
    cqsdi.pDisplayName = szBuffer;
    cqsdi.cchDisplayName = ARRAYSIZE(szBuffer);
    cqsdi.pIconLocation = szIconLocation;
    cqsdi.cchIconLocation = ARRAYSIZE(szIconLocation);
    cqsdi.iIconResID = 0;
    cqsdi.iIndent = 0;
    
    hres = _CallScopeProc(pQueryScope, CQSM_GETDISPLAYINFO, &cqsdi);
    FailGracefully(hres, "Failed to get display info for the scope");               

    // Now add the item to the control, if they gave as an image then
    // add that to the image list (and tweak the INSERTITEM structure
    // accordingly).

    cbi.mask = CBEIF_TEXT|CBEIF_INDENT;
    cbi.iItem = i;
    cbi.pszText = cqsdi.pDisplayName;
    cbi.iIndent = cqsdi.iIndent;

    Trace(TEXT("Indent is %d"), cqsdi.iIndent);

    if ( szIconLocation[0] && cqsdi.iIconResID )
    {
        INT iImage;

        if ( !_fScopeImageListSet )
        {
            HIMAGELIST himlSmall;

            Shell_GetImageLists(NULL, &himlSmall);
            SendMessage(_hwndLookIn, CBEM_SETIMAGELIST, 0, (LPARAM)himlSmall);

            _fScopeImageListSet = TRUE;
        }

        cbi.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
        cbi.iImage = Shell_GetCachedImageIndex(szIconLocation, cqsdi.iIconResID, 0x0);;
        cbi.iSelectedImage = cbi.iImage;

        Trace(TEXT("Image index set to: %d"), cbi.iImage);
    }

    item = (INT)SendMessage(_hwndLookIn, CBEM_INSERTITEM, 0, (LPARAM)&cbi);

    if ( item == -1 )
        ExitGracefully(hres, E_FAIL, "Failed when inserting the scope to the list");

    DoEnableControls();                     // reflect button changes into UI

    hres = ResultFromShort(item);

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::PopulateScopeControl
/ ---------------------------------
/   Collect the scopes that we want to display in the scope control and
/   then populate it.  If the handler doesn't return any scopes then
/   we remove the control and assume that know what to do when they
/   don't receive a scope pointer.
/
/ In:
/   -
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::PopulateScopeControl(VOID)
{
    HRESULT hres;
    LPQUERYSCOPE pQueryScope;
    INT i;
    
    TraceEnter(TRACE_SCOPES, "CQueryFrame::PopulateScopeControl");

    // Collect the scopes that we should be showing in the view, if we don't
    // get any back then we disable the scope control, if we do get some then
    // populate the scope control with them.

    hres = _pQueryHandler->AddScopes();    
    _fAddScopesNYI = (hres == E_NOTIMPL);

    if ( hres != E_NOTIMPL )
        FailGracefully(hres, "Failed when calling handler to add scopes");        

    if ( _hdsaScopes )
    {
        // We have some scopes, so now we create the image list that we can use
        // for icons with scopes.  Then walk through the DPA getting the scope
        // to give us some display information about itself that we can
        // add to the combo box.

        ComboBox_SetExtendedUI(_hwndLookIn, TRUE);

        for ( i = 0 ; i < DSA_GetItemCount(_hdsaScopes); i++ )
        {
            pQueryScope = (LPQUERYSCOPE)DSA_GetItemPtr(_hdsaScopes, i);
            TraceAssert(pQueryScope);

            AddScopeToControl(pQueryScope, i);
        }
    }
    else
    {
        // we don't have any scopes after calling AddScopes, this is either 
        // because the ::AddScopes method is not implemented, or the
        // scopes are being added async.  If IssueQuery returned a success
        // we assume they are coming in async and flag as such in our
        // state.

        if ( !_fAddScopesNYI )
        {
            TraceMsg("Handler adding scopes async, so marking so");
            _fScopesAddedAsync = TRUE;
        }
    }

    hres = S_OK;                                      // success

exit_gracefully:

    Trace(TEXT("Default scope is index %d"), _iDefaultScope);
    ComboBox_SetCurSel(_hwndLookIn, _iDefaultScope);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::GetSelectedScope
/ -----------------------------
/   Get the selected from the the scope ComboBox, this is a reference into the 
/   scope DSA.
/
/ In:
/   ppQueryScope = receives a pointer to the new scope
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::GetSelectedScope(LPQUERYSCOPE* ppQueryScope)
{
    HRESULT hres;
    COMBOBOXEXITEM cbi;
    INT iScope;

    TraceEnter(TRACE_SCOPES, "CQueryFrame::GetSelectedScope");

    *ppQueryScope = NULL;

    if ( _hdsaScopes )
    {
        // Get the index for the current scope, if it doesn't give a real
        // index to a item in our view then barf!  Otherwise look up the
        // associated scope.

        iScope = ComboBox_GetCurSel(_hwndLookIn);
        Trace(TEXT("iScope %d"), iScope);

        if ( iScope == -1 )
            ExitGracefully(hres, E_FAIL, "User entered scopes not supported yet");

        *ppQueryScope = (LPQUERYSCOPE)DSA_GetItemPtr(_hdsaScopes, iScope);
        TraceAssert(*ppQueryScope);
    }

    hres = S_OK;

exit_gracefully:

    Trace(TEXT("Returning LPQUERYSCOPE %08x"), *ppQueryScope); 

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Form handling functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ _FreeQueryForm
/ ---------------
/   Destroy the QUERYFORM allocation being used to describe the form in
/   our DPA.  We ensure that we issue a CQPM_RELEASE before doing anything
/
/ In:
/   pQueryForm -> query form to be destroyed
/
/ Out:
/   INT == 1 always
/----------------------------------------------------------------------------*/

INT _FreeQueryFormCB(LPVOID pItem, LPVOID pData)
{
    return _FreeQueryForm((LPQUERYFORM)pItem);
}

INT _FreeQueryForm(LPQUERYFORM pQueryForm)
{
    TraceEnter(TRACE_FORMS, "_FreeQueryForm");
 
    if ( pQueryForm )
    {
        if ( pQueryForm->hdsaPages )
        {
            DSA_DestroyCallback(pQueryForm->hdsaPages, _FreeQueryFormPageCB, NULL);
            pQueryForm->hdsaPages = NULL;
        }

        Str_SetPtr(&pQueryForm->pTitle, NULL);
        DestroyIcon(pQueryForm->hIcon);
    }

    TraceLeaveValue(TRUE);
}   


/*-----------------------------------------------------------------------------
/ _FreeQueryFormPage
/ ------------------
/   Given a pointer to a query form page structure release the members that
//  are of interest, including calling the PAGEPROC to releasee the underlying
/   object.
/
/ In:
/   pQueryFormPage -> page to be removed
/
/ Out:
/   INT == 1 always
/----------------------------------------------------------------------------*/

INT _FreeQueryFormPageCB(LPVOID pItem, LPVOID pData)
{
    return _FreeQueryFormPage((LPQUERYFORMPAGE)pItem);
}

INT _FreeQueryFormPage(LPQUERYFORMPAGE pQueryFormPage)
{   
    TraceEnter(TRACE_FORMS, "_FreeQueryFormPage");

    if ( pQueryFormPage )
    {
        _CallPageProc(pQueryFormPage, CQPM_RELEASE, 0, 0);          // NB: ignore return code

        if ( pQueryFormPage->hwndPage )
        {
            DestroyWindow(pQueryFormPage->hwndPage);
            pQueryFormPage->hwndPage = NULL;
        }

        if ( pQueryFormPage->pPage )
        {
            LocalFree(pQueryFormPage->pPage);
            pQueryFormPage->pPage = NULL;
        }
    }        

    TraceLeaveValue(TRUE);
}   


/*-----------------------------------------------------------------------------
/ _CallPageProc
/ -------------
/   Call the given page object thunking the arguments as required if the
/   page object is non-UNICODE (only if building UNICODE).
/
/ In:
/   pQueryFormPage -> page object to be called
/   uMsg, wParam, lParam = parameters for message
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _CallPageProc(LPQUERYFORMPAGE pQueryFormPage, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;

    TraceEnter(TRACE_FORMS, "_CallPageProc");
    Trace(TEXT("pQueryFormPage %08x, pPage %08x, uMsg %d, wParam %08x, lParam %08x"), 
                        pQueryFormPage, pQueryFormPage->pPage, uMsg, wParam, lParam);

    if ( !pQueryFormPage )
        ExitGracefully(hres, S_OK, "pQueryFormPage == NULL");
    
    hres = (pQueryFormPage->pPage->pPageProc)(pQueryFormPage->pPage, pQueryFormPage->hwndPage, uMsg, wParam, lParam);
    FailGracefully(hres, "Failed calling PageProc");

    // hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ ANSI functions for adding query forms/pages
/----------------------------------------------------------------------------*/

#ifdef UNICODE

// ANSI CB to add forms to the form DSA.

HRESULT _AddFormsProcA(LPARAM lParam, LPCQFORM_A pForm)
{
    HRESULT hres;
    QUERYFORM qf;
    HDSA hdsaForms = (HDSA)lParam;
    USES_CONVERSION;

    TraceEnter(TRACE_FORMS, "_AddFormsProcA");

    if ( !pForm || !hdsaForms )
        ExitGracefully(hres, E_INVALIDARG, "Failed to add page pForm == NULL");

    // Allocate and thunk as required

    qf.hdsaPages = NULL;               // DSA of pages
    qf.dwFlags = pForm->dwFlags;       // flags
    qf.clsidForm = pForm->clsid;       // CLSID identifier for this form
    qf.pTitle = NULL;                  // title used for drop down / title bar
    qf.hIcon = pForm->hIcon;           // hIcon passed by caller
    qf.iImage = -1;                    // image list index of icon
    qf.iForm = 0;                      // visible index of form in control
    qf.iPage = 0;                      // currently selected page on form

    if ( !Str_SetPtr(&qf.pTitle, A2T(pForm->pszTitle)) )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to copy form title string");

    // Allocate the DSA if one doesn't exist yet, then add in the form
    // structure as required.

    if ( -1 == DSA_AppendItem(hdsaForms, &qf) )
        ExitGracefully(hres, E_FAIL, "Failed to add form to the form DSA");

    hres = S_OK;                          // success
    
exit_gracefully:

    if ( FAILED(hres) )
        _FreeQueryForm(&qf);

    TraceLeaveResult(hres);
}

// ANSI CB to add pages to the page DSA.

HRESULT _AddPagesProcA(LPARAM lParam, REFCLSID clsidForm, LPCQPAGE_A pPage)
{
    HRESULT hres;
    QUERYFORMPAGE qfp;
    HDSA hdsaPages = (HDSA)lParam;

    TraceEnter(TRACE_FORMS, "_AddPagesProcA");

    if ( !pPage || !hdsaPages )
        ExitGracefully(hres, E_INVALIDARG, "Failed to add page pPage == NULL");

    // copy the pPage structure for us to pass to the PAGEPROC later, nb: we
    // use the cbStruct field to indicate the size of blob we must copy.

    Trace(TEXT("pPage->cbStruct == %d"), pPage->cbStruct);
    qfp.pPage = (LPCQPAGE)LocalAlloc(LPTR, pPage->cbStruct);
   
    if ( !qfp.pPage )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate copy of page structure");

    Trace(TEXT("Copying structure qfp.pPage %08x, pPage %08x"), qfp.pPage, pPage);
    CopyMemory(qfp.pPage, pPage, pPage->cbStruct);              // copy the page structure

    qfp.fPageIsANSI = TRUE;
    //qfp.pPage = NULL;
    qfp.clsidForm = clsidForm;
    qfp.pPageProcA = pPage->pPageProc;
    qfp.lParam = pPage->lParam;
    qfp.hwndPage = NULL;

    _CallPageProc(&qfp, CQPM_INITIALIZE, 0, 0);
        
    if ( -1 == DSA_AppendItem(hdsaPages, &qfp) )
        ExitGracefully(hres, E_FAIL, "Failed to add the form to the DSA");

    hres = S_OK;                      // succcess

exit_gracefully:

    if ( FAILED(hres) )
        _FreeQueryFormPage(&qfp);

    TraceLeaveResult(hres);
}

// Add forms/pages from a ANSI IQueryForm iface

HRESULT CQueryFrame::AddFromIQueryFormA(IQueryFormA* pQueryForm, HKEY hKeyForm)
{
    HRESULT hres;

    TraceEnter(TRACE_FORMS, "CQueryFrame::AddFromIQueryFormA");

    if ( !pQueryForm )
        ExitGracefully(hres, E_FAIL, "pQueryForm == NULL, failing");

    hres = pQueryForm->Initialize(hKeyForm);
    FailGracefully(hres, "Failed in IQueryFormA::Initialize");

    // Call the form object to add its form and then its pages

    hres = pQueryForm->AddForms(_AddFormsProcA, (LPARAM)_hdsaForms);
    
    if ( SUCCEEDED(hres) || (hres == E_NOTIMPL) )
    {
        hres = pQueryForm->AddPages(_AddPagesProcA, (LPARAM)_hdsaPages);
        FailGracefully(hres, "Failed in IQueryFormA::AddPages");
    }
    else    
    {
        FailGracefully(hres, "Failed when calling IQueryFormA::AddForms");
    }

    hres = S_OK;                      // success

exit_gracefully:

    TraceLeaveResult(hres);
}

#endif // #ifndef UNICODE


/*-----------------------------------------------------------------------------
/ Functions for adding query forms/pages
/----------------------------------------------------------------------------*/

// CB to add forms to the form DSA.

HRESULT _AddFormsProc(LPARAM lParam, LPCQFORM pForm)
{
    HRESULT hres;
    QUERYFORM qf;
    HDSA hdsaForms = (HDSA)lParam;

    TraceEnter(TRACE_FORMS, "_AddFormsProc");

    if ( !pForm || !hdsaForms )
        ExitGracefully(hres, E_INVALIDARG, "Failed to add page pForm == NULL");

    // Allocate and thunk as required

    qf.hdsaPages = NULL;               // DSA of pages
    qf.dwFlags = pForm->dwFlags;       // flags
    qf.clsidForm = pForm->clsid;       // CLSID identifier for this form
    qf.pTitle = NULL;                  // title used for drop down / title bar
    qf.hIcon = pForm->hIcon;           // hIcon passed by caller
    qf.iImage = -1;                    // image list index of icon
    qf.iForm = 0;                      // visible index of form in control
    qf.iPage = 0;                      // currently selected page on form

    if ( !Str_SetPtr(&qf.pTitle, pForm->pszTitle) )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to copy form title string");

    // Allocate the DSA if one doesn't exist yet, then add in the form
    // structure as required.

    if ( -1 == DSA_AppendItem(hdsaForms, &qf) )
        ExitGracefully(hres, E_FAIL, "Failed to add form to the form DSA");

    hres = S_OK;                          // success
    
exit_gracefully:

    if ( FAILED(hres) )
        _FreeQueryForm(&qf);

    TraceLeaveResult(hres);
}

// CB to add pages to the page DSA.

HRESULT _AddPagesProc(LPARAM lParam, REFCLSID clsidForm, LPCQPAGE pPage)
{
    HRESULT hres;
    QUERYFORMPAGE qfp;
    HDSA hdsaPages = (HDSA)lParam;

    TraceEnter(TRACE_FORMS, "_AddPagesProc");

    if ( !pPage || !hdsaPages )
        ExitGracefully(hres, E_INVALIDARG, "Failed to add page pPage == NULL");

    // copy the pPage structure for us to pass to the PAGEPROC later, nb: we
    // use the cbStruct field to indicate the size of blob we must copy.

    Trace(TEXT("pPage->cbStruct == %d"), pPage->cbStruct);
    qfp.pPage = (LPCQPAGE)LocalAlloc(LPTR, pPage->cbStruct);
   
    if ( !qfp.pPage )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate copy of page structure");

    Trace(TEXT("Copying structure qfp.pPage %08x, pPage %08x"), qfp.pPage, pPage);
    CopyMemory(qfp.pPage, pPage, pPage->cbStruct);              // copy the page structure

#ifdef UNICODE
    qfp.fPageIsANSI = FALSE;
#else
    qfp.fPageIsANSI = TRUE;
#endif
    //qfp.pPage = NULL;
    qfp.clsidForm = clsidForm;
    qfp.pPageProc = pPage->pPageProc;
    qfp.lParam = pPage->lParam;
    qfp.hwndPage = NULL;

    _CallPageProc(&qfp, CQPM_INITIALIZE, 0, 0);
        
    if ( -1 == DSA_AppendItem(hdsaPages, &qfp) )
        ExitGracefully(hres, E_FAIL, "Failed to add the form to the DSA");

    hres = S_OK;                      // succcess

exit_gracefully:

    if ( FAILED(hres) )
        _FreeQueryFormPage(&qfp);

    TraceLeaveResult(hres);
}

// Add forms/pages from a UNICODE IQueryForm iface

HRESULT CQueryFrame::AddFromIQueryForm(IQueryForm* pQueryForm, HKEY hKeyForm)
{
    HRESULT hres;

    TraceEnter(TRACE_FORMS, "CQueryFrame::AddFromIQueryForm");

    if ( !pQueryForm )
        ExitGracefully(hres, E_FAIL, "pQueryForm == NULL, failing");

    hres = pQueryForm->Initialize(hKeyForm);
    FailGracefully(hres, "Failed in IQueryFormW::Initialize");

    // Call the form object to add its form and then its pages

    hres = pQueryForm->AddForms(_AddFormsProc, (LPARAM)_hdsaForms);
    
    if ( SUCCEEDED(hres) || (hres == E_NOTIMPL) )
    {
        hres = pQueryForm->AddPages(_AddPagesProc, (LPARAM)_hdsaPages);
        FailGracefully(hres, "Failed in IQueryForm::AddPages");
    }
    else    
    {
        FailGracefully(hres, "Failed when calling IQueryForm::AddForms");
    }

    hres = S_OK;                      // success

exit_gracefully:

    TraceLeaveResult(hres);
}

#ifdef UNICODE
#define ADD_FROM_IQUERYFORM AddFromIQueryFormW
#else
#define ADD_FROM_IQUERYFORM AddFromIQueryFormA
#endif


/*-----------------------------------------------------------------------------
/ CQueryFrame::GatherForms
/ ------------------------
/   Enumerate all the query forms for the given query handler and build
/   the DPA containing the list of them.  Once we have done this we
/   can then populate the control at some more convientent moment.  
/
/   When gathering we first hit the "handler", then the "Forms" sub-key
/   trying to load all the InProc servers that provide forms.  We build
/   list of hidden, never shown etc.
/
/ In:
/   -
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT _AddPageToForm(LPQUERYFORM pQueryForm, LPQUERYFORMPAGE pQueryFormPage, BOOL fClone)
{
    HRESULT hres;
    QUERYFORMPAGE qfp;
    LPCQPAGE pPage;

    TraceEnter(TRACE_FORMS, "_AddPageToForm");
    TraceAssert(pQueryForm);
    TraceAssert(pQueryFormPage);

    // ensure that we have a page DSA for this form object

    if ( !pQueryForm->hdsaPages )
    {
        TraceMsg("Creating a new page DSA for form");
        pQueryForm->hdsaPages = DSA_Create(SIZEOF(QUERYFORMPAGE), 4);

        if ( !pQueryForm->hdsaPages )
            ExitGracefully(hres, E_OUTOFMEMORY, "*** No page DSA on form object ***");
    }

    if ( !fClone )
    {
        // Moving this page structure to the one associated with the query form,
        // therefore just ensure that the form has a DSA for pages and just 
        // insert an item at the header (yes, we add the pages in reverse).

        Trace(TEXT("Adding page %08x to form %s"), pQueryFormPage, pQueryForm->pTitle);

        if ( -1 == DSA_InsertItem(pQueryForm->hdsaPages, 0, pQueryFormPage) )
            ExitGracefully(hres, E_FAIL, "Failed to copy page to form page DSA");
    }
    else
    {
        LPCQPAGE pPage = pQueryFormPage->pPage;

        // Copying the page structure (it must be global), therefore clone
        // the QUERYFORMPAGE strucutre and the CQPAGE into a new allocation
        // and insert that into the page DSA.

        Trace(TEXT("Cloning page %08x to form %s"), pQueryFormPage, pQueryForm->pTitle);

        CopyMemory(&qfp, pQueryFormPage, SIZEOF(QUERYFORMPAGE));
        qfp.pPage = (LPCQPAGE)LocalAlloc(LPTR, pPage->cbStruct);

        if ( !qfp.pPage )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate copy of page structure");

        Trace(TEXT("Copying structure qfp.pPage %08x, pPage %08x"), qfp.pPage, pPage);
        CopyMemory(qfp.pPage, pPage, pPage->cbStruct);                                      // copy the page structure

        _CallPageProc(&qfp, CQPM_INITIALIZE, 0, 0);
        
        if ( -1 == DSA_AppendItem(pQueryForm->hdsaPages, &qfp) )
        {
            _FreeQueryFormPage(&qfp);
            ExitGracefully(hres, E_FAIL, "Failed to copy page to form DSA");
        }
    }

    hres = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hres);
}

HRESULT CQueryFrame::GatherForms(VOID)
{
    HRESULT hres;
    IQueryForm* pQueryForm = NULL;
    HKEY hKeyForms = NULL;
    TCHAR szBuffer[MAX_PATH];
    INT i, iPage, iForm;
    RECT rect;
    DWORD cbStruct;
    TC_ITEM tci;

    TraceEnter(TRACE_FORMS, "CQueryFrame::GatherForms");

    // Construct DSA's so we can store the forms and pages as required.

    _hdsaForms = DSA_Create(SIZEOF(QUERYFORM), 4);
    _hdsaPages = DSA_Create(SIZEOF(QUERYFORMPAGE), 4);

    if ( !_hdsaForms || !_hdsaPages )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to create DSA's for storing pages/forms");

    // First check the IQueryHandler to see if it supports IQueryForm, if it does
    // then call it to add its objects.  Note that we don't bother with ANSI/UNICODE
    // at this point as the handler is assumed to be built the same the 
    // the query framework. 

    if ( SUCCEEDED(_pQueryHandler->QueryInterface(IID_IQueryForm, (LPVOID*)&pQueryForm)) )
    {
        hres = AddFromIQueryForm(pQueryForm, NULL);
        FailGracefully(hres, "Failed when calling AddFromIQueryForm on handlers IQueryForm iface)");
    }

    // now attempt to build the list of forms and pages from the registered form
    // extensions.  These are declared under the handlers CLSID in the registry,
    // under the sub-key "Forms". 

    if ( ERROR_SUCCESS != RegOpenKeyEx(_hkHandler, c_szForms, NULL, KEY_READ, &hKeyForms) )
    {
        TraceMsg("No 'Forms' sub-key found, therefore skipping");
    }
    else
    {
        // Enumerate all the keys in the "Forms" key, these are assumed to be a list of
        // the form handlers.

        for ( i = 0 ; TRUE ; i++ )
        {
            cbStruct = SIZEOF(szBuffer);
            if ( ERROR_SUCCESS != RegEnumKeyEx(hKeyForms, i, szBuffer, &cbStruct, NULL, NULL, NULL, NULL) )
            {
                TraceMsg("RegEnumKeyEx return's false, therefore stopping eunmeration");
                break;
            }

            GetForms(hKeyForms, szBuffer);
        }
    }

    // Now tally the form/page information together and remove duplicates and attach the pages 
    // to forms, take special note of the global pages.   As all forms will now be in the
    // DSA we can check for a zero count and we don't have to worry about the order
    // in which the the forms and pages were added.

    if ( !DSA_GetItemCount(_hdsaForms) || !DSA_GetItemCount(_hdsaPages) )
        ExitGracefully(hres, E_FAIL, "Either the forms or pages DSA is empty");
        
    for ( iPage = DSA_GetItemCount(_hdsaPages) ; --iPage >= 0 ; )
    {
        LPQUERYFORMPAGE pQueryFormPage = (LPQUERYFORMPAGE)DSA_GetItemPtr(_hdsaPages, iPage);
        TraceAssert(pQueryFormPage);

        Trace(TEXT("iPage %d (of %d)"), iPage, DSA_GetItemCount(_hdsaPages));

        if ( !(pQueryFormPage->pPage->dwFlags & CQPF_ISGLOBAL) )
        {
            LPQUERYFORM pQueryForm = FindQueryForm(pQueryFormPage->clsidForm);
            TraceAssert(pQueryForm);

            TraceGUID("Adding page to form:", pQueryFormPage->clsidForm);

            if ( pQueryForm )
            {
                hres = _AddPageToForm(pQueryForm, pQueryFormPage, FALSE);
                FailGracefully(hres, "Failed when adding page to form");

                if ( !DSA_DeleteItem(_hdsaPages, iPage) )
                    TraceMsg("**** Failed to remove page from global DSA ****");
            }
        }
    }

    for ( iPage = DSA_GetItemCount(_hdsaPages) ; --iPage >= 0 ; )
    {
        LPQUERYFORMPAGE pQueryFormPage = (LPQUERYFORMPAGE)DSA_GetItemPtr(_hdsaPages, iPage);
        TraceAssert(pQueryFormPage);

        if ( (pQueryFormPage->pPage->dwFlags & CQPF_ISGLOBAL) )
        {
            Trace(TEXT("Adding global page to %d forms"), DSA_GetItemCount(_hdsaForms));

            for ( iForm = 0 ; iForm < DSA_GetItemCount(_hdsaForms); iForm++ )
            {
                LPQUERYFORM pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, iForm);
                TraceAssert(pQueryForm);

                if ( !(pQueryForm->dwFlags & CQFF_NOGLOBALPAGES) )
                {
                    hres = _AddPageToForm(pQueryForm, pQueryFormPage, TRUE);
                    FailGracefully(hres, "Failed when adding global page to form");
                }
            }

            _FreeQueryFormPage(pQueryFormPage);
            
            if ( !DSA_DeleteItem(_hdsaPages, iPage) )
                TraceMsg("**** Failed to remove page from global DSA ****");        
        }
    }

    // Walk the list of forms, rmeoving the ones which have no pages assocaited with
    // them, we don't need these around confusing the world around us.  Note that
    // we walk backwards through the list removing.
    //
    // Also remove the optional forms we don't want to ehw orld to see

    for ( iForm = DSA_GetItemCount(_hdsaForms) ; --iForm >= 0 ; )
    {
        LPQUERYFORM pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, iForm);
        TraceAssert(pQueryForm);

        Trace(TEXT("pQueryForm %08x (%s), pQueryForm->hdsaPages %08x (%d)"), 
                        pQueryForm, 
                        pQueryForm->pTitle,
                        pQueryForm->hdsaPages, 
                        pQueryForm->hdsaPages ? DSA_GetItemCount(pQueryForm->hdsaPages):0);

        if ( !pQueryForm->hdsaPages 
                || !DSA_GetItemCount(pQueryForm->hdsaPages )
                    || ((pQueryForm->dwFlags & CQFF_ISOPTIONAL) && !(_pOpenQueryWnd->dwFlags & OQWF_SHOWOPTIONAL))  )
        {
            TraceGUID("Removing form: ", pQueryForm->clsidForm);
            _FreeQueryForm(pQueryForm);
            DSA_DeleteItem(_hdsaForms, iForm);
        } 
    }

    if ( !DSA_GetItemCount(_hdsaForms ) )
        ExitGracefully(hres, E_FAIL, "!!!!! No forms registered after page/form fix ups !!!!!");

    // The pages have been attached to the forms so we can now attempt to create the
    // form/page objects.

    _szForm.cx = 0;
    _szForm.cy = 0;

    tci.mask = TCIF_TEXT;
    tci.pszText = TEXT("");
    tci.cchTextMax = 0;
    TabCtrl_InsertItem(_hwndFrame, 0, &tci);           // tabctrl needs at least one item so we can compute sizes

    for ( iForm = 0 ; iForm < DSA_GetItemCount(_hdsaForms); iForm++ )
    {
        LPQUERYFORM pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, iForm);
        TraceAssert(pQueryForm);

        // Create each of the modeless page dialoges that we show to allow the user
        // to edit the search criteria.  We also grab the size and modify the 
        // form informaiton we have so that the default size of the dialog can be 
        // correctly computed.

        for ( iPage = 0 ; iPage < DSA_GetItemCount(pQueryForm->hdsaPages); iPage++ )
        {
            LPQUERYFORMPAGE pQueryFormPage = (LPQUERYFORMPAGE)DSA_GetItemPtr(pQueryForm->hdsaPages, iPage);
            TraceAssert(pQueryFormPage);

#ifdef UNICODE
            if ( pQueryFormPage->fPageIsANSI )
            {
                pQueryFormPage->hwndPage = CreateDialogParamA(pQueryFormPage->pPageA->hInstance, 
                                                              MAKEINTRESOURCEA(pQueryFormPage->pPageA->idPageTemplate),
                                                              _hwnd, 
                                                              pQueryFormPage->pPageA->pDlgProc, 
                                                              (LPARAM)pQueryFormPage->pPage);
            }
            else
#endif
            {
                pQueryFormPage->hwndPage = CreateDialogParam(pQueryFormPage->pPage->hInstance, 
                                                             MAKEINTRESOURCE(pQueryFormPage->pPage->idPageTemplate),
                                                             _hwnd, 
                                                             pQueryFormPage->pPage->pDlgProc, 
                                                             (LPARAM)pQueryFormPage->pPage);
            }

            if ( !pQueryFormPage->hwndPage )
                ExitGracefully(hres, E_FAIL, "Failed to create query form page");

            GetRealWindowInfo(pQueryFormPage->hwndPage, &rect, NULL);
            TabCtrl_AdjustRect(_hwndFrame, TRUE, &rect);

            _szForm.cx = max(rect.right-rect.left, _szForm.cx);
            _szForm.cy = max(rect.bottom-rect.top, _szForm.cy);

            // flush the form parameters

            _CallPageProc(pQueryFormPage, CQPM_CLEARFORM, 0, 0);

            // Call the page with CQPM_SETDEFAULTPARAMETERS with the
            // OPENQUERYWINDOW structure.  wParam is TRUE/FALSE indiciating if
            // the form is the default one, and therefore if the pFormParam is 
            // valid.

            _CallPageProc(pQueryFormPage, CQPM_SETDEFAULTPARAMETERS, 
                          (WPARAM)((_pOpenQueryWnd->dwFlags & OQWF_DEFAULTFORM) &&
                                IsEqualCLSID(_pOpenQueryWnd->clsidDefaultForm, pQueryFormPage->clsidForm)),
                          (LPARAM)_pOpenQueryWnd);
        }

        // If the form has an hIcon then lets ensure that we add that to the form image
        // list, any failure here is non-fatal, in that we will just skip that forms
        // icon in the list (rather than barfing)

        if ( pQueryForm->hIcon )
        {
            if ( !_himlForms )
                _himlForms = ImageList_Create(COMBOEX_IMAGE_CX, COMBOEX_IMAGE_CY, 0, 4, 1);                
            
            if ( _himlForms )
            {
                pQueryForm->iImage = ImageList_AddIcon(_himlForms, pQueryForm->hIcon);
                TraceAssert(pQueryForm->iImage >= 0);
            }            

            DestroyIcon(pQueryForm->hIcon);
            pQueryForm->hIcon = NULL;
        }
    }

    hres = S_OK;                  // success

exit_gracefully:

    DoRelease(pQueryForm);

    if ( hKeyForms )
        RegCloseKey(hKeyForms);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::GetForms
/ ---------------------
/   Given a HKEY to the forms list and the value name for the form we want
/   to add, query for the form information add add the form objects
/   to the master list.
/
/ In:
/   hKeyForms = HKEY for the {CLSID provider}\Forms key
/   pName -> key value to query for
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::GetForms(HKEY hKeyForms, LPTSTR pName)
{
    HRESULT hres;
    HKEY hKeyForm = NULL;
    TCHAR szQueryFormCLSID[GUIDSTR_MAX+1];
    DWORD dwFlags;
    DWORD dwSize;
    IUnknown* pUnknown = NULL;
    IQueryForm* pQueryForm = NULL;
#ifdef UNICODE
    IQueryFormA* pQueryFormA = NULL;
#endif
    CLSID clsidForm;
    BOOL fIncludeForms = FALSE;

    TraceEnter(TRACE_FORMS, "CQueryFrame::_GetForms");
    Trace(TEXT("pName %s"), pName);

    if ( ERROR_SUCCESS != RegOpenKeyEx(hKeyForms, pName, NULL, KEY_READ, &hKeyForm) )
        ExitGracefully(hres, E_UNEXPECTED, "Failed to open the form key");

    // Read the flags and try to determine if we should invoke this form object.

    dwSize = SIZEOF(dwFlags);
    if ( ERROR_SUCCESS != RegQueryValueEx(hKeyForm, c_szFlags, NULL, NULL, (LPBYTE)&dwFlags, &dwSize) )
    {
        TraceMsg("No flags, defaulting to something sensible");
        dwFlags = QUERYFORM_CHANGESFORMLIST;
    }

    Trace(TEXT("Forms flag is %08x"), dwFlags);

    // should be invoke this form object?
    //
    //  - if dwFlags has QUERYFORM_CHANGESFORMSLIST, or
    //  - if dwFlags has QUERYFORM_CHANGESOPTFORMLIST and we are showing optional forms, or
    //  - neither set and the form object supports the requested form

    if ( !(dwFlags & QUERYFORM_CHANGESFORMLIST) ) 
    {
        if ( (dwFlags & QUERYFORM_CHANGESOPTFORMLIST) &&
                (_pOpenQueryWnd->dwFlags & OQWF_SHOWOPTIONAL) )
        {
            TraceMsg("Form is optional, are we are showing optional forms");
            fIncludeForms = TRUE;
        }
        else
        {
            // OK, so it either didn't update the form list, or wasn't marked as optional,
            // so lets check to see if it supports the form the user has requested, if not
            // then don't bother loading this guy.

            if ( _pOpenQueryWnd->dwFlags & OQWF_DEFAULTFORM )
            {
                TCHAR szBuffer[GUIDSTR_MAX+32];
                HKEY hkFormsSupported;

                TraceMsg("Checking for supported form");                

                if ( ERROR_SUCCESS == RegOpenKeyEx(hKeyForm, TEXT("Forms Supported"), NULL, KEY_READ, &hkFormsSupported) ) 
                {
                    TraceMsg("Form has a 'Supported Forms' sub-key");

                    GetStringFromGUID(_pOpenQueryWnd->clsidDefaultForm, szQueryFormCLSID, ARRAYSIZE(szQueryFormCLSID));
                    Trace(TEXT("Checking for: %s"), szQueryFormCLSID);

                    if ( ERROR_SUCCESS == RegQueryValueEx(hkFormsSupported, szQueryFormCLSID, NULL, NULL, NULL, NULL) )
                    {
                        TraceMsg("Query form is in supported list");
                        fIncludeForms = TRUE;
                    }

                    RegCloseKey(hkFormsSupported);
                }
                else
                {
                    TraceMsg("No forms supported sub-key, so loading form object anyway");
                    fIncludeForms = TRUE;
                }
            }                
        }
    }
    else
    {
        TraceMsg("Form updates form list");
        fIncludeForms = TRUE;
    }

    // if fIncludeForms is TRUE, then the checks above succeeded and we are including forms
    // from this object (identified by pName), so we must now get the CLSID of the object
    // we are invoking and use its IQueryForm interface to add the forms that we want.

    if ( fIncludeForms )
    {
        // get the form object CLSID, having parse it, then CoCreate it adding the forms.

        dwSize = SIZEOF(szQueryFormCLSID);
        if ( ERROR_SUCCESS != RegQueryValueEx(hKeyForm, c_szCLSID, NULL, NULL, (LPBYTE)szQueryFormCLSID, &dwSize) )
            ExitGracefully(hres, E_UNEXPECTED, "Failed to read the CLSID of the form");

        Trace(TEXT("szQueryFormCLSID: %s"), szQueryFormCLSID);

        if ( !GetGUIDFromString(szQueryFormCLSID, &clsidForm) )
            ExitGracefully(hres, E_UNEXPECTED, "Fialed to parse the string as a GUID");

        // we now have the CLISD of the form object, so we must attempt to CoCreate it, we try for
        // the current build type (eg UNICODE) and then fall back to ANSI if thats not supported,
        // so we can support ANSI query form objects on a UNICODE platform.

        hres = CoCreateInstance(clsidForm, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnknown);
        FailGracefully(hres, "Failed to CoCreate the form object");

        if ( SUCCEEDED(pUnknown->QueryInterface(IID_IQueryForm, (LPVOID*)&pQueryForm)) )
        {
            hres = AddFromIQueryForm(pQueryForm, hKeyForm);
            FailGracefully(hres, "Failed when adding forms from specified IQueryForm iface");
        }
#ifdef UNICODE
        else if ( SUCCEEDED(pUnknown->QueryInterface(IID_IQueryFormA, (LPVOID*)&pQueryFormA)) )
        {
            hres = AddFromIQueryFormA(pQueryFormA, hKeyForms);
            FailGracefully(hres, "(ANSI) Failed when adding forms from specified IQueryForm iface");
        }
#endif    
        else
        {
            ExitGracefully(hres, E_UNEXPECTED, "Form object doesn't support IQueryForm(A/W)");
        }
    }

    hres = S_OK;

exit_gracefully:

    if ( hKeyForm )
        RegCloseKey(hKeyForm);

    DoRelease(pUnknown);
    DoRelease(pQueryForm);
#ifdef UNICODE
    DoRelease(pQueryFormA);
#endif

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::PopulateFormControl
/ ---------------------------------
/   Enumerate all the query forms for the given query handler and build
/   the DPA containing the list of them.  Once we have done this we
/   can then populate the control at some more convientent moment.  
/
/   When gathering we first hit the "handler", then the "Forms" sub-key
/   trying to load all the InProc servers that provide forms.  We build
/   list of hidden, never shown etc.
/
/ In:
/   fIncludeHidden = list forms marked as hidden in control
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::PopulateFormControl(BOOL fIncludeHidden)
{
    HRESULT hres;
    COMBOBOXEXITEM cbi;
    LPQUERYFORM pQueryForm;
    INT i, iForm;

    TraceEnter(TRACE_FORMS, "CQueryFrame::PopulateFormControl");
    Trace(TEXT("fIncludeHidden: %d"), fIncludeHidden);

    // list which forms within the control

    if ( !_hdsaForms )
        ExitGracefully(hres, E_FAIL, "No forms to list");
        
    ComboBox_ResetContent(_hwndLookFor);                           // remove all items from that control
    
    for ( i = 0, iForm = 0 ; iForm < DSA_GetItemCount(_hdsaForms); iForm++ )
    {
        LPQUERYFORM pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, iForm);
        TraceAssert(pQueryForm);

        // filter out those forms that are not of interest to this instance of the
        // dialog.

        if ( ((pQueryForm->dwFlags & CQFF_ISOPTIONAL) && !fIncludeHidden) || 
              (pQueryForm->dwFlags & CQFF_ISNEVERLISTED) )
        {
            Trace(TEXT("Hiding form: %s"), pQueryForm->pTitle);
            continue;
        }

        // now add the form to the control, including the image if there is an image
        // specified.

        cbi.mask = CBEIF_TEXT|CBEIF_LPARAM;
        cbi.iItem = i++;
        cbi.pszText = pQueryForm->pTitle;
        cbi.cchTextMax = lstrlen(pQueryForm->pTitle);
        cbi.lParam = iForm;

        if ( pQueryForm->iImage >= 0 )
        {
            Trace(TEXT("Form has an image %d"), pQueryForm->iImage);

            cbi.mask |= CBEIF_IMAGE|CBEIF_SELECTEDIMAGE;
            cbi.iImage = pQueryForm->iImage;
            cbi.iSelectedImage = pQueryForm->iImage;
        }

        pQueryForm->iForm = (int)SendMessage(_hwndLookFor, CBEM_INSERTITEM, 0, (LPARAM)&cbi);

        if ( pQueryForm->iForm < 0 )
        {
            Trace(TEXT("Form name: %s"), pQueryForm->pTitle);
            ExitGracefully(hres, E_FAIL, "Failed to add the entry to the combo box");
        }
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveValue(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::SelectForm
/ -----------------------
/   Changes the current form to the one specified as an into the DPA.
/
/ In:
/   iForm = form to be selected
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::SelectForm(REFCLSID clsidForm)
{
    HRESULT hres;
    LPQUERYFORM pQueryForm, pOldQueryForm;
    LPQUERYFORMPAGE pQueryFormPage;
    LPCQPAGE pPage;
    INT nCmdShow = SW_SHOW;
    TCHAR szBuffer[64], szTitle[MAX_PATH];;
    TC_ITEM tci;
    INT i;
    
    TraceEnter(TRACE_FORMS, "CQueryFrame::SelectForm");
    
    pQueryForm = FindQueryForm(clsidForm);
    TraceAssert(pQueryForm);

    if ( !pQueryForm )
        ExitGracefully(hres, S_FALSE, "Failed to find the requested form");

    // Change the currently displayed form and change the displayed
    // tabs to correctly indicate this

    if ( (pQueryForm != _pCurrentForm) )
    {            
        if ( !OnNewQuery(FALSE) )                               // prompt the user
            ExitGracefully(hres, S_FALSE, "Failed to select the new form");

        TabCtrl_DeleteAllItems(_hwndFrame);

        for ( i = 0 ; i < DSA_GetItemCount(pQueryForm->hdsaPages) ; i++ )
        {
            pQueryFormPage = (LPQUERYFORMPAGE)DSA_GetItemPtr(pQueryForm->hdsaPages, i);
            pPage = pQueryFormPage->pPage;

            tci.mask = TCIF_TEXT;
            tci.pszText = pQueryForm->pTitle;
            tci.cchTextMax = MAX_PATH;

            if ( pPage->idPageName && 
                    LoadString(pPage->hInstance, pPage->idPageName, szBuffer, ARRAYSIZE(szBuffer)) ) 
            {
                Trace(TEXT("Loaded page title string %s"), szBuffer);
                tci.pszText = szBuffer;
            }

            TabCtrl_InsertItem(_hwndFrame, i, &tci);
        }

        ComboBox_SetCurSel(_hwndLookFor, pQueryForm->iForm);
        _pCurrentForm = pQueryForm;

        SelectFormPage(pQueryForm, pQueryForm->iPage);
       
        // Change the dialog title to reflect the new form

        if ( LoadString(GLOBAL_HINSTANCE, IDS_FIND, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            wsprintf(szTitle, szBuffer, pQueryForm->pTitle);
#if defined(DSUI_DEBUG) && !defined(UNICODE)
            StrCat(szTitle, TEXT(" (ANSI CMNQUERY)"));
#endif
            SetWindowText(_hwnd, szTitle);
        }

        // Tell the handler that we have changed the form, they can then use this
        // new form name to modify their UI.

        _pQueryHandler->ActivateView(CQRVA_FORMCHANGED, (WPARAM)lstrlen(pQueryForm->pTitle), (LPARAM)pQueryForm->pTitle);
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::SelectFormPage
/ ---------------------------
/   Change the currently active page of a query form to the one specified
/   by the index.
/
/ In:
/   pQueryForm = query form to be changed
/   iForm = form to be selected
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CQueryFrame::SelectFormPage(LPQUERYFORM pQueryForm, INT iPage)
{
    LPQUERYFORMPAGE pQueryFormPage;
    RECT rect;

    TraceEnter(TRACE_FORMS, "CQueryFrame::SelectFormPage");

    pQueryFormPage = (LPQUERYFORMPAGE)DSA_GetItemPtr(pQueryForm->hdsaPages, iPage);
       
    // Have we changed the query form page?  If so then display the now dialog
    // hiding the previous one.  We call the TabCtrl to find out where we should
    // be placing this new control.

    if ( pQueryFormPage != _pCurrentFormPage )
    {
        // Reflect the change into the tab control

        TabCtrl_SetCurSel(_hwndFrame, iPage);
        pQueryForm->iPage = iPage;

        // Fix the size and visability of the new form
        
        if ( _pCurrentFormPage )
            ShowWindow(_pCurrentFormPage->hwndPage, SW_HIDE);
    
        GetRealWindowInfo(_hwndFrame, &rect, NULL);
        TabCtrl_AdjustRect(_hwndFrame, FALSE, &rect);

        SetWindowPos(pQueryFormPage->hwndPage, 
                     HWND_TOP,
                     rect.left, rect.top, 
                     rect.right - rect.left,
                     rect.bottom - rect.top,
                     SWP_SHOWWINDOW);

        _pCurrentFormPage = pQueryFormPage;    
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::CallFormPages
/ --------------------------
/   Given a query form traverse the array of pages calling each of them
/   with the given message information.  If any of the pages return
/   an error code (other than E_NOTIMPL) we bail.
/
/ In:
/   pQueryForm = query form to call
/   dwFlags = flags indicating which pages to call
/   uMsg, wParam, lParam = parameters for the page
/   
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CQueryFrame::CallFormPages(LPQUERYFORM pQueryForm, DWORD dwFlags, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;    
    INT iPage;

    TraceEnter(TRACE_FORMS, "CQueryFrame::CallFormPages");

    if ( !pQueryForm || !pQueryForm->hdsaPages )
        ExitGracefully(hres, E_FAIL, "No pQueryForm || pQueryForm->hdsaPages == NULL");

    Trace(TEXT("pQueryForm %08x, dwFlags %08x"), pQueryForm, dwFlags);
    Trace(TEXT("uMsg %08x, wParam %08x, lParam %08x"), uMsg, wParam, lParam);
    Trace(TEXT("%d pages to call"), DSA_GetItemCount(pQueryForm->hdsaPages));

    // Call each page in turn if it matches the filter we have been given for calling
    // down.  If a page returns S_FALSE or a FAILURE then we exit the loop.  If the
    // failure however is E_NOTIMPL then we ignore.

    for ( iPage = 0 ; iPage < DSA_GetItemCount(pQueryForm->hdsaPages); iPage++ )
    {
        LPQUERYFORMPAGE pQueryFormPage = (LPQUERYFORMPAGE)DSA_GetItemPtr(pQueryForm->hdsaPages, iPage);
        TraceAssert(pQueryFormPage);

        if ( (  (pQueryFormPage->fPageIsANSI) && (dwFlags & CALLFORMPAGES_ANSI)) ||
             ( !(pQueryFormPage->fPageIsANSI) && (dwFlags & CALLFORMPAGES_UNICODE)) )
        {
            hres = _CallPageProc(pQueryFormPage, uMsg, wParam, lParam);

            if ( FAILED(hres) && (hres != E_NOTIMPL) )
            {
                TraceMsg("PageProc returned a FAILURE");
                break;
            }
            else if ( hres == S_FALSE )
            {
                TraceMsg("PageProc returned S_FALSE, exiting loop");
                break;
            }
        }
    }

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CQueryFrame::FindQueryForm
/ --------------------------
/   Given the CLSID for the form return a pointer to its LPQUERYFORM structure,
/   or NULL if not found.
/
/ In:
/   clsidForm = ID of the form
/   
/ Out:
/   LPQUERYFORM
/----------------------------------------------------------------------------*/
LPQUERYFORM CQueryFrame::FindQueryForm(REFCLSID clsidForm)
{
    LPQUERYFORM pQueryForm = NULL;
    INT i;

    TraceEnter(TRACE_FORMS, "CQueryFrame::FindQueryForm");
    TraceGUID("Form ID", clsidForm);

    for ( i = 0 ; _hdsaForms && (i < DSA_GetItemCount(_hdsaForms)) ; i++ )
    {
        pQueryForm = (LPQUERYFORM)DSA_GetItemPtr(_hdsaForms, i);
        TraceAssert(pQueryForm);

        if ( IsEqualCLSID(clsidForm, pQueryForm->clsidForm) )
        {
            Trace(TEXT("Form is index %d (%08x)"), i, pQueryForm);
            break;
        }
    }

    if ( !_hdsaForms || (i >= DSA_GetItemCount(_hdsaForms)) )
        pQueryForm = NULL;

    TraceLeaveValue(pQueryForm);
}
