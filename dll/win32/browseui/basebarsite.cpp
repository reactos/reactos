/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
Base bar that contains a vertical or horizontal explorer band. It also
provides resizing abilities.
*/

#include "precomp.h"

/*
TODO:
****When a new bar is added, resize correctly the band inside instead of keeping current size.
   *Translate close button label
  **Add owner draw for base bar -- hackplemented atm
  **Make label text in base bar always draw in black
 ***Set rebar band style flags accordingly to what band object asked.
 ***Set rebar style accordingly to direction
****This class should also manage desktop bands ? (another kind of explorer bands)
*/

class CBaseBarSite :
    public CWindowImpl<CBaseBarSite, CWindow, CControlWinTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
//    public IDockingWindowSite,
    public IInputObject,
    public IServiceProvider,
    public IWinEventHandler,
    public IInputObjectSite,
    public IDeskBarClient,
    public IOleCommandTarget,
    public IBandSite,
//    public IBandSiteHelper,
//    public IExplorerToolbar,
    public IPersistStream
{
private:
    class CBarInfo
    {
    public:
        CComPtr<IUnknown>                   fTheBar;
        CLSID                               fBarClass;              // class of active bar
        DWORD                               fBandID;

    };
    CBarInfo                                *fCurrentActiveBar;     //
//    HWND                                    fRebarWindow;           // rebar for top of window
    CComPtr<IUnknown>                       fDeskBarSite;
    DWORD                                   fNextBandID;
    HWND                                    toolbarWnd;
    HIMAGELIST                              toolImageList;
    BOOL                                    fVertical;
public:
    CBaseBarSite();
    ~CBaseBarSite();
    HRESULT Initialize(BOOL vert) { fVertical = vert; return S_OK; };
private:
    HRESULT InsertBar(IUnknown *newBar);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(
        HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IInputObjectSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // *** IDeskBarClient methods ***
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
        OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
        DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IBandSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD *pdwBandID);
    virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState,
        LPWSTR pszName, int cchName);
    virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
    virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // message handlers
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCustomDraw(LPNMCUSTOMDRAW pnmcd);

    // Helper functions
    HFONT GetTitleFont();
    HRESULT FindBandByGUID(REFIID pGuid, DWORD *pdwBandID);
    HRESULT ShowBand(DWORD dwBandID);
    HRESULT GetInternalBandInfo(UINT uBand, REBARBANDINFO *pBandInfo);
    HRESULT GetInternalBandInfo(UINT uBand, REBARBANDINFO *pBandInfo, DWORD fMask);


BEGIN_MSG_MAP(CBaseBarSite)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
END_MSG_MAP()

BEGIN_COM_MAP(CBaseBarSite)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
//    COM_INTERFACE_ENTRY_IID(IID_IDockingWindowSite, IDockingWindowSite)
    COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
    COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
    COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
    COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
    COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
    COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
//    COM_INTERFACE_ENTRY_IID(IID_IBandSiteHelper, IBandSiteHelper)
//    COM_INTERFACE_ENTRY_IID(IID_IExplorerToolbar, IExplorerToolbar)
    COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
    COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
END_COM_MAP()
};

CBaseBarSite::CBaseBarSite() : fVertical(TRUE)
{
    fCurrentActiveBar = NULL;
    fNextBandID = 1;
}

CBaseBarSite::~CBaseBarSite()
{
    TRACE("CBaseBarSite deleted\n");
}

HRESULT CBaseBarSite::InsertBar(IUnknown *newBar)
{
    CComPtr<IPersist>                       persist;
    CComPtr<IObjectWithSite>                site;
    CComPtr<IOleWindow>                     oleWindow;
    CComPtr<IDeskBand>                      deskBand;
    CBarInfo                                *newInfo;
    REBARBANDINFOW                          bandInfo;
    DESKBANDINFO                            deskBandInfo;
    DWORD                                   thisBandID;
    HRESULT                                 hResult;
    CLSID                                   tmp;

    hResult = newBar->QueryInterface(IID_PPV_ARG(IPersist, &persist));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = newBar->QueryInterface(IID_PPV_ARG(IObjectWithSite, &site));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = newBar->QueryInterface(IID_PPV_ARG(IOleWindow, &oleWindow));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = newBar->QueryInterface(IID_PPV_ARG(IDeskBand, &deskBand));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // Check if the GUID already exists
    hResult = persist->GetClassID(&tmp);
    if (!SUCCEEDED(hResult))
    {
        return E_INVALIDARG;
    }
    if (FindBandByGUID(tmp, &thisBandID) == S_OK)
    {
        return ShowBand(thisBandID);
    }

    hResult = site->SetSite(static_cast<IOleWindow *>(this));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    ATLTRY(newInfo = new CBarInfo);
    if (newInfo == NULL)
        return E_OUTOFMEMORY;

    // set new bar info
    thisBandID = fNextBandID++;
    newInfo->fTheBar = newBar;
    newInfo->fBandID = thisBandID;
    newInfo->fBarClass = tmp;

    // get band info
    deskBandInfo.dwMask = DBIM_MINSIZE | DBIM_ACTUAL | DBIM_TITLE | DBIM_BKCOLOR;
    deskBandInfo.wszTitle[0] = 0;
    hResult = deskBand->GetBandInfo(0, (fVertical) ? DBIF_VIEWMODE_VERTICAL : DBIF_VIEWMODE_NORMAL, &deskBandInfo);

    // insert band
    memset(&bandInfo, 0, sizeof(bandInfo));
    bandInfo.cbSize = sizeof(bandInfo);
    bandInfo.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_TEXT |
        RBBIM_LPARAM | RBBIM_ID;
    bandInfo.fStyle = RBBS_TOPALIGN | RBBS_VARIABLEHEIGHT | RBBS_NOGRIPPER;
    bandInfo.lpText = deskBandInfo.wszTitle;
    hResult = oleWindow->GetWindow(&bandInfo.hwndChild);
    /* It seems Windows XP doesn't take account of band minsize */
#if 0
    bandInfo.cxMinChild = 200; //deskBandInfo.ptMinSize.x;
    bandInfo.cyMinChild = 200; //deskBandInfo.ptMinSize.y;
#endif
    bandInfo.cx = 0;
    bandInfo.wID = thisBandID;
    bandInfo.cyChild = -1; //deskBandInfo.ptActual.y;
    bandInfo.cyMaxChild = 32000;
    bandInfo.cyIntegral = 1;
    bandInfo.cxIdeal = 0; //deskBandInfo.ptActual.x;
    bandInfo.lParam = reinterpret_cast<LPARAM>(newInfo);
    SendMessage(RB_INSERTBANDW, -1, reinterpret_cast<LPARAM>(&bandInfo));
    hResult = ShowBand(newInfo->fBandID);
    //fCurrentActiveBar = newInfo;
    return hResult;
 }

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (!fCurrentActiveBar)
        return S_OK;

    return IUnknown_UIActivateIO(fCurrentActiveBar->fTheBar, fActivate, lpMsg);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::HasFocusIO()
{
    if (!fCurrentActiveBar)
        return S_FALSE;

    return IUnknown_HasFocusIO(fCurrentActiveBar->fTheBar);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (!fCurrentActiveBar)
    {
        if (lpMsg)
        {
            TranslateMessage(lpMsg);
            DispatchMessage(lpMsg);
        }
        return S_OK;
    }

    return IUnknown_TranslateAcceleratorIO(fCurrentActiveBar->fTheBar, lpMsg);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    CComPtr<IServiceProvider>               serviceProvider;
    HRESULT                                 hResult;

    if (fDeskBarSite == NULL)
        return E_FAIL;
    hResult = fDeskBarSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &serviceProvider));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    // called for SID_STopLevelBrowser, IID_IBrowserService to find top level browser
    // called for SID_IWebBrowserApp, IID_IConnectionPointContainer
    // connection point called for DIID_DWebBrowserEvents2 to establish connection
    return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::OnWinEvent(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    CComPtr<IDeskBar>                       deskBar;
    CComPtr<IWinEventHandler>               winEventHandler;
    NMHDR                                   *notifyHeader;
    // RECT                                    newBounds;
    HRESULT                                 hResult;
    LRESULT                                 result;

    hResult = S_OK;
    if (uMsg == WM_NOTIFY)
    {
        notifyHeader = (NMHDR *)lParam;
        if (notifyHeader->hwndFrom == m_hWnd)
        {
            switch (notifyHeader->code)
            {
                case RBN_AUTOSIZE:
                    // For now, don't notify basebar we tried to resize ourselves, we don't
                    // get correct values at the moment.
#if 0
                    hResult = fDeskBarSite->QueryInterface(IID_PPV_ARG(IDeskBar, &deskBar));
                    GetClientRect(&newBounds);
                    hResult = deskBar->OnPosRectChangeDB(&newBounds);

#endif
                    break;
                case NM_CUSTOMDRAW:
                    result = OnCustomDraw((LPNMCUSTOMDRAW)lParam);
                    if (theResult)
                        *theResult = result;
                    return S_OK;
            }
        }
    }
    if (fCurrentActiveBar != NULL)
    {
        hResult = fCurrentActiveBar->fTheBar->QueryInterface(
            IID_PPV_ARG(IWinEventHandler, &winEventHandler));
        if (SUCCEEDED(hResult) && winEventHandler.p != NULL)
            hResult = winEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    }
    return hResult;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::IsWindowOwner(HWND hWnd)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus)
{
    // FIXME: should we directly pass-through, or advertise ourselves as focus owner ?
    return IUnknown_OnFocusChangeIS(fDeskBarSite, punkObj, fSetFocus);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetDeskBarSite(IUnknown *punkSite)
{
    CComPtr<IOleWindow>                     oleWindow;
    HWND                                    ownerWindow;
    HRESULT                                 hResult;
    DWORD                                   dwBandID;

    if (punkSite == NULL)
    {

        TRACE("Destroying site \n");
        /* Cleanup our bands */
        while(SUCCEEDED(EnumBands(-1, &dwBandID)) && dwBandID)
        {
            hResult = EnumBands(0, &dwBandID);
            if(FAILED_UNEXPECTEDLY(hResult))
                continue;
            RemoveBand(dwBandID);
        }
        fDeskBarSite = NULL;
    }
    else
    {
        TBBUTTON closeBtn;
        HBITMAP hBmp;

        hResult = punkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &oleWindow));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = punkSite->QueryInterface(IID_PPV_ARG(IUnknown, &fDeskBarSite));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = oleWindow->GetWindow(&ownerWindow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        DWORD dwStyle =  WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER |
             RBS_VARHEIGHT | RBS_REGISTERDROP | RBS_AUTOSIZE | RBS_VERTICALGRIPPER | RBS_DBLCLKTOGGLE |
             CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE;
        if (fVertical)
            dwStyle |= CCS_VERT;

        /* Create site window */
        HWND tmp = CreateWindowW(REBARCLASSNAMEW, NULL, dwStyle, 0, 0, 0, 0, ownerWindow, NULL,
                    _AtlBaseModule.GetModuleInstance(), NULL);

        /* Give window management to ATL */
        SubclassWindow(tmp);

        SendMessage(RB_SETTEXTCOLOR, 0, CLR_DEFAULT);
        SendMessage(RB_SETBKCOLOR, 0, CLR_DEFAULT);

        /* Create close toolbar and imagelist */
        toolbarWnd = CreateWindowW(TOOLBARCLASSNAMEW, NULL,
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS |
            CCS_NOMOVEY | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER
            , 0, 0, 0, 0, m_hWnd, NULL, _AtlBaseModule.GetModuleInstance(), NULL);

        toolImageList = ImageList_Create(13, 11, ILC_COLOR24 | ILC_MASK, 3, 0);

        hBmp = (HBITMAP)LoadImage(_AtlBaseModule.GetModuleInstance(),
            MAKEINTRESOURCE(IDB_BANDBUTTONS), IMAGE_BITMAP, 0, 0,
            LR_LOADTRANSPARENT);

        ImageList_AddMasked(toolImageList, hBmp, RGB(192, 192, 192));
        DeleteObject(hBmp);

        SendMessage(toolbarWnd, TB_SETIMAGELIST, 0, (LPARAM)toolImageList);

        /* Add button to toolbar */
        closeBtn.iBitmap = MAKELONG(1, 0);
        closeBtn.idCommand = IDM_BASEBAR_CLOSE;
        closeBtn.fsState = TBSTATE_ENABLED;
        closeBtn.fsStyle = BTNS_BUTTON;
        ZeroMemory(closeBtn.bReserved, sizeof(closeBtn.bReserved));
        closeBtn.dwData = 0;
        closeBtn.iString = (INT_PTR)L"Close";

        SendMessage(toolbarWnd, TB_INSERTBUTTON, 0, (LPARAM)&closeBtn);
        SendMessage(toolbarWnd, TB_SETMAXTEXTROWS, 0, 0);
        //SendMessage(toolbarWnd, TB_AUTOSIZE, 0, 0);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetModeDBC(DWORD dwMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::UIActivateDBC(DWORD dwState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetSize(DWORD dwWhich, LPRECT prc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
    {
        switch (nCmdID)
        {
            case 0:
                //update band info ?
            case 1:     // insert a new band
                if (V_VT(pvaIn) != VT_UNKNOWN)
                    return E_INVALIDARG;
                return InsertBar(V_UNKNOWN(pvaIn));
            case 0x17:
                // redim band
                break;
        }
    }
    return E_FAIL;
}

HRESULT CBaseBarSite::GetInternalBandInfo(UINT uBand, REBARBANDINFO *pBandInfo)
{
    if (!pBandInfo)
        return E_INVALIDARG;
    memset(pBandInfo, 0, sizeof(REBARBANDINFO));
    pBandInfo->cbSize = sizeof(REBARBANDINFO);
    pBandInfo->fMask = RBBIM_LPARAM | RBBIM_ID;

    // Grab our bandinfo from rebar control
    if (!SendMessage(RB_GETBANDINFO, uBand, reinterpret_cast<LPARAM>(pBandInfo)))
        return E_INVALIDARG;
    return S_OK;
}

HRESULT CBaseBarSite::GetInternalBandInfo(UINT uBand, REBARBANDINFO *pBandInfo, DWORD fMask)
{
    if (!pBandInfo)
        return E_INVALIDARG;
    pBandInfo->cbSize = sizeof(REBARBANDINFO);
    pBandInfo->fMask = fMask;

    // Grab our bandinfo from rebar control
    if (!SendMessage(RB_GETBANDINFO, uBand, reinterpret_cast<LPARAM>(pBandInfo)))
        return E_INVALIDARG;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::AddBand(IUnknown *punk)
{
    return InsertBar(punk);
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::EnumBands(UINT uBand, DWORD *pdwBandID)
{
    REBARBANDINFO bandInfo;

    if (pdwBandID == NULL)
        return E_INVALIDARG;
    if (uBand == 0xffffffff)
    {
        *pdwBandID = (DWORD)SendMessage(RB_GETBANDCOUNT, 0, 0);
        return S_OK;
    }
    if (!SUCCEEDED(GetInternalBandInfo(uBand, &bandInfo)))
        return E_INVALIDARG;
    *pdwBandID = bandInfo.wID;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::QueryBand(DWORD dwBandID, IDeskBand **ppstb,
    DWORD *pdwState, LPWSTR pszName, int cchName)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::RemoveBand(DWORD dwBandID)
{
    REBARBANDINFO                   bandInfo;
    HRESULT                         hr;
    CBarInfo                        *pInfo;
    CComPtr<IObjectWithSite>        pSite;
    CComPtr<IDeskBand>              pDockWnd;
    DWORD                           index;

    // Retrieve the right index of the coolbar knowing the id
    index = SendMessage(RB_IDTOINDEX, dwBandID, 0);
    if (index == 0xffffffff)
       return E_INVALIDARG;

    if (FAILED_UNEXPECTEDLY(GetInternalBandInfo(index, &bandInfo)))
        return E_INVALIDARG;

    pInfo = reinterpret_cast<CBarInfo*>(bandInfo.lParam);
    if (!pInfo)
        return E_INVALIDARG;

    hr = pInfo->fTheBar->QueryInterface(IID_PPV_ARG(IDeskBand, &pDockWnd));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        return E_NOINTERFACE;
    }
    hr = pInfo->fTheBar->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pSite));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        return E_NOINTERFACE;
    }
    /* Windows sends a CloseDW before setting site to NULL */
    pDockWnd->CloseDW(0);
    pSite->SetSite(NULL);

    // Delete the band from rebar
    if (!SendMessage(RB_DELETEBAND, index, 0))
    {
        ERR("Can't delete the band\n");
        return E_INVALIDARG;
    }
    if (pInfo == fCurrentActiveBar)
    {
        // FIXME: what to do when we are deleting active bar ? Let's assume we remove it for now
        fCurrentActiveBar = NULL;
    }
    delete pInfo;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetBandObject(DWORD dwBandID, REFIID riid, void **ppv)
{
    REBARBANDINFO               bandInfo;
    HRESULT                     hr;
    CBarInfo                    *pInfo;
    DWORD                       index;

    if (ppv == NULL)
        return E_POINTER;

    // Retrieve the right index of the coolbar knowing the id
    index = SendMessage(RB_IDTOINDEX, dwBandID, 0);
    if (index == 0xffffffff)
       return E_INVALIDARG;

    if (FAILED_UNEXPECTEDLY(GetInternalBandInfo(index, &bandInfo)))
        return E_INVALIDARG;

    pInfo = reinterpret_cast<CBarInfo*>(bandInfo.lParam);
    hr = pInfo->fTheBar->QueryInterface(riid, ppv);
    if (!SUCCEEDED(hr))
        return E_NOINTERFACE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    if (pbsinfo == NULL)
        return E_POINTER;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    if (pbsinfo == NULL)
        return E_POINTER;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    // TODO: what class to return here?
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBarSite::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    if (pcbSize == NULL)
        return E_POINTER;
    return E_NOTIMPL;
}

LRESULT CBaseBarSite::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    NMHDR                       *notifyHeader;

    notifyHeader = reinterpret_cast<NMHDR *>(lParam);
    if (notifyHeader->hwndFrom == m_hWnd)
    {
    }
    bHandled = FALSE; /* forward notification to parent */
    return 0;
}

LRESULT CBaseBarSite::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDM_BASEBAR_CLOSE)
    {
        /* Tell the base bar to hide */
        IUnknown_Exec(fDeskBarSite, IID_IDeskBarClient, 0, 0, NULL, NULL);
        bHandled = TRUE;
    }
    return 0;
}

LRESULT CBaseBarSite::OnCustomDraw(LPNMCUSTOMDRAW pnmcd)
{
    switch (pnmcd->dwDrawStage)
    {
        case CDDS_PREPAINT:
        case CDDS_PREERASE:
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
            if (fVertical)
            {
                REBARBANDINFO info;
                WCHAR wszTitle[MAX_PATH];
                DWORD index;
                RECT rt;
                HFONT newFont, oldFont;

                index = SendMessage(RB_IDTOINDEX, fCurrentActiveBar->fBandID , 0);
                ZeroMemory(&info, sizeof(info));
                ZeroMemory(wszTitle, sizeof(wszTitle));
                DrawEdge(pnmcd->hdc, &pnmcd->rc, EDGE_ETCHED, BF_BOTTOM);
                // We also resize our close button
                ::SetWindowPos(toolbarWnd, HWND_TOP, pnmcd->rc.right - 22, 0, 20, 18, SWP_SHOWWINDOW);
                // Draw the text
                info.cch = MAX_PATH;
                info.lpText = wszTitle;
                rt = pnmcd->rc;
                rt.right -= 24;
                rt.left += 2;
                rt.bottom -= 1;
                if (FAILED_UNEXPECTEDLY(GetInternalBandInfo(index, &info, RBBIM_TEXT)))
                    return CDRF_SKIPDEFAULT;
                newFont = GetTitleFont();
                if (newFont)
                    oldFont = (HFONT)SelectObject(pnmcd->hdc, newFont);
                DrawText(pnmcd->hdc, info.lpText, -1, &rt, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
                SelectObject(pnmcd->hdc, oldFont);
                DeleteObject(newFont);
                return CDRF_SKIPDEFAULT;
            }
            else
            {
                DrawEdge(pnmcd->hdc, &pnmcd->rc, EDGE_ETCHED, BF_BOTTOM);
                // We also resize our close button
                ::SetWindowPos(toolbarWnd, HWND_TOP, 0, 2, 20, 18, SWP_SHOWWINDOW);
            }
            return CDRF_SKIPDEFAULT;
        default:
            break;
    }
    return CDRF_DODEFAULT;
}

HFONT CBaseBarSite::GetTitleFont()
{
    NONCLIENTMETRICS mt;
    mt.cbSize = sizeof(mt);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(mt), &mt, 0))
    {
        ERR("Can't get system parameters !\n");
        return NULL;
    }
    return CreateFontIndirect(&mt.lfMenuFont);

}

HRESULT CBaseBarSite::FindBandByGUID(REFGUID pGuid, DWORD *pdwBandID)
{
    DWORD                       numBands;
    DWORD                       i;
    HRESULT                     hr;
    REBARBANDINFO               bandInfo;
    CBarInfo                    *realInfo;

    hr = EnumBands(-1, &numBands);
    if (FAILED_UNEXPECTEDLY(hr))
        return E_FAIL;

    for(i = 0; i < numBands; i++)
    {
        if (FAILED_UNEXPECTEDLY(GetInternalBandInfo(i, &bandInfo)))
            return E_FAIL;
        realInfo = (CBarInfo*)bandInfo.lParam;
        if (IsEqualGUID(pGuid, realInfo->fBarClass))
        {
            *pdwBandID = realInfo->fBandID;
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT CBaseBarSite::ShowBand(DWORD dwBandID)
{
    UINT                        index;
    CComPtr<IDeskBand>          dockingWindow;
    HRESULT                     hResult;
    REBARBANDINFO               bandInfo;

    // show our band
    hResult = GetBandObject(dwBandID, IID_PPV_ARG(IDeskBand, &dockingWindow));
    if (FAILED_UNEXPECTEDLY(hResult))
        return E_FAIL;

    hResult = dockingWindow->ShowDW(TRUE);

    // Hide old band while adding new one
    if (fCurrentActiveBar && fCurrentActiveBar->fBandID != dwBandID)
    {
        DWORD index;
        index = SendMessage(RB_IDTOINDEX, fCurrentActiveBar->fBandID, 0);
        if (index != 0xffffffff)
            SendMessage(RB_SHOWBAND, index, 0);
    }
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // Display the current band
    index = SendMessage(RB_IDTOINDEX, dwBandID, 0);
    if (index != 0xffffffff)
        SendMessage(RB_SHOWBAND, index, 1);
    if (FAILED_UNEXPECTEDLY(GetInternalBandInfo(index, &bandInfo)))
        return E_FAIL;
    fCurrentActiveBar = (CBarInfo*)bandInfo.lParam;
    return S_OK;
}

HRESULT CBaseBarSite_CreateInstance(REFIID riid, void **ppv, BOOL bVertical)
{
    return ShellObjectCreatorInit<CBaseBarSite, BOOL>(bVertical, riid, ppv);
}
