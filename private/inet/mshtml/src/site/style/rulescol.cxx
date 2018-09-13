//=================================================================
//
//   File:      rulescol.cxx
//
//  Contents:   CStyleSheetRuleArray class
//
//  Classes:    CStyleSheetRuleArray
//
//=================================================================

#include "headers.hxx"

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_RULESCOL_HXX_
#define X_RULESCOL_HXX_
#include "rulescol.hxx"
#endif

#ifndef X_RULESTYL_HXX_
#define X_RULESTYL_HXX_
#include "rulestyl.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "rulescol.hdl"

MtDefine(CStyleSheetRule, StyleSheets, "CStyleSheetRule")
MtDefine(CStyleSheetRuleArray, StyleSheets, "CStyleSheetRuleArray")


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CStyleSheetRuleArray
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------
const CBase::CLASSDESC CStyleSheetRuleArray::s_classdesc =
{
    &CLSID_HTMLStyleSheetRulesCollection,   // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLStyleSheetRulesCollection,    // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};

//+----------------------------------------------------------------
//
//  member : CTOR
//
//+----------------------------------------------------------------
CStyleSheetRuleArray::CStyleSheetRuleArray( CStyleSheet *pStyleSheet ) : _pStyleSheet(pStyleSheet)
{
}

//+----------------------------------------------------------------
//
//  member : DTOR
//
//+----------------------------------------------------------------

CStyleSheetRuleArray::~CStyleSheetRuleArray()
{
    Passivate();
}

//+----------------------------------------------------------------
//
//  member : StyleSheetRelease()
//      This method calls Release(), but it is called from the parent
//  stylesheet during its destruction - so we need to make sure to
//  clear our pointer to the parent stylesheet at the same time.
//
//+----------------------------------------------------------------
int CStyleSheetRuleArray::StyleSheetRelease()
{
    Assert( _pStyleSheet );
    _pStyleSheet = NULL;
    return Release();
}

//+---------------------------------------------------------------
//
//  Member  : CStyleSheetRuleArray::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------
HRESULT
CStyleSheetRuleArray::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        default:
        {
            const CLASSDESC *pclassdesc = BaseDesc();

            if (pclassdesc &&
                pclassdesc->_piidDispinterface &&
                (iid == *pclassdesc->_piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLStyleSheetRulesCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------
//
//  Member  : CStyleSheetRuleArray::length
//
//  Sysnopsis : IHTMLFiltersCollection interface method
//
//----------------------------------------------------------------

HRESULT
CStyleSheetRuleArray::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (_pStyleSheet ) 
        *pLength = _pStyleSheet->GetNumRules();
    else
        *pLength = 0;

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : CStyleSheetRuleArray::item
//
//  Sysnopsis : IHTMLStyleSheetRulesCollection interface method
//
//----------------------------------------------------------------

HRESULT
CStyleSheetRuleArray::item(long lIndex, IHTMLStyleSheetRule **ppSSRule)
{
    HRESULT         hr;
    VARIANT         varDispatch;

    if (!ppSSRule)
    {
        RRETURN(E_POINTER);
    }

    hr = THR(GetItem(lIndex, &varDispatch));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&varDispatch) == VT_DISPATCH);
    *ppSSRule = (IHTMLStyleSheetRule *) V_DISPATCH(&varDispatch);

Cleanup:
    RRETURN(hr);
}

HRESULT 
CStyleSheetRuleArray::GetItem (long lIndex, VARIANT *pvar)
{
    HRESULT hr = S_OK;

    // ppSSRule is NULL if we're validating lIndex 
    if ( pvar )    
        V_DISPATCH(pvar) = NULL;

    if ( !_pStyleSheet )
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    if ( lIndex < 0 || lIndex >= _pStyleSheet->GetNumRules())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( pvar )
    {
        hr = _pStyleSheet->GetOMRule( lIndex, (IHTMLStyleSheetRule**) &V_DISPATCH(pvar) );
        if (hr)
            goto Cleanup;

        V_VT(pvar) = VT_DISPATCH;
    }

Cleanup:
    RRETURN(hr);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CStyleSheetRule
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : ClassDesc Structure
//
//-----------------------------------------------------------------

const CBase::CLASSDESC CStyleSheetRule::s_classdesc =
{
    NULL,                                // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                // _pcpi
    0,                                   // _dwFlags
    &IID_IHTMLStyleSheetRule,            // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};


//+----------------------------------------------------------------
//
//  member : CTOR
//
//-----------------------------------------------------------------

CStyleSheetRule::CStyleSheetRule( CStyleSheet *pStyleSheet, DWORD dwSID, ELEMENT_TAG eTag ) :
    _pStyleSheet(pStyleSheet),
    _dwID(dwSID),
    _eTag(eTag),
    _pStyle(NULL)
{
}

//+----------------------------------------------------------------
//
//  member : DTOR
//
//-----------------------------------------------------------------

CStyleSheetRule::~CStyleSheetRule()
{
    if ( _pStyle )
    {
        _pStyle->ClearRule();
        _pStyle->PrivateRelease();
    }
}

//+----------------------------------------------------------------
//
//  member : StyleSheetRelease()
//      This method calls Release(), but it is called from the parent
//  stylesheet during its destruction - so we need to make sure to
//  clear our pointer to the parent stylesheet at the same time.
//
//+----------------------------------------------------------------
int CStyleSheetRule::StyleSheetRelease()
{
    Assert( _pStyleSheet );
    _pStyleSheet = NULL;
    return Release();
}

CStyleRule *CStyleSheetRule::GetRule()
{   // Can't be inlined because of undefined CStyleSheetArray class.
    if ( _pStyleSheet )
    {
        CStyleID sid(_dwID);
        return _pStyleSheet->GetRule( _eTag, sid );
    }
    return NULL;
}

//+---------------------------------------------------------------------
//
//  Class:      CStyleSheetRule::PrivateQueryInterface
//
//------------------------------------------------------------------------
HRESULT
CStyleSheetRule::PrivateQueryInterface( REFIID iid, LPVOID *ppv )
{
    HRESULT hr = S_OK;

    if ( !ppv )
        return E_INVALIDARG;

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF_DISPEX(this, NULL)

    default:
        if (iid == IID_IHTMLStyleSheetRule)
        {
            hr = THR(CreateTearOffThunk(this, s_apfnIHTMLStyleSheetRule, NULL, ppv));
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    if ( *ppv )
        ((IUnknown *)*ppv)->AddRef();

    RRETURN(hr);
}

//*********************************************************************
//  CStyleSheetRule::selectorText
//      IHTMLStyleSheetRule interface method
//*********************************************************************
HRESULT
CStyleSheetRule::get_selectorText(BSTR *pBSTR)
{
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

    if (!pBSTR)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pBSTR = NULL;

    if ( _pStyleSheet )
    {
        CStyleID sid( _dwID );

        CStyleRule *pRule = _pStyleSheet->GetRule( _eTag, sid );
        Assert( pRule );
        CStyleSelector *pSelector = pRule->GetSelector();
        if ( pSelector )
        {
            CStr cstrSelector;
            pSelector->GetString( &cstrSelector );
            cstrSelector.TrimTrailingWhitespace();
            hr = cstrSelector.AllocBSTR( pBSTR );
        }
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CStyleSheetRule::put_selectorText(BSTR bstr)
{
    HRESULT hr = E_NOTIMPL;

//Cleanup:
    RRETURN1( SetErrorInfo( hr ), E_NOTIMPL);
}

HRESULT CStyleSheetRule::get_style( IHTMLRuleStyle **ppStyle )
{
    HRESULT hr = S_OK;

    if ( !ppStyle )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *ppStyle = NULL;

    if ( !_pStyle )
    {
        _pStyle = new CRuleStyle( this );
        if ( !_pStyle )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = _pStyle->QueryInterface( IID_IHTMLRuleStyle, (void**)ppStyle);

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

HRESULT CStyleSheetRule::get_readOnly( VARIANT_BOOL * pvbReadOnly )
{
    Assert( pvbReadOnly );
    
    RRETURN( SetErrorInfo( _pStyleSheet->get_readOnly( pvbReadOnly ) ) );
}

