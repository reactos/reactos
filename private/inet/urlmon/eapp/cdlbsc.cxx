//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdlbsc.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    01-27-97   t-alans (Alan Shi)   Created
//
//----------------------------------------------------------------------------

#include <eapp.h>
#include "cdlbsc.hxx"

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::CCodeDLBSC
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCodeDLBSC::CCodeDLBSC(IOInetProtocolSink *pIOInetProtocolSink,
                       IOInetBindInfo *pIOInetBindInfo,
                       CCdlProtocol *pCDLProtocol,
                       BOOL fGetClassObject)
{
    _cRef = 1;
    _pIBinding = NULL;
    _pOInetProtocolSink = pIOInetProtocolSink;
    _fGetClassObject = fGetClassObject;

    if (_pOInetProtocolSink != NULL)
    {
        _pOInetProtocolSink->AddRef();
    }
    _pIOInetBindInfo = pIOInetBindInfo;
    if (_pIOInetBindInfo != NULL)
    {
        _pIOInetBindInfo->AddRef();
    }
    _pCDLProtocol = pCDLProtocol;
    if (_pCDLProtocol != NULL)
    {
        _pCDLProtocol->AddRef();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::~CCodeDLBSC
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCodeDLBSC::~CCodeDLBSC()
{
    if (_pOInetProtocolSink != NULL)
    {
        _pOInetProtocolSink->Release();
    }
    if (_pIOInetBindInfo != NULL)
    {
        _pIOInetBindInfo->Release();
    }
    if (_pCDLProtocol != NULL)
    {
        _pCDLProtocol->ClearCodeDLBSC();
        _pCDLProtocol->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::Abort
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

HRESULT CCodeDLBSC::Abort()
{
    if (_pIBinding)
    {
        return _pIBinding->Abort();
    }
    else
    {
        return S_OK;
    }
}

/*
 *
 * IUnknown Methods
 *
 */

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::QueryInterface
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT          hr = E_NOINTERFACE;

    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }
    if (*ppv != NULL)
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::AddRef
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCodeDLBSC::AddRef()
{
    return ++_cRef;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::Release
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCodeDLBSC::Release()
{
    if (--_cRef)
    {
        return _cRef;
    }
    delete this;

    return 0;
}

/*
 *
 * IBindStatusCallback Methods
 *
 */

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::OnStartBinding
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    if (_pIBinding != NULL)
    {
        _pIBinding->Release();
    }
    _pIBinding = pib;

    if (_pIBinding != NULL)
    {
        _pIBinding->AddRef();
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::OnStopBinding
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    HRESULT                   hr = S_OK;
    DWORD                     dwError = 0;
    DWORD                     grfBINDF = 0;
    BINDINFO                  bindinfo;

    _pCDLProtocol->SetDataPending(FALSE);

    if (SUCCEEDED(hresult))
    {
        if (_fGetClassObject)
        {
            EProtAssert(_pUnk);

            // put _pUnk into the bind context for CBinding to retrieve

            hresult = _pCDLProtocol->RegisterIUnknown(_pUnk);

            // no need for _pUnk anymore, release it

            _pUnk->Release();
            _pUnk = NULL;

            if (SUCCEEDED(hresult))
            {
                _pOInetProtocolSink->ReportProgress(BINDSTATUS_IUNKNOWNAVAILABLE,
                                                    NULL);
            }
        }
        else
        {
            if (!IsEqualGUID(_pCDLProtocol->GetClsid() , CLSID_NULL))
            {
                LPOLESTR pwzStrClsId;
                StringFromCLSID(_pCDLProtocol->GetClsid(), &pwzStrClsId);
                _pOInetProtocolSink->ReportProgress(BINDSTATUS_CLSIDCANINSTANTIATE, pwzStrClsId);
        
                delete [] pwzStrClsId;
            }
        }
    }

    hr = _pOInetProtocolSink->ReportResult(hresult, dwError, szError);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::OnObjectAvailable
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    EProtAssert(!_pUnk && punk);

    _pUnk = punk;
    _pUnk->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::GetPriority
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::GetPriority(LONG *pnPriority)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::OnLowResource
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::OnLowResource(DWORD dwReserved)
{
    return S_OK;
}  

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::OnProgress
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText)
{
    EProtAssert(_pOInetProtocolSink != NULL);
    return _pOInetProtocolSink->ReportProgress(ulStatusCode, szStatusText);
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::GetBindInfo
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
{
    EProtAssert(_pIOInetBindInfo != NULL);
    return _pIOInetBindInfo->GetBindInfo(pgrfBINDF, pbindInfo);
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::OnDataAvailable
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                         FORMATETC *pformatetc,
                                         STGMEDIUM *pstgmed)
{
    return S_OK;
}

/*
 *
 * IWindowForBindingUI Methods
 *
 */

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDLBSC::QueryService
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDLBSC::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT     hr = NOERROR;
    IServiceProvider        *pIServiceProvider = NULL;

    EProtAssert(ppvObj);
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = 0;

    hr = _pOInetProtocolSink->QueryInterface(IID_IServiceProvider,
                                             (LPVOID *)&pIServiceProvider);

    if (SUCCEEDED(hr))
    {
        hr = pIServiceProvider->QueryService(rsid, riid, (LPVOID *)ppvObj);
        pIServiceProvider->Release();
    }

    return hr;
}
