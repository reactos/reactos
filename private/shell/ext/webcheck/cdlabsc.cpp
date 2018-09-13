#include <urlmon.h>
#include "private.h"
#include "cdlabsc.h"

CDLAgentBSC::CDLAgentBSC(CCDLAgent *pCdlAgent, DWORD dwMaxSizeKB, BOOL fSilentOperation,
                         LPWSTR szCDFUrl)
{
    int iLen = 0;

    m_cRef = 1;
    m_pIBinding = NULL;
    m_pCdlAgent = pCdlAgent;
    m_fSilentOperation = fSilentOperation;
    m_dwMaxSize = dwMaxSizeKB*1024;
    m_pSecMgr = NULL;


    iLen = lstrlenW(szCDFUrl) + 1;
    iLen = (iLen < INTERNET_MAX_URL_LENGTH) ? (iLen) : (INTERNET_MAX_URL_LENGTH);
    StrCpyNW(m_pwzCDFBase, szCDFUrl, iLen);

    if (m_pCdlAgent != NULL)
    {
        m_pCdlAgent->AddRef();
    }
}

CDLAgentBSC::~CDLAgentBSC()
{
    if (m_pCdlAgent != NULL)
    {
        m_pCdlAgent->Release();
    }

    if (m_pIBinding != NULL)
    {
        m_pIBinding->Release();
    }

    if (m_pSecMgr) {
        m_pSecMgr->Release();
    }
}

HRESULT CDLAgentBSC::Abort()
{
    if (m_pIBinding != NULL) {

        return m_pIBinding->Abort();
    
    } else {
    
        return S_OK;
    
    }
}

/*
 *
 * IUnknown Methods
 *
 */

STDMETHODIMP CDLAgentBSC::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT          hr = E_NOINTERFACE;

    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *) this;
        AddRef();
    }

    if (*ppv != NULL)
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CDLAgentBSC::QueryService(REFGUID rsid, REFIID riid, void ** ppvObj)
{
    HRESULT     hr = NOERROR;
    IServiceProvider        *pIServiceProvider = NULL;

    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = 0;

    if (IsEqualGUID(rsid, IID_IInternetHostSecurityManager) &&
        IsEqualGUID(riid, IID_IInternetHostSecurityManager)) {

        if (m_pSecMgr == NULL) {
            hr = CoInternetCreateSecurityManager(NULL, &m_pSecMgr, NULL);
        }
        
        if (m_pSecMgr) {
            *ppvObj = (IInternetHostSecurityManager *)this;
            AddRef();
        }
        else {
            hr = E_NOINTERFACE;
        }
    }
    else {
        hr = E_NOINTERFACE;
        *ppvObj = NULL;
    }
        

    return hr;
}

STDMETHODIMP_(ULONG) CDLAgentBSC::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CDLAgentBSC::Release()
{
    if (0L != --m_cRef)
    {
        return m_cRef;
    }
    delete this;

    return 0;
}

/*
 *
 * IBindStatusCallback Methods
 *
 */

STDMETHODIMP CDLAgentBSC::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    if (m_pIBinding != NULL)
    {
        m_pIBinding->Release();
    }
    m_pIBinding = pib;

    if (m_pIBinding != NULL)
    {
        m_pIBinding->AddRef();
    }

    return S_OK;
}

STDMETHODIMP CDLAgentBSC::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    HRESULT         hr = S_OK;
    
    if (m_pCdlAgent != NULL)
    {
        m_pCdlAgent->SetErrorEndText(szError);
        m_pCdlAgent->SetEndStatus(hresult);
        m_pCdlAgent->CleanUp();
    }

    return hr;
}

STDMETHODIMP CDLAgentBSC::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    return S_OK;
}

STDMETHODIMP CDLAgentBSC::GetPriority(LONG *pnPriority)
{
    return S_OK;
}

STDMETHODIMP CDLAgentBSC::OnLowResource(DWORD dwReserved)
{
    return S_OK;
}  

STDMETHODIMP CDLAgentBSC::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText)
{
    if ((m_dwMaxSize > 0) && (ulStatusCode == BINDSTATUS_DOWNLOADINGDATA)) {

        if (ulProgress > m_dwMaxSize || ulProgressMax > m_dwMaxSize) {
        
           Abort();
        
        }

    }

    return S_OK;
}


STDMETHODIMP CDLAgentBSC::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
{
   if (m_fSilentOperation)
   {
       *pgrfBINDF |= BINDF_SILENTOPERATION;
   }
    return S_OK;
}

STDMETHODIMP CDLAgentBSC::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                         FORMATETC *pformatetc,
                                         STGMEDIUM *pstgmed)
{
    return S_OK;
}

HRESULT CDLAgentBSC::Pause()
{
    HRESULT              hr = E_FAIL;
    if (m_pIBinding != NULL)
    {
        hr = m_pIBinding->Suspend();
    }

    return hr;
}

HRESULT CDLAgentBSC::Resume()
{
    HRESULT              hr = E_FAIL;
    if (m_pIBinding != NULL)
    {
        hr = m_pIBinding->Resume();
    }

    return hr;
}

// IInternetHostSecurityManager
STDMETHODIMP CDLAgentBSC::GetSecurityId(BYTE *pbSecurityId, DWORD *pcbSecurityId,
                                        DWORD_PTR dwReserved)
{
    HRESULT                    hr = E_FAIL;

    if (m_pSecMgr) {
        hr = m_pSecMgr->GetSecurityId(m_pwzCDFBase, pbSecurityId,
                                     pcbSecurityId, dwReserved);
    }

    return hr;
}

STDMETHODIMP CDLAgentBSC::ProcessUrlAction(DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                           BYTE *pContext, DWORD cbContext, DWORD dwFlags,
                                           DWORD dwReserved)
{
    HRESULT                    hr = E_FAIL;

    if (m_pSecMgr) {
        hr = m_pSecMgr->ProcessUrlAction(m_pwzCDFBase, dwAction, pPolicy,
                                        cbPolicy, pContext, cbContext,
                                        dwFlags, dwReserved);
    }

    return hr;
}

STDMETHODIMP CDLAgentBSC::QueryCustomPolicy(REFGUID guidKey, BYTE **ppPolicy,
                                            DWORD *pcbPolicy, BYTE *pContext,
                                            DWORD cbContext, DWORD dwReserved)
{
    HRESULT                    hr = E_FAIL;

    if (m_pSecMgr) {
        hr = m_pSecMgr->QueryCustomPolicy(m_pwzCDFBase, guidKey, ppPolicy,
                                         pcbPolicy, pContext, cbContext,
                                         dwReserved);
    }

    return hr;
}

