/////////////////////////////////////////////////////////////////////////////
// BSC.CPP
//
// Implementation of CBindStatusCallback.
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     07/15/97    Created
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "bsc.h"

#define TF_LIFETIME         0
#define TF_DEBUGREFCOUNT    0
#define TF_DEBUGQI          0
#define TF_BINDING          0

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback
/////////////////////////////////////////////////////////////////////////////
CBindStatusCallback::CBindStatusCallback
(
) : m_cRef(1)
{
    TraceMsg(TF_LIFETIME, "0x%.8X ctor CBindStatusCallback", this);
    m_pBinding = NULL;
}

CBindStatusCallback::~CBindStatusCallback
(
)
{
    TraceMsg(TF_LIFETIME, "0x%.8X dtor CBindStatusCallback", this);

    if (m_pBinding != NULL)
        EVAL(m_pBinding->Release() == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Interfaces
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// IUnknown interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::AddRef
/////////////////////////////////////////////////////////////////////////////
ULONG CBindStatusCallback::AddRef
(
) 
{
    m_cRef++;

    TraceMsg(   TF_DEBUGREFCOUNT,
                "0x%.8X CBindStatusCallback::AddRef, m_cRef = %d",
                this,
                m_cRef);

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::Release
/////////////////////////////////////////////////////////////////////////////
ULONG CBindStatusCallback::Release
(
)
{
    m_cRef--;

    TraceMsg(   TF_DEBUGREFCOUNT,
                "0x%.8X CBindStatusCallback::Release, m_cRef = %d",
                this,
                m_cRef);

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::QueryInterface
/////////////////////////////////////////////////////////////////////////////
HRESULT CBindStatusCallback::QueryInterface
(
    REFIID      riid,
    LPVOID *    ppvObj
)
{
    IUnknown * pUnk;

    *ppvObj = NULL;

    if (InlineIsEqualGUID(riid, IID_IUnknown))
        pUnk = SAFECAST(this, IUnknown *);
    else if (InlineIsEqualGUID(riid, IID_IBindStatusCallback))
        pUnk = SAFECAST(this, IBindStatusCallback *);
    else
    {
#ifdef _DEBUG
        TCHAR szName[128];
        TraceMsg(   TF_DEBUGQI,
                    "0x%.8X CBindStatusCallback::QueryInterface(%s) -- FAILED",
                    this,
                    DebugIIDName(riid, szName));
#endif  // _DEBUG

        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = (void *)pUnk;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IBindStatusCallback interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::OnStartBinding
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::OnStartBinding
(
    DWORD       dwReserved,
    IBinding *  pBinding
)
{
    TraceMsg(TF_BINDING, "0x%.8X OnStartBinding()", this);

    if (pBinding == NULL)
        return E_INVALIDARG;

    ASSERT(m_pBinding == NULL);
    m_pBinding = pBinding;
    m_pBinding->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::GetPriority
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::GetPriority
(
    LONG * pnPriority
)
{
    TraceMsg(TF_BINDING, "0x%.8X GetPriority()", this);

    if (pnPriority == NULL)
        return E_INVALIDARG;

    *pnPriority = THREAD_PRIORITY_NORMAL;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::OnLowResource
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::OnLowResource
(
    DWORD reserved
)
{
    TraceMsg(TF_BINDING, "0x%.8X OnLowResource()", this);
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::OnProgress
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::OnProgress
(
    ULONG   ulProgress,
    ULONG   ulProgressMax,
    ULONG   ulStatusCode,
    LPCWSTR szStatusText
)
{
    TraceMsg(   TF_BINDING,
                "0x%.8X OnProgress(ulProgress = %u, ulProgressMax = %d, ulStatusCode = %u)",
                this,
                ulProgress, ulProgressMax,
                ulStatusCode);

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::OnStopBinding
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::OnStopBinding
(
    HRESULT hrResult,
    LPCWSTR szError
)
{
    TraceMsg(TF_BINDING, "0x%.8X CBindStatusCallback::OnStopBinding(0x%.8X)", this, hrResult);

    if (m_pBinding != NULL)
    {
        m_pBinding->Release();
        m_pBinding = NULL;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::GetBindInfo
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::GetBindInfo
(
    DWORD *     grfBINDF,
    BINDINFO *  pbindinfo
)
{
    TraceMsg(TF_BINDING, "0x%.8X GetBindInfo()", this);

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::OnDataAvailable
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::OnDataAvailable
(
    DWORD       grfBSCF,
    DWORD       dwSize,
    FORMATETC * pformatetc,
    STGMEDIUM * pstgmed
)
{
    TraceMsg(   TF_BINDING,
                "0x%.8X OnDataAvailable(0x%.8X, %d)",
                this, grfBSCF, dwSize);

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback::OnObjectAvailable
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBindStatusCallback::OnObjectAvailable
(
    REFIID      riid,
    IUnknown *  punk
)
{
    TraceMsg(TF_BINDING, "0x%.8X OnObjectAvailable()", this);
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC
/////////////////////////////////////////////////////////////////////////////

CTimeoutBSC::CTimeoutBSC
(
    DWORD dwTimeoutMS
) : m_dwTimeoutMS(dwTimeoutMS)
{
    TraceMsg(TF_LIFETIME, "0x%.8X ctor CTimeoutBSC", this);

    m_idTimeoutEvent = NULL;
}

CTimeoutBSC::~CTimeoutBSC
(
)
{
    TraceMsg(TF_LIFETIME, "0x%.8X dtor CTimeoutBSC", this);

    StopTimeoutTimer();
}

/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC::StopTimeoutTimer
/////////////////////////////////////////////////////////////////////////////
void CTimeoutBSC::StopTimeoutTimer
(
)
{
    if (m_idTimeoutEvent != NULL)
    {
        EVAL(timeKillEvent(m_idTimeoutEvent) == TIMERR_NOERROR);
        m_idTimeoutEvent = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC::Abort
/////////////////////////////////////////////////////////////////////////////
HRESULT CTimeoutBSC::Abort
(
)
{ 
    return ((m_pBinding != NULL) ? m_pBinding->Abort() : E_FAIL);
}

/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC::OnStartBinding
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTimeoutBSC::OnStartBinding
(
    DWORD       dwReserved,
    IBinding *  pBinding
)
{
    HRESULT hrResult;

    TraceMsg(TF_BINDING, "0x%.8X CTimeoutBSC::OnStartBinding()", this);

    for (;;)
    {
        if (FAILED(hrResult = CBindStatusCallback::OnStartBinding(  dwReserved,
                                                                    pBinding)))
        {
            break;
        }

        if ((m_idTimeoutEvent = timeSetEvent(   m_dwTimeoutMS,
                                                500,
                                                BSCTimeoutTimerProc,
                                                (DWORD_PTR)SAFECAST(this, CTimeoutBSC *),
                                                TIME_ONESHOT)) == NULL)
        {
            hrResult = E_FAIL;
            break;
        }

        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC::OnProgress
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTimeoutBSC::OnProgress
(
    ULONG   ulProgress,
    ULONG   ulProgressMax,
    ULONG   ulStatusCode,
    LPCWSTR szStatusText
)
{
    TraceMsg(   TF_BINDING,
                "0x%.8X OnProgress(ulProgress = %u, ulProgressMax = %d, ulStatusCode = %u)",
                this,
                ulProgress, ulProgressMax,
                ulStatusCode);

    if (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA)
        StopTimeoutTimer();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC::OnStopBinding
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTimeoutBSC::OnStopBinding
(
    HRESULT hrResult,
    LPCWSTR szError
)
{
    TraceMsg(TF_BINDING, "0x%.8X CTimeoutBSC::OnStopBinding(0x%X)", this, hrResult);

    StopTimeoutTimer();
    return CBindStatusCallback::OnStopBinding(hrResult, szError);;
}

/////////////////////////////////////////////////////////////////////////////
// BSCTimeoutTimerProc
/////////////////////////////////////////////////////////////////////////////
void CALLBACK BSCTimeoutTimerProc
(
    UINT    uID,
    UINT    uMsg,  
    DWORD_PTR dwUser,
    DWORD_PTR dw1,
    DWORD_PTR dw2
)
{
    CTimeoutBSC * pTimeoutBSC = (CTimeoutBSC *)dwUser;
    
    TraceMsg(TF_BINDING, "!!!ABORTING DOWNLOAD ATTEMPT!!!");

    if (pTimeoutBSC != NULL)
        EVAL(SUCCEEDED(pTimeoutBSC->Abort()));
}


/////////////////////////////////////////////////////////////////////////////
// CSSNavigateBSC
/////////////////////////////////////////////////////////////////////////////

CSSNavigateBSC::CSSNavigateBSC
(
    CScreenSaverWindow *    pScreenSaverWnd,
    DWORD                   dwTimeoutMS
) : CTimeoutBSC(dwTimeoutMS), m_pScreenSaverWnd(pScreenSaverWnd)
{
    TraceMsg(TF_LIFETIME, "0x%.8X ctor CSSNavigateBSC", this);

    ASSERT(pScreenSaverWnd != NULL);
}

CSSNavigateBSC::~CSSNavigateBSC
(
)
{
    TraceMsg(TF_LIFETIME, "0x%.8X dtor CSSNavigateBSC", this);
}

/////////////////////////////////////////////////////////////////////////////
// CSSNavigateBSC::OnStopBinding
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSSNavigateBSC::OnStopBinding
(
    HRESULT hrResultBind,
    LPCWSTR szError
)
{
    TraceMsg(TF_BINDING, "0x%.8X CSSNavigateBSC::OnStopBinding(0x%.8X)", this, hrResultBind);

    HRESULT hrResult = CTimeoutBSC::OnStopBinding(hrResultBind, szError);

    if (FAILED(hrResultBind))
    {
        // If we failed because of a resource problem (i.e. the
        // net is hosed) go offline for the rest of the session.
        if (hrResult == INET_E_RESOURCE_NOT_FOUND)
            m_pScreenSaverWnd->GoOffline();

        m_pScreenSaverWnd->PostMessage(WM_ABORTNAVIGATE);
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CSSNavigateBSC::GetBindInfo
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSSNavigateBSC::GetBindInfo
(
    DWORD *     grfBINDF,
    BINDINFO *  pbindinfo
)
{
    TraceMsg(TF_BINDING, "0x%.8X CSSNavigateBSC::GetBindInfo()", this);

    if ( !grfBINDF || !pbindinfo || !pbindinfo->cbSize )
        return E_INVALIDARG;

    // Clear BINDINFO, but keep its size
    DWORD cbSize = pbindinfo->cbSize;
    ZeroMemory( pbindinfo, cbSize );
    pbindinfo->cbSize = cbSize;

    *grfBINDF = BINDF_ASYNCHRONOUS
                    | BINDF_NOPROGRESSIVERENDERING
                    | BINDF_SILENTOPERATION
                    | BINDF_NO_UI;

    if (m_pScreenSaverWnd->IsOffline())
        *grfBINDF |= BINDF_OFFLINEOPERATION;

    // Default method is GET.
    pbindinfo->dwBindVerb = BINDVERB_GET;

    ASSERT(pbindinfo->stgmedData.tymed == TYMED_NULL);
    ASSERT(pbindinfo->stgmedData.hGlobal == NULL);
    ASSERT(pbindinfo->stgmedData.pUnkForRelease == NULL);
 
    return S_OK;
}
