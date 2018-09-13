//+---------------------------------------------------------------------
//
//   File:       vwadvhld.cxx
//
//   Contents:   Miscellaneous OLE helper routines
//
//------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::ViewAdviseHolder, protected
//
//  Synopsis:   Constructor for ViewAdviseHolder class
//
//----------------------------------------------------------------

ViewAdviseHolder::ViewAdviseHolder()
{
    _refs = 1;
    _pAdvSink = NULL;
    _dwAdviseAspects = 0;
    _dwAdviseFlags = 0;

}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::~ViewAdviseHolder, protected
//
//  Synopsis:   Destructor for ViewAdviseHolder class
//
//----------------------------------------------------------------

ViewAdviseHolder::~ViewAdviseHolder()
{
    if (_pAdvSink != NULL)
        _pAdvSink->Release();
}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
ViewAdviseHolder::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
#if DBG
    DOUT(L"ViewAdviseHolder::QueryInterface E_NOINTERFACE\r\n");
#endif
    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder:AddRef, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
ViewAdviseHolder::AddRef(void)
{
    return _refs++;
}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::Release, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
ViewAdviseHolder::Release(void)
{
    if (--_refs == 0)
        delete this;
    return (_refs);
}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::SetAdvise, public
//
//  Synopsis:   Places an advise in the advise holder
//
//  Arguments:  The arguments are the same as to IViewObject::SetAdvise
//
//  Notes:      IViewObject objects using this advise holder delegate
//              their IViewObject::SetAdvise methods to this method.
//
//----------------------------------------------------------------

STDMETHODIMP
ViewAdviseHolder::SetAdvise(DWORD aspects, DWORD advf, LPADVISESINK pAdvSink)
{
    if (_pAdvSink != NULL)
        _pAdvSink->Release();

    _pAdvSink = pAdvSink;

    if (_pAdvSink != NULL)
        _pAdvSink->AddRef();

    _dwAdviseAspects = aspects;
    _dwAdviseFlags = advf;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::GetAdvise, public
//
//  Synopsis:   Reports on an advise in the advise holder
//
//  Arguments:  The arguments are the same as to IViewObject::GetAdvise
//
//  Notes:      IViewObject objects using this advise holder delegate
//              their IViewObject::GetAdvise methods to this method.
//
//----------------------------------------------------------------

STDMETHODIMP
ViewAdviseHolder::GetAdvise(DWORD FAR* pAspects,
        DWORD FAR* pAdvf,
        LPADVISESINK FAR* ppAdvSink)
{
    *ppAdvSink = _pAdvSink;
    *pAspects = _dwAdviseAspects;
    *pAdvf = _dwAdviseFlags;
    return NOERROR;
    // review: is this the right return code if no advise has been set?
}

//+---------------------------------------------------------------
//
//  Member:     ViewAdviseHolder::SendOnViewChange, public
//
//  Synopsis:   Sends an OnViewChange notification to the registered advise.
//
//  Arguments:  [dwAspect] -- the display aspect that has changed
//
//  Notes:      The client IViewObject should call this method whenever
//              its view has changed.
//
//----------------------------------------------------------------

void ViewAdviseHolder::SendOnViewChange(DWORD dwAspect)
{
    if (_pAdvSink != NULL && (dwAspect&_dwAdviseAspects) != 0)
        _pAdvSink->OnViewChange(dwAspect, -1);
}

//+---------------------------------------------------------------
//
//  Function:   CreateViewAdviseHolder
//
//  Synopsis:   Creates a ViewAdviseHolder object
//
//  Arguments:  [ppViewHolder] -- place where the view holder is returned
//
//  Returns:    Success if the advise holder was created
//
//  Notes:      This function is directly analogous to the OLE provided
//              CreateDataAdviseHolder
//
//----------------------------------------------------------------

HRESULT
CreateViewAdviseHolder(LPVIEWADVISEHOLDER FAR* ppViewHolder)
{
    //LPVIEWADVISEHOLDER pViewAdvHolder = new (NullOnFail) ViewAdviseHolder;
    LPVIEWADVISEHOLDER pViewAdvHolder = new ViewAdviseHolder;
    *ppViewHolder = pViewAdvHolder;

    if (pViewAdvHolder == NULL)
    {
        DOUT(L"o2base/vwadvhld/CreateViewAdviseHolder failed\r\n");
        return E_OUTOFMEMORY;
    }
    else
        return NOERROR;
}
