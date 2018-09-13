#undef UNICODE
#include "urlmon.h"
#include "wininet.h"
#include "commctrl.h"
#include "windows.h"
#include <stdio.h>
#include "initguid.h"
#include "hlink.h"
///#include "hlguids.h"

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

typedef BOOL (WINAPI *PFNSPA)(HANDLE, DWORD);
typedef HRESULT (WINAPI * pfnCreateURLMoniker)(IMoniker *, LPCWSTR, IMoniker **);
typedef HRESULT (WINAPI * pfnRegisterBindStatusCallback)(LPBC, IBindStatusCallback *, IBindStatusCallback **, DWORD);

typedef struct
{
    TCHAR*   pBuf;      //Actual buffer to hold data
    DWORD    lNumRead;  //number of bytes read in buffer
    void*    pNext;     //Pointer to next buffer
} buffer;


HINSTANCE g_hUrlMon = NULL;
pfnCreateURLMoniker g_pfnCreateURLMoniker = NULL;
pfnRegisterBindStatusCallback g_pfnRegisterBindStatusCallback = NULL;

#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)

#define DO_DOWNLOAD   WM_USER + 10
#define DOWNLOAD_DONE WM_USER + 11

#pragma warning(disable:4100)

// ---------------------------------------------------------------------------
#define DBG_ERROR            0x80000000

// verbose flags
#define DBG_RESULTS          0x01
#define DBG_DEBUG            0x02
#define DBG_INFO             0x04
#define DBG_STARTBINDING     0x08
#define DBG_STOPBINDING      0x10
#define DBG_ONPROGRESS       0x20
#define DBG_ONAVAIL          0x40
#define DBG_BREAKONERROR     0x80

#define DBG_ALLVALID         DBG_RESULTS | DBG_DEBUG | DBG_STARTBINDING | DBG_STOPBINDING | DBG_ONPROGRESS | DBG_ONAVAIL

DWORD g_dwDbgFlags = DBG_RESULTS;
// ---------------------------------------------------------------------------

const INT MAX_BUF_SIZE = 1024 * 16;
const INT BUF_SIZE = 2 * 1024;
const INT URL_MAX = 4;
const INT BUF_NUM = 16*4;
const DWORD TIMEOUT = 10000000;
const INT LDG_DONE = 1;
const INT LDG_STARTED = 0;
const INT PRI_LOW = 1;
const INT PRI_MED = 2;
const INT PRI_HI  = 3;

DWORD dwBegin_Time = 0;
DWORD dwEnd_Time;
DWORD dwTot_Time;
BOOL bDelim = FALSE;
DWORD dwNum_Opens = 1;
DWORD dwBuf_Size = BUF_SIZE;
DWORD dwBytes_Read = 0;
DWORD dwMax_Simul_Downloads = URL_MAX;
DWORD g_dwCacheFlag = BINDF_NOWRITECACHE | BINDF_GETNEWESTVERSION;
char *pFilename = NULL;
char *pInFile = NULL;
char *g_pRunStr = NULL;
char *g_pTestName = NULL;
char g_CmdLine[1024];
TCHAR sUrl[(INTERNET_MAX_URL_LENGTH+1)];
TCHAR* g_pBuf = NULL;

// %%Classes: ----------------------------------------------------------------

class CInfo 
{
public:
    CInfo();
    ~CInfo();
    INT      incDownloads(void) { return m_iDownloads++; }
    INT      decDownloads(void) { return m_iDownloads--; }
    INT      getDownloads(void) { return m_iDownloads; }

    HANDLE           m_hCompleteEvent;
    CRITICAL_SECTION m_csInfo;        //for critical section
    HANDLE           m_hMaxDownloadSem;
    buffer*          m_pPool;         //Pointer to current available buffer in pool
    void*            m_pdFirst;       //pointer to the first element
private:
    INT              m_iDownloads;    //number of current downloads
};

class CDownload 
{
  public:
    CDownload(LPSTR sName, CInfo* pcInfo);
    ~CDownload();
    HRESULT      doDownload(void);
    INT          getStatus(void) { return m_iStatus; }
    INT          getPriority(void) { return m_iPriority; }
#ifdef USE_POOL
    INT          releasePool(void);
#endif

    WCHAR                m_pUrl[(INTERNET_MAX_URL_LENGTH+1)];
#ifdef USE_POOL    
    buffer*              m_pbStartBuffer; //first buffer to hold data
    buffer*              m_pbCurBuffer;   //Current Buffer
#endif    
    CInfo*               m_pcInfo;  
    void*                m_pdNext;        //pointer to next element
    INT                  m_iStatus;       //the url's status
    INT                  m_iPriority;     //the url's priority
    DWORD                lNumRead;  //number of bytes read in buffer for this download
    
  private:
    IMoniker*            m_pMoniker;
    IBindCtx*            m_pBindCtx;
    IBindStatusCallback* m_pBindCallback;
};


class CBindStatusCallback : public IBindStatusCallback 
{
  public:
    // IUnknown methods
    STDMETHODIMP    QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef()    { return m_cRef++; }
    STDMETHODIMP_(ULONG)    Release()   { if (--m_cRef == 0) { delete this; return 0; } return m_cRef; }

    // IBindStatusCallback methods
    STDMETHODIMP    OnStartBinding(DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                        STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // constructors/destructors
    CBindStatusCallback(CDownload* pcDownload);
    ~CBindStatusCallback();

    
    // data members
    DWORD           m_cRef;
    IBinding*       m_pBinding;
    IStream*        m_pStream;
    DWORD           m_cbOld;
    CDownload*      m_pcDownload;
};


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
INT dprintf(DWORD dwFlags, TCHAR *fmt, ... ) 
{
    INT      ret = 0;
    va_list  marker;
    TCHAR     szBuffer[256];

    if(dwFlags & (g_dwDbgFlags | DBG_ERROR))
    {
        va_start( marker, fmt );
        ret = vsprintf( szBuffer, fmt, marker );
        OutputDebugString( szBuffer );
        printf(szBuffer);

        if(g_dwDbgFlags & DBG_BREAKONERROR)
            DebugBreak();
    }
    return ret; 
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
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


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
HRESULT LoadUrlMon()
{
    g_hUrlMon = (HINSTANCE)LoadLibraryA("URLMON.DLL");

    if (g_hUrlMon == NULL)
    {
        dprintf(DBG_ERROR, "LoadLibraryA of URLMON.DLL failed\n");
        return(E_FAIL);
    }

    g_pfnCreateURLMoniker = (pfnCreateURLMoniker)GetProcAddress(g_hUrlMon, "CreateURLMoniker");
    
    if (g_pfnCreateURLMoniker == NULL)
    {
        dprintf(DBG_ERROR, "GetProcAddress CreateURLMoniker failed\n");
        return(E_FAIL);
    }

    g_pfnRegisterBindStatusCallback = (pfnRegisterBindStatusCallback)GetProcAddress(g_hUrlMon, "RegisterBindStatusCallback");

    if (g_pfnRegisterBindStatusCallback == NULL)
    {
        dprintf(DBG_ERROR, "GetProcAddress RegisterBindStatusCallback failed\n");
        return(E_FAIL);
    }

    return(S_OK);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UnloadUrlMon()
{
    if (g_hUrlMon)
    {
        FreeLibrary(g_hUrlMon);
    }
}

// ===========================================================================
//                     CBindStatusCallback Implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::CBindStatusCallback
// ---------------------------------------------------------------------------
CBindStatusCallback::CBindStatusCallback(CDownload* pcDownload)
{
    m_pBinding = NULL;
    m_pStream = NULL;
    m_cRef = 1;
    m_cbOld = 0;
    m_pcDownload = pcDownload;
}  // CBindStatusCallback

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::~CBindStatusCallback
// ---------------------------------------------------------------------------
CBindStatusCallback::~CBindStatusCallback()
{
}  // ~CBindStatusCallback

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::QueryInterface
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IBindStatusCallback) 
    {
        *ppv = this;
        AddRef();
        return S_OK;    
    }
    return E_NOINTERFACE;
 }  // CBindStatusCallback::QueryInterface

 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::OnStartBinding
 // ---------------------------------------------------------------------------
 STDMETHODIMP CBindStatusCallback::OnStartBinding(DWORD dwReserved, IBinding* pBinding)
 {
    if (m_pBinding != NULL)
        m_pBinding->Release();

    m_pBinding = pBinding;

    if (m_pBinding != NULL) 
        m_pBinding->AddRef();

    m_pcDownload->m_pcInfo->incDownloads();

    if(g_dwDbgFlags)    
        dprintf(DBG_STOPBINDING, "OnStartBinding getDownloads()=%d\n", m_pcDownload->m_pcInfo->getDownloads());
    return S_OK;

 }  // CBindStatusCallback::OnStartBinding

 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::GetPriority
 // ---------------------------------------------------------------------------
 STDMETHODIMP CBindStatusCallback::GetPriority(LONG* pnPriority)
 {
     return E_NOTIMPL;
 }  // CBindStatusCallback::GetPriority

 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::OnLowResource
 // ---------------------------------------------------------------------------
 STDMETHODIMP CBindStatusCallback::OnLowResource(DWORD dwReserved)
 {
     return E_NOTIMPL;
 }  // CBindStatusCallback::OnLowResource

 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::OnProgress
 // ---------------------------------------------------------------------------
 
STDMETHODIMP CBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    TCHAR sz[255];
    if(szStatusText != NULL) {
         WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, sz, 255,0,0);
    }
    if(g_dwDbgFlags)    
        dprintf(DBG_ONPROGRESS, "OnProgress: %d(%s) %d of %d\n", ulStatusCode, sz, ulProgress, (ulProgress>ulProgressMax)?ulProgress:ulProgressMax);
    return(NOERROR);
}  // CBindStatusCallback::OnProgress

 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::OnStopBinding
 // ---------------------------------------------------------------------------
 STDMETHODIMP CBindStatusCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
 {
     if (hrStatus != S_OK) 
     {
        if(g_dwDbgFlags & DBG_DEBUG)
        {
             TCHAR sUrl[(INTERNET_MAX_URL_LENGTH+1)];
             TCHAR sErr[1024];
             WideCharToMultiByte(CP_ACP, 0, m_pcDownload->m_pUrl, -1, 
                 sUrl, INTERNET_MAX_URL_LENGTH, 0, 0);
             WideCharToMultiByte(CP_ACP, 0, pszError, -1, 
                sErr, 1024, 0, 0);
             dprintf(DBG_ERROR, "*** ERROR *** %s OnStopBinding download failed. Status=%x Err=%s\n", sUrl, hrStatus, sErr);
         }
     }
     if (m_pBinding)	
     {
         m_pBinding->Release();
         m_pBinding = NULL;
     }
     
     m_pcDownload->m_pcInfo->decDownloads();
     if(g_dwDbgFlags)    
        dprintf(DBG_STOPBINDING, "OnStopBinding hrStatus=%d getDownloads()=%d\n", hrStatus, m_pcDownload->m_pcInfo->getDownloads());

     if(m_pcDownload->m_pcInfo->getDownloads() == 0) 
     {
         SetEvent(m_pcDownload->m_pcInfo->m_hCompleteEvent);
     }         

     return S_OK;
 }  // CBindStatusCallback::OnStopBinding


 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::GetBindInfo
 // ---------------------------------------------------------------------------
 STDMETHODIMP CBindStatusCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pBindInfo)
 {
     *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
     *pgrfBINDF |= g_dwCacheFlag;
     pBindInfo->cbSize = sizeof(BINDINFO);
     pBindInfo->szExtraInfo = NULL;
     memset(&pBindInfo->stgmedData, 0, sizeof(STGMEDIUM));
     pBindInfo->grfBindInfoF = 0;
     pBindInfo->dwBindVerb = BINDVERB_GET;
     pBindInfo->szCustomVerb = NULL;
     return S_OK;
 }  // CBindStatusCallback::GetBindInfo

 // ---------------------------------------------------------------------------
 // %%Function: CBindStatusCallback::OnDataAvailable
 // ---------------------------------------------------------------------------
 
 
STDMETHODIMP CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pfmtetc, STGMEDIUM* pstgmed)
{
    DWORD dwRead = dwSize - m_cbOld; // Amount to be read
    HRESULT hr = S_OK;

     // Get the Stream passed

     if(g_dwDbgFlags)
        dprintf(DBG_ONAVAIL, "OnDataAvailable(grfBSCF=%d pStream=0x%x dwRead=%d dwSize=%d pfmtetc=0x%x, pstgmed=0x%x\n",
            grfBSCF, m_pStream, dwRead, dwSize, pfmtetc, pstgmed);
    
     if (!m_pStream && pstgmed->tymed == TYMED_ISTREAM)
     {
         m_pStream = pstgmed->pstm;
     }

     // If there is some data to be read then go ahead and read
     if (m_pStream && dwRead) 
     {
         while(hr!=E_PENDING) 
         {
#ifdef USE_POOL    
             if(m_pcDownload->m_pcInfo->m_pPool) 
             {
                 //if pool ready
                 EnterCriticalSection(&(m_pcDownload->m_pcInfo->m_csInfo));    
                 if(!m_pcDownload->m_pbStartBuffer) 
                 {    
                     // if the first time
                     m_pcDownload->m_pbStartBuffer = 
                         m_pcDownload->m_pbCurBuffer = 
                         m_pcDownload->m_pcInfo->m_pPool;
                     m_pcDownload->m_pcInfo->m_pPool = 
                         (buffer *)m_pcDownload->m_pcInfo->m_pPool->pNext;
                     m_pcDownload->m_pbStartBuffer->pNext = NULL;    
                 }
                 else 
                 {
                     m_pcDownload->m_pbCurBuffer->pNext = 
                         m_pcDownload->m_pcInfo->m_pPool;
                     m_pcDownload->m_pcInfo->m_pPool =
                     (buffer *)m_pcDownload->m_pcInfo->m_pPool->pNext;
                     m_pcDownload->m_pbCurBuffer = (buffer *) m_pcDownload->m_pbCurBuffer->pNext;
                     m_pcDownload->m_pbCurBuffer->pNext = NULL;
                 }   
                 LeaveCriticalSection(&(m_pcDownload->m_pcInfo->m_csInfo));   
             }    
             else 
             {
                 //allocate buffers on the fly
                 if(!m_pcDownload->m_pbStartBuffer) 
                 {    
                     // if the first time
                     m_pcDownload->m_pbStartBuffer = m_pcDownload->m_pbCurBuffer =  new buffer;
                     if(!m_pcDownload->m_pbCurBuffer)
                     {
                         dprintf(DBG_ERROR, "*** ERROR *** on buff alloc\n");
                         return S_FALSE;
                     }
                     m_pcDownload->m_pbCurBuffer->pBuf = new TCHAR[dwBuf_Size];

                     if(!m_pcDownload->m_pbCurBuffer->pBuf)
                     {
                         dprintf(DBG_ERROR, "*** ERROR *** on buf alloc\n");
                         return S_FALSE;
                     }
                 
                     m_pcDownload->m_pbStartBuffer->pNext = NULL;        
                 }
             
                 else 
                 {    
                     m_pcDownload->m_pbCurBuffer->pNext = new buffer;
                     if(!m_pcDownload->m_pbCurBuffer->pNext)
                     {
                         dprintf(DBG_ERROR, "*** ERROR *** on buff alloc\n");
                         return S_FALSE;
                     }
                     m_pcDownload->m_pbCurBuffer = (buffer *) m_pcDownload->m_pbCurBuffer->pNext;
                     m_pcDownload->m_pbCurBuffer->pBuf = new TCHAR[dwBuf_Size];
                     if(!m_pcDownload->m_pbCurBuffer->pBuf)
                     {
                         dprintf(DBG_ERROR, "*** ERROR *** on buf alloc\n");
                         return S_FALSE;
                     }
                 
                     m_pcDownload->m_pbCurBuffer->pNext = NULL;
                 }
             }
#endif
             if(dwBegin_Time == 0)
                 dwBegin_Time = GetTickCount();

#ifdef USE_POOL    
             hr = m_pStream->Read(m_pcDownload->m_pbCurBuffer->pBuf,             
                 dwBuf_Size, &(m_pcDownload->m_pbCurBuffer->lNumRead));
             if(g_dwDbgFlags)
             {
                  dprintf(DBG_INFO & DBG_DEBUG, "Stream->Read Size=%d Read=%d hr=0x%x\n", dwBuf_Size, m_pcDownload->m_pbCurBuffer->lNumRead, hr);
                  if(hr != S_OK && hr != E_PENDING && hr != S_FALSE)
                    dprintf(DBG_ERROR, "************ Stream->Read hr=0x%x\n", hr);
             }
#else                 
             hr = m_pStream->Read(g_pBuf, dwBuf_Size, &(m_pcDownload->lNumRead));
             if(g_dwDbgFlags)
             {
                  dprintf(DBG_INFO & DBG_DEBUG, "Stream->Read Size=%d Read=%d hr=0x%x\n", dwBuf_Size, m_pcDownload->lNumRead, hr);
                  if(hr != S_OK && hr != E_PENDING && hr != S_FALSE)
                    dprintf(DBG_ERROR, "************ Stream->Read hr=0x%x\n", hr);
             }
#endif                 
             
             //need to check for error if read reaches end of stream
             if(hr == S_FALSE) 
             {
                 break;            
             }
#ifdef USE_POOL    
             if (m_pcDownload->m_pbCurBuffer->lNumRead > 0) 
             {
                 m_cbOld += m_pcDownload->m_pbCurBuffer->lNumRead;
             }
#else
             if (m_pcDownload->lNumRead > 0) 
             {
                 m_cbOld += m_pcDownload->lNumRead;
             }
#endif
         }
     }//     if(m_pstm && dwRead)

     if (BSCF_LASTDATANOTIFICATION & grfBSCF) 
     {
         WideCharToMultiByte(CP_ACP, 0, m_pcDownload->m_pUrl, -1, 
             sUrl, INTERNET_MAX_URL_LENGTH, 0, 0);
         if(g_dwDbgFlags && !bDelim)
            dprintf(DBG_INFO, "Status: %s downloaded.\n", sUrl);
//         m_pcDownload->m_pcInfo->decDownloads();
    
         m_pcDownload->m_iStatus = LDG_DONE;

         if(!ReleaseSemaphore(m_pcDownload->m_pcInfo->m_hMaxDownloadSem,1,NULL)) 
         {
             dprintf(DBG_ERROR, "*** ERROR *** ReleaseSemaphore failed!\n");
             return S_FALSE; 
         }

         dwBytes_Read += m_cbOld;  // accum buf size that was downloaded
     }
     return S_OK;
}  // CBindStatusCallback::OnDataAvailable

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnObjectAvailable
// ---------------------------------------------------------------------------
 STDMETHODIMP
CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    return E_NOTIMPL;
}  // CBindStatusCallback::OnObjectAvailable

// ===========================================================================
//                           CDownload Implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// %%Function: CDownload::CDownload
// ---------------------------------------------------------------------------
CDownload::CDownload(LPSTR sName, CInfo* pcInfo)
{
    MultiByteToWideChar(CP_ACP, 0, sName, -1, m_pUrl, INTERNET_MAX_URL_LENGTH);  
    m_pMoniker = 0;
    m_pBindCtx = 0;
    m_pBindCallback = 0;
    m_pdNext = NULL;

    m_iStatus = LDG_STARTED;
    m_iPriority = PRI_MED;
    m_pcInfo = pcInfo;
#ifdef USE_POOL    
    m_pbStartBuffer = m_pbCurBuffer = NULL;
#endif    

}  // CDownload

// ---------------------------------------------------------------------------
// %%Function: CDownload::~CDownload
// ---------------------------------------------------------------------------
CDownload::~CDownload()
{
    buffer* pbLastBuf = NULL;

    if (m_pMoniker)
        m_pMoniker->Release();
    if (m_pBindCtx)
        m_pBindCtx->Release();
    if (m_pBindCallback)
        m_pBindCallback->Release();
    delete m_pcInfo;

#ifdef USE_POOL    
    if(m_pbStartBuffer)
    {
        while(m_pbStartBuffer->lNumRead != 0 && 
            m_pbStartBuffer->lNumRead <= dwBuf_Size) 
        {
            delete m_pbStartBuffer->pBuf;
            pbLastBuf = m_pbStartBuffer;
            m_pbStartBuffer = (buffer *)m_pbStartBuffer->pNext;   
            delete pbLastBuf;   
        }   
    }
#endif
    GlobalFree(m_pUrl);
}  // ~CDownload

// ---------------------------------------------------------------------------
// %%Function: CDownload::DoDownload
// ---------------------------------------------------------------------------
 

HRESULT CDownload::doDownload(void) 
{
    IStream*        pstm;
    HRESULT         hr;

    hr = g_pfnCreateURLMoniker(NULL, m_pUrl, &m_pMoniker);
    if (FAILED(hr)) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** doDownload CreateURLMoniker failed hr=0x%x\n", hr);
        goto LErrExit;    
    }
            
    m_pBindCallback = new CBindStatusCallback(this);

    if (m_pBindCallback == NULL) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** doDownload CBindStatusCallback failed hr=0x%x\n", hr);
        hr = E_OUTOFMEMORY;
        goto LErrExit;        
    }

    hr = CreateBindCtx(0, &m_pBindCtx);
    if (FAILED(hr)) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** doDownload CreateBindCtx failed hr=0x%x\n", hr);
        goto LErrExit;      
    }

    hr = g_pfnRegisterBindStatusCallback(
        m_pBindCtx, 
        m_pBindCallback, 
        0, 0L);
        
    if (FAILED(hr)) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** doDownload RegisterBindStatusCallback failed hr=0x%x\n", hr);
        goto LErrExit;    
    }
    
    hr = m_pMoniker->BindToStorage(
        m_pBindCtx, 
        0, 
        IID_IStream, 
        (void**)&pstm);
    if (FAILED(hr)) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** doDownload BindToStorage failed hr=0x%x\n", hr);
        goto LErrExit;    
    }
    
    return(hr);

LErrExit:
    if (m_pBindCtx != NULL) 
    {
        m_pBindCtx->Release();
        m_pBindCtx = NULL;
        }
    if (m_pBindCallback != NULL) 
    {
        m_pBindCallback->Release();
        m_pBindCallback = NULL;
        }
    if (m_pMoniker != NULL) 
    {
        m_pMoniker->Release();
        m_pMoniker = NULL;
        }
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CDownload::releasePool
// ---------------------------------------------------------------------------
#ifdef USE_POOL
INT CDownload::releasePool()
{
    buffer *pbStart;

    EnterCriticalSection(&(m_pcInfo->m_csInfo));

    while(m_pbStartBuffer) 
    {
        // remember the start buf
        pbStart = (buffer *) m_pbStartBuffer->pNext;
        // adjust the start
        m_pbStartBuffer = (buffer *) m_pbStartBuffer->pNext;

        //insert the buffer at the beginning of the pool
        pbStart->pNext = m_pcInfo->m_pPool;

        // update the pool
        m_pcInfo->m_pPool = pbStart;
    }

    LeaveCriticalSection(&(m_pcInfo->m_csInfo));
    return TRUE;
}
#endif

// ===========================================================================
//                           CInfo Implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// %%Function: CInfo::CInfo
// ---------------------------------------------------------------------------
CInfo::CInfo()
{
#ifdef USE_POOL    
    INT i;

    buffer* pStartBuffer = NULL;
#endif

    m_hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!m_hCompleteEvent) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** on create Event!\n");
    }

    InitializeCriticalSection(&(m_csInfo));

    m_hMaxDownloadSem = CreateSemaphore(NULL,dwMax_Simul_Downloads,dwMax_Simul_Downloads, NULL);
    if(!m_hMaxDownloadSem) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** CreateSem failed!\n");
    }

#ifdef USE_POOL    
    pStartBuffer = m_pPool = new buffer; 
    if(!m_pPool)
        return;

    m_pPool->pBuf = new TCHAR[dwBuf_Size]; 
    if (!m_pPool->pBuf)
        return;

    m_pPool->lNumRead = 0;
#endif    

    m_iDownloads = 0;

#ifdef USE_POOL    
    m_pPool->pNext = NULL;
    for(i=1; i<BUF_NUM; i++) 
    {
        m_pPool->pNext = new buffer;
        if (!m_pPool->pNext)
            return;

        m_pPool = (buffer *)m_pPool->pNext;
        m_pPool->pBuf = new TCHAR[dwBuf_Size];
        
        if (!m_pPool->pBuf)
            return;

        m_pPool->lNumRead = 0;
        m_pPool->pNext = NULL;
    }

    m_pPool = pStartBuffer;
#endif    
    return;
}  // CInfo

// ---------------------------------------------------------------------------
// %%Function: CInfo::~CInfo
// ---------------------------------------------------------------------------
CInfo::~CInfo()
{
    buffer *pLastBuf;

    while(m_pPool) 
    {
        delete m_pPool->pBuf;
        pLastBuf = m_pPool;
        m_pPool = (buffer *)m_pPool->pNext;   
        delete pLastBuf;
    }
    delete this;
}  // ~CInfo



// ===========================================================================
//                  User Interface and Initialization Routines
// ===========================================================================

//----------------------------------------------------------------------------
//  Procedure:   DownloadThread
//  Purpose:     Opens internet connection and downloads URL.  Saves
//               URL to pOutQ (one chunk per buffer).
//  Arguments:   outQ
//  Return Val:  TRUE or FALSE based on error
//----------------------------------------------------------------------------

DWORD DownloadThread(LPDWORD lpdwParam) 
{

    INT retVal;
    MSG msg;
    CDownload *pcDownload = (CDownload *) lpdwParam;

    SetEvent(pcDownload->m_pcInfo->m_hCompleteEvent);
    if(g_dwDbgFlags)    
        dprintf(DBG_INFO, "DownloadThread: m_hCompleteEvent set.\n");

    StartCAP();

    for (;;)
    {
        SuspendCAP();

        retVal = GetMessage(&msg, NULL, 0, 0);

        ResumeCAP();

        if(retVal == -1) 
        {
            dprintf(DBG_ERROR, "*** ERROR *** on GetMessage\n");
            break;
        }
        if(retVal == FALSE) 
        {
            msg.message = DOWNLOAD_DONE;
        }
        pcDownload = (CDownload *) msg.wParam;
        switch(msg.message) 
        {
        case DOWNLOAD_DONE:
            delete pcDownload;
            if(g_dwDbgFlags)    
                dprintf(DBG_INFO, "DownloadThread: exit\n");
            return TRUE;
            break;
        
        case DO_DOWNLOAD:
            if(FAILED(pcDownload->doDownload()))
            {
                return FALSE;
            }
            break;
        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }    
    return TRUE;
}

//==================================================================
void Display_Usage(char **argv)
{
    printf("\nUsage: %s -fURLname [options]\n", argv[0]);
    printf("\n          -iInputFileName [options]\n");
    printf("\n\t options:\n");
    printf("\t\t -l   - read buffer length\n");
    printf("\t\t -m   - maximum number of simultaneous downloads\n");
    printf("\t\t -n## - number of times to download\n");
    printf("\t\t -z   - comma delimited format\n");
    printf("\t\t -c   - write to cache (default is NOWRITECACHE)\n");
    printf("\t\t -g   - read from cache (default is GETNEWESTVERSION)\n");
    printf("\t\t -d   - direct read (default uses QueryDataAvailable)\n");
    printf("\t\t -1   - single processor affinity (default multiprocessor)\n");
    printf("\t\t -x#  - verbose flags (default=0x%x)\n", g_dwDbgFlags);
    printf("\t\t\t Results          0x%02x\n",DBG_RESULTS);
    printf("\t\t\t Debug            0x%02x\n",DBG_DEBUG);
    printf("\t\t\t Info             0x%02x\n",DBG_INFO);
    printf("\t\t\t StartBinding     0x%02x\n",DBG_STARTBINDING);
    printf("\t\t\t StopBinding      0x%02x\n",DBG_STOPBINDING);
    printf("\t\t\t OnProgress       0x%02x\n",DBG_ONPROGRESS);
    printf("\t\t\t OnDataAvailable  0x%02x\n",DBG_ONAVAIL);
    printf("\t\t\t Break on Errors  0x%02x\n",DBG_BREAKONERROR);
}

//==================================================================
BOOL Process_Command_Line(int argcIn, char **argvIn)
{
    BOOL bRC = TRUE;
    int argc = argcIn;
    char **argv = argvIn;
    DWORD dwLen = 0;

    *g_CmdLine = '\0';
    
    argv++; argc--;
    while( argc > 0 && argv[0][0] == '-' )  
    {
        switch (argv[0][1]) 
        {
            case 'c':
                g_dwCacheFlag &= ~BINDF_NOWRITECACHE;
                break;
            case 'g':
                g_dwCacheFlag &= ~BINDF_GETNEWESTVERSION;
                break;
            case 'd':
                g_dwCacheFlag |= BINDF_DIRECT_READ;
                break;
            case 'f':
                pFilename = &argv[0][2];
                break;
            case 'i':
                pInFile = &argv[0][2];
                break;
            case 'n':
                dwNum_Opens = atoi(&argv[0][2]);
                break;
            case 'l':
                dwBuf_Size =  atoi(&argv[0][2]);
                if(dwBuf_Size > MAX_BUF_SIZE)
                    dwBuf_Size = MAX_BUF_SIZE;
                break;
            case 'm':
                dwMax_Simul_Downloads = atoi(&argv[0][2]);
                break;
            case 'r':
                g_pRunStr = &argv[0][2];
                break;
            case 't':
                g_pTestName = &argv[0][2];
                break;
            case 'z':
                bDelim = TRUE;
                break;
            case '1':
                SetSingleProcessorAffinity();
                break;
            case 'x':
                sscanf(&argv[0][2], "%x", &g_dwDbgFlags);
                if(!(g_dwDbgFlags & (DBG_ALLVALID)))
                {
                    printf("Invalid verbose flags %x\n", g_dwDbgFlags);
                    Display_Usage(argvIn);
                    bRC = FALSE;
                }
                break;
            default:
                Display_Usage(argvIn);
                bRC = FALSE;
        }
        if(bRC)
        {
            dwLen += lstrlen(argv[0]) + 1;   // length of arg and space
            if(dwLen < ((sizeof(g_CmdLine)/sizeof(g_CmdLine[0]))-1))
            {
                lstrcat(g_CmdLine, ",");
                lstrcat(g_CmdLine, argv[0]);
            }
        }
        
        argv++; argc--;
    }

    if(!pFilename && !pInFile)
    {
        Display_Usage(argvIn);
        bRC = FALSE;
    }

    return(bRC);
}

//----------------------------------------------------------------------------
// Function:  WinMain
// Purpose:   main entry procedure
// Args:      none
// RetVal:    TRUE or FALSE based on error
//----------------------------------------------------------------------------
//int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR szCmdLine, int nCmdShow)
int __cdecl main(INT argc, TCHAR *argv[]) //for console
{
    CDownload*      pcDownload = NULL;
    CDownload*      pcdFirst = NULL;
    CInfo*          pcInfo = NULL;
    DWORD           dwThreadID;
    DWORD           dwCnt;
    HANDLE          hDownloadThread;
    INT             iError;
    char            szName[MAX_PATH];
    __int64         ibeg, iend, ifrq;
    float fKB;
    float fSec;
    float fKBSec;

    if(!Process_Command_Line(argc, argv))
        exit(0);

    pcInfo = new CInfo();
    g_pBuf = new TCHAR[dwBuf_Size];

    if(!pcInfo) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** generating pool!\n");
        return(0);
    }

    dwCnt = 0;
    if(pFilename)
    {
        while(dwCnt++ < dwNum_Opens)
        {    

            if(g_dwCacheFlag & BINDF_NOWRITECACHE)
                lstrcpy(szName, pFilename);
            else
                wsprintf(szName, "%s.%d", pFilename, dwCnt);

            if(!pcDownload) 
            {
                pcdFirst = pcDownload = new CDownload(szName, pcInfo);
                pcDownload->m_pcInfo->m_pdFirst = pcDownload;
            }
            else 
            {
                pcDownload->m_pdNext = new CDownload(szName, pcInfo);
                pcDownload = (CDownload *) pcDownload->m_pdNext;
            }

            if(!pcDownload)
            {
                dprintf(DBG_ERROR, "*** ERROR *** initializing pcDownload!\n");
                return(0);
            }
        }
    }
    else if(pInFile)    // Process input file
    {
        FILE *fp;

        while(dwCnt++ < dwNum_Opens) 
        {
            if((fp = fopen(pInFile, "r")) == NULL) 
            {
                dprintf(DBG_ERROR, "*** ERROR *** opening file\n");
                return(0);
            }

            while(fgets(szName, INTERNET_MAX_URL_LENGTH, fp) != NULL) 
            {
                if(szName[0] != '#') 
                {
                    szName[strlen(szName) - sizeof(char)] = '\0';

                    if(!pcDownload) 
                    {
                        pcdFirst = pcDownload = new CDownload(szName, pcInfo);
                        pcDownload->m_pcInfo->m_pdFirst = pcDownload;
                    }
                    else 
                    {
                        pcDownload->m_pdNext = new CDownload(szName, pcInfo);
                        pcDownload = (CDownload *) pcDownload->m_pdNext;
                    }

                    if(!pcDownload)
                    {
                        dprintf(DBG_ERROR, "*** ERROR *** initializing pcDownload!\n");
                        return(0);
                    }
                }
            }

            fclose(fp);
        }
    }

    pcDownload = (CDownload *) pcDownload->m_pcInfo->m_pdFirst;

    if (LoadUrlMon() != S_OK)
    {
        dprintf(DBG_ERROR, "*** ERROR *** LoadUrlMon() failed\n");
        return(0);
    }

    if (CoInitialize(NULL) != S_OK) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** CoInitialize() failed\n");
        return(0);
    }

    pcDownload->m_pcInfo->m_hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!pcDownload->m_pcInfo->m_hCompleteEvent) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** on create Event!\n");
    }

    hDownloadThread = CreateThread(NULL,
        0,
        (LPTHREAD_START_ROUTINE)DownloadThread,
        (LPVOID)pcDownload,
        0,
        &dwThreadID );

    if (!hDownloadThread) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** Could not create Thread\n");
        return(0);
    }

    if(WaitForSingleObject(pcDownload->m_pcInfo->m_hCompleteEvent, TIMEOUT)
        == WAIT_TIMEOUT) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** timeout on init\n");
    }
    Sleep(100);

    QueryPerformanceCounter((LARGE_INTEGER *)&ibeg);

    while(pcDownload) 
    {
        if(WaitForSingleObject(pcDownload->m_pcInfo->m_hMaxDownloadSem, TIMEOUT) 
            == WAIT_TIMEOUT) 
        {
            dprintf(DBG_ERROR, "*** ERROR *** timeout on Sem\n");
        }

        if(g_dwDbgFlags)
        {
            TCHAR sz[255];
            WideCharToMultiByte(CP_ACP, 0, pcDownload->m_pUrl, -1, sz, 255,0,0);
            dprintf(DBG_INFO, "main: PostThreadMessage DO_DOWNLOAD %s\n", sz);
        }
        if(!PostThreadMessage(dwThreadID, DO_DOWNLOAD, (WPARAM) pcDownload, 0))
        {
            iError = GetLastError();
            dprintf(DBG_ERROR, "*** Error *** on PostThreadMessage(0x%X, %ld, 0x%lX, 0) [GLE=%d]\n", 
                dwThreadID, DO_DOWNLOAD, pcDownload, iError);
            return(0);
        }
        pcDownload = (CDownload *) pcDownload->m_pdNext;    
    }  
    //wait for completion downloads at one time
    if(WaitForSingleObject(pcdFirst->m_pcInfo->m_hCompleteEvent, TIMEOUT) == WAIT_TIMEOUT) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** timeout on Sem\n");
    }

    QueryPerformanceCounter((LARGE_INTEGER *) &iend);
    QueryPerformanceFrequency((LARGE_INTEGER *) &ifrq);

    dwTot_Time = (DWORD)((iend - ibeg) * 1000 / ifrq);
    if(dwTot_Time == 0)
     dwTot_Time = 1;
    fKB = ((float)dwBytes_Read)/1024;
    fSec = ((float)dwTot_Time)/1000;
    fKBSec = fKB / fSec;
    if(!bDelim)
    {
        dprintf(DBG_RESULTS, "Downloaded: %s\r\n", sUrl);
        dprintf(DBG_RESULTS, "%ld Bytes in %ld Milliseconds = %2.0f KB/Sec\r\n", dwBytes_Read, dwTot_Time, fKBSec );
        dprintf(DBG_RESULTS, "%ld Reads, %ld Downloads, %ld Byte Read Buffer\r\n",
            dwNum_Opens, dwMax_Simul_Downloads, dwBuf_Size);
    }
    else
        dprintf(DBG_RESULTS, "%s, %s, %ld, %ld, %2.0f %s\n", 
            g_pTestName ?g_pTestName :"urlmon",
            g_pRunStr ?g_pRunStr :"1",
            dwTot_Time, dwBytes_Read, fKBSec, g_CmdLine );

    if(g_dwDbgFlags)    
        dprintf(DBG_INFO, "realized finished on data ready\n");

    if(!PostThreadMessage(dwThreadID, DOWNLOAD_DONE, (WPARAM) pcDownload, 0))
    {
        iError = GetLastError();
        dprintf(DBG_ERROR, "*** Error *** on PostThreadMessage(0x%X, %ld, 0x%lX, 0) [GLE=%d]\n", 
            dwThreadID, DOWNLOAD_DONE, pcDownload, iError);
        return(0);
    }
    if(WaitForSingleObject(hDownloadThread, TIMEOUT) == WAIT_TIMEOUT) 
    {
        dprintf(DBG_ERROR, "*** ERROR *** timeout on DownloadThread exit\n");
    }
   
    CloseHandle(hDownloadThread);
    CoUninitialize();
    UnloadUrlMon();

    if(g_dwDbgFlags)    
        dprintf(DBG_INFO, "main: exit\n");

    return(1);
} 

