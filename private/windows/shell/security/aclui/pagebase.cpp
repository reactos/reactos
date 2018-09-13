//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pagebase.cpp
//
//  This file contains the implementation of the CSecurityPage base class.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"

CSecurityPage::CSecurityPage( LPSECURITYINFO psi, SI_PAGE_TYPE siType )
: m_siPageType(siType), m_psi(psi), m_psi2(NULL), m_pObjectPicker(NULL),
  m_flLastOPOptions(DWORD(-1))
{
    ZeroMemory(&m_siObjectInfo, sizeof(m_siObjectInfo));

    // Initialize COM incase our client hasn't
    m_hrComInit = CoInitialize(NULL);

    if (m_psi != NULL)
    {
        m_psi->AddRef();

        // It's normal for this to fail
        m_psi->QueryInterface(IID_ISecurityInformation2, (LPVOID*)&m_psi2);
    }
}

CSecurityPage::~CSecurityPage( void )
{
    DoRelease(m_psi);
    DoRelease(m_psi2);
    DoRelease(m_pObjectPicker);

    if (SUCCEEDED(m_hrComInit))
        CoUninitialize();
}

HPROPSHEETPAGE
CSecurityPage::CreatePropSheetPage(LPCTSTR pszDlgTemplate, LPCTSTR pszDlgTitle)
{
    PROPSHEETPAGE psp;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USECALLBACK;  // | PSP_HASHELP;
    psp.hInstance   = ::hModule;
    psp.pszTemplate = pszDlgTemplate;
    psp.pszTitle    = pszDlgTitle;
    psp.pfnDlgProc  = CSecurityPage::_DlgProc;
    psp.lParam      = (LPARAM)this;
    psp.pfnCallback = CSecurityPage::_PSPageCallback;

    if (pszDlgTitle != NULL)
        psp.dwFlags |= PSP_USETITLE;

    return CreatePropertySheetPage(&psp);
}

HRESULT
CSecurityPage::GetObjectPicker(IDsObjectPicker **ppObjectPicker)
{
#if(_WIN32_WINNT >= 0x0500)
    HRESULT hr = S_OK;

    if (!m_pObjectPicker)
    {
        if (!m_psi)
            return E_UNEXPECTED;

        // See if the object supports IDsObjectPicker
        hr = m_psi->QueryInterface(IID_IDsObjectPicker, (LPVOID*)&m_pObjectPicker);

        // If the object doesn't support IDsObjectPicker, create one.
        if (FAILED(hr))
        {
            hr = CoCreateInstance(CLSID_DsObjectPicker,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDsObjectPicker,
                                  (LPVOID*)&m_pObjectPicker);
        }
    }

    if (ppObjectPicker)
    {
        *ppObjectPicker = m_pObjectPicker;
        // Return a reference (caller must Release)
        if (m_pObjectPicker)
            m_pObjectPicker->AddRef();
    }

    return hr;
#else
    *ppObjectPicker = NULL;
    return E_NOTIMPL;
#endif
}


#if(_WIN32_WINNT >= 0x0500)
//
// Stuff used for initializing the Object Picker below
//
#define DSOP_FILTER_COMMON1 ( DSOP_FILTER_INCLUDE_ADVANCED_VIEW \
                            | DSOP_FILTER_USERS                 \
                            | DSOP_FILTER_UNIVERSAL_GROUPS_SE   \
                            | DSOP_FILTER_GLOBAL_GROUPS_SE      \
                            | DSOP_FILTER_COMPUTERS             \
                            )
#define DSOP_FILTER_COMMON2 ( DSOP_FILTER_COMMON1               \
                            | DSOP_FILTER_WELL_KNOWN_PRINCIPALS \
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE\
                            )
#define DSOP_FILTER_COMMON3 ( DSOP_FILTER_COMMON2               \
                            | DSOP_FILTER_BUILTIN_GROUPS        \
                            )
#define DSOP_FILTER_DL_COMMON1      ( DSOP_DOWNLEVEL_FILTER_USERS           \
                                    | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS   \
                                    | DSOP_DOWNLEVEL_FILTER_COMPUTERS       \
                                    )
#define DSOP_FILTER_DL_COMMON2      ( DSOP_FILTER_DL_COMMON1                    \
                                    | DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS  \
                                    )
#define DSOP_FILTER_DL_COMMON3      ( DSOP_FILTER_DL_COMMON2                \
                                    | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS    \
                                    )
// Same as DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS, except no CREATOR flags.
// Note that we need to keep this in sync with any object picker changes.
#define DSOP_FILTER_DL_WELLKNOWN    ( DSOP_DOWNLEVEL_FILTER_WORLD               \
                                    | DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER  \
                                    | DSOP_DOWNLEVEL_FILTER_ANONYMOUS           \
                                    | DSOP_DOWNLEVEL_FILTER_BATCH               \
                                    | DSOP_DOWNLEVEL_FILTER_DIALUP              \
                                    | DSOP_DOWNLEVEL_FILTER_INTERACTIVE         \
                                    | DSOP_DOWNLEVEL_FILTER_NETWORK             \
                                    | DSOP_DOWNLEVEL_FILTER_SERVICE             \
                                    | DSOP_DOWNLEVEL_FILTER_SYSTEM              \
                                    | DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER     \
                                    )

#if 0
{   // DSOP_SCOPE_INIT_INFO
    cbSize,
    flType,
    flScope,
    {   // DSOP_FILTER_FLAGS
        {   // DSOP_UPLEVEL_FILTER_FLAGS
            flBothModes,
            flMixedModeOnly,
            flNativeModeOnly
        },
        flDownlevel
    },
    pwzDcName,
    pwzADsPath,
    hr // OUT
}
#endif

#define DECLARE_SCOPE(t,f,b,m,n,d)  \
{ sizeof(DSOP_SCOPE_INIT_INFO), (t), (f), { { (b), (m), (n) }, (d) }, NULL, NULL, S_OK }

// The domain to which the target computer is joined.
// Make 2 scopes, one for uplevel domains, the other for downlevel.
#define JOINED_DOMAIN_SCOPE(f)  \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,(f),0,(DSOP_FILTER_COMMON2 & ~(DSOP_FILTER_UNIVERSAL_GROUPS_SE|DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE)),DSOP_FILTER_COMMON2,0), \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,(f),0,0,0,DSOP_FILTER_DL_COMMON2)

// The domain for which the target computer is a Domain Controller.
// Make 2 scopes, one for uplevel domains, the other for downlevel.
#define JOINED_DOMAIN_SCOPE_DC(f)  \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,(f),0,(DSOP_FILTER_COMMON3 & ~DSOP_FILTER_UNIVERSAL_GROUPS_SE),DSOP_FILTER_COMMON3,0), \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,(f),0,0,0,DSOP_FILTER_DL_COMMON3)

// Target computer scope.  Computer scopes are always treated as
// downlevel (i.e., they use the WinNT provider).
#define TARGET_COMPUTER_SCOPE(f)\
DECLARE_SCOPE(DSOP_SCOPE_TYPE_TARGET_COMPUTER,(f),0,0,0,DSOP_FILTER_DL_COMMON3)

// The Global Catalog
#define GLOBAL_CATALOG_SCOPE(f) \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_GLOBAL_CATALOG,(f),DSOP_FILTER_COMMON1|DSOP_FILTER_WELL_KNOWN_PRINCIPALS,0,0,0)

// The domains in the same forest (enterprise) as the domain to which
// the target machine is joined.  Note these can only be DS-aware
#define ENTERPRISE_SCOPE(f)     \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,(f),DSOP_FILTER_COMMON1,0,0,0)

// Domains external to the enterprise but trusted directly by the
// domain to which the target machine is joined.
#define EXTERNAL_SCOPE(f)       \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN|DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,\
    (f),DSOP_FILTER_COMMON1,0,0,DSOP_DOWNLEVEL_FILTER_USERS|DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS)

// Workgroup scope.  Only valid if the target computer is not joined
// to a domain.
#define WORKGROUP_SCOPE(f)      \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_WORKGROUP,(f),0,0,0,DSOP_FILTER_DL_COMMON1|DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS)

//
// Array of Default Scopes
//
static const DSOP_SCOPE_INIT_INFO g_aDefaultScopes[] =
{
    JOINED_DOMAIN_SCOPE(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    TARGET_COMPUTER_SCOPE(0),
    GLOBAL_CATALOG_SCOPE(0),
    ENTERPRISE_SCOPE(0),
    EXTERNAL_SCOPE(0),
};

//
// Same as above, but without the Target Computer
// Used when the target is a Domain Controller
//
static const DSOP_SCOPE_INIT_INFO g_aDCScopes[] =
{
    JOINED_DOMAIN_SCOPE_DC(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    GLOBAL_CATALOG_SCOPE(0),
    ENTERPRISE_SCOPE(0),
    EXTERNAL_SCOPE(0),
};

//
// Array of scopes for standalone machines
//
static const DSOP_SCOPE_INIT_INFO g_aStandAloneScopes[] =
{
    TARGET_COMPUTER_SCOPE(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    WORKGROUP_SCOPE(0),
};

//
// Attributes that we want the Object Picker to retrieve
//
static const LPCTSTR g_aszOPAttributes[] =
{
    TEXT("ObjectSid"),
};


HRESULT
CSecurityPage::InitObjectPicker(BOOL bMultiSelect)
{
    HRESULT hr = S_OK;
    DSOP_INIT_INFO InitInfo;
    PCDSOP_SCOPE_INIT_INFO pScopes;
    ULONG cScopes;

    USES_CONVERSION;

    TraceEnter(TRACE_MISC, "InitObjectPicker");

    hr = GetObjectPicker();
    if (FAILED(hr))
        TraceLeaveResult(hr);

    TraceAssert(m_pObjectPicker != NULL);

    InitInfo.cbSize = sizeof(InitInfo);
    // We do the DC check at WM_INITDIALOG
    InitInfo.flOptions = DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK;
    if (bMultiSelect)
        InitInfo.flOptions |= DSOP_FLAG_MULTISELECT;

    // flOptions is the only thing that changes from call to call,
    // so optimize this by only reinitializing if flOptions changes.
    if (m_flLastOPOptions == InitInfo.flOptions)
        TraceLeaveResult(S_OK); // Already initialized

    m_flLastOPOptions = (DWORD)-1;

    pScopes = g_aDefaultScopes;
    cScopes = ARRAYSIZE(g_aDefaultScopes);

    if (m_bStandalone)
    {
        cScopes = ARRAYSIZE(g_aStandAloneScopes);
        pScopes = g_aStandAloneScopes;
    }
    else if (m_siObjectInfo.dwFlags & SI_SERVER_IS_DC)
    {
        cScopes = ARRAYSIZE(g_aDCScopes);
        pScopes = g_aDCScopes;
    }

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //
    InitInfo.pwzTargetComputer = T2CW(m_siObjectInfo.pszServerName);
    InitInfo.cDsScopeInfos = cScopes;
    InitInfo.aDsScopeInfos = (PDSOP_SCOPE_INIT_INFO)LocalAlloc(LPTR, sizeof(*pScopes)*cScopes);
    if (!InitInfo.aDsScopeInfos)
        TraceLeaveResult(E_OUTOFMEMORY);
    CopyMemory(InitInfo.aDsScopeInfos, pScopes, sizeof(*pScopes)*cScopes);
    InitInfo.cAttributesToFetch = ARRAYSIZE(g_aszOPAttributes);
    InitInfo.apwzAttributeNames = (LPCTSTR*)g_aszOPAttributes;

    if ((m_siObjectInfo.dwFlags & SI_SERVER_IS_DC) || !(m_siObjectInfo.dwFlags & SI_CONTAINER))
    {
        for (ULONG i = 0; i < cScopes; i++)
        {
            // Set the DC name if appropriate
            if ((m_siObjectInfo.dwFlags & SI_SERVER_IS_DC) &&
                (InitInfo.aDsScopeInfos[i].flType & DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN))
            {
                InitInfo.aDsScopeInfos[i].pwzDcName = InitInfo.pwzTargetComputer;
            }

            // Turn off CREATOR_OWNER & CREATOR_GROUP for non-containers
            if (!(m_siObjectInfo.dwFlags & SI_CONTAINER) &&
                (InitInfo.aDsScopeInfos[i].FilterFlags.flDownlevel & DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS))
            {
                InitInfo.aDsScopeInfos[i].FilterFlags.flDownlevel &= ~DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;
                InitInfo.aDsScopeInfos[i].FilterFlags.flDownlevel |= DSOP_FILTER_DL_WELLKNOWN;
            }
        }
    }

    hr = m_pObjectPicker->Initialize(&InitInfo);

    if (SUCCEEDED(hr))
    {
        // Remember the Options for next time
        m_flLastOPOptions = InitInfo.flOptions;
    }

    LocalFree(InitInfo.aDsScopeInfos);

    TraceLeaveResult(hr);
}
#endif  // (_WIN32_WINNT >= 0x0500)


HRESULT
CSecurityPage::GetUserGroup(HWND hDlg, BOOL bMultiSelect, PUSER_LIST *ppUserList)
{
#if(_WIN32_WINNT < 0x0500)
    DWORD dwFlags = 0;

    if (bMultiSelect)
        dwFlags |= GU_MULTI_SELECT;
    if (m_siObjectInfo.dwFlags & SI_CONTAINER)
        dwFlags |= GU_CONTAINER;
    if (m_siObjectInfo.dwFlags & SI_SERVER_IS_DC)
        dwFlags |= GU_DC_SERVER;
    if (m_siPageType == SI_PAGE_AUDIT)
        dwFlags |= GU_AUDIT_HLP;

    return ::GetUserGroup(hDlg,
                          dwFlags,
                          m_siObjectInfo.pszServerName,
                          m_bStandalone,
                          ppUserList);
#else   // (_WIN32_WINNT >= 0x0500)
    HRESULT hr;
    LPDATAOBJECT pdoSelection = NULL;
    STGMEDIUM medium = {0};
    FORMATETC fe = { (CLIPFORMAT)g_cfDsSelectionList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    PDS_SELECTION_LIST pDsSelList = NULL;
    HCURSOR hcur = NULL;
    PSIDCACHE pSidCache = NULL;
    UINT idErrorMsg = IDS_GET_USER_FAILED;

    TraceEnter(TRACE_MISC, "GetUserGroup");
    TraceAssert(ppUserList != NULL);

    *ppUserList = NULL;

    //
    // Create and initialize the Object Picker object
    //
    hr = InitObjectPicker(bMultiSelect);
    FailGracefully(hr, "Unable to initialize Object Picker object");

    //
    // Create the global sid cache object, if necessary
    //
    pSidCache = GetSidCache();
    if (pSidCache == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create SID cache");

    //
    // Bring up the object picker dialog
    //
    hr = m_pObjectPicker->InvokeDialog(hDlg, &pdoSelection);
    FailGracefully(hr, "IDsObjectPicker->Invoke failed");
    if (S_FALSE == hr)
        ExitGracefully(hr, S_FALSE, "IDsObjectPicker->Invoke cancelled by user");

    hr = pdoSelection->GetData(&fe, &medium);
    FailGracefully(hr, "Unable to get CFSTR_DSOP_DS_SELECTION_LIST from DataObject");

    pDsSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);
    if (!pDsSelList)
        ExitGracefully(hr, E_FAIL, "Unable to lock stgmedium.hGlobal");

    TraceAssert(pDsSelList->cItems > 0);
    Trace((TEXT("%d items selected"), pDsSelList->cItems));

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Lookup the names/sids and cache them
    //
    if (!pSidCache->LookupNames(pDsSelList,
                                m_siObjectInfo.pszServerName,
                                ppUserList,
                                m_bStandalone))
    {
        hr = E_FAIL;
        idErrorMsg = IDS_SID_LOOKUP_FAILED;
    }

    SetCursor(hcur);

exit_gracefully:
    
    if (pSidCache)
        pSidCache->Release();

    if (FAILED(hr))
    {
        SysMsgPopup(hDlg,
                    MAKEINTRESOURCE(idErrorMsg),
                    MAKEINTRESOURCE(IDS_SECURITY),
                    MB_OK | MB_ICONERROR,
                    ::hModule,
                    hr);
    }

    if (pDsSelList)
        GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);
    DoRelease(pdoSelection);

    TraceLeaveResult(hr);
#endif  // (_WIN32_WINNT >= 0x0500)
}

UINT
CSecurityPage::PSPageCallback(HWND hwnd,
                              UINT uMsg,
                              LPPROPSHEETPAGE /*ppsp*/)
{
    m_hrLastPSPCallbackResult = E_FAIL;

    if (m_psi != NULL)
    {
        m_hrLastPSPCallbackResult = m_psi->PropertySheetPageCallback(hwnd, uMsg, m_siPageType);
        if (m_hrLastPSPCallbackResult == E_NOTIMPL)
            m_hrLastPSPCallbackResult = S_OK;
    }
    else
    {
        // BUGBUG put up message here?
    }

    return SUCCEEDED(m_hrLastPSPCallbackResult);
}

INT_PTR
CALLBACK
CSecurityPage::_DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPSECURITYPAGE pThis = (LPSECURITYPAGE)GetWindowLongPtr(hDlg, DWLP_USER);

    // BUGBUG the following messages arrive before WM_INITDIALOG
    // which means pThis is NULL for them.  We don't need these
    // messages so let DefDlgProc handle them.
    //
    // WM_SETFONT
    // WM_NOTIFYFORMAT
    // WM_NOTIFY (LVN_HEADERCREATED)

    if (uMsg == WM_INITDIALOG)
    {
        pThis = (LPSECURITYPAGE)(((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);

        if (pThis)
        {
            if (!pThis->PSPageCallback(hDlg, PSPCB_SI_INITDIALOG, NULL))
                pThis->m_bAbortPage = TRUE;

            if (pThis->m_psi)
            {
                BOOL bIsDC = FALSE;
                pThis->m_psi->GetObjectInformation(&pThis->m_siObjectInfo);
                pThis->m_bStandalone = IsStandalone(pThis->m_siObjectInfo.pszServerName, &bIsDC);
                if (bIsDC)
                    pThis->m_siObjectInfo.dwFlags |= SI_SERVER_IS_DC;
            }
        }
    }

    if (pThis != NULL)
        return pThis->DlgProc(hDlg, uMsg, wParam, lParam);

    return FALSE;
}

UINT
CALLBACK
CSecurityPage::_PSPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    LPSECURITYPAGE pThis = (LPSECURITYPAGE)ppsp->lParam;

    if (pThis)
    {
        UINT nResult = pThis->PSPageCallback(hWnd, uMsg, ppsp);

        switch (uMsg)
        {
        case PSPCB_CREATE:
            if (!nResult)
                pThis->m_bAbortPage = TRUE;
            break;

        case PSPCB_RELEASE:
            delete pThis;
            break;
        }
    }

    //
    // Always return non-zero or else our tab will disappear and whichever
    // property page becomes active won't repaint properly.  Instead, use
    // the m_bAbortPage flag during WM_INITDIALOG to disable the page if
    // the callback failed.
    //
    return 1;
}
