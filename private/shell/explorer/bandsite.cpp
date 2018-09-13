#include "cabinet.h"
#define CPP_FUNCTIONS
#include <crtfree.h>
#include "rcids.h"
#include <shguidp.h>
#include "bandsite.h"
#include "shellp.h"
#include "shdguid.h"
#include "taskband.h"
#include "taskbar.h"
#include <regstr.h>

#define DM_FOCUS            0           // focus
#define DM_FOCUS2           0           // focus (verbose)
#define DM_PERSIST          0           // IPS::Load, ::Save, etc.
#define DM_SHUTDOWN         DM_TRACE    // shutdown

IUnknown* Tasks_CreateInstance();


extern IStream *GetDesktopViewStream(DWORD grfMode, LPCTSTR pszName);

HRESULT PersistStreamLoad(IStream *pstm, IUnknown *punk);
HRESULT PersistStreamSave(IStream *pstm, BOOL fClearDirty, IUnknown *punk);

const TCHAR c_szTaskbar[] = TEXT("Taskbar");

// {69B3F106-0F04-11d3-AE2E-00C04F8EEA99}
static const GUID CLSID_TrayBandSite = 
{ 0x69b3f106, 0xf04, 0x11d3, { 0xae, 0x2e, 0x0, 0xc0, 0x4f, 0x8e, 0xea, 0x99 
} };

//***   CTrayBandSite {

class CTrayBandSite: public IBandSiteHelper, 
        public IBandSite
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IBandSiteHelper ***
    virtual STDMETHODIMP LoadFromStreamBS(IStream *pstm, REFIID riid, LPVOID *ppv);
    virtual STDMETHODIMP SaveToStreamBS(IUnknown *punk, IStream *pstm);

    // *** IBandSite methods ***
    STDMETHOD(AddBand)          (THIS_ IUnknown* punk);
    STDMETHOD(EnumBands)        (THIS_ UINT uBand, DWORD* pdwBandID);
    STDMETHOD(QueryBand)        (THIS_ DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName) ;
    STDMETHOD(SetBandState)     (THIS_ DWORD dwBandID, DWORD dwMask, DWORD dwState) ;
    STDMETHOD(RemoveBand)       (THIS_ DWORD dwBandID);
    STDMETHOD(GetBandObject)    (THIS_ DWORD dwBandID, REFIID riid, LPVOID* ppvObj);
    STDMETHOD(SetBandSiteInfo)  (THIS_ const BANDSITEINFO * pbsinfo);
    STDMETHOD(GetBandSiteInfo)  (THIS_ BANDSITEINFO * pbsinfo);
    
    IContextMenu* GetContextMenu();
    void SetInner(IUnknown* punk);
    void SetLoaded(BOOL fLoaded) {_fLoaded = fLoaded;}
    BOOL HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    
protected:
    CTrayBandSite();
    virtual ~CTrayBandSite();

    BOOL _CreateBandSiteMenu(IUnknown* punk);
    BOOL _IsTaskBand(IUnknown* punk);
    BOOL _IsTaskBandId(DWORD idBand);
    void _BroadcastExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    friend IUnknown* Tray_CreateView();
    friend HRESULT BandSite_AddRequiredBands(CTrayBandSite* pbs);
    friend HRESULT BandSite_CheckBands(CTrayBandSite* pbs);
    friend void BandSite_HandleDelayBootStuff(IUnknown* punk);
    
    UINT    _cRef;
    IUnknown *_punkInner;
    IBandSite *_pbsInner;
    IUnknown* _punkTasks;

    // bandsite context menu
    IContextMenu* _pcm;
    HWND _hwnd;
    BOOL _fLoaded;
    BOOL _fDelayBootStuffHandled;
};

CTrayBandSite* IUnknownToCTrayBandSite(IUnknown* punk)
{
    CTrayBandSite* ptbs;
    
    punk->QueryInterface(CLSID_TrayBandSite, (LPVOID*)&ptbs);
    ASSERT(ptbs);
    punk->Release();

    return ptbs;
}

CTrayBandSite::CTrayBandSite() : _cRef(1)
{
    return;
}

CTrayBandSite::~CTrayBandSite()
{
    if (_punkTasks)
        _punkTasks->Release();
    
    if (_pcm)
        _pcm->Release();
    
    return;
}

ULONG CTrayBandSite::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTrayBandSite::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    _cRef = 1000;               // guard against recursion
    
    if (_pbsInner) {
        AddRef();
        _pbsInner->Release();
    }
    
    // this must come last
    if (_punkInner)
        _punkInner->Release();  // paired w/ CCI aggregation
    
    ASSERT(_cRef == 1000);

    delete this;
    return 0;
}

HRESULT CTrayBandSite::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IBandSiteHelper))
    {
        // BUGBUG make IBandSite be the preferred iface
        *ppvObj = SAFECAST(this, IBandSiteHelper*);
    } 
    else if (IsEqualIID(riid, IID_IBandSite)) 
    {
        *ppvObj = SAFECAST(this, IBandSite*);
    }
    else if (IsEqualIID(riid, CLSID_TrayBandSite))
    {
        *ppvObj = this;
    }
    else if (_punkInner)
    {
        return _punkInner->QueryInterface(riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//*** CTrayBandSite::IBandSite::* {

HRESULT CTrayBandSite::AddBand(IUnknown* punk)
{
    if (!_fDelayBootStuffHandled)
    {
        //
        // Tell the band to go into "delay init" mode.  When the tray
        // timer goes off we'll tell the band to finish up.  (See
        // BandSite_HandleDelayBootStuff).
        //
        IUnknown_Exec(punk, &CGID_DeskBand, DBID_DELAYINIT, 0, NULL, NULL);
    }

    return _pbsInner->AddBand(punk);
}

HRESULT CTrayBandSite::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    return _pbsInner->EnumBands(uBand, pdwBandID);
}


HRESULT _IsSameObject(IUnknown* punk1, IUnknown* punk2)
{
    if (punk1) {
        // this must never fail
        punk1->QueryInterface(IID_IUnknown, (LPVOID*)&punk1);
        ASSERT(punk1);
        punk1->Release();
            
    }
    
    if (punk2) {
        // this must never fail
        punk2->QueryInterface(IID_IUnknown, (LPVOID*)&punk2);
        ASSERT(punk2);
        punk2->Release();
    }
    
    return (punk1 == punk2) ? S_OK : S_FALSE;
}

BOOL CTrayBandSite::_IsTaskBand(IUnknown* punk)
{
    return _IsSameObject(punk, _punkTasks) == S_OK;
}

HRESULT CTrayBandSite::QueryBand( DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName) 
{
    IDeskBand *pstb = NULL;
    if (!ppstb)
        ppstb = &pstb;
    
    HRESULT hres = _pbsInner->QueryBand( dwBandID, ppstb, pdwState, pszName, cchName);

    if (SUCCEEDED(hres) && pdwState) {
        // if this is the taskbar band, return undeleteable
        if (_IsTaskBand(*ppstb)) {
            *pdwState |= BSSF_UNDELETEABLE;
        }
    }

    if ((ppstb == &pstb) && (NULL != pstb))
        pstb->Release();        // We have a ref on the object but the caller can't free it.
    
    return hres;
}


HRESULT CTrayBandSite::SetBandState( DWORD dwBandID, DWORD dwMask, DWORD dwState) 
{
    HRESULT hres =  _pbsInner->SetBandState( dwBandID, dwMask, dwState);
    
    return hres;
}


HRESULT CTrayBandSite::RemoveBand( DWORD dwBandID)
{
    return _pbsInner->RemoveBand( dwBandID);
}


HRESULT CTrayBandSite::GetBandObject(DWORD dwBandID, REFIID riid, LPVOID* ppvObj)
{
    return _pbsInner->GetBandObject(dwBandID, riid, ppvObj);
}

HRESULT CTrayBandSite::SetBandSiteInfo (const BANDSITEINFO * pbsinfo)
{
    return _pbsInner->SetBandSiteInfo(pbsinfo);
}

HRESULT CTrayBandSite::GetBandSiteInfo (BANDSITEINFO * pbsinfo)
{
    return _pbsInner->GetBandSiteInfo(pbsinfo);
}

// }

//*** CTrayBandSite::IBandSiteHelper::* {

//***   LoadFromStreamBS -- special OLFS
// DESCRIPTION
//  like OLFS, but special handling for TrayBand (which can't be CCI'ed).
//  also, takes an IUnknown not an IPersistStream.

HRESULT CTrayBandSite::LoadFromStreamBS(IStream *pstm, REFIID riid, LPVOID *ppv)
{
    static const LARGE_INTEGER c_li0 = { 0, 0 };
    HRESULT hres;
    ULONG cbRead;
    ULARGE_INTEGER liCur;
    CLSID clsid;

    // save current position
    hres = pstm->Seek(c_li0, STREAM_SEEK_CUR, &liCur);
    if (FAILED(hres))
        return hres;

    hres = pstm->Read(&clsid, SIZEOF(clsid), &cbRead);
    if (FAILED(hres))
        return hres;
    if (cbRead != SIZEOF(clsid))
        return E_FAIL;

    if (IsEqualGUID(clsid, CLSID_TaskBand)) {
        // special case

        DebugMsg(DM_PERSIST, TEXT("ctbs.bsolfs: intercept!"));

        // CoCreateInstance (sort of...)
        _punkTasks = Tasks_CreateInstance();
        ASSERT(_punkTasks != NULL);
        ASSERT(_IsTaskBand(_punkTasks));
        hres = _punkTasks->QueryInterface(riid, ppv);
        if (FAILED(hres))
            goto Lerr1;

        // Load
        hres = PersistStreamLoad(pstm, _punkTasks);
        if (FAILED(hres))
            goto Lerr1;

        // that's all!
#ifdef DEBUG
        if (hres != S_OK)
            DebugMsg(DM_WARNING, TEXT("ctbs.bsolfs: hres=%x (!=S_OK)"), hres);
#endif
        // tell BandSite we handled it
        hres = S_OK;

    Lerr1:
        // dtor does _punkTasks->Release();
  //Lerr0:
        return hres;
    }
    else {
        // not special case

        // restore position
        LARGE_INTEGER liTmp = { 0, 0 };

        liTmp.LowPart = liCur.LowPart;
        ASSERT(liCur.HighPart == 0);
        hres = pstm->Seek(liTmp, STREAM_SEEK_SET, NULL);
        if (FAILED(hres))
            return hres;

        // call the standard guy
        return OleLoadFromStream(pstm, riid, ppv);
    }
}

HRESULT CTrayBandSite::SaveToStreamBS(IUnknown *punk, IStream *pstm)
{
    HRESULT hres;

    if (_IsTaskBand(punk)) {
        ULONG cbRead;

        hres = pstm->Write(&CLSID_TaskBand, SIZEOF(CLSID_TaskBand), &cbRead);
        if (SUCCEEDED(hres) && cbRead != SIZEOF(CLSID_TaskBand))
            hres = E_FAIL;
        return hres;
    }
    else {
        IPersistStream *ppstm;

        hres = punk->QueryInterface(IID_IPersistStream, (LPVOID*)&ppstm);
        if (SUCCEEDED(hres)) {
            hres = OleSaveToStream(ppstm, pstm);
            ppstm->Release();
        }
        return hres;
    }
}

// }

// }


HRESULT BandSite_AddRequiredBands(CTrayBandSite* pbs)
{    
    IUnknown *punk = Tasks_CreateInstance();
    
    if (g_ts.fCoolTaskbar) {

        pbs->AddBand(punk);
        pbs->_punkTasks = punk;

#ifdef DEBUG
        static int fInit = FALSE;

        if (fInit) {
            HKEY hkey;
            TCHAR szID[160];  // enough for ID
            if (RegCreateKey(g_hkeyExplorer, TEXT("Taskbar\\Bands"), &hkey) == ERROR_SUCCESS) {

                // add all the reg supplied bands
                for (int iKey = 0; 
                     RegEnumKey(hkey, iKey, szID, SIZEOF(szID))==ERROR_SUCCESS;
                     iKey++)
                {
                    CLSID clsid;
                    if (SUCCEEDED(SHCLSIDFromString(szID, &clsid))) {

                        if (SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&punk))) {
                            pbs->AddBand(punk);
                            punk->Release();
                        }

                    }
                }

                RegCloseKey(hkey);
            }
        }
#endif
        
    } else {
        IObjectWithSite *pitb;
        CTaskBar *pow = new CTaskBar();
        if (pow) {
            punk->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pitb);
            if (pitb) {
                pitb->SetSite(SAFECAST(pow, IOleWindow*));
            }
            pow->Release();

        }
        punk->Release();
        g_ts.hwndRebar = g_ts.hwndView;
    }
         
    return S_OK;
}

BOOL CTrayBandSite::_IsTaskBandId(DWORD idBand)
{
    IDeskBand *pstb;
    BOOL fRet = FALSE;

    if (SUCCEEDED(QueryBand(idBand, &pstb, NULL, NULL, 0))) {
        if (_IsTaskBand(pstb)) {
            fRet = TRUE;
        }
        pstb->Release();
    }
    return fRet;
}

//***   BandSite_CheckBands -- see if minimal set of bands are there
// ENTRY/EXIT
//  hres    returns S_OK if bands are present.
// DESCRIPTION
//  'minimal' means TaskBand.
HRESULT BandSite_CheckBands(CTrayBandSite* pbs)
{
    int cBand, iBand;
    DWORD idBand;

    if (!g_ts.fCoolTaskbar) {
        // Shell restriction ClassicShell is enabled so no bands.
        return S_FALSE;
    }
    
    cBand = pbs->EnumBands((UINT)-1, NULL); 
    for (iBand = 0; iBand < cBand; ++iBand) {
        if (EVAL(SUCCEEDED(pbs->EnumBands(iBand, &idBand)))) {
            if (pbs->_IsTaskBandId(idBand))
                return S_OK;
        }
    }

    return S_FALSE;
}

void BandSite_Initialize(IBandSite* pbs)
{
    HWND hwnd = v_hwndTray;
    // should we use coolbar?
    
    if (g_ts.fCoolTaskbar) {
        IDeskBarClient* pdbc;
        CTaskBar *pow = new CTaskBar();
        
        if (pow) {
            pbs->QueryInterface(IID_IDeskBarClient, (LPVOID*)&pdbc);
            if (pdbc) {
                // we need to set a dummy tray IOleWindow
                pdbc->SetDeskBarSite(SAFECAST(pow, IOleWindow*));
                
                pdbc->GetWindow(&hwnd);

                pdbc->Release();
            }
            pow->Release();
        }
    }
    
    g_ts.hwndRebar = hwnd;
    
}

IContextMenu* CTrayBandSite::GetContextMenu()
{
    if (!_pcm) {
        CoCreateInstance(CLSID_BandSiteMenu, NULL,CLSCTX_INPROC_SERVER, 
                         IID_IContextMenu, (LPVOID*)&_pcm);
        if (_pcm) {
            IShellService* pss;

            _pcm->QueryInterface(IID_IShellService, (LPVOID*)&pss);
            if (pss) {
                pss->SetOwner((IBandSite*)this);
                pss->Release();
            }
        }
    }
    if (_pcm)
        _pcm->AddRef();
    
    return _pcm;
}

void BandSite_AddMenus(IUnknown* punk, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast)
{
    CTrayBandSite* ptbs = IUnknownToCTrayBandSite(punk);
    
    IContextMenu* pcm = ptbs->GetContextMenu();

    if (!pcm) 
        return;
                  
    pcm->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, 0);
    pcm->Release();

}

void BandSite_HandleMenuCommand(IUnknown* punk, UINT idCmd)
{
    CTrayBandSite* ptbs = IUnknownToCTrayBandSite(punk);
    
    IContextMenu* pcm = ptbs->GetContextMenu();
    
    if (!pcm)
        return;
    
    CMINVOKECOMMANDINFOEX ici = {
        SIZEOF(CMINVOKECOMMANDINFOEX),
        0L,
        NULL,
        (LPSTR)MAKEINTRESOURCE(idCmd),
        NULL, NULL,
        SW_NORMAL,
    };

    pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
    pcm->Release();
}

void CTrayBandSite::SetInner(IUnknown* punk)
{
    _punkInner = punk;
    
    _punkInner->QueryInterface(IID_IBandSite, (LPVOID*)&_pbsInner);
    Release();
    
    ASSERT(_pbsInner);
    
}

STDAPI_(IUnknown*) Tray_CreateView()
{
    HWND hwndParent = v_hwndTray;
    IUnknown *punk;
    HRESULT hres;

    if (SHRestricted(REST_CLASSICSHELL))
        g_ts.fCoolTaskbar = FALSE;
    else
    {
        g_ts.fCoolTaskbar = TRUE;
    }
    
    // aggregate a TrayBandSite (from a RebarBandSite)
    CTrayBandSite *ptbs = new CTrayBandSite;
    ASSERT(ptbs != NULL);
    hres = CoCreateInstance(CLSID_RebarBandSite, (IBandSite*)ptbs, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&punk);
    if (!punk)
    {
        // where is my tray?!?
        TraceMsg(TF_ERROR, "(Tray) Could not CoCreate CLSID_RebarBandSite.  Where is my tray?!?");
        return NULL;
    }
    
    ptbs->SetInner(punk);    // paired w/ Release in outer (TBS::Release)
    

    BandSite_Initialize(ptbs);
    
    return (IBandSite*)ptbs;
}

STDAPI Tray_SaveView(IUnknown *pbs)
{
    HRESULT hres;
    IStream *pstm;

    if (g_ts.fCoolTaskbar) {
        hres = E_FAIL;

        pstm = GetDesktopViewStream(STGM_WRITE, c_szTaskbar);
        if (pstm) {
            hres = PersistStreamSave(pstm, TRUE, pbs);
            pstm->Release();
        }
        TraceMsg(DM_SHUTDOWN, "t_sv: hr=%x", hres);
    }
    else {
        // Shell restriction for no cool task bar (Win95 ClassicShell)
        TraceMsg(DM_SHUTDOWN, "t_sv: !fCoolTaskbar");
        hres = S_OK; 
    }

    return hres;
}

BOOL CTrayBandSite::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if (!_hwnd) {
        // BUGBUG should use IUnknown_GetWindow
        IOleWindow *pow;
        QueryInterface(IID_IOleWindow, (LPVOID*)&pow);
        if (pow) {
            pow->GetWindow(&_hwnd);
            pow->Release();
        }
    }

    switch (uMsg) {
        
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
            case NM_NCHITTEST:
            {
                NMMOUSE *pnm = (LPNMMOUSE)lParam;
                if (_hwnd == pnm->hdr.hwndFrom) {
                    if (pnm->dwHitInfo == RBHT_CLIENT || pnm->dwItemSpec == -1)
                        if (plres)
                            *plres = HTTRANSPARENT;
                    return TRUE;
                }
                break;
            }

            case RBN_CHILDSIZE : 
            {
                // We want the running task list to always line up with the Start button.
                // The deal is, the Start button is exactly even with the top of the 1st
                // rebar band (so we want zero additional space on the first row).  But,
                // on any row but the top row, we want to line up with the gripper, not
                // the top of the band (gripper is 2 pixels (g_cyEdge?) below the top).
                // Ideally, the rebar would include a g_cyEdge border between each row,
                // not just its top edge and the first row.

                NMREBARCHILDSIZE *pnm = (NMREBARCHILDSIZE*)lParam;

                if (!_IsTaskBandId(pnm->wID))
                    break;

                int cy = pnm->rcChild.top - pnm->rcBand.top;

// But that pushes us below the screen... and we can't simply increase band size...
#if 0
                if (pnm->rcBand.top >= g_cyEdge) // If this band is not the top band
                    cy -= g_cyEdge;              // move us 1 edgewidth down from the top
#endif
                // Make child always start at top of rebar
                pnm->rcChild.bottom -= cy;
                pnm->rcChild.top -= cy;

                if (_fLoaded)
                    Tray_AsyncSaveSettings();
                break;
            }

            case RBN_MINMAX:
                *plres = SHRestricted(REST_NOMOVINGBAND);
                return TRUE;
        }
        break;
    }
        
        
    IWinEventHandler *pweh;
    QueryInterface(IID_IWinEventHandler, (LPVOID*)&pweh);
    ASSERT(pweh); // Don't want aggregation to fail.
    if (pweh) {
        HRESULT hres = pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
        pweh->Release();

        return SUCCEEDED(hres);
    }
    
    return FALSE;
}

void CTrayBandSite::_BroadcastExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    // Broadcast an Exec to all child bands

    DWORD dwBandID;
    UINT uBand = 0;
    while (SUCCEEDED(EnumBands(uBand, &dwBandID)))
    {
        IOleCommandTarget* pct;
        if (SUCCEEDED(GetBandObject(dwBandID, IID_IOleCommandTarget, (void**)&pct)))
        {
            pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            pct->Release();
        }
        uBand++;
    }
}

void BandSite_HandleDelayBootStuff(IUnknown* punk)
{
    if (punk)
    {
        CTrayBandSite* pbs = IUnknownToCTrayBandSite(punk);
        pbs->_fDelayBootStuffHandled = TRUE;
        pbs->_BroadcastExec(&CGID_DeskBand, DBID_FINISHINIT, 0, NULL, NULL);
    }
}

// returns true or false whether it handled it
BOOL BandSite_HandleMessage(IUnknown *punk, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if (punk) 
    {
        CTrayBandSite* pbs = IUnknownToCTrayBandSite(punk);
        return pbs->HandleMessage(hwnd, uMsg, wParam, lParam, plres);
    }
    return FALSE;
} 

void BandSite_SetMode(IUnknown *punk, DWORD dwMode)
{
    IBandSite* pbs = (IBandSite*)punk;
    
    if (pbs) 
    {
        IDeskBarClient *pdbc;
        pbs->QueryInterface(IID_IDeskBarClient, (LPVOID*)&pdbc);
        if (pdbc) 
        {
            pdbc->SetModeDBC(dwMode);
            pdbc->Release();
        }
    }
} 

void BandSite_Update(IUnknown *punk)
{
    IBandSite* pbs = (IBandSite*)punk;
    if (pbs) 
    {
        IOleCommandTarget *pct;
        pbs->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pct);
        if (pct) 
        {
            pct->Exec(&CGID_DeskBand, DBID_BANDINFOCHANGED, 0, NULL, NULL);
            pct->Release();
        }
    }
} 

void BandSite_UIActivateDBC(IUnknown *punk, DWORD dwState)
{
    IBandSite* pbs = (IBandSite*)punk;
    if (pbs)
    {
        IDeskBarClient *pdbc;
        pbs->QueryInterface(IID_IDeskBarClient, (LPVOID*)&pdbc);
        if (pdbc)
        {
            pdbc->UIActivateDBC(dwState);
            pdbc->Release();
        }
    }
}

//***   PersistStreamLoad, PersistStreamSave
// NOTES
//  we don't insist on finding IPersistStream iface; absence of it is
//  assumed to mean there's nothing to init.
HRESULT PersistStreamLoad(IStream *pstm, IUnknown *punk)
{
    HRESULT hres;
    IPersistStream *pps;

    hres = punk->QueryInterface(IID_IPersistStream, (LPVOID*) &pps);
    if (FAILED(hres))
        return S_OK;    // n.b. S_OK not hres (don't insist on IID_IPS)

    hres = pps->Load(pstm);

    pps->Release();

    return hres;
}

HRESULT PersistStreamSave(IStream *pstm, BOOL fClearDirty, IUnknown *punk)
{
    HRESULT hres;
    IPersistStream *pps;

    hres = punk->QueryInterface(IID_IPersistStream, (LPVOID*) &pps);
    if (FAILED(hres))
        return S_OK;    // n.b. S_OK not hres (don't insist on IID_IPS)

    hres = pps->Save(pstm, fClearDirty);

    pps->Release();

    return hres;
}

// BUGBUG: these functions duplicated in shdocvw and browseui,
// should move to shlwapi.

HRESULT UnkTranslateAcceleratorIO(IUnknown* punk, LPMSG lpMsg)
{
    HRESULT hres;
    IInputObject *pio;

    hres = punk->QueryInterface(IID_IInputObject, (LPVOID*)&pio);
    if (SUCCEEDED(hres)) {
        hres = pio->TranslateAcceleratorIO(lpMsg);
        pio->Release();
    }
    return hres;
}

HRESULT UnkUIActivateIO(IUnknown *punkThis, BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hres = E_FAIL;

    if (punkThis != NULL) {
        IInputObject *pio;

        hres = punkThis->QueryInterface(IID_IInputObject, (LPVOID*)&pio);
        if (SUCCEEDED(hres)) {
            hres = pio->UIActivateIO(fActivate, lpMsg);
            pio->Release();
        }
    }

    if (FAILED(hres))
        TraceMsg(DM_FOCUS2, "uiaio(fActivate=%d) hres=%x (FAILED)", fActivate, hres);

    return hres;
}

HRESULT UnkOnFocusChangeIS(IUnknown *punkThis, IUnknown *punkSrc, BOOL fSetFocus)
{
    HRESULT hres = E_FAIL;

    if (punkThis != NULL) {
        IInputObjectSite *pis;

        hres = punkThis->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pis);
        if (SUCCEEDED(hres)) {
            hres = pis->OnFocusChangeIS(punkSrc, fSetFocus);
            pis->Release();
        }
    }

    if (FAILED(hres))
        TraceMsg(DM_FOCUS, "ofcis(punk=%x fSetFocus=%d) hres=%x (FAILED)", punkSrc, fSetFocus, hres);

    return hres;
}

HRESULT BandSite_DragEnter(IUnknown* punk, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres;
    IDropTarget* pdt;
    
    hres = punk->QueryInterface(IID_IDropTarget, (LPVOID*)&pdt);
    if (pdt) {
        hres = pdt->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
        pdt->Release();
    }
    
    return hres;
}

HRESULT BandSite_DragOver(IUnknown* punk, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres;
    IDropTarget* pdt;
    
    hres = punk->QueryInterface(IID_IDropTarget, (LPVOID*)&pdt);
    if (pdt) {
        hres = pdt->DragOver(grfKeyState, pt, pdwEffect);
        pdt->Release();
    }
    
    return hres;
}

HRESULT BandSite_DragLeave(IUnknown* punk)
{
    HRESULT hres;
    IDropTarget* pdt;
    
    hres = punk->QueryInterface(IID_IDropTarget, (LPVOID*)&pdt);
    if (pdt) {
        hres = pdt->DragLeave();
        pdt->Release();
    }
    
    return hres;
}


HRESULT BandSite_SimulateDrop(IUnknown* punk, IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HRESULT hres;
    IDropTarget* pdt;
    
    hres = punk->QueryInterface(IID_IDropTarget, (LPVOID*)&pdt);
    if (pdt) {
        hres = pdt->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
        if (*pdwEffect) 
        {
            hres = pdt->Drop(pdtobj, grfKeyState, pt, pdwEffect);
        } 
        else 
        {
            pdt->DragLeave();
        }
        pdt->Release();
    }
    
    return hres;
} 

STDAPI_(LRESULT) Tray_OnMarshalBS(WPARAM wParam, LPARAM lParam)
{
    GUID *riid = (GUID *) wParam;
    HRESULT hr;
    IStream *pstm;

    // paired w/ matching Unmarshal in shdocvw (TM_MARSHALBS)
    hr = CoMarshalInterThreadInterfaceInStream(*riid, g_ts.ptbs, &pstm);
    ASSERT(SUCCEEDED(hr));

    return (LRESULT) pstm;
}

// {
//***   clone.cpp -- cloned (vs. imported) stuff
// DESCRIPTION
//  rather than make explorer link to (e.g.) shdocvw and thus load it,
//  we clone routines here.
//
//  of course if the amount of cloning grows much it might be worth biting
//  the bullet and doing a delayed load of it.

//***   shdocvw clones {

IStream *GetDesktopViewStream(DWORD grfMode, LPCTSTR pszName)
{
    HKEY hkStreams;

    ASSERT(g_hkeyExplorer);

    if (RegCreateKey(g_hkeyExplorer, TEXT("Streams"), &hkStreams) == ERROR_SUCCESS)
    {
        IStream *pstm = OpenRegStream(hkStreams, TEXT("Desktop"), pszName, grfMode);
        RegCloseKey(hkStreams);
        return pstm;
    }
    return NULL;
}

// BUGBUG adp 980123: [SH]IsTempDisplayMode cloned from shdocvw
// for sp1 we'd have to export it from shdocvw.  however for ie5
// we'd move it to shell32 and need a fwder which is silly.  so
// for now we just clone it.
//
// for ie5 it should live in shell32 and be imported from explorer
// and shdocvw.  this version has an extra TraceMsg so use it not
// shdocvw's.
//----------------------------------------------------------------------------

BOOL Reg_GetString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR psz, DWORD cb)
{
    BOOL fRet = FALSE;
    if (!g_fCleanBoot)
    {
        fRet = ERROR_SUCCESS == SHGetValue(hkey, pszSubKey, pszValue, NULL, psz, &cb);
    }
    return fRet;
}

STDAPI_(BOOL) SHIsTempDisplayMode()
{
    BOOL fTempMode = FALSE;

    // BUGBUG (scotth): the original code in explorer was #ifdef WINNT or MEMPHIS.
    //  We're just checking for NT right now.
    // adp: cloned from shdocvw/desktop2.cpp so we don't add a new
    // export just for sp1.  for ie5 we should put this in shell32
    // and have both explorer and shdocvw import it from there.
    if (g_bRunOnNT || g_bRunOnMemphis)
    {
        DEVMODE dm;

        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);

        if (EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &dm) &&
            dm.dmPelsWidth > 0 && dm.dmPelsHeight > 0)
        {
            HDC hdc = GetDC(NULL);
            int xres = GetDeviceCaps(hdc, HORZRES);
            int yres = GetDeviceCaps(hdc, VERTRES);
            ReleaseDC(NULL, hdc);

            if (xres != (int)dm.dmPelsWidth || yres != (int)dm.dmPelsHeight)
                fTempMode = TRUE;
        }
    }
    else
    {
        TCHAR ach[80];

        if (Reg_GetString(HKEY_CURRENT_CONFIG, REGSTR_PATH_DISPLAYSETTINGS, REGSTR_VAL_RESOLUTION, ach, SIZEOF(ach)))
        {
            int xres = StrToInt(ach);
            HDC hdc = GetDC(NULL);

            if ((GetDeviceCaps(hdc, CAPS1) & C1_REINIT_ABLE) &&
                (xres > 0) && (xres != GetDeviceCaps(hdc, HORZRES))) {
                fTempMode = TRUE;
            }
            ReleaseDC(NULL, hdc);
        }

    }

#ifdef DEBUG
    // if somebody's calling us they're probably doing something
    // semi-weird when we return TRUE
    if (fTempMode)
        TraceMsg(DM_TRACE, "SHitdm: SHIsTempDisplayMode ret=1 (!)");
#endif
    return fTempMode;
}

// }


void BandSite_Load()
{
    CTrayBandSite* ptbs = IUnknownToCTrayBandSite(g_ts.ptbs);
    HRESULT hres = E_FAIL;
    
    if (g_ts.fCoolTaskbar) {
        IStream *pstm;

        // 1st, try persisted state
        pstm = GetDesktopViewStream(STGM_READ, c_szTaskbar);
        if (pstm) {
            hres = PersistStreamLoad(pstm, (IBandSite*)ptbs);
            pstm->Release();
        }

        // 2nd, if there is none (or if version mismatch or other failure),
        // try settings from setup
        // BUGBUG n.b. this works fine for ie4 where we have no old toolbars,
        // but for future releases we'll need some kind of merging scheme,
        // so we probably want to change this after ie4-beta-1.
        if (FAILED(hres)) {
            // n.b. HKLM not HKCU
            // like GetDesktopViewStream but for HKLM
            pstm = OpenRegStream(HKEY_LOCAL_MACHINE,
                REGSTR_PATH_EXPLORER TEXT("\\Streams\\Desktop"),
                TEXT("Default Taskbar"), STGM_READ);
            if (pstm) {
                hres = PersistStreamLoad(pstm, (IBandSite *)ptbs);
                pstm->Release();
            }
        }
    }

    // o.w., throw up our hands and force some hard-coded defaults
    // this is needed for a) unexpected failures; b) debug bootstrap;
    // and c) people who don't have g_ts.fCoolTaskbar set (old mode)
    if (FAILED(hres) || BandSite_CheckBands(ptbs) != S_OK) {
        // but for debug, need a way to bootstrap the entire process
        // failiure is normal case if REST_CLASSICSHELL is set so
        // change DF_ERROR to TF_WARNING...
        TraceMsg(TF_WARNING, "tcv: load failed, reverting to last-chance defaults");

        //
        // note that for the CheckBands case, we're assuming that
        // a) AddBands adds only the missing guys (for now there's
        // only 1 [TaskBand] so we're ok); and b) AddBands doesn't
        // create dups if only some are missing (again for now there's
        // only 1 so no pblm)
        BandSite_AddRequiredBands(ptbs);
    }
    ptbs->SetLoaded(TRUE);
}
                         

// }

// }
