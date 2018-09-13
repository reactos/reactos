#include "pch.h"
#include "stddef.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Result view
/----------------------------------------------------------------------------*/

//
// CDsQuery which implements IQueryHandler, IQueryForm etc
//

class CDsQuery; // forward reference

typedef HRESULT (*LPCOLLECTPROC)(CDsQuery* pdq, LPARAM lParam, INT item, LPQUERYRESULT pResult);

class CDsQuery : public IQueryHandler, IQueryForm, IObjectWithSite, IDsQueryHandler, CUnknown
{
    friend HRESULT _ScopeProc(LPCQSCOPE pScope, UINT uMsg, LPVOID pVoid);
    friend HRESULT _AddScope(INT iIndent, LPWSTR pPath, LPWSTR pObjectClass, BOOL fSelect);
    friend LRESULT CALLBACK _ResultViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend int CALLBACK _BrowseForScopeCB(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
    friend HRESULT _GetIDLCollectCB(CDsQuery* pdq, LPARAM lParam, INT item, LPQUERYRESULT pResult);

    public:
        CDsQuery();
        ~CDsQuery();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);                             
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IQueryForms
        STDMETHOD(Initialize)(THIS_ HKEY hkForm);
        STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
        STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

        // IQueryHandler        
        STDMETHOD(Initialize)(THIS_ IQueryFrame* pQueryFrame, DWORD dwOQWFlags, LPVOID pParameters);
        STDMETHOD(GetViewInfo)(THIS_ LPCQVIEWINFO pViewInfo);
        STDMETHOD(AddScopes)(THIS);
        STDMETHOD(BrowseForScope)(THIS_ HWND hwndParent, LPCQSCOPE pCurrentScope, LPCQSCOPE* ppScope);
        STDMETHOD(CreateResultView)(THIS_ HWND hwndParent, HWND* phWndView);
        STDMETHOD(ActivateView)(THIS_ UINT uState, WPARAM wParam, LPARAM lParam);
        STDMETHOD(InvokeCommand)(THIS_ HWND hwndParent, UINT uID);
        STDMETHOD(GetCommandString)(THIS_ UINT uID, DWORD dwFlags, LPTSTR pBuffer, INT cchBuffer);
        STDMETHOD(IssueQuery)(THIS_ LPCQPARAMS pQueryParams);
        STDMETHOD(StopQuery)(THIS);
        STDMETHOD(GetViewObject)(THIS_ UINT uScope, REFIID riid, void **ppvOut);
        STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery);
        STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery, LPCQSCOPE pScope);

        // IObjectWithSite
        STDMETHODIMP SetSite(IUnknown* punk);
        STDMETHODIMP GetSite(REFIID riid, void **ppv);

        // IDsQueryHandler
        STDMETHOD(UpdateView)(THIS_ DWORD dwType, LPDSOBJECTNAMES pdon);
        
    private:        
        LRESULT OnSize(INT cx, INT cy);
        LRESULT OnNotify(HWND hWnd, WPARAM wParam, LPARAM lParam);
        HRESULT OnAddResults(DWORD dwQueryReference, HDPA hdpaResults);
        LRESULT OnContextMenu(HWND hwndMenu, LPARAM lParam);    
        HRESULT OnFileProperties(VOID);
#if !DOWNLEVEL_SHELL
        HRESULT OnFileCreateShortcut(VOID);
#endif
        HRESULT OnFileSaveQuery(VOID);
        HRESULT OnEditSelectAll(VOID);
        HRESULT OnEditInvertSelection(VOID);
        HRESULT OnPickColumns(HWND hwndParent);

        HRESULT _InitNewQuery(LPDSQUERYPARAMS pDsQueryParams, BOOL fRefreshColumnTable);
        HRESULT _GetFilterValue(INT iColumn, HD_ITEM* pitem);
        HRESULT _FilterView(BOOL fCheck);
        HRESULT _PopulateView(INT iFirstItem, INT iLast);
        VOID _FreeResults(VOID);
        DWORD _SetViewMode(INT uID);
        VOID _SortResults(INT iColumn);
        VOID _SetFilter(BOOL fFilter);
        VOID _ShowBanner(UINT flags, UINT idPrompt);
        VOID _InitViewMenuItems(HMENU hMenu);
        HRESULT _GetQueryFormKey(REFCLSID clsidForm, HKEY* phKey);
        HRESULT _GetColumnTable(REFCLSID clsidForm, LPDSQUERYPARAMS pDsQueryParams, HDSA* pHDSA, BOOL fSetInView);
        VOID _SaveColumnTable(VOID);
        HRESULT _SaveColumnTable(REFCLSID clsidForm, HDSA hdsaColumns);
        HRESULT _CollectViewSelection(BOOL fGetAll, INT* pCount, LPCOLLECTPROC pCollectProc, LPARAM lParam);
        HRESULT _GetIDLsAndViewObject(BOOL fGetAll, REFIID riid, void **ppcm);
        VOID _GetContextMenuVerbs(IContextMenu* pcm, HMENU hMenu, DWORD dwFlags);
        HRESULT _CopyCredentials(LPWSTR *ppszUserName, LPWSTR *ppszPassword, LPWSTR *ppszServer);
        HRESULT _GetDirectorySF(IShellFolder **ppsf);
        HRESULT _ADsPathToIdList(LPITEMIDLIST* ppidl, LPWSTR pPath, LPWSTR pObjectClasse);
        VOID _DeleteViewItems(LPDSOBJECTNAMES pdon);

    private:
        IQueryFrame*  _pqf;                    // our parent window
        IUnknown*     _punkSite;               // site object
        IContextMenu* _pcm;                    // Curerntly displayed context menu / == NULL if none

        DWORD         _dwOQWFlags;             // flags passed to OpenQueryWindow
        DWORD         _dwFlags;                // flags as part of the ds query parameters

        LPWSTR        _pDefaultScope;          // default scope passed
        LPWSTR        _pDefaultSaveLocation;   // directory to save queries into by default
        LPTSTR        _pDefaultSaveName;       // default save name (from the query form)

        LPWSTR        _pServer;                // server to target
        LPWSTR        _pUserName;              // user name and password to authenticate with
        LPWSTR        _pPassword;

        BOOL          _fNoSelection:1;         // the IContextMenu was from no selection
        BOOL          _fColumnsModified:1;     // settings of the view modified
        BOOL          _fSortDescending:1;      // sort the results descending
        BOOL          _fFilter:1;              // filter enabled
        BOOL          _fFilterSupported:1;     // is the filter available, eg: comctl32 > 5.0
        
        INT           _idViewMode;             // default view mode
        INT           _iSortColumn;            // sort column

        HWND          _hwnd;                   // container window
        HWND          _hwndView;               // listview window (child of parent)
        HWND          _hwndBanner;             // banner window which is a child of the list view

        DWORD         _dwQueryReference;       // reference value passed to query 
        HANDLE        _hThread;                // worker thread handle
        DWORD         _dwThreadId;             // thread ID for the Query processing thread
        CLSID         _clsidForm;              // form being used for column table
        HDSA          _hdsaColumns;            // column information (size, filters, etc)
        HDPA          _hdpaResults;            // results for tr the query we have issued
        LPTSTR        _pFilter;                // current filter

        HMENU         _hFrameMenuBar;          // stored frame menu bar, stored from activate

        HMENU         _hFileMenu;              // added to the frames view menu
        HMENU         _hEditMenu;              // inserted into the menu bar
        HMENU         _hViewMenu;              // inserted into the menu bar
        HMENU         _hHelpMenu;              // inserted into the menu bar
};


//
// Window classes we create to show the results
//

#define VIEW_CLASS                  TEXT("ActiveDsQueryView")
LRESULT CALLBACK _ResultViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define BANNER_CLASS                TEXT("ActiveDsQueryBanner")
LRESULT CALLBACK _BannerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// 
// Registry values used for the settings
// 

#define VIEW_SETTINGS_VALUE         TEXT("ViewSettings")
#define ADMIN_VIEW_SETTINGS_VALUE   TEXT("AdminViewSettings");


//
// When filtering we populate the view using PostMessage, doing so many items
// at a time
//

#define FILTER_UPDATE_COUNT         128


// 
// All items within the list view contain the following LPARAM structure used
// for storing the magic properties we are interested in. 
//

#define ENABLE_MENU_ITEM(hMenu, id, fEnabled) \
                EnableMenuItem(hMenu, id, (fEnabled) ? (MF_BYCOMMAND|MF_ENABLED):(MF_BYCOMMAND|MF_GRAYED))


//
// Persisted column data, this is stored in the registry under the CLSID for the
// form we are interested in.
//

typedef struct
{
    DWORD cbSize;                   // offset to the next column / == 0 if none
    DWORD dwFlags;                  // flags
    DWORD offsetProperty;           // offset to property name (UNICODE)
    DWORD offsetHeading;            // offset to column heading
    INT cx;                         // pixel width of the column
    INT fmt;                        // format of the column
} SAVEDCOLUMN, * LPSAVEDCOLUMN;


//
// Table to map property types to useful information
//

struct
{
    LPCTSTR pMenuName;
    INT idOperator;
    INT hdft;
}
property_type_table[] =
{
    0, 0, 0,
    MAKEINTRESOURCE(IDR_OP_STRING), FILTER_CONTAINS, HDFT_ISSTRING,
    MAKEINTRESOURCE(IDR_OP_STRING), FILTER_CONTAINS, HDFT_ISSTRING,
    MAKEINTRESOURCE(IDR_OP_NUMBER), FILTER_IS,       HDFT_ISNUMBER,
    MAKEINTRESOURCE(IDR_OP_NUMBER), FILTER_IS,       HDFT_ISNUMBER,           // PROPERTY_ISBOOL
};


//
// Help information for the frame and the control
//

static DWORD const aHelpIDs[] =
{
    CQID_LOOKFORLABEL, IDH_FIND,
    CQID_LOOKFOR,      IDH_FIND, 
    CQID_LOOKINLABEL,  IDH_IN, 
    CQID_LOOKIN,       IDH_IN,
    CQID_BROWSE,       IDH_BROWSE,
    CQID_FINDNOW,      IDH_FIND_NOW,
    CQID_STOP,         IDH_STOP,
    CQID_CLEARALL,     IDH_CLEAR_ALL,
    IDC_RESULTS,       IDH_RESULTS,
    IDC_STATUS,        IDH_NO_HELP,
    0, 0,    
}; 


//
// Help information for the browse dialog that is shown for scopes
//

static DWORD const aBrowseHelpIDs[] =
{
    DSBID_BANNER, (DWORD)-1,
    DSBID_CONTAINERLIST, IDH_BROWSE_CONTAINER,
    0, 0,
};


//
// path to DsFolder
//

#if DELEGATE
WCHAR c_szDsMagicPath[] = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}";
#else
WCHAR c_szDsMagicPath[] = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\EntireNetwork\\::{fe1290f0-cfbd-11cf-a330-00aa00c16e65}";
#endif


/*-----------------------------------------------------------------------------
/ CDsQuery
/----------------------------------------------------------------------------*/

CDsQuery::CDsQuery() :
    _fNoSelection(TRUE),
    _iSortColumn(-1),      
    _idViewMode(DSQH_VIEW_DETAILS)
{
    TraceEnter(TRACE_HANDLER, "CDsQuery::CDsQuery");

    if ( CheckDsPolicy(NULL, c_szEnableFilter) )
    {
        TraceMsg("QuickFilter enabled in policy");
        _fFilter = TRUE;
    }

    TraceLeave();       
}

CDsQuery::~CDsQuery()
{
    TraceEnter(TRACE_HANDLER, "CDsQuery::~CDsQuery");

    // persist the column information if we need to

    if ( _hdsaColumns )
    {
        if ( _fColumnsModified )
        {
            _SaveColumnTable(_clsidForm, _hdsaColumns);
            _fColumnsModified = FALSE;
        }

        _SaveColumnTable();
    }

    // discard all the other random state we have

    LocalFreeStringW(&_pDefaultScope);
    LocalFreeStringW(&_pDefaultSaveLocation);
    LocalFreeString(&_pDefaultSaveName);

    LocalFreeStringW(&_pUserName);
    LocalFreeStringW(&_pPassword);
    LocalFreeStringW(&_pServer);

    if ( IsWindow(_hwnd) )
        DestroyWindow(_hwnd);

    if ( IsMenu(_hFileMenu) )
        DestroyMenu(_hFileMenu);
    if ( IsMenu(_hEditMenu) )
         DestroyMenu(_hEditMenu);
    if ( IsMenu(_hViewMenu) )
        DestroyMenu(_hViewMenu);
    if ( IsMenu(_hHelpMenu) )
        DestroyMenu(_hHelpMenu);

    // tell the thread its time to die

    if ( _hThread )
    {
        PostThreadMessage(_dwThreadId, RVTM_STOPQUERY, 0, 0);
        PostThreadMessage(_dwThreadId, WM_QUIT, 0, 0);
        CloseHandle(_hThread);
    }

    DoRelease(_pqf);
    DoRelease(_punkSite);
    DoRelease(_pcm);

    TraceLeave();
}

// IUnknown bits

#undef  CLASS_NAME
#define CLASS_NAME CDsQuery
#include "unknown.inc"

STDMETHODIMP CDsQuery::QueryInterface(REFIID riid, void **ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IQueryForm, (IQueryForm*)this,
        &IID_IQueryHandler, (IQueryHandler*)this,
        &IID_IObjectWithSite, (IObjectWithSite*)this,
        &IID_IDsQueryHandler, (IDsQueryHandler*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// Handle creating an instance of CLSID_DsQuery
//

STDAPI CDsQuery_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsQuery *pdq = new CDsQuery();
    if ( !pdq )
        return E_OUTOFMEMORY;

    HRESULT hres = pdq->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdq->Release();
    return hres;
}


/*-----------------------------------------------------------------------------
/ IQueryForm methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::Initialize(THIS_ HKEY hkForm)
{
    TraceEnter(TRACE_FORMS, "CDsQuery::Initialize");
    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

struct
{
    CLSID const * clsidForm;
    INT idsTitle;
    DWORD dwFlags;
}
forms[] =
{
    &CLSID_DsFindPeople,           IDS_FINDUSER,          0,
    &CLSID_DsFindComputer,         IDS_FINDCOMPUTER,      0,
    &CLSID_DsFindPrinter,          IDS_FINDPRINTERS,      0,
    &CLSID_DsFindVolume,           IDS_FINDSHAREDFOLDERS, 0,
    &CLSID_DsFindContainer,        IDS_FINDOU,            0,
    &CLSID_DsFindAdvanced,         IDS_CUSTOMSEARCH,      CQFF_NOGLOBALPAGES,

    &CLSID_DsFindDomainController, IDS_FINDDOMCTL,        CQFF_ISNEVERLISTED|CQFF_NOGLOBALPAGES,
    &CLSID_DsFindFrsMembers,       IDS_FINDFRSMEMBER,     CQFF_ISNEVERLISTED|CQFF_NOGLOBALPAGES,
};

STDMETHODIMP CDsQuery::AddForms(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam)
{
    HRESULT hres;
    TCHAR szBuffer[MAX_PATH];
    INT i;

    TraceEnter(TRACE_FORMS, "CDsQuery::AddForms");

    if ( !pAddFormsProc )
        ExitGracefully(hres, E_INVALIDARG, "No AddFormsProc");

    for ( i = 0; i < ARRAYSIZE(forms); i++ ) 
    {
        CQFORM qf = { 0 };

        qf.cbStruct = SIZEOF(qf);
        qf.dwFlags = forms[i].dwFlags;
        qf.clsid = *forms[i].clsidForm;
        qf.pszTitle = szBuffer;

        LoadString(GLOBAL_HINSTANCE, forms[i].idsTitle, szBuffer, ARRAYSIZE(szBuffer));

        hres = (*pAddFormsProc)(lParam, &qf);
        FailGracefully(hres, "Failed to add form (calling pAddFormsFunc)");        
    }

    hres = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

struct
{
    CLSID const * clisdForm;
    LPCQPAGEPROC pPageProc;
    DLGPROC pDlgProc;
    INT idPageTemplate;
    INT idPageName;
    DWORD dwFlags;
} 
pages[] =
{   
    //
    // Page list for the default forms that we add
    //

    &CLSID_DsFindPeople,           PageProc_User,             DlgProc_User,             IDD_FINDUSER,        IDS_FINDUSER,          0, 
    &CLSID_DsFindComputer,         PageProc_Computer,         DlgProc_Computer,         IDD_FINDCOMPUTER,    IDS_FINDCOMPUTER,      0,
    &CLSID_DsFindPrinter,          PageProc_Printers,         DlgProc_Printers,         IDD_FINDPRINT1,      IDS_FINDPRINTERS,      0, 
    &CLSID_DsFindPrinter,          PageProc_PrintersMore,     DlgProc_PrintersMore,     IDD_FINDPRINT2,      IDS_MORECHOICES,       0, 
    &CLSID_DsFindVolume,           PageProc_Volume,           DlgProc_Volume,           IDD_FINDVOLUME,      IDS_FINDSHAREDFOLDERS, 0, 
    &CLSID_DsFindContainer,        PageProc_Container,        DlgProc_Container,        IDD_FINDCONTAINER,   IDS_FINDOU,            0, 
    &CLSID_DsFindAdvanced,         PageProc_PropertyWell,     DlgProc_PropertyWell,     IDD_PROPERTYWELL,    IDS_CUSTOMSEARCH,      0,
    &CLSID_DsFindAdvanced,         PageProc_RawLDAP,          DlgProc_RawLDAP,          IDD_FINDUSINGLDAP,   IDS_ADVANCED,          0, 
    &CLSID_DsFindDomainController, PageProc_DomainController, DlgProc_DomainController, IDD_FINDDOMCTL,      IDS_FINDDOMCTL,        0, 
    &CLSID_DsFindFrsMembers,       PageProc_FrsMember,        DlgProc_FrsMember,        IDD_FINDFRSMEMBER,   IDS_FINDFRSMEMBER,     0, 

    //
    // Make the property well available on all pages (using the magic CQPF_ADDTOALLFORMS bit)
    //

    &CLSID_DsFindAdvanced,          PageProc_PropertyWell,    DlgProc_PropertyWell,     IDD_PROPERTYWELL,  IDS_ADVANCED,          CQPF_ISGLOBAL,
};

STDMETHODIMP CDsQuery::AddPages(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam)
{
    HRESULT hres;
    INT i;

    TraceEnter(TRACE_FORMS, "CDsQuery::AddPages");

    if ( !pAddPagesProc )
        ExitGracefully(hres, E_INVALIDARG, "No AddPagesProc");

    for ( i = 0 ; i < ARRAYSIZE(pages) ; i++ )
    {
        CQPAGE qp = { 0 };

        qp.cbStruct = SIZEOF(qp);
        qp.dwFlags = pages[i].dwFlags;
        qp.pPageProc = pages[i].pPageProc;
        qp.hInstance = GLOBAL_HINSTANCE;
        qp.idPageName = pages[i].idPageName;
        qp.idPageTemplate = pages[i].idPageTemplate;
        qp.pDlgProc = pages[i].pDlgProc;        

        hres = (*pAddPagesProc)(lParam, *pages[i].clisdForm, &qp);
        FailGracefully(hres, "Failed to add page (calling pAddPagesFunc)");        
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ IQueryHandler methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::Initialize(THIS_ IQueryFrame* pQueryFrame, DWORD dwOQWFlags, LPVOID pParameters)
{
    HRESULT hres;
    LPDSQUERYINITPARAMS pDsQueryInitParams = (LPDSQUERYINITPARAMS)pParameters;
    TCHAR szGUID[GUIDSTR_MAX];
    TCHAR szBuffer[MAX_PATH];
    HINSTANCE hInstanceComCtl32 = NULL;
    USES_CONVERSION;
  
    TraceEnter(TRACE_HANDLER, "CDsQuery::Initialize");

    // Keep the IQueryFrame interface, we need it for menu negotiation and other
    // view -> frame interactions.

    _pqf = pQueryFrame;
    _pqf->AddRef();

    _dwOQWFlags = dwOQWFlags;

    // If we have a parameter block then lets take copies of the interesting
    // fields from there.

    if ( pDsQueryInitParams )
    {
        _dwFlags = pDsQueryInitParams->dwFlags;

        // did the user specify a default scope?

        if ( pDsQueryInitParams->pDefaultScope && pDsQueryInitParams->pDefaultScope[0] )
        {
            Trace(TEXT("Default scope:"), W2T(pDsQueryInitParams->pDefaultScope));
            hres = LocalAllocStringW(&_pDefaultScope, pDsQueryInitParams->pDefaultScope);
            FailGracefully(hres, "Failed to cope default scope");
        }

        // default save location?

        if ( (_dwFlags & DSQPF_SAVELOCATION) && pDsQueryInitParams->pDefaultSaveLocation )
        {
            Trace(TEXT("Default save location:"), W2T(pDsQueryInitParams->pDefaultSaveLocation));
            hres = LocalAllocStringW(&_pDefaultSaveLocation, pDsQueryInitParams->pDefaultSaveLocation);
            FailGracefully(hres, "Failed to copy save location");
        }

        // do we have credential information?

        if ( _dwFlags & DSQPF_HASCREDENTIALS )
        {
            TraceMsg("Copying credential/server information from init params");

            if ( pDsQueryInitParams->pUserName )
            {
                hres = LocalAllocStringW(&_pUserName, pDsQueryInitParams->pUserName);
                FailGracefully(hres, "Failed to copy user name");
            }

            if ( pDsQueryInitParams->pPassword )
            {
                hres = LocalAllocStringW(&_pPassword, pDsQueryInitParams->pPassword);
                FailGracefully(hres, "Failed to copy password");
            }

            if ( pDsQueryInitParams->pServer )
            {
                hres = LocalAllocStringW(&_pServer, pDsQueryInitParams->pServer);
                FailGracefully(hres, "Failed to copy server");
            }

            Trace(TEXT("_pUserName : %s"), _pUserName ? W2T(_pUserName):TEXT("<not specified>"));
            Trace(TEXT("_pPassword : %s"), _pPassword ? W2T(_pPassword):TEXT("<not specified>"));
            Trace(TEXT("_pServer : %s"), _pServer ? W2T(_pServer):TEXT("<not specified>"));
        }
    }

    // Finally load the must structures that we are going to use, then modify them
    // based on the flags that the caller gave us.
    //
    // NB: removes the last two items from the file menu assumed to be the
    //     "save" and its seperator

    _hFileMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_MENU_FILE));
    _hEditMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_MENU_EDIT));
    _hViewMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_MENU_VIEW));
    _hHelpMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_MENU_HELP));

    if ( !_hFileMenu || !_hEditMenu || !_hViewMenu || !_hHelpMenu )
        ExitGracefully(hres, E_FAIL, "Failed to load resources for menus");

    if ( _dwFlags & DSQPF_NOSAVE )
    {
        HMENU hFileMenu = GetSubMenu(_hFileMenu, 0);
        INT i = GetMenuItemCount(hFileMenu);

        DeleteMenu(hFileMenu, i-1, MF_BYPOSITION);
        DeleteMenu(hFileMenu, i-2, MF_BYPOSITION);
    }

    // Init ComCtl32, including checking to see if we can use the filter control or not,
    // the filter control was added to the WC_HEADER32 in IE5, so check the DLL version
    // to see which we are using.

    InitCommonControls();

    hInstanceComCtl32 = GetModuleHandle(TEXT("comctl32"));
    TraceAssert(hInstanceComCtl32);

    if ( hInstanceComCtl32 )
    {
        DLLVERSIONINFO dllVersionInfo = { 0 };
        DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hInstanceComCtl32, "DllGetVersion");        
        TraceAssert(pfnDllGetVersion);

        dllVersionInfo.cbSize = SIZEOF(dllVersionInfo);

        if ( pfnDllGetVersion && SUCCEEDED(pfnDllGetVersion(&dllVersionInfo)) )
        {
            Trace(TEXT("DllGetVersion succeeded on ComCtl32, dwMajorVersion %08x"), dllVersionInfo.dwMajorVersion);
            _fFilterSupported = dllVersionInfo.dwMajorVersion >= 5;
        }
    }
    
    Trace(TEXT("_fFilterSupported is %d"), _fFilterSupported);

    hres = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::GetViewInfo(THIS_ LPCQVIEWINFO pViewInfo)
{
    HICON hIcon;

    TraceEnter(TRACE_HANDLER, "CDsQuery::GetViewInfo");

    pViewInfo->dwFlags      = 0;
    pViewInfo->hInstance    = GLOBAL_HINSTANCE;
    pViewInfo->idLargeIcon  = IDI_FIND;
    pViewInfo->idSmallIcon  = IDI_FIND;
    pViewInfo->idTitle      = IDS_WINDOWTITLE;
    pViewInfo->idAnimation  = IDR_DSFINDANIMATION;

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::AddScopes(THIS)
{
    HRESULT hres;
    DWORD dwThreadId;
    HANDLE hThread;
    LPSCOPETHREADDATA pstd = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_HANDLER, "CDsQuery::AddScopes");

    // Enumerate the rest of the scopes on a seperate thread to gather the
    // scopes we are interested in.

    pstd = (LPSCOPETHREADDATA)LocalAlloc(LPTR, SIZEOF(SCOPETHREADDATA));
    TraceAssert(pstd);

    if ( !pstd )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate scope data structure");

    _pqf->GetWindow(&pstd->hwndFrame);
    // pstd->pDefaultScope = NULL;

    // pstd->pServer = NULL;            // no credential stuff currently
    // pstd->pUserName = NULL;
    // pstd->pPassword = NULL;

    if ( _pDefaultScope )
    {
        hres = LocalAllocStringW(&pstd->pDefaultScope, _pDefaultScope);
        FailGracefully(hres, "Failed to copy the default scope");
    }

    hres = _CopyCredentials(&pstd->pUserName, &pstd->pPassword, &pstd->pServer);
    FailGracefully(hres, "Failed to copy credentails");

    InterlockedIncrement(&GLOBAL_REFCOUNT);

    hThread = CreateThread(NULL, 0, AddScopesThread, pstd, 0, &dwThreadId);
    TraceAssert(hThread);

    if ( !hThread )
    {
        InterlockedDecrement(&GLOBAL_REFCOUNT);
        ExitGracefully(hres, E_FAIL, "Failed to create background thread to enum scopes - BAD!");
    }

    CloseHandle(hThread);

    hres = S_OK;

exit_gracefully:

    if ( FAILED(hres) && pstd )
    {
        LocalFreeStringW(&pstd->pDefaultScope);
        LocalFree((HLOCAL)pstd);
    }

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

typedef struct
{
    IADsPathname *padp;
    WCHAR szGcPath[MAX_PATH];
} BROWSEFORSCOPE;

int CALLBACK _BrowseForScopeCB(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    HRESULT hres;
    INT iResult = 0;
    BROWSEFORSCOPE *pbfs = (BROWSEFORSCOPE*)lpData;
    LPTSTR pDirectoryName = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_HANDLER, "_BrowseForScopeCB");

    switch ( uMsg )
    {
        case DSBM_QUERYINSERT:
        {
            PDSBITEM pItem = (PDSBITEM)lParam;
            TraceAssert(pItem);

            // We are interested in modifying the root item of the tree, therefore
            // lets check for that being inserted, if it is then we change the
            // display name and the icon being shown.

            if ( pItem->dwState & DSBS_ROOT )
            {
                GetModuleFileName(GLOBAL_HINSTANCE, pItem->szIconLocation, ARRAYSIZE(pItem->szIconLocation));
                pItem->iIconResID = -IDI_GLOBALCATALOG;

                if ( SUCCEEDED(FormatDirectoryName(&pDirectoryName, GLOBAL_HINSTANCE, IDS_GLOBALCATALOG)) )
                {
                    StrCpyN(pItem->szDisplayName, pDirectoryName, DSB_MAX_DISPLAYNAME_CHARS);
                    LocalFreeString(&pDirectoryName);
                }

                pItem->dwMask |= DSBF_DISPLAYNAME|DSBF_ICONLOCATION;
                iResult = TRUE;
            }

            break;
        }

        case BFFM_SELCHANGED:
        {
            BOOL fEnableOK = TRUE;
            LPWSTR pszPath = (LPWSTR)lParam;
            LONG nElements = 0;

            // The user changes the selection in the browse dialog, therefore
            // lets see if we should be enabling the OK button.  If the user
            // selects GC, but we don't have a GC then we disable it.

            if ( SUCCEEDED(pbfs->padp->Set(pszPath, ADS_SETTYPE_FULL)) )
            {
                pbfs->padp->GetNumElements(&nElements);
                Trace(TEXT("nElements on exit from GetNumElements %d"), nElements);
            }

            if ( !nElements && !pbfs->szGcPath[0] )
            {
                TraceMsg("'entire directory' selected with NO GC!");
                fEnableOK = FALSE;
            }

            SendMessage(hwnd, BFFM_ENABLEOK, (WPARAM)fEnableOK, 0L);
            break;
        }

        case DSBM_HELP:
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    DSQUERY_HELPFILE,
                    HELP_WM_HELP,
                    (DWORD_PTR)aBrowseHelpIDs);
            break;
        }

        case DSBM_CONTEXTMENU:
        {
            WinHelp((HWND)lParam,
                    DSQUERY_HELPFILE,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)aBrowseHelpIDs);
            break;
        }
    }

    TraceLeaveValue(iResult);
}

STDMETHODIMP CDsQuery::BrowseForScope(THIS_ HWND hwndParent,  LPCQSCOPE pCurrentScope, LPCQSCOPE* ppScope)
{
    HRESULT hres;
    LPDSQUERYSCOPE pDsQueryScope = (LPDSQUERYSCOPE)pCurrentScope;
    BROWSEFORSCOPE bfs = { 0 };
    DSBROWSEINFO dsbi = { 0 };
    INT iResult;
    WCHAR szPath[2048];
    WCHAR szRoot[MAX_PATH+10];      // LDAP://
    WCHAR szObjectClass[64];
    LONG nElements;
    USES_CONVERSION;
    
    TraceEnter(TRACE_HANDLER, "CDsQuery::BrowseForScope");
    Trace(TEXT("hwndParent %08x, pCurrentScope %08x, ppScope %08x"), hwndParent, pCurrentScope, ppScope);

    *ppScope = NULL;                        // nothing yet!

    if ( SUCCEEDED(GetGlobalCatalogPath(_pServer, bfs.szGcPath, ARRAYSIZE(bfs.szGcPath))) )
        Trace(TEXT("GC path is: %s"), W2T(bfs.szGcPath));

    hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void **)&bfs.padp);
    FailGracefully(hres, "Failed to get the IADsPathname interface");

    // Fill out the browse info structure to display the object picker, if we have
    // enabled admin features then lets make all objects visible, otherwise
    // just the standard features.

    dsbi.cbStruct = SIZEOF(dsbi);
    dsbi.hwndOwner = hwndParent;
    dsbi.pszRoot = szRoot;
    dsbi.pszPath = szPath;
    dsbi.cchPath = ARRAYSIZE(szPath);
    dsbi.dwFlags = (DSBI_RETURNOBJECTCLASS|DSBI_EXPANDONOPEN|DSBI_ENTIREDIRECTORY) & ~DSBI_NOROOT;
    dsbi.pfnCallback = _BrowseForScopeCB;
    dsbi.lParam = (LPARAM)&bfs;
    dsbi.pszObjectClass = szObjectClass;
    dsbi.cchObjectClass = ARRAYSIZE(szObjectClass);

    if ( _dwFlags & DSQPF_SHOWHIDDENOBJECTS )
        dsbi.dwFlags |= DSBI_INCLUDEHIDDEN;

    FormatMsgResource((LPTSTR*)&dsbi.pszTitle, GLOBAL_HINSTANCE, IDS_BROWSEPROMPT);

    StrCpyW(szRoot, c_szLDAP);

    if ( _pServer )
    {
        if ( lstrlenW(_pServer) > MAX_PATH )
            ExitGracefully(hres, E_INVALIDARG, "_pServer is too big");

        StrCatW(szRoot, L"//");
        StrCatW(szRoot, _pServer);
    }

    if ( pDsQueryScope )
    {
        StrCpyNW(szPath, OBJECT_NAME_FROM_SCOPE(pDsQueryScope), ARRAYSIZE(szPath));
        Trace(TEXT("pDsQueryScope: %s"), W2T(szPath));
    }

    // copy the credential information if needed

    if ( _dwFlags & DSQPF_HASCREDENTIALS )
    {
        TraceMsg("Setting credentails information");
        dsbi.pUserName = _pUserName;
        dsbi.pPassword = _pPassword;
        dsbi.dwFlags |= DSBI_HASCREDENTIALS;
    }

    iResult = DsBrowseForContainer(&dsbi);
    Trace(TEXT("DsBrowseForContainer returns %d"), iResult);

    // iResult == IDOK if something was selected (szPath),
    // if it is -VE if the call failed and we should error

    if ( iResult == IDOK )
    {
        LPWSTR pszScope = szPath;
        LPWSTR pszObjectClass = szObjectClass;
        LONG nElements = 0;

        Trace(TEXT("Path on exit from DsBrowseForContainer: %s"), W2T(szPath));

        // does this look like the GC?  If so then default to it, as DsBrowseForContainer
        // will return us iffy looking information

        if ( SUCCEEDED(bfs.padp->Set(szPath, ADS_SETTYPE_FULL)) )
        {
            bfs.padp->GetNumElements(&nElements);
            Trace(TEXT("nElements on exit from GetNumElements %d"), nElements);
        }

        if ( !nElements )
        {
            TraceMsg("nElements = 0, so defaulting to GC");
            pszScope = bfs.szGcPath;
            pszObjectClass = GC_OBJECTCLASS;
        }

        Trace(TEXT("Scope selected is: %s, Object class: %s"), W2T(pszScope), W2T(pszObjectClass));

        hres = AllocScope(ppScope, 0, pszScope, pszObjectClass);
        FailGracefully(hres, "Failed converting the DS path to a scope");
    }
    else if ( iResult == IDCANCEL )
    {
        hres = S_FALSE;               // nothing selected, returning S_FALSE;
    }
    else if ( iResult < 0 )
    {
        ExitGracefully(hres, E_FAIL, "DsBrowseForContainer failed");
    }

exit_gracefully:

    LocalFreeString((LPTSTR*)&dsbi.pszTitle);
    Trace(TEXT("*ppScope == %08x"), *ppScope);

    DoRelease(bfs.padp);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

//
// WndProc for the banner window
//

LRESULT CALLBACK _BannerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch ( uMsg )
    {
        case WM_SIZE:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_ERASEBKGND:
            break;

        case WM_PAINT:
        {
            TCHAR szBuffer[MAX_PATH];
            HFONT hFont, hOldFont;
            SIZE szText;
            RECT rcClient;
            INT len;
            PAINTSTRUCT paint;
            COLORREF oldFgColor, oldBkColor;
    
            BeginPaint(hwnd, &paint);

            hFont = (HFONT)SendMessage(GetParent(hwnd), WM_GETFONT, 0, 0L);
            hOldFont = (HFONT)SelectObject(paint.hdc, hFont);

            if ( hOldFont )
            {
                oldFgColor = SetTextColor(paint.hdc, GetSysColor(COLOR_WINDOWTEXT));                    
                oldBkColor = SetBkColor(paint.hdc, ListView_GetBkColor(GetParent(hwnd)));

                len = GetWindowText(hwnd, szBuffer, ARRAYSIZE(szBuffer));

                GetTextExtentPoint32(paint.hdc, szBuffer, len, &szText);
                GetClientRect(GetParent(hwnd), &rcClient);
                
                ExtTextOut(paint.hdc, 
                           (rcClient.right - szText.cx) / 2, 
                           GetSystemMetrics(SM_CYBORDER)*4,
                           ETO_CLIPPED|ETO_OPAQUE, &rcClient, 
                           szBuffer, len,
                           NULL);

                SetTextColor(paint.hdc, oldFgColor);
                SetBkColor(paint.hdc, oldBkColor);

                SelectObject(paint.hdc, hOldFont);
            }

            EndPaint(hwnd, &paint);

            break;
        }

        case WM_SETTEXT:
        {
            InvalidateRect(hwnd, NULL, FALSE);
            //break;                                // deliberate drop through..
        }

        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }

    return lResult;
}

//
// WndProc for the bg window (lives behind list view, used by rest of the world)
//

LRESULT CALLBACK _ResultViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    CDsQuery* pDsQuery = NULL;

    if ( uMsg == WM_CREATE )
    {
        pDsQuery = (CDsQuery*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pDsQuery);
    }
    else
    {
        pDsQuery = (CDsQuery*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch ( uMsg )
        {
            case WM_SIZE:
                pDsQuery->OnSize(LOWORD(lParam), HIWORD(lParam));
                return(0);

            case WM_DESTROY:
                pDsQuery->_hwndView = NULL;         // view is gone!
                break;

            case WM_NOTIFY:
                return(pDsQuery->OnNotify(hwnd, wParam, lParam));

            case WM_SETFOCUS:
                SetFocus(pDsQuery->_hwndView);
                break;

            case WM_GETDLGCODE:
                return ((LRESULT)(DLGC_WANTARROWS | DLGC_WANTCHARS));

            case WM_CONTEXTMENU:
                pDsQuery->OnContextMenu(NULL, lParam);
                return TRUE;
        
            case DSQVM_ADDRESULTS:
                return SUCCEEDED(pDsQuery->OnAddResults((DWORD)wParam, (HDPA)lParam));
                                 
            case DSQVM_FINISHED:
                if ( (DWORD)wParam == pDsQuery->_dwQueryReference )
                {
                    // the references match so lets finish the query, and display 
                    // the "too many results" prompt if the user did a really
                    // big query and we chopped them off

                    pDsQuery->StopQuery();

                    if ( lParam )     // == 0 then we are OK!
                    {
                        HWND hwndFrame;
                        pDsQuery->_pqf->GetWindow(&hwndFrame);
                        FormatMsgBox(GetParent(hwndFrame),
                                     GLOBAL_HINSTANCE, IDS_WINDOWTITLE, IDS_ERR_MAXRESULT, 
                                     MB_OK|MB_ICONERROR);                        
                    }
                }
                SetFocus(pDsQuery->_hwndView);
                return(1);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

STDMETHODIMP CDsQuery::CreateResultView(THIS_ HWND hwndParent, HWND* phWndView)
{
    HRESULT hres;
    WNDCLASS wc;
    HWND hwndFilter, hwndFilterOld;
    HIMAGELIST himlSmall, himlLarge;
    DWORD dwLVStyle = LVS_AUTOARRANGE|LVS_SHAREIMAGELISTS|LVS_SHOWSELALWAYS|LVS_REPORT;
    RECT rc;

    TraceEnter(TRACE_HANDLER, "CDsQuery::CreateResultView");

    if ( IsWindow(_hwnd) )
        ExitGracefully(hres, E_FAIL, "Can only create one view at a time");

    // Create our result viewer, this is the parent window to the ListView
    // that we attach when we issue the query.
    
    ZeroMemory(&wc, SIZEOF(wc));
    wc.lpfnWndProc = _ResultViewWndProc;
    wc.hInstance =  GLOBAL_HINSTANCE;
    wc.lpszClassName = VIEW_CLASS;
    RegisterClass(&wc);

    _hwnd = CreateWindow(VIEW_CLASS, 
                          NULL,
                          WS_TABSTOP|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE,
                          0, 0, 0, 0,
                          hwndParent,
                          NULL,
                          GLOBAL_HINSTANCE,
                          this);
    if ( !_hwnd )
        ExitGracefully(hres, E_FAIL, "Failed to create view parent window");

    // Now register the window classes we are using.

    ZeroMemory(&wc, SIZEOF(wc));
    wc.lpfnWndProc = _BannerWndProc;
    wc.hInstance =  GLOBAL_HINSTANCE;
    wc.lpszClassName = BANNER_CLASS;
    RegisterClass(&wc);

    if ( _dwOQWFlags & OQWF_SINGLESELECT )
        dwLVStyle |= LVS_SINGLESEL;

    GetClientRect(_hwnd, &rc);
    _hwndView = CreateWindowEx(WS_EX_CLIENTEDGE,
                                WC_LISTVIEW,
                                NULL,
                                WS_TABSTOP|WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE|dwLVStyle,
                                0, 0, 
                                rc.right, rc.bottom,
                                _hwnd,
                                (HMENU)IDC_RESULTS,
                                GLOBAL_HINSTANCE,
                                NULL);
    if ( !_hwndView )
        ExitGracefully(hres, E_FAIL, "Failed to create the view window");

    ListView_SetExtendedListViewStyle(_hwndView, LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);
    
    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(_hwndView, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(_hwndView, himlSmall, LVSIL_SMALL);
    
    // Create the banner window, this is a child of the ListView, it is used to display
    // information about the query being issued

    _hwndBanner = CreateWindow(BANNER_CLASS, NULL,
                                WS_CHILD,
                                0, 0, 0, 0,               // nb: size fixed later
                                _hwndView,
                                (HMENU)IDC_STATUS, 
                                GLOBAL_HINSTANCE, 
                                NULL);
    if ( !_hwndBanner )
        ExitGracefully(hres, E_FAIL, "Failed to create the static banner window");

    _SetFilter(_fFilter);
    _SetViewMode(_idViewMode);
    _ShowBanner(SWP_SHOWWINDOW, IDS_INITALIZING);                

    hres = S_OK;                      // success

exit_gracefully:
    
    if ( SUCCEEDED(hres) )
        *phWndView = _hwnd;

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

#define MGW_EDIT 2

STDMETHODIMP CDsQuery::ActivateView(THIS_ UINT uState, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;
    HWND hwnd;
    INT i;
    
    TraceEnter(TRACE_HANDLER, "CDsQuery::ActivateView");

    switch ( uState )
    {
        case CQRVA_ACTIVATE:
        {
            HMENU hMenu;
            OLEMENUGROUPWIDTHS omgw = { 0, 0, 0, 0, 0, 0 };

            // Allow the cframe to merge its menus into our menu bar before we
            // add ours to it.  

            if ( !(hMenu = CreateMenu()) )
                ExitGracefully(hres, E_FAIL, "Failed to create a base menu bar to be used");

            hres = _pqf->InsertMenus(hMenu, &omgw);
            FailGracefully(hres, "Failed when calling CQueryFrame::InsertMenus");

            Shell_MergeMenus(GetSubMenu(hMenu, 0), GetSubMenu(_hFileMenu, 0), 0x0, 0x0, 0x7fff, 0);

            MergeMenu(hMenu, _hEditMenu, omgw.width[0]);
            MergeMenu(hMenu, _hViewMenu, omgw.width[0]+1);
            MergeMenu(hMenu, _hHelpMenu, omgw.width[0]+MGW_EDIT+omgw.width[2]+omgw.width[4]);

            if ( _dwOQWFlags & OQWF_SINGLESELECT )
            {
                ENABLE_MENU_ITEM(hMenu, DSQH_EDIT_SELECTALL, FALSE);
                ENABLE_MENU_ITEM(hMenu, DSQH_EDIT_INVERTSELECTION, FALSE);
            }

            hres = _pqf->SetMenu(hMenu, NULL);                           // set the frames menu bar
            FailGracefully(hres, "Failed when calling CQueryFrame::SetMenu");

            break;
        }

        case CQRVA_INITMENUBAR:
        {
            // we recieve a CQRVA_INITMENUBAR before the popup so that we can store the 
            // menu bar information, and invalidate an interface pointers we maybe holding
            // onto.

            Trace(TEXT("Received an CQRVA_INITMENUBAR, hMenu %08x"), wParam);

            _hFrameMenuBar = (HMENU)wParam;
            DoRelease(_pcm);

            break;
        }

        case CQRVA_INITMENUBARPOPUP:
        {
            HMENU hFileMenu;
            BOOL fDeleteItems = FALSE;

            TraceMsg("Received an CQRVA_INITMENUBARPOPUP");

            hFileMenu = GetSubMenu(_hFrameMenuBar, 0);

            // if we have a view then lets try and collect the selection from it,
            // having done that we can merge the verbs for that selection into the
            // views "File" menu.

            if ( (hFileMenu == (HMENU)wParam) && !_pcm )
            {
                _fNoSelection = TRUE;             // no selection currenlty

                if ( IsWindow(_hwndView) )
                {
                    for ( i = GetMenuItemCount(hFileMenu) - 1; i >= 0 ; i-- )
                    {
#if !DOWNLEVEL_SHELL
                        if ( !fDeleteItems && (GetMenuItemID(hFileMenu, i) == DSQH_FILE_CREATESHORTCUT) )
#else
                        if ( !fDeleteItems && (GetMenuItemID(hFileMenu, i) == DSQH_FILE_PROPERTIES) )
#endif
                        {
                            Trace(TEXT("Setting fDeleteItems true on index %d"), i);
                            fDeleteItems = TRUE;
                        }
                        else
                        {
                            if ( fDeleteItems )
                                DeleteMenu(hFileMenu, i, MF_BYPOSITION);
                        }
                    }

                    // Collect the selection, and using that construct an IContextMenu interface, if that works
                    // then we can merge in the verbs that relate to this object.

                    hres = _GetIDLsAndViewObject(FALSE, IID_IContextMenu, (void **)&_pcm);    
                    FailGracefully(hres, "Failed when calling _GetIDLsAndViewObject");

                    if ( ShortFromResult(hres) > 0 )
                    {
                        _GetContextMenuVerbs(_pcm, hFileMenu, CMF_VERBSONLY);
                        _fNoSelection = FALSE;
                    }
                }

                ENABLE_MENU_ITEM(hFileMenu, DSQH_FILE_CREATESHORTCUT, !_fNoSelection);
                ENABLE_MENU_ITEM(hFileMenu, DSQH_FILE_PROPERTIES,     !_fNoSelection);
            }

            ENABLE_MENU_ITEM(_hFrameMenuBar, DSQH_VIEW_PICKCOLUMNS, _hdsaColumns);
            ENABLE_MENU_ITEM(_hFrameMenuBar, DSQH_VIEW_REFRESH, _dwThreadId);
            
            _InitViewMenuItems(_hFrameMenuBar);       
            break;
        }

        case CQRVA_FORMCHANGED:
        {
            // we receieve a form change, we store the form name as we will use it
            // as the default name for saved queries authored by the user.

            Trace(TEXT("Form '%s' selected"), (LPTSTR)lParam);

            LocalFreeString(&_pDefaultSaveName);
            hres = LocalAllocString(&_pDefaultSaveName, (LPCTSTR)lParam);
            FailGracefully(hres, "Failed to set the default save name");

            break;
        }

        case CQRVA_STARTQUERY:
        {
            Trace(TEXT("Query is: %s"), wParam ? TEXT("starting"):TEXT("stopping"));
            break;
        }

        case CQRVA_HELP:
        {
            LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
            TraceAssert(pHelpInfo)

            TraceMsg("Invoking help on the objects in the windows");                
            WinHelp((HWND)pHelpInfo->hItemHandle, DSQUERY_HELPFILE, HELP_WM_HELP, (DWORD_PTR)aHelpIDs);

            break;
        }

        case CQRVA_CONTEXTMENU:
        {
            HWND hwndForHelp = (HWND)wParam;
            Trace(TEXT("CQRVA_CONTEXTMENU recieved on the bg of the frame %d"), GetDlgCtrlID(hwndForHelp));
            WinHelp(hwndForHelp, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aHelpIDs);
            break;
        }
    }
    
    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::InvokeCommand(THIS_ HWND hwndParent, UINT uID)
{
    HRESULT hres;
    HWND hwndFrame;
    DECLAREWAITCURSOR;

    TraceEnter(TRACE_HANDLER, "CDsQuery::InvokeCommand");
    Trace(TEXT("hwndParent %08x, uID %d"), hwndParent, uID);

    SetWaitCursor();

    switch ( uID )
    {
        case DSQH_BG_SELECT:
            SendMessage(hwndParent, WM_COMMAND, IDOK, 0);
            break;

        case DSQH_FILE_PROPERTIES:
            hres = OnFileProperties();
            break;

#if !DOWNLEVEL_SHELL
        case DSQH_FILE_CREATESHORTCUT:
            hres = OnFileCreateShortcut();
            break;
#endif

        case DSQH_FILE_SAVEQUERY:
            hres = OnFileSaveQuery();
            break;

        case DSQH_EDIT_SELECTALL:
            hres = OnEditSelectAll();
            break;

        case DSQH_EDIT_INVERTSELECTION:
            hres = OnEditInvertSelection();
            break;

        case DSQH_VIEW_FILTER:
            _SetFilter(!_fFilter);
            break;

        case DSQH_VIEW_LARGEICONS:
        case DSQH_VIEW_SMALLICONS:
        case DSQH_VIEW_LIST:
        case DSQH_VIEW_DETAILS:
            _SetViewMode(uID);
            break;
        
        case DSQH_VIEW_REFRESH:
        {
            if ( IsWindow(_hwndView) && _dwThreadId )
            {
                _InitNewQuery(NULL, FALSE);
                PostThreadMessage(_dwThreadId, RVTM_REFRESH, _dwQueryReference, 0L);
            }
            break;
        }

        case DSQH_VIEW_PICKCOLUMNS:
        {
            TraceAssert(_hdsaColumns);
            OnPickColumns(hwndParent);
            break;
        }

        case DSQH_HELP_CONTENTS:
        {
            TraceMsg("Calling for to display help topics");
            _pqf->GetWindow(&hwndFrame);
            _pqf->CallForm(NULL, DSQPM_HELPTOPICS, 0, (LPARAM)hwndFrame);
			break;
        }

        case DSQH_HELP_WHATISTHIS:
            _pqf->GetWindow(&hwndFrame);
            SendMessage(hwndFrame, WM_SYSCOMMAND, SC_CONTEXTHELP, MAKELPARAM(0,0)); 
            break;
            
        default:
        {
            // if it looks like a sort request then lets handle it, otherwise attempt
            // to send to the context menu handler we may have at htis poiunt.

            if ( (uID >= DSQH_VIEW_ARRANGEFIRST) && (uID < DSQH_VIEW_ARRANGELAST) )
            {
                TraceAssert(_hdsaColumns);
                if ( _hdsaColumns )
                {
                    Trace(TEXT("Calling _SortResults for column %d"), uID - DSQH_VIEW_ARRANGEFIRST);
                    _SortResults(uID - DSQH_VIEW_ARRANGEFIRST);
                }
            }
            else if ( _pcm )
            {       
                CMINVOKECOMMANDINFO ici;

                ici.cbSize = SIZEOF(ici);
                ici.fMask = 0;
                _pqf->GetWindow(&ici.hwnd);
                ici.lpVerb = (LPCSTR)(uID - DSQH_FILE_CONTEXT_FIRST);
                ici.lpParameters = NULL;
                ici.lpDirectory = NULL;
                ici.nShow = SW_NORMAL;
                ici.dwHotKey = 0;
                ici.hIcon = NULL;

                hres = _pcm->InvokeCommand(&ici);
                FailGracefully(hres, "Failed when calling IContextMenu::InvokeCommand");

                DoRelease(_pcm);                  // no longer needed
            }

            break;
        }
    }

exit_gracefully:

    ResetWaitCursor();

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::GetCommandString(THIS_ UINT uID, DWORD dwFlags, LPTSTR pBuffer, INT cchBuffer)
{
    HRESULT hres;
    TCHAR szBuffer[MAX_PATH];

    TraceEnter(TRACE_HANDLER, "CDsQuery::GetCommandString");
    Trace(TEXT("uID %08x, dwFlags %08x, pBuffer %08x, cchBuffer %d"), uID, dwFlags, pBuffer, cchBuffer);

    if ( (uID >= DSQH_FILE_CONTEXT_FIRST) && (uID < DSQH_FILE_CONTEXT_LAST) )
    {
        if ( _pcm )
        {
            TraceMsg("Trying the IContextMenu::GetCommandString");

            hres = _pcm->GetCommandString((uID - DSQH_FILE_CONTEXT_FIRST), GCS_HELPTEXT, NULL, (LPSTR)pBuffer, cchBuffer);
#if UNICODE
            // we build UNICODE, therefore if we failed then try and pick up the ANSI string from
            // the handler, if that works then we convert the multi-byte string to UNICODE 
            // and hand that back to the caller.

            if ( FAILED(hres) )
            {
                CHAR szBuffer[MAX_PATH];
                
                hres = _pcm->GetCommandString((uID - DSQH_FILE_CONTEXT_FIRST), GCS_HELPTEXTA, NULL, szBuffer, ARRAYSIZE(szBuffer));

                if ( SUCCEEDED(hres) )
                {
                    TraceMsg("Handler provided an ANSI string");
                    MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, pBuffer, cchBuffer);
                }
            }
#endif
            FailGracefully(hres, "Failed when asking for help text from IContextMenu iface");
        }
    }
    else
    {
        if ( (uID >= DSQH_VIEW_ARRANGEFIRST) && (uID < DSQH_VIEW_ARRANGELAST) )
        {
            INT iColumn = uID-DSQH_VIEW_ARRANGEFIRST;
            TCHAR szFmt[MAX_PATH];

            Trace(TEXT("Get command text for column %d"), iColumn);

            if ( _hdsaColumns && (iColumn < DSA_GetItemCount(_hdsaColumns)) )
            {
                LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, iColumn);
                TraceAssert(pColumn);

                LoadString(GLOBAL_HINSTANCE, IDS_ARRANGEBY_HELP, szFmt, ARRAYSIZE(szFmt));
                wsprintf(pBuffer, szFmt, pColumn->pHeading);

                Trace(TEXT("Resulting string is: %s"), pBuffer);
            }
        }
        else
        {
            if ( !LoadString(GLOBAL_HINSTANCE, uID, pBuffer, cchBuffer) )
                ExitGracefully(hres, E_FAIL, "Failed to load the command text for this verb");
        }
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::IssueQuery(THIS_ LPCQPARAMS pQueryParams)
{
    HRESULT hres;
    LPTHREADINITDATA ptid = NULL;
    LPDSQUERYSCOPE pDsQueryScope = (LPDSQUERYSCOPE)pQueryParams->pQueryScope;
    LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)pQueryParams->pQueryParameters;
    LPTSTR pBuffer = NULL;
    MSG msg;

    TraceEnter(TRACE_HANDLER, "CDsQuery::IssueQuery");
    Trace(TEXT("pQueryParams %08x, pDsQueryScope %08x, pDsQueryParams %08x"), pQueryParams, pDsQueryScope, pDsQueryParams);    

    // Persist the existing column information if there was some, then 
    // get the new column table initialized and the columns added to the
    // view

    if ( _hdsaColumns )
    {
        if ( _fColumnsModified )
        {
            _SaveColumnTable(_clsidForm, _hdsaColumns);
            _fColumnsModified = FALSE;
        }

        _SaveColumnTable();       
    }

    // Initialize the view with items
    
    _clsidForm = pQueryParams->clsidForm;          // keep the form ID (for persistance)

    hres = _InitNewQuery(pDsQueryParams, TRUE);
    FailGracefully(hres, "Failed to initialize the new query");

    // Now build the thread information needed to get the thread
    // up and running.

    ptid = (LPTHREADINITDATA)LocalAlloc(LPTR, SIZEOF(THREADINITDATA));
    TraceAssert(ptid);

    if ( !ptid )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate THREADINITDATA");

    ptid->dwReference = _dwQueryReference;
    //ptid->hwndView = NULL;
    //ptid->pQuery = NULL;
    //ptid->pScope = NULL;
    //ptid->hdsaColumns = NULL;
    //ptid->fShowHidden = FALSE;

    //ptid->pServer = NULL;
    //ptid->pUserName = NULL;
    //ptid->pPassword = NULL;

    Trace(TEXT("_dwFlags %08x (& DSQPF_SHOWHIDDENOBJECTS)"), _dwFlags, _dwFlags & DSQPF_SHOWHIDDENOBJECTS);

    ptid->fShowHidden = (_dwFlags & DSQPF_SHOWHIDDENOBJECTS) ? 1:0;
    ptid->hwndView = _hwndView;

    hres = _GetColumnTable(_clsidForm, pDsQueryParams, &ptid->hdsaColumns, FALSE);
    FailGracefully(hres, "Failed to create column DSA");

    hres = LocalAllocStringW(&ptid->pQuery, (LPWSTR)ByteOffset(pDsQueryParams, pDsQueryParams->offsetQuery));
    FailGracefully(hres, "Failed to copy query filter string");

    hres = LocalAllocStringW(&ptid->pScope, OBJECT_NAME_FROM_SCOPE(pDsQueryScope));
    FailGracefully(hres, "Failed to copy scope to thread init data");

    hres = _CopyCredentials(&ptid->pUserName, &ptid->pPassword, &ptid->pServer);
    FailGracefully(hres, "Failed to copy credentails");

    // now create the thread that is going to perform the query, this includes
    // telling the previous one that it needs to close down

    if ( _hThread && _dwThreadId )
    {
        Trace(TEXT("Killing old query thread %08x, ID %d"), _hThread, _dwThreadId);

        PostThreadMessage(_dwThreadId, RVTM_STOPQUERY, 0, 0);
        PostThreadMessage(_dwThreadId, WM_QUIT, 0, 0);

        CloseHandle(_hThread);

        _hThread = NULL;
        _dwThreadId = 0;
    }

    InterlockedIncrement(&GLOBAL_REFCOUNT);

    _hThread = CreateThread(NULL, 0, QueryThread, ptid, 0, &_dwThreadId);
    TraceAssert(_hThread);

    if ( !_hThread )
    {
        InterlockedDecrement(&GLOBAL_REFCOUNT);
        ExitGracefully(hres, E_FAIL, "Failed to create background thread - BAD!");
    }

    hres = S_OK;                      // success

exit_gracefully:

    if ( SUCCEEDED(hres) && IsWindow(_hwndView) )
        SetFocus(_hwndView);

    if ( FAILED(hres) )
    {
        QueryThread_FreeThreadInitData(&ptid);
        _pqf->StartQuery(FALSE);
    }

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::StopQuery(THIS)
{
    HRESULT hres;
    INT cResults = _hdpaResults ? DPA_GetPtrCount(_hdpaResults):0;
    LPTSTR pBuffer;

    TraceEnter(TRACE_HANDLER, "CDsQuery::StopQuery");

    if ( !IsWindow(_hwndView) )
        ExitGracefully(hres, E_FAIL, "View not initalized yet");

    // we are stopping the query, we are going to tidy up the UI now
    // and we just want the thread to closedown cleanly, therefore lets
    // do so, increasing our query reference

    _pqf->StartQuery(FALSE);
    _dwQueryReference++;

    _PopulateView(-1, -1);                // update status bar etc

    if ( _dwThreadId )
        PostThreadMessage(_dwThreadId, RVTM_STOPQUERY, 0, 0);

    hres = S_OK;              // success

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

HRESULT _SetDataObjectData(IDataObject* pDataObject, UINT cf, LPVOID pData, DWORD cbSize)
{
    HRESULT hres;
    FORMATETC fmte = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = { TYMED_NULL, NULL, NULL };
    LPVOID pAlloc;

    TraceEnter(TRACE_HANDLER, "_SetDataObjectData");

    hres = AllocStorageMedium(&fmte, &medium, cbSize, &pAlloc);
    FailGracefully(hres, "Failed to allocate STGMEDIUM for data");

    CopyMemory(pAlloc, pData, cbSize);

    hres = pDataObject->SetData(&fmte, &medium, TRUE);
    FailGracefully(hres, "Failed to pass the data to the IDataObject");

    hres = S_OK;

exit_gracefully:

    ReleaseStgMedium(&medium);

    TraceLeaveResult(hres);
}

STDMETHODIMP CDsQuery::GetViewObject(THIS_ UINT uScope, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    IDataObject* pDataObject = NULL;
    LPDSQUERYPARAMS pDsQueryParams = NULL;
    LPDSQUERYSCOPE pDsQueryScope = NULL;
    UINT cfDsQueryParams = RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);
    UINT cfDsQueryScope = RegisterClipboardFormat(CFSTR_DSQUERYSCOPE);
    BOOL fJustSelection = !(_dwFlags & DSQPF_RETURNALLRESULTS);

    TraceEnter(TRACE_HANDLER, "CDsQuery::GetViewObject");

    // We only support returning the selection as an IDataObject

    DECLAREWAITCURSOR;
    SetWaitCursor();

    if ( !ppvOut && ((uScope & CQRVS_MASK) != CQRVS_SELECTION) )
        ExitGracefully(hres, E_INVALIDARG, "Bad arguments to GetViewObject");

    if ( !IsEqualIID(riid, IID_IDataObject) )
        ExitGracefully(hres, E_NOINTERFACE, "Object IID supported");

    //
    // write the extra data we have into the IDataObject:
    //
    //  - query parameters (filter)
    //  - scope
    //  - attribute prefix information
    //

    hres = _GetIDLsAndViewObject(fJustSelection, IID_IDataObject, (void **)&pDataObject);
    FailGracefully(hres, "Failed to get the IDataObject from the namespace");

    if ( SUCCEEDED(_pqf->CallForm(NULL, CQPM_GETPARAMETERS, 0, (LPARAM)&pDsQueryParams)) )
    {
        if ( pDsQueryParams )
        {
            hres = _SetDataObjectData(pDataObject, cfDsQueryParams, pDsQueryParams, pDsQueryParams->cbStruct);
            FailGracefully(hres, "Failed set the DSQUERYPARAMS into the data object");
        }
    }

    if ( SUCCEEDED(_pqf->GetScope((LPCQSCOPE*)&pDsQueryScope)) )
    {
        if ( pDsQueryScope )
        {
            LPWSTR pScope = OBJECT_NAME_FROM_SCOPE(pDsQueryScope);
            TraceAssert(pScope);

            hres = _SetDataObjectData(pDataObject, cfDsQueryScope, pScope, StringByteSizeW(pScope));
            FailGracefully(hres, "Failed set the DSQUERYSCOPE into the data object");
        }
    }

    // success, so lets pass out the IDataObject.

    pDataObject->AddRef();
    *ppvOut = (LPVOID)pDataObject;

    hres = S_OK;

exit_gracefully:

    DoRelease(pDataObject);
    
    if ( pDsQueryParams )
        CoTaskMemFree(pDsQueryParams);

    if ( pDsQueryScope )
        CoTaskMemFree(pDsQueryScope);

    ResetWaitCursor();

    TraceLeaveResult(hres);
}   

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::LoadQuery(THIS_ IPersistQuery* pPersistQuery)
{
    HRESULT hres;
    WCHAR szBuffer[MAX_PATH];
    IADs *pDsObject = NULL;
    BSTR bstrObjectClass = NULL;
    INT iFilter;
    LPCQSCOPE pScope = NULL;
    INT cbScope;
    USES_CONVERSION;

    TraceEnter(TRACE_HANDLER, "CDsQuery::LoadQuery");
    
    if ( !pPersistQuery )
        ExitGracefully(hres, E_INVALIDARG, "No IPersistQuery object");

    if ( SUCCEEDED(pPersistQuery->ReadInt(c_szDsQuery, c_szScopeSize, &cbScope)) &&
         (cbScope < SIZEOF(szBuffer)) &&
         SUCCEEDED(pPersistQuery->ReadStruct(c_szDsQuery, c_szScope, szBuffer, cbScope)) )
    {
        Trace(TEXT("Selected scope from file is %s"), W2T(szBuffer));

        // get the object class from the file - this should be written to the file

        hres = ADsOpenObject(szBuffer, _pUserName, _pPassword, ADS_SECURE_AUTHENTICATION, IID_IADs, (void **)&pDsObject);
        FailGracefully(hres, "Failed to bind to the specified object");

        hres = pDsObject->get_Class(&bstrObjectClass);
        FailGracefully(hres, "Failed to get the object class");

        // allocate a new scope

        if ( SUCCEEDED(AllocScope(&pScope, 0, szBuffer, bstrObjectClass)) )
        {
            hres = _pqf->AddScope(pScope, 0x0, TRUE);
            FailGracefully(hres, "Failed to add scope to list");
        }
    }

    // Read the remainder of the view state

    if ( SUCCEEDED(pPersistQuery->ReadInt(c_szDsQuery, c_szViewMode, &_idViewMode)) )
    {
        Trace(TEXT("View mode is: %0x8"), _idViewMode);
        _SetViewMode(_idViewMode);
    }

    if ( SUCCEEDED(pPersistQuery->ReadInt(c_szDsQuery, c_szEnableFilter, &iFilter)) )
    {
        Trace(TEXT("Filter mode set to %d"), _fFilter);
        _SetFilter(iFilter);
    }

    hres = S_OK;

exit_gracefully:

    if ( pScope )
        CoTaskMemFree(pScope);

    DoRelease(pDsObject);
    SysFreeString(bstrObjectClass);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::SaveQuery(THIS_ IPersistQuery* pPersistQuery, LPCQSCOPE pScope)
{
    HRESULT hres;
    LPDSQUERYSCOPE pDsQueryScope = (LPDSQUERYSCOPE)pScope;
    LPWSTR pScopePath = OBJECT_NAME_FROM_SCOPE(pDsQueryScope);
    WCHAR szGcPath[MAX_PATH];
    
    TraceEnter(TRACE_HANDLER, "CDsQuery::SaveQuery");

    if ( !pPersistQuery || !pScope )
        ExitGracefully(hres, E_INVALIDARG, "No IPersistQuery/pScope object");

    if ( SUCCEEDED(GetGlobalCatalogPath(_pServer, szGcPath, ARRAYSIZE(szGcPath))) && StrCmpW(pScopePath, szGcPath) ) 
    {
        // if this is not the GC then persist

        TraceMsg("GC path differs from scope, so persisting");

        hres = pPersistQuery->WriteInt(c_szDsQuery, c_szScopeSize, StringByteSizeW(pScopePath));
        FailGracefully(hres, "Failed to write the scope size");

        hres = pPersistQuery->WriteStruct(c_szDsQuery, c_szScope, pScopePath, StringByteSizeW(pScopePath));
        FailGracefully(hres, "Failed to write scope");
    }

    hres = pPersistQuery->WriteInt(c_szDsQuery, c_szViewMode, _idViewMode);
    FailGracefully(hres, "Failed to write view mode");

    hres = pPersistQuery->WriteInt(c_szDsQuery, c_szEnableFilter, _fFilter);
    FailGracefully(hres, "Failed to write filter state");

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*----------------------------------------------------------------------------
/ IObjectWithSite
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsQuery::SetSite(IUnknown* punk)
{
    HRESULT hres = S_OK;

    TraceEnter(TRACE_HANDLER, "CDsQuery::SetSite");

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

STDMETHODIMP CDsQuery::GetSite(REFIID riid, void **ppv)
{
    HRESULT hres;
    
    TraceEnter(TRACE_HANDLER, "CDsQuery::GetSite");

    if ( !_punkSite )
        ExitGracefully(hres, E_NOINTERFACE, "No site to QI from");

    hres = _punkSite->QueryInterface(riid, ppv);
    FailGracefully(hres, "QI failed on the site unknown object");

exit_gracefully:

    TraceLeaveResult(hres);
}


/*----------------------------------------------------------------------------
/ IDsQueryHandler
/----------------------------------------------------------------------------*/

VOID CDsQuery::_DeleteViewItems(LPDSOBJECTNAMES pdon)
{
    INT iResult;
    DWORD iItem;
    USES_CONVERSION;

    TraceEnter(TRACE_HANDLER, "CDsQuery::_DeleteObjectNames");

    if ( pdon->cItems )
    {
        // walk through all the items in the view deleting as required.

        for ( iItem = 0 ; iItem != pdon->cItems ; iItem++ )
        {
            // do we have an item to delete?

            if ( pdon->aObjects[iItem].offsetName )
            {
                LPCWSTR pwszName = (LPCWSTR)ByteOffset(pdon, pdon->aObjects[iItem].offsetName);
                Trace(TEXT("pwszName to delete: %s"), W2CT(pwszName));

                // walk all the results in the view deleting them as we go.

                for ( iResult = 0 ; iResult < DPA_GetPtrCount(_hdpaResults); iResult++ )
                {
                    LPQUERYRESULT pResult = (LPQUERYRESULT)DPA_GetPtr(_hdpaResults, iResult);
                    TraceAssert(pResult);

                    // if we match the item we want to delete then remove it, if the view
                    // is not filtered then remove ite from the list, otherwise leave the 
                    // view update until we have finished deleting

                    if ( !StrCmpW(pwszName, pResult->pPath) )
                    {
                        Trace(TEXT("Item maps to result %d in the list"), iResult);
                        
                        FreeQueryResult(pResult, DSA_GetItemCount(_hdsaColumns));
                        DPA_DeletePtr(_hdpaResults, iResult); 
                        
                        if ( !_fFilter )
                        {
                            TraceMsg("Deleting the item from the view");
                            ListView_DeleteItem(_hwndView, iResult);
                        }
                    }
                }
            }
        }

        // the view was filtered, so lets repopulate with the items

        if ( _fFilter )
        {
            TraceMsg("View is filter, therefore just forcing a refresh");
            _FilterView(FALSE);    
        }
    }        

    TraceLeave();
}


STDMETHODIMP CDsQuery::UpdateView(DWORD dwType, LPDSOBJECTNAMES pdon)
{
    HRESULT hres;

    TraceEnter(TRACE_HANDLER, "CDsQuery::UpdateView");

    switch ( dwType & DSQRVF_OPMASK )
    {
        case DSQRVF_ITEMSDELETED:
        {
            if ( !pdon )
                ExitGracefully(hres, E_INVALIDARG, "Invlaidate pdon specified for refresh");

            _DeleteViewItems(pdon);
            break;
        }

        default:
            ExitGracefully(hres, E_INVALIDARG, "Invalidate refresh type speciifed");
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Message/Command Handlers
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ CDsQuery::OnSize
/ ----------------
/   Result viewer is being sized, so ensure that our children have their
/   sizes correctly addjusted.
/
/ In:
/   cx, cy = new size of the parent window
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
LRESULT CDsQuery::OnSize(INT cx, INT cy)
{
    TraceEnter(TRACE_VIEW, "CDsQuery::OnSize");

    SetWindowPos(_hwndView, NULL, 0, 0, cx, cy, SWP_NOZORDER|SWP_NOMOVE);
    _ShowBanner(0, 0);

    TraceLeaveValue(0);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnNotify
/ ------------------
/   Notify message being recieved by the view, so try and handle it as best
/   we can.
/
/ In:
/   hWnd = window handle of the notify
/   wParam, lParam = parameters for the notify event
/
/ Out:
/   LRESULT
/----------------------------------------------------------------------------*/
LRESULT CDsQuery::OnNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres;
    LRESULT lr = 0;
    DECLAREWAITCURSOR = GetCursor();
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "CDsQuery::OnNotify");

    switch ( ((LPNMHDR)lParam)->code )
    {
        case HDN_FILTERCHANGE:
            _FilterView(TRUE);
            break;

        case HDN_FILTERBTNCLICK:
        {
            NMHDFILTERBTNCLICK* pNotify = (NMHDFILTERBTNCLICK*)lParam;
            HMENU hMenu;
            POINT pt;
            HD_ITEM hdi;
            UINT uID;
            
            if ( _hdsaColumns && (pNotify->iItem < DSA_GetItemCount(_hdsaColumns)) )
            {
                LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, pNotify->iItem);
                TraceAssert(pColumn);                   

                hMenu = LoadMenu(GLOBAL_HINSTANCE, property_type_table[pColumn->iPropertyType].pMenuName);
                TraceAssert(hMenu);

                if ( hMenu )
                {
                    pt.x = pNotify->rc.right;
                    pt.y = pNotify->rc.bottom;
                    MapWindowPoints(pNotify->hdr.hwndFrom, NULL, &pt, 1);

                    CheckMenuRadioItem(GetSubMenu(hMenu, 0), 
                                       FILTER_FIRST, FILTER_LAST, pColumn->idOperator, 
                                       MF_BYCOMMAND);

                    uID = TrackPopupMenu(GetSubMenu(hMenu, 0),
                                         TPM_RIGHTALIGN|TPM_RETURNCMD,  
                                         pt.x, pt.y,
                                         0, pNotify->hdr.hwndFrom, NULL);                  
                    switch ( uID )
                    {
                        case DSQH_CLEARFILTER:
                            Header_ClearFilter(ListView_GetHeader(_hwndView), pNotify->iItem);
                            break;


                        case DSQH_CLEARALLFILTERS:
                            Header_ClearAllFilters(ListView_GetHeader(_hwndView));
                            break;
                        
                        default:
                        {
                            if ( uID && (uID != pColumn->idOperator) )
                            {
                                // update the filter string based on the new operator
                                pColumn->idOperator = uID;              
                                _GetFilterValue(pNotify->iItem, NULL);
                                lr = TRUE;
                            }
                            break;
                        }
                    }

                    DestroyMenu(hMenu);
                }       
            }

            break;
        }

        case HDN_ITEMCHANGED:
        {
            HD_NOTIFY* pNotify = (HD_NOTIFY*)lParam;
            HD_ITEM* pitem = (HD_ITEM*)pNotify->pitem;
        
            if ( _hdsaColumns && (pNotify->iItem < DSA_GetItemCount(_hdsaColumns)) )
            {
                LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, pNotify->iItem);
                TraceAssert(pColumn);

                // store the new column width information in the column structure and
                // mark the column table as dirty

                if ( pitem->mask & HDI_WIDTH )
                {
                   Trace(TEXT("Column %d, cx %d (marking state as dirty)"), pNotify->iItem, pitem->cxy);
                    pColumn->cx = pitem->cxy;
                    _fColumnsModified = TRUE;
                }
            
                if ( pitem->mask & HDI_FILTER )
                {
                    Trace(TEXT("Filter for column %d has been changed"), pNotify->iItem);
                    _GetFilterValue(pNotify->iItem, pitem);
                }
            }

            break;
        }

        case LVN_GETDISPINFO:
        {
            LV_DISPINFO* pNotify = (LV_DISPINFO*)lParam;
            TraceAssert(pNotify);

            if ( pNotify && (pNotify->item.mask & LVIF_TEXT) && pNotify->item.lParam )
            {
                LPQUERYRESULT pResult = (LPQUERYRESULT)pNotify->item.lParam;
                INT iColumn = pNotify->item.iSubItem;

                pNotify->item.pszText[0] = TEXT('\0');          // nothing to display yet

                switch ( pResult->aColumn[iColumn].iPropertyType )
                {
                    case PROPERTY_ISUNDEFINED:
                        break;

                    case PROPERTY_ISUNKNOWN:
                    case PROPERTY_ISSTRING:
                    {
                        if ( pResult->aColumn[iColumn].pszText )
                            StrCpyN(pNotify->item.pszText, pResult->aColumn[iColumn].pszText, pNotify->item.cchTextMax);

                        break;
                    }
                        
                    case PROPERTY_ISNUMBER:
                    case PROPERTY_ISBOOL:
                        wsprintf(pNotify->item.pszText, TEXT("%d"), pResult->aColumn[iColumn].iValue);
                        break;
                }

                lr = TRUE;          // we formatted a value
            }

            break;
        }

        case LVN_ITEMACTIVATE:
        {
            LPNMHDR pNotify = (LPNMHDR)lParam;
            DWORD dwFlags = CMF_NORMAL;
            HWND hwndFrame;
            HMENU hMenu;
            UINT uID;

            // convert the current selection to IDLITs and an IContextMenu interface
            // that we can then get the default verb from.

            SetWaitCursor();
            DoRelease(_pcm);

            hres = _GetIDLsAndViewObject(FALSE, IID_IContextMenu, (void **)&_pcm);
            FailGracefully(hres, "Failed when calling _GetIDLsAndViewObject");

            _fNoSelection = !ShortFromResult(hres);

            if ( !_fNoSelection )
            {
                // create a popup menu pickup the context menu for the current selection
                // and then pass it down to the invoke command handler.

                hMenu = CreatePopupMenu();
                TraceAssert(hMenu);

                if ( hMenu )
                {
                    if ( GetKeyState(VK_SHIFT) < 0 )
                        dwFlags |= CMF_EXPLORE;          // SHIFT + dblclick does a Explore by default

                    _GetContextMenuVerbs(_pcm, hMenu, dwFlags);

                    uID = GetMenuDefaultItem(hMenu, MF_BYCOMMAND, 0);
                    Trace(TEXT("Default uID after double click %08x"), uID);

                    if ( uID != -1 )
                    {
                        _pqf->GetWindow(&hwndFrame);                
                        InvokeCommand(hwndFrame, uID);
                    }

                    DoRelease(_pcm);          // no longer needed
                    DestroyMenu(hMenu);
                }
            }

            break;
        }

        case LVN_COLUMNCLICK:
        {
            NM_LISTVIEW* pNotify = (NM_LISTVIEW*)lParam;
            TraceAssert(pNotify);
            _SortResults(pNotify->iSubItem);
            break;
        }

        default:
            lr = DefWindowProc(hWnd, WM_NOTIFY, wParam, lParam);
            break;
    }

exit_gracefully:

    ResetWaitCursor();

    TraceLeaveValue(lr);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnAddResults
/ ----------------------
/   The background thread has sent us some results, so lets add them to
/   the DPA of results, discarding the ones we don't add because we cannot
/   grow the DPA.
/
/   dwQueryReference conatins the reference ID for this query, only add
/   results where these match.
/
/ In:
/   dwQueryReference = reference that this block is for
/   hdpaResults = DPA containing the results to add
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::OnAddResults(DWORD dwQueryReference, HDPA hdpaResults)
{
    HRESULT hres;
    INT i, iPopulateFrom;

    TraceEnter(TRACE_VIEW, "CDsQuery::OnAddResults");

    if ( (dwQueryReference != _dwQueryReference) || !hdpaResults )
        ExitGracefully(hres, E_FAIL, "Failed to add results, bad DPA/reference ID");

    // the caller gives us a DPA then we add them to our result DPA, we then
    // update the view populating from the first item we added.

    iPopulateFrom = DPA_GetPtrCount(_hdpaResults);

    for ( i = DPA_GetPtrCount(hdpaResults); --i >= 0 ;  )
    {
        LPQUERYRESULT pResult = (LPQUERYRESULT)DPA_GetPtr(hdpaResults, i);
        TraceAssert(pResult);

        // add the result to the main DPA, if that fails then ensure we nuke
        // this result blob!
    
        if ( -1 == DPA_AppendPtr(_hdpaResults, pResult) )
            FreeQueryResult(pResult, DSA_GetItemCount(_hdsaColumns));

        DPA_DeletePtr(hdpaResults, i);          // remove from result DPA
    }

    _PopulateView(iPopulateFrom, DPA_GetPtrCount(_hdpaResults));

    TraceAssert(DPA_GetPtrCount(hdpaResults) == 0);
    DPA_Destroy(hdpaResults);

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnContextMenu
/ -----------------------
/   The user has right clicked in the result view, therefore we must attempt
/   to display the context menu for those objects
/
/ In:
/   hwndMenu = window that that the user menued over
/   pt = point to show the context menu
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
LRESULT CDsQuery::OnContextMenu(HWND hwndMenu, LPARAM lParam)
{
    HRESULT hres;
    HMENU hMenu = NULL;
    POINT pt = { 0, 0 };
    INT i;
    RECT rc;
    HWND hwndFrame;

    TraceEnter(TRACE_VIEW, "CDsQuery::OnContextMenu");

    // Collect the selection, obtaining a IContextMenu interface pointer, or a HR == S_FALSE
    // if there is no selection for us to be using.

    DoRelease(_pcm);

    hres = _GetIDLsAndViewObject(FALSE, IID_IContextMenu, (void **)&_pcm);
    FailGracefully(hres, "Failed when calling _GetIDLsAndViewObject");

    _fNoSelection = !ShortFromResult(hres);

    if ( !(hMenu = CreatePopupMenu()) )
        ExitGracefully(hres, E_FAIL, "Failed to create the popup menu");

    if ( !_fNoSelection )
    {
        // pick up the context menu that maps tot he current selection, including fixing
        // the "select" verb if we need one.

        _GetContextMenuVerbs(_pcm, hMenu, CMF_NORMAL);
    }
    else
    {
        // There is no selection so lets pick up the view bg menu, this contains
        // some useful helpers for modifying the view state.

        HMENU hBgMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_VIEWBACKGROUND));

        if ( !hBgMenu )
            ExitGracefully(hres, E_FAIL, "Failed to load pop-up menu for the background");

        Shell_MergeMenus(hMenu, GetSubMenu(hBgMenu, 0), 0, 0, CQID_MAXHANDLERMENUID, 0x0);
        DestroyMenu(hBgMenu);

        _InitViewMenuItems(hMenu);
    }

    // if lParam == -1 then we know that the user hit the "context menu" key
    // so lets set the co-ordinates of the item.

    if ( lParam == (DWORD)-1 )
    {
        i = ListView_GetNextItem(_hwndView, -1, LVNI_FOCUSED|LVNI_SELECTED);
        Trace(TEXT("Item with focus + selection: %d"), i);

        if ( i == -1 )
        {
            i = ListView_GetNextItem(_hwndView, -1, LVNI_SELECTED);
            Trace(TEXT("1st selected item: %D"), i);
        }            

        if ( i != -1 )
        {
            TraceMsg("We have an item, so getting bounds of the icon for position");
            ListView_GetItemRect(_hwndView, i, &rc, LVIR_ICON);
            pt.x = (rc.left+rc.right)/2;
            pt.y = (rc.top+rc.bottom)/2;
        }

        MapWindowPoints(_hwndView, HWND_DESKTOP, &pt, 1);      // they are in client co-ordinates
    }
    else
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
    }
    
    // we have the position so lets use it

    _pqf->GetWindow(&hwndFrame);
    TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hwndFrame, NULL);

exit_gracefully:

    if ( hMenu )
        DestroyMenu(hMenu);

    TraceLeaveValue(0);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnFileProperties
/ --------------------------
/   Show properties for the given selection.  To do this we CoCreate 
/   IDsFolderProperties on the IDsFolder implementation and that
/   we can invoke properties using.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::OnFileProperties(VOID)
{
    HRESULT hres;
    IDataObject* pDataObject = NULL;
    IDsFolderProperties* pDsFolderProperties = NULL;

    TraceEnter(TRACE_VIEW, "CDsQuery::OnFileProperties");

    hres = GetViewObject(CQRVS_SELECTION, IID_IDataObject, (void **)&pDataObject);
    FailGracefully(hres, "Failed to get IDataObject for shortcut creation");

    hres = CoCreateInstance(CLSID_DsFolderProperties, NULL, CLSCTX_INPROC_SERVER, IID_IDsFolderProperties, (void **)&pDsFolderProperties);
    FailGracefully(hres, "Failed to get IDsFolderProperties for the desktop object");

    hres = pDsFolderProperties->ShowProperties(_hwnd, pDataObject);
    FailGracefully(hres, "Failed to invoke property UI for the given selection");

    // hres = S_OK;                  // success

exit_gracefully:
    
    DoRelease(pDataObject);
    DoRelease(pDsFolderProperties);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnFileCreateShortcut
/ ------------------------------
/   Get the selection as a IDataObject then call the shell for it to
/   create shortucts to the objects.  These shortcuts are placed onto
/   the desktop.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

#if !DOWNLEVEL_SHELL

HRESULT CDsQuery::OnFileCreateShortcut(VOID)
{
    HRESULT hres;
    IDataObject* pDataObject = NULL;

    TraceEnter(TRACE_VIEW, "CDsQuery::OnFileCreateShortcut");

    hres = GetViewObject(CQRVS_SELECTION, IID_IDataObject, (void **)&pDataObject);
    FailGracefully(hres, "Failed to get IDataObject for shortcut creation");

    hres = SHCreateLinks(_hwnd, NULL, pDataObject, SHCL_USETEMPLATE | SHCL_USEDESKTOP | SHCL_CONFIRM, NULL);
    FailGracefully(hres, "Failed when calling SHCreateLinks");

    // hres = S_OK;                  // success

exit_gracefully:
    
    DoRelease(pDataObject);

    TraceLeaveResult(hres);
}

#endif


/*-----------------------------------------------------------------------------
/ CDsQuery::OnFileSaveQuery
/ -------------------------
/   Allow the user to choose a location to save the query (initial directory
/   is nethood).  Having done that we then start the save process by passing
/   the frame object a IQueryIO object that allows them to persist the
/   query into.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::OnFileSaveQuery(VOID)
{
    HRESULT hres;
    OPENFILENAME ofn;
    TCHAR szFilename[MAX_PATH];
    TCHAR szDirectory[MAX_PATH];
    TCHAR szFilter[64];
    TCHAR szTitle[64];
    LPTSTR pFilter;
    CDsPersistQuery* pPersistQuery = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "CDsQuery::OnFileSaveQuery");

    // Load the default strings and fix up the filter string as it needs
    // NULL's seperating the various resource sections.

    LoadString(GLOBAL_HINSTANCE, IDS_SAVETITLE, szTitle, ARRAYSIZE(szTitle));
    StrCpy(szFilename, _pDefaultSaveName);
    LoadString(GLOBAL_HINSTANCE, IDS_SAVEFILTER, szFilter, ARRAYSIZE(szFilter));

    for ( pFilter = szFilter ; *pFilter ; pFilter++ )
    {
        if ( *pFilter == TEXT('\n') )
            *pFilter = TEXT('\0');
    }

    // fix the open filename structure ready to do our save....

    ZeroMemory(&ofn, SIZEOF(ofn));

    ofn.lStructSize = SIZEOF(ofn);
    _pqf->GetWindow(&ofn.hwndOwner);
    ofn.hInstance = GLOBAL_HINSTANCE;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = ARRAYSIZE(szFilename);

    if ( _pDefaultSaveLocation )
    {
        Trace(TEXT("Saving into: %s"), W2T(_pDefaultSaveLocation));
        StrCpy(szDirectory, W2T(_pDefaultSaveLocation));
        ofn.lpstrInitialDir = szDirectory;
    }

    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("dsq");

    // If we get a save filename then lets ensure that we delete the previous
    // query saved there (if there is one) and then we can create an IPersistQuery
    // object that will save to that location.

    if ( GetSaveFileName(&ofn) )
    {
        Trace(TEXT("Saving query as: %s"), szFilename);

        if ( !DeleteFile(szFilename) && (GetLastError() != ERROR_FILE_NOT_FOUND) )
            ExitGracefully(hres, E_FAIL, "Failed to delete previous query");

        pPersistQuery = new CDsPersistQuery(szFilename);
        if ( !pPersistQuery )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate CDsPersistQuery");

        hres = _pqf->SaveQuery(pPersistQuery);
        FailGracefully(hres, "Failed when calling IQueryFrame::SaveSearch");
    }   

    hres = S_OK;

exit_gracefully:

    DoRelease(pPersistQuery);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnEditSelectAll
/ -------------------------
/   Walk all the items in the view setting their selected state.    
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::OnEditSelectAll(VOID)
{
    TraceEnter(TRACE_VIEW, "CDsQuery::OnEditSelectAll");

    for ( INT i = ListView_GetItemCount(_hwndView) ; --i >= 0 ; )
    {
        ListView_SetItemState(_hwndView, i, LVIS_SELECTED, LVIS_SELECTED);
    }

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnEditInvertSelection
/ -------------------------------
/   Walk all the items in the view and invert their selected state.
/
/ In:
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::OnEditInvertSelection(VOID)
{
    TraceEnter(TRACE_VIEW, "CDsQuery::OnEditInvertSelection");

    for ( INT i = ListView_GetItemCount(_hwndView) ; --i >= 0 ; )
    {
        DWORD dwState = ListView_GetItemState(_hwndView, i, LVIS_SELECTED);
        ListView_SetItemState(_hwndView, i, dwState ^ LVIS_SELECTED, LVIS_SELECTED); 
    }

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_InitNewQuery
/ ----------------------
/   Initialize the view ready for a new query, including setting the frame
/   into a query running state.
/
/ In:
/   pDsQueryParams = DSQUERYPARAMs structure used to issue the query.
/   fRefreshColumnTable = re-read the column table from the params/registry etc
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_InitNewQuery(LPDSQUERYPARAMS pDsQueryParams, BOOL fRefreshColumnTable)
{
    HRESULT hres;
    LPTSTR pBuffer;

    TraceEnter(TRACE_VIEW, "CDsQuery::_InitNewQuery");

    hres = _pqf->StartQuery(TRUE);
    TraceAssert(SUCCEEDED(hres));

    // Claim the cached DS object for this window

    // if refreshing the column table then _GetColumnTable handles this all for us,
    // otherwise lets just nuke the result set ourselves.

    if ( fRefreshColumnTable )
    {
        _SaveColumnTable();

        hres = _GetColumnTable(_clsidForm, pDsQueryParams, &_hdsaColumns, TRUE);
        FailGracefully(hres, "Failed to create column DSA");
    }
    else
    {
        _FreeResults();
    }

    // initialize the view to start the query running, display the prompt banner and
    // initialize the result DPA.

    _ShowBanner(SWP_SHOWWINDOW, IDS_SEARCHING);      // we are now searching

    if ( SUCCEEDED(FormatMsgResource(&pBuffer, GLOBAL_HINSTANCE, IDS_SEARCHING)) )
    {
        _pqf->SetStatusText(pBuffer);
        LocalFreeString(&pBuffer);
    }

    TraceAssert(_hdpaResults==NULL);           // should never catch

    _hdpaResults = DPA_Create(16);
    TraceAssert(_hdpaResults);

    if ( !_hdpaResults )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate result DPA");

    _dwQueryReference++;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_GetFilterValue
/ ------------------------
/   Given a column index collect the filter value from it, note that
/   when doing this
/
/ In:
/   i = column to retrieve
/   pitem -> HD_ITEM structure for the current filter / == NULL then read from header
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_GetFilterValue(INT i, HD_ITEM* pitem)
{
    HRESULT hres;
    HD_ITEM hdi;
    HD_TEXTFILTER textFilter;
    TCHAR szBuffer[MAX_PATH];
    INT iValue;
    UINT cchFilter = 0;
    LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, i);
    TraceAssert(pColumn);

    TraceEnter(TRACE_VIEW, "CDsQuery::_GetFilterValue");

    // if pitem == NULL then lets pick up the filter value from the 
    // header control, using the stored property type (the filter one
    // has already been nuked) to defined which filter we want.

    if ( !pitem )
    {
        hdi.mask = HDI_FILTER;

        switch ( pColumn->iPropertyType )
        {
            case PROPERTY_ISUNKNOWN:
            case PROPERTY_ISSTRING:
            {
                hdi.type = HDFT_ISSTRING;
                hdi.pvFilter = &textFilter;
                textFilter.pszText = szBuffer;
                textFilter.cchTextMax = ARRAYSIZE(szBuffer);
                break;
            }

            case PROPERTY_ISNUMBER:
            case PROPERTY_ISBOOL:
            {
                hdi.type = HDFT_ISNUMBER;
                hdi.pvFilter = &iValue;
                break;
            }
        }

        if ( !Header_GetItem(ListView_GetHeader(_hwndView), i, &hdi) )
            ExitGracefully(hres, E_FAIL, "Failed to get the filter string");

        pitem = &hdi;
    }    

    // discard the previous filter value and lets read from the
    // structure the information we need to cache our filter information

    FreeColumnValue(&pColumn->filter);              

    if ( !(pitem->type & HDFT_HASNOVALUE) && pitem->pvFilter )
    {
        switch ( pitem->type & HDFT_ISMASK )
        {
            case HDFT_ISSTRING:
            {
                LPHD_TEXTFILTER ptextFilter = (LPHD_TEXTFILTER)pitem->pvFilter;
                TraceAssert(ptextFilter);

                pColumn->filter.iPropertyType = PROPERTY_ISSTRING;

                // text filters are stored in their wildcarded state, therefore
                // filtering doesn't require converting from the text form
                // to something more elobrate each pass through.  the down
                // side is when the operator changes we must rebuild the
                // filter string for that column (small price)

                GetPatternString(NULL, &cchFilter, pColumn->idOperator, ptextFilter->pszText);
                TraceAssert(cchFilter != 0);

                if ( cchFilter )
                {
                    hres = LocalAllocStringLen(&pColumn->filter.pszText, cchFilter);
                    FailGracefully(hres, "Failed to allocate buffer to read string into");

                    GetPatternString(pColumn->filter.pszText, &cchFilter, pColumn->idOperator, ptextFilter->pszText);
                    Trace(TEXT("Filter (with pattern info): %s"), pColumn->filter.pszText);

                    LCMapString(0x0, LCMAP_UPPERCASE, pColumn->filter.pszText, -1, pColumn->filter.pszText, cchFilter+1);
                    Trace(TEXT("After converting to uppercase (LCMapString): %s"), pColumn->filter.pszText);
                }

                break;
            }

            case HDFT_ISNUMBER:
            {
                INT* piFilter = (INT*)pitem->pvFilter;
                TraceAssert(piFilter);

                pColumn->filter.iPropertyType = PROPERTY_ISNUMBER;
                pColumn->filter.iValue = *piFilter;
                Trace(TEXT("Filter: %d"), pColumn->filter.iValue);

                break;
            }
        }
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_FilterView
/ --------------------
/   Filter the result set populating the view again with the changes
/
/ In:
/   fCheck
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

UINT _GetFilter(HDSA hdsaColumns, LPTSTR pBuffer, UINT* pcchBuffer)
{
    INT i;
    TCHAR szBuffer[MAX_PATH];

    TraceEnter(TRACE_VIEW, "_GetFilter");
    TraceAssert(hdsaColumns && pcchBuffer);

    *pcchBuffer = 0;

    // form the string containin [operatorID]value pairs for each of the
    // filter columns that is defined.

    for ( i = 0 ; i < DSA_GetItemCount(hdsaColumns); i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(hdsaColumns, i);
        TraceAssert(pColumn);

        if ( pColumn->filter.iPropertyType != PROPERTY_ISUNDEFINED )
        {
            wsprintf(szBuffer, TEXT("[%d]"), pColumn->idOperator);
            PutStringElement(pBuffer, pcchBuffer, szBuffer);

            switch ( pColumn->filter.iPropertyType )
            {
                case PROPERTY_ISUNDEFINED:
                    break;

                case PROPERTY_ISUNKNOWN:
                case PROPERTY_ISSTRING:
                    PutStringElement(pBuffer, pcchBuffer, pColumn->filter.pszText);
                    break;

                case PROPERTY_ISNUMBER:
                case PROPERTY_ISBOOL:
                    wsprintf(szBuffer, TEXT("%d"), pColumn->filter.iValue);
                    PutStringElement(pBuffer, pcchBuffer, szBuffer);
                    break;
            }
        }
    }

    Trace(TEXT("pBuffer contains: %s (%d)"), pBuffer ? pBuffer:TEXT("<NULL>"), *pcchBuffer);

    TraceLeaveValue(*pcchBuffer);
}

HRESULT CDsQuery::_FilterView(BOOL fCheck)
{
    HRESULT hres;
    LPTSTR pFilter = NULL;
    UINT cchFilter;

    TraceEnter(TRACE_VIEW, "CDsQuery::_FilterView");
    
    if ( !_hdpaResults )
        ExitGracefully(hres, S_OK, "FitlerView bailing, no results");

    DECLAREWAITCURSOR;
    SetWaitCursor();                // this could take some time
   
    // get the current filter string, this consists of the filter
    // information from all the columns

    if ( _GetFilter(_hdsaColumns, NULL, &cchFilter) )
    {
        hres = LocalAllocStringLen(&pFilter, cchFilter);
        FailGracefully(hres, "Failed to allocate filter string");

        _GetFilter(_hdsaColumns, pFilter, &cchFilter);
    }

    // if the filters don't match then re-populate the view,
    // as the criteria for the results has changed

    if ( !fCheck ||
            (!pFilter || !_pFilter) ||
                (pFilter && _pFilter && StrCmpI(pFilter, _pFilter)) )
    {
        LPTSTR pBuffer;

        TraceMsg("Filtering the view, filters differ");

        ListView_DeleteAllItems(_hwndView);
        _ShowBanner(SWP_SHOWWINDOW, IDS_FILTERING);

        if ( SUCCEEDED(FormatMsgResource(&pBuffer, GLOBAL_HINSTANCE, IDS_FILTERING)) )
        {
            _pqf->SetStatusText(pBuffer);
            LocalFreeString(&pBuffer);
        }
    
        _PopulateView(0, DPA_GetPtrCount(_hdpaResults));
    }

    // ensure we hang onto the new filter, discarding the previous one

    LocalFreeString(&_pFilter);
    _pFilter = pFilter;                   

exit_gracefully:

    ResetWaitCursor();

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_PopulateView
/ ----------------------
/   Add items from the result DPA to the view filtering as required.  The 
/   caller gives us the start index (0 if all) and we walk the results
/   adding them to the view.
/
/ In:
/   iItem = first item to add / == 0 first / == -1 then add none, just update status
/   iLast = last item to be updated
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_PopulateView(INT iItem, INT iLast)
{
    HRESULT hres;
    BOOL fBannerShown = IsWindowVisible(_hwndBanner);
    LPTSTR pBuffer = NULL;
    LV_ITEM lvi;
    INT iColumn, i;
    INT iVisible = 0;
    INT iHidden = 0;
    BOOL fIncludeItem;
    MSG msg;
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "CDsQuery::_PopulateView");
    Trace(TEXT("Range %d to %d"), iItem, iLast);

    if ( iItem > -1 )
    {    
        Trace(TEXT("Adding items %d to %d"), iItem, DPA_GetPtrCount(_hdpaResults));

        lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
        lvi.iItem = 0x7fffffff;
        lvi.iSubItem = 0;
        lvi.pszText = LPSTR_TEXTCALLBACK;

        // Walk the results in the range we need to add and add them to the view
        // applying the filter to remove items we are not interested in.

        for ( i = 0; iItem < iLast ; i++, iItem++ )
        {
            LPQUERYRESULT pResult = (LPQUERYRESULT)DPA_GetPtr(_hdpaResults, iItem);
            TraceAssert(pResult);

            fIncludeItem = TRUE;                // new items always get included

            // if the filter is visilbe then lets walk it removing items from the list
            // of results.  fIncludeItem starts as TRUE and after the filter
            // loop should become either TRUE/FALSE.  All columns are ANDed together
            // therefore the logic is quite simple.

            if ( _fFilter )
            {
                for ( iColumn = 0 ; fIncludeItem && (iColumn < DSA_GetItemCount(_hdsaColumns)); iColumn++ )
                {
                    LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, iColumn);
                    TraceAssert(pColumn);

                    // if the column has a filter defined (!PROPERTY_ISUNDEFINED) then
                    // check that the properties match, if they don't then skip the
                    // itmer otherwise lets try applying the filter based on
                    // the type.

                    if ( pColumn->filter.iPropertyType == PROPERTY_ISUNDEFINED )
                        continue;                

                    if ( pResult->aColumn[iColumn].iPropertyType == PROPERTY_ISUNDEFINED )
                    {
                        // column is undefined therefore lets ignore it, it won't
                        // match the criteria
                        fIncludeItem = FALSE;
                    }
                    else
                    {
                        switch ( pColumn->filter.iPropertyType ) 
                        {
                            case PROPERTY_ISUNDEFINED:
                                break;

                            case PROPERTY_ISUNKNOWN:
                            case PROPERTY_ISSTRING:
                            {
                                TCHAR szBuffer[MAX_PATH];
                                LPTSTR pszBuffer = NULL;
                                LPTSTR pszValue = pResult->aColumn[iColumn].pszText;
                                INT cchValue = lstrlen(pszValue);
                                LPTSTR pszValueUC = szBuffer;

                                // the filter value is stored in uppercase, so to ensure we are case insensitive
                                // we must convert the string to uppercase.  We have a buffer we will use, 
                                // however, if the value is too large we will allocate a buffer we can use.

                                if ( cchValue > ARRAYSIZE(szBuffer) )
                                {
                                    TraceMsg("Value too big for our static buffer, so allocating");
                                    
                                    if ( FAILED(LocalAllocStringLen(&pszBuffer, cchValue)) )
                                    {
                                        TraceMsg("Failed to allocate a buffer for the string, so ignoring it!");
                                        fIncludeItem = FALSE;
                                        break;
                                    }

                                    pszValueUC = pszBuffer;              // fix the pointer to the new string
                                }

                                LCMapString(0x0, LCMAP_UPPERCASE, pszValue, -1, pszValueUC, cchValue+1);
                                Trace(TEXT("After converting to uppercase (LCMapString): %s"), pszValueUC);

                                // string properties need to be compared using the match filter
                                // function, this code is given the filter and the result
                                // and we must compare, in return we get TRUE/FALSE, therefore
                                // catch the NOT cases specificly.

                                switch ( pColumn->idOperator ) 
                                {
                                    case FILTER_CONTAINS:
                                    case FILTER_STARTSWITH:
                                    case FILTER_ENDSWITH:
                                    case FILTER_IS:
                                        fIncludeItem = MatchPattern(pszValueUC, pColumn->filter.pszText);
                                        break;

                                    case FILTER_NOTCONTAINS:
                                    case FILTER_ISNOT:
                                        fIncludeItem = !MatchPattern(pszValueUC, pColumn->filter.pszText);
                                        break;
                                }

                                LocalFreeString(&pszBuffer);        // ensure we don't leak, in thise case it would be costly!

                                break;
                            }

                            case PROPERTY_ISBOOL:
                            case PROPERTY_ISNUMBER:
                            {
                                // numeric properties are handled only as ints, therefore
                                // lets compare the numeric value we have

                                switch ( pColumn->idOperator ) 
                                {
                                    case FILTER_IS:
                                        fIncludeItem = (pColumn->filter.iValue == pResult->aColumn[iColumn].iValue);
                                        break;

                                    case FILTER_ISNOT:
                                        fIncludeItem = (pColumn->filter.iValue != pResult->aColumn[iColumn].iValue);
                                        break;

                                    case FILTER_GREATEREQUAL:
                                        fIncludeItem = (pColumn->filter.iValue <= pResult->aColumn[iColumn].iValue);
                                        break;

                                    case FILTER_LESSEQUAL:
                                        fIncludeItem = (pColumn->filter.iValue >= pResult->aColumn[iColumn].iValue);
                                        break;
                                }

                                break;
                            }
                        }
                    }
                }
            }
            
            Trace(TEXT("fInclude item is %d"), fIncludeItem);

            if ( fIncludeItem )
            {
                // we are going to add the item to the view, so lets hide the banner
                // if it is shown, then add a list view item.  The list view
                // item has text-callback and the lParam -> pResult structure
                // we are using.
                //
                // also, if the view hasn't had the image list set then lets
                // take care of that now

                if ( fBannerShown )
                {
                    TraceMsg("Adding an item and banner visible, therefore hiding");
                    _ShowBanner(SWP_HIDEWINDOW, 0);         // hide the banner
                    fBannerShown = FALSE;
                }

                lvi.lParam = (LPARAM)pResult;
                lvi.iImage = pResult->iImage;
                ListView_InsertItem(_hwndView, &lvi);

                if ( i % FILTER_UPDATE_COUNT )
                    UpdateWindow(_hwndView);
            }
        }
    }

    // lets update the status bar to reflect the world around us

    TraceAssert(_hdpaResults);

    if ( _hdpaResults )
    {
        iVisible = ListView_GetItemCount(_hwndView);
        iHidden = DPA_GetPtrCount(_hdpaResults)-iVisible;
    }

    if ( iVisible <= 0 )
    {
        _ShowBanner(SWP_SHOWWINDOW, IDS_NOTHINGFOUND);                
    }
    else
    {
        // ensure that at least one item in the view has focus

        if ( -1 == ListView_GetNextItem(_hwndView, -1, LVNI_FOCUSED) )
            ListView_SetItemState(_hwndView, 0, LVIS_FOCUSED, LVIS_FOCUSED);
    }
    
    if ( SUCCEEDED(FormatMsgResource(&pBuffer, 
                        GLOBAL_HINSTANCE, iHidden ? IDS_FOUNDITEMSHIDDEN:IDS_FOUNDITEMS,
                            iVisible, iHidden)) )
    {
        Trace(TEXT("Setting status text to: %s"), pBuffer);
        _pqf->SetStatusText(pBuffer);
        LocalFreeString(&pBuffer);
    }

    _iSortColumn = -1;                                 // sort is no longer valid!

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_FreeResults
/ ---------------------
/   Do the tidy up to release the results from the view.  This includes
/   destroying the DPA and removing the items from the list view.
/
/ In:
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CDsQuery::_FreeResults(VOID)
{
    TraceEnter(TRACE_VIEW, "CDsQuery::_FreeResults");

    if ( IsWindow(_hwndView) )
        ListView_DeleteAllItems(_hwndView);

    if ( _hdpaResults )
    {
        DPA_DestroyCallback(_hdpaResults, FreeQueryResultCB, (LPVOID)DSA_GetItemCount(_hdsaColumns));
        _hdpaResults = NULL;
    }

    LocalFreeString(&_pFilter);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_SetViewMode
/ ---------------------
/   Convert the command ID of the view mode into a LVS_ style bit that can
/   then we applied to the view.
/
/ In:
/   uID = view mode to be selected
/
/ Out:
/   DWORD = LVS_ style for this view mode
/----------------------------------------------------------------------------*/
DWORD CDsQuery::_SetViewMode(INT uID)
{
    const DWORD dwIdToStyle[] = { LVS_ICON, LVS_SMALLICON, LVS_LIST, LVS_REPORT };
    DWORD dwResult = 0;
    DWORD dwStyle;
    
    TraceEnter(TRACE_HANDLER|TRACE_VIEW, "CDsQuery::_SetViewMode");
    Trace(TEXT("Setting view mode to %08x"), uID);

    _idViewMode = uID;
    uID -= DSQH_VIEW_LARGEICONS;

    if ( uID < ARRAYSIZE(dwIdToStyle ) )
    {
        dwResult = dwIdToStyle[uID];

        if ( IsWindow(_hwndView) )
        {
            dwStyle = GetWindowLong(_hwndView, GWL_STYLE);

            if ( ( dwStyle & LVS_TYPEMASK ) != dwResult )
            {
                TraceMsg("Changing view style to reflect new mode");
                SetWindowLong(_hwndView, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK)|dwResult);
            }
        }
    }

    _ShowBanner(0, 0);

    TraceLeaveValue(dwResult);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_SetFilter
/ -------------------
/   Sets the visible state of the filter, refreshing the view as required.
/   
/   NB: To ensure that the ListView correctly refrehes its contents we first
/       remove the header from the view, then toggle the filter state
/       and re-enable the banner.   
/ In:
/   fFilter = flag indicating the filter state
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
VOID CDsQuery::_SetFilter(BOOL fFilter)
{
    TraceEnter(TRACE_HANDLER|TRACE_VIEW, "CDsQuery::_SetFilter");

    _fFilter = fFilter;                // store the new filter value

    if ( IsWindow(_hwndView) )
    {
        HWND hwndHeader = ListView_GetHeader(_hwndView);
        DWORD dwStyle = GetWindowLong(hwndHeader, GWL_STYLE) & ~(HDS_FILTERBAR|WS_TABSTOP);

        SetWindowLong(_hwndView, GWL_STYLE, GetWindowLong(_hwndView, GWL_STYLE) | LVS_NOCOLUMNHEADER);

        if ( _fFilter )
            dwStyle |= HDS_FILTERBAR|WS_TABSTOP;

        SetWindowLong(hwndHeader, GWL_STYLE, dwStyle);
        SetWindowLong(_hwndView, GWL_STYLE, GetWindowLong(_hwndView, GWL_STYLE) & ~LVS_NOCOLUMNHEADER);

        _ShowBanner(0, 0);        

        if ( _hdpaResults )
        {
            if ( (_fFilter && _pFilter) ||
                    (!_fFilter && (DPA_GetPtrCount(_hdpaResults) != ListView_GetItemCount(_hwndView))) )
            {
                _FilterView(FALSE);
            }
        }
    }
    
    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_SortResults
/ ---------------------
/   Sort the view given a column, handles either being clicked or
/   invoked on from an API.
/
/ In:
/   iColumn = column to sort on
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

INT _ResultSortCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParam)
{
    LPQUERYRESULT pResult1, pResult2;
    INT iColumn = LOWORD(lParam);

    // if lParam != 0 then we are reverse sorting, therefore swap
    // over the object pointers

    if ( !HIWORD(lParam) )
    {
        pResult1 = (LPQUERYRESULT)lParam1;
        pResult2 = (LPQUERYRESULT)lParam2;
    }
    else
    {
        pResult2 = (LPQUERYRESULT)lParam1;
        pResult1 = (LPQUERYRESULT)lParam2;
    }

    if ( pResult1 && pResult2 )
    {
        LPCOLUMNVALUE pColumn1 = (LPCOLUMNVALUE)&pResult1->aColumn[iColumn];
        LPCOLUMNVALUE pColumn2 = (LPCOLUMNVALUE)&pResult2->aColumn[iColumn];
        BOOL fHasColumn1 = pColumn1->iPropertyType != PROPERTY_ISUNDEFINED;
        BOOL fHasColumn2 = pColumn2->iPropertyType != PROPERTY_ISUNDEFINED;

        // check that both properties are defined, if they are not then return the 
        // comparison based on that field.  then we check that the properties
        // are the same type, if that matches then lets compare based on the
        // type. 

        if ( !fHasColumn1 || !fHasColumn2 )
        {
            return fHasColumn1 ? -1:+1;
        }
        else
        {
            TraceAssert(pColumn1->iPropertyType == pColumn2->iPropertyType);

            switch ( pColumn1->iPropertyType )
            {
                case PROPERTY_ISUNDEFINED:
                    break;

                case PROPERTY_ISUNKNOWN:
                case PROPERTY_ISSTRING:
                    return StrCmpI(pColumn1->pszText, pColumn2->pszText);

                case PROPERTY_ISBOOL:
                case PROPERTY_ISNUMBER:
                    return pColumn1->iValue - pColumn2->iValue;
            }
        }
    }
    
    return 0;
}

VOID CDsQuery::_SortResults(INT iColumn)
{
    DECLAREWAITCURSOR;
    
    TraceEnter(TRACE_VIEW, "CDsQuery::_SortResults");
    Trace(TEXT("iColumn %d"), iColumn);

    if ( (iColumn >= 0) && (iColumn < DSA_GetItemCount(_hdsaColumns)) )
    {
        // if we have already hit the column then lets invert the sort order,
        // there is no indicator to worry about so this should just work out
        // fine.  if we haven't used this column before then default oto
        // ascending, then do the sort!
        //
        // ensure that the focused item is visible when the sort has completed
  
        if ( _iSortColumn == iColumn )
            _fSortDescending = !_fSortDescending;
        else
            _fSortDescending = FALSE;

        _iSortColumn = iColumn;

        Trace(TEXT("Sorting on column %d, %s"), 
                _iSortColumn, _fSortDescending ? TEXT("(descending)"):TEXT("(ascending)"));

        SetWaitCursor();
        ListView_SortItems(_hwndView, _ResultSortCB, MAKELPARAM(_iSortColumn, _fSortDescending));
        ResetWaitCursor();
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_ShowBanner
/ --------------------
/   Show the views banner, including sizing it to obscure only the top section
/   of the window.
/
/ In:
/   uFlags = flags to combine when calling SetWindowPos
/   idPrompt = resource ID of prompt text ot be displayed
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CDsQuery::_ShowBanner(UINT uFlags, UINT idPrompt)
{
    HRESULT hres;
    WINDOWPOS wpos;
    RECT rcClient;
    HD_LAYOUT hdl;
    TCHAR szBuffer[MAX_PATH];

    TraceEnter(TRACE_VIEW, "CDsQuery::_ShowBanner");

    // if we have a resource id then lets load the string and
    // set the window text to have it

    if ( idPrompt )
    {
        LoadString(GLOBAL_HINSTANCE, idPrompt, szBuffer, ARRAYSIZE(szBuffer));
        SetWindowText(_hwndBanner, szBuffer);
    }

    // now position the window back to real location, this we need to
    // talk to the listview/header control to work out exactly where it
    // should be living

    GetClientRect(_hwndView, &rcClient);

    if ( (GetWindowLong(_hwndView, GWL_STYLE) & LVS_TYPEMASK) == LVS_REPORT )
    {
        TraceMsg("Calling header for layout information");

        wpos.hwnd = ListView_GetHeader(_hwndView);
        wpos.hwndInsertAfter = NULL;
        wpos.x = 0;
        wpos.y = 0;
        wpos.cx = rcClient.right;
        wpos.cy = rcClient.bottom;
        wpos.flags = SWP_NOZORDER;

        hdl.prc = &rcClient;
        hdl.pwpos = &wpos;

        if ( !Header_Layout(wpos.hwnd, &hdl) )
            ExitGracefully(hres, E_FAIL, "Failed to get the layout information (HDM_LAYOUT)");
    }

    SetWindowPos(_hwndBanner,
                 HWND_TOP, 
                 rcClient.left, rcClient.top, 
                 rcClient.right - rcClient.left, 100,
                 uFlags);    

exit_gracefully:

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_InitViewMenuItems
/ ---------------------------
/   Setup the view menu based on the given view mode and the filter state, enabled
/   disable the items as required.
/
/ In:
/   hMenu = menu to set the menu items on
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CDsQuery::_InitViewMenuItems(HMENU hMenu)
{
    MENUITEMINFO mii;
    HMENU hArrangeMenu;
    INT i;

    TraceEnter(TRACE_HANDLER|TRACE_VIEW, "CDsQuery::_InitViewMenuItems");

    CheckMenuItem(hMenu, DSQH_VIEW_FILTER,  MF_BYCOMMAND| (_fFilter ? MF_CHECKED:0) );   
    ENABLE_MENU_ITEM(hMenu, DSQH_VIEW_FILTER, _fFilterSupported && (_idViewMode == DSQH_VIEW_DETAILS));

    CheckMenuRadioItem(hMenu, DSQH_VIEW_LARGEICONS, DSQH_VIEW_DETAILS, _idViewMode, MF_BYCOMMAND);

    // construct the arrange menu, add it to the view menu that we have been given.

    hArrangeMenu = CreatePopupMenu();
    TraceAssert(hArrangeMenu);

    if ( _hdsaColumns && DSA_GetItemCount(_hdsaColumns) )
    {
        TCHAR szFmt[32];
        TCHAR szBuffer[MAX_PATH];
        
        hArrangeMenu = CreatePopupMenu();
        TraceAssert(hArrangeMenu);

        LoadString(GLOBAL_HINSTANCE, IDS_ARRANGEBY, szFmt, ARRAYSIZE(szFmt));

        if ( hArrangeMenu )
        {
            for ( i = 0 ; i < DSA_GetItemCount(_hdsaColumns); i++ )
            {
                LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, i);
                TraceAssert(pColumn);

                wsprintf(szBuffer, szFmt, pColumn->pHeading);
                InsertMenu(hArrangeMenu, i, MF_STRING|MF_BYPOSITION, DSQH_VIEW_ARRANGEFIRST+i, szBuffer);
            }

        }
    }

    // now place the arrange menu into the view and ungrey as required.

    ZeroMemory(&mii, SIZEOF(mii));
    mii.cbSize = SIZEOF(mii);
    mii.fMask = MIIM_SUBMENU|MIIM_ID;
    mii.hSubMenu = hArrangeMenu;
    mii.wID = DSQH_VIEW_ARRANGEICONS;
    
    if ( SetMenuItemInfo(hMenu, DSQH_VIEW_ARRANGEICONS, FALSE, &mii) )
    {
        ENABLE_MENU_ITEM(hMenu, DSQH_VIEW_ARRANGEICONS, GetMenuItemCount(hArrangeMenu));
        hArrangeMenu = NULL;
    }

    if ( hArrangeMenu )
        DestroyMenu(hArrangeMenu);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_GetQueryFormKey
/ --------------------------
/   Given the CLSID for the query form we are interested in, get the 
/   forms key for it, note that these settings are stored per-user.
/
/ In:
/   clsidForm = CLSID of form to pick up
/   phKey -> recevies the HKEY of the form we are intersted in.
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_GetQueryFormKey(REFCLSID clsidForm, HKEY* phKey)
{
    HRESULT hres;
    TCHAR szGUID[GUIDSTR_MAX];
    TCHAR szBuffer[MAX_PATH];

    TraceEnter(TRACE_VIEW, "CDsQuery::_GetQueryFormKey");

    GetStringFromGUID(clsidForm, szGUID, ARRAYSIZE(szGUID));
    wsprintf(szBuffer, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Directory UI\\QueryForms\\%s"), szGUID);
    Trace(TEXT("Settings key is: %s"), szBuffer);

    if ( ERROR_SUCCESS != RegCreateKey(HKEY_CURRENT_USER, szBuffer, phKey) )
        ExitGracefully(hres, E_FAIL, "Failed to open settings key");

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres); 
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_GetColumnTable
/ ------------------------
/   Build the column table for the view we are about to display.  The column
/   table is constructed from either the query parameters or the persisted
/   column settings stored in the registry.
/
/ In:
/   clsidForm = clisd of the form to be used
/   pDsQueryParams -> query parameter structure to be used
/   pHDSA -> DSA to recieve the column table (sorted by visible order)
/   fSetInView = Set the columns into the view
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_GetColumnTable(REFCLSID clsidForm, LPDSQUERYPARAMS pDsQueryParams, HDSA* pHDSA, BOOL fSetInView)
{
    HRESULT hres;
    HKEY hKey = NULL;
    BOOL fDefaultSettings = TRUE;
    LPSAVEDCOLUMN aSavedColumn = NULL;
    LPTSTR pSettingsValue = VIEW_SETTINGS_VALUE;
    BOOL fHaveSizeInfo = FALSE;
    LPWSTR pProperty;
    DWORD dwType, cbSize;
    LV_COLUMN lvc;
    SIZE sz;
    INT i, iNewColumn;
    HD_ITEM hdi;
    IDsDisplaySpecifier *pdds = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "CDsQuery::_GetColumnTable");
    TraceGUID("clsidForm ", clsidForm);
    Trace(TEXT("pDsQueryParams %08x, pHDSA %08x"), pDsQueryParams, pHDSA);

    DECLAREWAITCURSOR;
    SetWaitCursor();

    if ( !pHDSA )
        ExitGracefully(hres, E_INVALIDARG, "Bad pDsQueryParams / pHDSA");

    // construct the column DSA then attempt to look up the view settings stored in
    // the registry for the current form.

    *pHDSA = DSA_Create(SIZEOF(COLUMN), 16);
    TraceAssert(*pHDSA);

    if ( !*pHDSA )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to construct the column DSA");

    // If invoked for admin UI then we look at the admin view settings, this way
    // the admin can have one set of columns, and the user can have another.

    if ( _dwFlags & DSQPF_ENABLEADMINFEATURES )
        pSettingsValue = ADMIN_VIEW_SETTINGS_VALUE;

    Trace(TEXT("View settings value: %s"), pSettingsValue);

    if ( SUCCEEDED(_GetQueryFormKey(clsidForm, &hKey)) )
    {
        // we have the handle to the forms sub-key table, now lets check and
        // see what size the view settings stream is.

        if ( (ERROR_SUCCESS == RegQueryValueEx(hKey, pSettingsValue, NULL, &dwType, NULL, &cbSize)) && 
             (dwType == REG_BINARY) && 
             (cbSize > SIZEOF(SAVEDCOLUMN)) )
        {
            Trace(TEXT("Reading view settings from registry (size %d)"), cbSize);
            
            aSavedColumn = (LPSAVEDCOLUMN)LocalAlloc(LPTR, cbSize);
            TraceAssert(aSavedColumn);

            if ( aSavedColumn && 
                 (ERROR_SUCCESS == RegQueryValueEx(hKey, pSettingsValue, NULL, NULL, (LPBYTE)aSavedColumn, &cbSize)) )
            {
                // compute the size of the table from the values that we have
                // read from the registry and now lets allocate a table for it

                for ( i = 0; aSavedColumn[i].cbSize; i++ )
                {
                    COLUMN column = { 0 };
                    LPCWSTR pProperty = (LPCWSTR)ByteOffset(aSavedColumn, aSavedColumn[i].offsetProperty);
                    LPCTSTR pHeading = (LPCTSTR)ByteOffset(aSavedColumn, aSavedColumn[i].offsetHeading);

                    hres = LocalAllocStringW(&column.pProperty, pProperty);
                    FailGracefully(hres, "Failed to allocate property name");

                    hres = LocalAllocString(&column.pHeading, pHeading);
                    FailGracefully(hres, "Failed to allocate column heading");

                    //column.fHasColumnProvider = FALSE;
                    column.cx = aSavedColumn[i].cx;
                    column.fmt = aSavedColumn[i].fmt;
                    column.iPropertyType = PROPERTY_ISUNKNOWN;
                    //column.idOperator = 0;
                    //column.clsidColumnHandler = { 0 };
                    //column.pColumnHandler = NULL;

                    ZeroMemory(&column.filter, SIZEOF(column.filter));
                    column.filter.iPropertyType = PROPERTY_ISUNDEFINED;

                    Trace(TEXT("pProperty: '%s', pHeading: '%s', cx %d, fmt %08x"), 
                                            W2T(column.pProperty), column.pHeading, column.cx, column.fmt);

                    if ( -1 == DSA_AppendItem(*pHDSA, &column) )
                        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to add column to the DSA");
                }

                fDefaultSettings = FALSE;           // success we have a table
            }
        }
    }

    if ( fDefaultSettings )
    {
        // unable to read the settings from the registy, therefore defaulting to
        // those defined in the query parameters block.

        if ( !pDsQueryParams )
            ExitGracefully(hres, E_INVALIDARG, "No DSQUERYPARAMs to default using");

        for ( i = 0 ; i < pDsQueryParams->iColumns; i++ )
        {
            COLUMN column = { 0 };

            switch ( pDsQueryParams->aColumns[i].offsetProperty )
            {
                case DSCOLUMNPROP_ADSPATH:
                    pProperty = c_szADsPathCH;
                    break;
            
                case DSCOLUMNPROP_OBJECTCLASS:
                    pProperty = c_szObjectClassCH;
                    break;

                default:
                    pProperty = (LPWSTR)ByteOffset(pDsQueryParams, pDsQueryParams->aColumns[i].offsetProperty);
                    break;
            }

            hres = LocalAllocStringW(&column.pProperty, pProperty);
            FailGracefully(hres, "Failed to allocate property name");

            hres = FormatMsgResource(&column.pHeading, pDsQueryParams->hInstance, pDsQueryParams->aColumns[i].idsName);
            FailGracefully(hres, "Failed to allocate column heading");

            //column.fHasColumnProvider = FALSE;
            column.cx = pDsQueryParams->aColumns[i].cx;
            column.fmt = pDsQueryParams->aColumns[i].fmt;
            column.iPropertyType = PROPERTY_ISUNKNOWN;
            //column.idOperator = 0;
            //column.clsidColumnHandler = { 0 };
            //column.pColumnHandler = NULL;

            ZeroMemory(&column.filter, SIZEOF(column.filter));
            column.filter.iPropertyType = PROPERTY_ISUNDEFINED;

            // Now fix the width of the column we are about to specify, this
            // value has the following meaning:
            //
            //  == 0 => use default width
            //  >  0 => user 'n' magic characters
            //  <  0 => pixel width
            
// BUGBUG: -ve should be % of the view

            if ( column.cx < 0 )
            {
                TraceMsg("Column width specified in pixels");
                column.cx = -column.cx;
            }
            else
            {
                // Default the size if it is == 0, then having done this
                // lets grab the font we want to use before moving on
                // to create a DC and measure the character we need.

                if ( !column.cx )
                    column.cx = DEFAULT_WIDTH;
                           
                if ( !fHaveSizeInfo )
                {
                    HDC hDC;
                    LOGFONT lf;
                    HFONT hFont, hOldFont;

                    SystemParametersInfo(SPI_GETICONTITLELOGFONT, SIZEOF(lf), &lf, FALSE);

                    hFont = CreateFontIndirect(&lf);            // icon title font                  
                    TraceAssert(hFont);
                    hDC = CreateCompatibleDC(NULL);             // screen compatible DC
                    TraceAssert(hDC);

                    hOldFont = (HFONT)SelectObject(hDC, hFont);
                    GetTextExtentPoint(hDC, TEXT("0"), 1, &sz); 
                    SelectObject(hDC, hOldFont);
                    DeleteDC(hDC);
                    DeleteFont(hFont);

                    fHaveSizeInfo = TRUE;
                }

                column.cx = column.cx*sz.cx;            // n chars width
            }

            Trace(TEXT("pProperty: '%s', pHeading: '%s', cx %d, fmt %08x"), 
                                    W2T(column.pProperty), column.pHeading, column.cx, column.fmt);

            if ( -1 == DSA_AppendItem(*pHDSA, &column) )
                ExitGracefully(hres, E_OUTOFMEMORY, "Failed to add column to the DSA");
        }
    }

    // Scan the column list getting both the property name and the CLSID for the
    // column handler (if there is one).

    hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&pdds);
    FailGracefully(hres, "Failed to get the IDsDisplaySpecifier interface");

    if (  _dwFlags & DSQPF_HASCREDENTIALS )
    {
        hres = pdds->SetServer(_pServer, _pUserName, _pPassword, DSSSF_DSAVAILABLE);
        FailGracefully(hres, "Failed to server information");
    }    

    for ( i = 0 ; i < DSA_GetItemCount(*pHDSA) ; i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(*pHDSA, i);
        TraceAssert(pColumn);

        Trace(TEXT("Property for column %d, %s"), i, W2T(pColumn->pProperty));

        // lets get the property type, column handler and the default operator for it.

        hres = GetColumnHandlerFromProperty(pColumn, NULL);
        FailGracefully(hres, "Failed to get the column handler from property string");

        if ( pColumn->fHasColumnHandler )
        {
            TraceMsg("Has a column handler, therefore property is now a string");
            pColumn->iPropertyType = PROPERTY_ISSTRING;
        }
        else
        {
            pColumn->iPropertyType = PropertyIsFromAttribute(pColumn->pProperty, pdds);
        }

        pColumn->idOperator = property_type_table[pColumn->iPropertyType].idOperator;
    }

    // Set the columns up in the view (remove all items first) to allow us
    // to add/remove columns as required.

    if ( fSetInView )
    {
        for ( i = Header_GetItemCount(ListView_GetHeader(_hwndView)); --i >= 0 ; )
            ListView_DeleteColumn(_hwndView, i);

        // add the columns to the view, then having done that set
        // the type of the filter to reflect the property being
        // shown.

        for ( i = 0 ; i < DSA_GetItemCount(_hdsaColumns); i++ )
        {
            LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, i);
            TraceAssert(pColumn);
        
            lvc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_FMT;
            lvc.fmt = pColumn->fmt;
            lvc.cx = pColumn->cx;
            lvc.pszText = pColumn->pHeading;
    
            iNewColumn = ListView_InsertColumn(_hwndView, i, &lvc);
            TraceAssert(iNewColumn != -1);

            if ( iNewColumn != i )
                ExitGracefully(hres, E_FAIL, "Failed to add the column to the view");

            hdi.mask = HDI_FILTER;
            hdi.type = property_type_table[pColumn->iPropertyType].hdft|HDFT_HASNOVALUE;
            hdi.pvFilter = NULL;

            Trace(TEXT("iPropertyType %d, hdi.type %08x"), pColumn->iPropertyType, hdi.type);

            if ( !Header_SetItem(ListView_GetHeader(_hwndView), iNewColumn, &hdi) )
                ExitGracefully(hres, E_FAIL, "Failed to set the filter type into the view");
        }
    }

    hres = S_OK;                  // success

exit_gracefully:

    if ( hKey )
        RegCloseKey(hKey);

    if ( aSavedColumn )
        LocalFree((HLOCAL)aSavedColumn);

    DoRelease(pdds);
    ResetWaitCursor();

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_SaveColumnTable
/ -------------------------
/   Free the column table stored in a DSA.  This code released all the
/   allocated memory stored with the table.
/
/ In:
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID CDsQuery::_SaveColumnTable(VOID)
{
    TraceEnter(TRACE_VIEW, "CDsQuery::_SaveColumnTable");

    _FreeResults();

    if ( _hdsaColumns )
    {
        DSA_DestroyCallback(_hdsaColumns, FreeColumnCB, NULL);
        _hdsaColumns = NULL;
    }

    _iSortColumn = -1;
    _fSortDescending = FALSE;

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_SaveColumnTable
/ -------------------------
/   Save the current column table from the DPA into the registry so 
/   we can restore it the next time the user uses this query form.
/
/ In:
/   clsidForm = form ID to store it under
/   hdsaColumns -> DSA to destory
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_SaveColumnTable(REFCLSID clsidForm, HDSA hdsaColumns)
{
    HRESULT hres;
    LPWSTR pProperty;
    LPSAVEDCOLUMN aSavedColumn = NULL;
    DWORD cbData, offset;
    HKEY hKey = NULL;
    LPTSTR pSettingsValue = VIEW_SETTINGS_VALUE;
    INT i;
    USES_CONVERSION;
    
    TraceEnter(TRACE_VIEW, "CDsQuery::_SaveColumnTable");
    TraceGUID("clsidForm ", clsidForm);
    
    if ( !hdsaColumns )
        ExitGracefully(hres, E_FAIL, "No column data to save");

    // first compute the size of the blob we are going to store into
    // the registry, whilst doing this compute the offset to 
    // the start of the string data.

    offset = SIZEOF(SAVEDCOLUMN);
    cbData = SIZEOF(SAVEDCOLUMN);

    for ( i = 0 ; i < DSA_GetItemCount(hdsaColumns); i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(hdsaColumns, i);
        TraceAssert(pColumn);

        offset += SIZEOF(SAVEDCOLUMN);
        cbData += SIZEOF(SAVEDCOLUMN);
        cbData += StringByteSizeW(pColumn->pProperty);

// BUGBUG: this is a potential problem, must be kept in sync with GetPropertyFromColumn

        if ( pColumn->fHasColumnHandler )
            cbData += SIZEOF(WCHAR)*(GUIDSTR_MAX + 1);                          // nb+1 for seperator (,} + string is UNICODE

        cbData += StringByteSize(pColumn->pHeading);
    }

    Trace(TEXT("offset %d, cbData %d"), offset, cbData);

    // Allocate the structure and lets place all the data into it,
    // again running over the data blocks, append the string data
    // to the end of the blob in a single go.

    aSavedColumn = (LPSAVEDCOLUMN)LocalAlloc(LPTR, cbData);
    TraceAssert(aSavedColumn);

    if ( !aSavedColumn )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate settings data");

    Trace(TEXT("Building data blob at %08x"), aSavedColumn);

    for ( i = 0 ; i < DSA_GetItemCount(hdsaColumns); i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(hdsaColumns, i);
        TraceAssert(pColumn);
       
        hres = GetPropertyFromColumn(&pProperty, pColumn);
        FailGracefully(hres, "Failed to allocate property from column");

        aSavedColumn[i].cbSize = SIZEOF(SAVEDCOLUMN);
        aSavedColumn[i].dwFlags = 0;
        aSavedColumn[i].offsetProperty = offset;
        aSavedColumn[i].offsetHeading = offset + StringByteSizeW(pProperty);
        aSavedColumn[i].cx = pColumn->cx;
        aSavedColumn[i].fmt = pColumn->fmt;
        
        StringByteCopyW(aSavedColumn, aSavedColumn[i].offsetProperty, pProperty);
        offset += StringByteSizeW(pProperty);

        StringByteCopy(aSavedColumn, aSavedColumn[i].offsetHeading, pColumn->pHeading);
        offset += StringByteSize(pColumn->pHeading);

        LocalFreeStringW(&pProperty);
    }

    aSavedColumn[i].cbSize = 0;                // terminate the list of columns with a NULL

    Trace(TEXT("offset %d, cbData %d"), offset, cbData);
    TraceAssert(offset == cbData);

    // now put the data into the registry filed under the key for the query for that
    // it maps to.

    hres = _GetQueryFormKey(clsidForm, &hKey);
    FailGracefully(hres, "Failed to get settings sub-key");

    // If invoked for admin UI then we look at the admin view settings, this way
    // the admin can have one set of columns, and the user can have another.

    if ( _dwFlags & DSQPF_ENABLEADMINFEATURES )
        pSettingsValue = ADMIN_VIEW_SETTINGS_VALUE;

    Trace(TEXT("View settings value: %s"), pSettingsValue);

    if ( ERROR_SUCCESS != RegSetValueEx(hKey, pSettingsValue, 0, REG_BINARY, (LPBYTE)aSavedColumn, cbData) )
        ExitGracefully(hres, E_FAIL, "Failed to write setting into the view");

    hres = S_OK;

exit_gracefully:

    if ( aSavedColumn )
        LocalFree((HLOCAL)aSavedColumn);

    if ( hKey )
        RegCloseKey(hKey);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_GetIDLsAndViewObject
/ ------------------------------
/   For the current selection lets ensure we get the current view object
/   for it.  Do this by converting the results to IDLISTs and then
/   calling ::GetViewObject of with a RIID.
/
/ In:
/   fGetAll = return all items in the view, not just the selection
/   riid = interface needed
/   ppvovj -> receives the interface pointer (== NULL if not needed).
/
/ Out:
/   HRESULT / ShortFromResult gives item count
/----------------------------------------------------------------------------*/

HRESULT _GetIDLCollectCB(CDsQuery* pdq, LPARAM lParam, INT item, LPQUERYRESULT pResult)
{
    HRESULT hres;
    LPITEMIDLIST* aidl = (LPITEMIDLIST*)lParam;
    USES_CONVERSION;

    // Ensure that the items have IDLISTs associated with them, if not then 
    // lets parse and store.  If we have a return array then fill that in with
    // some data also.

    TraceEnter(TRACE_VIEW, "_GetIDLCollectCB");

    if ( aidl )
    {
        Trace(TEXT("Constructing IDLIST for %s"), W2T(pResult->pPath));

        hres = pdq->_ADsPathToIdList(&aidl[item], pResult->pPath, pResult->pObjectClass);
        FailGracefully(hres, "Failed when building IDLIST");
    }

    hres = S_OK;                          // success

exit_gracefully:

    TraceLeaveResult(hres);
}

HRESULT CDsQuery::_GetIDLsAndViewObject(BOOL fGetAll, REFIID riid, void **ppvobj)
{
    HRESULT hres;
    IShellFolder* psf = NULL;
    IUnknown *punk;
    LPITEMIDLIST* aidl = NULL;
    INT cItems = NULL;

    TraceEnter(TRACE_VIEW, "CDsQuery::_GetIDLsAndViewObject");

    // get the selection count and then build the IDLISTs for them.

    hres = _GetDirectorySF(&psf);
    FailGracefully(hres, "Failed to get the DS ShellFolder implementation");

    hres = _CollectViewSelection(fGetAll, &cItems, NULL, 0);
    FailGracefully(hres, "Failed 1st pass of collect selection (get number of selected items)");

    if ( cItems )
    {
        aidl = (LPITEMIDLIST*)LocalAlloc(LPTR, SIZEOF(LPITEMIDLIST)*cItems);
        TraceAssert(aidl);

        if ( !aidl )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed when allocating IDLIST array");

        // get the IDTEMIDLISTs for the selection

        hres = _CollectViewSelection(fGetAll, NULL, _GetIDLCollectCB, (LPARAM)aidl);
        FailGracefully(hres, "Failed 2nd pass of collect selection (get IDLSIT array)");
    }
        
    hres = psf->GetUIObjectOf(NULL, cItems, (LPCITEMIDLIST*)aidl, riid, 0, ppvobj);
    FailGracefully(hres, "Failed to get the interface from GetUIObjectOf");

    hres = ResultFromShort(cItems);

exit_gracefully:

    DoRelease(psf);

    //
    // can we get an IObjectWithSite iface from this COM object, if so then lets ensure
    // that we set site (if we have a site to be set).
    //

    if ( SUCCEEDED(hres) && _punkSite )
    {
        IUnknown *punk = (IUnknown*)*ppvobj;
        IObjectWithSite *pows;

        if (  punk && SUCCEEDED(punk->QueryInterface(IID_IObjectWithSite, (void **)&pows)) )
        {
            pows->SetSite(_punkSite);
            pows->Release();
        }
    }

    if ( aidl )
        LocalFree(aidl);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_GetContextMenuVerbs
/ -----------------------------
/   Do the query context menu handling the case where we have been invoked
/   modally.
/
/ In:
/   pcm = IContextMenu interface to interact with
/   hMenu = menu handle to merge into
/   dwFlags = flags for QueryContextMenu
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
VOID CDsQuery::_GetContextMenuVerbs(IContextMenu* pcm, HMENU hMenu, DWORD dwFlags)
{
    TCHAR szBuffer[MAX_PATH];

    TraceEnter(TRACE_VIEW, "CDsQuery::_GetContextMenuVerbs");

    pcm->QueryContextMenu(hMenu, 0, DSQH_FILE_CONTEXT_FIRST, DSQH_FILE_CONTEXT_LAST, dwFlags);                
    Trace(TEXT("Menu item count after QueryContextMenu %d (%08x)"), GetMenuItemCount(hMenu), hMenu);

    if ( (_dwOQWFlags & OQWF_OKCANCEL) && LoadString(GLOBAL_HINSTANCE, IDS_SELECT, szBuffer, ARRAYSIZE(szBuffer)) )
    {
        InsertMenu(hMenu, 0, MF_BYPOSITION|MF_STRING, DSQH_BG_SELECT, szBuffer);
        InsertMenu(hMenu, 1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
        SetMenuDefaultItem(hMenu, DSQH_BG_SELECT, FALSE);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_CollectViewSelection
/ ------------------------------
/   Walk through all the items in the view that are selected, calling
/   the callback function so that it can do with the ADsPath / Class
/   as it will.
/
/ In:
/   fGetAll -> returns all items, not just selected ones
/   pCount = receives the number of items selected
/   pCollectProc, lParam -> called as required for each item we collect
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

HRESULT _CallSelectionCB(CDsQuery *pqd, HWND hWnd, INT i, LPINT pIndex, LPCOLLECTPROC pcp, LPARAM lParam)
{
    HRESULT hres;
    LV_ITEM lvi = { 0 };
    USES_CONVERSION;

    TraceEnter(TRACE_VIEW, "_CallSelectionCB");

    lvi.mask = LVIF_PARAM;
    lvi.iItem = i;

    if ( ListView_GetItem(hWnd, &lvi) )
    {
        if ( pcp )
        {
            LPQUERYRESULT pResult = (LPQUERYRESULT)lvi.lParam;
            TraceAssert(pResult);

            Trace(TEXT("Item %d, Path %s, class %s"), i, W2T(pResult->pPath), W2T(pResult->pObjectClass));

            hres = (*pcp)(pqd, lParam, *pIndex, pResult);
            FailGracefully(hres, "Failed in the callback function");

        }

        *pIndex += 1;
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

HRESULT CDsQuery::_CollectViewSelection(BOOL fGetAll, INT* pCount, LPCOLLECTPROC pCollectProc, LPARAM lParam)
{
    HRESULT hres;
    INT item = -1;
    INT focused = -1;
    INT count = 0;
    LPARAM flags = LVNI_ALL|LVNI_SELECTED;

    TraceEnter(TRACE_VIEW, "CDsQuery::_CollectViewSelection");

    DECLAREWAITCURSOR;
    SetWaitCursor();

    // find the focused item, this must be the first in the selection list
    // therefore special case it.

    if ( IsWindow(_hwndView) )
    {
        focused = ListView_GetNextItem(_hwndView, -1, flags|LVNI_FOCUSED);
        Trace(TEXT("focused item is %d"), focused);

        if ( focused >= 0 )
        {
            hres = _CallSelectionCB(this, _hwndView, focused, &count, pCollectProc, lParam);
            FailGracefully(hres, "Failed calling the selection CB");
        }   

        // now walk the rest of the list passing out all the selected items
        // to the callback (skipping the selected one);

        do
        {
            item = ListView_GetNextItem(_hwndView, item, flags);
            Trace(TEXT("item %d (focused %d)"), item, focused);

            if ( (item >= 0) && (item != focused) )
            {
                hres = _CallSelectionCB(this, _hwndView, item, &count, pCollectProc, lParam);
                FailGracefully(hres, "Failed calling the selection CB");
            }
        }
        while ( item != -1 );
    }

    hres = S_OK;

exit_gracefully:

    Trace(TEXT("Selection count is %d"), count);

    if ( pCount )
        *pCount = count;

    ResetWaitCursor();

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_CopyCredentials
/ --------------------------
/   Copy the user name, password and server as required.
/
/ In:
/   ppszUserName, psszPassword & ppszServer => destinations
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_CopyCredentials(LPWSTR *ppszUserName, LPWSTR *ppszPassword, LPWSTR *ppszServer)
{
    HRESULT hres;

    TraceEnter(TRACE_VIEW, "CDsQuery::_CopyCredentials");

    hres = LocalAllocStringW(ppszUserName, _pUserName);
    FailGracefully(hres, "Failed to copy the user name");

    hres = LocalAllocStringW(ppszPassword, _pPassword);
    FailGracefully(hres, "Failed to copy the password");

    hres = LocalAllocStringW(ppszServer, _pServer);
    FailGracefully(hres, "Failed to copy the server name");

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_GetDirectorySF
/ -------------------------
/   Get the IShellFolder for the directory namespace.
/
/ In:
/   ppsf -> returned with a shell folder object

/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_GetDirectorySF(IShellFolder **ppsf)
{
    HRESULT hres;
    IShellFolder *psf = NULL;
    IDsFolderInternalAPI *pdfi = NULL;
#if DOWNLEVEL_SHELL
    IPersistFolder* ppf = NULL;
#endif

    TraceEnter(TRACE_VIEW, "CDsQuery::GetDirectorySF");

#if !DOWNLEVEL_SHELL
    // just bind to the path if this is not a downlevel shell.

    hres = BindToPath(c_szDsMagicPath, IID_IShellFolder, (void **)&psf);
    FailGracefully(hres, "Failed to get IShellFolder view of the DS namespace");
#else
    // on the downlevel shell we need to CoCreate the IShellFolder implementation for the
    // DS namespace and initialize it manually

    hres = CoCreateInstance(CLSID_MicrosoftDS, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (void **)&psf);
    FailGracefully(hres, "Failed to get the IShellFolder interface we need");

    if ( SUCCEEDED(psf->QueryInterface(IID_IPersistFolder, (void **)&ppf)) )
    {
        ITEMIDLIST idl = { 0 };
        ppf->Initialize(&idl);       // it calls ILCLone, so we are safe with stack stuff
        ppf->Release();
    }
#endif

    // using the internal API set all the parameters for dsfolder

// BUGBUG: replace with a property bag?

    hres = psf->QueryInterface(IID_IDsFolderInternalAPI, (void **)&pdfi);
    FailGracefully(hres, "Failed to get the IDsFolderInternalAPI");

    pdfi->SetAttributePrefix((_dwFlags & DSQPF_ENABLEADMINFEATURES) ? DS_PROP_ADMIN_PREFIX:NULL);

    if ( _dwFlags & DSQPF_ENABLEADVANCEDFEATURES )
        pdfi->SetProviderFlags(~DSPROVIDER_ADVANCED, DSPROVIDER_ADVANCED);

    hres = pdfi->SetComputer(_pServer, _pUserName, _pPassword);
    FailGracefully(hres, "Failed when setting credential information to be used");

exit_gracefully:
    
    if ( SUCCEEDED(hres) )
        psf->QueryInterface(IID_IShellFolder, (void **)ppsf);

    DoRelease(psf);
    DoRelease(pdfi);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::_ADsPathToIdList
/ --------------------------
/   Convert an ADsPath into an IDLIST that is suitable for the DS ShellFolder
/   implementation.
/
/ In:
/   ppidl -> receives the resulting IDLIST
/   pPath = name to be parsed / = NULL then generate only the root of the namespace
/   pObjectClass = class to use
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CDsQuery::_ADsPathToIdList(LPITEMIDLIST* ppidl, LPWSTR pPath, LPWSTR pObjectClass)
{
    HRESULT hres;
    IBindCtx *pbc = NULL;
    IPropertyBag *ppb = NULL;
    IShellFolder *psf = NULL;
    VARIANT var;
    USES_CONVERSION;
    
    TraceEnter(TRACE_HANDLER, "CDsQuery::_ADsPathToIdList");

    *ppidl = NULL;                  // incase we fail
                                       
    // if we have an object class then create a bind context containing it

    if ( pObjectClass )
    {
        Trace(TEXT("Object class is: %s"), W2T(pObjectClass));

        hres = CPropertyBag_CreateInstance(IID_IPropertyBag, (void **)&ppb);
        FailGracefully(hres, "Failed to create a property bag");

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(pObjectClass);

        if ( V_BSTR(&var) )
        {
            ppb->Write(DS_PDN_OBJECTLCASS, &var);
            VariantClear(&var);
        }

        hres = CreateBindCtx(0, &pbc);
        FailGracefully(hres, "Failed to create the BindCtx object");

        hres = pbc->RegisterObjectParam(DS_PDN_PROPERTYBAG, ppb);
        FailGracefully(hres, "Failed to register the property bag with bindctx");
    }

    hres = _GetDirectorySF(&psf);
    FailGracefully(hres, "Failed to get ShellFolder for DS namespace");

    hres = psf->ParseDisplayName(NULL, pbc, pPath, NULL, ppidl, NULL);
    FailGracefully(hres, "Failed to parse the name");

exit_gracefully:

    DoRelease(pbc);
    DoRelease(ppb);
    DoRelease(psf);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnPickColumns
/----------------------------------------------------------------------------*/

typedef struct 
{
    LPWSTR pProperty;
    LPTSTR pHeading;
    BOOL fIsColumn;                     // item is column and added to column list box on init
    INT cx;                             // pixel width of column
    INT fmt;                            // formatting information of column
} PICKERITEM, * LPPICKERITEM;

typedef struct
{
    HDPA hdpaItems;                     // all the items in the view
    HDSA hdsaColumns;                   // column table generated by this dialog
    HWND hwndProperties;                // hwnd's for the columns/property tables
    HWND hwndColumns;
} PICKERSTATE, * LPPICKERSTATE;

VOID _PickerMoveColumn(HWND hwndDest, HWND hwndSrc, BOOL fInsert);
HRESULT _Picker_GetColumnTable(HWND hwndColumns, HDSA* pHDSA);
INT_PTR _PickerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT _PickerItemFreeCB(LPVOID pData, LPVOID lParam);
INT _PickerItemCmpCB(LPVOID p1, LPVOID p2, LPARAM lParam);

//
// Help ID mappings
//

static DWORD const aPickColumnsHelpIDs[] =
{
    IDC_LBPROPERTIES, IDH_COLUMNS_AVAILABLE,
    IDC_LBCOLUMNS,    IDH_COLUMNS_SHOWN,
    IDC_ADD,          IDH_ADD_COLUMNS,
    IDC_REMOVE,       IDH_REMOVE_COLUMNS,
    0, 0
};


/*-----------------------------------------------------------------------------
/ _PickerMoveColumn
/ -----------------
/   Move the selected item in one list box to another, we assume that the
/   item data points to a PICKERITEM structure, we transfer the selection
/   and 
/
/ In:
/   hwndSrc, hwndState = windows to move selection within
/   fInsert = insert at selection point in destionation, or add the string (sorted)
/
/ Out:
/   VOID
/----------------------------------------------------------------------------*/
VOID _PickerMoveColumn(HWND hwndDest, HWND hwndSrc, BOOL fInsert)
{
    INT iSelection, i;
    LPPICKERITEM pItem;
    USES_CONVERSION;

    TraceEnter(TRACE_FIELDCHOOSER, "_PickerMoveColumn");

    iSelection = ListBox_GetCurSel(hwndSrc);
    TraceAssert(iSelection >= 0 );

    if ( iSelection >= 0 )
    {
        LPPICKERITEM pItem = (LPPICKERITEM)ListBox_GetItemData(hwndSrc, iSelection);
        TraceAssert(pItem);
        TraceAssert(pItem->pHeading);

        // Add the new item to the view (if this it the properties)
        // then this will result in the list being sorted and select
        // it to allow the user to remove/add another

        if ( fInsert )
        {
            Trace(TEXT("Inserting the item at index %d"), ListBox_GetCurSel(hwndDest)+1);
            i = ListBox_InsertString(hwndDest, ListBox_GetCurSel(hwndDest)+1, pItem->pHeading);
        }
        else
        {
            TraceMsg("Adding string to listbox");
            i = ListBox_AddString(hwndDest, pItem->pHeading);
        }

        TraceAssert(i != -1);

        ListBox_SetItemData(hwndDest, i, (LPARAM)pItem);
        ListBox_SetCurSel(hwndDest, i);

        // remove the item from the source, ensuring that the
        // selection stays visually at the same place (nb
        // cope with removing the last item).
        
        ListBox_DeleteString(hwndSrc, iSelection);

        if ( iSelection >= ListBox_GetCount(hwndSrc) )
            iSelection = ListBox_GetCount(hwndSrc)-1;
        if ( iSelection >= 0 )
            ListBox_SetCurSel(hwndSrc, iSelection);
    }

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ _Picker_GetColumnTable
/ ---------------------
/   The user has hit OK, therefore we must build a new column table, this
/   code walks the items in the columns ListBox and generates the
/   column DSA that we should be using.
/
/ In:
/   hwndColumns -> ListBox containing the columns
/   pHDSA -> DSA to receive the columnt able
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _Picker_GetColumnTable(HWND hwndColumns, HDSA* pHDSA)
{
    HRESULT hres;
    HDSA hdsaColumns = NULL;
    INT i;
    INT cxColumn = 0;

    TraceEnter(TRACE_FIELDCHOOSER, "_Picker_GetColumnTable");

    // Construct a DSA to store the column table in

    hdsaColumns = DSA_Create(SIZEOF(COLUMN), 4);
    TraceAssert(hdsaColumns);

    if ( !hdsaColumns )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to create the column DSA");

    // For each entry in the ListBox add an item to the DSA that contains
    // the relevant column information

    for ( i = 0 ; i < ListBox_GetCount(hwndColumns) ; i++ )
    {
        COLUMN column = { 0 };
        LPPICKERITEM pItem = (LPPICKERITEM)ListBox_GetItemData(hwndColumns, i);
        TraceAssert(pItem);

        // column.fHasColumnHandler = FALSE;
        column.pProperty = NULL;
        column.pHeading = NULL;
        //column.cx = 0;
        //column.fmt = 0;
        column.iPropertyType = PROPERTY_ISUNKNOWN;
        column.idOperator = 0;
        // column.clsidColumnHandler = { 0 };
        // column.pColumnHandler = NULL;

        // fIsColumn indicates that the entry was originally a column therefore
        // has extra state information, ensure that we copy this over, otherwise
        // just pick sensible defaults.  Having done that we can allocate the
        // strings, add the item and continue...

        if ( pItem->fIsColumn )
        {
            column.cx = pItem->cx;
            column.fmt = pItem->fmt;
        }
        else
        {
            // Have we cached the column width yet?  If not then lets do
            // so and apply it to all the columns which are using the
            // default width (as they have not yet been sized)
            
            if ( !cxColumn )
            {
                HDC hDC;
                LOGFONT lf;
                HFONT hFont, hOldFont;
                SIZE sz;

                SystemParametersInfo(SPI_GETICONTITLELOGFONT, SIZEOF(lf), &lf, FALSE);

                hFont = CreateFontIndirect(&lf);            // icon title font                  
                TraceAssert(hFont);
                hDC = CreateCompatibleDC(NULL);             // screen compatible DC
                TraceAssert(hDC);

                hOldFont = (HFONT)SelectObject(hDC, hFont);
                GetTextExtentPoint(hDC, TEXT("0"), 1, &sz); 
                SelectObject(hDC, hOldFont);
                DeleteDC(hDC);
                DeleteObject(hFont);

                cxColumn = DEFAULT_WIDTH * sz.cx;
            }

            column.cx = cxColumn;
            column.fmt = 0;
        }

        if ( FAILED(GetColumnHandlerFromProperty(&column, pItem->pProperty)) ||
                    FAILED(LocalAllocString(&column.pHeading, pItem->pHeading)) ||
                            (-1 == DSA_AppendItem(hdsaColumns, &column)) )
        {
            LocalFreeStringW(&column.pProperty);
            LocalFreeString(&column.pHeading);
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to construct the column entry");
        }
    }

    Trace(TEXT("New column table contains %d items"), DSA_GetItemCount(hdsaColumns));
    hres = S_OK;

exit_gracefully:

    if ( FAILED(hres) && hdsaColumns )
    {
        DSA_DestroyCallback(hdsaColumns, FreeColumnCB, NULL);
        hdsaColumns = NULL;
    }

    *pHDSA = hdsaColumns;

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ _PickerDlgProc
/ --------------
/   Dialog proc for handling the dialog messages (there is a suprise)
/
/ In:
/   hwnd, uMsg, wParam, lParam = message information
/   DWL_USER => LPPICKERSTATE structure
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/

#define SET_BTN_STYLE(hwnd, idc, style) \
            SendDlgItemMessage(hwnd, idc, BM_SETSTYLE, MAKEWPARAM(style, 0), MAKELPARAM(TRUE, 0));


INT_PTR _PickerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    USES_CONVERSION;
    INT_PTR fResult = FALSE;
    LPPICKERSTATE pState = NULL;
    BOOL fUpdateButtonState = TRUE;
    PICKERITEM item;
    INT i, j;

    if ( uMsg == WM_INITDIALOG )
    {
        pState = (LPPICKERSTATE)lParam;
        TraceAssert(pState);

        SetWindowLongPtr(hwnd, DWLP_USER, lParam);

        pState->hwndProperties = GetDlgItem(hwnd, IDC_LBPROPERTIES);
        pState->hwndColumns = GetDlgItem(hwnd, IDC_LBCOLUMNS);

        // pState->hdsaColumns contains the currently visible column table, this is
        // the table being used by the current view, therefore we must not modify
        // it, just treat it as read only.   Add the columns to the ListBox
        // marking the visible items in the properties DPA, then add those
        // items not already shown to the properties list box.

        for ( i = 0 ; i < DSA_GetItemCount(pState->hdsaColumns) ; i++ )
        {
            LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(pState->hdsaColumns, i);
            TraceAssert(pColumn);

            if ( SUCCEEDED(GetPropertyFromColumn(&item.pProperty, pColumn)) )
            {
                j = DPA_Search(pState->hdpaItems, &item, 0, _PickerItemCmpCB, NULL, DPAS_SORTED);

                Trace(TEXT("Searching for %s yielded %d"), W2T(item.pProperty), j);

                if ( j >= 0 )
                {
                    LPPICKERITEM pItem = (LPPICKERITEM)DPA_GetPtr(pState->hdpaItems, j);
                    TraceAssert(pItem);                

                    ListBox_SetItemData(pState->hwndColumns, 
                            ListBox_AddString(pState->hwndColumns, pItem->pHeading), 
                                (LPARAM)pItem);
                }
                
                LocalFreeStringW(&item.pProperty);
            }
        }

        for ( i = 0 ; i < DPA_GetPtrCount(pState->hdpaItems) ; i++ )
        {
            LPPICKERITEM pItem = (LPPICKERITEM)DPA_GetPtr(pState->hdpaItems, i);
            TraceAssert(pItem && pItem->pHeading);

            if ( !pItem->fIsColumn )
            {
                ListBox_SetItemData(pState->hwndProperties,
                        ListBox_AddString(pState->hwndProperties, pItem->pHeading), 
                            (LPARAM)pItem);
            }
        }

        // Ensure we default select the top items of each list

        ListBox_SetCurSel(pState->hwndProperties, 0);
        ListBox_SetCurSel(pState->hwndColumns, 0);
    }
    else
    {
        pState = (LPPICKERSTATE)GetWindowLongPtr(hwnd, DWLP_USER);
        TraceAssert(pState);

        switch ( uMsg )
        {
            case WM_HELP:
            {
                LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
                WinHelp((HWND)pHelpInfo->hItemHandle,
                        DSQUERY_HELPFILE,
                        HELP_WM_HELP,
                        (DWORD_PTR)aPickColumnsHelpIDs);
                break;
            }

            case WM_CONTEXTMENU:
                WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aPickColumnsHelpIDs);
                fResult = TRUE;
                break;

            case WM_COMMAND:
            {
                switch ( LOWORD(wParam) )
                {
                    case IDOK:
                    {                        
                        _Picker_GetColumnTable(pState->hwndColumns, &pState->hdsaColumns);
                        EndDialog(hwnd, IDOK);
                        break;
                    }

                    case IDCANCEL:
                        EndDialog(hwnd, IDCANCEL);
                        break;

                    case IDC_ADD:
                        _PickerMoveColumn(pState->hwndColumns, pState->hwndProperties, TRUE);
                        break;

                    case IDC_REMOVE:
                        _PickerMoveColumn(pState->hwndProperties, pState->hwndColumns, FALSE);
                        break;

                    case IDC_LBPROPERTIES:
                    {
                        if ( ListBox_GetCount(pState->hwndProperties) > 0 )
                        {
                            if ( HIWORD(wParam) == LBN_DBLCLK )
                                _PickerMoveColumn(pState->hwndColumns, pState->hwndProperties, TRUE);
                        }

                        break;                            
                    }

                    case IDC_LBCOLUMNS:
                    {
                        if ( ListBox_GetCount(pState->hwndColumns) > 1 )
                        {
                            if ( HIWORD(wParam) == LBN_DBLCLK )
                                _PickerMoveColumn(pState->hwndProperties, pState->hwndColumns, FALSE);
                        }

                        break;                            
                    }

                    default:
                        fUpdateButtonState = FALSE;
                        break;
                }

                break;
            }

            default:
                fUpdateButtonState = FALSE;
                break;
        }

    }

    // if the selections state change, or something which would cause
    // us to refresh the add/remove buttons state then lets do it.
    // each button is enabled only if there is a selection and the
    // number of items is > the min value.

    if ( pState && fUpdateButtonState )
    {   
        BOOL fEnableAdd = FALSE;
        BOOL fEnableRemove = FALSE;
        DWORD dwButtonStyle;

        if ( (ListBox_GetCount(pState->hwndProperties) > 0) )
        {
            fEnableAdd = TRUE;   
        }

        if ( (ListBox_GetCount(pState->hwndColumns) > 1) )
        {
            fEnableRemove = TRUE;   
        }

        // Make sure the DefButton is an enabled button
        // BUGBUG:  Need to add an SHSetDefID() export to shlwapi
        // which is simply a SetDefID that "does the right thing"
        // wrt disabled buttons.

        if ( (!fEnableRemove) && (!fEnableAdd) ) 
        {
            SET_BTN_STYLE(hwnd, IDC_ADD, BS_PUSHBUTTON);
            SET_BTN_STYLE(hwnd, IDC_REMOVE, BS_PUSHBUTTON);

            SendMessage(hwnd, DM_SETDEFID, IDOK, 0);
            SET_BTN_STYLE(hwnd, IDOK, BS_DEFPUSHBUTTON);
            SetFocus(GetDlgItem(hwnd, IDOK));
        }
        else if ( !fEnableAdd ) 
        {
            dwButtonStyle = (DWORD)GetWindowLong(GetDlgItem(hwnd, IDC_ADD), GWL_STYLE);
            if ( dwButtonStyle & BS_DEFPUSHBUTTON ) 
            {
                SET_BTN_STYLE(hwnd, IDC_ADD, BS_PUSHBUTTON);
                SendMessage(hwnd, DM_SETDEFID, IDC_REMOVE, 0);

                SET_BTN_STYLE(hwnd, IDC_REMOVE, BS_DEFPUSHBUTTON);
                SetFocus(GetDlgItem(hwnd, IDC_REMOVE));
            }
        }
        else if ( !fEnableRemove ) 
        {
            dwButtonStyle = (DWORD) GetWindowLong(GetDlgItem(hwnd, IDC_REMOVE), GWL_STYLE);
            if ( dwButtonStyle & BS_DEFPUSHBUTTON ) 
            {
                SET_BTN_STYLE(hwnd, IDC_REMOVE, BS_PUSHBUTTON);                    
                SendMessage(hwnd, DM_SETDEFID, IDC_ADD, 0);
                SET_BTN_STYLE(hwnd, IDC_ADD, BS_DEFPUSHBUTTON);
                SetFocus(GetDlgItem(hwnd, IDC_ADD));
            }
        }

        Button_Enable(GetDlgItem(hwnd, IDC_ADD), fEnableAdd);
        Button_Enable(GetDlgItem(hwnd, IDC_REMOVE), fEnableRemove);

    }

    return fResult;
}


/*-----------------------------------------------------------------------------
/ CDsQuery::OnPickColumns
/ -----------------------
/   Handle picking the columns that should be displayed in the result view,
/   if the user selects new columns and hits OK then we refresh the 
/   view and the internal table of visible columns.
/
/ In:
/   hwndParent = parent for the dialog
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

INT _PickerItemFreeCB(LPVOID pData, LPVOID lParam)
{
    LPPICKERITEM pItem = (LPPICKERITEM)pData;
    LocalFreeStringW(&pItem->pProperty);
    LocalFreeString(&pItem->pHeading);
    LocalFree(pItem);
    return 1;
}

INT _PickerItemCmpCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    USES_CONVERSION;
    LPPICKERITEM pEntry1 = (LPPICKERITEM)p1;
    LPPICKERITEM pEntry2 = (LPPICKERITEM)p2;
    INT nResult = -1;
  
    if ( pEntry1 && pEntry2 )
        nResult = StrCmpW(pEntry1->pProperty, pEntry2->pProperty);

    return nResult;
}

typedef struct
{
    PICKERSTATE *pps;
    HDPA hdpaProperties;         // attributes to be appeneded.
} PICKERENUMATTRIB;

HRESULT CALLBACK _PickerEnumAttribCB(LPARAM lParam, LPCWSTR pAttributeName, LPCWSTR pDisplayName, DWORD dwFlags)
{
    HRESULT hres;
    PICKERENUMATTRIB *ppea = (PICKERENUMATTRIB*)lParam;
    PICKERITEM item;
    INT j;
    USES_CONVERSION;

    TraceEnter(TRACE_FIELDCHOOSER, "_PickerEnumAttribCB");

// BUGBUG: fix casting
    item.pProperty = (LPWSTR)pAttributeName;

    j = DPA_Search(ppea->pps->hdpaItems, &item, 0, _PickerItemCmpCB, NULL, DPAS_SORTED);
    if ( j == -1 )
    {
        Trace(TEXT("Property not already in list: %s"), W2CT(pAttributeName));

        hres = StringDPA_AppendStringW(ppea->hdpaProperties, pAttributeName, NULL);
        FailGracefully(hres, "Failed to add unique property to DPA");
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

HRESULT CDsQuery::OnPickColumns(HWND hwndParent)
{
    USES_CONVERSION;
    HRESULT hres;
    HDPA hdpaProperties = NULL;
    PICKERSTATE state;
    PICKERENUMATTRIB pea = { 0 };
    INT i, j, iProperty, iColumn;
    LPDSQUERYCLASSLIST pDsQueryClassList = NULL;
    IDsDisplaySpecifier* pdds = NULL;

    TraceEnter(TRACE_FIELDCHOOSER, "CDsQuery::OnPickColumns");
    Trace(TEXT("Column count %d"), DSA_GetItemCount(_hdsaColumns));

    state.hdpaItems = NULL;
    state.hdsaColumns = _hdsaColumns;
    state.hwndProperties = NULL;
    state.hwndColumns = NULL;

    // Build a list of unique properties sorted and remove the ones
    // which match the currently displayed property set. 

    state.hdpaItems = DPA_Create(16);
    TraceAssert(state.hdpaItems);

    if ( !state.hdpaItems )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to create property DPA");

    //
    // attempt to get the IDsDisplaySpecifier object
    //

    hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&pdds);
    FailGracefully(hres, "Failed to get the IDsDisplaySpecifier interface");

    hres = pdds->SetServer(_pServer, _pUserName, _pPassword, DSSSF_DSAVAILABLE);
    FailGracefully(hres, "Failed to server information");

    // add the columns properties to the the list, marking them as active columns
    // storing their size and other information.

    for ( i = 0 ; i < DPA_GetPtrCount(_hdsaColumns); i++ )
    {
        LPCOLUMN pColumn = (LPCOLUMN)DSA_GetItemPtr(_hdsaColumns, i);
        TraceAssert(pColumn);

        LPPICKERITEM pItem = (LPPICKERITEM)LocalAlloc(LPTR, SIZEOF(PICKERITEM));
        TraceAssert(pItem);

        if ( pItem )
        {
            pItem->pProperty = NULL;
            pItem->pHeading = NULL;
            pItem->fIsColumn = TRUE;
            pItem->cx = pColumn->cx;
            pItem->fmt = pColumn->fmt;

            hres = GetPropertyFromColumn(&pItem->pProperty, pColumn);
            TraceAssert(SUCCEEDED(hres));

            if ( SUCCEEDED(hres) )
            {
                hres = LocalAllocString(&pItem->pHeading, pColumn->pHeading);
                TraceAssert(SUCCEEDED(hres));
            }

            Trace(TEXT("Adding column %d, with property %s"), i, W2T(pItem->pProperty));

            if ( FAILED(hres) || (-1 == DPA_AppendPtr(state.hdpaItems, pItem)) )
            {
                TraceMsg("Failed to add property to the DPA");
                hres = E_FAIL;
            }
        
            if ( FAILED(hres) )
                _PickerItemFreeCB(pItem, NULL);
        }
    }

    DPA_Sort(state.hdpaItems, _PickerItemCmpCB, NULL);         // sort the DPA now we have all the elements

    // for all the classes we have loop through and build the unique 
    // list, sorted as we go.

    hres = _pqf->CallForm(&_clsidForm, DSQPM_GETCLASSLIST, 0, (LPARAM)&pDsQueryClassList);
    FailGracefully(hres, "Failed to get the class list");

    if ( !pDsQueryClassList )
        ExitGracefully(hres, E_FAIL, "Failed to get the class list");

    Trace(TEXT("Classes returned from DSQPM_GETCLASSLIST %d"), pDsQueryClassList->cClasses);

    for ( i = 0 ; i < pDsQueryClassList->cClasses ; i++ )
    {
        LPWSTR pObjectClass = (LPWSTR)ByteOffset(pDsQueryClassList, pDsQueryClassList->offsetClass[i]);
        TraceAssert(pObjectClass);

        Trace(TEXT("Adding class '%s' to the property DPA"), W2T(pObjectClass));

        // allocate a DPA to be filed with items

        StringDPA_Destroy(&pea.hdpaProperties);

        pea.pps = &state;
        pea.hdpaProperties = DPA_Create(16);

        if ( !pea.hdpaProperties )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate string DPA");

        if ( SUCCEEDED(EnumClassAttributes(pdds, pObjectClass, _PickerEnumAttribCB, (LPARAM)&pea)) )
        {
            Trace(TEXT("Unique property list has %d entries"), DPA_GetPtrCount(pea.hdpaProperties));

            // Having constructed the unique list of properties for this class lets now
            // add them to the item data list and allocate real structures for them.

            for ( iProperty = 0 ; iProperty < DPA_GetPtrCount(pea.hdpaProperties); iProperty++ )
            {
                LPWSTR pProperty = StringDPA_GetStringW(pea.hdpaProperties, iProperty);
                TraceAssert(pProperty != NULL);

                LPPICKERITEM pItem = (LPPICKERITEM)LocalAlloc(LPTR, SIZEOF(PICKERITEM));
                TraceAssert(pItem);

                if ( pItem )
                {
                    WCHAR szBuffer[MAX_PATH];

                    GetFriendlyAttributeName(pdds, pObjectClass, pProperty, szBuffer, ARRAYSIZE(szBuffer));

                    pItem->pProperty = NULL;
                    pItem->pHeading = NULL;
                    pItem->fIsColumn = FALSE;
                    pItem->cx = 0;
                    pItem->fmt = 0;
    
                    hres = LocalAllocStringW(&pItem->pProperty, pProperty);
                    TraceAssert(SUCCEEDED(hres));

                    if ( SUCCEEDED(hres) )
                    {
                        hres = LocalAllocStringW2T(&pItem->pHeading, szBuffer);
                        TraceAssert(SUCCEEDED(hres));
                    }

                    if ( FAILED(hres) || (-1 == DPA_AppendPtr(state.hdpaItems, pItem)) )
                    {
                        TraceMsg("Failed to add property to the DPA");
                        hres = E_FAIL;
                    }
                    
                    if ( FAILED(hres) )
                        _PickerItemFreeCB(pItem, NULL);
                }

                DPA_Sort(state.hdpaItems, _PickerItemCmpCB, NULL);         // sort the DPA now we have all the elements
            }
        }
    }

    Trace(TEXT("Property table is %d items in size"), DPA_GetPtrCount(state.hdpaItems));

    // If the user selects OK then the DlgProc will have generated a new column
    // table stored in the PICKERSTATE structure, we should persist this, then
    // load it ready to refresh the result viewer.

    i = (int)DialogBoxParam(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDD_PICKCOLUMNS),
                       hwndParent, 
                       _PickerDlgProc, (LPARAM)&state); 
    if ( i == IDOK )
    {
        hres = _SaveColumnTable(_clsidForm, state.hdsaColumns);
        FailGracefully(hres, "Failed to write column table");

        hres = _InitNewQuery(NULL, TRUE);             // initialize the view
        FailGracefully(hres, "Failed when starting new query");

        TraceAssert(_dwThreadId);
        PostThreadMessage(_dwThreadId, RVTM_SETCOLUMNTABLE, _dwQueryReference, (LPARAM)state.hdsaColumns);

        _fColumnsModified = FALSE;
    }
   
    hres = S_OK;

exit_gracefully:

    StringDPA_Destroy(&pea.hdpaProperties);

    if ( state.hdpaItems )
        DPA_DestroyCallback(state.hdpaItems, _PickerItemFreeCB, NULL);

    if ( pDsQueryClassList )
        CoTaskMemFree(pDsQueryClassList);

    DoRelease(pdds);

    TraceLeaveValue(hres);
}
