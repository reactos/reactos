//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       MPXBSC.CXX
//
//  Contents:   Code to handle multiplexing multiple concurrent
//              IBindStatusCallback interfaces.
//
//  Classes:    CBSCHolder
//
//  Functions:
//
//  History:    01-04-96    JoeS (Joe Souza)    Created
//              01-15-96    JohannP (Johann Posch)  Modified to new IBSC
//
//----------------------------------------------------------------------------
#include <mon.h>
#include "mpxbsc.hxx"

PerfDbgTag(tagCBSCHolder, "Urlmon", "Log CBSCHolder", DEB_BINDING);

HRESULT GetObjectParam(IBindCtx *pbc, LPOLESTR pszKey, REFIID riid, IUnknown **ppUnk);

//+---------------------------------------------------------------------------
//
//  Function:   UrlMonInvokeExceptionFilterMSN
//
//  Synopsis:
//
//  Arguments:  [lCode] --
//              [lpep] --
//
//  Returns:
//
//  History:    8-25-97   DanpoZ(Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG UrlMonInvokeExceptionFilterMSN( DWORD lCode, LPEXCEPTION_POINTERS lpep )
{
#if DBG == 1
    DbgLog2(tagCBSCHolder, NULL, "Exception 0x%x at address 0x%x",
               lCode, lpep->ExceptionRecord->ExceptionAddress);
    DebugBreak();
#endif
    LONG exr = EXCEPTION_CONTINUE_EXECUTION;

    if( lCode == STATUS_ACCESS_VIOLATION )
    {
        char achProgname[256];
        achProgname[0] = 0;
        GetModuleFileNameA(NULL,achProgname,sizeof(achProgname));

        if( StrStrI(achProgname, "msnviewr.exe") )
        {
            exr = EXCEPTION_EXECUTE_HANDLER;
        }
    }

    return exr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetBSCHolder
//
//  Synopsis:   Returns a holder for IBindStatusCallback
//
//  Arguments:  [pBC] --
//              [ppCBSHolder] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetBSCHolder(LPBC pBC, CBSCHolder **ppCBSHolder)
{
    PerfDbgLog(tagCBSCHolder, NULL, "+GetBSCHolder");
    UrlMkAssert((ppCBSHolder != NULL));

    HRESULT hr;
    CBSCHolder *pCBSCBHolder = NULL;

    hr = GetObjectParam(pBC, REG_BSCB_HOLDER, IID_IBindStatusCallbackHolder, (IUnknown **)&pCBSCBHolder);
    if (pCBSCBHolder == NULL)
    {
        pCBSCBHolder = new CBSCHolder;
        if (!pCBSCBHolder)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pBC->RegisterObjectParam(REG_BSCB_HOLDER, (IBindStatusCallback *) pCBSCBHolder);
            *ppCBSHolder = pCBSCBHolder;
        }
    }
    else
    {
        *ppCBSHolder = pCBSCBHolder;
    }

    PerfDbgLog1(tagCBSCHolder, NULL, "-GetBSCHolder (hr:%lx)", hr);
    return hr;
}

CBSCHolder::CBSCHolder(void) : _CRefs(), _cElements(0)
{
    _pCBSCNode = NULL;
    _fBindStarted = FALSE;
    _fBindStopped = FALSE;
    _fHttpNegotiate = FALSE;
    _fAuthenticate  = FALSE;
}

CBSCHolder::~CBSCHolder(void)
{
    CBSCNode *pNode, *pNextNode;

    pNode = _pCBSCNode;

    while (pNode)
    {
        pNextNode = pNode->GetNextNode();
        delete pNode;
        pNode = pNextNode;
    }
}

STDMETHODIMP CBSCHolder::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::QueryInterface");

    if (   (riid == IID_IUnknown)
        || (riid == IID_IBindStatusCallback)
        || (riid == IID_IBindStatusCallbackHolder) )
    {
        // the holder is not marshalled!!
        *ppvObj = (void*)(IBindStatusCallback *) this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppvObj = (void*)(IServiceProvider *) this;
    }
    else if (riid == IID_IHttpNegotiate)
    {
        *ppvObj = (void*)(IHttpNegotiate *) this;
    }
    else if (riid == IID_IAuthenticate)
    {
        *ppvObj = (void*)(IAuthenticate *) this;
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

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::QueryInterface (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP_(ULONG) CBSCHolder::AddRef(void)
{
    LONG lRet = ++_CRefs;
    PerfDbgLog1(tagCBSCHolder, this, "CBSCHolder::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

STDMETHODIMP_(ULONG) CBSCHolder::Release(void)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::Release");
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::Release (cRefs:%ld)",lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::GetBindInfo
//
//  Synopsis:
//
//  Arguments:  [grfBINDINFOF] --
//              [pbindinfo] --
//
//  Returns:
//
//  History:
//
//  Notes:      Only the first BSC which also receives data gets called
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::GetBindInfo(DWORD *grfBINDINFOF,BINDINFO * pbindinfo)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::GetBindInfo");
    HRESULT     hr = E_FAIL;
    CBSCNode   *pNode;

    pNode = _pCBSCNode;

    if (pNode && (pNode->GetFlags() & BSCO_GETBINDINFO))
    {
        UrlMkAssert(( pbindinfo && (pbindinfo->cbSize == sizeof(BINDINFO)) ));

        // We only call the first link for GetBindInfo.
        hr = pNode->GetBSCB()->GetBindInfo(grfBINDINFOF, pbindinfo);
    }
    else
    {
        UrlMkAssert((FALSE && "Not IBSC node called on GetBindInfo"));
    }

    PerfDbgLog2(tagCBSCHolder, this, "-CBSCHolder::CallGetBindInfo (grfBINDINFOF:%lx, hr:%lx)", grfBINDINFOF,  hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnStartBinding
//
//  Synopsis:
//
//  Arguments:  [grfBINDINFOF] --
//              [pib] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnStartBinding(DWORD grfBINDINFOF, IBinding * pib)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::OnStartBinding");
    VDATETHIS(this);

    HRESULT     hr = E_FAIL;
    CBSCNode   *pNode;
    BOOL        fFirstNode = TRUE;

    _fBindStarted = TRUE;

    pNode = _pCBSCNode;

    while (pNode)
    {
        grfBINDINFOF = pNode->GetFlags();

        if (fFirstNode)
        {
            grfBINDINFOF |= (BSCO_ONDATAAVAILABLE | BSCO_ONOBJECTAVAILABLE);
        }
        else
        {
            grfBINDINFOF &= ~(BSCO_ONDATAAVAILABLE | BSCO_ONOBJECTAVAILABLE);
        }

        DbgLog1(tagCBSCHolder, this, "CBSCHolder::OnStartBinding on (IBSC:%lx)", pNode->GetBSCB());

        hr = pNode->GetBSCB()->OnStartBinding(grfBINDINFOF, pib);

        pNode = pNode->GetNextNode();
        fFirstNode = FALSE;
    }

    // BUGBUG: hr is set to return code only from last node we called.
    // Is this what we want?
    // What if one of the earlier nodes failed?

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::OnStartBinding (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnProgress
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ulProgressMax] --
//              [ulStatusCode] --
//              [szStatusText] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnProgress(ULONG ulProgress,ULONG ulProgressMax,
                               ULONG ulStatusCode, LPCWSTR szStatusText)
{
    PerfDbgLog4(tagCBSCHolder, this, "+CBSCHolder::OnProgress (StatusCode:%ld, StatusText:%ws, Progress:%ld, ProgressMax:%ld)",
        ulStatusCode, szStatusText?szStatusText:L"", ulProgress, ulProgressMax);
    VDATETHIS(this);
    HRESULT     hr = NOERROR;
    CBSCNode   *pNode;

    pNode = _pCBSCNode;

    while (pNode)
    {
        if (pNode->GetFlags() & BSCO_ONPROGRESS)
        {
            hr = pNode->GetBSCB()->OnProgress(ulProgress, ulProgressMax, ulStatusCode,szStatusText);
        }

        pNode = pNode->GetNextNode();
    }

    // BUGBUG: hr is set to return code only from last node we called.
    // Is this what we want?
    // What if one of the earlier nodes failed?

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::OnProgress (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnDataAvailable
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [FORMATETC] --
//              [pformatetc] --
//              [pstgmed] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnDataAvailable(DWORD grfBSC,DWORD dwSize,FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::OnDataAvailable");
    VDATETHIS(this);
    HRESULT     hr = E_FAIL;
    CBSCNode   *pNode;

    pNode = _pCBSCNode;

    if (pNode && (pNode->GetFlags() & BSCO_ONDATAAVAILABLE))
    {
        pNode->GetBSCB()->OnDataAvailable(grfBSC, dwSize, pformatetc, pstgmed);
        hr = NOERROR;
    }

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::OnDataAvailable (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnObjectAvailable
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [punk] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::OnObjectAvailable");
    VDATETHIS(this);
    CBSCNode   *pNode;

    pNode = _pCBSCNode;

    if (pNode && (pNode->GetFlags() & BSCO_ONOBJECTAVAILABLE))
    {
        pNode->GetBSCB()->OnObjectAvailable(riid, punk);
    }

    PerfDbgLog(tagCBSCHolder, this, "-CBSCHolder::OnObjectAvailable (hr:0)");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnLowResource
//
//  Synopsis:
//
//  Arguments:  [reserved] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnLowResource(DWORD reserved)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::OnLowResource");
    VDATETHIS(this);
    HRESULT     hr = E_FAIL;
    CBSCNode   *pNode;

    pNode = _pCBSCNode;

    while (pNode)
    {
        if (pNode->GetFlags() & BSCO_ONLOWRESOURCE)
        {
            hr = pNode->GetBSCB()->OnLowResource(reserved);
        }

        pNode = pNode->GetNextNode();
    }

    // BUGBUG: hr is set to return code only from last node we called.
    // Is this what we want?
    // What if one of the earlier nodes failed?

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::OnLowResource (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnStopBinding
//
//  Synopsis:
//
//  Arguments:  [LPCWSTR] --
//              [szError] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnStopBinding(HRESULT hrRes,LPCWSTR szError)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::OnStopBinding");
    HRESULT     hr = E_FAIL;
    CBSCNode   *pNode;
    CBSCNode   *pNodeNext;
    DWORD      dwFault;

    VDATETHIS(this);

    _fBindStopped = TRUE;   // Allow consumer to remove node on OnStopBinding.

    pNode = _pCBSCNode;

        while (pNode)
        {
            // save the next node since this node
            // we using now might get deleted
            // by RevokeBindStatusCallback
            pNodeNext = pNode->GetNextNode();

            pNode->SetLocalFlags(NODE_FLAG_REMOVEOK);

            PerfDbgLog2(tagCBSCHolder, this, "+CBSCHolder::OnStopBinding calling (Node:%lx, IBSC:%lx)",
            pNode,pNode->GetBSCB());

            // IE4 bug #32739, the CBSC might no longer be there (MSN) 
            _try
            {
                hr = pNode->GetBSCB()->OnStopBinding(hrRes, szError);
            }
            //_except(UrlMonInvokeExceptionFilterMSN(GetExceptionCode(), GetExceptionInformation()))
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                #if DBG == 1
                {
                    dwFault = GetExceptionCode();
                    DbgLog1(tagCBSCHolder, this, "fault 0x%08x at OnStopBinding", dwFault);
                }
                #endif
            }
#ifdef unix
            __endexcept
#endif /* unix */
            PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::OnStopBinding done (Node:%lx)", pNode);

            pNode = pNodeNext;
        }

    // Reset bind active flags.

    _fBindStarted = FALSE;
    _fBindStopped = FALSE;

    // BUGBUG: hr is set to return code only from last node we called.
    // Is this what we want?
    // What if one of the earlier nodes failed?

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::OnStopBinding (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::GetPriority
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::GetPriority(LONG * pnPriority)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::GetPriority");
    HRESULT     hr = E_FAIL;
    CBSCNode   *pNode;

    pNode = _pCBSCNode;

    if (pNode && (pNode->GetFlags() & BSCO_GETPRIORITY))
    {
        hr = pNode->GetBSCB()->GetPriority(pnPriority);
    }

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::GetPriority (hr:%lx)", hr);
    return S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::QueryService
//
//  Synopsis:
//
//  Arguments:  [rsid] --
//              [iid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    4-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::QueryService");
    HRESULT     hr = NOERROR;
    VDATETHIS(this);
    UrlMkAssert((ppvObj));

    *ppvObj = 0;

    hr = ObtainService(rsid, riid, ppvObj);
    UrlMkAssert(( (hr == E_NOINTERFACE) || ((hr == NOERROR) && *ppvObj) ));

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::QueryService (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::BeginningTransaction
//
//  Synopsis:
//
//  Arguments:  [szURL] --
//              [szHeaders] --
//              [dwReserved] --
//              [pszAdditionalHeaders] --
//
//  Returns:
//
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
            DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    PerfDbgLog2(tagCBSCHolder, this, "+CBSCHolder::BeginningTransaction (szURL:%ws, szHeaders:%ws)", szURL, XDBG(szHeaders,""));
    VDATETHIS(this);
    HRESULT    hr = NOERROR;
    CBSCNode   *pNode;
    LPWSTR     szTmp = NULL, szNew = NULL, szRunning = NULL;

    pNode = _pCBSCNode;
    UrlMkAssert((szURL));

    while (pNode)
    {
        if (pNode->GetHttpNegotiate())
        {
            hr = pNode->GetHttpNegotiate()->BeginningTransaction(szURL, szHeaders, dwReserved, &szNew);
            PerfDbgLog2(tagCBSCHolder, this, "CBSCHolder::BeginningTransaction (IHttpNegotiate:%lx, szNew:%ws)",
                pNode->GetHttpNegotiate(), XDBG(szNew,L""));

            // shdocvw might return uninitialized hr, so we
            // should just check for szNew not NULL and reset hr
            if( hr != NOERROR && szNew != NULL )
            {
                hr = NOERROR;
            }

            if (hr == NOERROR && szNew != NULL && szRunning != NULL)
            {
                szTmp = szRunning;
                szRunning = new WCHAR [wcslen(szTmp) + 1 + wcslen(szNew) + 1];
                if (szRunning)
                {
                    if (szTmp)
                    {
                        wcscpy(szRunning, szTmp);
                        wcscat(szRunning, szNew);
                    }
                    else
                    {
                        wcscpy(szRunning, szNew);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                delete szTmp;
                delete szNew;

                if (hr != NOERROR)
                {
                    goto BegTransExit;
                }
            }
            else
            {
                szRunning = szNew;
            }
        }

        pNode = pNode->GetNextNode();
    }

    *pszAdditionalHeaders = szRunning;

BegTransExit:
    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::BeginningTransaction (pszAdditionalHeaders:%ws)", (hr || !*pszAdditionalHeaders) ? L"":*pszAdditionalHeaders);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::OnResponse
//
//  Synopsis:
//
//  Arguments:  [LPCWSTR] --
//              [szResponseHeaders] --
//              [LPWSTR] --
//              [pszAdditionalRequestHeaders] --
//
//  Returns:
//
//  History:    4-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::OnResponse(DWORD dwResponseCode,LPCWSTR wzResponseHeaders,
                        LPCWSTR wzRequestHeaders,LPWSTR *pszAdditionalRequestHeaders)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::OnResponse");
    VDATETHIS(this);
    HRESULT    hr;
    CBSCNode   *pNode;
    LPWSTR     szTmp = NULL, szNew = NULL, szRunning = NULL;

    pNode = _pCBSCNode;

    hr = (IsStatusOk(dwResponseCode)) ? S_OK : S_FALSE;

    while (pNode)
    {
        if (pNode->GetHttpNegotiate())
        {
            PerfDbgLog1(tagCBSCHolder, this, "+CBSCHolder::OnResponse on Node: %lx", pNode);
            hr = pNode->GetHttpNegotiate()->OnResponse(dwResponseCode, wzResponseHeaders, wzRequestHeaders, &szNew);
            PerfDbgLog2(tagCBSCHolder, this, "-CBSCHolder::OnResponse on Node: %lx, hr:%lx", pNode, hr);
            if (hr == NOERROR && szNew != NULL && szRunning != NULL)
            {
                szTmp = szRunning;
                szRunning = new WCHAR [wcslen(szTmp) + 1 + wcslen(szNew) + 1];
                if (szRunning)
                {
                    if (szTmp)
                    {
                        wcscpy(szRunning, szTmp);
                        wcscat(szRunning, szNew);
                    }
                    else
                    {
                        wcscpy(szRunning, szNew);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                delete szTmp;
                delete szNew;

                if (hr != NOERROR)
                {
                    goto OnErrorExit;
                }
            }
            else
            {
                szRunning = szNew;
            }
        }
        pNode = pNode->GetNextNode();
    }

    if (pszAdditionalRequestHeaders)
    {
        *pszAdditionalRequestHeaders = szRunning;
    }

    if (hr == E_NOTIMPL)
    {
        hr = NOERROR;
    }

OnErrorExit:

    PerfDbgLog(tagCBSCHolder, this, "-CBSCHolder::OnResponse");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::Authenticate
//
//  Synopsis:
//
//  Arguments:  [phwnd] --
//              [pszUsername] --
//              [pszPassword] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::Authenticate(HWND* phwnd, LPWSTR *pszUsername,
            LPWSTR *pszPassword)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::Authenticate");
    VDATETHIS(this);
    HRESULT hr = NOERROR;
    CBSCNode *pNode;
    pNode = _pCBSCNode;

    while (pNode)
    {
        if (pNode->GetAuthenticate())
        {
            hr = pNode->GetAuthenticate()->Authenticate(phwnd, pszUsername, pszPassword);
            if (hr == S_OK)
            {
                break;
            }
        }

        pNode = pNode->GetNextNode();
    }

    PerfDbgLog(tagCBSCHolder, this, "-CBSCHolder::Authenticate");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::AddNode
//
//  Synopsis:
//
//  Arguments:  [pIBSC] --
//              [grfFlags] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::AddNode(IBindStatusCallback *pIBSC, DWORD grfFlags)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::AddNode");
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    CBSCNode *pFirstNode = _pCBSCNode;
    CBSCNode *pNode;
    CBSCNode *pNodeTmp;
    LPVOID pvLocal = NULL;

    // No new nodes allowed after binding has started.
    if (_fBindStarted)
    {
        hr = E_FAIL;
        goto AddNodeExit;
    }

    // Allocate memory for new pNode member.
    pNode = new CBSCNode(pIBSC, grfFlags);

    if (!pNode)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // addref the IBSC pointer
        pIBSC->AddRef();

        // QI for IServiceProvider - QI addref IBSC
        if (pIBSC->QueryInterface(IID_IServiceProvider, &pvLocal) == NOERROR)
        {
            pNode->SetServiceProvider((IServiceProvider *)pvLocal);
        }

        PerfDbgLog3(tagCBSCHolder, this, "CBSCHolder::AddNode (New Node:%lx, IBSC:%lx, IServiceProvider:%lx)",
            pNode, pNode->GetBSCB(), pvLocal);

        // If we have a node already
        if (pFirstNode)
        {
            if (pNode->GetFlags() & BSCO_ONDATAAVAILABLE)
            {
                // If the new node gets the data, link it first.

                pNode->SetNextNode(pFirstNode);
                _pCBSCNode = pNode;
            }
            else
            {
                // The new node does not get data, link it second in list.

                pNodeTmp = pFirstNode->GetNextNode();

                pFirstNode->SetNextNode(pNode);
                pNode->SetNextNode(pNodeTmp);
            }
        }
        else
        {
            _pCBSCNode = pNode;
        }

        _cElements++;
    }

AddNodeExit:

    PerfDbgLog2(tagCBSCHolder, this, "-CBSCHolder::AddNode (NewNode:%lx, hr:%lx)", pNode, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::RemoveNode
//
//  Synopsis:
//
//  Arguments:  [pIBSC] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::RemoveNode(IBindStatusCallback *pIBSC)
{
    PerfDbgLog1(tagCBSCHolder, this, "+CBSCHolder::RemoveNode (IBSC:%lx)", pIBSC);
    HRESULT hr = E_FAIL;
    CLock lck(_mxs);

    CBSCNode *pNextNode = NULL;
    CBSCNode *pPrevNode = _pCBSCNode;

    // If binding has started, removal of nodes not allowed until binding stops.

    if (_fBindStarted && !_fBindStopped)
    {
        UrlMkAssert((FALSE && "IBSC in use - can not be revoked"));

        goto RemoveNodeExit;
    }

    if (pPrevNode)
    {
        pNextNode = pPrevNode->GetNextNode();
    }
    else
    {
        TransAssert((_cElements == 0));
        hr = S_FALSE;
        goto RemoveNodeExit;
    }

    if (_pCBSCNode->GetBSCB() == pIBSC)
    {
        UrlMkAssert((_pCBSCNode->GetBSCB() == pIBSC));
        if (!_fBindStarted || _pCBSCNode->CheckLocalFlags(NODE_FLAG_REMOVEOK))
        {
            PerfDbgLog2(tagCBSCHolder, this, "CBSCHolder::RemoveNode (Delete Node:%lx, IBSC:%lx)",
                _pCBSCNode, _pCBSCNode->GetBSCB());

            // release all obtained objects in the disdructor
            delete _pCBSCNode;

            _pCBSCNode = pNextNode;
            _cElements--;

            if (_cElements == 0)
            {
                hr = S_FALSE;
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    else while (pNextNode)
    {
        PerfDbgLog2(tagCBSCHolder, this, "CBSCHolder::RemoveNode (pNextNode:%lx, pNextNode->pIBSC:%lx)",pNextNode,pNextNode->GetBSCB());

        if (pNextNode->GetBSCB() == pIBSC && (!_fBindStarted || pNextNode->CheckLocalFlags(NODE_FLAG_REMOVEOK)))
        {
            //we found the Node
            if (pPrevNode)
            {
                pPrevNode->SetNextNode(pNextNode->GetNextNode());
            }

            PerfDbgLog2(tagCBSCHolder, this, "CBSCHolder::RemoveNode (Delete Node:%lx, IBSC:%lx)",
                pNextNode,pNextNode->GetBSCB());

            // release all obtained objects in the disdructor
            delete pNextNode;

            hr = S_OK;
            _cElements--;

            UrlMkAssert((_cElements > 0));
        }
        else
        {
            pPrevNode = pNextNode;
            pNextNode = pNextNode->GetNextNode();
        }
        UrlMkAssert((hr == S_OK && "Node not found"));
    }

RemoveNodeExit:

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::RemoveNode (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::SetMainNode
//
//  Synopsis:
//
//  Arguments:  [pIBSC] --
//              [ppIBSCPrev] --
//
//  Returns:
//
//  History:    5-08-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::SetMainNode(IBindStatusCallback *pIBSC, IBindStatusCallback **ppIBSCPrev)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::SetMainNode");
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    CBSCNode *pFirstNode = _pCBSCNode;
    CBSCNode *pNode;
    CBSCNode *pNodeTmp;
    LPVOID pvLocal = NULL;

    // No new nodes allowed after binding has started.
    if (_fBindStarted)
    {
        hr = E_FAIL;
        goto GetFirsNodeExit;
    }
    if (pFirstNode)
    {
        IBindStatusCallback *pBSC = pFirstNode->GetBSCB();

        // addref the node here and return it
        if (ppIBSCPrev)
        {
            pBSC->AddRef();
            *ppIBSCPrev = pBSC;
        }

        hr = RemoveNode(pBSC);
    }

    pFirstNode = _pCBSCNode;

    // Allocate memory for new pNode member.
    pNode = new CBSCNode(pIBSC, BSCO_ALLONIBSC);

    if (!pNode)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // addref the IBSC pointer
        pIBSC->AddRef();
        hr = NOERROR;

        // QI for IServiceProvider - QI addref IBSC
        if (pIBSC->QueryInterface(IID_IServiceProvider, &pvLocal) == NOERROR)
        {
            pNode->SetServiceProvider((IServiceProvider *)pvLocal);
        }

        PerfDbgLog3(tagCBSCHolder, this, "CBSCHolder::SetMainNode (New Node:%lx, IBSC:%lx, IServiceProvider:%lx)",
            pNode, pNode->GetBSCB(), pvLocal);

        // If we have a node already
        if (pFirstNode)
        {
            if (pNode->GetFlags() & BSCO_ONDATAAVAILABLE)
            {
                // If the new node gets the data, link it first.

                pNode->SetNextNode(pFirstNode);
                _pCBSCNode = pNode;
            }
            else
            {
                // The new node does not get data, link it second in list.

                pNodeTmp = pFirstNode->GetNextNode();

                pFirstNode->SetNextNode(pNode);
                pNode->SetNextNode(pNodeTmp);
            }
        }
        else
        {
            _pCBSCNode = pNode;
        }

        _cElements++;
    }
GetFirsNodeExit:

    PerfDbgLog2(tagCBSCHolder, this, "-CBSCHolder::SetMainNode (NewNode:%lx, hr:%lx)", pNode, hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CBSCHolder::ObtainService
//
//  Synopsis:   Retrieves the requested service with QI and QueryService
//              for all nodes. The interfaces is addref'd and kept in the node.
//
//  Arguments:  [rsid] --
//              [riid] --
//
//  Returns:
//
//  History:    4-09-96   JohannP (Johann Posch)   Created
//
//  Notes:      The obtained interfaces are released in the disdructor of the
//              CNode.
//
//----------------------------------------------------------------------------
HRESULT CBSCHolder::ObtainService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagCBSCHolder, this, "+CBSCHolder::ObtainService");
    HRESULT     hr = NOERROR;
    CBSCNode   *pNode;
    VDATETHIS(this);
    LPVOID pvLocal = NULL;

    pNode = _pCBSCNode;

    // the old code was under the assumption that rsid was always the same
    // as riid. it checked riid when it should have been checking rsid, and it
    // always passed riid on in the place of rsid! All callers that I've
    // seen that use IID_IHttpNegotiate and IID_IAuthenticate pass the
    // same iid in both rsid and riid, so fixing this should be safe.
    if (rsid == IID_IHttpNegotiate)
    {
        *ppvObj = (void*)(IHttpNegotiate *) this;
        AddRef();

        // loop once to get all interfaces
        if (!_fHttpNegotiate)
        {
            while (pNode)
            {
                if (   (pNode->GetBSCB()->QueryInterface(riid, &pvLocal) == NOERROR)
                    || (   pNode->GetServiceProvider()
                        && (pNode->GetHttpNegotiate() == NULL)
                        && (pNode->GetServiceProvider()->QueryService(rsid, riid, &pvLocal)) == NOERROR)
                    )
                {
                    // Note: the interface is addref'd by QI or QS
                    pNode->SetHttpNegotiate((IHttpNegotiate *)pvLocal);
                }

                pNode = pNode->GetNextNode();
            }

            _fHttpNegotiate = TRUE;
        }
    }
    else if (rsid == IID_IAuthenticate)
    {
        *ppvObj = (void*)(IAuthenticate *) this;
        AddRef();

        if (!_fAuthenticate)
        {
            while (pNode)
            {
                if (   (pNode->GetBSCB()->QueryInterface(riid, &pvLocal) == NOERROR)
                    || (   pNode->GetServiceProvider()
                        && (pNode->GetAuthenticate() == NULL)
                        && (pNode->GetServiceProvider()->QueryService(rsid, riid, &pvLocal)) == NOERROR)
                    )
                {
                    // Note: the interface is addref'd by QI or QS
                    pNode->SetAuthenticate((IAuthenticate *)pvLocal);
                }

                pNode = pNode->GetNextNode();
            }

            _fAuthenticate = TRUE;
        }

    }
	else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;

        while (pNode)
        {
			// old urlmon code did a QueryInterface on this object (CBSCHolder)
			// without regard to rsid. That's QueryService badness, but CINet
			// (and several other places) call QueryService using the same rsid/riid
			// (in this case IID_IHttpSecurity) and *expect* the below QI to pick
			// the interface off the BSCB. We should create an URLMON service id
			// that means "ask the BSCB for this interface" and use that...
            if (    (pNode->GetBSCB()->QueryInterface(riid, &pvLocal) == NOERROR)
                 || (pNode->GetServiceProvider()
                 && (pNode->GetServiceProvider()->QueryService(rsid, riid, &pvLocal)) == NOERROR)
                )
            {
                *ppvObj = pvLocal;
                hr = NOERROR;
                // Note: the interface is addref'd by QI or QS
                // stop looking at other nodes for this service
                pNode = NULL;
            }

            if (pNode)
            {
                pNode = pNode->GetNextNode();
            }
        }
    }

    PerfDbgLog1(tagCBSCHolder, this, "-CBSCHolder::ObtainService (hr:%lx)", hr);
    return hr;
}


