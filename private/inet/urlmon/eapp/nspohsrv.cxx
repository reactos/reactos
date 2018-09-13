//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       resprot.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>


//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pTrans] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pProt, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD dwReserved)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::Start\n", this));
    HRESULT hr = NOERROR;
    WCHAR    wzURL[MAX_URL_SIZE];

    EProtAssert((!_pProtSink && pOIBindInfo && pProt));
    EProtAssert((_pwzUrl == NULL));

    hr = CBaseProtocol::Start(pwzUrl,pProt, pOIBindInfo, grfSTI, dwReserved);

    if ( (grfSTI & PI_PARSE_URL) )
    {
        hr =  ParseAndStart(FALSE);
    }
    else if (hr == NOERROR)
    {
        // asked to go async as soon as possible
        // use the switch mechanism which will \
        // call back later on ::Continue
        if (grfSTI & PI_FORCE_ASYNC)
        {
            hr = E_PENDING;
            PROTOCOLDATA protdata;
            protdata.grfFlags = PI_FORCE_ASYNC;
            protdata.dwState = RES_STATE_BIND;
            protdata.pData = 0;
            protdata.cbData = 0;

            _pProtSink->Switch(&protdata);
        }
        else
        {
            hr =  ParseAndStart(TRUE);
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::Start (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::Continue(PROTOCOLDATA *pStateInfoIn)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::Continue\n", this));
    HRESULT hr = E_FAIL;

    EProtAssert((!pStateInfoIn->pData && pStateInfoIn->cbData && (pStateInfoIn->dwState == RES_STATE_BIND)));

    if (pStateInfoIn->dwState == RES_STATE_BIND)
    {
        hr =  ParseAndStart();
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::Continue (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::Read
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ULONG] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::Read (cb:%ld)\n", this,cb));
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->Read(pv, cb, pcbRead);
    }
    else
    {
        hr = S_FALSE;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::Read (pcbRead:%ld, hr:%lx)\n",this,*pcbRead, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:      WORK: not done
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::Seek\n", this));
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);
    }
    else
    {
        hr = S_FALSE;
    }


    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::Seek (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::LockRequest(DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::LockRequest\n", this));

    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->LockRequest(dwOptions);
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::LockRequest (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::UnlockRequest()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::UnlockRequest\n", this));
    HRESULT hr = NOERROR;

    if (_pProt)
    {
        hr = _pProt->UnlockRequest();
    }


    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::UnlockRequest (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::COhServNameSp
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
COhServNameSp::COhServNameSp(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner) : CBaseProtocol(rclsid, pUnkOuter, ppUnkInner)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN/OUT     COhServNameSp::COhServNameSp \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::~COhServNameSp
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
COhServNameSp::~COhServNameSp()
{

    EProtDebugOut((DEB_PLUGPROT, "%p _IN/OUT COhServNameSp::~COhServNameSp \n", this));
}


//+---------------------------------------------------------------------------
//
//  Method:     COhServNameSp::ParseAndStart
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COhServNameSp::ParseAndStart(BOOL fBind)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN COhServNameSp::ParseAndStart\n", this));

    HRESULT hr = INET_E_USE_DEFAULT_PROTOCOLHANDLER;

    WCHAR wzUrl[MAX_URL_SIZE];

    LPWSTR pwzOhservHttp = L"http://ohserv";
    LPWSTR pwzOhservFile = L"file://\\\\ohserv\\http";
    LPWSTR pwzOhservRoot = L"\\\\ohserv\\http";
    ULONG cServerLen = wcslen(pwzOhservHttp);

    do
    {
        if ( wcsnicmp(_wzFullURL, pwzOhservHttp, cServerLen) )
        {
            // not http://ohserv - return default error
            break;
        }

        // find the file name and path
        LPWSTR pwzRest = _wzFullURL + cServerLen;

        EProtAssert((pwzRest));

        wcscpy(wzUrl, pwzOhservRoot);
        wcscat(wzUrl, pwzRest);

        DWORD dwAttr = 0;
        {
            char szUrl[MAX_URL_SIZE];
            W2A(wzUrl, szUrl, MAX_URL_SIZE);

            dwAttr = GetFileAttributes(szUrl);
        }

        if (   (dwAttr == 0xffffffff)
            || (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
           )
        {
            break;
        }


        wcscpy(wzUrl, pwzOhservFile);
        wcscat(wzUrl, pwzRest);

        // create an APP file
        {
            IClassFactory *pCF = 0;
            hr = CoCreateInstance(CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER,IID_IClassFactory, (void**)&pCF);
            if (hr == NOERROR)
            {
                IUnknown *pUnk = 0;
                //IOInetProtocol *pProt = 0;


                EProtAssert((pCF));

                hr = pCF->CreateInstance((IOInetProtocol *)this, IID_IUnknown, (void **)&_pUnkInner);

                if (hr == NOERROR)
                {
                    EProtAssert((_pUnkInner));
                    hr = (_pUnkInner)->QueryInterface(IID_IOInetProtocol, (void **) &_pProt);
                }

                // create an instance without aggregation
                if (hr == CLASS_E_NOAGGREGATION)
                {
                    hr = pCF->CreateInstance(NULL, IID_IOInetProtocol, (void **) &_pProt);
                }

                pCF->Release();

                {
                    hr = _pProt->Start(wzUrl, _pProtSink, _pOIBindInfo, _grfSTI, 0);
                }

                if (hr != NOERROR)
                {
                    hr = INET_E_USE_DEFAULT_PROTOCOLHANDLER;
                }

           }
        }

        break;

    }  while (1);


    if (hr == MK_E_SYNTAX)
    {
        _pProtSink->ReportResult(hr, 0, 0);
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT COhServNameSp::ParseAndStart (hr:%lx)\n", this,hr));
    return hr;
}


