//+------------------------------------------------------------------------
//
//  File:       IACCX.CXX
//
//  Contents:   X-Object IAccessible Implementation
//
//  Classes:    COleSite
//
//  Notes:      Notice that some methods are implemented directly here, others
//              are delegated to the contained object. That reflects that the
//              IAccessible interface has a mixture of methods.
//                I believe that the choices of which are to be delegated are
//              correct.
//                You will find implementations of many of these methods in
//              CServer, or in some of its subclasses such are CDoc and
//              CMorphDataControl.
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#include "formkrnl.hxx"
#include "olesite.hxx"
#include "oleacc.h"


DeclareTag(tagIAccessible, "IAccessible", "IAccessible interface calls");


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::NotAChild
//
//  Synopsis:   Our use of IAccessible does not include support for the
//              varChild parameter. We require it to be zero, signalling this
//              object, not a child of this object.
//
//----------------------------------------------------------------------------

HRESULT COleSite::NotAChild( VARIANT * pvarChild )
{
    if (V_VT(pvarChild) != VT_I4 || V_I4(pvarChild) != 0 )
    {
        RRETURN(E_INVALIDARG);
    }
    return S_OK;
}


//+---------------------------------------------------------------------------
//
// Member:      COleSite::accGetTypeInfoCount
//
// Synopsis:    Returns the number of typeinfos available on this object
//
// Arguments:   [pctinfo] - The number of typeinfos
//
//----------------------------------------------------------------------------

HRESULT
COleSite::accGetTypeInfoCount(UINT FAR *pcTinfo)
{
#if !defined(_MAC)
	return DispatchGetTypeInfoCount(pcTinfo);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accGetIDsOfNames
//
// Synopsis:    Returns the ID of the given name
//
// Arguments:   [riid]      - Interface id to interpret names for
//              [rgszNames] - Array of names
//              [cNames]    - Number of names in [rgszNames]
//              [lcid]      - Locale ID to interpret names in
//              [rgdispid]  - Returned array of IDs
//
//----------------------------------------------------------------------------

HRESULT
COleSite::accGetIDsOfNames(
        REFIID      riid,
        OLECHAR **  rgszNames,
        UINT        cNames,
        LCID        lcid,
        DISPID *    rgdispid)
{
#if !defined(_MAC)
    return DispatchGetIDsOfNames(IID_IAccessible, riid, rgszNames, cNames, lcid, rgdispid);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accGetTypeInfo, IDispatch
//
//  Synopsis:   As per IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accGetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** ppTI)
{
#if !defined(_MAC)
    HRESULT hr;
    ITypeLib * pTypeLib = 0;

    const TCHAR * pszOleAcc = _T("OLEACC.DLL");
    hr = THR_NOTRACE(LoadTypeLib(pszOleAcc, &pTypeLib));
    if (hr)
        goto Cleanup;

    // BUGBUG97: This ignores itinfo and lcid.

    hr = pTypeLib->GetTypeInfoOfGuid(IID_IAccessible, ppTI);

Cleanup:
    ReleaseInterface(pTypeLib);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accInvoke, IDispatch
//
//  Synopsis:   As per IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accInvoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
    DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,
    UINT * puArgErr)
{
#if !defined(_MAC)
	HRESULT hr;
    ITypeInfo * pTI;
    IAccessible * pControlAccessible;

    // Segregate the dispids into those that are handled in the x-object
    // versus those sent to the control.

    switch (dispidMember)
    {
    case DISPID_ACC_NAME:
    case DISPID_ACC_HELP:
    case DISPID_ACC_HELPTOPIC:
    case DISPID_ACC_SELECT:
    case DISPID_ACC_LOCATION:
    case DISPID_ACC_NAVIGATE:
        hr = accGetTypeInfo(0, lcid, &pTI);
        if (hr)
            goto Cleanup;

        hr = pTI->Invoke(
                    (IAccessible*)this,
                    dispidMember,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr);

        ReleaseInterface(pTI);
        break;

    default:
        hr = QueryControlInterface(IID_IAccessible, (void **)&pControlAccessible);
        if (S_OK == hr)
        {
            Assert(pControlAccessible);
            hr = pControlAccessible->Invoke(
                        dispidMember,
                        riid,
                        lcid,
                        wFlags,
                        pdispparams,
                        pvarResult,
                        pexcepinfo,
                        puArgErr);
            ReleaseInterface(pControlAccessible);
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        break;
    }

Cleanup:
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accParent
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accParent(IDispatch ** ppdispParent)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accParent"));

    Assert(ppdispParent);

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accParent(ppdispParent);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accChildCount
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accChildCount(long * pChildCount)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accChildCount"));

    Assert(pChildCount);

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accChildCount(pChildCount);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accChild
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accChild(VARIANT varChildIndex, IDispatch ** ppdispChild)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accChild"));

    Assert(ppdispChild);

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accChild(varChildIndex, ppdispChild);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accName
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accName(VARIANT varChild, BSTR* pszName)
{
#if !defined(_MAC)
    TraceTag((tagIAccessible, "COleSite::get_accName"));

    if (!pszName)
        RRETURN(E_POINTER);

    // We don't support child references.  Must only be a reference to us.

    HRESULT hr;
    hr = NotAChild( &varChild );
    if (hr)
        RRETURN(hr);

    // BUGBUG rgardner removed IControl
    //RRETURN(GetName(pszName));
    return DISP_E_MEMBERNOTFOUND;
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accValue
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accValue(VARIANT varChild, BSTR* pszValue)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accValue"));

    Assert(pszValue);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accValue(varChild, pszValue);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accDescription
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accDescription(VARIANT varChild, BSTR FAR* pszDescription)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accDescription"));

    Assert(pszDescription);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accDescription(varChild, pszDescription);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accRole
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accRole"));

    Assert(pvarRole);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accRole(varChild, pvarRole);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accState(VARIANT varChild, VARIANT *pvarState)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accState"));

    Assert(pvarState);

    // We don't support child references.  Must only be a reference to us.

    // BUGBUG97: Investigate whether the state bits are exclusively from
    // the contained object or whether they are also partly from this x-object.
    // What an ill-considered mess.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accState(varChild, pvarState);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accHelp
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
#if !defined(_MAC)
    TraceTag((tagIAccessible, "COleSite::get_accHelp"));

    if (!pszHelp)
        RRETURN(E_POINTER);

    // We don't support child references.  Must only be a reference to us.

    HRESULT hr;
    hr = NotAChild( &varChild );
    if (hr)
        RRETURN(hr);


    RRETURN(get_title(pszHelp));
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accHelpTopic
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic)
{
    TraceTag((tagIAccessible, "COleSite::get_accHelpTopic"));
    return DISP_E_MEMBERNOTFOUND;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accKeyboardShortcut
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accKeyboardShortcut"));

    Assert(pszKeyboardShortcut);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accFocus
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accFocus(VARIANT * pvarFocusChild)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accFocus"));

    Assert(pvarFocusChild);

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accFocus(pvarFocusChild);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accSelection
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accSelection(VARIANT * pvarSelectedChildren)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accSelection"));

    Assert(pvarSelectedChildren);

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accSelection(pvarSelectedChildren);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::get_accDefaultAction
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::get_accDefaultAction"));

    Assert(pszDefaultAction);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->get_accDefaultAction(varChild, pszDefaultAction);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accSelect
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accSelect(long flagsSelect, VARIANT varChild)
{
#if !defined(_MAC)
    HRESULT hr;

    TraceTag((tagIAccessible, "COleSite::accSelect"));

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    // The only action we support is selecting the object itself.
    // The spec defines many more options, but the others were not available
    // in the Feb. implementation.

    if (flagsSelect == SELFLAG_NONE)
    {
        // do nothing
        return S_OK;
    }

    if (flagsSelect == SELFLAG_TAKESELECTION)
    {
        RRETURN(DISP_E_MEMBERNOTFOUND);
    }

    // Modifies selection state.
    //
    // In run mode, we only set focus, we do not support altering the
    // selection.
    //
    // The bit testing below is to only allow bits that we are ready to
    // honor.

    if (!_pDoc->_fDesignMode && !(flagsSelect & ~(SELFLAG_TAKEFOCUS)))
    {
        if (flagsSelect & SELFLAG_TAKEFOCUS)
        {
            hr = THR(focus());
        }
    }
    else if (!(flagsSelect & ~(SELFLAG_ADDSELECTION | SELFLAG_REMOVESELECTION)))
    {
        DWORD       dwSS    = 0;
        CSite *     pSite   = this;
        CSite **    ppSite  = NULL;
        int         c       = 0;

        //  NOTE that ppSite, c, and dwSS are all defaulted to
        //    assume SELFLAG_REMOVESELECTION, so we don't need
        //    to check for that flag

        if (flagsSelect & SELFLAG_ADDSELECTION)
        {
            ppSite = &pSite;
            c = 1;

            dwSS |= SS_KEEPOLDSELECTION;
        }

        hr = THR(_pDoc->_pSiteCurrent->SelectSites(c, ppSite, dwSS));
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

Cleanup:
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accLocation
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
#if !defined(_MAC)
    HRESULT hr;
    POINT   ptTopLeft;
    POINT   ptBottomRight;

    TraceTag((tagIAccessible, "COleSite::accLocation"));

    if (!(pxLeft && pyTop && pcxWidth && pcyHeight))
        RRETURN(E_POINTER);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        RRETURN(hr);

    _pDoc->ScreenFromWindow(&ptTopLeft, *(POINT *)&_rc.left);
    _pDoc->ScreenFromWindow(&ptBottomRight, *(POINT *)&_rc.right);

    *pxLeft     = ptTopLeft.x;
    *pyTop      = ptTopLeft.y;
    *pcxWidth   = ptBottomRight.x - ptTopLeft.x;
    *pcyHeight  = ptBottomRight.y - ptBottomRight.y;

    return S_OK;
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accNavigate
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
#if !defined(_MAC)
    HRESULT     hr;
    CSite *     pSiteNext = NULL;
    IDispatch * pDispatch = NULL;
    NAVIGATE_DIRECTION dir;

    TraceTag((tagIAccessible, "COleSite::accNavigate"));

    if (!pvarEndUpAt)
        RRETURN(E_POINTER);

    // We only support navigating to peers, not children. Peers are
    // signalled by V_VT(&varStart) == VT_EMPTY.

    if (V_VT(&varStart) != VT_EMPTY)
        return DISP_E_MEMBERNOTFOUND;

    // The spec changed from February. Now (April) this method is
    // not supposed to move the focus, only to return the control that focus
    // would move to.
    //
    // I've converted the existing code for up, down, left, and right, but
    // not for first, last, next, or previous.


    switch (navDir)
    {
    case NAVDIR_UP:
        dir = NAVIGATE_UP;
        goto ArrowTo;

    case NAVDIR_DOWN:
        dir = NAVIGATE_DOWN;
        goto ArrowTo;

    case NAVDIR_LEFT:
        dir = NAVIGATE_LEFT;
        goto ArrowTo;

    case NAVDIR_RIGHT:
        dir = NAVIGATE_RIGHT;
ArrowTo:
        // BUGBUG: (anandra) The spec only wants the site that
        // lies in that direction.  We can't just call NavigateSite
        // because that will actually change the site.
#ifdef NEVER
        hr = THR(_pParent->NextControl(dir, this, &pSiteNext, FALSE));
#else
        hr = DISP_E_MEMBERNOTFOUND;
#endif
        break;

    default:
        Assert(0 && "Bad NAVDIR value");
        // fall through

    case NAVDIR_FIRSTCHILD:
    case NAVDIR_LASTCHILD:
    case NAVDIR_NEXT:
    case NAVDIR_PREVIOUS:
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

#ifdef NEVER
    // BUGBUG: see above BUGBUG. The compiler is too damned smart.

    if (FAILED(hr))
        goto Cleanup;

    // Convert the site to an IDispatch, if possible.

    hr = pSiteNext->QueryInterface(IID_IDispatch, (void **)&pDispatch);
    pSiteNext->Release();
    if (hr)
        goto Cleanup;

    V_VT( pvarEndUpAt ) = VT_DISPATCH;
    V_DISPATCH( pvarEndUpAt ) = pDispatch;

Cleanup:
    if (FAILED(hr))
        hr = DISP_E_MEMBERNOTFOUND;
#else
Cleanup:
#endif
    RRETURN(hr);

#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accHitTest
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::accHitTest"));

    Assert(pvarChildAtPoint);

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->accHitTest(xLeft, yTop, pvarChildAtPoint);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::accDoDefaultAction
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::accDoDefaultAction(VARIANT varChild)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::accDoDefaultAction"));

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->accDoDefaultAction(varChild);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::put_accName
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::put_accName(VARIANT varChild, BSTR szName)
{
    TraceTag((tagIAccessible, "COleSite::put_accName"));
    return DISP_E_MEMBERNOTFOUND;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::put_accValue
//
//  Synopsis:   IAccessible Method
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::put_accValue(VARIANT varChild, BSTR pszValue)
{
#if !defined(_MAC)
    HRESULT hr;
    IAccessible * pAccessible = NULL;

    TraceTag((tagIAccessible, "COleSite::put_accValue"));

    Assert(pszValue);

    // We don't support child references.  Must only be a reference to us.

    hr = NotAChild( &varChild );
    if (hr)
        goto Cleanup;

    hr = QueryControlInterface(IID_IAccessible, (void **)&pAccessible);

    if (S_OK == hr)
    {
        Assert(pAccessible);
        hr = pAccessible->put_accValue(varChild, pszValue);
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }


Cleanup:
    ReleaseInterface(pAccessible);
    RRETURN(hr);
#else
    return DISP_E_MEMBERNOTFOUND;
#endif
}
