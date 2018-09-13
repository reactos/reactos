#include "private.h"
#include "subsmgrp.h"

#include <mluisupp.h>

// These next three are just so we can set the gleam on the channel bar
#include "chanmgr.h"
#include "chanmgrp.h"
#include "shguidp.h"    // IID_IChannelMgrPriv
//

#include "helper.h"
#include "propshts.h"
#define TF_THISMODULE TF_DELAGENT

CDeliveryAgent::CDeliveryAgent()
{
    // Maintain global count of objects
    DllAddRef();

    // Initialize object
    m_cRef = 1;

#ifdef AGENT_AUTODIAL
    m_iDialerStatus = DIALER_OFFLINE;
#endif

    SetEndStatus(INET_S_AGENT_BASIC_SUCCESS);
}

CDeliveryAgent::~CDeliveryAgent()
{
    DllRelease();

    CleanUp();
}

//
// IUnknown members
//

STDMETHODIMP_(ULONG) CDeliveryAgent::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CDeliveryAgent::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CDeliveryAgent::QueryInterface(REFIID riid, void ** ppv)
{

    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_ISubscriptionAgentControl == riid))
    {
        *ppv=(ISubscriptionAgentControl *)this;
    }
    else if (IID_IShellPropSheetExt == riid)
    {
        *ppv=(IShellPropSheetExt *)this;
    }
#ifdef UNICODE
    else if (IID_IExtractIconA == riid)
    {
        *ppv=(IExtractIconA *)this;
    }
#endif
    else if (IID_IExtractIcon == riid)
    {
        *ppv=(IExtractIcon *)this;
    }
    else if (IID_ISubscriptionAgentShellExt == riid)
    {
        *ppv=(ISubscriptionAgentShellExt *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    // Addref through the interface
    ((LPUNKNOWN)*ppv)->AddRef();

    return S_OK;
}

// IShellPropSheetExt members

HRESULT CDeliveryAgent::RemovePages(HWND hdlg)
{
    HRESULT hr = S_OK;

    for (int i = 0; i < ARRAYSIZE(m_hPage); i++)
    {
        if (NULL != m_hPage[i])
        {
            PropSheet_RemovePage(hdlg, 0, m_hPage[i]);
            m_hPage[i] = NULL;
        }
    }
    
    return hr;
}

HRESULT CDeliveryAgent::SaveSubscription()
{
    return SaveBufferChange(m_pBuf, TRUE);
}

HRESULT CDeliveryAgent::URLChange(LPCWSTR pwszNewURL)
{
    return E_NOTIMPL;
}

HRESULT CDeliveryAgent::AddPages(LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
    HRESULT hr = S_OK;  //  optimistic
    PROPSHEETPAGE psp;

    // initialize propsheet page.
    psp.dwSize          = sizeof(PROPSHEETPAGE);
    psp.dwFlags         = PSP_DEFAULT;
    psp.hInstance       = MLGetHinst();
    psp.pszIcon         = NULL;
    psp.pszTitle        = NULL;
    psp.lParam          = (LPARAM)m_pBuf;

    psp.pszTemplate     = MAKEINTRESOURCE(IDD_SUBSPROPS_SCHEDULE);
    psp.pfnDlgProc      = SchedulePropDlgProc;

    m_hPage[0] = CreatePropertySheetPage(&psp);

    psp.pszTemplate     = MAKEINTRESOURCE((m_pBuf->clsidDest == CLSID_ChannelAgent) ?
                                          IDD_SUBSPROPS_DOWNLOAD_CHANNEL :
                                          IDD_SUBSPROPS_DOWNLOAD_URL);
    psp.pfnDlgProc      = DownloadPropDlgProc;
    m_hPage[1] = CreatePropertySheetPage(&psp);

    if ((NULL != m_hPage[0]) && (NULL != m_hPage[1]))
    {
        for (int i = 0; i < ARRAYSIZE(m_hPage); i++)
        {
            if (!lpfn(m_hPage[i], lParam))
            {
                hr = E_FAIL;
                break;
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if (FAILED(hr))
    {
        for (int i = 0; i < ARRAYSIZE(m_hPage); i++)
        {
            if (NULL != m_hPage[i]) 
            {
                DestroyPropertySheetPage(m_hPage[i]);
                m_hPage[i] = NULL;
            }
        }
    }

    return hr;
}

HRESULT CDeliveryAgent::ReplacePage(UINT pgId, LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
    return E_NOTIMPL;
}

#ifdef UNICODE
// IExtractIconA members
HRESULT CDeliveryAgent::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    return IExtractIcon_GetIconLocationThunk((IExtractIconW *)this, uFlags, szIconFile, cchMax, piIndex, pwFlags);
}

HRESULT CDeliveryAgent::Extract(LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    return IExtractIcon_ExtractThunk((IExtractIconW *)this, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
}
#endif

// IExtractIconT members
HRESULT CDeliveryAgent::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    return E_NOTIMPL;
}

HRESULT CDeliveryAgent::Extract(LPCTSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    return E_NOTIMPL;
}

HRESULT CDeliveryAgent::Initialize(SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                                   LPCWSTR pwszURL, LPCWSTR pwszName, 
                                   SUBSCRIPTIONTYPE subsType)
{
    HRESULT hr;

    ASSERT(NULL == m_pBuf);

    m_pBuf = (POOEBuf)MemAlloc(LPTR, sizeof(OOEBuf));

    if (NULL != m_pBuf)
    {
        ISubscriptionItem *psi;

        hr = SubscriptionItemFromCookie(FALSE, pSubscriptionCookie, &psi);
        
        if (SUCCEEDED(hr))
        {
            DWORD dwSize;

            m_SubscriptionCookie = *pSubscriptionCookie;

            hr = LoadWithCookie(NULL, m_pBuf, &dwSize, pSubscriptionCookie);
            psi->Release();
        }
        else
        {
            hr = GetDefaultOOEBuf(m_pBuf, subsType);
            MyOleStrToStrN(m_pBuf->m_URL, ARRAYSIZE(m_pBuf->m_URL), pwszURL);
            MyOleStrToStrN(m_pBuf->m_Name, ARRAYSIZE(m_pBuf->m_Name), pwszName);
            m_pBuf->m_Cookie = *pSubscriptionCookie;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


// ISubscriptionAgentControl members
STDMETHODIMP CDeliveryAgent::StartUpdate(IUnknown *pItem, IUnknown *punkAdvise)
{
    HRESULT hr;
    DWORD dwTemp;
    VARIANT_BOOL  fTemp;

    m_lSizeDownloadedKB = -1;

    SAFERELEASE(m_pAgentEvents);
    punkAdvise->QueryInterface(IID_ISubscriptionAgentEvents, (void **)&m_pAgentEvents);

    // For now detect either notification or subscription item
    if (FAILED(pItem->QueryInterface(IID_ISubscriptionItem, (void **)&m_pSubscriptionItem)))
    {
        DBG_WARN("CDeliveryAgent::StartUpdate not an ISubscriptionItem!");
        return E_FAIL;
    }

    // We have a subscription item! Use it.
    TraceMsg(TF_THISMODULE, "CDeliveryAgent::StartUpdate at thread 0x%08x", GetCurrentThreadId());

    ASSERT(!IsAgentFlagSet(FLAG_BUSY));
    if (IsAgentFlagSet(FLAG_BUSY))
        return E_FAIL;

    ASSERT(m_pSubscriptionItem);

    SetEndStatus(INET_S_AGENT_BASIC_SUCCESS);
    m_dwAgentFlags = 0;

    m_pSubscriptionItem->GetCookie(&m_SubscriptionCookie);

    if (SUCCEEDED(ReadDWORD(m_pSubscriptionItem, c_szPropAgentFlags, &dwTemp)))
    {
        ASSERT(!(dwTemp & 0xFFFF0000));
        dwTemp &= 0xFFFF;           // only let them set lower 16 bits
        m_dwAgentFlags |= dwTemp;   // set flags client specifies
    }

    fTemp=FALSE;
    ReadBool(m_pSubscriptionItem, c_szPropCrawlChangesOnly, &fTemp);
    if (fTemp)
    {
        SetAgentFlag(FLAG_CHANGESONLY);
    }

    SetAgentFlag(FLAG_OPSTARTED);
    hr = StartOperation();

    return hr;
}

STDMETHODIMP CDeliveryAgent::PauseUpdate(DWORD dwFlags)
{
    DBG("CDeliveryAgent::PauseUpdate");

    if (!IsAgentFlagSet(FLAG_PAUSED | FLAG_WAITING_FOR_INCREASED_CACHE))
    {
        SetAgentFlag(FLAG_PAUSED);
        return AgentPause(dwFlags);
    }

    return S_FALSE;
}

HRESULT CDeliveryAgent::AgentPause(DWORD dwFlags)
{
    return S_OK;
}

STDMETHODIMP CDeliveryAgent::ResumeUpdate(DWORD dwFlags)
{
    DBG("CDeliveryAgent::ResumeUpdate");

    if (IsAgentFlagSet(FLAG_PAUSED | FLAG_WAITING_FOR_INCREASED_CACHE))
    {
        if (IsAgentFlagSet(FLAG_WAITING_FOR_INCREASED_CACHE))
            dwFlags |= SUBSCRIPTION_AGENT_RESUME_INCREASED_CACHE;

        ClearAgentFlag(FLAG_PAUSED | FLAG_WAITING_FOR_INCREASED_CACHE);
        return AgentResume(dwFlags);
    }

    return S_FALSE;
}

HRESULT CDeliveryAgent::AgentResume(DWORD dwFlags)
{
    return S_OK;
}

STDMETHODIMP CDeliveryAgent::AbortUpdate(DWORD dwFlags)
{
    TraceMsg(TF_THISMODULE, "AbortUpdate at Thread %d", GetCurrentThreadId());

    // Fill in status code if someone else hasn't already
    if (INET_S_AGENT_BASIC_SUCCESS == GetEndStatus())
    {
        if (IsAgentFlagSet(FLAG_WAITING_FOR_INCREASED_CACHE))
        {
            SetEndStatus(INET_E_AGENT_CACHE_SIZE_EXCEEDED);
        }
        else
        {
            SetEndStatus(E_ABORT);
        }
    }

    AddRef();

    // This may release us if the agent cleans itself up
    if (E_PENDING != AgentAbort(dwFlags))
    {
        // Will call "UpdateEnd" if necessary
        CleanUp();
    }

    Release();

    return S_OK;
}

HRESULT CDeliveryAgent::AgentAbort(DWORD dwFlags)
{
    return S_OK;
}

HRESULT CDeliveryAgent::SubscriptionControl(IUnknown *pItem, DWORD dwControl)
{
    if (dwControl & SUBSCRIPTION_AGENT_DELETE)
    {
        // Clean up our cache group
        GROUPID llGroupID;
        ISubscriptionItem *psi=NULL;

        pItem->QueryInterface(IID_ISubscriptionItem, (void **)&psi);
        if (psi)
        {
            if (SUCCEEDED(ReadLONGLONG(psi, c_szPropCrawlGroupID, &llGroupID))
                && (0 != llGroupID))
            {
                if (ERROR_SUCCESS != DeleteUrlCacheGroup(llGroupID, 0, 0))
                {
                    DBG_WARN("Failed to delete subscription cache group!");
                }
            }

            psi->Release();
        }
    }

    return S_OK;
}


#ifdef AGENT_AUTODIAL
HRESULT CDeliveryAgent::OnInetOnline()
{
    HRESULT hr=S_OK;

    if (m_iDialerStatus == DIALER_CONNECTING)
    {
        DBG("Delivery Agent: connection successful, beginning download");

        m_iDialerStatus=DIALER_ONLINE;

        hr = DoStartDownload();
    }

    return hr;
}
#endif

HRESULT CDeliveryAgent::DoStartDownload()
{
    HRESULT hr;

    // Always reset cache browser session. Webcrawler will avoid downloading dups.
    // Reset the cache session to hit the net on urls
    // CUrlDownload will use RESYNCHRONIZE flag if SYNC_MODE is Never
    InternetSetOption(NULL, INTERNET_OPTION_RESET_URLCACHE_SESSION, NULL, 0);

    // Refcount just in case our derived class cleans itself up synchronously, yet
    //  returns failure (cdlagent)
    AddRef();
    
    hr = StartDownload();

    if (FAILED(hr))
    {
        DBG_WARN("DeliveryAgent: StartDownload failed");
        if (GetEndStatus() == INET_S_AGENT_BASIC_SUCCESS)
            SetEndStatus(hr);
        CleanUp();
    }

    Release();

    return hr;
}

#ifdef AGENT_AUTODIAL
HRESULT CDeliveryAgent::OnInetOffline()
{
    DBG("DeliveryAgent: received InetOffline, aborting");

    m_iDialerStatus=DIALER_OFFLINE;

    ASSERT(IsAgentFlagSet(FLAG_BUSY));    // we have send update begin

    SetEndStatus(INET_E_AGENT_CONNECTION_FAILED);

    // we can look at Status from dialer notification here

    AbortUpdate(0);

    return S_OK;
}
#endif // AGENT_AUTODIAL

void CDeliveryAgent::SendUpdateBegin()
{
    ASSERT(!IsAgentFlagSet(FLAG_BUSY));
    ASSERT(m_pAgentEvents);

    if (!IsAgentFlagSet(FLAG_BUSY))
    {
        SetAgentFlag(FLAG_BUSY);

        AddRef();       // Keep an additional reference while "busy"
    }

    // New interface way
    m_pAgentEvents->UpdateBegin(&m_SubscriptionCookie);
}

void CDeliveryAgent::SendUpdateProgress(LPCWSTR pwszURL, long lCurrent, long lMax, long lCurSizeKB)
{
    ASSERT(IsAgentFlagSet(FLAG_BUSY));

    // New interface way
    m_pAgentEvents->UpdateProgress(&m_SubscriptionCookie, lCurSizeKB,
                                        lCurrent, lMax, S_OK, pwszURL);
}

void CDeliveryAgent::SendUpdateEnd()
{
    ASSERT(m_pSubscriptionItem);
    ASSERT(m_pAgentEvents);

    UINT uiRes;
    ISubscriptionItem *pEndItem=NULL;
    LPWSTR pwszEndStatus=NULL;
    TCHAR szEndStatus[MAX_RES_STRING_LEN];
    WCHAR wszEndStatus[MAX_RES_STRING_LEN];

    WriteSCODE(m_pSubscriptionItem, c_szPropStatusCode, GetEndStatus());

    if (SUCCEEDED(GetEndStatus()))
    {
        // Put in end time.
        SYSTEMTIME st;
        DATE dt;

        GetLocalTime(&st);
        SystemTimeToVariantTime(&st, &dt);

        WriteDATE(m_pSubscriptionItem, c_szPropCompletionTime, &dt);
    }

    if (GetEndStatus() == INET_S_AGENT_BASIC_SUCCESS)
        SetEndStatus(S_OK);

    switch (GetEndStatus())
    {
    case INET_E_AGENT_MAX_SIZE_EXCEEDED     : uiRes = IDS_AGNT_STATUS_SIZELIMIT; break;
    case INET_E_AGENT_CACHE_SIZE_EXCEEDED   : uiRes = IDS_AGNT_STATUS_CACHELIMIT; break;
    case INET_E_AUTHENTICATION_REQUIRED     : uiRes = IDS_STATUS_AUTHFAILED; break;
    case INET_E_AGENT_CONNECTION_FAILED     : uiRes = IDS_STATUS_DIAL_FAIL; break;
    case E_OUTOFMEMORY                      : uiRes = IDS_STATUS_OUTOFMEMORY; break;
    case E_INVALIDARG                       : uiRes = IDS_STATUS_BAD_URL; break;
    case E_ABORT                            : uiRes = IDS_STATUS_ABORTED; break;
    case S_FALSE                            : uiRes = IDS_STATUS_UNCHANGED; break;
    default:
        if (FAILED(GetEndStatus()))
            uiRes = IDS_STATUS_NOT_OK;
        else
            uiRes = IDS_STATUS_OK;
        break;
    }
    DoCloneSubscriptionItem(m_pSubscriptionItem, NULL, &pEndItem);

    ModifyUpdateEnd(pEndItem, &uiRes);

    // Write returned uiRes string into end report (returned -1 means don't touch it)
    if (uiRes != (UINT)-1)
    {
        if (MLLoadString(uiRes, szEndStatus, ARRAYSIZE(szEndStatus)))
        {
            MyStrToOleStrN(wszEndStatus, ARRAYSIZE(wszEndStatus), szEndStatus);
            if (pEndItem)
                WriteOLESTR(pEndItem, c_szPropStatusString, wszEndStatus);
            WriteOLESTR(m_pSubscriptionItem, c_szPropStatusString, wszEndStatus);
            pwszEndStatus = wszEndStatus;
        }
        else
            WriteEMPTY(m_pSubscriptionItem, c_szPropStatusString);
    }

    // ReportError if our end status is an error
    if (FAILED(GetEndStatus()))
    {
        m_pAgentEvents->ReportError(&m_SubscriptionCookie, GetEndStatus(), pwszEndStatus);
    }

    m_pAgentEvents->UpdateEnd(&m_SubscriptionCookie, 
                    m_lSizeDownloadedKB, GetEndStatus(), pwszEndStatus);

#ifdef AGENTS_AUTODIAL
    // Tell the dialer it can hang up now
    if (m_pConnAgent != NULL)
        NotifyAutoDialer(DIALER_HANGUP);

    m_iDialerStatus = DIALER_OFFLINE;
#endif

    // Check for appropriate behavior on end item. Don't do anything if we're
    //  not a subscription in our own right.
    if (!IsAgentFlagSet(DELIVERY_AGENT_FLAG_NO_BROADCAST))
    {
        if (pEndItem)
            ProcessEndItem(pEndItem);
        else
            ProcessEndItem(m_pSubscriptionItem);
    }

    if (!IsAgentFlagSet(FLAG_HOSTED))
    {
        m_pSubscriptionItem->NotifyChanged();
    }

    SAFERELEASE(pEndItem);

    if (IsAgentFlagSet(FLAG_BUSY))
    {
        ClearAgentFlag(FLAG_BUSY);

        // Release the reference we had to ourself
        Release();
    }
}

// This calls callback and cleans everything up properly
void CDeliveryAgent::SendUpdateNone()
{
    ASSERT(FAILED(GetEndStatus()));  // set this before calling
    ASSERT(!IsAgentFlagSet(FLAG_BUSY));// shouldn't call here if busy

    AddRef();

    if (!IsAgentFlagSet(FLAG_BUSY))
        SendUpdateEnd();

    CleanUp();

    Release();
}

// Process the End Item including all stuff set by the base class
// This has functionality previously in the Tray Agent
// Send email, set gleam, refresh desktop, etc.
HRESULT CDeliveryAgent::ProcessEndItem(ISubscriptionItem *pEndItem)
{
    HRESULT hr;

    if (SUCCEEDED(GetEndStatus()))
    {
        //
        // Special feature for desktop HTML:
        // If we receive an end report with "DesktopComponent=1" in it,
        // let the desktop know that it needs to refresh itself.  We always
        // do this instead of only on "changes detected" because desktop
        // component authors don't want to change their CDFs.
        //
        DWORD dwRet;
        HRESULT hr2 = ReadDWORD(pEndItem, c_szPropDesktopComponent, &dwRet);
        if (SUCCEEDED(hr2) && (dwRet == 1))
        {
            IActiveDesktop *pAD = NULL;
            hr2 = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void**)&pAD);
            DBGASSERT(SUCCEEDED(hr2), "Unable to create ActiveDesktop in order to refresh desktop component");
            if (SUCCEEDED(hr2))
            {
                ASSERT(pAD);
                pAD->ApplyChanges(AD_APPLY_FORCE | AD_APPLY_REFRESH | AD_APPLY_BUFFERED_REFRESH);
                pAD->Release();
            }
        }
    }

    //
    // Gleam the Internet Shortcut for the URL if requested.  (EnableShortcutGleam=1)
    // Filter End Reports without changes (S_FALSE)
    //
    if (SUCCEEDED(GetEndStatus()) && (S_FALSE != GetEndStatus()))
    {
        DWORD dwRet;
        hr = ReadDWORD(pEndItem, c_szPropEnableShortcutGleam, &dwRet);
        if (SUCCEEDED(hr) && dwRet)
        {
            LPTSTR strURL = NULL;
            hr = ReadTSTR(pEndItem, c_szPropURL, &strURL);
            if (SUCCEEDED(hr))
            {
                PROPVARIANT propvar;
                PropVariantInit(&propvar);
                hr = IntSiteHelper(strURL, &c_rgPropRead[PROP_FLAGS], &propvar, 1, FALSE);
                if (SUCCEEDED(hr) && (VT_UI4 == propvar.vt))
                {
                    // Set our flag without disturbing the others.
                    propvar.ulVal |= PIDISF_RECENTLYCHANGED;  
                }
                else
                {
                    // Be sure to clear the variant if it wasn't a DWORD.
                    PropVariantClear(&propvar);
                    propvar.vt = VT_UI4;
                    propvar.ulVal = PIDISF_RECENTLYCHANGED;  
                }

                //
                // Update channels (painful).
                //

                hr = ReadDWORD(pEndItem, c_szPropChannel, &dwRet);
                BOOL bChannel = SUCCEEDED(hr) && dwRet;

                //  BUGBUG - this is lame.  Once cdfview is fixed, we can fix this.
                
                TCHAR tszChanImgPath[MAX_PATH];
                CHAR szChanImgPath[MAX_PATH];
                CHAR szChanImgHash[MAX_PATH];
                int  iChanImgIndex = 0; // init to keep compiler happy
                UINT uChanImgFlags = 0; // init to keep compiler happy
                int  iChanImgImageIndex = 0; // init to keep compiler happy

                IChannelMgrPriv*   pIChannelMgrPriv = NULL;
                HRESULT            hr2 = E_FAIL;

                if (bChannel)
                {
                    hr2 = GetChannelPath(strURL, tszChanImgPath,
                                         ARRAYSIZE(tszChanImgPath),
                                         &pIChannelMgrPriv);
                    if (SUCCEEDED(hr2))
                    {
                        SHTCharToAnsi(tszChanImgPath, szChanImgPath, ARRAYSIZE(szChanImgPath));
                        hr2 = (pIChannelMgrPriv)->PreUpdateChannelImage(
                                                    szChanImgPath,
                                                    szChanImgHash,
                                                    &iChanImgIndex,
                                                    &uChanImgFlags,
                                                    &iChanImgImageIndex);
                    }
                }

                // Set the gleam in the intsite database
                hr = IntSiteHelper(strURL, &c_rgPropRead[PROP_FLAGS], &propvar, 1, TRUE);
                DBGASSERT(SUCCEEDED(hr), "CTrayAgent::OnNotification - failed to set gleam.");

                if (bChannel && SUCCEEDED(hr2))
                {
                    ASSERT(pIChannelMgrPriv);

                    pIChannelMgrPriv->InvalidateCdfCache();
                    // brilliant - the api requires us to convert their own return value
                    WCHAR wszHash[MAX_PATH];
                    SHAnsiToUnicode(szChanImgHash, wszHash, ARRAYSIZE(wszHash));

                    pIChannelMgrPriv->UpdateChannelImage(
                                                wszHash,
                                                iChanImgIndex,
                                                uChanImgFlags,
                                                iChanImgImageIndex);
                }
                if (pIChannelMgrPriv)
                    pIChannelMgrPriv->Release();
            }
            MemFree(strURL); // Free the string allocated by ReadAnsiSTR().
        }// end setting gleam

        //
        // Send Email to notify the user if requested (EmailNotification=1)
        // NOTE: Updates without changes (S_FALSE) were filtered above.
        //
        hr = ReadDWORD(pEndItem, c_szPropEmailNotf, &dwRet);
        if (SUCCEEDED(hr) && dwRet)
        {
            hr = SendEmailFromItem(pEndItem);
        }
    }

    return S_OK;
}

// Checks the status code after all actions such as authentication and redirections
//  have taken place.
HRESULT CDeliveryAgent::CheckResponseCode(DWORD dwHttpResponseCode)
{
    TraceMsg(TF_THISMODULE, "CDeliveryAgent processing HTTP status code %d", dwHttpResponseCode);

    switch (dwHttpResponseCode / 100)
    {
        case 1 :    DBG("HTTP 1xx response?!?");
        case 2 :
            return S_OK;    // Success

        case 3 :
            if (dwHttpResponseCode == 304)
                return S_OK;    // Not Modified
            SetEndStatus(E_INVALIDARG);
            return E_ABORT;     // Redirection

        case 4 :
            if (dwHttpResponseCode == 401)
            {
                SetEndStatus(INET_E_AUTHENTICATION_REQUIRED);
                return E_ABORT;
            }
            SetEndStatus(E_INVALIDARG);
            return E_ABORT;

        case 5 :
        default:
            SetEndStatus(E_INVALIDARG);
            return E_ABORT;
    }

/*  
    //  Unreachable code
    SetEndStatus(E_FAIL);
    return E_FAIL;

*/
}

//============================================================
//   virtual functions designed to be overridden as necessary
//============================================================

HRESULT CDeliveryAgent::StartOperation()
{
    HRESULT hr = S_OK;

#ifdef AGENT_AUTODIAL
    // We are ready to go. Now we make sure we're actually connected to
    //  the internet and then go for it.
    if (IsAgentFlagSet(DELIVERY_AGENT_FLAG_SILENT_DIAL))
    {
        m_iDialerStatus = DIALER_CONNECTING;

        hr = NotifyAutoDialer(DIALER_START);
    }

    if (SUCCEEDED(hr))
    {
        // Send this whether we're 'dialing' or not
        SendUpdateBegin();
    }
    else
    {
        DBG("NotifyAutoDialer failed, delivery agent aborting.");
        SetEndStatus(E_ACCESSDENIED);
        SendUpdateNone();
        return E_FAIL;
    }

    if (IsAgentFlagSet(DELIVERY_AGENT_FLAG_SILENT_DIAL))
    {
        hr = DoStartDownload();
    }
#else
    SendUpdateBegin();
    hr = DoStartDownload();
#endif

    return hr;
}

HRESULT CDeliveryAgent::ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes)
{
    return S_OK;
}

void CDeliveryAgent::CleanUp()
{
    BOOL fAdded=FALSE;

    if (m_cRef > 0)
    {
        fAdded = TRUE;
        AddRef();
    }

    if (IsAgentFlagSet(FLAG_BUSY))
        SendUpdateEnd();

    SAFERELEASE(m_pAgentEvents);
    SAFERELEASE(m_pSubscriptionItem);

    if (fAdded)
        Release();
}
