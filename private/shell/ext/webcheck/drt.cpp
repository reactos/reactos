#include "private.h"
#include "resource.h"
#include <urlmon.h>

#define TF_THISMODULE TF_WEBCHECKCORE

// BUGBUG: These constants (and this file) should be removed from
// webcheck before the final product is shippped.
const TCHAR c_szDefaultDRTURL[] = TEXT("http://adsrv/DRT/");
const TCHAR c_szDefaultLoggedURL[] = TEXT("http://adsrv/drt/cdfpages/page1.htm");

// We cannot define DEFINE_FLOAT_STUFF because there is floating-point
// arithmetic performed in this component.

STDAPI OfflineFolderRegisterServer();
STDAPI OfflineFolderUnregisterServer();

// Prototype of the dialog function
LRESULT CALLBACK DRT_Dialog(HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////////////////////////////////////////
//
// DRT functions
//
//////////////////////////////////////////////////////////////////////////
class CWCRunSynchronous : public INotificationSink
{
    ULONG   m_cRef;
    int     m_iActive;
    HWND    m_hwndDRT;
    INotificationMgr *m_pManager;
    
public:
    CWCRunSynchronous();
    ~CWCRunSynchronous();

    HRESULT Init();    
    HRESULT SetHwnd(HWND hwnd)  { m_hwndDRT = hwnd; return S_OK; }
    HRESULT UpdateHwnd(INotification  *pNotification);

    INotification *CreateNotification();
    HRESULT RunObject(INotification *pNot, REFCLSID rclsidDest);

    HRESULT OnBeginReport(INotification *pNotification, INotificationReport *pNotificationReport);
    HRESULT OnEndReport(INotification  *pNotification);
    HRESULT OnProgressReport(INotification  *pNotification);
    
    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INotificationSink members
    STDMETHODIMP         OnNotification(
                INotification          *pNotification,
                INotificationReport    *pNotificationReport,
                DWORD                   dwReserved);
};

CWCRunSynchronous::CWCRunSynchronous()
{
    m_cRef = 1;
    m_iActive=0;
    m_pManager = NULL;
}
    
CWCRunSynchronous::~CWCRunSynchronous()
{
    SAFERELEASE(m_pManager);
}

STDMETHODIMP
CWCRunSynchronous::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
    {
        *ppv=(INotificationSink *)this;
    }

    // Addref through the interface
    if( NULL != *ppv ) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CWCRunSynchronous::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CWCRunSynchronous::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

HRESULT CWCRunSynchronous::Init()
{
    HRESULT hr=S_OK;

    // Get reference to the notification manager
    if (!m_pManager)
        hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER,
                                IID_INotificationMgr, (void**)&m_pManager);
    if (FAILED(hr) || !m_pManager)
    {
        return E_FAIL;
    }

    return S_OK;
}

INotification *CWCRunSynchronous::CreateNotification()
{
    INotification *pNot=NULL;

    if (m_pManager)
    {
        m_pManager->CreateNotification(
            NOTIFICATIONTYPE_AGENT_START,
            (NOTIFICATIONFLAGS) 0,
            NULL,
            &pNot,
            0);
    }

    return pNot;
}

HRESULT CWCRunSynchronous::RunObject(INotification *pNot, REFCLSID rclsidDest)
{
    HRESULT hr = E_FAIL;

    if (m_pManager)
    {
        m_iActive ++;

        hr = m_pManager->DeliverNotification(pNot, rclsidDest, 
            DM_NEED_COMPLETIONREPORT,
            (INotificationSink *)this, NULL, 0);
    }

    if (SUCCEEDED(hr))
    {
        MSG msg;
        DBG("CWCRunSynchronous RunItemNow succeeded");
        // Yield and wait for "UpdateEnd" notification
        while (m_iActive > 0 && GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DBG("RunSynchronous RunObject returning");
    return hr;
}

HRESULT CWCRunSynchronous::UpdateHwnd(INotification  *pNotification)
{
    if (m_hwndDRT)
    {
        BSTR bstrURL = NULL;
        if (SUCCEEDED(ReadBSTR(pNotification, NULL, c_szPropCurrentURL, &bstrURL)))
        {
            ASSERT(bstrURL);
            TCHAR szURL[MAX_PATH];
            MyOleStrToStrN(szURL, ARRAYSIZE(szURL), bstrURL);
            PathSetDlgItemPath(m_hwndDRT, IDC_DRTSTATUS, szURL);
            SAFEFREEBSTR(bstrURL);
        }
    }
    
    return S_OK;    
}
                
// 
// INotificationSink members
//
STDMETHODIMP CWCRunSynchronous::OnNotification(
                INotification          *pNotification,
                INotificationReport    *pNotificationReport,
                DWORD                   dwReserved)
{
    NOTIFICATIONTYPE    nt;
    HRESULT             hr=S_OK;

    DBG("CWCRunSynchronous receiving OnNotification");

    hr = pNotification->GetNotificationInfo(&nt, NULL,NULL,NULL,0);

    if (FAILED(hr))
    {
        DBG("Failed to get notification type!");
        return E_INVALIDARG;
    }

    if (IsEqualGUID(nt, NOTIFICATIONTYPE_BEGIN_REPORT))
        hr = OnBeginReport(pNotification, pNotificationReport);
    else if (IsEqualGUID(nt, NOTIFICATIONTYPE_END_REPORT))
        hr = OnEndReport(pNotification);
    else if (IsEqualGUID(nt, NOTIFICATIONTYPE_PROGRESS_REPORT))
        hr = OnProgressReport(pNotification);
    else DBG("CWCRunSynchronous: Unknown notification type received");        

    // Avoid bogus assert
    if (SUCCEEDED(hr)) hr = S_OK;
    
    return hr;
}

HRESULT CWCRunSynchronous::OnBeginReport(INotification *pNotification, INotificationReport *pNotificationReport)
{
    DBG("CWCRunSynchronous BeginReport received");

    UpdateHwnd(pNotification);

    return S_OK;
}

HRESULT CWCRunSynchronous::OnEndReport(INotification  *pNotification)
{
    DBG("CWCRunSynchronous EndReport received");

    UpdateHwnd(pNotification);
    
    m_iActive --;

    return S_OK;
}

HRESULT CWCRunSynchronous::OnProgressReport(INotification  *pNotification)
{
    DBG("CWCRunSynchronous ProgressReport received");

    UpdateHwnd(pNotification);
    
    return S_OK;
}
#define WF_STATUS_UNINITIALIZED 0
//  We got URL.
#define WF_STATUS_REMOTE        1
//  We got URL & Local File Path
#define WF_STATUS_LOCAL         2
//  Somebody opens this file
#define WF_STATUS_BUSY          3

#define WF_CONTENT_NOTHING      0
#define WF_CONTENT_URL          1
#define WF_CONTENT_PATH         2
#define WF_CONTENT_HANDLE       4
#define WF_CONTENT_TRANSIENT    0x80000000

#define WF_CMD_DELETE       0
#define WF_CMD_CHECK_YES    1
#define WF_CMD_CHECK_NO     2

#define ON_FAILURE_RETURN(HR)   {if(FAILED(HR)) return (HR);}

class CWebFile
{
    protected:
        TCHAR   m_URL[INTERNET_MAX_URL_LENGTH];
        TCHAR   m_Path[MAX_PATH];
        HANDLE  m_Handle;
        UINT    m_uStatus;
        UINT    m_uContentFlags;

        HRESULT WebCrawl(CWCRunSynchronous *);

    public:
        ~CWebFile();
        CWebFile();
        CWebFile(LPCTSTR szURL);

        HRESULT Open(HANDLE &);
        HRESULT Close(void);
        HRESULT Remove(void);
        HRESULT Update(CWCRunSynchronous *);
        HRESULT GetPath(LPTSTR szPath, UINT bufSize)  {
                if (!(m_uContentFlags & WF_CONTENT_PATH))
                    return E_FAIL;

                lstrcpyn(szPath, m_Path, bufSize);
                return S_OK;
            }
        HRESULT GetURL(LPTSTR szURL, UINT bufSize)    {
                if (!(m_uContentFlags & WF_CONTENT_URL))
                    return E_FAIL;

                lstrcpyn(szURL, m_URL, bufSize);
                return S_OK;
            }
        HRESULT SetURL(LPCTSTR szURL)  {
                if (m_uStatus == WF_STATUS_BUSY)
                    return E_FAIL;

                lstrcpyn(m_URL, szURL, INTERNET_MAX_URL_LENGTH);
                m_uContentFlags = WF_CONTENT_URL;
                m_uStatus = WF_STATUS_REMOTE;
                return S_OK;
            }
};

CWebFile::CWebFile()
{
    m_Handle = NULL;
    m_URL[0] = m_Path[0] = (TCHAR)0;
    m_uStatus = WF_STATUS_UNINITIALIZED;
    m_uContentFlags = WF_CONTENT_NOTHING;
}

CWebFile::CWebFile(LPCTSTR szURL)
{
    //  BUGBUG. We don't check if it's a valid URL.
    if (!szURL)   {
        CWebFile();
        return;
    }
    
    m_Handle = NULL;
    m_Path[0] = (TCHAR)0;
    SetURL(szURL);
}

CWebFile::~CWebFile()
{
    if (m_uStatus == WF_STATUS_BUSY)
        Close();

    if (m_uStatus == WF_STATUS_LOCAL)   {
        if (m_uContentFlags & WF_CONTENT_TRANSIENT) {
            Remove();
        }
    }
    m_URL[0] = m_Path[0] = (TCHAR)0;
    m_uStatus = WF_STATUS_UNINITIALIZED;
    m_uContentFlags = WF_CONTENT_NOTHING;
}

HRESULT CWebFile::Open(HANDLE & hFile)
{
    if (m_uStatus != WF_STATUS_LOCAL)
        return E_FAIL;
    //  We won't allow multiple open.
    //  BUGBUG should implement ref count. Now only allow access once a time.

    ASSERT(m_uContentFlags & WF_CONTENT_PATH);
    HANDLE  hTmp;

    m_Handle = NULL;
    hTmp = CreateFile(
                m_Path,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if (hTmp == INVALID_HANDLE_VALUE) 
        return E_FAIL;

    hFile = hTmp;
    m_Handle = hTmp;
    m_uContentFlags |= WF_CONTENT_HANDLE;
    m_uStatus = WF_STATUS_BUSY;
    return S_OK;
}

HRESULT CWebFile::Close()
{
    if (WF_STATUS_BUSY == m_uStatus)
        m_uStatus = WF_STATUS_LOCAL;
    else
        return E_FAIL;

    if (m_uContentFlags & WF_CONTENT_HANDLE)    {
        BOOL fRet = CloseHandle(m_Handle);
        ASSERT(fRet);
        m_Handle = NULL;
        m_uContentFlags &= (~WF_CONTENT_HANDLE);
    }
    
    return S_OK;
}

HRESULT CWebFile::WebCrawl(CWCRunSynchronous *pCourier)
{
    if (m_uStatus != WF_STATUS_REMOTE)
        return E_INVALIDARG;

    INotification       * pNot = NULL;
    INotificationMgr    * pMgr = NULL;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ON_FAILURE_RETURN(hr);

    hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER,
                IID_INotificationMgr, (void **)&pMgr);

    CoUninitialize();

    ON_FAILURE_RETURN(hr);
    ASSERT(pMgr);

    hr = pMgr->CreateNotification(
                NOTIFICATIONTYPE_AGENT_START,
                (NOTIFICATIONFLAGS) 0,
                NULL, &pNot, 0);

    if (FAILED(hr)) {
        SAFERELEASE(pMgr);
    }
    ASSERT(pNot);

    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    MyStrToOleStrN(wszURL, INTERNET_MAX_URL_LENGTH, m_URL);
    WriteOLESTR(pNot, NULL, c_szPropURL, wszURL);
    WriteDWORD(pNot, NULL, c_szPropCrawlFlags, WEBCRAWL_DONT_MAKE_STICKY);
    WriteDWORD(pNot, NULL, c_szPropCrawlLevels, 0);

    hr = pCourier->RunObject(pNot, CLSID_WebCrawlerAgent);
    SAFERELEASE(pMgr);
    SAFERELEASE(pNot);
    return hr;
}

HRESULT CWebFile::Update(CWCRunSynchronous *pCourier)
{
    BYTE    bBuf[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO pEntry = (LPINTERNET_CACHE_ENTRY_INFO)bBuf;  
    DWORD   dwSize = MAX_CACHE_ENTRY_INFO_SIZE;

    if (m_uStatus == WF_STATUS_LOCAL)   {
        m_uStatus = WF_STATUS_REMOTE;   //  Reload.
        m_uContentFlags &= (~WF_CONTENT_PATH);
    }

    if (m_uStatus != WF_STATUS_REMOTE)
        return E_FAIL;

    ASSERT(pCourier);
    HRESULT hr = WebCrawl(pCourier);
    ON_FAILURE_RETURN(hr); 
    
    if (GetUrlCacheEntryInfo(m_URL, pEntry, &dwSize))   {
        lstrcpyn(m_Path, pEntry->lpszLocalFileName, MAX_PATH);
        m_uStatus = WF_STATUS_LOCAL;
        m_uContentFlags |= WF_CONTENT_PATH;
        return S_OK;
    } else  {
        return E_FAIL;
    }
}

HRESULT CWebFile::Remove()
{
    return E_NOTIMPL;
}

const TCHAR c_szStrURL[] = TEXT("URL");
const TCHAR c_szStrLoggedURL[] = TEXT("LoggedURL");
const TCHAR c_szStrFiles[] = TEXT("Files");
const TCHAR c_szStrNoFiles[] = TEXT("NoFiles");
const TCHAR c_szIntLevels[] = TEXT("Levels");
const TCHAR c_szIntFlags[] = TEXT("Flags");
const TCHAR c_szCDF[] = TEXT("CDF");
const TCHAR c_szCaption[] = TEXT("WebCheck DRT Error");

//BUGBUG-FIXED-OVERFLOW
const TCHAR c_szFailedToGet[] = TEXT("Failed to get\n\t");
const TCHAR c_szIncorrectlyFetched[] = TEXT("Incorrectly fetched\n\t");
const TCHAR c_szFailedToDelete[] = TEXT("Failed to delete\n\t");
void CheckCacheLine(TCHAR *strURL, UINT cmd)
{
    BYTE    bBuf[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO pEntry = (LPINTERNET_CACHE_ENTRY_INFO)bBuf;
    TCHAR szError[256 + INTERNET_MAX_URL_LENGTH];
    DWORD dwSize = sizeof(bBuf);
    LPCTSTR pszErrorText = NULL;
    DWORD dwErrorTextLen = 0; //  Not including NULL terminator
    
    switch (cmd) {
        case WF_CMD_CHECK_YES:
            if (!GetUrlCacheEntryInfo(strURL, pEntry, &dwSize))
            {
                pszErrorText = c_szFailedToGet;
                dwErrorTextLen = ARRAYSIZE(c_szFailedToGet) - sizeof(WCHAR);
            }
            break;
            
        case WF_CMD_CHECK_NO:
            if (GetUrlCacheEntryInfo(strURL, pEntry, &dwSize))
            {
                pszErrorText = c_szIncorrectlyFetched;
                dwErrorTextLen = ARRAYSIZE(c_szIncorrectlyFetched) - sizeof(WCHAR);
            }
            break;
            
        case WF_CMD_DELETE:
            DeleteUrlCacheEntry(strURL);
            if (GetUrlCacheEntryInfo(strURL, pEntry, &dwSize))
            {
                pszErrorText = c_szFailedToDelete;
                dwErrorTextLen = ARRAYSIZE(c_szFailedToDelete) - sizeof(WCHAR);
            }
            break;
            
        default:
            ASSERT(0);
            break;
    }

    if (NULL != pszErrorText)
    {
        ASSERT(0 != dwErrorTextLen);
        lstrcpy(szError, pszErrorText);
        lstrcatn(szError, strURL, ARRAYSIZE(szError));
        MessageBox(NULL, szError, c_szCaption, 0);
    }
}


HRESULT CheckCache(LPCTSTR szURL, CWCRunSynchronous * prs, UINT cmd)
{
    ASSERT(szURL);

    if (szURL[0] == (TCHAR)0)
        return S_FALSE;

    CWebFile wFile(szURL);
    HRESULT hr;

    hr = wFile.Update(prs);
    ON_FAILURE_RETURN(hr);

    HANDLE hFile = NULL;
    hr = wFile.Open(hFile);
    ON_FAILURE_RETURN(hr);

    // Get the file size while we still have the handle open
    UINT cchMax = GetFileSize(hFile, NULL);
    if (0 == cchMax || 0xFFFFFFFF == cchMax)
    {
        wFile.Close();
        return E_FAIL;
    }

    HANDLE hFileMapping = NULL;
    hFileMapping = CreateFileMapping(
            hFile,
            NULL,
            PAGE_READONLY,
            0,
            0,      //  Same size as file.
            NULL    //  No name.
            );
    wFile.Close();

    if (!hFileMapping)
        return E_FAIL;

    char *strBuf = (char *) MapViewOfFile(hFileMapping,
                    FILE_MAP_READ,
                    0, 0, 0);   //  From offset 0, map whole.
    //  Should have internel handle in mapping object.
    BOOL fRet = CloseHandle(hFileMapping);
    ASSERT(fRet);
    if (!strBuf)
        return E_FAIL;

    TCHAR strURL[INTERNET_MAX_URL_LENGTH];
    UINT  i = 0;
    UINT  cch = 0;
    while (cch < cchMax)
    {
        switch (strBuf[cch])
        {
            case '\r':
            case '\n':
                while (cch < cchMax && (strBuf[cch] == '\r' || strBuf[cch] == '\n'))
                    cch++;
                if (i > 0)
                {
                    strURL[i] = 0;
                    CheckCacheLine(strURL, cmd);
                    i = 0;
                }
                break;
                
            case ';':
                if (0 == i)
                {
                    // skip the line if the first char is a semi-colon
                    while (cch < cchMax && strBuf[cch] != '\r' && strBuf[cch] != '\n')
                        cch++;
                    while (cch < cchMax && (strBuf[cch] == '\r' || strBuf[cch] == '\n'))
                        cch++;
                    break;
                }
                // else fall through

            default:
                strURL[i] = (TCHAR)strBuf[cch];
                i++;
                cch++;
                ASSERT(INTERNET_MAX_URL_LENGTH > i);
                break;
        }
    }
    ASSERT(cch == cchMax);
    if (i > 0)    // check for abnormal terminations
    {
        strURL[i] = 0;
        CheckCacheLine(strURL, cmd);
    }

    fRet = UnmapViewOfFile(strBuf);
    ASSERT(fRet);
    return S_OK;
}

HRESULT APIENTRY f3drt(VARIANT *pParam, long *plRetVal)
{
    TCHAR szSection[128];
    
    // Set a default DRT INI file and section in case 
    // the caller didn't specify one.
    CWebFile fIni(TEXT("http://adsrv/drt/drt.ini"));
    lstrcpyn(szSection, TEXT("drt1"), ARRAYSIZE(szSection));

    // Override the default
    if (pParam)
    {
        if (pParam->vt == VT_BSTR)  {
            TCHAR iniURL[INTERNET_MAX_URL_LENGTH];

            BSTR bstr = pParam->bstrVal;
            iniURL[0] = (TCHAR)0;
            MyOleStrToStrN(iniURL, ARRAYSIZE(iniURL),bstr);
            if (PathIsURL(iniURL))  {
                LPTSTR pBreak = NULL;

                pBreak = StrChr(iniURL, (WORD)'?');
                if (pBreak != NULL) {
                    * pBreak = (TCHAR)0;
                    pBreak ++;
                    lstrcpyn(szSection, pBreak, 
                            ARRAYSIZE(szSection));
                }

                fIni.SetURL(iniURL);
            }
        }
    }
        
    *plRetVal = -2; // assume error

    HRESULT hr=S_OK;
    CWCRunSynchronous *prs = new CWCRunSynchronous;
    if (!prs || FAILED(prs->Init()))
    {
        if (prs) prs->Release();
        return S_FALSE;
    }

    // Download the INI file
    hr = fIni.Update(prs);
    ON_FAILURE_RETURN(hr);

    TCHAR   targetURL[INTERNET_MAX_URL_LENGTH];
    TCHAR   loggedURL[INTERNET_MAX_URL_LENGTH];
    TCHAR   filesURL[INTERNET_MAX_URL_LENGTH];
    TCHAR   nofilesURL[INTERNET_MAX_URL_LENGTH];

    TCHAR   szPath[MAX_PATH];
    DWORD   dwSize, dwFlags, dwLevels, dwCDF;

    hr = fIni.GetPath(szPath, MAX_PATH);
    ASSERT(SUCCEEDED(hr));
    dwSize = GetPrivateProfileString(
                    szSection,
                    c_szStrURL,
                    c_szDefaultDRTURL,
                    targetURL,
                    INTERNET_MAX_URL_LENGTH,
                    szPath);
                    
    dwSize = GetPrivateProfileString(
                    szSection,
                    c_szStrLoggedURL,
                    c_szDefaultLoggedURL,
                    loggedURL,
                    INTERNET_MAX_URL_LENGTH,
                    szPath);

    dwSize = GetPrivateProfileString(
                    szSection,
                    c_szStrFiles,
                    c_szStrEmpty,
                    filesURL,
                    INTERNET_MAX_URL_LENGTH,
                    szPath);
    CheckCache(filesURL, prs, WF_CMD_DELETE);

    dwSize = GetPrivateProfileString(
                    szSection,
                    c_szStrNoFiles,
                    c_szStrEmpty,
                    nofilesURL,
                    INTERNET_MAX_URL_LENGTH,
                    szPath);
    CheckCache(nofilesURL, prs, WF_CMD_DELETE);

    dwLevels = GetPrivateProfileInt(
                    szSection,
                    c_szIntLevels,
                    1,      //  Default 1 level deep.
                    szPath);

    dwCDF = GetPrivateProfileInt(
                    szSection,
                    c_szCDF,
                    0,
                    szPath);

    dwFlags = GetPrivateProfileInt(
                    szSection,
                    c_szIntFlags,
                    (dwCDF)?(CHANNEL_AGENT_PRECACHE_ALL):(WEBCRAWL_GET_IMAGES | WEBCRAWL_DONT_MAKE_STICKY),
                    szPath);

    INotification *pNot = NULL;
    HWND hWait;    

    // create the AgentStart notification
    pNot = prs->CreateNotification();

    if (!pNot)
        return S_FALSE;

    WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
    MyStrToOleStrN(wszURL, INTERNET_MAX_URL_LENGTH, targetURL);

    WriteOLESTR(pNot, NULL, c_szPropURL, wszURL);
    WriteDWORD(pNot, NULL, c_szPropAgentFlags, DELIVERY_AGENT_FLAG_NO_BROADCAST);

    // Set different properties for webcrawls and channels
    if (dwCDF)
    {
        WriteDWORD(pNot, NULL, c_szPropChannelFlags, dwFlags);
        WriteDWORD(pNot, NULL, c_szPropChannel, 1);
    }
    else
    {
        WriteDWORD(pNot, NULL, c_szPropCrawlFlags, dwFlags);
        WriteDWORD(pNot, NULL, c_szPropCrawlLevels, dwLevels);
    }
 
    // Display the modeless dialog
    hWait = CreateDialog(MLGetHinst(), MAKEINTRESOURCE(IDD_DRT), 
                GetDesktopWindow(), (DLGPROC)DRT_Dialog);            
    ShowWindow(hWait, SW_SHOW);
    SetWindowPos(hWait, HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hWait);
    
    prs->SetHwnd(hWait);

    if (dwCDF)
    {
        // Write something to the log file so logging takes affect
        HIT_LOGGING_INFO hli;
        hli.dwStructSize = sizeof(hli);
        hli.lpszLoggedUrlName = loggedURL;
        GetLocalTime(&(hli.StartTime));
        GetLocalTime(&(hli.EndTime));
        BOOL bRet = WriteHitLogging(&hli);
        ASSERT(bRet);
        
        hr = prs->RunObject(pNot, CLSID_ChannelAgent);
    }
    else
    {
        hr = prs->RunObject(pNot, CLSID_WebCrawlerAgent);
    }

    CheckCache(filesURL, prs, WF_CMD_CHECK_YES);
    CheckCache(nofilesURL, prs, WF_CMD_CHECK_NO);
    prs->SetHwnd(NULL);
    
    pNot->Release(); pNot=NULL;

    // Get rid of the modeless dialog
    if (hWait != NULL)
        DestroyWindow(hWait);

    if (FAILED(hr))
        return S_FALSE;

    *plRetVal = 0;  // no errors

    return S_OK;
}        

//===========================================================================
//
//  FUNCTION: DRT_Dialog(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "DRT Test Running" dialog box.
//            This dialog is displayed when a webcheck DRT test is running.
//
//  MESSAGES:
//
//  WM_INITDIALOG - initialize dialog box
//
//===========================================================================

LRESULT CALLBACK DRT_Dialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static RECT rect;

    switch (message) 
    {
        case WM_INITDIALOG:
            // Center the dialog
            GetWindowRect(GetDesktopWindow(), &rect);
            SetWindowPos(hDlg, NULL, rect.left + (rect.right / 4), 
            rect.top + (rect.bottom / 4), 0, 0, SWP_NOSIZE | SWP_NOZORDER);    
            
            return (TRUE);
            break;

        //case WM_COMMAND:
          //  if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            //{
              //  EndDialog(hDlg, TRUE);
                //return (TRUE);
            //}
            //break;
    }

    return FALSE;
}
