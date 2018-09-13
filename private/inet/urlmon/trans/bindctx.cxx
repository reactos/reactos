//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       bindctx.cxx
//
//  Contents:   CBindCtx methods implementations
//              to support custom marshaling
//
//  Classes:
//
//  Functions:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#include "bindctx.hxx"

PerfDbgTag(tagCBindCtx, "Urlmon", "Log CBindCtx", DEB_URLMON);

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::Create
//
//  Synopsis:
//
//  Arguments:  [ppCBCtx] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBindCtx::Create(CBindCtx **ppCBCtx, IBindCtx *pbc)
{
    PerfDbgLog1(tagCBindCtx, NULL, "+CBindCtx::Create (pbc:%lx)", pbc);
    HRESULT hr = NOERROR;

    *ppCBCtx = NULL;

    if (pbc == NULL)
    {
        hr = CreateBindCtx(0, &pbc);
    }
    else
    {
        // check if this is actually  a wrapped object
        // if so don't wrap it again
        hr = pbc->QueryInterface(IID_IAsyncBindCtx, (void **)ppCBCtx);
        if (hr != NOERROR)
        {
            hr = NOERROR;
            *ppCBCtx = NULL;
        }
        pbc->AddRef();
    }

    if (hr == NOERROR && *ppCBCtx == NULL)
    {
        TransAssert((pbc));
        *ppCBCtx = new CBindCtx(pbc);

        if (*ppCBCtx  == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (pbc)
    {
        pbc->Release();
    }
    PerfDbgLog2(tagCBindCtx, NULL, "-CBindCtx::Create (out:%lx,hr:%lx)", *ppCBCtx, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::CBindCtx
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBindCtx::CBindCtx(IBindCtx *pbc)
{
    _pbcLocal = pbc;
    if (_pbcLocal)
    {
        _pbcLocal->AddRef();
    }
    _pbcRem = NULL;
    _dwThreadId = GetCurrentThreadId();
    _pCTrans = 0;
    _pCTransData = 0;
    
    DllAddRef();
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::~CBindCtx
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CBindCtx::~CBindCtx()
{
    //TransAssert(( _dwThreadId == GetCurrentThreadId() ));
    if (_pbcRem)
    {
        _pbcRem->Release();
    }
    if (_pbcLocal)
    {
        _pbcLocal->Release();
    }

    if (_pCTrans)
    {
        _pCTrans->Release();
    }
    if (_pCTransData)
    {
        _pCTransData->Release();
    }


    DllRelease();
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::CanMarshalIID
//
//  Synopsis:   Checks whether this object supports marshalling this IID.
//
//  Arguments:  [riid] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL CBindCtx::CanMarshalIID(REFIID riid)
{
    // keep this in sync with the QueryInterface
    return (BOOL) (riid == IID_IBindCtx);
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::ValidateMarshalParams
//
//  Synopsis:   Validates the standard set parameters that are passed into most
//              of the IMarshal methods
//
//  Arguments:  [riid] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBindCtx::ValidateMarshalParams(REFIID riid,void *pvInterface,
                    DWORD dwDestContext,void *pvDestContext,DWORD mshlflags)
{

    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::ValidateMarshalParams");
    TransAssert(( _dwThreadId == GetCurrentThreadId() ));
    HRESULT hr = NOERROR;

    if (CanMarshalIID(riid))
    {
        UrlMkAssert((dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_LOCAL || dwDestContext == MSHCTX_NOSHAREDMEM));
        UrlMkAssert((mshlflags == MSHLFLAGS_NORMAL || mshlflags == MSHLFLAGS_TABLESTRONG));

        if (   (dwDestContext != MSHCTX_INPROC && dwDestContext != MSHCTX_LOCAL && dwDestContext != MSHCTX_NOSHAREDMEM)
            || (mshlflags != MSHLFLAGS_NORMAL && mshlflags != MSHLFLAGS_TABLESTRONG))
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::ValidateMarshalParams (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::QueryInterface");

    TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    if (   riid == IID_IUnknown
        || riid == IID_IBindCtx
        || riid == IID_IAsyncBindCtx)
    {
        *ppvObj = this;
    }
    else if (riid == IID_IMarshal)
    {
        *ppvObj = (void*) (IMarshal *) this;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    if (hr == NOERROR)
    {
        AddRef();
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBindCtx::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBindCtx::AddRef(void)
{
    //TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    LONG lRet = ++_CRefs;

    PerfDbgLog1(tagCBindCtx, this, "CBindCtx::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBindCtx::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CBindCtx::Release(void)
{
    //TransAssert(( _dwThreadId == GetCurrentThreadId() ));
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::Release");

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::GetObjectParam
//
//  Synopsis:
//
//  Arguments:  [pszKey] --
//              [ppunk] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::GetObjectParam(LPOLESTR pszKey, IUnknown **ppunk)
{
    HRESULT hr = NOERROR;
    PerfDbgLog3(tagCBindCtx, this, "+CBindCtx::GetObjectParam (_pbcLocal:%lx, _pbcRem:%lx, szParam:%ws)", _pbcLocal, _pbcRem, pszKey);
    UrlMkAssert((pszKey && ppunk));

    if (    _pbcRem
        &&  !wcscmp(pszKey, SZ_TRANSACTIONDATA))
    {
        // get the interface from the remote object
        PerfDbgLog2(tagCBindCtx, this, "=== CBindCtx::GetObjectParam (_pbcRem:%lx, szParam:%ws)", _pbcRem, pszKey);

        hr =  _pbcLocal->GetObjectParam(pszKey, ppunk);
        
        if (hr != NOERROR)
        {
            hr =  _pbcRem->GetObjectParam(pszKey, ppunk);
        }
    }
    else
    {
        hr =  _pbcLocal->GetObjectParam(pszKey, ppunk);
    }

    if (   (hr != NOERROR)
        && (_pbcRem)
        && (   wcscmp(pszKey, SZ_BINDING)
            && wcscmp(pszKey, REG_BSCB_HOLDER)
            && wcscmp(pszKey, REG_ENUMFORMATETC)
            && wcscmp(pszKey, REG_MEDIA_HOLDER)
            && wcscmp(pszKey, SZ_TRANSACTIONDATA)) )
    {
        hr =  _pbcRem->GetObjectParam(pszKey, ppunk);
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::GetObjectParam (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::EnumObjectParam
//
//  Synopsis:
//
//  Arguments:  [ppenum] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:      BUGBUG - this implementation is wrong; need to wrap
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::EnumObjectParam(IEnumString **ppenum)
{
    return _pbcLocal->EnumObjectParam(ppenum);
}

STDMETHODIMP CBindCtx::RevokeObjectParam(LPOLESTR pszKey)
{
    return _pbcLocal->RevokeObjectParam(pszKey);
}


//+---------------------------------------------------------------------------
//
// IMarshal methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::GetUnmarshalClass
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//              [pCid] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::GetUnmarshalClass(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::GetUnmarshalClass");
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
    if (hr == NOERROR)
    {
        *pCid = (CLSID) CLSID_UrlMkBindCtx;
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::GetUnmarshalClass (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::GetMarshalSizeMax
//
//  Synopsis:
//
//  Arguments:  [void] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//              [pSize] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::GetMarshalSizeMax(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::GetMarshalSizeMax");
    HRESULT hr;

    if (pSize == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
        if (hr == NOERROR)
        {
            hr = CoGetMarshalSizeMax(pSize, IID_IBindCtx, _pbcLocal, dwDestContext,pvDestContext,mshlflags);
            // marshal also the transaction object
            *pSize += sizeof(_pCTrans) + sizeof(DWORD);
        }
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::GetMarshalSizeMax (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::MarshalInterface
//
//  Synopsis:
//
//  Arguments:  [REFIID] --
//              [riid] --
//              [DWORD] --
//              [void] --
//              [DWORD] --
//              [mshlflags] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::MarshalInterface(IStream *pistm,REFIID riid,
                                void *pvInterface,DWORD dwDestContext,
                                void *pvDestContext,DWORD mshlflags)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::MarshalInterface");
    HRESULT hr;
    BOOL fTransfered = FALSE;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
    if (hr == NOERROR)
    {
        hr = CoMarshalInterface(pistm, IID_IBindCtx, _pbcLocal, dwDestContext, pvDestContext, mshlflags);
    }

    if (   (hr == NOERROR)
        && (dwDestContext == MSHCTX_INPROC)
        && (_pCTrans != NULL) )
    {
        TransAssert((_pCTrans));
        TransAssert((_pCTransData));
        
        hr = _pCTrans->PrepareThreadTransfer();

        if (hr == NOERROR)
        {
            DWORD dwProcessId = GetCurrentProcessId();
            
            // marshal also the transaction object
            hr = pistm->Write(&_pCTrans, sizeof(_pCTrans), NULL);
            TransAssert((hr == NOERROR));
            // addref the pointer here to keep the object alive!
            // 
            _pCTrans->AddRef();

            // marshal also the transdata object
            hr = pistm->Write(&_pCTransData, sizeof(_pCTransData), NULL);
            TransAssert((hr == NOERROR));

            if (_pCTransData)
            {
                _pCTransData->PrepareThreadTransfer();

                // addref the pointer here to keep the object alive!
                // 
                _pCTransData->AddRef();
            }
            
            hr = pistm->Write(&dwProcessId, sizeof(DWORD), NULL);
            TransAssert((hr == NOERROR));
            fTransfered = TRUE;
        }
    }

    if (!fTransfered)
    {
        DWORD dwProcessId = 0;
        // marshal also the transaction object
        hr = pistm->Write(&dwProcessId, sizeof(_pCTrans), NULL);
        TransAssert((hr == NOERROR));
        hr = pistm->Write(&dwProcessId, sizeof(DWORD), NULL);
        TransAssert((hr == NOERROR));
        hr = pistm->Write(&dwProcessId, sizeof(DWORD), NULL);
        TransAssert((hr == NOERROR));
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::MarshalInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::UnmarshalInterface
//
//  Synopsis:   Unmarshals an Urlmon interface out of a stream
//
//  Arguments:  [REFIID] --
//              [void] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    9-12-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::UnmarshalInterface(IStream *pistm,REFIID riid,void ** ppvObj)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::UnmarshalInterface");
    HRESULT hr = NOERROR;

    TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    if (ppvObj == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (! CanMarshalIID(riid))
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    else
    {
        *ppvObj = NULL;

        hr = CoUnmarshalInterface(pistm, IID_IBindCtx, (void **) &_pbcRem);

        // call QI to get the requested interface
        if (hr == NOERROR)
        {
            hr = QueryInterface(riid, ppvObj);

            {
                HRESULT hr1;
                DWORD dwProcessId;
                // marshal also the transaction object
                hr1 = pistm->Read(&_pCTrans, sizeof(_pCTrans), NULL);
                // Note: pTrans was addref'd as the object was marshaled            // it is now addref'd
                // keep it since we hold on to the object

                if (FAILED(hr1))
                {
                    _pCTrans = 0;
                }

                // marshal also the transdata object
                hr1 = pistm->Read(&_pCTransData, sizeof(_pCTransData), NULL);
                // Note: pTrans was addref'd as the object was marshaled            // it is now addref'd
                // keep it since we hold on to the object
                if (FAILED(hr1))
                {
                    _pCTransData = 0;
                }

                //TransAssert((hr1 == NOERROR));
                hr1 = pistm->Read(&dwProcessId, sizeof(DWORD), NULL);
                TransAssert((hr1 == NOERROR));
                if (FAILED(hr1))
                {
                    dwProcessId = 0;
                }

            }

        }
        else
        {
            TransAssert(( _pbcRem == 0));
        }

    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::UnmarshalInterface (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP CBindCtx::ReleaseMarshalData(IStream *pStm)
{
    PerfDbgLog(tagCBindCtx, this, "CBindCtx::ReleaseMarshalData");
    return NOERROR;
}

STDMETHODIMP CBindCtx::DisconnectObject(DWORD dwReserved)
{
    PerfDbgLog(tagCBindCtx, this, "CBindCtx::DisconnectObject");
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::SetTransactionObject
//
//  Synopsis:
//
//  Arguments:  [pCTrans] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::SetTransactionObject(CTransaction *pCTrans)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::SetTransactionObject");
    //TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    if (pCTrans != _pCTrans)
    {
        if (_pCTrans)
        {
            _pCTrans->Release();
        }
        _pCTrans = pCTrans;
        if (_pCTrans)
        {
            _pCTrans->AddRef();
        }
    }

    PerfDbgLog(tagCBindCtx, this, "-CBindCtx::SetTransactionObject (hr:0)");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::GetTransactionObject
//
//  Synopsis:
//
//  Arguments:  [ppCTrans] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::GetTransactionObject(CTransaction **ppCTrans)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::GetTransactionObject");
    //TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    HRESULT hr = NOERROR;
    if (_pCTrans)
    {

        *ppCTrans = _pCTrans;
        _pCTrans->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppCTrans = NULL;
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::GetTransactionObject (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::SetTransactionObjects
//
//  Synopsis:
//
//  Arguments:  [pCTrans] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::SetTransactionObjects(CTransaction *pCTrans,CTransData *pCTransData)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::SetTransactionObjects");
    //TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    if (pCTrans != _pCTrans)
    {
        if (_pCTrans)
        {
            _pCTrans->Release();
        }
        _pCTrans = pCTrans;
        if (_pCTrans)
        {
            _pCTrans->AddRef();
        }
    }

    if (pCTransData != _pCTransData)
    {
        if (_pCTransData)
        {
            _pCTransData->Release();
        }
        _pCTransData = pCTransData;
        if (_pCTransData)
        {
            _pCTransData->AddRef();
        }
    }

    PerfDbgLog(tagCBindCtx, this, "-CBindCtx::SetTransactionObjects (hr:0)");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::GetTransactionObject
//
//  Synopsis:
//
//  Arguments:  [ppCTrans] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::GetTransactionObjects(CTransaction **ppCTrans,CTransData **ppCTransData)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::GetTransactionObjects");
    TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    HRESULT hr = NOERROR;
    if (_pCTrans)
    {

        *ppCTrans = _pCTrans;
        _pCTrans->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppCTrans = NULL;
    }

    if (_pCTransData && ppCTransData)
    {

        *ppCTransData = _pCTransData;
        _pCTransData->AddRef();
    }
    else
    {   
        if (!_pCTrans)
        {
            hr = E_NOINTERFACE;
        }
        if (ppCTransData)
        {
            *ppCTransData = NULL;
        }
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::GetTransactionObjects (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::SetTransData
//
//  Synopsis:
//
//  Arguments:  [pCTransData] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::SetTransData(CTransData *pCTransData)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::SetTransData");
    TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    if (pCTransData != _pCTransData)
    {
        if (_pCTransData)
        {
            _pCTransData->Release();
        }
        _pCTransData = pCTransData;
        if (_pCTransData)
        {
            _pCTransData->AddRef();
        }
    }

    PerfDbgLog(tagCBindCtx, this, "-CBindCtx::SetTransData (hr:0)");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindCtx::GetTransData
//
//  Synopsis:
//
//  Arguments:  [ppCTransData] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CBindCtx::GetTransData(CTransData **ppCTransData)
{
    PerfDbgLog(tagCBindCtx, this, "+CBindCtx::GetTransData");
    TransAssert(( _dwThreadId == GetCurrentThreadId() ));

    HRESULT hr = NOERROR;
    if (_pCTransData)
    {

        *ppCTransData = _pCTransData;
        _pCTransData->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppCTransData = NULL;
    }

    PerfDbgLog1(tagCBindCtx, this, "-CBindCtx::GetTransData (hr:%lx)", hr);
    return hr;
}

