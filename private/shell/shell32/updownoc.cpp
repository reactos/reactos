#include "shellprv.h"
#include "updownoc.h"

#define SUBCLASS_COOKIE         4876

CUpDownOC::CUpDownOC()
{
    DllAddRef();

    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_fHorizontal);
    _fEnabled = VARIANT_TRUE;        // Default to non-gray.

    InitCommonControls();
    m_bWindowOnly = TRUE;
    m_bEnabled = TRUE;
    m_bRecomposeOnResize = TRUE;
    m_bResizeNatural = TRUE;
}

CUpDownOC::~CUpDownOC()
{
    Str_SetPtrW(&_pwzBuddyID, NULL);

    if (_hwndUpDown)
        ::DestroyWindow(_hwndUpDown);

    DllRelease();
}


// ATL maintainence functions
LRESULT CUpDownOC::_OnPosChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    DWORD dwResult = (DWORD) ::SendMessage(_hwndUpDown, UDM_GETPOS, 0, 0);

    if (EVAL(HIWORD(dwResult))) // Make sure there wasn't an error.
    {
        LONG lValue = LOWORD(dwResult);

        lValue = _Filter(lValue);       // In case the user went outside of the range.
        _SetBuddiesValue(lValue);
    }

    bHandled = TRUE;
    //TraceMsg(TF_CUSTOM1, "in CUpDownOC::_OnDeltaPos() CreateUpDownControl() The control was clicked on.");
    return 1;
}


LONG CUpDownOC::_Filter(LONG lValue)
{
    _GetRange();    // Update _lMinValue and _lMaxValue

    // The range might be reversed
    LONG lMin = ((_lMinValue < _lMaxValue) ? _lMinValue : _lMaxValue);
    LONG lMax = ((_lMaxValue < _lMinValue) ? _lMinValue : _lMaxValue);

    if (lMin > lValue)
        lValue = lMin;

    if (lMax < lValue)
        lValue = lMax;

    return lValue;
}


HRESULT CUpDownOC::_GetRange(void)
{
    if (_hwndUpDown)
    {
        INT nUpper = 0;
        INT nLower = 0;
        DWORD dwRange = (DWORD) ::SendMessage(_hwndUpDown, UDM_GETRANGE32, (WPARAM) &nLower, (LPARAM) &nUpper);

        _lMinValue = nUpper;
        _lMaxValue = nLower;
    }

    return S_OK;
}


LRESULT CUpDownOC::_ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    LRESULT lResult = ::SendMessage(_hwndUpDown, uMsg, wParam, lParam);

    //TraceMsg(TF_CUSTOM1, "in CUpDownOC::_ForwardMessage() forward msg=%d to real window. (lParam=%d, wParam=%d)", uMsg, lParam, wParam);
    bHandled = (lResult ? TRUE : FALSE);
    return lResult;
}


LRESULT CUpDownOC::_OnFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    // CComControlBase::InPlaceActivate() will call us back recursively, so ignore the second call. 
    if (_fInRecursion)
        return 1;       // We already did the set focus.

    _fInRecursion = TRUE;
    if (!m_bUIActive)
        CComControlBase::InPlaceActivate(OLEIVERB_UIACTIVATE);

    LRESULT lResult = ::SendMessage(_hwndUpDown, uMsg, wParam, lParam);
    bHandled = FALSE;
    _fInRecursion = FALSE;
    return 1;
}


LRESULT CUpDownOC::SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData)
{
    if (SUBCLASS_COOKIE == uIdSubclass)
    {
        switch (uMessage)
        {
        case WM_SETFOCUS:
            {
                BOOL fHandled;
                CUpDownOC * pdtp = (CUpDownOC *) dwRefData;

                if (EVAL(pdtp))
                    pdtp->_OnFocus(uMessage, wParam, lParam, fHandled);
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hwnd, CUpDownOC::SubClassWndProc, SUBCLASS_COOKIE);
            break;
        }
    }

    return DefSubclassProc(hwnd, uMessage, wParam, lParam);
}


HWND CUpDownOC::Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID)
{
    HWND hwndParent = NULL;
    HRESULT hr = S_OK;
    CComQIPtr<IOleWindow, &IID_IOleWindow> spOleWindow(m_spClientSite);

    if (EVAL(spOleWindow.p))
        spOleWindow.p->GetWindow(&hwndParent);

    // Please call BryanSt if this happens.
    ASSERT(hwndParent);     // If this happens we need to reparent our selves later.
    if (!m_hWnd)
    {
        DWORD dwStyle = (WS_CHILD | WS_VISIBLE);
        UINT_PTR nID = (UINT_PTR)this;

        if (_fHorizontal)
            dwStyle |= UDS_HORZ;

        ATOM atom = GetWndClassInfo().Register(&m_pfnSuperWindowProc);
        m_hWnd = CWindowImplBase::Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID, atom);

        if (!EVAL(m_hWnd))
        {
            hr = E_FAIL;
            TraceMsg(TF_CUSTOM1, "in CUpDownOC::Create() CreateUpDownControl() failed (%d)", GetLastError());
        }
        else
            hr = _CreateUpDownWindow();

        TraceMsg(TF_CUSTOM1, "in CUpDownOC::Create() new window created.");
    }
    else
        TraceMsg(TF_CUSTOM1, "in CUpDownOC::Create() window already exists.");

    return m_hWnd;
}


HRESULT CUpDownOC::IOleObject_SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = CComControlBase::IOleObject_SetClientSite(pClientSite);

    if (!pClientSite)
        _spBuddyEditbox = NULL; // Release it to break cycle.

    //TraceMsg(TF_CUSTOM1, "in CUpDownOC::IOleObject_SetClientSite() pClientSite=%#08lx.", pClientSite);
    return hr;
}


HRESULT CUpDownOC::_CreateUpDownWindow(void)
{
    DWORD dwStyle = (WS_CHILD | WS_BORDER | WS_VISIBLE); // ? (WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP);
    UINT nID = (UINT)UPDOWN_CONTROL_ID;
    HRESULT hr = S_OK;

    TraceMsg(TF_CUSTOM1, "in CUpDownOC::_CreateUpDownWindow() _hwndUpDown=%#08lx.", _hwndUpDown);
    if (!EVAL(m_hWnd))
        return E_FAIL;

    if (_fHorizontal)
        dwStyle |= UDS_HORZ;

    if (_hwndUpDown)
    {
        // We want to persist the state of the control while we destroy and recreate the window.
        _GetRange();
        _getValue(&_lValue);
        ::DestroyWindow(_hwndUpDown);
    }

    _hwndUpDown = CreateUpDownControl(dwStyle, 0, 0, (m_rcPos.right - m_rcPos.left), (m_rcPos.bottom - m_rcPos.top),
            m_hWnd, nID, HINST_THISDLL, NULL, _lMaxValue, _lMinValue, _lValue);

    if (EVAL(_hwndUpDown))
    {
        // CreateUpDownControl() truncates the max/min values to ints.
        ::SendMessage(_hwndUpDown, UDM_SETRANGE32, (WPARAM)(int)_lMinValue, (LPARAM)(int)_lMaxValue);
        ::EnableWindow(_hwndUpDown, ((VARIANT_TRUE == _fEnabled) ? TRUE : FALSE));     // Set the gray property in case we needed to cache it until the window was created.
        BOOL fSucceeded = SetWindowSubclass(_hwndUpDown, CUpDownOC::SubClassWndProc, SUBCLASS_COOKIE, (ULONG_PTR) this);
        ASSERT(fSucceeded);
    }
    _SetBuddiesValue(_lValue);
    if (!EVAL(_hwndUpDown))
    {
        hr = E_FAIL;
        TraceMsg(TF_CUSTOM1, "in CUpDownOC::_CreateUpDownWindow() CreateUpDownControl() failed (%d)", GetLastError());
    }

    return hr;
}

// IUpDownOC
HRESULT CUpDownOC::get_Value(OUT BSTR * pbstrValue)
{
    HRESULT hr = S_FALSE;

    if (EVAL(pbstrValue))
    {
        LONG lValue;

        *pbstrValue = NULL;
        hr = _getValue(&lValue);
        if (EVAL(S_OK == hr))
        {
            TCHAR szValue[MAX_PATH];

            wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%ld"), lValue);
            *pbstrValue = SysAllocStringT(szValue);
        }
    }

    return hr;
}

HRESULT CUpDownOC::put_Value(IN BSTR bstrValue)
{    
    LONG lValue = 0;
    HRESULT hr = S_FALSE;

    if (bstrValue)    // Treat NULL strings as Zero.
        lValue = StrToIntW(bstrValue);

    return _putValue(lValue);
}

HRESULT CUpDownOC::_getValue(OUT LONG * plValue)
{
    HRESULT hr = S_FALSE;

    if (_hwndUpDown)
    {
        DWORD dwPos = (DWORD) ::SendMessage(_hwndUpDown, UDM_GETPOS, 0, 0);
        ASSERT(_hwndUpDown);
        if (EVAL(plValue && HIWORD(dwPos)))
        {
            hr = S_OK;
            _lValue = *plValue = (LONG) LOWORD(dwPos);
            TraceMsg(TF_CUSTOM1, "in CUpDownOC::_getValue() result is %d.", *plValue);
        }
    }
    else
        *plValue = _lValue;

    return hr;
}

HRESULT CUpDownOC::_putValue(OUT LONG lValue)
{    
    _lValue = _Filter(lValue);

    // If the window 
    if (_hwndUpDown)
        ::SendMessage(_hwndUpDown, UDM_SETPOS, 0, (LPARAM)(int)_lValue);

    _SetBuddiesValue(_lValue);
    TraceMsg(TF_CUSTOM1, "in CUpDownOC::_putValue(%d)", lValue);
    return S_OK;
}

HRESULT CUpDownOC::Range(IN LONG lMinValue, IN LONG lMaxValue)
{
    _lMinValue = lMinValue;
    _lMaxValue = lMaxValue;

    // Yes, so this won't work for signed values
    if (_hwndUpDown)
        ::SendMessage(_hwndUpDown, UDM_SETRANGE32, (WPARAM)(int)lMinValue, (LPARAM)(int)lMaxValue);

    TraceMsg(TF_CUSTOM1, "in CUpDownOC::Range(lMinValue=%d, lMaxValue=%d)", lMinValue, lMaxValue);
    return S_OK;
}



HRESULT CUpDownOC::get_Enabled(OUT VARIANT_BOOL * pfEnabled)
{
    if (!EVAL(pfEnabled))
        return S_FALSE;

    *pfEnabled = _fEnabled;
    return S_OK;
}

HRESULT CUpDownOC::put_Enabled(IN VARIANT_BOOL fEnabled)
{    
    if (_hwndUpDown)
    {
        _fEnabled = fEnabled;
        ::EnableWindow(_hwndUpDown, ((VARIANT_TRUE == _fEnabled) ? TRUE : FALSE));
    }
    else
        _fEnabled = fEnabled;

    return S_OK;
}

// *** IPersistPropertyBag ***
HRESULT CUpDownOC::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr;
    VARIANT var;

    TraceMsg(TF_CUSTOM1, "in CUpDownOC::Load()");

    // BUGBUG/TODO: This property is not read until after the
    //      window is created.
    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"Horizontal", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BOOL)
        _fHorizontal = var.boolVal;

    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"Wrap", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        _fWrap = var.lVal;

    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"ArrowKeys", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        _fArrowKeys = var.lVal;

    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"AlignLeft", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        _fAlignLeft = var.lVal;

    var.vt = VT_BOOL;
    hr = pPropBag->Read(L"NoPropagation", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        _fNoPropagation = var.lVal;


    var.vt = VT_BSTR;
    var.bstrVal = NULL;
    hr = pPropBag->Read(L"Buddy", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_BSTR)
        Str_SetPtrW(&_pwzBuddyID, var.bstrVal);
    else
        Str_SetPtrW(&_pwzBuddyID, NULL);

    var.vt = VT_I4;
    hr = pPropBag->Read(L"MinRange", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        hr = Range(_lMinValue, var.lVal);

    var.vt = VT_I4;
    hr = pPropBag->Read(L"MaxRange", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        hr = Range(var.lVal, _lMaxValue);

    // We are guaranteed that ::Load() comes after ::IOleObject_SetClientSite()
    // so we know m_spClientSite is valid.  _pwzBuddyID just became valid
    // so it's now time to fetch _spBuddyEditbox.
    _GetEditboxBuddy();
    hr = _putValue(_GetBuddiesValue());

    var.vt = VT_I4;
    hr = pPropBag->Read(L"Value", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_I4)
        hr = _putValue(var.lVal);

    var.vt = VT_UI4;
    var.ulVal = NULL;
    hr = pPropBag->Read(L"Enabled", &var, NULL);
    if (SUCCEEDED(hr) && (var.vt==VT_UI4) && (!var.ulVal))
        _fEnabled = VARIANT_FALSE;

    _SetBuddiesValue(_lValue);
    return hr;
}

HRESULT CUpDownOC::_GetEditboxBuddy(void)
{
    if (m_spClientSite)
    {
        CComPtr<IOleContainer> spContainer;

        m_spClientSite->GetContainer(&spContainer);
        if (EVAL(spContainer))
        {
            CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> spHTMLDoc(spContainer);

            if (EVAL(spHTMLDoc))
            {
                CComPtr<IHTMLElementCollection> spElementCollection;
                spHTMLDoc->get_all(&spElementCollection);

                if (EVAL(spElementCollection) && _pwzBuddyID)
                {
                    CComVariant var1 = _pwzBuddyID;
                    CComVariant var2 = 0;
                    CComPtr<IDispatch> spDispatch;

                    spElementCollection->item(var1, var2, &spDispatch);
                    if (spDispatch)
                    {
                        _spBuddyEditbox = NULL;
                        spDispatch->QueryInterface(IID_IHTMLInputTextElement, (void **)&_spBuddyEditbox);
                        TraceMsg(TF_CUSTOM1, "in CUpDownOC::_GetEditboxBuddy() Got control=%#08lx.", _spBuddyEditbox);
                    }
                    else
                        TraceMsg(TF_CUSTOM1, "in CUpDownOC::_GetEditboxBuddy() That control doesn't exist.");
                }
            }
        }
    }
    else
        TraceMsg(TF_CUSTOM1, "in CUpDownOC::_GetEditboxBuddy() We don't have a parent yet.");

    return S_OK;
}


LONG CUpDownOC::_GetBuddiesValue(void)
{
    LONG lResult;

    if (_spBuddyEditbox)
    {
        BSTR bstrValue = NULL;

        if ((S_OK == _spBuddyEditbox->get_value(&bstrValue)) && bstrValue)
        {
            lResult = StrToIntW(bstrValue);
            SysFreeString(bstrValue);
        }
        else
            _getValue(&lResult);
    }
    else
        _getValue(&lResult);

    return lResult;
}


HRESULT CUpDownOC::_SetBuddiesValue(LONG lNewValue)
{
    HRESULT hr = E_FAIL;

    // We will set it later when it's available if we fail now.
    _lValue = lNewValue;

    if (_spBuddyEditbox)
    {
        WCHAR wzNumericalValue[20];
        BSTR bstrValue;

        wsprintfW(wzNumericalValue, L"%ld", lNewValue);
        bstrValue = SysAllocString(wzNumericalValue);
        if (bstrValue)
        {
            hr = _spBuddyEditbox->put_value(bstrValue);
            _spBuddyEditbox->select();
            ASSERT(SUCCEEDED(hr));
            SysFreeString(bstrValue);
        }
    }

    return hr;
}


