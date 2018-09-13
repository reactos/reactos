//  File:      zonemgr.cxx
//
//  Contents:  Implementation of the IInternetZoneManager interface for basic (i.e. not
//             pluggable protocols with weird Urls)              
//
//  Classes:    CUrlZoneManager
//
//  Functions:
//
//  History: 
//
//----------------------------------------------------------------------------

#include "zonepch.h"


CRegZoneContainer*  CUrlZoneManager::s_pRegZoneContainer = NULL;
CRITICAL_SECTION    CUrlZoneManager::s_csect;
BOOL                CUrlZoneManager::s_bcsectInit = FALSE;

STDAPI 
InternetCreateZoneManager
(
    IUnknown * pUnkOuter,
    REFIID  riid,
    void **ppvObj,
    DWORD dwReserved
)
{   
    HRESULT hr = S_OK;      
    *ppvObj = NULL;

    if ( !IsZonesInitialized() )
        return E_UNEXPECTED;

    if (dwReserved != 0 || !ppvObj || (pUnkOuter && riid != IID_IUnknown))
    {
        // If the object has to be aggregated the caller can only ask
        // for an IUnknown back.
        hr = E_INVALIDARG;
    }
    else 
    {
        CUrlZoneManager * pZoneMgr = new CUrlZoneManager(pUnkOuter, (IUnknown **)ppvObj);

        if ( pZoneMgr )
        {

            if (!pZoneMgr->Initialize())
            {
                hr = E_UNEXPECTED;
            }
            else 
            {
                if (riid == IID_IUnknown || riid == IID_IInternetZoneManager)
                {
                    // The correct pointer is in ppvObj
                    *ppvObj = (IInternetZoneManager *)pZoneMgr;
                }
                else 
                {
                    hr = E_NOINTERFACE;
                }
            }
        }
        else 
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


CUrlZoneManager::CUrlZoneManager(IUnknown *pUnkOuter, IUnknown **ppUnkInner)
{
    DllAddRef();

    m_pSP = NULL;


    if (!pUnkOuter)
    {
        pUnkOuter = &m_Unknown;
    }
    else
    {
        TransAssert(ppUnkInner);
        if (ppUnkInner)
        {
            *ppUnkInner = &m_Unknown;
            m_ref = 0;
        }
    }

    m_pUnkOuter = pUnkOuter;
}


CUrlZoneManager::~CUrlZoneManager()
{
    DllRelease();
}


BOOL CUrlZoneManager::Initialize()
{
    BOOL bReturn = TRUE;
    
    EnterCriticalSection(&s_csect);

    if (s_pRegZoneContainer == NULL)
    {
        // We want to defer the initialization of the shared memory section.
        // This is a convenient place since it is already guarded by a critical 
        // section and this code has to be run before any caching related operations
        // happen. This should be done before the reg zones themselves are initialized
        // since they can call into the shared memory sections.

        g_SharedMem.Init(SM_SECTION_NAME, SM_SECTION_SIZE);

        s_pRegZoneContainer = new CRegZoneContainer();

        if (s_pRegZoneContainer == NULL )
        {
            bReturn = FALSE;   // We are hosed.
        }
        else if (!s_pRegZoneContainer->Attach(g_bUseHKLMOnly))
        {
            bReturn = FALSE;
        }

    }

    LeaveCriticalSection(&s_csect);

    return bReturn;        
}

STDMETHODIMP CUrlZoneManager::CPrivUnknown::QueryInterface(REFIID riid, void** ppvObj)
{
    HRESULT hr = S_OK;
    
    *ppvObj = NULL;

    CUrlZoneManager * pUrlZoneManager = GETPPARENT(this, CUrlZoneManager, m_Unknown);
        
    if (riid == IID_IUnknown || riid == IID_IInternetZoneManager)
    {
        *ppvObj = (IInternetZoneManager *)pUrlZoneManager;
        pUrlZoneManager->AddRef();
    }
    else 
    {
        hr = E_NOINTERFACE;
    }
    
    return hr;
}
                
STDMETHODIMP_(ULONG) CUrlZoneManager::CPrivUnknown::AddRef()
{
    LONG lRet = ++m_ref;

    return lRet;
}

STDMETHODIMP_(ULONG) CUrlZoneManager::CPrivUnknown::Release()
{

    CUrlZoneManager *pUrlZoneManager = GETPPARENT(this, CUrlZoneManager, m_Unknown);

    LONG lRet = --m_ref;

    if (lRet == 0)
    {
        delete pUrlZoneManager;
    }

    return lRet;
}

// IUnknown methods.
STDMETHODIMP CUrlZoneManager::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (riid == IID_IUnknown || riid == IID_IInternetZoneManager)
    {
        *ppvObj = (IInternetZoneManager *)this;
    }

    if (*ppvObj != NULL)
    {
        ((IUnknown *)*ppvObj)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CUrlZoneManager::AddRef()
{
    LONG lRet = m_pUnkOuter->AddRef();

    return lRet;
}

STDMETHODIMP_(ULONG) CUrlZoneManager::Release()
{                            
    LONG lRet = m_pUnkOuter->Release();

    // Controlling Unknown will delete the object if reqd.
        
    return lRet;
}


// The IInternetZoneManager methods

HRESULT
CUrlZoneManager::GetZoneAttributes
(
    DWORD dwZone,
    ZONEATTRIBUTES *pZoneAttributes
)
{
    if (pZoneAttributes == NULL /* || !IsZoneValid(dwZone) */)
    {
        return E_INVALIDARG;
    }

    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone != NULL)
    {
        return pRegZone->GetZoneAttributes(*pZoneAttributes);
    }

    return E_FAIL;
}

HRESULT
CUrlZoneManager::SetZoneAttributes
(
    DWORD dwZone,
    ZONEATTRIBUTES *pZoneAttributes
)
{
    if (pZoneAttributes == NULL /* || !IsZoneValid(dwZone) */) 
    {
        return E_INVALIDARG;
    }
    
    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone != NULL)
    {
        return (pRegZone->SetZoneAttributes(*pZoneAttributes));
    }

    return E_FAIL;
}


HRESULT
CUrlZoneManager::GetZoneCustomPolicy
(
    DWORD dwZone,
    REFGUID guidKey,
    BYTE **ppPolicy,
    DWORD *pcbPolicy,
    URLZONEREG urlZoneReg
)
{
    if (ppPolicy == NULL || pcbPolicy == NULL)
    {
        return E_INVALIDARG;
    }

    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone == NULL)
    {
        return E_INVALIDARG; // Could be screwed up registry here. 
    }

    return (pRegZone->GetCustomPolicy(guidKey, urlZoneReg, ppPolicy, pcbPolicy));
}

HRESULT
CUrlZoneManager::SetZoneCustomPolicy
(
    DWORD dwZone,
    REFGUID guidKey,
    BYTE *pPolicy,
    DWORD cbPolicy,
    URLZONEREG urlZoneReg
)
{
    if (pPolicy == NULL )
    {
        return E_INVALIDARG;
    }

    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone == NULL)
    {
        return E_INVALIDARG;
    }

    return (pRegZone->SetCustomPolicy(guidKey, urlZoneReg, pPolicy, cbPolicy));
}


HRESULT
CUrlZoneManager::GetZoneActionPolicy
(
    DWORD dwZone,
    DWORD dwAction,
    BYTE* pPolicy,
    DWORD cbPolicy,
    URLZONEREG urlZoneReg
)
{
    if (pPolicy == NULL || cbPolicy < sizeof(DWORD))
    {
        return E_INVALIDARG;
    }

        
    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone == NULL)
    {
        return E_INVALIDARG; // Could be screwed up registry here. 
    }

    return (pRegZone->GetActionPolicy(dwAction, urlZoneReg, *((DWORD *)pPolicy)));
}
         

HRESULT 
CUrlZoneManager::SetZoneActionPolicy
(
    DWORD dwZone,
    DWORD dwAction,
    BYTE* pPolicy,
    DWORD cbPolicy,
    URLZONEREG urlZoneReg
)
{
    if (pPolicy == NULL || cbPolicy != sizeof(DWORD))
    {
        return E_INVALIDARG;
    }
        
    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone == NULL)
    {
        return E_INVALIDARG; // Could be screwed up registry here. 
    }

    return (pRegZone->SetActionPolicy(dwAction, urlZoneReg, *((DWORD *)pPolicy)));
}



// Actions that are actually carried out by the Zone Manager.


HRESULT
CUrlZoneManager::PromptAction
(
    DWORD dwAction,
    HWND hwndParent,
    LPCWSTR pwszUrl,
    LPCWSTR pwszText,
    DWORD dwPromptFlags
)
{
    return E_NOTIMPL;
}


HRESULT
CUrlZoneManager::LogAction
(
    DWORD dwAction,
    LPCWSTR pwszUrl,
    LPCWSTR pwszText,
    DWORD dwLogFlags
)
{
    return E_NOTIMPL;
}


// Zone enumerations
// This is really convoluted. These functions don't belong to the CUrlZoneManager but 
// really to the collection of zones. To support this we remember the pointer to the container
// that created us and delegate these functions on. 

HRESULT
CUrlZoneManager::CreateZoneEnumerator
(
    DWORD* pdwEnum,
    DWORD* pdwCount,
    DWORD  dwFlags   
)
{
    if (dwFlags != 0)
        return E_INVALIDARG;

    if (s_pRegZoneContainer != NULL)
    {
        return s_pRegZoneContainer->CreateZoneEnumerator(pdwEnum, pdwCount);
    }
    else
    {
        TransAssert(FALSE);
        return E_FAIL;
    }
}

HRESULT
CUrlZoneManager::GetZoneAt
(
    DWORD dwEnum,
    DWORD dwIndex,
    DWORD *pdwZone
)
{
    if (s_pRegZoneContainer != NULL)
    {
        return s_pRegZoneContainer->GetZoneAt(dwEnum, dwIndex, pdwZone);
    }
    else
    {
        TransAssert(FALSE);
        return E_FAIL;
    }
}

HRESULT
CUrlZoneManager::DestroyZoneEnumerator
(
    DWORD dwEnum
)
{
    if (s_pRegZoneContainer != NULL)
    {
        return s_pRegZoneContainer->DestroyZoneEnumerator(dwEnum);
    }
    else
    {
        TransAssert(FALSE);
        return E_FAIL;
    }
}



HRESULT
CUrlZoneManager::CopyTemplatePoliciesToZone
(
    DWORD dwTemplate,
    DWORD dwZone,
    DWORD dwReserved
)
{

    if (URLTEMPLATE_PREDEFINED_MIN > dwTemplate || URLTEMPLATE_PREDEFINED_MAX < dwTemplate)
        return E_INVALIDARG;
        
    CRegZone *pRegZone = GetRegZoneById(dwZone);
    if (pRegZone == NULL)
    {
        return E_INVALIDARG; // Could be screwed up registry here. 
    }

    return (pRegZone->CopyTemplatePolicies(dwTemplate));
}

    
