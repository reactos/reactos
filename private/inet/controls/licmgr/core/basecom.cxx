//+----------------------------------------------------------------------------
//  File:       basecom.cxx
//
//  Synopsis:   This file contains implementations of the root COM objects
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <core.hxx>


//+----------------------------------------------------------------------------
//  Function:   SRelease, SClear
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
void
SRelease(
    IUnknown *  pUnk)
{
    if (pUnk)
    {
        pUnk->Release();
    }
}


void
SClear(
    IUnknown ** ppUnk)
{
    Assert(ppUnk);

    if (*ppUnk)
    {
        (*ppUnk)->Release();
        *ppUnk = NULL;
    }
}


//+----------------------------------------------------------------------------
//  Function:   PublicQueryInterface
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CComponent::PublicQueryInterface(
    REFIID  riid,
    void ** ppvObj)
{
    return _pUnkOuter->QueryInterface(riid, ppvObj);
}


//+----------------------------------------------------------------------------
//  Function:   PublicAddRef
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CComponent::PublicAddRef()
{
    return _pUnkOuter->AddRef();
}


//+----------------------------------------------------------------------------
//  Function:   PublicRelease
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CComponent::PublicRelease()
{
    return _pUnkOuter->Release();
}


//+----------------------------------------------------------------------------
//  Function:   PrivateQueryInterface
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CComponent::PrivateQueryInterface(
    REFIID  riid,
    void ** ppvObj)
{
    if (riid == IID_IUnknown)
    {
        *ppvObj = (void *)(IUnknown *)&_Unk;
        return S_OK;
    }
    return E_NOINTERFACE;
}


//+----------------------------------------------------------------------------
//  Function:   CUnknown::QueryInterface
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CComponent::CUnknown::QueryInterface(
    REFIID  riid,
    void ** ppvObj)
{
    HRESULT hr;

    Assert(ppvObj);
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    hr = OWNING_CLASS(CComponent, _Unk)->PrivateQueryInterface(riid, ppvObj);

    if (!hr)
    {
        Assert(*ppvObj);
        ((IUnknown *)*ppvObj)->AddRef();
        hr = S_OK;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//  Function:   CUnknown::AddRef
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CComponent::CUnknown::AddRef()
{
    return ++(OWNING_CLASS(CComponent, _Unk)->_cRefs);
}


//+----------------------------------------------------------------------------
//  Function:   CUnknown::Release
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CComponent::CUnknown::Release()
{
    CComponent *    pComp = OWNING_CLASS(CComponent, _Unk);

    Assert(OWNING_CLASS(CComponent, _Unk)->_cRefs);

    if (!--pComp->_cRefs)
    {
        pComp->_cRefs += REF_GUARD;
        delete pComp;
        return 0;
    }
    return pComp->_cRefs;
}
