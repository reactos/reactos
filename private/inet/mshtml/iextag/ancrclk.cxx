//=================================================
//
//  File : ancrclk.cxx
//
//  purpose : implementation of the 
//            CAnchorClick class
//            Gives anchor tags the ability to 
//            navigate to web folders
//
//=================================================
// Chad Lindhorst, 1998

#include "headers.h"
#include "ancrclk.h"
#include "httpwfh.h"
#include "utils.hxx"

// ========================================================================
// CAnchorClick
// ========================================================================

//+------------------------------------------------------------------------
//
//  Members:    CAnchorClick::CAnchorClick
//              CAnchorClick::~CAnchorClick
//
//  Synopsis:   Constructor/destructor
//
//-------------------------------------------------------------------------

CAnchorClick::CAnchorClick() 
{
    m_pSite = NULL;
    m_pSink = NULL;
    m_pSinkContextMenu = NULL;
}

CAnchorClick::~CAnchorClick() 
{
    if (m_pSite)
        m_pSite->Release();
    if (m_pSink)
        delete m_pSink;
    if (m_pSinkContextMenu) 
        delete m_pSinkContextMenu;
}

// ========================================================================
// IAnchorClick
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::ProcOnClick
//
//  Synopsis:   Handles the onclick events.
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
CAnchorClick::ProcOnClick () 
{
    //BSTR bstrProtocol = NULL; //bug 50463
    BSTR bstrTarget = NULL;
    BSTR bstrUrl = NULL;
    HRESULT hr = E_FAIL;
    IHTMLElement *pElem = NULL;
    IElementBehavior * pPeer = NULL;
    Iwfolders * pWF = NULL;
    CComObject<Cwfolders> *pInstance = NULL; 
    IHTMLEventObj * pEvent = NULL;
    IHTMLWindow2 * pWin = NULL;

    if (FAILED(hr = m_pSite->GetElement (&pElem)))
        goto cleanup;
    
    if ( SUCCEEDED(hr = GetProperty_BSTR (pElem, L"href", &bstrUrl)))
    //bug 50463 && SUCCEEDED(hr = GetProperty_BSTR (pElem, L"navType", &bstrProtocol)) )
    {
        VARIANT vFalse;
        vFalse.vt = VT_BOOL;
        vFalse.boolVal = FALSE;

        /* //bug 50463
        if ((StrCmpIW (bstrProtocol, L"any")) &&
            (StrCmpIW (bstrProtocol, L"dav")) &&
            (StrCmpIW (bstrProtocol, L"wec")))
        {
            // this is a bad protocol.  Let's get outta here.
            goto cleanup;
        }
        */
        // if we got this far, we are gonna do a folder navigate        
        // prevent us from getting handled by anyone else.
        if (FAILED(hr = GetHTMLWindow (m_pSite, &pWin)))
            goto cleanup;

        if (FAILED(hr = pWin->get_event (&pEvent)) ||
            FAILED(hr = pEvent->put_returnValue(vFalse)))
        {
            goto cleanup;
        }
        
        // sets the target frame
        hr = GetProperty_BSTR (pElem, L"target", &bstrTarget);
        if (FAILED(hr) || !bstrTarget)
            bstrTarget = NULL;

        // get a hold of the httpFolder behavior.
        if (FAILED(hr = CComObject<Cwfolders>::CreateInstance(&pInstance)) ||
            FAILED(hr = pInstance->QueryInterface(IID_Iwfolders, (void **)&pWF)))
        {
            goto cleanup;
        }
       
        {
            IUnknown * punk = NULL;
            HWND hwnd = 0;

            // gets browser window handle (for ui)
            hr = GetClientSiteWindow(m_pSite, &hwnd);
            hr = IUnknown_QueryService(m_pSite, SID_SWebBrowserApp, IID_IUnknown, 
                                       (LPVOID *) &punk);          

            // navigates to the folder view of the url specified

            hr = pWF->navigateNoSite (bstrUrl, bstrTarget, /*bstrProtocol,*/ (DWORD)(DWORD_PTR)hwnd, punk);
        }

    }
    else 
        hr = S_OK;  // since the navType wasn't set, and no we ignore everything 

cleanup:
    SysFreeString (bstrTarget);
    SysFreeString (bstrUrl);
    //SysFreeString (bstrProtocol);

    ReleaseInterface (pEvent);
    ReleaseInterface (pWin);    
    ReleaseInterface (pElem);
    ReleaseInterface (pWF);
    ReleaseInterface (pPeer);

    return hr;
}


// ========================================================================
// IElementBehavior
// ========================================================================

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::Init
//
//  Synopsis:   Called when this code is initialized as a behavior
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
CAnchorClick::Init (IElementBehaviorSite __RPC_FAR *pBehaviorSite) 
{
    HRESULT hr = E_INVALIDARG;
    BSTR bstrEvent = NULL;
    BSTR bstronContextMenu = NULL;
    BSTR bstrUrl = NULL;
    //bug 50463 BSTR bstrProtocol = NULL;
    VARIANT_BOOL vSuccess = VARIANT_FALSE;
    IHTMLElement *pElem = NULL;
    IHTMLElement2 *pElem2 = NULL;

    // this puppy is ref counted when used.
    m_pSink = new CEventSink (this);
    m_pSinkContextMenu = new CEventSink (this);
    
    if (!m_pSink || !m_pSinkContextMenu)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (pBehaviorSite != NULL)
    {
        m_pSite = pBehaviorSite;
        m_pSite->AddRef();
    }

    if (FAILED(hr = m_pSite->GetElement (&pElem)))
        goto cleanup;

    hr = pElem->QueryInterface(IID_IHTMLElement2, (void **)&pElem2);
    if (FAILED(hr))
        goto cleanup;

    // we are adding a phony href attribute to the tag if there is
    // a folder attribute set.  This will make sure that the 
    // browser uses the right underlining and colors for anchors
    // that have no href attribute, but do have a folder attribute.
    if ( SUCCEEDED(hr = GetProperty_BSTR (pElem, L"folder", &bstrUrl))) 
    /*    && (FAILED(hr = GetProperty_BSTR (pElem, L"navType", &bstrProtocol)) ||
             ((!StrCmpIW (bstrProtocol, L"any")) ||
              (!StrCmpIW (bstrProtocol, L"dav")) ||
              (!StrCmpIW (bstrProtocol, L"wec")))) )
    */
    {
        VARIANT vbstrUrl;
        vbstrUrl.vt = VT_BSTR;
        vbstrUrl.bstrVal = bstrUrl;

        pElem->setAttribute (L"href", vbstrUrl, VARIANT_FALSE);
    }
    else 
    {
        // If there is no folder specified then we assume this to be a regular anchor tag
        // Bug 54304
        goto cleanup;
    }

    /*
    // now we add a navtype if one wasn't specified and there is a "folder"
    // we add the default "any" protocol
    if ( bstrUrl && !bstrProtocol)
    {
        VARIANT vbstrUrl;
        vbstrUrl.vt = VT_BSTR;
        vbstrUrl.bstrVal = SysAllocString (L"any");
        if (!V_BSTR(&vbstrUrl))
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        
        pElem->setAttribute (L"navType", vbstrUrl, VARIANT_FALSE);
        SysFreeString (vbstrUrl.bstrVal);
    }
    */
    // we want to sink a few events.
    // we will take over the click and mouseover events because they
    // act differently for folder view anchors
    bstrEvent = SysAllocString (L"onclick");
    if (!bstrEvent)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    pElem2->attachEvent (bstrEvent, (IDispatch *) m_pSink, &vSuccess);
    if (vSuccess == VARIANT_TRUE) 
        hr = S_OK;
    else 
    {
        hr = E_FAIL;
        goto cleanup;
    }

    bstronContextMenu = SysAllocString(L"oncontextmenu");
    if (bstronContextMenu) {
        pElem2->attachEvent (bstronContextMenu, (IDispatch *) m_pSinkContextMenu, &vSuccess);
        if (vSuccess == VARIANT_TRUE) 
            hr = S_OK;
        else 
        {
            hr = E_FAIL;
            goto cleanup;
        }
    }    
    
cleanup:
    ReleaseInterface (pElem);
    ReleaseInterface (pElem2);

    SysFreeString (bstrEvent);
    SysFreeString (bstronContextMenu);
    SysFreeString (bstrUrl);
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::Notify
//
//  Synopsis:   Not really used, but needed by the interface...
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE 
CAnchorClick::Notify (LONG lEvent, VARIANT __RPC_FAR *pVar) 
{
    return S_OK;    
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::GetProperty_Variant
//
//  Synopsis:   Gets a property from the element passed in as pDisp.
//              Taken from \mshtml\src\f3\drt\activex\peerdecl
//
//-------------------------------------------------------------------------

HRESULT
CAnchorClick::GetProperty_Variant (IDispatch * pDisp, LPWSTR  pchName, VARIANT * pvarRes)
{
    HRESULT     hr = S_OK;
    DISPID      dispid;
    EXCEPINFO   excepinfo;
    UINT        nArgErr;
    DISPPARAMS  dispparams = {NULL, NULL, 0, 0};

    if (!pvarRes || !pDisp)
        return E_POINTER;

    hr = pDisp->GetIDsOfNames(IID_NULL, &pchName, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (hr)
        goto Cleanup;

    hr = pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
                       &dispparams, pvarRes, &excepinfo, &nArgErr);

 Cleanup:

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::GetProperty_BSTR
//
//  Synopsis:   Gets a property from the element passed in as pDisp.
//              Taken from \mshtml\src\f3\drt\activex\peerdecl
//
//-------------------------------------------------------------------------

HRESULT
CAnchorClick::GetProperty_BSTR  (IDispatch * pDisp, LPWSTR  pchName, LPWSTR * pbstrRes)
{
    HRESULT     hr = S_OK;
    VARIANT     varRes;

    if (!pbstrRes)
        return E_POINTER;

    *pbstrRes = NULL;

    hr = GetProperty_Variant (pDisp, pchName, &varRes);
    if (hr)
        goto Cleanup;

    if (VT_BSTR != V_VT(&varRes))
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    (*pbstrRes) = V_BSTR(&varRes);

Cleanup:

    return hr;
}

// ========================================================================
// CEventSink::IDispatch
// ========================================================================

// The event sink's IDispatch interface is what gets called when events
// are fired.

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::CEventSink::GetTypeInfoCount
//              CAnchorClick::CEventSink::GetTypeInfo
//              CAnchorClick::CEventSink::GetIDsOfNames
//
//  Synopsis:   We don't really need a nice IDispatch... this minimalist
//              version does just plenty.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
CAnchorClick::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CAnchorClick::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                      /* [in] */ LCID /*lcid*/,
                                      /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CAnchorClick::CEventSink::GetIDsOfNames( REFIID          riid,
                                         OLECHAR**       rgszNames,
                                         UINT            cNames,            
                                         LCID            lcid,
                                         DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::CEventSink::Invoke
//
//  Synopsis:   This gets called for all events on our object.  (it was 
//              registered to do so in Init with attach_event.)  It calls
//              the appropriate parent functions to handle the events.
//
//-------------------------------------------------------------------------

STDMETHODIMP 
CAnchorClick::CEventSink::Invoke( DISPID dispIdMember,
                                  REFIID, LCID,
                                  WORD wFlags,
                                  DISPPARAMS* pDispParams,
                                  VARIANT* pVarResult,
                                  EXCEPINFO*,
                                  UINT* puArgErr)
{
    HRESULT hr = TRUE;
    if (m_pParent && pDispParams && pDispParams->cArgs>=1)
    {
        if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
        {
            IHTMLEventObj *pObj=NULL;

            if (SUCCEEDED(pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, 
                (void **)&pObj) && pObj))
            {
                BSTR bstrEvent=NULL;

                pObj->get_type(&bstrEvent);

                // user clicked one of our anchors
                if (! StrCmpICW (bstrEvent, L"click"))
                    hr = m_pParent->ProcOnClick();
                else if (! StrCmpICW (bstrEvent, L"ContextMenu")) 
                {
                    VARIANT vboolCancel;
                    vboolCancel.vt = VT_BOOL;
                    vboolCancel.boolVal = FALSE;
                    pObj->put_returnValue(vboolCancel);
                    hr = S_OK;
                }

                pObj->Release();
            }
        }
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorClick::CEventSink
//
//  Synopsis:   This is used to allow communication between the parent class
//              and the event sink class.  The event sink will call the ProcOn*
//              methods on the parent at the appropriate times.
//
//-------------------------------------------------------------------------

CAnchorClick::CEventSink::CEventSink (CAnchorClick * pParent)
{
    m_pParent = pParent;
}

// ========================================================================
// CEventSink::IUnknown
// ========================================================================

// Vanilla IUnknown implementation for the event sink.

STDMETHODIMP 
CAnchorClick::CEventSink::QueryInterface(REFIID riid, void ** ppUnk)
{
    void * pUnk = NULL;

    if (riid == IID_IDispatch)
        pUnk = (IDispatch *) this;

    if (riid == IID_IUnknown)
        pUnk = (IUnknown *) this;

    if (pUnk)
    {
        *ppUnk = pUnk;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) 
CAnchorClick::CEventSink::AddRef(void)
{
    return ((IElementBehavior *)m_pParent)->AddRef();
}

STDMETHODIMP_(ULONG) 
CAnchorClick::CEventSink::Release(void)
{
    return ((IElementBehavior *)m_pParent)->Release();
}

