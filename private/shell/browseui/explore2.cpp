#include "priv.h"

#ifndef UNIX

#include "sccls.h"
#include "explore2.h"
#include <iethread.h>
#include "resource.h"
#include "itbar.h"
#include "inpobj.h"

#include "mluisupp.h"

#define DM_FOCUS    DM_TRACE

#define SUPERCLASS CSHELLBROWSER

TCHAR const c_szSettings[] = TEXT("Settings");
TCHAR const c_szCabinetExpView[]    = TEXT("ExpView");

CExplorerBrowser::CExplorerBrowser()
{
    // warning: can't call SUPERCLASS until _Initialize has been called
    // (since that's what does the aggregation)
}

CExplorerBrowser::~CExplorerBrowser()
{
    if (GetUIVersion() < 5) {
        if (_hmenuTemplate)
            DestroyMenu(_hmenuTemplate);

        if (_hmenuFull)
            DestroyMenu(_hmenuFull);
    }
}

HRESULT CExplorerBrowser::_Initialize(HWND hwnd, IUnknown *pauto)
{
    HRESULT hr;
    SHELLSTATE ss = {0};

    hr = SUPERCLASS::_Initialize(hwnd, pauto);
    if (SUCCEEDED(hr)) {
        _fSubclassed = TRUE;
    }

    return hr;
}

void CExplorerBrowser::v_InitMembers()
{
    if (GetUIVersion() < 5) {
        _hmenuTemplate = _MenuTemplate(MENU_TEMPLATE, TRUE);
        _hmenuFull = _MenuTemplate(MENU_FULL, TRUE);
        _hmenuCur = _hmenuTemplate;
    } else {
        // explorer/non-explorer shell menus same on nt5
        SUPERCLASS::v_InitMembers();
    }
}

HRESULT CExplorerBrowser_CreateInstance(HWND hwnd, LPVOID* ppsb)
{
    HRESULT hr;
    CExplorerBrowser *psb = new CExplorerBrowser();

    if (psb) {
        hr = psb->_Initialize(hwnd, NULL);      // aggregation, etc.
        if (FAILED(hr)) {
            ASSERT(0);    // shouldn't happen
            ATOMICRELEASE(psb);
        }
    } else {
        // low mem
        hr = E_OUTOFMEMORY;
    }

    *ppsb = (LPVOID)psb;
    return hr;
}

IStream* CExplorerBrowser::_GetITBarStream(BOOL fWebBrowser, DWORD grfMode)
{
    return GetITBarStream(ITBS_EXPLORER, grfMode);
}

HRESULT CExplorerBrowser::OnCreate(LPCREATESTRUCT pcs)
{
    HRESULT hres = SUPERCLASS::OnCreate(pcs);
    v_ShowControl(FCW_TREE, SBSC_SHOW);
    return hres;
}

DWORD CExplorerBrowser::v_ShowControl(UINT iControl, int iCmd)
{
    int iShowing = -1;
    
    switch (iControl) {
    case FCW_TREE:
    {
        // get the current state
        iShowing = (IsControlWindowShown(FCW_TREE, NULL) == S_OK) ? SBSC_SHOW : SBSC_HIDE;
        
        if (iCmd != SBSC_QUERY) {
            // turn it on/off as requested
            VARIANTARG v = {0};
            v.vt = VT_I4;
            v.lVal = SBSC_SHOW ? 1 : 0;
            Exec(&CGID_Explorer, SBCMDID_EXPLORERBAR, 0, &v, NULL);
        }
        break;
    }
 
    default:
        return SUPERCLASS::v_ShowControl(iControl, iCmd);
    }
    
    return iShowing;
}


// BUGBUG: should go to cshellbrowser
void CExplorerBrowser::_EnableMenuItemsByAttribs(HMENU hmenu)
{    
    if (_pbbd->_pidlCur) {
        DWORD dwAttrib = SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET;
        
        IEGetAttributesOf(_pbbd->_pidlCur, &dwAttrib);

        _EnableMenuItem(hmenu, FCIDM_DELETE, (dwAttrib & SFGAO_CANDELETE));
        _EnableMenuItem(hmenu, FCIDM_RENAME, (dwAttrib & SFGAO_CANRENAME));
        _EnableMenuItem(hmenu, FCIDM_PROPERTIES, (dwAttrib & SFGAO_HASPROPSHEET));
        
    }
}

BOOL CExplorerBrowser::_ExplorerTreeHasFocus()
{
    BOOL bRet = FALSE;
    IInputObject* pio;
    if (SUCCEEDED(_QIExplorerBand(IID_IInputObject, (void**)&pio)))
    {
        bRet = (pio->HasFocusIO() == S_OK);
        pio->Release();
    }
    return bRet;
}


LRESULT CExplorerBrowser::v_OnInitMenuPopup(HMENU hmenuPopup, int nIndex, BOOL fSystemMenu)
{
    if (hmenuPopup == _GetMenuFromID(FCIDM_MENU_FILE)) 
    {
        if (_ExplorerTreeHasFocus())
            _EnableMenuItemsByAttribs(hmenuPopup);
    }
    
    return SUPERCLASS::v_OnInitMenuPopup(hmenuPopup, nIndex, fSystemMenu);
}

HRESULT CExplorerBrowser::InsertMenusSB(HMENU hmenuShared,
                            LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    SUPERCLASS::InsertMenusSB(hmenuShared, lpMenuWidths);

    if (GetUIVersion() < 5) {
        if (lpMenuWidths->width[4] == 1)
            lpMenuWidths->width[4] = 2;  // we have tools AND help for explorer mode
    }

    return S_OK;
}

void CExplorerBrowser::v_GetDefaultSettings(IETHREADPARAM *piei)
{
    if (GetUIVersion() < 5)
    {
        // set the flags
        piei->fs.fFlags = 0;

        piei->fs.ViewMode = FVM_LIST;

        ASSERT(DFS_VID_Default == VID_WebView);
#if 0 // If DFS_VID_Default differs from VID_WebView, then turn these lines back on
        piei->m_vidRestore = VID_List;
        piei->m_dwViewPriority = VIEW_PRIORITY_NONE; // let anyone override the VID_List default
#endif

        piei->wv.bStatusBar = g_dfs.bDefStatusBar;
        piei->wp.length = 0;
        piei->wHotkey = 0;
    }
    else
        SUPERCLASS::v_GetDefaultSettings(piei);
}

void CExplorerBrowser::v_ParentFolder()
{
    if (_ShouldAllowNavigateParent()) {
        BrowseObject(NULL, SBSP_PARENT | SBSP_SAMEBROWSER);
    }
}

HRESULT CExplorerBrowser::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    // turn non-explore to newbrowser
    if ((GetUIVersion() < 5) && (wFlags & SBSP_OPENMODE))
    {
        wFlags &= ~(SBSP_DEFBROWSER | SBSP_SAMEBROWSER);
        wFlags |= SBSP_NEWBROWSER;
    }
    
    return SUPERCLASS::BrowseObject(pidl, wFlags);
}

DWORD CExplorerBrowser::v_RestartFlags()
{
    return COF_CREATENEWWINDOW | COF_EXPLORE;
}

void CExplorerBrowser::v_GetAppTitleTemplate(LPTSTR pszBuffer, LPTSTR szFullName)
{
    if (GetUIVersion() < 5) {
        // "exploring - la de da"
        TCHAR szBuffer[80];
        MLLoadString(IDS_EXPLORING, szBuffer, ARRAYSIZE(szBuffer));
        wnsprintf(pszBuffer, ARRAYSIZE(szBuffer) /*Lack of something better*/, TEXT("%s - %%s"), szBuffer);
    } else {
        SUPERCLASS::v_GetAppTitleTemplate(pszBuffer, szFullName);
    }
}

void CExplorerBrowser::_UpdateFolderSettings(LPCITEMIDLIST pidl)
{
    if (GetUIVersion() < 5)
    {
        // Explorer always inherits the view a la win95
        _pbbd->_psv->GetCurrentInfo(&_fsd._fs);
    }
    else
        SUPERCLASS::_UpdateFolderSettings(pidl);
}

LPSTREAM CExplorerBrowser::v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode,
        LPCWSTR pwszName)
{
    // if it's asking for general browser info, give it the one browser info.
    // otherwise give do what our superclass does

    if ((GetUIVersion() < 5) && (StrCmpW(pwszName, L"CabView") == 0))
        return OpenRegStream(SHGetExplorerHkey(), c_szCabinetExpView, c_szSettings, grfMode);

    return SUPERCLASS::v_GetViewStream(pidl, grfMode, pwszName);
}

#endif
