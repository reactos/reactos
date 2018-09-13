#include "private.h"
#include "subsmgrp.h"
#include "downld.h"
#include "chanmgr.h"
#include "chanmgrp.h"
#include "helper.h"
#include "shguidp.h"    // IID_IChannelMgrPriv

#undef TF_THISMODULE
#define TF_THISMODULE   TF_CDFAGENT


const int MINUTES_PER_DAY = 24 * 60;


//==============================================================================
// Convert an XML OM to a TaskTrigger by looking for and converting <schedule>
//==============================================================================
// returns S_OK if succeeded. (S_FALSE if succeeded but TASK_TRIGGER was truncated).
// returns E_FAIL if no task trigger retrieved (returned *ptt is invalid TASK_TRIGGER)
//  You Must fill in ptt->cbTriggerSize!!!
// User can pass in Schedule element itself, or any parent element, in pRootEle
HRESULT XMLScheduleElementToTaskTrigger(IXMLElement *pRootEle, TASK_TRIGGER *ptt)
{
    HRESULT hr = E_FAIL;

    if (!pRootEle || !ptt)
        return E_INVALIDARG;

    ASSERT(ptt->cbTriggerSize == sizeof(TASK_TRIGGER));

    CExtractSchedule *pSched = new CExtractSchedule(pRootEle, NULL);

    if (pSched)
    {
        if (SUCCEEDED(pSched->Run()))
        {
            hr = pSched->GetTaskTrigger(ptt);
        }

        delete pSched;
    }

    return hr;
}

// CExtractSchedule doesn't get used during channel update
//  It's just used to traverse the OM and find the first Schedule tag, to
//   parse out the schedule info
CExtractSchedule::CExtractSchedule(IXMLElement *pEle, CExtractSchedule *pExtractRoot) :
        CProcessElement(NULL, NULL, pEle)
{
    m_pExtractRoot = pExtractRoot;
    if (!pExtractRoot)
        m_pExtractRoot = this;
}

HRESULT CExtractSchedule::Run()
{
    // Allow user to pass in Schedule element itself, or Root element
    BSTR bstrItem=NULL;
    HRESULT hr;

    m_pElement->get_tagName(&bstrItem);

    if (bstrItem && *bstrItem && !StrCmpIW(bstrItem, L"Schedule"))
    {
        hr = ProcessItemInEnum(bstrItem, m_pElement);
    }
    else
    {
        hr = CProcessElement::Run();
    }

    SysFreeString(bstrItem);
    return hr;
}

HRESULT CExtractSchedule::ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem)
{
    if (!StrCmpIW(pwszTagName, L"Schedule"))
    {
        CProcessSchedule *pPS = new CProcessSchedule(this, NULL, pItem);
        if (pPS)
        {
            pPS->Run();

            if (pPS->m_tt.cbTriggerSize)
            {
                ASSERT(pPS->m_tt.cbTriggerSize == sizeof(m_tt));
                m_pExtractRoot->m_tt = pPS->m_tt;
            }

            delete pPS;
        }
        return E_ABORT; // abort our enumerations
    }
    else if (!StrCmpIW(pwszTagName, L"Channel"))
    {
        return DoChild(new CExtractSchedule(pItem, m_pExtractRoot));
    }

    return S_OK;    // ignore other tags
}

HRESULT CExtractSchedule::GetTaskTrigger(TASK_TRIGGER *ptt)
{
    if ((0 == m_tt.cbTriggerSize) ||            // No task trigger
        (0 == m_tt.wBeginYear))                 // Invalid task trigger
    {
        return E_FAIL;
    }

    if (m_tt.cbTriggerSize <= ptt->cbTriggerSize)
    {
        *ptt = m_tt;
        return S_OK;
    }

    WORD cbTriggerSize = ptt->cbTriggerSize;

    CopyMemory(ptt, &m_tt, cbTriggerSize);
    ptt->cbTriggerSize = cbTriggerSize;

    return S_FALSE;
}

//==============================================================================
// XML OM Helper functions
//==============================================================================
HRESULT GetXMLAttribute(IXMLElement *pItem, LPCWSTR pwszAttribute, VARIANT *pvRet)
{
    BSTR bstrName=NULL;
    HRESULT hr=E_FAIL;

    pvRet->vt = VT_EMPTY;
    bstrName = SysAllocString(pwszAttribute);
    if (bstrName && SUCCEEDED(pItem->getAttribute(bstrName, pvRet)))
    {
        hr = S_OK;
    }
    SysFreeString(bstrName);
    return hr;
}

HRESULT GetXMLStringAttribute(IXMLElement *pItem, LPCWSTR pwszAttribute, BSTR *pbstrRet)
{
    VARIANT var;
    BSTR bstrName=NULL;
    HRESULT hr=E_FAIL;

    *pbstrRet = NULL;

    var.vt = VT_EMPTY;
    bstrName = SysAllocString(pwszAttribute);
    if (bstrName && SUCCEEDED(pItem->getAttribute(bstrName, &var)))
    {
        if (var.vt == VT_BSTR && var.bstrVal != NULL)
        {
            *pbstrRet = var.bstrVal;

            hr = S_OK;
        }
    }
    SysFreeString(bstrName);
    if (FAILED(hr) && var.vt != VT_EMPTY)
        VariantClear(&var);

    return hr;
}

DWORD GetXMLDwordAttribute(IXMLElement *pItem, LPCWSTR pwszAttribute, DWORD dwDefault)
{
    VARIANT var;

    if (SUCCEEDED(GetXMLAttribute(pItem, pwszAttribute, &var)))
    {
        if (var.vt == VT_I4)
            return var.lVal;

        if (var.vt == VT_I2)
            return var.iVal;

        if (var.vt == VT_BSTR)
        {
            LPCWSTR pwsz = var.bstrVal;
            DWORD   dwRet;

            if (!StrToIntExW(pwsz, 0, (int *)&dwRet))
                dwRet = dwDefault;

            SysFreeString(var.bstrVal);
            return dwRet;
        }

        VariantClear(&var);
    }

    return dwDefault;
}

// If failure return code, *pfRet wasn't changed
HRESULT GetXMLBoolAttribute(IXMLElement *pItem, LPCWSTR pwszAttribute, BOOL *pfRet)
{
    VARIANT var;
    HRESULT hr=E_FAIL;

    if (SUCCEEDED(GetXMLAttribute(pItem, pwszAttribute, &var)))
    {
        if (var.vt == VT_BOOL)
        {
            *pfRet = (var.boolVal == VARIANT_TRUE);
            hr = S_OK;
        }
        else if (var.vt == VT_BSTR)
        {
            if (!StrCmpIW(var.bstrVal, L"YES") ||
                !StrCmpIW(var.bstrVal, L"\"YES\""))
            {
                *pfRet = TRUE;
                hr = S_OK;
            }
            else if (!StrCmpIW(var.bstrVal, L"NO") ||
                     !StrCmpIW(var.bstrVal, L"\"NO\""))
            {
                *pfRet = FALSE;
                hr = S_OK;
            }
        }
        else
            hr = E_FAIL;

        VariantClear(&var);
    }

    return hr;
}

HRESULT GetXMLTimeAttributes(IXMLElement *pItem, CDF_TIME *pTime)
{
    pTime->wReserved = 0;

    pTime->wDay = (WORD)  GetXMLDwordAttribute(pItem, L"DAY", 0);
    pTime->wHour = (WORD) GetXMLDwordAttribute(pItem, L"HOUR", 0);
    pTime->wMin = (WORD)  GetXMLDwordAttribute(pItem, L"MIN", 0);

    pTime->dwConvertedMinutes = (24 * 60 * pTime->wDay) +
                                (     60 * pTime->wHour) +
                                (          pTime->wMin);

    return S_OK;
}

inline BOOL IsNumber(WCHAR x) { return (x >= L'0' && x <= L'9'); }

HRESULT GetXMLTimeZoneAttribute(IXMLElement *pItem, LPCWSTR pwszAttribute, int *piRet)
{
    BSTR    bstrVal;
    HRESULT hrRet = E_FAIL;

    ASSERT(pItem && piRet);

    if (SUCCEEDED(GetXMLStringAttribute(pItem, pwszAttribute, &bstrVal)))
    {
        if(bstrVal && bstrVal[0] &&
            IsNumber(bstrVal[1]) && IsNumber(bstrVal[2]) &&
            IsNumber(bstrVal[3]) && IsNumber(bstrVal[4]))
        {
            *piRet  =   1000*(bstrVal[1] - L'0') +
                         100*(bstrVal[2] - L'0') +
                          10*(bstrVal[3] - L'0') +
                              bstrVal[4] - L'0';

            hrRet = S_OK;
        }
        if(bstrVal[0] == L'-')
            *piRet *= -1;
    }

    SysFreeString(bstrVal);

    return hrRet;
}
//==============================================================================
// TEMP fn to convert ISO 1234:5678 to SYSTEMTIME
// ISODateToSystemTime returns false if there is a parse error
// true if there isn't
//==============================================================================

// yyyy-mm-dd[Thh:mm[+zzzz]]
BOOL ISODateToSystemTime(LPCWSTR string, SYSTEMTIME *time, long *timezone)
{
    if (!string || (lstrlenW(string) < 10) || !time)
        return FALSE;

    ZeroMemory(time, sizeof(SYSTEMTIME));

    if (timezone)
        *timezone = 0;

    if (IsNumber(string[0]) &&
        IsNumber(string[1]) &&
        IsNumber(string[2]) &&
        IsNumber(string[3]) &&
       (string[4] != L'\0') &&
        IsNumber(string[5]) &&
        IsNumber(string[6]) &&
       (string[7] != L'\0') &&
        IsNumber(string[8]) &&
        IsNumber(string[9]))
    {
        time->wYear = 1000*(string[0] - L'0') +
                       100*(string[1] - L'0') +
                        10*(string[2] - L'0') +
                            string[3] - L'0';

        time->wMonth = 10*(string[5] - L'0') + string[6] - L'0';

        time->wDay = 10*(string[8] - L'0') + string[9] - L'0';
    }
    else
    {
        return FALSE;
    }

    if ((string[10]!= L'\0') &&
        IsNumber(string[11]) &&
        IsNumber(string[12]) &&
       (string[13] != L'\0') &&
        IsNumber(string[14]) &&
        IsNumber(string[15]))
    {
        time->wHour   = 10*(string[11] - L'0') + string[12] - L'0';
        time->wMinute = 10*(string[14] - L'0') + string[15] - L'0';
    }
    else
    {
        return TRUE;
    }

    if (timezone &&
        (string[16]!= L'\0') &&
        IsNumber(string[17]) &&
        IsNumber(string[18]) &&
        IsNumber(string[19]) &&
        IsNumber(string[20]))
    {
        *timezone  =    1000*(string[17] - L'0') +
                        100*(string[18] - L'0') +
                        10*(string[19] - L'0') +
                        string[20] - L'0';

        if(string[16] == L'-')
            *timezone = - *timezone;
    }

    return TRUE;
}


//==============================================================================
// CProcessElement class provides generic support for sync or async enumeration
//   of an XML OM
//==============================================================================
CProcessElement::CProcessElement(CProcessElementSink *pParent,
                                 CProcessRoot *pRoot,
                                 IXMLElement *pEle)
{
    ASSERT(m_pRunAgent == NULL && m_pCurChild == NULL && m_pCollection == NULL);
        
    m_pElement = pEle; pEle->AddRef();
    m_pRoot = pRoot;
    m_pParent = pParent;
}

CProcessElement::~CProcessElement()
{
    ASSERT(!m_pCurChild);

    CRunDeliveryAgent::SafeRelease(m_pRunAgent);

    SAFERELEASE(m_pCollection);
    SAFERELEASE(m_pElement);
    SAFERELEASE(m_pChildElement);
}

HRESULT CProcessElement::Pause(DWORD dwFlags)
{
    if (m_pCurChild)
        return m_pCurChild->Pause(dwFlags);

    ASSERT(m_pRunAgent);

    if (m_pRunAgent)
        return m_pRunAgent->AgentPause(dwFlags);

    return E_FAIL;
}

HRESULT CProcessElement::Resume(DWORD dwFlags)
{
    if (m_pCurChild)
        return m_pCurChild->Resume(dwFlags);

    if (m_pRunAgent)
        m_pRunAgent->AgentResume(dwFlags);
    else
        DoEnumeration();

    return S_OK;
}

HRESULT CProcessElement::Abort(DWORD dwFlags)
{
    if (m_pCurChild)
    {
        m_pCurChild->Abort(dwFlags);
        SAFEDELETE(m_pCurChild);
    }
    if (m_pRunAgent)
    {
        // Prevent reentrancy into OnAgentEnd
        m_pRunAgent->LeaveMeAlone();
        m_pRunAgent->AgentAbort(dwFlags);
        CRunDeliveryAgent::SafeRelease(m_pRunAgent);
    }

    return S_OK;
}


HRESULT CProcessElement::Run()
{
    ASSERT(!m_pCollection);
    ASSERT(m_lMax == 0);
//  ASSERT(m_fSentEnumerationComplete == FALSE);    // DoEnumeration may have sent this

    m_lIndex = 0;

    if (SUCCEEDED(m_pElement->get_children(&m_pCollection)) && m_pCollection)
    {
        m_pCollection->get_length(&m_lMax);
    }
    else
        m_lMax = 0;

    return DoEnumeration(); // Will call OnChildDone when appropriate
}

HRESULT CProcessElement::OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                               long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                               BOOL fSynchronous)
{
    // Our delivery agent is done. Continue enumeration
    ASSERT(!m_pCurChild);

    if (lSizeDownloaded > 0)
        m_pRoot->m_dwCurSizeKB += (ULONG) lSizeDownloaded;

    TraceMsg(TF_THISMODULE, "ChannelAgent up to %dkb of %dkb", m_pRoot->m_dwCurSizeKB, m_pRoot->m_pChannelAgent->m_dwMaxSizeKB);

    if ((hrResult == INET_E_AGENT_MAX_SIZE_EXCEEDED) ||
        (hrResult == INET_E_AGENT_CACHE_SIZE_EXCEEDED))
    {
        DBG("CProcessElement got max size or cache size exceeded; not running any more delivery agents");

        m_pRoot->m_fMaxSizeExceeded = TRUE;
        m_pRoot->m_pChannelAgent->SetEndStatus(hrResult);
    }

    CRunDeliveryAgent::SafeRelease(m_pRunAgent);

    if (fSynchronous)
    {
        // we are still in our DoDeliveryAgent call. Let it return out through there.
        return S_OK;
    }

    // Continue enumeration, or start enumeration if we haven't yet.
    if (m_fStartedEnumeration)
        DoEnumeration();
    else
        Run();

    return S_OK;
}

HRESULT CProcessElement::DoEnumeration()
{
    IDispatch   *pDisp;
    IXMLElement *pItem;
    BSTR        bstrTagName;
    VARIANT     vIndex, vEmpty;
    HRESULT     hr = S_OK;
    BOOL        fStarted = FALSE;

    m_fStartedEnumeration = TRUE;

    ASSERT(m_pCollection || !m_lMax);

    if (m_pRoot && m_pRoot->IsPaused())
    {
        DBG("CProcessElement::DoEnumeration returning E_PENDING, we're paused");
        return E_PENDING;
    }

    vEmpty.vt = VT_EMPTY;

    for (; (m_lIndex < m_lMax) && !fStarted && (hr != E_ABORT); m_lIndex++)
    {
        vIndex.vt = VT_UI4;
        vIndex.lVal = m_lIndex;

        if (SUCCEEDED(m_pCollection->item(vIndex, vEmpty, &pDisp)))
        {
            if (SUCCEEDED(pDisp->QueryInterface(IID_IXMLElement, (void **)&pItem)))
            {
                if (SUCCEEDED(pItem->get_tagName(&bstrTagName)) && bstrTagName)
                {
                    SAFERELEASE(m_pChildElement);
                    m_pChildElement=pItem;
                    m_pChildElement->AddRef();

                    hr = ProcessItemInEnum(bstrTagName, pItem);
                    SysFreeString(bstrTagName);
                    if (hr == E_PENDING)
                        fStarted = TRUE;
                }
                pItem->Release();
            }
            pDisp->Release();
        }
    }

    // Tell this instance we're done with enumeration, unless we already have
    if (!fStarted && !m_fSentEnumerationComplete)
    {
        m_fSentEnumerationComplete = TRUE;
        hr = EnumerationComplete();     // bugbug this eats E_ABORT
        if (hr == E_PENDING)
            fStarted = TRUE;
    }

    // Notify our parent if we're done with our enumeration,
    if (!fStarted)
    {
        if (m_pParent)  // Check for CExtractSchedule
            m_pParent->OnChildDone(this, hr); // This may delete us
    }

    if (hr == E_ABORT)
        return E_ABORT;

    return (fStarted) ? E_PENDING : S_OK;
}

HRESULT CProcessElement::OnChildDone(CProcessElement *pChild, HRESULT hr)
{
    ASSERT(pChild && (!m_pCurChild || (pChild == m_pCurChild)));

    if (m_pCurChild)
    {
        // A child returned from async operation.
        SAFEDELETE(m_pCurChild);

        // Continue enumeration. This will call our parent's ChildDone if it
        //  finishes, so it may delete us.
        DoEnumeration();
    }
    else
    {
        // Our child has finished synchronously. Ignore (DoChild() will take care of it).
    }

    return S_OK;
}

HRESULT CProcessElement::DoChild(CProcessElement *pChild)
{
    HRESULT hr;

    ASSERT(m_pCurChild == NULL);

    if (!pChild)
        return E_POINTER;   // BUGBUG should call parent's OnChildDone here

    hr = pChild->Run();

    if (hr == E_PENDING)
    {
        // Returned async. Will call back OnChildDone.
        m_pCurChild = pChild;
        return E_PENDING;
    }

    // Synchronously returned. Clean up.
    delete pChild;

    return hr;
}

// E_PENDING if async operation started
HRESULT CProcessElement::DoDeliveryAgent(ISubscriptionItem *pItem, REFCLSID rclsid, LPCWSTR pwszURL)
{
    ASSERT(pItem);

    HRESULT hr=E_FAIL;

    if (m_pRoot->m_fMaxSizeExceeded)
    {
//      DBG("CProcessElement::RunDeliveryAgent failing; exceeded max size.");
        return E_FAIL;
    }

    if (m_pRunAgent)
    {
        DBG_WARN("CProcessElement::DoDeliveryAgent already running!");
        return E_FAIL;
    }

    m_pRunAgent = new CChannelAgentHolder(m_pRoot->m_pChannelAgent, this);

    if (m_pRunAgent)
    {
        hr = m_pRunAgent->Init(this, pItem, rclsid);

        if (SUCCEEDED(hr))
            hr = m_pRunAgent->StartAgent();

        if (hr == E_PENDING)
        {
            m_pRoot->m_pChannelAgent->SendUpdateProgress(pwszURL, ++(m_pRoot->m_iTotalStarted), -1, m_pRoot->m_dwCurSizeKB);
        }
        else
            CRunDeliveryAgent::SafeRelease(m_pRunAgent);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT CProcessElement::DoSoftDist(IXMLElement *pItem)
{
HRESULT hr = S_OK;
ISubscriptionItem *pSubsItem;

    if (SUCCEEDED(m_pRoot->CreateStartItem(&pSubsItem)))
    {
        if (pSubsItem)
        {
            hr = DoDeliveryAgent(pSubsItem, CLSID_CDLAgent);
            pSubsItem->Release();
        }
    }
    return hr;
}

HRESULT CProcessElement::DoWebCrawl(IXMLElement *pItem, LPCWSTR pwszURL /* = NULL */)
{
    BSTR bstrURL=NULL, bstrTmp=NULL;
    HRESULT hr = S_OK;
    ISubscriptionItem *pSubsItem;
    DWORD   dwLevels=0, dwFlags;
    LPWSTR  pwszUrl2=NULL;
    BOOL    fOffline=FALSE;

    if (!pwszURL && SUCCEEDED(GetXMLStringAttribute(pItem, L"HREF", &bstrURL)) && bstrURL)
        pwszURL = bstrURL;

    if (pwszURL)
    {
        SYSTEMTIME  stLastMod;
        long        lTimezone;

        hr = CombineWithBaseUrl(pwszURL, &pwszUrl2);

        if (SUCCEEDED(hr) && pwszUrl2)
            pwszURL = pwszUrl2;     // Got a new URL

        hr = CUrlDownload::IsValidURL(pwszURL);

        if (SUCCEEDED(hr) &&
            SUCCEEDED(GetXMLStringAttribute(m_pElement, L"LastMod", &bstrTmp)) &&
            ISODateToSystemTime(bstrTmp, &stLastMod, &lTimezone))
        {
            // Check Last Modified time
            TCHAR   szThisUrl[INTERNET_MAX_URL_LENGTH];
            char    chBuf[MY_MAX_CACHE_ENTRY_INFO];
            DWORD   dwBufSize = sizeof(chBuf);
            LPINTERNET_CACHE_ENTRY_INFO lpInfo = (LPINTERNET_CACHE_ENTRY_INFO) chBuf;

            MyOleStrToStrN(szThisUrl, INTERNET_MAX_URL_LENGTH, pwszURL);
            hr = GetUrlInfoAndMakeSticky(NULL, szThisUrl, lpInfo, dwBufSize, 0);

            if (SUCCEEDED(hr))
            {
                FILETIME ft;

                if (SystemTimeToFileTime(&stLastMod, &ft))
                {
                    // BUGBUG: In an ideal world, all servers would support LastModifiedTime accurately.
                    // In our world, some do not support it and wininet returns a value of zero.
                    // Without maintaining checksums of the files, we have two options: always download
                    // the URL or never update it. Since it would be silly not to update it, we always do.
                    if ((lpInfo->LastModifiedTime.dwHighDateTime || lpInfo->LastModifiedTime.dwLowDateTime)
                        && (lpInfo->LastModifiedTime.dwHighDateTime >= ft.dwHighDateTime)
                        && ((lpInfo->LastModifiedTime.dwHighDateTime > ft.dwHighDateTime)
                         || (lpInfo->LastModifiedTime.dwLowDateTime >= ft.dwLowDateTime)))
                    {
                        // Skip it.
                        TraceMsg(TF_THISMODULE, "Running webcrawl OFFLINE due to Last Modified time URL=%ws", pwszURL);
                        fOffline = TRUE;
                    }
                }
            }

            hr = S_OK;
        }

        SAFEFREEBSTR(bstrTmp);

        if (SUCCEEDED(hr) && SUCCEEDED(m_pRoot->CreateStartItem(&pSubsItem)))
        {
            WriteOLESTR(pSubsItem, c_szPropURL, pwszURL);

            dwLevels = GetXMLDwordAttribute(pItem, L"LEVEL", 0);
            if (dwLevels && m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SOME))
            {
                // Note: MaxChannelLevels is stored as N+1 because 0
                // means the restriction is disabled.
                DWORD dwMaxLevels = SHRestricted2W(REST_MaxChannelLevels, NULL, 0);
                if (!dwMaxLevels)
                    dwMaxLevels = MAX_CDF_CRAWL_LEVELS + 1;
                if (dwLevels >= dwMaxLevels)
                    dwLevels = dwMaxLevels - 1;
                WriteDWORD(pSubsItem, c_szPropCrawlLevels, dwLevels);
            }

            if (fOffline)
            {
                if (SUCCEEDED(ReadDWORD(pSubsItem, c_szPropCrawlFlags, &dwFlags)))
                {
                    dwFlags |= CWebCrawler::WEBCRAWL_PRIV_OFFLINE_MODE;
                    WriteDWORD(pSubsItem, c_szPropCrawlFlags, dwFlags);
                }
            }

            hr = DoDeliveryAgent(pSubsItem, CLSID_WebCrawlerAgent, pwszURL);
            
            SAFERELEASE(pSubsItem);
        }
    }

    if (bstrURL)
        SysFreeString(bstrURL);

    if (pwszUrl2)
        MemFree(pwszUrl2);

    return hr;
}

BOOL CProcessElement::ShouldDownloadLogo(IXMLElement *pLogo)
{
    return m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SOME);
}

// If relative url, will combine with most recent base URL
// *ppwszRetUrl should be NULL & will be MemAlloced
HRESULT CProcessElement::CombineWithBaseUrl(LPCWSTR pwszUrl, LPWSTR *ppwszRetUrl)
{
    ASSERT(ppwszRetUrl && !*ppwszRetUrl && pwszUrl);

    // Optimization: if pwszURL is absolute, we don't need to do this expensive
    //  combine operation
//  if (*pwszUrl != L'/')   // BOGUS
//  {
//      *ppwszRetUrl = StrDupW(pwszUrl);
//      return S_FALSE;     // Succeeded; pwszUrl is already OK
//  }

    // Find appropriate Base URL to use
    LPCWSTR pwszBaseUrl = GetBaseUrl();

    WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = ARRAYSIZE(wszUrl);

    if (SUCCEEDED(UrlCombineW(pwszBaseUrl, pwszUrl, wszUrl, &dwLen, 0)))
    {
        *ppwszRetUrl = StrDupW(wszUrl);
        return (*ppwszRetUrl) ? S_OK : E_OUTOFMEMORY;
    }

    *ppwszRetUrl = NULL;

    return E_FAIL;  // Erg?
}



//==============================================================================
// CProcessElement derived classes, to handle specific CDF tags
//==============================================================================
// CProcessRoot doesn't behave like a normal CProcessElement class. It calls
//  CProcessChannel to process the *same element*
CProcessRoot::CProcessRoot(CChannelAgent *pParent, IXMLElement *pItem) : 
        CProcessElement(pParent, NULL, pItem)
{
    ASSERT(m_pDefaultStartItem == FALSE && m_pTracking == NULL && !m_dwCurSizeKB);

    m_pRoot = this;
    m_pChannelAgent = pParent; pParent->AddRef();
    m_iTotalStarted = 1;
}

CProcessRoot::~CProcessRoot()
{
    SAFEDELETE(m_pTracking);
    SAFERELEASE(m_pChannelAgent);
    SAFERELEASE(m_pDefaultStartItem);
}

// Should never get called. CProcessRoot is an odd duck.
HRESULT CProcessRoot::ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT CProcessRoot::CreateStartItem(ISubscriptionItem **ppItem)
{
    if (ppItem)
        *ppItem = NULL;

    if (!m_pDefaultStartItem)
    {
        DoCloneSubscriptionItem(m_pChannelAgent->GetStartItem(), NULL, &m_pDefaultStartItem);

        if (m_pDefaultStartItem)
        {
            DWORD   dwTemp;

            // Clear out properties we don't want
            const LPCWSTR pwszPropsToClear[] =
            {
                c_szPropCrawlLevels,
                c_szPropCrawlLocalDest,
                c_szPropCrawlActualSize,
                c_szPropCrawlMaxSize,
                c_szPropCrawlGroupID
            };

            VARIANT varEmpty[ARRAYSIZE(pwszPropsToClear)] = {0};

            ASSERT(ARRAYSIZE(pwszPropsToClear) == ARRAYSIZE(varEmpty));
            m_pDefaultStartItem->WriteProperties(
                ARRAYSIZE(pwszPropsToClear), pwszPropsToClear, varEmpty);

            // Add in properties we do want
            dwTemp = DELIVERY_AGENT_FLAG_NO_BROADCAST |
                     DELIVERY_AGENT_FLAG_NO_RESTRICTIONS;
            WriteDWORD(m_pDefaultStartItem, c_szPropAgentFlags, dwTemp);
            if (FAILED(ReadDWORD(m_pDefaultStartItem, c_szPropCrawlFlags, &dwTemp)))
            {
                WriteDWORD(m_pDefaultStartItem, c_szPropCrawlFlags,
                    WEBCRAWL_GET_IMAGES|WEBCRAWL_LINKS_ELSEWHERE);
            }

            WriteLONGLONG(m_pDefaultStartItem, c_szPropCrawlNewGroupID, m_pChannelAgent->m_llCacheGroupID);
        }
    }

    if (m_pDefaultStartItem && ppItem)
    {
        DoCloneSubscriptionItem(m_pDefaultStartItem, NULL, ppItem);

        if (*ppItem)
        {
            // Add in properties for our new clone
            if ((m_pChannelAgent->m_dwMaxSizeKB > 0) &&
                (m_dwCurSizeKB <= m_pChannelAgent->m_dwMaxSizeKB))

            {
                WriteDWORD(*ppItem, c_szPropCrawlMaxSize, 
                    (m_pChannelAgent->m_dwMaxSizeKB - m_dwCurSizeKB));
            }
        }
    }

    return (ppItem) ? (*ppItem) ? S_OK : E_FAIL :
                      (m_pDefaultStartItem) ? S_OK : E_FAIL;
}

HRESULT CProcessRoot::Run()
{
    if (FAILED(CreateStartItem(NULL)))
        return E_FAIL;

    return DoChild(new CProcessChannel(this, this, m_pElement));
}

HRESULT CProcessRoot::DoTrackingFromItem(IXMLElement *pItem, LPCWSTR pwszUrl, BOOL fForceLog)
{
    HRESULT hr = E_FAIL;

    // if m_pTracking is not created before this call, means no <LogTarget> tag was found or
    // global logging is turned off
    if (m_pTracking)
        hr = m_pTracking->ProcessTrackingInItem(pItem, pwszUrl, fForceLog);
        
    return hr;
}

HRESULT CProcessRoot::DoTrackingFromLog(IXMLElement *pItem)
{
    HRESULT hr = S_OK;

    if (!m_pTracking 
        && !SHRestricted2W(REST_NoChannelLogging, m_pChannelAgent->GetUrl(), 0)
        && !ReadRegDWORD(HKEY_CURRENT_USER, c_szRegKey, c_szNoChannelLogging))
    {
        m_pTracking = new CUrlTrackingCache(m_pChannelAgent->GetStartItem(), m_pChannelAgent->GetUrl());
    }
    
    if (!m_pTracking)
        return E_OUTOFMEMORY;

    hr = m_pTracking->ProcessTrackingInLog(pItem);

    // skip tracking if PostURL is not specified
    if (m_pTracking->get_PostURL() == NULL)
    {
        SAFEDELETE(m_pTracking);
    }

    return hr;
}

// Overload this since we never do enumeration. Call delivery agent if necessary,
//  call m_pParent->OnChildDone if necessary
HRESULT CProcessRoot::OnChildDone(CProcessElement *pChild, HRESULT hr)
{
    ASSERT(pChild && (!m_pCurChild || (pChild == m_pCurChild)));

    // Our processing is done. Now we decide if we'd like to call the post agent.
    BSTR bstrURL=NULL;
    ISubscriptionItem *pStartItem;

    hr = S_OK;

    SAFEDELETE(m_pCurChild);

    ASSERT(m_pDefaultStartItem);
    ReadBSTR(m_pDefaultStartItem, c_szTrackingPostURL, &bstrURL);

    if (bstrURL && *bstrURL)
    {
        TraceMsg(TF_THISMODULE, "ChannelAgent calling post agent posturl=%ws", bstrURL);
        if (SUCCEEDED(m_pRoot->CreateStartItem(&pStartItem)))
        {
            m_pRunAgent = new CChannelAgentHolder(m_pChannelAgent, this);

            if (m_pRunAgent)
            {
                hr = m_pRunAgent->Init(this, pStartItem, CLSID_PostAgent);
                if (SUCCEEDED(hr))
                    hr = m_pRunAgent->StartAgent();
                if (hr != E_PENDING)
                    CRunDeliveryAgent::SafeRelease(m_pRunAgent);
            }
            pStartItem->Release();
        }
    }

    SysFreeString(bstrURL);

    if (hr != E_PENDING)
        m_pParent->OnChildDone(this, hr); // This may delete us

    return hr;
}

// Our delivery agent (post agent) is done running. Tell CDF agent we're done.
HRESULT CProcessRoot::OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                               long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                               BOOL fSynchronous)
{
    if (!fSynchronous)
        m_pParent->OnChildDone(this, S_OK); // This may delete us

    return S_OK;
}

CProcessChannel::CProcessChannel(CProcessElementSink *pParent,
                                 CProcessRoot *pRoot,
                                 IXMLElement *pItem) :
        CProcessElement(pParent, pRoot, pItem)
{
    m_fglobalLog = FALSE;
}

CProcessChannel::~CProcessChannel()
{
    SAFEFREEBSTR(m_bstrBaseUrl);
}

HRESULT CProcessChannel::CheckPreCache()
{
    BOOL fPreCache;

    if (SUCCEEDED(GetXMLBoolAttribute(m_pElement, L"PreCache", &fPreCache)))
    {
        if (fPreCache)
            return S_OK;
        
        return S_FALSE;
    }

    return S_OK;
}

HRESULT CProcessChannel::Run()
{
    // Process Channel attributes, then any sub elements
    if (0 == m_lIndex)
    {
        m_lIndex ++;

        BSTR bstrURL=NULL;
        LPWSTR pwszUrl=NULL;
        HRESULT hr = S_OK;

        ASSERT(!m_bstrBaseUrl);

        // Get base URL if specified
        GetXMLStringAttribute(m_pElement, L"BASE", &m_bstrBaseUrl);

        if (SUCCEEDED(GetXMLStringAttribute(m_pElement, L"HREF", &bstrURL)) && bstrURL)
            CombineWithBaseUrl(bstrURL, &pwszUrl);

        if (pwszUrl && (m_pRoot==m_pParent))
        {
            // Use this as default "email url"
            WriteOLESTR(m_pRoot->m_pChannelAgent->GetStartItem(), c_szPropEmailURL, pwszUrl);
        }

        if (pwszUrl && m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SOME) &&
            (S_OK == CheckPreCache()))
        {
            if (E_PENDING == DoWebCrawl(m_pElement, pwszUrl))
            {
                m_fDownloadedHREF = TRUE;
                hr = E_PENDING;
            }

        }

        // If no URL for this <Channel> log, check if global log exists       
        if (SUCCEEDED(m_pRoot->DoTrackingFromItem(m_pElement, pwszUrl, m_pParent->IsGlobalLog())))
        {
            SetGlobalLogFlag(TRUE);
        }

        SAFELOCALFREE(pwszUrl);
        SAFEFREEBSTR(bstrURL);

        if (hr == E_PENDING)
            return hr;
    }

    // We've processed attributes. Run sub-elements.

    return CProcessElement::Run();
}

HRESULT CProcessChannel::ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem)
{
    HRESULT hr;
    BSTR    bstrTemp;

    if (!StrCmpIW(pwszTagName, L"Logo"))
    {
        if (ShouldDownloadLogo(pItem))
            return DoWebCrawl(pItem);
        else
            return S_OK;
    }
    else if (!StrCmpIW(pwszTagName, L"Item"))
    {
        return DoChild(new CProcessItem(this, m_pRoot, pItem));
    }
    else if (!StrCmpIW(pwszTagName, L"Channel"))
    {
        return DoChild(new CProcessChannel(this, m_pRoot, pItem));
    }
/*
    else if (!StrCmpIW(pwszTagName, L"Login"))
    {
        // No sub-elements to process. Do it here.
        return m_pRoot->ProcessLogin(pItem);
    }
*/
    else if (!StrCmpIW(pwszTagName, L"LOGTARGET"))
    {
        return m_pRoot->DoTrackingFromLog(pItem);
    }
    else if (!StrCmpIW(pwszTagName, L"Schedule"))
    {
        if (m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_DYNAMIC_SCHEDULE))
            return DoChild(new CProcessSchedule(this, m_pRoot, pItem));
        else
            return S_OK;
    }
    else if (!StrCmpIW(pwszTagName, L"SoftPkg"))
    {
        return DoSoftDist(pItem);
    }
    else if (!StrCmpIW(pwszTagName, L"A"))
    {
        // Process Anchor tag
        if (!m_fDownloadedHREF
            && (m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SOME) || (m_pRoot==m_pParent))
            && SUCCEEDED(GetXMLStringAttribute(pItem, L"HREF", &bstrTemp))
            && bstrTemp)
        {
            LPWSTR pwszUrl=NULL;

            hr = S_OK;

            CombineWithBaseUrl(bstrTemp, &pwszUrl); // not really necessary (a href)

            if (pwszUrl)
            {
                // Use this as default "email url"
                if (m_pRoot == m_pParent)
                    WriteOLESTR(m_pRoot->m_pChannelAgent->GetStartItem(), c_szPropEmailURL, pwszUrl);

                if (m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SOME) &&
                    (S_OK == CheckPreCache()))
                {
                    hr = DoWebCrawl(m_pElement, pwszUrl);

                    if (E_PENDING == hr)
                        m_fDownloadedHREF = TRUE;

                    // Process tracking for this item
                    if (SUCCEEDED(m_pRoot->DoTrackingFromItem(m_pElement, pwszUrl, m_pParent->IsGlobalLog())))
                        SetGlobalLogFlag(TRUE);
                }
            }

            SAFELOCALFREE(pwszUrl);

            SysFreeString(bstrTemp);
            return hr;
        }

        return S_OK;
    }

    return S_OK;
}

CProcessItem::CProcessItem(CProcessElementSink *pParent,
                                 CProcessRoot *pRoot,
                                 IXMLElement *pItem) :
        CProcessElement(pParent, pRoot, pItem)
{
}

CProcessItem::~CProcessItem()
{
    SAFEFREEBSTR(m_bstrAnchorURL);
}

HRESULT CProcessItem::ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem)
{
    if (!StrCmpIW(pwszTagName, L"Logo"))
    {
        if (ShouldDownloadLogo(pItem))
            return DoWebCrawl(pItem);
        else
            return S_OK;
    }
    else if (!StrCmpIW(pwszTagName, L"Usage"))
    {
        // Usage tag found.
        BSTR    bstrValue;

        if (SUCCEEDED(GetXMLStringAttribute(pItem, L"Value", &bstrValue)))
        {
            if (!m_fScreenSaver &&
                (!StrCmpIW(bstrValue, L"ScreenSaver") ||
                 !StrCmpIW(bstrValue, L"SmartScreen"))) // PCN compat only
            {
                m_fScreenSaver = TRUE;
            }
            
            if (!m_fDesktop &&
                !StrCmpIW(bstrValue, L"DesktopComponent"))
            {
                m_fDesktop = TRUE;
            }

            if (!m_fEmail &&
                !StrCmpIW(bstrValue, L"Email"))
            {
                m_fEmail = TRUE;
            }

            SysFreeString(bstrValue);
        }
    }
    else if (!StrCmpIW(pwszTagName, L"A"))
    {
        // Anchor tag found; save URL
        if (!m_bstrAnchorURL)
            GetXMLStringAttribute(pItem, L"HREF", &m_bstrAnchorURL);
    }

    return S_OK;
}

HRESULT CProcessItem::EnumerationComplete()
{
    BOOL fPreCache, fPreCacheValid=FALSE;
    BOOL fDoDownload=FALSE;
    BSTR bstrShow=NULL;
    BSTR bstrURL=NULL;
    HRESULT hr = S_OK;
    LPWSTR pwszUrl=NULL;

    // PCN Compat only - not in spec
    if (SUCCEEDED(GetXMLStringAttribute(m_pElement, L"Show", &bstrShow)) && bstrShow)
    {
        if (!StrCmpIW(bstrShow, L"SmartScreen") &&
            !StrCmpIW(bstrShow, L"ScreenSaver"))
        {
            m_fScreenSaver = TRUE;
        }
        SysFreeString(bstrShow); 
        bstrShow=NULL;
    }
    // End PCN compat

    if (SUCCEEDED(GetXMLBoolAttribute(m_pElement, L"PreCache", &fPreCache)))
    {
        fPreCacheValid = TRUE;
    }

    // Get the URL from our attribute, or from Anchor tag if not available
    if (FAILED(GetXMLStringAttribute(m_pElement, L"HREF", &bstrURL)) || !bstrURL)
    {
        bstrURL = m_bstrAnchorURL;
        m_bstrAnchorURL = NULL;
    }

    // Get the combined URL
    if (bstrURL)
        CombineWithBaseUrl(bstrURL, &pwszUrl);

    if (pwszUrl)
    {
        // Process tracking for this item
        m_pRoot->DoTrackingFromItem(m_pElement, pwszUrl, IsGlobalLog());

        // Find if we should use this url for the Email agent
        if (m_fEmail)
        {
            // Yes, put this url in the end report
            DBG("Using custom email url");
            WriteOLESTR(m_pRoot->m_pChannelAgent->GetStartItem(), c_szPropEmailURL, pwszUrl);
        }

        if (m_fScreenSaver)
        {
            m_pRoot->m_pChannelAgent->SetScreenSaverURL(pwszUrl);
        }

        // Figure out if we should download our "href" based on Usage and Precache tag
        if (fPreCacheValid)
        {
            if (fPreCache)
            {
                if (m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SOME))
                    fDoDownload = TRUE;
                else if (m_fScreenSaver && m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SCRNSAVER))
                    fDoDownload = TRUE;
            }
        }
        else
        {
            if (m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_ALL))
                fDoDownload = TRUE;
            else if (m_fScreenSaver && m_pRoot->IsChannelFlagSet(CHANNEL_AGENT_PRECACHE_SCRNSAVER))
                fDoDownload = TRUE;
        }

        // if (m_fDesktop)
        // Do something for desktop components

        if (fDoDownload && pwszUrl)
            hr = DoWebCrawl(m_pElement, pwszUrl);
    } // pwszUrl

    SAFEFREEBSTR(bstrURL);
    SAFELOCALFREE(pwszUrl);

    return hr;
}

CProcessSchedule::CProcessSchedule(CProcessElementSink *pParent,
                                 CProcessRoot *pRoot,
                                 IXMLElement *pItem) :
        CProcessElement(pParent, pRoot, pItem)
{
}

HRESULT CProcessSchedule::Run()
{
    // Get attributes (Start and End date) first
    BSTR    bstr=NULL;
    long    lTimeZone;

    if (FAILED(GetXMLStringAttribute(m_pElement, L"StartDate", &bstr)) ||
        !ISODateToSystemTime(bstr, &m_stStartDate, &lTimeZone))
    {
        GetLocalTime(&m_stStartDate);
    }
    SAFEFREEBSTR(bstr);

    if (FAILED(GetXMLStringAttribute(m_pElement, L"StopDate", &bstr)) ||
        !ISODateToSystemTime(bstr, &m_stEndDate, &lTimeZone))
    {
        ZeroMemory(&m_stEndDate, sizeof(SYSTEMTIME));
    }
    SAFEFREEBSTR(bstr);

    return CProcessElement::Run();
}

HRESULT CProcessSchedule::ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem)
{
    if (!StrCmpIW(pwszTagName, L"IntervalTime"))
    {
        GetXMLTimeAttributes(pItem, &m_timeInterval);
    }
    else if (!StrCmpIW(pwszTagName, L"EarliestTime"))
    {
        GetXMLTimeAttributes(pItem, &m_timeEarliest);
    }
    else if (!StrCmpIW(pwszTagName, L"LatestTime"))
    {
        GetXMLTimeAttributes(pItem, &m_timeLatest);
    }

    return S_OK;
}

HRESULT CProcessSchedule::EnumerationComplete()
{
    DBG("CProcessSchedule::EnumerationComplete");

    int iZone;

    if (FAILED(GetXMLTimeZoneAttribute(m_pElement, L"TimeZone", &iZone)))
        iZone = 9999;

    m_tt.cbTriggerSize = sizeof(m_tt);

    // m_pRoot is null for XMLElementToTaskTrigger call
    // Always run ScheduleToTaskTrigger
    if (SUCCEEDED(ScheduleToTaskTrigger(&m_tt, &m_stStartDate, &m_stEndDate,
            (long) m_timeInterval.dwConvertedMinutes,
            (long) m_timeEarliest.dwConvertedMinutes,
            (long) m_timeLatest.dwConvertedMinutes,
            iZone))
        && m_pRoot)
    {
        SUBSCRIPTIONITEMINFO sii = { sizeof(SUBSCRIPTIONITEMINFO) };
        if (SUCCEEDED(m_pRoot->m_pChannelAgent->GetStartItem()->GetSubscriptionItemInfo(&sii)))
        {
            if (sii.ScheduleGroup != GUID_NULL)
            {
                if (FAILED(UpdateScheduleTrigger(&sii.ScheduleGroup, &m_tt)))
                {
                    DBG_WARN("Failed to update trigger in publisher's recommended schedule.");
                }
            }
            else
                DBG_WARN("No publisher's recommended schedule in sii");
        }
    }

    return S_OK;
}

HRESULT ScheduleToTaskTrigger(TASK_TRIGGER *ptt, SYSTEMTIME *pstStartDate, SYSTEMTIME *pstEndDate,
                              long lInterval, long lEarliest, long lLatest, int iZone/*=9999*/)
{
    // Convert our schedule info to a TASK_TRIGGER struct

    ASSERT(pstStartDate);
    
    int iZoneCorrectionMinutes=0;
    TIME_ZONE_INFORMATION tzi;
    long lRandom;
    
    if ((lInterval == 0) ||
        (lInterval > 366 * MINUTES_PER_DAY))
    {
        DBG_WARN("ScheduleToTaskTrigger: Invalid IntervalTime - failing");
        return E_INVALIDARG;
    }

    if (ptt->cbTriggerSize < sizeof(TASK_TRIGGER))
    {
        DBG_WARN("ScheduleToTaskTrigger: ptt->cbTriggerSize not initialized");
        ASSERT(!"ScheduleToTaskTrigger");
        return E_INVALIDARG;
    }

    // Fix any invalid stuff
    if (lInterval < MINUTES_PER_DAY)
    {
        // ROUND so that dwIntervalMinutes is an even divisor of one day
        lInterval = MINUTES_PER_DAY / (MINUTES_PER_DAY / lInterval);
    }
    else
    {
        // ROUND to nearest day
        lInterval = MINUTES_PER_DAY * ((lInterval + 12*60)/MINUTES_PER_DAY);
    }
    if (lEarliest >= lInterval)
    {
        DBG("Invalid EarliestTime specified. Fixing."); // Earliest >= Interval!
        lEarliest = lInterval-1;
    }
    if (lLatest < lEarliest)
    {
        DBG("Invalid LatestTime specified. Fixing."); // Latest < Earliest!
        lLatest = lEarliest;
    }
    if (lLatest-lEarliest > lInterval)
    {
        DBG("Invalid LatestTime specified. Fixing.");   // Latest > Interval!
        lLatest = lEarliest+lInterval;
    }

    lRandom = lLatest - lEarliest;
    ASSERT(lRandom>=0 && lRandom<=lInterval);

    if (iZone != 9999)
    {
        int iCorrection;
        iCorrection = (60 * (iZone/100)) + (iZone % 100);

        if (iCorrection < -12*60 || iCorrection > 12*60)
        {
            DBG("ScheduleElementToTaskTrigger: Invalid timezone; ignoring");
        }
        else
        {
            if (TIME_ZONE_ID_INVALID != GetTimeZoneInformation(&tzi))
            {
                // tzi.bias has correction from client timezone to UTC (+8 for US west coast)
                // iCorrection has correction from UTC to server time zone (-5 for US east coast)
                // result is correction from server to client time zone (-3 for east to west coast)
                iZoneCorrectionMinutes = - (iCorrection + tzi.Bias + tzi.StandardBias);
                TraceMsg(TF_THISMODULE, "ServerTimeZone = %d, LocalBias = %d min, RelativeCorrection = %d min", iZone, tzi.Bias+tzi.StandardBias, iZoneCorrectionMinutes);
            }
            else
            {
                DBG_WARN("Unable to get local time zone. Not correcting for time zone.");
            }
        }
    }

    TraceMsg(TF_THISMODULE, "StartDate = %d/%d/%d StopDate = %d/%d/%d", (int)(pstStartDate->wMonth),(int)(pstStartDate->wDay),(int)(pstStartDate->wYear),(int)(pstEndDate->wMonth),(int)(pstEndDate->wDay),(int)(pstEndDate->wYear));
    TraceMsg(TF_THISMODULE, "IntervalTime = %6d minutes", (int)lInterval);
    TraceMsg(TF_THISMODULE, "EarliestTime = %6d minutes", (int)lEarliest);
    TraceMsg(TF_THISMODULE, "LatestTime   = %6d minutes", (int)lLatest);
    TraceMsg(TF_THISMODULE, "RandomTime   = %6d minutes", (int)lRandom);

    if (iZoneCorrectionMinutes != 0)
    {
        if (lInterval % 60)
        {
            DBG("Not correcting for time zone ; interval not multiple of 1 hour");
        }
        else
        {
            // Correct Earliest time for time zone
            lEarliest += (iZoneCorrectionMinutes % lInterval);

            if (lEarliest < 0)
                lEarliest += lInterval;

            TraceMsg(TF_THISMODULE, "EarliestTime = %6d minutes (after timezone)", (int)lEarliest);
        }
    }

    ZeroMemory(ptt, sizeof(*ptt));
    ptt->cbTriggerSize = sizeof(*ptt);
    ptt->wBeginYear = pstStartDate->wYear;
    ptt->wBeginMonth = pstStartDate->wMonth;
    ptt->wBeginDay = pstStartDate->wDay;
    if (pstEndDate && pstEndDate->wYear)
    {
        ptt->rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
        ptt->wEndYear = pstEndDate->wYear;
        ptt->wEndMonth = pstEndDate->wMonth;
        ptt->wEndDay = pstEndDate->wDay;
    }

    // Set up Random period ; difference between Latesttime and Earliesttime
    ptt->wRandomMinutesInterval = (WORD) lRandom;

    ptt->wStartHour = (WORD) (lEarliest / 60);
    ptt->wStartMinute = (WORD) (lEarliest % 60);

    // Set up according to IntervalTime
    if (lInterval < MINUTES_PER_DAY)
    {
        // Less than one day (1/2 day, 1/3 day, 1/4 day, etc)
        ptt->MinutesDuration = MINUTES_PER_DAY - lEarliest;
        ptt->MinutesInterval = lInterval;
        ptt->TriggerType = TASK_TIME_TRIGGER_DAILY;
        ptt->Type.Daily.DaysInterval = 1;
    }
    else
    {
        // Greater than or equal to one day.
        DWORD dwIntervalDays = lInterval / MINUTES_PER_DAY;

        TraceMsg(TF_THISMODULE, "Using %d day interval", dwIntervalDays);

        ptt->TriggerType = TASK_TIME_TRIGGER_DAILY;
        ptt->Type.Daily.DaysInterval = (WORD) dwIntervalDays;
    }

    return S_OK;
}


//==============================================================================
// CRunDeliveryAgent provides generic support for synchronous operation of a
//   delivery agent
// It is aggregatable so that you can add more interfaces to the callback
//==============================================================================
CRunDeliveryAgent::CRunDeliveryAgent()
{
    m_cRef = 1;
}

HRESULT CRunDeliveryAgent::Init(CRunDeliveryAgentSink *pParent,
                                ISubscriptionItem *pItem,
                                REFCLSID rclsidDest)
{
    ASSERT(pParent && pItem);

    if (m_pParent || m_pItem)
        return E_FAIL;  // already initialized. can't reuse an instance.

    if (!pParent || !pItem)
        return E_FAIL;

    m_pParent = pParent;
    m_clsidDest = rclsidDest;

    m_pItem = pItem;
    pItem->AddRef();

    return S_OK;
}

CRunDeliveryAgent::~CRunDeliveryAgent()
{
    CleanUp();
}

//
// IUnknown members
//
STDMETHODIMP_(ULONG) CRunDeliveryAgent::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CRunDeliveryAgent::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CRunDeliveryAgent::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_ISubscriptionAgentEvents == riid))
    {
        *ppv=(ISubscriptionAgentEvents *)this;
    }
    else
        return E_NOINTERFACE;

    // Addref through the interface
    ((LPUNKNOWN)*ppv)->AddRef();

    return S_OK;
}

//
// ISubscriptionAgentEvents members
//
STDMETHODIMP CRunDeliveryAgent::UpdateBegin(const SUBSCRIPTIONCOOKIE *)
{
    return S_OK;
}

STDMETHODIMP CRunDeliveryAgent::UpdateProgress(
                const SUBSCRIPTIONCOOKIE *,
                long lSizeDownloaded,
                long lProgressCurrent,
                long lProgressMax,
                HRESULT hrStatus,
                LPCWSTR wszStatus)
{
    if (m_pParent)
        m_pParent->OnAgentProgress();
    return S_OK;
}

STDMETHODIMP CRunDeliveryAgent::UpdateEnd(const SUBSCRIPTIONCOOKIE *pCookie,
                long    lSizeDownloaded,
                HRESULT hrResult,
                LPCWSTR wszResult)
{
    ASSERT((hrResult != INET_S_AGENT_BASIC_SUCCESS) && (hrResult != E_PENDING));

    m_hrResult = hrResult;
    if (hrResult == INET_S_AGENT_BASIC_SUCCESS || hrResult == E_PENDING)
    {
        // Shouldn't happen; let's be robust anyway.
        m_hrResult = S_OK;
    }

    if (m_pParent)
    {
        m_pParent->OnAgentEnd(pCookie, lSizeDownloaded, hrResult, wszResult, m_fInStartAgent);
    }

    CleanUp();

    return S_OK;
}

STDMETHODIMP CRunDeliveryAgent::ReportError(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        HRESULT hrError, 
        LPCWSTR wszError)
{
    return S_FALSE;
}

HRESULT CRunDeliveryAgent::StartAgent()
{
    HRESULT hr;

    if (!m_pParent || !m_pItem || m_pAgent)
        return E_FAIL;

    AddRef();   // Release before we return from this function
    m_fInStartAgent = TRUE;

    m_hrResult = INET_S_AGENT_BASIC_SUCCESS;

    DBG("Using new interfaces to host agent");

    ASSERT(!m_pAgent);

    hr = CoCreateInstance(m_clsidDest, NULL, CLSCTX_INPROC_SERVER,
                          IID_ISubscriptionAgentControl, (void **)&m_pAgent);

    if (m_pAgent)
    {
        hr = m_pAgent->StartUpdate(m_pItem, (ISubscriptionAgentEvents *)this);
    }

    hr = m_hrResult;

    m_fInStartAgent = FALSE;
    Release();

    if (hr != INET_S_AGENT_BASIC_SUCCESS)
    {
        return hr;
    }

    return E_PENDING;
};

HRESULT CRunDeliveryAgent::AgentPause(DWORD dwFlags)
{
    if (m_pAgent)
        return m_pAgent->PauseUpdate(0);

    DBG_WARN("CRunDeliveryAgent::AgentPause with no running agent!!");
    return S_FALSE;
}

HRESULT CRunDeliveryAgent::AgentResume(DWORD dwFlags)
{
    if (m_pAgent)
        return m_pAgent->ResumeUpdate(0);

    DBG_WARN("CRunDeliveryAgent::AgentResume with no running agent!!");

    return E_FAIL;
}

HRESULT CRunDeliveryAgent::AgentAbort(DWORD dwFlags)
{
    if (m_pAgent)
        return m_pAgent->AbortUpdate(0);

    DBG_WARN("CRunDeliveryAgent::AgentAbort with no running agent!!");
    return S_FALSE;
}

void CRunDeliveryAgent::CleanUp()
{
    SAFERELEASE(m_pItem);
    SAFERELEASE(m_pAgent);
    m_pParent = NULL;
}

// static
#if 0       // unused by us, but do not remove
HRESULT CRunDeliveryAgent::CreateNewItem(ISubscriptionItem **ppItem, REFCLSID rclsidAgent)
{
    ISubscriptionMgrPriv *pSubsMgrPriv=NULL;
    SUBSCRIPTIONITEMINFO info;

    *ppItem = NULL;

    CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
        IID_ISubscriptionMgrPriv, (void**)&pSubsMgrPriv);

    if (pSubsMgrPriv)
    {
        SUBSCRIPTIONCOOKIE cookie;

        info.cbSize = sizeof(info);
        info.dwFlags = SI_TEMPORARY;
        info.dwPriority = 0;
        info.ScheduleGroup = GUID_NULL;
        info.clsidAgent = rclsidAgent;

        pSubsMgrPriv->CreateSubscriptionItem(&info, &cookie, ppItem);

        pSubsMgrPriv->Release();
    }

    return (*ppItem) ? S_OK : E_FAIL;
}
#endif

//////////////////////////////////////////////////////////////////////////
//
// CChannelAgentHolder, derives from CRunDeliveryAgent
//
//////////////////////////////////////////////////////////////////////////
CChannelAgentHolder::CChannelAgentHolder(CChannelAgent *pChannelAgent, CProcessElement *pProcess)
{
    m_pChannelAgent = pChannelAgent;
    m_pProcess = pProcess;
}

CChannelAgentHolder::~CChannelAgentHolder()
{
}

// Won't compile unless we have addref & release here.
STDMETHODIMP_(ULONG) CChannelAgentHolder::AddRef(void)
{
    return CRunDeliveryAgent::AddRef();
}

STDMETHODIMP_(ULONG) CChannelAgentHolder::Release(void)
{
    return CRunDeliveryAgent::Release();
}

STDMETHODIMP CChannelAgentHolder::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    if (IID_IServiceProvider == riid)
    {
        *ppv = (IServiceProvider *)this;
    }   
    else
        return CRunDeliveryAgent::QueryInterface(riid, ppv);

    // Addref through the interface
    ((LPUNKNOWN)*ppv)->AddRef();

    return S_OK;
}

// IQueryService
// CLSID_ChannelAgent   IID_ISubscriptionItem       channel agent start item
// CLSID_XMLDocument    IID_IXMLElement             current element
STDMETHODIMP CChannelAgentHolder::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    ASSERT(ppvObject);
    if (!ppvObject)
        return E_INVALIDARG;
    
    if (!m_pChannelAgent || !m_pProcess || !m_pParent)
        return E_FAIL;

    *ppvObject = NULL;

    if (guidService == CLSID_ChannelAgent)
    {
        if (riid == IID_ISubscriptionItem)
        {
            *ppvObject = m_pChannelAgent->GetStartItem();
        }
//      if (riid == IID_IXMLElement)    Root XML document?
    }
    else if (guidService == CLSID_XMLDocument)
    {
        if (riid == IID_IXMLElement)
        {
            *ppvObject = m_pProcess->GetCurrentElement();
        }
    }

    if (*ppvObject)
    {
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }

    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////
//
// CChannelAgent implementation
//
//////////////////////////////////////////////////////////////////////////

CChannelAgent::CChannelAgent()
{
    DBG("Creating CChannelAgent object");

    // Initialize object
    // Many vars are initialized in StartOperation
    m_pwszURL = NULL;
    m_pCurDownload = NULL;
    m_pProcess = NULL;
    m_fHasInitCookie = FALSE;
    m_pChannelIconHelper = NULL;
}

CChannelAgent::~CChannelAgent()
{
//  DBG("Destroying CChannelAgent object");

    if (m_pwszURL)
        CoTaskMemFree(m_pwszURL);
    
    SAFELOCALFREE (m_pBuf);

    ASSERT(!m_pProcess);

    SAFERELEASE(m_pChannelIconHelper);

    SAFELOCALFREE(m_pwszScreenSaverURL);

    DBG("Destroyed CChannelAgent object");
}

void CChannelAgent::CleanUp()
{
    if (m_pCurDownload)
    {
        m_pCurDownload->LeaveMeAlone();     // no more calls from them
        m_pCurDownload->DoneDownloading();
        m_pCurDownload->Release();
        m_pCurDownload = NULL;
    }
    SAFEFREEOLESTR(m_pwszURL);
    SAFEDELETE(m_pProcess);
    SAFELOCALFREE(m_pBuf);

    CDeliveryAgent::CleanUp();
}

void CChannelAgent::SetScreenSaverURL(LPCWSTR pwszURL)
{
    //  We only take the first one
    if (NULL == m_pwszScreenSaverURL)
    {
        m_pwszScreenSaverURL = StrDupW(pwszURL);
    }
}


HRESULT CChannelAgent::StartOperation()
{
    DBG("Channel Agent in StartOperation");
    
    DWORD dwTemp;

    SAFEFREEOLESTR(m_pwszURL);
    if (FAILED(ReadOLESTR(m_pSubscriptionItem, c_szPropURL, &m_pwszURL)) ||
        !CUrlDownload::IsValidURL(m_pwszURL))
    {
        DBG_WARN("Couldn't get valid URL, aborting");
        SetEndStatus(E_INVALIDARG);
        SendUpdateNone();
        return E_INVALIDARG;
    }

    if (FAILED(ReadDWORD(m_pSubscriptionItem, c_szPropChannelFlags, &m_dwChannelFlags)))
        m_dwChannelFlags = 0;
    
    // If we download all, we also download some. Makes assumptions easier.
    if (m_dwChannelFlags & CHANNEL_AGENT_PRECACHE_ALL)
        m_dwChannelFlags |= CHANNEL_AGENT_PRECACHE_SOME;

    // BUGBUG: We may want REST_NoChannelContent to be similar to the webcrawl version.
    // Probably not though because the headlines are useful in the UI.
    if (SHRestricted2W(REST_NoChannelContent, NULL, 0))
        ClearFlag(m_dwChannelFlags, CHANNEL_AGENT_PRECACHE_ALL | CHANNEL_AGENT_PRECACHE_SOME);

    m_dwMaxSizeKB = SHRestricted2W(REST_MaxChannelSize, NULL, 0);
    if (SUCCEEDED(ReadDWORD(m_pSubscriptionItem, c_szPropCrawlMaxSize, &dwTemp))
        && dwTemp
        && (0 == m_dwMaxSizeKB || dwTemp < m_dwMaxSizeKB))
    {
        m_dwMaxSizeKB = dwTemp;
    }

    if (IsAgentFlagSet(FLAG_CHANGESONLY))
    {
        ClearFlag(m_dwChannelFlags, CHANNEL_AGENT_PRECACHE_ALL|
            CHANNEL_AGENT_PRECACHE_SOME|CHANNEL_AGENT_PRECACHE_SCRNSAVER);
        DBG("Channel agent is in 'changes only' mode.");
    }
    else
    {
        // Read old group ID
        ReadLONGLONG(m_pSubscriptionItem, c_szPropCrawlGroupID, &m_llOldCacheGroupID);

        // Read new ID if present
        m_llCacheGroupID = 0;
        ReadLONGLONG(m_pSubscriptionItem, c_szPropCrawlNewGroupID, &m_llCacheGroupID);
    }

    return CDeliveryAgent::StartOperation();
}

HRESULT CChannelAgent::StartDownload()
{
    ASSERT(!m_pCurDownload);
    TraceMsg(TF_THISMODULE, "Channel agent starting download of CDF: URL=%ws", m_pwszURL);

    m_pCurDownload = new CUrlDownload(this, 0);
    if (!m_pCurDownload)
        return E_OUTOFMEMORY;

    // Change detection
    m_varChange.vt = VT_EMPTY;
    if (IsAgentFlagSet(FLAG_CHANGESONLY))
    {
        // "Changes Only" mode, we have persisted a change detection code
        ReadVariant(m_pSubscriptionItem, c_szPropChangeCode, &m_varChange);
        m_llCacheGroupID = 0;
    }
    else
    {
        // Create new cache group
        if (!m_llCacheGroupID)
        {
            m_llCacheGroupID = CreateUrlCacheGroup(CACHEGROUP_FLAG_NONPURGEABLE, 0);

            ASSERT_MSG(m_llCacheGroupID != 0, "Create cache group failed");
        }
    }

    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];

    MyOleStrToStrN(szUrl, INTERNET_MAX_URL_LENGTH, m_pwszURL);
    PreCheckUrlForChange(szUrl, &m_varChange, NULL);

    SendUpdateProgress(m_pwszURL, 0, -1, 0);

    // Start download
    return m_pCurDownload->BeginDownloadURL2(
        m_pwszURL, BDU2_URLMON, BDU2_NEEDSTREAM, NULL, m_dwMaxSizeKB<<10);
}

HRESULT CChannelAgent::OnAuthenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword)
{
    HRESULT hr;
    ASSERT(phwnd && ppszUsername && ppszPassword);
    ASSERT((HWND)-1 == *phwnd && NULL == *ppszUsername && NULL == *ppszPassword);

    hr = ReadOLESTR(m_pSubscriptionItem, c_szPropCrawlUsername, ppszUsername);
    if (SUCCEEDED(hr))
    {
        BSTR bstrPassword = NULL;
        hr = ReadPassword(m_pSubscriptionItem, &bstrPassword);
        if (SUCCEEDED(hr))
        {
            int len = (lstrlenW(bstrPassword) + 1) * sizeof(WCHAR);
            *ppszPassword = (LPWSTR) CoTaskMemAlloc(len);
            if (*ppszPassword)
            {
                CopyMemory(*ppszPassword, bstrPassword, len);
            }
            SAFEFREEBSTR(bstrPassword);
            if (*ppszPassword)
            {
                return S_OK;
            }
        }
    }

    SAFEFREEOLESTR(*ppszUsername);
    SAFEFREEOLESTR(*ppszPassword);
    return E_FAIL;
}

HRESULT CChannelAgent::OnDownloadComplete(UINT iID, int iError)
{
    TraceMsg(TF_THISMODULE, "Channel Agent: OnDownloadComplete(%d)", iError);

    IStream *pStm = NULL;
    HRESULT hr;
    BOOL    fProcessed=FALSE;
    DWORD   dwCDFSizeKB=0, dwResponseCode;
    BSTR    bstrTmp;
    char    chBuf[MY_MAX_CACHE_ENTRY_INFO];
    DWORD   dwBufSize = sizeof(chBuf);

    LPINTERNET_CACHE_ENTRY_INFO lpInfo = (LPINTERNET_CACHE_ENTRY_INFO) chBuf;

    if (iError)
        hr = E_FAIL;
    else
    {
        hr = m_pCurDownload->GetResponseCode(&dwResponseCode);

        if (SUCCEEDED(hr))
        {
            hr = CheckResponseCode(dwResponseCode);
        }
        else
            DBG_WARN("CChannelAgent failed to GetResponseCode");
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pCurDownload->GetStream(&pStm);
        m_pCurDownload->ReleaseStream();
    }

    if (SUCCEEDED(hr))
    {
        TCHAR   szThisUrl[INTERNET_MAX_URL_LENGTH];
        LPWSTR  pwszThisUrl;

        m_pCurDownload->GetRealURL(&pwszThisUrl);

        if (pwszThisUrl)
        {
            MyOleStrToStrN(szThisUrl, INTERNET_MAX_URL_LENGTH, pwszThisUrl);

            LocalFree(pwszThisUrl);

            if (SUCCEEDED(GetUrlInfoAndMakeSticky(
                            NULL,
                            szThisUrl,
                            lpInfo,
                            dwBufSize,
                            m_llCacheGroupID)))
            {
                dwCDFSizeKB = (((LPINTERNET_CACHE_ENTRY_INFO)chBuf)->dwSizeLow+512) >> 10;
                TraceMsg(TF_THISMODULE, "CDF size %d kb", dwCDFSizeKB);

                hr = PostCheckUrlForChange(&m_varChange, lpInfo, lpInfo->LastModifiedTime);
                // If we FAILED, we mark it as changed.
                if (hr == S_OK || FAILED(hr))
                {
                    SetAgentFlag(FLAG_CDFCHANGED);
                    DBG("CDF has changed; will flag channel as changed");
                }

                // "Changes Only" mode, persist change detection code
                if (IsAgentFlagSet(FLAG_CHANGESONLY))
                {
                    WriteVariant(m_pSubscriptionItem, c_szPropChangeCode, &m_varChange);
                }

                hr = S_OK;
            }
        }
    }
    else
    {
        SetEndStatus(E_INVALIDARG);
    }

    // Get an object model on our Channel Description File
    if (SUCCEEDED(hr) && pStm)
    {
        IPersistStreamInit *pPersistStm=NULL;

        CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC,
                         IID_IPersistStreamInit, (void **)&pPersistStm);

        if (pPersistStm)
        {
            pPersistStm->InitNew();
            hr = pPersistStm->Load(pStm);
            if (SUCCEEDED(hr))
            {
                IXMLDocument *pDoc;

                hr = pPersistStm->QueryInterface(IID_IXMLDocument, (void **)&pDoc);
                if (SUCCEEDED(hr) && pDoc)
                {
                    IXMLElement *pRoot;
                    BSTR        bstrCharSet=NULL;

                    if (SUCCEEDED(pDoc->get_charset(&bstrCharSet)) && bstrCharSet)
                    {
                        WriteOLESTR(m_pSubscriptionItem, c_szPropCharSet, bstrCharSet);
                        TraceMsg(TF_THISMODULE, "Charset = \"%ws\"", bstrCharSet);
                        SysFreeString(bstrCharSet);
                    }
                    else
                        WriteEMPTY(m_pSubscriptionItem, c_szPropCharSet);

                    hr = pDoc->get_root(&pRoot);
                    if (SUCCEEDED(hr) && pRoot)
                    {
                        if (SUCCEEDED(pRoot->get_tagName(&bstrTmp)) && bstrTmp)
                        {
                            if (!StrCmpIW(bstrTmp, L"Channel"))
                            {
                                ASSERT(!m_pProcess);
                                m_pProcess = new CProcessRoot(this, pRoot);
                                if (m_pProcess)
                                {
                                    if (IsAgentFlagSet(FLAG_CDFCHANGED))
                                        SetEndStatus(S_OK);
                                    else
                                        SetEndStatus(S_FALSE);
                                     
                                    m_pProcess->m_dwCurSizeKB = dwCDFSizeKB;
                                    WriteEMPTY(m_pSubscriptionItem, c_szPropEmailURL);
            
                                    hr = m_pProcess->Run();     // This will get us cleaned up (now or later)
                                    fProcessed = TRUE;          // So we shouldn't do it ourselves
                                }
                            }
                            else
                                DBG_WARN("Valid XML but invalid CDF");

                            SAFEFREEBSTR(bstrTmp);
                        }
                        pRoot->Release();
                    }
                    pDoc->Release();
                }
            }
            pPersistStm->Release();
        }
    }

    if (!fProcessed || (FAILED(hr) && (hr != E_PENDING)))
    {
        if (INET_S_AGENT_BASIC_SUCCESS == GetEndStatus())
            SetEndStatus(E_FAIL);
        DBG_WARN("Failed to process CDF ; XML load failed?");
        CleanUp();      // CleanUp only if the process failed (otherwise OnChildDone does it)
    }

#ifdef DEBUG
    if (hr == E_PENDING)
        DBG("CChannelAgent::OnDownloadComplete not cleaning up, webcrawl pending");
#endif

    return S_OK;
}

HRESULT CChannelAgent::OnChildDone(CProcessElement *pChild, HRESULT hr)
{
    // Our CProcessRoot has reported that it's done. Clean up.
    DBG("CChannelAgent::OnChildDone cleaning up Channel delivery agent");

    if (m_llOldCacheGroupID)
    {
        DBG("Nuking old cache group.");
        if (!DeleteUrlCacheGroup(m_llOldCacheGroupID, 0, 0))
        {
            DBG_WARN("Failed to delete old cache group!");
        }
    }

    if (SUCCEEDED(GetEndStatus()))
    {
        IChannelMgrPriv2 *pChannelMgrPriv2;
        
        HRESULT hrTmp = CoCreateInstance(CLSID_ChannelMgr, 
                                         NULL, 
                                         CLSCTX_INPROC_SERVER, 
                                         IID_IChannelMgrPriv2, (void**)&pChannelMgrPriv2);
        if (SUCCEEDED(hrTmp))
        {
            ASSERT(NULL != pChannelMgrPriv2);

            hrTmp = pChannelMgrPriv2->WriteScreenSaverURL(m_pwszURL, m_pwszScreenSaverURL);

            pChannelMgrPriv2->Release();
        }
    }
    
    WriteLONGLONG(m_pSubscriptionItem, c_szPropCrawlGroupID, m_llCacheGroupID);

    // Add "total size" property
    m_lSizeDownloadedKB = (long) (m_pProcess->m_dwCurSizeKB);
    WriteDWORD(m_pSubscriptionItem, c_szPropCrawlActualSize, m_lSizeDownloadedKB);

    WriteDWORD(m_pSubscriptionItem, c_szPropActualProgressMax, m_pProcess->m_iTotalStarted);

    CleanUp();
    return S_OK;
}

HRESULT CChannelAgent::AgentPause(DWORD dwFlags)
{
    DBG("CChannelAgent::AgentPause");

    if (m_pProcess)
        m_pProcess->Pause(dwFlags);

    return CDeliveryAgent::AgentPause(dwFlags);
}

HRESULT CChannelAgent::AgentResume(DWORD dwFlags)
{
    DBG("CChannelAgent::AgentResume");

    if (m_pProcess)
        m_pProcess->Resume(dwFlags);

    return CDeliveryAgent::AgentResume(dwFlags);
}

// Forcibly abort current operation
HRESULT CChannelAgent::AgentAbort(DWORD dwFlags)
{
    DBG("CChannelAgent::AgentAbort");

    if (m_pCurDownload)
        m_pCurDownload->DoneDownloading();

    if (m_pProcess)
        m_pProcess->Abort(dwFlags);

    return CDeliveryAgent::AgentAbort(dwFlags);
}

HRESULT CChannelAgent::ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes)
{
    // Customize our end status string
    switch (GetEndStatus())
    {
        case INET_E_AGENT_MAX_SIZE_EXCEEDED :
                              *puiRes = IDS_AGNT_STATUS_SIZELIMIT; break;
        case INET_E_AGENT_CACHE_SIZE_EXCEEDED :
                              *puiRes = IDS_AGNT_STATUS_CACHELIMIT; break;
        case E_FAIL         : *puiRes = IDS_CRAWL_STATUS_NOT_OK; break;
        case S_OK           :
            if (!IsAgentFlagSet(FLAG_CHANGESONLY))
                *puiRes = IDS_CRAWL_STATUS_OK;
            else
                *puiRes = IDS_URL_STATUS_OK;
            break;
        case S_FALSE        :
            if (!IsAgentFlagSet(FLAG_CHANGESONLY))
                *puiRes = IDS_CRAWL_STATUS_UNCHANGED;
            else
                *puiRes = IDS_URL_STATUS_UNCHANGED;
            break;
        case INET_S_AGENT_PART_FAIL : *puiRes = IDS_CRAWL_STATUS_MOSTLYOK; break;
    }

    return CDeliveryAgent::ModifyUpdateEnd(pEndItem, puiRes);
}


const GUID  CLSID_CDFICONHANDLER =
{0xf3ba0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x35}};

extern HRESULT LoadWithCookie(LPCTSTR, POOEBuf, DWORD *, SUBSCRIPTIONCOOKIE *);

// IExtractIcon members
STDMETHODIMP CChannelAgent::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    DWORD   dwSize;
    IChannelMgrPriv*   pIChannelMgrPriv = NULL;
    HRESULT            hr = E_FAIL;
    TCHAR              szPath[MAX_PATH];

    if (!m_pBuf)    {
        m_pBuf = (POOEBuf)MemAlloc(LPTR, sizeof(OOEBuf));
        if (!m_pBuf)
            return E_OUTOFMEMORY;

        HRESULT hr = LoadWithCookie(NULL, m_pBuf, &dwSize, &m_SubscriptionCookie);
        RETURN_ON_FAILURE(hr);
    }

    hr = GetChannelPath(m_pBuf->m_URL, szPath, ARRAYSIZE(szPath), &pIChannelMgrPriv);

    if (SUCCEEDED(hr) && pIChannelMgrPriv)
    {
        IPersistFile* ppf = NULL;
        BOOL          bCoinit = FALSE;
        HRESULT       hr2 = E_FAIL;

        pIChannelMgrPriv->Release();

        hr = CoCreateInstance(CLSID_CDFICONHANDLER, NULL, CLSCTX_INPROC_SERVER,
                          IID_IPersistFile, (void**)&ppf);

        if ((hr == CO_E_NOTINITIALIZED || hr == REGDB_E_IIDNOTREG) &&
            SUCCEEDED(CoInitialize(NULL)))
        {
            bCoinit = TRUE;
            hr = CoCreateInstance(CLSID_CDFICONHANDLER, NULL, CLSCTX_INPROC_SERVER,
                          IID_IPersistFile, (void**)&ppf);
        }

        if (SUCCEEDED(hr))
        {
            
            hr = ppf->QueryInterface(IID_IExtractIcon, (void**)&m_pChannelIconHelper);

            WCHAR wszPath[MAX_PATH];
            MyStrToOleStrN(wszPath, ARRAYSIZE(wszPath), szPath);
            hr2 = ppf->Load(wszPath, 0);

            ppf->Release();
        }

        if (SUCCEEDED(hr) && m_pChannelIconHelper)
        {
            hr = m_pChannelIconHelper->GetIconLocation(uFlags, szIconFile, cchMax, piIndex, pwFlags);
        }

        if (bCoinit)
            CoUninitialize();

    }

    if (m_pChannelIconHelper == NULL)
    {
        WCHAR wszCookie[GUIDSTR_MAX];

        ASSERT (piIndex && pwFlags && szIconFile);

        StringFromGUID2(m_SubscriptionCookie, wszCookie, ARRAYSIZE(wszCookie));
        MyOleStrToStrN(szIconFile, cchMax, wszCookie);
        *piIndex = 0;
        *pwFlags |= GIL_NOTFILENAME | GIL_PERINSTANCE;
        hr = NOERROR;
    }

    return hr;
}

STDMETHODIMP CChannelAgent::Extract(LPCTSTR szIconFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    static HICON channelIcon = NULL;

    if (!phiconLarge || !phiconSmall)
        return E_INVALIDARG;

    * phiconLarge = * phiconSmall = NULL;

    if (m_pChannelIconHelper)
    {
        return m_pChannelIconHelper->Extract(szIconFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
    }
    else
    {
        DWORD   dwSize;

        if (!m_pBuf)    {
            m_pBuf = (POOEBuf)MemAlloc(LPTR, sizeof(OOEBuf));
            if (!m_pBuf)
                return E_OUTOFMEMORY;

            HRESULT hr = LoadWithCookie(NULL, m_pBuf, &dwSize, &m_SubscriptionCookie);
            RETURN_ON_FAILURE(hr);
        }

        BYTE    bBuf[MY_MAX_CACHE_ENTRY_INFO];
        LPINTERNET_CACHE_ENTRY_INFO pEntry = (INTERNET_CACHE_ENTRY_INFO *)bBuf;

        dwSize = sizeof(bBuf);
        if (GetUrlCacheEntryInfo(m_pBuf->m_URL, pEntry, &dwSize))   {
            SHFILEINFO  sfi;
            UINT    cbFileInfo = sizeof(sfi), uFlags = SHGFI_ICON | SHGFI_LARGEICON;

            if (NULL != SHGetFileInfo(pEntry->lpszLocalFileName, 0,
                                            &sfi, cbFileInfo, uFlags))
            {
                ASSERT(sfi.hIcon);
                *phiconLarge = *phiconSmall = sfi.hIcon;
                return NOERROR;
            }
        }

        if (channelIcon == NULL) {
            channelIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CHANNEL));
            ASSERT(channelIcon);
        }

        * phiconLarge = * phiconSmall = channelIcon;
        return NOERROR;
    }
}
