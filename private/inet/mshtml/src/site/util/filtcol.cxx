//=================================================================
//
//   File:      filtcol.cxx
//
//  Contents:   CFilterArray class
//
//  Classes:    CFilterArray
//
//=================================================================

#include "headers.hxx"

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "filter.hdl"

MtDefine(CCSSFilterSite, Elements, "CCSSFilterSite")
MtDefine(CFilterArray, ObjectModel, "CFilterArray")
MtDefine(CFilterArray_aryFilters_pv, CFilterArray, "CFilterArray::_aryFilters::_pv")

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CFilterSite
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : CTOR
//
//-----------------------------------------------------------------

CCSSFilterSite::CCSSFilterSite(CElement * pElem, LPCTSTR pName ) : super()
{
    _pElem    = pElem;
    _pFilter  = NULL;

    _cstrFilterName.Set(pName);
    _fSafeToScript = FALSE;
}

//+----------------------------------------------------------------
//
//  member : DTOR
//
//-----------------------------------------------------------------

CCSSFilterSite::~CCSSFilterSite()
{
    if (_pFilter)
    _pFilter->Release();
}

//+----------------------------------------------------------------
//
//  member : ClassDesc Structure
//
//-----------------------------------------------------------------

const CBase::CLASSDESC CCSSFilterSite::s_classdesc =
{
    NULL,                                // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                // _pcpi
    0,                                   // _dwFlags
    &IID_ICSSFilterSite,                 // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};


//+---------------------------------------------------------------------
//
//  Class:      CCSSFilterSite::PrivateQueryInterface
//
//  Synapsis:   Site object for CSS Extension objects on CElement
//
//------------------------------------------------------------------------
HRESULT
CCSSFilterSite::PrivateQueryInterface( REFIID iid, LPVOID *ppv )
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS((IServiceProvider *)this, IServiceProvider)
        QI_INHERITS((ICSSFilterSite *)this, ICSSFilterSite)
        QI_TEAROFF(this, IBindHost, (IOleClientSite*)this)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        
        default:
        if (iid == IID_IDispatchEx || iid == IID_IDispatch)
        {
            hr = THR(CreateTearOffThunk(this, s_apfnIDispatchEx, NULL, ppv));
        }
    }

    if (*ppv)
        ((IUnknown *)*ppv)->AddRef();
    else if (!hr)
        hr = E_NOINTERFACE;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCSSFilterSite::GetElement, public
//
//  Synopsis:   Returns the IHTMLElement pointer of the Element this site
//              belongs to.
//              This causes a hard AddRef on CElement. Clients are *not* to
//              cache this pointer.
//
//----------------------------------------------------------------------------
HRESULT
CCSSFilterSite::GetElement( IHTMLElement **ppElement )
{
    RRETURN(_pElem->QueryInterface( IID_IHTMLElement,
                       (void **)ppElement ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CCSSFilterSite::FireOnFilterEvent, public
//
//  Synopsis:   Fires an event for the extension object
//
//----------------------------------------------------------------------------
HRESULT
CCSSFilterSite::FireOnFilterChangeEvent()
{
    HRESULT     hr = S_OK;

    CBase::CLock Lock1(this);
    CBase::CLock Lock2(_pElem);
    CDoc *       pDoc = _pElem->Doc();

    CLayout * pLayout = _pElem->GetUpdatedNearestLayout();

    if (pLayout && !pLayout->ElementOwner()->GetAAdisabled())
    {
        EVENTPARAM  param(pDoc, TRUE);

        CDoc::CLock Lock(pDoc);

        param.SetNodeAndCalcCoordinates(_pElem->GetFirstBranch());
        param.SetType(_T("filterchange"));
        param.psrcFilter = _pFilter;
        if ( _pFilter )
            _pFilter->AddRef(); // Released by dtor of EVENTPARAM

        // Fire against the element - want the elements attr array
        hr = _pElem->FireEvent(
            DISPID_EVMETH_ONFILTER,
            DISPID_EVPROP_ONFILTER,
            NULL,
            (BYTE *) VTS_NONE);
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CCSSFilterSite::QueryService
//
//  Synopsis:   Get service from host. this delegates to the documents
//              implementation.
//
//-------------------------------------------------------------------------
HRESULT
CCSSFilterSite::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    if (IsEqualGUID(guidService, SID_SBindHost))
    {
    RRETURN (THR(QueryInterface(iid, ppv)));
    }
    RRETURN(_pElem->Doc()->QueryService(guidService, iid, ppv));
}

//+-----------------------------------------------------------------------
//
//  member : CCSSFilterSite::connect
//
//  SYNOPSIS : internal helper methid this fills the propbag and sets the
//      contents of the CCSSFrameSite
//
//-------------------------------------------------------------------------

HRESULT
CCSSFilterSite::Connect( LPTSTR pNameValues )
{
    CPropertyBag * pBag = NULL;
    HRESULT        hr;

    hr = THR(ParseFilterNameValuePair(pNameValues, &pBag));
    if (hr)
		goto Cleanup;

    hr = THR(_pElem->AddExtension((LPTSTR)GetFilterName(), pBag, this));

Cleanup:
    delete pBag;
    RRETURN( hr );
}


//+-----------------------------------------------------------------------
//
//  member : ( protected ) ParseFilterNameValuePair
//
//  SYNOPSIS : internal helper methid this fills the propbag and sets the
//      contents ofteh CCSSFrameSite
//
//-------------------------------------------------------------------------

HRESULT
CCSSFilterSite::ParseFilterNameValuePair(LPTSTR pchNameValue,
                     CPropertyBag **ppPropBag)
{
    HRESULT hr = S_OK;
    TCHAR chQuote = 0;
    TCHAR *pchNameStart = NULL;
    TCHAR *pchValueStart = NULL;
    TCHAR *pchNameEnd = NULL;
    TCHAR *pchValueEnd = NULL;
    TCHAR cTemp;

    if (!ppPropBag || !pchNameValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppPropBag = new CPropertyBag();
    if (!*ppPropBag)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    while (*pchNameValue)
    {
        // remove leading white space b4 name if any
        while (isspace(*pchNameValue))
            pchNameValue++;

        pchNameStart = pchNameValue;

        // cover the name until =,white space or end of string
        while (*pchNameValue && !isspace(*pchNameValue) && *pchNameValue != _T('=') && *pchNameValue != _T(':') && *pchNameValue != _T(','))
            pchNameValue++;

        pchNameEnd = pchNameValue;

        // remove trailing white space if any or other chars after name until =, or end of string
        if (isspace(*pchNameValue))
        {
            while (*pchNameValue && *pchNameValue != _T('=') && *pchNameValue != _T(':') && *pchNameValue != _T(','))
            pchNameValue++;
        }

        // if only name w/o value
        if (*pchNameValue == _T(','))
        {
            // disallow null names
            if (pchNameEnd != pchNameStart)
            {
            hr = THR((*ppPropBag)->AddProp(pchNameStart, pchNameEnd - pchNameStart, pchValueStart, 0));
            if (hr)
                goto Cleanup;
            }
            pchNameValue++;
            continue;
        }
        else if ((*pchNameValue == _T('='))||(*pchNameValue == _T(':')))
            pchNameValue++;

        // remove leading white space b4 value if any
        while (isspace(*pchNameValue))
            pchNameValue++;

        pchValueStart = pchNameValue;

        // check for quoted string
        if (*pchNameValue == _T('\'') || *pchNameValue == _T('\"'))
        {
            pchValueStart++;
            chQuote = *pchNameValue++;

            while(*pchNameValue && *pchNameValue != chQuote)
            pchNameValue++;
        }
        else // other value until white space ' " , or end of string
        {
            while(*pchNameValue && !isspace(*pchNameValue) && *pchNameValue != _T(',') &&
              *pchNameValue != _T('\"') && *pchNameValue != _T('\''))
            pchNameValue++;
        }
        pchValueEnd = pchNameValue;

        // skip rest of the chars until next , or end of string
        while (*pchNameValue && *pchNameValue != _T(','))
            pchNameValue++;
    
        if (*pchNameValue)
            pchNameValue++;

        // disallow null names
        if (pchNameEnd != pchNameStart)
        {
            // Special handling for colors
            // add to property bag. For now just look for the name Color
            cTemp = *pchNameEnd;
            *pchNameEnd = _T('\0');
            if ( _tcsistr ( _T("Color"), pchNameStart ) )
            {
                CColorValue cuv;
                TCHAR c;

                *pchNameEnd = cTemp;
                cTemp = *pchValueEnd;

                *pchValueEnd = _T('\0');
                // BUGBUG CUnitValue::FromString needs to be able to handle non-null terminated
                // strings.
                hr = cuv.FromString ( pchValueStart, TRUE, pchValueEnd - pchValueStart );
                *pchValueEnd = cTemp;

                if ( hr )
                    continue; // Not a valid color

                CVariant varValue;
                V_VT(&varValue) = VT_I4;
                V_I4(&varValue) = cuv.GetIntoRGB();

                c = *pchNameEnd;
                *pchNameEnd = _T('\0');
                hr = THR( (*ppPropBag)->Write (pchNameStart, &varValue ) );
                *pchNameEnd = c;
            }
            else
            {
                *pchNameEnd = cTemp;
                hr = THR((*ppPropBag)->AddProp(pchNameStart, pchNameEnd - pchNameStart,
                    pchValueStart, pchValueEnd - pchValueStart ));
            }
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CCSSFilterSite::InvokeEx(DISPID dispidMember,
                 LCID lcid,
                 WORD wFlags,
                 DISPPARAMS * pdispparams,
                 VARIANT * pvarResult,
                 EXCEPINFO * pexcepinfo,
                 IServiceProvider *pSrvProvider)
{
    HRESULT hr = S_OK;

    if (!pvarResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    switch (dispidMember)
    {
        case DISPID_AMBIENT_PALETTE:
        V_VT(pvarResult) = VT_HANDLE;
        V_BYREF(pvarResult) = (void *)_pElem->Doc()->GetPalette();
        break;
    }

Cleanup:
    RRETURN(hr);
}


// defined in site\ole\OleBindh.cxx:
extern HRESULT
MonikerBind(
    CDoc *                  pDoc,
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppv,
    BOOL                    fObject,
    DWORD                   dwCompatFlags);


BEGIN_TEAROFF_TABLE(CCSSFilterSite, IBindHost)
    TEAROFF_METHOD(CCSSFilterSite, CreateMoniker, createmoniker,
    (LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved))
    TEAROFF_METHOD(CCSSFilterSite, MonikerBindToStorage, monikerbindtostorage,
    (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
    TEAROFF_METHOD(CCSSFilterSite, MonikerBindToObject, monikerbindtoobject,
    (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
END_TEAROFF_TABLE()

//+---------------------------------------------------------------------------
//
//  Member:     CCSSFilterSite::CreateMoniker, IBindHost
//
//  Synopsis:   Parses display name and returns a URL moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCSSFilterSite::CreateMoniker(LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved)
{
    TCHAR       cBuf[pdlUrlLen];
    TCHAR *     pchUrl = cBuf;
    HRESULT     hr;

    hr = THR(_pElem->Doc()->ExpandUrl(szName, ARRAY_SIZE(cBuf), pchUrl, _pElem));
        if (hr)
        goto Cleanup;

    hr = THR(CreateURLMoniker(NULL, pchUrl, ppmk));
        if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCSSFilterSite::MonikerBindToStorage, IBindHost
//
//  Synopsis:   Calls BindToStorage on the moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCSSFilterSite::MonikerBindToStorage(
    IMoniker * pmk,
    IBindCtx * pbc,
    IBindStatusCallback * pbsc,
    REFIID riid,
    void ** ppvObj)
{
    RRETURN1(MonikerBind(
        _pElem->Doc(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        FALSE,
        0), S_ASYNCHRONOUS);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCSSFilterSite::MonikerBindToObject, IBindHost
//
//  Synopsis:   Calls BindToObject on the moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCSSFilterSite::MonikerBindToObject(
    IMoniker * pmk,
    IBindCtx * pbc,
    IBindStatusCallback * pbsc,
    REFIID riid,
    void ** ppvObj)
{
    RRETURN1(MonikerBind(
        _pElem->Doc(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        TRUE,
        0), S_ASYNCHRONOUS);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CFilterArray
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CFilterArray::s_classdesc =
{
    &CLSID_HTMLFiltersCollection,             // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLFiltersCollection,        // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};


ULONG CFilterArray::PrivateAddRef ( void )
{
    Assert ( _pElemOwner );
    return _pElemOwner->AddRef();
}

ULONG CFilterArray::PrivateRelease( void )
{
    Assert ( _pElemOwner );
    return _pElemOwner->Release();
}

HRESULT
CFilterArray::ParseFilterProperty( LPCTSTR pcszFilter, CTreeNode * pNodeOwner )
{
    HRESULT hr = S_OK;
    LPTSTR pszCopy;
    LPTSTR pszString;
    LPTSTR pszNextToken;
    LPTSTR pszBegin;
    LPTSTR pszEnd;
    CCSSFilterSite * pFilterSite = NULL;

    pszString = pszCopy = _tcsdup( pcszFilter );

    Assert ( pNodeOwner );

    for ( ; pszString && *pszString; pszString = pszNextToken )
    {
        pszEnd = pszNextToken = NextParenthesizedToken( pszString );
        if ( pszNextToken && *pszNextToken )
        {
            *pszNextToken++ = _T('\0');
            while ( _istspace( *pszNextToken ) )
            pszNextToken++;       // Skip any leading whitespace
            if ( pszEnd > pszString )
            pszEnd--;   // Go back to the ')'.
        }
        else
            pszEnd = pszString + _tcslen( pszString ) - 1;  // We know tcslen > 1 because *pszString != '\0'
        if ( *pszEnd == _T(')') )
        {
            *pszEnd-- = _T('\0');
            while ( ( pszEnd > pszString ) && _istspace( *pszEnd ) )
            *pszEnd-- = _T('\0');       // Skip any trailing whitespace
        }
        else
            ;   // Uh-oh... we got an unterminated functional notation, or just a string.

        for ( pszBegin = pszString; ; pszBegin++ )
        {
            if ( *pszBegin == _T('(') )
            {
                *pszBegin++ = _T('\0');
                while ( _istspace( *pszBegin ) )
                    pszBegin++;       // Skip any leading whitespace
                break;
            }
            else if ( ! *pszBegin )
            {
                break;
            }
        }
        // pszString should point to the function name, and pszBegin should point to the parameter string

        pFilterSite = new CCSSFilterSite(pNodeOwner->Element(), pszString);
        if (!pFilterSite)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pFilterSite->Connect(pszBegin));
        if (!hr)
        {
            AddFilterSite(pFilterSite);
        }
        else
        {
            delete pFilterSite;
        }
        pszEnd = pszBegin;
    }

Cleanup:
    if ( pszCopy )
    MemFree ( pszCopy );
    return hr;
}

//+----------------------------------------------------------------
//
//  member : CTOR
//
//+----------------------------------------------------------------

CFilterArray::CFilterArray(CElement * pOwner,
               LPCTSTR    pText) : _pElemOwner(pOwner)
{
    _aryFilters.SetSize(0);
    _strFullText.Set(pText);
}

//+----------------------------------------------------------------
//
//  member : DTOR
//
//+----------------------------------------------------------------

CFilterArray::~CFilterArray()
{
    Passivate();
}


//+----------------------------------------------------------------
//
//  member : CFilterArray::Passivate
//
//+----------------------------------------------------------------

void
CFilterArray::Passivate()
{
    long  lSize = _aryFilters.Size();
    long  i;

    for (i=0; i<lSize; i++)
    {
        if (_aryFilters[i]->GetExtension())
            _aryFilters[i]->GetExtension()->SetSite(NULL);

        _aryFilters[i]->Release();
    }

    _strFullText.Free();

    _aryFilters.SetSize(0);

    super::Passivate();
}

HRESULT
CFilterArray::OnAmbientPropertyChange ( DISPID dspID )
{
    long  lSize = _aryFilters.Size();
    long  i;

    for (i=0; i<lSize; i++)
    {
        if (_aryFilters[i]->GetExtension())
            _aryFilters[i]->GetExtension()->OnAmbientPropertyChange(dspID);
    }
    return S_OK;
}

HRESULT
CFilterArray::OnCommand ( COnCommandExecParams * pParm )
{
    long  lSize = _aryFilters.Size();
    long  i;

    for (i=0; i<lSize; i++)
    {
        if (_aryFilters[i]->GetExtension())
        {
            CTExec(_aryFilters[i]->GetExtension(), pParm->pguidCmdGroup, pParm->nCmdID,
                       pParm->nCmdexecopt, pParm->pvarargIn,
                       pParm->pvarargOut);
        }
    }
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member  : CFilterArray::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CFilterArray::PrivateQueryInterface(REFIID iid, void **ppv)
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
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLFiltersCollection, NULL, ppv));
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
//  Member  : CFilterArray::length
//
//  Sysnopsis : IHTMLFiltersCollection interface method
//
//----------------------------------------------------------------

HRESULT
CFilterArray::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    EnsureCollection();

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aryFilters.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : CFilterArray::item
//
//  Sysnopsis : IHTMLFilterCollection interface method
//      actually returns ICSSFilter pointer
//
//----------------------------------------------------------------

HRESULT
CFilterArray::item(VARIANT * pvarIndex, VARIANT * pvarRet)
{
    HRESULT   hr   = S_OK;
    long      lIndex;
    CVariant  varArg;

    if (!pvarRet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // ECMA rule - return empty for access beyond array bounds
    V_VT(pvarRet) = VT_EMPTY;

    // first attempt ordinal access...
    hr = THR(varArg.CoerceVariantArg(pvarIndex, VT_I4));
    if (hr==S_OK)
    {
        lIndex = V_I4(&varArg);
    }
    else
    {
    // not a number, try named access
    hr = THR_NOTRACE(varArg.CoerceVariantArg(pvarIndex, VT_BSTR));
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    else
    {
        // find the filter with this name
        lIndex = FindByName((LPTSTR)V_BSTR(&varArg));
    }
    }

    hr = THR(GetItem(lIndex, pvarRet));
    if(hr)
    {
        if(hr == S_FALSE)
            hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------
//
//  Member  : CFilterArray::GetItem
//
//  Sysnopsis : Returns the item that has given index in the collection.
//              If the index is out of range returns S_FALSE.
//              If ppDisp is NULL only checks that range and returns
//               S_OK if index is in range, S_FALSE if out
//
//----------------------------------------------------------------

HRESULT
CFilterArray::GetItem ( long lIndex, VARIANT *pvar )
{
    HRESULT hr = S_OK;
    ICSSFilter * piFilter = NULL;

    EnsureCollection();

    if (lIndex < 0 || lIndex >= _aryFilters.Size())
    {
        if(pvar)
            V_DISPATCH(pvar) = NULL;
        hr = S_FALSE;
        goto Cleanup;
    }

    if(!pvar)
    {
        // Not ppDisp, caller wanted only to check for correct range
        hr = S_OK;
        goto Cleanup;
    }

    V_DISPATCH(pvar) = NULL;

    // make sure that this filter is safe to script
    if ( !_aryFilters[lIndex]->SafeToScript() )
        goto Cleanup;

    piFilter = _aryFilters[lIndex]->GetExtension();
    if (!piFilter)
    {
        goto Cleanup;
    }

    // Either QI For IDispatchEx or get the type library & Invoke (Just like OBJECT's)
    hr = THR(piFilter->QueryInterface(IID_IDispatch, (void **) &V_DISPATCH(pvar)));
    if ( hr == E_NOINTERFACE )
    {
        // equivalent to not being automatable - still takes up slot in collection
        hr = S_OK;
    }

    V_VT(pvar) = VT_DISPATCH;
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member  : CFilterArray::_newEnum
//
//  Sysnopsis :
//
//----------------------------------------------------------------

HRESULT
CFilterArray::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    EnsureCollection();

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryFilters.EnumVARIANT(VT_BSTR,
                (IEnumVARIANT**)ppEnum,
                FALSE,
                FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------
//
//  Member  : CFilterArray::FindByName
//
//  Sysnopsis : this helper function will return the index of the
//      filter that has the name. other wise it returns -1.
//
//----------------------------------------------------------------

long
CFilterArray::FindByName(LPCTSTR pszTargetName,
                   BOOL fCaseSensitive /* ==true */)
{
    EnsureCollection();

    long  lSize = _aryFilters.Size();
    long  lIdx;
    STRINGCOMPAREFN pfnStrCmp = fCaseSensitive ? StrCmpC : StrCmpI;

    for (lIdx=0; lIdx < lSize; lIdx++ )
    {
        if ( !pfnStrCmp( pszTargetName, _aryFilters[lIdx]->GetFilterName() ) )
        {
            return lIdx;
        }
    }

    return -1;
}


//+---------------------------------------------------------------
//
//  Member  : CFilterArray::AddFilterSite
//
//  Sysnopsis : this helper function will add a filter record to the
//      internal array. it returns the index where the filter was
//      added, or -1 if there was an error
//
//----------------------------------------------------------------

long
CFilterArray::AddFilterSite(CCSSFilterSite * pFilter)
{
    long lIdx = -1;
    long lSize = _aryFilters.Size();

    if (!pFilter)
        goto Cleanup;

    if (S_OK == _aryFilters.Append(pFilter))
        lIdx = lSize;

Cleanup:
    return lIdx;
}

//+---------------------------------------------------------------
//
//  Member  : CFilterArray::RemoveFilter
//
//  Sysnopsis : this helper function will remove a filter record
//      from the internal array
//
//----------------------------------------------------------------

HRESULT
CFilterArray::RemoveFilterSite(long lIndex)
{
    HRESULT hr = S_OK;

    if (lIndex < 0 || lIndex >= _aryFilters.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // release the object ref's

    if (_aryFilters[lIndex]->GetExtension())
    {
        // tell the filter to release filterSite
        _aryFilters[lIndex]->GetExtension()->SetSite(NULL);
    }

    _aryFilters.ReleaseAndDelete(lIndex);

Cleanup:
    RRETURN( hr );
}


void
CFilterArray::EnsureCollection ( void )
{
    Assert ( _pElemOwner  );
    if (_pElemOwner && _pElemOwner->GetFirstBranch())
    {
        // Getting the fancy format builds the filters collection
        _pElemOwner->GetFirstBranch()->GetFancyFormat();
        if (_pElemOwner->_fHasPendingFilterTask)
        {
            // This could call out to external code.
            _pElemOwner->Doc()->ExecuteSingleFilterTask(_pElemOwner);
        }
    }
}


//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::GetName
//
//  Sysnopsis : This virtual function returns the name of given item.
//              In out case it just returns the index as a string
//----------------------------------------------------------------

LPCTSTR 
CFilterArray::GetName(long lIdx)
{
    return _aryFilters[lIdx]->GetFilterName();
}
