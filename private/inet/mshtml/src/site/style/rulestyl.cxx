//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       rulestyl.cxx
//
//  Contents:   Support for Cascading Style Sheets Object Model - style object
//              that hangs off a Rule.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_RULESTYL_HXX_
#define X_RULESTYL_HXX_
#include "rulestyl.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_RULESCOL_HXX_
#define X_RULESCOL_HXX_
#include "rulescol.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

MtDefine(CRuleStyle, StyleSheets, "CRuleStyle")

//+---------------------------------------------------------------------------
//
// CRuleStyle
//
//----------------------------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Member:     CRuleStyle::CRuleStyle
//
//-------------------------------------------------------------------------
CRuleStyle::CRuleStyle( CStyleSheetRule *pRule ) :
    CStyle(NULL, 0, 0), _pRule(pRule)
{
    CStyleRule *pSSRule;

    if ( _pRule && NULL != (pSSRule = _pRule->GetRule() ) )
    {
        _pAA = pSSRule->_paaStyleProperties;
    }
    else
        _pAA = NULL;
    // CStyle::~CStyle will clean the _pAA up for us (that is, set it to
    // NULL so that CBase::~CBase will not destroy it, since it doesn't
    // belong to us.  Look at CStyle::Passivate() for more info.
}

//+------------------------------------------------------------------------
//
//  Member:     CRuleStyle::~CRuleStyle
//
//-------------------------------------------------------------------------
CRuleStyle::~CRuleStyle()
{
    Passivate();
}

void CRuleStyle::Passivate()
{
    if ( !_pRule )
        delete _pAA;    // We don't have a rule, this must just be junk floating around.
    _pAA = NULL;
    super::Passivate();
}

ULONG CRuleStyle::PrivateAddRef ( void )
{
    return CBase::PrivateAddRef();
}

ULONG CRuleStyle::PrivateRelease( void )
{
    return CBase::PrivateRelease();
}

const CRuleStyle::CLASSDESC CRuleStyle::s_classdesc =
{
    {
        &CLSID_HTMLRuleStyle,                // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLRuleStyle,                 // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLRuleStyle,                    // _apfnTearOff
};

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//-------------------------------------------------------------------------
HRESULT CRuleStyle::OnPropertyChange( DISPID dispid, DWORD dwFlags )
{
    HRESULT hr = S_OK;
    
    if (TestFlag(STYLE_MASKPROPERTYCHANGES))
        goto Cleanup;

    if (_pRule && _pRule->GetStyleSheet())
    {
        CMarkup *pMarkup = _pRule->GetStyleSheet()->GetMarkup();
        Assert(pMarkup);

        CDoc *pDoc = pMarkup->Doc();
        Assert( pDoc );

        if(dispid == DISPID_A_POSITION)
        {
            if(!pDoc->NeedRegionCollection())
            {
                DWORD dwVal;
                CAttrArray * pAA = *GetAttrArray();  Assert(pAA);
                BOOL fFound = pAA->FindSimple(DISPID_A_POSITION, &dwVal);

                if(fFound && ((stylePosition)dwVal == stylePositionrelative || 
                    (stylePosition)dwVal == stylePositionabsolute))
                {
                    pDoc->_fRegionCollection = TRUE;
                }
            }
        }

        hr = THR( pMarkup->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ (DISPID_A_BEHAVIOR == dispid) ) );

        goto Cleanup; // done
//        RRETURN( pDoc->_pSiteRoot->_pElementHTML->OnPropertyChange(dispid, dwFlags | ELEMCHNG_INLINESTYLE_PROPERTY));
    }
    
    hr = E_FAIL;
    goto Cleanup;

Cleanup:

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member:     CRuleStyle::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------
HRESULT
CRuleStyle::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    // All interfaces derived from IDispatch must be handled
    // using the ElementDesc()->_apfnTearOff tearoff interface.
    // This allows classes such as COleSite to override the
    // implementation of IDispatch methods.

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_HTML_TEAROFF(this, IHTMLRuleStyle2, NULL)

    default:
        {
            const CLASSDESC *pclassdesc = (CLASSDESC *) BaseDesc();
            if (pclassdesc &&
                pclassdesc->_apfnTearOff &&
                pclassdesc->_classdescBase._piidDispinterface &&
                (iid == *pclassdesc->_classdescBase._piidDispinterface || DispNonDualDIID(iid)) 
                && _pRule)
            {
                hr = THR(CreateTearOffThunk(this, pclassdesc->_apfnTearOff, NULL, ppv, 
                                                (void *)(CRuleStyle::s_ppropdescsInVtblOrderIHTMLRuleStyle)));
            }
        }
    }
    if (!hr)
    {
        if (*ppv)
            (*(IUnknown **)ppv)->AddRef();
        else
            hr = E_NOINTERFACE;
    }
    RRETURN(hr);
}

CAttrArray **CRuleStyle::GetAttrArray ( void ) const
{
    CStyleRule *pRule;

    if ( _pRule && NULL != (pRule = _pRule->GetRule() ) )
        return &pRule->_paaStyleProperties;
    else
        return (CAttrArray **)&_pAA;    // In case we're disconnected, we still need to
                                        // support GetAA.  This is a junk-heap AA - it will
                                        // exist only to satisfy this requirement, and get
                                        // thrown away when we destruct.
}

CAtomTable *CRuleStyle::GetAtomTable ( BOOL *pfExpando )
{
    if ( _pRule && _pRule->GetStyleSheet() )
    {
        CDoc *pDoc = _pRule->GetStyleSheet()->GetDocument();
        Assert( pDoc );
        return pDoc->GetAtomTable(pfExpando);
    }
    return NULL;
};


// We use this to make sure that if an AA is creatid during CBase::GetDispID 
// it goes to the right place (CRuleStyle does not own an AttrArray)
HRESULT
CRuleStyle::GetDispID(BSTR bstrName, DWORD grfdex,  DISPID *pid)
{
    HRESULT         hr;
    BOOL            fNoAA;
    CStyleRule    * pSSRule = NULL;

    if(!_pAA && _pRule && NULL != (pSSRule = _pRule->GetRule()))
    {
        if(pSSRule->_paaStyleProperties)
            _pAA = pSSRule->_paaStyleProperties;
    }

    fNoAA = (_pAA == NULL);

    hr = THR_NOTRACE(CBase::GetDispID(bstrName, grfdex, pid));

    if(fNoAA && _pAA)
    {
        // The AA was created by CBase::InvokeEx, make sure we save it in the owner
        if(pSSRule)
        {
            Assert(pSSRule->_paaStyleProperties == NULL);
            pSSRule->_paaStyleProperties = _pAA;
        }
        else
        {
            delete _pAA;
            _pAA = NULL;
        }
    }

    RRETURN1(hr, DISP_E_UNKNOWNNAME);
}



//*********************************************************************
// CRuleStyle::Invoke, IDispatch
// Provides access to properties and members of the object. We use it
//      to invalidate the caches when a expando is changed on the style
//      so that it is propagated down to the elements it affects
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
//*********************************************************************

STDMETHODIMP
CRuleStyle::InvokeEx( DISPID       dispidMember,
                        LCID         lcid,
                        WORD         wFlags,
                        DISPPARAMS * pdispparams,
                        VARIANT *    pvarResult,
                        EXCEPINFO *  pexcepinfo,
                        IServiceProvider *pSrvProvider)
{
    HRESULT         hr = DISP_E_MEMBERNOTFOUND;
    BOOL            fNoAA;
    CStyleRule    * pSSRule = NULL;

    if(!_pAA && _pRule && NULL != (pSSRule = _pRule->GetRule()))
    {
        if(pSSRule->_paaStyleProperties)
            _pAA = pSSRule->_paaStyleProperties;
    }

    fNoAA = (_pAA == NULL);

    // Jump directly to CBase. super:: will try to invalidate a branch
    hr = THR_NOTRACE(CBase::InvokeEx( dispidMember,
                                    lcid,
                                    wFlags,
                                    pdispparams,
                                    pvarResult,
                                    pexcepinfo,
                                    pSrvProvider));

    if(fNoAA && _pAA)
    {
        // The AA was created by CBase::InvokeEx, make sure we save it in the owner
        if(pSSRule)
        {
            Assert(pSSRule->_paaStyleProperties == NULL);
            pSSRule->_paaStyleProperties = _pAA;
        }
        else
        {
            delete _pAA;
            _pAA = NULL;
        }
    }

    if(hr)
        goto Cleanup;
    

    if( (_pRule && IsExpandoDISPID(dispidMember) && (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))) || 
        (dispidMember == DISPID_IHTMLSTYLE_REMOVEATTRIBUTE && (wFlags & DISPATCH_METHOD)) )
    {
        CDoc *pDoc = _pRule->GetStyleSheet()->GetDocument();
        Assert( pDoc );
        // Invalidate the whole document, a global style has changed
        pDoc->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);
    }


Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

// All putters/getters must not have a pointer into the element attrArray is it could move.
// Use the below macros to guarantee we're pointing to a local variable which is pointing to the
// style sheet attrArray and not pointing to the attrValue on the element attrArray which can
// move if the elements attrArray has attrValues added to or deleted from.
#define GETATTR_ARRAY   \
    CAttrArray *pTempStyleAA = *GetAttrArray();

#define USEATTR_ARRAY   \
    &pTempStyleAA
    

#ifdef USE_STACK_SPEW
#pragma check_stack(off)  
#endif 
STDMETHODIMP
CRuleStyle::put_StyleComponent(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_StyleComponentHelper(v, pPropDesc, GetAttrArray());
}

STDMETHODIMP
CRuleStyle::put_Url(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_UrlHelper(v, pPropDesc, GetAttrArray());
}

STDMETHODIMP
CRuleStyle::put_String(BSTR v)
{
    GET_THUNK_PROPDESC
    return put_StringHelper(v, pPropDesc, GetAttrArray());
}

STDMETHODIMP
CRuleStyle::put_Long(long v)
{
    GET_THUNK_PROPDESC
    return put_LongHelper(v, pPropDesc, GetAttrArray());
}


STDMETHODIMP
CRuleStyle::put_Bool(VARIANT_BOOL v)
{
    GET_THUNK_PROPDESC
    return put_BoolHelper(v, pPropDesc, GetAttrArray());
}

STDMETHODIMP
CRuleStyle::put_Variant(VARIANT v)
{
    GET_THUNK_PROPDESC
    return put_VariantHelper(v, pPropDesc, GetAttrArray());
}

STDMETHODIMP
CRuleStyle::put_DataEvent(VARIANT v)
{
    GET_THUNK_PROPDESC
    return put_DataEventHelper(v, pPropDesc, GetAttrArray());
}

STDMETHODIMP
CRuleStyle::get_Url(BSTR *p)
{
    GET_THUNK_PROPDESC
    GETATTR_ARRAY
    if (!pTempStyleAA)
    {
        *p = NULL;
        return S_OK;
    }
    else
        return get_UrlHelper(p, pPropDesc, USEATTR_ARRAY);
}

STDMETHODIMP
CRuleStyle::get_StyleComponent(BSTR *p)
{
    GET_THUNK_PROPDESC
    GETATTR_ARRAY
    if (!pTempStyleAA)
    {
        *p = NULL;
        return S_OK;
    }
    else
        return get_StyleComponentHelper(p, pPropDesc, USEATTR_ARRAY);
}

STDMETHODIMP
CRuleStyle::get_Property(void *p)
{
    GET_THUNK_PROPDESC
    GETATTR_ARRAY
    if (!pTempStyleAA)
    {
        return S_OK;
    }
    else
        return get_PropertyHelper(p, pPropDesc, USEATTR_ARRAY);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)  
#endif 

#ifndef NO_EDIT
IOleUndoManager * 
CRuleStyle::UndoManager(void) 
{ 
    return _pRule->UndoManager(); 
}

BOOL 
CRuleStyle::QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange)
{
    return _pRule->QueryCreateUndo( fRequiresParent, fDirtyChange );
}
#endif // NO_EDIT
