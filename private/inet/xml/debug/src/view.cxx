//+------------------------------------------------------------------------
//
//  Microsoft Forms
// Copyright (c) 1995 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       view.cxx
//
//  Contents:   IViewObject viewer.
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define IDM_REFRESH 20

enum
{
    TYPE_VIEW,
    TYPE_METAFILE,
    TYPE_ENHMETAFILE
};

static struct
{
    int         type;
    DWORD       dwAspect;
    int         mm;
} s_aModeInfo[] = {
    // Elements in this array correspond to the menu items in f3debug.rc
    { 0 },
    { TYPE_VIEW,        DVASPECT_CONTENT, MM_TEXT },
    { TYPE_VIEW,        DVASPECT_CONTENT, MM_ANISOTROPIC },
    { TYPE_VIEW,        DVASPECT_ICON,    MM_TEXT },
    { TYPE_VIEW,        DVASPECT_ICON,    MM_ANISOTROPIC },
    { TYPE_METAFILE,    DVASPECT_CONTENT, 0 },
    { TYPE_METAFILE,    DVASPECT_ICON,    0 },
    { TYPE_ENHMETAFILE, DVASPECT_CONTENT, 0 },
    { TYPE_ENHMETAFILE, DVASPECT_ICON,    0 },
};

static BOOL s_fWndClassRegistered = FALSE;
static SIZE s_sizePixelsPerInch;
static FORMATETC s_FormatEtcMetaFile =
    { (CLIPFORMAT) CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT };

class CViewer : public IAdviseSink2
{
public:

    CViewer();
    ~CViewer();

    // IUnknown methods

    STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IAdviseSink methods

    void STDMETHODCALLTYPE OnDataChange(
            FORMATETC FAR* pFormatetc,
            STGMEDIUM FAR* pStgmed);
    void STDMETHODCALLTYPE OnViewChange(
            DWORD dwAspect, LONG lindex);
    void STDMETHODCALLTYPE OnRename(LPMONIKER pmk);
    void STDMETHODCALLTYPE OnSave();
    void STDMETHODCALLTYPE OnClose();
    void STDMETHODCALLTYPE OnLinkSrcChange(IMoniker * pmk);

    // New methods

    void OnButtonDown(POINTS pts);
    void OnPaint();
    void OnCommand(int idm);
    void DrawMetaFile(HDC hdc, RECT *prc, DWORD dwApsect);
    void DrawEnhMetaFile(HDC hdc, RECT *prc, DWORD dwAspect);
    void DrawView(HDC hdc, RECT *prc, DWORD_PTR dwAspect, int mm);

    ULONG           _ulRef;
    HWND            _hwnd;
    IDataObject *   _pDO;
    IViewObject *   _pVO;
    int             _cDataChange;
    int             _idmMode;
    DWORD           _dwCookie;
};

CViewer::CViewer()
{
    _ulRef = 0;
    _hwnd  = NULL;
    _pDO  = NULL;
    _cDataChange = 0;
    _idmMode = 1;
    _dwCookie = 0;
}

CViewer::~CViewer()
{
    _ulRef = 256;

    if (_pDO)
    {
        if (_dwCookie)
        {
            _pDO->DUnadvise(_dwCookie);
            _dwCookie = 0;
        }
        _pDO->Release();
        _pDO = NULL;
    }

    if (_pVO)
    {
        _pVO->SetAdvise(DVASPECT_CONTENT, 0, NULL);
        _pVO->Release();
        _pVO = 0;
    }

    if (_hwnd)
    {
        SetWindowLongA(_hwnd, GWLP_USERDATA, 0L);
        Verify(DestroyWindow(_hwnd));
        _hwnd = NULL;
    }
}

STDMETHODIMP
CViewer::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown ||
            iid == IID_IAdviseSink2 ||
            iid == IID_IAdviseSink)
    {
        *ppv = (IAdviseSink2 *)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CViewer::AddRef()
{
    return _ulRef += 1;
}

STDMETHODIMP_(ULONG)
CViewer::Release()
{
    _ulRef -= 1;
    if (_ulRef == 0)
    {
        delete this;
        return 0;
    }
    return _ulRef;
}

STDMETHODIMP_(void)
CViewer::OnDataChange(FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed)
{
    _cDataChange += 1;
    InvalidateRect(_hwnd, 0, FALSE);
}

STDMETHODIMP_(void)
CViewer::OnViewChange(DWORD dwAspects, LONG lindex)
{
    InvalidateRect(_hwnd, 0, FALSE);
}

STDMETHODIMP_(void)
CViewer::OnRename(LPMONIKER pmk)
{
}

STDMETHODIMP_(void)
CViewer::OnSave()
{
}

STDMETHODIMP_(void)
CViewer::OnClose()
{
}

STDMETHODIMP_(void)
CViewer::OnLinkSrcChange(IMoniker * pmk)
{
}

void
CViewer::OnCommand(int idm)
{
    switch (idm)
    {
    case IDM_REFRESH:
        InvalidateRect(_hwnd, NULL, TRUE);
        break;

    default:

        if (idm >= 1 && idm < ARRAY_SIZE(s_aModeInfo))
        {
            _idmMode = idm;
            InvalidateRect(_hwnd, NULL, TRUE);
        }
        break;
    }
}

void
CViewer::OnButtonDown(POINTS pts)
{
    POINT pt = { pts.x, pts.y };
    HMENU hmenu;
    HMENU hmenuSub;
    int   idm;

    hmenu = LoadMenuA(g_hinstMain, "ViewerMenu");
    ClientToScreen(_hwnd, &pt);
    hmenuSub = GetSubMenu(hmenu, 0);
    for (idm = 0; idm < ARRAY_SIZE(s_aModeInfo); idm++)
    {
        CheckMenuItem(hmenuSub,
                idm,
                (_idmMode == idm) ?
                        MF_BYCOMMAND|MF_CHECKED :
                        MF_BYCOMMAND|MF_UNCHECKED);
    }
#ifndef _MAC
    TrackPopupMenu(hmenuSub, TPM_LEFTALIGN | TPM_LEFTBUTTON,
#else
    TrackPopupMenu(hmenuSub, 0,
#endif
        pt.x, pt.y, 0, _hwnd, NULL);
    DestroyMenu(hmenu);
}

void
CViewer::DrawView(HDC hdc, RECT *prc, DWORD_PTR dwAspect, int mm)
{
    HRESULT       hr;

    if (mm == MM_ANISOTROPIC)
    {
        // himetric
        LPtoDP(hdc, (POINT *)prc, 2);
        SetMapMode(hdc, mm);
        SetWindowExtEx(hdc, 2540, 2540, NULL);
        SetViewportExtEx(hdc,
                GetDeviceCaps(hdc, LOGPIXELSX),
                GetDeviceCaps(hdc, LOGPIXELSY), NULL);
        DPtoLP(hdc, (POINT *)prc, 2);
    }
#ifndef _MAC
    hr = OleDraw(_pVO, (ULONG)dwAspect, hdc, prc);
#else
	hr = _pVO->Draw(	 dwAspect,-1,0,0,0, hdc, (LPRECTL)prc, 0,0,0);
#endif
}

void
CViewer::DrawMetaFile(HDC hdc, RECT *prc, DWORD dwApsect)
{
    HRESULT         hr;
    STGMEDIUM       medium;
    METAFILEPICT *  pPict;
    FORMATETC       fmtetc =
        { CF_METAFILEPICT, NULL, dwApsect, -1, TYMED_MFPICT };

    memset(&medium, 0, sizeof(medium));

    hr = _pDO->GetData(&fmtetc, &medium);
    if (FAILED(hr))
        goto Cleanup;

    // STGFIX: t-gpease 8-13-97
    Assert(medium.tymed == TYMED_HGLOBAL);

    pPict = (METAFILEPICT *)GlobalLock(medium.hGlobal);

    SetMapMode(hdc, MM_ANISOTROPIC);
    SetWindowExtEx(hdc, pPict->xExt, pPict->yExt, NULL);
    SetViewportExtEx(hdc, prc->right - prc->left, prc->bottom - prc->top, NULL);
    // SetWindowExtEx(hdc, pPict->xExt, pPict->yExt, NULL);
    PlayMetaFile(hdc, pPict->hMF);

Cleanup:
    ReleaseStgMedium(&medium);
}

void
CViewer::DrawEnhMetaFile(HDC hdc, RECT *prc, DWORD dwApsect)
{
#ifndef _MAC
    HRESULT         hr;
    STGMEDIUM       medium;
    FORMATETC       fmtetc =
        { CF_ENHMETAFILE, NULL, dwApsect, -1, TYMED_ENHMF };

    memset(&medium, 0, sizeof(medium));

    hr = _pDO->GetData(&fmtetc, &medium);
    if (FAILED(hr))
        goto Cleanup;
    PlayEnhMetaFile(hdc, medium.hEnhMetaFile, prc);

Cleanup:
    ReleaseStgMedium(&medium);
#endif
}

void
CViewer::OnPaint()
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    char achMode[128];
    char achTitle[128];
    HMENU hmenu;
    HMENU hmenuSub;

    hmenu = LoadMenuA(g_hinstMain, "ViewerMenu");
    hmenuSub = GetSubMenu(hmenu, 0);
    GetMenuStringA(hmenuSub, _idmMode, achMode, sizeof(achMode), MF_BYCOMMAND);
    DestroyMenu(hmenu);
    wsprintfA(achTitle, "%s %d", achMode, _cDataChange);
    SetWindowTextA(_hwnd, achTitle);

    GetClientRect(_hwnd, &rc);
    hdc = ::BeginPaint(_hwnd, &ps);

    switch (s_aModeInfo[_idmMode].type)
    {
    case TYPE_VIEW:
        DrawView(
                hdc,
                &rc,
                s_aModeInfo[_idmMode].dwAspect,
                s_aModeInfo[_idmMode].mm);
        break;

    case TYPE_METAFILE:
        DrawMetaFile(
                hdc,
                &rc,
                s_aModeInfo[_idmMode].dwAspect);
        break;

    case TYPE_ENHMETAFILE:
        DrawEnhMetaFile(
                hdc,
                &rc,
                s_aModeInfo[_idmMode].dwAspect);
        break;
    }

    SelectPalette(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
    EndPaint(_hwnd, &ps);
}

LRESULT CALLBACK
ViewerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CViewer *pViewer;

    if (msg == WM_NCCREATE)
    {
        pViewer = (CViewer *) ((LPCREATESTRUCTW)lParam)->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LPARAM) pViewer);
        pViewer->_hwnd = hwnd;
    }
    else
    {
        pViewer = (CViewer *)GetWindowLongA(hwnd, GWLP_USERDATA);
    }

    if (pViewer)
    {
        switch (msg)
        {
        case WM_ERASEBKGND:
            if (s_aModeInfo[pViewer->_idmMode].dwAspect == DVASPECT_CONTENT)
                return TRUE;
            break;
            
        case WM_COMMAND:
            pViewer->OnCommand(GET_WM_COMMAND_ID(wParam, lParam));
            return 0;

        case WM_RBUTTONDOWN:
            pViewer->OnButtonDown(MAKEPOINTS(lParam));
            return 0;

        case WM_PAINT:
            pViewer->OnPaint();
            return 0;

        case WM_NCDESTROY:
            pViewer->_hwnd = NULL;
            delete pViewer;
            break;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void
EXPORT WINAPI 
OpenViewObjectMonitor(HWND hwndOwner, IUnknown *pUnk, BOOL fUseFrameSize)
{
    CViewer *           pViewer = NULL;
    IOleObject *        pObj = NULL;
    IOleClientSite *    pClientSite = NULL;
    IOleInPlaceSite *   pIPSite = NULL;
    IOleInPlaceFrame *  pIPFrame = NULL;
    IOleInPlaceUIWindow * pIPWin = NULL;
    OLEINPLACEFRAMEINFO FI;
    RECT            rcPos;
    RECT            rcClip;
    WNDCLASSA       wc;
    HRESULT         hr;
    SIZEL           sizel;
    SIZE            size;
    HDC             hdc;

    EnsureThreadState();

    pViewer = new CViewer;
    if (!pViewer)
        goto Error;

    {   LOCK_GLOBALS;

        if (!s_fWndClassRegistered)
        {
            hdc = GetDC(NULL);
            if (!hdc)
                goto Error;

            s_sizePixelsPerInch.cx = GetDeviceCaps(hdc, LOGPIXELSX);
            s_sizePixelsPerInch.cy = GetDeviceCaps(hdc, LOGPIXELSY);

            ReleaseDC(NULL, hdc);

            memset(&wc, 0, sizeof(wc));
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = ViewerWndProc;
            wc.hInstance = g_hinstMain;
            wc.lpszClassName = "F3Viewer";
            wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);

            if (RegisterClassA(&wc) == 0)
                goto Error;

            s_fWndClassRegistered = TRUE;
        }
    }

    hr = pUnk->QueryInterface(IID_IDataObject, (void **)&pViewer->_pDO);
    if (hr)
        goto Error;

    hr = pUnk->QueryInterface(IID_IViewObject, (void **)&pViewer->_pVO);
    if (FAILED(hr))
        goto Error;

    hr = pUnk->QueryInterface(IID_IOleObject, (void **)&pObj);
    if (hr)
        goto Error;

    hr = pObj->GetClientSite(&pClientSite);
    if (hr)
        goto Error;

    hr = pClientSite->QueryInterface(IID_IOleInPlaceSite, (void **) &pIPSite);
    if (hr)
        goto Error;

    hr = pIPSite->GetWindowContext(
            &pIPFrame,
            &pIPWin,
            &rcPos,
            &rcClip,
            &FI);
    if (hr)
        goto Error;

    GetWindowRect(FI.hwndFrame, &rcPos);

    if (fUseFrameSize)
    {
        size.cx = rcPos.right - rcPos.left;
        size.cy = rcPos.bottom - rcPos.top;
    }
    else
    {
        RECT    rcClient;
        GetClientRect(FI.hwndFrame, &rcClient);

        hr = pObj->GetExtent(DVASPECT_CONTENT, &sizel);
        if (hr)
            goto Error;                
        size.cx = MulDiv(sizel.cx, s_sizePixelsPerInch.cx, 2540) + 
                (rcPos.right - rcPos.left) - (rcClient.right - rcClient.left);
        size.cy = MulDiv(sizel.cy, s_sizePixelsPerInch.cy, 2540) +
                (rcPos.bottom - rcPos.top) - (rcClient.bottom - rcClient.top);
    }

    if (CreateWindowExA(
            0,
            "F3Viewer",
            "IViewObject",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            rcPos.left,
            rcPos.bottom,
            size.cx,
            size.cy,
            hwndOwner,
            NULL,
            g_hinstMain,
            pViewer) == 0)
        goto Error;

    hr = pViewer->_pDO->DAdvise(
            &s_FormatEtcMetaFile,
            ADVF_NODATA,
            pViewer,
            &pViewer->_dwCookie);
    if (hr)
        goto Error;

    hr = pViewer->_pVO->SetAdvise(DVASPECT_CONTENT, 0, pViewer);
    if (hr)
        goto Error;

Cleanup:
    if (pObj)
        pObj->Release();
    if (pClientSite)
        pClientSite->Release();
    if (pIPSite)
        pIPSite->Release();
    if (pIPFrame)
        pIPFrame->Release();
    if (pIPWin)
        pIPWin->Release();
    return;

Error:
    delete pViewer;
    goto Cleanup;
}
