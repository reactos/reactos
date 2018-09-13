#include "priv.h"

#ifndef UNIX

#include "sccls.h"
#include "explband.h"
#include "resource.h"
#include "inpobj.h"
#include "onetree.h"
#include "uemapp.h"

#include <iethread.h>
#include <shellp.h>

#include "mluisupp.h"

typedef struct {
    CExplorerBand* _peb;
    LPCITEMIDLIST _pidlSearch;
    IShellFolder* _psf;
} STRSEARCH;

#define SUPERCLASS CToolBand

extern DWORD g_dwStopWatchMode;  // Shell performance mode

const TCHAR c_szLink[] = TEXT("link");
const TCHAR c_szRename[] = TEXT("rename");
const TCHAR c_szMove[] = TEXT("cut");
const TCHAR c_szPaste[] = TEXT("paste");
const TCHAR c_szCopy[] = TEXT("copy");
const TCHAR c_szDelete[] = TEXT("delete");
const TCHAR c_szProperties[] = TEXT("properties");

HRESULT CExplorerBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;
    CExplorerBand* peb = new CExplorerBand();
    if (peb)
    {
        *ppunk = SAFECAST(peb, IDeskBand*);
        hres = NOERROR;
    }
    // BUGBUG This shouldn't be an assert
    // since it's not a logic error to run out of memry
    ASSERT(SUCCEEDED(hres));
    return hres;
}

long g_cExplorerBands = 0;

CExplorerBand::CExplorerBand()
{
    _cRef = 1;
    _lEvents =  SHCNE_ASSOCCHANGED | SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_MKDIR |
                SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_ATTRIBUTES | SHCNE_NETSHARE | SHCNE_NETUNSHARE |
                SHCNE_DRIVEREMOVED | SHCNE_MEDIAREMOVED | SHCNE_DRIVEADD | SHCNE_DRIVEADDGUI |
                SHCNE_MEDIAINSERTED | SHCNE_SERVERDISCONNECT | SHCNE_UPDATEIMAGE;

    InterlockedIncrement(&g_cExplorerBands);
}


STDAPI_(LPITEMIDLIST) IEGetInternetRootID(void);

typedef struct 
{
    LPOneTreeNode lpn;
    IShellFolder *psf;
} SFCITEM;

int CALLBACK _ReleaseSFC(void *p, void *pv)
{
    SFCITEM *psfc = (SFCITEM *)p;
    OTRelease(psfc->lpn);
    psfc->psf->Release();
    return TRUE;
}

CExplorerBand::~CExplorerBand()
{
    if (0 == InterlockedDecrement(&g_cExplorerBands))
    {   
        //
        //  we want to cleanup the onetree data underneath of 
        //  the Internet SF.  otherwise it will
        //  accumulate until process exit.
        LPITEMIDLIST pidl = IEGetInternetRootID();
        
        if (pidl)
        {
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, (void*)pidl, NULL);
            ILFree(pidl);
        }
    }
    ASSERT(g_cExplorerBands >= 0);
    
    ATOMICRELEASE(_punkSite);
    OTUnregister(_hwnd);
    if (_hdpaEnum)
    {
        // If this assert fires, it means we were destroyed while there
        // are still items being watched.
        ASSERT(DPA_GetPtrCount(_hdpaEnum) == 0);
        DPA_Destroy(_hdpaEnum);
    }

    Str_SetPtr(&_pszUnedited, NULL);
    ILFree(_pidlRootCache);

    if (_hdsaLegacySFC)
        DSA_DestroyCallback(_hdsaLegacySFC, _ReleaseSFC, NULL);
}

// *** IUnknown methods ***

HRESULT CExplorerBand::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    static const QITAB qit[] = {
        QITABENT(CExplorerBand, IWinEventHandler),
        QITABENT(CExplorerBand, IDispatch),
        QITABENT(CExplorerBand, IShellChangeNotify),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);
    return hres;
}


ULONG CExplorerBand::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CExplorerBand::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// *** IDockingWindow methods ***

HRESULT CExplorerBand::CloseDW(DWORD dw)
{
    _ConnectToBrowser(FALSE);
    RevokeDragDrop(_hwnd);
    _UnregisterWindow(_hwnd);
    _UnsubclassWindow(_hwnd);
    return SUPERCLASS::CloseDW(dw);
}

HRESULT CExplorerBand::ShowDW(BOOL fShow)
{
    int nShowCmd = fShow ? SW_SHOW : SW_HIDE;
    if (fShow && !_fNavigated)
        _OnNavigate();
    ShowWindow(_hwnd, nShowCmd);

    return S_OK;
}

// *** IObjectWithSite methods ***

HRESULT CExplorerBand::SetSite(IUnknown* punkSite)
{
    HRESULT hres = SUPERCLASS::SetSite(punkSite);
    if (_hwndParent && !_hwnd) 
    {
        hres = _CreateTree();
        if (FAILED(hres))
            return hres;
        _ConnectToBrowser(TRUE);
        THR(RegisterDragDrop(_hwnd, &_tdt));
    }
    return hres;
}

// *** IInputObject methods ***

HRESULT CExplorerBand::TranslateAcceleratorIO(LPMSG lpmsg) 
{ 
    HWND hwndFocus = GetFocus(); 

    if (hwndFocus == _hwnd)
    {
        if ((lpmsg->message == g_msgMSWheel || lpmsg->message == WM_MOUSEWHEEL) &&
            _FilterMouseWheel(lpmsg->hwnd, lpmsg->message, lpmsg->wParam, lpmsg->lParam))
        {
            return S_OK;
        }
        else if (TranslateAcceleratorWrap(_hwnd, _haccTree, lpmsg))
        {
            return S_OK;
        }
        else if (SendMessage(_hwnd, TVM_TRANSLATEACCELERATOR, 0, (LPARAM)lpmsg))
        {
            return S_OK;
        }
    }
    else if (IsChild(_hwnd, hwndFocus))
    {
        return EditBox_TranslateAcceleratorST(lpmsg);
    }
    
    return S_FALSE;
}

// *** IDispatch methods ***

HRESULT CExplorerBand::Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                  DISPPARAMS * pdispparams, VARIANT * pvarResult,
                  EXCEPINFO * pexcepinfo,UINT * puArgErr)
{
    HRESULT hr = S_OK;

    if (!pdispparams)
        return E_INVALIDARG;

    switch(dispidMember)
    {
        case DISPID_DOCUMENTCOMPLETE:
        case DISPID_NAVIGATECOMPLETE2:
            _OnNavigate();
            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}

// *** IShellChangeNotify methods ***

HRESULT CExplorerBand::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    return _TreeHandleFileSysChange(lEvent, pidl1, pidl2);
}

HRESULT _GetCtxMenu(HWND hwnd, LPCITEMIDLIST pidlCur, IContextMenu **ppcm)
{
    IShellFolder *psf;
    HRESULT hr = E_FAIL;
    
    *ppcm = NULL;
    if (ILIsEmpty(pidlCur))
    {
        // it's the desktop folder. do a CreateViewObject
        if (SUCCEEDED(SHGetDesktopFolder(&psf)))
        {
            hr = psf->CreateViewObject(hwnd, IID_IContextMenu, (void **)ppcm);
            psf->Release();
        }
    }
    else
    {
        LPCITEMIDLIST pidlChild;
        if (SUCCEEDED(IEBindToParentFolder(pidlCur, &psf, &pidlChild)))
        {
            hr = psf->GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pidlChild, IID_IContextMenu, NULL, (void **)ppcm);
            psf->Release();
        }
    }

    return hr;
}


//
// BUGBUG: none of our callers verify that the commands are legal!!!
//
BOOL CExplorerBand::_InvokeCommandOnItem(LPCTSTR pszCmd)
{
    BOOL fSuccess = FALSE;
    HTREEITEM hti = TreeView_GetSelection(_hwnd);
    LPITEMIDLIST pidlCur = _TreeGetAbsolutePidl(hti);

    ASSERT(_hwnd);

    if (pidlCur)
    {
        IContextMenu *pcm;
        CMINVOKECOMMANDINFOEX ici = {
            SIZEOF(CMINVOKECOMMANDINFOEX),
            0L,
            _hwnd,
            NULL,
            NULL, NULL,
            SW_NORMAL,
        };
#ifdef UNICODE
        USES_CONVERSION;
        ici.lpVerb = W2A(pszCmd);
        ici.lpVerbW = pszCmd;
        ici.fMask |= CMIC_MASK_UNICODE;
#else
        ici.lpVerb = pszCmd;
#endif

        if (SUCCEEDED(_GetCtxMenu(_hwnd, pidlCur, &pcm)))
        {
            fSuccess = SUCCEEDED(pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici));
            pcm->Release();
        }
            
        ILFree(pidlCur);
    }
    
    // do any visuals for cut state
    if (fSuccess && _hwnd && !lstrcmpi(pszCmd, c_szMove)) 
    {
        HTREEITEM hti = TreeView_GetSelection(_hwnd);
        if (hti) 
        {
            _TreeSetItemState(hti, TVIS_CUT, TVIS_CUT);
            ASSERT(!_hwndNextViewer);
            _hwndNextViewer = SetClipboardViewer(_hwnd);
            TraceMsg(DM_TRACE, "CABINET: Set ClipboardViewer %d %d", _hwnd, _hwndNextViewer);
            _htiCut = hti;
        }
    }
        
    return fSuccess;
}

#define IsDescendent(hwndParent, hwndChild) (IsChildOrSelf(hwndParent, hwndChild) == S_OK)

// BUGBUG: (toddgre) this function is currently busted
#define _hwndView 0
BOOL CExplorerBand::_FilterMouseWheel(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    POINT pt;
    HWND hwndT = NULL;
    LRESULT lRes = 0;
    RECT rc;

    // Don't do any funky processing if this isn't the original message
    // generated by the driver, infinite loop city!
    if (uMsg == g_msgMSWheel) 
    {
        if (hwnd != _hwnd)
            return FALSE;
    } 
    else 
    {
        if (hwnd != GetFocus())
            return FALSE;
    }

    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    hwndT = WindowFromPoint(pt);
    if (hwndT != _hwnd && _hwndView) 
    {
        if (uMsg == g_msgMSWheel)   // Need propogation to direct child?
        {
            if (IsDescendent(_hwndView, hwndT))
                hwndT = _hwndView;
            else
                hwndT = NULL;
        }
    }

    // If we didn't hit either then pass it to the focus window
    if (!hwndT) 
    {
        // Bail completely in the data zooming case if the mouse lies outside
        // our main window.
        if (GetKeyState(VK_SHIFT) < 0) 
        {
            GetWindowRect(_hwnd, &rc);
            if (!PtInRect(&rc, pt))
                return FALSE;
        }
        hwndT = GetFocus();
        // If the focus window is a child of the view window, then pass
        // the message to the view window.
        if (uMsg == g_msgMSWheel && _hwndView && IsDescendent(_hwndView, hwndT))
            hwndT = _hwndView;
    }

    lRes = SendMessage(hwndT, uMsg, wParam, lParam);

    // If the message wasn't handled and shift is down, map
    // to history navigation.
    if (!lRes && GetKeyState(VK_SHIFT) < 0) 
    {
        IBrowserService* pibs;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IBrowserService, (void**)&pibs)))
        {
            pibs->NavigateToPidl(NULL, ((int)wParam < 0) ? HLNF_NAVIGATINGBACK : HLNF_NAVIGATINGFORWARD);
            pibs->Release();
        }
    }

    return TRUE;
#else
    return FALSE;
#endif
}

LPITEMIDLIST CExplorerBand::_GetPidlCur()
{
    LPITEMIDLIST pidl = NULL;
    IBrowserService* pibs;

    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IBrowserService, (void**)&pibs)))
    {
        pibs->GetPidl(&pidl);
        pibs->Release();
    }

    return pidl;
}

BOOL CExplorerBand::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
    HWND hwndControl = GET_WM_COMMAND_HWND(wParam, lParam);
    BOOL fSelChange = BOOLIFY(_nSelChangeTimer);
    
    switch (idCmd) 
    {
    case FCIDM_MOVE:
        _InvokeCommandOnItem(c_szMove);
        break;

    case FCIDM_COPY:
        _InvokeCommandOnItem(c_szCopy);
        break;

    case FCIDM_PASTE:
        _InvokeCommandOnItem(c_szPaste);
        break;

    case FCIDM_LINK:
        _InvokeCommandOnItem(c_szLink);
        break;

    case FCIDM_DELETE:
        _InvokeCommandOnItem(c_szDelete);
        if (_hwnd)
            SHChangeNotifyHandleEvents();
        break;

    case FCIDM_PROPERTIES:
        _InvokeCommandOnItem(c_szProperties);
        break;

    case FCIDM_RENAME:
        {
            HTREEITEM hti = TreeView_GetSelection(_hwnd);
            if (hti)
                TreeView_EditLabel(_hwnd, hti);
        }
        break;

    default:
        // nothing
        return FALSE;
    }

    // do it here because selchagne is async, and if the post
    // hits when a delete confirm is up (for example), 
    // fCannotNavigate will blow off the selchange
    if (fSelChange)
        _TreeRealHandleSelChange();
    return TRUE;
}

typedef struct tagCmdPair {
    DWORD dwCmdID;
    UINT uiFcidm;
} CmdPair;

const CmdPair c_CmdTableOle[] = {
    { OLECMDID_CUT,         FCIDM_MOVE       },
    { OLECMDID_COPY,        FCIDM_COPY       },
    { OLECMDID_PASTE,       FCIDM_PASTE      },
    { OLECMDID_DELETE,      FCIDM_DELETE     },
    { OLECMDID_PROPERTIES,  FCIDM_PROPERTIES },
};

const CmdPair c_CmdTableSB[] = {
    { SBCMDID_FILERENAME,      FCIDM_RENAME     },
    { SBCMDID_FILEDELETE,      FCIDM_DELETE     },
    { SBCMDID_FILEPROPERTIES,  FCIDM_PROPERTIES },
};

UINT FcidmFromCmdid(DWORD dwCmdID, const CmdPair c_CmdTable[], int cSize)
{
    DWORD uiFcidm = 0;

    for (int i=0; i < cSize; i++)
    {
        if (c_CmdTable[i].dwCmdID == dwCmdID)
        {
            uiFcidm = c_CmdTable[i].uiFcidm;
            break;
        }
    }

    return uiFcidm;
}

HRESULT CExplorerBand::_QueryStatusExplorer(ULONG cCmds, OLECMD rgCmds[])
{
    HRESULT hres = E_INVALIDARG;
    LPITEMIDLIST pidlCur = _GetPidlCur();
    if (pidlCur)
    {
        DWORD dwAttrib = SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET;        
        IEGetAttributesOf(pidlCur, &dwAttrib);

        for (ULONG cItem = 0; cItem < cCmds; cItem++)
        {
            DWORD nCmdID;
        
            static const DWORD tbtab[] = {
                SBCMDID_FILEDELETE, SBCMDID_FILEPROPERTIES, SBCMDID_FILERENAME };
            static const DWORD cttab[] = {
                SFGAO_CANDELETE,    SFGAO_HASPROPSHEET,     SFGAO_CANRENAME };

            nCmdID = SHSearchMapInt((int*)tbtab, (int*)cttab, ARRAYSIZE(tbtab), rgCmds[cItem].cmdID);

            if (nCmdID != -1 && (dwAttrib & nCmdID))
                rgCmds[cItem].cmdf = OLECMDF_ENABLED;
        }

        ILFree(pidlCur);

        hres = S_OK;
    }
    return hres;
}

// *** IOleCommandTarget methods ***

HRESULT CExplorerBand::QueryStatus(const GUID *pguidCmdGroup,
                                   ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres;

    ASSERT(_hwnd);

    if (rgCmds == NULL)
    {
        hres = E_INVALIDARG;
    }
    else if (pguidCmdGroup == NULL)
    {
        IContextMenu *pcm = NULL;
        LPITEMIDLIST pidlCur = _GetPidlCur();

        if (pidlCur && SUCCEEDED(_GetCtxMenu(_hwnd, pidlCur, &pcm)))
        {
            HMENU hmenu = CreatePopupMenu();
            if (hmenu)
            {
                if (SUCCEEDED(pcm->QueryContextMenu(hmenu, 0, 0, 255, 0)))
                {
                    UINT ilast = GetMenuItemCount(hmenu);
                    for (UINT ipos=0; ipos < ilast; ipos++)
                    {
                        MENUITEMINFO mii = {0};
                        TCHAR szVerb[40];
                        UINT idCmd;

                        mii.cbSize = SIZEOF(MENUITEMINFO);
                        mii.fMask = MIIM_ID|MIIM_STATE;

                        if (!GetMenuItemInfoWrap(hmenu, ipos, TRUE, &mii)) continue;
                        if (0 != (mii.fState & (MF_GRAYED|MF_DISABLED))) continue;
                        idCmd = mii.wID;

                        hres = ContextMenu_GetCommandStringVerb(pcm, idCmd, szVerb, ARRAYSIZE(szVerb));
                        
                        if (SUCCEEDED(hres))
                        {
                            LPCTSTR szCmd = NULL;

                            for (ULONG cItem = 0; cItem < cCmds; cItem++)
                            {
                                switch (rgCmds[cItem].cmdID)
                                {
                                case OLECMDID_CUT:
                                    szCmd = c_szMove;
                                    break;
                                case OLECMDID_COPY:
                                    szCmd = c_szCopy;
                                    break;
                                case OLECMDID_PASTE:
                                    szCmd = c_szPaste;
                                    break;
                                case OLECMDID_DELETE:
                                    szCmd = c_szDelete;
                                    break;
                                case OLECMDID_PROPERTIES:
                                    szCmd = c_szProperties;
                                    break;
                                }
                                if (lstrcmpi(szVerb, szCmd)==0)
                                {
                                    rgCmds[cItem].cmdf = OLECMDF_ENABLED;
                                }
                            }
                        }
                    }
                }
                DestroyMenu(hmenu);
            }
            pcm->Release();
        }

        ILFree(pidlCur);
            
        hres = S_OK;
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        hres = _QueryStatusExplorer(cCmds, rgCmds);
    }
    else
    {
        hres = SUPERCLASS::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
    }
    return hres;
}

HRESULT CExplorerBand::Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn,
        VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;
    UINT uiFcidm;

    if (pguidCmdGroup == NULL)
    {
        switch(nCmdID) 
        {
        case OLECMDID_CUT:
        case OLECMDID_COPY:
        case OLECMDID_PASTE:
        case OLECMDID_DELETE:
        case OLECMDID_PROPERTIES:
            uiFcidm = FcidmFromCmdid(nCmdID, c_CmdTableOle, ARRAYSIZE(c_CmdTableOle));
            if (uiFcidm)
            {
                _OnCommand(MAKELPARAM(uiFcidm, 0), 0);
                hres = S_OK;
            }
            return hres;

        case OLECMDID_REFRESH:
            OTInvalidateAll();
            _TreeRefreshAll();
            break;

        default:
            break;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SBCMDID_FILERENAME:
        case SBCMDID_FILEDELETE:
        case SBCMDID_FILEPROPERTIES:
            uiFcidm = FcidmFromCmdid(nCmdID, c_CmdTableSB, ARRAYSIZE(c_CmdTableSB));
            if (uiFcidm)
            {
                _OnCommand(MAKELPARAM(uiFcidm, 0), 0);
                hres = S_OK;
            }
            return hres;
        }
    }

    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

void CExplorerBand::_OnNavigate()
{
    LPITEMIDLIST pidl = _GetPidlCur();
    if (pidl) 
    {
        HTREEITEM hti = _TreeBuild(pidl, !_fNavigated, TRUE);

        _MaybeAddToLegacySFC(_TreeGetFCTreeData(hti), pidl, NULL);
            
        _fNavigated = TRUE;
        ILFree(pidl);
    }
}

HRESULT CExplorerBand::_ConnectToBrowser(BOOL fConnect)
{
    IBrowserService* pibs;
    IConnectionPointContainer* pcpc;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IBrowserService, (void**)&pibs);
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryService(pibs, SID_SWebBrowserApp, IID_IConnectionPointContainer, (void **)&pcpc);
        pibs->Release();
        // Let's now have the Browser Window give us notification when something happens.
        if (SUCCEEDED(hr))
        {
            hr = ConnectToConnectionPoint(SAFECAST(this, IDispatch*), DIID_DWebBrowserEvents2, fConnect,
                                          pcpc, &_dwcpCookie, NULL);
            pcpc->Release();
        }
    }
    ASSERT(SUCCEEDED(hr));
    return hr;
}

void InitExplorerGlobals()
{
    static BOOL fInited = FALSE;
    
    if (!fInited)
    {
        ENTERCRITICAL;
        if (!fInited) 
        {
            fInited = TRUE;
        }
        LEAVECRITICAL;
    }
}

LPCITEMIDLIST CExplorerBand::_pidlRoot(void)
{
    if (!_pidlRootCache)
    {
        LPITEMIDLIST pidlCur = _GetPidlCur();
        if (pidlCur)
        {
            if (ILIsRooted(pidlCur))
                _pidlRootCache = ILCloneFirst(pidlCur);
            ILFree(pidlCur);

            if (!_pidlRootCache)
                _pidlRootCache = ILClone(&s_idlNULL);
        }

    }

    return _pidlRootCache;
}
            

HRESULT CExplorerBand::_CreateTree()
{
    // perform misc initialization
   DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | TVS_HASBUTTONS 
                   | TVS_EDITLABELS | TVS_HASLINES;

    _fCanFocus = TRUE;
    _haccTree = LoadAccelerators(MLGetHinst(), MAKEINTRESOURCE(ACCEL_MERGEEXPLORER));
    InitExplorerGlobals();

    if (_hdpaEnum == NULL && !(_hdpaEnum = DPA_Create(4)))
    {
        return E_OUTOFMEMORY;
    }
    // If the parent window is mirrored then the treeview window will inheret the mirroring flag
    // And we need the reading order to be Left to right, which is the right to left in the mirrored mode.

    if (IS_WINDOW_RTL_MIRRORED(_hwndParent)) 
    {
        // This means left to right reading order because this window will be mirrored.
        dwStyle |= TVS_RTLREADING;
    }


    // now the tree
    _hwnd = CreateWindowEx(0, WC_TREEVIEW, NULL, dwStyle,
                0, 0, 0, 0, _hwndParent, (HMENU)FCIDM_TREE, HINST_THISDLL, NULL);

    if (_hwnd)
    {
        HIMAGELIST himlSysSmall;
        Shell_GetImageLists(NULL, &himlSysSmall);

        TreeView_SetImageList(_hwnd, himlSysSmall, TVSIL_NORMAL);
        if (_SubclassWindow(_hwnd))
        {
            UINT uFlags = ILIsRooted(_pidlRoot()) ? SHCNRF_ShellLevel : SHCNRF_ShellLevel | SHCNRF_InterruptLevel;
            
            _RegisterWindow(_hwnd, &s_idlNULL, _lEvents, uFlags);
        }

        SetWindowLongPtr(_hwnd, GWLP_USERDATA, (LONG_PTR)this);

        OTRegister(_hwnd);
        return S_OK;
    }
    return E_FAIL;
}


// *** IDeskBand methods ***

HRESULT CExplorerBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;
    pdbi->dwModeFlags = DBIMF_FIXEDBMP | DBIMF_VARIABLEHEIGHT;
    
    pdbi->ptMinSize.x = 16;
    pdbi->ptMinSize.y = 0;
    pdbi->ptMaxSize.x = 32000; // random
    pdbi->ptMaxSize.y = 32000; // random
    pdbi->ptActual.y = -1;
    pdbi->ptActual.x = -1;
    pdbi->ptIntegral.y = 1;
    MLLoadStringW(IDS_TREETITLE, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    return S_OK;
} 


/*----------------------------------------------------------
Purpose: IWinEventHandler::IsWindowOwner method.

*/
HRESULT CExplorerBand::IsWindowOwner(HWND hwnd)
{
    if (hwnd == _hwnd || 
        _hwnd && (hwnd == (HWND)SendMessage(_hwnd, TVM_GETEDITCONTROL, 0,0)))
        return S_OK;
    
    return S_FALSE;
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::OnWinEvent method

         Processes messages passed on from the bandsite.
*/
HRESULT CExplorerBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    *plres = 0;
    
    switch (uMsg) 
    {
    case WM_NOTIFY:
        *plres = _TreeOnNotify((LPNMHDR)lParam);
        break;
    }
    
    return S_OK;
} 

LRESULT CExplorerBand::_DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = FALSE;

    switch (uMsg)
    {
    case WM_COMMAND:
        if (_OnCommand(wParam, lParam))
            lres = TRUE;
        break;

    case WM_CONTEXTMENU:
        // mouse clicks get picked off as rclick before
        // generating wm_contextmenu, no need to handle
        // that here -- keyboard context menu still gets here
        if ((((HWND)wParam) == _hwnd) && (lParam == -1))
        {
            _TreeContextMenu(NULL);
            lres = TRUE;
        }
        break;

    case WM_INITMENUPOPUP:
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
        if (_pcmTree)
        {
            _pcmTree->HandleMenuMsg(uMsg, wParam, lParam);
            return TRUE;
        }
        break;
    }

    if (!lres)
        lres = CNotifySubclassWndProc::_DefWindowProc(hwnd, uMsg, wParam, lParam);

    return lres;
}

//BUGBUG make that near after tree  is sucked out of cabinet.
LPOneTreeNode CExplorerBand::_TreeGetFCTreeData(HTREEITEM hItem)
{
    TV_ITEM ti;

    if (!hItem) 
    {
        // if hItem is false, GETITEM will fail, but we always store the OT node
        // data in here, so we'll just return the root's node.
        // change that if we start storing something else in the dwData
        return NULL;
    }

    ti.mask = TVIF_PARAM;
    ti.hItem = hItem;
    ti.lParam = 0;

    TreeView_GetItem(_hwnd, &ti);
    return (LPOneTreeNode)ti.lParam;
}

void CExplorerBand::_TreeSetItemState(HTREEITEM hti, UINT stateMask, UINT state)
{
    if (hti) 
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_STATE;
        tvi.stateMask = stateMask;
        tvi.hItem = hti;
        tvi.state = state;
        TreeView_SetItem(_hwnd, &tvi);
    }
}

void CExplorerBand::_TreeNukeCutState()
{
    _TreeSetItemState(_htiCut, TVIS_CUT, 0);
    _htiCut = NULL;

    ChangeClipboardChain(_hwnd, _hwndNextViewer);
    _hwndNextViewer = NULL;
}

BOOL CExplorerBand::_IsInSFC(LPOneTreeNode lpn)
{
    ASSERTCRITICAL;
    ASSERT(_hdsaLegacySFC);

    for (int i = 0; i < DSA_GetItemCount(_hdsaLegacySFC); i++)
    {
        SFCITEM *psfc = (SFCITEM *)DSA_GetItemPtr(_hdsaLegacySFC, i);

        ASSERT(psfc);
        
        if (psfc->lpn == lpn)
            return TRUE;
    }
    return FALSE;
}

void CExplorerBand::_AddToLegacySFC(LPOneTreeNode lpn, IShellFolder *psf)
{
    ASSERT(lpn && psf);

    ENTERCRITICAL;
    if (!_hdsaLegacySFC)
    {
        _hdsaLegacySFC = DSA_Create(SIZEOF(SFCITEM), 4);
    }

    if (_hdsaLegacySFC)
    {
        if (!_IsInSFC(lpn))
        {
            SFCITEM sfc = {lpn, psf};
            if (-1 != DSA_InsertItem(_hdsaLegacySFC, 0, &sfc))
            {
                OTAddRef(lpn);
                psf->AddRef();
            }
        }
    }
    LEAVECRITICAL;
}

void CExplorerBand::_MaybeAddToLegacySFC(LPOneTreeNode lpn, LPCITEMIDLIST pidl, IShellFolder *psf)
{
    IShellFolder *psfRelease = NULL;
    if (!psf && pidl)
    {
        psf = psfRelease = OTBindToFolder(lpn);
    }

    if (psf && lpn)
    {
        //
        //  BUGBUGLEGACY - Compatibility.  needs the Shell folder cache,  - ZekeL - 4-MAY-99
        //  some apps, specifically WS_FTP and AECO Zip Pro,
        //  rely on having a shellfolder existing in order for them to work.
        //  we pulled the SFC because it wasnt any perf win.
        //
        if (OBJCOMPATF_OTNEEDSSFCACHE & SHGetObjectCompatFlags(psf, NULL))
            _AddToLegacySFC(lpn, psf);
    }

    if (psfRelease)
        psfRelease->Release();
}

IShellFolder *CExplorerBand::_TreeBindToFolder(HTREEITEM hItem)
{
    LPOneTreeNode lpn;
    IShellFolder * psf;

    lpn = _TreeGetFCTreeData(hItem);
    psf = OTBindToFolder(lpn);

    _MaybeAddToLegacySFC(lpn, NULL, psf);
    return psf;
}

IShellFolder *CExplorerBand::_TreeBindToParentFolder(HTREEITEM hItem)
{
    HTREEITEM htiParent;
    IShellFolder *psf = NULL;

    htiParent = TreeView_GetParent(_hwnd, hItem);
    ASSERT(htiParent);

    if (htiParent)
        psf = _TreeBindToFolder(htiParent);
    //  else
    //  maybe we should do something special for the root??

    return psf;
}


LPITEMIDLIST CExplorerBand::_TreeGetFolderID(HTREEITEM hItem)
{
    LPOneTreeNode lpn = _TreeGetFCTreeData(hItem);
    if (lpn)
        return OTCloneFolderID(lpn);
    return NULL;
}

void CExplorerBand::_TreeCompleteCallbacks(HTREEITEM hItem)
{
    TV_ITEM ti;
    TCHAR szPath[MAX_PATH];
    
    LPOneTreeNode lpn = _TreeGetFCTreeData(hItem);

    ti.mask = TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    ti.hItem = hItem;
    ti.pszText = szPath;
    ti.cchTextMax = ARRAYSIZE(szPath);
    OTNodeFillTV_ITEM(lpn, &ti);

    TreeView_SetItem(_hwnd, &ti);
}

//---------------------------------------------------------------------------
// Get the text of the requested item;
void CExplorerBand::_TreeGetItemText(HTREEITEM hti, LPTSTR lpszText, int cbText)
{
    TV_ITEM tvi;

    tvi.mask = TVIF_TEXT;
    tvi.hItem = hti;
    tvi.pszText = lpszText;
    tvi.cchTextMax = cbText;

    TreeView_GetItem(_hwnd, &tvi);
}

//---------------------------------------------------------------------------
// Get the state of the requested item;
UINT CExplorerBand::_TreeGetItemState(HTREEITEM hti)
{
    TV_ITEM tvi;

    tvi.mask = TVIF_STATE;
    tvi.hItem = hti;
    tvi.stateMask = TVIS_ALL;

    TreeView_GetItem(_hwnd, &tvi);
    return tvi.state;
}

//---------------------------------------------------------------------------
// If the given pt is within an items text or icon area then return that
// item.
HTREEITEM CExplorerBand::_TreeHitTest(POINT pt, DWORD *pdwFlags)
{
    HTREEITEM hti;
    TV_HITTESTINFO tvht;
    tvht.pt = pt;

    hti = TreeView_HitTest(_hwnd, &tvht);
    if (pdwFlags)
        *pdwFlags = tvht.flags;
    return hti;
}


LRESULT CExplorerBand::_TreeContextMenu(LPPOINT ppt)
{
    HTREEITEM hti;
    POINT ptPopup;      // in screen coordinate

    if (SHRestricted(REST_NOVIEWCONTEXTMENU))
        return 0;

    if (ppt)
    {
        //
        // Mouse-driven: Pick the treeitem from the position.
        //
        ptPopup = *ppt;
        ScreenToClient(_hwnd, ppt);
        hti = _TreeHitTest(*ppt, NULL);
    }
    else
    {
        //
        // Keyboard-driven: Get the popup position from the selected item.
        //
        hti = TreeView_GetSelection(_hwnd);
        if (hti)
        {
            RECT rc;
            //
            // Note that TV_GetItemRect returns it in client coordinate!
            //
            TreeView_GetItemRect(_hwnd, hti, &rc, TRUE);
            ptPopup.x = (rc.left+rc.right)/2;
            ptPopup.y = (rc.top+rc.bottom)/2;
            MapWindowPoints(_hwnd, HWND_DESKTOP, &ptPopup, 1);
        }
    }

    if (hti != NULL)
    {
        LPITEMIDLIST pidl = _TreeGetAbsolutePidl(hti);

        // Unfortunately, the context menu that you get from the desktop
        // is lame.  All it has is "New" and even that menu doesn't work!
        // So just blow off trying to get the context menu of the desktop.

#ifdef DESKTOP_CONTEXTMENU_ACTUALLY_WORKS
        if (pidl)
#else
        if (!ILIsEmpty(pidl))
#endif
        {
            IContextMenu * pcm;
            //
            // Note: We should pass _hwnd instead of _hwnd
            // because we send that hwnd random messages, and tree may
            // fault on these messages intended for hwndMain.
            // in general, that hwnd needs to be something that can support
            // GETISHELLBROWSESR
            //
            if (SUCCEEDED(_GetCtxMenu(_hwnd, pidl, &pcm)))
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    UINT idCmd;
                    //
                    // Step 2: Let the object handler add menu items for verbs.
                    //
                    pcm->QueryContextMenu(hmenu, 0, 1, 0x7fff,
                            CMF_EXPLORE|CMF_CANRENAME);

                    ASSERT(!_pcmTree);
                    pcm->QueryInterface(IID_IContextMenu2, (void **)&_pcmTree);

                    //
                    // Pop up the menu and let the user select an item.
                    //
                    idCmd = TrackPopupMenu(hmenu,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        ptPopup.x, ptPopup.y, 0, _hwnd, NULL);

                    ATOMICRELEASE(_pcmTree);

                    if (idCmd)
                    {
                        TCHAR szCommandString[40];
                        BOOL fHandled = FALSE;
                        BOOL fCutting = FALSE;

                        // We need to special case the rename command
                        HRESULT hres = ContextMenu_GetCommandStringVerb(pcm,
                                        idCmd-1,
                                        szCommandString,
                                        ARRAYSIZE(szCommandString));

                        if (SUCCEEDED(hres))
                        {
                            if (lstrcmpi(szCommandString, c_szRename)==0) 
                            {
                                TreeView_EditLabel(_hwnd, hti);
                                fHandled = TRUE;
                            } 
                            else if (!lstrcmpi(szCommandString, c_szMove)) 
                            {
                                if (hti) 
                                {
                                    // For cut-effect after InvokeCommand.
                                    fCutting = TRUE;
                                }
                            }
                        }

                        if (!fHandled)
                        {
                            //
                            // Call InvokeCommand (-1 is from 1,0x7fff)
                            //
                            CMINVOKECOMMANDINFOEX ici = {
                                SIZEOF(CMINVOKECOMMANDINFOEX),
                                0L,
                                _hwnd,
                                (LPSTR)MAKEINTRESOURCE(idCmd - 1),
                                NULL, NULL,
                                SW_NORMAL,
                            };

                            hres = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
                            if (fCutting && SUCCEEDED(hres))
                            {
                                _TreeSetItemState(hti, TVIS_CUT, TVIS_CUT);
                                ASSERT(!_hwndNextViewer);
                                _hwndNextViewer = SetClipboardViewer(_hwnd);
                                TraceMsg(DM_TRACE, "CABINET: Set ClipboardViewer %d %d", _hwnd, _hwndNextViewer);
                                _htiCut = hti;
                            }
                        }
                    }
                    DestroyMenu(hmenu);
                }
                pcm->Release();
            }
            ILFree(pidl);
        }
    }
    return 0;
}

// Right click gets picked off by bar before it
// goes to a context menu, we need to pick it off here
//
LRESULT CExplorerBand::_TreeHandleRClick()
{
    POINT pt;
    DWORD dwPos = GetMessagePos();
    pt.x = GET_X_LPARAM(dwPos);
    pt.y = GET_Y_LPARAM(dwPos);

    return _TreeContextMenu(&pt);
}

BOOL CExplorerBand::_TreeValidateNode(HTREEITEM hti)
{
    BOOL fRefreshed = FALSE;
    if (hti) 
    {
        LPOneTreeNode lpnd = _TreeGetFCTreeData(hti);
        if (lpnd && OTIsRemovableRoot(lpnd)) 
        {
            // if we did that on the current selection, do a full refresh
            if (hti == TreeView_GetSelection(_hwnd)) 
            {
                //  need to send REFRESH to the TV and the browser.
                IOleCommandTarget *pct;
                if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IOleCommandTarget, (void**)&pct)))
                {
                    VARIANT v = {0};
                    v.vt = VT_I4;
                    v.lVal = OLECMDIDF_REFRESH_NO_CACHE;
                    pct->Exec(NULL, OLECMDID_REFRESH, OLECMDEXECOPT_DONTPROMPTUSER, &v, NULL);
                    pct->Release();
                }
    
                PostMessage(_hwnd, WM_COMMAND, GET_WM_COMMAND_MPS(FCIDM_REFRESH, 0, _hwnd));
            } 
            else 
            {
                // we hit a removeable drive.  invalidate and refresh that
                DoInvalidateAll(lpnd, -1);
                _TreeTrimInvisible(hti);
                _TreeInvalidateItemInfo(hti);
                _TreeRefreshOneLevel(hti, TRUE);
            }
            
            fRefreshed = TRUE;
        }
    }
    return fRefreshed;
}

void CExplorerBand::_TreeHandleClick()
{
    POINT pt;
    HTREEITEM hti;
    DWORD dwFlags;
    DWORD dwPos = GetMessagePos();
    pt.x = GET_X_LPARAM(dwPos);
    pt.y = GET_Y_LPARAM(dwPos);

    ScreenToClient(_hwnd, &pt);
    hti = _TreeHitTest(pt, &dwFlags);
    if (dwFlags & TVHT_ONITEM)
        _TreeValidateNode(hti);
}

void CExplorerBand::_TreeValidateCurrentSelection()
{
    HTREEITEM hti = TreeView_GetSelection(_hwnd);
    _TreeValidateNode(hti);
}

//
// Returns the absolute pidl to the specified tree node.
//
LPITEMIDLIST CExplorerBand::_TreeGetAbsolutePidl(HTREEITEM hti)
{
    LPITEMIDLIST pidlRight = _TreeGetFolderID(hti);
    if (pidlRight) 
    {
        LPITEMIDLIST pidlLeft;
        HTREEITEM htiParent = TreeView_GetParent(_hwnd, hti);
        if (htiParent) 
        {
            LPITEMIDLIST pidlFull;
            
            pidlLeft = _TreeGetAbsolutePidl(htiParent);
            if (!pidlLeft) 
            {
                // out of memory!
                ILFree(pidlRight);
                return NULL;
            }
            
            pidlFull = ILCombine(pidlLeft, pidlRight);
            ILFree(pidlRight);
            pidlRight = pidlFull;
        }
    }

    return pidlRight;
}

//---------------------------------------------------------------------------
//
//  hwnd -- Specifies the owner window for message box/dialog box
//
void CExplorerBand::_TreeHandleBeginDrag(BOOL fShowMenu, LPNM_TREEVIEW lpnmhdr)
{
    HTREEITEM hti = lpnmhdr->itemNew.hItem;
    HTREEITEM htiParent = TreeView_GetParent(_hwnd, hti);

    IShellFolder *psfParent = _TreeBindToFolder(htiParent);
    if (psfParent)
    {
        LPITEMIDLIST pidl = _TreeGetFolderID(hti);
        if (pidl)
        {
            HRESULT hres;
            IDataObject *pdtobj;

            //
            // First let's ask the parent to create the IDataObject.
            //
            //
            hres = psfParent->GetUIObjectOf(_hwnd, 1, (LPCITEMIDLIST*)&pidl, IID_IDataObject, NULL, (void **)&pdtobj);

            //
            // If it failed, we create a default one.
            //
            if (FAILED(hres))
            {
                LPITEMIDLIST pidlAbs = _TreeGetAbsolutePidl(htiParent);
                if (pidlAbs)
                {
                    hres = CIDLData_CreateFromIDArray(pidlAbs, 1, (LPCITEMIDLIST*)&pidl, &pdtobj);
                    ILFree(pidlAbs);
                }
            }

            if (SUCCEEDED(hres))
            {
                HIMAGELIST himlDrag = TreeView_CreateDragImage(_hwnd, lpnmhdr->itemNew.hItem);

                DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
                psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwEffect);
                dwEffect &= DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;

                if (himlDrag) 
                {
                    if (DAD_SetDragImage(himlDrag, NULL))
                    {
                        SHDoDragDrop(_hwnd, pdtobj, NULL, dwEffect, &dwEffect);
                        DAD_SetDragImage((HIMAGELIST)-1, NULL);
                    }
                    else
                    {
                        TraceMsg(DM_TRACE, "sh ER - _TreeHandleBeginDrag DAD_SetDragImage failed");
                        ASSERT(0);
                    }
                    ImageList_Destroy(himlDrag);
                }
                pdtobj->Release();
            }
            ILFree(pidl);
        }
        psfParent->Release();
    }
}

HRESULT IUnknown_BrowseObject(IUnknown *punk, LPCITEMIDLIST pidl)
{
    IShellBrowser* psb;
    HRESULT hr = IUnknown_QueryService(punk, SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb);

    if (SUCCEEDED(hr))
    {
        //
        // WARNING:
        //
        //  We should return FALSE, if Cabinet_ChangeView returns
        // S_FALSE (SETPATH is posted). That's why we don't use
        // SUCCEEDED macro here.
        //
        hr = psb->BrowseObject(pidl, SBSP_SAMEBROWSER);
        psb->Release();
    }

    return hr;
}

// We're going to delete pidl, but that may be the hti (or
// a parent of the hti) that we are viewing!  In this case,
// we need to navigate to the parent of pidl.
//
// Note, as a side-effect, we return htiParent
//
HTREEITEM CExplorerBand::_TreeMayNavigateDueToDelete(LPCITEMIDLIST pidl)
{
    HTREEITEM htiParent = NULL;

    if (pidl)
    {
        // Get the parent htiParent first, as we need to return it
        HTREEITEM hti = _TreeGetItemFromIDList(pidl);
        if (hti)
        {
            htiParent = TreeView_GetParent(_hwnd, hti);
            if (htiParent)
            {
                // Now check the selection
                LPITEMIDLIST pidlCur = _GetPidlCur();
                if (pidlCur) 
                {
                    if (ILIsEqual(pidl, pidlCur) || ILIsParent(pidl, pidlCur, FALSE))
                    {
                        // It's being nuked, need to navigate
                        LPOneTreeNode lpnd = _TreeGetFCTreeData(htiParent);
                        if (lpnd)
                        {
                            LPITEMIDLIST pidlParent = OTCreateIDListFromNode(lpnd);
                            if (pidlParent)
                            {
                                IUnknown_BrowseObject(_punkSite, pidlParent);

                                ILFree(pidlParent);
                            }
                        }
                    }
                    ILFree(pidlCur);
                }
            }
        }
    }
    
    return htiParent;
}
    


//---------------------------------------------------------------------------
// CExplorerBand::_TreeRealHandleSelChange
//
//  that function returns TRUE, only if we were able to switch the view
// to the selected item.
//---------------------------------------------------------------------------

BOOL CExplorerBand::_TreeRealHandleSelChange()
{
    BOOL fRet = FALSE;
    HTREEITEM hti;

    //
    // Kill the timer
    //
    KillTimer(_hwnd, _nSelChangeTimer);
    _nSelChangeTimer = 0;

    hti = TreeView_GetSelection(_hwnd);

    if (hti == NULL)
    {
        // that can happen if we get reentered at a bad time,
        // such as doing a refresh when viewing a network resource
        return(FALSE);
    }
    LPOneTreeNode lpnd = _TreeGetFCTreeData(hti);
    if (lpnd == NULL)
        return FALSE;

    LPITEMIDLIST pidlCur = _GetPidlCur();
    if (pidlCur) 
    {
        HTREEITEM htiCur = _TreeGetItemFromIDList(pidlCur, TRUE);
        ILFree(pidlCur);
        // we're already there...  they navigated around and back
        // to the beginning
        if (hti == htiCur)
            return TRUE;
    }

    //
    // Notes: The fully-qualified path name in szPath will be used to
    //  maintain the MRU list of tree items.
    //
    LPITEMIDLIST pidl = OTCreateIDListFromNode(lpnd);
    if (pidl)
    {
        _MaybeAddToLegacySFC(lpnd, pidl, NULL);
        fRet = SUCCEEDED(IUnknown_BrowseObject(_punkSite, pidl));

        ILFree(pidl);
    }

    //
    //  If any of FSNotify message is ignored during that function,
    // update the tree now.
    //
    if (_fUpdateTree) 
    {
        _TreeRefreshAll();
        _fUpdateTree = FALSE;
    }

    // (whether or not succeeded)
    UEMFireEvent(&UEMIID_BROWSER, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_NAVIGATE, UIBL_NAVFOLDERS);

    return fRet;
}

#ifdef DEBUG
DWORD g_tmMsg = 0;
DWORD g_tmStart = 0;
DWORD g_tmReceived = 0;
#endif

void CALLBACK CExplorerBand::_TreeTimerProc(HWND hWnd, UINT uMessage, UINT_PTR wTimer, DWORD dwTime)
{
    // BUGBUG: we aren't guaranteed that is our HWND!
    CExplorerBand* peb = (CExplorerBand*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
#ifdef DEBUG
    g_tmReceived = GetTickCount();
#endif

    peb->_TreeRealHandleSelChange();

#ifdef DEBUG
    TraceMsg(DM_TRACE, "ca TR - _TreeTimerProc (msg=%d, timer=%d, selchange=%d)",
             g_tmStart-g_tmMsg, g_tmReceived-g_tmStart, GetTickCount()-g_tmReceived);
#endif
}


void CExplorerBand::_TreeHandleSelChange(BOOL fDelayed)
{
    // We don't want to change selection right away in case the user is
    // just scrolling through the list.  We'll wait 3/4 second for keyboard
    // and no time for mouse (we get strange flashing if we do it right
    // away) and then do it.
    // Also we probably don't want to do this is we are not visible.  This can happen by
    // us watching FSNotifies and having what we think is selected be removed from the list...
    if (!IsWindowVisible(_hwnd))
        return;

    if (_nSelChangeTimer)
    {
        KillTimer(_hwnd, _nSelChangeTimer);      // BUGBUG: using wrong timer ID?
    }

#ifdef DEBUG
    g_tmMsg = GetMessageTime();
    g_tmStart = GetTickCount();
#endif
    _nSelChangeTimer = SetTimer(_hwnd, 1,
            fDelayed ? (GetDoubleClickTime()*3/2) : 1, _TreeTimerProc);

}


//---------------------------------------------------------------------------
// Find an child of the given item with the given name,
// Returns null if the item can't be found or there are no children.
// REVIEW HACK TV_FINDTEM will do that.
HTREEITEM CExplorerBand::_TreeFindChildItem(HTREEITEM htiParent, LPCITEMIDLIST pidl)
{
    HTREEITEM hti = NULL;
    IShellFolder *psf;
    LPITEMIDLIST pidlFirst;
    HRESULT hres;

    pidlFirst = ILCloneFirst(pidl);
    if (!pidlFirst)
        return NULL;

    psf = _TreeBindToFolder(htiParent);
    if (psf)
    {
        for (hti = TreeView_GetChild(_hwnd, htiParent);
             hti;
             hti = TreeView_GetNextSibling(_hwnd, hti))
        {
            LPITEMIDLIST pidl2 = _TreeGetFolderID(hti);
            if (!pidl2)
            {
                hti = NULL;
                break;
            }
            
            hres = psf->CompareIDs(0, pidlFirst, pidl2);
            ILFree(pidl2);
            if (SUCCEEDED(hres) && (hres == ResultFromShort(0)))
                break;
        }

        psf->Release();
    }

    ILFree(pidlFirst);
    return hti;
}


// Pointer comparision function for Sort and Search functions.
// lParam is lParam passed to sort/search functions.  Returns
// -1 if p1 < p2, 0 if p1 == p2, and 1 if p1 > p2.
//
int CALLBACK CExplorerBand::_ItemCompare(void *p1, void *p2, LPARAM lParam)
{
    STRSEARCH *pss = (STRSEARCH *)lParam;
    int iRet;

    ENTERCRITICAL;

    LPCITEMIDLIST pidl1, pidl2;

    if (p1)
    {
        LPOneTreeNode pData1 = pss->_peb->_TreeGetFCTreeData((HTREEITEM)p1);
        pidl1 = pData1 ? pData1->pidl : NULL;
    }
    else
        pidl1 = pss->_pidlSearch;

    if (p2)
    {
        LPOneTreeNode pData2 = pss->_peb->_TreeGetFCTreeData((HTREEITEM)p2);
        pidl2 = pData2 ? pData2->pidl : NULL;
    }
    else
        pidl2 = NULL;

    if (pidl1 && pidl2)
    {
        // NOTE: old code used to memcmp the pidl. This works because
        // when we errenously say they don't match, we delete the old one out
        // of the tree and add the new one.  Doing it this way has less flicker.
        HRESULT hres = pss->_psf->CompareIDs(0, pidl1, pidl2);
        iRet = ShortFromResult(hres);
    }
    else
        iRet = 0;

    LEAVECRITICAL;

    return iRet;
}


//---------------------------------------------------------------------------
// Create a DPA list of HTREEITEMS, one for each child of the given node
// in the given tree.

// that HTIList stuff gets punted when OneTree works.
HDPA CExplorerBand::_TreeCreateHTIList(IShellFolder* psfParent, HTREEITEM htiParent)
{
    HDPA hdpa = NULL;
    HTREEITEM htiChild = TreeView_GetChild(_hwnd, htiParent);
    if (htiChild)
    {
        hdpa = DPA_Create(8);
        if (hdpa)
        {
            STRSEARCH ss;

            while (htiChild)
            {
                DPA_AppendPtr(hdpa, htiChild);
                htiChild = TreeView_GetNextSibling(_hwnd, htiChild);
            }

            ss._peb = this;
            ss._pidlSearch = NULL;
            ss._psf = psfParent;

            DPA_Sort(hdpa, _ItemCompare, (LPARAM)&ss);
        }
    }
    return hdpa;
}

//---------------------------------------------------------------------------
// Delete all the items from the tree that are in the given list.
BOOL CExplorerBand::_DeleteHTIListItemsFromTree(HDPA hdpa)
{
    int i, cItems;
    HTREEITEM htiTmp;
    BOOL fDeleted = FALSE;

    if (hdpa)
    {
        // Delete all non-null items from the tree.
        cItems = DPA_GetPtrCount(hdpa);
        HTREEITEM htiSel = TreeView_GetSelection(_hwnd);

        for (i=0; i<cItems; i++)
        {
            htiTmp = (HTREEITEM)DPA_FastGetPtr(hdpa, i);
            if (htiTmp) 
            {
                BOOL fDeleteThisItem = TRUE;
                
                if (htiTmp == htiSel) 
                {
                    // If this node was forcibly inserted into the tree, validate that it
                    // no longer exists before deleting it and navigating to the parent:
                    LPOneTreeNode lpn = _TreeGetFCTreeData(htiSel);
                    if (lpn && lpn->fInserted)
                    {
                        IShellFolder* psf = _TreeBindToParentFolder(htiSel);
                        if (NULL != psf)
                        {
                            LPITEMIDLIST pidlSel = _TreeGetFolderID(htiSel);
                            if (NULL != pidlSel)
                            {
                                DWORD dwAttr = SFGAO_VALIDATE;
                                if (SUCCEEDED(psf->GetAttributesOf(1, (LPCITEMIDLIST*) &pidlSel, &dwAttr)))
                                {
                                    fDeleteThisItem = FALSE;
                                }
                                ILFree(pidlSel);
                            }
                            psf->Release();
                        }
                    }

                    if (fDeleteThisItem)
                    {
                        // We should only be asked to delete the selected item
                        // if it no longer exists.  In this case, we should navigate
                        // to the parent...
                        //
                        HTREEITEM htiParent = TreeView_GetParent(_hwnd, htiTmp);
                        if (htiParent)
                        {
                            LPITEMIDLIST pidlParent = _TreeGetFolderID(htiParent);
                            if (pidlParent)
                            {
                                IUnknown_BrowseObject(_punkSite, pidlParent);
                                ILFree(pidlParent);
                            }
                        }
                    }
                }

                if (fDeleteThisItem)
                {
                    TreeView_DeleteItem(_hwnd, htiTmp);
                    fDeleted = TRUE;
                }
            }
        }
    }

    return fDeleted;
}


int CExplorerBand::_TreeHTIListFindChildItem(IShellFolder* psfParent, HDPA hdpa, LPCITEMIDLIST pidlChild)
{
    STRSEARCH ss;

    if (!hdpa)
    {
        return(-1);
    }

    ss._peb = this;
    ss._pidlSearch = pidlChild;
    ss._psf = psfParent;

    return DPA_Search(hdpa, NULL, 0, _ItemCompare, (LPARAM)&ss, DPAS_SORTED);
}

void CExplorerBand::_TreeInvalidateItemInfoEx(HTREEITEM hItem, BOOL fRecurse)
{
    TV_ITEM ti;

    // If we are invalidating from within a FileSysChange notification,
    // then we are racing against the OneTree (which might not have received
    // the notification yet).  In which case, forcibly invalidate the
    // entry so OneTree won't accidentally use the cached value.
    //
    // Must do this before TreeView_SetItem because TreeView_SetItem sometimes
    // does an UpdateWindow, so we have to have everything all good and
    // invalid before we tell the treeview.

    if (_iInFSC)
    {
        LPOneTreeNode lpn = _TreeGetFCTreeData(hItem);
        if (lpn)
        {
            lpn->fInvalid = TRUE;
            lpn->fHasAttributes = FALSE;
            if (fRecurse)
                OTInvalidateAttributeRecursive(lpn);
        }
    }

    ti.mask = TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    ti.hItem = hItem;
    ti.cChildren = I_CHILDRENCALLBACK;
    ti.iImage = I_IMAGECALLBACK;
    ti.iSelectedImage = I_IMAGECALLBACK;
    ti.pszText = LPSTR_TEXTCALLBACK;
    TreeView_SetItem(_hwnd, &ti);

}

HTREEITEM CExplorerBand::_TreeInsertItem(HTREEITEM htiParent, LPOneTreeNode lpnKid)
{
    TV_INSERTSTRUCT tii;

    // use callbacks for the expensive fields
    DebugDumpNode(lpnKid, TEXT("_TreeInsertItem"));
    ASSERT(lpnKid);

    tii.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
    tii.hParent = htiParent;
    tii.hInsertAfter = TVI_FIRST;
    tii.item.iImage = I_IMAGECALLBACK;
    tii.item.iSelectedImage = I_IMAGECALLBACK;

    tii.item.pszText = LPSTR_TEXTCALLBACK;
    tii.item.cChildren = I_CHILDRENCALLBACK; // OTHasSubFolders(lpnKid);
    tii.item.lParam = (LPARAM)lpnKid;

    if (lpnKid)
        OTAddRef(lpnKid);

    return TreeView_InsertItem(_hwnd, &tii);
}

void CExplorerBand::_PushRecursion()
{
    _uRecurse++;
}

void CExplorerBand::_PopRecursion()
{
    if (_uRecurse == 1) 
    {
        if (_fUpdateTree) 
        {
            //  If any of FSNotify message is ignored during this function,
            // update the tree now.
            _TreeRefreshAll();
            _fUpdateTree = FALSE;
        }
    }
    _uRecurse--;
}

//---------------------------------------------------------------------------
// Tidyup.
#define HTIList_Destroy(hdpa) if (hdpa) DPA_Destroy(hdpa)

BOOL CExplorerBand::_TreeFillOneLevel(HTREEITEM htiParent,  BOOL bInvalOld)
{
    BOOL bTreeChanged = FALSE;
    BOOL fSuccess = FALSE;
    DECLAREWAITCURSOR;

    // _TreeGetFCTreeData will deal right if htiParent is null meaning root
    LPOneTreeNode lpn = _TreeGetFCTreeData(htiParent);
    if (!lpn)
    {
        // We did everything that can be done, so return TRUE
        return(TRUE);
    }

    CExpBandItemWatch w(htiParent);
    if (!_AttachItemWatch(&w))
    {
        AssertMsg(0, TEXT("_TreeFillOneLevel: Unable to register watch"));
        return FALSE;
    }

    _PushRecursion();
    SetWaitCursor();

    // Build list of all the items currently in the tree
    IShellFolder *psfParent = _TreeBindToFolder(htiParent);
    if (psfParent)
    {
        UINT iSubNodes;
        HDPA hdpa = _TreeCreateHTIList(psfParent, htiParent);

        // Get the OT count.  Note: as a side effect, this will reenumerate if the node is invalid
        fSuccess = OTSubNodeCount(_hwnd, lpn, &_ei, &iSubNodes, _fInteractive);

        // Walk through the two lists comparing items
        for (int i = iSubNodes - 1; i >= 0; i--)
        {
            LPITEMIDLIST pidlSubFolder;
            LPOneTreeNode lpnKid = OTGetNthSubNode(_hwnd, lpn, i);

            if (!lpnKid)
                continue;

            // NOTE: abort if esc key is down
            if (GetAsyncKeyState(VK_ESCAPE)) 
            {
                break;
            }

            // Something bad may have happened during that Peek, so revalidate
            if (!w.StillValid()) 
            {
                break;
            }

            pidlSubFolder = OTCloneFolderID(lpnKid);
            if (pidlSubFolder) 
            {
                int iChild = _TreeHTIListFindChildItem(psfParent, hdpa, pidlSubFolder);

                // already in tree?
                if (iChild >= 0) 
                {
                    // Yep, Remove it from the list of things to delete.
                    if (bInvalOld)
                        _TreeInvalidateItemInfo((HTREEITEM)DPA_FastGetPtr(hdpa, iChild));
                    DPA_DeletePtr(hdpa, iChild);
                } 
                else 
                {
                    // if hdpa doesn't exist, then we're just added stuff straight
                    //  since onetree is kept in sorted order ,we don't need to sort
                    if (hdpa)
                        bTreeChanged = TRUE;
                    _TreeInsertItem(htiParent, lpnKid);
                }
                ILFree(pidlSubFolder);
            }
            
            OTRelease(lpnKid);
        }

        if (_DeleteHTIListItemsFromTree(hdpa))
            bTreeChanged = TRUE;;
        HTIList_Destroy(hdpa);
        
        psfParent->Release();
    }



    if (bTreeChanged && w.StillValid())
    {
        _TreeSortChildren(htiParent, lpn);
    }

    ResetWaitCursor();
    _PopRecursion();
    _DetachItemWatch(&w);
    return fSuccess && w.StillValid();
}

void CExplorerBand::_TreeSortChildren(HTREEITEM htiParent, LPOneTreeNode lpn)
{
    // Make sure the parent folder is currently cached
    IShellFolder *psf = OTBindToFolder(lpn);
    if (psf)
    {
        TV_SORTCB sSortCB;
        
        sSortCB.hParent = htiParent;
        sSortCB.lpfnCompare = OTTreeViewCompare;
        sSortCB.lParam = (LPARAM)psf;
        
        TreeView_SortChildrenCB(_hwnd, &sSortCB, FALSE);
        psf->Release();
    }
    else
    {
        // We'll just do default sorting
        TreeView_SortChildren(_hwnd, htiParent, FALSE);
    }
}

BOOL CExplorerBand::_TreeChildAttribs(LPOneTreeNode lpn, LPCITEMIDLIST pidlChild, DWORD *pdwInOutAttrs)
{
    BOOL fRet = FALSE;
    IShellFolder *psf = OTBindToFolder(lpn);
    if (psf)
    {
        fRet = SUCCEEDED(psf->GetAttributesOf(1, &pidlChild, pdwInOutAttrs));
        psf->Release();
    }
    return fRet;
}

//---------------------------------------------------------------------------
// Build a single child item in the tree
//
//  w.Item() is the parent into which the child should be inserted.
//  pidl is the child itself.
//

#define _ILIsTerminal(pidl) (_ILNext(pidl)->mkid.cb == 0)

HTREEITEM CExplorerBand::_TreeBuildChild(CExpBandItemWatch &w, LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlChild = NULL;
    HTREEITEM hti = NULL;
    LPOneTreeNode lpnChild = NULL;

    // If that is a network case, we know the item should exist, but
    // was not found, which is a semi-normal condition as network
    // enumeration is not an exact science.  As such we we should
    // force the item in to the tree.
    //
    // w.Item() is the parent into which we want to force the item.
    //
    LPOneTreeNode lpn = _TreeGetFCTreeData(w.Item());

    // Now try to add the item to the list

    if (lpn && (pidlChild = ILCloneFirst(pidl)))
    {
#ifdef DEBUG
        // _TreeBuild should've filtered out nonfolders.
        DWORD dwAttribs = SFGAO_FOLDER;
        ASSERT(!_ILIsTerminal(pidl) ||
               (_TreeChildAttribs(lpn, pidlChild, &dwAttribs) && (dwAttribs & SFGAO_FOLDER)));
#endif
        _TreeInvalidateItemInfo(w.Item());
        _TreeFillOneLevel(w.Item(), FALSE);
        if (w.StillValid())
        {
            hti = _TreeFindChildItem(w.Item(), pidl);
            if (!hti)
            {
                // Phooey, we have a non-enumerated item.
                // Add it manually.
                OTAddSubFolder(lpn, pidlChild, 0, &lpnChild);
                if (lpnChild)
                {
                    // We're forcing this node in, validate before removing it later:
                    lpnChild->fInserted = TRUE;
                    hti = _TreeInsertItem(w.Item(), lpnChild);
                    OTRelease(lpnChild);
                }
            }

            TreeView_Expand(_hwnd, w.Item(), TVE_EXPAND);
            if (!w.StillValid())    // item deleted while expanding
                hti = NULL;
        }

        ILFree(pidlChild);
    }

    return hti;

}

//---------------------------------------------------------------------------
// Build a tree for the given path.
// REVIEW UNDONE - To facilitate doing the drag drop we could do with
// markers in the tree to indicate special objects
// NB Given the path "C:\foo\bar\fred" we build complete sub-trees for
// c:\ and c:\foo and c:\foo\bar and then set the focus to c:\foo\bar\fred
// NB We don't build bit's of the tree that have already been built

HTREEITEM CExplorerBand::_TreeBuild(LPCITEMIDLIST pidlFull,
                                BOOL bExpand, BOOL bDontFail)
{
    HTREEITEM hti;
    ASSERT(_hwnd);
    ASSERT(pidlFull);
    ASSERT(ILIsEqualRoot(_pidlRoot(), pidlFull));

    // Get the "root" node; insert it if necessary
    hti = TreeView_GetChild(_hwnd, NULL);
            
    if (!hti)
    {
        LPOneTreeNode lpnRoot = OTGetNodeFromIDList(_pidlRoot(), OTGNF_VALIDATE | OTGNF_TRYADD);
        if (!lpnRoot)
            return NULL;       // Really bad! No item in the tree.

        _TreeInsertItem(NULL, lpnRoot);
        OTRelease(lpnRoot);
        
        hti = TreeView_GetChild(_hwnd, NULL);
        if (!hti)
        {
            return NULL;       // Really bad! No item in the tree.
        }
    }
    
    // Keep walking down the tree, expanding the node that contains our item,
    // until we find it or we run out of time.

    _ei.fAllowTimeout = TRUE;

    // The watched item is the last known good item
    // hti is the item we are futzing with
    CExpBandItemWatch w(hti);
    if (_AttachItemWatch(&w)) 
    {
        LPCITEMIDLIST pidl = ILIsRooted(pidlFull) ? ILGetNext(pidlFull) : pidlFull;

        for (; !ILIsEmpty(pidl) && w.StillValid() && hti; pidl = ILGetNext(pidl))
        {
            BOOL fWasTimedOut = _ei.fTimedOut;

            ASSERT(w.Item() == hti);

            if (!_ei.fTimedOut) 
            {
                //
                //  We used to expand the item and hunt for the child.
                //  Unfortunately, this causes "The Internet" to expand
                //  and collapse its brains out, since all http:// pidls
                //  are non-enumerated non-folder children.  So when
                //  we reach the end of the pidl, sniff it to see if it's
                //  a folder or not.  If not, then don't waste time
                //  expanding since it won't be there.
                //
                if (_ILIsTerminal(pidl))        // is last component
                {
                    DWORD dwAttribs = SFGAO_FOLDER;
                    LPOneTreeNode lpnParent = _TreeGetFCTreeData(w.Item());
                    if (!lpnParent ||
                        !_TreeChildAttribs(lpnParent, pidl, &dwAttribs) ||
                        !(dwAttribs & SFGAO_FOLDER))
                    {
                        //
                        //  Okay, we're in one of the sucky cases...
                        //
                        //      Parent mysteriously died, or
                        //      Parent denies existence of child, or
                        //      Child is not a folder.
                        //
                        //  Just give up now and stay on the parent.
                        //
                        ASSERT(w.Item() == hti);
                        break;
                    }
                }

                TreeView_Expand(_hwnd, hti, TVE_EXPAND);
                if (!w.StillValid())    // Item deleted while we were expanding
                    break;

                if (!_ei.fTimedOut) 
                {
                    hti = _TreeFindChildItem(hti, pidl);

                    if (!hti)
                    {
                        hti = _TreeBuildChild(w, pidl);
                        if (!hti)       // Dead-end.  Bail.
                            break;
                    }
                }
            }

            // Start watching hti instead
            if (hti)
                w.Restart(hti);
            else if (w.StillValid())
            {
                //
                //  use the last valid hti
                //  that leaves us on the best parent
                //
                hti = w.Item();
                break;
            }

            if (_ei.fTimedOut && hti) 
            {
                LPOneTreeNode lpn;
                LPITEMIDLIST pidlChild;

                // if we just now timed out, we need to toss whatever was added
                if (!fWasTimedOut) 
                {
                    TreeView_Expand(_hwnd, hti, TVE_COLLAPSE | TVE_COLLAPSERESET);
                }

                // now add it into onetree, then add it into our tree.
                lpn = _TreeGetFCTreeData(hti);
                if (lpn && (pidlChild = ILCloneFirst(pidl)))
                {
                    LPOneTreeNode lpnChild;
                    OTAddSubFolder(lpn, pidlChild, 0, &lpnChild);
                    if (lpnChild) 
                    {
                        // We're forcing this node in, validate before removing it later:
                        lpnChild->fInserted = TRUE;
                        hti = _TreeInsertItem(hti, lpnChild);
                        OTRelease(lpnChild);
                    } 
                    else 
                    {
                        hti = NULL;
                    }
                    w.Restart(hti);
                    ILFree(pidlChild);
                }

                if (hti)
                    TreeView_Expand(_hwnd, TreeView_GetParent(_hwnd, hti), TVE_EXPAND | TVE_EXPANDPARTIAL);
            }
        }

        if (w.StillValid())
        {
            //
            // Firewall : Check if we could find the specified item in the tree.
            //
            if (hti == NULL)
            {
                // No, Go back to the previous selection.
                hti = TreeView_GetSelection(_hwnd);
                TraceMsg(DM_WARNING, "sh Firewall - _TreeBuild: Can't find the item");

                // Check if we have any selection
                if (hti == NULL && bDontFail)
                {
                    // Go back to the root
                    hti = TreeView_GetRoot(_hwnd);
                    ASSERT(hti);        //  should not hit.
                }
            }

            if (hti)
            {
                // Start watching the new hti so we can see if it got nuked on the expand
                w.Restart(hti);

                // Expand one more level and set the selection...
                if (bExpand && !_ei.fTimedOut)
                    TreeView_Expand(_hwnd, hti, TVE_EXPAND);

                if (w.StillValid())
                    TreeView_SelectItem(_hwnd, hti);
            }
        }
        _DetachItemWatch(&w);
    }

    _ei.fAllowTimeout = FALSE;
    _ei.fTimedOut = FALSE;
    return hti;
}

//---------------------------------------------------------------------------
// Process Treeview begin of label editing

LRESULT CExplorerBand::_TreeHandleBeginLabelEdit(TV_DISPINFO *ptvdi)
{
    HTREEITEM htiParent;
    BOOL fCantRename = TRUE;

    htiParent = TreeView_GetParent(_hwnd, ptvdi->item.hItem);

    Str_SetPtr(&_pszUnedited, NULL);

    //
    // Top level guys are always inconvincible (just like Microsoft).
    //
    if (htiParent)
    {
        IShellFolder *psfParent = _TreeBindToFolder(htiParent);
        if (psfParent)
        {
            LPOneTreeNode lpn = _TreeGetFCTreeData(ptvdi->item.hItem);
            LPITEMIDLIST pidl = OTCloneFolderID(lpn);
            if (pidl) 
            {
                HWND hwndEdit = TreeView_GetEditControl(_hwnd);
                if (hwndEdit)
                {
                    DWORD dwAttribs = SFGAO_CANRENAME;
                    psfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&pidl, &dwAttribs);
                    if (dwAttribs & SFGAO_CANRENAME)
                    {
                        TCHAR szName[MAX_PATH];
                        STRRET str;
                        fCantRename = FALSE;
                        if (SUCCEEDED(psfParent->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FOREDITING, &str)) &&
                            StrRetToStrN(szName, ARRAYSIZE(szName), &str, pidl))
                        {
                            SetWindowText(hwndEdit, szName);
                        }
                        else
                        {
                            GetWindowText(hwndEdit, szName, ARRAYSIZE(szName));
                        }
                        Str_SetPtr(&_pszUnedited, szName);
                    }
                }
                ILFree(pidl);
            }
            psfParent->Release();
        }
    }

    if (fCantRename)
        MessageBeep(0);

    return fCantRename;
}


//---------------------------------------------------------------------------
// Process Treeview end of label editing
//
//  We have to compare against the text before we started editing, because
//  our SetWindowText in BeginLabelEdit and the TVM_EDITLABEL will mark the
//  control as dirty, and we will try to SetNameOf the item even though
//  nothing may have changed.  This in turn has several bad effects:
//
//  o   It generates useless SHChangeNotify's.
//  o   It causes us to renavigate (and replay the nav sound),
//  o   It causes WS_FTP to barf.  WS_FTP doesn't like it when you
//      SetNameOf a pidl to the same name.

//
LRESULT CExplorerBand::_TreeHandleEndLabelEdit(TV_DISPINFO *ptvdi)
{
    HTREEITEM htiParent;
    IShellFolder *psfParent;
    BOOL fResult;

    //  Must stash in a local because sometimes we start a new edit cycle,
    //  which will wipe out _pszUnedited.
    //
    LPTSTR pszUnedited = _pszUnedited;
    _pszUnedited = NULL;

    // See if the user cancelled
    if (ptvdi->item.pszText == NULL)
    {
        fResult = TRUE;        // nothing to do here
        goto done;
    }

    ASSERT(ptvdi->item.hItem);
    htiParent = TreeView_GetParent(_hwnd, ptvdi->item.hItem);
    ASSERT(htiParent);

    psfParent = _TreeBindToFolder(htiParent);

    if (psfParent)
    {
        LPOneTreeNode lpn = _TreeGetFCTreeData(ptvdi->item.hItem);
        LPITEMIDLIST pidl = OTCloneFolderID(lpn);
        if (pidl)
        {
            WCHAR wszName[MAX_PATH];

            SHTCharToUnicode(ptvdi->item.pszText, wszName, ARRAYSIZE(wszName));

            // Try to rename only if the name actually changed.
            if (pszUnedited == NULL || lstrcmpW(pszUnedited, wszName) != 0)
            {
                if (SUCCEEDED(psfParent->SetNameOf(_hwnd, pidl, wszName, 0, NULL)))
                {
                    //
                    // we need to update the display of everything...
                    //
                    // Rebuild the parent
                    //
                    SHChangeNotifyHandleEvents();

                    // NOTES: pidl is no longer valid here.

                    //
                    // Set the handle to NULL in the notification to let
                    // the system know that the pointer is probably not
                    // valid anymore.
                    //
                    ptvdi->item.hItem = NULL;
                }
                else
                {
                    SendMessage(_hwnd, TVM_EDITLABEL,
                            (WPARAM)ptvdi->item.pszText, (LPARAM)ptvdi->item.hItem);
                }
            }
            ILFree(pidl);
        }
        psfParent->Release();
    }

    fResult = FALSE;        // We always return 0, "we handled it".

done:
    Str_SetPtr(&pszUnedited, NULL);
    return fResult;
}


#ifdef SETALLVIS
void CExplorerBand::_TreeSetAllVisItemInfos()
{
    HTREEITEM hItem;
    int nVisible;
    HWND _hwnd = _hwnd;

    // Now set the info for all visible items
    hItem = TreeView_GetFirstVisible(_hwnd);
    nVisible = TreeView_GetVisibleCount(_hwnd);
    for ( ; nVisible > 0 && hItem; --nVisible)
    {
        _TreeCompleteCallbacks(hItem);
        hItem = TreeView_GetNextVisible(_hwnd, hItem);
    }
}
#endif


void CExplorerBand::_TreeGetDispInfo(TV_DISPINFO *lpnm)
{
    LPOneTreeNode lpn;
    TV_ITEM ti;

    // if the dwData is null, we likely are in the process of deleting it.
    lpn = (LPOneTreeNode)lpnm->item.lParam;
    if (!lpn)
        return;

    ti.hItem = lpnm->item.hItem;
    ti.mask = 0;

    // Use that as a flag as to whether we have set the image and kids
    // Only set them if we are being asked for them
    if (lpnm->item.mask & (TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE))
    {
        ti.pszText = lpnm->item.pszText;
        ti.cchTextMax = lpnm->item.cchTextMax;
        ti.mask = lpnm->item.mask & (TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE);

        OTNodeFillTV_ITEM(lpn, &ti);
        // on start, removable root nodes have the +/-
        if (OTIsRemovableRoot(lpn))
            lpnm->item.cChildren = TRUE;

        // Return the right values, so it gets painted correctly that time
        if (lpnm->item.mask & TVIF_CHILDREN)
            lpnm->item.cChildren = ti.cChildren;
        if (lpnm->item.mask & TVIF_IMAGE) 
        {
            lpnm->item.iImage = ti.iImage;
            lpnm->item.mask |= TVIF_STATE;
            lpnm->item.state = (OTIsShared(lpn) ? INDEXTOOVERLAYMASK(IDOI_SHARE) : 0);
            if (OTIsBold(lpn))
                lpnm->item.state |= TVIS_BOLD;
            
            lpnm->item.stateMask = TVIS_OVERLAYMASK | TVIS_BOLD;
        }
        if (lpnm->item.mask & TVIF_SELECTEDIMAGE)
            lpnm->item.iSelectedImage = ti.iSelectedImage;


#ifdef SETALLVIS
        // Now set the info for all visible items
        _TreeSetAllVisItemInfos(lpnm->hdr.hwndFrom);
#endif
    }
    lpnm->item.mask |= TVIF_DI_SETITEM;
}


HTREEITEM CExplorerBand::_TreeGetItemFromIDList(LPCITEMIDLIST pidlFull, BOOL fNearest)
{
    LPCITEMIDLIST pidl;
    HTREEITEM hti;
    HTREEITEM htiNearest = NULL;

    ASSERT(_hwnd);
    ASSERT(pidlFull);

    // the first parent is child of 0.
    // that is new with the desktop being the root, and the mycomputer/network/etc
    // being the child of the desktop node in the tree.
    hti = TreeView_GetChild(_hwnd, 0);

    ASSERT(ILIsEqualRoot(_pidlRoot(), pidlFull));
    if (ILIsRooted(pidlFull))
        pidl = _ILNext(pidlFull);
    else
        pidl = pidlFull;

    for (; !ILIsEmpty(pidl) && hti; pidl = ILGetNext(pidl))
    {
        if (fNearest)
            htiNearest = hti;
            
        hti = _TreeFindChildItem(hti, pidl);

    }

    if (fNearest && !hti)
        return htiNearest;
    else
        return hti;
}


//---------------------------------------------------------------------------
// Cause the specified folder to be refreshed.
void CExplorerBand::_TreeRefresh(LPCITEMIDLIST pidl)
{
    HTREEITEM hti = _TreeGetItemFromIDList(pidl);

    ASSERT(pidl);

    if (hti)
    {
        _TreeInvalidateItemInfo(hti);
        _TreeFillOneLevel(hti, TRUE);

        // WARNING: hti may no longer be valid here...
    }
    // We don't need to update the tree which is not yet opened.
}

//---------------------------------------------------------------------------
// Do a Free() on all the pointers in the given DPA.
// BUGBUG: replace with DPA_DeleteAllPtrs()

void DPA_FreePtrs(HDPA hdpa)
{
    UINT j, cItems = DPA_GetPtrCount(hdpa);
    for (j = 0; j < cItems; j++) 
    {
        LPVOID lp = DPA_FastGetPtr(hdpa, j);
        if (lp)
            Free(lp);
    }
}


void CExplorerBand::_TreeUpdateHasKidsButton(LPNM_TREEVIEW lpnmtv)
{
    TV_ITEM ti;
    UINT i;
    int j;
    UINT cChildren;

    OTSubNodeCount(_hwnd, (LPOneTreeNode)lpnmtv->itemNew.lParam, &_ei, &cChildren, _fInteractive);
    j = (cChildren != 0);
    i = (lpnmtv->itemNew.cChildren != 0);

    // if they don't agree, set it.
    if ( i ^ j ) 
    {
        ti.mask = TVIF_CHILDREN;
        ti.hItem = lpnmtv->itemNew.hItem;
        ti.cChildren = cChildren;

        TreeView_SetItem(_hwnd, &ti);
    }
}

//---------------------------------------------------------------------------
LRESULT CExplorerBand::_TreeHandleExpanding(LPNM_TREEVIEW lpnmtv)
{
    BOOL fSuccess = TRUE;   // assume no error.

    if (lpnmtv->action != TVE_EXPAND || _ei.fTimedOut)
        return FALSE;

    // We may have "reset" that item by removing all its children, so we
    // need to refresh it

    // if we have never expanded or if it was only partially expanded, 
    // do the real thing now.
    if ((!(lpnmtv->itemNew.state & TVIS_EXPANDEDONCE)) ||
          (lpnmtv->itemNew.state & TVIS_EXPANDPARTIAL))
        {
            //
            //  If we are expanding the currently selected item, don't be
            // interactive to avoid duplicated dialog boxes.
            //
            LPOneTreeNode lpnd = _TreeGetFCTreeData(lpnmtv->itemNew.hItem);
            if (lpnd && OTIsRemovableRoot(lpnd))
                DoInvalidateAll(lpnd, -1);

            _fExpandingItem = TRUE;
            if (!_fNoInteractive && lpnmtv->itemNew.hItem != TreeView_GetSelection(_hwnd))
                _fInteractive = TRUE;
            fSuccess = _TreeFillOneLevel(lpnmtv->itemNew.hItem, FALSE);
            _fExpandingItem = FALSE;
            _fInteractive = FALSE;

            //
            // Don't call _TreeUpdateHasKidsButton if enumeration failed.
            // Otherwise, we end up enumerating it again.
            //
            if (fSuccess)
            {
                _TreeUpdateHasKidsButton(lpnmtv);
            }
        }

        return !fSuccess;
}

//---------------------------------------------------------------------------
//
//  Take an external pidl and wrap it up so it makes sense in our rooted world.
//
//  Returns NULL if the pidl is not meaningful in our world (or if we run
//  out of memory).
//
//  WARNING!  You need to ILFree the return value if it is different from
//  pidlExt.

LPITEMIDLIST CExplorerBand::_pidlRootify(LPITEMIDLIST pidlExt)
{
    LPITEMIDLIST pidlRc = NULL;
    LPCITEMIDLIST pidlRoot = _pidlRoot();

    //  If rooted on desktop, then nothing special needs to happen.
    //  All pidls are visible from the desktop.
    if (ILIsEmpty(pidlRoot))
    {
        pidlRc = pidlExt;
    }
    else
    {
        // Rooted on a pidl created via ILRootedCreateIDList.
        LPCITEMIDLIST pidlAbsRoot = ILRootedFindIDList(pidlRoot);
        ASSERT(pidlAbsRoot);

        // See if the external pidl is visible in our namespace.
        // If so, then create the version of the pidl that makes sense to us
        LPCITEMIDLIST pidlChild = ILFindChild(pidlAbsRoot, pidlExt);
        if (pidlChild)
        {
            pidlRc = ILCombine(pidlRoot, pidlChild);
        }
    }

    return pidlRc;

}

//---------------------------------------------------------------------------
//
// that function updates the drives in the tree
//
// that is only called by Tree.c where we know a _hwnd already exists
void CExplorerBand::_TreeUpdateDrives()
{
   // Refresh the list of drives in the tree
    LPITEMIDLIST pidlDrives = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
    if (pidlDrives)
    {
        LPITEMIDLIST pidl = _pidlRootify(pidlDrives);
        if (pidl) 
        {
            HTREEITEM hDrives = _TreeGetItemFromIDList(pidl);
            if (hDrives)
            {
                _TreeRefreshOneLevel(hDrives, FALSE);
            }

            if (pidl != pidlDrives) 
            {
                ILFree(pidl);
            }
        }

        ILFree(pidlDrives);
    }
}

//
//  Find the guy who got removed and blow away all his children.
//
void CExplorerBand::_TreeMediaRemoved(LPCITEMIDLIST pidl)
{

    // Even though we will be nuking the item recursively, we don't need
    // to do it this way, since our TVE_COLLAPSERESET is much more powerful.
    HTREEITEM hti = _TreeInvalidateFromIDList(pidl, FALSE);
    if (!hti)
        return;

    // Removing a CD is sort of like deleting in the sense that we
    // don't want to be viewing it after the removal.
    _TreeMayNavigateDueToDelete(pidl);

    // and collapse and reset everything from here down
    // since our world has changed
    TreeView_Expand(_hwnd, hti, TVE_COLLAPSE | TVE_COLLAPSERESET);
}

void CExplorerBand::_TreeInvalidateImageIndex(HTREEITEM hItem, int iImage)
{
    HTREEITEM hChild;
    TV_ITEM tvi;

    if (hItem) 
    {
        tvi.mask = TVIF_SELECTEDIMAGE | TVIF_IMAGE;
        tvi.hItem = hItem;

        TreeView_GetItem(_hwnd, &tvi);
        if (iImage == -1 ||
            tvi.iImage == iImage ||
            tvi.iSelectedImage == iImage) 
        {
            _TreeInvalidateItemInfo(hItem);
        }
    }

    hChild = TreeView_GetChild(_hwnd, hItem);
    if (!hChild)
        return;

    for ( ; hChild; hChild = TreeView_GetNextSibling(_hwnd, hChild))
    {
        _TreeInvalidateImageIndex(hChild, iImage);
    }
}

void CExplorerBand::_CancelMenuMode()
{
    // make sure we're not in menu mode because all that is going to nuke the menu
    if (GetForegroundWindow() == _hwnd)
        SendMessage(_hwnd, WM_CANCELMODE, 0, 0);
}

// given a rename (move) of pidlSrc to pidlDest, if
// pidl was a child of pidlSrc, what should it then become?
//
// returns:
// NULL if pidl wasn't the child of pidlSrc to begin with
LPITEMIDLIST GetPidlAfterRename(LPITEMIDLIST pidl, LPITEMIDLIST pidlSrc, LPITEMIDLIST pidlDest)
{
    LPITEMIDLIST pidlChild = ILFindChild(pidlSrc, pidl);
    if (pidlChild) 
    {
        pidlChild = ILCombine(pidlDest, pidlChild);
    }
    
    return pidlChild;
}

#ifdef DEBUG
BOOL CExplorerBand::_OTMatchesTV(LPCITEMIDLIST pidl, HTREEITEM hti)
{
    //  ASSERT that the node hasnt changed
    LPOneTreeNode lpn = OTGetNodeFromIDList(pidl, 0);
    if (lpn)
        OTRelease(lpn);
       
    return (_TreeGetFCTreeData(hti) == lpn);
}
#endif // DEBUG

void CExplorerBand::_RenameInFolder(HTREEITEM htiParent, LPCITEMIDLIST pidl)
{
    // a rename keeps the same onetree node, so just refresh
    HTREEITEM hti = _TreeInvalidateFromIDList(pidl, FALSE);
    if (hti) 
    {
        ASSERT(_OTMatchesTV(pidl, hti));

        LPOneTreeNode lpndParent = _TreeGetFCTreeData(hti);
        if (lpndParent) 
        {
            _TreeSortChildren(htiParent, lpndParent);
        }
    }        
}

//  for onetree.
BOOL TryQuickRename(LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra);

void CExplorerBand::_HandleRename(LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    LPITEMIDLIST pidlSrcParent = ILCloneParent(pidl);
    LPITEMIDLIST pidlDestParent = ILCloneParent(pidlExtra);
    HTREEITEM hti = pidlSrcParent ? _TreeGetItemFromIDList(pidlSrcParent) : NULL;
    HTREEITEM hti2 = pidlDestParent ? _TreeGetItemFromIDList(pidlDestParent) : NULL;

    if (hti && hti == hti2) 
    {
        //  make sure onetree is updated first...
        ENTERCRITICAL;
        TryQuickRename(pidl, pidlExtra);
        LEAVECRITICAL;
        _RenameInFolder(hti, pidlExtra);

    } 
    else 
    {
        // it was a move
        // delete one except the root
        _HandleRmDir(pidl);

        // add the other if it does not already exist
        if (hti2)
        {
            //  mk the new dir
            _HandleMkDir(pidlExtra);
            
            if (hti2 == TreeView_GetRoot(_hwnd))
            {
                // make sure the root item is expanded
                // it's confusing if you add an item in and
                // don't expand the root node because we don't show lines
                // at root, so you might not know that something was added
                TreeView_Expand(_hwnd, hti2, TVE_EXPAND);
            }

            // Update lpNode so that it knows it has children
            LPOneTreeNode lpndParent = _TreeGetFCTreeData(hti2);

            if (lpndParent)
                ++(lpndParent->cChildren);
        }
        
        if (pidl && pidlExtra) 
        {
            // that was a move.. we may need to re-select the tree
            // if our selection was under the pidlSrc
            LPITEMIDLIST pidlCur = _GetPidlCur();
            if (pidlCur) 
            {            
                LPITEMIDLIST pidlNewSelection = GetPidlAfterRename(pidlCur, pidl, pidlExtra);
                if (pidlNewSelection) 
                {
                    
                    LPOneTreeNode lpnNew = OTGetNodeFromIDList(pidlNewSelection, OTGNF_TRYADD);
                    if (lpnNew) 
                    {
                        hti = _TreeBuild(pidlNewSelection, FALSE, FALSE);
                        OTRelease(lpnNew);
                    }
                    ILFree(pidlNewSelection);
                }
                ILFree(pidlCur);
            }
        }
    }

    ILFree(pidlDestParent);
    ILFree(pidlSrcParent);
}

void CExplorerBand::_HandleRmDir(LPCITEMIDLIST pidl)
{
    HTREEITEM htiParent = NULL;
    HTREEITEM hti = _TreeGetItemFromIDList(pidl);
    if (hti) 
    {
        if (hti == TreeView_GetRoot(_hwnd)) 
        {
            // user deleted the root node...
            // do a full refresh
            _TreeRefreshAll();
        } 
        else 
        {
            htiParent = _TreeMayNavigateDueToDelete(pidl);
                
            TreeView_DeleteItem(_hwnd, hti);
        }
    }
    else 
    {
        // it's possible that we haven't expanded to that child, only
        // to the parent
        LPITEMIDLIST pidlParent = ILCloneParent(pidl);
        if (pidlParent)
        {
            htiParent = _TreeGetItemFromIDList(pidlParent);
            ILFree(pidlParent);
        }
    }

    // to ge tthe +/- right
    if (htiParent)
        _TreeInvalidateItemInfo(htiParent);
}

void CExplorerBand::_HandleMkDir(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);

    if (pidlParent)
    {
        HTREEITEM htiParent = _TreeGetItemFromIDList(pidlParent);

        //
        // Check if that child already is present for that parent.
        // If so, don't add it further. (Fix for duplicate sitemaps)
        //
        if (htiParent && !_TreeFindChildItem(htiParent, ILFindLastID(pidl)))
        {
            LPOneTreeNode lpnKid = OTGetNodeFromIDList(pidl, OTGNF_TRYADD);
            if (lpnKid)
            {
                _TreeInsertItem(htiParent, lpnKid);
                _TreeInvalidateItemInfo(htiParent);
                LPOneTreeNode lpndParent = OTGetParent(lpnKid);

                if (lpndParent) 
                {
                    _TreeSortChildren(htiParent, lpndParent);
                    if (htiParent == TreeView_GetRoot(_hwnd)) 
                    {
                        // make sure the root item is expanded
                        // it's confusing if you add an item in and
                        // don't expand the root node because we don't show lines
                        // at root, so you might not know that something was added
                        TreeView_Expand(_hwnd, htiParent, TVE_EXPAND);
                    }
                    OTRelease(lpndParent);
                }                   
                OTRelease(lpnKid);
            }
        }

        ILFree(pidlParent);
    }
}

HTREEITEM CExplorerBand::_TreeInvalidateFromIDList(LPCITEMIDLIST pidl, BOOL fRecurse)
{
    HTREEITEM hti = _TreeGetItemFromIDList(pidl);
    if (hti)
    {
        _TreeInvalidateItemInfoEx(hti, fRecurse);
    }
    return hti;
}

LPITEMIDLIST MyDocsIDList(void);

void CExplorerBand::_HandleUpdateDir(LPCITEMIDLIST pidl)
{
    HTREEITEM hti = _TreeInvalidateFromIDList(pidl, TRUE);    
    BOOL fIsDesktop = ILIsEmpty(pidl);

    // if we found the hti or if the pidl pointed to desktop
    if (hti) 
    {
        _TreeRefreshOneLevel(hti, !fIsDesktop);

        if (fIsDesktop)
        {
            //
            //  HACKHACK - super badness tradeoff: perf vs accuracy - ZekeL - 27-OCT-99
            //  when we get an UPDATEDIR for the desktop this can mean that 
            //  SCN actually collapsed any number of notifies to this single
            //  notify.  this can happen easily with any of the Aliased folders
            //  because they always send notifies for both their logical and
            //  physical locations in the namespace.  this includes MyDocs, NetHood
            //  and any filesystem folders on the desktop.  when moves or deletions
            //  of folders occur, and they are collapsed we can either:
            //
            //  1.  Update the entire tree - bad because it causes the treeview
            //  to flash, and if there are slow nodes, they must all be enum'd again.
            //  -- OR --
            //  2.  update only the desktop - bad because its the desktop's children
            //  that actually need the update.  the only problem is that we dont
            //  know which child it is that needs updating.
            //  -- OR --
            //  3.  Hack it to pieces - here we take the middle path and assume
            //  that its good enough to just update MyDocs, which is the store
            //  that most people would be managing.  and anyone else who 
            //  performs operations that leave us in state 2 (above), will
            //  be smart enough to use F5 to refresh the treeview.  the nodes 
            //  are useless so its unlikely that anything bad will come of
            //  these dead items, we just look lame.
            //
            //  THE REAL FIX - is in SCN (fsnotify.c).  we should update the 
            //  collapsing code to never identify the common parent as the
            //  desktop.  that way, in the Alias folder case, we would get
            //  two UPDATEDIR's. one for the physical folder (possibly collapsing all
            //  the way to MyComputer) and a second for the logical location 
            //  also limited to the first node off the desktop.
            //
          
            
            LPITEMIDLIST pidlMyDocs = MyDocsIDList();
            if (pidlMyDocs)
            {
                _HandleUpdateDir(pidlMyDocs);
                ILFree(pidlMyDocs);
            }
        }
    }
}

void CExplorerBand::_HandleServerDisconnect(LPCITEMIDLIST pidl)
{
    //  BUGBUG - the only place this is called from is the NetWare ShellExt ctxmenu.
    //  i am not sure why it is important to delete the children here
    HTREEITEM hti = _TreeInvalidateFromIDList(pidl, FALSE);

    if (hti)
    {
        IShellFolder *psf = _TreeBindToFolder(hti);
        if (psf)
        {
            HDPA htilist = _TreeCreateHTIList(psf, hti);
            _DeleteHTIListItemsFromTree(htilist);
            psf->Release();
        }
    }
}


void CExplorerBand::_HandleUpdateImage(LPSHChangeDWORDAsIDList pImage, LPCITEMIDLIST pidl)
{
    int iOldImage;

    if( pidl )
    {
        iOldImage = SHHandleUpdateImage(pidl);
        
        // This is a little confusing; -1 here means SHHandleUpdateImage failed, whereas if
        // pImage->dwItem1 == -1 (below), it means to invalidate all images.
        if( -1 == iOldImage )
        {
            return;
        }
    }
    else
    {
        iOldImage = (int)pImage->dwItem1;
    }

    _TreeInvalidateImageIndex(NULL, iOldImage);
}

//---------------------------------------------------------------------------
// Processes a FSNotify messages
//

//
//  There is a race condition between us and OneTree, where
//  it's random who gets the change notification first.  If we
//  gets it first, then we "invalidate" the item, then TreeView
//  asks us for the info, so we ask OneTree, but OneTree gives us
//  stale info because it hasn't been told about the change yet.
//
//  So if we invalidate an item due to a change notify, we have to
//  turn around and invalidate the OneTree cache so he won't give
//  us stale data.  We use _iInFSC to detect that this is happening.
//

HRESULT CExplorerBand::_TreeHandleFileSysChange(LONG lEvent, LPCITEMIDLIST c_pidl1, LPCITEMIDLIST c_pidl2)
{
    //
    //  If we are in the middle of Cabinet_ChangeView function,
    // ignore that event and update the entire tree later.
    //
    if (_uRecurse > 0) 
    {
        _fUpdateTree = TRUE;
        return S_OK;
    }

    //
    //  NO INTERNAL RETURNS ALLOWED or _iInFSC will get out of sync.
    //  To prevent you from making this mistake, I will neuter "return".
    //
    _iInFSC++;
#define return __not_allowed_to_return_because_iInFSC_is_still_incremented

    //
    //  Note that renames between directories are changed to
    //  create/delete pairs by SHChangeNotify.
    //
    LPITEMIDLIST pidl = c_pidl1 ? _pidlRootify((LPITEMIDLIST)c_pidl1) : NULL;
    LPITEMIDLIST pidl2 = c_pidl2 ? _pidlRootify((LPITEMIDLIST)c_pidl2) : NULL;
    
    TraceMsg(DM_TRACE, "sh TR - _TreeHandleFileSysChange called (fse = %x)", lEvent);

    if (pidl)
    {
        switch(lEvent)
        {
        case SHCNE_ASSOCCHANGED:
            OTInvalidateAll();
            _TreeRefreshAll();
            break;

        case SHCNE_RENAMEFOLDER:
            _HandleRename(pidl, pidl2);
            break;

        case SHCNE_RMDIR:
            _HandleRmDir(pidl);
            break;

        case SHCNE_MKDIR:
            _HandleMkDir(pidl);
            break;

        case SHCNE_UPDATEDIR:
            _HandleUpdateDir(pidl);
            break;

        case SHCNE_ATTRIBUTES:
        case SHCNE_UPDATEITEM:
        case SHCNE_NETSHARE:
        case SHCNE_NETUNSHARE:
        case SHCNE_MEDIAINSERTED:
            _TreeInvalidateFromIDList(pidl, FALSE);
            break;

        case SHCNE_MEDIAREMOVED:
            _TreeMediaRemoved(pidl);
            break;

        case SHCNE_DRIVEREMOVED:
        case SHCNE_DRIVEADD:
        case SHCNE_DRIVEADDGUI:
            _TreeUpdateDrives();
            break;

        case SHCNE_SERVERDISCONNECT:
            _HandleServerDisconnect(pidl);
            break;

        case SHCNE_UPDATEIMAGE:
            _HandleUpdateImage((LPSHChangeDWORDAsIDList)c_pidl1, pidl2);
            break;
        }
    }
    
    // ILFree checks for NULL
    if (pidl != c_pidl1)
        ILFree(pidl);
    if (pidl2 != c_pidl2)
        ILFree(pidl2);

    //
    //  Only now (after we decrement _iInFSC) is it safe to return.
    //
    _iInFSC--;
#undef return

    return S_OK;
}

BOOL CExplorerBand::_DetachItemWatch(CExpBandItemWatch *pwDetach)
{
    int i = DPA_GetPtrCount(_hdpaEnum);
    while (--i >= 0)
    {
        CExpBandItemWatch *pw = (CExpBandItemWatch *)DPA_FastGetPtr(_hdpaEnum, i);
        ASSERT(pw);
        if (pw == pwDetach)
        {
#ifdef DEBUG
            pw->_fAttached = FALSE;
#endif
            DPA_DeletePtr(_hdpaEnum, i);
            return TRUE;
        }
    }
    ASSERT(!"_DetachItemWatch: Item not in list");
    return FALSE;
}

void CExplorerBand::_TreeHandleDeleteItem(LPNM_TREEVIEW lpnmtv)
{
    _CancelMenuMode();

    HTREEITEM htiDeleted = lpnmtv->itemOld.hItem;
    if (htiDeleted == _htiCut) 
    {
        _htiCut = NULL;
        _TreeNukeCutState();
    }

    //
    //  For all pending enumerations, see if they're deleting an item
    //  that is active in the enumeration.  If so, then mark the
    //  enumeration stale and record enough information to continue
    //  the enumeration at the item after the stale one.
    //
    if (_hdpaEnum)
    {
        int i = DPA_GetPtrCount(_hdpaEnum);
        while (--i >= 0)
        {
            CExpBandItemWatch *pw = (CExpBandItemWatch *)DPA_FastGetPtr(_hdpaEnum, i);
            ASSERT(pw);
            if (htiDeleted == pw->_hti)
            {
                // Deleted an active item, oh my.
                // Note that this works if the sibling itself is deleted
                // (we merely step to the next next sibling)
                TraceMsg(DM_TRACE, "ca TR - _TreeHandleDeletedItem %x is active in enumeration", htiDeleted);
                pw->_hti = TreeView_GetNextSibling(_hwnd, pw->_hti);
                pw->_fStale = TRUE;
            }
        }
    }
    
    OTRelease(_TreeGetFCTreeData(htiDeleted));
}

//---------------------------------------------------------------------------
// Performance timing for treeview expand and contract.  This is a seperate function 
// so we don't use as much stack space during normal shell usage (ie the perf mode is not enabled).
// bFlag and bActive are used to prevent stop nodes from being entered without a start node since we
// get NM_CUSTOMDRAW's when moving throughout the tree.
void DoStopWatchTreeExpand(LPNM_TREEVIEW lpnmtv, HWND hwndTree, BOOL bFlag)
{
    static BOOL bActive = FALSE;
    TV_ITEM tvi;
    TCHAR szText[MAX_PATH];
    LPTSTR pszText = szText;
    int iLen;

    if(bFlag == TRUE)   // Start timing
    {
        bActive = TRUE;
        wnsprintf(szText, ARRAYSIZE(szText), TEXT("Shell TreeView: Start %s("), (lpnmtv->action == 2 ? TEXT("Expanding") : TEXT("Contracting")));
        iLen = lstrlen(szText);
        pszText = &szText[iLen];
        
        tvi.mask = TVIF_TEXT;
        tvi.hItem = ((TV_ITEM)lpnmtv->itemNew).hItem;
        tvi.pszText = pszText;
        tvi.cchTextMax = MAX_PATH - iLen - 3;
        TreeView_GetItem(hwndTree, &tvi);

        StrCat(szText, TEXT(")"));
        StopWatch_Start(SWID_TREE, szText, SPMODE_SHELL | SPMODE_DEBUGOUT);
    }
    else    // Stop timing
    {
        if(bActive)
        {
            bActive = FALSE;
            StopWatch_Stop(SWID_TREE, TEXT("Shell TreeView: Stop"), SPMODE_SHELL | SPMODE_DEBUGOUT);
        }
    }
}

//---------------------------------------------------------------------------
// Handle notification from the tree view.
LRESULT CExplorerBand::_TreeOnNotify(LPNMHDR lpnmhdr)
{
    ASSERT(_hwnd);      // that must be true...

    // now the ones to handle only if a tree is there.
    switch (lpnmhdr->code) 
    {
    case NM_RCLICK:
        _TreeHandleRClick();
        return 1;

    case NM_DBLCLK:
        //
        //  In case of double click, force the delayed selection now.
        // Otherwise, we can't prevent double logon dialog problems
        // described below.
        //
        if (!_TreeRealHandleSelChange())
        {
            //
            //  If it failed (canceled, out-of-memory, not accessible),
            // return TRUE (no further action).
            //  that code will prevent second logon dialog box when
            // the first one is canceled.
            //
            return TRUE;
        }
        break;

    case NM_RETURN:
        _TreeValidateCurrentSelection();
        return TRUE;

    case NM_CLICK:
        _TreeHandleClick();
        break;

    case NM_CUSTOMDRAW:
        if(g_dwStopWatchMode)   // Performance stop timing for tree expand/contract
        {
            DoStopWatchTreeExpand((LPNM_TREEVIEW) lpnmhdr, _hwnd, FALSE);
        }
        
        if (g_fRunningOnNT)
        {
            LPNMTVCUSTOMDRAW lpCD = (LPNMTVCUSTOMDRAW)lpnmhdr;

            switch(lpCD->nmcd.dwDrawStage) 
            {
                case CDDS_PREPAINT:
                    return g_fShowCompColor ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;

                case CDDS_ITEMPREPAINT:
                    // Do funky coloring only if the item would otherwise
                    // be the normal color.  Otherwise we try coloring blue
                    // when the background is already blue and it turns
                    // unreadable.
                    if (lpCD->clrText == GetSysColor(COLOR_WINDOWTEXT)) 
                    {
                        LPOneTreeNode lpn = (LPOneTreeNode)lpCD->nmcd.lItemlParam;
                        if (OTIsCompressed(lpn)) 
                        {
                            lpCD->clrText = g_crAltColor;
                        }
                    }
                    return CDRF_DODEFAULT;
            }

        }
        return CDRF_DODEFAULT;

    case TVN_BEGINDRAG:
    case TVN_BEGINRDRAG:
        if (!_fExpandingItem)
        {
            _TreeHandleBeginDrag(lpnmhdr->code == TVN_BEGINRDRAG, (LPNM_TREEVIEW)lpnmhdr);
        }
        else
        {
            MessageBeep(0);
        }
        break;

    case TVN_ITEMEXPANDING:
        if(g_dwStopWatchMode)   // Performance start timing for tree expand/contract
        {
            DoStopWatchTreeExpand((LPNM_TREEVIEW) lpnmhdr, _hwnd, TRUE);
        }

        if (!_fExpandingItem)
        {
            return _TreeHandleExpanding((LPNM_TREEVIEW) lpnmhdr);
        }
        else
        {
            MessageBeep(0);
        }
        break;

    case TVN_SELCHANGING:
        if (_fExpandingItem)
        {
            MessageBeep(0);
            return TRUE;
        }
        break;

    case TVN_SELCHANGED:
        _TreeHandleSelChange(((NM_TREEVIEW*)lpnmhdr)->action == TVC_BYKEYBOARD);
        break;

    case TVN_GETDISPINFO:
        _TreeGetDispInfo((TV_DISPINFO *)lpnmhdr);
        break;

    case TVN_BEGINLABELEDIT:
        return _TreeHandleBeginLabelEdit((TV_DISPINFO *)lpnmhdr);

    case TVN_ENDLABELEDIT:
        return _TreeHandleEndLabelEdit((TV_DISPINFO *)lpnmhdr);

    case TVN_DELETEITEM:
        _TreeHandleDeleteItem((LPNM_TREEVIEW)lpnmhdr);
        break;

    case NM_SETFOCUS:
        // q: why is this necessary?  no other bands do it
        // a: actually they do, e.g. CBEN_BEGINEDIT in aeditbox.cpp
        //_et.UIActivateIO(TRUE, NULL);   // cause OnFocusChangeIS
        UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
        break;
    }
    return 0L;
}

void CExplorerBand::_TreeRefreshOneLevel(HTREEITEM hItem, BOOL bRecurse)
{
    // we might want a flag to not traverse through unexpanded
    // branches, but failing out all the time might be wrong
    HTREEITEM hChild = TreeView_GetChild(_hwnd, hItem);
    if (!hChild)
        return;
    
    LPOneTreeNode lpn = _TreeGetFCTreeData(hItem);
    if (lpn)
    {
        lpn->fInvalid = TRUE;
        lpn->fHasAttributes = FALSE;
    }

    // Don't recurse if _TreeFillOneLevel failed.
    if (_TreeFillOneLevel(hItem, TRUE) && bRecurse)
    {
        // Make sure we start of at the first child after filling...
        CExpBandItemWatch w(TreeView_GetChild(_hwnd, hItem));

        if (_AttachItemWatch(&w))
        {
            while (w.Item())
            {
                _TreeRefreshOneLevel(w.Item(), TRUE);
                w.NextItem(_hwnd);
            }
        }
        _DetachItemWatch(&w);
    }
}


void CExplorerBand::_TreeTrimInvisible(HTREEITEM hItem)
{
    HTREEITEM hChild;

    hChild = TreeView_GetChild(_hwnd, hItem);
    if (!hChild)
        return;

    // Check if the child is visible to see if the level is
    // currently expanded, and if the folder still exists; delete
    // the child and siblings if not
    // Note that the root level is always expanded
    if (hItem && TreeView_GetNextVisible(_hwnd, hItem) != hChild)
    {
        TreeView_Expand(_hwnd, hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
    }
    else
    {
        for ( ; hChild; hChild = TreeView_GetNextSibling(_hwnd, hChild))
        {
            _TreeTrimInvisible(hChild);
        }
    }
}


BOOL CExplorerBand::_TreeRefreshAll()
{
    BOOL fRet = FALSE;

    HTREEITEM htiOld = TreeView_GetSelection(_hwnd);
    _TreeTrimInvisible(NULL);
    _TreeRefreshOneLevel(NULL, TRUE);
    HTREEITEM htiNew = TreeView_GetSelection(_hwnd);

    //
    // If hti!=htiNew, it means the node is removed.
    //
    if (htiOld != htiNew)
    {
        LPOneTreeNode lpnNew;
        LPITEMIDLIST pidlCur = _GetPidlCur();
        if (pidlCur) 
        {
            // if our selection got lost, it could have been because the network
            // enumeration isn't reliable..  try forcing it in
            lpnNew = OTGetNodeFromIDList(pidlCur, OTGNF_TRYADD);
            if (lpnNew) 
            {
                htiNew = _TreeBuild(pidlCur, FALSE, FALSE);
                OTRelease(lpnNew);
                
            } 
            else 
            {

                TraceMsg(DM_TRACE, "ca TR - _TreeRefreshAll hti,htiNew=%x,%x fExp=%x", htiOld, htiNew,
                         _TreeGetItemState(htiNew) & TVIS_EXPANDEDONCE);

                lpnNew = _TreeGetFCTreeData(htiNew);

                //
                // Check if the new node is invalidated or not.
                //
                if (lpnNew && lpnNew->fInvalid)
                {
                    //
                    //  It is invalidated. It means EnumObjects on that node failed.
                    // Go to the parent (to avoid the same error on right-side).
                    //
                    HTREEITEM htiParent = TreeView_GetParent(_hwnd, htiNew);
                    TraceMsg(DM_TRACE, "ca TR - _TreeRefreshAll lpnNew is invalid");
                    TreeView_SelectItem(_hwnd, htiParent);
                    htiNew = htiParent;
                }
            }

            //
            //  Handle the selection change synchronously to avoid refreshing
            // invalid right pane.
            //
            if (htiOld != htiNew)
                _TreeRealHandleSelChange();

            fRet = TRUE;
            ILFree(pidlCur);
        }
    }


#ifdef SETALLVIS
    // We want to make sure the following UpdateWindow does the job
    // completely
    _TreeSetAllVisItemInfos();
#endif
    UpdateWindow(_hwnd);
    return fRet;
}


#endif
