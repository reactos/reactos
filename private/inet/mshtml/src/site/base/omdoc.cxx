//+------------------------------------------------------------------------
//
//  File:       OMDOC.CXX
//
//  Contents:   OmDocument
//
//  Classes:    COmDocument
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

IMPLEMENT_SUBOBJECT_IUNKNOWN(COmDocument, CDoc, Doc, _OmDocument)


//+-------------------------------------------------------------------------
//
//  Method:     COmDocument::QueryInterface
//
//--------------------------------------------------------------------------

HRESULT
COmDocument::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT      hr = S_OK;
    const void * apfn = NULL;
    void *       pv = NULL;
    const IID * const * apIID = NULL;
    extern const IID * const g_apIID_IDispatchEx[];

    *ppv = NULL;

    if (!IsMyParentAlive())
    {
        RRETURN(E_UNEXPECTED);
    }
    else if (iid == IID_IProvideMultipleClassInfo ||
        iid == IID_IProvideClassInfo ||
        iid == IID_IProvideClassInfo2)
    {
        pv = Doc();
        apfn = CDoc::s_apfnIProvideMultipleClassInfo;
    }
    else if (iid == IID_IDispatchEx || iid == IID_IDispatch)
    {
        apIID = g_apIID_IDispatchEx;
        pv = (IDispatchEx *)Doc();
        apfn = *(void **)pv;
    }
    else if (iid == IID_IHTMLDocument ||
        iid == IID_IHTMLDocument2)
    {
        pv = (IHTMLDocument2 *)Doc();
        apfn = *(void **)pv;
    }
    else if (iid == IID_IConnectionPointContainer)
    {
        *((IConnectionPointContainer **)ppv) = new CConnectionPointContainer(Doc(), this);
        if (!*ppv)
            RRETURN(E_OUTOFMEMORY);
    }
    else if (iid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
    }
    else if (iid == IID_IMarkupServices)
    {
        pv = Doc();
        apfn = CDoc::s_apfnIMarkupServices;
    }
    else if (iid == IID_IHTMLViewServices)
    {
        pv = Doc();
        apfn = CDoc::s_apfnIHTMLViewServices;
    }
#if DBG == 1
    else if ( iid == IID_IEditDebugServices )
    {
        pv = Doc();
        apfn = CDoc::s_apfnIEditDebugServices;        
    }
#endif
    else if (iid == IID_IServiceProvider )
    {
        pv = Doc();
        apfn = CDoc::s_apfnIServiceProvider;
    }
    else if (iid == IID_IOleWindow)
    {
        pv = Doc();
        apfn = CDoc::s_apfnIOleInPlaceObjectWindowless;
    }
    else if (iid == IID_IOleCommandTarget)
    {
        pv = Doc();
        apfn = CDoc::s_apfnIOleCommandTarget;
    }
    else
    {
        RRETURN(E_NOINTERFACE);
    }

    if (pv)
    {
        Assert(apfn);
        hr = THR(CreateTearOffThunk(
                pv, 
                apfn, 
                NULL, 
                ppv, 
                (IUnknown *)this, 
                *(void **)(IUnknown *)this,
                QI_MASK | ADDREF_MASK | RELEASE_MASK,
                apIID));
        if (hr)
            RRETURN(hr);
    }

    Assert(*ppv);
    ((IUnknown *)*ppv)->AddRef();

    DbgTrackItf(iid, "COmDocument", FALSE, ppv);

    return S_OK;
}
