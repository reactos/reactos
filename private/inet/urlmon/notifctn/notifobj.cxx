#include <notiftn.h>

class CNotificationObj : public INotificationObj, public IPersistStream
{
public:

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // notification methods
    STDMETHODIMP SetValue(LPNAMEVALUE   pNameValue);
    STDMETHODIMP GetValue(LPNAMEVALUE   pNameValue);
    STDMETHODIMP GetEnumMAP(LPENUMMAP    pEnumMap);
    // persiststream methods
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);


public:
    CNotificationObj() : _CRefs()
    {
    }

    ~CNotificationObj()
    {
    }

private:
    CRefCount   _CRefs;         // the total refcount of this object
    BOOL        _fDirty;
};


//+---------------------------------------------------------------------------
//
//  Method:     CNotificationObj::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationObj::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_SESSION, "%p _IN CNotificationObj::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_INotificationObj) )
    {
        *ppvObj = this;
        AddRef();
    }
    else if (riid == IID_IPersistStream)
    {
        *ppvObj = (IPersistStream *)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    TransDebugOut((DEB_SESSION, "%p OUT CNotificationObj::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationObj::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotificationObj::AddRef(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN CNotificationObj::AddRef\n", this));

    LONG lRet = ++_CRefs;

    TransDebugOut((DEB_SESSION, "%p OUT CNotificationObj::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationObj::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotificationObj::Release(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN CNotificationObj::Release\n", this));
    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    TransDebugOut((DEB_SESSION, "%p OUT CNotificationObj::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

STDMETHODIMP CNotificationObj::SetValue(LPNAMEVALUE   pNameValue)
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::SetValue\n", this));
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::SetValue (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotificationObj::GetValue(LPNAMEVALUE   pNameValue)
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::GetValue\n", this));
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::GetValue (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotificationObj::GetEnumMAP(LPENUMMAP    pEnumMap)
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::GetEnumMAP\n", this));
    HRESULT hr = NOERROR;


    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::GetEnumMAP (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotificationObj::IsDirty()
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::IsDirty\n", this));
    HRESULT hr = NOERROR;

    hr =  (_fDirty) ? S_OK : S_FALSE;

    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::IsDirty (hr:%lx)\n",this, hr));
    return hr;
}


STDMETHODIMP CNotificationObj::Load(IStream *pStm)
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::Load\n", this));
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::Load (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotificationObj::Save(IStream *pStm, BOOL fClearDirty)
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::Save\n", this));
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::Save (hr:%lx)\n",this, hr));
    return hr;
}


STDMETHODIMP CNotificationObj::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    TransDebugOut((DEB_PROT, "%p _IN CNotificationObj::GetSizeMax\n", this));
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_PROT, "%p OUT CNotificationObj::GetSizeMax (hr:%lx)\n",this, hr));
    return hr;
}



