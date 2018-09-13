//+---------------------------------------------------------------------------
//
//   File:      oleclnt.cxx
//
//  Contents:   True client site to embeddings.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_VBCURSOR_OCDB_H_
#define X_VBCURSOR_OCDB_H_
#include <vbcursor/ocdb.h>
#endif

#ifndef X_VBCURSOR_VBDSC_H_
#define X_VBCURSOR_VBDSC_H_
#include <vbcursor/vbdsc.h>
#endif

#ifndef X_VBCURSOR_OLEBIND_H_
#define X_VBCURSOR_OLEBIND_H_
#include <vbcursor/olebind.h>
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#include <msdatsrc.h>
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

PerfDbgExtern(tagDocPaint)
DeclareTagOther(tagOleSiteClientNoWindowless, "OleSite", "Disable windowless inplace activation")
DeclareTag(tagOleSiteClient, "OleSite", "OleSiteClient methods");
ExternTag(tagOleSiteRect);

// to make all QI_TEAROFFs directed to global function CreateTearOffThunk instead of attempt
// to use COleSite::CreateTearOffThunk inherited from CElement
#define CreateTearOffThunk ::CreateTearOffThunk

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IOleInPlaceSiteWindowless)
    // IOleWindow
    TEAROFF_METHOD_SUB(COleSite, CClient, GetWindow, getwindow, (HWND *phwnd))
    TEAROFF_METHOD_SUB(COleSite, CClient, ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode))
    // IOleInPlaceSite
    TEAROFF_METHOD_SUB(COleSite, CClient, CanInPlaceActivate, caninplaceactivate, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, OnInPlaceActivate, oninplaceactivate, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, OnUIActivate, onuiactivate, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, GetWindowContext, getwindowcontext, (IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPOLERECT lprcPosRect, LPOLERECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo))
    TEAROFF_METHOD_SUB(COleSite, CClient, Scroll, scroll, (OLESIZE scrollExtant))
    TEAROFF_METHOD_SUB(COleSite, CClient, OnUIDeactivate, onuideactivate, (BOOL fUndoable))
    TEAROFF_METHOD_SUB(COleSite, CClient, OnInPlaceDeactivate, oninplacedeactivate, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, DiscardUndoState, discardundostate, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, DeactivateAndUndo, deactivateandundo, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, OnPosRectChange, onposrectchange, (LPCOLERECT lprcPosRect))
    // IOleInPlaceSiteEx
    TEAROFF_METHOD_SUB(COleSite, CClient, OnInPlaceActivateEx, oninplaceactivateex, (BOOL *, DWORD))
    TEAROFF_METHOD_SUB(COleSite, CClient, OnInPlaceDeactivateEx, oninplacedeactivateex, (BOOL))
    TEAROFF_METHOD_SUB(COleSite, CClient, RequestUIActivate, requestuiactivate, ())
    // IOleInPlaceSiteWindowless
    TEAROFF_METHOD_SUB(COleSite, CClient, CanWindowlessActivate, canwindowlessactivate, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, GetCapture, getcapture, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, SetCapture, setcapture, (BOOL fCapture))
    TEAROFF_METHOD_SUB(COleSite, CClient, GetFocus, getfocus, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, SetFocus, setfocus, (BOOL fFocus))
    TEAROFF_METHOD_SUB(COleSite, CClient, GetDC, getdc, (LPCRECT prc, DWORD grfFlags, HDC *phDC))
    TEAROFF_METHOD_SUB(COleSite, CClient, ReleaseDC, releasedc, (HDC hdc))
    TEAROFF_METHOD_SUB(COleSite, CClient, InvalidateRect, invalidaterect, (LPCRECT prc, BOOL fErase))
    TEAROFF_METHOD_SUB(COleSite, CClient, InvalidateRgn, invalidatergn, (HRGN hrgn, BOOL fErase))
    TEAROFF_METHOD_SUB(COleSite, CClient, ScrollRect, scrollrect, (int dx, int dy, LPCRECT prcScroll, LPCRECT prcClip))
    TEAROFF_METHOD_SUB(COleSite, CClient, AdjustRect, adjustrect, (LPRECT prc))
    TEAROFF_METHOD_SUB(COleSite, CClient, OnDefWindowMessage, ondefwindowmessage, (UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IOleControlSite)
    TEAROFF_METHOD_SUB(COleSite, CClient, OnControlInfoChanged, oncontrolinfochanged, ())
    TEAROFF_METHOD_SUB(COleSite, CClient, LockInPlaceActive, lockinplaceactive, (BOOL fLock))
    TEAROFF_METHOD_SUB(COleSite, CClient, GetExtendedControl, getextendedcontrol, (IDispatch ** ppDisp))
    TEAROFF_METHOD_SUB(COleSite, CClient, TransformCoords, transformcoords, (POINTL * lpptlHimetric, POINTF * lpptfContainer, DWORD flags))
    TEAROFF_METHOD_SUB(COleSite, CClient, TranslateAccelerator, translateaccelerator, (LPMSG lpMsg, DWORD grfModifiers))
    TEAROFF_METHOD_SUB(COleSite, CClient, OnFocus, onfocus, (BOOL fGotFocus))
    TEAROFF_METHOD_SUB(COleSite, CClient, ShowPropertyFrame, showpropertyframe, ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IDispatch)
    TEAROFF_METHOD_SUB(COleSite, CClient, GetTypeInfoCount, gettypeinfocount, (UINT * pctinfo))
    TEAROFF_METHOD_SUB(COleSite, CClient, GetTypeInfo, gettypeinfo, (UINT, ULONG, ITypeInfo**))
    TEAROFF_METHOD_SUB(COleSite, CClient, GetIDsOfNames, getidsofnames, (REFIID riid, LPTSTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid))
    TEAROFF_METHOD_SUB(COleSite, CClient, Invoke, invoke, (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IOleCommandTarget)
    TEAROFF_METHOD_SUB(COleSite, CClient, QueryStatus, querystatus, (GUID *pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT *pcmdText))
    TEAROFF_METHOD_SUB(COleSite, CClient, Exec, exec, (GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IServiceProvider)
    TEAROFF_METHOD_SUB(COleSite, CClient, QueryService, queryservice, (REFGUID guidService, REFIID iid, void ** ppv))
END_TEAROFF_TABLE()

#ifndef NO_DATABINDING
BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IBoundObjectSite)
    TEAROFF_METHOD_SUB(COleSite, CClient, GetCursor, getcursor, (DISPID dispid, ICursor **ppCursor, LPVOID FAR* ppcidOut))
END_TEAROFF_TABLE()
#endif // ndef NO_DATABINDING

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, IBindHost)
    TEAROFF_METHOD_SUB(COleSite, CClient, CreateMoniker, createmoniker,
        (LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved))
    TEAROFF_METHOD_SUB(COleSite, CClient, MonikerBindToStorage, monikerbindtostorage,
        (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
    TEAROFF_METHOD_SUB(COleSite, CClient, MonikerBindToObject, monikerbindtoobject,
        (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(COleSite, CClient, ISecureUrlHost)
    TEAROFF_METHOD_SUB(COleSite, CClient, ValidateSecureUrl, validatesecureurl,
        (BOOL* fAllow, OLECHAR* pchUrlInQuestion, DWORD dwFlags))
END_TEAROFF_TABLE()

EXTERN_C const IID IID_IHTMLDialog;

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::AddRef, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

ULONG
COleSite::CClient::AddRef()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::AddRef SSN=0x%x ulRefs=%ld", MyOleSite()->_ulSSN, ++_ulRefsLocal));

    return MyOleSite()->SubAddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::Release, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

ULONG
COleSite::CClient::Release()
{
    Assert(_ulRefsLocal > 0);
    TraceTag((tagOleSiteClient, "COleSite::CClient::Release SSN=0x%x ulRefs=%ld", MyOleSite()->_ulSSN, --_ulRefsLocal));

    return MyOleSite()->SubRelease();
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::QueryInterface, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::CClient::QueryInterface(REFIID iid, void **ppv)
{
    if (MyOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

#if DBG==1
    char * pszTearoff = NULL, * pszInherit = NULL;
    if (iid == IID_IUnknown)                        pszInherit = "IUnknown";
    else if (iid == IID_IAdviseSinkEx)              pszInherit = "IAdviseSinkEx";
    else if (iid == IID_IAdviseSink)                pszInherit = "IAdviseSink";
    else if (iid == IID_IOleClientSite)             pszInherit = "IOleClientSite)";
    else if (iid == IID_IPropertyNotifySink)        pszInherit = "IPropertyNotifySink";
    else if (iid == IID_IDispatch)                  pszTearoff = "IDispatch";
    else if (iid == IID_IOleControlSite)            pszTearoff = "IOleControlSite";
    else if (iid == IID_IOleWindow)                 pszTearoff = "IOleWindow";
    else if (iid == IID_IOleInPlaceSite)            pszTearoff = "IOleInPlaceSite";
    else if (iid == IID_IOleInPlaceSiteWindowless)  pszTearoff = "IOleInPlaceSiteWindowless";
    else if (iid == IID_IOleInPlaceSiteEx)          pszTearoff = "IOleInPlaceSiteEx";
    else if (iid == IID_IOleCommandTarget)          pszTearoff = "IOleCommandTarget";
    else if (iid == IID_IServiceProvider)           pszTearoff = "IServiceProvider";
    else if (iid == IID_IBindHost)                  pszTearoff = "IBindHost";
    else if (iid == IID_IBoundObjectSite)           pszTearoff = "IBoundObjectSite";
    else if (iid == IID_ISecureUrlHost)             pszTearoff = "ISecureUrlHost";
    else                                            pszInherit = "(Unknown IID)";
    TraceTag((tagOleSiteClient, "COleSite::CClient::QueryInterface %s %s%s",
              pszTearoff ? "(Tearoff)" : "(Inherit)", pszTearoff ? pszTearoff : "",
              pszInherit ? pszInherit : ""));
#endif

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *) this, IUnknown)
#if 0 // IEUNIX
        QI_INHERITS2(this, IUnknown, IOleClientSite) // IEUNIX don't cast to IPrivateUnknown, because we don't implement that
#endif
        QI_INHERITS(this, IAdviseSinkEx)
        QI_INHERITS2(this, IAdviseSink, IAdviseSinkEx)
        QI_INHERITS2(this, IAdviseSink2, IAdviseSinkEx)
        QI_INHERITS(this, IOleClientSite)
#ifndef NO_DATABINDING
        QI_INHERITS(this, IPropertyNotifySink)
#endif // ndef NO_DATABINDING
        QI_TEAROFF(this, IDispatch, (IOleClientSite *)this)
        QI_TEAROFF(this, IOleControlSite, (IOleClientSite *)this)
        QI_TEAROFF2(this, IOleWindow, IOleInPlaceSiteWindowless, (IOleClientSite *)this)
        QI_TEAROFF2(this, IOleInPlaceSite, IOleInPlaceSiteWindowless, (IOleClientSite *)this)
        QI_TEAROFF(this, IOleInPlaceSiteWindowless, (IOleClientSite *)this)
        QI_TEAROFF2(this, IOleInPlaceSiteEx, IOleInPlaceSiteWindowless, (IOleClientSite *)this)
        QI_TEAROFF(this, IOleCommandTarget, (IOleClientSite *)this)
        QI_TEAROFF(this, IServiceProvider, (IOleClientSite *)this)
        QI_TEAROFF(this, IBindHost, (IOleClientSite*)this)
#ifndef NO_DATABINDING
        QI_TEAROFF(this, IBoundObjectSite, (IOleClientSite *)this)
#endif // ndef NO_DATABINDING
        QI_TEAROFF(this, ISecureUrlHost, (IOleClientSite *)this)
    }

    if (!*ppv)
    {
        TraceTag((tagOleSiteClient, "COleSite::CClient::QueryInterface --> E_NOINTERFACE"));
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    (*(IUnknown **)ppv)->AddRef();

    TraceTag((tagOleSiteClient, "COleSite::CClient::QueryInterface --> %08lX", *ppv));

#if DBG==1
    if (pszTearoff)
    {
        MemSetName((*ppv, "COleSite::CClient::QI %s", pszTearoff));
    }
#endif

    DbgTrackItf(iid, "CClient", FALSE, ppv);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::SaveObject, IOleClientSite
//
//  Synopsis:   Object is asking us to save it
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::SaveObject()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::SaveObject SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetMoniker, IOleClientSite
//
//  Synopsis:   Answer the specified moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetMoniker SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    HRESULT hr;

    switch (dwWhichMoniker)
    {
    case OLEWHICHMK_CONTAINER:
        hr = THR_NOTRACE(Doc()->GetMoniker(dwAssign, OLEWHICHMK_OBJFULL, ppmk));
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    RRETURN_NOTRACE(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetContainer, IOleClientSite
//
//  Synopsis:   Answer pointer to IOleContainer (if supported)
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    HRESULT hr;

    TraceTag((tagOleSiteClient, "COleSite::CClient::GetContainer SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED | VALIDATE_DOC_ALIVE))
        RRETURN(E_UNEXPECTED);

    hr = THR(Doc()->QueryInterface(IID_IOleContainer, (void **)ppContainer));
    DbgTrackItf(IID_IOleContainer, "GetContainer", TRUE, (void **)ppContainer);

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::ShowObject, IOleClientSite
//
//  Synopsis:   Asks container to show itself (and object at this site)
//
//  Notes:  This method is called during the binding process to
//          a link source. The code was launched either:
//              a) with /Embedding, in which case it will
//                 be waiting to show itself and this or DoVerb(SHOW)
//                 will be the ocassion for doing so
//              b) without /Embedding, in which case it will have registered
//                 via CoRegisterClassObject and will already be
//                 visible: this method now means "bring the link
//                 source into view".
//
//          If this object is in turn embedded in another container,
//          it should (recursively) call this same method on it's
//          client site...
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::ShowObject()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::ShowObject SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

#ifdef WISH_THAT_ACTIVEX_CTLS_WERENT_SO_STUPID
    //
    // The webbrowser oc as well as the marcwan (basectl) framework
    // call ShowObject while inplace-activating.  This makes no sense
    // for ocx's to do this at this juncture so bail out immediately.
    //
    // Also, bail out right now if the ocx is not inplace yet.  The ppt
    // viewer ocx extraordinaire calls ShowObject in the middle of
    // SetClientSite!  We don't have any code anyway for inside-out
    // inplace activation.
    //
    // IE3 didn't implement this method anyway, so there should be
    // no ill effects here. (anandra)
    //

    if (MyOleSite()->TestLock(COleSite::OLESITELOCK_INPLACEACTIVATE) ||
        MyOleSite()->State() < OS_INPLACE)
    {
        return S_OK;
    }

    //
    // If we're scrolling then we better not be in the middle of sizing
    // or positioning.  I.e. before we can scroll we need to know how big
    // and where we are!!
    //

    Assert(!MyOleSite()->TestLock(ELEMENTLOCK_SIZING) &&
           !MyOleSite()->TestLock(ELEMENTLOCK_POSITIONING));

    RRETURN(MyOleSite()->ScrollIntoView());
#else
    return S_OK;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnShowWindow, IOleClientSite
//
//  Synopsis:   Informs container (us) about object Show/Hide events
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnShowWindow(BOOL fShow)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnShowWindow SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    OLE_SERVER_STATE stateNew = MyOleSite()->State();

    if (fShow)
    {
        stateNew = OS_OPEN;
    }
    else if (MyOleSite()->_state > OS_RUNNING)
    {
        stateNew = OS_RUNNING;
    }

    if (MyOleSite()->State() != stateNew)
    {
        MyOleSite()->_state = stateNew;
        MyOleSite()->GetCurLayout()->Invalidate();
    }
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::RequestNewObjectLayout, IOleClientSite
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::RequestNewObjectLayout( )
{
    HRESULT     hr;
    IOleObject *pOleObject = NULL;
    SIZEL       sizel;
    SIZE        size;
    CRect       rcBounds;
    CRect       rcClient;

// If we are in the midst of recalculating the size, ignore this call.
    if (MyOleSite()->CElement::TestLock(ELEMENTLOCK_RECALC))
        return S_OK;

    COleLayout * pLayout = DYNCAST(COleLayout, MyOleSite()->GetCurLayout());
    Assert(pLayout);

    TraceTag((tagOleSiteClient, "COleSite::CClient::RequestNewObjectLayout SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // Ask the object how big it wants to be and stuff the info into
    // the width/height attributes.
    //

    hr = THR_NOTRACE(MyOleSite()->QueryControlInterface(IID_IOleObject, (void **)&pOleObject));
    if (hr)
        goto Cleanup;

    //
    // Transition to the running state because GetExtent can fail
    // if not OS_RUNNING.
    //

    hr = THR(MyOleSite()->TransitionTo(OS_RUNNING));
    if (hr)
        goto Cleanup;

    hr = THR_OLE(pOleObject->GetExtent(DVASPECT_CONTENT, &sizel));
    if (hr)
        goto Cleanup;

    Doc()->_dci.DeviceFromHimetric(&size, sizel);

    pLayout->GetRect(&rcBounds);
    pLayout->GetClientRect(&rcClient, COORDSYS_PARENT);

    rcBounds.right  = rcBounds.left + (rcBounds.Width()  - rcClient.Width())  + size.cx;
    rcBounds.bottom = rcBounds.top  + (rcBounds.Height() - rcClient.Height()) + size.cy;

    hr = THR(pLayout->Move(&rcBounds, SITEMOVE_RESIZEONLY));
    if (hr)
        goto Cleanup;

    MyOleSite()->ResizeElement();

Cleanup:
    ReleaseInterface(pOleObject);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetWindow, IOleWindow
//
//  Synopsis:   Answer containing window handle
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetWindow(HWND * phwnd)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetWindow SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED | VALIDATE_DOC_ALIVE))
        RRETURN(E_UNEXPECTED);

    RRETURN(Doc()->GetWindow(phwnd));
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::ContextSensitiveHelp, IOleWindow
//
//  Synopsis:   Notifies enter/exit CSH mode
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::ContextSensitiveHelp SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    RRETURN(Doc()->_pInPlace->_pInPlaceSite->ContextSensitiveHelp(fEnterMode));
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::CanInPlaceActivate, IOleInPlaceSite
//
//  Synopsis:   Answer wether or not object can IPActivate
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::CanInPlaceActivate()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::CanInPlaceActivate SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

    return S_OK;     // we always allow in-place activation
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnInPlaceActivate, IOleInPlaceSite
//
//  Synopsis:   Notifies container (us) of object activation
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnInPlaceActivate()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnInPlaceActivate SSN=0x%x", MyOleSite()->_ulSSN));

    RRETURN(THR_OLEO(OnInPlaceActivateEx(NULL, 0),MyOleSite()));
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnInPlaceActivateEx, IOleInPlaceSiteEx
//
//  Synopsis:   Notifies container (us) of object activation
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnInPlaceActivateEx(BOOL *pfNoRedraw, DWORD dwFlags)
{
    HRESULT hr;
    RECT    rcUpdate;
    RECT    rcSite;
    CDoc *  pDoc = Doc();

    TraceTag((tagOleSiteClient, "COleSite::CClient::OnInPlaceActivateEx SSN=0x%x", MyOleSite()->_ulSSN));

    //
    // Don't allow inplace activation if the site is not loaded yet
    // or if the doc is not at least inplace active.
    //

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED | VALIDATE_DOC_INPLACE ))
        RRETURN(E_UNEXPECTED);

    TraceTag((tagCDoc, "COleSite::CClient::OnInPlaceActivate %x", this));

    Assert(MyOleSite()->_pInPlaceObject == 0);

    // handle the above assert failing. If this site is already inplace, return S_OK.
    // This situation should never happen, but Adobe's Acrobat control does this.
    if (MyOleSite()->_pInPlaceObject != 0)
    {
        Assert(MyOleSite()->State() >= OS_INPLACE);
        return S_OK;
    }
        
    if (dwFlags & ACTIVATE_WINDOWLESS)
    {
        hr = THR_OLEO(MyOleSite()->QueryControlInterface(
                IID_IOleInPlaceObjectWindowless,
                (void **) &MyOleSite()->_pInPlaceObject),MyOleSite());
        if (hr)
            goto Cleanup;

        MyOleSite()->_fWindowlessInplace = TRUE;
        MyOleSite()->_fUseInPlaceObjectWindowless = TRUE;
    }
    // CHROME
    // Chrome hosted and windowless then we can't support
    // windowed controls (as are, ourselves, windowless). Therefore,
    // don't attempt to query for a windowed object. Just fail
    // in that case.
    else if (!pDoc->IsChromeHosted())
    {
        hr = THR_OLEO(MyOleSite()->QueryControlInterface(
                IID_IOleInPlaceObject,
                (void **) &MyOleSite()->_pInPlaceObject),MyOleSite());
        if (hr)
            goto Cleanup;
    }
    else
    {
        // Chrome hosted and windowless but the control does not
        // support windowless operation.
        // Here is the steps taken to disable the ocx:
        //     1.  Clear cached IDispatch pointer for control
        //     2.  Clear cached IViewObject(Ex) pointer for control
        //     3.  revoke the client site so ocx cannot call on site
        //     4.  Release the pointer to the control instance
        //     5.  set the state to OS_LOADED
        //     6.  Set the _fNoUIActivate flag so this ocx does not get
        //         UI activated.
        //     7.  Call OnFailToCreate() which will move the object into
        //         the disfuntional object array.

        ClearInterface(&MyOleSite()->_pDisp);
        MyOleSite()->_fDispatchCached = false;
        ClearInterface(&MyOleSite()->_pVO);
        MyOleSite()->_fUseViewObjectEx = false;

        MyOleSite()->SetClientSite(NULL);
        ClearInterface(&MyOleSite()->_pUnkCtrl);

        MyOleSite()->_state = OS_LOADED;
        CLayout *pLayout = DYNCAST(CLayout, MyOleSite()->GetCurLayout());
        Assert(pLayout);
        pLayout->_fNoUIActivate = false;
        MyOleSite()->OnFailToCreate();
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    MyOleSite()->_state = OS_INPLACE;

    // Adjust z-order of all HWNDs within the document and transparency of the associated display node (if any)

    if (!pDoc->_fInPlaceActivating && !MyOleSite()->_fWindowlessInplace)
    {
        HWND hwndPar = pDoc->_pInPlace->_hwnd;
        HWND hwndCtl = MyOleSite()->GetHwnd();
        CLayout *   pLayout   = MyOleSite()->GetCurLayout();
        CDispNode * pDispNode = pLayout
                                    ? pLayout->GetElementDispNode()
                                    : NULL;

        if (hwndCtl)
        {
            if (GetParent(hwndCtl) == hwndPar)
            {
                pDoc->FixZOrder();
            }

            if (    pDispNode
                &&  pDispNode->IsOpaque()
                &&  ::GetWindowLong(hwndCtl, GWL_EXSTYLE) & WS_EX_TRANSPARENT)
            {
                MyOleSite()->ResizeElement();
            }
        }
    }

    if (pfNoRedraw)
    {
        // If the baseline state is OS_INPLACE then IVO::Draw() won't, so we
        // have to have them draw themselves during this activation process.
        *pfNoRedraw = (MyOleSite()->BaselineState(OS_INPLACE) != OS_INPLACE);

        // If the control is windowless, then the normal window invalidation
        // mechanism will take care of getting the bits on the screen correct.
        // If the control has a window, then we need to tell the object if
        // it needs to invalidate or not.

        if (!MyOleSite()->_fWindowlessInplace)
        {
            // If the form's update rect intersects the site, then
            // redraw is required.

            if (GetUpdateRect(pDoc->_pInPlace->_hwnd, &rcUpdate, FALSE))
            {
                MyOleSite()->GetCurLayout()->GetRect(&rcSite, COORDSYS_GLOBAL);
                if (IntersectRect(&rcSite, &rcSite, &rcUpdate))
                {
                    *pfNoRedraw = FALSE;
                }
            }

            if (*pfNoRedraw)
            {
                *pfNoRedraw = !MyOleSite()->ActivationChangeRequiresRedraw();
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnInPlaceDeactivateEx, IOleInPlaceSiteEx
//
//  Synopsis:   Notifies container (us) of object activation
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnInPlaceDeactivateEx(BOOL fNoRedraw)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnInPlaceDeactivateEx SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

    if (!fNoRedraw ||
        (!MyOleSite()->_fWindowlessInplace &&
            MyOleSite()->ActivationChangeRequiresRedraw()))
    {
        MyOleSite()->GetCurLayout()->Invalidate();
    }

    RRETURN(THR(OnInPlaceDeactivate()));
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnInPlaceDeactivate, IOleInPlaceSite
//
//  Synopsis:   Notifies container (us) of object deactivation
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnInPlaceDeactivate()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnInPlaceDeactivate SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

    TraceTag((tagCDoc, "COleSite::CClient::OnInPlaceDeactivate %x", this));

    //
    // If this site is still UIACTIVE fire the OnUIDeactivate event for it.
    // This is protection against ill-behaved objects like WordArt 2.0 which
    // fire OnUIActivate but fail to send OnUIDeactivate if it encounters
    // trouble.  It does send OnInPlaceDeactivate, hence the catch here.
    //

    if (MyOleSite()->_state == OS_UIACTIVE)
    {
        OnUIDeactivate(FALSE);
    }

    // Assert(MyOleSite()->_pInPlaceObject); // (IE5 bug 65988) this is not a valid assert - the control can call OnInPlaceDeactivate any time it wants to, even in the middle of inplace activation)

    ClearInterface(&MyOleSite()->_pInPlaceObject);
    MyOleSite()->_fWindowlessInplace = FALSE;
    MyOleSite()->_fUseInPlaceObjectWindowless = FALSE;
    MyOleSite()->_state = OS_RUNNING;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnUIActivate, IOleInPlaceSite
//
//  Synopsis:   Notifies container (us) of object UIactivation
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnUIActivate()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnUIActivate SSN=0x%x", MyOleSite()->_ulSSN));

    HRESULT     hr = S_OK;
    CLock       Lock(MyOleSite(), OLESITELOCK_TRANSITION);
    INSTANTCLASSINFO * pici = MyOleSite()->GetInstantClassInfo();

    //
    // Do not allow ui-activation if we're positioning the control.
    // The wonderful msn company news ocx does this when we
    // first inplace activate it.
    //

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED | VALIDATE_DOC_INPLACE) ||
        MyOleSite()->State() < OS_RUNNING ||
        (pici && (pici->dwCompatFlags & COMPAT_NO_UIACTIVATE)))
    {
        TraceTag((tagError, "Unexpected call to OnUIActivate!"));
        RRETURN(E_UNEXPECTED);
    }

    hr = Doc()->SetUIActiveElement(MyOleSite());
    if (hr)
        RRETURN(hr);
        
    MyOleSite()->_state = OS_UIACTIVE;
    
    if (!MyOleSite()->_fInBecomeCurrent)
    {
        // if, on the other hand, MyOleSite()->!_fInBecomeCurrent were TRUE,
        // we pretend that this is ui-active.  We will complete
        // the activation process in NTYPE_ELEMENT_SETFOCUS notification
        // once SetCurrentElem() succeeds. If, oin the other hand,
        // SetCurrentElem fails, we will reset the state in
        // NTYPE_ELEMENT_SETFOCUSFAILED notification.

        hr = MyOleSite()->BecomeCurrent(0);
        if (hr)
            hr = E_UNEXPECTED;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnUIDeactivate, IOleInPlaceSite
//
//  Synopsis:   Notifies container (us) of object UIdeactivation
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnUIDeactivate(BOOL fUndoable)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnUIDeactivate SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_INPLACE))
        RRETURN(E_UNEXPECTED);

    HRESULT     hr;
    CDoc *      pDoc = Doc();

    Assert (pDoc); // should be checked by IllegalSiteCall

    if (!pDoc->_pInPlace)  // if the doc has deactived already,
        return S_OK;       // nothing more to do

    Assert (pDoc->_pInPlace);

    // Mark ourselves as no longer UIActive:
    pDoc->_pInPlace->_fChildActive = FALSE;
    MyOleSite()->_state = OS_INPLACE;
    MyOleSite()->TakeCapture(FALSE);

    //
    // If we are the ui-active or current site, make sure
    // we clean up the doc's internal state variables.
    //

    if (pDoc->_pElemUIActive == MyOleSite() &&
        !pDoc->_pInPlace->_fChildActivating)
    {
        //
        // If the doc is not deactivating, just make this
        // site's parent current and active.
        //

        if (!pDoc->_pInPlace->_fDeactivating)
        {
            CElement * pElementPL = MyOleSite()->GetFirstBranch()
                                        ? MyOleSite()->GetUpdatedParentLayoutElement()
                                        : NULL;

            // The callee's expect this to no longer be so...
            pDoc->_pElemUIActive = NULL;

            if(pElementPL)
            {
                pElementPL->BecomeCurrentAndActive(NULL, 0, TRUE);
            }
            else
            {
                IGNORE_HR(pDoc->PrimaryRoot()->BecomeCurrentAndActive());
            }
        }
        else
        {
            //
            // Otherwise the doc is deactivating, so just force
            // the rootsite to become current and active.
            // Don't use the BecomeUIActive method on the rootsite
            // because that could cause certain unpleasant side-effects
            //

            pDoc->_pElemUIActive = pDoc->_pElementDefault;

            IGNORE_HR(pDoc->PrimaryRoot()->BecomeCurrent(0));
        }
    }

    hr = S_OK;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetWindowContext, IOleInplaceSite
//
//  Synopsis:   Answer a description of the IP window context
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetWindowContext(
    IOleInPlaceFrame ** ppFrame,
    IOleInPlaceUIWindow ** ppDoc,
    LPOLERECT prcPosRect,
    LPOLERECT prcClipRect,
    OLEINPLACEFRAMEINFO * pFI)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetWindowContext SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    HRESULT     hr;
    OLERECT     rcT;
    CDoc *      pDoc = Doc();

    COleLayout * pLayout;

    pLayout = DYNCAST(COleLayout, MyOleSite()->GetCurLayout());
    Assert(pLayout);

    if (MyOleSite()->_state < OS_LOADED)
    {
        Assert(0 && "Unexpected call to client site.");
        *ppFrame = NULL;
        *ppDoc = NULL;
        SetRectEmpty(prcPosRect);
        SetRectEmpty(prcClipRect);
        memset(pFI, 0, sizeof(OLEINPLACEFRAMEINFO));
        RRETURN(E_UNEXPECTED);
    }

    *ppFrame = NULL;
    *ppDoc = NULL;

    pFI->cb = sizeof(OLEINPLACEFRAMEINFO);
    hr = THR(pDoc->_pInPlace->_pInPlaceSite->GetWindowContext(
            ppFrame,
            ppDoc,
            &rcT,
            &rcT,
            pFI));
    if (hr)
        goto Cleanup;

    if (*ppFrame)
    {
        ReleaseInterface(*ppFrame);
        *ppFrame = &pDoc->_FakeInPlaceFrame;
        (*ppFrame)->AddRef();
        DbgTrackItf(IID_IOleInPlaceFrame, "FrmWnd", TRUE, (void **)ppFrame);
    }

    if (*ppDoc)
    {
        ReleaseInterface(*ppDoc);
        *ppDoc = &pDoc->_FakeDocUIWindow;
        (*ppDoc)->AddRef();
        DbgTrackItf(IID_IOleInPlaceUIWindow, "DocWnd", TRUE, (void **)ppDoc);
    }

    pLayout->GetClientRect(prcPosRect, COORDSYS_GLOBAL);

#ifdef WIN16
    RECTL rcClipRectl;
    MyOleSite()->GetParentLayout()->GetVisibleClientRect(&rcClipRectl);
    CopyRect(prcClipRect, &rcClipRectl);
#else
    pLayout->GetClippedClientRect((CRect*)prcClipRect, COORDSYS_GLOBAL);
#endif

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::Scroll, IOleInPlaceSite
//
//  Synopsis:   Object wants us to scroll by sizeScroll, which is in
//              pixels.
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::Scroll(OLESIZE sizeScroll)
{
#if 0
// BUGBUG: Fix this! (brendand)
    int             i;
    POINT           ptBefore;
    POINT           ptAfter;
    SIZEL           sizelScroll;
    CTreeNode *     pNodeParent;
    int             cx;
    HRESULT         hr = S_OK;

    TraceTag((tagOleSiteClient, "COleSite::CClient::Scroll SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_INPLACE  | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    for (pNodeParent = MyOleSite()->GetParentLayoutNode();
            pNodeParent && (sizeScroll.cx || sizeScroll.cy);
            pNodeParent = pNodeParent->GetParentLayoutNode())
    {
        ptBefore.x = MyOleSite()->GetCurLayout()->_rc.left;
        ptBefore.y = MyOleSite()->GetCurLayout()->_rc.top;
        pNodeParent->Doc()->DocumentFromWindow(&sizelScroll, sizeScroll);
        hr = THR(pNodeParent->GetCurLayout()->ScrollBy(
                                sizelScroll.cx,
                                sizelScroll.cy,
                                fmScrollActionControlRequest,
                                fmScrollActionControlRequest));
        if (hr)
            goto Cleanup;

        ptAfter.x = MyOleSite()->GetCurLayout()->_rc.left;
        ptAfter.y = MyOleSite()->GetCurLayout()->_rc.top;

        // Calculate the remaining amount to scroll using the positions
        // before and after scrolling. This calculation is tricky because we
        // want to handle the case where the parent site overshoots the scroll
        // amount. This might happen because a datadoc is aligning a row.

        for (i = 0; i < 2; i++)
        {
            cx = (&sizeScroll.cx)[i] + (&ptAfter.x)[i] - (&ptBefore.x)[i];
            if ((&sizeScroll.cx)[i] < 0)
            {
                if (cx > 0)
                    cx = 0;
            }
            else if ((&sizeScroll.cx)[i] > 0)
            {
                if (cx < 0)
                    cx = 0;
            }
            else
            {
                cx = 0;
            }
            (&sizeScroll.cx)[i] = cx;
        }
    }

    if (sizeScroll.cx || sizeScroll.cy)
        hr = THR(Doc()->_pInPlace->_pInPlaceSite->Scroll(sizeScroll));

Cleanup:
    RRETURN(hr);
#else
    RRETURN(S_OK);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::DiscardUndoState, IOleInPlaceSite
//
//  Synopsis:   dump any saved undo state for this object
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::DiscardUndoState()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::DiscardUndoState SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_INPLACE))
        RRETURN(E_UNEXPECTED);

    // SITEBUG -- Undo
    // Doc()->SetUndoState(NULL);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::DeactivateAndUndo, IOleInPlaceSite
//
//  Synopsis:   Object wants us to deactivate it and undo a prior action
//
//  Notes:      The object is asking us to back up, deactivating it
//              and undo (local to our state). We must call UIDeactivate
//              on the object.
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::DeactivateAndUndo()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::DeactivateAndUndo SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_INPLACE))
        RRETURN(E_UNEXPECTED);

    HRESULT     hr;
    CDoc *      pDoc = Doc();
    CDoc::CLock Lock(pDoc);

    pDoc->_fNoUndoActivate = TRUE;

    // BUGBUG:  Need to handle error result if error from SaveData call in
    //          TransitionToBaselineState.
    MyOleSite()->TransitionToBaselineState(OS_UIACTIVE);

    //
    // This will call Undo on the form's site if the first thing the user did
    // after the form was activated was activate something inside it.  This is
    // because the current undo object will be a CUndoActivate in that
    // situation.
    //

#ifdef NO_EDIT
    hr = S_OK;
#else
    hr = THR(pDoc->EditUndo());
#endif // NO_EDIT
    pDoc->ShowLastErrorInfo(hr);

    pDoc->_fNoUndoActivate = FALSE;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnPosRectChange, IOleInPlaceSite
//
//  Synopsis:   object self-move/resize
//
//  Arguments:  prcPos  New Physical Rect
//
//  Notes:      We are being informed that the object wants a new position/size
//              in the document. We should try to be accomodating by laying
//              ourself out differently. This may of course be a recursive
//              process (if we are embedded). When we are done, the InPlace
//              object is notified of the result via SetObjectRects(). The
//              result may be that we have clipped some or all of the new
//              PosRect, or that the new PosRect is zoomed with respect
//              to the request -- in either case the object may choose
//              to deactivate and proceed with an open edit. Notice that
//              this means we may be deactivated if we choose to
//              recursively renegotiate with our InPlaceSite.
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnPosRectChange(LPCOLERECT prcPos)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnPosRectChange SSN=0x%x", MyOleSite()->_ulSSN));

    // If we are in the midst of recalculating the size, ignore this call.
    if (MyOleSite()->CElement::TestLock(CElement::ELEMENTLOCK_RECALC))
        return S_OK;

    if (MyOleSite()->IllegalSiteCall(VALIDATE_INPLACE))
        RRETURN(E_UNEXPECTED);

//BUGBUG:(ferhane)
//          Although the MSDN says that we MUST call SetObjectRects() if we return
//          S_OK, we are not doing that here. This check should only be caught if the
//          control is calling us with this method from inside an IViewobject::Draw(). and
//          that is not allowed anyway.
//

    if (MyOleSite()->IllegalSiteCall(VALIDATE_NOTRENDERING))
    {
        if (!_prcPending)
        {
            _prcPending = new CRect(*prcPos);

            IGNORE_HR(GWPostMethodCall(this, 
		        ONCALL_METHOD(COleSite::CClient, DeferredOnPosRectChange, deferredonposrectchange), 
		        0, 
		        TRUE, 
		        "COleSite::CClient::DeferredOnPosRectChange"));
        }

        // we have to return here instead of going to the cleanup,
        // since cleanup frees the _prcPending and resets it.
        RRETURN(E_UNEXPECTED);
    }

    HRESULT hr  = S_OK;
    OLERECT rcPos;
    RECT    rcClip;

// BUGBUG: Fix this (brendand)
    rcClip = g_Zero.rc;

    COleLayout * pLayout = DYNCAST(COleLayout, MyOleSite()->GetCurLayout());
    Assert(pLayout);
    
    //
    // If we are in the middle of COleSite::Move (_fSettingExtent == TRUE),
    // then we ignore the argument (prcPos) and tell the control to move
    // where COleSite::Move was told to move the control.  This protects
    // against calls to OnPosRectChange from the CDK's implementation of
    // IOleObject::SetExtent.
    //

    if (MyOleSite()->TestLock(COleSite::OLESITELOCK_SETEXTENT))
    {
        SIZE    size;

        Doc()->DeviceFromHimetric(&size, pLayout->_sizelLast);
        pLayout->GetClientRect(&rcPos, COORDSYS_GLOBAL);
        rcPos.right  = rcPos.left + size.cx;
        rcPos.bottom = rcPos.top + size.cy;

        TraceTag((
            tagOleSiteRect,
            "COleSite::Client::OnPosRectChange: SSN=%x rcPosIn=l=%ld t=%ld r=%ld b=%ld SOR=l=%ld t=%ld r=%ld b=%ld ",
            MyOleSite()->_ulSSN,
            prcPos->left, prcPos->top, prcPos->right, prcPos->bottom,
            rcPos.left, rcPos.top, rcPos.right, rcPos.bottom));

        hr = THR_OLEO(MyOleSite()->_pInPlaceObject->SetObjectRects(&rcPos, ENSUREOLERECT(&rcClip)),MyOleSite());
        if (hr)
            goto Cleanup;
    }
    else
    {
        BOOL            fLayoutOnly = FALSE;
        BOOL            fStatic = TRUE;
        CRect           rcPosClient;

        pLayout->GetClientRect(&rcPosClient, COORDSYS_GLOBAL);

        //
        // If the site is position is absolute or relative we will allow
        // changes to the location as well as size.  Otherwise only changes
        // to the size are allowed.
        //

        if (!MyOleSite()->IsPositionStatic())
        {
            fStatic = FALSE;
            rcPos = *prcPos;
        }
        else
        {
            rcPos = rcPosClient;
            rcPos.right = rcPos.left + prcPos->right - prcPos->left;
            rcPos.bottom = rcPos.top + prcPos->bottom - prcPos->top;
        }

        //
        // OPTIMIZATION:  If the new size is the same as the old size, only
        //   a relayout needs to be done.  Otherwise a full resizing
        //   a relayout is necessary.
        //
        if (    (rcPos.right - rcPos.left == rcPosClient.Width())
            &&  (rcPos.bottom - rcPos.top == rcPosClient.Height()))
        {
            fLayoutOnly = TRUE;
        }

        hr = THR_OLEO(MyOleSite()->_pInPlaceObject->SetObjectRects(
                &rcPos,
                ENSUREOLERECT(&rcClip)),
                MyOleSite());
        if (hr)
            goto Cleanup;

        // convert the rc to parent content relative coordinates
        pLayout->TransformRect(&rcPos, COORDSYS_GLOBAL, COORDSYS_PARENT);

        hr = THR(pLayout->Move(&rcPos, SITEMOVE_NOFIREEVENT));
        if (hr)
            goto Cleanup;

        TraceTag((
            tagOleSiteRect,
            "COleSite::Client::OnPosRectChange: SSN=%x \n\t rcPosIn=l=%ld t=%ld r=%ld b=%ld SOR=l=%ld t=%ld r=%ld b=%ld",
            MyOleSite()->_ulSSN,
            prcPos->left, prcPos->top, prcPos->right, prcPos->bottom,
            rcPos.left, rcPos.top, rcPos.right, rcPos.bottom));

        hr = THR(MyOleSite()->OnPropertyChange(
                DISPID_UNKNOWN,
                ELEMCHNG_CLEARCACHES));
        if (hr)
            goto Cleanup;

        MyOleSite()->ResizeElement();
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnDataChange, IAdviseSink
//
//  Synopsis:   Data-changed event
//
//  Notes:      the memory pointed to by pStgmed is owned by the caller,
//              it must not be free's here (but can of course be copied).
//
//+---------------------------------------------------------------------------

void
COleSite::CClient::OnDataChange(FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnDataChange SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        return;

    // Ignore calls from DataDoc instances.  Because they are
    // never saved, they can't be dirty.

    MyOleSite()->_fDirty = TRUE;
    Doc()->OnDataChange(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnViewChange, IAdviseSink
//
//  Synopsis:   View-changed event
//
//+---------------------------------------------------------------------------

void
COleSite::CClient::OnViewChange(DWORD dwAspects, LONG lindex)
{
    COleLayout * pLayout = DYNCAST(COleLayout, MyOleSite()->GetCurLayout());
    Assert(pLayout);

    TraceTag((tagOleSiteClient, "COleSite::CClient::OnViewChange SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED) || 
        MyOleSite()->IllegalSiteCall(VALIDATE_NOTRENDERING))
        return;

    if ((dwAspects & DVASPECT_CONTENT) && Doc()->_state >= OS_INPLACE)
    {
        if (MyOleSite()->_state < OS_INPLACE)
        {
            pLayout->Invalidate();
        }
    }

    // We only InvalidateColors is the object
    // doesn't know how to use SHDVID_ONCOLORSCHANGE
    //
    if (!MyOleSite()->_fCanDoShColorsChange)
            MyOleSite()->Doc()->InvalidateColors();

    Doc()->OnViewChange(dwAspects);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnRename, IAdviseSink
//
//  Synopsis:   Linking clients need to update source monikers.
//
//+---------------------------------------------------------------------------

void
COleSite::CClient::OnRename(LPMONIKER pmk)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnRenameSSN=0x%x", MyOleSite()->_ulSSN));
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnSave, IAdviseSink
//
//  Synopsis:   Object-saved event
//
//+---------------------------------------------------------------------------

void
COleSite::CClient::OnSave()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnSave SSN=0x%x", MyOleSite()->_ulSSN));
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnClose, IAdviseSink
//
//  Synopsis:   Object-closed event
//
//+---------------------------------------------------------------------------

void
COleSite::CClient::OnClose()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnClose SSN=0x%x", MyOleSite()->_ulSSN));
}


//+---------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnLinkSrcChange, IAdviseSink2
//
//  Synopsis:   Object-closed event
//
//---------------------------------------------------------------
void
COleSite::CClient::OnLinkSrcChange(IMoniker * pmk)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnLinkSrcChange SSN=0x%x", MyOleSite()->_ulSSN));
}


//+---------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnViewStatusChange, IAdviseSinkEx
//
//  Synopsis:   View Status flags has changed
//
//---------------------------------------------------------------
void
COleSite::CClient::OnViewStatusChange(DWORD dwViewStatus)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnViewStatusChange SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        return;

    MyOleSite()->SetViewStatusFlags(dwViewStatus);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::CanWindowlessActivate, IOleInPlaceSiteWindowless
//
//  Synopsis:   object is asking if it can in-place activate without a window
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::CanWindowlessActivate()
{
    INSTANTCLASSINFO * pici;
    
    TraceTag((tagOleSiteClient, "COleSite::CClient::CanWindowlessActivate SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

#if DBG==1
    if (IsTagEnabled(tagOleSiteClientNoWindowless))
        return S_FALSE;
#endif
    pici = MyOleSite()->GetInstantClassInfo();

    if (!pici)
        return S_FALSE;
        
    return (pici->dwCompatFlags & COMPAT_DISABLEWINDOWLESS) ?
                    S_FALSE :
                    S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetCapture, IOleInPlaceSiteWindowless
//
//  Synopsis:   object wants to know if it still has mouse capture
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetCapture()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetCapture SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    CDoc *  pDoc = Doc();

    // CHROME
    // If Chrome hosted we test capture using the container's windowless
    // interface rather than using Win32's GetCapture and an HWND (because
    // we are windowless and have no HWND).
    return (pDoc->GetCaptureObject() == (void *)MyOleSite() &&
           (!pDoc->IsChromeHosted() ? (::GetCapture() == pDoc->_pInPlace->_hwnd) : pDoc->GetCapture())) ? S_OK : S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::SetCapture, IOleInPlaceSiteWindowless
//
//  Synopsis:   object wants to capture the mouse, or release capture
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::SetCapture(BOOL fCapture)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::SetCapture SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        RRETURN(E_UNEXPECTED);

    MyOleSite()->TakeCapture(fCapture);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetFocus, IOleInPlaceSiteWindowless
//
//  Synopsis:   object wants to know if it still has focus
//
//+---------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetFocus()
{
    CDoc * pDoc = Doc();

    TraceTag((tagOleSiteClient, "COleSite::CClient::GetFocus SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    // CHROME
    // If Chrome hosted there is no valid HWND so ::GetFocus() cannot be used.
    // Instead use CServer::GetFocus() which handles the windowless case.
    if (!pDoc->IsChromeHosted())
    {
        return (pDoc->_pElemCurrent == MyOleSite() &&
                ::GetFocus() == pDoc->_pInPlace->_hwnd) ? S_OK : S_FALSE;
    }
    else
    {
        return (pDoc->_pElemCurrent == MyOleSite() &&
                pDoc->GetFocus()) ? S_OK : S_FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::SetFocus, IOleInPlaceSiteWindowless
//
//  Synopsis:   Windowless control wants to grab the focus.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::SetFocus(BOOL fFocus)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::SetFocus SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    if (fFocus)
    {
        if (S_OK != MyOleSite()->BecomeCurrent(0))
        {
            RRETURN(E_FAIL);
        }

        // CHROME
        // If Chrome hosted there is no valid HWND so ::GetFocus()
        // ::SetFocus() cannot be used. Instead use CServer::GetFocus()
        // and CServer()::SetFocus() which handle the windowless case
        if (!pDoc->IsChromeHosted() ? ::GetFocus() != pDoc->_pInPlace->_hwnd : !pDoc->GetFocus())
        {
            // This will cause WM_SETFOCUS to be sent to this site.
            if (!pDoc->IsChromeHosted())
                ::SetFocus(pDoc->_pInPlace->_hwnd);
            else
                pDoc->SetFocus(TRUE);
        }
        else
        {
            // Focus already on the form. Need to force message to control.

            LRESULT lResult;

            hr = THR_OLEO(((IOleInPlaceObjectWindowless *)MyOleSite()->_pInPlaceObject)->
                OnWindowMessage(
                    WM_SETFOCUS,
                    0,
                    0,
                    &lResult),MyOleSite());
        }
    }
    // CHROME
    // Again must use CServer functions as if Chrome hosted there
    // is no valid HWND.
    else if (pDoc->_pElemCurrent == MyOleSite() &&
            (!pDoc->IsChromeHosted() ? ::GetFocus() == pDoc->_pInPlace->_hwnd : pDoc->GetFocus()))
    {
        if (!pDoc->IsChromeHosted())
            ::SetFocus(NULL);
        else
            pDoc->SetFocus(FALSE);
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnDefWindowMessage, IOleInPlaceSiteWindowless
//
//  Synopsis:   Implement default behavior for messages. Can be called by
//              object as an alternative to returning S_FALSE from
//              IOleInPlaceObjectWindowless::OnWindowMessage.
//
//  Arguments:  see OnWindowMessage
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    TraceTag((tagOleSiteClient,
        "COleSite::CClient::OnDefWindowMessage SSN=0x%x msg=0x%x wParam=0x%x lParam=0x%x",
        MyOleSite()->_ulSSN,
        msg, wParam, lParam));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    CMessage Message(NULL, msg, wParam, lParam);
    HRESULT hr;
    CDoc *  pDoc = Doc();

    Message.SetNodeHit( MyOleSite()->GetFirstBranch() );

    // Tell the form that this message came from a control inside it.
    pDoc->_pInPlace->_fBubbleInsideOut = TRUE;
    hr = THR( MyOleSite()->CElement::HandleMessage( &Message ) );
    pDoc->_pInPlace->_fBubbleInsideOut = FALSE;
    // BUGBUG (carled) plResult is not set by this routine. eventually we will want to
    // put in   *plResult = Message.lResult;
    // but for now...
    *plResult = hr;
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   COleSite::CClient::GetDC, IOleInPlaceSiteWindowless
//
//  Synopsis:   Provides hDC for windowless object to paint itself.
//
//  Arguments:  [prc]     -- rect which object wants to draw in pixels
//              [dwFlags] -- flags to determine hDC returned by container
//              [phDC]    -- pointer to returned hDC
//
//  Returns:    HRESULT (S_OK if valid HDC, OLE_E_NESTEDPAINT if already painting)
//
//  Notes:      if [prc] is NULL, assume object wants to draw entire rect.
//
//              Following is the description of how we process depending on
//              various flags.
//
//                              no OFFSCREEN              OFFSCREEN
//
//              no PAINTBKGND   pre: exclude opaque       pre: nothing
//              (opaque)        areas in front
//
//                              post: paint transparent   post: paint all in
//                              areas in front            front
//
//
//              PAINTBKGND      pre: exclude opaque       pre: paint all
//              (transparent)   areas in front. paint     behind, also
//                              all sites behind, also    paint form.
//                              paint form.
//
//                              post: paint transparent   post: paint all
//                              areas in front            in front
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetDC(LPCRECT prc, DWORD dwFlags, HDC * phDC)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetDC SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        RRETURN(E_UNEXPECTED);

    CRect   rcClip;

    //  restrict paint area to the control's rectangle
    //
    COleLayout * pLayout = DYNCAST(COleLayout, MyOleSite()->GetCurLayout());

    Assert(pLayout);

    pLayout->GetClippedRect(&rcClip, COORDSYS_GLOBAL);
    
    if (prc)
    {
        rcClip.IntersectRect(*prc);
    }
    
    if (!rcClip.IsEmpty())
    {
        // if the caller isn't going to use this DC for drawing, we can safely
        // return a DC
        if (dwFlags & OLEDC_NODRAW)
        {
            return pLayout->GetDC(&rcClip, dwFlags, phDC);
        }
        
        // just invalidate and fail, and we will draw the control later
        pLayout->Invalidate(rcClip, COORDSYS_GLOBAL);
    }
    
    return E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Function:   COleSite::CClient::ReleaseDC, IOleInPlaceSiteWindowless
//
//  Synopsis:   Signals end of paint for object
//
//  Arguments:  [hDC] -- hDC returned by object
//
//  Returns:    HRESULT (S_OK)
//
//  Notes:      See notes for GetDC
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::ReleaseDC(HDC hDC)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::ReleaseDC SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        RRETURN(E_UNEXPECTED);

    COleLayout * pLayout = DYNCAST(COleLayout, MyOleSite()->GetCurLayout());
    Assert(pLayout);

    RRETURN(pLayout->ReleaseDC(hDC));
}


//+---------------------------------------------------------------------------
//
//  Function:   COleSite::CClient::InvalidateRect, IOleInPlaceSiteWindowless
//
//  Synopsis:   Invalidates a given rectangular area within object's rectangle
//
//  Arguments:  [prc]    -- rectangle to invalidate
//              [fErase] -- repaint the background?
//
//  Returns:    HRESULT (S_OK)
//
//  Notes:      [prc] is assumed to be in Window Pixel coordinates.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::InvalidateRect(LPCRECT prc, BOOL fErase)
{
    COleSite *  pOleSite = MyOleSite();
    CLayout *   pLayout  = pOleSite->GetCurLayout();

    TraceTag((tagOleSiteClient, "COleSite::CClient::InvalidateRect SSN=0x%x", MyOleSite()->_ulSSN));

    if (pOleSite->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        RRETURN(E_UNEXPECTED);


    // make sure the doc flag for disabling tiled paint is set
    if (pLayout->_fSurface)
    {
        pOleSite->Doc()->_fDisableTiledPaint = TRUE;
    }

    // prc is in global coordinates, so invalidate appropriately
    if (prc)
    {
        CRect   rc = *prc;

        pLayout->TransformRect(&rc, COORDSYS_GLOBAL, COORDSYS_CONTENT);
        pLayout->Invalidate(&rc);
    }
    else
        pLayout->Invalidate(prc);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   COleSite::CClient::InvalidateRgn, IOleInPlaceSiteWindowless
//
//  Synopsis:   Invalidates a given region within object's rectangle
//
//  Arguments:  [hrgn]    -- region to invalidate
//              [fErase]  -- repaint the background?
//
//  Returns:    HRESULT (S_OK)
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::InvalidateRgn(HRGN hrgn, BOOL fErase)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::InvalidateRgn SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        RRETURN(E_UNEXPECTED);

    // make sure the doc flag for disabling tiled paint is set
    if (MyOleSite()->GetCurLayout()->_fSurface)
    {
        MyOleSite()->Doc()->_fDisableTiledPaint = TRUE;
    }

// BUGBUG: This region is in global coordinates and must be transformed to local before calling CLayout (brendand)
    MyOleSite()->GetCurLayout()->Invalidate(hrgn);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   COleSite::CClient::ScrollRect, IOleInPlaceSiteWindowless
//
//  Synopsis:   Scrolls the site window as requested
//
//  Arguments:  [dx]        --
//              [dy]        --
//              [prcScroll] --
//              [prcClip]   --
//
//  Returns:    HRESULT (S_OK)
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::ScrollRect(
    int dx, int dy, LPCRECT prcScroll, LPCRECT prcClip )
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::ScrollRect SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE | VALIDATE_DOC_INPLACE))
        RRETURN(E_UNEXPECTED);

    HRESULT         hr = S_OK;
#if 0
// BUGBUG: Fix this! (brendand)
    RECT            rcParent;
    RECT            rcScroll;
    RECT            rcClip;
    RECT            rcSite;
    BOOL            fAboveMe;
    CDoc *          pDoc = Doc();

    Assert(pDoc->_pInPlace);

    // Get the visible site rect in device units

    rcSite = MyOleSite()->GetCurLayout()->_rc;
    MyOleSite()->GetParentLayout()->GetVisibleClientRect(&rcParent);
    IntersectRect(&rcSite, &rcSite, &rcParent);

    // If not scroll rect was passed in, then scroll the entire site

    rcScroll = prcScroll ? * prcScroll : rcSite;

    // If no clip rect was passed in, then set  it to the scroll rect

    rcClip = prcClip ? * prcClip : rcScroll;

    // Make sure rects are contained within the client site.  If either
    // does not intersect the site, then do nothing.

    if (!IntersectRect(&rcScroll, &rcScroll, &rcSite) ||
        !IntersectRect(&rcClip,   &rcClip,   &rcSite))
    {
        goto Cleanup;
    }

    // If this site has a transparent control, then just invalidate.

    if (!MyOleSite()->GetCurLayout()->_fOpaque)
        goto InvalidateClip;

    // Check to see if another site above us in the zorder intersects.
    // If so, then invalidate.

    fAboveMe = FALSE;
    if (pDoc->_pSiteRoot->GetCurLayout()->CheckLayoutIntersect(
                                     MyOleSite()->GetCurLayout(),
                                     &fAboveMe,
                                     SI_ABOVE))
        goto InvalidateClip;

    Assert(pDoc->_pInPlace->_hwnd);

    ::ScrollWindowEx(
        pDoc->_pInPlace->_hwnd,
        dx, dy, &rcScroll, &rcClip, 0, 0, SW_INVALIDATE);

Cleanup:
#endif

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   COleSite::CClient::AdjustRect, IOleInPlaceSiteWindowless
//
//  Synopsis:   Invalidates object's rectangle
//
//  Arguments:  [prc] -- return clipped rectangle here.
//
//  Returns:    HRESULT (S_OK)
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::AdjustRect(LPRECT prc)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::AdjustRect SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_WINDOWLESSINPLACE))
        RRETURN(E_UNEXPECTED);

#if 0
    BUGBUG fix later, what is this doing ?
    CSite *         pSiteLoop;
    RECT            rcSiteOpaque;
    POINT           pt;

    if (!MyOleSite()->_fWindowlessInplace)
    {
        Assert(0 && "Unexpected call to client site.");
        RRETURN(E_UNEXPECTED);
    }

    Assert(prc);

    pt.x = prc->left;
    pt.y = prc->top;

    while ((pSiteLoop = iterBackToFront.Next()) != MyOleSite()) {};

    while ((pSiteLoop = iterBackToFront.Next()) != NULL)
    {
        if (pSiteLoop->HitTestRect(&(MyOleSite()->_rc)) == HTC_YES)
        {
            if (pSiteLoop->GetOpaqueRect(&rcSiteOpaque))
            {
                if (!IntersectRect(prc, prc, &rcSiteOpaque))
                {
                    prc->left = prc->right = pt.x;
                    prc->top = prc->bottom = pt.y;
                    return S_FALSE;
                }
            }
        }
        else
        {
            iterBackToFront.SkipChildren(pSiteLoop);
        }
    }
#endif
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnControlInfoChanged, IOleControlSite
//
//  Synopsis:   notification from the control of change
//
//  Returns:    S_OK iff sucessful, else error
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnControlInfoChanged()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnControlInfoChanged SSN=0x%x", MyOleSite()->_ulSSN));

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::LockInPlaceActive, IOleControlSite
//
//  Synopsis:   Increments or decrements an in-place active lock
//              count.  If non-zero, then the form will ensure that
//              this site is not deactivated.
//
//  Arguments:  [fLock] -- Increment if TRUE
//
//  Returns:    S_OK iff sucessful, else error
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::LockInPlaceActive(BOOL fLock)
{
    TraceTag((tagOleSiteClient,
        "COleSite::CClient::LockInPlaceActive SSN=0x%x fLock=%s",
        MyOleSite()->_ulSSN,
        fLock ? "TRUE" : "FALSE"));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_INPLACE))
        RRETURN(E_UNEXPECTED);

    HRESULT             hr;
    IOleControlSite *   pOCS;

    //  Part of the contract of locking the control in the in-place
    //    active state is that the form itself needs to remain
    //    in-place active.  We can't do this unless the form's
    //    site supports the LockInPlaceActive method

    hr = THR(Doc()->_pClientSite->QueryInterface(
            IID_IOleControlSite,
            (void **) &pOCS));
    if (hr)
        RRETURN(E_FAIL);

    hr = THR(pOCS->LockInPlaceActive(fLock));
    if (hr)
        goto Cleanup;

    if (fLock)
    {
        if (MyOleSite()->_cLockInPlaceActive == MAX_LOCK_INPLACEACTIVE)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        MyOleSite()->_cLockInPlaceActive++;
    }
    else
    {
        if (MyOleSite()->_cLockInPlaceActive == 0)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        MyOleSite()->_cLockInPlaceActive--;
    }

    //  Changing the lock count may change the proper state for
    //    this control; this call moves the control to the
    //    proper state.  Note that errors are ignored.

    IGNORE_HR(MyOleSite()->TransitionToCorrectState());

Cleanup:
    pOCS->Release();

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetExtendedControl, IOleControlSite
//
//  Synopsis:   Answer pointer to the XObject for this control
//
//  Arguments:  [ppUnk] -- where to return pointer
//
//  Returns:    S_OK iff sucessful, else error
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetExtendedControl(IDispatch **ppDisp)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetExtendedControl SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    RRETURN(THR_OLEO(MyOleSite()->QueryInterface(IID_IDispatch,(void **)ppDisp),MyOleSite()));
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::TransformCoords, IOleControlSite
//
//  Synopsis:   Answer pointer to the XObject for this control
//
//  Arguments:  [ppUnk] -- where to return pointer
//
//  Returns:    S_OK iff sucessful, else error
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::TransformCoords(POINTL *pptl, POINTF *pptf, DWORD dwFlags)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::TransformCoords SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

     if (dwFlags & XFORMCOORDS_HIMETRICTOCONTAINER)
     {
         pptf->x = HPixFromHimetric(pptl->x);
         pptf->y = VPixFromHimetric(pptl->y);
     }
     else
     {
         pptl->x = HimetricFromHPix(pptf->x + 0.5);
         pptl->y = HimetricFromVPix(pptf->y + 0.5);
     }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::TranslateAccelerator, IOleControlSite
//
//  Synopsis:   Called by our embedded control if it doesn't process an
//              accelerator message.
//
//  Arguments:  [pmsg] -- Message to translate
//
//  Returns:    S_OK if handled, S_FALSE if not, error HRESULT on error.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::TranslateAccelerator(LPMSG pmsg, DWORD grfModifiers)
{
    TraceTag((tagOleSiteClient,
        "COleSite::CClient::TranslateAccelerator SSN=0x%x message=0x%x",
        MyOleSite()->_ulSSN,
        pmsg->message));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    CMessage Message(pmsg);

    // Give host a chance to handle it first
    if (Doc()->HostTranslateAccelerator(pmsg) == S_OK)
        return S_OK;

    // We get many messages besides the keystroke messages here.  This is
    // expected and used by the other site types, but for ole sites we
    // need to ensure only keystroke messages are handled and propagated
    // here.  -Tom

    switch (pmsg->message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        break;
    default:
        return S_FALSE;
    }

    //
    // Backspace is a navigation keystroke for us.  Block this out
    // for any ocx, as long as alt & ctrl aren't depressed either.
    //

    if (Message.wParam == VK_BACK &&
        !(Message.dwKeyState & (MK_CONTROL | MK_ALT)))
        return S_FALSE;

    CTreeNode * pNodeParentSite = MyOleSite()->GetUpdatedParentLayoutNode();

    if (!pNodeParentSite)
        return S_FALSE;

    RRETURN1(THR(pNodeParentSite->Doc()->PumpMessage(&Message, pNodeParentSite, TRUE)), S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::RequestUIActivate, IOleInPlaceSiteEx
//
//  Synopsis:   Notifies the container that the control intends to transition
//              to the UIActivate state
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::RequestUIActivate()
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::RequestUIActivate SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED))
        RRETURN(E_UNEXPECTED);

    RRETURN1(MyOleSite()->IsFocussable(0) ? S_OK : S_FALSE, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnFocus, IOleControlSite
//
//  Synopsis:   Notifies the container that the control grabbed the focus.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnFocus(BOOL)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnFocus SSN=0x%x", MyOleSite()->_ulSSN));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::ShowPropertyFrame, IOleControlSite
//
//  Synopsis:   allows a container to hook the control's display of a
//              property frame.  Whenever the control wants to display
//              a property frame, it calls this method.  If this method
//              returns S_OK, then this site has displayed the property
//              frame, and the control should take no further action.
//              Otherwise, the control should proceed with displaying
//              its own property frame.
//
//  Returns:    S_OK or propagates error code.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::ShowPropertyFrame()
{
#ifdef NO_PROPERTY_PAGE
    return S_OK;
#else
    TraceTag((tagOleSiteClient, "COleSite::CClient::ShowPropertyFrame SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED | VALIDATE_DOC_ALIVE))
        RRETURN(E_UNEXPECTED);


    HRESULT hr = E_FAIL;
    IOleControlSite * pCtrlSite = NULL;
    CDoc *  pDoc = Doc();

    //
    // Allow Trident host to handle this call
    //

    // First, give inplace object a chance to provide control site.
    if (pDoc->_pInPlace && pDoc->_pInPlace->_pInPlaceSite)
    {
        pDoc->_pInPlace->_pInPlaceSite->QueryInterface(IID_IOleControlSite, (void **) &pCtrlSite);
    }

    // if this fails, try the client site.
    if (!pCtrlSite && pDoc->_pClientSite)
    {
        pDoc->_pClientSite->QueryInterface(IID_IOleControlSite, (void **) &pCtrlSite);
    }

    if (pCtrlSite)
    {
        hr = pCtrlSite->ShowPropertyFrame();
        ReleaseInterface(pCtrlSite);
    }

    RRETURN(hr);
#endif // NO_PROPERTY_PAGE
}

//+------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::QueryStatus, IOleCommandTarget
//
//  Synopsis:   Delegates QS of commands upward
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

HRESULT
COleSite::CClient::QueryStatus(
                GUID * pguidCmdGroup,
                ULONG cCmds,
                MSOCMD rgCmds[],
                MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::QueryStatus SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED | VALIDATE_DOC_SITE))
        RRETURN(OLECMDERR_E_UNKNOWNGROUP);

    HRESULT hr;

    hr = THR_NOTRACE(CTQueryStatus(
        Doc()->_pClientSite,
        pguidCmdGroup,
        cCmds,
        rgCmds,
        pcmdtext));

    if (hr == E_NOINTERFACE)
        hr = OLECMDERR_E_UNKNOWNGROUP;

    // Disable Office documents in frameset from showing/hiding toolbars.

    if (pguidCmdGroup == NULL)
    {
        for (UINT i = 0; i < cCmds; i++)
        {
            if (rgCmds[i].cmdID == OLECMDID_HIDETOOLBARS)
            {
                rgCmds[i].cmdf = 0;
            }
        }
    }

    RRETURN(hr);
}






//+------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::Exec, IOleCommandTarget
//
//  Synopsis:   Delegates Exec of commands upward
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

HRESULT
COleSite::CClient::Exec(
                GUID * pguidCmdGroup,
                DWORD nCmdID,
                DWORD nCmdexecopt,
                VARIANTARG * pvarargIn,
                VARIANTARG * pvarargOut)
{
    HRESULT hr;

    TraceTag((tagOleSiteClient, "COleSite::CClient::Exec SSN=0x%x", MyOleSite()->_ulSSN));

// New code to implement refresh for HTML OSP
    
    if ((pguidCmdGroup == NULL) && (nCmdID == OLECMDID_HTTPEQUIV))
    {
        extern BOOL ParseRefreshContent(LPCTSTR pchContent,
                                        UINT * puiDelay,
                                        LPTSTR pchUrlBuf,
                                        UINT cchUrlBuf);
        COleSite *pOleSite = MyOleSite();
        if (pOleSite->_iRefreshTime == 0)
        {
            LPCTSTR psz = pvarargIn->bstrVal;
            if (_tcsnipre(_T("refresh:"), 8, psz, -1))
            {
                UINT uiRefresh = 0;
                TCHAR ach[pdlUrlLen];
                psz += 8;

                if (ParseRefreshContent(psz, &uiRefresh, ach, ARRAY_SIZE(ach)))
                {
                    pOleSite->ClearRefresh();
                    pOleSite->_iRefreshTime = uiRefresh * 1000;
                    if (ach[0])
                        pOleSite->_pstrRefreshURL = SysAllocString(ach);
                }
            }
        }

        return S_OK;
    }

    if ((pguidCmdGroup == NULL) && (nCmdID == OLECMDID_HTTPEQUIV_DONE))
        return S_OK;

    if (IDMFromCmdID(pguidCmdGroup, nCmdID) == IDM_SHDV_ONCOLORSCHANGE)
    {
        MyOleSite()->Doc()->InvalidateColors();
        return S_OK;
    }

// End Refresh code

    if (MyOleSite()->IllegalSiteCall(VALIDATE_LOADED | VALIDATE_DOC_SITE))
        RRETURN(OLECMDERR_E_UNKNOWNGROUP);

    //  Doc aggregates the Enter/Leaving scripts of embeddings with its own
    //  and fires these execs on aggregate entry/leaving
    if (pguidCmdGroup && *pguidCmdGroup == CGID_ShellDocView)
    {
        if (nCmdID == SHDVID_NODEACTIVATENOW)
        {
            RRETURN(THR(Doc()->EnterScript()));
        }
        else if (nCmdID == SHDVID_DEACTIVATEMENOW)
        {
            RRETURN(THR(Doc()->LeaveScript()));
        }
    }
    else if (pguidCmdGroup == NULL && nCmdID == OLECMDID_HIDETOOLBARS)
    {
        // Disable Office documents in frameset from showing/hiding toolbars.

        RRETURN(OLECMDERR_E_DISABLED);
    }



    hr = THR_NOTRACE(CTExec(
            Doc()->_pClientSite,
            pguidCmdGroup,
            nCmdID,
            nCmdexecopt,
            pvarargIn,
            pvarargOut));

    if (hr == E_NOINTERFACE)
        hr = OLECMDERR_E_UNKNOWNGROUP;

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::QueryService, IServiceProvider
//
//  Synopsis:   Return any requested services that the site supports,
//              otherwise delegate to the form.  The form will delegate to
//              its site.
//
//  Arguments:  [guidService] -- GUID of requested service
//              [iid]         -- Interface to return on requested service
//              [ppv]         -- Place to put requested service
//
//  Returns:    HRESULT (STDMETHOD)
//
//----------------------------------------------------------------------------

EXTERN_C const GUID CLSID_HTMLFrameBase;
EXTERN_C const GUID CLSID_HTMLIFrame;

STDMETHODIMP
COleSite::CClient::QueryService(REFGUID guidService,
                                REFIID iid,
                                void ** ppv)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::QueryService SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_NOINTERFACE);

    *ppv = NULL;

    if (IsEqualGUID (guidService, CLSID_HTMLFrameBase))
    {
        if (MyOleSite()->OlesiteTag() == OSTAG_FRAME)
        {
            // NOTE that we're not AddRef()'ing the return
            // value.  Callers beware!
            *ppv = MyOleSite();
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }
    else if (IsEqualGUID (guidService, CLSID_HTMLIFrame))
    {
        if (MyOleSite()->OlesiteTag() == OSTAG_IFRAME)
        {
            // NOTE that we're not AddRef()'ing the return
            // value.  Callers beware!
            *ppv = MyOleSite();
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }
    else if (IsEqualGUID(guidService, SID_SBindHost))
    {
        RRETURN (THR(QueryInterface(iid, ppv)));
    }
    else if (MyOleSite()->OlesiteTag() != OSTAG_FRAME &&
             MyOleSite()->OlesiteTag() != OSTAG_IFRAME &&
             IsEqualGUID(guidService, CLSID_HTMLDocument))
    {
        //
        // (anandra) No bubbling of private QS's if not in a frame.
        //

        RRETURN(E_NOINTERFACE);
    }
    else if (IsEqualGUID(guidService, IID_IHTMLDialog))
    {
        // (alexz) (anandra) IID_IHTMLDialog is our private interface and it is used by document /
        // script window to find out if they are hosted in / aggregated by CHtmlDlg. No objects
        // hosted inside olesite can make QueryService for IID_IHTMLDialog except WebBrowser OC. WebBrowser OC
        // makes the QS in case if it contains Trident doc / window inside. In that case, however, we should
        // block the propagation of QS so that the doc / window will not get confused thinking that
        // they are hosted in / aggregated by html dlg.
        //
        // the way to get to this codepath: bring up an html dialog with <iframe> inside.

        RRETURN (E_NOINTERFACE);
    }

    RRETURN(Doc()->QueryService(guidService, iid, ppv));
}

//+---------------------------------------------------------------------------
//
// Member:      COleSite::CClient::GetTypeInfoCount
//
// Synopsis:    Returns the number of typeinfos available on this object
//
// Arguments:   [pctinfo] - The number of typeinfos
//
//----------------------------------------------------------------------------

HRESULT
COleSite::CClient::GetTypeInfoCount(UINT FAR *pcTinfo)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetTypeInfoCount SSN=0x%x", MyOleSite()->_ulSSN));

    *pcTinfo = 0;
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetIDsOfNames
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
COleSite::CClient::GetIDsOfNames(
        REFIID      riid,
        TCHAR **    rgszNames,
        UINT        cNames,
        LCID        lcid,
        DISPID *    rgdispid)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetIDsOfNames SSN=0x%x", MyOleSite()->_ulSSN));

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetTypeInfo, IDispatch
//
//  Synopsis:   As per IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** ppTI)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetTypeInfo SSN=0x%x", MyOleSite()->_ulSSN));

    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::Invoke, IDispatch
//
// Synopsis:    Provides access to properties and members of the control
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
//----------------------------------------------------------------------------

HRESULT
COleSite::CClient::Invoke(DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR *pdispparams,
        VARIANT FAR *pvarResult,
        EXCEPINFO FAR *pexcepinfo,
        UINT FAR *puArgErr)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::Invoke SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    HRESULT hr = S_OK;

    if (puArgErr)
        *puArgErr = 0;

    if (pexcepinfo)
        memset(pexcepinfo, 0, sizeof(*pexcepinfo));

    if (!pdispparams)
        RRETURN(E_INVALIDARG);

    if (pvarResult != NULL)
        VariantInit(pvarResult);

    if (wFlags & DISPATCH_PROPERTYGET)
    {
        if (pvarResult == NULL)
            return E_INVALIDARG;

        hr = MyOleSite()->GetAmbientProp(dispidMember, pvarResult);
    }
    else if (wFlags & DISPATCH_PROPERTYPUT)
    {
        hr = DISP_E_MEMBERNOTFOUND;

        if (pdispparams->cArgs < 1)
            return E_INVALIDARG;
    }
    else
        hr = DISP_E_MEMBERNOTFOUND;

    return hr;
}

#ifndef NO_DATABINDING
//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::GetCursor (IBoundObjectSite)
//
//  Synopsis:   Called by Data consuming control to get its ICursor
//
//  Arguments:  dispid      dispid of data-bound property
//              ppCursor    where to put the cursor pointer
//                          may not be NULL
//              ppcidOut    If a simple-valued binding (not Cursor-valued,
//                          where to a return an ICursor DBCOLUMNID for
//                          which column boun to.
//
//  Returns:    S_OK        success
//              E_INVALIDARG
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::GetCursor(DISPID dispid,
        ICursor **ppCursor,
        LPVOID FAR* ppcidOut)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::GetCursor SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    HRESULT     hr = E_INVALIDARG;
    DISPID      dispidCursor = MyOleSite()->GetClassInfo()->dispidCursor;

    if (ppCursor == NULL)
        goto Cleanup;

    *ppCursor = NULL;

    // ppcidOut should only be NULL, otherwise error.
    if (ppcidOut != NULL)
    {
        *ppcidOut = NULL;
        goto Cleanup;
    }

    if (dispid == dispidCursor)
    {
        // we only support ICursor binding on the tag itself;
        //  any binding set on PARAMs uses IDataSource.
        DBMEMBERS *pdbm = MyOleSite()->GetDBMembers();
        CDataSourceBinder *pdsbBinder;

        if (!pdbm)
            goto Error;

        pdsbBinder = pdbm->GetBinder(ID_DBIND_DEFAULT);
        if (!pdsbBinder)
            goto Error;

        hr = pdsbBinder->GetICursor(ppCursor);
    }

Cleanup:
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::::OnChanged
//
//  Synopsis:   Forwards to the connection point.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnChanged(DISPID dispid)
{
    TraceTag((tagOleSiteClient,
        "COleSite::CClient::OnChanged SSN=0x%x dispid=0x%x",
        MyOleSite()->_ulSSN,
        dispid));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    HRESULT hr;

    hr = THR(MyOleSite()->OnControlChanged(dispid));
    if (hr == S_FALSE)
        hr = S_OK;
    else if (!hr)
        hr = THR(MyOleSite()->FireOnChanged(dispid));

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::OnRequestEdit
//
//  Synopsis:   Forwards to the connection point.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::OnRequestEdit(DISPID dispid)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::OnRequestEdit SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    HRESULT hr;

    hr = THR(MyOleSite()->OnControlRequestEdit(dispid));
    if (!hr)
    {
        hr = THR(MyOleSite()->FireRequestEdit(dispid));
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::ValidateSecureUrl
//
//  Synopsis:   Forwards to the connection point.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::ValidateSecureUrl(BOOL* pfAllow, OLECHAR* pchUrlInQuestion, DWORD dwFlags)
{
    TraceTag((tagOleSiteClient, "COleSite::CClient::ValidateSecureUrl SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED))
        RRETURN(E_UNEXPECTED);

    HRESULT hr;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchNewUrl = cBuf;
    CDoc *  pDoc = MyOleSite()->Doc();

    hr = THR(pDoc->ExpandUrl(pchUrlInQuestion, ARRAY_SIZE(cBuf), pchNewUrl, MyOleSite()));
    if (hr)
        goto Cleanup;

    *pfAllow = (pDoc->ValidateSecureUrl(pchNewUrl,
        !!(SUHV_PROMPTBEFORENO & dwFlags),
        !!(SUHV_SILENTYES & dwFlags),
        !!(SUHV_UNSECURESOURCE & dwFlags)));

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//	Member:		COleSite::CClient::DeferredOnPosRectChange
//
//	Synopsis:	OnPostRectChange posts a call to itself through this method,
//				if we are in the rendering /view updating phase when OnPosRectChange
//				is called by the control site.
//				This member recalls the OnPosRectChange.
//----------------------------------------------------------------------------
void
COleSite::CClient::DeferredOnPosRectChange( DWORD_PTR dwContext )
{
    if (!_prcPending)
        return;

    THR(OnPosRectChange( _prcPending ));

    delete _prcPending;
    _prcPending = 0;
}

