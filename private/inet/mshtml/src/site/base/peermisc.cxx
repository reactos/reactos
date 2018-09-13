#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

MtDefine(CPeerUrnCollection, Elements, "CPeerUrnCollection")

const CBase::CLASSDESC CPeerUrnCollection::s_classdesc =
{
    &CLSID_HTMLUrnCollection,               // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLUrnCollection,                // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};

//////////////////////////////////////////////////////////////////////////////
//
// Class:  CPeerUrnCollection
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CPeerUrnCollection constructor
//
//----------------------------------------------------------------------------

CPeerUrnCollection::CPeerUrnCollection( CElement *pElement ) : _pElement(pElement)
{
    pElement->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerUrnCollection destructor
//
//----------------------------------------------------------------------------

CPeerUrnCollection::~CPeerUrnCollection()
{
    Passivate();
    _pElement->Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerUrnCollection::PrivateQueryInterface
//
//----------------------------------------------------------------------------

HRESULT
CPeerUrnCollection::PrivateQueryInterface(REFIID iid, void **ppv)
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

            // BUGBUG (alexz) replace this with QI_TEAROFF
            if (pclassdesc &&
                pclassdesc->_piidDispinterface &&
                (iid == *pclassdesc->_piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLUrnCollection, NULL, ppv));
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

//+---------------------------------------------------------------------------
//
//  Member  : CPeerUrnCollection::length
//
//----------------------------------------------------------------------------

HRESULT
CPeerUrnCollection::get_length(long * pLength)
{
    HRESULT                 hr = S_OK;
    CPeerHolder::CListMgr   List;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = 0;
    Assert(_pElement);

    if (_pElement->HasPeerHolder())
    {
        List.Init(_pElement->GetPeerHolder());

        *pLength = List.GetPeerHolderCount(/* fNonEmptyOnly = */TRUE);
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerUrnCollection::item
//
//----------------------------------------------------------------------------

HRESULT
CPeerUrnCollection::item(long lIndex, BSTR *ppUrn)
{
    HRESULT                 hr;
    VARIANT                 varBstr;

    if (!ppUrn)
    {
        RRETURN(E_POINTER);
    }

    hr = THR(GetItem(lIndex, &varBstr));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&varBstr) == VT_BSTR);
    *ppUrn = V_BSTR(&varBstr);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerUrnCollection::GetItem
//
//----------------------------------------------------------------------------

HRESULT 
CPeerUrnCollection::GetItem (long lIndex, VARIANT *pvar)
{
    HRESULT                 hr = E_INVALIDARG;
    CPeerHolder::CListMgr   List;
    CPeerHolder *           pPeerHolder;

    // this is NULL if we're validating lIndex 
    if (pvar)    
        V_BSTR(pvar) = NULL;

    if ( !_pElement || !_pElement->HasPeerHolder())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    List.Init(_pElement->GetPeerHolder());
    pPeerHolder = List.GetPeerHolderByIndex(lIndex, /* fNonEmptyOnly = */TRUE);

    if (!pPeerHolder)
        goto Cleanup;

    // pvar can be NULL when checking for valid index
    if (pvar)
    {
        hr = THR(pPeerHolder->_cstrUrn.AllocBSTR(&V_BSTR(pvar)));
        if (hr)
            goto Cleanup;

        V_VT(pvar) = VT_BSTR;
    }
    else
        hr = S_OK;

Cleanup:
    RRETURN(hr);
}
