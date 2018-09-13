#include "priv.h"
#include "sccls.h"
#include "resource.h"
#include "mshtmhst.h"
#include "deskbar.h"
#include "bands.h"
#include "multimon.h"
#define WANT_CBANDSITE_CLASS
#include "bandsite.h"
#include "mmhelper.h"  // Helper functions for Multi-Mon

#include <trayp.h>      // TM_*
#include <desktray.h>   // IDeskTray

#include "dbapp.h"

#include "mluisupp.h"

/*
 this virtual app implments DeskBars that you have on the desktop.
 it has the glue that combines CDeskBar with CBandSite and populates the 
 bands (as well as persistance and such)
 
 -Chee
 */

#define DM_INIT         0       
#define DM_PERSIST      0               // trace IPS::Load, ::Save, etc.
#define DM_MENU         0               // menu code
#define DM_DRAG         0               // drag&drop
#define DM_TRAY         0               // tray: marshal, side, etc.

#ifdef DEBUG
extern unsigned long DbStreamTell(IStream *pstm);
#else
#define DbStreamTell(pstm)      ((ULONG) 0)
#endif


#define SUPERCLASS CDeskBar

/* 
 Instead of just 4 Deskbars on the whole desktop, we now have 4 deskbars for
 each monitor, however, this brings problem whenever a monitor goes away, we 
 need to clean up the following datastructure.  
 - dli
 */

// BUGBUG: (dli) maybe this should be moved into multimon.h
// however, people should not get into the habbit of depending on this. 
// and it's really not used anywhere else, so, keep it here for now. 
#define DSA_MONITORSGROW 1

typedef struct DeskBarsPerMonitor {
    HMONITOR        hMon; 
    IDeskBar*       Deskbars[4];
} DESKBARSPERMONITOR, *LPDESKBARSPERMONITOR;

HDSA g_hdsaDeskBars = NULL;

enum ips_e {
    IPS_FALSE,    // reserved, must be 0 (FALSE)
    IPS_LOAD,
    IPS_INITNEW
};

CASSERT(IPS_FALSE == 0);

CDeskBarApp::~CDeskBarApp()
{
    _LeaveSide();
    
    if (_pbs)
        _pbs->Release();
    
    if (_pcm)
        _pcm->Release();
    
}

LRESULT CDeskBarApp::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);

    if (!_hwnd) {
        return lres;                        // destroyed by superclass
    }

    if (_eMode == WBM_BFLOATING) {
        switch (uMsg) {
        case WM_NOTIFY:
        {
            //
            // override the hittest value to be HTCAPTION if we're docked browser based
            //
            NMHDR* pnm = (NMHDR*)lParam;
            
            if (pnm->code == NM_NCHITTEST && 
                pnm->hwndFrom == _hwndChild) {
                //
                // in the floating bug docked int he browser, we don't do
                // mdi child stuff, so we make the gripper work as the caption
                // 
                NMMOUSE* pnmm = (NMMOUSE*)pnm;
                if (pnmm->dwHitInfo == RBHT_CAPTION ||
                    pnmm->dwHitInfo == RBHT_GRABBER) 
                    lres = HTTRANSPARENT;
            }
        }
        break;
        
        case WM_NCHITTEST:
            // all "client" areas are captions in this mode
            if (lres == HTCLIENT)
                lres = HTCAPTION;
            break;
        
        case WM_SETCURSOR:
            DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
            return TRUE;
        }
    }
    
    return lres;
}

BOOL CDeskBarApp::_OnCloseBar(BOOL fConfirm)
{
    // if we are closing a bar with no bands in it, don't pop up the dialog
    if ((_pbs && (_pbs->EnumBands(-1,NULL)==0)) ||
        (!fConfirm || ConfirmRemoveBand(_hwnd, IDS_CONFIRMCLOSEBAR, TEXT(""))) )
        return SUPERCLASS::_OnCloseBar(FALSE);
    return FALSE;
}

// Gets the Deskbars on a specific monitor 
// DBPM -- DeskBars Per Monitor 
LPDESKBARSPERMONITOR GetDBPMWithMonitor(HMONITOR hMon, BOOL fCreate)
{
    int ihdsa;
    LPDESKBARSPERMONITOR pdbpm;

    if (!g_hdsaDeskBars) {
        if (fCreate)
            g_hdsaDeskBars = DSA_Create(SIZEOF(DESKBARSPERMONITOR), DSA_MONITORSGROW);
    }

    if (!g_hdsaDeskBars)
        return NULL;
    
    // If we find the DBPM with this HMONITOR, return it. 
    for (ihdsa = 0; ihdsa < DSA_GetItemCount(g_hdsaDeskBars); ihdsa++) {
        pdbpm = (LPDESKBARSPERMONITOR)DSA_GetItemPtr(g_hdsaDeskBars, ihdsa);
        if (pdbpm->hMon == hMon)
            return pdbpm;
    }

    if (fCreate) {
        DESKBARSPERMONITOR dbpm = {0};
        // This monitor is not setup, so set it, and set us the
        // the ownder of _uSide
        dbpm.hMon = hMon;
        ihdsa = DSA_AppendItem(g_hdsaDeskBars, &dbpm);
        pdbpm = (LPDESKBARSPERMONITOR)DSA_GetItemPtr(g_hdsaDeskBars, ihdsa);
        return pdbpm;
    }
    
    // When all else fails, return NULL
    return NULL;
}
    
void CDeskBarApp::_LeaveSide()
{
    if (ISABE_DOCK(_uSide) && !ISWBM_FLOAT(_eMode)) {
        // remove ourselves from the array list of where we were
        LPDESKBARSPERMONITOR pdbpm = GetDBPMWithMonitor(_hMon, FALSE);
        if (pdbpm && (pdbpm->Deskbars[_uSide] == this)) {
            ASSERT(pdbpm->hMon);
            ASSERT(pdbpm->hMon == _hMon);
            pdbpm->Deskbars[_uSide] = NULL;
        }
    }
}

//***
// NOTES
//  BUGBUG should we create/use IDeskTray::AppBarGetState?
UINT GetTraySide(HMONITOR * phMon)
{
    LRESULT lTmp;
    APPBARDATA abd;
    
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = GetTrayWindow();
    if (phMon)
        Tray_GetHMonitor(abd.hWnd, phMon);

    abd.uEdge = (UINT)-1;
    //lTmp = g_pdtray->AppBarGetTaskBarPos(&abd);
    lTmp = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    ASSERT(lTmp);
    TraceMsg(DM_TRAY, "gts: ret=ABE_%d", abd.uEdge);
    return abd.uEdge;
}

//***
// ENTRY/EXIT
//  fNoMerge    is for the IPS::Load case
// NOTES
//  warning: be careful of reentrancy!  fNoMove is how we guard against it.
void CDeskBarApp::_SetModeSide(UINT eMode, UINT uSide, HMONITOR hMonNew, BOOL fNoMerge) 
{
    BOOL fNoMove;

    // make sure we don't merge etc. on NOOP moves.
    // we do such moves to force refresh (e.g. for autohide and IPS::Load);
    // also happens w/ drags which end up back where they started
    fNoMove = (eMode == _eMode && uSide == _uSide && hMonNew == _hMon);

    if (!fNoMove)
        _LeaveSide();
    
    // warning: this may call (e.g.) AppBarRegister, which causes a
    // resize, which calls back to us.  careful of reentrancy!!!
    // if we do reenter we end up w/ nt5:155043, where entry #1 has
    // fNoMove==0, then we get a recalc, entry #2 has fNoMove==1,
    // and we set our side array to us, then return back to entry
    // #1 which merges into itself!
    SUPERCLASS::_SetModeSide(eMode, uSide, hMonNew, fNoMerge);

    if (!fNoMove) {
        if (ISABE_DOCK(_uSide) && !ISWBM_FLOAT(_eMode)) {
            LPDESKBARSPERMONITOR pdbpm = GetDBPMWithMonitor(hMonNew, TRUE);
            HMONITOR hMonTray = NULL;
            if (pdbpm) {
                if (fNoMerge) {
                    if (!pdbpm->Deskbars[_uSide]) {
                        // 1st guy on an edge owns it
                        // if we don't do this, when we load persisted state on logon
                        // we end up w/ *no* edge owner (since fNoMerge), so we don't
                        // merge on subsequent moves.
                        goto Lsetowner;
                    }
                }
                else if (pdbpm->Deskbars[_uSide]) {
                    // if someone already there, try merging into them
#ifdef DEBUG
                    // alt+drag suppresses merge
                    // DEBUG only since don't track >1 per side, but useful
                    // for testing appbars and toolbars anyway
                    if (!(GetKeyState(VK_MENU) < 0))
#endif
                    {
                        extern IBandSite* _GetBandSite(IDeskBar * pdb);
                        IBandSite *pbs;
                        
                        pbs = _GetBandSite(pdbpm->Deskbars[_uSide]);
                        // nt5:215952: should 'never' have pbs==0 but somehow
                        // it does happen (during deskbar automation tests).
                        // call andyp or tjgreen if you hit this assert so
                        // we can figure out why.
                        if (TPTR(pbs)) {
                            _MergeSide(pbs);            // dst=pbs, src=this
                            pbs->Release();
                        }
                    }
                }
                else if ((GetTraySide(&hMonTray) == _uSide) && (hMonTray == _hMon) && !(GetKeyState(VK_SHIFT) < 0)) {
                    // ditto for tray (but need to marshal/unmarshal)
#ifdef DEBUG
                    // alt+drag suppresses merge
                    // DEBUG only since don't track >1 per side, but useful
                    // for testing appbars and toolbars anyway
                    if (!(GetKeyState(VK_MENU) < 0))
#endif
                    {
                        _MergeSide((IBandSite *)1);     // dst=pbs, src=this
                    }
                }
                else {
                    // o.w. nobody there yet, set ourselves as owner
                    ASSERT(pdbpm->hMon);
                    ASSERT(pdbpm->hMon == hMonNew);
Lsetowner:
                    TraceMsg(DM_TRAY, "cdba._sms: 1st side owner this=0x%x", this);
                    pdbpm->Deskbars[_uSide] = this;
                }
            }
        }
    }
}

void CDeskBarApp::_UpdateCaptionTitle()
{
    if (ISWBM_FLOAT(_eMode)) {
        int iCount = (int)_pbs->EnumBands((UINT)-1, NULL);
        if (iCount == 1) {
            DWORD dwBandID;
            if (SUCCEEDED(_pbs->EnumBands(0, &dwBandID))) {
                WCHAR wszTitle[80];
                if (SUCCEEDED(_pbs->QueryBand(dwBandID, NULL, NULL, wszTitle, ARRAYSIZE(wszTitle)))) {
                    USES_CONVERSION;
                    SetWindowText(_hwnd, W2T(wszTitle));
                }
            }
        }
        else {
            TCHAR szTitle[80];
            szTitle[0] = 0;
            MLLoadString(IDS_WEBBARSTITLE,szTitle,ARRAYSIZE(szTitle));
            SetWindowText(_hwnd, szTitle);
        }
    }
}


void CDeskBarApp::_NotifyModeChange(DWORD dwMode)
{
    SUPERCLASS::_NotifyModeChange(dwMode);
    _UpdateCaptionTitle();
}

//***   GetTrayIface -- get iface from tray (w/ marshal/unmarshal)
//
HRESULT GetTrayIface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_FAIL;
    HWND hwndTray;
    IStream *pstm;

    TraceMsg(DM_TRAY, "gtif: marshal!");

    *ppvObj = NULL;

    hwndTray = GetTrayWindow();
    if (hwndTray) {
        pstm = (IStream *) SendMessage(hwndTray, TM_MARSHALBS, (WPARAM)(GUID *)&riid, 0);

        if (EVAL(pstm)) {
            // paired w/ matching Marshal in explorer (TM_MARSHALBS)
            hr = CoGetInterfaceAndReleaseStream(pstm, riid, ppvObj);
            ASSERT(SUCCEEDED(hr));
        }
    }

    return hr;
}

//***   _MergeSide -- merge two deskbars into one
// ENTRY/EXIT
//  this    [INOUT] destination deskbar (ptr:1 if tray)
//  pdbSrc  [INOUT] source deskbar; deleted if all bands moved successfully
//  ret     S_OK if all bands moved; S_FALSE if some moved; E_* o.w.
HRESULT CDeskBarApp::_MergeSide(IBandSite *pbsDst)
{
    extern HRESULT _MergeBS(IDropTarget *pdtDst, IBandSite *pbsSrc);
    HRESULT hr;
    IDropTarget *pdtDst;

    AddRef();   // make sure we don't disappear partway thru operation

    if (pbsDst == (IBandSite *)1) {
        // get (marshal'ed) iface from tray
        hr = GetTrayIface(IID_IDropTarget, (void **)&pdtDst);
        ASSERT(SUCCEEDED(hr));
    }
    else {
        // don't merge into ourself!
        ASSERT(pbsDst != _pbs);
        ASSERT(!SHIsSameObject(pbsDst, SAFECAST(_pbs, IBandSite*)));

        hr = pbsDst->QueryInterface(IID_IDropTarget, (void **)&pdtDst);
        ASSERT(SUCCEEDED(hr));
    }
    ASSERT(SUCCEEDED(hr) || pdtDst == NULL);

    if (pdtDst) {
        hr = _MergeBS(pdtDst, _pbs);
        pdtDst->Release();
    }

    Release();

    return hr;
}

void CDeskBarApp::_CreateBandSiteMenu()
{
    CoCreateInstance(CLSID_BandSiteMenu, NULL,CLSCTX_INPROC_SERVER, 
                     IID_IContextMenu, (LPVOID*)&_pcm);
    if (_pcm) {
        IShellService* pss;
        
        _pcm->QueryInterface(IID_IShellService, (LPVOID*)&pss);
        if (pss) {
            pss->SetOwner((IBandSite*)_pbs);
            pss->Release();
        }
    }
}

HRESULT CDeskBarApp::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IContextMenu)) {
        
        if (!_pcm) {
            _CreateBandSiteMenu();
        }
        
        // only return out our pointer if we got the one we're going
        // to delegate to
        if (_pcm) {
            *ppvObj = SAFECAST(this, IContextMenu*);
            AddRef();
            return S_OK;
        }
    }
    return SUPERCLASS::QueryInterface(riid, ppvObj);
}

HRESULT CDeskBarApp::QueryService(REFGUID guidService,
                                    REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService,SID_SBandSite)) {
        return QueryInterface(riid, ppvObj);
    }
    
    return SUPERCLASS::QueryService(guidService, riid, ppvObj);
}


HRESULT CDeskBarApp::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    int idCmd = -1;

    if (!HIWORD(pici->lpVerb))
        idCmd = LOWORD(pici->lpVerb);

    if (idCmd >= _idCmdDeskBarFirst) {
        _AppBarOnCommand(idCmd - _idCmdDeskBarFirst);
        return S_OK;
    }
    
    return _pcm->InvokeCommand(pici);
    
}

HRESULT CDeskBarApp::GetCommandString(  UINT_PTR    idCmd,
                                        UINT        uType,
                                        UINT       *pwReserved,
                                        LPSTR       pszName,
                                        UINT        cchMax)
{
    return _pcm->GetCommandString(idCmd, uType, pwReserved, pszName, cchMax);
}


HRESULT CDeskBarApp::QueryContextMenu(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags)
{
    int i;
    HMENU hmenuSrc;
    i = _pcm->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    _idCmdDeskBarFirst = i;
    hmenuSrc = _GetContextMenu();

    // BUGBUG off-by-1 and by idCmdFirst+i, i think...
    i += Shell_MergeMenus(hmenu, hmenuSrc, (UINT)-1, idCmdFirst + i, idCmdLast, MM_ADDSEPARATOR) - (idCmdFirst + i);
    DestroyMenu(hmenuSrc);
    
    return i;   // potentially off-by-1, but who cares...
}


//***
// NOTES
//  BUGBUG nuke this, fold it into CDeskBarApp_CreateInstance
HRESULT DeskBarApp_Create(IUnknown** ppunk)
{
    HRESULT hres;

    *ppunk = NULL;
    
    CDeskBarApp *pdb = new CDeskBarApp();
    if (!pdb)
        return E_OUTOFMEMORY;
    
    CBandSite *pcbs = new CBandSite(NULL);
    if (pcbs)
    {
        IDeskBarClient *pdbc = SAFECAST(pcbs, IDeskBarClient*);
        hres = pdb->SetClient(pdbc);
        if (SUCCEEDED(hres))
        {
            pdb->_pbs = pcbs;
            pcbs->AddRef();
            *ppunk = SAFECAST(pdb, IDeskBar*);
        }
    
        pdbc->Release();
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    if (FAILED(hres))
        pdb->Release();
        
    return hres;
}


STDAPI CDeskBarApp_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres;
    IUnknown *punk;

    // aggregation checking is handled in class factory

    hres = DeskBarApp_Create(&punk);
    if (SUCCEEDED(hres)) {
        *ppunk = SAFECAST(punk, IDockingWindow*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//*** CDeskBarApp::IInputObject*::* {
//

HRESULT CDeskBarApp::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (lpMsg->message == WM_SYSKEYDOWN) {
        if (lpMsg->wParam == VK_F4) {
            // ie4:28819: need to trap VK_F4 here, o.w. CBaseBrowser::TA
            // does a last-chance (winsdk)::TA (to IDM_CLOSE) and doing a
            // shutdown!
            PostMessage(_hwnd, WM_CLOSE, 0, 0);
            return S_OK;
        }
    }

    return SUPERCLASS::TranslateAcceleratorIO(lpMsg);
}

// }

//*** CDeskBarApp::IPersistStream*::* {
//

HRESULT CDeskBarApp::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_DeskBarApp;
    return S_OK;
}

HRESULT CDeskBarApp::IsDirty(void)
{
    return S_FALSE; // Never be dirty
}


//
// Persisted CDeskBarApp
//
#define STC_VERSION     1

struct SThisClass
{
    DWORD   cbSize;
    DWORD   cbVersion;
};

HRESULT CDeskBarApp::Load(IStream *pstm)
{
    SThisClass stc;
    ULONG cbRead;
    HRESULT hres;

    TraceMsg(DM_PERSIST, "cdba.l enter(this=%x pstm=%x) tell()=%x", this, pstm, DbStreamTell(pstm));

    ASSERT(!_eInitLoaded);
    _eInitLoaded = IPS_LOAD;

    hres = pstm->Read(&stc, SIZEOF(stc), &cbRead);
#ifdef DEBUG
    // just in case we toast ourselves (offscreen or something)...
    static BOOL fNoPersist = FALSE;
    if (fNoPersist)
        hres = E_FAIL;
#endif
    if (hres==S_OK && cbRead==SIZEOF(stc)) {
        if (stc.cbSize==SIZEOF(SThisClass) && stc.cbVersion==STC_VERSION) {
            _eInitLoaded = IPS_LOAD;    // BUGBUG what if OLFS of bands fails?

            hres = SUPERCLASS::Load(pstm);

            TraceMsg(DM_INIT, "cdba::Load succeeded");
        } else {
            TraceMsg(DM_ERROR, "cdba::Load failed stc.cbSize==SIZEOF(SThisClass) && stc.cbVersion==SWB_VERSION");
            hres = E_FAIL;
        }
    } else {
        TraceMsg(DM_ERROR, "cdba::Load failed (hres==S_OK && cbRead==SIZEOF(_adEdge)");
        hres = E_FAIL;
    }
    TraceMsg(DM_PERSIST, "cdba.l leave tell()=%x", DbStreamTell(pstm));
    
    // after loading this, if we find that we're supposed to be browser docked,
    // make our bandsite always have a gripper
    if (_eMode == WBM_BFLOATING)
    {
        BANDSITEINFO bsinfo;

        bsinfo.dwMask = BSIM_STYLE;
        bsinfo.dwStyle = BSIS_ALWAYSGRIPPER;

        _pbs->SetBandSiteInfo(&bsinfo);
    }
    return hres;
}

HRESULT CDeskBarApp::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hres;
    SThisClass stc;

    TraceMsg(DM_PERSIST, "cdba.s enter(this=%x pstm=%x) tell()=%x", this, pstm, DbStreamTell(pstm));
    stc.cbSize = SIZEOF(SThisClass);
    stc.cbVersion = STC_VERSION;

    hres = pstm->Write(&stc, SIZEOF(stc), NULL);
    if (SUCCEEDED(hres)) {
        SUPERCLASS::Save(pstm, fClearDirty);
    }
    
    TraceMsg(DM_PERSIST, "cdba.s leave tell()=%x", DbStreamTell(pstm));
    return hres;
}

HRESULT CDeskBarApp::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ULARGE_INTEGER cbMax = { SIZEOF(SThisClass), 0 };
    *pcbSize = cbMax;
    return S_OK;
}

HRESULT CDeskBarApp::InitNew(void)
{
    HRESULT hres;

    ASSERT(!_eInitLoaded);
    _eInitLoaded = IPS_INITNEW;
    TraceMsg(DM_INIT, "CDeskBar::InitNew called");

    hres = SUPERCLASS::InitNew();
    if (FAILED(hres))
        return hres;

    // can't call _InitPos4 until set site in SetSite

    return hres;
}


HRESULT CDeskBarApp::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL) {
        /*NOTHING*/
    } 
    else if (IsEqualGUID(CGID_DeskBarClient, *pguidCmdGroup)) {
        switch (nCmdID) {
        case DBCID_EMPTY:
            if (_pbs) {
                // if we have no bands left, close
                PostMessage(_hwnd, WM_CLOSE, 0, 0);
            }
            return S_OK;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_DeskBand)) {
        switch (nCmdID) {
        case DBID_BANDINFOCHANGED:
            _UpdateCaptionTitle();
            return S_OK;
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_BandSite)) {
        switch (nCmdID) {
        case BSID_BANDADDED:
        case BSID_BANDREMOVED:
            _UpdateCaptionTitle();
            return S_OK;
        }
    }

    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

HRESULT CDeskBarApp::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    HRESULT hres;

    ASSERT(!_eInitLoaded);
    _eInitLoaded = IPS_LOAD;
    TraceMsg(DM_INIT, "CDeskBarApp::Load(bag) called");

    hres = SUPERCLASS::Load(pPropBag, pErrorLog);
    
    // after loading this, if we find that we're supposed to be browser docked,
    // make our bandsite always have a gripper
    if (_eMode == WBM_BFLOATING)
    {
        BANDSITEINFO bsinfo;

        bsinfo.dwMask = BSIM_STYLE;
        bsinfo.dwStyle = BSIS_ALWAYSGRIPPER;

        _pbs->SetBandSiteInfo(&bsinfo);
    }
    return hres;
}

IBandSite * _GetBandSite(IDeskBar * pdb)
{
    IBandSite* pbs = NULL;
    
    if (pdb) {
        IUnknown* punkClient;
        
        pdb->GetClient(&punkClient);
        if (punkClient) {
            punkClient->QueryInterface(IID_IBandSite, (LPVOID*)&pbs);
            punkClient->Release();
        }
    }
    
    return pbs;
}

        
IBandSite* DeskBarApp_GetBandSiteOnEdge(UINT uEdge)
{
    // BUGBUG: (dli) HACK ALERT!! if no HMONITOR is passed in, use the primary monitor
    // should make sure that there is always a valid HMONITOR passed in
    HMONITOR hMon = GetPrimaryMonitor();
    // --------------------------------------------------------------

    LPDESKBARSPERMONITOR pdbpm = GetDBPMWithMonitor(hMon, FALSE);
    if (pdbpm) {
        ASSERT(pdbpm->hMon);
        ASSERT(pdbpm->hMon == hMon);
        return _GetBandSite(pdbpm->Deskbars[uEdge]);
    }
    return NULL;
}



IBandSite* DeskBarApp_GetBandSiteAtPoint(LPPOINT ppt)
{
    HWND hwnd = WindowFromPoint(*ppt);
    HMONITOR hMon = MonitorFromPoint(*ppt, MONITOR_DEFAULTTONULL);
    if (hwnd && hMon) {
        LPDESKBARSPERMONITOR pdbpm = GetDBPMWithMonitor(hMon, FALSE);
        if (pdbpm) {
            ASSERT(pdbpm->hMon);
            ASSERT(pdbpm->hMon == hMon);
            int i;
            for (i = 0; i < 4; i++) {
                if (pdbpm->Deskbars[i]) {
                    HWND hwndDeskbar;
                    pdbpm->Deskbars[i]->GetWindow(&hwndDeskbar);
            
                    if (hwndDeskbar == hwnd) {
                        return _GetBandSite(pdbpm->Deskbars[i]); 
                    }
                }
            }
        }
    }
    return NULL;
}

// }
