#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <urlmon.h>
#include <string.h>
#include <malloc.h>
#include <urlmon.hxx>

#define IE5

#ifdef PRODUCT_PROF
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
extern "C" void _stdcall StartCAPAll(void);
extern "C" void _stdcall StopCAPAll(void);
#else
#define StartCAP()
#define StopCAP()
#define SuspendCAP()
#define ResumeCAP()
#define StartCAPAll()
#define StopCAPAll()
#endif

#define FLAG_TRACE    1
#define FLAG_DUMPDATA 2
#define MAX_DOWNLOADS 2000
#define MAX_URL INTERNET_MAX_URL_LENGTH
const INT BUF_SIZE = 2 * 1024;
const INT MAX_BUF_SIZE = 1024 * 16;

BOOL g_dwVerbose = 0;
DWORD g_dwNumDownloads = 1;
DWORD g_dwDownloads = 1;
DWORD g_dwTotalBytes = 0;
DWORD g_dwCacheFlag = BINDF_NOWRITECACHE | BINDF_GETNEWESTVERSION;
DWORD dwBuf_Size = BUF_SIZE;
LPWSTR g_pwzUrl = NULL;
LPCSTR g_pInfile = NULL;
LPCSTR g_pTitle = NULL;
LPCSTR g_pRun = NULL;
LPCSTR g_pModule = NULL;
HANDLE g_hCompleted;

__int64 g_ibeg = 0, g_iend, g_ifreq;

//------------------------------------------------------------------------
//      Class:      COInetProtocolHook
//
//      Purpose:    Sample Implementation of ProtocolSink and BindInfo
//                  interface for simplified urlmon async download 
//
//      Interfaces: 
//          [Needed For All]
//                  IOInetProtocolSink
//                      - provide sink for pluggable prot's callback  
//                  IOInetBindInfo    
//                      - provide the bind options
//
//          [Needed For Http] 
//                  IServiceProvider
//                      - used to query http specific services
//                        e.g. HttpNegotiate, Authentication, UIWindow
//                  IHttpNegotiate    
//                      - http negotiation service, it has two methods, 
//                        one is the http pluggable protocol asks the 
//                        client for additional headers, the other is
//                        callback for returned http server status
//                        e.g 200, 401, etc. 
//                                                
//
//      Author:     DanpoZ (Danpo Zhang)
//
//      History:    11-20-97    Created
//                  05-19-98    Modified to act as performance test
//
//      NOTE:       IOInetXXXX == IInternetXXXX
//                  on the SDK, you will see IInternetXXXX, these are same
//                  interfaces
//
//------------------------------------------------------------------------
class COInetProtocolHook :  public IOInetProtocolSink, 
                            public IOInetBindInfo,
                            public IHttpNegotiate,
                            public IServiceProvider
{
public:
    COInetProtocolHook(HANDLE g_hCompleted, IOInetProtocol* pProt);

    virtual ~COInetProtocolHook();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IOInetProtocolSink methods
    STDMETHODIMP Switch( PROTOCOLDATA *pStateInfo);
    STDMETHODIMP ReportProgress( ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHODIMP ReportData( 
        DWORD grfBSCF, 
        ULONG ulProgress, 
        ULONG ulProgressMax
    );
    STDMETHODIMP ReportResult(
        HRESULT hrResult,
        DWORD   dwError,
        LPCWSTR wzResult
    );

    //IOInetBindInfo methods
    STDMETHODIMP GetBindInfo(
        DWORD *grfBINDF,
        BINDINFO * pbindinfo
    );
    STDMETHODIMP GetBindString(
        ULONG ulStringType,
        LPOLESTR *ppwzStr,
        ULONG cEl,
        ULONG *pcElFetched
    );

    //IService Provider methods
    STDMETHODIMP QueryService(
        REFGUID guidService,
        REFIID  riid,
        void    **ppvObj 
    );

    //IHttpNegotiate methods
    STDMETHODIMP BeginningTransaction(
        LPCWSTR szURL,
        LPCWSTR szHeaders,
        DWORD   dwReserved,
        LPWSTR  *pszAdditionalHeaders
    );

    STDMETHODIMP OnResponse(
        DWORD    dwResponseCode,
        LPCWSTR  szResponseHeaders,
        LPCWSTR  szRequestHeaders,
        LPWSTR   *pszAdditionalHeaders
    );
private:
    IOInetProtocol* _pProt; 
    HANDLE          _hCompleted;
    CRefCount       _CRefs;          
};


typedef struct tagInfo
{
    WCHAR wzUrl[INTERNET_MAX_URL_LENGTH];
    IOInetProtocol* pProt;
    COInetProtocolHook* pHook;
    IOInetProtocolSink* pSink;
    IOInetBindInfo*     pBindInfo;
} INFO, *PINFO;

typedef BOOL (WINAPI *PFNSPA)(HANDLE, DWORD);

INFO Info[MAX_DOWNLOADS];

//------------------------------------------------------------------------
//------------------------------------------------------------------------

void MylstrcpyW(WCHAR *pwd, WCHAR *pws)
{
    while (*pws)
    {
        *pwd++ = *pws++;
    }
    *pwd = 0;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
WCHAR *MyDupA2W(LPSTR pa)
{
    int i;
    WCHAR *pw, *pwd;

    i = lstrlen(pa);
    pw = (WCHAR *)CoTaskMemAlloc((i+1) * sizeof(WCHAR));
    pwd = pw;
    while (*pa)
    {
        *pwd++ = (WCHAR)*pa++;
    }
    *pwd++ = 0;

    return pw;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
void SetSingleProcessorAffinity()
{
    PFNSPA pfn;

    pfn = (PFNSPA)GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),
            "SetProcessAffinityMask");

    if (pfn)
    {
        pfn(GetCurrentProcess(), 1);
    }
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
VOID DisplayUsage(char **argv)
{
    printf("Usage: %s /u:url [/n:# /t:Title /r:RunStr /v:#]\n", argv[0]);
    printf("       %s /i:infile [/t:Title /r:RunStr /v:#]\n", argv[0]);
    printf("   /c   - write to cache (default is NOWRITECACHE)\n");
    printf("   /g   - read from cache (default is GETNEWESTVERSION)\n");
    printf("   /d   - direct read (default uses QueryDataAvailable)\n");
    printf("   /l:# - read buffer length\n");
    printf("   /m:module - pre load module\n");
    printf("   /n:# - download # times.\n");
    printf("   /1   - single processor affinity (default multiprocessor)\n");
    printf("   /v:# - verbose level 1=trace 2=dump data\n");
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
BOOL ProcessCommandLine(int argcIn, char **argvIn)
{
    BOOL bRC = FALSE;
    int argc = argcIn;
    char **argv = argvIn;

    argv++; argc--;
    if(argc == 0)
    {
        DisplayUsage(argvIn);
        return(FALSE);
    }
    
    while( argc > 0 && argv[0][0] == '/' )  
    {
        switch (argv[0][1]) 
        {
            case 'c':
            case 'C':
                g_dwCacheFlag &= ~BINDF_NOWRITECACHE;
                break;
            case 'g':
            case 'G':
                g_dwCacheFlag &= ~BINDF_GETNEWESTVERSION;
                break;
            case 'd':
            case 'D':
                g_dwCacheFlag |= BINDF_DIRECT_READ;
                break;
            case 'i':
            case 'I':
                g_pInfile = &argv[0][3];
                bRC = TRUE;
                break;
            case 'l':
            case 'L':
                dwBuf_Size =  atoi(&argv[0][3]);
                if(dwBuf_Size > MAX_BUF_SIZE)
                    dwBuf_Size = MAX_BUF_SIZE;
                break;
            case 'm':
            case 'M':
                g_pModule = &argv[0][3];
                break;
            case 'n':
            case 'N':
                g_dwNumDownloads = (DWORD)atoi(&argv[0][3]);
				g_dwNumDownloads = max(1, g_dwNumDownloads);
				g_dwNumDownloads = min(MAX_DOWNLOADS, g_dwNumDownloads);
                break;
            case 'r':
            case 'R':
                g_pRun = &argv[0][3];
                break;
            case 't':
            case 'T':
                g_pTitle = &argv[0][3];
                break;
            case 'u':
            case 'U':
                g_pwzUrl = MyDupA2W(&argv[0][3]);
                bRC = TRUE;
                break;
            case 'v':
            case 'V':
                g_dwVerbose = (DWORD)atoi(&argv[0][3]);
                break;
            case '1':
                SetSingleProcessorAffinity();
                break;
            case '?':
            default:
                DisplayUsage(argvIn);
                bRC = FALSE;
        }
        argv++; argc--;
    }

    return(bRC);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
BOOL BuildInfoList(PINFO pInfo, DWORD dwNumDownloads)
{
    DWORD i = 0;
    
    if(g_pInfile != NULL)
    {
        TCHAR szName[INTERNET_MAX_URL_LENGTH+1];
        FILE *fp;

        if((fp = fopen(g_pInfile, "r")) == NULL)
        {
            printf("error opening file:%s GLE=%d\n", g_pInfile, GetLastError());
            return NULL;
        }

        while(fgets(szName, INTERNET_MAX_URL_LENGTH, fp) != NULL)
        {
            if(szName[0] != '#')
            {
                szName[strlen(szName) - sizeof(char)] = '\0';

                int rc;
                rc = MultiByteToWideChar(CP_ACP, 0, szName, -1, (pInfo+i)->wzUrl, INTERNET_MAX_URL_LENGTH);
                if (!rc)
                {
                    (pInfo+i)->wzUrl[INTERNET_MAX_URL_LENGTH-1] = 0;
                    wprintf(L"BuildInfoList:string too long; truncated to %s\n", (pInfo+i)->wzUrl);
                }
                i++;
            }
        }
        
        g_dwNumDownloads = i;
        fclose(fp);
    }
    else
    {
        for(i =0; i < dwNumDownloads; i++)
        {
            MylstrcpyW((pInfo+i)->wzUrl, g_pwzUrl);
        }
    }

    g_dwDownloads = g_dwNumDownloads;

    return(TRUE);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
HRESULT StartDownloads(IOInetSession* pSession, PINFO pInfo, DWORD dwNumDownloads)
{
    HRESULT hr = NOERROR;
    
    for(DWORD i =0; i < dwNumDownloads; i++)
    {
        // Create a pluggable protocol
            hr = pSession->CreateBinding(
                NULL,           // [in ] BindCtx, always NULL 
                (pInfo+i)->wzUrl,         // [in ] url 
                NULL,           // [in ] IUnknown for Aggregration
                NULL,           // [out] IUNknown for Aggregration
                &(pInfo+i)->pProt,         // [out] return pProt pointer 
                0               // [in ] bind option, pass 0
            );
            if(g_dwVerbose & FLAG_TRACE)
                printf("MAIN:    Session->CreateBinding: %lx\n", hr);

        // Create a protocolHook (sink) and Start the async operation
        if( hr == NOERROR )
        {
            (pInfo+i)->pHook = new COInetProtocolHook(g_hCompleted, (pInfo+i)->pProt);
            (pInfo+i)->pSink = NULL;
            (pInfo+i)->pBindInfo = NULL;
            
            if( (pInfo+i)->pHook )
            {
                hr = (pInfo+i)->pHook->QueryInterface(IID_IOInetProtocolSink, (void**)&(pInfo+i)->pSink);
                hr = (pInfo+i)->pHook->QueryInterface(IID_IOInetBindInfo, (void**)&(pInfo+i)->pBindInfo);
            }

            if( (pInfo+i)->pProt && (pInfo+i)->pSink && (pInfo+i)->pBindInfo )
            {
                hr = (pInfo+i)->pProt->Start(
                    (pInfo+i)->wzUrl,                        
                    (pInfo+i)->pSink,    
                    (pInfo+i)->pBindInfo,
                    PI_FORCE_ASYNC, 
                    0 
                );
                if(g_dwVerbose & FLAG_TRACE)
                    printf("MAIN:    pProtocol->Start: %lx\n", hr);
            }
        }
    }

    return(hr);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
VOID CleanInfoList(PINFO pInfo, DWORD dwNumDownloads)
{
    for(DWORD i = 0; i < dwNumDownloads; i++)
    {
        if((pInfo+i)->pProt)
            (pInfo+i)->pProt->Terminate(0);
        
        if( (pInfo+i)->pSink )
        {
            (pInfo+i)->pSink->Release();
        }
        if( (pInfo+i)->pBindInfo )
        {
            (pInfo+i)->pBindInfo->Release();
        }
        if( (pInfo+i)->pHook )
        {
            (pInfo+i)->pHook->Release();
        }
        
        // release COM objects
        if( (pInfo+i)->pProt )
        {
            //
            // BUG (POSSIBLE RESOURCE LEAK) 
            // If the pProt is IE's http/gopher/ftp implementation, 
            // calling pProt->Release() now might cause resource leak
            // since pProt (although finished the download), might
            // be still waiting wininet to call back about the 
            // confirmation of the handle closing.
            // The correct time to release pProt is to wait after 
            // pProtSink get destroyed.  
            //
            (pInfo+i)->pProt->Release();
        }
    }
}

//------------------------------------------------------------------------
//
//      Purpose:    create a session object
//                  Get a pluggable procotol from the session
//                  Start the pluggable protocol async download 
//
//      Author:     DanpoZ (Danpo Zhang)
//
//      History:    11-20-97    Created
//
//------------------------------------------------------------------------
int _cdecl main(int argc, char** argv) 
{
    IOInetSession* pSession = NULL;
    IOInetProtocol* pProt = NULL;
    HRESULT hr = NOERROR;
    DWORD dwLoadTime;
    HMODULE hMod = NULL;
    
    if(!ProcessCommandLine(argc, argv))
        return 0;
        
    // Init COM
    CoInitialize(NULL); 

	if(g_pModule != NULL)
	{
		hMod = LoadLibrary(g_pModule);
	}
	
    g_hCompleted = CreateEvent(NULL, FALSE, FALSE, NULL);
    
    // Get a Session
    hr = CoInternetGetSession(0, &pSession, 0);    
    if(g_dwVerbose & FLAG_TRACE)
        printf("MAIN:    Created Session: %lx\n", hr);

    if( hr == NOERROR )
    {
        if(!BuildInfoList(&Info[0], g_dwNumDownloads))
            return(2);

        hr = StartDownloads(pSession, &Info[0], g_dwNumDownloads);
    
        if( hr == NOERROR )
        {
            // wait until the async download finishes
            WaitForSingleObject(g_hCompleted, INFINITE);
        }

        StopCAP();
		QueryPerformanceCounter((LARGE_INTEGER *)&g_iend);
        QueryPerformanceFrequency((LARGE_INTEGER *)&g_ifreq);

        dwLoadTime = (LONG)(((g_iend - g_ibeg) * 1000) / g_ifreq);
        float fKB;
        float fSec;
        float fKBSec;

        if(dwLoadTime == 0)
            dwLoadTime = 1;
        fKB = ((float)g_dwTotalBytes)/1024;
        fSec = ((float)dwLoadTime)/1000;
        fKBSec = fKB / fSec;        
        printf("%s,%s,%d,%d,%2.0f\n", 
            g_pTitle ?g_pTitle :"Oinetperf",
            g_pRun ?g_pRun :"1",
            dwLoadTime, g_dwTotalBytes, fKBSec);

        CleanInfoList(&Info[0], g_dwNumDownloads);
    
    }


    if( pSession )
    {
        pSession->Release();
    }
    
    CoTaskMemFree(g_pwzUrl);

	if((g_pModule != NULL) && (hMod != NULL))
	{
		FreeLibrary(hMod);
	}

    // kill COM 
    CoUninitialize();

    return(0); 
}


COInetProtocolHook::COInetProtocolHook
(
    HANDLE g_hCompleted, 
    IOInetProtocol* pProt
)
{
    _hCompleted = g_hCompleted;
    _pProt = pProt;
}

COInetProtocolHook::~COInetProtocolHook() 
{
    CloseHandle(_hCompleted);
}

HRESULT
COInetProtocolHook::QueryInterface(REFIID iid, void **ppvObj)
{
    HRESULT hr = NOERROR;
    *ppvObj = NULL;

    if( iid == IID_IUnknown  || iid == IID_IOInetProtocolSink )
    {
        *ppvObj = static_cast<IOInetProtocolSink*>(this);
    } 
    else
    if( iid == IID_IOInetBindInfo )
    {
        *ppvObj = static_cast<IOInetBindInfo*>(this);
    }
    else
    if( iid == IID_IServiceProvider)
    {
        *ppvObj = static_cast<IServiceProvider*>(this);
    }
    else
    if( iid == IID_IHttpNegotiate )
    {
        *ppvObj = static_cast<IHttpNegotiate*>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    if( *ppvObj )
    {
        AddRef();
    }

    return hr;
}    

ULONG    
COInetProtocolHook::AddRef(void)
{
    LONG lRet = ++_CRefs;
    return lRet;
}

ULONG
COInetProtocolHook::Release(void)
{
    LONG lRet = --_CRefs;

    if (lRet == 0)
    {
        delete this;
    }

    return lRet;
}

HRESULT
COInetProtocolHook::Switch( PROTOCOLDATA *pStateInfo)
{
    printf("Are you crazy? I don't know how to do Thread switching!!!\n"); 
    return E_NOTIMPL;
}

HRESULT
COInetProtocolHook::ReportProgress( ULONG ulStatusCode, LPCWSTR szStatusText)
{
    switch( ulStatusCode )
    {
        case BINDSTATUS_FINDINGRESOURCE:
            if(g_dwVerbose & FLAG_TRACE)
                wprintf(
                    L"CALLBACK(ReportProgress): Resolving name %s\n", szStatusText );  
            break;

        case BINDSTATUS_CONNECTING:
        if(g_dwVerbose & FLAG_TRACE)
                wprintf(L"CALLBACK(ReportProgress): Connecting to %s\n", szStatusText );
            break;

        case BINDSTATUS_SENDINGREQUEST:
            if(g_dwVerbose & FLAG_TRACE)
                wprintf(L"CALLBACK(ReportProgress): Sending request\n");
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            if(g_dwVerbose & FLAG_TRACE)
                wprintf(L"CALLBACK(ReportProgress): cache filename available\n");
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            if(g_dwVerbose & FLAG_TRACE)
                wprintf(L"CALLBACK(ReportProgress): mimetype available = %s\n", szStatusText);
            break;

        case BINDSTATUS_REDIRECTING:
            if(g_dwVerbose & FLAG_TRACE)
                 wprintf(L"CALLBACK(ReportProgress): Redirecting to %s\n", szStatusText);
            break;

        default:
            if(g_dwVerbose & FLAG_TRACE)
                 wprintf(L"CALLBACK(ReportProgress): others...\n");
            break;
    }
    return NOERROR;
}

HRESULT 
COInetProtocolHook::ReportData( 
    DWORD grfBSCF, 
    ULONG ulProgress, 
    ULONG ulProgressMax
)
{
    if(g_dwVerbose & FLAG_TRACE)
        printf("CALLBACK(ReportData) %d, %d, %d \n", grfBSCF, ulProgress, ulProgressMax);

    // Pull data via pProt->Read(), here are the possible returned 
    // HRESULT values and how we should act upon: 
    // 
    // if E_PENDING is returned:  
    //    client already get all the data in buffer, there is nothing
    //    can be done here, client should walk away and wait for the  
    //    next chuck of data, which will be notified via ReportData()
    //    callback.
    // 
    // if S_FALSE is returned:
    //    this is EOF, everything is done, however, client must wait
    //    for ReportResult() callback to indicate that the pluggable 
    //    protocol is ready to shutdown.
    // 
    // if S_OK is returned:
    //    keep on reading, until you hit E_PENDING/S_FALSE/ERROR, the deal 
    //    is that the client is supposed to pull ALL the available
    //    data in the buffer
    // 
    // if none of the above is returning:
    //    Error occured, client should decide how to handle it, most
    //    commonly, client will call pProt->Abort() to abort the download
 
    char *pBuf = (char *)_alloca(dwBuf_Size);
    HRESULT hr = NOERROR;
    ULONG cbRead;

    while( hr == S_OK )       
    {
        cbRead = 0;

		if (g_ibeg == 0)
		{
			QueryPerformanceCounter((LARGE_INTEGER *)&g_ibeg);
			StartCAP();
		}


        // pull data

        if(g_dwVerbose & FLAG_TRACE)
        {
            printf("MAIN:    pProtocol->Read attempting %d bytes\n", dwBuf_Size);
        }

        hr = _pProt->Read((void*)pBuf, dwBuf_Size, &cbRead);
        if( (hr == S_OK || hr == E_PENDING || hr == S_FALSE) && cbRead )
        {
            if( g_dwVerbose & FLAG_DUMPDATA )
            {
                for( ULONG i = 0; i < cbRead; i++)
                {
                    printf("%c", pBuf[i]);    
                }    
            }

            if(g_dwVerbose & FLAG_TRACE)
            {
                printf("MAIN:    pProtocol->Read %d bytes\n", cbRead);
            }
            g_dwTotalBytes += cbRead;
        }
    }

    if( hr == S_FALSE )
    {
        if(g_dwVerbose & FLAG_TRACE)
            printf("MAIN:    pProtocol->Read returned EOF \n");
    }
    else 
    if( hr != E_PENDING )
    {
        if(g_dwVerbose & FLAG_TRACE)
        {
            printf("MAIN:    pProtocol->Read returned Error %1x \n, hr");
            printf("MAIN:    pProtocol->Abort called \n", hr);
        }
        _pProt->Abort(hr, 0);
    }

    return NOERROR;
}

HRESULT
COInetProtocolHook::ReportResult(
    HRESULT hrResult,
    DWORD   dwError,
    LPCWSTR wzResult
)
{
    // This is the last call back from the pluggable protocol, 
    // this call is equivlant to the IBindStatusCallBack::OnStopBinding()
    // it basically tells you that the pluggable protocol is ready
    // to shutdown

    if(g_dwVerbose & FLAG_TRACE)
        printf("CALLBACK(ReportResult): Download completed with status %1x\n", hrResult);

    // set event to the main thread
    if(InterlockedDecrement((LONG*)&g_dwDownloads ) == 0)
        SetEvent(g_hCompleted);
            
    return NOERROR;
}

HRESULT
COInetProtocolHook::GetBindInfo(
    DWORD *grfBINDF,
    BINDINFO * pbindinfo
)
{
    HRESULT hr = NOERROR;
    
//    *grfBINDF = BINDF_DIRECT_READ | BINDF_ASYNCHRONOUS | BINDF_PULLDATA;
//    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_IGNORESECURITYPROBLEM;
    *grfBINDF |= g_dwCacheFlag;

    // for HTTP GET,  VERB is the only field we interested
    // for HTTP POST, BINDINFO will point to Storage structure which 
    //                contains data
    BINDINFO bInfo;
    ZeroMemory(&bInfo, sizeof(BINDINFO));

    // all we need is size and verb field
    bInfo.cbSize = sizeof(BINDINFO);
    bInfo.dwBindVerb = BINDVERB_GET;

    // src -> dest 
    hr = CopyBindInfo(&bInfo, pbindinfo ); 

    return hr;
}


static LPSTR g_szAcceptStrAll = "*/*";
HRESULT
COInetProtocolHook::GetBindString(
    ULONG ulStringType,
    LPOLESTR *ppwzStr,
    ULONG cEl,
    ULONG *pcElFetched
)
{

    HRESULT hr = INET_E_USE_DEFAULT_SETTING;

    switch (ulStringType)
    {
    case BINDSTRING_HEADERS     :
    case BINDSTRING_EXTRA_URL   :
    case BINDSTRING_LANGUAGE    :
    case BINDSTRING_USERNAME    :
    case BINDSTRING_PASSWORD    :
    case BINDSTRING_ACCEPT_ENCODINGS:
    case BINDSTRING_URL:
    case BINDSTRING_USER_AGENT  :
    case BINDSTRING_POST_COOKIE :
    case BINDSTRING_POST_DATA_MIME:
        break;

    case BINDSTRING_ACCEPT_MIMES:
        // IE4's http pluggable protocol implementation does not 
        // honer INET_E_USE_DEFAULT_SETTING returned by this function 
        // starting from IE5, client can just return the USE_DEFAULT 
        
// use for ie5 so we don't need a seperate bin for ie4  // #ifndef IE5
        // this will be freed by the caller
        *(ppwzStr + 0) = MyDupA2W(g_szAcceptStrAll);
        *(ppwzStr + 1) = NULL;
        *pcElFetched = 1;
        
        hr = NOERROR;
//#endif

        break;

    default:
        break; 
    }

    return hr;
}


HRESULT
COInetProtocolHook::QueryService(
    REFGUID guidService,
    REFIID  riid,
    void    **ppvObj 
)
{
    HRESULT hr = E_NOINTERFACE;
    *ppvObj = NULL;
    if( guidService == IID_IHttpNegotiate )
    {
        *ppvObj = static_cast<IHttpNegotiate*>(this);
    }
   
    if( *ppvObj )
    {
        AddRef();
        hr = NOERROR;
    } 
    
    
    return hr;
}


HRESULT
COInetProtocolHook::BeginningTransaction(
    LPCWSTR szURL,
    LPCWSTR szHeaders,
    DWORD   dwReserved,
    LPWSTR  *pszAdditionalHeaders
)
{
    if(g_dwVerbose & FLAG_TRACE)
        printf("HTTPNEGOTIATE: Additional Headers? - No \n"); 
    *pszAdditionalHeaders = NULL;
    return NOERROR;
}

HRESULT
COInetProtocolHook::OnResponse(
    DWORD    dwResponseCode,
    LPCWSTR  szResponseHeaders,
    LPCWSTR  szRequestHeaders,
    LPWSTR   *pszAdditionalHeaders
)
{
    if(g_dwVerbose & FLAG_TRACE)
        printf("HTTPNEGOTIATE: Http server response code %d\n", dwResponseCode);
    return NOERROR;
}
