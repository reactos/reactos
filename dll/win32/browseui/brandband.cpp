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
Implements the logo band of a cabinet window. Most remarkable feature is the
animation.
*/

#include "precomp.h"

/*
TODO:
    Add Exec command handlers
    Properly implement GetBandInfo
    Fix SetSite to revoke brand band service when site is cleared
*/

inline void FillSolidRect(HDC dc, const RECT *bounds)
{
    ::ExtTextOut(dc, 0, 0, ETO_OPAQUE, bounds, NULL, 0, NULL);
}

inline void FillSolidRect(HDC dc, const RECT *bounds, COLORREF clr)
{
    ::SetBkColor(dc, clr);
    ::ExtTextOut(dc, 0, 0, ETO_OPAQUE, bounds, NULL, 0, NULL);
}

static const int                            gSmallImageSize = 22;
static const int                            gMediumImageSize = 26;
static const int                            gLargeImageSize = 38;

static const int                            gTrueColorResourceBase = 240;
static const int                            g256ColorResourceBase = 245;

CBrandBand::CBrandBand()
{
    fProfferCookie = 0;
    fCurrentFrame = 0;
    fMaxFrameCount = 0;
    fImageBitmap = NULL;
    fBitmapSize = 0;
    fAdviseCookie = 0;
}

CBrandBand::~CBrandBand()
{
    DeleteObject(fImageBitmap);
}

void CBrandBand::StartAnimation()
{
    fCurrentFrame = 0;
    SetTimer(5678, 30, NULL);
}

void CBrandBand::StopAnimation()
{
    KillTimer(5678);
    fCurrentFrame = 0;
    Invalidate(FALSE);
}

void CBrandBand::SelectImage()
{
    int                                     screenDepth;
    RECT                                    clientRect;
    int                                     clientWidth;
    int                                     clientHeight;
    int                                     clientSize;
    HINSTANCE                               shell32Instance;
    BITMAP                                  bitmapInfo;
    int                                     resourceID;

    screenDepth = SHGetCurColorRes();
    GetClientRect(&clientRect);
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;
    clientSize = min(clientWidth, clientHeight);
    if (screenDepth > 8)
        resourceID = gTrueColorResourceBase;
    else
        resourceID = g256ColorResourceBase;
    if (clientSize >= gLargeImageSize)
        resourceID += 2;
    else if (clientSize >= gMediumImageSize)
        resourceID += 1;
    shell32Instance = GetModuleHandle(L"shell32.dll");
    fImageBitmap = LoadBitmap(shell32Instance, MAKEINTRESOURCE(resourceID));
    GetObjectW(fImageBitmap, sizeof(bitmapInfo), &bitmapInfo);
    fBitmapSize = bitmapInfo.bmWidth;
    fMaxFrameCount = bitmapInfo.bmHeight / fBitmapSize;
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 38;
        pdbi->ptMinSize.y = 22;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        pdbi->ptMaxSize.x = 38;
        pdbi->ptMaxSize.y = 38;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 38;
        pdbi->ptIntegral.y = 38;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 38;
        pdbi->ptActual.y = 38;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBrandBand::SetSite(IUnknown* pUnkSite)
{
    CComPtr<IBrowserService>                browserService;
    CComPtr<IOleWindow>                     oleWindow;
    CComPtr<IServiceProvider>               serviceProvider;
    CComPtr<IProfferService>                profferService;
    HWND                                    parentWindow;
    HWND                                    hwnd;
    HRESULT                                 hResult;

    fSite.Release();
    if (pUnkSite == NULL)
    {
        hResult = AtlUnadvise(fSite, DIID_DWebBrowserEvents, fAdviseCookie);
        // TODO: revoke brand band service
        return S_OK;
    }

    // get window handle of parent
    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &fSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    parentWindow = NULL;
    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &oleWindow));
    if (SUCCEEDED(hResult))
        hResult = oleWindow->GetWindow(&parentWindow);
    if (!::IsWindow(parentWindow))
        return E_FAIL;

    // create worker window in parent window
    hwnd = SHCreateWorkerWindowW(0, parentWindow, 0,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, 0);
    if (hwnd == NULL)
        return E_FAIL;
    SubclassWindow(hwnd);

    // take advice to watch events
    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &serviceProvider));
    if (SUCCEEDED(hResult))
    {
        hResult = serviceProvider->QueryService(
            SID_SBrandBand, IID_PPV_ARG(IProfferService, &profferService));
        if (SUCCEEDED(hResult))
            hResult = profferService->ProfferService(SID_SBrandBand,
                static_cast<IServiceProvider *>(this), &fProfferCookie);
        hResult = serviceProvider->QueryService(SID_SShellBrowser,
            IID_PPV_ARG(IBrowserService, &browserService));
        if (SUCCEEDED(hResult))
            hResult = AtlAdvise(browserService, static_cast<IDispatch *>(this), DIID_DWebBrowserEvents, &fAdviseCookie);
    }

    // ignore any hResult errors up to here - they are nonfatal
    hResult = S_OK;
    SelectImage();
    return hResult;
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetSite(REFIID riid, void **ppvSite)
{
    if (ppvSite == NULL)
        return E_POINTER;
    if (fSite.p == NULL)
    {
        *ppvSite = NULL;
        return E_FAIL;
    }
    return fSite.p->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBrandBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (IsWindow())
        DestroyWindow();

    m_hWnd = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBrandBand::ResizeBorderDW(
    const RECT* prcBorder, IUnknown* punkToolbarSite, BOOL fReserved)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::ShowDW(BOOL fShow)
{
    if (m_hWnd)
    {
        if (fShow)
            ShowWindow(SW_SHOW);
        else
            ShowWindow(SW_HIDE);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBrandBand::HasFocusIO()
{
    if (GetFocus() == m_hWnd)
        return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CBrandBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_BrandBand;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBrandBand::IsDirty()
{
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CBrandBand::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::OnWinEvent(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::IsWindowOwner(HWND hWnd)
{
    if (hWnd == m_hWnd)
        return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CBrandBand::QueryStatus(
    const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_PrivCITCommands))
    {
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_BrandCmdGroup))
    {
        switch (nCmdID)
        {
            case BBID_STARTANIMATION:
                StartAnimation();
                return S_OK;
            case BBID_STOPANIMATION:
                StopAnimation();
                return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    CComPtr<IServiceProvider>               serviceProvider;
    HRESULT                                 hResult;

    if (IsEqualIID(guidService, SID_SBrandBand))
        return this->QueryInterface(riid, ppvObject);
    hResult = fSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &serviceProvider));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
    LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBrandBand::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
    DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    if (pDispParams == NULL)
        return E_INVALIDARG;
    switch (dispIdMember)
    {
        case DISPID_DOWNLOADCOMPLETE:
            StopAnimation();
            break;
        case DISPID_DOWNLOADBEGIN:
            StartAnimation();
            break;
    }
    return E_INVALIDARG;
}

LRESULT CBrandBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    Invalidate(FALSE);
    return 0;
}

LRESULT CBrandBand::OnEraseBkgnd (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return 1;
}

LRESULT CBrandBand::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    PAINTSTRUCT                             paintInfo;
    HDC                                     dc;
    POINT                                   destinationPoint;
    HDC                                     sourceDC;
    HBITMAP                                 oldBitmap;
    RECT                                    clientRect;
    RECT                                    tempRect;

    dc = BeginPaint(&paintInfo);
    GetClientRect(&clientRect);

    destinationPoint.x = (clientRect.right - clientRect.left - fBitmapSize) / 2;
    destinationPoint.y = (clientRect.bottom - clientRect.top - fBitmapSize) / 2;

    ::SetBkColor(dc, RGB(255, 255, 255));

    tempRect.left = 0;
    tempRect.top = 0;
    tempRect.right = clientRect.right;
    tempRect.bottom = destinationPoint.y;
    FillSolidRect(dc, &tempRect, RGB(255, 255, 255));

    tempRect.left = 0;
    tempRect.top = destinationPoint.y + fBitmapSize;
    tempRect.right = clientRect.right;
    tempRect.bottom = clientRect.bottom;
    FillSolidRect(dc, &paintInfo.rcPaint, RGB(255, 255, 255));

    tempRect.left = 0;
    tempRect.top = destinationPoint.y;
    tempRect.right = destinationPoint.x;
    tempRect.bottom = destinationPoint.y + fBitmapSize;
    FillSolidRect(dc, &paintInfo.rcPaint, RGB(255, 255, 255));

    tempRect.left = destinationPoint.x + fBitmapSize;
    tempRect.top = destinationPoint.y;
    tempRect.right = clientRect.right;
    tempRect.bottom = destinationPoint.y + fBitmapSize;
    FillSolidRect(dc, &paintInfo.rcPaint, RGB(255, 255, 255));

    sourceDC = CreateCompatibleDC(dc);
    oldBitmap = reinterpret_cast<HBITMAP>(SelectObject(sourceDC, fImageBitmap));

    BitBlt(dc, destinationPoint.x, destinationPoint.y, fBitmapSize, fBitmapSize, sourceDC, 0, fCurrentFrame * fBitmapSize, SRCCOPY);

    SelectObject(sourceDC, oldBitmap);
    DeleteDC(sourceDC);

    EndPaint(&paintInfo);
    return 0;
}

LRESULT CBrandBand::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    fCurrentFrame++;
    if (fCurrentFrame >= fMaxFrameCount)
        fCurrentFrame = 0;
    Invalidate(FALSE);
    return 0;
}

HRESULT CreateBrandBand(REFIID riid, void **ppv)
{
    return ShellObjectCreator<CBrandBand>(riid, ppv);
}
