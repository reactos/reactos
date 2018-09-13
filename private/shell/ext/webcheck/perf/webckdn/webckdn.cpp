#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right for oleaut32.h, we are defing these
#define _WINMM_         // get DECLSPEC_IMPORT stuff right for mmsystem.h, we are defing these
#define _INTSHCUT_      // get DECLSPEC_IMPORT stuff right for intshcut.h, we are defing these
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for wininet.h, we are defing these
#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet.h, we are defing these

#define INC_OLE2
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <ccstock.h>
#include <ole2.h>
#include <ole2ver.h>
#include <oleauto.h>
#include <docobj.h>
#include <shlwapi.h>
#include <wininet.h>   // INTERNET_MAX_URL_LENGTH.  Must be before shlobjp.h!
#include <shlobj.h>
#include <shlobjp.h>
#include <msxml.h>
#include <subsmgr.h>
#include <webcheck.h>
#include "iimgctx.h"

#ifdef _DEBUG
#ifdef _X86_
// Use int 3 so we stop immediately in the source
#define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
#define DEBUG_BREAK        do { _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#endif

#define ASSERT(exp) \
if(!exp) \
{        \
    printf("ASSERT: %s %s (%s) failed\r\n", __FILE__, __LINE__, TEXT(#exp)); \
    DEBUG_BREAK; \
}        \

#else
#define ASSERT(exp)
#endif

#define DBGOUT(s) printf("%s\r\n", s)

#ifndef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; } else
#endif
#ifndef SAFEFREEBSTR
#define SAFEFREEBSTR(p) if ((p) != NULL) { SysFreeString(p); (p) = NULL; } else
#endif
#ifndef SAFEFREEOLESTR
#define SAFEFREEOLESTR(p) if ((p) != NULL) { CoTaskMemFree(p); (p) = NULL; } else
#endif
#ifndef SAFELOCALFREE
#define SAFELOCALFREE(p) if ((p) != NULL) { LocalFree(p); (p) = NULL; } else
#endif
#ifndef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; } else
#endif

#define ON_FAILURE_RETURN(HR)   {if(FAILED(HR)) return (HR);}
#define MAX_RES_STRING_LEN 128      // max resource string len for WriteStringRes

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Notification property names
// Agent Start
extern const WCHAR  c_szPropURL[] = L"URL";
extern const WCHAR  c_szPropBaseURL[] = L"BaseURL";
extern const WCHAR  c_szPropName[] = L"Name";
extern const WCHAR  c_szPropPriority[] = L"Priority";   // BUGBUG: remove this soon
extern const WCHAR  c_szPropAgentFlags[] = L"AgentFlags";
extern const WCHAR  c_szPropCrawlLevels[] = L"RecurseLevels";
extern const WCHAR  c_szPropCrawlFlags[] = L"RecurseFlags";
extern const WCHAR  c_szPropCrawlMaxSize[] = L"MaxSizeKB";
extern const WCHAR  c_szPropCrawlChangesOnly[] = L"CheckChangesOnly";
extern const WCHAR  c_szPropCrawlExemptPeriod[] = L"ExemptPeriod";
extern const WCHAR  c_szPropCrawlUsername[] = L"Username";
extern const WCHAR  c_szPropCrawlPassword[] = L"Password";
extern const WCHAR  c_szPropEmailNotf[] = L"EmailNotification";
extern const WCHAR  c_szPropCrawlLocalDest[] = L"LocalDest";
extern const WCHAR  c_szPropCrawlGroupID[] = L"GroupID";
extern const WCHAR  c_szPropCrawlActualSize[] = L"ActualSizeKB";
extern const WCHAR  c_szPropEnableShortcutGleam[] = L"EnableShortcutGleam";
extern const WCHAR  c_szPropCDFStartCookie[] = L"CDFStartCookie";
extern const WCHAR  c_szPropChannelFlags[] = L"ChannelFlags";
extern const WCHAR  c_szPropAuthMethod[] = L"AuthMethod";
extern const WCHAR  c_szPropAuthDomain[] = L"AuthDomain";
extern const WCHAR  c_szPropChannel[] = L"Channel";
extern const WCHAR  c_szPropDesktopComponent[] = L"DesktopComponent";

// Agent Control
extern const WCHAR  c_szPropControlType[] = L"ControlType";
// Progress Report
extern const WCHAR  c_szPropProgress[] = L"Progress";
extern const WCHAR  c_szPropProgressMax[] = L"ProgressMax";
extern const WCHAR  c_szPropCurrentURL[] = L"CurrentURL";
// End Report
extern const WCHAR  c_szPropStatusCode[] = L"StatusCode";
extern const WCHAR  c_szPropStatusString[] = L"StatusString";
extern const WCHAR  c_szPropCompletionTime[] = L"CompletionTime";
extern const WCHAR  c_szPropEmailURL[] = L"EmailURL";

// Tray Agent Properties
extern const WCHAR  c_szPropGuidsArr[] = L"Guids Array";

// Update Agent Properties
extern const WCHAR  c_szTimeStamp[] = L"Update TS";

// Tracking Properties
extern const WCHAR  c_szTrackingCookie[] = L"LogGroupID";
extern const WCHAR  c_szTrackingPostURL[] = L"PostURL";
extern const WCHAR  c_szPostingRetry[] = L"PostFailureRetry";
extern const WCHAR  c_szPostHeader[] = L"PostHeader";

// Delivery Agent Properties
extern const WCHAR  c_szStartCookie[] = L"StartCookie";

// Initial cookie in AGENT_INIT
extern const WCHAR  c_szInitCookie[] = L"InitCookie";

// Helper function protos
int MyOleStrToStrN(LPSTR psz, int cchMultiByte, LPCOLESTR pwsz);
int MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCSTR psz);
HRESULT ReadBSTR(ISubscriptionItem *pItem, LPCWSTR szName, BSTR *bstrRet);
HRESULT ReadOLESTR(ISubscriptionItem *pItem, LPCWSTR szName, LPWSTR *ppszRet);
HRESULT ReadAnsiSTR(ISubscriptionItem *pItem, LPCWSTR szName, LPSTR *ppszRet);
HRESULT ReadBool(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT_BOOL *pBoolRet);
HRESULT ReadSCODE(ISubscriptionItem *pItem, LPCWSTR szName, SCODE *pscRet);
HRESULT WriteEMPTY(ISubscriptionItem *pItem, LPCWSTR szName);
HRESULT WriteSCODE(ISubscriptionItem *pItem, LPCWSTR szName, SCODE scVal);
HRESULT ReadDWORD(ISubscriptionItem *pItem, LPCWSTR szName, DWORD *pdwRet);
HRESULT ReadLONGLONG(ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG *pllRet);
HRESULT ReadGUID(ISubscriptionItem *pItem, LPCWSTR szName, GUID *pGuid);
HRESULT WriteGUID(ISubscriptionItem *pItem, LPCWSTR szName, GUID *pGuid);
HRESULT WriteLONGLONG(ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG llVal);
HRESULT WriteDWORD(ISubscriptionItem *pItem, LPCWSTR szName, DWORD dwVal);
HRESULT ReadDATE(ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal);
HRESULT WriteDATE(ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal);
HRESULT ReadVariant(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarRet);
HRESULT WriteVariant(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarVal);
HRESULT WriteOLESTR(ISubscriptionItem *pItem, LPCWSTR szName, LPCWSTR szVal);
HRESULT WriteAnsiSTR(ISubscriptionItem *pItem, LPCWSTR szName, LPCSTR szVal);

///////////////////////////////////////////////////////////////////////
// CLASSSES
///////////////////////////////////////////////////////////////////////
#if 0
class CConApp;

//////////////////////////////////////////////////////////////////////////
class CRunDeliveryAgentSink
{
private:
    int m_iActive;
    
public:
    CRunDeliveryAgentSink()
    {
        m_iActive = 0;
    }

    virtual HRESULT OnAgentBegin()
    {
        m_iActive++;
        return S_OK;
    }

    // OnAgentProgress not currently called
    virtual HRESULT OnAgentProgress()
    { 
        return E_NOTIMPL; 
    }
    
    // OnAgentEnd called when agent is complete. fSynchronous means that StartAgent call
    //  has not yet returned; hrResult will be returned from StartAgent
    virtual HRESULT OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
                               long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                               BOOL fSynchronous)
    {
        m_iActive--;
        return S_OK;
    }

    virtual int AgentActive()
    {
        return m_iActive;
    }
};
#endif

//////////////////////////////////////////////////////////////////////////
//
// CRunDeliveryAgent object
// Will run a delivery agent and host it for you
// Create, call Init, then call StartAgent
// Use static function SafeRelease to safely release this class.
//
//////////////////////////////////////////////////////////////////////////
class CConApp;

class CRunDeliveryAgent : public ISubscriptionAgentEvents
{
protected:
    virtual ~CRunDeliveryAgent();

///    CRunDeliveryAgentSink *m_pParent;
    CConApp*    m_pParent;

    ULONG           m_cRef;

    ISubscriptionItem         *m_pItem;
    ISubscriptionAgentControl *m_pAgent;

    HRESULT     m_hrResult;
    BOOL        m_fInStartAgent;

    CLSID       m_clsidDest;
    
    void        CleanUp();

public:
    CRunDeliveryAgent();

    HRESULT Init(CConApp *pParent, ISubscriptionItem *pItem, REFCLSID rclsidDest);

    inline static void SafeRelease(CRunDeliveryAgent * &pThis)
    { 
        if (pThis) 
        { 
            pThis->m_pParent=NULL; pThis->Release(); pThis=NULL; 
        } 
    }

    HRESULT CreateNewItem(ISubscriptionItem **ppItem, REFCLSID rclsidAgent);

    // StartAgent will return E_PENDING if agent is running. Otherwise it will return
    //  synchronous result code from agent.
    HRESULT     StartAgent();

    HRESULT     AgentPause(DWORD dwFlags);
    HRESULT     AgentResume(DWORD dwFlags);
    HRESULT     AgentAbort(DWORD dwFlags);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **ppunk);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISubscriptionAgentEvents members
    STDMETHODIMP UpdateBegin(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie);
    STDMETHODIMP UpdateProgress(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                        long lSizeDownloaded, long lProgressCurrent, long lProgressMax,
                        HRESULT hrStatus, LPCWSTR wszStatus);
    STDMETHODIMP UpdateEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                            long lSizeDownloaded,
                            HRESULT hrResult, LPCWSTR wszResult);
    STDMETHODIMP ReportError(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                             HRESULT hrError, LPCWSTR wszError);
};

//---------------------------------------------------------------------
class CConApp
{
private:
    int m_argc;
    char **m_argv;
    char *m_pszURL;
    char *m_pRunStr;
    char *m_pTestName;
    char m_CmdLine[1024];
    int m_iActive;
    DWORD m_dwTime;     // Download time
    DWORD m_dwFlags;
    DWORD m_dwLevels;
    DWORD m_dwChannel;
    DWORD m_dwChannelFlags;
    BOOL m_bVerbose;
    BOOL m_bPreLoad;
    BOOL m_bChannelAgent;
    
public:
    CConApp(int argc, char **argv);
    ~CConApp();
    HRESULT Init();
    HRESULT Download();
    BOOL PrintResults();
    BOOL ParseCommandLine();
    void Display_Usage();
    BOOL Verbose();
    HRESULT MessageLoop();

    // Delivery agent events
    virtual HRESULT OnAgentBegin();
    virtual HRESULT OnAgentProgress();
    virtual HRESULT OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
                           long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                           BOOL fSynchronous);
};


///////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------
int MyOleStrToStrN(LPSTR psz, int cchMultiByte, LPCOLESTR pwsz)
{
    int i;
    i=WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz,
                    cchMultiByte, NULL, NULL);
    if (!i)
    {
        DBGOUT("MyOleStrToStrN string too long; truncated");
        psz[cchMultiByte-1]=0;
    }
#ifdef DEBUG
    else
        ZeroMemory(psz+i, sizeof(TCHAR)*(cchMultiByte-i));
#endif

    return i;
}

//---------------------------------------------------------------------
int MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCSTR psz)
{
    int i;
    i=MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
    if (!i)
    {
        DBGOUT("MyStrToOleStrN string too long; truncated");
        pwsz[cchWideChar-1]=0;
    }
#ifdef DEBUG
    else
        ZeroMemory(pwsz+i, sizeof(OLECHAR)*(cchWideChar-i));
#endif

    return i;
}


//---------------------------------------------------------------------
// Can return S_OK with NULL bstr
HRESULT ReadBSTR(ISubscriptionItem *pItem, LPCWSTR szName, BSTR *bstrRet)
{
    ASSERT(pItem && bstrRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_BSTR))
    {
        *bstrRet = Val.bstrVal;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        *bstrRet = NULL;
        return E_INVALIDARG;
    }
}

//---------------------------------------------------------------------
// Cannot return S_OK with emptry string
HRESULT ReadOLESTR(ISubscriptionItem *pItem, LPCWSTR szName, LPWSTR *ppszRet)
{
    HRESULT hr;
    BSTR bstrRet = NULL;
    *ppszRet = NULL;
    hr = ReadBSTR(pItem, szName, &bstrRet);
    if (SUCCEEDED(hr) && bstrRet && bstrRet[0])
    {
        int len = (lstrlenW(bstrRet) + 1) * sizeof(WCHAR);
        *ppszRet = (LPWSTR) CoTaskMemAlloc(len);
        if (*ppszRet)
        {
            CopyMemory(*ppszRet, bstrRet, len);
        }
    }
    
    SAFEFREEBSTR(bstrRet);
    if (*ppszRet)
        return S_OK;
    else
        return E_FAIL;
}

//---------------------------------------------------------------------
HRESULT ReadAnsiSTR(ISubscriptionItem *pItem, LPCWSTR szName, LPSTR *ppszRet)
{
    HRESULT hr;
    BSTR bstrRet = NULL;
    *ppszRet = NULL;
    hr = ReadBSTR(pItem, szName, &bstrRet);
    if (SUCCEEDED(hr) && bstrRet && bstrRet[0])
    {
        // Don't forget to allocate a long string for DBCS.
        int len = (lstrlenW(bstrRet) + 1) * sizeof(CHAR) * 2;
        *ppszRet = (LPSTR) LocalAlloc(NULL, len);
        if (*ppszRet)
        {
            MyOleStrToStrN(*ppszRet, len, bstrRet);
        }
    }
    
    SAFEFREEBSTR(bstrRet);
    if (*ppszRet)
        return S_OK;
    else
        return E_FAIL;
}

//---------------------------------------------------------------------
HRESULT ReadBool(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT_BOOL *pBoolRet)
{
    ASSERT(pItem && pBoolRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    // accept VT_I4 or VT_BOOL
    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_BOOL || Val.vt==VT_I4))
    {
        if (Val.vt==VT_I4)
        {
            if (Val.lVal)
                *pBoolRet = VARIANT_TRUE;
            else
                *pBoolRet = VARIANT_FALSE;
        }
        else
            *pBoolRet = Val.boolVal;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}

//---------------------------------------------------------------------
HRESULT ReadSCODE(ISubscriptionItem *pItem, LPCWSTR szName, SCODE *pscRet)
{
    ASSERT(pItem && pscRet);

    VARIANT Val;

    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) && Val.vt == VT_ERROR)
    {
        *pscRet = Val.scode;
        return S_OK;
    }
    else
    {
        VariantClear(&Val);
        return E_INVALIDARG;
    }
}

//---------------------------------------------------------------------
HRESULT WriteEMPTY(ISubscriptionItem *pItem, LPCWSTR szName)
{
    ASSERT(pItem);

    VARIANT Val;

    Val.vt = VT_EMPTY;
    return pItem->WriteProperties(1, &szName, &Val);
}

//---------------------------------------------------------------------
HRESULT WriteSCODE(ISubscriptionItem *pItem, LPCWSTR szName, SCODE scVal)
{
    ASSERT(pItem);

    VARIANT Val;

    Val.vt = VT_ERROR;
    Val.scode = scVal;

    return pItem->WriteProperties(1, &szName, &Val);
}
    
//---------------------------------------------------------------------
HRESULT ReadDWORD(ISubscriptionItem *pItem, LPCWSTR szName, DWORD *pdwRet)
{
    ASSERT(pItem && pdwRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_I4 || Val.vt==VT_I2))
    {
        if (Val.vt==VT_I4)
            *pdwRet = (DWORD) Val.lVal;
        else
            *pdwRet = (DWORD) Val.iVal;

        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}

//---------------------------------------------------------------------
HRESULT ReadLONGLONG(ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG *pllRet)
{
    ASSERT(pItem && pllRet);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) &&
            (Val.vt==VT_CY))
    {
        *pllRet = *((LONGLONG *) &(Val.cyVal));

        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}
    
//---------------------------------------------------------------------
HRESULT ReadGUID(ISubscriptionItem *pItem, LPCWSTR szName, GUID *pGuid)
{
    ASSERT(pItem && pGuid);

    BSTR    bstrGUID = NULL;
    HRESULT hr = E_INVALIDARG;
    
    if (SUCCEEDED(ReadBSTR(pItem, szName, &bstrGUID)) &&
        SUCCEEDED(CLSIDFromString(bstrGUID, pGuid)))
    {
        hr = NOERROR;
    }
    SAFEFREEBSTR(bstrGUID);

    return hr;
}

//---------------------------------------------------------------------
HRESULT WriteGUID(ISubscriptionItem *pItem, LPCWSTR szName, GUID *pGuid)
{
    ASSERT(pItem && pGuid);
    
    WCHAR   wszCookie[GUIDSTR_MAX];

#ifdef DEBUG
    int len = 
#endif
    StringFromGUID2(*pGuid, wszCookie, sizeof(wszCookie));
    ASSERT(GUIDSTR_MAX == len);
    return WriteOLESTR(pItem, szName, wszCookie);
}

//---------------------------------------------------------------------
HRESULT WriteLONGLONG(ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG llVal)
{
    VARIANT Val;

    Val.vt = VT_CY;
    Val.cyVal = *((CY *) &llVal);

    return pItem->WriteProperties(1, &szName, &Val);
}

//---------------------------------------------------------------------
HRESULT WriteDWORD(ISubscriptionItem *pItem, LPCWSTR szName, DWORD dwVal)
{
    VARIANT Val;

    Val.vt = VT_I4;
    Val.lVal = dwVal;

    return pItem->WriteProperties(1, &szName, &Val);
}

//---------------------------------------------------------------------
HRESULT ReadDATE(ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal)
{
    ASSERT(pItem && dtVal);

    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pItem->ReadProperties(1, &szName, &Val)) && (Val.vt==VT_DATE))
    {
        *dtVal = Val.date;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        return E_INVALIDARG;
    }
}

//---------------------------------------------------------------------
HRESULT WriteDATE(ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal)
{
    VARIANT Val;

    Val.vt = VT_DATE;
    Val.date= *dtVal;

    return pItem->WriteProperties(1, &szName, &Val);
}

//---------------------------------------------------------------------
HRESULT ReadVariant(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarRet)
{
    ASSERT(pvarRet->vt == VT_EMPTY);
    return pItem->ReadProperties(1, &szName, pvarRet);
}

//---------------------------------------------------------------------
HRESULT WriteVariant(ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarVal)
{
    return pItem->WriteProperties(1, &szName, pvarVal);
}

//---------------------------------------------------------------------
HRESULT WriteOLESTR(ISubscriptionItem *pItem, LPCWSTR szName, LPCWSTR szVal)
{
    VARIANT Val;

    Val.vt = VT_BSTR;
    Val.bstrVal = SysAllocString(szVal);

    HRESULT hr = pItem->WriteProperties(1, &szName, &Val);

    SysFreeString(Val.bstrVal);

    return hr;
}

//---------------------------------------------------------------------
HRESULT WriteAnsiSTR(ISubscriptionItem *pItem, LPCWSTR szName, LPCSTR szVal)
{
    VARIANT Val;
    BSTR    bstrVal;
    int     iLen;
    HRESULT hr;

    iLen = lstrlen(szVal);
    bstrVal = SysAllocStringLen(NULL, iLen);
    if (bstrVal)
    {
        MyStrToOleStrN(bstrVal, iLen + 1, szVal);

        Val.vt = VT_BSTR;
        Val.bstrVal = bstrVal;

        hr = pItem->WriteProperties(1, &szName, &Val);

        SysFreeString(bstrVal);
    }

    return hr;
}

//==============================================================================
// CRunDeliveryAgent provides generic support for synchronous operation of a
//   delivery agent
// It is aggregatable so that you can add more interfaces to the callback
//
// Taken from webcheck\cdfagent.cpp
//==============================================================================
CRunDeliveryAgent::CRunDeliveryAgent()
{
    m_cRef = 1;
}

//---------------------------------------------------------------------
HRESULT CRunDeliveryAgent::Init(CConApp* pParent,
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

//---------------------------------------------------------------------
CRunDeliveryAgent::~CRunDeliveryAgent()
{
    CleanUp();
}

//
// IUnknown members
//
//---------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRunDeliveryAgent::AddRef(void)
{
    return ++m_cRef;
}

//---------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRunDeliveryAgent::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

//---------------------------------------------------------------------
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
//---------------------------------------------------------------------
STDMETHODIMP CRunDeliveryAgent::UpdateBegin(const SUBSCRIPTIONCOOKIE *)
{
    if (m_pParent)
        m_pParent->OnAgentBegin();
    return S_OK;
}

//---------------------------------------------------------------------
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

//---------------------------------------------------------------------
#define INET_S_AGENT_BASIC_SUCCESS           _HRESULT_TYPEDEF_(0x000C0F8FL) // From webcheck/delagent.h

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

//---------------------------------------------------------------------
STDMETHODIMP CRunDeliveryAgent::ReportError(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        HRESULT hrError, 
        LPCWSTR wszError)
{
    return S_FALSE;
}

//---------------------------------------------------------------------
HRESULT CRunDeliveryAgent::StartAgent()
{
    HRESULT hr;

    if (!m_pParent || !m_pItem || m_pAgent)
        return E_FAIL;

    AddRef();   // Release before we return from this function
    m_fInStartAgent = TRUE;

    m_hrResult = INET_S_AGENT_BASIC_SUCCESS;

    if(m_pParent->Verbose())
        DBGOUT("Using new interfaces to host agent");

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

//---------------------------------------------------------------------
HRESULT CRunDeliveryAgent::AgentPause(DWORD dwFlags)
{
    if (m_pAgent)
        return m_pAgent->PauseUpdate(0);

    if(m_pParent->Verbose())
        DBGOUT("CRunDeliveryAgent::AgentPause with no running agent!!");
    return S_FALSE;
}

//---------------------------------------------------------------------
HRESULT CRunDeliveryAgent::AgentResume(DWORD dwFlags)
{
    if (m_pAgent)
        return m_pAgent->ResumeUpdate(0);

    if(m_pParent->Verbose())
        DBGOUT("CRunDeliveryAgent::AgentResume with no running agent!!");

    return E_FAIL;
}

//---------------------------------------------------------------------
HRESULT CRunDeliveryAgent::AgentAbort(DWORD dwFlags)
{
    if (m_pAgent)
        return m_pAgent->AbortUpdate(0);

    if(m_pParent->Verbose())
        DBGOUT("CRunDeliveryAgent::AgentAbort with no running agent!!");
    return S_FALSE;
}

//---------------------------------------------------------------------
void CRunDeliveryAgent::CleanUp()
{
    SAFERELEASE(m_pItem);
    SAFERELEASE(m_pAgent);
    m_pParent = NULL;
}

//---------------------------------------------------------------------
HRESULT CRunDeliveryAgent::CreateNewItem(ISubscriptionItem **ppItem, REFCLSID rclsidAgent)
{
    ISubscriptionMgrPriv *pSubsMgrPriv=NULL;
    SUBSCRIPTIONITEMINFO info;

    *ppItem = NULL;

    HRESULT hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
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
    else
    {
        printf("CoCreateInstance IID_ISubscriptionMgrPriv failed. hr=0x%x\r\n", hr);
    }

    return (*ppItem) ? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------
CConApp::CConApp(int argc, char **argv)
{
    m_argc = argc;
    m_argv = argv;
    m_dwTime = 0;
    m_pszURL = NULL;
    m_bVerbose = FALSE;
    m_bPreLoad = FALSE;
    m_bChannelAgent = FALSE;
    m_dwFlags = 3;
    m_dwLevels = 0;
    m_dwChannel = 1;
    m_dwChannelFlags = CHANNEL_AGENT_PRECACHE_ALL;
    m_pRunStr = NULL;
    m_pTestName = NULL;
    m_iActive = 0;
}

//---------------------------------------------------------------------
CConApp::~CConApp()
{
    CoUninitialize();
}

//---------------------------------------------------------------------
HRESULT CConApp::Init()
{
    HRESULT hr = CoInitialize(NULL);
    ON_FAILURE_RETURN(hr);
        
    if(!ParseCommandLine())
        return(E_FAIL);

    return(S_OK);
}

//---------------------------------------------------------------------
HRESULT CConApp::Download()
{
    ISubscriptionItem  *pSubscriptItem = NULL;
    IImgCtx *pImgCtx = NULL;
    IClassFactory *pImageCF = NULL;

    if(m_bVerbose)
    {
        printf("URL=%s\r\n", m_pszURL);
        if(m_bPreLoad)
            printf("Preloading Mshtml\r\n");
        if(m_bChannelAgent)
            printf("ChannelAgent: Channel=%d Flags=0x%x\r\n", m_dwChannel, m_dwChannelFlags);
        else
            printf("WebCrawlerAgent:  Levels=%d Flags=0x%x\r\n", m_dwFlags, m_dwLevels);
    }
    
    HRESULT hr = S_OK;
    CLSID clsid;

    if (m_bChannelAgent)
        clsid = CLSID_ChannelAgent;
    else
        clsid = CLSID_WebCrawlerAgent;

    CRunDeliveryAgent *prda = new CRunDeliveryAgent;
    
    hr = prda->CreateNewItem(&pSubscriptItem, clsid);
    if (FAILED(hr) || !pSubscriptItem)
    {
        printf("prda->CreateNewItem failed.\r\n");
        return E_FAIL;
    }

    if (!prda || FAILED(prda->Init((CConApp *)this, pSubscriptItem, clsid)))
    {
        if (prda) 
            prda->Release();
        else
            printf("new CRunDeliveryAgent failed.\r\n");
        return E_FAIL;
    }

    // Preload mshtml
    if (m_bPreLoad)
    {
        if (FAILED(hr = CoGetClassObject(CLSID_IImgCtx, CLSCTX_SERVER, NULL, IID_IClassFactory, (void **)&pImageCF)))
        {
            printf("CoGetClassObject(CLSID_IImgCtx...) failed hr=%x\r\n", hr);
            return E_FAIL;
        }

        if (FAILED(hr = pImageCF->CreateInstance(NULL, IID_IImgCtx, (void **)&pImgCtx)))
        {
            printf("CreateInstance(IID_IImgCtx...) failed hr=%x\r\n", hr);
            return E_FAIL;
        }
    }

    // Set properties
    if (m_bChannelAgent)
    {
        WriteDWORD(pSubscriptItem, c_szPropChannel, m_dwChannel);
        WriteDWORD(pSubscriptItem, c_szPropChannelFlags, m_dwChannelFlags);
    }
    else
    {
        WriteDWORD(pSubscriptItem, c_szPropCrawlFlags, m_dwFlags);
        WriteDWORD(pSubscriptItem, c_szPropCrawlLevels, m_dwLevels);
    }

    // Set url property and start the download
    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    MyStrToOleStrN(wszURL, INTERNET_MAX_URL_LENGTH, m_pszURL);
    WriteOLESTR(pSubscriptItem, c_szPropURL, wszURL);
    
    m_dwTime = GetTickCount();  // Start time

    hr = prda->StartAgent();
    if (hr == E_PENDING)
    {
        hr = S_OK;
        
        if (Verbose()) 
            DBGOUT("CRunDeliveryAgent StartAgent succeeded");
        MessageLoop();
    }

    m_dwTime = GetTickCount() - m_dwTime;   // End time

    // Clean up
    if (pSubscriptItem)
    {
        pSubscriptItem->Release(); 
        pSubscriptItem = NULL;
    }

    if (pImgCtx)
    {
        pImgCtx->Release();
        pImgCtx = NULL;
    }

    if (pImageCF)
    {
        pImageCF->Release();
        pImageCF = NULL;
    }
        
    if (prda) 
    {
        prda->Release();
        prda = NULL;
    }
        
    if (FAILED(hr))
        return E_FAIL;

    return S_OK;
}

//---------------------------------------------------------------------
HRESULT CConApp::MessageLoop()
{
    MSG msg;
    BOOL dw;
    
    // Yield and wait for "UpdateEnd" notification
    while (m_iActive > 0 && (dw = ::GetMessage(&msg, NULL, 0, 0)))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    if(dw == 0)
        printf("GetMessage = 0, GLE=%d\r\n", GetLastError());

    return S_OK;
}

//---------------------------------------------------------------------
BOOL CConApp::PrintResults()
{
    printf("%s, %s, %ld, %ld, %ld %s\r\n", 
        m_pTestName ?m_pTestName :(m_bChannelAgent) ?"Webcheck ChannelAgent" :"Webcheck WebCrawlAgent",
        m_pRunStr ?m_pRunStr :"1",
        m_dwTime, 
        0,  // bytes read - future use, matches other tests
        0,  // kb/sec - future use
        m_CmdLine
        );
    return(TRUE);
}

//---------------------------------------------------------------------
BOOL CConApp::ParseCommandLine()
{
    BOOL bRC = TRUE;
    int argc = m_argc;
    char **argv = m_argv;
    DWORD dwLen = 0;

    *m_CmdLine = '\0';

    argv++; argc--;
    while( argc > 0 && argv[0][0] == '-' )  
    {
        switch (argv[0][1]) 
        {
            case 'c':
                m_bChannelAgent = TRUE;
                break;
            case 'f':
                m_dwFlags = atoi(&argv[0][2]);
                break;
            case 'l':
                m_dwLevels = atoi(&argv[0][2]);
                break;
            case 'p':
                m_bPreLoad = TRUE;
                break;
            case 'r':
                m_pRunStr = &argv[0][2];
                break;
            case 't':
                m_pTestName = &argv[0][2];
                break;
            case 'u':
                m_pszURL = &argv[0][2];
                break;
            case 'v':
                m_bVerbose = TRUE;
                break;
            default:
                bRC = FALSE;
                break;
        }
        
        if(bRC)
        {
            dwLen += lstrlen(argv[0]) + 1;   // length of arg and space
            if(dwLen < ((sizeof(m_CmdLine)/sizeof(m_CmdLine[0]))-1))
            {
                lstrcat(m_CmdLine, ",");
                lstrcat(m_CmdLine, argv[0]);
            }
        }
        
        argv++; argc--;
    }

    if(!m_pszURL || (bRC == FALSE))
    {
        Display_Usage();
        bRC = FALSE;
    }

    return(bRC);

}

//---------------------------------------------------------------------
void CConApp::Display_Usage()
{
    printf("Usage: %s -uURL [Options]\r\n\n", m_argv[0]);
    printf("Options:\r\n");
    printf("\t-c    Run ChannelAgent instead of WebCrawl.\r\n");
    printf("\t-f#   Webcrawl agent flags.\r\n");
    printf("\t-l#   Delivery agent levels to crawl.\r\n");
    printf("\t-p    Preload Mshtml.\r\n");
    printf("\t-v    Turn on verbose output.\r\n");
    printf("\t-tStr test name string (used in results output)\n");
    printf("\t-rStr run# string (used in results output)\n");
}

//---------------------------------------------------------------------
inline BOOL CConApp::Verbose()
{
    return(m_bVerbose);
}

//---------------------------------------------------------------------
HRESULT CConApp::OnAgentBegin()
{
    m_iActive++;
    return S_OK;
}

//---------------------------------------------------------------------
// OnAgentProgress not currently called
HRESULT CConApp::OnAgentProgress()
{ 
    return E_NOTIMPL; 
}

//---------------------------------------------------------------------
// OnAgentEnd called when agent is complete. fSynchronous means that StartAgent call
//  has not yet returned; hrResult will be returned from StartAgent
HRESULT CConApp::OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
                           long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                           BOOL fSynchronous)
{
    m_iActive--;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int __cdecl main(int argc, char **argv)
{
    HRESULT hr = S_OK;
    
    CConApp App(argc, argv);
    hr = App.Init();
    if(!FAILED(hr))
    {
        hr = App.Download();
        App.PrintResults();
    }

    return((int)hr);
}

