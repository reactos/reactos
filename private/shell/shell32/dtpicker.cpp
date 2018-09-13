#include "shellprv.h"
#include "dtpicker.h"
#include "docfind.h"   // to use some of the date time manipulation functions.

#define SUBCLASS_COOKIE         436

CDateTimePickerOC::CDateTimePickerOC()
{
    INITCOMMONCONTROLSEX icex;
    DllAddRef();

    // This allocator should have zero inited the memory, so assert the member variables are empty.
    ASSERT(!_hwndDTPicker);

    // We do this so comctl32.dll won't fail to create the DATETIME_PICKER
    // control.
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    _fEnabled = VARIANT_TRUE;        // Default to non-gray.
    // ATL needs these to make our window resize automatically.
    m_bWindowOnly = TRUE;
    m_bEnabled = TRUE;
    m_bRecomposeOnResize = TRUE;
    m_bResizeNatural = TRUE;
}

CDateTimePickerOC::~CDateTimePickerOC()
{
    if (_hwndDTPicker)
        ::DestroyWindow(_hwndDTPicker);

    DllRelease();
}


LRESULT CDateTimePickerOC::_OnSetSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    //  we need to set the size of our child
    ::SetWindowPos(_hwndDTPicker, NULL, 0,0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOMOVE );
    return 1;
}


LRESULT CDateTimePickerOC::_OnSetFocusControl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    ::SetFocus(_hwndDTPicker);
    return 1;
}


LRESULT CDateTimePickerOC::_ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    LRESULT lResult = ::SendMessage(_hwndDTPicker, uMsg,  wParam,  lParam);
    bHandled = (lResult ? TRUE : FALSE);
    return lResult;
}


LRESULT CDateTimePickerOC::_OnSetFocusDatePicker(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!m_bUIActive)
        CComControlBase::InPlaceActivate(OLEIVERB_UIACTIVATE);

    return 1;
}


LRESULT CDateTimePickerOC::SubClassWndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam, WPARAM uIdSubclass, ULONG_PTR dwRefData)
{
    if (SUBCLASS_COOKIE == uIdSubclass)
    {
        switch (uMessage)
        {
        case WM_SETFOCUS:
            {
                CDateTimePickerOC * pdtp = (CDateTimePickerOC *) dwRefData;

                if (EVAL(pdtp))
                    pdtp->_OnSetFocusDatePicker(uMessage, wParam, lParam);
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hwnd, CDateTimePickerOC::SubClassWndProc, SUBCLASS_COOKIE);
            break;
        }
    }

    return DefSubclassProc(hwnd, uMessage, wParam, lParam);
}


#define DTPICKER_WINDOW_STYLE           (WS_BORDER | WS_CHILD | WS_VISIBLE | DTS_SHORTDATECENTURYFORMAT)    // DTS_SHOWNONE

HWND CDateTimePickerOC::Create(HWND hWndParent, RECT& rcPos, LPCTSTR pszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT_PTR nID)
{
    HWND hwndParent = NULL;
    HRESULT hr = S_OK;
    CComQIPtr<IOleWindow, &IID_IOleWindow> spOleWindow(m_spClientSite);

    if (EVAL(spOleWindow))
        spOleWindow.p->GetWindow(&hwndParent);

    // Please call BryanSt if this happens.
    ASSERT(hwndParent);     // If this happens we need to reparent our selves later.
    if (!m_hWnd)
    {
        DWORD dwStyle = (WS_CHILD | WS_VISIBLE);
        UINT_PTR nID = (UINT_PTR)this;

        m_hWnd = CWindowImpl<CDateTimePickerOC>::Create(hWndParent, rcPos, pszWindowName, dwStyle, dwExStyle, nID);
        if (EVAL(m_hWnd))
        {
            _hwndDTPicker = CreateWindowEx(0, DATETIMEPICK_CLASS, TEXT("Comctl32 DateTime Picker"), DTPICKER_WINDOW_STYLE,
                0, 0, (rcPos.right - rcPos.left), (rcPos.bottom - rcPos.top), m_hWnd, NULL, HINST_THISDLL, NULL);
            if (EVAL(_hwndDTPicker))
            {
                ::EnableWindow(_hwndDTPicker, ((VARIANT_TRUE == _fEnabled) ? TRUE : FALSE));     // Set the gray property in case we needed to cache it until the window was created.
                BOOL fSucceeded = SetWindowSubclass(_hwndDTPicker, CDateTimePickerOC::SubClassWndProc, SUBCLASS_COOKIE, (ULONG_PTR) this);
                ASSERT(fSucceeded);

                if (_st.wYear)  // Have we been initialized to some date?
                    DateTime_SetSystemtime(_hwndDTPicker, GDT_VALID, &_st);
            }
        }
        else
            hr = E_FAIL;

        TraceMsg(TF_CUSTOM1, "in CDateTimePickerOC::Create() new window created.");
    }
    else
        TraceMsg(TF_CUSTOM1, "in CDateTimePickerOC::Create() window already exists.");

    return m_hWnd;
}


// *** IPersistPropertyBag ***
HRESULT CDateTimePickerOC::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr = S_OK;
    VARIANT var;

    var.vt = VT_UI4;
    var.ulVal = NULL;
    hr = pPropBag->Read(L"Enabled", &var, NULL);
    if (SUCCEEDED(hr) && (var.vt==VT_UI4) && (!var.ulVal))
        _fEnabled = VARIANT_FALSE;

    return hr;
}
 
// IDateTimePickerOC
HRESULT CDateTimePickerOC::get_DateTime(OUT DATE * pdatetime)
{
    HRESULT hr = S_FALSE;

    if (EVAL(_hwndDTPicker) && EVAL(pdatetime))
    {
        SYSTEMTIME sysTime;
        LRESULT lResult = ::SendMessage(_hwndDTPicker, DTM_GETSYSTEMTIME, NULL, (LPARAM)&sysTime);

        if (EVAL(GDT_VALID == lResult))
        {
            if (EVAL(SUCCEEDED(SystemTimeToVariantTime(&sysTime, pdatetime))))
                hr = S_OK;
        }
    }

    return hr;
}

HRESULT CDateTimePickerOC::put_DateTime(IN DATE datetime)
{    
    HRESULT hr = S_FALSE;

    if (EVAL(_hwndDTPicker))
    {
        SYSTEMTIME sysTime;

        if (EVAL(SUCCEEDED(VariantTimeToSystemTime(datetime, &sysTime))))
        {
            LRESULT lResult = DateTime_SetSystemtime(_hwndDTPicker, GDT_VALID, &sysTime);
            hr = S_OK;
        }
        else
            _st = sysTime;
    }

    return hr;
}


HRESULT CDateTimePickerOC::get_Enabled(OUT VARIANT_BOOL * pfEnabled)
{
    if (!EVAL(pfEnabled))
        return S_FALSE;

    *pfEnabled = _fEnabled;
    return S_OK;
}

HRESULT CDateTimePickerOC::put_Enabled(IN VARIANT_BOOL fEnabled)
{    
    if (_hwndDTPicker)
    {
        _fEnabled = fEnabled;
        ::EnableWindow(_hwndDTPicker, ((VARIANT_TRUE == _fEnabled) ? TRUE : FALSE));
    }
    else
        _fEnabled = fEnabled;

    return S_OK;
}

HRESULT CDateTimePickerOC::Reset(VARIANT vDelta)
{
    VARIANT vNum;
    SYSTEMTIME st;
    int nDays = 0;

    VariantInit(&vNum);
    if (SUCCEEDED(VariantChangeType(&vNum, &vDelta, 0, VT_I4))) 
        nDays = vNum.iVal;
    VariantClear(&vNum);

    WORD wDate = DocFind_GetTodaysDosDateMinusNDays(nDays);
    DocFind_DosDateToSystemtime(wDate, &st);
    if (_hwndDTPicker)
        DateTime_SetSystemtime(_hwndDTPicker, GDT_VALID, &st);
    else
        _st = st;   // save away until later...

    return S_OK;
}
