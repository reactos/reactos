#include "private.h"
#include <urlmon.h>
#include <wininet.h>
#include <msxml.h>
#include "cdfagent.h"
#include "cdlabsc.h"
#include "cdlagent.h"
#include <urlmon.h>
#include <subsmgr.h>
#include "subsmgrp.h"

#include <mluisupp.h>

HRESULT GetXMLAttribute(IXMLElement *pItem, LPCWSTR pwszAttribute, VARIANT *pvRet);

HRESULT GetNextChildTag(IXMLElement *pRoot, LPCWSTR szTag, IXMLElement **ppChildReq, int &nLastChild)
{
    BSTR bstrTag = NULL;
    IXMLElementCollection * pChildren = NULL;
    HRESULT hr = S_FALSE;   // assume not found.
    IXMLElement * pChild = NULL;

    //
    // Find the children if they exist
    //
    if (SUCCEEDED(pRoot->get_children(&pChildren)) && pChildren)
    {
        long length = 0;

        if (SUCCEEDED(pChildren->get_length(&length)) && length > 0)
        {
            VARIANT vIndex, vEmpty;
            vIndex.vt = VT_I4;
            vEmpty.vt = VT_EMPTY;

            nLastChild++;

            for (long i=nLastChild; i<length; i++)
            {
                vIndex.lVal = i;
                IDispatch *pDispItem = NULL;
                if (SUCCEEDED(pChildren->item(vIndex, vEmpty, &pDispItem)))
                {

                    if (SUCCEEDED(pDispItem->QueryInterface(IID_IXMLElement, (void **)&pChild)))
                    {
                        // look for first SoftDist tag

                        pChild->get_tagName(&bstrTag);

                        if (StrCmpIW(bstrTag, szTag) == 0) {
                            nLastChild = i;
                            hr = S_OK;
                            goto Exit;
                        }

                        SAFEFREEBSTR(bstrTag);

                        SAFERELEASE(pChild);
                    }

                pDispItem->Release();

                }


            }
        }

    }
    else
    {
        hr = E_FAIL;
    }


Exit:

    *ppChildReq = pChild;

    if (pChildren)
        SAFERELEASE(pChildren);

    SAFEFREEBSTR(bstrTag);

    return hr;
}

HRESULT GetFirstChildTag(IXMLElement *pRoot, LPCWSTR szTag, IXMLElement **ppChildReq)
{
    int nLastChild = -1; // first child, never seen any before this one

    return GetNextChildTag(pRoot, szTag, ppChildReq, nLastChild);
}

CCDLAgent::CCDLAgent()
: m_pCCDLAgentBSC(NULL)
, m_szCDF(NULL)
, m_bAcceptSoftware(FALSE)
{
    m_sdi.cbSize = sizeof(SOFTDISTINFO);
    m_bSilentMode = TRUE;
}

CCDLAgent::~CCDLAgent()
{

    SAFERELEASE(m_pSoftDistElement);
    SAFERELEASE(m_pSoftDistExt);

    CRunDeliveryAgent::SafeRelease(m_pAgent);

    SAFEFREEOLESTR(m_szCDF);
    SAFEFREEBSTR(m_szErrorText);
    SAFEDELETE(m_sdi.szAbstract);
    SAFEDELETE(m_sdi.szTitle);
    SAFEDELETE(m_sdi.szHREF);
    SAFEFREEOLESTR(m_szDistUnit);
}

HRESULT CCDLAgent::StartOperation()
{
    HRESULT                  hr = S_OK, hr2;

    // unknown pointers
    IUnknown                *punk = NULL;
    IServiceProvider        *pSP;
    
    m_pSoftDistElement = NULL;
    if (FAILED(ReadOLESTR(m_pSubscriptionItem, c_szPropURL, &m_szURL)))
    {
        hr = E_INVALIDARG;
        goto Failed;
    }


    hr2 = E_FAIL;

    if (SUCCEEDED(m_pAgentEvents->QueryInterface(IID_IServiceProvider, (void **)&pSP)) && pSP)
    {
        hr2 = pSP->QueryService(CLSID_XMLDocument, IID_IXMLElement, (void **)&punk);
        pSP->Release();
    }

    if (FAILED(hr2) || !punk)
    {
        // We are processing a request to pull a CAB, probably from Web Crawler agent.
        
        if (FAILED(ReadOLESTR(m_pSubscriptionItem, L"DistUnit", &m_szDistUnit)) ||
            FAILED(ReadDWORD(m_pSubscriptionItem, L"VersionMS",&m_dwVersionMS)) ||
            FAILED(ReadDWORD(m_pSubscriptionItem, L"VersionLS", &m_dwVersionLS)))
        {
            hr = E_INVALIDARG;
            goto Failed;
        }

        m_pSoftDistElement = NULL;
    }
    else
    {
        if (FAILED(punk->QueryInterface(IID_IXMLElement, (void **)&m_pSoftDistElement)))
        {
            SAFERELEASE(punk);
            hr = E_INVALIDARG;
            goto Failed;
        }

        SAFERELEASE(punk);

        Assert(m_pSoftDistElement);
    }


    ReadDWORD(m_pSubscriptionItem, c_szPropCrawlMaxSize, &m_dwMaxSizeKB);
    ReadDWORD(m_pSubscriptionItem, c_szPropChannelFlags, &m_dwChannelFlags);
    ReadDWORD(m_pSubscriptionItem, c_szPropAgentFlags, &m_dwAgentFlags);

    hr = CDeliveryAgent::StartOperation();

    return hr;

Failed:
    SetEndStatus(hr);
    SendUpdateNone();

    return hr;
}

HRESULT CCDLAgent::StartDownload()
{
    IBindCtx *pbc = NULL;
    HRESULT hr = S_OK;
    LPWSTR szCodeBase;
    DWORD dwSize;
    BOOL bCleanUpNow = FALSE;
    DWORD dwPolicy = 0;
    DWORD dwContext = 0;
    IInternetSecurityManager * pism = NULL;

    if (FAILED(GetEndStatus())) {
    
        hr = GetEndStatus();
        goto Exit;
    }

    hr = CoCreateInstance(CLSID_SoftDistExt, NULL, CLSCTX_INPROC_SERVER, IID_ISoftDistExt, (void **)&m_pSoftDistExt);

    if (FAILED(hr))
        goto Exit;

    // Process SOFTDIST tag structure if present.
    if (m_pSoftDistElement != NULL) {

        dwPolicy = 0xFFFF0000;

        if (FAILED(CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IInternetSecurityManager, (void**)&pism)) || !pism)
        {
            hr = E_ACCESSDENIED;
            goto Exit;
        }

        hr = pism->ProcessUrlAction(m_szURL, URLACTION_CHANNEL_SOFTDIST_PERMISSIONS,
                                (BYTE *)&dwPolicy, sizeof(dwPolicy),
                                (BYTE *)&dwContext, sizeof(dwContext), PUAF_NOUI, 0);
        
        pism->Release();

        if (FAILED(hr))
        {
            goto Exit;
        }

        dwPolicy &= 0xFFFF0000;

        if (dwPolicy != URLPOLICY_CHANNEL_SOFTDIST_PROHIBIT
            && dwPolicy != URLPOLICY_CHANNEL_SOFTDIST_PRECACHE
            && dwPolicy != URLPOLICY_CHANNEL_SOFTDIST_AUTOINSTALL)
        {
            hr = E_INVALIDARG;
            goto Exit;
        }

        if (dwPolicy == URLPOLICY_CHANNEL_SOFTDIST_PROHIBIT)
        {
            hr = E_ACCESSDENIED;
            goto Exit;
        }

        hr = m_pSoftDistExt->ProcessSoftDist(m_szCDF, m_pSoftDistElement, &m_sdi);

        if (m_sdi.dwFlags & SOFTDIST_FLAG_DELETE_SUBSCRIPTION) {
            
            ISubscriptionMgr *pSubMgr = NULL;
            hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&pSubMgr);
            if (SUCCEEDED(hr))
            {
                hr = pSubMgr->DeleteSubscription(m_szURL,NULL);
                pSubMgr->Release();
            }
      
            hr = S_FALSE;
        }

        // Send email & update software?
        if (hr == S_OK) {

        
            if (m_sdi.dwFlags) {
                
                m_bSendEmail = TRUE;

            } else {

                // no usage flag and no restriction implies no email.
                m_bSendEmail = FALSE;            
            }

            if (m_sdi.dwFlags & SOFTDIST_FLAG_USAGE_AUTOINSTALL) {
                
                m_bAcceptSoftware = (dwPolicy == URLPOLICY_CHANNEL_SOFTDIST_AUTOINSTALL) ? TRUE : FALSE;
                m_bSilentMode = FALSE;

            } else if (m_sdi.dwFlags & SOFTDIST_FLAG_USAGE_PRECACHE) {

                // to get here, we must have precache or autoinstall policy permissions
                m_bAcceptSoftware = TRUE;
                
            } else {

                m_bAcceptSoftware = FALSE;

            }

        } else {
            m_bSendEmail = FALSE;
            m_bAcceptSoftware = FALSE;
            bCleanUpNow = TRUE;
        }
      
        // Do only code download from here on.
        if (!m_bAcceptSoftware || 
            !((m_dwChannelFlags & CHANNEL_AGENT_PRECACHE_SOME) ||
            (m_dwChannelFlags & CHANNEL_AGENT_PRECACHE_ALL)) ) {

            // No caching allowed, return immediately.
    
            bCleanUpNow = TRUE;
            goto Exit;

         } else {
         
            if (m_dwChannelFlags & CHANNEL_AGENT_PRECACHE_ALL) {
                m_dwMaxSizeKB = 0;
            }
        }
        
    }

    m_pCCDLAgentBSC = new CDLAgentBSC(this, m_dwMaxSizeKB, m_bSilentMode, m_szCDF);
    if (m_pCCDLAgentBSC == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    // attempt to use AsyncInstallDistributionUnit

    hr = CreateBindCtx(0, &pbc);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    hr = RegisterBindStatusCallback(pbc, m_pCCDLAgentBSC, NULL, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (m_pSoftDistElement != NULL) {

        hr = m_pSoftDistExt->AsyncInstallDistributionUnit(pbc, NULL, 0, NULL);

        if (hr == S_OK) {
            SendUpdateNone();
        }
    
    } else {
        
        CODEBASEHOLD *pcbh = new CODEBASEHOLD;
        if (pcbh == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        pcbh->cbSize = sizeof(CODEBASEHOLD);
        pcbh->szDistUnit = m_szDistUnit;
        pcbh->szCodeBase = m_szURL;
        pcbh->dwVersionMS = m_dwVersionMS;
        pcbh->dwVersionLS = m_dwVersionLS;
        pcbh->dwStyle = 0;

        // Since notification is likely from web crawler and we only support MSICD we
        // don't fire a notification back.
        hr = m_pSoftDistExt->AsyncInstallDistributionUnit(pbc, NULL, 0, pcbh);

        if (hr == S_OK) {
            SendUpdateNone();
        }
        
        SAFEDELETE(pcbh);
        goto Exit;

    }
    
    if (hr != E_NOTIMPL) {
        // May have succeeded or failed, either way, we are out of here.
        goto Exit;
    }

    hr = m_pSoftDistExt->GetFirstCodeBase(&szCodeBase, &dwSize);

    if (SUCCEEDED(hr) && szCodeBase) {

        hr = StartNextDownload(szCodeBase,dwSize);
        SAFEDELETE(szCodeBase);

    } else {

        // no CODEBASE, return OK
        bCleanUpNow = TRUE;
        hr = S_OK;
    }

Exit:
    // In case of SOFTDIST tag we work asychronously and send an END_REPORT back immediately.  If we were called
    // to install a particular CAB then CleanUp is called by CDLABSC::OnStopBinding and report is sent back then.

    SAFERELEASE(pbc);

    if (FAILED(hr) || bCleanUpNow)
    {
        SetEndStatus(hr);
        CleanUp();
    }

    return hr;
}

HRESULT CCDLAgent::StartNextDownload(LPWSTR wzCodeBase, DWORD dwSize)
{
HRESULT hr = E_FAIL;
DWORD dwTemp = 0;
ISubscriptionItem *pItem;

    if (m_dwMaxSizeKB && (dwSize > m_dwMaxSizeKB))
    {
        hr = INET_E_AGENT_MAX_SIZE_EXCEEDED;
        goto Exit;
    }
    else
    {
        // Any other type of INSTALL protocol.

        // Send notification to WebCrawl agent to crawl the codebase.  This should force it in the 
        // case. Only do this if there is any chance the DL will not overflow the cache.
        // Note this will only download the CAB file and not any dependencies inside the CAB.  They
        // should be included as separate CONFIG entries.

        if (m_dwMaxSizeKB && ((m_dwCurSize>>10) > m_dwMaxSizeKB))
        {
            // We've exceeded our maximum download KB limit and can't continue.
            hr = INET_E_AGENT_MAX_SIZE_EXCEEDED;
            goto Exit;
        }

        if (FAILED(hr = DoCloneSubscriptionItem(m_pSubscriptionItem, NULL, &pItem)) || !pItem)
        {
            goto Exit;
        }

        dwTemp = DELIVERY_AGENT_FLAG_NO_BROADCAST;
        WriteDWORD(pItem, c_szPropAgentFlags, dwTemp);
        WriteOLESTR(pItem, c_szPropURL, wzCodeBase);
        if (m_dwMaxSizeKB)
        {
            // KB limit for us to pull.
            WriteDWORD(pItem, c_szPropCrawlMaxSize, m_dwMaxSizeKB - (m_dwCurSize>>10));
        }
        WriteDWORD(pItem, c_szPropCrawlLevels, 0);

        m_dwCurSize += dwSize;

        m_pAgent = new CRunDeliveryAgent();
        if (m_pAgent)
            hr = m_pAgent->Init((CRunDeliveryAgentSink *)this, pItem, CLSID_WebCrawlerAgent);
        pItem->Release();

        if (m_pAgent && SUCCEEDED(hr))
        {
            hr = m_pAgent->StartAgent();
            if (hr == E_PENDING)
            {
                hr = S_OK;
            }
            else
            {
                DBG_WARN("StartNextDownload in CDL agent failed!");
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

Exit:
    return hr;
}

HRESULT CCDLAgent::OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                               long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                               BOOL fSynchronous)
{
    HRESULT hr = S_OK;
    BOOL fDone = FALSE;
    LPWSTR wzCodeBase = NULL;
    DWORD dwSize;

    ASSERT(m_pAgent != NULL);

    if (fSynchronous)
    {
        // We must have failed. Let StartNextDownload return failure.
        return S_OK;
    }

    CRunDeliveryAgent::SafeRelease(m_pAgent);

    if (SUCCEEDED(hrResult))
    {
        hr = m_pSoftDistExt->GetNextCodeBase(&wzCodeBase, &dwSize);

        if (SUCCEEDED(hr) && wzCodeBase)
        {
            hr = StartNextDownload(wzCodeBase, dwSize);
            SAFEDELETE(wzCodeBase);
            
            if (FAILED(hr)) {
                // we are done
                fDone = TRUE;
            }
            
        } else {

            // no more codebases to crawl
            hr = S_OK;
            fDone = TRUE;
        }
    }
    else
    {
        hr = hrResult;
        fDone = TRUE;
    }

    if (fDone) {
        SetEndStatus(hr);
        CleanUp();
    }

    return hr;
}

void CCDLAgent::CleanUp()
{
    if (m_pCCDLAgentBSC != NULL) {
        m_pCCDLAgentBSC->Release();
    }

    m_pCCDLAgentBSC = NULL;
    CDeliveryAgent::CleanUp();
}

void CCDLAgent::SetErrorEndText(LPCWSTR szErrorText)
{
    if (szErrorText)
        m_szErrorText = SysAllocString(szErrorText);
}

HRESULT CCDLAgent::AgentAbort(DWORD dwFlags)
{
    HRESULT              hr = S_OK;

    if (m_pCCDLAgentBSC != NULL )
    {
        hr = m_pCCDLAgentBSC->Abort();
    }
    
    return hr;
}

HRESULT CCDLAgent::AgentPause(DWORD dwFlags)
{
    HRESULT              hr = S_OK;

    if (m_pCCDLAgentBSC != NULL )
    {
        hr = m_pCCDLAgentBSC->Pause();
    }
    
    return hr;
}

HRESULT CCDLAgent::AgentResume(DWORD dwFlags)
{
    HRESULT              hr = S_OK;

    if (m_pCCDLAgentBSC != NULL )
    {
        hr = m_pCCDLAgentBSC->Resume();
    }
    
    return hr;
}


HRESULT CCDLAgent::ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes)
{
    VARIANT vHref;

    ASSERT(pEndItem);

    // The END_REPORT is sent for both functionalities of CDL agent (SOFTDIST and Pull single CAB).
    // customize our end status string

    switch (GetEndStatus())
    {
        case E_OUTOFMEMORY  : *puiRes = IDS_AGNT_STATUS_SIZELIMIT; break;
        case E_FAIL         : *puiRes = IDS_CRAWL_STATUS_NOT_OK; break;
        case S_FALSE        : *puiRes = IDS_CRAWL_STATUS_UNCHANGED; break;
        case INET_S_AGENT_PART_FAIL : *puiRes = IDS_CRAWL_STATUS_MOSTLYOK; break;

            // This is actually a success code from URLMON
        case HRESULT_FROM_WIN32(ERROR_CANCELLED)
                            : SetEndStatus(S_OK);
                              *puiRes = IDS_CRAWL_STATUS_OK; break;

        case TRUST_E_FAIL   : SetEndStatus(TRUST_E_SUBJECT_NOT_TRUSTED);

        case TRUST_E_SUBJECT_NOT_TRUSTED :
            
        case HRESULT_FROM_WIN32(ERROR_IO_INCOMPLETE) : SetEndStatus(S_OK);
                                // fall through
        case S_OK           : *puiRes = IDS_CRAWL_STATUS_OK; break;
        default             : *puiRes = IDS_CRAWL_STATUS_NOT_OK; break;
                    
            break;
    }

    // force gleam on this channel if we got S_OK on precaching bits
    if (SUCCEEDED(GetEndStatus()) && (GetEndStatus() != S_FALSE)) {
        WriteDWORD(pEndItem, c_szPropEnableShortcutGleam, 1);
    }

    // If we are sending email the status must be S_OK, we incorporate the error 
    // message into the text body for reporting.

    if (m_bSendEmail) {

        VariantInit(&vHref);

        WriteDWORD(pEndItem, c_szPropEmailFlags, MAILAGENT_FLAG_CUSTOM_MSG);

        // This must exist or m_bSendEmail would never have been set in first place.
        GetXMLAttribute(m_pSoftDistElement, L"HREF", &vHref);
    
        WriteOLESTR(pEndItem, c_szPropURL, vHref.bstrVal);
                   
        VariantClear(&vHref);

        if (m_sdi.szTitle) {
            BSTR bstrTitle = SysAllocString(m_sdi.szTitle);

            if (bstrTitle)
                WriteOLESTR(pEndItem, c_szPropEmailTitle, m_sdi.szTitle);

            SAFEFREEBSTR(bstrTitle);
        }

        if (FAILED(GetEndStatus()) && !m_szErrorText) {
            m_szErrorText = GetErrorMessage(GetEndStatus());
        }

        if (m_sdi.szAbstract) {

            BSTR bstrAbstract = SysAllocString(m_sdi.szAbstract);
                       
            if (bstrAbstract != NULL) {
                           
                if (m_szErrorText) {

                    //This is wrecking havoc with the email message, some resource strings
                    //have a 'CR/LF' tacked on the end.  We kill any that exist.
                    DWORD dwLen = lstrlenW(m_szErrorText)-1;
                    while (dwLen > 0 && 
                        (m_szErrorText[dwLen] == 0x0a 
                        || m_szErrorText[dwLen] == 0x0d
                        || m_szErrorText[dwLen] == L'.'))
                    {
                        m_szErrorText[dwLen] = L'\0';
                        dwLen--;
                    }

                    //  BUGBUG - needs cleanup!
                    CHAR szPrefixMsg[MAX_PATH], szFormattedPrefixMsg[MAX_PATH*2];
                    if (MLLoadStringA(IDS_CDLAGENT_ERROR_EMAIL, szPrefixMsg, ARRAYSIZE(szPrefixMsg))>0) {
                                
                        LPWSTR wszNewAbstract = NULL;
                        LPSTR szNewAbstract = NULL;

                        wnsprintfA(szFormattedPrefixMsg, 
                                   ARRAYSIZE(szFormattedPrefixMsg),
                                   szPrefixMsg, 
                                   m_szErrorText);
                        DWORD dwNewLen = lstrlenA(szFormattedPrefixMsg) + lstrlenW(bstrAbstract) + 4;
                        szNewAbstract = (LPSTR)LocalAlloc(0,dwNewLen*sizeof(CHAR));
                        if (szNewAbstract) {
                            wnsprintfA(szNewAbstract, 
                                       dwNewLen*sizeof(CHAR),
                                       "%s%ws", 
                                       szFormattedPrefixMsg, 
                                       bstrAbstract);
                            dwNewLen = lstrlenA(szNewAbstract) + 1;
                            wszNewAbstract = (LPWSTR)LocalAlloc(0,dwNewLen*sizeof(WCHAR));

                            if (wszNewAbstract &&
                                (MultiByteToWideChar(CP_ACP, 0, szNewAbstract, -1, wszNewAbstract, dwNewLen)>0)) {
                                 SAFEFREEBSTR(bstrAbstract);
                                 bstrAbstract = SysAllocString(wszNewAbstract);               
                            }

                            if (wszNewAbstract)
                                LocalFree(wszNewAbstract);

                            LocalFree(szNewAbstract);
                        }    
                                    
                    }
                }
                            
                WriteOLESTR(pEndItem, c_szPropEmailAbstract, bstrAbstract);

                SAFEFREEBSTR(bstrAbstract);
            }
                                            
        }

        // because user is notified of error we don't pass it on anywhere else
        SetEndStatus(S_OK);

        WriteSCODE(pEndItem, c_szPropStatusCode, S_OK);
    }

    ClearAgentFlag(DELIVERY_AGENT_FLAG_NO_BROADCAST);
    
    return CDeliveryAgent::ModifyUpdateEnd(pEndItem, puiRes);
}

LPWSTR CCDLAgent::GetErrorMessage(HRESULT hr)
{
    LPSTR szBuf = NULL;
    LPWSTR wszBuf = NULL;
    DWORD dwLen;
    DWORD dwResource = 0;

    if (SUCCEEDED(hr))
        return NULL;

    dwLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                          hr, 0, (LPTSTR)&szBuf, 0, NULL);
    if (!dwLen) {

        // NOTE: If out of memory we return NULL.
        if (SUCCEEDED(hr))
            dwResource = IDS_CDLAGENT_SUCCESS;
        else if (hr == TRUST_E_SUBJECT_NOT_TRUSTED)
            dwResource = IDS_CDLAGENT_TRUST_ERROR;
        else 
            dwResource = IDS_CDLAGENT_FAILURE;

        // We know strings will fit into max_path
        WCHAR szTmp[MAX_PATH];
        if (MLLoadStringW(dwResource, szTmp, MAX_PATH)>0) {
            wszBuf = SysAllocString(szTmp);
        }
        
    } else {

        WCHAR wszTemp[MAX_PATH];
        if (MultiByteToWideChar(CP_ACP, 0, szBuf, -1, wszTemp, MAX_PATH)>0) {   
            wszBuf = SysAllocString(wszTemp);
        } else
            wszBuf = NULL;

        SAFEDELETE(szBuf);
    }

    return wszBuf;
}
