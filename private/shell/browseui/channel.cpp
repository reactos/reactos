#include "priv.h"
#ifdef ENABLE_CHANNELS
#include "channel.h"
#include "bands.h"
#include "isfband.h"
#include "itbar.h"
#include "qlink.h"
#define WANT_CBANDSITE_CLASS
#include "bandsite.h"
#include "resource.h"
#include "deskbar.h"
#include "../lib/dpastuff.h"

#include "dbapp.h"
#include "chanbar.h"

#include "subsmgr.h"
#include "chanmgr.h"
#include "chanmgrp.h"

#include "mluisupp.h"

void FrameTrack(HDC hdc, LPRECT prc, UINT uFlags);

HRESULT Channel_GetFolder(LPTSTR pszPath, int cchPath)
{
    TCHAR szChannel[MAX_PATH];
    TCHAR szFav[MAX_PATH];
    ULONG cbChannel = sizeof(szChannel);
    
    if (SHGetSpecialFolderPath(NULL, szFav, CSIDL_FAVORITES, TRUE))
    {
        //
        // Get the potentially localized name of the Channel folder from the
        // registry if it is there.  Otherwise just read it from the resource.
        // Then tack this on the favorites path.
        //

        if (ERROR_SUCCESS != SHRegGetUSValue(L"Software\\Microsoft\\Windows\\CurrentVersion",
                                             L"ChannelFolderName", NULL, (void*)szChannel,
                                             &cbChannel, TRUE, NULL, 0))
        {
            MLLoadString(IDS_CHANNEL, szChannel, ARRAYSIZE(szChannel));
        }

        PathCombine(pszPath, szFav, szChannel);

        //
        // For IE5+ use the channels dir if it exists - else use favorites
        //
        if (!PathFileExists(pszPath))
            StrCpyN(pszPath, szFav, cchPath);

        return S_OK;
    }    
    
    return E_FAIL;
}

LPITEMIDLIST Channel_GetFolderPidl()
{
    LPITEMIDLIST pidl = NULL;
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(Channel_GetFolder(szPath, ARRAYSIZE(szPath))))
    {
        pidl = ILCreateFromPath(szPath);
        if (!pidl && CreateDirectory(szPath, NULL)) {
            pidl = ILCreateFromPath(szPath);
        }
    }
    return pidl;
}



HRESULT ChannelBand_CreateInstance(IUnknown** ppunk)
{
    LPITEMIDLIST pidl = Channel_GetFolderPidl();
    if (pidl)
    {
        CISFBand* pb = CISFBand_CreateEx(NULL, pidl);

        ILFree(pidl);

        if (pb)
        {
            pb->SetCascade(TRUE);

            *ppunk = SAFECAST(pb, IContextMenu*);
            return S_OK;
        }
    }

    *ppunk = NULL;
    return E_FAIL;
}

//
// Navigates the left browser pane to the channels directory.
//
void NavigateBrowserBarToChannels(IWebBrowser2* pwb)
{
    ASSERT(pwb);

    IChannelMgrPriv* pIChannelMgrPriv;

    if (SUCCEEDED(CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IChannelMgrPriv, (void**)&pIChannelMgrPriv)))
    {
        ASSERT(pIChannelMgrPriv);

        LPITEMIDLIST pidl;

        if (SUCCEEDED(pIChannelMgrPriv->GetChannelFolder(&pidl, IChannelMgrPriv::CF_CHANNEL)))
        {
            ASSERT(pidl);

            VARIANT varPath;

            if (InitVariantFromIDList(&varPath, pidl))
            {
                VARIANT varFlags;

                varFlags.vt   = VT_I4;
                varFlags.lVal = navBrowserBar;

                pwb->Navigate2(&varPath, &varFlags, PVAREMPTY, PVAREMPTY, PVAREMPTY);

                VariantClear(&varPath);
            }

            ILFree(pidl);
        }

        pIChannelMgrPriv->Release();
    }

    return;
}


STDAPI NavigateToPIDL(IWebBrowser2* pwb, LPCITEMIDLIST pidl)
{
    ASSERT(pwb);
    ASSERT(pidl);

    HRESULT hr;

    VARIANT varThePidl;

    if (InitVariantFromIDList(&varThePidl, pidl))
    {
        hr = pwb->Navigate2(&varThePidl, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);

        VariantClear(&varThePidl);       // Needed to free the copy of the PIDL in varThePidl.
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//
// Implements the IE4 channel quick launch shell control file functionality.
// This gets called from shdoc401 on pre-NT5 platforms and from shell32 on
// Nt5 or greater.
//
HRESULT Channel_QuickLaunch(void)
{
    HRESULT hr;

    IWebBrowser2* pIWebBrowser2;

    hr = Channels_OpenBrowser(&pIWebBrowser2, FALSE);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIWebBrowser2);

        NavigateBrowserBarToChannels(pIWebBrowser2);

        LPITEMIDLIST pidl;
        TCHAR szURL[MAX_URL_STRING] = TEXT("");

        GetFirstUrl(szURL, SIZEOF(szURL));

        if (szURL[0])
        {
            hr = IECreateFromPath(szURL, &pidl);

            if (SUCCEEDED(hr))
            {
                ASSERT(pidl);

                hr = NavigateToPIDL(pIWebBrowser2, pidl);

                ILFree(pidl);
            }
        }
        else
        {
            hr = E_FAIL;
        }

        pIWebBrowser2->Release();
    }

    return hr;
}



/////////////////////////////////////
////// Browser only channel band support


// the CProxyWin95Desktop class implements an OleWindow 
// to represent the win95 desktop
// the browseronly channel band will use this as its host


class CProxyWin95Desktop : 
   public IOleWindow
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    CProxyWin95Desktop(HWND hwnd);
    
protected:
    
    UINT _cRef;
    HWND _hwnd;
};

CProxyWin95Desktop::CProxyWin95Desktop(HWND hwnd) : _cRef(1), _hwnd(hwnd)
{
}

ULONG CProxyWin95Desktop::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CProxyWin95Desktop::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CProxyWin95Desktop::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, SID_SShellDesktop)  // private hack for deskbar.cpp
       ) {
        *ppvObj = SAFECAST(this, IOleWindow*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

HRESULT CProxyWin95Desktop::GetWindow(HWND * lphwnd) 
{
    *lphwnd = _hwnd; 
    if (_hwnd)
        return S_OK; 
    return E_FAIL;
}

void Channels_InitState(IUnknown* punkBar)
{
    // initialize properties
    CDeskBarPropertyBag* ppb = new CDeskBarPropertyBag();
    if (ppb) {
        // Get the default rc
        CISSTRUCT cis;
        DWORD     cbSize = sizeof(CISSTRUCT);
        RECT     *prc = &cis.rc;

        cis.iVer = 1;  // set version number to 1
        SystemParametersInfoA(SPI_GETWORKAREA, 0, prc, 0);
        prc->bottom = min(prc->bottom - 20, prc->top + 12*38 + 28); // 12 icons + caption

        if(IS_BIDI_LOCALIZED_SYSTEM())
        {
            prc->right = prc->left + 90;
            OffsetRect(prc, 20, 10);
        }
        else
        {
            prc->left = prc->right - 90;
            OffsetRect(prc, -20, 10);
        }

        // query registry for persisted state
        SHRegGetUSValue(SZ_REGKEY_CHANBAR, SZ_REGVALUE_CHANBAR, NULL, 
                        (LPVOID)&cis, &cbSize, FALSE, (LPVOID)&cis, cbSize);

        // set ppb by prc
        ppb->SetDataDWORD(PROPDATA_MODE, WBM_FLOATING | WBMF_BROWSER);
        ppb->SetDataDWORD(PROPDATA_X, prc->left);
        ppb->SetDataDWORD(PROPDATA_Y, prc->top);
        ppb->SetDataDWORD(PROPDATA_CX, RECTWIDTH(*prc));
        ppb->SetDataDWORD(PROPDATA_CY, RECTHEIGHT(*prc));
        SHLoadFromPropertyBag(punkBar, ppb);
        ppb->Release();
    }
}

void Channels_MainLoop(IDockingWindow *pdw)
{
    MSG msg;
    HWND hwnd;
    // loop while the window exists
    do {
        GetMessage(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        pdw->GetWindow(&hwnd);
    } while (hwnd);
}

void Channels_SetBandInfoSFB(IUnknown* punkBand)
{
    BANDINFOSFB bi;

    // Set band startup conditions
    bi.dwMask = ISFB_MASK_STATE | ISFB_MASK_VIEWMODE;
    bi.dwStateMask = ISFB_STATE_CHANNELBAR | ISFB_STATE_NOSHOWTEXT;
    bi.dwState = ISFB_STATE_CHANNELBAR | ISFB_STATE_NOSHOWTEXT;
    bi.wViewMode = ISFBVIEWMODE_LOGOS;

    IUnknown_SetBandInfoSFB(punkBand, &bi);
}

// from isfband.cpp
extern IDeskBand * ChannelBand_Create( LPCITEMIDLIST pidl );

// this does the desktop channel in browser only mode
void DesktopChannel()
{
    _InitComCtl32();

    // Don't show channel bar:
    //      *. in integrated mode with active desktop turned on, or
    //      *. NoChannelUI restriction is set, or
    //      *. there is already one on desktop
    
    if (SHRestricted2(REST_NoChannelUI, NULL, 0))
        return;
        
    if (WhichPlatform() == PLATFORM_INTEGRATED) {
        SHELLSTATE  ss = { 0 };

        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE); // Get the setting
        if (ss.fDesktopHTML) {
            return;
        }
    }
        
    if (FindWindowEx(GetShellWindow(), NULL, TEXT("BaseBar"), TEXT("ChanApp")) ||
        FindWindowEx(NULL, NULL, TEXT("BaseBar"), TEXT("ChanApp"))) // can be a toplevel window
        return;

    LPITEMIDLIST pidl = Channel_GetFolderPidl();
    if (pidl) {
        IUnknown* punk = (IUnknown *) ChannelBand_Create( pidl );
        if (punk) {

            Channels_SetBandInfoSFB(punk);

            IUnknown* punkBar;
            IUnknown* punkBandSite;

            HRESULT hres = ChannelDeskBarApp_Create(&punkBar, &punkBandSite);
            if (SUCCEEDED(hres)) {
                CProxyWin95Desktop* pow = new CProxyWin95Desktop(GetShellWindow());
                if (pow) {
                    IBandSite* pbs;
                    IDockingWindow* pdw;

                    Channels_InitState(punkBar);

                    // these are always our own guys, so these QI's MUST succeed if the creation succeeded
                    punkBandSite->QueryInterface(IID_IBandSite, (LPVOID*)&pbs);
                    punkBar->QueryInterface(IID_IDockingWindow, (LPVOID*)&pdw);
                    ASSERT(pbs && pdw);

                    hres = pbs->AddBand(punk);
                    IUnknown_SetSite(pdw, (IOleWindow*)pow);

                    pbs->SetBandState((DWORD)-1, BSSF_NOTITLE, BSSF_NOTITLE);

                    pdw->ShowDW(TRUE);

                    Channels_MainLoop(pdw);

                    pdw->Release();
                    pbs->Release();
                    pow->Release();
                }
                punkBar->Release();
                punkBandSite->Release();
            }

            punk->Release();
        }
        ILFree(pidl);
    }
}

HRESULT Channels_OpenBrowser(IWebBrowser2 **ppwb, BOOL fInPlace)
{
    HRESULT hres;
    IWebBrowser2* pwb;

    if (fInPlace) {
        ASSERT(ppwb && *ppwb != NULL);
        pwb = *ppwb;
        hres = S_OK;
    }
    else {
#ifndef UNIX
        hres = CoCreateInstance(CLSID_InternetExplorer, NULL,
                                    CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void **)&pwb);
#else
        hres = CoCreateInternetExplorer( IID_IWebBrowser2,
                                         CLSCTX_LOCAL_SERVER,
                                         (LPVOID*) &pwb );
#endif
    }
    
    if (SUCCEEDED(hres))
    {
        SA_BSTRGUID  strGuid;
        VARIANT      vaGuid;

        // Don't special case full-screen mode for channels post IE4.  Use the
        // browser's full screen setting.
        // 
        //BOOL fTheater = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Channels"),
        BOOL fTheater = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                                            TEXT("FullScreen"), FALSE, FALSE);
        pwb->put_TheaterMode( fTheater ? VARIANT_TRUE : VARIANT_FALSE);
        pwb->put_Visible(VARIANT_TRUE);


        if (!SHRestricted2(REST_NoChannelUI, NULL, 0))
        {
#ifdef ENABLE_CHANNELPANE
            StringFromGUID2(CLSID_ChannelBand, strGuid.wsz, ARRAYSIZE(strGuid.wsz));
#else
            StringFromGUID2(CLSID_FavBand, strGuid.wsz, ARRAYSIZE(strGuid.wsz));
#endif

            strGuid.cb = lstrlenW(strGuid.wsz) * SIZEOF(WCHAR);

            vaGuid.vt = VT_BSTR;
            vaGuid.bstrVal = strGuid.wsz;

            pwb->ShowBrowserBar(&vaGuid, PVAREMPTY, PVAREMPTY);
        }
        
        // don't release, we're going to return pwb.
    }
    
    if (ppwb)
        *ppwb = pwb;
    else if (pwb)
        pwb->Release();
    
    return hres;
}

BOOL GetFirstUrl(TCHAR szURL[], DWORD cb)
{
    //BOOL fFirst = FALSE;
    DWORD dwType;

    // Don't special case first channel click post IE4.
    /*if (SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), TEXT("ChannelsFirstURL"), 
            &dwType, szURL, &cb, FALSE, NULL, 0) == ERROR_SUCCESS) 
    {        
        HUSKEY hUSKey;
                
        if (SHRegOpenUSKey(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), KEY_WRITE, NULL, 
                &hUSKey, FALSE) == ERROR_SUCCESS)
        {
            SHRegDeleteUSValue(hUSKey, TEXT("ChannelsFirstURL"), SHREGDEL_HKCU);
            SHRegCloseUSKey(hUSKey);
        }
        fFirst = TRUE;
    } 
    else if (SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"), TEXT("ChannelsURL"), 
        &dwType, szURL, &cb, FALSE, NULL, 0) == ERROR_SUCCESS)
    {
        // nothing
    }
    else
    {
        // BUGBUG if code is ever revived, this res:// needs to be
        // accessed through MLBuildResURLWrap because of pluggable UI
        szURL = lstrcpy(szURL, TEXT("res://ie4tour.dll/channels.htm"));
    }*/

    SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                    TEXT("ChannelsURL"), &dwType, szURL, &cb, FALSE, NULL, 0);
    return FALSE; 
}


//////////////////////////////////////////////////
//
// ChannelBand
//
// This is a special band that only looks at the channels folder.
// It overrides several functions from CISFBand.
//

#undef  SUPERCLASS
#define SUPERCLASS CISFBand

class ChannelBand : public SUPERCLASS
{
public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual HRESULT OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);

protected:
    ChannelBand();
    friend IDeskBand * ChannelBand_Create(LPCITEMIDLIST pidl);

    virtual HRESULT _LoadOrderStream();

    virtual HWND _CreatePager(HWND hwndParent);

    virtual LRESULT _OnCustomDraw(NMCUSTOMDRAW* pnmcd);

    virtual void _Dropped(int nIndex, BOOL fDroppedOnSource);
    virtual HRESULT _AfterLoad();
    virtual void _OnDragBegin(int iItem, DWORD dwPreferedEffect);
} ;


#define COLORBK     RGB(0,0,0)
ChannelBand::ChannelBand() :
    SUPERCLASS()
{
    _lEvents |= SHCNE_EXTENDED_EVENT;
    _dwStyle |= TBSTYLE_CUSTOMERASE;

    _crBkgnd = COLORBK;     // i see a channelband and i want to paint it black
    _fHaveBkColor = TRUE;
}

HWND ChannelBand::_CreatePager(HWND hwndParent)
{
    // we do want a pager for this band, so
    // override isfband's implementation w/ grandpa's
    return CSFToolbar::_CreatePager(hwndParent);
}

HRESULT ChannelBand::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IContextMenu))
        return E_NOINTERFACE;
    
    
    return SUPERCLASS::QueryInterface(riid, ppvObj);
}


IDeskBand * ChannelBand_Create(LPCITEMIDLIST pidlDefault)
{
    ChannelBand * pBand = NULL;
    LPITEMIDLIST pidl = NULL;

    if (!pidlDefault)
    {
        pidl = Channel_GetFolderPidl();
        pidlDefault = pidl;
    }
    if (EVAL(pidlDefault))
    {
        pBand = new ChannelBand;

        if (pBand)
        {
            if (FAILED(pBand->InitializeSFB(NULL, pidlDefault)))
            {
                ATOMICRELEASE(pBand);
            }
        }

        ILFree(pidl);
    }

    return pBand;
}

HRESULT ChannelBand::_AfterLoad()
{
    HRESULT hres = SUPERCLASS::_AfterLoad();

    _LoadOrderStream();

    return hres;
}

HRESULT ChannelBand::_LoadOrderStream()
{
    OrderList_Destroy(&_hdpaOrder);

    COrderList_GetOrderList(&_hdpaOrder, _pidl, _psf);
    return S_OK;
}


HRESULT ChannelBand::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = E_FAIL;

    switch (lEvent)
    {
    case SHCNE_EXTENDED_EVENT:
    {
        SHChangeMenuAsIDList UNALIGNED * pdwidl = (SHChangeMenuAsIDList UNALIGNED *)pidl1;
        if ( pdwidl->dwItem1 == SHCNEE_ORDERCHANGED )
        {
            if (SHChangeMenuWasSentByMe(this, pidl1))
            {
                // We sent this order change, ignore it
                TraceMsg(TF_BAND, "ChannelBand::OnChange SHCNEE_ORDERCHANGED skipped (we're source)");
                hres = S_OK;
            }
            else if (EVAL(pidl2) && _pidl)
            {
                if (ILIsEqual(_pidl, pidl2))
                {
                    TraceMsg(TF_BAND, "ChannelBand::OnChange SHCNEE_ORDERCHANGED accepted");

                    _LoadOrderStream();

                    if (_fShow)
                        _FillToolbar();

                    hres = S_OK;
                }
            }
            break;
        }
        // if it wasn't SHCNEE_ORDERCHANGED, then drop through to pass to the base class..
    }

    default:
        hres = SUPERCLASS::OnChange(lEvent, pidl1, pidl2);
        break;
    }

    return hres;
}

HRESULT ChannelBand::OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (_iDragSource >= 0)
    {
        return SUPERCLASS::OnDropDDT(pdt, pdtobj, pgrfKeyState, pt, pdwEffect);
    }
    else
    {
        // we don't call superclass in this case 'cuz we want to undo
        // it's "always use shortcut" override.
        //
        _fDropping = TRUE;
        return S_OK;
    }
}

LRESULT ChannelBand::_OnCustomDraw(NMCUSTOMDRAW* pnmcd)
{
    NMTBCUSTOMDRAW * ptbcd = (NMTBCUSTOMDRAW *)pnmcd;
    LRESULT lres;

    lres = SUPERCLASS::_OnCustomDraw(pnmcd);
        
    switch (pnmcd->dwDrawStage)
    {
    case CDDS_PREPAINT:
        lres |= CDRF_NOTIFYITEMDRAW;
        break;

    case CDDS_PREERASE:
        // Channel band has a darker background color
        {
            RECT rc;
            GetClientRect(_hwndTB, &rc);
            // BUGBUG perf: use SHFillRectClr not SetBk/ExtText/SetBk
            COLORREF old = SetBkColor(pnmcd->hdc, _crBkgnd);
            ExtTextOut(pnmcd->hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
            SetBkColor(pnmcd->hdc, old);
            lres = CDRF_SKIPDEFAULT;                
        }
        break;

    case CDDS_ITEMPREPAINT:
        // Channel band doesn't draw as buttons
        lres |= TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOMARK |
                CDRF_NOTIFYPOSTPAINT;
        break;

    case CDDS_ITEMPOSTPAINT:
        // Channel band draws the hot item (CDIS_HOT)
        //
        
        pnmcd->rc.top++;
        pnmcd->rc.left++;
        if (pnmcd->uItemState & CDIS_SELECTED)
            // Mark the selected item 
            FrameTrack(pnmcd->hdc,  &(pnmcd->rc), TRACKNOCHILD);                           
        else if (pnmcd->uItemState & CDIS_HOT)
            // Mark the hot item 
            FrameTrack(pnmcd->hdc,  &(pnmcd->rc), TRACKHOT);                           
        break;

    }
    
    return lres;
}

void ChannelBand::_Dropped(int nIndex, BOOL fDroppedOnSource)
{
    ASSERT(_fDropping);

    // I'm not changing this to match the other derivatives (ISFBand, mnfolder, quick links),
    // because this structure is slightly different
    _fDropped = TRUE;

    // Persist the new order out to the registry
    if (SUCCEEDED(COrderList_SetOrderList(_hdpa, _pidl, _psf)))
    {
        // Notify everyone that the order changed
        SHSendChangeMenuNotify(this, SHCNEE_ORDERCHANGED, 0, _pidl);
    }
}

void ChannelBand::_OnDragBegin(int iItem, DWORD dwPreferedEffect)
{
    //
    // Don't allow drag if REST_NoRemovingChannels is enabled.
    //

    if (!SHRestricted2(REST_NoRemovingChannels, NULL, 0))
        SUPERCLASS::_OnDragBegin(iItem, dwPreferedEffect);

    return;
}




#endif // ENABLE_CHANNELS
