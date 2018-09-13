//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padbox.cxx
//
//  Contents:   Implements control palette.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

static BOOL s_fWndClassRegistered = FALSE;

static void CheckButton(int ici);

static const CLSID CLSID_SuperLabel = {0x67EEB7C3L,0x6242,0x11CF,0xA0,0xC0,0x00,0xAA,0x00,0x62,0xBE,0x57};

static const struct CLASSINFO
{
    const CLSID *   pclsid;
    int             idr;
}
s_aci[] =
{
    { &CLSID_NULL, IDR_SELECT_TOOL },
    { &CLSID_HTMLImg, IDR_IMAGE_TOOL },
    { &CLSID_HTMLInputElement, IDR_BUTTON_TOOL },
    { &CLSID_HTMLInputElement, IDR_TEXTBOX_TOOL },
    { &CLSID_HTMLListElement, IDR_LISTBOX_TOOL },
    { &CLSID_HTMLInputElement, IDR_CHECKBOX_TOOL },
    { &CLSID_HTMLInputElement, IDR_RADIOBUTTON_TOOL },
    { &CLSID_HTMLDivPosition, IDR_TEXTSITE_TOOL },
};

//+------------------------------------------------------------------------
//
//  Class:   CControlPaletteService
//
//-------------------------------------------------------------------------

class CControlPaletteService : public IControlPalette
{
public:
    CControlPaletteService();
    ~CControlPaletteService();

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // IControlPalette methods

    STDMETHOD(SetCursor)();
    STDMETHOD(GetData)(IDataObject **ppDO);
    STDMETHOD(DataUsed)();

    // Data members

    ULONG   _ulRefs;
};

//+------------------------------------------------------------------------
//
//  Class:   CControlPaletteData
//
//-------------------------------------------------------------------------

class CControlPaletteData : public IDataObject, public IDropSource
{
public:
    CControlPaletteData(const CLSID *pclsid);
    ~CControlPaletteData();

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // IDataObject methods

    STDMETHOD(GetData)(FORMATETC * pformatetc, STGMEDIUM * pmedium);
    STDMETHOD(GetDataHere)(FORMATETC * pformatetc, STGMEDIUM * pmedium);
    STDMETHOD(QueryGetData)(FORMATETC * pfe);
    STDMETHOD(GetCanonicalFormatEtc)(FORMATETC * pformatetc, FORMATETC * pformatetcOut);
    STDMETHOD(SetData)(FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC ** ppenum);
    STDMETHOD(DAdvise)(FORMATETC * pformatetc, DWORD advf, IAdviseSink * pAdvSink, DWORD * pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(IEnumSTATDATA ** ppenumAdvise);

    // IDropSource methods

    STDMETHOD(QueryContinueDrag)(BOOL fEscPressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

    // Data members

    ULONG _ulRefs;
    CLSID _clsid;
};

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::CControlPaletteService
//
//-------------------------------------------------------------------------

CControlPaletteService::CControlPaletteService()
{
    _ulRefs = 1;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::~CControlPaletteService
//
//-------------------------------------------------------------------------

CControlPaletteService::~CControlPaletteService( )
{
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteService::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IControlPalette)
    {
        *ppv = (IControlPalette *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *) *ppv)->AddRef();
    return NOERROR;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::AddRef, IUnknown
//
//-------------------------------------------------------------------------

ULONG
CControlPaletteService::AddRef()
{
    return _ulRefs += 1;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::Release, IUnknown
//
//-------------------------------------------------------------------------

ULONG
CControlPaletteService::Release()
{
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
    }

    return 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::SetCursor, IControlPalette
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteService::SetCursor()
{
    if (s_iciChecked == 0)
    {
        return S_FALSE;
    }
    else
    {
        ::SetCursor(LoadCursor(NULL, IDC_CROSS));
        return S_OK;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::GetData, IControlPalette
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteService::GetData(IDataObject **ppDO)
{
    CControlPaletteData *pData = NULL;

    if (s_iciChecked == 0)
    {
        *ppDO = NULL;
        return S_FALSE;
    }
    else
    {
        *ppDO = new CControlPaletteData(s_aci[s_iciChecked].pclsid);
        RRETURN(*ppDO == NULL ? E_OUTOFMEMORY : S_OK);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteService::DataUsed, IControlPalette
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteService::DataUsed()
{
    CheckButton(0);
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::CControlPaletteData
//
//-------------------------------------------------------------------------

CControlPaletteData::CControlPaletteData(const CLSID *pclsid)
{
    _ulRefs = 1;
    _clsid = *pclsid;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::~CControlPaletteData
//
//-------------------------------------------------------------------------

CControlPaletteData::~CControlPaletteData( )
{
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDataObject)
    {
        *ppv = (IDataObject *) this;
    }
    else if (iid == IID_IDropSource)
    {
        *ppv = (IDropSource *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *) *ppv)->AddRef();
    return NOERROR;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::AddRef, IUnknown
//
//-------------------------------------------------------------------------

ULONG
CControlPaletteData::AddRef()
{
    return _ulRefs += 1;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::Release, IUnknown
//
//-------------------------------------------------------------------------

ULONG
CControlPaletteData::Release()
{
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
    }

    return 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::QueryContinueDrag, IDropSource
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::QueryContinueDrag(BOOL fEscPressed, DWORD grfKeyState)
{
    if (fEscPressed)
    {
        return DRAGDROP_S_CANCEL;
    }

    if (!((grfKeyState & MK_LBUTTON) || (grfKeyState & MK_RBUTTON)))
    {
        return DRAGDROP_S_DROP;
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::QueryContinueDrag, IDropSource
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::GetData, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::GetData(FORMATETC * pfe, STGMEDIUM * pmedium)
{
    HRESULT     hr = S_OK;
    void *      ptr;
#ifdef _MAC
    HANDLE      hdl;
    HANDLE  *   phdl = &hdl;
#else
    HANDLE  *   phdl = &pmedium->hGlobal;
#endif

    TCHAR       tszClsid[MAX_PATH] = _T("");

    if (pfe->cfFormat == s_cfCLSID &&
        pfe->dwAspect == DVASPECT_CONTENT &&
        pfe->tymed == TYMED_HGLOBAL)
    {
        hr = THR(Format (
            0, tszClsid, MAX_PATH,
            _T("<0g>"),
            &_clsid));
        if (!OK(hr))
            goto Cleanup;

        *phdl = GlobalAlloc(
            GMEM_SHARE | GMEM_MOVEABLE, 
            (CLSID_STRLEN+1)*sizeof(TCHAR));
        if (*phdl == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

#ifndef _MAC
        ptr = GlobalLock(*phdl);
        memcpy((void *) ptr, tszClsid, (CLSID_STRLEN+1)*sizeof(TCHAR));
        GlobalUnlock(*phdl);
#else
        ptr = GlobalLock(*phdl)
        memcpy(ptr, ptszClsid, (CLSID_STRLEN+1)*sizeof(TCHAR));
        GlobalUnlock(*phdl);
        if(!UnwrapHandle(*phdl,(Handle*)&pmedium->hGlobal))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
#endif
    }
    else
    {
        hr = DV_E_FORMATETC;
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::GetDataHere, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::GetDataHere(FORMATETC * pformatetc, STGMEDIUM * pmedium)
{
    return E_FAIL;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::QueryGetData, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::QueryGetData(FORMATETC * pfe)
{
    HRESULT hr;

    if (pfe->cfFormat == s_cfCLSID &&
        pfe->dwAspect == DVASPECT_CONTENT &&
        pfe->tymed == TYMED_HGLOBAL)
    {
        hr = S_OK;
    }
    else
    {
        hr = DV_E_FORMATETC;
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::GetCanonicalFormatEtc, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::GetCanonicalFormatEtc(
        FORMATETC * pformatetc,
        FORMATETC * pformatetcOut)
{
    pformatetcOut = NULL;
    return E_FAIL;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::SetData, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::SetData(
        FORMATETC * pformatetc,
        STGMEDIUM * pmedium,
        BOOL fRelease)
{
    return E_FAIL;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::EnumFormatEtc, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenum)
{
    *ppenum = NULL;
    return E_UNEXPECTED;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::DAdvise, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::DAdvise(
        FORMATETC * pformatetc,
        DWORD advf,
        IAdviseSink * pAdvSink,
        DWORD * pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::DUnadvise, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

//+------------------------------------------------------------------------
//
//  Member:     CControlPaletteData::EnumDAdvise, IDataObject
//
//-------------------------------------------------------------------------

HRESULT
CControlPaletteData::EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

static void
CheckButton(int ici)
{
    if (s_hwndBar && ici != s_iciChecked)
    {
        SendMessage(s_hwndBar, TB_CHECKBUTTON, ici, MAKELONG(TRUE, 0));
        s_iciChecked = ici;
    }
}

static LRESULT
FrameOnCommand(WORD wNotifyCode, WORD ici, HWND hwndCtl)
{
    // Get out if it does not look like one of our commands.
    if (hwndCtl != s_hwndBar)
        return 0;

    s_iciChecked = ici;

    return 0;
}

static LRESULT
FrameOnClose()
{
    ShowWindow(s_hwndFrame, SW_HIDE);
    return 0;
}

LRESULT
FrameOnNCHitTest(WPARAM wParam, LPARAM lParam)
{
    LRESULT l;

    l = DefWindowProc(s_hwndFrame, WM_NCHITTEST, wParam, lParam);
    switch (l)
    {
    case HTTOPLEFT:
    case HTBOTTOMLEFT:
    case HTLEFT:
    case HTTOPRIGHT:
    case HTBOTTOMRIGHT:
    case HTRIGHT:
    case HTTOP:
    case HTBOTTOM:
        // Force fixed size window.
        l = HTCAPTION;
        break;
    }

    return l;
}

#if !defined(UNIX)
static LRESULT
FrameOnNotify(int idCtrl, NMHDR *pnmhdr)
{
    TBBUTTON    tbb;
    DWORD       dw;

    if (pnmhdr->hwndFrom == s_hwndBar)
    {
        if (pnmhdr->code == TBN_BEGINDRAG)
        {
            SendMessage(pnmhdr->hwndFrom,
                    TB_GETBUTTON,
                    ((TBNOTIFY *)pnmhdr)->iItem,
                    (LPARAM)&tbb);
            s_iciDrag = tbb.idCommand;
            dw = GetMessagePos();
            s_ptsDrag = MAKEPOINTS(dw);
        }
        else if (pnmhdr->code == TBN_ENDDRAG)
        {
            s_iciDrag = 0;
        }
    }

    return DefWindowProc(s_hwndFrame, WM_NOTIFY, idCtrl, (LPARAM)pnmhdr);
}
#endif

static LRESULT CALLBACK
FrameWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm)
    {
    case WM_COMMAND:
        return FrameOnCommand(GET_WM_COMMAND_CMD(wParam, lParam),
                              GET_WM_COMMAND_ID(wParam, lParam), 
                              GET_WM_COMMAND_HWND(wParam, lParam));

    case WM_CLOSE:
        return FrameOnClose();

    case WM_NCHITTEST:
        return FrameOnNCHitTest(wParam, lParam);

#if !defined(UNIX)
    case WM_NOTIFY:
        return FrameOnNotify(wParam, (NMHDR *)lParam);
#endif

    default:
        return DefWindowProc(hwnd, wm, wParam, lParam);
    }
}

static LRESULT
BarOnMouseMove(WORD fwKeys, int xPos, int yPos)
{
    POINTS      pts;
    DWORD       dwEffect;
    CControlPaletteData *  pData;
    DWORD       dw;
    POINTS      ptsDrag = s_ptsDrag;

    if (s_iciDrag != 0)
    {
        dw = GetMessagePos();
        pts = MAKEPOINTS(dw);
        if (pts.x < ptsDrag.x - 2 ||
            pts.x > ptsDrag.x + 2 ||
            pts.y < ptsDrag.y - 2 ||
            pts.y > ptsDrag.y + 2)
        {
            Verify(pData = new CControlPaletteData(s_aci[s_iciDrag].pclsid));
            if (pData)
            {
#if DBG==1
                Assert(!TLS(fHandleCaptureChanged));
#endif
                ReleaseCapture();
                CheckButton(s_iciDrag);
                s_iciDrag = 0;
                IGNORE_HR(DoDragDrop(pData, pData, DROPEFFECT_COPY|DROPEFFECT_MOVE, &dwEffect));
                pData->Release();
                CheckButton(0);
            }
        }
    }

    return s_pfnBarWndProc(s_hwndBar, WM_MOUSEMOVE, fwKeys, MAKELONG(xPos, yPos));
}

static LRESULT CALLBACK
BarWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm)
    {
    case WM_MOUSEMOVE:
        return BarOnMouseMove(wParam, LOWORD(lParam), HIWORD(lParam));

    default:
        return s_pfnBarWndProc(hwnd, wm, wParam, lParam);
    }
}

static BOOL
InitControlPalette()
{
    WNDCLASS    wc;
    TCHAR       achKey[MAX_PATH];
    TCHAR       achValue[MAX_PATH];
    TCHAR *     pch;
    long        cb;
    int         idr;
    TBADDBITMAP tbab;
    TBBUTTON    tbb;
    int         i, c;
    HINSTANCE   hinst;
    RECT        rc, rcBar;

    s_cfCLSID = RegisterClipboardFormatA("MS Forms CLSID");

    if (!s_fWndClassRegistered)
    {
        LOCK_GLOBALS;

        if (!s_fWndClassRegistered)
        {
            memset(&wc, 0, sizeof(wc));
            wc.style = NULL;
            wc.lpfnWndProc = FrameWndProc;                                                                            // windows of this class.
            wc.hInstance = g_hInstCore;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.lpszClassName = SZ_APPLICATION_NAME TEXT(" CP Frame");

            if (!RegisterClass(&wc))
                return FALSE;
                                  
            if (!GetClassInfo(NULL, TOOLBARCLASSNAME, &wc))
                return FALSE;

            s_pfnBarWndProc = wc.lpfnWndProc;
            wc.lpfnWndProc = BarWndProc;                                                                            // windows of this class.
            wc.hInstance = g_hInstCore;
            wc.lpszClassName = SZ_APPLICATION_NAME TEXT(" CP ToolBar");

            if (!RegisterClass(&wc))
                return FALSE;

            s_fWndClassRegistered = TRUE;
        }
    }

    s_hwndFrame = CreateWindowEx(
            WS_EX_TOOLWINDOW,
            SZ_APPLICATION_NAME TEXT(" CP Frame"),
            TEXT("Controls"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0,
            s_hwndOwner,
            NULL,
            g_hInstCore,
            NULL);

    if (!s_hwndFrame)
        return FALSE;

    s_hwndBar = CreateWindowEx(
            0,
            SZ_APPLICATION_NAME TEXT(" CP Toolbar"),
            (LPCTSTR) NULL,
            CCS_NODIVIDER |
                TBSTYLE_WRAPABLE |
                WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            s_hwndFrame,
            (HMENU) 0,
            g_hInstCore,
            NULL);
    if (!s_hwndBar)
        return FALSE;

    SendMessage(s_hwndBar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

    _tcscpy(achKey, TEXT("CLSID\\"));

    c = 0;
    for (i = 0; i < ARRAY_SIZE(s_aci); i++)
    {
        if (s_aci[i].idr)
        {
            tbab.hInst = g_hInstResource;
            tbab.nID   = s_aci[i].idr;
            tbb.iBitmap = SendMessage(s_hwndBar, TB_ADDBITMAP, 1, (WPARAM) &tbab);
            tbb.idCommand = i;
            tbb.fsState = TBSTATE_ENABLED;
            if (i == 0)
            {
                tbb.fsState |= TBSTATE_CHECKED;
            }
            tbb.fsStyle = TBSTYLE_CHECK | TBSTYLE_CHECKGROUP;
            tbb.dwData = 0;
            tbb.iString = 0;
            SendMessage(s_hwndBar, TB_ADDBUTTONS, 1, (LPARAM) &tbb);
        }
        else
        {
            Verify(StringFromGUID2(*s_aci[i].pclsid, &achKey[6], ARRAY_SIZE(achKey) - 6));
            _tcscat(achKey, TEXT("\\ToolboxBitmap32"));

            cb = sizeof(achValue);
            if (RegQueryValue(HKEY_CLASSES_ROOT, achKey, achValue, &cb))
                continue;

            pch = _tcschr(achValue, TEXT(','));
            if (pch)
            {
                *pch++ = TEXT('\0');
                idr = (int)_tcstol(pch, NULL, 10);
            }
            else
            {
                idr = 0;
            }

            hinst = LoadLibraryEx(achValue, NULL, DONT_RESOLVE_DLL_REFERENCES);
            if (!hinst)
                continue;

            tbab.hInst = hinst;
            tbab.nID   = idr;
            tbb.iBitmap = SendMessage(s_hwndBar, TB_ADDBITMAP, 1, (WPARAM) &tbab);
            tbb.idCommand = i;
            tbb.fsState = TBSTATE_ENABLED;
            tbb.fsStyle = TBSTYLE_CHECK | TBSTYLE_CHECKGROUP;
            tbb.dwData = 0;
            tbb.iString = 0;
            SendMessage(s_hwndBar, TB_ADDBUTTONS, 1, (LPARAM) &tbb);

            c += 1;

            FreeLibrary(hinst);
        }
    }

    SendMessage(s_hwndBar, TB_SETROWS, MAKEWPARAM((c + 2) / 3, TRUE), (LPARAM)&rcBar);

    rc = rcBar;

    AdjustWindowRectEx(&rc,
            WS_OVERLAPPEDWINDOW,
            FALSE,
            WS_EX_TOOLWINDOW);

    SetWindowPos(s_hwndFrame,
            NULL,
            0, 0,
            rc.right - rc.left,
            rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(s_hwndBar,
            NULL,
            0, 0,
            rcBar.right - rcBar.left,
            rcBar.bottom - rcBar.top,
            SWP_NOZORDER | SWP_NOACTIVATE);

    s_fInitialized = TRUE;

    return TRUE;
}

void
DeinitControlPalette()
{
    if (s_hwndFrame)
    {
        DestroyWindow(s_hwndFrame);
    }
}

BOOL
IsControlPaletteVisible()
{
    return s_hwndFrame && IsWindowVisible(s_hwndFrame);
}

void
ToggleControlPaletteVisibility()
{
    if (!s_fInitialized)
    {
        if (!InitControlPalette())
            return;
    }

    ShowWindow(s_hwndFrame, IsControlPaletteVisible() ? SW_HIDE : SW_SHOW);
}

void
SetControlPaletteOwner(HWND hwnd)
{
    s_hwndOwner = hwnd;
}

HRESULT
GetControlPaletteService(REFIID iid, void **ppv)
{
    HRESULT hr;
    CControlPaletteService *pService;

    pService = new CControlPaletteService();
    if (pService == NULL)
        RRETURN(E_OUTOFMEMORY);

    hr = THR(pService->QueryInterface(iid, ppv));

    pService->Release();

    RRETURN(hr);
}
