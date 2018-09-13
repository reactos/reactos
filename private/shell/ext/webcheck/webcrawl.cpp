// TODO: Allow trident to download frames (and process new html)
// nuke urlmon code (use trident always)

#include "private.h"
#include "shui.h"
#include "downld.h"
#include "subsmgrp.h"
#include <ocidl.h>

#include <initguid.h>

#include <mluisupp.h>

extern HICON g_webCrawlerIcon;
extern HICON g_channelIcon;
extern HICON g_desktopIcon;

void LoadDefaultIcons();

#undef TF_THISMODULE
#define TF_THISMODULE   TF_WEBCRAWL

#define _ERROR_REPROCESSING -1

// DWORD field of the m_pPages string list
const DWORD DATA_RECURSEMASK = 0x000000FF;  // Levels of recursion from this page
const DWORD DATA_DLSTARTED =   0x80000000;  // Have we started downloading
const DWORD DATA_DLFINISHED =  0x40000000;  // Have we finished this page
const DWORD DATA_DLERROR =     0x20000000;  // An error during download
const DWORD DATA_CODEBASE =    0x10000000;  // Is codebase
const DWORD DATA_LINK =        0x08000000;  // Is link from page (not dependency)

// DWORD field of m_pPendingLinks string list
const DWORD DATA_ROBOTSTXTMASK=0x00000FFF;  // index into m_pRobotsTxt list

// used internally; not actually stored in string list field
const DWORD DATA_ROBOTSTXT =   0x01000000;  // Is robots.txt

// m_pDependencyLinks uses m_pPages values

// DWORD field of m_pRobotsTxt is NULL or (CWCDwordStringList *)

// DWORD field of m_pRobotsTxt referenced string list
const DWORD DATA_ALLOW =        0x80000000;
const DWORD DATA_DISALLOW =     0x40000000;

const WCHAR c_wszRobotsMetaName[] = L"Robots\n";
const int c_iRobotsMetaNameLen = 7;        // string len without nullterm

const WCHAR c_wszRobotsNoFollow[] = L"NoFollow";
const int c_iRobotsNoFollow = 8;

const WCHAR c_wszRobotsTxtURL[] = L"/robots.txt";

const DWORD MAX_ROBOTS_SIZE = 8192;         // Max size of robots.txt file

// tokens for parsing of robots.txt
const CHAR  c_szRobots_UserAgent[] = "User-Agent:";
const CHAR  c_szRobots_OurUserAgent[] = "MSIECrawler";
const CHAR  c_szRobots_Allow[] = "Allow:";
const CHAR  c_szRobots_Disallow[] = "Disallow:";

// This GUID comes from Trident and is a hack for getting PARAM values for APPLET tags.
DEFINE_GUID(CGID_JavaParambagCompatHack, 0x3050F405, 0x98B5, 0x11CF, 0xBB, 0x82, 0x00, 0xAA, 0x00, 0xBD, 0xCE, 0x0B);

// This GUID is helpfully not defined elsewhere.
DEFINE_GUID(CLSID_JavaVM, 0x08B0E5C0, 0x4FCB, 0x11CF, 0xAA, 0xA5, 0x00, 0x40, 0x1C, 0x60, 0x85, 0x01);

// Get host channel agent's subscription item, if any.
inline HRESULT CWebCrawler::GetChannelItem(ISubscriptionItem **ppChannelItem)
{
    IServiceProvider *pSP;
    HRESULT hr = E_NOINTERFACE;
    

    if (SUCCEEDED(m_pAgentEvents->QueryInterface(IID_IServiceProvider, (void **)&pSP)) && pSP)
    {
        ISubscriptionItem *pTempChannelItem = NULL;
        pSP->QueryService(CLSID_ChannelAgent, IID_ISubscriptionItem, (void **)&pTempChannelItem);
        pSP->Release();

        if(pTempChannelItem) 
            hr = S_OK;
            
        if(ppChannelItem)
            *ppChannelItem = pTempChannelItem;
        else
        {
            if(pTempChannelItem)
                pTempChannelItem->Release();    
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Helper functions - copied over from urlmon\download\helpers.cxx - Is there
// an equivalent routine or better place for this, webcrawl.cpp?
//
//////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------------
// %%Function: GetVersionFromString
//
//    converts version in text format (a,b,c,d) into two dwords (a,b), (c,d)
//    The printed version number is of format a.b.d (but, we don't care)
// ---------------------------------------------------------------------------
HRESULT
GetVersionFromString(const char *szBuf, LPDWORD pdwFileVersionMS, LPDWORD pdwFileVersionLS)
{
    const char *pch = szBuf;
    char ch;

    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    if (!pch)            // default to zero if none provided
        return S_OK;

    if (StrCmpA(pch, "-1,-1,-1,-1") == 0) {
        *pdwFileVersionMS = 0xffffffff;
        *pdwFileVersionLS = 0xffffffff;
    }

    USHORT n = 0;

    USHORT a = 0;
    USHORT b = 0;
    USHORT c = 0;
    USHORT d = 0;

    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;


    for (ch = *pch++;;ch = *pch++) {

        if ((ch == ',') || (ch == '\0')) {

            switch (have) {

            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                return E_INVALIDARG; // invalid arg
            }

            if (ch == '\0') {
                // all done convert a,b,c,d into two dwords of version

                *pdwFileVersionMS = ((a << 16)|b);
                *pdwFileVersionLS = ((c << 16)|d);

                return S_OK;
            }

            n = 0; // reset

        } else if ( (ch < '0') || (ch > '9'))
            return E_INVALIDARG;    // invalid arg
        else
            n = n*10 + (ch - '0');


    } /* end forever */

    // NEVERREACHED
}

/////////////////////////////////////////////////////////////////////////////////////////
// CombineBaseAndRelativeURLs -
//         Three URLs are combined by following rules (this is used for finding the URL
//         to load Applet CABs from.)  Three inputs, the Base URL, the Code Base URL
//         and the file name URL.
//
//         If file name URL is absolute return it.
//         Otherwise if CodeBase URL is absolute combine it with filename and return.
//         Otherwise if Base URL is absolute, combine CodeBase and fileName URL, then
//            combine with Base URL and return it.
////////////////////////////////////////////////////////////////////////////////////////

HRESULT CombineBaseAndRelativeURLs(LPCWSTR szBaseURL, LPCWSTR szRelative1, LPWSTR *szRelative2)
{

    WCHAR wszTemp[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = ARRAYSIZE(wszTemp);

    ASSERT(szRelative2);                // should never happen.
    if (szRelative2 == NULL)
        return E_FAIL;

    if (IsValidURL(NULL, *szRelative2, 0) == S_OK)
        return S_OK;

    if (szRelative1 && (IsValidURL(NULL, szRelative1, 0) == S_OK))
    {

        if (SUCCEEDED(UrlCombineW((LPCWSTR)szRelative1, (LPCWSTR)*szRelative2, (LPWSTR)wszTemp, &dwLen, 0)))
        {
            BSTR bstrNew = SysAllocString(wszTemp);
            if (bstrNew)
            {
                SAFEFREEBSTR(*szRelative2);
                *szRelative2 = bstrNew;
                return S_OK;
            }
        }
    }

    if (szBaseURL && (IsValidURL(NULL, szBaseURL, 0) == S_OK))
    {
        LPWSTR szNewRel = NULL;
        WCHAR wszCombined[INTERNET_MAX_URL_LENGTH];

        if (szRelative1)
        {
            // NOTE: lstr[cpy|cat]W are macroed to work on Win95.
            DWORD dwLen2 = lstrlenW(*szRelative2);
            StrCpyNW(wszTemp, szRelative1, ARRAYSIZE(wszTemp) - 1); //paranoia
            DWORD dwTempLen = lstrlenW(wszTemp);
            if ((dwLen2 > 0) && ((*szRelative2)[dwLen2-1] == (unsigned short)L'\\') ||
                                ((*szRelative2)[dwLen2-1] == (unsigned short) L'/'))
            {
                StrNCatW(wszTemp, *szRelative2, ARRAYSIZE(wszTemp) - dwTempLen);
            }
            else
            {
                StrNCatW(wszTemp, L"/", ARRAYSIZE(wszTemp) - dwTempLen);
                StrNCatW(wszTemp, *szRelative2, ARRAYSIZE(wszTemp) - dwTempLen - 1);
            }

            szNewRel = wszTemp;
        }
        else
        {
            szNewRel = *szRelative2;
        }

        dwLen = INTERNET_MAX_URL_LENGTH;
        if (SUCCEEDED(UrlCombineW((LPCWSTR)szBaseURL, (LPCWSTR)szNewRel, (LPWSTR)wszCombined, &dwLen, 0)))
        {
            BSTR bstrNew = SysAllocString(wszCombined);
            if (bstrNew)
            {
                SAFEFREEBSTR(*szRelative2);
                *szRelative2 = bstrNew;
                return S_OK;
            }
        }
    }

    // In all likelyhood one of the URL's in bad and nothing good can be done.
    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////
//
// CWebCrawler implementation
//
//////////////////////////////////////////////////////////////////////////

//
// CWebCrawler Helpers
//

CWebCrawler::CWebCrawler()
{
    DBG("Creating CWebCrawler object");

    InitializeCriticalSection(&m_critDependencies);
}

CWebCrawler::~CWebCrawler()
{
    _CleanUp();

    DeleteCriticalSection(&m_critDependencies);
    DBG("Destroyed CWebCrawler object");
}

void CWebCrawler::CleanUp()
{
    _CleanUp();
    CDeliveryAgent::CleanUp();
}

void CWebCrawler::_CleanUp()
{
    if (m_pCurDownload)
    {
        m_pCurDownload->DoneDownloading();
        m_pCurDownload->Release();
        m_pCurDownload = NULL;
    }

    CRunDeliveryAgent::SafeRelease(m_pRunAgent);

    SAFEFREEBSTR(m_bstrHostName);
    SAFEFREEBSTR(m_bstrBaseURL);
    SAFELOCALFREE(m_pszLocalDest);
    SAFELOCALFREE(m_pBuf);

    EnterCriticalSection(&m_critDependencies);
    SAFEDELETE(m_pDependencies);
    LeaveCriticalSection(&m_critDependencies);
    if (m_pDownloadNotify)
    {
        m_pDownloadNotify->LeaveMeAlone();
        m_pDownloadNotify->Release();
        m_pDownloadNotify=NULL;
    }

    SAFEDELETE(m_pPages);
    SAFEDELETE(m_pPendingLinks);
    SAFEDELETE(m_pDependencyLinks);
    SAFERELEASE(m_pUrlIconHelper);

    FreeRobotsTxt();
    FreeCodeBaseList();
}

// Format of m_pRobotsTxt:
// Array of hostnames for which we have attempted to get Robots.txt
// DWORD for each hostname contains pointer to CDwordStringList of Robots.txt data,
//  or 0 if we couldn't find robots.txt for that host name
// Robots.txt data stored in form: url, flag = allow or disallow
void CWebCrawler::FreeRobotsTxt()
{
    if (m_pRobotsTxt)
    {
        DWORD_PTR dwPtr;
        int iLen = m_pRobotsTxt->NumStrings();
        for (int i=0; i<iLen; i++)
        {
            dwPtr = m_pRobotsTxt->GetStringData(i);
            if (dwPtr)
            {
                delete ((CWCStringList *)dwPtr);
                m_pRobotsTxt->SetStringData(i, 0);
            }
        }

        delete m_pRobotsTxt;
        m_pRobotsTxt = NULL;
    }
}

void CWebCrawler::FreeCodeBaseList()
{
    if (m_pCodeBaseList) {
        CCodeBaseHold *pcbh;
        int iLen = m_pCodeBaseList->NumStrings();
        for (int i=0; i<iLen; i++)
        {
            pcbh = (CCodeBaseHold *)m_pCodeBaseList->GetStringData(i);
            if (pcbh != NULL)
            {
                SAFEFREEBSTR(pcbh->szDistUnit);
                SAFEDELETE(pcbh);
                m_pCodeBaseList->SetStringData(i, 0);
            }
        }

        SAFEDELETE(m_pCodeBaseList);
    }
}

HRESULT CWebCrawler::StartOperation()
{
    ISubscriptionItem *pItem = m_pSubscriptionItem;

    DWORD           dwTemp;

    ASSERT(pItem);

    DBG("CWebCrawler in StartOperation");

    if (m_pCurDownload || GetBusy())
    {
        DBG_WARN("Webcrawl busy, returning failure");
        return E_FAIL;
    }

    SAFEFREEBSTR(m_bstrBaseURL);
    if (FAILED(
        ReadBSTR(pItem, c_szPropURL, &m_bstrBaseURL)) ||
        !m_bstrBaseURL ||
        !CUrlDownload::IsValidURL(m_bstrBaseURL))
    {
        DBG_WARN("Couldn't get valid URL, aborting");
        SetEndStatus(E_INVALIDARG);
        SendUpdateNone();
        return E_INVALIDARG;
    }

    if (SHRestricted2W(REST_NoSubscriptionContent, NULL, 0))
        SetAgentFlag(FLAG_CHANGESONLY);

    if (IsAgentFlagSet(FLAG_CHANGESONLY))
    {
        m_dwRecurseLevels = 0;
        m_dwRecurseFlags = WEBCRAWL_DONT_MAKE_STICKY;
        DBG("Webcrawler is in 'changes only' mode.");
    }
    else
    {
/*
        BSTR bstrLocalDest=NULL;
        SAFELOCALFREE(m_pszLocalDest);
        ReadBSTR(c_szPropCrawlLocalDest, &bstrLocalDest);
        if (bstrLocalDest && bstrLocalDest[0])
        {
            int iLen = SysStringByteLen(bstrLocalDest)+1;
            m_pszLocalDest = (LPTSTR) MemAlloc(LMEM_FIXED, iLen);
            if (m_pszLocalDest)
            {
                MyOleStrToStrN(m_pszLocalDest, iLen, bstrLocalDest);
            }
        }
        SAFEFREEBSTR(bstrLocalDest);
*/

        m_dwRecurseLevels=0;
        ReadDWORD(pItem, c_szPropCrawlLevels, &m_dwRecurseLevels);

        if (!IsAgentFlagSet(DELIVERY_AGENT_FLAG_NO_RESTRICTIONS))
        {
            // Note: MaxWebcrawlLevels is stored as N+1 because 0
            // disables the restriction
            dwTemp = SHRestricted2W(REST_MaxWebcrawlLevels, NULL, 0);
            if (dwTemp && m_dwRecurseLevels >= dwTemp)
                m_dwRecurseLevels = dwTemp - 1;
        }

        m_dwRecurseFlags=0;
        ReadDWORD(pItem, c_szPropCrawlFlags, &m_dwRecurseFlags);

        // Read max size in cache in KB
        m_dwMaxSize=0;
        ReadDWORD(pItem, c_szPropCrawlMaxSize, &m_dwMaxSize);
        if (!IsAgentFlagSet(DELIVERY_AGENT_FLAG_NO_RESTRICTIONS))
        {
            dwTemp = SHRestricted2W(REST_MaxSubscriptionSize, NULL, 0);
            if (dwTemp && (!m_dwMaxSize || m_dwMaxSize > dwTemp))
                m_dwMaxSize = dwTemp;
        }

        if (IsRecurseFlagSet(WEBCRAWL_DONT_MAKE_STICKY))
            dwTemp = 0;

        // Read old group ID
        ReadLONGLONG(pItem, c_szPropCrawlGroupID, &m_llOldCacheGroupID);

        // Read new ID if present
        m_llCacheGroupID = 0;
        ReadLONGLONG(pItem, c_szPropCrawlNewGroupID, &m_llCacheGroupID);
        if (m_llCacheGroupID)
        {
            DBG("Adding to existing cache group");
        }
    } // !ChangesOnly

    // finish initializing new operation
    m_iDownloadErrors = 0;
    m_dwCurSize = 0;
    m_lMaxNumUrls = (m_dwRecurseLevels) ? -1 : 1;
    SAFEFREEBSTR(m_bstrHostName);

    m_dwCurSize = NULL;
    m_pPages = NULL;
    m_pDependencies = NULL;

    // After calling this, we'll reenter either in "StartDownload" (connection successful)
    //  or in "AbortUpdate" with GetEndStatus() == INET_E_AGENT_CONNECTION_FAILED
    return CDeliveryAgent::StartOperation();
}

HRESULT CWebCrawler::AgentPause(DWORD dwFlags)
{
    DBG("CWebCrawler::AgentPause");

    // Abort our current url
    if (m_pRunAgent)
    {
        m_pRunAgent->AgentPause(dwFlags);
    }

    if (m_pCurDownload)
    {
        m_pCurDownload->AbortDownload();
        m_pCurDownload->DestroyBrowser();
    }

    return CDeliveryAgent::AgentPause(dwFlags);
}

HRESULT CWebCrawler::AgentResume(DWORD dwFlags)
{
    DBG("CWebCrawler::AgentResume");

    if (m_pRunAgent)
    {
        m_pRunAgent->AgentResume(dwFlags);
    }
    else
    {
        // If we just increased our cache size, reprocess same url
        if (SUBSCRIPTION_AGENT_RESUME_INCREASED_CACHE & dwFlags)
        {
            DBG("CWebCrawler reprocessing same url after cache size increase");
            OnDownloadComplete(0, _ERROR_REPROCESSING);
        }
        else
        {
            // If we're not still downloading, restart our same url
            if (0 == m_iNumPagesDownloading)
            {
                if (FAILED(ActuallyStartDownload(m_pCurDownloadStringList, m_iCurDownloadStringIndex, TRUE)))
                {
                    ASSERT_MSG(0, "CWebCrawler::AgentResume"); // this should never happen
                    SetEndStatus(E_FAIL);
                    CleanUp();
                }
            }
        }
    }

    return CDeliveryAgent::AgentResume(dwFlags);
}

// Forcibly abort current operation
HRESULT CWebCrawler::AgentAbort(DWORD dwFlags)
{
    DBG("CWebCrawler::AgentAbort");

    if (m_pCurDownload)
    {
        m_pCurDownload->DoneDownloading();
    }

    if (m_pRunAgent)
    {
        m_pRunAgent->AgentAbort(dwFlags);
    }

    return CDeliveryAgent::AgentAbort(dwFlags);
}

//---------------------------------------------------------------
//

HRESULT CWebCrawler::StartDownload()
{
    ASSERT(!m_pCurDownload);

    m_iPagesStarted = 0;
    m_iRobotsStarted = 0;
    m_iDependencyStarted = 0;
    m_iDependenciesProcessed = 0;
    m_iTotalStarted = 0;
    m_iCodeBaseStarted = 0;
    m_iNumPagesDownloading = 0;

    // Create new cache group
    if (IsAgentFlagSet(FLAG_CHANGESONLY))
    {
        m_llCacheGroupID = 0;
    }
    else
    {
        if (!m_llCacheGroupID)
        {
            m_llCacheGroupID = CreateUrlCacheGroup(
                (IsRecurseFlagSet(WEBCRAWL_DONT_MAKE_STICKY) ? 0 : CACHEGROUP_FLAG_NONPURGEABLE), 0);

            ASSERT_MSG(m_llCacheGroupID != 0, "Create cache group failed");
        }
    }

    // Create string lists
    m_pPages = new CWCDwordStringList;
    if (m_pPages)
        m_pPages->Init(m_dwRecurseLevels ? -1 : 512);
    else
        SetEndStatus(E_FAIL);

    if (m_dwRecurseLevels && !IsRecurseFlagSet(WEBCRAWL_IGNORE_ROBOTSTXT))
    {
        m_pRobotsTxt = new CWCDwordStringList;
        if (m_pRobotsTxt)
            m_pRobotsTxt->Init(512);
        else
            SetEndStatus(E_FAIL);
    }

    // BUGBUG : Shouldn't allocate this memory in changes only mode
    m_pCodeBaseList = new CWCDwordStringList;
    if (m_pCodeBaseList)
        m_pCodeBaseList->Init(512);
    else
        SetEndStatus(E_FAIL);

    // Avoid duplicate processing of dependencies
    if (!IsAgentFlagSet(FLAG_CHANGESONLY))
    {
        m_pDependencies = new CWCDwordStringList;
        if (m_pDependencies)
            m_pDependencies->Init();
        else
            SetEndStatus(E_FAIL);
    }

    if (GetEndStatus() == E_FAIL)
        return E_FAIL;

    m_pCurDownload = new CUrlDownload(this, 0);
    if (!m_pCurDownload)
        return E_OUTOFMEMORY;

    // Add first URL to string list, then start it
    if ((CWCStringList::STRLST_ADDED == m_pPages->AddString(m_bstrBaseURL, m_dwRecurseLevels)) &&
        m_pPages->NumStrings() == 1)
    {
        return StartNextDownload();
    }

    SetEndStatus(E_FAIL);
    return E_FAIL;
}

// Attempts to begin the next download
HRESULT CWebCrawler::StartNextDownload()
{
    if (!m_pPages || m_iNumPagesDownloading)
        return E_FAIL;

    CWCStringList *pslUrls = NULL;
    int iIndex = 0;

    // See if we have any more URLs to download.
    // Check dependency links first
    if (m_pDependencyLinks)
    {
        ProcessDependencyLinks(&pslUrls, &iIndex);
#ifdef DEBUG
        if (pslUrls) DBG("Downloading dependency link (frame):");
#endif
    }

    if (!pslUrls)
    {
        // Check robots.txt
        if (m_pRobotsTxt && (m_iRobotsStarted < m_pRobotsTxt->NumStrings()))
        {
            pslUrls = m_pRobotsTxt;
            iIndex = m_iRobotsStarted ++;
        }
        else if (m_pPendingLinks)   // add pending links to pages list
        {
            // Pending links to process and we've retrieved all robots.txt
            // Process pending links (validate & add to download list)
            ProcessPendingLinks();
        }

        if (!pslUrls && (m_iPagesStarted < m_pPages->NumStrings()))
        {
            DWORD_PTR dwTmp;
            ASSERT(!m_pDependencyLinks);// should be downloaded already
            ASSERT(!m_pPendingLinks);   // should be validated already
            // Skip any pages we've started
            while (m_iPagesStarted < m_pPages->NumStrings())
            {
                dwTmp = m_pPages->GetStringData(m_iPagesStarted);
                if (IsFlagSet(dwTmp, DATA_DLSTARTED))
                    m_iPagesStarted++;
                else
                    break;
            }
            if (m_iPagesStarted < m_pPages->NumStrings())
            {
                pslUrls = m_pPages;
                iIndex = m_iPagesStarted ++;
            }
        }

        if (!pslUrls && (m_iCodeBaseStarted < m_pCodeBaseList->NumStrings()))
        {
            // Nothing else pull, do code bases last.

            while (m_iCodeBaseStarted < m_pCodeBaseList->NumStrings())
            {
                CCodeBaseHold *pcbh = (CCodeBaseHold *)
                                    m_pCodeBaseList->GetStringData(m_iCodeBaseStarted);
                if (IsFlagSet(pcbh->dwFlags, DATA_DLSTARTED))
                    m_iCodeBaseStarted++;
                else
                    break;
            }
            while (m_iCodeBaseStarted < m_pCodeBaseList->NumStrings())
            {
                // We have some codebases to download.
                // We return if the download is async and simply
                // start the next one if it finishes synchronously
                iIndex = m_iCodeBaseStarted;
                m_iCodeBaseStarted++; // increment so that next download is not repeated

                // Init the cur download infor for resume if paused
                m_iCurDownloadStringIndex = iIndex;
                m_pCurDownloadStringList = m_pCodeBaseList;
                
               if(ActuallyDownloadCodeBase(m_pCodeBaseList, iIndex, FALSE) == E_PENDING)
                    return S_OK; // We break out of the while and try next download in OnAgentEnd()

            }
        }
    }

    if (pslUrls)
    {
        m_iCurDownloadStringIndex = iIndex;
        m_pCurDownloadStringList = pslUrls;

        return ActuallyStartDownload(pslUrls, iIndex);
    }

    DBG("WebCrawler: StartNextDownload failing, nothing more to download.");
    return E_FAIL;
}

HRESULT CWebCrawler::ActuallyStartDownload(CWCStringList *pslUrls, int iIndex, BOOL fReStart /* = FALSE */)
{
    // We have urls to download. Do it.
    DWORD_PTR dwData;
    LPCWSTR pwszURL;
    DWORD   dwBrowseFlags;
    BDUMethod method;
    BDUOptions options;

    if(pslUrls == m_pCodeBaseList)
    {
        ASSERT(fReStart); // Should happen only with resume
        HRESULT hr = ActuallyDownloadCodeBase(m_pCodeBaseList, iIndex, fReStart);
        if(E_PENDING == hr)
            return S_OK;
        return E_FAIL; // hackhack - since we don't handle synchronous downloads well - we hang if 
                       // resumed download is synchronous
    }

    if (pslUrls != m_pRobotsTxt)
    {
        dwData = pslUrls->GetStringData(iIndex);
#ifdef DEBUG
        if (fReStart)
            if (~(dwData & DATA_DLSTARTED)) DBG_WARN("WebCrawler: Trying to restart one we haven't started yet!");
        else
            if ((dwData & DATA_DLSTARTED)) DBG_WARN("WebCrawler: Trying to download one we've already started?");
#endif
        pslUrls->SetStringData(iIndex, DATA_DLSTARTED | dwData);
    }
    else
        dwData = DATA_ROBOTSTXT;

    pwszURL = pslUrls->GetString(iIndex);

    ASSERT(iIndex < pslUrls->NumStrings());

#ifdef DEBUG
    int iMax = m_lMaxNumUrls;
    if (iMax<0)
        iMax = m_pPages->NumStrings() + ((m_pRobotsTxt) ? m_pRobotsTxt->NumStrings() : 0);
    TraceMsgA(TF_THISMODULE, "WebCrawler GET_URL (%d of %c%d) Recurse %d : %ws",
        m_iTotalStarted+1, ((m_lMaxNumUrls>0) ? ' ' : '?'), iMax,
        pslUrls->GetStringData(iIndex) & DATA_RECURSEMASK, pwszURL);
#endif

    dwBrowseFlags = DLCTL_DOWNLOADONLY |
        DLCTL_NO_FRAMEDOWNLOAD | DLCTL_NO_SCRIPTS | DLCTL_NO_JAVA |
        DLCTL_NO_RUNACTIVEXCTLS;

    if (IsRecurseFlagSet(WEBCRAWL_GET_IMAGES))      dwBrowseFlags |= DLCTL_DLIMAGES;
    if (IsRecurseFlagSet(WEBCRAWL_GET_VIDEOS))      dwBrowseFlags |= DLCTL_VIDEOS;
    if (IsRecurseFlagSet(WEBCRAWL_GET_BGSOUNDS))    dwBrowseFlags |= DLCTL_BGSOUNDS;
    if (!IsRecurseFlagSet(WEBCRAWL_GET_CONTROLS))   dwBrowseFlags |= DLCTL_NO_DLACTIVEXCTLS;
    if (IsRecurseFlagSet(WEBCRAWL_PRIV_OFFLINE_MODE))
    {
        dwBrowseFlags |= DLCTL_FORCEOFFLINE;
        dwBrowseFlags &= ~(DLCTL_DLIMAGES | DLCTL_VIDEOS | DLCTL_BGSOUNDS);
        DBG("GET is OFFLINE");
    }

    m_pCurDownload->SetDLCTL(dwBrowseFlags);

#ifdef DEBUG
    if (fReStart)
    {
        ASSERT(m_iCurDownloadStringIndex == iIndex);
        ASSERT(m_pCurDownloadStringList == pslUrls);
    }
#endif

    if (!fReStart)
    {
        // Get the info for change detection, unless we already know it's changed
        if (!IsAgentFlagSet(FLAG_CRAWLCHANGED) && !(dwData & DATA_ROBOTSTXT))
        {
            TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];

            m_varChange.vt = VT_EMPTY;

            if (IsAgentFlagSet(FLAG_CHANGESONLY))
            {
                // "Changes Only" mode, we have persisted a change detection code
                ASSERT(m_iTotalStarted == 0);
                LPCWSTR pPropChange = c_szPropChangeCode;
                m_pSubscriptionItem->ReadProperties(1, &pPropChange, &m_varChange);
            }

            BOOL fMustGET = TRUE;

            MyOleStrToStrN(szUrl, INTERNET_MAX_URL_LENGTH, pwszURL);
            PreCheckUrlForChange(szUrl, &m_varChange, &fMustGET);

            if (IsAgentFlagSet(FLAG_CHANGESONLY) && !fMustGET)
                SetAgentFlag(FLAG_HEADONLY);
        }

        m_iTotalStarted ++;
    }

    if (IsPaused())
    {
        DBG("WebCrawler paused, not starting another download");
        if (m_pCurDownload)
            m_pCurDownload->DestroyBrowser(); // free browser until resumed
        return E_PENDING;
    }

    m_iNumPagesDownloading ++;

    // Send our update progress with the url we're about to download
    SendUpdateProgress(pwszURL, m_iTotalStarted, m_lMaxNumUrls, (m_dwCurSize >> 10));

    if (IsAgentFlagSet(FLAG_HEADONLY))
    {
        ASSERT(m_iTotalStarted == 1);
        method = BDU2_HEADONLY;                 // Only get HEAD info with Urlmon
    }
    else if (IsAgentFlagSet(FLAG_CHANGESONLY)   // Only want HTML, or
        || m_pszLocalDest                       // We're going to move this one file, or
        || (dwData & DATA_ROBOTSTXT))           // This is a robots.txt, so
    {
        method = BDU2_URLMON;                   // Get with Urlmon
    }
    else if (m_iTotalStarted == 1)              // First file, we need status code, so
    {
        ISubscriptionItem *pCDFItem;
        method = BDU2_SNIFF;                    // Get with Urlmon then MSHTML (if HTML)

        // Find out if we're hosted by channel agent
        if (SUCCEEDED(GetChannelItem(&pCDFItem)))
        {
            // If we're hosted by channel agent, use its original hostname
            BSTR bstrBaseUrl;
            if (SUCCEEDED(ReadBSTR(pCDFItem, c_szPropURL, &bstrBaseUrl)))
            {
                GetHostName(bstrBaseUrl, &m_bstrHostName);
                SysFreeString(bstrBaseUrl);
            }
#ifdef DEBUG
            if (m_bstrHostName)
                TraceMsg(TF_THISMODULE, "Got host name from channel agent: %ws", m_bstrHostName);
#endif
            pCDFItem->Release();

            DBG("Using 'smart' mode for first url in webcrawl; spawned from channel crawl");
            method = BDU2_SMART;                // Use 'smart' mode for first url if channel crawl
            SetAgentFlag(FLAG_HOSTED);
        }
    }
    else
        method = BDU2_SMART;                    // Get with Urlmon or MSHTML as appropriate

    if (dwData & DATA_ROBOTSTXT)
        options = BDU2_NEEDSTREAM;              // Need IStream to parse robots.txt
    else
        options = BDU2_NONE;

    options |= BDU2_DOWNLOADNOTIFY_REQUIRED;    // Always get download notify callbacks

    if (IsRecurseFlagSet(WEBCRAWL_ONLY_LINKS_TO_HTML) && (dwData & DATA_LINK))
    {
        // Don't follow any links unless they are to html pages.
        options |= BDU2_FAIL_IF_NOT_HTML;
    }

    if (FAILED(m_pCurDownload->BeginDownloadURL2(pwszURL,
            method, options, m_pszLocalDest, 
            m_dwMaxSize ? (m_dwMaxSize<<10)-m_dwCurSize : 0)))
    {
        DBG("BeginDownloadURL2 failed (ignoring & waiting for OnDownloadComplete call)");
    }

    return S_OK;
}

HRESULT CWebCrawler::ActuallyDownloadCodeBase(CWCStringList *pslUrls, int iIndex, BOOL fReStart)
{
    CCodeBaseHold *pcbh;
    LPCWSTR pwszURL;
    HRESULT hr = S_OK;

    if (pslUrls != m_pCodeBaseList)
    {
        ASSERT(0);
        DBG_WARN("WebCrawler: Wrong URLs being processed as CodeBase.");
        hr = E_FAIL;
        goto Exit;
    }

    pcbh = (CCodeBaseHold *)pslUrls->GetStringData(iIndex);

#ifdef DEBUG
    if (fReStart)
        if (~(pcbh->dwFlags & DATA_DLSTARTED)) DBG_WARN("WebCrawler: Trying to restart CodeBase D/L we haven't started yet!");
    else
        if ((pcbh->dwFlags & DATA_DLSTARTED)) DBG_WARN("WebCrawler: Trying to download CodeBase D/L we've already started?");
#endif
    pcbh->dwFlags |= DATA_DLSTARTED;

    pwszURL = pslUrls->GetString(iIndex);

    ASSERT(iIndex < pslUrls->NumStrings());

    if (!fReStart)
        m_iTotalStarted ++;

    if (IsPaused())
    {
        DBG("WebCrawler paused, not starting another download");
        if (m_pCurDownload)
            m_pCurDownload->DestroyBrowser(); // free browser until resumed
        return S_FALSE;
    }

    m_iNumPagesDownloading ++;

    // Send our update progress with the CODEBASE we're about to download
    SendUpdateProgress(pwszURL, m_iTotalStarted, m_lMaxNumUrls);

    if (m_pRunAgent)
    {
        ASSERT(0);
        DBG_WARN("WebCrawler: Attempting to download next CODEBASE when not done last one.");
        hr = E_FAIL;
        goto Exit;
    }
    else
    {
        // create subscription item for CDL agent.

        ISubscriptionItem *pItem = NULL;

        if (m_dwMaxSize && ((m_dwCurSize>>10) >= m_dwMaxSize))
        {
            // We've exceeded our maximum download KB limit and can't continue.
            DBG_WARN("WebCrawler: Exceeded Maximum KB download limit with CodeBase download.");
            SetEndStatus(hr = INET_E_AGENT_MAX_SIZE_EXCEEDED);
            goto Exit;
        }

        if (!m_pSubscriptionItem ||
            FAILED(hr = DoCloneSubscriptionItem(m_pSubscriptionItem, NULL, &pItem)))
        {
            goto Exit;
        }
        ASSERT(pItem != NULL);

        WriteOLESTR(pItem, c_szPropURL, pwszURL);
        WriteOLESTR(pItem, L"DistUnit", pcbh->szDistUnit);
        WriteDWORD(pItem, L"VersionMS", pcbh->dwVersionMS);
        WriteDWORD(pItem, L"VersionLS", pcbh->dwVersionLS);
        if (m_dwMaxSize)
            WriteDWORD(pItem, c_szPropCrawlMaxSize, m_dwMaxSize - (m_dwCurSize>>10));    // KB limit for us to pull.

        m_pRunAgent = new CRunDeliveryAgent();
        if (m_pRunAgent)
            hr = m_pRunAgent->Init((CRunDeliveryAgentSink *)this, pItem, CLSID_CDLAgent);
        pItem->Release();

        if (m_pRunAgent && SUCCEEDED(hr))
        {
            hr = m_pRunAgent->StartAgent();
            //if (hr == E_PENDING)
            //{
                //hr = S_OK;
            //}
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

Exit:
    return hr;

}

HRESULT CWebCrawler::ProcessDependencyLinks(CWCStringList **ppslUrls, int *piStarted)
{
    ASSERT(ppslUrls && !*ppslUrls && piStarted);

    int iIndex;
    DWORD_PTR dwData;

    if (!m_pDependencyLinks)
        return S_FALSE;

    // See if we have any more dependency links to download
    while (m_iDependencyStarted < m_pDependencyLinks->NumStrings())
    {
        if (!m_pPages->FindString(m_pDependencyLinks->GetString(m_iDependencyStarted),
                               m_pDependencyLinks->GetStringLen(m_iDependencyStarted), &iIndex))
        {
            ASSERT(0);  // find string failed?!? We added it above!
            return E_FAIL;
        }

        ASSERT(iIndex>=0 && iIndex<m_pPages->NumStrings());

        m_iDependencyStarted ++;

        // See if we've downloaded this yet.
        dwData = m_pPages->GetStringData(iIndex);
        if (!(dwData & DATA_DLSTARTED))
        {
            // Nope. Start download.
            *ppslUrls = m_pPages;
            *piStarted = iIndex;
            return S_OK;
        }

        // We have already downloaded this page. Go to next dependency link.
    }

    // Done processing. Clear for next page.
    SAFEDELETE(m_pDependencyLinks);

    return S_FALSE;
}

HRESULT CWebCrawler::ProcessPendingLinks()
{
    int         iNumLinks, iAddCode, i, iAddIndex, iRobotsIndex;
    LPCWSTR     pwszUrl;
    BOOL        fAllow;

    if (!m_pPendingLinks)
        return S_FALSE;

    ASSERT(m_lMaxNumUrls<0);
    ASSERT(0 == (m_dwPendingRecurseLevel & ~DATA_RECURSEMASK));

    iNumLinks = m_pPendingLinks->NumStrings();

    TraceMsg(TF_THISMODULE, "Processing %d pending links from %ws",
        iNumLinks, m_pPages->GetString(m_iPagesStarted-1));

    // Add the links to our global page list
    for (i=0; i<iNumLinks; i++)
    {
        // Validate with robots.txt if appropriate
        pwszUrl = m_pPendingLinks->GetString(i);
        iRobotsIndex = (int)(m_pPendingLinks->GetStringData(i) & DATA_ROBOTSTXTMASK);
        ValidateWithRobotsTxt(pwszUrl, iRobotsIndex, &fAllow);

        if (fAllow)
        {
/*
As long as we retrieve pages in decreasing-recursion order (top to bottom), we don't
have to worry about bumping pages to a higher recurse level (except for frames).
*/
            iAddCode = m_pPages->AddString(pwszUrl,
                        DATA_LINK | m_dwPendingRecurseLevel,
                        &iAddIndex);
            if (iAddCode == CWCStringList::STRLST_FAIL)
                break;
        }
    }
    SAFEDELETE(m_pPendingLinks);

    return S_OK;
}


// Combine with our base url to get full url
// We use this for frames, but also for <Link> tags, since the processing is identical
HRESULT CWebCrawler::CheckFrame(IUnknown *punkItem, BSTR *pbstrItem, DWORD_PTR dwBaseUrl, DWORD *pdwStringData)
{
    WCHAR   wszCombined[INTERNET_MAX_URL_LENGTH];
    DWORD   dwLen = ARRAYSIZE(wszCombined);

    ASSERT(pbstrItem && *pbstrItem && punkItem && dwBaseUrl);
    if (!pbstrItem || !*pbstrItem || !punkItem || !dwBaseUrl)
        return E_FAIL;      // bogus

    if (SUCCEEDED(UrlCombineW((LPCWSTR)dwBaseUrl, *pbstrItem, wszCombined, &dwLen, 0)))
    {
        BSTR bstrNew = SysAllocString(wszCombined);

        if (bstrNew)
        {
            SysFreeString(*pbstrItem);
            *pbstrItem = bstrNew;
            return S_OK;
        }
    }

    TraceMsg(TF_WARNING, "CWebCrawler::CheckFrame failing. Not getting frame or <link> url=%ws.", *pbstrItem);
    return E_FAIL;  // Couldn't combine url; don't add
}

// See if we should follow this link. Clears pbstrItem if not.
// Accepts either pLink or pArea
HRESULT CWebCrawler::CheckLink(IUnknown *punkItem, BSTR *pbstrItem, DWORD_PTR dwThis, DWORD *pdwStringData)
{
    HRESULT         hrRet = S_OK;
    CWebCrawler    *pThis = (CWebCrawler *)dwThis;

    ASSERT(pbstrItem && *pbstrItem && punkItem && dwThis);
    if (!pbstrItem || !*pbstrItem || !punkItem || !dwThis)
        return E_FAIL;      // bogus

    // First see if it's 'valid'
    // We only add the link if it's HTTP (or https)
    // (we don't want to get mailto: links, for example)
    if (CUrlDownload::IsValidURL(*pbstrItem))
    {
        // Strip off any anchor
        CUrlDownload::StripAnchor(*pbstrItem);
    }
    else
    {
        // Skip this link
        SysFreeString(*pbstrItem);
        *pbstrItem = NULL;
        return S_FALSE;
    }

    if (pThis->IsRecurseFlagSet(WEBCRAWL_ONLY_LINKS_TO_HTML))
    {
        // See if we can tell that this is not an HTML link
        if (CUrlDownload::IsNonHtmlUrl(*pbstrItem))
        {
            // Skip this link
            SysFreeString(*pbstrItem);
            *pbstrItem = NULL;
            return S_FALSE;
        }
    }

    if (!(pThis->IsRecurseFlagSet(WEBCRAWL_LINKS_ELSEWHERE)))
    {
        BSTR bstrHost=NULL;
        IHTMLAnchorElement *pLink=NULL;
        IHTMLAreaElement *pArea=NULL;

        // Check to see if the host names match
        punkItem->QueryInterface(IID_IHTMLAnchorElement, (void **)&pLink);

        if (pLink)
        {
            pLink->get_hostname(&bstrHost);
            pLink->Release();
        }
        else
        {
            punkItem->QueryInterface(IID_IHTMLAreaElement, (void **)&pArea);

            if (pArea)
            {
                pArea->get_hostname(&bstrHost);
                pArea->Release();
            }
            else
            {
                DBG_WARN("CWebCrawler::CheckLink Unable to get Area or Anchor interface!");
                return E_FAIL;      // Bad element
            }
        }

        if (!bstrHost || !*bstrHost)
        {
            DBG_WARN("CWebCrawler::CheckLink : (pLink|pArea)->get_hostname() failed");
            hrRet = S_OK;      // always accept if get_hostname fails
        }
        else
        {
            if (pThis->m_bstrHostName && MyAsciiCmpW(bstrHost, pThis->m_bstrHostName))
            {
                // Skip url; different host name.
                SAFEFREEBSTR(*pbstrItem);
                hrRet = S_FALSE;
            }
        }

        SAFEFREEBSTR(bstrHost);
    }

    if (*pbstrItem && pdwStringData)
    {
        pThis->GetRobotsTxtIndex(*pbstrItem, TRUE, pdwStringData);
        *pdwStringData &= DATA_ROBOTSTXTMASK;
    }
    else if (pdwStringData)
        *pdwStringData = 0;

    return hrRet;
}

// S_OK    : Already retrieved this robots.txt info
// S_FALSE : Haven't yet retrieved this robots.txt info
// E_*     : Bad
HRESULT CWebCrawler::GetRobotsTxtIndex(LPCWSTR pwszUrl, BOOL fAddToList, DWORD *pdwRobotsTxtIndex)
{
    HRESULT hr=S_OK;
    int    iIndex=-1;

    if (m_pRobotsTxt)
    {
        // See which robots.txt file we should use to validate this link
        // If not yet available, add it to the list to be downloaded
        DWORD  dwBufLen = lstrlenW(pwszUrl) + ARRAYSIZE(c_wszRobotsTxtURL); //This get's us a terminating NULL
        LPWSTR pwszRobots = (LPWSTR)MemAlloc(LMEM_FIXED, dwBufLen * sizeof(WCHAR));
        int    iAddCode;

        if (pwszRobots)
        {
            // PERF: do the internetcombine in startnextdownload
            if (SUCCEEDED(UrlCombineW(pwszUrl, c_wszRobotsTxtURL, pwszRobots, &dwBufLen, 0))
                && !memcmp(pwszRobots, L"http", 4 * sizeof(WCHAR)))
            {
                if (fAddToList)
                {
                    iAddCode = m_pRobotsTxt->AddString(pwszRobots, 0, &iIndex);
                }
                else
                {
                    if (m_pRobotsTxt->FindString(pwszRobots, -1, &iIndex))
                    {
                        iAddCode = CWCStringList::STRLST_DUPLICATE;
                    }
                    else
                    {
                        iIndex=-1;
                        iAddCode = CWCStringList::STRLST_FAIL;
                    }
                }

                if (CWCStringList::STRLST_FAIL == iAddCode)
                    hr = E_FAIL;    // bad news
                else if (CWCStringList::STRLST_ADDED == iAddCode)
                    hr = S_FALSE;   // haven't gotten it yet
                else
                    hr = S_OK;      // already got it
            }
            MemFree(pwszRobots);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = E_FAIL;    // too many robots.txt files???
    }

    *pdwRobotsTxtIndex = iIndex;

    return hr;
}

// iRobotsIndex : Index into robots.txt, -1 if unavailable
HRESULT CWebCrawler::ValidateWithRobotsTxt(LPCWSTR pwszUrl, int iRobotsIndex, BOOL *pfAllow)
{
    int iNumDirectives, i;
    CWCStringList *pslThisRobotsTxt=NULL;

    *pfAllow = TRUE;

    if (!m_pRobotsTxt)
        return S_OK;

    if (iRobotsIndex == -1)
    {
        DWORD dwIndex;

        if (S_OK != GetRobotsTxtIndex(pwszUrl, FALSE, &dwIndex))
            return E_FAIL;

        iRobotsIndex = (int)dwIndex;
    }

    if ((iRobotsIndex >= 0) && iRobotsIndex<m_pRobotsTxt->NumStrings())
    {
        pslThisRobotsTxt = (CWCStringList *)(m_pRobotsTxt->GetStringData(iRobotsIndex));

        if (pslThisRobotsTxt)
        {
            iNumDirectives = pslThisRobotsTxt->NumStrings();

            for (i=0; i<iNumDirectives; i++)
            {
                // See if this url starts with the same thing as the directive
                if (!MyAsciiCmpNIW(pwszUrl, pslThisRobotsTxt->GetString(i), pslThisRobotsTxt->GetStringLen(i)))
                {
                    // hit! see if this is "allow" or "disallow"
                    if (!(pslThisRobotsTxt->GetStringData(i) & DATA_ALLOW))
                    {
                        TraceMsg(TF_THISMODULE, "ValidateWithRobotsTxt disallowing: (%ws) (%ws)",
                            pslThisRobotsTxt->GetString(i), pwszUrl);
                        *pfAllow = FALSE;
                        m_iSkippedByRobotsTxt ++;
                    }
                    break;
                }
            }
        }
        return S_OK;
    }

    return E_FAIL;
}

typedef struct
{
    LPCWSTR         pwszThisUrl;
    CWCStringList   *pslGlobal;
    BOOL            fDiskFull;
    DWORD           dwSize;
    GROUPID         llGroupID;
}
ENUMDEPENDENCIES;

// Doesn't process it if we already have it in the global dependency list
HRESULT CWebCrawler::CheckImageOrLink(IUnknown *punkItem, BSTR *pbstrItem, DWORD_PTR dwEnumDep, DWORD *pdwStringData)
{
    if (!dwEnumDep)
        return E_FAIL;

    ENUMDEPENDENCIES *pEnumDep = (ENUMDEPENDENCIES *) dwEnumDep;

    WCHAR   wszCombinedUrl[INTERNET_MAX_URL_LENGTH];
    DWORD   dwLen = ARRAYSIZE(wszCombinedUrl);

    HRESULT hr;

    if (pEnumDep->fDiskFull)
        return E_ABORT;     // Abort enumeration

    if (SUCCEEDED(UrlCombineW(pEnumDep->pwszThisUrl, *pbstrItem, wszCombinedUrl, &dwLen, 0)))
    {
        TCHAR   szCombinedUrl[INTERNET_MAX_URL_LENGTH];
        BYTE    chBuf[MY_MAX_CACHE_ENTRY_INFO];

        if (pEnumDep->pslGlobal != NULL)
        {
            int iCode = pEnumDep->pslGlobal->AddString(*pbstrItem, 0);

            if (CWCStringList::STRLST_ADDED != iCode)
            {
                // The string already existed (or Add failed). Don't process this.
                return S_OK;
            }
        }

        // Process this url.
        MyOleStrToStrN(szCombinedUrl, INTERNET_MAX_URL_LENGTH, wszCombinedUrl);

        hr = GetUrlInfoAndMakeSticky(NULL, szCombinedUrl,
                (LPINTERNET_CACHE_ENTRY_INFO)chBuf, sizeof(chBuf),
                pEnumDep->llGroupID);

        if (E_OUTOFMEMORY == hr)
        {
            pEnumDep->fDiskFull = TRUE;
            return E_ABORT;     // Skip rest of enumeration
        }

        if (SUCCEEDED(hr))
            pEnumDep->dwSize += ((LPINTERNET_CACHE_ENTRY_INFO)chBuf)->dwSizeLow;
    }

    return S_OK;
}

HRESULT CWebCrawler::MatchNames(BSTR bstrName, BOOL fPassword)
{
    static const WCHAR c_szPassword1[] = L"password";
    static const WCHAR c_szUsername1[] = L"user";
    static const WCHAR c_szUsername2[] = L"username";

    HRESULT hr = E_FAIL;
    LPCTSTR pszKey = c_szRegKeyPasswords;

    // See if the name matches our preset options.
    // Should these be localized?  I don't think so or subscribing to
    // US sites will fail in international versions of the browser.
    if (fPassword)
    {
        if (StrCmpIW(bstrName, c_szPassword1) == 0)
        {
            hr = S_OK;
        }
    }
    else
    {
        if ((StrCmpIW(bstrName, c_szUsername1) == 0) ||
            (StrCmpIW(bstrName, c_szUsername2) == 0))
        {
            hr = S_OK;
        }
        else
        {
            pszKey = c_szRegKeyUsernames;
        }
    }

    // Try the registry for custom form names if the presets didn't match.
    if (FAILED(hr))
    {
        LONG lRes;
        HKEY hKey;
        DWORD cValues;
        DWORD i;
        lRes = RegOpenKeyEx(HKEY_CURRENT_USER, pszKey, 0, KEY_READ, &hKey);
        if (ERROR_SUCCESS == lRes)
        {
            lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL);
            if (ERROR_SUCCESS == lRes)
            {
                for (i = 0; i < cValues; i++)
                {
                    TCHAR szValueName[MAX_PATH];
                    DWORD cchValueName = ARRAYSIZE(szValueName);

                    lRes = SHEnumValue(hKey, i, szValueName, &cchValueName, NULL, NULL, NULL);
                    if (ERROR_SUCCESS == lRes)
                    {
                        WCHAR wszValueName[MAX_PATH];
                        MyStrToOleStrN(wszValueName, ARRAYSIZE(wszValueName), szValueName);
                        if (StrCmpIW(bstrName, wszValueName) == 0)
                        {
                            hr = S_OK;
                            break;
                        }
                    }
                }
            }
            lRes = RegCloseKey(hKey);
            ASSERT(ERROR_SUCCESS == lRes);
        }
    }

    return hr;
}

HRESULT CWebCrawler::FindAndSubmitForm(void)
{
    // FindAndSubmitForm - If there is a user name and password in
    // the start item, this will attempt to fill in and submit
    // a form.  It should only be called on the top level page of a
    // webcrawl. We still need to check the host name in case we were
    // spawned from a channel crawl.
    //
    // return values: S_OK      successfully found and submitted a form -> restart webcrawl
    //                S_FALSE   no username, no form, or unrecognized form ->continue webcrawl
    //                E_FAIL    submit failed -> abort webcrawl
    //
    HRESULT hrReturn = S_FALSE;
    HRESULT hr = S_OK;
    BSTR bstrUsername = NULL;
    BSTR bstrPassword = NULL;
    BSTR bstrInputType= NULL;

    static const WCHAR c_szInputTextType[]=L"text";

    // If our host name doesn't match the root host name, don't return auth
    // information.
    if (m_bstrHostName)
    {
        LPWSTR pwszUrl, bstrHostName=NULL;

        m_pCurDownload->GetRealURL(&pwszUrl);   // may re-enter Trident

        if (pwszUrl)
        {
            GetHostName(pwszUrl, &bstrHostName);
            LocalFree(pwszUrl);
        }

        if (bstrHostName)
        {
            if (MyAsciiCmpW(bstrHostName, m_bstrHostName))
            {
                hr = E_FAIL;
            }
            SysFreeString(bstrHostName);
        }
    }

    if (SUCCEEDED(hr))
        hr = ReadBSTR(m_pSubscriptionItem, c_szPropCrawlUsername, &bstrUsername);

    if (SUCCEEDED(hr) && bstrUsername && bstrUsername[0])
    {
        // BUGBUG: We don't allow NULL passwords.
        hr = ReadPassword(m_pSubscriptionItem, &bstrPassword);
        if (SUCCEEDED(hr) && bstrPassword && bstrPassword[0])
        {
            IHTMLDocument2 *pDoc = NULL;
            hr = m_pCurDownload->GetDocument(&pDoc);
            if (SUCCEEDED(hr) && pDoc)
            {
                IHTMLElementCollection *pFormsCollection = NULL;
                hr = pDoc->get_forms(&pFormsCollection);
                if (SUCCEEDED(hr) && pFormsCollection)
                {
                    long length;
                    hr = pFormsCollection->get_length(&length);
                    TraceMsg(TF_THISMODULE, "**** FOUND USER NAME, PASSWORD, & %d FORMS ****", (int)length);
                    if (SUCCEEDED(hr) && length > 0)
                    {
                        // BUGBUG: We only check the first form for a user name and password.
                        // BUGBUG: Why do we pass an index to IHTMLElementCollection when
                        // the interface prototype says it takes a name?
                        IDispatch *pDispForm = NULL;
                        VARIANT vIndex, vEmpty;
                        VariantInit(&vIndex);
                        VariantInit(&vEmpty);
                        vIndex.vt = VT_I4;
                        vIndex.lVal = 0;
                        hr = pFormsCollection->item(vIndex, vEmpty, &pDispForm);
                        if (SUCCEEDED(hr) && pDispForm)
                        {
                            IHTMLFormElement *pForm = NULL;
                            hr = pDispForm->QueryInterface(IID_IHTMLFormElement, (void **)&pForm);
                            if (SUCCEEDED(hr) && pForm)
                            {
                                // Enum form elements looking for the input types we care about.
                                // BUGBUG: Would it be faster to use tags()?
                                hr = pForm->get_length(&length);
                                if (SUCCEEDED(hr) && length >= 2)
                                {
                                    // TraceMsg(TF_THISMODULE, "**** FORM ELEMENTS (%d) ****", (int)length);
                                    BOOL fUsernameSet = FALSE;
                                    BOOL fPasswordSet = FALSE;
                                    IDispatch *pDispItem = NULL;
                                    long i;
                                    for (i = 0; i < length; i++)
                                    {
                                        vIndex.lVal = i;    // re-use vIndex above
                                        hr = pForm->item(vIndex, vEmpty, &pDispItem);
                                        if (SUCCEEDED(hr) && pDispItem)
                                        {
                                            IHTMLInputTextElement *pInput = NULL;
                                            // BUGBUG: QI was the easiest way to tell them apart...
                                            // InputText is derived from InputPassword
                                            hr = pDispItem->QueryInterface(IID_IHTMLInputTextElement, (void **)&pInput);
                                            SAFERELEASE(pDispItem);
                                            if (SUCCEEDED(hr) && pInput)
                                            {
                                                hr = pInput->get_type(&bstrInputType);
                                                ASSERT(SUCCEEDED(hr) && bstrInputType);
                                                BSTR bstrName = NULL;
                                                if (StrCmpIW(bstrInputType, c_szInputTextType) == 0)
                                                {
                                                    // We found an INPUT element with attribute TYPE="text".
                                                    // Set it if the NAME attribute matches.
                                                    // BUGBUG: Only setting the first matching input.
                                                    // BUGBUG: Do we care about max length or does put_value handle it?
                                                    // TraceMsg(TF_THISMODULE, "**** FORM ELEMENT INPUT (%d) ****", (int)i);
                                                    if (!fUsernameSet)
                                                    {
                                                        hr = pInput->get_name(&bstrName);
                                                        ASSERT(SUCCEEDED(hr) && bstrName);
                                                        if (SUCCEEDED(hr) && bstrName && SUCCEEDED(MatchNames(bstrName, FALSE)))
                                                        {
                                                            hr = pInput->put_value(bstrUsername);
                                                            if (SUCCEEDED(hr))
                                                                fUsernameSet = TRUE;
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    // We found an INPUT element with attribute TYPE="password"
                                                    // Set it if the name attribute matches.
                                                    // BUGBUG: Only setting the first matching input.
                                                    // BUGBUG: Do we care about max length or does put_value handle it?
                                                    // TraceMsg(TF_THISMODULE, "**** FORM ELEMENT PASSWORD (%d) ****", (int)i);
                                                    if (!fPasswordSet)
                                                    {
                                                        hr = pInput->get_name(&bstrName);
                                                        ASSERT(SUCCEEDED(hr) && bstrName);
                                                        if (SUCCEEDED(hr) && bstrName  && SUCCEEDED(MatchNames(bstrName, TRUE)))
                                                        {
                                                            hr = pInput->put_value(bstrPassword);
                                                            if (SUCCEEDED(hr))
                                                                fPasswordSet = TRUE;
                                                        }
                                                    }
                                                }
                                                SAFEFREEBSTR(bstrName);
                                                SAFERELEASE(pInput);
                                            }
                                        }
                                    }
                                    // Submit the form is everything was set.
                                    if (fUsernameSet && fPasswordSet)
                                    {
                                        ASSERT(!m_pCurDownload->GetFormSubmitted());
                                        m_pCurDownload->SetFormSubmitted(TRUE);
                                        hr = pForm->submit();
                                        if (SUCCEEDED(hr))
                                        {
                                            m_iNumPagesDownloading ++;
                                            TraceMsg(TF_THISMODULE, "**** FORM SUBMIT WORKED ****");
                                            hrReturn = S_OK;
                                        }
                                        else
                                        {
                                            TraceMsg(TF_THISMODULE, "**** FORM SUBMIT FAILED ****");
                                            hrReturn = E_FAIL;
                                        }
                                    }
                                }
                                SAFERELEASE(pForm);
                            }
                            SAFERELEASE(pDispForm);
                        }
                        // only length
                    }
                    SAFERELEASE(pFormsCollection);
                }
                SAFERELEASE(pDoc);
            }
            // free bstr below because we check for empty bstrs
        }
        SAFEFREEBSTR(bstrPassword);
    }
    SAFEFREEBSTR(bstrUsername);
    return hrReturn;
}

// Make page and dependencies sticky and get total size
HRESULT CWebCrawler::MakePageStickyAndGetSize(LPCWSTR pwszURL, DWORD *pdwSize, BOOL *pfDiskFull)
{
    ASSERT(m_pDependencies || IsRecurseFlagSet(WEBCRAWL_DONT_MAKE_STICKY));

    HRESULT hr;
    TCHAR   szThisUrl[INTERNET_MAX_URL_LENGTH]; // bugbug use ansi internally
    BYTE    chBuf[MY_MAX_CACHE_ENTRY_INFO];

    LPINTERNET_CACHE_ENTRY_INFO lpInfo = (LPINTERNET_CACHE_ENTRY_INFO) chBuf;

    DWORD   dwBufSize = sizeof(chBuf);

    *pdwSize = 0;

    // First we make our base url sticky and check it for changes

    MyOleStrToStrN(szThisUrl, INTERNET_MAX_URL_LENGTH, pwszURL);

    hr = GetUrlInfoAndMakeSticky(NULL, szThisUrl, lpInfo, dwBufSize, m_llCacheGroupID);

    if (E_OUTOFMEMORY != hr)
    {
        if (SUCCEEDED(hr))
            *pdwSize += lpInfo->dwSizeLow;

        if (!IsAgentFlagSet(FLAG_CRAWLCHANGED) && SUCCEEDED(hr))
        {
            hr = PostCheckUrlForChange(&m_varChange, lpInfo, lpInfo->LastModifiedTime);
            // If we FAILED, we mark it as changed.
            if (hr == S_OK || FAILED(hr))
            {
                SetAgentFlag(FLAG_CRAWLCHANGED);
                DBG("URL has changed; will flag webcrawl as changed");
            }

            // "Changes Only" mode, persist change detection code
            if (IsAgentFlagSet(FLAG_CHANGESONLY))
            {
                ASSERT(m_iTotalStarted == 1);
                WriteVariant(m_pSubscriptionItem, c_szPropChangeCode, &m_varChange);
                return S_OK;    // We know there are no dependencies
            }

            hr = S_OK;
        }
    }
    else
    {
        *pfDiskFull = TRUE;
    }

    // Now we make all the new dependencies we downloaded for this page sticky
    if (!*pfDiskFull && m_pDependencies)
    {
        EnterCriticalSection(&m_critDependencies);

        for (; m_iDependenciesProcessed < m_pDependencies->NumStrings(); m_iDependenciesProcessed ++)
        {
            MyOleStrToStrN(szThisUrl, INTERNET_MAX_URL_LENGTH, m_pDependencies->GetString(m_iDependenciesProcessed));

            hr = GetUrlInfoAndMakeSticky(NULL, szThisUrl, lpInfo, dwBufSize, m_llCacheGroupID);

            if (E_OUTOFMEMORY == hr)
            {
                *pfDiskFull = TRUE;
                break;
            }

            if (SUCCEEDED(hr))
                *pdwSize += lpInfo->dwSizeLow;
        }

        LeaveCriticalSection(&m_critDependencies);
    }

    if (*pfDiskFull)
    {
        DBG_WARN("Webcrawler: UrlCache full trying to make sticky");
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

// true if found token & made null-term
LPSTR GetToken(LPSTR pszBuf, /*inout*/int *piBufPtr, /*out*/int *piLen)
{
static const CHAR szWhitespace[] = " \t\n\r";

    int iPtr = *piBufPtr;
    int iLen;

    while (1)
    {
        // skip leading whitespace
        iPtr += StrSpnA(pszBuf+iPtr, szWhitespace);

        if (!pszBuf[iPtr])
            return NULL;

        if (pszBuf[iPtr] == '#')
        {
            // comment; skip line
            while (pszBuf[iPtr] && pszBuf[iPtr]!='\r' && pszBuf[iPtr]!='\n') iPtr++;

            if (!pszBuf[iPtr])
                return NULL;

            continue;
        }

        // skip to next whitespace
        iLen = StrCSpnA(pszBuf+iPtr, szWhitespace);

        if (iLen == 0)
            return NULL;        // shoudln't happen

        *piBufPtr = iLen + iPtr;

        if (piLen)
            *piLen = iLen;

        if (pszBuf[iLen+iPtr])
        {
            pszBuf[iLen+iPtr] = NULL;
            ++ *piBufPtr;
        }

        break;
    }

//  TraceMsgA(TF_THISMODULE, "GetToken returning \"%s\"", (LPSTR)(pszBuf+iPtr));
    return pszBuf + iPtr;
}


// === Support functions for OnDownloadComplete

// ParseRobotsTxt gets the stream from CUrlDownload, parses it, and fills in parsed
//  info to *ppslRet
HRESULT CWebCrawler::ParseRobotsTxt(LPCWSTR pwszRobotsTxtURL, CWCStringList **ppslRet)
{
    // Given a robots.txt file (from CUrlDownload), it
    //  parses the file and fills in a string list with appropriate
    //  info.
    *ppslRet = FALSE;

    CHAR    szRobotsTxt[MAX_ROBOTS_SIZE];
    HRESULT hr=S_OK;
    LPSTR   pszToken;
    IStream *pstm=NULL;
    DWORD_PTR dwData;

    hr = m_pCurDownload->GetStream(&pstm);

    if (SUCCEEDED(hr))
    {
        STATSTG st;
        DWORD   dwSize;

        DBG("CWebCrawler parsing robots.txt file");

        pstm->Stat(&st, STATFLAG_NONAME);

        dwSize = st.cbSize.LowPart;

        if (st.cbSize.HighPart || dwSize >= MAX_ROBOTS_SIZE)
        {
            szRobotsTxt[0] = 0;
            DBG("CWebCrawler: Robots.Txt too big; ignoring");
            hr = E_FAIL;
        }
        else
        {
            hr = pstm->Read(szRobotsTxt, dwSize, NULL);
            szRobotsTxt[dwSize] = 0;
        }

        pstm->Release();
        pstm=NULL;

        if ((szRobotsTxt[0] == 0xff) && (szRobotsTxt[1] == 0xfe))
        {
            DBG_WARN("Unicode robots.txt! Ignoring ...");
            hr = E_FAIL;
        }
    }

    if (FAILED(hr))
        return hr;

    int iPtr = 0;
    WCHAR wchBuf2[256];
    WCHAR wchBuf[INTERNET_MAX_URL_LENGTH];
    DWORD dwBufSize;

    // Find the first "user-agent" which matches
    while ((pszToken = GetToken(szRobotsTxt, &iPtr, NULL)) != NULL)
    {
        if (lstrcmpiA(pszToken, c_szRobots_UserAgent))
            continue;

        pszToken = GetToken(szRobotsTxt, &iPtr, NULL);
        if (!pszToken)
            break;

        if ((*pszToken == '*') ||
            (!lstrcmpiA(pszToken, c_szRobots_OurUserAgent)))
        {
            TraceMsgA(TF_THISMODULE, "Using user agent segment: \"%s\"", pszToken);
            break;
        }
    }

    if (!pszToken)
        return E_FAIL;

    CWCStringList *psl = new CWCDwordStringList;
    if (psl)
    {
        psl->Init(2048);

        // Look for Allow: or Disallow: sections
        while ((pszToken = GetToken(szRobotsTxt, &iPtr, NULL)) != NULL)
        {
            if (!lstrcmpiA(pszToken, c_szRobots_UserAgent))
                break;  // end of our 'user-agent' section

            dwData = 0;

            if (!lstrcmpiA(pszToken, c_szRobots_Allow))     dwData = DATA_ALLOW;
            if (!lstrcmpiA(pszToken, c_szRobots_Disallow))  dwData = DATA_DISALLOW;

            if (!dwData)
                continue;   // look for next token

            pszToken = GetToken(szRobotsTxt, &iPtr, NULL);
            if (!pszToken)
                break;

            // Ensure that they don't have blank entries; we'll abort if so
            if (!lstrcmpiA(pszToken, c_szRobots_UserAgent) ||
                !lstrcmpiA(pszToken, c_szRobots_Allow) ||
                !lstrcmpiA(pszToken, c_szRobots_Disallow))
            {
                break;
            }

            // Combine this url with the base for this site.
            dwBufSize = ARRAYSIZE(wchBuf);
            if (SHAnsiToUnicode(pszToken, wchBuf2, ARRAYSIZE(wchBuf2)) &&
                SUCCEEDED(UrlCombineW(pwszRobotsTxtURL, wchBuf2, wchBuf, &dwBufSize, 0)))
            {
                TraceMsgA(TF_THISMODULE, "Robots.txt will %s urls with %s (%ws)",
                    ((dwData==DATA_ALLOW) ? c_szRobots_Allow : c_szRobots_Disallow),
                    pszToken, wchBuf);

                // if this is a duplicate url we effectively ignore this directive
                //  thanks to CWCStringList removing duplicates for us

                psl->AddString(wchBuf, dwData);
            }
        }
    }

    if (psl && (psl->NumStrings() > 0))
    {
        *ppslRet = psl;
        return S_OK;
    }

    if (psl)
        delete psl;

    return E_FAIL;
}

HRESULT CWebCrawler::GetRealUrl(int iPageIndex, LPWSTR *ppwszThisUrl)
{
    m_pCurDownload->GetRealURL(ppwszThisUrl);

    if (*ppwszThisUrl)
    {
        return S_OK;
    }

    DBG_WARN("m_pCurDownload->GetRealURL failed!!!");

    // Get url from string list
    LPCWSTR pwszUrl=NULL;

    pwszUrl = m_pPages->GetString(iPageIndex);

    if (pwszUrl)
    {
        *ppwszThisUrl = StrDupW(pwszUrl);
    }

    return (*ppwszThisUrl) ? S_OK : E_OUTOFMEMORY;
}

// Allocates BSTR for host name.
HRESULT CWebCrawler::GetHostName(LPCWSTR pwszThisUrl, BSTR *pbstrHostName)
{
    if (pwszThisUrl)
    {
        URL_COMPONENTSA comp;
        LPSTR           pszUrl;
        int             iLen;

//      InternetCrackUrlW(pszUrl, 0, 0, &comp)  // this is even slower than converting it ourselves...

        // convert to ansi
        iLen = lstrlenW(pwszThisUrl) + 1;
        pszUrl = (LPSTR)MemAlloc(LMEM_FIXED, iLen);
        if (pszUrl)
        {
            SHUnicodeToAnsi(pwszThisUrl, pszUrl, iLen);

            // crack out the host name
            ZeroMemory(&comp, sizeof(comp));
            comp.dwStructSize = sizeof(comp);
            comp.dwHostNameLength = 1;  // indicate that we want the host name

            if (InternetCrackUrlA(pszUrl, 0, 0, &comp))
            {
                *pbstrHostName = SysAllocStringLen(NULL, comp.dwHostNameLength);
                if (*pbstrHostName)
                {
                    comp.lpszHostName[comp.dwHostNameLength] = 0; // avoid debug rip
                    SHAnsiToUnicode(comp.lpszHostName, *pbstrHostName, comp.dwHostNameLength + 1);
                    ASSERT((*pbstrHostName)[comp.dwHostNameLength] == 0);
                }
            }

            MemFree((HLOCAL)pszUrl);
        }
    }

    return S_OK;
}

// Gets partly validated (CUrlDownload::IsValidUrl and hostname validation)
//  string lists and leaves in m_pPendingLinks
// Remaining validation is robots.txt if any
HRESULT CWebCrawler::GetLinksFromPage()
{
    // Get links from this page that we want to follow.
    CWCStringList *pslLinks=NULL, slMeta;

    IHTMLDocument2  *pDoc;
    BOOL            fFollowLinks = TRUE;
    int             i;

    slMeta.Init(2048);

    m_pCurDownload->GetDocument(&pDoc);
    if (pDoc)
    {
        // See if there is a META tag telling us not to follow
        CHelperOM::GetCollection(pDoc, &slMeta, CHelperOM::CTYPE_META, NULL, 0);
        for (i=0; i<slMeta.NumStrings(); i++)
        {
            if (!StrCmpNIW(slMeta.GetString(i), c_wszRobotsMetaName, c_iRobotsMetaNameLen))
            {
                LPCWSTR pwszContent = slMeta.GetString(i) + c_iRobotsMetaNameLen;
                TraceMsg(TF_THISMODULE, "Found 'robots' meta tag; content=%ws", pwszContent);

                while (pwszContent && *pwszContent)
                {
                    if (!StrCmpNIW(pwszContent, c_wszRobotsNoFollow, c_iRobotsNoFollow))
                    {
                        DBG("Not following links from this page.");
                        fFollowLinks = FALSE;
                        break;
                    }
                    pwszContent = StrChrW(pwszContent+1, L',');
                    if (pwszContent && *pwszContent)
                        pwszContent ++;
                }
                break;
            }
        }
        if (fFollowLinks)
        {
            if (m_pPendingLinks)
                pslLinks = m_pPendingLinks;
            else
            {
                pslLinks = new CWCDwordStringList;
                if (pslLinks)
                    pslLinks->Init();
                else
                    return E_OUTOFMEMORY;
            }

            CHelperOM::GetCollection(pDoc, pslLinks, CHelperOM::CTYPE_LINKS, &CheckLink, (DWORD_PTR)this);
            CHelperOM::GetCollection(pDoc, pslLinks, CHelperOM::CTYPE_MAPS, &CheckLink, (DWORD_PTR)this);
        }
        pDoc->Release();
        pDoc=NULL;
    }

    m_pPendingLinks = pslLinks;

    return S_OK;
}

// Gets 'dependency links' such as frames from a page
HRESULT CWebCrawler::GetDependencyLinksFromPage(LPCWSTR pwszThisUrl, DWORD dwRecurse)
{
    CWCStringList *psl=NULL;
    IHTMLDocument2 *pDoc;
    int i, iAdd, iIndex, iOldMax;
    DWORD_PTR dwData;

    if (m_pDependencyLinks)
        psl = m_pDependencyLinks;
    else
    {
        m_iDependencyStarted = 0;
        psl = new CWCStringList;
        if (psl)
            psl->Init(2048);
        else
            return E_OUTOFMEMORY;
    }

    iOldMax = psl->NumStrings();

    m_pCurDownload->GetDocument(&pDoc);
    if (pDoc)
    {
        // Add Frames ("Frame" and "IFrame" tags) if present
        CHelperOM::GetCollection(pDoc, psl, CHelperOM::CTYPE_FRAMES, CheckFrame, (DWORD_PTR)pwszThisUrl);
    }

    SAFERELEASE(pDoc);

    m_pDependencyLinks = psl;

    // Add the new urls to the main page list
    for (i = iOldMax; i<psl->NumStrings(); i++)
    {
        iAdd = m_pPages->AddString(m_pDependencyLinks->GetString(i),
                        dwRecurse,
                        &iIndex);

        if (m_lMaxNumUrls > 0 && iAdd==CWCStringList::STRLST_ADDED)
            m_lMaxNumUrls ++;

        if (iAdd == CWCStringList::STRLST_FAIL)
            return E_OUTOFMEMORY;

        if (iAdd == CWCStringList::STRLST_DUPLICATE)
        {
            // bump up recursion level of old page if necessary
            // See if we've downloaded this yet.
            dwData = m_pPages->GetStringData(iIndex);
            if (!(dwData & DATA_DLSTARTED))
            {
                // Haven't downloaded it yet.
                // Update the recurse levels if necessary.
                if ((dwData & DATA_RECURSEMASK) < dwRecurse)
                {
                    dwData = (dwData & ~DATA_RECURSEMASK) | dwRecurse;
                }

                // Turn off the "link" bit
                dwData &= ~DATA_LINK;

                m_pPages->SetStringData(iIndex, dwData);
            }
#ifdef DEBUG
            // Shouldn't happen; this frame already dl'd with lower recurse level
            else
                ASSERT((dwData & DATA_RECURSEMASK) >= dwRecurse);
#endif
        }
    }

    return S_OK;
}

//-------------------------------------
// OnDownloadComplete
//
// Called when a url is finished downloading, it processes the url
//  and kicks off the next download
//
HRESULT CWebCrawler::OnDownloadComplete(UINT iID, int iError)
{
    int         iPageIndex = m_iCurDownloadStringIndex;
    BOOL        fOperationComplete = FALSE;
    BOOL        fDiskFull = FALSE;
    BSTR        bstrCDFURL = NULL; //  CDF URL if there is one
    LPWSTR      pwszThisUrl=NULL;

    HRESULT     hr;

    TraceMsg(TF_THISMODULE, "WebCrawler: OnDownloadComplete(%d)", iError);
    ASSERT(m_pPages);
    ASSERT(iPageIndex < m_pCurDownloadStringList->NumStrings());

    if (_ERROR_REPROCESSING != iError)
    {
        m_iNumPagesDownloading --;
        ASSERT(m_iNumPagesDownloading == 0);
    }

    if (m_pCurDownloadStringList == m_pRobotsTxt)
    {
        CWCStringList *pslNew=NULL;

        // Process robots.txt file
        if (SUCCEEDED(ParseRobotsTxt(m_pRobotsTxt->GetString(iPageIndex), &pslNew)))
        {
            m_pRobotsTxt->SetStringData(iPageIndex, (DWORD_PTR)(pslNew));
        }
    }
    else
    {
        // Process normal file
        ASSERT(m_pCurDownloadStringList == m_pPages);

        DWORD dwData, dwRecurseLevelsFromThisPage;

        dwData = (DWORD)m_pPages->GetStringData(iPageIndex);
        dwRecurseLevelsFromThisPage = dwData & DATA_RECURSEMASK;

        dwData |= DATA_DLFINISHED;
        if (iError > 0)
            dwData |= DATA_DLERROR;

        // mark as downloaded
        m_pCurDownloadStringList->SetStringData(iPageIndex, dwData);

        // Is this the first page?
        if (m_iTotalStarted == 1)
        {
            // Check the HTTP response code
            DWORD dwResponseCode;

            hr = m_pCurDownload->GetResponseCode(&dwResponseCode);

            if (SUCCEEDED(hr))
            {
                hr = CheckResponseCode(dwResponseCode);
                if (FAILED(hr))
                    fOperationComplete = TRUE;
            }
            else
                DBG("CWebCrawler failed to GetResponseCode");

            // Get the Charset
            BSTR bstrCharSet=NULL;
            IHTMLDocument2 *pDoc=NULL;


            // -> Bharats --------
            // Find a link tag and store it away the cdf by copying it (if it points to a cdf.)
            // do url combine of this cdf 
            if (SUCCEEDED(m_pCurDownload->GetDocument(&pDoc)) && pDoc &&
                SUCCEEDED(pDoc->get_charset(&bstrCharSet)) && bstrCharSet)
            {
                WriteOLESTR(m_pSubscriptionItem, c_szPropCharSet, bstrCharSet);
                TraceMsg(TF_THISMODULE, "Charset = \"%ws\"", bstrCharSet);
                SysFreeString(bstrCharSet);        
            }
            else
                WriteEMPTY(m_pSubscriptionItem, c_szPropCharSet);

            if(pDoc)
            {
                if(FAILED(GetChannelItem(NULL)))   // A Doc exists and this download is not from a channel itself
                {
                    IHTMLLinkElement *pLink = NULL;
                    hr = SearchForElementInHead(pDoc, OLESTR("REL"), OLESTR("OFFLINE"), 
                                            IID_IHTMLLinkElement, (IUnknown **)&pLink);
                    if(S_OK == hr)
                    {
                        hr = pLink->get_href(&bstrCDFURL);
                        pLink->Release();
                    }
                }   
                pDoc->Release();
                pDoc = NULL;
            }
        }

        if ((iError != _ERROR_REPROCESSING) && (iError != BDU2_ERROR_NONE))
        {
            if (iError != BDU2_ERROR_NOT_HTML)
                m_iDownloadErrors ++;

            if (iError == BDU2_ERROR_MAXSIZE)
            {
                SetEndStatus(INET_E_AGENT_MAX_SIZE_EXCEEDED);
                fOperationComplete = TRUE;
            }
        }
        else
        {
            // Don't process this url if we already have set fOperationComplete
            if (!fOperationComplete)
            {
                // Did we get *just* the HEAD info?
                if (IsAgentFlagSet(FLAG_HEADONLY))
                {
                    SYSTEMTIME stLastModified;
                    FILETIME   ftLastModified;

                    if (SUCCEEDED(m_pCurDownload->GetLastModified(&stLastModified)) &&
                                  SystemTimeToFileTime(&stLastModified, &ftLastModified))
                    {
                        DBG("Retrieved 'HEAD' info; change detection based on Last Modified");

                        hr = PostCheckUrlForChange(&m_varChange, NULL, ftLastModified);
                        // If we FAILED, we mark it as changed.
                        if (hr == S_OK || FAILED(hr))
                        {
                            SetAgentFlag(FLAG_CRAWLCHANGED);
                            DBG("URL has changed; will flag webcrawl as changed");
                        }

                        // "Changes Only" mode, persist change detection code
                        ASSERT(IsAgentFlagSet(FLAG_CHANGESONLY));
                        ASSERT(m_iTotalStarted == 1);
                        WriteVariant(m_pSubscriptionItem, c_szPropChangeCode, &m_varChange);
                    }
                }
                else
                {
                    // Get real URL in case we were redirected
                    if (FAILED(GetRealUrl(iPageIndex, &pwszThisUrl)))
                    {
                        fOperationComplete = TRUE;        // bad
                    }
                    else
                    {
                        ASSERT(pwszThisUrl);

                        // Get host name from first page if necessary
                        if ((iPageIndex==0) &&
                            (m_dwRecurseLevels>0) &&
                            !IsRecurseFlagSet(WEBCRAWL_LINKS_ELSEWHERE) &&
                            !m_bstrHostName)
                        {
                            GetHostName(pwszThisUrl, &m_bstrHostName);
#ifdef DEBUG
                            if (m_bstrHostName)
                                TraceMsg(TF_THISMODULE, "Just got first host name: %ws", m_bstrHostName);
                            else
                                DBG_WARN("Get first host name failed!!!");
#endif
                        }

                        DWORD dwCurSize = 0, dwRepeat = 0;

                        HRESULT hr1;

                        do
                        {
                            hr1 = S_OK;

                            // Make page and dependencies sticky and get their total size
                            fDiskFull = FALSE;
                            MakePageStickyAndGetSize(pwszThisUrl, &dwCurSize, &fDiskFull);

                            if (fDiskFull && (dwRepeat < 2))
                            {
                                // If we couldn't make stuff sticky, ask host to make cache bigger
                                hr1 = m_pAgentEvents->ReportError(&m_SubscriptionCookie,
                                            INET_E_AGENT_EXCEEDING_CACHE_SIZE, NULL);

                                if (hr1 == E_PENDING)
                                {
                                    // Host is going to ask the user to increase the cache size.
                                    // Host should either abort or resume us later.
                                    SetAgentFlag(FLAG_WAITING_FOR_INCREASED_CACHE);
                                    goto done;
                                }
                                else if (hr1 == INET_S_AGENT_INCREASED_CACHE_SIZE)
                                {
                                    // Host just increased the cache size. Try it again.
                                }
                                else
                                {
                                    // Not gonna do it. Abort.
                                }
                            }
                        }
                        while ((hr1 == INET_S_AGENT_INCREASED_CACHE_SIZE) && (++dwRepeat <= 2));

                        m_dwCurSize += dwCurSize;

                        // Is there form based authentication that we need to handle
                        // on the top page of this subscription?
                        if (!fDiskFull && (0 == iPageIndex) && !m_pCurDownload->GetFormSubmitted())
                        {
                            hr = FindAndSubmitForm();
                            if (S_OK == hr)
                            {
                                // Successfully submitted form.  Bail and wait for the next OnDownloadComplete() call.
                                // BUGBUG: Should we make the form URL and dependencies sticky?
                                return S_OK;
                            }
                            else if (FAILED(hr))
                            {
                                // We failed trying to submit the form.  Bail.
                                // BUGBUG: Should we set a better error string?
                                SetEndStatus(E_FAIL);
                                CleanUp();
                                return S_OK;
                            }
                            // else no form - fall through
                        }

                        TraceMsg(TF_THISMODULE, "WebCrawler up to %d kb", (int)(m_dwCurSize>>10));

                        if ((m_lMaxNumUrls < 0) &&
                            !dwRecurseLevelsFromThisPage &&
                            !(dwData & DATA_CODEBASE))
                        {
                            m_lMaxNumUrls = m_pPages->NumStrings() + ((m_pRobotsTxt) ? m_pRobotsTxt->NumStrings() : 0);
                        }
                    }  // SUCCEEDED(GetRealUrl)
                }  // !FLAG_HEADONLY
            } // !fOperationComplete

            // If we're in "Changes Only" mode, we're done.
            if (IsAgentFlagSet(FLAG_CHANGESONLY))
                fOperationComplete = TRUE;

            // Check to see if we're past our max size
            if (!fOperationComplete && fDiskFull || (m_dwMaxSize && (m_dwCurSize >= (m_dwMaxSize<<10))))
            {
        #ifdef DEBUG
                if (fDiskFull)
                    DBG_WARN("Disk/cache full; aborting.");
                else
                    TraceMsg(TF_WARNING, "Past maximum size; aborting. (%d kb of %d kb)", (int)(m_dwCurSize>>10), (int)m_dwMaxSize);
        #endif
                // abort operation
                fOperationComplete = TRUE;

                if (fDiskFull)
                {
                    SetEndStatus(INET_E_AGENT_CACHE_SIZE_EXCEEDED);
                }
                else
                {
                    SetEndStatus(INET_E_AGENT_MAX_SIZE_EXCEEDED);
                }
            }

            if (!fOperationComplete)
            {
                // Get any links from page
                // Get "dependency links" from page - frames, etc.

                // we do this even if a CDF file is specified
                // Essentially, since the user has no clue about the CDF
                // file - we do not want to confuse the user
                GetDependencyLinksFromPage(pwszThisUrl, dwRecurseLevelsFromThisPage);

                if (dwRecurseLevelsFromThisPage)
                {
                    // Get links from this page that we want to follow.
                    GetLinksFromPage();

                    if (m_pPendingLinks)
                        TraceMsg(TF_THISMODULE,
                            "Total of %d unique valid links found", m_pPendingLinks->NumStrings());

                    m_dwPendingRecurseLevel = dwRecurseLevelsFromThisPage - 1;
                }

            }
        }   // !iError
    } // !robots.txt

    if(!fOperationComplete)
        StartCDFDownload(bstrCDFURL, pwszThisUrl);
        
    if(!m_fCDFDownloadInProgress)
    {
        // Don't try code downloads or any of the rest until you're done with
        // the cdf download
        // See if we have any more URLs to download.
        if (!fOperationComplete && FAILED(StartNextDownload()))
            fOperationComplete = TRUE;  // No, we're done!
    }

    CheckOperationComplete(fOperationComplete);

done:
    if (pwszThisUrl)
        MemFree(pwszThisUrl);

    SAFEFREEBSTR(bstrCDFURL);
            

    return S_OK;
}



HRESULT CWebCrawler::StartCDFDownload(WCHAR *pwszCDFURL, WCHAR *pwszBaseUrl)
{
    HRESULT hr = E_FAIL;
    m_fCDFDownloadInProgress = FALSE;
    if(pwszCDFURL)
    {
        // We have a CDF File - begin download of it
    
        if (m_pRunAgent)
        {
            ASSERT(0);
            DBG_WARN("WebCrawler: Attempting to download next CDF when nother CDF exists.");
            hr = E_FAIL;
            goto Exit;
        }
        else
        {
             // create subscription item for CDL agent.

            ISubscriptionItem *pItem = NULL;
            
            
            if (m_dwMaxSize && ((m_dwCurSize>>10) >= m_dwMaxSize))
            {
                // We've exceeded our maximum download KB limit and can't continue.
                DBG_WARN("WebCrawler: Exceeded Maximum KB download limit with CodeBase download.");
                SetEndStatus(hr = INET_E_AGENT_MAX_SIZE_EXCEEDED);
                goto Exit;
            }

            if (!m_pSubscriptionItem ||
                FAILED(hr = DoCloneSubscriptionItem(m_pSubscriptionItem, NULL, &pItem)))
            {
                goto Exit;
            }
            ASSERT(pItem != NULL);
            ASSERT(pwszCDFURL != NULL);
            WCHAR   wszCombined[INTERNET_MAX_URL_LENGTH];
            DWORD dwBufSize = ARRAYSIZE(wszCombined);
            
            if (SUCCEEDED(UrlCombineW(pwszBaseUrl, pwszCDFURL, wszCombined, &dwBufSize, 0)))
            {
            
                WriteOLESTR(pItem, c_szPropURL, wszCombined);
            
                WriteEMPTY(pItem, c_szPropCrawlGroupID); // clear the old cache group id - don't want 
                                                         // children to know of it 
                // The crawler already has a cache group id that we simply use as the new ID
                WriteLONGLONG(pItem, c_szPropCrawlNewGroupID, m_llCacheGroupID);
                WriteDWORD(pItem, c_szPropChannelFlags, CHANNEL_AGENT_PRECACHE_ALL);
                // Finally - since we know that this is for offline use, we just set the flags to precache all
            
                m_pRunAgent = new CRunDeliveryAgent();
                if (m_pRunAgent)
                    hr = m_pRunAgent->Init((CRunDeliveryAgentSink *)this, pItem, CLSID_ChannelAgent);
                pItem->Release();

                if (m_pRunAgent && SUCCEEDED(hr))
                {
                    hr = m_pRunAgent->StartAgent(); 
                    if (hr == E_PENDING)
                    {
                        hr = S_OK;
                        m_fCDFDownloadInProgress = TRUE;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
Exit:
    if((S_OK != hr) && m_pRunAgent)
    {
        CRunDeliveryAgent::SafeRelease(m_pRunAgent);
    }
    return hr;

}

// CRunDeliveryAgentSink call back method to signal the end of a codebase download.

HRESULT CWebCrawler::OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                               long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                               BOOL fSynchronous)
{
    ASSERT(m_pRunAgent != NULL);
    BOOL        fOperationComplete = FALSE;
    CRunDeliveryAgent::SafeRelease(m_pRunAgent);



    if(m_fCDFDownloadInProgress)
    {
        m_fCDFDownloadInProgress = FALSE; 
    }
    else
    {
        int         iPageIndex = m_iCurDownloadStringIndex;
        BOOL        fDiskFull = FALSE;
        CCodeBaseHold *pcbh = NULL;
        BOOL        fError;
        LPCWSTR     pwszThisURL=NULL;

        TraceMsg(TF_THISMODULE, "WebCrawler: OnAgentEnd of CRunDeliveryAgentSink");
        ASSERT(m_pCodeBaseList);
        ASSERT(iPageIndex < m_pCurDownloadStringList->NumStrings());
        ASSERT(m_pCurDownloadStringList == m_pCodeBaseList);

        m_iNumPagesDownloading --;
        ASSERT(m_iNumPagesDownloading == 0);

        pcbh = (CCodeBaseHold *)m_pCodeBaseList->GetStringData(iPageIndex);
        pwszThisURL = m_pCodeBaseList->GetString(iPageIndex);
        ASSERT(pwszThisURL);

        pcbh->dwFlags |= DATA_DLFINISHED;

        fError = FAILED(hrResult);
        if (fSynchronous)
        {
            fError = TRUE;
            ASSERT(FAILED(hrResult));       // we can't succeed synchronously...
        }

        //NOTE: The CDL agent will abort if it finds the file exceeds the MaxSizeKB.  In this case the file is not
        //      counted and there may be other smaller CAB's that can be downloaded, so we continue to proceed.

        if (fError)
        {
            pcbh->dwFlags |= DATA_DLERROR;
            m_iDownloadErrors ++;
            SetEndStatus(hrResult);
        }
        else
        {
            BYTE chBuf[MY_MAX_CACHE_ENTRY_INFO];
            LPINTERNET_CACHE_ENTRY_INFO lpInfo = (LPINTERNET_CACHE_ENTRY_INFO) chBuf;
            TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];

            MyOleStrToStrN(szUrl, INTERNET_MAX_URL_LENGTH, pwszThisURL);

            if (FAILED(GetUrlInfoAndMakeSticky(NULL, szUrl,
                                 lpInfo, sizeof(chBuf), m_llCacheGroupID)))
            {
                //REVIEW: Do something here?  Unlikely to occur in practice.
                fOperationComplete = TRUE;
                ASSERT(0);
            }
            else
            {
                m_dwCurSize += lpInfo->dwSizeLow;
            }

            TraceMsg(TF_THISMODULE, "WebCrawler up to %d kb", (int)(m_dwCurSize>>10));

            if (m_dwMaxSize && ((m_dwCurSize>>10)>m_dwMaxSize))
            {

                // abort operation
                fOperationComplete = TRUE;
                if (fDiskFull)
                    SetEndStatus(INET_E_AGENT_CACHE_SIZE_EXCEEDED);
                else
                    SetEndStatus(INET_E_AGENT_MAX_SIZE_EXCEEDED);
            }

        } // !fError
    }
    // See if we have any more URLs to download.
    if (!fOperationComplete && FAILED(StartNextDownload()))
        fOperationComplete = TRUE;  // No, we're done!

    if(!fSynchronous)
        CheckOperationComplete(fOperationComplete);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// CheckCompleteOperation :: If parameter is TRUE, then all downloads are
//                           complete, the appropriate STATUS_CODE is set
//                           and clean up initiated.
//
//////////////////////////////////////////////////////////////////////////
void CWebCrawler::CheckOperationComplete(BOOL fOperationComplete)
{
    if (fOperationComplete)
    {
        DBG("WebCrawler complete. Shutting down.");
        if (INET_S_AGENT_BASIC_SUCCESS == GetEndStatus())
        {
            // Set end status appropriately
            if (m_iDownloadErrors)
            {
                if (m_iPagesStarted<=1)
                {
                    DBG("Webcrawl failed - first URL failed.");
                    SetEndStatus(E_INVALIDARG);
                }
                else
                {
                    DBG("Webcrawl succeeded - some URLs failed.");
                    SetEndStatus(INET_S_AGENT_PART_FAIL);
                }
            }
            else
            {
                DBG("Webcrawl succeeded");
                if (!IsAgentFlagSet(FLAG_CRAWLCHANGED))
                {
                    SetEndStatus(S_FALSE);
                    DBG("No changes were detected");
                }
                else
                {
                    DBG("Webcrawl succeeded");
                    SetEndStatus(S_OK);
                }
            }
        }

        if (m_llOldCacheGroupID)
        {
            DBG("Nuking old cache group.");
            if (!DeleteUrlCacheGroup(m_llOldCacheGroupID, 0, 0))
            {
                DBG_WARN("Failed to delete old cache group!");
            }
        }

        WriteLONGLONG(m_pSubscriptionItem, c_szPropCrawlGroupID, m_llCacheGroupID);

        m_lSizeDownloadedKB = ((m_dwCurSize+511)>>10);

        WriteDWORD(m_pSubscriptionItem, c_szPropCrawlActualSize, m_lSizeDownloadedKB);

        if (m_lMaxNumUrls >= 0)
        {
            WriteDWORD(m_pSubscriptionItem, c_szPropActualProgressMax, m_lMaxNumUrls);
        }

        // Send a robots.txt warning to the user if we ended up not downloading stuff
        //  because of the server's robots.txt file
        if (m_iSkippedByRobotsTxt != 0)
        {
            HRESULT hr = S_OK;      // Make it an "information" message
            WCHAR wszMessage[200];

            if (m_iPagesStarted==1)
            {
                hr = INET_E_AGENT_WARNING;  // Unless we're missing almost everything
            }

            if (MLLoadStringW(IDS_CRAWL_ROBOTS_TXT_WARNING, wszMessage, ARRAYSIZE(wszMessage)))
            {
                m_pAgentEvents->ReportError(&m_SubscriptionCookie, hr, wszMessage);
            }
        }

        // Will call "UpdateEnd"
        CleanUp();
    }
}

HRESULT CWebCrawler::ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes)
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

HRESULT CWebCrawler::DownloadStart(LPCWSTR pchUrl, DWORD dwDownloadId, DWORD dwType, DWORD dwReserved)
{
    HRESULT hr = S_OK, hr2;

    // free threaded
    EnterCriticalSection(&m_critDependencies);

    if (NULL == pchUrl)
    {
        DBG_WARN("CWebCrawler::DownloadStart pchUrl=NULL");
    }
    else
    {
        // Check to see if this is already in our dependencies list and abort if so
        if (CWCStringList::STRLST_ADDED != m_pDependencies->AddString(pchUrl, 0))
        {
            hr = E_ABORT;       // Don't download this thing.
            TraceMsg(TF_THISMODULE, "Aborting mshtml url (already added): %ws", pchUrl);
        }

        if (SUCCEEDED(hr))
        {
            // Check to see if this fails the robots.txt and abort if so
            // Note, this will only work if we happen to have already gotten this robots.txt
            // Need to abort here if we haven't gotten it, then get it, then get just this dep. Yuck.
            // Also shouldn't do the check if this is the first page downloaded
            DWORD dwIndex;
            hr2 = GetRobotsTxtIndex(pchUrl, FALSE, &dwIndex);
            if (SUCCEEDED(hr2))
            {
                BOOL fAllow;
                if (SUCCEEDED(ValidateWithRobotsTxt(pchUrl, dwIndex, &fAllow)))
                {
                    if (!fAllow)
                        hr = E_ABORT;   // ooh, failed the test.
                }
            }
        }
    }

    LeaveCriticalSection(&m_critDependencies);

    return hr;
}

HRESULT CWebCrawler::DownloadComplete(DWORD dwDownloadId, HRESULT hrNotify, DWORD dwReserved)
{
    // free threaded
    // Do nothing. We may wish to post message to make sticky here. We may wish to
    //  mark as downloaded in string list here.
//  EnterCriticalSection(&m_critDependencies);
//  LeaveCriticalSection(&m_critDependencies);
    return S_OK;
}


/* 41927 (IE5 4491)
HRESULT CWebCrawler::OnGetReferer(LPCWSTR *ppwszReferer)
{
    if (m_iPagesStarted <= 1)
    {
        *ppwszReferer = NULL;
        return S_FALSE;
    }

    if (m_pCurDownloadStringList == m_pRobotsTxt)
    {
        // Referer is last page from main list to be downloaded
        *ppwszReferer = m_pPages->GetString(m_iPagesStarted-1);
        return S_OK;
    }

    if (m_pCurDownloadStringList == m_pPages)
    {
        // Referer is stored in string list data
        *ppwszReferer = m_pPages->GetString(
            ((m_pPages->GetStringData(m_iCurDownloadStringIndex) & DATA_REFERERMASK) >> DATA_REFERERSHIFT));
        return S_OK;
    }

    // We don't return a referer for code bases
    ASSERT(m_pCurDownloadStringList == m_pCodeBaseList);

    return S_FALSE;
}
*/

HRESULT CWebCrawler::OnAuthenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword)
{
    HRESULT hr, hrRet=E_FAIL;
    ASSERT(phwnd && ppszUsername && ppszPassword);
    ASSERT((HWND)-1 == *phwnd && NULL == *ppszUsername && NULL == *ppszPassword);

    // If our host name doesn't match the root host name, don't return auth
    // information.

    LPWSTR pwszUrl, bstrHostName=NULL;

    m_pCurDownload->GetRealURL(&pwszUrl);   // may re-enter Trident

    if (pwszUrl)
    {
        GetHostName(pwszUrl, &bstrHostName);
        LocalFree(pwszUrl);
    }

    if (bstrHostName)
    {
        if (!m_bstrHostName || !MyAsciiCmpW(bstrHostName, m_bstrHostName))
        {
            // Host names match. Return auth information.
            // If we're hosted by channel agent, use its auth information
            ISubscriptionItem *pChannel=NULL;
            ISubscriptionItem *pItem=m_pSubscriptionItem;
            
            if (SUCCEEDED(GetChannelItem(&pChannel)))
            {
                pItem = pChannel;
            }
            
            hr = ReadOLESTR(pItem, c_szPropCrawlUsername, ppszUsername);
            if (SUCCEEDED(hr))
            {
                BSTR bstrPassword = NULL;
                hr = ReadPassword(pItem, &bstrPassword);
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
                        hrRet = S_OK;
                    }
                }
            }

            if (FAILED(hrRet))
            {
                SAFEFREEOLESTR(*ppszUsername);
                SAFEFREEOLESTR(*ppszPassword);
            }

            SAFERELEASE(pChannel);
        }

        SysFreeString(bstrHostName);
    }
    return hrRet;
}

HRESULT CWebCrawler::OnClientPull(UINT iID, LPCWSTR pwszOldURL, LPCWSTR pwszNewURL)
{
    // CUrlDownload is informing us it's about to do a client pull.

    // Let's send out a progress report for the new url
    SendUpdateProgress(pwszNewURL, m_iTotalStarted, m_lMaxNumUrls);

    // Now we need to process the current url: make it and dependencies sticky
    DWORD dwCurSize=0;
    BOOL fDiskFull=FALSE;
    MakePageStickyAndGetSize(pwszOldURL, &dwCurSize, &fDiskFull);
    m_dwCurSize += dwCurSize;
    TraceMsg(TF_THISMODULE, "WebCrawler processed page prior to client pull - now up to %d kb", (int)(m_dwCurSize>>10));

    // Tell CUrlDownload to go ahead and download the new url
    return S_OK;
}

HRESULT CWebCrawler::OnOleCommandTargetExec(const GUID *pguidCmdGroup, DWORD nCmdID,
                                DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                                VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    IPropertyBag2 *pPropBag = NULL;
    int i;

    //REVIEW: CLSID for this not yet defined.
    if (    pguidCmdGroup 
        && (*pguidCmdGroup == CGID_JavaParambagCompatHack) 
        && (nCmdID == 0) 
        && (nCmdexecopt == MSOCMDEXECOPT_DONTPROMPTUSER))
    {
        if (!IsRecurseFlagSet(WEBCRAWL_GET_CONTROLS))
        {
            goto Exit;
        }

        uCLSSPEC ucs;
        QUERYCONTEXT qc = { 0 };

        ucs.tyspec = TYSPEC_CLSID;
        ucs.tagged_union.clsid = CLSID_JavaVM;

        // Check to see if Java VM is installed. Don't try to get applets if not.
        if (!SUCCEEDED(FaultInIEFeature(NULL, &ucs, &qc, FIEF_FLAG_PEEK)))
        {
            goto Exit;
        }

        ULONG enIndex;
        const DWORD enMax = 7, enMin = 0;
        PROPBAG2 pb[enMax];
        VARIANT vaProps[enMax];
        HRESULT hrResult[enMax];
        enum { enCodeBase = 0, enCabBase, enCabinets, enArchive, enUsesLib, enLibrary, enUsesVer };
        LPWSTR pwszThisURL = NULL;
        int chLen;

        //REVIEW: This will need to be reviewed later when matching trident code is available
        //        and details worked out.

        if ((pvarargIn->vt != VT_UNKNOWN) || 
            (FAILED(pvarargIn->punkVal->QueryInterface(IID_IPropertyBag2, (void **)&pPropBag))))
        {
             goto Exit;
        }

        if (FAILED(GetRealUrl(m_iCurDownloadStringIndex, &pwszThisURL)))
        {
            pwszThisURL = StrDupW(L"");
        }

        // PROPBAG2 structure for data retrieval
        for (i=enMin; i<enMax; i++)
        {
            pb[i].dwType = PROPBAG2_TYPE_DATA;
            pb[i].vt = VT_BSTR;
            pb[i].cfType = NULL;                   // CLIPFORMAT
            pb[i].dwHint = 0;                      // ????
            pb[i].pstrName = NULL;
            pb[i].clsid = CLSID_NULL;              // ????
            vaProps[i].vt = VT_EMPTY;
            vaProps[i].bstrVal = NULL;
            hrResult[i] = E_FAIL;
        }

        if (((pb[enCodeBase].pstrName = SysAllocString(L"CODEBASE")) != NULL) &&
            ((pb[enCabBase].pstrName = SysAllocString(L"CABBASE")) != NULL) &&
            ((pb[enCabinets].pstrName = SysAllocString(L"CABINETS")) != NULL) &&
            ((pb[enArchive].pstrName = SysAllocString(L"ARCHIVE")) != NULL) &&
            ((pb[enUsesLib].pstrName = SysAllocString(L"USESLIBRARY")) != NULL) &&
            ((pb[enLibrary].pstrName = SysAllocString(L"USESLIBRARYCODEBASE")) != NULL) &&
            ((pb[enUsesVer].pstrName = SysAllocString(L"USESLIBRARYVERSION")) != NULL))
        {

            //BUGBUG: Read returns E_FAIL even if it read some of the properties.
            //Since we check hrResult's below this isn't a big deal.

            hr = pPropBag->Read(enMax, &pb[0], NULL, &vaProps[0], &hrResult[0]);

            {
                BSTR szCodeBase = NULL;

                // check for CODEBASE
                if (SUCCEEDED(hrResult[enCodeBase]) && (vaProps[enCodeBase].vt == VT_BSTR))
                {
                    szCodeBase = vaProps[enCodeBase].bstrVal;
                }

                // add a trailing slash if not already present
                chLen = lstrlenW(szCodeBase);
                if (chLen && szCodeBase[chLen-1] != '/')
                {
                    LPWSTR szNewCodeBase = 0;
                    szNewCodeBase = (LPWSTR) LocalAlloc(0,sizeof(WCHAR)*(chLen+2));
                    if (szNewCodeBase)
                    {
                        StrCpyW(szNewCodeBase, szCodeBase);
                        StrCatW(szNewCodeBase, L"/");
                        SAFEFREEBSTR(szCodeBase);
                        szCodeBase = vaProps[enCodeBase].bstrVal = SysAllocString(szNewCodeBase);
                        LocalFree(szNewCodeBase);     
                    }
                }

                // check for CABBASE
                if (SUCCEEDED(hrResult[enCabBase]) && (vaProps[enCabBase].vt == VT_BSTR))
                {
                    BSTR szCabBase = vaProps[enCabBase].bstrVal;

                    // Add CABBASE URL to list of CABs to pull.
                    if (SUCCEEDED(CombineBaseAndRelativeURLs(pwszThisURL, szCodeBase, &szCabBase)))
                    {
                        m_pPages->AddString(szCabBase, 0);
                    }
                }

                // check for CABINETS
                for (enIndex = enCabinets; enIndex<(enArchive+1); enIndex++)
                {
                    if (SUCCEEDED(hrResult[enIndex]) && (vaProps[enIndex].vt == VT_BSTR))
                    {
                        BSTR szCur = vaProps[enIndex].bstrVal, szPrev = NULL;
                        while (szCur)
                        {
                            WCHAR wcCur = *szCur;

                            if ((wcCur == L'+') || (wcCur == L',') || (wcCur == L'\0'))
                            {
                                BSTR szLast = szPrev, szCabBase = NULL;
                                BOOL bLastFile = FALSE;
                                if (!szPrev)
                                {
                                    szLast = vaProps[enIndex].bstrVal;
                                }
                                szPrev = szCur; szPrev++;

                                if (*szCur == L'\0')
                                {
                                    bLastFile = TRUE;
                                }
                                *szCur = (unsigned short)L'\0';

                                // szLast points to current CabBase.
                                szCabBase = SysAllocString(szLast);
                                if (SUCCEEDED(CombineBaseAndRelativeURLs(pwszThisURL, szCodeBase, &szCabBase)))
                                {
                                    int iAdd=m_pPages->AddString(szCabBase, DATA_CODEBASE);
                                    if (m_lMaxNumUrls > 0 && iAdd==CWCStringList::STRLST_ADDED)
                                        m_lMaxNumUrls ++;
                                }
                                SAFEFREEBSTR(szCabBase);

                                if (bLastFile)
                                {
                                    szCur = NULL;
                                    break;
                                }
                            }
                            szCur++;
                        }  // while (szCur)
                    }  // cabinets
                }

                // check for USESLIBRARY* parameters.
                CCodeBaseHold *pcbh = NULL;
                if (SUCCEEDED(hrResult[enUsesLib]) && (vaProps[enUsesLib].vt == VT_BSTR) &&
                    SUCCEEDED(hrResult[enLibrary]) && (vaProps[enLibrary].vt == VT_BSTR))
                {
                    BSTR szThisLibCAB = NULL;
                    pcbh = new CCodeBaseHold();
                    if (pcbh)
                    {
                        pcbh->szDistUnit = SysAllocString(vaProps[enUsesLib].bstrVal);
                        pcbh->dwVersionMS = pcbh->dwVersionLS = -1;
                        pcbh->dwFlags = 0;
                        szThisLibCAB = SysAllocString(vaProps[enLibrary].bstrVal);
                        if (FAILED(CombineBaseAndRelativeURLs(pwszThisURL, szCodeBase, &szThisLibCAB)) ||
                            m_pCodeBaseList->AddString(szThisLibCAB, (DWORD_PTR)pcbh) != CWCStringList::STRLST_ADDED)
                        {
                            SAFEFREEBSTR(pcbh->szDistUnit);
                            SAFEDELETE(pcbh);
                        }
                        SAFEFREEBSTR(szThisLibCAB);
                    }
                }

                // Check for USESLIBRARYVERSION (optional)
                if (pcbh && SUCCEEDED(hrResult[enUsesVer]) && (vaProps[enUsesVer].vt == VT_BSTR))
                {
                    int iLen = SysStringByteLen(vaProps[enUsesVer].bstrVal)+1;
                    CHAR *szVerStr = (LPSTR)MemAlloc(LMEM_FIXED, iLen);

                    if (szVerStr)
                    {
                        SHUnicodeToAnsi(vaProps[enUsesVer].bstrVal, szVerStr, iLen);

                        if (FAILED(GetVersionFromString(szVerStr,
                                     &pcbh->dwVersionMS, &pcbh->dwVersionLS)))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                            MemFree(szVerStr);
                            SAFEFREEBSTR(pcbh->szDistUnit);
                            SAFEDELETE(pcbh);
                        }
                        MemFree(szVerStr);
                    }
                }
            }
        } // Read properties

        for (i=enMin; i<enMax; i++)
        {
            SAFEFREEBSTR(pb[i].pstrName);
        }

        if (pwszThisURL)
            LocalFree(pwszThisURL);

        hr = S_OK;
    }

Exit:
    SAFERELEASE(pPropBag);
    return hr;
}

HRESULT CWebCrawler::GetDownloadNotify(IDownloadNotify **ppOut)
{
    HRESULT hr=S_OK;

    if (m_pDownloadNotify)
    {
        m_pDownloadNotify->LeaveMeAlone();
        m_pDownloadNotify->Release();
        m_pDownloadNotify=NULL;
    }

    m_pDownloadNotify = new CDownloadNotify(this);

    if (m_pDownloadNotify)
    {
        *ppOut = m_pDownloadNotify;
        m_pDownloadNotify->AddRef();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppOut = NULL;
    }

    return hr;
}

//---------------------------------------------------------------
// CWebCrawler::CDownloadNotify class
//---------------------------------------------------------------
CWebCrawler::CDownloadNotify::CDownloadNotify(CWebCrawler *pParent)
{
    ASSERT(pParent);

    m_cRef = 1;

    m_pParent = pParent;
    pParent->AddRef();

    InitializeCriticalSection(&m_critParent);
}

CWebCrawler::CDownloadNotify::~CDownloadNotify()
{
    DBG("Destroying CWebCrawler::CDownloadNotify");

    ASSERT(!m_pParent);
    SAFERELEASE(m_pParent);
    DeleteCriticalSection(&m_critParent);
}

void CWebCrawler::CDownloadNotify::LeaveMeAlone()
{
    if (m_pParent)
    {
        EnterCriticalSection(&m_critParent);
        SAFERELEASE(m_pParent);
        LeaveCriticalSection(&m_critParent);
    }
}

// IUnknown members
HRESULT CWebCrawler::CDownloadNotify::QueryInterface(REFIID riid, void **ppv)
{
    if ((IID_IUnknown == riid) ||
        (IID_IDownloadNotify == riid))
    {
        *ppv = (IDownloadNotify *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();

    return S_OK;
}

ULONG CWebCrawler::CDownloadNotify::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CWebCrawler::CDownloadNotify::Release(void)
{
    if (0L != InterlockedDecrement(&m_cRef))
        return 1L;

    delete this;
    return 0L;
}

// IDownloadNotify
HRESULT CWebCrawler::CDownloadNotify::DownloadStart(LPCWSTR pchUrl, DWORD dwDownloadId, DWORD dwType, DWORD dwReserved)
{
    HRESULT hr = E_ABORT;   // abort it if we have nobody listening

    TraceMsg(TF_THISMODULE, "DownloadStart id=%d url=%ws", dwDownloadId, pchUrl ? pchUrl : L"(null)");

    EnterCriticalSection(&m_critParent);
    if (m_pParent)
        hr = m_pParent->DownloadStart(pchUrl, dwDownloadId, dwType, dwReserved);
    LeaveCriticalSection(&m_critParent);

    return hr;
}

HRESULT CWebCrawler::CDownloadNotify::DownloadComplete(DWORD dwDownloadId, HRESULT hrNotify, DWORD dwReserved)
{
    HRESULT hr = S_OK;

//  TraceMsg(TF_THISMODULE, "DownloadComplete id=%d hr=%x", dwDownloadId, hrNotify);

    EnterCriticalSection(&m_critParent);
    if (m_pParent)
        hr = m_pParent->DownloadComplete(dwDownloadId, hrNotify, dwReserved);
    LeaveCriticalSection(&m_critParent);

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Other functions
//
//////////////////////////////////////////////////////////////////////////
// Make a single absolute or relative url sticky and get size
HRESULT GetUrlInfoAndMakeSticky(
            LPCTSTR                     pszBaseUrl,
            LPCTSTR                     pszThisUrl,
            LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,
            DWORD                       dwBufSize,
            GROUPID                     llCacheGroupID)
{
    DWORD   dwSize;
    TCHAR   szCombined[INTERNET_MAX_URL_LENGTH];

#if 0   // Make lpCacheEntryInfo optional
    BYTE    chBuf[MY_MAX_CACHE_ENTRY_INFO];
    if (!lpCacheEntryInfo)
    {
        lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)chBuf;
        dwBufSize = sizeof(chBuf);
    }
#else
    ASSERT(lpCacheEntryInfo);
#endif

    // Combine urls if necessary
    if (pszBaseUrl)
    {
        dwSize = ARRAYSIZE(szCombined);
        if (SUCCEEDED(UrlCombine(pszBaseUrl, pszThisUrl,
                szCombined, &dwSize, 0)))
        {
            pszThisUrl = szCombined;
        }
        else
            DBG_WARN("UrlCombine failed!");
    }

    // Add the size of this URL
    lpCacheEntryInfo->dwStructSize = dwBufSize;
    if (!GetUrlCacheEntryInfo(pszThisUrl, lpCacheEntryInfo, &dwBufSize))
    {
#ifdef DEBUG
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            DBG_WARN("Failed GetUrlCacheEntryInfo, insufficient buffer");
        else
            TraceMsgA(llCacheGroupID ? TF_WARNING : TF_THISMODULE,
                "Failed GetUrlCacheEntryInfo (not in cache) URL=%ws", pszThisUrl);
#endif
        return E_FAIL;
    }

    // Add to new group
    if (llCacheGroupID != 0)
    {
        if (!SetUrlCacheEntryGroup(pszThisUrl, INTERNET_CACHE_GROUP_ADD,
            llCacheGroupID, NULL, 0, NULL))
        {
            switch (GetLastError())
            {
                case ERROR_FILE_NOT_FOUND:  //  Huh? Must not have been able to add the index entry?
                case ERROR_DISK_FULL:
                    return E_OUTOFMEMORY;

                case ERROR_NOT_ENOUGH_QUOTA:
                    return S_OK;            //  We do our own quota handling.

                default:
                    TraceMsgA(TF_WARNING | TF_THISMODULE, "GetUrlInfoAndMakeSticky: Got unexpected error from SetUrlCacheEntryGroup() - GLE = 0x%08x", GetLastError());
                    return E_FAIL;
            }
        }
    }

    return S_OK;
}

// GenerateCode will generate a DWORD code from a file.

#define ELEMENT_PER_READ        256
#define ELEMENT_SIZE            sizeof(DWORD)

HRESULT GenerateCode(LPCTSTR lpszLocalFileName, DWORD *pdwRet)
{
    DWORD dwCode=0;
    DWORD dwData[ELEMENT_PER_READ], i, dwRead;
    HRESULT hr = S_OK;
    HANDLE  hFile;

    hFile = CreateFile(lpszLocalFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            0, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        do
        {
            dwRead = 0;
            if (ReadFile(hFile, dwData, ELEMENT_PER_READ * ELEMENT_SIZE, &dwRead, NULL))
            {
                for (i=0; i<dwRead / ELEMENT_SIZE; i++)
                {
                    dwCode = (dwCode << 31) | (dwCode >> 1) + dwData[i];
//                  dwCode += dwData[i];
                }
            }   
        }
        while (ELEMENT_PER_READ * ELEMENT_SIZE == dwRead);

        CloseHandle(hFile);
    }
    else
    {
        hr = E_FAIL;
        TraceMsg(TF_THISMODULE|TF_WARNING,"GenerateCode: Unable to open cache file, Error=%x", GetLastError());
    }

    *pdwRet = dwCode;

    return hr;
}

// S_OK : We retrieved a good last modified or content code to use
// S_FALSE : We fell back to using the one passed into pvarChange
// E_FAIL : We failed miserably.
// E_INVALIDARG : Get a clue
// *pfGetContent : TRUE if we need a GET for PostCheckUrlForChange to work right
HRESULT PreCheckUrlForChange(LPCTSTR lpURL, VARIANT *pvarChange, BOOL *pfGetContent)
{
    BYTE    chBuf[MY_MAX_CACHE_ENTRY_INFO];

    LPINTERNET_CACHE_ENTRY_INFO lpInfo = (LPINTERNET_CACHE_ENTRY_INFO) chBuf;

    if (pvarChange->vt != VT_EMPTY && pvarChange->vt != VT_I4 && pvarChange->vt != VT_CY)
        return E_INVALIDARG;

    if (SUCCEEDED(GetUrlInfoAndMakeSticky(NULL, lpURL, lpInfo, sizeof(chBuf), 0)))
    {
        FILETIME ftOldLastModified = *((FILETIME *) &pvarChange->cyVal);

        if (lpInfo->LastModifiedTime.dwHighDateTime || lpInfo->LastModifiedTime.dwLowDateTime)
        {
            // We have a last modified time. Use it or the persisted one.

            if (pfGetContent)
                *pfGetContent = FALSE;

            if ((pvarChange->vt != VT_CY)
             || (lpInfo->LastModifiedTime.dwHighDateTime > ftOldLastModified.dwHighDateTime)
             || ((lpInfo->LastModifiedTime.dwHighDateTime == ftOldLastModified.dwHighDateTime)
                && (lpInfo->LastModifiedTime.dwLowDateTime > ftOldLastModified.dwLowDateTime)))
            {
                // Cache Last Modified is newer than saved Last Modified. Use cache's.
                pvarChange->vt = VT_CY;
                pvarChange->cyVal = *((CY *)&(lpInfo->LastModifiedTime));

                return S_OK;
            }

            ASSERT(pvarChange->vt == VT_CY);

            // Persisted Last Modified time is most recent. Use it.
            return S_OK;
        }

        DWORD dwCode;

        if (SUCCEEDED(GenerateCode(lpInfo->lpszLocalFileName, &dwCode)))
        {
            pvarChange->vt = VT_I4;
            pvarChange->lVal = (LONG) dwCode;

            if (pfGetContent)
                *pfGetContent = TRUE;

            return S_OK;
        }

        // Failed GenerateCode. Weird. Fall through.
    }

    if (pvarChange->vt != VT_EMPTY)
    {
        if (pfGetContent)
            *pfGetContent = (pvarChange->vt == VT_I4);

        return S_FALSE;
    }

    // We don't have old change detection, we don't have cache content, better GET
    if (pfGetContent)
        *pfGetContent = TRUE;

    return E_FAIL;  // Couldn't get anything. pvarChange->vt==VT_EMPTY
}

// S_FALSE : no change
// S_OK    : changed
// E_      : failure of some sort

// pvarChange from PreCheckUrlForChange. We return a new one.
// lpInfo  : must be valid if *pfGetContent was TRUE
// ftNewLastModified : must be filled in if *pfGetContent was FALSE
HRESULT PostCheckUrlForChange(VARIANT                    *pvarChange,
                              LPINTERNET_CACHE_ENTRY_INFO lpInfo,
                              FILETIME                    ftNewLastModified)
{
    HRESULT hr = S_FALSE;
    VARIANT varChangeNew;

    DWORD   dwNewCode = 0;

    if (!pvarChange || (pvarChange->vt != VT_I4 && pvarChange->vt != VT_CY && pvarChange->vt != VT_EMPTY))
        return E_INVALIDARG;

    varChangeNew.vt = VT_EMPTY;

    if (ftNewLastModified.dwHighDateTime || ftNewLastModified.dwLowDateTime)
    {
        varChangeNew.vt = VT_CY;
        varChangeNew.cyVal = *((CY *) &ftNewLastModified);
    }
    else
    {
        if (lpInfo &&
            SUCCEEDED(GenerateCode(lpInfo->lpszLocalFileName, &dwNewCode)))
        {
            varChangeNew.vt = VT_I4;
            varChangeNew.lVal = dwNewCode;
        }
    }

    if (pvarChange->vt == VT_CY)
    {
        // We have an old last modified time. Use that to determine change.
        FILETIME ftOldLastModified = *((FILETIME *) &(pvarChange->cyVal));

        if ((!ftNewLastModified.dwHighDateTime && !ftNewLastModified.dwLowDateTime)
            || (ftNewLastModified.dwHighDateTime > ftOldLastModified.dwHighDateTime)
            || ((ftNewLastModified.dwHighDateTime == ftOldLastModified.dwHighDateTime)
                && (ftNewLastModified.dwLowDateTime > ftOldLastModified.dwLowDateTime)))
        {
            // NewLastModified > OldLastModified (or we don't have a NewLastModified)
            DBG("PostCheckUrlForChange change detected via Last Modified");
            hr = S_OK;      // We have changed
        }
    }
    else if (pvarChange->vt == VT_I4)
    {
        // We have an old code. Use that to determine change.
        DWORD dwOldCode = (DWORD) (pvarChange->lVal);

        if ((dwOldCode != dwNewCode) ||
            !dwNewCode)
        {
            DBG("PostCheckUrlForChange change detected via content code");
            hr = S_OK;  // We have changed
        }
    }
    else
        hr = E_FAIL;    // No old code.

    *pvarChange = varChangeNew;

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// CHelperOM implementation
//
//////////////////////////////////////////////////////////////////////////

CHelperOM::CHelperOM(IHTMLDocument2 *pDoc)
{
    ASSERT(pDoc);
    m_pDoc = pDoc;
    if (pDoc)
        pDoc->AddRef();
}

CHelperOM::~CHelperOM()
{
    SAFERELEASE(m_pDoc);
}

HRESULT CHelperOM::GetTagCollection(
                        IHTMLDocument2          *pDoc,
                        LPCWSTR                  wszTagName,
                        IHTMLElementCollection **ppCollection)
{
    IHTMLElementCollection *pAll=NULL;
    IDispatch              *pDisp=NULL;
    VARIANT                 TagName;
    HRESULT                 hr;

    // We have to get "all", then sub-collection
    hr = pDoc->get_all(&pAll);
    if (pAll)
    {
        TagName.vt = VT_BSTR;
        TagName.bstrVal = SysAllocString(wszTagName);
        if (NULL == TagName.bstrVal)
            hr = E_OUTOFMEMORY;
        else
        {
            hr = pAll->tags(TagName, &pDisp);
            SysFreeString(TagName.bstrVal);
        }
        pAll->Release();
    }
    if (pDisp)
    {
        hr = pDisp->QueryInterface(IID_IHTMLElementCollection,
                                        (void **)ppCollection);
        pDisp->Release();
    }
    if (FAILED(hr)) DBG("GetSubCollection failed");

    return hr;
}


// Collections we get:
//
// IHTMLWindow2->get_document
//  IHTMLDocument2  ->get_links
//  IHTMLElementCollection->item
//                          ->get_hostname
//                          ->get_href
//                  ->get_all
//                      ->tags("map")
//  IHTMLElementCollection  ->item
//                              ->get_areas
//  IHTMLElementCollection          ->item
//  IHTMLAreaElement                    ->get_href
//                  ->get_all
//                      ->tags("meta")
//  IHTMLElementCollection  ->item
//                  ->get_all
//                      ->tags("frame")
//  IHTMLElementCollection  ->item
//                  ->get_all
//                      ->tags("iframe")
//  IHTMLElementCollection  ->item

// We recurse EnumCollection to get the maps (since
//      it's a collection of collections)


// hideous hack: IHTMLElementCollection can actually be IHTMLAreasCollection
//  the interface used to be derived from the other. It still has identical
//  methods. We typecast just in case that changes. Hopefully they will fix
//  so that Areas is derived from Element again.
HRESULT CHelperOM::EnumCollection(
            IHTMLElementCollection *pCollection,
            CWCStringList          *pStringList,
            CollectionType          Type,
            PFN_CB                  pfnCB,
            DWORD_PTR               dwCBData)
{
    IHTMLAnchorElement *pLink;
    IHTMLMapElement  *pMap;
    IHTMLAreaElement *pArea;
    IHTMLMetaElement *pMeta;
    IHTMLElement     *pEle;
    IDispatch        *pDispItem = NULL;

    HRESULT hr;
    BSTR    bstrItem=NULL;
    long    l, lCount;
    VARIANT vIndex, vEmpty, vData;
    BSTR    bstrTmp1, bstrTmp2;
    DWORD   dwStringData;

    VariantInit(&vEmpty);
    VariantInit(&vIndex);
    VariantInit(&vData);

    if (Type==CTYPE_MAP)
        hr = ((IHTMLAreasCollection *)pCollection)->get_length(&lCount);
    else
        hr = pCollection->get_length(&lCount);

    if (FAILED(hr))
        lCount = 0;

#ifdef DEBUG
    LPSTR lpDSTR[]={"Links","Maps","Areas (links) In Map", "Meta", "Frames"};
    TraceMsgA(TF_THISMODULE, "CWebCrawler::GetCollection, %d %s found", lCount, lpDSTR[(int)Type]);
#endif

    for (l=0; l<lCount; l++)
    {
        vIndex.vt = VT_I4;
        vIndex.lVal = l;
        dwStringData = 0;

        if (Type==CTYPE_MAP)
            hr = ((IHTMLAreasCollection *)pCollection)->item(vIndex, vEmpty, &pDispItem);
        else
            hr = pCollection->item(vIndex, vEmpty, &pDispItem);

        if (SUCCEEDED(hr))
        {
            ASSERT(vData.vt == VT_EMPTY);
            ASSERT(!bstrItem);

            if (pDispItem)
            {
                // Get the URL from the IDispatch
                switch(Type)
                {
                    case CTYPE_LINKS:       // get href from <a>
                        hr = pDispItem->QueryInterface(IID_IHTMLAnchorElement, (void **)&pLink);

                        if (SUCCEEDED(hr) && pLink)
                        {
                            hr = pLink->get_href(&bstrItem);
                            pLink->Release();
                        }
                        break;

                    case CTYPE_MAPS:    // enumeration areas for this map
                        hr = pDispItem->QueryInterface(IID_IHTMLMapElement, (void **)&pMap);

                        if (SUCCEEDED(hr) && pMap)
                        {
                            IHTMLAreasCollection *pNewCollection=NULL;
                            // This gives us another collection. Enumerate it
                            //  for the strings.
                            hr = pMap->get_areas(&pNewCollection);
                            if (pNewCollection)
                            {
                                hr = EnumCollection((IHTMLElementCollection *)pNewCollection, pStringList, CTYPE_MAP, pfnCB, dwCBData);
                                pNewCollection->Release();
                            }
                            pMap->Release();
                        }
                        break;

                    case CTYPE_MAP:     // get href for this area
                        hr = pDispItem->QueryInterface(IID_IHTMLAreaElement, (void **)&pArea);

                        if (SUCCEEDED(hr) && pArea)
                        {
                            hr = pArea->get_href(&bstrItem);
                            pArea->Release();
                        }
                        break;

                    case CTYPE_META:    // get meta name and content as single string
                        hr = pDispItem->QueryInterface(IID_IHTMLMetaElement, (void **)&pMeta);

                        if (SUCCEEDED(hr) && pMeta)
                        {
                            pMeta->get_name(&bstrTmp1);
                            pMeta->get_content(&bstrTmp2);
                            if (bstrTmp1 && bstrTmp2 && *bstrTmp1 && *bstrTmp2)
                            {
                                bstrItem = SysAllocStringLen(NULL, lstrlenW(bstrTmp1) +
                                                                   lstrlenW(bstrTmp2) + 1);

                                StrCpyW(bstrItem, bstrTmp1);
                                StrCatW(bstrItem, L"\n");
                                StrCatW(bstrItem, bstrTmp2);
                            }
                            SysFreeString(bstrTmp1);
                            SysFreeString(bstrTmp2);
                            pMeta->Release();
                        }
                        break;

                    case CTYPE_FRAMES:      // get "src" attribute
                        hr = pDispItem->QueryInterface(IID_IHTMLElement, (void **)&pEle);

                        if (SUCCEEDED(hr) && pEle)
                        {
                            bstrTmp1 = SysAllocString(L"SRC");

                            if (bstrTmp1)
                            {
                                hr = pEle->getAttribute(bstrTmp1, VARIANT_FALSE, &vData);
                                if (SUCCEEDED(hr) && vData.vt == VT_BSTR)
                                {
                                    bstrItem = vData.bstrVal;
                                    vData.vt = VT_EMPTY;
                                }
                                else
                                    VariantClear(&vData);

                                SysFreeString(bstrTmp1);
                            }
                            else
                            {
                                hr = E_FAIL;
                            }

                            pEle->Release();
                        }
                        break;

                    default:
                        ASSERT(0);
                        // bug in calling code
                }

                if (SUCCEEDED(hr) && bstrItem)
                {
                    // Verify we want to add this item to string list & get data
                    if (pfnCB)
                        hr = pfnCB(pDispItem, &bstrItem, dwCBData, &dwStringData);

                    if (SUCCEEDED(hr) && bstrItem && pStringList)
                        pStringList->AddString(bstrItem, dwStringData);
                }
                SAFERELEASE(pDispItem);
                SAFEFREEBSTR(bstrItem);
            }
        }
        if (E_ABORT == hr)
        {
            DBG_WARN("Aborting enumeration in CHelperOM::EnumCollection at callback's request.");
            break;
        }
    }

    return hr;
}


// Gets all urls from a collection, recursing through frames
HRESULT CHelperOM::GetCollection(
    IHTMLDocument2 *pDoc,
    CWCStringList  *pStringList,
    CollectionType  Type,
    PFN_CB          pfnCB,
    DWORD_PTR       dwCBData)
{
    HRESULT         hr;

    // Get the collection from the document
    ASSERT(pDoc);
    ASSERT(pStringList || pfnCB);

    hr = _GetCollection(pDoc, pStringList, Type, pfnCB, dwCBData);

#if 0
    // we enumerate for the top-level interface, then for subframes if any
    long lLen=0, lIndex;
    VARIANT vIndex, vResult;
    vResult.vt = VT_EMPTY;
    IHTMLWindow2 *pWin2=NULL;

    if (SUCCEEDED(pWin->get_length(&lLen)) && lLen>0)
    {
        TraceMsg(TF_THISMODULE, "Also enumerating for %d subframes", (int)lLen);
        for (lIndex=0; lIndex<lLen; lIndex++)
        {
            vIndex.vt = VT_I4;
            vIndex.lVal = lIndex;

            pWin->item(&vIndex, &vResult);

            if (vResult.vt == VT_DISPATCH && vResult.pdispVal)
            {
                if (SUCCEEDED(vResult.pdispVal->QueryInterface(IID_IHTMLWindow2, (void **)&pWin2)) && pWin2)
                {
                    GetCollection(pWin2, pStringList, Type);
                    pWin2->Release();
                    pWin2=NULL;
                }
            }
            VariantClear(&vResult);
        }
    }
#endif

    return hr;
}

// get all urls from a collection
HRESULT CHelperOM::_GetCollection(
    IHTMLDocument2 *pDoc,
    CWCStringList  *pStringList,
    CollectionType  Type,
    PFN_CB          pfnCB,
    DWORD_PTR       dwCBData)
{
    HRESULT         hr;
    IHTMLElementCollection *pCollection=NULL;

    // From IHTMLDocument2 we get IHTMLElementCollection, then enumerate for the urls

    // Get appropriate collection from document
    switch (Type)
    {
        case CTYPE_LINKS:
            hr = pDoc->get_links(&pCollection);
            break;
        case CTYPE_MAPS:
            hr = GetTagCollection(pDoc, L"map", &pCollection);
            break;
        case CTYPE_META:
            hr = GetTagCollection(pDoc, L"meta", &pCollection);
            break;
        case CTYPE_FRAMES:
            hr = GetTagCollection(pDoc, L"frame", &pCollection);
            break;

        default:
            hr = E_FAIL;
    }
    if (!pCollection) hr=E_NOINTERFACE;
#ifdef DEBUG
    if (FAILED(hr)) DBG_WARN("CWebCrawler::_GetCollection:  get_collection failed");
#endif

    if (SUCCEEDED(hr))
    {
        hr = EnumCollection(pCollection, pStringList, Type, pfnCB, dwCBData);

        // If we're getting frames, we need to enum "iframe" tags separately
        if (SUCCEEDED(hr) && (Type == CTYPE_FRAMES))
        {
            SAFERELEASE(pCollection);
            hr = GetTagCollection(pDoc, L"iframe", &pCollection);

            if (SUCCEEDED(hr) && pCollection)
            {
                hr = EnumCollection(pCollection, pStringList, Type, pfnCB, dwCBData);
            }
        }
    }

    if (pCollection)
        pCollection->Release();

    return hr;
}

extern HRESULT LoadWithCookie(LPCTSTR, POOEBuf, DWORD *, SUBSCRIPTIONCOOKIE *);

// IExtractIcon members
STDMETHODIMP CWebCrawler::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    IUniformResourceLocator* pUrl = NULL;
    IExtractIcon* pUrlIcon = NULL;
    HRESULT hr = S_OK;
    BOOL bCalledCoInit = FALSE;

    if (!szIconFile || !piIndex || !pwFlags)
        return E_INVALIDARG;
    //zero out return values in case one of the COM calls fails...
    *szIconFile = 0;
    *piIndex = -1;

    if (!m_pBuf)    {
        m_pBuf = (POOEBuf)MemAlloc(LPTR, sizeof(OOEBuf));
        if (!m_pBuf)
            return E_OUTOFMEMORY;

        DWORD   dwSize;
        hr = LoadWithCookie(NULL, m_pBuf, &dwSize, &m_SubscriptionCookie);
        RETURN_ON_FAILURE(hr);
    }


    if (m_pBuf->bDesktop)
    {
        StrCpyN(szIconFile, TEXT(":desktop:"), cchMax);
    }
    else
    {
        if (m_pUrlIconHelper)
        {
            hr = m_pUrlIconHelper->GetIconLocation (uFlags, szIconFile, cchMax, piIndex, pwFlags);
        }
        else
        {
            hr = CoCreateInstance (CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (void**)&pUrl);
            if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
                SUCCEEDED (CoInitialize(NULL)))
            {
                bCalledCoInit = TRUE;
                hr = CoCreateInstance (CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (void**)&pUrl);
            }

            if (SUCCEEDED (hr))
            {
                hr = pUrl->SetURL (m_pBuf->m_URL, 1);
                if (SUCCEEDED (hr))
                {
                    hr = pUrl->QueryInterface (IID_IExtractIcon, (void**)&pUrlIcon);
                    if (SUCCEEDED (hr))
                    {
                        hr = pUrlIcon->GetIconLocation (uFlags, szIconFile, cchMax, piIndex, pwFlags);

                        //pUrlIcon->Release();  //released in destructor
                        ASSERT (m_pUrlIconHelper == NULL);
                        m_pUrlIconHelper = pUrlIcon;
                    }
                }
                pUrl->Release();
            }

            //balance CoInit with CoUnit
            //(we still have a pointer to the CLSID_InternetShortcut object, m_pUrlIconHelper,
            //but since that code is in shdocvw there's no danger of it getting unloaded and
            //invalidating our pointer, sez cdturner.)
            if (bCalledCoInit)
                CoUninitialize();
        }
    }

    return hr;
}

STDMETHODIMP CWebCrawler::Extract(LPCTSTR szIconFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    HRESULT hr = S_OK;

    if (!phiconLarge || !phiconSmall)
        return E_INVALIDARG;

    //zero out return values in case one of the COM calls fails...
    *phiconLarge = NULL;
    *phiconSmall = NULL;

    if ((NULL != m_pBuf) && (m_pBuf->bDesktop))
    {
        LoadDefaultIcons();
        *phiconLarge = *phiconSmall = g_desktopIcon;
    }
    else
    {
        if (!m_pUrlIconHelper)
            return E_FAIL;

        hr = m_pUrlIconHelper->Extract (szIconFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
    }

    return hr;
}
